#include "mqtt_client.h"
#include "config.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include <map>
#include <functional>

// Forward declaration for status callback
namespace MQTTClient {
  static std::map<String, String> deviceStates;
  static std::map<String, std::function<void(const String&)>> stateCallbacks;
}

WiFiClient espClient;
PubSubClient client(espClient);

void MQTTClient::begin() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) delay(500);

  client.setServer(MQTT_BROKER, MQTT_PORT);
  while (!client.connected()) {
    client.connect("t_embed_ble_proxy");
  }
  client.setCallback(MQTTClient::onMqttMessage);
}

void MQTTClient::update() {
  client.loop();
}

void MQTTClient::publish(const char* topic, const char* payload) {
  if (client.connected()) {
    client.publish(topic, payload, false); // Command topics MUST NOT be retained!
  }
}

void MQTTClient::subscribeToState(const char* device, std::function<void(const String&)> onState) {
  String stateTopic = String("ha/riego/") + device + "/state";
  client.subscribe(stateTopic.c_str());
  stateCallbacks[device] = onState;
}

String MQTTClient::getDeviceState(const char* device) {
  return deviceStates[String(device)];
}

// Called by PubSubClient when any message is received
void MQTTClient::onMqttMessage(char* topic, byte* payload, unsigned int length) {
  String topicStr(topic);
  String payloadStr;
  for (unsigned int i = 0; i < length; ++i) payloadStr += (char)payload[i];

  // Check if this is a state topic we care about
  if (topicStr.startsWith("ha/riego/") && topicStr.endsWith("/state")) {
    // Extract device name
    int start = String("ha/riego/").length();
    int end = topicStr.length() - String("/state").length();
    String device = topicStr.substring(start, end);

    deviceStates[device] = payloadStr;
    if (stateCallbacks.count(device)) {
      stateCallbacks[device](payloadStr); // Notify UI or menu
    }
  }
}
