#pragma once
namespace SettingsManager {
  void begin();
  String get(const String& key);
  void set(const String& key, const String& value);
}