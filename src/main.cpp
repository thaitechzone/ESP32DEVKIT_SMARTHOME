#include <Arduino.h>
#include "WeatherMonitor.h"

// ---------- Relay (Active LOW) ----------
#define RELAY1_PIN 17
#define RELAY2_PIN 16
#define RELAY3_PIN 4

#define RELAY_ON  LOW
#define RELAY_OFF HIGH

// ---------- Switch (Active LOW, External Pull-up) ----------
#define SW1_PIN 34
#define SW2_PIN 35
#define SW3_PIN 32

#define SW_PRESSED  LOW
#define SW_RELEASED HIGH

#define DEBOUNCE_MS 50

struct ToggleSwitch {
  const char *name;
  uint8_t swPin;
  uint8_t relayPin;
  bool relayState;      // true = ON
  int lastReading;       // last raw reading
  int stableState;       // debounced state
  unsigned long lastDebounceTime;
};

ToggleSwitch sw1 = { "SW1/Relay1", SW1_PIN, RELAY1_PIN, false, SW_RELEASED, SW_RELEASED, 0 };
ToggleSwitch sw2 = { "SW2/Relay2", SW2_PIN, RELAY2_PIN, false, SW_RELEASED, SW_RELEASED, 0 };
ToggleSwitch sw3 = { "SW3/Relay3", SW3_PIN, RELAY3_PIN, false, SW_RELEASED, SW_RELEASED, 0 };

void updateSwitch(ToggleSwitch &s) {
  int reading = digitalRead(s.swPin);

  if (reading != s.lastReading) {
    s.lastDebounceTime = millis();
  }

  if ((millis() - s.lastDebounceTime) > DEBOUNCE_MS) {
    if (reading != s.stableState) {
      int previousStableState = s.stableState;
      s.stableState = reading;

      // Toggle relay only on press edge (RELEASED -> PRESSED)
      if (previousStableState == SW_RELEASED && s.stableState == SW_PRESSED) {
        s.relayState = !s.relayState;
        digitalWrite(s.relayPin, s.relayState ? RELAY_ON : RELAY_OFF);

        Serial.print(s.name);
        Serial.print(" -> ");
        Serial.println(s.relayState ? "ON" : "OFF");
      }
    }
  }

  s.lastReading = reading;
}

void setup() {
  Serial.begin(115200);

  // กำหนดค่าพินเป็น OFF ก่อนตั้งค่า pinMode เป็น OUTPUT ป้องกันรีเลย์ทำงานโดยไม่ได้ตั้งใจ
  digitalWrite(RELAY1_PIN, RELAY_OFF);
  digitalWrite(RELAY2_PIN, RELAY_OFF);
  digitalWrite(RELAY3_PIN, RELAY_OFF);

  pinMode(RELAY1_PIN, OUTPUT);
  pinMode(RELAY2_PIN, OUTPUT);
  pinMode(RELAY3_PIN, OUTPUT);

  pinMode(SW1_PIN, INPUT);
  pinMode(SW2_PIN, INPUT);
  pinMode(SW3_PIN, INPUT);

  Serial.println("=== ESP32 Smart Home: Relay Toggle Ready ===");
  Serial.println("SW1/Relay1 -> OFF");
  Serial.println("SW2/Relay2 -> OFF");
  Serial.println("SW3/Relay3 -> OFF");

  weatherMonitor_setup();
}

void loop() {
  updateSwitch(sw1);
  updateSwitch(sw2);
  updateSwitch(sw3);

  weatherMonitor_loop();
}
