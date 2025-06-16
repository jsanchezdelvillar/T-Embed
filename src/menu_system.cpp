#include "menu_system.h"
#include "settings_manager.h"
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

void buildMenu() {
  MenuItem wifiSettings = { "WiFi", {}, []() {
    // Placeholder for WiFi config logic
  }};
  MenuItem mqttSettings = { "MQTT", {}, []() {
    // Placeholder for MQTT config logic
  }};
  MenuItem bleStatus = { "BLE Devices", {}, []() {
    // Placeholder for BLE status screen
  }};
  MenuItem backOption = { "< Back", {}, []() { goBack(); } };

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
      settings
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
