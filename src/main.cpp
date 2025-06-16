#include <Arduino.h>
#include "menu_system.h"
#include "ble_scanner.h"
#include "mqtt_client.h"
#include "settings_manager.h"

void setup() {
  Serial.begin(115200);
  SettingsManager::begin();
  MenuSystem::begin();
  BLEScanner::begin();
  MQTTClient::begin();
}

void loop() {
  MenuSystem::update();
  BLEScanner::update();
  MQTTClient::update();
}