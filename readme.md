# ESP32 Devkit V2 - Smart Home

ระบบควบคุมบ้านอัจฉริยะบน ESP32 Devkit V2 ควบคุมรีเลย์ 3 ช่องผ่านสวิตช์ทางกาย/เว็บ/MQTT พร้อมแสดงผลสภาพอากาศ (OpenWeather), เซนเซอร์ DHT11, จอ OLED และเว็บ Dashboard ในตัว

## สารบัญ

1. [ภาพรวมระบบ](#1-ภาพรวมระบบ)
2. [ตารางขา GPIO ทั้งหมด](#2-ตารางขา-gpio-ทั้งหมด)
3. [ไลบรารีที่ต้องใช้](#3-ไลบรารีที่ต้องใช้)
4. [ขั้นตอนตั้งค่าก่อนใช้งาน](#4-ขั้นตอนตั้งค่าก่อนใช้งาน)
5. [โครงสร้างไฟล์ในโปรเจกต์](#5-โครงสร้างไฟล์ในโปรเจกต์)
6. [วิธีใช้งานแต่ละฟีเจอร์](#6-วิธีใช้งานแต่ละฟีเจอร์)
7. [เว็บ Dashboard](#7-เว็บ-dashboard)
8. [MQTT (HiveMQ Public Broker)](#8-mqtt-hivemq-public-broker)
9. [ข้อควรระวังด้านความปลอดภัย](#9-ข้อควรระวังด้านความปลอดภัย)

---

## 1. ภาพรวมระบบ

| ฟีเจอร์ | รายละเอียดโดยย่อ |
|---|---|
| ควบคุม Relay 1-3 | ผ่านสวิตช์ทางกาย (SW1-3), เว็บ Dashboard, และ MQTT |
| โหมดการทำงานของ Relay | Manual / Threshold (ตาม Temp-Hum จาก DHT11) / Schedule (ตามเวลา NTP) |
| แสดงสภาพอากาศ | ดึงจาก OpenWeather API (Temp, Hum, Wind, สภาพอากาศ, พยากรณ์ฝน, AQI, PM2.5) |
| เซนเซอร์ DHT11 | อ่านค่า Temp/Hum จริง พร้อมโหมดสุ่มค่าจำลอง (SIM) หากไม่ได้ต่อเซนเซอร์ |
| จอ OLED (SSD1306) | แสดงผล 3 หน้าสลับอัตโนมัติ: สภาพแวดล้อม, สถานะระบบ, DHT11 |
| LED สถานะ | กระพริบเมื่อเชื่อมต่อ WiFi สำเร็จ |
| WiFi | ตั้งค่าผ่าน WiFiManager (ไม่ต้อง hardcode SSID/Password) |
| รีเซ็ต WiFi | กด SW1 ค้าง 5 วินาที |
| เว็บ Dashboard | ควบคุม Relay และตั้งค่าโหมดอัตโนมัติผ่านเบราว์เซอร์ |
| MQTT | เชื่อมต่อ broker.hivemq.com (public) รับ-ส่งข้อมูลและคำสั่งควบคุม |

---

## 2. ตารางขา GPIO ทั้งหมด

### Relay Module (Active LOW)

ส่งสัญญาณ `LOW` เพื่อสั่ง ON และ `HIGH` เพื่อสั่ง OFF

| Relay | GPIO Pin | สถานะ ON | สถานะ OFF |
|-------|----------|----------|-----------|
| Relay 1 | GPIO17 | `LOW` | `HIGH` |
| Relay 2 | GPIO16 | `LOW` | `HIGH` |
| Relay 3 | GPIO4  | `LOW` | `HIGH` |

> ควรกำหนดค่าพินเป็น `HIGH` (OFF) ก่อนตั้ง `pinMode` เป็น `OUTPUT` เสมอ เพื่อป้องกันรีเลย์ทำงานโดยไม่ได้ตั้งใจตอนบูต (จัดการให้แล้วใน `relayController_setup()`)

### Switch ปุ่มกด (Active LOW, External Pull-up 10k)

| Switch | GPIO Pin | สถานะกด | สถานะปล่อย | ควบคุม |
|--------|----------|---------|-------------|---------|
| SW1 | GPIO34 | `LOW` | `HIGH` | Relay1 (Toggle) + กดค้าง 5 วิ = รีเซ็ต WiFi |
| SW2 | GPIO35 | `LOW` | `HIGH` | Relay2 (Toggle) |
| SW3 | GPIO32 | `LOW` | `HIGH` | Relay3 (Toggle) |

> GPIO34/35 เป็น Input-only ไม่มี Pull-up ภายใน ต้องต่อ R Pull-up 10k ภายนอกทุกขา (รวมถึง GPIO32 เพื่อความสอดคล้องกัน)

### LED สถานะ (Active HIGH)

| LED | GPIO Pin | ความหมาย |
|-----|----------|----------|
| LED Status | GPIO2 | กระพริบทุก 2 วิเมื่อเชื่อมต่อ WiFi สำเร็จ / ดับถ้ายังไม่เชื่อมต่อ |

### DHT11 Sensor (Single-wire)

| Sensor | GPIO Pin | ค่าที่วัด |
|--------|----------|-----------|
| DHT11 | GPIO15 | อุณหภูมิ (°C), ความชื้นสัมพัทธ์ (%RH) |

> ต้องต่อ R Pull-up 4.7k–10k โอห์มจาก Data ไป VCC หากโมดูลไม่มีในตัว อ่านค่าไม่เกิน 1 ครั้ง/วินาที (โค้ดตั้งไว้ 2 วินาที/ครั้ง)

### จอ OLED SSD1306 (I2C, 128x64)

| Device | SDA | SCL | I2C Address |
|--------|-----|-----|-------------|
| OLED | GPIO21 | GPIO22 | `0x3C` |

### RS232/RS485 (MAX13487) — Serial0/UART0

| Device | TX | RX | หมายเหตุ |
|--------|----|----|----------|
| MAX13487 | GPIO1 | GPIO3 | ใช้ร่วมกับพอร์ต USB Debug/Flash ต้องถอดสายก่อนอัปโหลดโปรแกรม |

### Isolated Input / Opto Isolator (Active LOW, รองรับสูงสุด 12V)

| Isolated Input | GPIO Pin |
|-----------------|----------|
| ISO1 | GPIO33 |
| ISO2 | GPIO27 |

> ยังไม่มีโค้ดใช้งานส่วนนี้ในโปรเจกต์ปัจจุบัน (เตรียมขาไว้เผื่อขยายในอนาคต)

---

## 3. ไลบรารีที่ต้องใช้

ระบุไว้ใน [platformio.ini](platformio.ini) แล้ว — PlatformIO จะติดตั้งให้อัตโนมัติเมื่อ build ครั้งแรก:

| ไลบรารี | ใช้สำหรับ |
|---|---|
| `bblanchon/ArduinoJson` | แปลง JSON (OpenWeather API, เว็บ Dashboard API) |
| `tzapu/WiFiManager` | ตั้งค่า WiFi ผ่าน Access Point แทนการ hardcode |
| `adafruit/Adafruit SSD1306` + `Adafruit GFX Library` | ควบคุมจอ OLED |
| `adafruit/DHT sensor library` + `Adafruit Unified Sensor` | อ่านค่า DHT11 |
| `ESP32Async/ESPAsyncWebServer` + `AsyncTCP` | เว็บเซิร์ฟเวอร์ Dashboard |
| `knolleary/PubSubClient` | เชื่อมต่อ MQTT |

---

## 4. ขั้นตอนตั้งค่าก่อนใช้งาน

### ขั้นตอนที่ 1: ตั้งค่า OpenWeather API

เปิดไฟล์ [src/WeatherMonitor.h](src/WeatherMonitor.h) แล้วแก้ไข:

```cpp
#define OPENWEATHER_API_KEY "YOUR_OPENWEATHER_API_KEY"   // สมัครฟรีที่ openweathermap.org
#define OPENWEATHER_CITY "Nakhon Si Thammarat"            // ชื่อจังหวัด/เมือง (รองรับช่องว่าง)
#define OPENWEATHER_COUNTRY_CODE "TH"                     // รหัสประเทศ ISO 3166 alpha-2
```

### ขั้นตอนที่ 2: Build และอัปโหลดโปรแกรม

```bash
pio run --target upload
pio device monitor
```

Serial Monitor ตั้งไว้ที่ **115200 baud**

### ขั้นตอนที่ 3: ตั้งค่า WiFi ผ่าน WiFiManager

1. บอร์ดบูตครั้งแรก (หรือหลังรีเซ็ต WiFi) จะเปิด Access Point ชื่อ **`ESP32-SmartHome-Setup`**
2. เชื่อมต่อมือถือ/คอมเข้า WiFi นี้ จะมีหน้าตั้งค่าเด้งขึ้นอัตโนมัติ (หรือเปิดเบราว์เซอร์ไปที่ `192.168.4.1`)
3. เลือก WiFi บ้าน/สำนักงาน ใส่รหัสผ่าน แล้วบันทึก
4. บอร์ดจะรีสตาร์ทและเชื่อมต่อ WiFi ที่ตั้งไว้ — IP Address จะแสดงบน Serial Monitor และหน้าจอ OLED (หน้า 2)

> **รีเซ็ต WiFi ใหม่:** กดปุ่ม **SW1 ค้างไว้ 5 วินาที** ระบบจะนับถอยหลังใน Serial Monitor และหน้าจอ OLED เมื่อครบเวลาจะลบค่า WiFi เดิมและรีสตาร์ทเข้าสู่โหมดตั้งค่าใหม่

### ขั้นตอนที่ 4 (ถ้าต้องการ): ตั้งค่า MQTT

ดูรายละเอียดที่ [หัวข้อ 8](#8-mqtt-hivemq-public-broker) — ค่าเริ่มต้นใช้งานได้ทันทีไม่ต้องแก้ไขอะไร

---

## 5. โครงสร้างไฟล์ในโปรเจกต์

| ไฟล์ | หน้าที่ |
|---|---|
| [src/main.cpp](src/main.cpp) | จุดเริ่มโปรแกรม, จัดการสวิตช์ทางกาย, LED สถานะ, WiFi Reset |
| [src/RelayController.h](src/RelayController.h) | จัดการสถานะ/โหมดของ Relay ทั้ง 3 ตัว (Manual/Threshold/Schedule) |
| [src/WeatherMonitor.h](src/WeatherMonitor.h) | เชื่อมต่อ WiFi (WiFiManager), ดึงข้อมูล OpenWeather, ซิงก์เวลา NTP |
| [src/DhtSensor.h](src/DhtSensor.h) | อ่านค่า DHT11 พร้อมโหมดจำลองค่า (SIM) |
| [src/DisplayManager.h](src/DisplayManager.h) | ควบคุมจอ OLED แบบ 3 หน้า |
| [src/WebDashboard.h](src/WebDashboard.h) | เว็บเซิร์ฟเวอร์ Dashboard + REST API |
| [src/MqttClient.h](src/MqttClient.h) | เชื่อมต่อ MQTT (HiveMQ Public Broker) |
| [platformio.ini](platformio.ini) | ตั้งค่าบอร์ดและไลบรารี |

---

## 6. วิธีใช้งานแต่ละฟีเจอร์

### 6.1 ควบคุม Relay ผ่านสวิตช์ทางกาย

กด SW1 / SW2 / SW3 เพื่อ Toggle Relay1 / Relay2 / Relay3 ตามลำดับ — **สวิตช์ทางกายควบคุมได้เสมอ** ไม่ว่ารีเลย์ตัวนั้นจะอยู่โหมด Threshold หรือ Schedule อยู่ก่อนก็ตาม (การกดจะบังคับสลับกลับเป็น Manual mode ทันที)

### 6.2 โหมดการทำงานของ Relay

ตั้งค่าได้ผ่านเว็บ Dashboard เท่านั้น (ดูหัวข้อ 7) มี 3 โหมดต่อ Relay:

| โหมด | คำอธิบาย |
|---|---|
| **Manual** | เปิด/ปิดด้วยตัวเอง ผ่านปุ่มบนเว็บ/สวิตช์/MQTT |
| **Threshold** | เปิดอัตโนมัติเมื่อค่า Temp หรือ Hum (จาก DHT11) ถึงค่าที่ตั้งไว้ (Turn ON at) และปิดเมื่อค่าลดลงถึงอีกค่าหนึ่ง (Turn OFF at) — มี hysteresis กันรีเลย์สวิตช์ถี่เกินไป |
| **Schedule** | เปิด-ปิดตามช่วงเวลาที่ตั้งไว้ (รองรับช่วงเวลาที่ข้ามเที่ยงคืน เช่น 22:00–06:00) อ้างอิงเวลาจาก NTP (`pool.ntp.org`, UTC+7) |

### 6.3 จอ OLED (สลับหน้าอัตโนมัติทุก 5 วินาที)

**หน้า 1 — Environment:** Temp, Hum, Wind, สภาพอากาศ, พยากรณ์ฝน, AQI, PM2.5 (จาก OpenWeather)

**หน้า 2 — System Status:** สถานะ WiFi, IP Address, สถานะการรีเซ็ต WiFi (นับถอยหลัง), สถานะ SW/Relay ทั้ง 3 ช่อง

**หน้า 3 — DHT11 Sensor:** Temp/Hum ตัวเลขใหญ่ พร้อม badge `[SIM]` เมื่อใช้ค่าจำลอง (ไม่ได้ต่อเซนเซอร์จริง หรืออ่านค่าล้มเหลวติดต่อกัน 3 ครั้ง)

### 6.4 โหมดจำลองค่า DHT11 (SIM Mode)

หากไม่ได้ต่อเซนเซอร์ DHT11 จริง ระบบจะตรวจพบและสลับไปใช้ค่าสุ่มโดยอัตโนมัติ (Temp 25.0–35.0°C, Hum 40–80%) เพื่อให้ทดสอบฟีเจอร์อื่น (Threshold mode, เว็บ, MQTT) ได้โดยไม่ต้องมีฮาร์ดแวร์ครบ — สังเกตได้จาก badge `SIM` บนจอ OLED และเว็บ Dashboard

---

## 7. เว็บ Dashboard

เข้าผ่านเบราว์เซอร์ที่ **`http://<IP Address ของบอร์ด>/`** (ดู IP ได้จาก Serial Monitor หรือจอ OLED หน้า 2)

**แสดงผล:** สภาพอากาศ (OpenWeather), DHT11 (พร้อม SIM badge), สถานะ WiFi/IP/เวลา, สถานะ MQTT พร้อมรายการ Topic ทั้งหมด

**ควบคุมได้ต่อ Relay:**
- ปุ่ม **Turn ON / Turn OFF** — สั่งงานทันที (สลับเป็น Manual mode อัตโนมัติ)
- เลือกโหมด **Manual / Threshold / Schedule**
- ตั้งค่า Threshold: เลือก metric (Temp/Hum), ค่า Turn ON at, ค่า Turn OFF at
- ตั้งค่า Schedule: เวลาเปิด, เวลาปิด

หน้าเว็บรีเฟรชข้อมูลอัตโนมัติทุก 5 วินาที รองรับ Dark Mode ตามธีมเบราว์เซอร์

### REST API

| Endpoint | Method | หน้าที่ |
|---|---|---|
| `/` | GET | หน้าเว็บ Dashboard |
| `/api/status` | GET | คืนค่าสถานะทั้งหมดเป็น JSON |
| `/api/relay` | POST | ตั้งโหมด/ค่า/สถานะของ Relay (ดู body ตัวอย่างด้านล่าง) |

ตัวอย่าง body สำหรับ `/api/relay`:
```json
{ "relay": 0, "mode": 0, "state": true }
{ "relay": 1, "mode": 1, "thresholdMetric": 0, "turnOnAt": 30.0, "turnOffAt": 28.0 }
{ "relay": 2, "mode": 2, "onHour": 18, "onMinute": 0, "offHour": 6, "offMinute": 0 }
```
(`relay`: 0=Relay1, 1=Relay2, 2=Relay3 / `mode`: 0=Manual, 1=Threshold, 2=Schedule)

---

## 8. MQTT (HiveMQ Public Broker)

ใช้ **`broker.hivemq.com`** พอร์ต **1883** — broker สาธารณะสำหรับทดสอบ **ไม่ต้อง login** ใช้งานได้ทันทีโดยไม่ต้องแก้ไขโค้ดใดๆ

เนื่องจากเป็น broker สาธารณะที่ทุกคนเห็น topic ได้ ระบบจึงต่อ **Chip ID** (จาก MAC Address ของบอร์ด) ต่อท้าย topic prefix โดยอัตโนมัติ เพื่อกันชนกับอุปกรณ์อื่น เช่น `smarthome/esp32/A1B2C3D4E5F6/...` — ดู Chip ID และ Topic เต็มของบอร์ดตัวเองได้จากการ์ด **"MQTT Topics"** ในเว็บ Dashboard

### โครงสร้าง Topic (`smarthome/esp32/<chipId>/...`)

| หมวดหมู่ | Topic | ทิศทาง | ตัวอย่างค่า |
|---|---|---|---|
| สภาพอากาศ | `weather/temp`, `weather/hum`, `weather/wind`, `weather/desc`, `weather/rain`, `weather/aqi`, `weather/pm25` | Publish | `32.9`, `51`, `5.6`, `Clouds`, `0`/`1`, `1`, `2.5` |
| DHT11 | `dht/temp`, `dht/hum`, `dht/sim` | Publish | `32.9`, `51`, `0`/`1` |
| ระบบ | `system/ip`, `system/wifi` | Publish | `192.168.1.50`, `connected` |
| สถานะ Relay | `relay1/state`, `relay2/state`, `relay3/state` | Publish (retained) | `ON` / `OFF` |
| คำสั่ง Relay | `relay1/set`, `relay2/set`, `relay3/set` | **Subscribe** | ส่ง `ON` หรือ `OFF` เพื่อสั่งงาน |

- ข้อมูล publish ทุก **10 วินาที**
- ส่งคำสั่งไปที่ topic `.../relayX/set` (payload `ON`/`OFF`/`1`/`0`/`TRUE`) จะสั่งงาน Relay ทันทีและสลับเป็น Manual mode
- เชื่อมต่อหลุดจะพยายาม reconnect อัตโนมัติทุก 5 วินาที

---

## 9. ข้อควรระวังด้านความปลอดภัย

- **`src/WeatherMonitor.h`** มี OpenWeather API Key ฝังในโค้ด — หากจะ push repo ขึ้น public ควรแยกออกเป็นไฟล์ `secrets.h` แล้วเพิ่มใน `.gitignore`
- **MQTT ผ่าน `broker.hivemq.com`** ไม่มีการเข้ารหัสหรือ authentication ใดๆ ข้อมูลและคำสั่งควบคุม Relay ส่งแบบ plain text ให้ทุกคนที่รู้ topic เห็น/สั่งงานได้ **เหมาะกับ demo/ทดสอบเท่านั้น ไม่ควรใช้ควบคุมอุปกรณ์จริงในบ้านที่ต้องการความปลอดภัย** หากต้องการความปลอดภัยจริงจัง ให้เปลี่ยนไปใช้ HiveMQ Cloud แบบสมัครบัญชี (รองรับ TLS + username/password)
- **เว็บ Dashboard** (`/`, `/api/*`) ไม่มีระบบ login ใครก็ตามที่อยู่ในเครือข่าย WiFi เดียวกันเข้าควบคุม Relay ได้ทันที ควรใช้ในเครือข่ายบ้าน/ส่วนตัวที่เชื่อถือได้เท่านั้น
- Serial0 (GPIO1/GPIO3 สำหรับ RS232/RS485) ใช้ร่วมกับพอร์ต USB Debug/Flash ต้องถอดสาย RS232/RS485 ก่อนอัปโหลดโปรแกรมทุกครั้ง
