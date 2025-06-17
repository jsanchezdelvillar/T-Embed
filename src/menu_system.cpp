#include "menu_system.h"
#include "settings_manager.h"
#include "ble_scanner.h"
#include "mqtt_client.h"
#include "config.h"
#include <TFT_eSPI.h>
#include <RotaryEncoder.h>
#include <vector>
#include <map>

const char* wateringDevices[] = { "FrontLawn", "BackGarden" };
const int wateringDeviceCount = sizeof(wateringDevices) / sizeof(wateringDevices[0]);

struct WateringConfig {
    String time = "06:30:00";
    int duration = 15;
    String periodicity = "24H";
};
std::map<String, WateringConfig> zoneConfigs;

const char* periodicityOptions[] = {"Man", "12H", "24H", "48H", "72H"};
const int periodicityOptionsCount = sizeof(periodicityOptions) / sizeof(periodicityOptions[0]);

TFT_eSPI tft = TFT_eSPI();
RotaryEncoder encoder(34, 35, RotaryEncoder::LatchMode::TWO03);
int lastPos = -1;
bool buttonPressed = false;
const int BUTTON_PIN = 0;

enum EditField {
    FIELD_TIME,
    FIELD_DURATION,
    FIELD_PERIODICITY,
    FIELD_START,
    FIELD_DELAY,
    FIELD_BACK,
    FIELD_COUNT
};

int selectedField = 0;
bool editing = false;
int editSubfield = 0;
int deviceMenuIdx = 0;

void onConfigStateReceived(const String& zone, const WateringConfig& cfg) {
    zoneConfigs[zone] = cfg;
}

void publishConfigChange(const String& zone, const WateringConfig& cfg, const String& fieldChanged) {
    String topic = "ha/riego/" + zone + "/config/set";
    String payload = "{";
    if (fieldChanged == "time") {
        payload += "\"time\":\"" + cfg.time + "\"";
    } else if (fieldChanged == "duration") {
        payload += "\"duration\":" + String(cfg.duration);
    } else if (fieldChanged == "periodicity") {
        payload += "\"periodicity\":\"" + cfg.periodicity + "\"";
    }
    payload += "}";
    MQTTClient::publish(topic.c_str(), payload.c_str());
}

void publishCommand(const String& zone, const char* cmd) {
    String topic = "ha/riego/" + zone + "/set";
    MQTTClient::publish(topic.c_str(), cmd);
}

void drawZoneMenu(const String& zone) {
    tft.fillScreen(TFT_BLACK);
    tft.setTextDatum(TL_DATUM);

    WateringConfig& cfg = zoneConfigs[zone];
    int hour = 6, min = 30, sec = 0;
    sscanf(cfg.time.c_str(), "%d:%d:%d", &hour, &min, &sec);

    int y = 44;
    int x = 10;
    int fieldW = 100;
    int fieldH = 38;

    // Time Field
    tft.fillRect(x, y, fieldW, fieldH, selectedField == FIELD_TIME ? TFT_YELLOW : TFT_DARKGREY);
    tft.setTextColor(selectedField == FIELD_TIME ? TFT_BLACK : TFT_WHITE, selectedField == FIELD_TIME ? TFT_YELLOW : TFT_DARKGREY);
    tft.drawString("Time:", x+4, y+4, 2);
    char timeBuf[12];
    sprintf(timeBuf, "%02d:%02d:%02d", hour, min, sec);
    tft.drawString(timeBuf, x+4, y+20, 2);

    // Duration Field
    x += fieldW+4;
    tft.fillRect(x, y, fieldW, fieldH, selectedField == FIELD_DURATION ? TFT_YELLOW : TFT_DARKGREY);
    tft.setTextColor(selectedField == FIELD_DURATION ? TFT_BLACK : TFT_WHITE, selectedField == FIELD_DURATION ? TFT_YELLOW : TFT_DARKGREY);
    tft.drawString("Dur:", x+4, y+4, 2);
    tft.drawString(String(cfg.duration) + "m", x+4, y+20, 2);

    // Periodicity Field
    x += fieldW+4;
    tft.fillRect(x, y, fieldW, fieldH, selectedField == FIELD_PERIODICITY ? TFT_YELLOW : TFT_DARKGREY);
    tft.setTextColor(selectedField == FIELD_PERIODICITY ? TFT_BLACK : TFT_WHITE, selectedField == FIELD_PERIODICITY ? TFT_YELLOW : TFT_DARKGREY);
    tft.drawString("Per:", x+4, y+4, 2);
    tft.drawString(cfg.periodicity, x+4, y+20, 2);

    // Second line: Start/Delay/Back
    y += fieldH + 16;
    x = 10;
    int btnW = 85;
    int btnH = 38;

    // Start button
    tft.fillRect(x, y, btnW, btnH, selectedField == FIELD_START ? TFT_YELLOW : TFT_DARKGREEN);
    tft.setTextColor(selectedField == FIELD_START ? TFT_BLACK : TFT_WHITE, selectedField == FIELD_START ? TFT_YELLOW : TFT_DARKGREEN);
    tft.drawString("Start", x+16, y+10, 2);

    // Delay button
    x += btnW + 8;
    tft.fillRect(x, y, btnW, btnH, selectedField == FIELD_DELAY ? TFT_YELLOW : TFT_DARKCYAN);
    tft.setTextColor(selectedField == FIELD_DELAY ? TFT_BLACK : TFT_WHITE, selectedField == FIELD_DELAY ? TFT_YELLOW : TFT_DARKCYAN);
    tft.drawString("Delay 24H", x+4, y+10, 2);

    // Back button
    x += btnW + 8;
    tft.fillRect(x, y, btnW, btnH, selectedField == FIELD_BACK ? TFT_YELLOW : TFT_DARKGREY);
    tft.setTextColor(selectedField == FIELD_BACK ? TFT_BLACK : TFT_WHITE, selectedField == FIELD_BACK ? TFT_YELLOW : TFT_DARKGREY);
    tft.drawString("< Back", x+16, y+10, 2);

    if (editing) {
        tft.setTextColor(TFT_RED, TFT_BLACK);
        tft.drawString("Edit mode: rotate to change, click to save", 10, 220, 2);
    }
}

void zoneMenuEnterEdit(const String& zone) {
    editing = true;
    editSubfield = 0;
}

void zoneMenuExitEdit(const String& zone) {
    editing = false;
    editSubfield = 0;
    WateringConfig& cfg = zoneConfigs[zone];
    if (selectedField == FIELD_TIME) {
        publishConfigChange(zone, cfg, "time");
    } else if (selectedField == FIELD_DURATION) {
        publishConfigChange(zone, cfg, "duration");
    } else if (selectedField == FIELD_PERIODICITY) {
        publishConfigChange(zone, cfg, "periodicity");
    }
    drawZoneMenu(zone);
}

void handleZoneMenuInput(const String& zone) {
    WateringConfig& cfg = zoneConfigs[zone];

    if (editing) {
        if (selectedField == FIELD_TIME) {
            int hour, min, sec;
            sscanf(cfg.time.c_str(), "%d:%d:%d", &hour, &min, &sec);
            int newPos = encoder.getPosition();
            int delta = newPos - lastPos;
            if (delta != 0) {
                if (editSubfield == 0) { hour = (hour + delta + 24) % 24; }
                else if (editSubfield == 1) { min = (min + delta + 60) % 60; }
                else if (editSubfield == 2) { sec = (sec + delta + 60) % 60; }
                char buf[12];
                sprintf(buf, "%02d:%02d:%02d", hour, min, sec);
                cfg.time = String(buf);
                lastPos = newPos;
                drawZoneMenu(zone);
            }
        } else if (selectedField == FIELD_DURATION) {
            int newPos = encoder.getPosition();
            int delta = newPos - lastPos;
            if (delta != 0) {
                cfg.duration = max(1, cfg.duration + delta);
                lastPos = newPos;
                drawZoneMenu(zone);
            }
        } else if (selectedField == FIELD_PERIODICITY) {
            int idx = 0;
            for (; idx < periodicityOptionsCount; ++idx)
                if (cfg.periodicity == periodicityOptions[idx]) break;
            int newPos = encoder.getPosition();
            int delta = newPos - lastPos;
            if (delta != 0) {
                idx = (idx + delta + periodicityOptionsCount) % periodicityOptionsCount;
                cfg.periodicity = periodicityOptions[idx];
                lastPos = newPos;
                drawZoneMenu(zone);
            }
        }
    } else {
        int newPos = encoder.getPosition();
        int delta = newPos - lastPos;
        if (delta != 0) {
            selectedField = (selectedField + delta + FIELD_COUNT) % FIELD_COUNT;
            lastPos = newPos;
            drawZoneMenu(zone);
        }
        if (buttonPressed) {
            buttonPressed = false;
            if (selectedField == FIELD_TIME ||
                selectedField == FIELD_DURATION ||
                selectedField == FIELD_PERIODICITY) {
                zoneMenuEnterEdit(zone);
                lastPos = encoder.getPosition();
            } else if (selectedField == FIELD_START) {
                publishCommand(zone, "start");
            } else if (selectedField == FIELD_DELAY) {
                publishCommand(zone, "delay");
            } else if (selectedField == FIELD_BACK) {
                selectedField = 0;
                editing = false;
                buildRiegoMenu();
                return;
            }
            drawZoneMenu(zone);
        }
    }
}

void handleZoneEditButton(const String& zone) {
    if (!editing) return;
    if (selectedField == FIELD_TIME) {
        editSubfield++;
        if (editSubfield >= 3) {
            zoneMenuExitEdit(zone);
        }
    } else {
        zoneMenuExitEdit(zone);
    }
}

void buildZoneMenu(int deviceIdx) {
    deviceMenuIdx = deviceIdx;
    selectedField = 0;
    editing = false;
    WateringConfig& cfg = zoneConfigs[wateringDevices[deviceIdx]];
    if (cfg.time.length() < 5) {
        cfg.time = "06:30:00";
        cfg.duration = 15;
        cfg.periodicity = "24H";
    }
    drawZoneMenu(wateringDevices[deviceIdx]);
}

void buildRiegoMenu() {
    tft.fillScreen(TFT_BLACK);
    tft.setTextDatum(TC_DATUM);

    tft.fillRect(0, 0, 320, 34, TFT_DARKGREY);
    tft.setTextColor(TFT_WHITE, TFT_DARKGREY);
    tft.drawString("Riego", 160, 17, 4);

    for (int i = 0; i < wateringDeviceCount; ++i) {
        int y = 44 + i * 40;
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        tft.drawString(wateringDevices[i], 20, y, 2);
    }
    tft.setTextColor(TFT_WHITE, TFT_DARKGREY);
    tft.drawString("< Back", 160, 44 + wateringDeviceCount * 40, 2);
}

void handleRiegoMenuInput() {
    int menuItems = wateringDeviceCount + 1;
    int newPos = encoder.getPosition();
    int idx = (newPos % menuItems + menuItems) % menuItems;
    if (lastPos != newPos) {
        for (int i = 0; i < wateringDeviceCount; ++i) {
            int y = 44 + i * 40;
            tft.fillRect(0, y, 320, 36, idx == i ? TFT_YELLOW : TFT_BLACK);
            tft.setTextColor(idx == i ? TFT_BLACK : TFT_WHITE, idx == i ? TFT_YELLOW : TFT_BLACK);
            tft.drawString(wateringDevices[i], 20, y, 2);
        }
        int y = 44 + wateringDeviceCount * 40;
        tft.fillRect(0, y, 320, 36, idx == wateringDeviceCount ? TFT_YELLOW : TFT_DARKGREY);
        tft.setTextColor(idx == wateringDeviceCount ? TFT_BLACK : TFT_WHITE, idx == wateringDeviceCount ? TFT_YELLOW : TFT_DARKGREY);
        tft.drawString("< Back", 160, y, 2);
        lastPos = newPos;
    }
    if (buttonPressed) {
        buttonPressed = false;
        if (idx < wateringDeviceCount) {
            buildZoneMenu(idx);
        } else {
            buildMenu();
        }
    }
}

// You should add appropriate context for main menu, BLE, settings, etc.
// This code focuses on the Riego/zone menu logic.

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
    // TODO: Subscribe to config state for each zone (see MQTT setup in main.cpp)
}

void MenuSystem::update() {
    // You need to track your current menu state.
    // For demonstration, let's say you keep a state variable like `menuState`
    // with values: MAIN_MENU, RIEGO_MENU, ZONE_MENU, etc.

    // Example:
    // if (menuState == ZONE_MENU) {
    //     String zone = wateringDevices[deviceMenuIdx];
    //     handleZoneMenuInput(zone);
    //     if (editing && buttonPressed) {
    //         buttonPressed = false;
    //         handleZoneEditButton(zone);
    //     }
    // } else if (menuState == RIEGO_MENU) {
    //     handleRiegoMenuInput();
    // } else {
    //     // handle main menu, BLE, settings, etc.
    // }
}
