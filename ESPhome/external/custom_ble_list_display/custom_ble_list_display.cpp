#include "esphome/core/log.h"
#include "esphome/core/component.h"
#include "esphome/components/display/display_buffer.h"
#include "esphome/components/font/font.h"
#include "custom_ble_list_display.h"

namespace esphome {
namespace custom_ble_list_display {

static const char *const TAG = "custom_ble_list_display";

// Declare the schema
static const auto CUSTOM_BLE_LIST_DISPLAY_SCHEMA = 
  esphome::config_validation::cv::object({
    esphome::config_validation::cv::required("id") = esphome::config_validation::cv::string,
    esphome::config_validation::cv::required("font_id") = esphome::config_validation::cv::string,
    esphome::config_validation::cv::required("display_id") = esphome::config_validation::cv::string,
  });

void CustomBLEListDisplay::setup() {
  // Register as BLE listener
  auto *ble_tracker = App.get<esp32_ble_tracker::ESP32BLETracker>();
  if (ble_tracker != nullptr) {
    ble_tracker->add_on_device_listener(this);
  }
}

void CustomBLEListDisplay::loop() {
  // Nothing here for now
}

void CustomBLEListDisplay::draw(display::DisplayBuffer &it) {
  if (!font_)
    return;

  it.fill(Color::BLACK);
  it.printf(10, 4, font_, Color::WHITE, "BLE Devices:");
  int y = 30;
  int max_show = 7;
  int shown = 0;

  if (devices_.empty()) {
    it.printf(10, y, font_, Color::YELLOW, "No devices found");
  } else {
    for (int i = scroll_pos_; i < (int)devices_.size() && shown < max_show; i++, shown++) {
      const auto &dev = devices_[i];
      it.printf(10, y, font_, Color::CYAN, "%s  %d dBm", dev.mac.c_str(), dev.rssi);
      y += 22;
    }
  }
}

void CustomBLEListDisplay::on_device_found(const esp32_ble_tracker::ESPBTDevice &device) {
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
  // Sort by RSSI
  std::sort(devices_.begin(), devices_.end(), [](const BLEDeviceInfo &a, const BLEDeviceInfo &b) {
    return a.rssi > b.rssi;
  });
}

void CustomBLEListDisplay::scroll(int delta) {
  int max_scroll = std::max((int)devices_.size() - 1, 0);
  scroll_pos_ += delta;
  if (scroll_pos_ < 0) scroll_pos_ = 0;
  if (scroll_pos_ > max_scroll) scroll_pos_ = max_scroll;
}

// YAML schema registration
class CustomBLEListDisplayComponent : public Component {
 public:
  void set_font(font::Font *font) { this->font_ = font; }
  void set_display(display::DisplayBuffer *display) { this->display_ = display; }
  CustomBLEListDisplay *get_ble_list_display() { return &ble_list_display_; }

  void setup() override {
    ble_list_display_.set_font(font_);
    ble_list_display_.setup();
    if (display_ != nullptr) {
      display_->register_renderer(&ble_list_display_);
    }
  }
  void loop() override { ble_list_display_.loop(); }

 protected:
  font::Font *font_{nullptr};
  display::DisplayBuffer *display_{nullptr};
  CustomBLEListDisplay ble_list_display_;
};

}  // namespace custom_ble_list_display
}  // namespace esphome

using namespace esphome::custom_ble_list_display;

Component *custom_ble_list_display_create(const esphome::yaml::YamlConfig &config) {
  auto *component = new CustomBLEListDisplayComponent();
  if (config.has("font_id")) {
    component->set_font(config.get<font::Font *>("font_id"));
  }
  if (config.has("display_id")) {
    component->set_display(config.get<display::DisplayBuffer *>("display_id"));
  }
  return component;
}

// Register in ESPHome
ESPHOME_REGISTER_COMPONENT_WITH_SCHEMA(
  custom_ble_list_display, 
  custom_ble_list_display_create, 
  {
    ESPHOME_SCHEMA_REQUIRED("font_id", "ID"),
    ESPHOME_SCHEMA_OPTIONAL("display_id", "ID")
  }
)
