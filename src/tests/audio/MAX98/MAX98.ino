#include <ESP_I2S.h>

I2SClass I2S;

const int BCLK_PIN = 4;
const int LRC_PIN  = 5;
const int DIN_PIN  = 6;

const int SAMPLE_RATE = 16000;
const int TONE_HZ = 440;

void setup() {
  Serial.begin(115200);
  delay(1000);

  I2S.setPins(BCLK_PIN, LRC_PIN, DIN_PIN);

  if (!I2S.begin(
        I2S_MODE_STD,
        SAMPLE_RATE,
        I2S_DATA_BIT_WIDTH_16BIT,
        I2S_SLOT_MODE_MONO)) {
    Serial.println("I2S initialization failed!");
    while (1) delay(1000);
  }

  Serial.println("Playing 440 Hz test tone...");
}

void loop() {
  static uint32_t sampleIndex = 0;

  // Generate a quiet square-wave test tone.
  int16_t sample =
      ((sampleIndex++ % (SAMPLE_RATE / TONE_HZ)) <
       (SAMPLE_RATE / TONE_HZ / 2))
          ? 1500 : -1500;

  uint8_t audioData[4];
  audioData[0] = sample & 0xFF;
  audioData[1] = (sample >> 8) & 0xFF;
  audioData[2] = audioData[0];
  audioData[3] = audioData[1];

  I2S.write(audioData, sizeof(audioData));
}