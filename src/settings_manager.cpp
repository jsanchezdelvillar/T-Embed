#include "settings_manager.h"
#include <EEPROM.h>
#include <ArduinoJson.h>

#define EEPROM_SIZE 1024

StaticJsonDocument<512> settings;

void SettingsManager::begin() {
  EEPROM.begin(EEPROM_SIZE);
  String json;
  for (int i = 0; i < EEPROM_SIZE; ++i) {
    char c = EEPROM.read(i);
    if (c == '\0') break;
    json += c;
  }
  deserializeJson(settings, json);
}

String SettingsManager::get(const String& key) {
  return settings[key] | "";
}

void SettingsManager::set(const String& key, const String& value) {
  settings[key] = value;
  String json;
  serializeJson(settings, json);
  for (int i = 0; i < json.length(); ++i) EEPROM.write(i, json[i]);
  EEPROM.write(json.length(), '\0');
  EEPROM.commit();
}