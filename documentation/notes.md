# DeskBot — Engineering Notes

This document records technical concepts, implementation observations, design decisions and development lessons encountered while building DeskBot.

It serves as a reference for understanding the hardware and firmware beyond the high-level system documentation.

## 1. ESP32-C3 as the Central Controller

The ESP32-C3 Super Mini coordinates the peripherals and executes the integrated firmware.

It provides the interfaces required by DeskBot:

- GPIO for physical inputs.
- I2C for the OLED and MPU6050.
- I2S for microphone input and audio output.
- USB connectivity for programming and power.

The controller handles multiple tasks, including sensor acquisition, audio processing, display rendering and operating-mode management.

**Engineering consideration:** Peripheral operations and display updates must be organised carefully to maintain responsive behaviour.

## 2. I2C — Inter-Integrated Circuit

I2C is a synchronous serial communication protocol used by the OLED display and MPU6050.

It uses two primary signal lines:

| Signal | Function |
|---|---|
| SDA | Serial Data |
| SCL | Serial Clock |

Both peripherals share the same SDA and SCL connections.

Each device is selected using its I2C address.

**Important considerations:**

- Devices sharing a bus must have compatible addresses.
- SDA and SCL require suitable pull-up resistors.
- Both devices must share a common ground.
- The bus voltage must be compatible with the connected peripherals.

## 3. I2S — Integrated Interchip Sound

I2S is a digital audio communication protocol used by the INMP441 microphone and MAX98357A amplifier.

DeskBot uses four signal connections:

| Signal | Function |
|---|---|
| BCLK | Bit clock |
| WS | Word Select / Left-Right Clock |
| MIC_SD | Microphone data output |
| SPK_DIN | Amplifier data input |

The microphone and amplifier share the clock signals while maintaining separate data connections.

### Audio Input

`INMP441 → I2S → ESP32-C3`

The microphone supplies digital audio samples to the controller.

### Audio Output

`ESP32-C3 → I2S → MAX98357A → Speaker`

The controller transmits digital audio data to the amplifier, which drives the speaker.

**Engineering consideration:** I2S clock configuration, sample format, channel selection and sampling rate must be compatible between the transmitting and receiving devices.

## 4. Digital Audio Sampling

Digital audio represents a continuous sound waveform through discrete samples.

Two important parameters are:

- **Sampling rate:** Number of samples captured per second.
- **Bit depth:** Number of bits used to represent each sample.

The voice-mimic implementations use a sampling rate of 16 kHz.

At this sampling rate:

**16,000 samples per second**

For a mono recording:

| Duration | Number of Samples |
|---|---:|
| 5 seconds | 80,000 |
| 10 seconds | 160,000 |

### PCM Storage

Pulse Code Modulation (PCM) represents audio using numerical sample values.

The two voice-mimic implementations explore different PCM representations:

| Implementation | Sample Format | Approximate Storage |
|---|---|---:|
| 5-second version | 16-bit mono PCM | 160 KB |
| 10-second version | 8-bit mono PCM | 160 KB |

The 16-bit representation provides more amplitude resolution per sample, whereas the 8-bit representation uses fewer bits per sample.

**Engineering consideration:** Audio buffers must be sized according to the sample rate, bit depth, channel count and recording duration.

## 5. FFT — Fast Fourier Transform

The Fast Fourier Transform is an algorithm used to efficiently calculate the Discrete Fourier Transform (DFT).

It converts a signal from the time domain into its frequency-domain representation.

DeskBot uses FFT processing in its real-time audio spectrum analyser.

### Signal Processing Concepts

**Mean Removal**

The average value of the sample block is removed to reduce the effect of DC offset.

**Hann Window**

A Hann window reduces spectral leakage by gradually reducing the amplitude of samples near the boundaries of the processing block.

**Radix-2 Cooley-Tukey FFT**

The implementation uses a radix-2 Cooley-Tukey algorithm to calculate the frequency-domain representation efficiently.

**Frequency-Band Grouping**

Frequency bins are grouped into 16 visual bands for display on the OLED.

### Processing Flow

`Microphone Samples → Mean Removal → Hann Window → FFT → Frequency Bins → 16 Visual Bands → OLED`

The spectrum display also includes peak-hold indicators.

## 6. OLED Graphics and Rendering

The SSD1306 is a monochrome OLED display with a resolution of 128×64 pixels.

DeskBot uses the display for:

- Animated companion expressions.
- Audio spectrum bars.
- Optical illusion experiments.
- Particle physics simulation.

The display library uses a framebuffer-based rendering approach.

Graphics are prepared in memory and then transferred to the display.

**Engineering consideration:** Frequent full-screen updates and complex rendering can affect the time available for sensor acquisition and audio processing.

## 7. MPU6050 — Motion Sensing

The MPU6050 combines:

- Three-axis accelerometer.
- Three-axis gyroscope.

The accelerometer measures linear acceleration, while the gyroscope measures angular velocity.

DeskBot uses motion measurements for:

- Companion movement reactions.
- Tilt-controlled particle simulation.
- Motion-triggered interactions.

The physics simulator uses orientation-related measurements to influence particle acceleration.

**Engineering consideration:** Sensor readings can contain noise and offsets. Filtering, calibration and appropriate thresholds help prevent unwanted reactions.

## 8. Physics Simulation

The physics simulator represents particles using position and velocity values.

Each particle maintains:

- Position along the X-axis.
- Position along the Y-axis.
- Horizontal velocity.
- Vertical velocity.

The simulator updates particle positions over time and handles collisions.

Implemented features include:

- Boundary collision detection.
- Particle-to-particle collision handling.
- Positional overlap correction.
- Damping.
- MPU6050-based tilt interaction.
- Particle-scattering interaction.

The simulation demonstrates how mathematical models can be translated into real-time graphics on a resource-constrained embedded platform.

## 9. Embedded Memory Considerations

Embedded systems have limited RAM compared with desktop computers.

DeskBot's audio recording implementations demonstrate the importance of memory planning.

For uncompressed mono PCM audio:

**Storage (bytes) = Sample Rate × Duration × (Bit Depth ÷ 8)**

For example, a 5-second recording at 16 kHz and 16 bits per sample requires:

**16,000 × 5 × (16 ÷ 8) = 160,000 bytes**

This is approximately **160 KB** using decimal units.

Memory allocation must account for additional firmware variables, display buffers, I2S buffers and other runtime requirements.

**Engineering consideration:** The available heap should be checked before allocating large audio buffers.

## 10. I2S Audio Playback and Processing

The voice-mimic implementations use a capture-and-playback pipeline.

The general process is:

1. Capture microphone samples.
2. Store the samples in memory.
3. Convert samples into the playback representation.
4. Apply playback gain and sample-rate indexing.
5. Transmit audio through I2S.
6. Amplify the signal using the MAX98357A.

The implementations also explore gain adjustment and playback-rate modification.

Changing the playback sample indexing can alter the apparent playback speed and pitch.

## 11. GPIO and Push Button Handling

DeskBot uses two physical push buttons connected to GPIO7 and GPIO10.

Each button connects its GPIO signal to GND when pressed.

With an internal pull-up configuration:

- Released button: HIGH.
- Pressed button: LOW.

Button handling must account for mechanical contact bounce.

**Debouncing** prevents a single physical press from being interpreted as multiple rapid presses.

The firmware also distinguishes between short presses and longer holds to support different interactions.

## 12. Power Distribution

DeskBot uses 5V and 3.3V power connections.

| Voltage | Main Connections |
|---|---|
| 5V | ESP32-C3 5V rail and MAX98357A |
| 3.3V | INMP441, OLED and MPU6050 |

The working prototype uses a USB power bank connected to the ESP32-C3 through USB-C.

A common ground reference is maintained across the connected peripherals.

**Engineering consideration:** Power-source current capability, voltage drop, connector ratings and ground-return paths must be considered when consolidating the breadboard circuit onto a PCB.

## 13. PCB Design and KiCad

The custom PCB was designed using KiCad.

The project contains three primary editable files:

| File | Purpose |
|---|---|
| `.kicad_pro` | Project configuration |
| `.kicad_sch` | Electrical schematic |
| `.kicad_pcb` | Physical PCB layout |

The board uses two copper layers:

- F.Cu — Front Copper.
- B.Cu — Back Copper.

GND copper zones are assigned on both layers.

### Design Rule Check

The Design Rule Checker identifies violations of configured PCB design rules.

DeskBot's recorded initial DRC results:

- 0 PCB violations.
- 0 unconnected items.
- 1 extra-footprint schematic-parity warning.
- 5 ignored tests.

A successful DRC does not independently verify electrical correctness, power capacity or physical assembly compatibility.

### Gerber Files

Gerber files describe the PCB manufacturing layers.

The fabrication package includes copper, solder-mask, silkscreen, board-outline and drill information.

The editable KiCad project should be preserved separately from the exported manufacturing files.

## 14. Development Lessons

The development of DeskBot highlights several important embedded-systems principles:

1. Test individual peripherals before integrating them into the complete system.
2. Understand communication protocols before debugging connected devices.
3. Plan RAM usage carefully when working with audio buffers.
4. Separate sensor acquisition, processing and display rendering where practical.
5. Use state-based logic to organise interactive behaviour.
6. Verify firmware pin assignments against the electrical schematic.
7. Treat PCB DRC results as one part of hardware verification, not proof of a working circuit.
8. Maintain separate experimental implementations when comparing different approaches.

## 15. Further Technical Investigation

The following topics are useful areas for future optimisation and deeper investigation:

- I2S DMA buffering and audio latency.
- FFT performance and frequency resolution.
- Audio filtering and noise reduction.
- Memory allocation and fragmentation.
- Non-blocking firmware architecture.
- I2C bus reliability.
- Sensor calibration and filtering.
- PCB power integrity and grounding.
- Thermal behaviour of the audio amplifier.

---

*This document is intended to evolve alongside the project. Implementation-specific details should be updated whenever the firmware or hardware design changes.*