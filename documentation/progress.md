# DeskBot — Development Progress

This document tracks the development of DeskBot, from initial hardware experimentation to firmware integration and custom PCB design.

The project is being developed incrementally, with individual peripheral testing followed by subsystem integration and system-level testing.

## 1. Project Development Overview

| Development Stage | Status |
|---|---|
| Initial hardware experimentation | Completed |
| Individual peripheral testing | Completed |
| OLED graphics and animations | Completed |
| Audio input and output integration | Completed |
| Motion sensing integration | Completed |
| Interactive companion behaviour | Completed |
| Audio spectrum analyser | Completed |
| Optical illusion engine | Completed |
| Physics simulator | Completed |
| Voice mimic implementations | Completed |
| Integrated firmware prototype | Completed |
| Custom PCB schematic and layout | Designed |
| PCB design-rule verification | Initial checks passed with a non-electrical footprint warning |
| PCB fabrication | Pending |
| Physical PCB assembly and testing | Pending |

## 2. Development Milestones

### Phase 1 — Initial Hardware Setup

- Selected the ESP32-C3 Super Mini as the primary controller.
- Acquired and tested the OLED display, MPU6050, INMP441 microphone and MAX98357A amplifier.
- Established the initial breadboard-based hardware setup.
- Tested individual peripherals before integrating them into the main firmware.

**Status:** Completed

### Phase 2 — OLED Display and Graphics

- Established communication with the SSD1306 OLED display.
- Implemented animated eyes and facial expressions.
- Developed multiple eye states and visual reactions.
- Created standalone display experiments before integrating graphics into the main firmware.

**Status:** Completed

### Phase 3 — Sensor Integration

- Integrated the MPU6050 accelerometer and gyroscope.
- Established I2C communication between the ESP32-C3 and sensor.
- Implemented motion and orientation detection.
- Used sensor readings to control companion reactions and physics simulation.

**Status:** Completed

### Phase 4 — Audio Input and Output

- Integrated the INMP441 digital MEMS microphone.
- Established I2S communication for microphone input.
- Integrated the MAX98357A I2S amplifier.
- Implemented audio output through a connected speaker.
- Developed sound-based interaction and audio processing functionality.

**Status:** Completed

### Phase 5 — Interactive OLED Companion

- Implemented the main companion behaviour.
- Developed normal, sleepy, surprised, shaking, excited and loud expression states.
- Integrated sound-triggered reactions.
- Implemented clap detection and double-clap interaction.
- Integrated motion-based reactions using the MPU6050.
- Implemented inactivity-based sleepy behaviour and wake-up responses.

**Status:** Completed and tested on the working prototype.

### Phase 6 — Real-Time Audio Spectrum Analyser

- Implemented microphone sample acquisition.
- Developed a custom radix-2 Cooley-Tukey FFT implementation.
- Incorporated mean removal and Hann windowing.
- Implemented frequency-bin grouping into 16 visual bands.
- Added dynamic spectrum bars and peak-hold indicators.

**Status:** Completed

### Phase 7 — Optical Illusion Engine

Implemented three monochrome visual experiments:

1. **Rubin's Vase** — Figure-ground perception illusion.
2. **Hypnotic Tunnel** — Animated concentric-circle effect.
3. **Hermann Grid** — Grid-based optical illusion.

Integrated the experiments into the OLED operating modes.

**Status:** Completed

### Phase 8 — Tilt-Controlled Physics Simulator

- Implemented a 40-particle simulation.
- Integrated MPU6050-based tilt interaction.
- Added velocity-based particle movement.
- Implemented boundary collision detection.
- Added particle-to-particle collision handling.
- Implemented positional overlap correction and particle-scattering interaction.

**Status:** Completed

### Phase 9 — Voice Mimic Implementations

Developed two independent voice-mimic implementations.

**5-Second Implementation**

- 16-bit mono PCM audio storage.
- 16 kHz sampling rate.
- 80,000 audio samples.
- Approximately 160 KB of sample storage.

Source: `src/main/bot/mimic/5secs/5secs.ino`

**10-Second Implementation**

- 8-bit mono PCM audio storage.
- 16 kHz sampling rate.
- 160,000 audio samples.
- Approximately 160 KB of sample storage.

Source: `src/main/bot/mimic/10secs/10secs.ino`

Both implementations use the INMP441 microphone and MAX98357A amplifier through the ESP32-C3 I2S audio interface.

**Status:** Both implementations developed and tested independently.

### Phase 10 — Integrated Firmware Prototype

- Consolidated the major hardware and software subsystems into the integrated prototype.
- Implemented operating-mode selection.
- Integrated physical button controls.
- Added Serial Monitor commands for operating-mode control.
- Combined audio processing, sensor interaction, OLED graphics and simulation functionality.
- Established the working USB-powered breadboard prototype.

**Status:** Completed and tested on the working hardware prototype.

### Phase 11 — Custom PCB Design

- Developed a custom PCB using KiCad.
- Created the corresponding schematic.
- Designed connections for the ESP32-C3 and peripheral modules.
- Integrated power, I2C, I2S and GPIO connections.
- Added two physical push-button connections.
- Routed the PCB using front and back copper layers.
- Added GND copper zones on both layers.
- Incorporated PCB branding.

**PCB design specifications**

| Parameter | Specification |
|---|---|
| PCB dimensions | 74.55 × 60.55 mm |
| Copper layers | 2 |
| Board thickness | 1.6 mm |
| Substrate | FR4 |
| Design software | KiCad |

**Initial DRC results**

- 0 PCB violations.
- 0 unconnected items.
- 1 schematic-parity warning associated with an extra footprint (`REF**`), identified as the PCB logo.
- 5 ignored design-rule tests.

**Status:** PCB designed; initial design-rule checks completed. Fabrication and physical board validation remain pending.

## 3. Current Project Status

DeskBot has progressed from individual peripheral experiments to an integrated ESP32-C3-based interactive prototype.

The major software features have been implemented and tested on the working hardware prototype.

The custom PCB design has also been developed to consolidate the hardware connections into a dedicated board.

The remaining work is focused on documentation, final PCB review, fabrication, assembly and hardware validation.

## 4. Remaining Tasks

- [ ] Complete and review all repository documentation.
- [ ] Fabricate the custom PCB.
- [ ] Assemble the PCB with the required modules.
- [ ] Test power distribution and peripheral connections on the assembled PCB.
- [ ] Validate all operating modes on the assembled board.
- [ ] Document final hardware testing and any modifications.

## 5. Development Philosophy

DeskBot is being developed through incremental experimentation and hardware-software integration.

Individual components are tested independently before being incorporated into the integrated prototype. The project explores embedded programming, digital signal processing, sensor integration, real-time graphics, audio systems and human–robot interaction.

The custom PCB represents the transition from a breadboard-based prototype towards a more consolidated hardware implementation.

---

*This document is updated as new features, hardware revisions and development milestones are completed.*