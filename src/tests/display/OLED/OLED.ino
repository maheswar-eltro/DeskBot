#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define OLED_ADDRESS 0x3C

Adafruit_SSD1306 display (
  SCREEN_WIDTH,
  SCREEN_HEIGHT,
  &Wire,
  OLED_RESET
);

void setup() {
  Serial.begin(115200);
  Wire.begin(0, 1);
  
  if (!display.begin(
    SSD1306_SWITCHCAPVCC,
    OLED_ADDRESS)) {
      Serial.println("OLED not detected");
      while (true) delay (100);
  }

  display.clearDisplay();

  // Two robot eyes
    display.fillRoundRect(24, 22, 30, 25, 8,
                          SSD1306_WHITE);
    display.fillRoundRect(74, 22, 30, 25, 8,
                          SSD1306_WHITE);

    display.display();

}

void loop() {
  // put your main code here, to run repeatedly:

}
