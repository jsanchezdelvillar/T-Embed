#pragma once

namespace MQTTClient {
  void begin();
  void update();
  void publish(const char* topic, const char* payload);
}