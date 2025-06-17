#include "esphome.h"
#include <vector>
#include <string>
#include <algorithm>

using namespace esphome;

struct BLEDeviceInfo {
  std::string mac;
  int rssi;
};

class CustomBLEListDisplay : public Component {
 public:
  CustomBLEListDisplay(font::Font *font) : font_(font), scroll_pos_(0) {}

  void setup() override {}

  void loop() override {}

  void draw(display::DisplayBuffer &it) {
    it.fill(COLOR_BLACK);
    it.printf(10, 4, font_, COLOR_WHITE, "BLE Devices:");
    int y = 30;
    int max_show = 10;
    int shown = 0;

    if (devices_.empty()) {
      it.printf(10, y, font_, COLOR_YELLOW, "No devices found");
    } else {
      for (int i = scroll_pos_; i < (int)devices_.size() && shown < max_show; i++, shown++) {
        const auto &dev = devices_[i];
        it.printf(10, y, font_, COLOR_CYAN, "%s  %d dBm", dev.mac.c_str(), dev.rssi);
        y += 22;
      }
    }
  }

  void on_ble_advertise(const esp32_ble_tracker::ESPBTDevice &device) {
    std::string mac = device.address_str();
    int rssi = device.get_rssi();
    // Check for duplicates
    auto it = std::find_if(devices_.begin(), devices_.end(), [&](const BLEDeviceInfo &d) {
      return d.mac == mac;
    });
    if (it == devices_.end()) {
      devices_.push_back({mac, rssi});
    } else {
      it->rssi = rssi;
    }
    // Sort by RSSI (optional)
    std::sort(devices_.begin(), devices_.end(), [](const BLEDeviceInfo &a, const BLEDeviceInfo &b) {
      return a.rssi > b.rssi;
    });
  }

  void scroll(int delta) {
    int max_scroll = std::max((int)devices_.size() - 1, 0);
    scroll_pos_ += delta;
    if (scroll_pos_ < 0) scroll_pos_ = 0;
    if (scroll_pos_ > max_scroll) scroll_pos_ = max_scroll;
  }

  int device_count() const { return devices_.size(); }

 protected:
  font::Font *font_;
  std::vector<BLEDeviceInfo> devices_;
  int scroll_pos_;
};

CustomBLEListDisplay *custom_ble_list_display = nullptr;

// ESPHome BLE callback
class MyBLEListener : public esp32_ble_tracker::ESPBTDeviceListener {
  void on_device_found(const esp32_ble_tracker::ESPBTDevice &device) override {
    if (custom_ble_list_display) {
      custom_ble_list_display->on_ble_advertise(device);
    }
  }
};

MyBLEListener my_ble_listener;

void register_ble_list_display(CustomBLEListDisplay *display) {
  custom_ble_list_display = display;
  App.get<esp32_ble_tracker::ESP32BLETracker>()->add_on_device_listener(&my_ble_listener);
}
