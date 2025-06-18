#pragma once

#include "esphome/core/component.h"
#include "esphome/components/font/font.h"
#include "esphome/components/display/display_buffer.h"
#include "esphome/components/esp32_ble_tracker/esp32_ble_tracker.h"
#include <vector>
#include <string>
#include <algorithm>

namespace esphome {
namespace custom_ble_list_display {

struct BLEDeviceInfo {
  std::string mac;
  int rssi;
};

class CustomBLEListDisplay : public Component, public display::DisplayRenderer, public esp32_ble_tracker::ESPBTDeviceListener {
 public:
  void set_font(font::Font *font) { font_ = font; }

  void setup() override;
  void loop() override;

  void draw(display::DisplayBuffer &it) override;

  void on_device_found(const esp32_ble_tracker::ESPBTDevice &device) override;

  void scroll(int delta);
  int device_count() const { return devices_.size(); }

 protected:
  font::Font *font_{nullptr};
  std::vector<BLEDeviceInfo> devices_;
  int scroll_pos_{0};
};

}  // namespace custom_ble_list_display
}  // namespace esphome
