#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "WeatherMonitor.h"
#include "DhtSensor.h"
#include "RelayController.h"

// ---------- HiveMQ Public Broker (broker.hivemq.com) ----------
// Broker สาธารณะสำหรับทดสอบ ไม่ต้อง login (ไม่มี TLS/username/password)
// ทุกคนบนอินเทอร์เน็ตเห็น topic นี้ได้ จึงใส่ chip ID ต่อท้าย prefix กัน topic ชนกับอุปกรณ์อื่น
// ใช้เหมาะกับ demo/prototype เท่านั้น ห้ามส่งข้อมูล/คำสั่งที่ต้องการความปลอดภัยจริงจัง
#define MQTT_BROKER "broker.hivemq.com"
#define MQTT_PORT   1883

#define MQTT_PUBLISH_INTERVAL_MS 10000UL

WiFiClient mqttNetClient;
PubSubClient mqttClient(mqttNetClient);

static String mqttClientId;
static String mqttTopicPrefix; // "smarthome/esp32/<chipId>/"

// Topic แบบเต็ม (สร้างครั้งเดียวตอน setup กัน String churn ทุกรอบ loop)
static String mqttTopicWeatherTemp;
static String mqttTopicWeatherHum;
static String mqttTopicWeatherWind;
static String mqttTopicWeatherDesc;
static String mqttTopicWeatherRain;
static String mqttTopicWeatherAqi;
static String mqttTopicWeatherPm25;
static String mqttTopicDhtTemp;
static String mqttTopicDhtHum;
static String mqttTopicDhtSim;
static String mqttTopicSystemIp;
static String mqttTopicSystemWifi;
static String mqttRelayStateTopics[3];
static String mqttRelaySetTopics[3];

static unsigned long mqttLastPublishTime = 0;
static unsigned long mqttLastReconnectAttempt = 0;

static void mqttClient_onMessage(char *topic, byte *payload, unsigned int length) {
  String message;
  message.reserve(length);
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  message.trim();
  message.toUpperCase();

  for (uint8_t i = 0; i < 3; i++) {
    if (mqttRelaySetTopics[i] == topic) {
      bool newState = (message == "ON" || message == "1" || message == "TRUE");
      relayController_setManualState(relayConfigs[i], newState);

      Serial.print("[MQTT] Command ");
      Serial.print(topic);
      Serial.print(" -> ");
      Serial.println(message);
      return;
    }
  }
}

inline void mqttClient_setup() {
  char chipId[13];
  snprintf(chipId, sizeof(chipId), "%012llX", ESP.getEfuseMac());

  mqttClientId = String("ESP32-SmartHome-") + chipId;
  mqttTopicPrefix = String("smarthome/esp32/") + chipId + "/";

  mqttTopicWeatherTemp = mqttTopicPrefix + "weather/temp";
  mqttTopicWeatherHum = mqttTopicPrefix + "weather/hum";
  mqttTopicWeatherWind = mqttTopicPrefix + "weather/wind";
  mqttTopicWeatherDesc = mqttTopicPrefix + "weather/desc";
  mqttTopicWeatherRain = mqttTopicPrefix + "weather/rain";
  mqttTopicWeatherAqi = mqttTopicPrefix + "weather/aqi";
  mqttTopicWeatherPm25 = mqttTopicPrefix + "weather/pm25";
  mqttTopicDhtTemp = mqttTopicPrefix + "dht/temp";
  mqttTopicDhtHum = mqttTopicPrefix + "dht/hum";
  mqttTopicDhtSim = mqttTopicPrefix + "dht/sim";
  mqttTopicSystemIp = mqttTopicPrefix + "system/ip";
  mqttTopicSystemWifi = mqttTopicPrefix + "system/wifi";

  for (uint8_t i = 0; i < 3; i++) {
    mqttRelayStateTopics[i] = mqttTopicPrefix + "relay" + String(i + 1) + "/state";
    mqttRelaySetTopics[i] = mqttTopicPrefix + "relay" + String(i + 1) + "/set";
  }

  mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
  mqttClient.setCallback(mqttClient_onMessage);
}

static bool mqttClient_reconnect() {
  Serial.print("[MQTT] Connecting to broker.hivemq.com...");

  if (mqttClient.connect(mqttClientId.c_str())) {
    Serial.println(" connected!");
    for (uint8_t i = 0; i < 3; i++) {
      mqttClient.subscribe(mqttRelaySetTopics[i].c_str());
    }
    return true;
  }

  Serial.print(" failed, rc=");
  Serial.println(mqttClient.state());
  return false;
}

static void mqttClient_publishStatus() {
  char buf[32];

  dtostrf(latestWeatherData.temperature, 0, 1, buf);
  mqttClient.publish(mqttTopicWeatherTemp.c_str(), buf);
  dtostrf(latestWeatherData.humidity, 0, 0, buf);
  mqttClient.publish(mqttTopicWeatherHum.c_str(), buf);
  dtostrf(latestWeatherData.windSpeed, 0, 1, buf);
  mqttClient.publish(mqttTopicWeatherWind.c_str(), buf);
  mqttClient.publish(mqttTopicWeatherDesc.c_str(), latestWeatherData.weatherDesc.c_str());
  mqttClient.publish(mqttTopicWeatherRain.c_str(), latestWeatherData.rainExpected ? "1" : "0");
  snprintf(buf, sizeof(buf), "%d", latestWeatherData.aqi);
  mqttClient.publish(mqttTopicWeatherAqi.c_str(), buf);
  dtostrf(latestWeatherData.pm25, 0, 1, buf);
  mqttClient.publish(mqttTopicWeatherPm25.c_str(), buf);

  dtostrf(latestDhtData.temperature, 0, 1, buf);
  mqttClient.publish(mqttTopicDhtTemp.c_str(), buf);
  dtostrf(latestDhtData.humidity, 0, 0, buf);
  mqttClient.publish(mqttTopicDhtHum.c_str(), buf);
  mqttClient.publish(mqttTopicDhtSim.c_str(), latestDhtData.isSimulated ? "1" : "0");

  mqttClient.publish(mqttTopicSystemIp.c_str(), WiFi.localIP().toString().c_str());
  mqttClient.publish(mqttTopicSystemWifi.c_str(), WiFi.status() == WL_CONNECTED ? "connected" : "disconnected");

  for (uint8_t i = 0; i < 3; i++) {
    // retain = true เพื่อให้ dashboard/ผู้ subscribe ใหม่เห็นสถานะล่าสุดทันทีโดยไม่ต้องรอรอบ publish ถัดไป
    mqttClient.publish(mqttRelayStateTopics[i].c_str(), relayConfigs[i].state ? "ON" : "OFF", true);
  }
}

inline void mqttClient_loop() {
  if (WiFi.status() != WL_CONNECTED) return;

  if (!mqttClient.connected()) {
    unsigned long now = millis();
    if (now - mqttLastReconnectAttempt >= 5000UL) {
      mqttLastReconnectAttempt = now;
      mqttClient_reconnect();
    }
    return;
  }

  mqttClient.loop();

  unsigned long now = millis();
  if (now - mqttLastPublishTime >= MQTT_PUBLISH_INTERVAL_MS || mqttLastPublishTime == 0) {
    mqttLastPublishTime = now;
    mqttClient_publishStatus();
  }
}
