#include "menu_system.h"
#include "settings_manager.h"
#include "ble_scanner.h"
#include "mqtt_client.h"
#include "config.h"
#include <TFT_eSPI.h>
#include <RotaryEncoder.h>
#include <vector>
#include <map>

// --- Watering device list (hardcoded for now) ---
const char* wateringDevices[] = { "FrontLawn", "BackGarden" };
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
  bool enabled;
  std::vector<MenuItem> subitems;
  std::function<void()> action;
};

std::vector<MenuItem> currentMenu;
std::vector<std::vector<MenuItem>> menuStack;
std::vector<int> indexStack; // To remember previous selected index per menu
int selectedIndex = 0;

// --- Device state cache for UI feedback ---
std::map<String, String> wateringStates;

// Subscribe once to each device's state topic
void subscribeDeviceStates() {
  for (int i = 0; i < wateringDeviceCount; ++i) {
    MQTTClient::subscribeToState(
      wateringDevices[i],
      [i](const String& newState) {
        wateringStates[wateringDevices[i]] = newState;
        // Optionally: redraw menu if this device's menu is open
        // Only redraw if we're in this device's menu
        if (!menuStack.empty() && menuStack.back().size() &&
            menuStack.back()[0].label == wateringDevices[i]) {
          drawMenu();
        }
      }
    );
  }
}

// --- Watering Device Actions ---
void startWatering(int deviceIdx) {
  String topic = String("ha/riego/") + wateringDevices[deviceIdx] + "/set";
  MQTTClient::publish(topic.c_str(), "start");
  tft.fillScreen(TFT_BLACK);
  tft.fillRect(0, 0, 320, 34, TFT_DARKGREEN);
  tft.setTextColor(TFT_WHITE, TFT_DARKGREEN);
  tft.setTextDatum(TC_DATUM);
  tft.drawString("Order sent!", 160, 17, 4);
  delay(700);
  goBack();
}

void delayWatering(int deviceIdx) {
  String topic = String("ha/riego/") + wateringDevices[deviceIdx] + "/set";
  MQTTClient::publish(topic.c_str(), "delay");
  tft.fillScreen(TFT_BLACK);
  tft.fillRect(0, 0, 320, 34, TFT_DARKCYAN);
  tft.setTextColor(TFT_WHITE, TFT_DARKCYAN);
  tft.setTextDatum(TC_DATUM);
  tft.drawString("Delay sent!", 160, 17, 4);
  delay(700);
  goBack();
}

// --- Submenu builders ---
void buildWateringDeviceMenu(int deviceIdx) {
  String device = wateringDevices[deviceIdx];
  String state = MQTTClient::getDeviceState(device.c_str());
  if (state.length() == 0) state = "unknown";
  wateringStates[device] = state; // cache

  // Only allow "Start" if idle
  bool canStart = (state == "idle");

  MenuItem backOption = { "< Back", true, {}, goBack };
  MenuItem startOption = { "Start", canStart, {}, [deviceIdx]() { startWatering(deviceIdx); } };
  MenuItem delayOption = { "Delay 1 day", true, {}, [deviceIdx]() { delayWatering(deviceIdx); } };
  std::vector<MenuItem> deviceMenu = { startOption, delayOption, backOption };
  menuStack.push_back(currentMenu);
  indexStack.push_back(selectedIndex);
  currentMenu = deviceMenu;
  selectedIndex = 0;
  drawMenu();
}

void buildRiegoMenu() {
  MenuItem backOption = { "< Back", true, {}, goBack };
  std::vector<MenuItem> riegoMenu;
  for (int i = 0; i < wateringDeviceCount; ++i) {
    riegoMenu.push_back({ wateringDevices[i], true, {}, [i]() { buildWateringDeviceMenu(i); } });
  }
  riegoMenu.push_back(backOption);
  menuStack.push_back(currentMenu);
  indexStack.push_back(selectedIndex);
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
      // Find which device we're on
      if (!menuStack.back().empty())
        title = "Riego > " + menuStack.back()[selectedIndex].label;
      else
        title = "Riego > ?";
    } else if (currentMenu[0].label == "FrontLawn" || currentMenu[0].label == "BackGarden") {
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

  // If in a device menu, show its current state
  if (!menuStack.empty() &&
      menuStack.back().size() &&
      (menuStack.back()[0].label == "FrontLawn" || menuStack.back()[0].label == "BackGarden")) {
    String device = menuStack.back()[selectedIndex].label;
    String state = wateringStates[device];
    if (state.length() == 0) state = "unknown";
    uint16_t color = (state == "idle") ? TFT_GREEN :
                     (state == "watering") ? TFT_BLUE :
                     (state == "error") ? TFT_RED : TFT_ORANGE;
    tft.fillRect(0, 150, 320, 20, color);
    tft.setTextColor(TFT_WHITE, color);
    tft.setTextDatum(TC_DATUM);
    tft.drawString("Status: " + state, 160, 160, 2);
  }

  // Draw menu items
  for (int i = 0; i < currentMenu.size(); ++i) {
    int y = 44 + i * 32;
    bool isSelected = (i == selectedIndex);
    bool enabled = currentMenu[i].enabled;
    uint16_t bg = isSelected ? (enabled ? TFT_YELLOW : TFT_DARKGREY) : TFT_BLACK;
    uint16_t fg = enabled ? (isSelected ? TFT_BLACK : TFT_WHITE) : TFT_LIGHTGREY;

    tft.fillRect(0, y, 320, 32, bg);
    tft.setTextColor(fg, bg);

    // Draw icon based on label (just a few examples)
    if (currentMenu[i].label == "Start") {
      tft.drawCircle(24, y+16, 10, enabled ? TFT_GREEN : TFT_DARKGREY);
      tft.fillCircle(24, y+16, 8, enabled ? TFT_GREEN : TFT_DARKGREY);
    }
    if (currentMenu[i].label == "Delay 1 day") {
      tft.drawCircle(24, y+16, 10, TFT_CYAN);
      tft.drawLine(24, y+16, 34, y+16, TFT_CYAN);
    }
    if (currentMenu[i].label.startsWith("<")) {
      tft.fillTriangle(14, y+16, 34, y+8, 34, y+24, TFT_DARKGREY);
    }
    tft.setTextDatum(TL_DATUM);
    tft.drawString(currentMenu[i].label, 48, y+8, 2); // leave space for icon
  }
}

void selectItem() {
  auto item = currentMenu[selectedIndex];
  if (!item.enabled) return; // Disabled item, do nothing
  if (!item.subitems.empty()) {
    menuStack.push_back(currentMenu);
    indexStack.push_back(selectedIndex);
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
    if (!indexStack.empty()) {
      selectedIndex = indexStack.back();
      indexStack.pop_back();
    } else {
      selectedIndex = 0;
    }
    drawMenu();
  }
}

// --- Root menu builder ---
void buildMenu() {
  MenuItem backOption = { "< Back", true, {}, goBack };

  // Riego menu (dynamic)
  MenuItem haRiego = { "Riego", true, {backOption}, buildRiegoMenu };
  MenuItem haMenu = { "HA", true, { haRiego, backOption }, showHAMenu };

  // Settings submenu
  MenuItem wifiSettings = { "WiFi", true, {backOption}, []() {
    tft.fillScreen(TFT_BLACK);
    tft.fillRect(0, 0, 320, 34, TFT_DARKGREY);
    tft.setTextColor(TFT_WHITE, TFT_DARKGREY);
    tft.setTextDatum(TC_DATUM);
    tft.drawString("WiFi config coming", 160, 17, 4);
    delay(2000);
    goBack();
  }};
  MenuItem mqttSettings = { "MQTT", true, {backOption}, []() {
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
    true,
    {
      wifiSettings,
      mqttSettings,
      backOption
    }
  };

  MenuItem bleStatus = { "BLE Devices", true, {backOption}, showBLEDevices };

  MenuItem root = {
    "Main Menu",
    true,
    {
      bleStatus,
      settings,
      haMenu
    }
  };

  currentMenu = root.subitems;
  indexStack.clear();
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
  subscribeDeviceStates(); // Subscribe to HA state topics!
  drawMenu();
}

void MenuSystem::update() {
  encoder.tick();
  int newPos = encoder.getPosition();
  int menuSize = currentMenu.size();
  // Only allow scrolling to enabled items
  if (menuSize > 0) {
    if (newPos != lastPos) {
      int direction = (newPos > lastPos) ? 1 : -1;
      int newIdx = selectedIndex;
      do {
        newIdx = (newIdx + direction + menuSize) % menuSize;
      } while (!currentMenu[newIdx].enabled && newIdx != selectedIndex);
      selectedIndex = newIdx;
      lastPos = newPos;
      drawMenu();
    }
  }

  if (buttonPressed) {
    buttonPressed = false;
    selectItem();
  }
}
