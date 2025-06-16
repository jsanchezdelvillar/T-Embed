#include "mqtt_client.h"
#include "config.h"
#include <WiFi.h>
#include <PubSubClient.h>

WiFiClient espClient;
PubSubClient client(espClient);

void MQTTClient::begin() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) delay(500);

  client.setServer(MQTT_BROKER, MQTT_PORT);
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
