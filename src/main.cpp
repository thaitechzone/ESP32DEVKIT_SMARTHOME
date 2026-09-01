#include <Arduino.h>
#include <WiFi.h>
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

// ---------- SW1 Long-press: WiFi Reset ----------
#define WIFI_RESET_HOLD_MS 5000UL

unsigned long sw1PressStartTime = 0;
bool sw1LongPressTriggered = false;
unsigned long sw1LastCountdownPrintSecond = 0;

// ---------- LED Status (Active HIGH) ----------
#define LED_STATUS_PIN 2

#define LED_ON  HIGH
#define LED_OFF LOW

#define LED_BLINK_INTERVAL_MS 2000UL

unsigned long lastLedToggleTime = 0;
bool ledState = false;

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

// ตรวจจับการกด SW1 ค้างไว้ >= WIFI_RESET_HOLD_MS วินาที เพื่อรีเซ็ตค่า WiFi
// แสดงผลนับถอยหลังใน Serial Monitor ระหว่างที่กดค้าง
void checkWiFiResetHold() {
  int reading = digitalRead(SW1_PIN);

  if (reading == SW_PRESSED) {
    if (sw1PressStartTime == 0) {
      sw1PressStartTime = millis();
      sw1LongPressTriggered = false;
      sw1LastCountdownPrintSecond = 0;
      Serial.println("[WiFi Reset] กด SW1 ค้างไว้ 5 วินาทีเพื่อรีเซ็ตค่า WiFi...");
    }

    unsigned long heldMs = millis() - sw1PressStartTime;

    if (!sw1LongPressTriggered && heldMs < WIFI_RESET_HOLD_MS) {
      unsigned long remainingSeconds = (WIFI_RESET_HOLD_MS - heldMs + 999) / 1000;
      if (remainingSeconds != sw1LastCountdownPrintSecond) {
        sw1LastCountdownPrintSecond = remainingSeconds;
        Serial.print("[WiFi Reset] นับถอยหลัง: ");
        Serial.print(remainingSeconds);
        Serial.println(" วินาที");
      }
    }

    if (!sw1LongPressTriggered && heldMs >= WIFI_RESET_HOLD_MS) {
      sw1LongPressTriggered = true;
      Serial.println("[WiFi Reset] ครบเวลา! กำลังรีเซ็ตค่า WiFi...");
      weatherMonitor_resetWiFiSettings();
    }
  } else {
    sw1PressStartTime = 0;
    sw1LongPressTriggered = false;
    sw1LastCountdownPrintSecond = 0;
  }
}

// กระพริบ LED ทุก 2 วินาทีเมื่อเชื่อมต่อ WiFi ได้ ปิดไฟถ้ายังไม่ได้เชื่อมต่อ
void updateStatusLed() {
  if (WiFi.status() != WL_CONNECTED) {
    if (ledState) {
      ledState = false;
      digitalWrite(LED_STATUS_PIN, LED_OFF);
    }
    lastLedToggleTime = millis();
    return;
  }

  if (millis() - lastLedToggleTime >= LED_BLINK_INTERVAL_MS) {
    lastLedToggleTime = millis();
    ledState = !ledState;
    digitalWrite(LED_STATUS_PIN, ledState ? LED_ON : LED_OFF);
  }
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

  digitalWrite(LED_STATUS_PIN, LED_OFF);
  pinMode(LED_STATUS_PIN, OUTPUT);

  Serial.println("=== ESP32 Smart Home: Relay Toggle Ready ===");
  Serial.println("SW1/Relay1 -> OFF");
  Serial.println("SW2/Relay2 -> OFF");
  Serial.println("SW3/Relay3 -> OFF");

  weatherMonitor_setup();
}

void loop() {
  updateStatusLed();
  checkWiFiResetHold();

  if (!sw1LongPressTriggered) {
    updateSwitch(sw1);
  }
  updateSwitch(sw2);
  updateSwitch(sw3);

  weatherMonitor_loop();
}
