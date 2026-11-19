# DeskBot — Design Philosophy and Engineering Decisions

## 1. Project Concept

DeskBot is an interactive desktop companion developed around the ESP32-C3 Super Mini.

The project explores how embedded systems can combine sensing, audio processing, visual feedback and user interaction within a compact hardware platform.

Rather than functioning as a conventional robotic vehicle or a simple display-based gadget, DeskBot is designed as an interactive system capable of responding to sound, movement and user input.

Its behaviour is expressed through animated OLED graphics, audio responses, visual experiments and sensor-driven simulations.

## 2. Design Objectives

The primary objectives of DeskBot are:

- Develop an interactive embedded system using the ESP32-C3.
- Integrate multiple digital peripherals through I2C and I2S communication.
- Implement real-time audio acquisition and processing.
- Develop expressive OLED graphics and animations.
- Incorporate motion sensing into interactive behaviour.
- Explore physics simulation and optical illusions on a monochrome display.
- Implement audio recording and playback using digital audio hardware.
- Consolidate the hardware connections through a custom PCB.
- Develop a modular firmware structure that supports independent experimentation and integration.

## 3. Core Design Philosophy

DeskBot follows an incremental and modular development approach.

Individual hardware components and software features are developed and tested independently before being integrated into the complete system.

The design is guided by four main principles:

### 3.1 Modularity

Different functionalities are organised into separate operating modes and functional blocks.

This allows individual features, such as audio visualisation, motion interaction and OLED animations, to be developed and tested without redesigning the entire system.

### 3.2 Hardware–Software Integration

The project combines multiple hardware interfaces with application-level firmware.

The ESP32-C3 coordinates the connected peripherals and processes their data to generate visual, audio and interactive responses.

### 3.3 Interactive Behaviour

DeskBot is designed to respond to its environment rather than simply execute a fixed sequence of operations.

Microphone input, motion measurements and physical button presses influence its behaviour.

### 3.4 Experimental Development

The project incorporates experiments beyond conventional embedded-device functionality.

The optical illusion engine, custom FFT implementation and particle physics simulator provide opportunities to explore signal processing, mathematical computation and real-time graphics on resource-constrained hardware.

## 4. Controller Selection — ESP32-C3

The ESP32-C3 Super Mini was selected as the central controller.

Its capabilities make it suitable for coordinating the different subsystems of DeskBot.

Important considerations include:

- Integrated I2C and I2S peripheral support.
- Digital GPIO for physical inputs.
- Sufficient processing capability for the project's graphics and signal-processing tasks.
- Compact development-board form factor.
- USB connectivity for programming and power.

The ESP32-C3 provides a common platform for integrating the display, motion sensor, microphone and audio amplifier.

## 5. Peripheral Selection

### 5.1 SSD1306 OLED Display

The 0.96-inch SSD1306 monochrome OLED was selected as the primary visual output device.

Its 128×64 resolution supports:

- Animated companion expressions.
- Real-time audio spectrum visualisation.
- Optical illusion experiments.
- Particle-based physics simulation.

The display communicates with the ESP32-C3 through I2C.

### 5.2 MPU6050 Motion Sensor

The MPU6050 combines a three-axis accelerometer and three-axis gyroscope.

It provides motion and orientation measurements for:

- Motion-triggered companion reactions.
- Tilt-controlled physics simulation.
- Movement-based interaction.

It communicates with the ESP32-C3 through I2C.

### 5.3 INMP441 Digital Microphone

The INMP441 was selected for digital audio acquisition.

It provides microphone data through the I2S interface, allowing the ESP32-C3 to process incoming audio.

Its applications include:

- Sound detection.
- Clap-based interaction.
- Audio spectrum analysis.
- Voice recording.

### 5.4 MAX98357A Audio Amplifier

The MAX98357A provides amplified audio output from digital I2S data.

It interfaces with the ESP32-C3 and drives the connected speaker.

This enables audio playback and voice-mimic functionality without requiring an external analogue audio amplifier stage.

### 5.5 Physical Push Buttons

Two push buttons provide direct user input.

They allow interaction with the operating modes and trigger specific firmware actions.

The buttons are connected to GPIO7 and GPIO10.

## 6. Communication Interface Decisions

DeskBot uses I2C and I2S for its primary peripheral communication.

### I2C

I2C is used for communication with the OLED display and MPU6050.

The shared bus reduces the number of dedicated signal connections required.

### I2S

I2S is used for digital audio input and output.

The INMP441 microphone and MAX98357A amplifier share the clock signals while using separate data connections.

This arrangement supports simultaneous integration of audio input and output hardware within the same system.

## 7. Software Design Decisions

### 7.1 Multiple Operating Modes

Instead of creating separate firmware projects for every feature, the integrated prototype combines several functionalities into a single application.

The four primary operating modes are:

1. Interactive OLED Companion.
2. Real-Time Audio Spectrum Analyzer.
3. Optical Illusion Engine.
4. Tilt-Controlled Physics Simulator.

This approach allows multiple experiments to coexist on one embedded platform.

### 7.2 Custom FFT Implementation

The audio spectrum analyser uses a custom radix-2 Cooley-Tukey FFT implementation.

The processing pipeline incorporates mean removal, Hann windowing, frequency-domain transformation and frequency-band grouping.

This provides an opportunity to explore the mathematical and computational aspects of digital signal processing rather than relying entirely on an external FFT library.

### 7.3 State-Based Interaction

The companion uses a `FaceState` enumeration to represent different expressions and behavioural conditions.

This separates the companion's current state from the logic responsible for detecting sound, movement and inactivity.

### 7.4 Independent Experiments

Separate source files are maintained for experimental implementations.

The voice-mimic project, for example, includes independent 5-second and 10-second implementations.

This makes it easier to compare approaches without replacing the integrated prototype firmware.

## 8. Custom PCB Design

The initial hardware development used a breadboard-based configuration.

As the system became more complex, a custom two-layer PCB was designed using KiCad.

The PCB consolidates the connections for:

- ESP32-C3 Super Mini.
- SSD1306 OLED.
- INMP441 microphone.
- MAX98357A amplifier.
- MPU6050 sensor.
- Two push buttons.

The design uses dedicated headers for peripheral connections and GND copper zones on both copper layers.

The objective is to provide a more organised and reproducible hardware arrangement than the initial breadboard setup.

## 9. Engineering Challenges and Considerations

Several engineering considerations arise from integrating multiple subsystems into a compact embedded platform.

### Memory Constraints

Audio recording requires substantial memory.

The two voice-mimic implementations explore different sample representations and recording durations to manage storage requirements.

### Real-Time Processing

Audio acquisition, FFT computation, sensor reading, particle simulation and OLED rendering compete for processing time.

Firmware execution must therefore be organised to maintain responsive interaction.

### Shared Communication Resources

The OLED and MPU6050 share the I2C bus, while the microphone and amplifier share I2S clock signals.

Their configurations must remain compatible with the integrated firmware.

### Power Distribution

The system uses both 5V and 3.3V power connections.

The amplifier's supply requirements and the common ground arrangement must be considered when designing the PCB and selecting the power source.

### Hardware Consolidation

Moving from a breadboard prototype to a custom PCB introduces additional considerations, including connector orientation, track dimensions, power distribution, manufacturing tolerances and physical assembly.

## 10. Project Scope and Future Development

The current project focuses on an interactive embedded desktop companion with integrated audio, motion sensing, visualisation and simulation capabilities.

The custom PCB is intended to consolidate the hardware connections into a dedicated board.

Further development will focus on:

- Fabrication and assembly of the custom PCB.
- Physical validation of the assembled board.
- Further optimisation of firmware execution.
- Improvements to audio processing and interaction.
- Additional visual experiments and companion behaviours.
- Improved enclosure and physical integration.

## 11. Conclusion

DeskBot demonstrates the integration of embedded systems, digital signal processing, sensor interfaces, real-time graphics and interactive firmware.

The project progresses beyond individual peripheral demonstrations by combining multiple independently developed subsystems into a unified interactive platform.

Its development provides practical experience in hardware interfacing, firmware architecture, signal processing, PCB design and system-level integration.