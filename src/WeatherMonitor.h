#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// ---------- OpenWeather ----------
#define OPENWEATHER_API_KEY "1bef650d2c6ea7a91f58252948c2d325"

// ชื่อจังหวัด/เมือง และรหัสประเทศ (ISO 3166 อัลฟา-2) ตามรูปแบบ OpenWeather "q=city,countryCode"
#define OPENWEATHER_CITY "Nakhon Si Thammarat"
#define OPENWEATHER_COUNTRY_CODE "TH"

#define WEATHER_READ_INTERVAL_MS 10000UL

static unsigned long lastWeatherReadTime = 0;

// ---------- ค่าล่าสุดที่อ่านได้ (ให้โมดูลอื่น เช่น OLED นำไปแสดงผลได้) ----------
struct WeatherData {
  bool weatherValid = false;
  float temperature = NAN;
  float humidity = NAN;
  float windSpeed = NAN;
  String weatherDesc = "N/A";
  bool rainExpected = false;

  bool airQualityValid = false;
  int aqi = 0;
  float pm25 = NAN;
};

static WeatherData latestWeatherData;

// เข้ารหัส URL ให้ชื่อจังหวัดที่มีช่องว่าง/อักขระพิเศษ ใช้เป็น query string ได้อย่างปลอดภัย
inline String weatherMonitor_urlEncode(const String &value) {
  String encoded;
  encoded.reserve(value.length() * 3);

  for (size_t i = 0; i < value.length(); i++) {
    char c = value[i];
    if (isalnum((unsigned char)c) || c == '-' || c == '_' || c == '.' || c == '~') {
      encoded += c;
    } else if (c == ' ') {
      encoded += "%20";
    } else {
      char buf[4];
      snprintf(buf, sizeof(buf), "%%%02X", (unsigned char)c);
      encoded += buf;
    }
  }

  return encoded;
}

inline String weatherMonitor_cityQuery() {
  return weatherMonitor_urlEncode(String(OPENWEATHER_CITY) + "," + String(OPENWEATHER_COUNTRY_CODE));
}

// ตั้ง Access Point ชั่วคราวชื่อนี้เมื่อยังไม่เคยตั้งค่า WiFi หรือหลัง Reset
#define WIFIMANAGER_AP_NAME "ESP32-SmartHome-Setup"

// เวลาประเทศไทย UTC+7 ไม่มี Daylight Saving Time
#define NTP_GMT_OFFSET_SEC 25200
#define NTP_DAYLIGHT_OFFSET_SEC 0
#define NTP_SERVER "pool.ntp.org"

static bool ntpTimeSynced = false;

inline void weatherMonitor_connectWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;

  WiFiManager wm;
  Serial.println("Starting WiFiManager...");

  bool connected = wm.autoConnect(WIFIMANAGER_AP_NAME);

  if (connected) {
    Serial.println("WiFi connected!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());

    if (!ntpTimeSynced) {
      configTime(NTP_GMT_OFFSET_SEC, NTP_DAYLIGHT_OFFSET_SEC, NTP_SERVER);
      ntpTimeSynced = true;
      Serial.println("[NTP] Time sync started");
    }
  } else {
    Serial.println("WiFi connect failed / config portal timeout.");
  }
}

// ลบค่า WiFi ที่บันทึกไว้ แล้วรีสตาร์ทเพื่อเข้าสู่โหมดตั้งค่าใหม่ (Access Point)
inline void weatherMonitor_resetWiFiSettings() {
  Serial.println("Resetting WiFi settings...");
  WiFiManager wm;
  wm.resetSettings();
  Serial.println("WiFi settings cleared. Restarting...");
  delay(500);
  ESP.restart();
}

// อ่านค่า Temp, Humidity, ลม, และพยากรณ์ฝนตกในช่วง 3 ชม.ถัดไปจาก /data/2.5/weather
// พร้อมคืนพิกัด lat/lon ของเมืองที่ค้นเจอ ไว้ใช้เรียก /air_pollution ต่อ (endpoint นี้ไม่รองรับชื่อเมือง)
inline bool weatherMonitor_fetchCurrentWeather(float &temperature, float &humidity,
                                                float &windSpeed, String &weatherDesc,
                                                bool &rainExpected, float &outLat, float &outLon) {
  HTTPClient http;
  String url = "http://api.openweathermap.org/data/2.5/weather?q=" + weatherMonitor_cityQuery() +
               "&appid=" + String(OPENWEATHER_API_KEY) +
               "&units=metric&lang=en";

  http.begin(url);
  int httpCode = http.GET();

  if (httpCode != HTTP_CODE_OK) {
    Serial.print("[Weather] HTTP error: ");
    Serial.println(httpCode);
    http.end();
    return false;
  }

  String payload = http.getString();
  http.end();

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, payload);
  if (err) {
    Serial.print("[Weather] JSON parse error: ");
    Serial.println(err.c_str());
    return false;
  }

  temperature = doc["main"]["temp"] | NAN;
  humidity = doc["main"]["humidity"] | NAN;
  windSpeed = doc["wind"]["speed"] | NAN;
  weatherDesc = doc["weather"][0]["description"] | "N/A";
  outLat = doc["coord"]["lat"] | NAN;
  outLon = doc["coord"]["lon"] | NAN;

  // รหัสสภาพอากาศกลุ่ม 2xx,3xx,5xx = ฝน/พายุ (ตาม OpenWeather condition codes)
  int weatherId = doc["weather"][0]["id"] | 0;
  rainExpected = (weatherId < 700);

  return true;
}

// อ่านค่า AQI และ PM2.5 จาก /data/2.5/air_pollution (ต้องใช้ lat/lon ที่ได้จาก current weather)
inline bool weatherMonitor_fetchAirQuality(float lat, float lon, int &aqi, float &pm25) {
  HTTPClient http;
  String url = "http://api.openweathermap.org/data/2.5/air_pollution?lat=" + String(lat, 6) +
               "&lon=" + String(lon, 6) +
               "&appid=" + String(OPENWEATHER_API_KEY);

  http.begin(url);
  int httpCode = http.GET();

  if (httpCode != HTTP_CODE_OK) {
    Serial.print("[AirQuality] HTTP error: ");
    Serial.println(httpCode);
    http.end();
    return false;
  }

  String payload = http.getString();
  http.end();

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, payload);
  if (err) {
    Serial.print("[AirQuality] JSON parse error: ");
    Serial.println(err.c_str());
    return false;
  }

  JsonObject item = doc["list"][0];
  aqi = item["main"]["aqi"] | 0;   // 1=Good ... 5=Very Poor (ดัชนีของ OpenWeather เอง ไม่ใช่มาตรฐาน US AQI)
  pm25 = item["components"]["pm2_5"] | NAN;

  return true;
}

inline void weatherMonitor_printReport() {
  float temperature, humidity, windSpeed, lat, lon;
  String weatherDesc;
  bool rainExpected;
  int aqi;
  float pm25;

  bool weatherOk = weatherMonitor_fetchCurrentWeather(temperature, humidity, windSpeed, weatherDesc, rainExpected, lat, lon);
  bool airOk = weatherOk && weatherMonitor_fetchAirQuality(lat, lon, aqi, pm25);

  latestWeatherData.weatherValid = weatherOk;
  if (weatherOk) {
    latestWeatherData.temperature = temperature;
    latestWeatherData.humidity = humidity;
    latestWeatherData.windSpeed = windSpeed;
    latestWeatherData.weatherDesc = weatherDesc;
    latestWeatherData.rainExpected = rainExpected;
  }

  latestWeatherData.airQualityValid = airOk;
  if (airOk) {
    latestWeatherData.aqi = aqi;
    latestWeatherData.pm25 = pm25;
  }

  Serial.print("===== สภาพแวดล้อม: ");
  Serial.print(OPENWEATHER_CITY);
  Serial.println(" =====");
  if (weatherOk) {
    Serial.print("Temp: "); Serial.print(temperature); Serial.println(" C");
    Serial.print("Hum: "); Serial.print(humidity); Serial.println(" %");
    Serial.print("Wind: "); Serial.print(windSpeed); Serial.println(" m/s");
    Serial.print("สภาพอากาศ: "); Serial.println(weatherDesc);
    Serial.print("พยากรณ์ฝนตก: "); Serial.println(rainExpected ? "มีโอกาสฝนตก" : "ไม่มีฝน");
  } else {
    Serial.println("อ่านค่าสภาพอากาศล้มเหลว");
  }

  if (airOk) {
    Serial.print("AQI (1-5): "); Serial.println(aqi);
    Serial.print("PM2.5: "); Serial.print(pm25); Serial.println(" ug/m3");
  } else {
    Serial.println("อ่านค่าคุณภาพอากาศล้มเหลว");
  }
  Serial.println("========================================");
}

inline void weatherMonitor_setup() {
  weatherMonitor_connectWiFi();
}

inline void weatherMonitor_loop() {
  unsigned long now = millis();
  if (now - lastWeatherReadTime < WEATHER_READ_INTERVAL_MS && lastWeatherReadTime != 0) return;
  lastWeatherReadTime = now;

  weatherMonitor_connectWiFi();
  if (WiFi.status() == WL_CONNECTED) {
    weatherMonitor_printReport();
  } else {
    Serial.println("[Weather] WiFi ไม่ได้เชื่อมต่อ ข้ามการอ่านค่ารอบนี้");
  }
}
