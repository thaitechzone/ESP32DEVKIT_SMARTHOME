#pragma once

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include "WeatherMonitor.h"

#define OLED_SDA_PIN 21
#define OLED_SCL_PIN 22

#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 64
#define OLED_ADDRESS  0x3C

#define DISPLAY_PAGE_INTERVAL_MS   5000UL
#define DISPLAY_REFRESH_INTERVAL_MS 500UL

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

static bool displayReady = false;
static uint8_t displayCurrentPage = 0;
static unsigned long displayLastPageSwitchTime = 0;
static unsigned long displayLastRefreshTime = 0;

// สถานะจากภายนอก (main.cpp) ที่หน้าจอต้องใช้แสดงผล ถูกอัปเดตทุกรอบ loop ผ่าน displayManager_setStatus()
struct DisplayStatus {
  bool relay1On = false;
  bool relay2On = false;
  bool relay3On = false;
  bool sw1Pressed = false;
  bool sw2Pressed = false;
  bool sw3Pressed = false;
  bool wifiResetHolding = false;
  unsigned long wifiResetRemainingSec = 0;
};

static DisplayStatus displayStatus;

inline void displayManager_setStatus(const DisplayStatus &status) {
  displayStatus = status;
}

inline void displayManager_setup() {
  Wire.begin(OLED_SDA_PIN, OLED_SCL_PIN);

  displayReady = display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS);
  if (!displayReady) {
    Serial.println("[Display] เชื่อมต่อจอ OLED ล้มเหลว");
    return;
  }

  display.setTextColor(SSD1306_WHITE);
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 24);
  display.println("ESP32 Smart Home");
  display.println("Booting...");
  display.display();
}

// วาดเส้นหัวข้อ + ชื่อหน้าจอ พร้อมตัวเลขหน้า (เช่น "1/2") มุมขวาบน
static void displayManager_drawHeader(const char *title, uint8_t pageNum, uint8_t totalPages) {
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print(title);

  char pageLabel[8];
  snprintf(pageLabel, sizeof(pageLabel), "%u/%u", pageNum, totalPages);
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(pageLabel, 0, 0, &x1, &y1, &w, &h);
  display.setCursor(SCREEN_WIDTH - w, 0);
  display.print(pageLabel);

  display.drawFastHLine(0, 10, SCREEN_WIDTH, SSD1306_WHITE);
}

// Page 1: สภาพแวดล้อม (Temp, Hum, Wind, สภาพอากาศ, พยากรณ์ฝน, AQI, PM2.5)
static void displayManager_drawEnvironmentPage() {
  displayManager_drawHeader("Environment", 1, 2);

  display.setCursor(0, 14);

  if (latestWeatherData.weatherValid) {
    display.print("Temp: ");
    display.print(latestWeatherData.temperature, 1);
    display.println(" C");

    display.print("Hum:  ");
    display.print(latestWeatherData.humidity, 0);
    display.println(" %");

    display.print("Wind: ");
    display.print(latestWeatherData.windSpeed, 1);
    display.println(" m/s");

    display.print("Sky:  ");
    // ตัดข้อความให้พอดีกับความกว้างจอ (128px, text size 1 = ~6px/ตัวอักษร) กัน overflow ล้นขอบ
    String sky = latestWeatherData.weatherDesc;
    const uint8_t maxSkyChars = 15;
    if (sky.length() > maxSkyChars) {
      sky = sky.substring(0, maxSkyChars);
    }
    display.println(sky);

    display.print("Rain: ");
    display.println(latestWeatherData.rainExpected ? "Expected" : "None");
  } else {
    display.println("Weather: N/A");
    display.println();
    display.println();
    display.println();
    display.println();
  }

  display.drawFastHLine(0, 46, SCREEN_WIDTH, SSD1306_WHITE);
  display.setCursor(0, 50);

  if (latestWeatherData.airQualityValid) {
    display.print("AQI:");
    display.print(latestWeatherData.aqi);
    display.print("/5 PM2.5:");
    display.print(latestWeatherData.pm25, 1);
  } else {
    display.print("Air Quality: N/A");
  }
}

// Page 2: สถานะระบบ (WiFi/IP, WiFi Reset, SW/Relay)
static void displayManager_drawSystemPage() {
  displayManager_drawHeader("System Status", 2, 2);

  display.setCursor(0, 14);

  display.print("WiFi: ");
  if (WiFi.status() == WL_CONNECTED) {
    display.println("Connected");
    display.print("IP: ");
    display.println(WiFi.localIP());
  } else {
    display.println("Disconnected");
    display.println("IP: -");
  }

  if (displayStatus.wifiResetHolding) {
    display.print("Reset in: ");
    display.print(displayStatus.wifiResetRemainingSec);
    display.println("s");
  } else {
    display.println("Reset: hold SW1 5s");
  }

  display.drawFastHLine(0, 40, SCREEN_WIDTH, SSD1306_WHITE);

  display.setCursor(0, 44);
  display.print("SW    [");
  display.print(displayStatus.sw1Pressed ? "P" : "-");
  display.print("] [");
  display.print(displayStatus.sw2Pressed ? "P" : "-");
  display.print("] [");
  display.print(displayStatus.sw3Pressed ? "P" : "-");
  display.print("]");

  display.setCursor(0, 54);
  display.print("Relay [");
  display.print(displayStatus.relay1On ? "ON" : "--");
  display.print("][");
  display.print(displayStatus.relay2On ? "ON" : "--");
  display.print("][");
  display.print(displayStatus.relay3On ? "ON" : "--");
  display.print("]");
}

inline void displayManager_loop() {
  if (!displayReady) return;

  unsigned long now = millis();
  if (now - displayLastPageSwitchTime >= DISPLAY_PAGE_INTERVAL_MS) {
    displayLastPageSwitchTime = now;
    displayCurrentPage = (displayCurrentPage + 1) % 2;
  }

  // จำกัดอัตราการวาดจอ (2 fps) กัน I2C bus ทำงานหนักเกินจำเป็นในทุกรอบ loop()
  if (now - displayLastRefreshTime < DISPLAY_REFRESH_INTERVAL_MS) return;
  displayLastRefreshTime = now;

  display.clearDisplay();

  if (displayCurrentPage == 0) {
    displayManager_drawEnvironmentPage();
  } else {
    displayManager_drawSystemPage();
  }

  display.display();
}
