#include "menu_system.h"
#include "settings_manager.h"
#include "ble_scanner.h"
#include "mqtt_client.h"
#include <TFT_eSPI.h>
#include <RotaryEncoder.h>
#include <vector>

// --- Watering device list (hardcoded for now) ---
const char* wateringDevices[] = { "Front Lawn", "Back Garden" };
const int wateringDeviceCount = sizeof(wateringDevices) / sizeof(wateringDevices[0]);

// Forward declarations
void buildMenu();
void drawMenu();
void selectItem();
void goBack();
void showBLEDevices();
void showHAMenu();
void buildRiegoMenu();
void buildWateringDeviceMenu(int index);

// Menu system
TFT_eSPI tft = TFT_eSPI();
RotaryEncoder encoder(34, 35, RotaryEncoder::LatchMode::TWO03);
int lastPos = -1;
bool buttonPressed = false;
const int BUTTON_PIN = 0;

struct MenuItem {
  String label;
  std::vector<MenuItem> subitems;
  std::function<void()> action;
};

std::vector<MenuItem> currentMenu;
std::vector<std::vector<MenuItem>> menuStack;
int selectedIndex = 0;

// --- Watering Device Actions ---
void startWatering(int deviceIdx) {
  String topic = String("ha/riego/") + wateringDevices[deviceIdx] + "/set";
  MQTTClient::publish(topic.c_str(), "start");
  tft.fillScreen(TFT_BLACK);
  tft.drawString("Started!", 10, 20, 2);
  delay(1000);
  goBack();
}

void delayWatering(int deviceIdx) {
  String topic = String("ha/riego/") + wateringDevices[deviceIdx] + "/set";
  MQTTClient::publish(topic.c_str(), "delay");
  tft.fillScreen(TFT_BLACK);
  tft.drawString("Delayed 1 day!", 10, 20, 2);
  delay(1000);
  goBack();
}

// --- Submenu builders ---
void buildWateringDeviceMenu(int deviceIdx) {
  MenuItem backOption = { "< Back", {}, goBack };
  MenuItem startOption = { "Start", {}, [deviceIdx]() { startWatering(deviceIdx); } };
  MenuItem delayOption = { "Delay 1 day", {}, [deviceIdx]() { delayWatering(deviceIdx); } };
  std::vector<MenuItem> deviceMenu = { startOption, delayOption, backOption };
  menuStack.push_back(currentMenu);
  currentMenu = deviceMenu;
  selectedIndex = 0;
  drawMenu();
}

void buildRiegoMenu() {
  MenuItem backOption = { "< Back", {}, goBack };
  std::vector<MenuItem> riegoMenu;
  for (int i = 0; i < wateringDeviceCount; ++i) {
    riegoMenu.push_back({ wateringDevices[i], {}, [i]() { buildWateringDeviceMenu(i); } });
  }
  riegoMenu.push_back(backOption);
  menuStack.push_back(currentMenu);
  currentMenu = riegoMenu;
  selectedIndex = 0;
  drawMenu();
}

// --- BLE Devices menu ---
void showBLEDevices() {
  tft.fillScreen(TFT_BLACK);
  const auto& devices = BLEScanner::getDevices();
  int y = 20;
  for (const auto& dev : devices) {
    tft.drawString(dev, 10, y, 2);
    y += 20;
    if (y > 220) break;
  }
  delay(2000); // Show for 2 seconds, then go back
  goBack();
}

// --- HA menu (currently just redraw) ---
void showHAMenu() {
  drawMenu();
}

// --- Menu navigation ---
void drawMenu() {
  tft.fillScreen(TFT_BLACK);
  for (int i = 0; i < currentMenu.size(); ++i) {
    if (i == selectedIndex) {
      tft.setTextColor(TFT_BLACK, TFT_YELLOW);
    } else {
      tft.setTextColor(TFT_WHITE, TFT_BLACK);
    }
    tft.drawString(currentMenu[i].label, 10, 20 + i * 20, 2);
  }
}

void selectItem() {
  auto item = currentMenu[selectedIndex];
  if (!item.subitems.empty()) {
    menuStack.push_back(currentMenu);
    currentMenu = item.subitems;
    selectedIndex = 0;
    drawMenu();
  } else if (item.action) {
    item.action();
  }
}

void goBack() {
  if (!menuStack.empty()) {
    currentMenu = menuStack.back();
    menuStack.pop_back();
    selectedIndex = 0;
    drawMenu();
  }
}

// --- Root menu builder ---
void buildMenu() {
  MenuItem backOption = { "< Back", {}, goBack };

  // Riego menu (dynamic)
  MenuItem haRiego = { "Riego", {backOption}, buildRiegoMenu };
  MenuItem haMenu = { "HA", { haRiego, backOption }, showHAMenu };

  // Settings submenu
  MenuItem wifiSettings = { "WiFi", {backOption}, []() {
    tft.fillScreen(TFT_BLACK);
    tft.drawString("WiFi config coming", 10, 20, 2);
    delay(2000);
    goBack();
  }};
  MenuItem mqttSettings = { "MQTT", {backOption}, []() {
    tft.fillScreen(TFT_BLACK);
    tft.drawString("MQTT config coming", 10, 20, 2);
    delay(2000);
    goBack();
  }};
  MenuItem settings = {
    "Settings",
    {
      wifiSettings,
      mqttSettings,
      backOption
    }
  };

  MenuItem bleStatus = { "BLE Devices", {backOption}, showBLEDevices };

  MenuItem root = {
    "Main Menu",
    {
      bleStatus,
      settings,
      haMenu
    }
  };

  currentMenu = root.subitems;
}

// --- Button interrupt handler ---
void IRAM_ATTR handleButton() {
  buttonPressed = true;
}

// --- Public API ---
void MenuSystem::begin() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  attachInterrupt(BUTTON_PIN, handleButton, FALLING);
  encoder.setPosition(0);
  tft.init();
  tft.setRotation(1);
  tft.setTextSize(2);
  tft.fillScreen(TFT_BLACK);

  buildMenu();
  drawMenu();
}

void MenuSystem::update() {
  encoder.tick();
  int newPos = encoder.getPosition();
  if (newPos != lastPos) {
    lastPos = newPos;
    selectedIndex = newPos % currentMenu.size();
    if (selectedIndex < 0) selectedIndex += currentMenu.size();
    drawMenu();
  }

  if (buttonPressed) {
    buttonPressed = false;
    selectItem();
  }
}
