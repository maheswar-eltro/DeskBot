#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP_I2S.h>
#include <math.h>
#include <string.h>
// ==================================================
// OLED
// ==================================================
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define OLED_ADDRESS 0x3C
Adafruit_SSD1306 display(
  SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET
);
// ==================================================
// MPU6050
// ==================================================
#define MPU_ADDR 0x68
int16_t ax = 0, ay = 0, az = 0;
int16_t prevAx = 0, prevAy = 0, prevAz = 0;
// ==================================================
// I2S: MAX98357A + INMP441
// ==================================================
I2SClass I2S;
const int BCLK_PIN = 4;
const int LRC_PIN = 5;
const int SPEAKER_DIN_PIN = 6;
const int MIC_SD_PIN = 3;
const int SAMPLE_RATE = 16000;
const int AUDIO_BUFFER_SAMPLES = 128;
int32_t audioBuffer[AUDIO_BUFFER_SAMPLES * 2];
int32_t micBuffer[256];
bool audioBusy = false;
// Voice mimic: 5 seconds of mono 16-bit audio (about 160 KB RAM).
const int VOICE_SAMPLE_RATE = 16000;
const int VOICE_RECORD_SECONDS = 5;
const int VOICE_SAMPLE_COUNT = VOICE_SAMPLE_RATE * VOICE_RECORD_SECONDS;
int16_t voiceSamples[VOICE_SAMPLE_COUNT];
// SPECTRUM ANALYSER: type 's' in Serial Monitor to toggle.
const int FFT_SIZE = 128;
const int SPECTRUM_BARS = 16;
float fftReal[FFT_SIZE];
float fftImag[FFT_SIZE];
uint8_t spectrumBars[SPECTRUM_BARS] = {0};
uint8_t spectrumPeaks[SPECTRUM_BARS] = {0};
bool showSpectrum = false;
bool showPhysics = false;
int spectrumSampleIndex = 0;

// Tilt-controlled physics sandbox: bouncing particles use the full OLED screen.
const int PHYSICS_BALL_COUNT = 40;
const float PHYSICS_RADIUS = 2.2f;
struct PhysicsBall {
  float x, y, vx, vy;
};
PhysicsBall physicsBalls[PHYSICS_BALL_COUNT];
unsigned long lastPhysicsUpdate = 0;
unsigned long lastPhysicsMPURead = 0;
int physicsShakeEnergy = 0;
// Optical illusion modes: 1 = Rubin vase, 2 = hypnotic tunnel, 3 = Hermann grid.
bool showIllusion = false;
int illusionType = 0;

// ==================================================
// PHYSICAL BUTTONS (ESP32-C3 Super Mini)
// Buttons connect GPIO to GND; INPUT_PULLUP is used.
// GPIO7 and GPIO10 avoid the pins already used by OLED/I2S.
// ==================================================
const int LEFT_BUTTON_PIN = 7;   // MODE / return to main face
const int RIGHT_BUTTON_PIN = 10; // ACTION / select
const unsigned long BUTTON_DEBOUNCE_MS = 35;
const unsigned long BOTH_HOLD_MS = 5000;
const unsigned long LEFT_HOLD_MS = 900;

struct ButtonState {
  bool stableState = HIGH;
  bool lastReading = HIGH;
  unsigned long lastChange = 0;
  unsigned long pressedAt = 0;
  bool pressEvent = false;
  bool releaseEvent = false;
  bool longHandled = false;
};
ButtonState leftButton;
ButtonState rightButton;
bool bothHoldHandled = false;
// ==================================================
// MICROPHONE SETTINGS
// ==================================================
const int64_t MIC_THRESHOLD = 1200000000LL;
const unsigned long MIC_SAMPLE_INTERVAL = 25;
const unsigned long SOUND_RELEASE_MS = 80;
const unsigned long LOUD_DURATION_MS = 450;
const unsigned long DOUBLE_CLAP_WINDOW = 1200;
const unsigned long MIC_AFTER_SOUND_MS = 300;
unsigned long lastMicRead = 0;
unsigned long micIgnoreUntil = 0;
unsigned long loudStartedAt = 0;
unsigned long lastLoudAt = 0;
unsigned long singleClapAt = 0;
bool soundActive = false;
bool loudReactionPlayed = false;
bool pendingSingleClap = false;
// ==================================================
// FACE STATES
// ==================================================
enum FaceState {
  NORMAL,
  SLEEPY,
  SURPRISED,
  SHAKING,
  EXCITED,
  LOUD
};
FaceState face = NORMAL;
unsigned long lastRead = 0;
unsigned long lastDraw = 0;
unsigned long lastActivity = 0;
unsigned long lastBlink = 0;
unsigned long blinkEnd = 0;
unsigned long surprisedUntil = 0;
unsigned long shakingUntil = 0;
unsigned long excitedUntil = 0;
unsigned long loudUntil = 0;
unsigned long lastShakeHit = 0;
unsigned long shakeWindowStart = 0;
bool eyesClosed = false;
int glanceX = 0;
int glanceY = 0;
int targetX = 0;
int targetY = 0;
int shakeCount = 0;
// ==================================================
// BEHAVIOUR SETTINGS
// ==================================================
const unsigned long SLEEP_AFTER = 7000;
const unsigned long BLINK_INTERVAL = 2000;
const unsigned long BLINK_DURATION = 130;
const unsigned long SURPRISE_DURATION = 900;
const unsigned long SHAKE_DURATION = 1000;
const unsigned long EXCITED_DURATION = 1000;
const unsigned long LOUD_DURATION_FACE = 1200;
const long MOVEMENT_THRESHOLD = 3500;
const long SUDDEN_MOVEMENT_THRESHOLD = 10000;
const long SHAKE_THRESHOLD = 6500;
// ==================================================
// AUDIO GENERATOR
// ==================================================
void playTone(
  int frequency,
  int durationMs,
  int32_t amplitude = 54000000
) {
  if (frequency <= 0 || durationMs <= 0) return;
  uint32_t phase = 0;
  uint32_t phaseStep =
    (uint64_t(frequency) << 32) / SAMPLE_RATE;
  int totalSamples = SAMPLE_RATE * durationMs / 1000;
  while (totalSamples > 0) {
    int count = min(totalSamples, AUDIO_BUFFER_SAMPLES);
    for (int i = 0; i < count; i++) {
      int32_t sample =
        (phase & 0x80000000UL) ? amplitude : -amplitude;
      phase += phaseStep;
      audioBuffer[2 * i] = sample;
      audioBuffer[2 * i + 1] = sample;
    }
    I2S.write(
      (uint8_t*)audioBuffer,
      count * 2 * sizeof(int32_t)
    );
    totalSamples -= count;
  }
}
void silenceAudio(int durationMs) {
  memset(audioBuffer, 0, sizeof(audioBuffer));
  int totalSamples = SAMPLE_RATE * durationMs / 1000;
  while (totalSamples > 0) {
    int count = min(totalSamples, AUDIO_BUFFER_SAMPLES);
    I2S.write(
      (uint8_t*)audioBuffer,
      count * 2 * sizeof(int32_t)
    );
    totalSamples -= count;
  }
}
// ==================================================
// SOUND EFFECTS
// ==================================================
void soundSurprise() {
  audioBusy = true;
  playTone(850, 90, 54000000);
  silenceAudio(35);
  playTone(1350, 150, 60000000);
  silenceAudio(50);
  audioBusy = false;
  micIgnoreUntil = millis() + MIC_AFTER_SOUND_MS;
}
void soundShake() {
  audioBusy = true;
  for (int i = 0; i < 4; i++) {
    playTone(1050, 65, 54000000);
    playTone(650, 65, 54000000);
  }
  silenceAudio(30);
  audioBusy = false;
  micIgnoreUntil = millis() + MIC_AFTER_SOUND_MS;
}
void soundSleep() {
  audioBusy = true;
  playTone(950, 100, 36000000);
  silenceAudio(40);
  playTone(720, 140, 30000000);
  silenceAudio(60);
  playTone(520, 190, 25500000);
  silenceAudio(80);
  playTone(380, 260, 21000000);
  silenceAudio(40);
  audioBusy = false;
  micIgnoreUntil = millis() + MIC_AFTER_SOUND_MS;
}
void soundWake() {
  audioBusy = true;
  playTone(420, 120, 24000000);
  silenceAudio(35);
  playTone(620, 120, 30000000);
  silenceAudio(35);
  playTone(820, 150, 36000000);
  silenceAudio(40);
  playTone(1100, 180, 39000000);
  silenceAudio(30);
  audioBusy = false;
  micIgnoreUntil = millis() + MIC_AFTER_SOUND_MS;
}
void soundExcited() {
  audioBusy = true;
  playTone(1100, 70, 33000000);
  silenceAudio(25);
  playTone(1500, 70, 39000000);
  silenceAudio(25);
  playTone(1900, 110, 42000000);
  audioBusy = false;
  micIgnoreUntil = millis() + MIC_AFTER_SOUND_MS;
}
void soundLoud() {
  audioBusy = true;
  playTone(500, 180, 30000000);
  silenceAudio(40);
  playTone(700, 250, 36000000);
  audioBusy = false;
  micIgnoreUntil = millis() + MIC_AFTER_SOUND_MS;
}
// ==================================================
// MPU6050 READING
// ==================================================
bool readMPU() {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom(MPU_ADDR, 6, true) != 6) return false;
  ax = (int16_t)((Wire.read() << 8) | Wire.read());
  ay = (int16_t)((Wire.read() << 8) | Wire.read());
  az = (int16_t)((Wire.read() << 8) | Wire.read());
  return true;
}
// ==================================================
// MICROPHONE: CLAP AND LOUD SOUND DETECTION
// ==================================================
// ==================================================
// SPECTRUM ANALYSER
// ==================================================
void runFFT() {
  // Remove DC offset so silence does not lift every bar.
  double mean = 0;
  for (int i = 0; i < FFT_SIZE; i++) mean += fftReal[i];
  mean /= FFT_SIZE;
  // Hann window reduces leakage between frequency bins.
  for (int i = 0; i < FFT_SIZE; i++) {
    float window = 0.5f - 0.5f * cosf(2.0f * PI * i / (FFT_SIZE - 1));
    fftReal[i] = (fftReal[i] - (float)mean) * window;
    fftImag[i] = 0;
  }
  // Bit-reversal permutation.
  int j = 0;
  for (int i = 1; i < FFT_SIZE; i++) {
    int bit = FFT_SIZE >> 1;
    while (j & bit) { j ^= bit; bit >>= 1; }
    j ^= bit;
    if (i < j) {
      float temp = fftReal[i]; fftReal[i] = fftReal[j]; fftReal[j] = temp;
    }
  }
  // Radix-2 Cooley-Tukey FFT.
  for (int length = 2; length <= FFT_SIZE; length <<= 1) {
    float angle = -2.0f * PI / length;
    float stepReal = cosf(angle), stepImag = sinf(angle);
    for (int start = 0; start < FFT_SIZE; start += length) {
      float wr = 1.0f, wi = 0.0f;
      int half = length >> 1;
      for (int k = 0; k < half; k++) {
        int even = start + k, odd = even + half;
        float oddReal = fftReal[odd] * wr - fftImag[odd] * wi;
        float oddImag = fftReal[odd] * wi + fftImag[odd] * wr;
        float evenReal = fftReal[even], evenImag = fftImag[even];
        fftReal[even] = evenReal + oddReal; fftImag[even] = evenImag + oddImag;
        fftReal[odd] = evenReal - oddReal; fftImag[odd] = evenImag - oddImag;
        float nextWr = wr * stepReal - wi * stepImag;
        wi = wr * stepImag + wi * stepReal; wr = nextWr;
      }
    }
  }
  // Group FFT bins into 16 bands, with more detail in lower frequencies.
  const uint8_t bandEdges[SPECTRUM_BARS + 1] = {
    1, 2, 3, 4, 5, 6, 8, 10, 13, 16, 20, 25, 31, 38, 47, 57, 64
  };
  for (int bar = 0; bar < SPECTRUM_BARS; bar++) {
    float bandMax = 0;
    for (int bin = bandEdges[bar]; bin < bandEdges[bar + 1]; bin++) {
      float magnitude = sqrtf(fftReal[bin] * fftReal[bin] + fftImag[bin] * fftImag[bin]);
      if (magnitude > bandMax) bandMax = magnitude;
    }
    // Stronger gain and logarithmic compression make quiet and loud sounds visible.
    float scaled = log10f(1.0f + bandMax / 2500000.0f) * 15.0f;
    int target = constrain((int)scaled, 0, 44);
    // Fast attack, lively fall: the bars respond quickly to changing sounds.
    int current = spectrumBars[bar];
    if (target > current) current = min(target, current + 9);
    else current = max(target, current - 3);
    spectrumBars[bar] = (uint8_t)current;
    // Peak markers rise instantly and then fall gradually.
    if (spectrumBars[bar] >= spectrumPeaks[bar]) {
      spectrumPeaks[bar] = spectrumBars[bar];
    } else if (spectrumPeaks[bar] > 0) {
      spectrumPeaks[bar] = (uint8_t)max((int)spectrumBars[bar], (int)spectrumPeaks[bar] - 1);
    }
  }
}
void addSpectrumSamples(int frames) {
  if (!showSpectrum) return;
  for (int i = 0; i < frames; i++) {
    // INMP441 L/R is tied to GND: use the left stereo slot.
    fftReal[spectrumSampleIndex++] = (float)micBuffer[2 * i];
    if (spectrumSampleIndex >= FFT_SIZE) {
      runFFT();
      spectrumSampleIndex = 0;
    }
  }
}
void drawSpectrum() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print("LIVE AUDIO FFT");
  const int barWidth = 6, gap = 2, baseline = 62;
  for (int i = 0; i < SPECTRUM_BARS; i++) {
    int x = i * (barWidth + gap);
    int h = constrain((int)spectrumBars[i], 0, 47);
    // Draw a solid bar plus small separated caps for an equalizer look.
    if (h > 0) {
      display.fillRect(x, baseline - h, barWidth, h, SSD1306_WHITE);
      if (h > 7) {
        for (int y = baseline - h + 2; y < baseline - 2; y += 5)
          display.drawFastHLine(x, y, barWidth, SSD1306_BLACK);
      }
    }
    // Peak cap decays from the FFT update, so preserve it above the live bar.
    int peak = constrain((int)spectrumPeaks[i], 0, 47);
    int peakY = baseline - peak;
    if (peak > h && peakY > 14) display.drawFastHLine(x, peakY, barWidth, SSD1306_WHITE);
  }
  display.display();
}
// ==================================================
// OPTICAL ILLUSIONS
// ==================================================
// Generated Rubin-vase reference, reduced to a 64x60 monochrome bitmap.
// Alternating positive/negative display helps the eye switch between vase and profiles.
const uint8_t rubinVaseBitmap[480] PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xC0,
  0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x80, 0x00, 0x3F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFC, 0x00,
  0x00, 0x07, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0, 0x00, 0x00, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0xC0, 0x00,
  0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x7F, 0xFF, 0xFF, 0xFE, 0x00, 0x00,
  0x00, 0x00, 0x3F, 0xFF, 0xFF, 0xFC, 0x00, 0x00, 0x00, 0x00, 0x1F, 0xFF, 0xFF, 0xF8, 0x00, 0x00,
  0x00, 0x00, 0x0F, 0xFF, 0xFF, 0xF8, 0x00, 0x00, 0x00, 0x00, 0x0F, 0xFF, 0xFF, 0xF0, 0x00, 0x00,
  0x00, 0x00, 0x07, 0xFF, 0xFF, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x07, 0xFF, 0xFF, 0xE0, 0x00, 0x00,
  0x00, 0x00, 0x07, 0xFF, 0xFF, 0xE0, 0x00, 0x00, 0x00, 0x00, 0x03, 0xFF, 0xFF, 0xE0, 0x00, 0x00,
  0x00, 0x00, 0x03, 0xFF, 0xFF, 0xE0, 0x00, 0x00, 0x00, 0x00, 0x03, 0xFF, 0xFF, 0xE0, 0x00, 0x00,
  0x00, 0x00, 0x07, 0xFF, 0xFF, 0xE0, 0x00, 0x00, 0x00, 0x00, 0x07, 0xFF, 0xFF, 0xE0, 0x00, 0x00,
  0x00, 0x00, 0x03, 0xFF, 0xFF, 0xE0, 0x00, 0x00, 0x00, 0x00, 0x03, 0xFF, 0xFF, 0xC0, 0x00, 0x00,
  0x00, 0x00, 0x01, 0xFF, 0xFF, 0x80, 0x00, 0x00, 0x00, 0x00, 0x01, 0xFF, 0xFF, 0x80, 0x00, 0x00,
  0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7F, 0xFE, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x7F, 0xFE, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3F, 0xFC, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x7F, 0xFE, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x7F, 0xFE, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7F, 0xFE, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x01, 0xFF, 0xFF, 0x80, 0x00, 0x00, 0x00, 0x00, 0x01, 0xFF, 0xFF, 0x80, 0x00, 0x00,
  0x00, 0x00, 0x01, 0xFF, 0xFF, 0x80, 0x00, 0x00, 0x00, 0x00, 0x01, 0xFF, 0xFF, 0x80, 0x00, 0x00,
  0x00, 0x00, 0x01, 0xFF, 0xFF, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x01, 0xFF, 0xFF, 0x80, 0x00, 0x00, 0x00, 0x00, 0x1F, 0xFF, 0xFF, 0xF8, 0x00, 0x00,
  0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xC0, 0x00,
  0x00, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0, 0x00, 0x00, 0x1F, 0xFF, 0xFF, 0xFF, 0xFF, 0xF8, 0x00,
  0x00, 0x3F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFC, 0x00, 0x00, 0x7F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFE, 0x00,
  0x00, 0x7F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFE, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00,
  0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

void drawRubinVase(unsigned long now) {
  (void)now;  // The Rubin vase stays in one fixed black-and-white polarity.
  display.clearDisplay();
  display.fillScreen(SSD1306_BLACK);
  // White vase; the surrounding black negative space forms the two faces.
  display.drawBitmap(32, 2, rubinVaseBitmap, 64, 60, SSD1306_WHITE);
  // A thin white frame anchors the figure.
  display.drawRect(31, 1, 66, 62, SSD1306_WHITE);
  display.display();
}

void drawHermannGrid() {
  // Dense black boxes separated by white gutters.
  display.clearDisplay();
  display.fillScreen(SSD1306_WHITE);
  const int cellW = 9;
  const int cellH = 9;
  const int boxW = 6;
  const int boxH = 6;
  for (int y = 1; y < SCREEN_HEIGHT; y += cellH) {
    for (int x = 1; x < SCREEN_WIDTH; x += cellW) {
      display.fillRect(x, y, boxW, boxH, SSD1306_BLACK);
    }
  }
  display.display();
}

void drawHypnoticTunnel(unsigned long now) {
  // Nested rings whose centres gradually shift around the screen,
  // creating a twisting tunnel effect.
  display.clearDisplay();
  float phase = now * 0.0014f;
  const int rings = 15;
  for (int i = rings; i >= 0; --i) {
    float depth = (float)i / rings;
    int radius = 3 + i * 6;
    float twist = phase + depth * 2.35f;
    int cx = 64 + (int)(cos(twist) * depth * 25.0f);
    int cy = 32 + (int)(sin(twist) * depth * 17.0f);
    // Large outer circles intentionally extend beyond screen edges.
    display.drawCircle(cx, cy, radius, SSD1306_WHITE);
    if ((i % 2) == 0 && radius > 7) {
      display.drawCircle(cx, cy, radius - 2, SSD1306_WHITE);
    }
  }
  // A small centre point strengthens the tunnel's vanishing point.
  display.fillCircle(
    64 + (int)(cos(phase + 2.35f) * 2.0f),
    32 + (int)(sin(phase + 2.35f) * 2.0f),
    2, SSD1306_WHITE
  );
  display.display();
}

// ==================================================
// TILT PHYSICS SANDBOX
// MPU6050 tilt acts like gravity; balls collide with walls and one another.
// ==================================================
void resetPhysicsBalls(bool scatter) {
  for (int i = 0; i < PHYSICS_BALL_COUNT; i++) {
    if (scatter) {
      physicsBalls[i].x = random(4, SCREEN_WIDTH - 4);
      physicsBalls[i].y = random(4, SCREEN_HEIGHT - 4);
      physicsBalls[i].vx = random(-45, 46) / 10.0f;
      physicsBalls[i].vy = random(-45, 46) / 10.0f;
    } else {
      // Spread particles across the entire display with gentle initial drift.
      physicsBalls[i].x = random(4, SCREEN_WIDTH - 4);
      physicsBalls[i].y = random(4, SCREEN_HEIGHT - 4);
      physicsBalls[i].vx = random(-12, 13) / 10.0f;
      physicsBalls[i].vy = random(-12, 13) / 10.0f;
    }
  }
  lastPhysicsUpdate = millis();
}

void updatePhysics(unsigned long now) {
  if (now - lastPhysicsMPURead >= 20) {
    lastPhysicsMPURead = now;
    readMPU();
  }
  float dt = (now - lastPhysicsUpdate) / 1000.0f;
  lastPhysicsUpdate = now;
  if (dt > 0.04f) dt = 0.04f;
  if (dt <= 0) return;

  // Convert lateral accelerometer readings to gravity. A dead zone around
  // level prevents sensor bias/noise from making the balls drift when flat.
  float tiltX = -((float)ay / 16384.0f);
  float tiltY = -((float)ax / 16384.0f); // corrected: tilt down moves balls down
  const float DEAD_ZONE = 0.12f;
  if (fabsf(tiltX) < DEAD_ZONE) tiltX = 0.0f;
  else tiltX = (tiltX > 0 ? 1.0f : -1.0f) * (fabsf(tiltX) - DEAD_ZONE);
  if (fabsf(tiltY) < DEAD_ZONE) tiltY = 0.0f;
  else tiltY = (tiltY > 0 ? 1.0f : -1.0f) * (fabsf(tiltY) - DEAD_ZONE);
  float gx = constrain(tiltX * 95.0f, -95.0f, 95.0f);
  float gy = constrain(tiltY * 95.0f, -95.0f, 95.0f);

  for (int i = 0; i < PHYSICS_BALL_COUNT; i++) {
    PhysicsBall &b = physicsBalls[i];
    b.vx += gx * dt;
    b.vy += gy * dt;
    // Air resistance keeps the simulation controllable on a tiny screen.
    b.vx *= 0.995f;
    b.vy *= 0.995f;
    b.x += b.vx * dt;
    b.y += b.vy * dt;

    if (b.x < PHYSICS_RADIUS + 1) {
      b.x = PHYSICS_RADIUS + 1; b.vx = fabsf(b.vx) * 0.82f;
    } else if (b.x > SCREEN_WIDTH - PHYSICS_RADIUS - 1) {
      b.x = SCREEN_WIDTH - PHYSICS_RADIUS - 1; b.vx = -fabsf(b.vx) * 0.82f;
    }
    if (b.y < PHYSICS_RADIUS + 1) {
      b.y = PHYSICS_RADIUS + 1; b.vy = fabsf(b.vy) * 0.82f;
    } else if (b.y > SCREEN_HEIGHT - PHYSICS_RADIUS - 1) {
      b.y = SCREEN_HEIGHT - PHYSICS_RADIUS - 1; b.vy = -fabsf(b.vy) * 0.82f;
    }
  }

  // Simple equal-mass particle collisions. The small count keeps this light.
  const float minDist = PHYSICS_RADIUS * 2.0f;
  for (int i = 0; i < PHYSICS_BALL_COUNT; i++) {
    for (int j = i + 1; j < PHYSICS_BALL_COUNT; j++) {
      PhysicsBall &a = physicsBalls[i];
      PhysicsBall &b = physicsBalls[j];
      float dx = b.x - a.x, dy = b.y - a.y;
      float dist2 = dx * dx + dy * dy;
      if (dist2 >= minDist * minDist) continue;
      float dist = sqrtf(dist2);
      if (dist < 0.01f) { dx = 0.1f; dy = 0.0f; dist = 0.1f; }
      float nx = dx / dist, ny = dy / dist;
      float overlap = minDist - dist;
      a.x -= nx * overlap * 0.5f; a.y -= ny * overlap * 0.5f;
      b.x += nx * overlap * 0.5f; b.y += ny * overlap * 0.5f;
      float rel = (b.vx - a.vx) * nx + (b.vy - a.vy) * ny;
      if (rel < 0) {
        a.vx += rel * nx; a.vy += rel * ny;
        b.vx -= rel * nx; b.vy -= rel * ny;
      }
    }
  }
}

void drawPhysics() {
  display.clearDisplay();
  for (int i = 0; i < PHYSICS_BALL_COUNT; i++) {
    int x = (int)(physicsBalls[i].x + 0.5f);
    int y = (int)(physicsBalls[i].y + 0.5f);
    display.fillCircle(x, y, 2, SSD1306_WHITE);
  }
  display.display();
}

void updateButtonState(ButtonState &button, int pin, unsigned long now) {
  bool reading = digitalRead(pin);
  button.pressEvent = false;
  button.releaseEvent = false;

  if (reading != button.lastReading) {
    button.lastChange = now;
    button.lastReading = reading;
  }
  if (now - button.lastChange >= BUTTON_DEBOUNCE_MS &&
      reading != button.stableState) {
    button.stableState = reading;
    if (button.stableState == LOW) {
      button.pressEvent = true;
      button.pressedAt = now;
      button.longHandled = false;
    } else {
      button.releaseEvent = true;
    }
  }
}

void drawVoiceMimicScreen(const char *line1, const char *line2) {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(27, 7);
  display.println("TALKING BOT");
  display.drawRoundRect(6, 21, 116, 36, 7, SSD1306_WHITE);
  display.setTextSize(2);
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(line1, 0, 0, &x1, &y1, &w, &h);
  display.setCursor((SCREEN_WIDTH - (int)w) / 2, 28);
  display.println(line1);
  display.setTextSize(1);
  display.getTextBounds(line2, 0, 0, &x1, &y1, &w, &h);
  display.setCursor((SCREEN_WIDTH - (int)w) / 2, 47);
  display.println(line2);
  display.display();
}

void runVoiceMimic() {
  // Pause normal microphone reactions while capturing and replaying.
  bool oldAudioBusy = audioBusy;
  audioBusy = true;
  pendingSingleClap = false;
  soundActive = false;
  loudReactionPlayed = false;

  drawVoiceMimicScreen("READY...", "Speak in a moment");
  Serial.println();
  Serial.println("=== VOICE MIMIC DIAGNOSTICS ===");
  Serial.println("Recording for 5 seconds. Speak close to the INMP441...");
  delay(400);
  drawVoiceMimicScreen("LISTEN!", "Speak for 5 seconds");

  int captured = 0;
  int64_t peakLeft = 0;
  int64_t peakRight = 0;
  int64_t peakChosen = 0;
  unsigned long recordStart = millis();

  // Read stereo 32-bit I2S frames. INMP441 data may appear in either
  // slot depending on the L/R strap and driver slot selection, so inspect
  // both slots and select the stronger one for each short block.
  while (captured < VOICE_SAMPLE_COUNT && millis() - recordStart < 6000) {
    int availableBytes = I2S.available();
    if (availableBytes < (int)(2 * sizeof(int32_t))) {
      delay(1);
      continue;
    }

    size_t requestBytes = min((size_t)sizeof(micBuffer), (size_t)availableBytes);
    requestBytes -= requestBytes % (2 * sizeof(int32_t));
    if (requestBytes == 0) continue;

    size_t bytesRead = I2S.readBytes((char*)micBuffer, requestBytes);
    int frames = bytesRead / (2 * sizeof(int32_t));
    if (frames <= 0) continue;

    int64_t blockPeakLeft = 0;
    int64_t blockPeakRight = 0;
    for (int i = 0; i < frames; i++) {
      int64_t l = micBuffer[2 * i];
      int64_t r = micBuffer[2 * i + 1];
      if (l < 0) l = -l;
      if (r < 0) r = -r;
      if (l > blockPeakLeft) blockPeakLeft = l;
      if (r > blockPeakRight) blockPeakRight = r;
      if (l > peakLeft) peakLeft = l;
      if (r > peakRight) peakRight = r;
    }

    bool useRight = blockPeakRight > blockPeakLeft;
    for (int i = 0; i < frames && captured < VOICE_SAMPLE_COUNT; i++) {
      int32_t raw = micBuffer[2 * i + (useRight ? 1 : 0)];
      // INMP441's 24-bit signal is left-aligned in the 32-bit I2S slot.
      int32_t sample = raw >> 16;
      voiceSamples[captured++] = (int16_t)constrain(sample, -32768, 32767);
      int64_t magnitude = sample;
      if (magnitude < 0) magnitude = -magnitude;
      if (magnitude > peakChosen) peakChosen = magnitude;
    }
  }

  Serial.printf("Captured samples: %d / %d\n", captured, VOICE_SAMPLE_COUNT);
  Serial.printf("Raw I2S peak L: %lld, R: %lld\n",
                (long long)peakLeft, (long long)peakRight);
  Serial.printf("Chosen audio peak (16-bit): %lld\n", (long long)peakChosen);

  if (captured < 4000 || peakChosen < 8) {
    drawVoiceMimicScreen("NO MIC AUDIO", "Check mic signal");
    Serial.println("ERROR: microphone samples are missing or nearly silent.");
    Serial.println("Check INMP441 VDD=3V3, GND, SCK=GPIO4, WS=GPIO5, SD=GPIO3, L/R=GND.");
    delay(1400);
    audioBusy = oldAudioBusy;
    micIgnoreUntil = millis() + MIC_AFTER_SOUND_MS;
    return;
  }

  drawVoiceMimicScreen("MY TURN!", "Repeating you...");
  Serial.println("Playing recorded audio...");

  // Boost the previously working playback path. Clamp before converting to 32-bit I2S output.
  const float playbackGain = 6.0f;
  const float pitchRate = 1.12f;
  int outputFrames = (int)(captured / pitchRate);
  int outputIndex = 0;
  while (outputIndex < outputFrames) {
    int count = min(AUDIO_BUFFER_SAMPLES, outputFrames - outputIndex);
    for (int i = 0; i < count; i++) {
      int sourceIndex = min(captured - 1, (int)((outputIndex + i) * pitchRate));
      int32_t sample16 = (int32_t)((float)voiceSamples[sourceIndex] * playbackGain);
      sample16 = constrain(sample16, -30000, 30000);
      // Expand 16-bit PCM across the 32-bit I2S word for much stronger output.
      // Shift by 16 (not 11): the smaller shift was leaving output amplitude low.
      int32_t sample = sample16 * 65536L;
      audioBuffer[2 * i] = sample;
      audioBuffer[2 * i + 1] = sample;
    }
    I2S.write((uint8_t*)audioBuffer, count * 2 * sizeof(int32_t));
    outputIndex += count;
  }
  silenceAudio(100);
  delay(200);
  audioBusy = oldAudioBusy;
  micIgnoreUntil = millis() + MIC_AFTER_SOUND_MS;
  lastActivity = millis();
  face = EXCITED;
  excitedUntil = millis() + 900;
  Serial.println("Voice mimic: playback finished.");
  Serial.println("=== END VOICE MIMIC DIAGNOSTICS ===");
}

void handlePhysicalButtons() {
  unsigned long now = millis();
  updateButtonState(leftButton, LEFT_BUTTON_PIN, now);
  updateButtonState(rightButton, RIGHT_BUTTON_PIN, now);

  bool leftDown = leftButton.stableState == LOW;
  bool rightDown = rightButton.stableState == LOW;

  // Holding BOTH buttons takes priority over individual button actions.
  if (leftDown && rightDown) {
    if (!bothHoldHandled && now - max(leftButton.pressedAt, rightButton.pressedAt) >= BOTH_HOLD_MS) {
      bothHoldHandled = true;
      runVoiceMimic();
      drawNormalEyes(0);
      Serial.println("Secret voice-mimic shortcut completed.");
    }
    return;
  }
  if (!leftDown && !rightDown && bothHoldHandled) {
    bothHoldHandled = false;
    return;
  }

  // Left held: jump straight back to the main companion face.
  if (leftDown && !leftButton.longHandled && now - leftButton.pressedAt >= LEFT_HOLD_MS) {
    leftButton.longHandled = true;
    showIllusion = false;
    showSpectrum = false;
    showPhysics = false;
    pendingSingleClap = false;
    soundActive = false;
    drawNormalEyes(0);
    Serial.println("Returned to companion face");
    return;
  }

  // Left short press cycles Face -> Illusions -> Spectrum -> Physics -> Face.
  if (leftButton.releaseEvent && !leftButton.longHandled) {
    if (!showIllusion && !showSpectrum && !showPhysics) {
      showIllusion = true;
      showSpectrum = false;
      showPhysics = false;
      illusionType = 0;
      pendingSingleClap = false;
      soundActive = false;
      Serial.println("Illusion mode: Rubin vase. Right button cycles illusions.");
    } else if (showIllusion) {
      showIllusion = false;
      showSpectrum = true;
      showPhysics = false;
      memset(spectrumBars, 0, sizeof(spectrumBars));
      memset(spectrumPeaks, 0, sizeof(spectrumPeaks));
      spectrumSampleIndex = 0;
      pendingSingleClap = false;
      soundActive = false;
      Serial.println("Spectrum analyser mode");
    } else if (showSpectrum) {
      showSpectrum = false;
      showPhysics = true;
      resetPhysicsBalls(false);
      pendingSingleClap = false;
      soundActive = false;
      Serial.println("Tilt physics sandbox: tilt to roll balls; right button scatters them.");
    } else {
      showPhysics = false;
      showSpectrum = false;
      showIllusion = false;
      drawNormalEyes(0);
      lastActivity = now;
      Serial.println("Companion face mode");
    }
  }

  // Right short press: context-sensitive action.
  if (rightButton.releaseEvent && !rightButton.longHandled) {
    if (showIllusion) {
      illusionType = (illusionType + 1) % 3;
      if (illusionType == 0) Serial.println("Rubin vase selected");
      else if (illusionType == 1) Serial.println("Hypnotic tunnel selected");
      else if (illusionType == 2) Serial.println("Hermann grid selected");
    } else if (showSpectrum) {
      memset(spectrumPeaks, 0, sizeof(spectrumPeaks));
      Serial.println("Spectrum peak markers reset");
    } else if (showPhysics) {
      resetPhysicsBalls(true);
      Serial.println("Physics balls scattered!");
    } else {
      face = EXCITED;
      excitedUntil = now + 1800;
      soundExcited();
      lastActivity = millis();
      Serial.println("Happy reaction!");
    }
  }
}

void handleSpectrumSerial() {
  while (Serial.available() > 0) {
    char command = (char)Serial.read();
    if (command == 's' || command == 'S') {
      showSpectrum = !showSpectrum;
      if (showSpectrum) {
        showIllusion = false;
        showPhysics = false;
        memset(spectrumBars, 0, sizeof(spectrumBars));
        memset(spectrumPeaks, 0, sizeof(spectrumPeaks));
        spectrumSampleIndex = 0;
        // Clear any pending clap so it cannot trigger in spectrum mode.
        pendingSingleClap = false;
        soundActive = false;
        loudReactionPlayed = false;
        micIgnoreUntil = millis();
        Serial.println("Spectrum mode ON");
      } else {
        Serial.println("Spectrum mode OFF");
        drawNormalEyes(0);
      }
    } else if (command == 'p' || command == 'P') {
      showPhysics = !showPhysics;
      if (showPhysics) {
        showIllusion = false;
        showSpectrum = false;
        resetPhysicsBalls(false);
        pendingSingleClap = false;
        soundActive = false;
        Serial.println("Physics sandbox ON. Flat = gentle drift; tilt down/up to roll balls down/up. Right button scatters balls; send p to exit.");
      } else {
        Serial.println("Physics sandbox OFF");
        drawNormalEyes(0);
      }
    } else if (command == 'i' || command == 'I') {
      showIllusion = !showIllusion;
      if (showIllusion) {
        showSpectrum = false;
        showPhysics = false;
        illusionType = 0;
        pendingSingleClap = false;
        soundActive = false;
        loudReactionPlayed = false;
        Serial.println("Illusion mode ON. 1=Rubin vase, 2=tunnel, 3=Hermann grid. Send i to exit.");
      } else {
        Serial.println("Illusion mode OFF");
        drawNormalEyes(0);
      }
    } else if (command >= '1' && command <= '3' && showIllusion) {
      illusionType = command - '1';
      if (illusionType == 0) Serial.println("Rubin vase selected");
      else if (illusionType == 1) Serial.println("Hypnotic tunnel selected");
      else if (illusionType == 2) Serial.println("Hermann grid selected");
    } else if (command == 'r' || command == 'R') {
      memset(spectrumPeaks, 0, sizeof(spectrumPeaks));
      Serial.println("Spectrum peak markers reset");
    }
  }
}
void checkMicrophone() {
  if (audioBusy) return;
  unsigned long now = millis();
  // Ignore microphone input during and shortly after
  // speaker playback to reduce acoustic feedback.
  if ((long)(now - micIgnoreUntil) < 0) return;
  if (now - lastMicRead < MIC_SAMPLE_INTERVAL) return;
  lastMicRead = now;
  int availableBytes = I2S.available();
  if (availableBytes < (int)sizeof(micBuffer)) return;
  size_t bytesRead = I2S.readBytes(
    (char*)micBuffer,
    sizeof(micBuffer)
  );
  if (bytesRead < 8) return;
  int frames = bytesRead / (2 * sizeof(int32_t));
  addSpectrumSamples(frames);
  // In spectrum mode, use microphone samples only for the FFT.
  // Skip clap, double-clap, and sustained-loud-sound detection.
  if (showSpectrum) return;
  int64_t peak = 0;
  for (int i = 0; i < frames * 2; i++) {
    int64_t sample = micBuffer[i];
    if (sample < 0) sample = -sample;
    if (sample > peak) peak = sample;
  }
  // Serial Monitor diagnostics
  static unsigned long lastPrint = 0;
  if (now - lastPrint >= 250) {
    lastPrint = now;
    Serial.print("Mic peak: ");
    Serial.println((long long)peak);
  }
  bool loudNow = peak >= MIC_THRESHOLD;
  // ----------------------------------------------
  // SOUND IS ABOVE THRESHOLD
  // ----------------------------------------------
  if (loudNow) {
    if (!soundActive) {
      soundActive = true;
      loudStartedAt = now;
      loudReactionPlayed = false;
    }
    lastLoudAt = now;
    // Sustained loud sound
    if (!loudReactionPlayed &&
        now - loudStartedAt >= LOUD_DURATION_MS) {
      loudReactionPlayed = true;
      lastActivity = now;
      // A sustained sound takes priority over a pending
      // single clap.
      pendingSingleClap = false;
      if (face != SHAKING) {
        face = LOUD;
        loudUntil = now + LOUD_DURATION_FACE;
        Serial.println("Sustained loud sound detected!");
        soundLoud();
      }
    }
    return;
  }
  // ----------------------------------------------
  // WAIT UNTIL THE SOUND HAS ENDED
  // ----------------------------------------------
  if (!soundActive ||
      now - lastLoudAt < SOUND_RELEASE_MS) {
    return;
  }
  unsigned long soundDuration =
    lastLoudAt - loudStartedAt;
  soundActive = false;
  // Sustained sound already handled above
  if (loudReactionPlayed) {
    loudReactionPlayed = false;
    return;
  }
  // Reject extremely short spikes
  if (soundDuration < 15) return;
  // ----------------------------------------------
  // SHORT SOUND: POSSIBLE CLAP OR TAP
  // ----------------------------------------------
  if (soundDuration < LOUD_DURATION_MS) {
    lastActivity = now;
    // Wake the robot if sleeping
    if (face == SLEEPY) {
      face = NORMAL;
      eyesClosed = false;
      targetX = 0;
      targetY = 0;
      lastBlink = now;
    }
    if (face == SHAKING) return;
    // SECOND CLAP: arrived within the double-clap window
    if (pendingSingleClap &&
        now - singleClapAt <= DOUBLE_CLAP_WINDOW) {
      pendingSingleClap = false;
      face = EXCITED;
      excitedUntil = now + EXCITED_DURATION;
      Serial.println("DOUBLE CLAP DETECTED!");
      soundExcited();
      return;
    }
    // If the previous clap's window expired, the
    // pending handler will have played its sound.
    // This sound now starts a new single-clap window.
    pendingSingleClap = true;
    singleClapAt = now;
    face = SURPRISED;
    surprisedUntil = now + SURPRISE_DURATION;
    Serial.println("First clap detected; waiting for second.");
  }
}
// ==================================================
// DELAYED SINGLE-CLAP SOUND
// ==================================================
void handlePendingClap() {
  if (!pendingSingleClap) return;
  if (millis() - singleClapAt >= DOUBLE_CLAP_WINDOW) {
    pendingSingleClap = false;
    Serial.println("Single clap confirmed.");
    soundSurprise();
  }
}
// ==================================================
// NORMAL WHITE EYES
// ==================================================
void drawNormalEyes(int bob) {
  display.clearDisplay();
  int x1 = 12 + glanceX;
  int x2 = 78 + glanceX;
  int y = 16 + glanceY + bob;
  display.fillRoundRect(
    x1, y, 38, 32, 5, SSD1306_WHITE
  );
  display.fillRoundRect(
    x2, y, 38, 32, 5, SSD1306_WHITE
  );
  display.display();
}
// ==================================================
// SLEEPY EYES
// ==================================================
void drawSleepyEyes() {
  display.clearDisplay();
  display.fillRoundRect(
    12, 22, 38, 20, 5, SSD1306_WHITE
  );
  display.fillRoundRect(
    78, 22, 38, 20, 5, SSD1306_WHITE
  );
  display.fillRect(
    12, 22, 38, 12, SSD1306_BLACK
  );
  display.fillRect(
    78, 22, 38, 12, SSD1306_BLACK
  );
  display.display();
}
// ==================================================
// BLINK
// ==================================================
void drawClosedEyes() {
  display.clearDisplay();
  display.fillRoundRect(
    12, 29, 38, 6, 3, SSD1306_WHITE
  );
  display.fillRoundRect(
    78, 29, 38, 6, 3, SSD1306_WHITE
  );
  display.display();
}
// ==================================================
// SURPRISED EYES
// ==================================================
void drawSurprisedEyes() {
  display.clearDisplay();
  display.fillRoundRect(
    8, 5, 48, 54, 7, SSD1306_WHITE
  );
  display.fillRoundRect(
    72, 5, 48, 54, 7, SSD1306_WHITE
  );
  display.display();
}
// ==================================================
// SHAKING EYES
// ==================================================
void drawShakingEyes(unsigned long now) {
  display.clearDisplay();
  display.fillRoundRect(
    23, 16, 34, 32, 5, SSD1306_WHITE
  );
  display.fillRoundRect(
    71, 16, 34, 32, 5, SSD1306_WHITE
  );
  bool phase = (now / 120) % 2;
  if (!phase) {
    display.drawCircleHelper(
      25, 32, 25, 0b00001001, SSD1306_WHITE
    );
    display.drawCircleHelper(
      25, 32, 17, 0b00001001, SSD1306_WHITE
    );
    display.drawCircleHelper(
      103, 32, 25, 0b00000110, SSD1306_WHITE
    );
    display.drawCircleHelper(
      103, 32, 17, 0b00000110, SSD1306_WHITE
    );
  } else {
    display.drawCircleHelper(
      23, 32, 27, 0b00001001, SSD1306_WHITE
    );
    display.drawCircleHelper(
      23, 32, 19, 0b00001001, SSD1306_WHITE
    );
    display.drawCircleHelper(
      105, 32, 27, 0b00000110, SSD1306_WHITE
    );
    display.drawCircleHelper(
      105, 32, 19, 0b00000110, SSD1306_WHITE
    );
  }
  display.display();
}
// ==================================================
// EXCITED EYES
// ==================================================
void drawExcitedEyes(unsigned long now) {
  display.clearDisplay();
  int bob = ((now / 100) % 2 == 0) ? 0 : 2;
  display.fillRoundRect(
    8, 12 + bob, 44, 38, 10, SSD1306_WHITE
  );
  display.fillRoundRect(
    76, 12 + bob, 44, 38, 10, SSD1306_WHITE
  );
  display.drawLine(16, 5, 11, 1, SSD1306_WHITE);
  display.drawLine(32, 5, 32, 0, SSD1306_WHITE);
  display.drawLine(48, 5, 53, 1, SSD1306_WHITE);
  display.drawLine(80, 5, 75, 1, SSD1306_WHITE);
  display.drawLine(96, 5, 96, 0, SSD1306_WHITE);
  display.drawLine(112, 5, 117, 1, SSD1306_WHITE);
  display.display();
}
// ==================================================
// LOUD SOUND ALERT EYES
// ==================================================
void drawLoudEyes() {
  display.clearDisplay();
  display.fillRoundRect(
    8, 19, 48, 25, 8, SSD1306_WHITE
  );
  display.fillRoundRect(
    72, 19, 48, 25, 8, SSD1306_WHITE
  );
  display.fillRect(
    8, 19, 48, 8, SSD1306_BLACK
  );
  display.fillRect(
    72, 19, 48, 8, SSD1306_BLACK
  );
  display.display();
}
// ==================================================
// SETUP
// ==================================================
void setup() {
  Serial.begin(115200);
  pinMode(LEFT_BUTTON_PIN, INPUT_PULLUP);
  pinMode(RIGHT_BUTTON_PIN, INPUT_PULLUP);
  // OLED and MPU6050 share I2C
  Wire.begin(0, 1);
  if (!display.begin(
        SSD1306_SWITCHCAPVCC,
        OLED_ADDRESS)) {
    Serial.println("OLED not detected!");
    while (true) delay(100);
  }
  // Wake MPU6050
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x6B);
  Wire.write(0x00);
  if (Wire.endTransmission() != 0) {
    Serial.println("MPU6050 wake failed!");
  }
  // Shared I2S clocks; separate data pins
  I2S.setPins(
    BCLK_PIN,
    LRC_PIN,
    SPEAKER_DIN_PIN,
    MIC_SD_PIN
  );
  if (!I2S.begin(
        I2S_MODE_STD,
        SAMPLE_RATE,
        I2S_DATA_BIT_WIDTH_32BIT,
        I2S_SLOT_MODE_STEREO)) {
    Serial.println("I2S initialization failed!");
    while (true) delay(1000);
  }
  delay(100);
  if (readMPU()) {
    prevAx = ax;
    prevAy = ay;
    prevAz = az;
  }
  randomSeed(micros());
  lastActivity = millis();
  lastBlink = millis();
  Serial.println("Robot starting...");
  soundWake();
  lastActivity = millis();
  lastBlink = millis();
  drawNormalEyes(0);
  Serial.println("--------------------------------");
  Serial.println("Robot integrated system ready!");
  Serial.println("Mic threshold: 80000000");
  Serial.println("One clap: surprised");
  Serial.println("Two claps: excited");
  Serial.println("Sustained loud sound: alert");
  Serial.println("Buttons: LEFT cycles Face/Illusions/Spectrum/Physics; RIGHT selects, resets peaks, scatters physics balls, or reacts; hold LEFT to return home; hold BOTH 5s to record and replay your voice; open Serial Monitor at 115200 for mic diagnostics.");
  Serial.println("Type s to toggle spectrum mode; r resets spectrum peaks");
  Serial.println("Type i for illusions: 1=Rubin vase, 2=hypnotic tunnel, 3=Hermann grid");
  Serial.println("Type p to toggle physics sandbox; keep board flat for gentle drift, tilt to roll balls.");
  Serial.println("--------------------------------");
}
// ==================================================
// MAIN LOOP
// ==================================================
void loop() {
  handlePhysicalButtons();
  handleSpectrumSerial();

  // Spectrum mode is dedicated to audio visualisation:
  // pause movement reactions and all robot sound reactions.
  if (showSpectrum) {
    checkMicrophone();
    unsigned long spectrumNow = millis();
    if (spectrumNow - lastDraw >= 50) {
      lastDraw = spectrumNow;
      drawSpectrum();
    }
    return;
  }

  // Physics mode uses the MPU6050 as gravity input and pauses companion reactions.
  if (showPhysics) {
    unsigned long physicsNow = millis();
    updatePhysics(physicsNow);
    if (physicsNow - lastDraw >= 33) {
      lastDraw = physicsNow;
      drawPhysics();
    }
    delay(2);
    return;
  }

  // Illusion mode pauses normal movement and sound reactions.
  if (showIllusion) {
    unsigned long illusionNow = millis();
    if (illusionNow - lastDraw >= 100) {
      lastDraw = illusionNow;
      if (illusionType == 0) drawRubinVase(illusionNow);
      else if (illusionType == 1) drawHypnoticTunnel(illusionNow);
      else if (illusionType == 2) drawHermannGrid();
    }
    delay(5);
    return;
  }

  unsigned long now = millis();
  // ----------------------------------------------
  // READ MPU6050
  // ----------------------------------------------
  if (now - lastRead >= 25) {
    lastRead = now;
    if (readMPU()) {
      long dx = abs((long)ax - prevAx);
      long dy = abs((long)ay - prevAy);
      long dz = abs((long)az - prevAz);
      long movement = dx + dy + dz;
      if (movement > MOVEMENT_THRESHOLD) {
        lastActivity = now;
        if (face == SLEEPY) {
          face = NORMAL;
          eyesClosed = false;
          targetX = 0;
          targetY = 0;
          soundWake();
          lastActivity = millis();
          lastBlink = millis();
        }
      }
      // Sudden movement -> surprise
      if (movement > SUDDEN_MOVEMENT_THRESHOLD &&
          face != SHAKING &&
          face != SURPRISED &&
          face != EXCITED &&
          face != LOUD) {
        face = SURPRISED;
        surprisedUntil = millis() + SURPRISE_DURATION;
        eyesClosed = false;
        Serial.println("Sudden movement!");
        soundSurprise();
      }
      // Repeated sharp movements -> shake
      if (movement > SHAKE_THRESHOLD &&
          millis() - lastShakeHit > 100) {
        lastShakeHit = millis();
        if (millis() - shakeWindowStart > 1200) {
          shakeWindowStart = millis();
          shakeCount = 0;
        }
        shakeCount++;
        if (shakeCount >= 4 && face != SHAKING) {
          face = SHAKING;
          shakingUntil = millis() + SHAKE_DURATION;
          shakeCount = 0;
          shakeWindowStart = millis();
          Serial.println("Robot shaken!");
          soundShake();
          shakingUntil = millis() + SHAKE_DURATION;
        }
      }
      // Existing tested tilt mapping
      if (face == NORMAL) {
        if (ay > 5000) targetX = -7;
        else if (ay < -5000) targetX = 7;
        else targetX = 0;
        if (ax > 5000) targetY = -5;
        else if (ax < -5000) targetY = 5;
        else targetY = 0;
      }
      prevAx = ax;
      prevAy = ay;
      prevAz = az;
    }
  }
  // ----------------------------------------------
  // MICROPHONE + DELAYED CLAP
  // ----------------------------------------------
  checkMicrophone();
  handlePendingClap();
  now = millis();
  // ----------------------------------------------
  // END TEMPORARY REACTIONS
  // ----------------------------------------------
  if (face == SURPRISED &&
      (long)(now - surprisedUntil) >= 0) {
    face = NORMAL;
    lastActivity = now;
    lastBlink = now;
  }
  if (face == SHAKING &&
      (long)(now - shakingUntil) >= 0) {
    face = NORMAL;
    targetX = 0;
    targetY = 0;
    lastActivity = now;
    lastBlink = now;
  }
  if (face == EXCITED &&
      (long)(now - excitedUntil) >= 0) {
    face = NORMAL;
    lastActivity = now;
    lastBlink = now;
  }
  if (face == LOUD &&
      (long)(now - loudUntil) >= 0) {
    face = NORMAL;
    lastActivity = now;
    lastBlink = now;
  }
  // ----------------------------------------------
  // SLEEP AFTER INACTIVITY
  // ----------------------------------------------
  if (face == NORMAL &&
      now - lastActivity >= SLEEP_AFTER) {
    face = SLEEPY;
    eyesClosed = false;
    // A pending single clap shouldn't play while sleeping
    pendingSingleClap = false;
    Serial.println("Going to sleep...");
    soundSleep();
    lastActivity = millis();
  }
  // ----------------------------------------------
  // SMOOTH EYE MOVEMENT
  // ----------------------------------------------
  if (glanceX < targetX) glanceX++;
  if (glanceX > targetX) glanceX--;
  if (glanceY < targetY) glanceY++;
  if (glanceY > targetY) glanceY--;
  // ----------------------------------------------
  // BLINKING
  // ----------------------------------------------
  if (face == NORMAL) {
    if (!eyesClosed &&
        now - lastBlink >= BLINK_INTERVAL) {
      eyesClosed = true;
      blinkEnd = now + BLINK_DURATION;
    }
    if (eyesClosed &&
        (long)(now - blinkEnd) >= 0) {
      eyesClosed = false;
      lastBlink = now;
    }
  } else {
    eyesClosed = false;
  }
  // ----------------------------------------------
  // SUBTLE IDLE BOB
  // ----------------------------------------------
  int bob = 0;
  if (face == NORMAL) {
    bob = ((now / 700) % 2 == 0) ? 0 : 1;
  }
  // ----------------------------------------------
  // REFRESH DISPLAY
  // ----------------------------------------------
  if (now - lastDraw >= 50) {
    lastDraw = now;
    if (showSpectrum) {
      drawSpectrum();
    } else if (face == NORMAL) {
      if (eyesClosed) {
        drawClosedEyes();
      } else {
        drawNormalEyes(bob);
      }
    } else if (face == SLEEPY) {
      drawSleepyEyes();
    } else if (face == SURPRISED) {
      drawSurprisedEyes();
    } else if (face == SHAKING) {
      drawShakingEyes(now);
    } else if (face == EXCITED) {
      drawExcitedEyes(now);
    } else if (face == LOUD) {
      drawLoudEyes();
    }
  }
}
