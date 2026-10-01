# DeskBot Companion 

An expressive, interactive mini desk robot built around the ESP32-C3, combining animated OLED eyes, motion sensing, touch interaction, audio input/output, and servo-driven head movement.

The goal is to build a small robotic companion that can respond to its environment through movement, expressions, sounds, and user interaction.

## Features

* **Animated expressions:** Display expressive eyes and facial animations on a 0.96-inch SSD1306 OLED.
* **Motion sensing:** Use the MPU6050 accelerometer and gyroscope to detect movement and orientation changes.
* **Touch interaction:** Use a TTP223 capacitive touch sensor to trigger reactions.
* **Audio input:** Capture sound through the INMP441 digital MEMS microphone.
* **Audio output:** Play sounds through a MAX98357A amplifier and speaker.
* **Head movement:** Use two SG90-type servo motors for expressive movement.
* **Sound effects:** Use a buzzer for beeps and simple alerts.
* **Integrated behaviour:** Combine sensor inputs, expressions, sounds, and movement through the ESP32-C3.

*Features will be implemented and tested incrementally.*

## Hardware Components

| Component                      | Purpose                                    |
| ------------------------------ | ------------------------------------------ |
| ESP32-C3 development board     | Main controller                            |
| SSD1306 0.96-inch OLED, 128×64 | Animated eyes and expressions              |
| MPU6050 IMU                    | Acceleration and rotational motion sensing |
| TTP223 touch sensor            | Capacitive touch detection                 |
| INMP441 microphone             | Digital audio input                        |
| MAX98357A amplifier            | I²S audio output amplification             |
| 4 Ω, 3 W speaker               | Sound output                               |
| 2 × SG90-type servos           | Head movement                              |
| Buzzer                         | Beeps and sound effects                    |
| Breadboard and jumper wires    | Prototyping and circuit connections        |
| Perfboard                      | Final circuit assembly                     |

## System Overview

The ESP32-C3 acts as the central controller and coordinates the robot's peripherals.

* **I²C:** Communication with the OLED display and MPU6050.
* **I²S:** Digital audio communication with the INMP441 microphone and MAX98357A amplifier.
* **Digital GPIO:** Touch sensor and buzzer control.
* **PWM-capable GPIO:** Servo position control.

The OLED and MPU6050 can share the I²C bus when configured with distinct addresses. The microphone and audio amplifier use I²S rather than I²C.

Final GPIO assignments and power connections will be documented after hardware testing.

## Repository Structure

```text
mini-robot-companion/
├── README.md
├── documentation/
│   ├── notes.md
│   ├── design.md
│   └── progress.md
├── hardware/
│   ├── components.md
│   └── wiring.md
└── src/
    ├── main/
    │   └── mini_robot.ino
    └── tests/
        ├── display/
        │   └── oled_test.ino
        ├── sensors/
        │   ├── mpu6050_test.ino
        │   └── touch_test.ino
        ├── audio/
        │   ├── microphone_test.ino
        │   └── speaker_test.ino
        └── servos/
            └── servo_test.ino
```

### Directory Guide

* `documentation/` — Technical notes, design decisions, and development progress.
* `hardware/` — Component inventory and wiring information.
* `src/main/` — Integrated firmware for the complete robot.
* `src/tests/` — Independent sketches for testing individual hardware modules.

Test sketches will be added as each subsystem is developed.

## Development Setup

### Requirements

* Arduino IDE or a compatible development environment.
* ESP32 board support package.
* USB data cable compatible with the development board.
* Appropriate libraries for the OLED, MPU6050, and other peripherals used in the firmware.

### Getting Started

1. Clone or download this repository.
2. Install the ESP32 board support package in Arduino IDE.
3. Connect the ESP32-C3 and select the correct board and serial port.
4. Open an individual test sketch from `src/tests/`.
5. Verify each component independently before integrating it into `src/main/mini_robot.ino`.
6. Refer to `hardware/wiring.md` for the verified connections.

Library choices, board configuration, and pin assignments will be documented as the firmware develops.

## Power and Safety Considerations

* Servos should use a suitable regulated supply capable of handling their current demand.
* Do not power servos directly from ESP32 GPIO pins.
* Connect grounds appropriately between the controller, peripherals, and external supplies.
* Verify the operating-voltage requirements of each module before connecting it.
* Check the battery voltage, regulator capacity, and total current requirements before final assembly.
* Test the circuit on a breadboard before transferring it to perfboard.

The final power-distribution and battery arrangements will be documented after testing.

## Development Roadmap

* [ ] Verify ESP32-C3 board configuration and GPIO availability.
* [ ] Test OLED display and basic eye animations.
* [ ] Test MPU6050 motion readings.
* [ ] Test TTP223 touch detection.
* [ ] Capture audio samples from the INMP441 microphone.
* [ ] Test audio output through the MAX98357A and speaker.
* [ ] Test both servos independently.
* [ ] Verify buzzer operation.
* [ ] Finalize wiring and power distribution.
* [ ] Integrate the tested modules into the main firmware.
* [ ] Develop coordinated expressions, movement, and sound responses.
* [ ] Assemble and test the complete robot.

## Documentation

* `documentation/notes.md` — Learning notes and technical concepts.
* `documentation/design.md` — System architecture and design decisions.
* `documentation/progress.md` — Milestones, completed tests, and development updates.
* `hardware/components.md` — Component list and specifications.
* `hardware/wiring.md` — Pin assignments, wiring diagrams, and power connections.

## Project Goal

The objective is to build a compact robotic companion that demonstrates embedded systems, sensor integration, digital audio, motion control, and expressive human–robot interaction.

This project is being developed incrementally, with individual hardware testing followed by firmware integration and final assembly.
