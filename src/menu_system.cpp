#include "menu_system.h"
#include "settings_manager.h"
#include "ble_scanner.h"
#include <TFT_eSPI.h>
#include <RotaryEncoder.h>
#include <vector>

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

// Forward declarations
void drawMenu();
void selectItem();
void goBack();
void showBLEDevices();
void showHAMenu();
void showRiegoMenu();

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

void showHAMenu() {
  // Placeholder for HA menu display logic
  drawMenu();
}

void showRiegoMenu() {
  tft.fillScreen(TFT_BLACK);
  tft.drawString("Riego setup coming soon", 10, 20, 2);
  delay(2000);
  goBack();
}

void buildMenu() {
  MenuItem wifiSettings = { "WiFi", {}, []() {
    tft.fillScreen(TFT_BLACK);
    tft.drawString("WiFi config coming", 10, 20, 2);
    delay(2000);
    goBack();
  }};
  MenuItem mqttSettings = { "MQTT", {}, []() {
    tft.fillScreen(TFT_BLACK);
    tft.drawString("MQTT config coming", 10, 20, 2);
    delay(2000);
    goBack();
  }};
  MenuItem bleStatus = { "BLE Devices", {}, showBLEDevices };
  MenuItem haRiego = { "Riego", {}, showRiegoMenu };
  MenuItem haMenu = { "HA", { haRiego }, showHAMenu };
  MenuItem backOption = { "< Back", {}, goBack };

  MenuItem settings = {
    "Settings",
    {
      wifiSettings,
      mqttSettings,
      backOption
    }
  };

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

void IRAM_ATTR handleButton() {
  buttonPressed = true;
}

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