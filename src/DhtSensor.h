#pragma once

#include <Arduino.h>
#include <DHT.h>

#define DHT_PIN  15
#define DHT_TYPE DHT11

// DHT11 อ่านค่าได้ไม่เกิน 1 ครั้ง/วินาที ตาม README เว้นระยะอ่านอย่างน้อย 2 วินาที
#define DHT_READ_INTERVAL_MS 2000UL

// ถ้าอ่านค่าจริงล้มเหลวติดต่อกันครบจำนวนนี้ (ไม่ได้ต่อเซนเซอร์จริง) จะสลับไปใช้ค่าสุ่มจำลองแทน
#define DHT_FAIL_THRESHOLD_FOR_SIM 3

// ช่วงค่าที่ใช้สุ่มจำลอง (สภาพอากาศในบ้าน/ห้องทั่วไป)
#define DHT_SIM_TEMP_MIN_C   250   // 25.0 C (x10 เพื่อสุ่มเป็นทศนิยม 1 ตำแหน่ง)
#define DHT_SIM_TEMP_MAX_C   350   // 35.0 C
#define DHT_SIM_HUM_MIN_PCT  40
#define DHT_SIM_HUM_MAX_PCT  80

DHT dht(DHT_PIN, DHT_TYPE);

struct DhtData {
  bool valid = false;
  bool isSimulated = false;
  float temperature = NAN;
  float humidity = NAN;
};

static DhtData latestDhtData;
static unsigned long lastDhtReadTime = 0;
static uint8_t dhtConsecutiveFailCount = 0;

inline void dhtSensor_setup() {
  dht.begin();
  randomSeed(analogRead(0));
}

// สุ่มค่าจำลองในช่วงที่สมเหตุสมผล ใช้แทนเมื่อไม่ได้ต่อเซนเซอร์จริง
static void dhtSensor_generateSimulatedReading(float &temperature, float &humidity) {
  temperature = random(DHT_SIM_TEMP_MIN_C, DHT_SIM_TEMP_MAX_C + 1) / 10.0f;
  humidity = random(DHT_SIM_HUM_MIN_PCT, DHT_SIM_HUM_MAX_PCT + 1);
}

inline void dhtSensor_loop() {
  unsigned long now = millis();
  if (now - lastDhtReadTime < DHT_READ_INTERVAL_MS && lastDhtReadTime != 0) return;
  lastDhtReadTime = now;

  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();

  if (isnan(humidity) || isnan(temperature)) {
    if (dhtConsecutiveFailCount < 255) dhtConsecutiveFailCount++;

    if (dhtConsecutiveFailCount >= DHT_FAIL_THRESHOLD_FOR_SIM) {
      dhtSensor_generateSimulatedReading(temperature, humidity);

      latestDhtData.valid = true;
      latestDhtData.isSimulated = true;
      latestDhtData.temperature = temperature;
      latestDhtData.humidity = humidity;

      Serial.print("[DHT11] ไม่พบเซนเซอร์ ใช้ค่าจำลอง (SIM) -> Temp: ");
      Serial.print(temperature);
      Serial.print(" C, Hum: ");
      Serial.print(humidity);
      Serial.println(" %");
      return;
    }

    latestDhtData.valid = false;
    Serial.println("[DHT11] อ่านค่าจากเซนเซอร์ล้มเหลว");
    return;
  }

  dhtConsecutiveFailCount = 0;

  latestDhtData.valid = true;
  latestDhtData.isSimulated = false;
  latestDhtData.temperature = temperature;
  latestDhtData.humidity = humidity;

  Serial.print("[DHT11] Temp: ");
  Serial.print(temperature);
  Serial.print(" C, Hum: ");
  Serial.print(humidity);
  Serial.println(" %");
}
