/**
 * ESP32 I2C Humidity Sensor + OLED Display
 *
 * Reads humidity (and temperature) from an AHT10/AHT20 I2C sensor
 * and displays the data on a 128x64 SSD1306 OLED screen.
 *
 * Wiring:
 *   SDA -> GPIO 21
 *   SCL -> GPIO 22
 *   VCC -> 3.3V
 *   GND -> GND
 *
 * Libraries required (install via Arduino Library Manager):
 *   - Adafruit SSD1306
 *   - Adafruit GFX
 *   - Adafruit AHTX0
 */

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_AHTX0.h>

// ---- OLED Configuration ----
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT  64
#define OLED_RESET     -1   // No reset pin
#define OLED_ADDRESS 0x3C

// ---- I2C Pins (ESP32 default) ----
#define I2C_SDA 21
#define I2C_SCL 22

// ---- Update interval ----
#define READ_INTERVAL_MS 2000

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
Adafruit_AHTX0   aht;

bool sensorFound = false;

// ---- Draw a thin horizontal divider line ----
void drawDivider(int y) {
  display.drawFastHLine(0, y, SCREEN_WIDTH, SSD1306_WHITE);
}

// ---- Draw a centered title bar ----
void drawHeader(const char* title) {
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(title, 0, 0, &x1, &y1, &w, &h);
  display.setCursor((SCREEN_WIDTH - w) / 2, 2);
  display.print(title);
  drawDivider(12);
}

// ---- Render a labelled metric ----
void drawMetric(const char* label, float value, const char* unit,
                int y, uint8_t bigSize) {
  // Label (small)
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, y);
  display.print(label);

  // Value (large)
  display.setTextSize(bigSize);
  char buf[16];
  dtostrf(value, 4, 1, buf);
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(buf, 0, 0, &x1, &y1, &w, &h);
  display.setCursor((SCREEN_WIDTH - w) / 2, y + 10);
  display.print(buf);

  // Unit (small, right-aligned)
  display.setTextSize(1);
  display.getTextBounds(unit, 0, 0, &x1, &y1, &w, &h);
  display.setCursor(SCREEN_WIDTH - w, y + 10 + (bigSize - 1) * 8);
  display.print(unit);
}

void setup() {
  Serial.begin(115200);

  // Init I2C on ESP32 default pins
  Wire.begin(I2C_SDA, I2C_SCL);

  // Init OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
    Serial.println(F("SSD1306 init failed"));
    for (;;);  // Halt
  }
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  // Splash screen
  display.setTextSize(1);
  display.setCursor(10, 20);
  display.println(F("ESP32 Humidity"));
  display.setCursor(18, 35);
  display.println(F("OLED Monitor"));
  display.display();
  delay(1500);

  // Init AHT sensor
  if (aht.begin()) {
    sensorFound = true;
    Serial.println(F("AHT sensor found."));
  } else {
    Serial.println(F("AHT sensor NOT found! Check wiring."));
  }
}

void loop() {
  display.clearDisplay();
  drawHeader("HUMIDITY MONITOR");

  if (!sensorFound) {
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(8, 28);
    display.println(F("Sensor not found!"));
    display.setCursor(8, 42);
    display.println(F("Check I2C wiring."));
    display.display();
    delay(READ_INTERVAL_MS);
    return;
  }

  sensors_event_t humidity_evt, temp_evt;
  aht.getEvent(&humidity_evt, &temp_evt);

  float humidity    = humidity_evt.relative_humidity;
  float temperature = temp_evt.temperature;

  // Serial output for debugging
  Serial.print(F("Humidity: "));
  Serial.print(humidity, 1);
  Serial.print(F(" %  Temp: "));
  Serial.print(temperature, 1);
  Serial.println(F(" C"));

  // ---- Monochromatic OLED layout ----
  //
  //  +--------------------------+
  //  |    HUMIDITY MONITOR      |  (header, size 1)
  //  |--------------------------|  (divider @ y=12)
  //  |  HUM                     |
  //  |        68.5           %  |  (value, size 2)
  //  |--------------------------|  (divider @ y=38)
  //  |  TEMP                    |
  //  |        24.3           C  |  (value, size 2)
  //  +--------------------------+

  drawDivider(38);

  // Humidity row
  drawMetric("HUM", humidity, "%", 14, 2);

  // Temperature row
  drawMetric("TEMP", temperature, "C", 40, 2);

  display.display();
  delay(READ_INTERVAL_MS);
}