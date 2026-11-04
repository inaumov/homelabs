#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDRESS 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

void setup() {
  Serial.begin(115200);

  Wire.begin(21, 22);

  Serial.println();
  Serial.println("I2C scanning...");

  bool oledFound = false;

  for (byte address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    byte error = Wire.endTransmission();

    if (error == 0) {
      Serial.print("FOUND: 0x");

      if (address < 16) {
        Serial.print("0");
      }

      Serial.println(address, HEX);

      if (address == OLED_ADDRESS) {
        oledFound = true;
      }
    }
  }

  Serial.println("Scanning finished.");

  if (!oledFound) {
    Serial.println("OLED NOT FOUND at 0x3C.");
    return;
  }

  Serial.println("Initializing OLED...");

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
    Serial.println("OLED initialization FAILED!");
    return;
  }

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  // First line
  display.setTextSize(1);

  const char* message = "check one two";

  int16_t x1, y1;
  uint16_t w, h;

  display.getTextBounds(message, 0, 0, &x1, &y1, &w, &h);

  display.setCursor((SCREEN_WIDTH - w) / 2, 22);
  display.println(message);

  // Second line
  const char* result = "OLED OK";

  display.getTextBounds(result, 0, 0, &x1, &y1, &w, &h);

  display.setCursor((SCREEN_WIDTH - w) / 2, 38);
  display.println(result);

  display.display();

  Serial.println("Text sent to OLED.");
}

void loop() {
  // Nothing to do.
}
