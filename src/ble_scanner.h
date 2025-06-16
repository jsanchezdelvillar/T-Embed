#pragma once
#include <vector>
#include <Arduino.h>

namespace BLEScanner {
  void begin();
  void update();
  const std::vector<String>& getDevices();
  void clearDevices();
}