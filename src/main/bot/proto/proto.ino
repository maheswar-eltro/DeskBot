
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

// ==================================================
// MICROPHONE SETTINGS
// ==================================================

const int64_t MIC_THRESHOLD = 80000000LL;

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
  int32_t amplitude = 18000000
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

  playTone(850, 90, 18000000);
  silenceAudio(35);
  playTone(1350, 150, 20000000);
  silenceAudio(50);

  audioBusy = false;
  micIgnoreUntil = millis() + MIC_AFTER_SOUND_MS;
}

void soundShake() {
  audioBusy = true;

  for (int i = 0; i < 4; i++) {
    playTone(1050, 65, 18000000);
    playTone(650, 65, 18000000);
  }

  silenceAudio(30);

  audioBusy = false;
  micIgnoreUntil = millis() + MIC_AFTER_SOUND_MS;
}

void soundSleep() {
  audioBusy = true;

  playTone(950, 100, 12000000);
  silenceAudio(40);

  playTone(720, 140, 10000000);
  silenceAudio(60);

  playTone(520, 190, 8500000);
  silenceAudio(80);

  playTone(380, 260, 7000000);
  silenceAudio(40);

  audioBusy = false;
  micIgnoreUntil = millis() + MIC_AFTER_SOUND_MS;
}

void soundWake() {
  audioBusy = true;

  playTone(420, 120, 8000000);
  silenceAudio(35);

  playTone(620, 120, 10000000);
  silenceAudio(35);

  playTone(820, 150, 12000000);
  silenceAudio(40);

  playTone(1100, 180, 13000000);
  silenceAudio(30);

  audioBusy = false;
  micIgnoreUntil = millis() + MIC_AFTER_SOUND_MS;
}

void soundExcited() {
  audioBusy = true;

  playTone(1100, 70, 11000000);
  silenceAudio(25);

  playTone(1500, 70, 13000000);
  silenceAudio(25);

  playTone(1900, 110, 14000000);

  audioBusy = false;
  micIgnoreUntil = millis() + MIC_AFTER_SOUND_MS;
}

void soundLoud() {
  audioBusy = true;

  playTone(500, 180, 10000000);
  silenceAudio(40);

  playTone(700, 250, 12000000);

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
  Serial.println("--------------------------------");
}

// ==================================================
// MAIN LOOP
// ==================================================

void loop() {
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

    if (face == NORMAL) {
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
