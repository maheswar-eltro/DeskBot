# DeskBot — System Architecture

## 1. Introduction

DeskBot is an ESP32-C3-based interactive desktop companion that combines embedded programming, digital audio processing, sensor integration, real-time graphics and interactive behaviour.

The ESP32-C3 acts as the central processing unit, coordinating the OLED display, microphone, audio amplifier, motion sensor and physical inputs.

The system integrates several independent experiences into one embedded platform:

- Interactive OLED Companion
- Real-Time Audio Spectrum Analyzer
- Optical Illusion Engine
- Tilt-Controlled Physics Simulator
- Voice Mimic

The firmware combines sensor acquisition, event detection, audio processing, state management and display rendering.

## 2. System Architecture Overview

The ESP32-C3 coordinates all peripheral communication and application logic.

### Main Controller

**ESP32-C3 Super Mini**

Responsibilities:

- Reads microphone data through I2S.
- Processes audio samples for sound detection and spectrum analysis.
- Generates digital audio output for the MAX98357A.
- Reads accelerometer and gyroscope data from the MPU6050.
- Controls OLED graphics and animations.
- Handles physical button inputs.
- Manages the different operating modes.

### Peripheral Architecture

| Peripheral | Interface | Primary Function |
|---|---|---|
| SSD1306 OLED | I2C | Visual output and animations |
| MPU6050 | I2C | Motion and orientation sensing |
| INMP441 | I2S | Digital audio input |
| MAX98357A | I2S | Digital audio amplification |
| Push Buttons | GPIO | User input |

## 3. Firmware Architecture

The integrated firmware is primarily organised around the following functional blocks:

### A. Hardware Initialisation

During `setup()`, the firmware:

1. Initialises Serial communication at 115200 baud.
2. Configures both push buttons as inputs with internal pull-ups.
3. Initialises the shared I2C bus.
4. Initialises the SSD1306 OLED.
5. Wakes the MPU6050 from sleep mode.
6. Configures the I2S interface for audio input and output.
7. Reads the initial MPU6050 measurements.
8. Initialises activity and animation timers.
9. Starts the companion in its normal visual state.

### B. Main Execution Loop

The `loop()` function continuously coordinates:

- Physical button handling
- Serial command handling
- Microphone sampling
- Motion detection
- Audio reactions
- Operating-mode selection
- Physics updates
- OLED rendering

Dedicated modes temporarily take control of the main loop. For example, spectrum mode prioritises audio visualisation, while physics mode prioritises motion-based simulation.

### C. Face State Management

The companion's expressions are controlled using a `FaceState` enumeration.

The implemented states are:

| State | Behaviour |
|---|---|
| NORMAL | Default animated companion face |
| SLEEPY | Displays sleepy eyes after inactivity |
| SURPRISED | Reacts to sudden movement or a clap |
| SHAKING | Reacts to repeated sharp movements |
| EXCITED | Reacts to double claps and user interaction |
| LOUD | Reacts to sustained loud sound |

Temporary expressions return to the normal state after their configured durations.

## 4. Operating Modes

DeskBot has four primary operating modes.

### Mode 1 — Interactive OLED Companion

This is the default operating mode.

The OLED displays animated eyes, with the ESP32-C3 continuously monitoring microphone input and MPU6050 measurements.

Implemented behaviours include:

**Idle Behaviour**
- Smooth eye movement.
- Periodic blinking.
- Subtle idle animation.
- Sleepy expression after approximately 7 seconds of inactivity.

**Sound-Based Reactions**
- Single clap: surprised expression.
- Double clap: excited expression.
- Sustained loud sound: loud/alert expression.

**Motion-Based Reactions**
- Sudden movement triggers a surprised expression.
- Repeated sharp movements trigger a shaking expression.
- Board orientation controls the direction of eye movement.
- Movement can wake the companion from its sleepy state.

The firmware uses activity timestamps and temporary state durations to coordinate these behaviours.

### Mode 2 — Real-Time Audio Spectrum Analyzer

The spectrum analyser converts incoming microphone samples into a visual frequency representation.

**Signal Processing Pipeline**

1. The INMP441 supplies digital audio samples through I2S.
2. The ESP32-C3 collects microphone samples.
3. A block of 128 samples is processed.
4. The mean value is removed from the sample block.
5. A Hann window is applied.
6. A custom radix-2 Cooley-Tukey FFT is performed.
7. Frequency bins are grouped into 16 frequency bands.
8. The resulting values are scaled and displayed on the OLED.

**Visualisation Features**
- 16 frequency bars.
- Dynamic bar heights.
- Peak-hold markers.
- Peak marker reset functionality.
- Logarithmic compression for visual scaling.

The spectrum analyser uses a custom FFT implementation rather than relying on a separate FFT library.

While this mode is active, normal companion sound and movement reactions are paused.

### Mode 3 — Optical Illusion Engine

The optical illusion engine provides three visual experiments on the OLED.

**1. Rubin's Vase**

A figure-ground perception illusion in which the viewer can perceive either a central vase or two opposing face profiles.

The firmware renders the illusion using a monochrome bitmap.

**2. Hypnotic Tunnel**

An animated tunnel composed of nested circles.

The circle positions are modified using time-dependent trigonometric calculations to create a moving visual effect.

**3. Hermann Grid**

A grid-based optical illusion consisting of contrasting squares and intersections.

The display renders the pattern using monochrome graphics.

The right button cycles through the three illusions.

Normal companion reactions are paused while illusion mode is active.

### Mode 4 — Tilt-Controlled Physics Simulator

The physics simulator uses the MPU6050 to control the apparent direction of gravity.

The OLED displays 40 particles moving inside the screen boundaries.

**Simulation Features**
- 40 simulated particles.
- MPU6050-based tilt interaction.
- Velocity-based particle movement.
- Boundary collision detection.
- Particle-to-particle collision handling.
- Positional overlap correction.
- Particle scattering through user input.

The physics update loop reads the MPU6050 approximately every 20 milliseconds.

The simulation also incorporates damping and collision response to control particle movement.

The right button triggers a particle-scattering effect.

Normal companion reactions are paused while physics mode is active.

## 5. Voice Mimic System

DeskBot includes two voice-mimic implementations.

Both use the INMP441 microphone, I2S audio input, ESP32-C3 processing, MAX98357A amplification and a speaker.

### Audio Processing Pipeline

1. The microphone captures incoming audio.
2. The ESP32-C3 reads stereo 32-bit I2S frames.
3. The firmware selects the stronger microphone channel from each frame.
4. Captured samples are stored in memory.
5. The recorded audio is converted into the playback representation.
6. Playback gain and sample indexing are applied.
7. The processed audio is transmitted through I2S to the MAX98357A.
8. The amplifier drives the speaker.

### Implementation A — 5-Second Voice Mimic

Source:

`src/main/bot/mimic/5secs/5secs.ino`

Characteristics:

- 5-second recording duration.
- 16-bit mono PCM storage.
- 16 kHz sampling rate.
- Approximately 80,000 samples.
- Approximately 160 KB of audio storage.

### Implementation B — 10-Second Voice Mimic

Source:

`src/main/bot/mimic/10secs/10secs.ino`

Characteristics:

- 10-second recording duration.
- 8-bit mono PCM storage.
- 16 kHz sampling rate.
- Approximately 160,000 samples.
- Approximately 160 KB of audio storage.

The two implementations explore different recording durations and audio storage representations within the memory constraints of the ESP32-C3.

The integrated prototype also contains a voice-mimic routine triggered by holding both physical buttons for approximately five seconds.

## 6. User Interaction and Controls

DeskBot supports physical button inputs and Serial Monitor commands.

### Physical Button Controls

| Input | Action |
|---|---|
| Left short press | Cycle through Face → Illusions → Spectrum → Physics → Face |
| Right short press in Face mode | Trigger excited reaction |
| Right short press in Illusion mode | Cycle through illusions |
| Right short press in Spectrum mode | Reset peak markers |
| Right short press in Physics mode | Scatter particles |
| Hold Left | Return to companion face |
| Hold Both for approximately 5 seconds | Trigger voice mimic |

### Serial Monitor Commands

Serial communication operates at 115200 baud.

| Command | Action |
|---|---|
| `s` | Toggle spectrum analyser |
| `i` | Toggle illusion mode |
| `p` | Toggle physics simulator |
| `1` | Select Rubin's Vase |
| `2` | Select Hypnotic Tunnel |
| `3` | Select Hermann Grid |
| `r` | Reset spectrum peak markers |

The numbered illusion commands are used while illusion mode is active.

## 7. Communication and Data Flow

### Audio Input Path

`INMP441 → I2S → ESP32-C3`

Microphone samples are processed for sound detection, audio visualisation and voice recording.

### Audio Output Path

`ESP32-C3 → I2S → MAX98357A → Speaker`

The ESP32-C3 generates digital audio data, which is amplified by the MAX98357A.

### Motion Input Path

`MPU6050 → I2C → ESP32-C3`

Motion and orientation measurements are used for companion reactions and physics simulation.

### Display Output Path

`ESP32-C3 → I2C → SSD1306 OLED`

The controller generates the graphics and animations displayed on the OLED.

### User Input Path

`Push Buttons → GPIO → ESP32-C3`

Physical button inputs allow the user to switch modes and trigger interactive actions.

## 8. Software Organisation

The repository separates the integrated firmware, standalone experiments and hardware documentation.

```text
DeskBot/
│
├── README.md
│
├── documentation/
│   ├── design.md
│   ├── notes.md
│   ├── progress.md
│   └── system-architecture.md
│
├── hardware/
│   ├── components.md
│   ├── wiring.md
│   └── pinout.md
│
└── src/
    ├── main/
    │   └── bot/
    │       ├── eyes/
    │       ├── mimic/
    │       │   ├── 5secs/
    │       │   └── 10secs/
    │       └── proto/
    │
    └── tests/
        ├── audio/
        ├── display/
        ├── sensors/
        └── servo/
```

The `proto` directory contains the integrated prototype firmware, while the other directories preserve individual experiments and alternative implementations.

## 9. Design Approach

DeskBot was developed incrementally by integrating independently tested hardware and software components.

The development approach involved:

- Testing individual peripherals before integration.
- Establishing I2C and I2S communication.
- Developing OLED graphics and animations.
- Implementing microphone-based sound detection.
- Integrating motion sensing.
- Developing interactive operating modes.
- Combining the subsystems into an integrated firmware prototype.
- Designing a custom PCB to consolidate hardware connections.

This modular development approach made it possible to experiment with individual features before integrating them into the complete system.

## 10. Current System Status

The integrated prototype includes the companion interface, audio spectrum analyser, optical illusions, tilt-controlled physics simulator and voice-mimic functionality.

The project also includes a custom KiCad PCB design intended to consolidate the hardware connections.

For component details, refer to [`../hardware/components.md`](../hardware/components.md).

For pin assignments, refer to [`../hardware/pinout.md`](../hardware/pinout.md).

For electrical connections, refer to [`../hardware/wiring.md`](../hardware/wiring.md).