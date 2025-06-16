#pragma once
#include <functional>
#include <Arduino.h>

namespace MQTTClient {
  void begin();
  void update();
  void publish(const char* topic, const char* payload);

  // Subscribe to state topic for a device (e.g., "FrontLawn")
  void subscribeToState(const char* device, std::function<void(const String&)> onState);

  // Get cached state (if available)
  String getDeviceState(const char* device);

  // For PubSubClient
  void onMqttMessage(char* topic, byte* payload, unsigned int length);
}
