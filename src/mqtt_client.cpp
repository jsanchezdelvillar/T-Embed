#include "mqtt_client.h"
#include <WiFi.h>
#include <PubSubClient.h>

WiFiClient espClient;
PubSubClient client(espClient);

void MQTTClient::begin() {
  WiFi.begin("YOUR_WIFI_SSID", "YOUR_WIFI_PASS");
  while (WiFi.status() != WL_CONNECTED) delay(500);

  client.setServer("YOUR_MQTT_BROKER", 1883);
  while (!client.connected()) {
    client.connect("t_embed_ble_proxy");
  }
}

void MQTTClient::update() {
  client.loop();
}

void MQTTClient::publish(const char* topic, const char* payload) {
  if (client.connected()) {
    client.publish(topic, payload);
  }
}