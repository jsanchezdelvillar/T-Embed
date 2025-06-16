#include "ble_scanner.h"
#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEScan.h>
#include "mqtt_client.h"

BLEScan* pBLEScan;
std::vector<String> discoveredDevices;

void BLEScanner::begin() {
  BLEDevice::init("");
  pBLEScan = BLEDevice::getScan();
  pBLEScan->setActiveScan(true);
}

class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    String mac = advertisedDevice.getAddress().toString().c_str();
    int rssi = advertisedDevice.getRSSI();
    String payload = "{\"mac\":\"" + mac + "\",\"rssi\":" + String(rssi) + "}";
    MQTTClient::publish("t_embed/ble/device", payload.c_str());
    discoveredDevices.push_back(mac + " RSSI: " + String(rssi));
  }
};

void BLEScanner::update() {
  static MyAdvertisedDeviceCallbacks callbacks;
  pBLEScan->setAdvertisedDeviceCallbacks(&callbacks);
  pBLEScan->start(2, false);
  pBLEScan->clearResults();
}

const std::vector<String>& BLEScanner::getDevices() {
  return discoveredDevices;
}

void BLEScanner::clearDevices() {
  discoveredDevices.clear();
}