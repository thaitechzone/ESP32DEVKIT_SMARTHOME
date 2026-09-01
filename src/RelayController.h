#pragma once

#include <Arduino.h>
#include <time.h>
#include "DhtSensor.h"

#define RELAY_ON  LOW
#define RELAY_OFF HIGH

enum RelayMode {
  RELAY_MODE_MANUAL = 0,
  RELAY_MODE_THRESHOLD = 1,
  RELAY_MODE_SCHEDULE = 2
};

enum ThresholdMetric {
  THRESHOLD_METRIC_TEMPERATURE = 0,
  THRESHOLD_METRIC_HUMIDITY = 1
};

struct RelayConfig {
  uint8_t pin;
  const char *name;
  bool state;           // true = ON (สถานะจริงปัจจุบันของ relay)
  RelayMode mode;

  // Threshold mode: เปิด relay เมื่อค่า >= turnOnAt และปิดเมื่อค่า <= turnOffAt (มี hysteresis กันสวิตช์รัว)
  ThresholdMetric thresholdMetric;
  float thresholdTurnOnAt;
  float thresholdTurnOffAt;

  // Schedule mode: ช่วงเวลาเปิด (HH:MM - HH:MM) แบบ 24 ชม. รองรับข้ามเที่ยงคืน (เช่น 22:00-06:00)
  uint8_t scheduleOnHour;
  uint8_t scheduleOnMinute;
  uint8_t scheduleOffHour;
  uint8_t scheduleOffMinute;
};

RelayConfig relayConfigs[3] = {
  { 17, "Relay1", false, RELAY_MODE_MANUAL, THRESHOLD_METRIC_TEMPERATURE, 30.0f, 28.0f, 18, 0, 6, 0 },
  { 16, "Relay2", false, RELAY_MODE_MANUAL, THRESHOLD_METRIC_TEMPERATURE, 30.0f, 28.0f, 18, 0, 6, 0 },
  { 4,  "Relay3", false, RELAY_MODE_MANUAL, THRESHOLD_METRIC_TEMPERATURE, 30.0f, 28.0f, 18, 0, 6, 0 },
};

inline void relayController_apply(RelayConfig &r, bool newState) {
  if (r.state == newState) return;
  r.state = newState;
  digitalWrite(r.pin, r.state ? RELAY_ON : RELAY_OFF);
  Serial.print("[Relay] ");
  Serial.print(r.name);
  Serial.print(" -> ");
  Serial.println(r.state ? "ON" : "OFF");
}

// เรียกจาก SW: บังคับ relay ตัวนั้นกลับสู่ Manual mode แล้วสลับสถานะ (สวิตช์ทางกายคุมได้เสมอ)
inline void relayController_manualToggleFromSwitch(RelayConfig &r) {
  r.mode = RELAY_MODE_MANUAL;
  relayController_apply(r, !r.state);
}

// เรียกจากเว็บ: ตั้งสถานะ Manual โดยตรง (ใช้ตอนกดปุ่ม ON/OFF บนหน้าเว็บ)
inline void relayController_setManualState(RelayConfig &r, bool newState) {
  r.mode = RELAY_MODE_MANUAL;
  relayController_apply(r, newState);
}

static bool relayController_getNtpTime(struct tm &timeInfo) {
  return getLocalTime(&timeInfo, 100);
}

// ตรวจสอบว่าเวลาปัจจุบันอยู่ในช่วง on-time หรือไม่ รองรับช่วงที่ข้ามเที่ยงคืน
static bool relayController_isWithinSchedule(const RelayConfig &r, const struct tm &timeInfo) {
  int nowMinutes = timeInfo.tm_hour * 60 + timeInfo.tm_min;
  int onMinutes = r.scheduleOnHour * 60 + r.scheduleOnMinute;
  int offMinutes = r.scheduleOffHour * 60 + r.scheduleOffMinute;

  if (onMinutes == offMinutes) return false;

  if (onMinutes < offMinutes) {
    return nowMinutes >= onMinutes && nowMinutes < offMinutes;
  }
  // ช่วงข้ามเที่ยงคืน เช่น 22:00 - 06:00
  return nowMinutes >= onMinutes || nowMinutes < offMinutes;
}

static void relayController_evaluateThreshold(RelayConfig &r) {
  if (!latestDhtData.valid) return;

  float value = (r.thresholdMetric == THRESHOLD_METRIC_TEMPERATURE)
                    ? latestDhtData.temperature
                    : latestDhtData.humidity;

  if (!r.state && value >= r.thresholdTurnOnAt) {
    relayController_apply(r, true);
  } else if (r.state && value <= r.thresholdTurnOffAt) {
    relayController_apply(r, false);
  }
}

static void relayController_evaluateSchedule(RelayConfig &r) {
  struct tm timeInfo;
  if (!relayController_getNtpTime(timeInfo)) return;

  bool shouldBeOn = relayController_isWithinSchedule(r, timeInfo);
  relayController_apply(r, shouldBeOn);
}

inline void relayController_setup() {
  for (uint8_t i = 0; i < 3; i++) {
    digitalWrite(relayConfigs[i].pin, RELAY_OFF);
    pinMode(relayConfigs[i].pin, OUTPUT);
  }
}

#define RELAY_AUTO_EVAL_INTERVAL_MS 2000UL
static unsigned long relayControllerLastEvalTime = 0;

inline void relayController_loop() {
  unsigned long now = millis();
  if (now - relayControllerLastEvalTime < RELAY_AUTO_EVAL_INTERVAL_MS && relayControllerLastEvalTime != 0) return;
  relayControllerLastEvalTime = now;

  for (uint8_t i = 0; i < 3; i++) {
    RelayConfig &r = relayConfigs[i];
    if (r.mode == RELAY_MODE_THRESHOLD) {
      relayController_evaluateThreshold(r);
    } else if (r.mode == RELAY_MODE_SCHEDULE) {
      relayController_evaluateSchedule(r);
    }
  }
}
