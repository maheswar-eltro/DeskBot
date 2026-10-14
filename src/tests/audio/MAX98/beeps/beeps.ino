#include <ESP_I2S.h>
#include <math.h>

I2SClass I2S;

const int BCLK_PIN = 4;
const int LRC_PIN  = 5;
const int DIN_PIN  = 6;

const int SAMPLE_RATE = 16000;
const int BUFFER_SAMPLES = 256;

int16_t audioBuffer[BUFFER_SAMPLES * 2];

void playTone(int frequency, int durationMs) {
  int totalSamples = SAMPLE_RATE * durationMs / 1000;
  int generated = 0;
  uint32_t phase = 0;
  uint32_t phaseStep =
      ((uint64_t)frequency << 32) / SAMPLE_RATE;

  while (generated < totalSamples) {
    int count = min(BUFFER_SAMPLES,
                    totalSamples - generated);

    for (int i = 0; i < count; i++) {
      // Generate a square wave
      int16_t sample =
          (phase & 0x80000000UL) ? 2000 : -2000;

      phase += phaseStep;

      // Duplicate mono audio into both I2S channels
      audioBuffer[i * 2] = sample;
      audioBuffer[i * 2 + 1] = sample;
    }

    I2S.write((uint8_t *)audioBuffer,
              count * 2 * sizeof(int16_t));

    generated += count;
  }
}

void pauseSound(int durationMs) {
  // Write silence so the amplifier receives continuous audio
  int remaining = SAMPLE_RATE * durationMs / 1000;

  while (remaining > 0) {
    int count = min(BUFFER_SAMPLES, remaining);

    for (int i = 0; i < count * 2; i++) {
      audioBuffer[i] = 0;
    }

    I2S.write((uint8_t *)audioBuffer,
              count * 2 * sizeof(int16_t));

    remaining -= count;
  }
}

void setup() {
  Serial.begin(115200);
  delay(500);

  I2S.setPins(BCLK_PIN, LRC_PIN, DIN_PIN);

  if (!I2S.begin(
        I2S_MODE_STD,
        SAMPLE_RATE,
        I2S_DATA_BIT_WIDTH_16BIT,
        I2S_SLOT_MODE_STEREO)) {
    Serial.println("I2S initialization failed!");
    while (true) delay(1000);
  }

  Serial.println("Robot sound test started!");
}

void loop() {
  playTone(300, 200);
  pauseSound(300);

  playTone(1000, 150);
  pauseSound(300);

  playTone(660, 120);
  pauseSound(70);
  playTone(880, 180);
  pauseSound(400);

  playTone(400, 100);
  playTone(600, 100);
  playTone(800, 100);
  playTone(1100, 180);

  pauseSound(1200);
}
