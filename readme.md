# ESP32 Devkit V2 - Smart Home

## คุณลักษณะ Relay Module

บอร์ดควบคุมใช้ ESP32 Devkit V2 เชื่อมต่อกับรีเลย์ 3 ช่อง โดยทำงานแบบ **Active LOW** (ส่งสัญญาณ `LOW` เพื่อสั่ง ON และ `HIGH` เพื่อสั่ง OFF)

| Relay | GPIO Pin | โหมดทำงาน | สถานะ ON | สถานะ OFF |
|-------|----------|-----------|----------|-----------|
| Relay 1 | GPIO17 | Active LOW | `LOW` | `HIGH` |
| Relay 2 | GPIO16 | Active LOW | `LOW` | `HIGH` |
| Relay 3 | GPIO4  | Active LOW | `LOW` | `HIGH` |

### หมายเหตุ

- เนื่องจากเป็นโหมด Active LOW ควรกำหนดค่าพินเป็น `HIGH` (OFF) ตั้งแต่ตอนเริ่มต้น (`setup()`) ก่อนตั้งค่า `pinMode` เป็น `OUTPUT` เพื่อป้องกันรีเลย์ทำงานโดยไม่ได้ตั้งใจในช่วงบูต
- ตัวอย่างโค้ดกำหนดพิน:

```cpp
#define RELAY1_PIN 17
#define RELAY2_PIN 16
#define RELAY3_PIN 4

#define RELAY_ON  LOW
#define RELAY_OFF HIGH

void setup() {
  digitalWrite(RELAY1_PIN, RELAY_OFF);
  digitalWrite(RELAY2_PIN, RELAY_OFF);
  digitalWrite(RELAY3_PIN, RELAY_OFF);

  pinMode(RELAY1_PIN, OUTPUT);
  pinMode(RELAY2_PIN, OUTPUT);
  pinMode(RELAY3_PIN, OUTPUT);
}
```

## คุณลักษณะ Switch (ปุ่มกด)

บอร์ดควบคุมเชื่อมต่อกับสวิตช์ 3 ปุ่ม โดยทำงานแบบ **Active LOW** ผ่านวงจร **Pull-up 10k แบบภายนอก** (External Pull-up) เมื่อกดปุ่มสัญญาณจะเป็น `LOW` และเมื่อปล่อยปุ่มสัญญาณจะเป็น `HIGH`

| Switch | GPIO Pin | โหมดทำงาน | Pull-up | สถานะกด (Pressed) | สถานะปล่อย (Released) |
|--------|----------|-----------|---------|--------------------|-------------------------|
| SW1 | GPIO34 | Active LOW | External 10k | `LOW` | `HIGH` |
| SW2 | GPIO35 | Active LOW | External 10k | `LOW` | `HIGH` |
| SW3 | GPIO32 | Active LOW | External 10k | `LOW` | `HIGH` |

### หมายเหตุ

- GPIO34 และ GPIO35 เป็นพินแบบ **Input Only** (ไม่มีวงจร Pull-up/Pull-down ภายในชิป) จึงจำเป็นต้องต่อ **R Pull-up 10k จากภายนอก** เพื่อให้สถานะพินเป็น `HIGH` เมื่อไม่ได้กดปุ่ม
- GPIO32 มีวงจร Pull-up ภายในชิป แต่ในการเชื่อมต่อนี้ใช้ **R Pull-up 10k จากภายนอก** เช่นเดียวกันเพื่อความสอดคล้องกันของวงจรทั้ง 3 ปุ่ม จึงตั้งค่า `pinMode` เป็น `INPUT` ธรรมดา (ไม่ใช้ `INPUT_PULLUP`)
- ควรทำ Debounce (เช่น หน่วงเวลาอ่านค่าซ้ำ หรือใช้ไลบรารีปุ่มกด) เพื่อป้องกันสัญญาณกระเพื่อมจากการกดปุ่ม
- ตัวอย่างโค้ดกำหนดพิน:

```cpp
#define SW1_PIN 34
#define SW2_PIN 35
#define SW3_PIN 32

#define SW_PRESSED  LOW
#define SW_RELEASED HIGH

void setup() {
  pinMode(SW1_PIN, INPUT);
  pinMode(SW2_PIN, INPUT);
  pinMode(SW3_PIN, INPUT);
}

void loop() {
  if (digitalRead(SW1_PIN) == SW_PRESSED) {
    // SW1 ถูกกด
  }
}
```

## คุณลักษณะ LED Status

บอร์ดควบคุมมี LED สำหรับแสดงสถานะการทำงานของโปรแกรม เชื่อมต่อกับ **GPIO2** โดยทำงานแบบ **Active HIGH** (ส่งสัญญาณ `HIGH` เพื่อสั่งติด และ `LOW` เพื่อสั่งดับ)

| LED | GPIO Pin | โหมดทำงาน | สถานะติด (ON) | สถานะดับ (OFF) |
|-----|----------|-----------|----------------|------------------|
| LED Status | GPIO2 | Active HIGH | `HIGH` | `LOW` |

### หมายเหตุ

- GPIO2 เป็นพินที่ต่อกับ LED บนบอร์ด (Onboard LED) ของ ESP32 Devkit อยู่แล้วในหลายรุ่น จึงใช้เป็น LED แสดงสถานะได้โดยไม่ต้องต่อวงจรเพิ่มเติม
- GPIO2 เป็นหนึ่งใน Strapping Pin ของ ESP32 มีผลต่อโหมดการบูต ควรตรวจสอบสถานะพินนี้ในช่วง Boot/Flash หากมีการต่อวงจรเพิ่มเติมภายนอก
- ใช้สำหรับบอกสถานะการทำงานต่าง ๆ ของโปรแกรม เช่น สถานะการเชื่อมต่อ WiFi, สถานะการทำงานปกติ (Heartbeat), หรือสถานะข้อผิดพลาด (Error) โดยอาจกำหนดรูปแบบการกระพริบ (Blink Pattern) ที่แตกต่างกันสำหรับแต่ละสถานะ
- ตัวอย่างโค้ดกำหนดพิน:

```cpp
#define LED_STATUS_PIN 2

#define LED_ON  HIGH
#define LED_OFF LOW

void setup() {
  digitalWrite(LED_STATUS_PIN, LED_OFF);
  pinMode(LED_STATUS_PIN, OUTPUT);
}

void loop() {
  // ตัวอย่าง: กระพริบ LED บอกสถานะทำงานปกติ (Heartbeat)
  digitalWrite(LED_STATUS_PIN, LED_ON);
  delay(500);
  digitalWrite(LED_STATUS_PIN, LED_OFF);
  delay(500);
}
```

## คุณลักษณะ DHT11 Sensor

บอร์ดควบคุมเชื่อมต่อกับเซนเซอร์วัดอุณหภูมิและความชื้น **DHT11** ผ่านขา Data เพียงเส้นเดียว (Single-wire) ที่ **GPIO15**

| Sensor | GPIO Pin | โปรโตคอล | ค่าที่วัดได้ |
|--------|----------|-----------|---------------|
| DHT11 | GPIO15 | 1-Wire (Single-bus) | อุณหภูมิ (°C), ความชื้นสัมพัทธ์ (%RH) |

### หมายเหตุ

- ขา Data ของ DHT11 ต้องต่อ **R Pull-up ประมาณ 4.7k–10k โอห์ม** จาก Data ไปยัง VCC (หากโมดูลที่ใช้ไม่มีตัวต้านทาน Pull-up มาให้ในตัวอยู่แล้ว)
- DHT11 มีอัตราการอ่านค่าไม่เกิน **1 ครั้งต่อวินาที** ควรหน่วงเวลาอ่านค่าอย่างน้อย 1-2 วินาทีต่อการอ่านหนึ่งครั้ง
- ใช้งานร่วมกับไลบรารี `DHT sensor library` (Adafruit) หรือ `DHT.h` ในการอ่านค่า
- ตัวอย่างโค้ดกำหนดพิน:

```cpp
#include <DHT.h>

#define DHT_PIN  15
#define DHT_TYPE DHT11

DHT dht(DHT_PIN, DHT_TYPE);

void setup() {
  dht.begin();
}

void loop() {
  float humidity    = dht.readHumidity();
  float temperature = dht.readTemperature();

  if (isnan(humidity) || isnan(temperature)) {
    // อ่านค่าจากเซนเซอร์ล้มเหลว
  }

  delay(2000);
}
```

## คุณลักษณะ จอ OLED (I2C)

บอร์ดควบคุมเชื่อมต่อกับจอแสดงผล **OLED ขนาด 0.96" ความละเอียด 128x64 พิกเซล (ชิปควบคุม SSD1306)** ผ่านบัส **I2C**

| Device | SDA | SCL | Bus | ความละเอียด | I2C Address |
|--------|-----|-----|-----|---------------|-------------|
| OLED (SSD1306) | GPIO21 | GPIO22 | I2C (Wire) | 128x64 | `0x3C` (ค่าปกติ) |

### หมายเหตุ

- GPIO21 (SDA) และ GPIO22 (SCL) เป็นขา I2C มาตรฐาน (Default) ของ ESP32 Arduino core หากไม่ได้เรียก `Wire.begin()` กำหนดขาเอง ระบบจะใช้ขานี้โดยอัตโนมัติ
- บัส I2C ควรมี **R Pull-up ประมาณ 4.7k โอห์ม** ที่ขา SDA และ SCL แต่โมดูล OLED ส่วนใหญ่มีตัวต้านทาน Pull-up ติดตั้งมาบนบอร์ดแล้ว
- ค่า I2C Address ของโมดูล SSD1306 มักเป็น `0x3C` หรือ `0x3D` ขึ้นอยู่กับรุ่นของโมดูล ควรตรวจสอบด้วยการสแกนบัส I2C (I2C Scanner) หากเชื่อมต่อไม่ติด
- หากมีอุปกรณ์ I2C อื่นต่อร่วมบัสเดียวกัน (เช่นเซนเซอร์ I2C อื่น) ต้องมี Address ไม่ซ้ำกัน
- ใช้งานร่วมกับไลบรารี `Adafruit_SSD1306` และ `Adafruit_GFX`
- ตัวอย่างโค้ดกำหนดพิน:

```cpp
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define OLED_SDA_PIN 21
#define OLED_SCL_PIN 22

#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 64
#define OLED_ADDRESS  0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

void setup() {
  Wire.begin(OLED_SDA_PIN, OLED_SCL_PIN);

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
    // เชื่อมต่อจอ OLED ล้มเหลว
  }

  display.clearDisplay();
  display.display();
}
```

## คุณลักษณะ RS232/RS485 (MAX13487)

บอร์ดควบคุมเชื่อมต่อกับ IC แปลงสัญญาณ **MAX13487** ผ่านพอร์ต **Serial0 (UART0)** ของ ESP32 ซึ่งเป็นพอร์ตเดียวกับที่ใช้ Debug/Flash โปรแกรมผ่าน USB โดยมี **Switch เลือกโหมด** สำหรับสลับการทำงานระหว่าง **RS232** และ **RS485** แบบ Manual (ทางกายภาพ ไม่ผ่าน GPIO)

| Device | TX | RX | โหมดที่รองรับ | การควบคุมทิศทาง |
|--------|----|----|----------------|--------------------|
| MAX13487 | GPIO1 (TX0) | GPIO3 (RX0) | RS232 / RS485 (เลือกด้วย Switch) | Auto Direction Control (ไม่ต้องใช้ขา DE/RE) |

### หมายเหตุ

- **MAX13487E** มีวงจร **Auto Direction Control** ในตัว จึงไม่จำเป็นต้องใช้ขา GPIO ควบคุมทิศทางการรับ-ส่ง (DE/RE) เหมือน IC แปลงสัญญาณ RS485 ทั่วไป เช่น MAX485
- การเลือกโหมด **RS232 หรือ RS485** ทำผ่าน **Switch ทางกายภาพ** บนบอร์ด (Hardware Switch/Jumper) ไม่ได้เชื่อมต่อกับขา GPIO ของ ESP32 จึงไม่ต้องเขียนโค้ดควบคุมการสลับโหมด
- Serial0 (GPIO1/GPIO3) เป็นพอร์ตเดียวกับที่ใช้ในการอัปโหลดโปรแกรมและ Debug ผ่าน USB-Serial ควรถอด/ปลดการเชื่อมต่อ RS232/RS485 ออกก่อนทำการอัปโหลดโปรแกรม (Flash) เพื่อป้องกันสัญญาณชนกัน (Conflict)
- เนื่องจากใช้ Serial0 ร่วมกับ USB Debug ควรหลีกเลี่ยงการใช้ `Serial.print()` เพื่อ Debug ระหว่างที่สื่อสารกับอุปกรณ์ RS232/RS485 เพราะข้อมูลจะปนกันบนพอร์ตเดียวกัน
- ตัวอย่างโค้ดใช้งาน Serial0:

```cpp
void setup() {
  Serial.begin(9600); // ความเร็ว Baud Rate ตามอุปกรณ์ปลายทาง
}

void loop() {
  if (Serial.available()) {
    String data = Serial.readStringUntil('\n');
    // ประมวลผลข้อมูลที่รับจาก RS232/RS485
  }
}
```

## คุณลักษณะ Isolated Input (Opto Isolator)

บอร์ดควบคุมมีขาอินพุตแบบแยกกราวด์ (Isolated Input) จำนวน 2 ช่อง เชื่อมต่อผ่านวงจร **Opto Isolator** ทำงานแบบ **Active LOW** รองรับแรงดันสัญญาณอินพุตภายนอกได้สูงสุด **12V**

| Isolated Input | GPIO Pin | โหมดทำงาน | วงจรแยกสัญญาณ | แรงดันอินพุตสูงสุด | สถานะ Active | สถานะ Inactive |
|-----------------|----------|-----------|-----------------|----------------------|---------------|------------------|
| ISO1 | GPIO33 | Active LOW | Opto Isolator | 12V | `LOW` | `HIGH` |
| ISO2 | GPIO27 | Active LOW | Opto Isolator | 12V | `LOW` | `HIGH` |

### หมายเหตุ

- ใช้วงจร **Opto Isolator** แยกกราวด์ระหว่างวงจรภายนอก (สูงสุด 12V) กับวงจรของ ESP32 เพื่อป้องกันความเสียหายจากแรงดันที่แตกต่างกันหรือสัญญาณรบกวน (Noise Isolation)
- เนื่องจากผ่านวงจร Opto Isolator สัญญาณด้านขา GPIO จึงเป็นแบบ **Active LOW** (เมื่อมีสัญญาณอินพุตเข้ามาทางฝั่ง TTL/12V ขา GPIO จะอ่านค่าได้เป็น `LOW`)
- ควรกำหนด `pinMode` เป็น `INPUT` (วงจร Opto Isolator ทำหน้าที่ดึงสัญญาณอยู่แล้ว ไม่จำเป็นต้องใช้ Pull-up ภายในของ ESP32)
- ควรทำ Debounce หากนำไปใช้ตรวจจับสัญญาณที่มีการเปลี่ยนแปลงเร็ว (เช่น สัญญาณพัลส์)
- ตัวอย่างโค้ดกำหนดพิน:

```cpp
#define ISO1_PIN 33
#define ISO2_PIN 27

#define ISO_ACTIVE   LOW
#define ISO_INACTIVE HIGH

void setup() {
  pinMode(ISO1_PIN, INPUT);
  pinMode(ISO2_PIN, INPUT);
}

void loop() {
  if (digitalRead(ISO1_PIN) == ISO_ACTIVE) {
    // ISO1 มีสัญญาณอินพุตเข้า
  }

  if (digitalRead(ISO2_PIN) == ISO_ACTIVE) {
    // ISO2 มีสัญญาณอินพุตเข้า
  }
}
```
