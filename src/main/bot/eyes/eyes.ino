
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define OLED_ADDRESS 0x3C

Adafruit_SSD1306 display(
  SCREEN_WIDTH, SCREEN_HEIGHT,
  &Wire, OLED_RESET
);

// Eye dimensions and positions
const int EYE_W = 38;
const int EYE_H = 32;
const int LEFT_X = 12;
const int RIGHT_X = 78;
const int EYE_Y = 16;

void drawOpenEyes() {
  display.clearDisplay();

  // Big boxy eyes with slightly rounded corners
  display.fillRoundRect(
    LEFT_X, EYE_Y, EYE_W, EYE_H, 5, SSD1306_WHITE
  );

  display.fillRoundRect(
    RIGHT_X, EYE_Y, EYE_W, EYE_H, 5, SSD1306_WHITE
  );


  display.display();
}

void drawClosedEyes() {
  display.clearDisplay();

  // Closed eyelids: thick horizontal lines
  display.fillRoundRect(
    LEFT_X, EYE_Y + 13, EYE_W, 7, 3, SSD1306_WHITE
  );

  display.fillRoundRect(
    RIGHT_X, EYE_Y + 13, EYE_W, 7, 3, SSD1306_WHITE
  );

  display.display();
}

void setup() {
  Wire.begin(0, 1);  // SDA = GPIO 0, SCL = GPIO 1

  if (!display.begin(
        SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
    while (true) delay(100);
  }

  randomSeed(micros());

  drawOpenEyes();
}

void loop() {
  // Keep eyes open for a random interval
  delay(random(2000, 5000));

  // Blink
  drawClosedEyes();
  delay(140);

  // Open eyes again
  drawOpenEyes();
}
