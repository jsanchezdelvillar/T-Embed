#include "menu_system.h"
#include "settings_manager.h"
#include "ble_scanner.h"
#include "mqtt_client.h"
#include "config.h"
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
  tft.fillRect(0, 0, 320, 34, TFT_DARKGREEN);
  tft.setTextColor(TFT_WHITE, TFT_DARKGREEN);
  tft.setTextDatum(TC_DATUM);
  tft.drawString("Started!", 160, 17, 4);
  delay(1000);
  goBack();
}

void delayWatering(int deviceIdx) {
  String topic = String("ha/riego/") + wateringDevices[deviceIdx] + "/set";
  MQTTClient::publish(topic.c_str(), "delay");
  tft.fillScreen(TFT_BLACK);
  tft.fillRect(0, 0, 320, 34, TFT_DARKCYAN);
  tft.setTextColor(TFT_WHITE, TFT_DARKCYAN);
  tft.setTextDatum(TC_DATUM);
  tft.drawString("Delayed 1 day!", 160, 17, 4);
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
  tft.fillRect(0, 0, 320, 34, TFT_NAVY);
  tft.setTextColor(TFT_WHITE, TFT_NAVY);
  tft.setTextDatum(TC_DATUM);
  tft.drawString("BLE Devices", 160, 17, 4);

  const auto& devices = BLEScanner::getDevices();
  int y = 44;
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextDatum(TL_DATUM);
  int count = 0;
  for (const auto& dev : devices) {
    tft.drawString(dev, 10, y, 2);
    y += 24;
    count++;
    if (count >= 5) break; // Fit up to 5 devices
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

  // Draw title bar with breadcrumbs
  String title = "Main Menu";
  if (!menuStack.empty()) {
    if (currentMenu.size() > 0 && currentMenu[0].label == "Start") {
      title = "Riego > " + wateringDevices[menuStack[menuStack.size()-1].size()-3]; // crude, but works for now
    } else if (currentMenu[0].label == "Front Lawn" || currentMenu[0].label == "Back Garden") {
      title = "Riego";
    } else if (currentMenu[0].label == "WiFi") {
      title = "Settings";
    } else if (currentMenu[0].label == "Riego") {
      title = "HA";
    } else if (currentMenu[0].label == "BLE Devices") {
      title = "Main Menu";
    }
  }
  tft.fillRect(0, 0, 320, 34, TFT_DARKGREY);
  tft.setTextColor(TFT_WHITE, TFT_DARKGREY);
  tft.setTextDatum(TC_DATUM);
  tft.drawString(title, 160, 17, 4);

  // Draw menu items
  for (int i = 0; i < currentMenu.size(); ++i) {
    int y = 44 + i * 32;
    if (i == selectedIndex) {
      tft.fillRect(0, y, 320, 32, TFT_YELLOW);
      tft.setTextColor(TFT_BLACK, TFT_YELLOW);
      // Draw icon based on label (just a few examples)
      if (currentMenu[i].label == "Start") {
        tft.drawCircle(24, y+16, 10, TFT_GREEN);
        tft.fillCircle(24, y+16, 8, TFT_GREEN);
      }
      if (currentMenu[i].label == "Delay 1 day") {
        tft.drawCircle(24, y+16, 10, TFT_CYAN);
        tft.drawLine(24, y+16, 34, y+16, TFT_CYAN);
      }
      if (currentMenu[i].label.startsWith("<")) {
        tft.fillTriangle(14, y+16, 34, y+8, 34, y+24, TFT_DARKGREY);
      }
    } else {
      tft.fillRect(0, y, 320, 32, TFT_BLACK);
      tft.setTextColor(TFT_WHITE, TFT_BLACK);
      if (currentMenu[i].label == "Start") {
        tft.drawCircle(24, y+16, 10, TFT_DARKGREEN);
        tft.fillCircle(24, y+16, 8, TFT_DARKGREEN);
      }
      if (currentMenu[i].label == "Delay 1 day") {
        tft.drawCircle(24, y+16, 10, TFT_DARKCYAN);
        tft.drawLine(24, y+16, 34, y+16, TFT_DARKCYAN);
      }
      if (currentMenu[i].label.startsWith("<")) {
        tft.fillTriangle(14, y+16, 34, y+8, 34, y+24, TFT_DARKGREY);
      }
    }
    tft.setTextDatum(TL_DATUM);
    tft.drawString(currentMenu[i].label, 48, y+8, 2); // leave space for icon
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
    tft.fillRect(0, 0, 320, 34, TFT_DARKGREY);
    tft.setTextColor(TFT_WHITE, TFT_DARKGREY);
    tft.setTextDatum(TC_DATUM);
    tft.drawString("WiFi config coming", 160, 17, 4);
    delay(2000);
    goBack();
  }};
  MenuItem mqttSettings = { "MQTT", {backOption}, []() {
    tft.fillScreen(TFT_BLACK);
    tft.fillRect(0, 0, 320, 34, TFT_DARKGREY);
    tft.setTextColor(TFT_WHITE, TFT_DARKGREY);
    tft.setTextDatum(TC_DATUM);
    tft.drawString("MQTT config coming", 160, 17, 4);
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
