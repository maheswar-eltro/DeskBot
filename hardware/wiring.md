# DeskBot — Hardware Wiring

This document describes the electrical interconnections between the ESP32-C3, audio peripherals, display, motion sensor and input buttons used in DeskBot.

The wiring architecture is based on the custom KiCad PCB schematic.

## 1. System Overview

The ESP32-C3 Super Mini acts as the central controller of DeskBot.

It interfaces with the following peripherals:

- SSD1306 OLED display
- MPU6050 accelerometer and gyroscope
- INMP441 digital microphone
- MAX98357A I2S audio amplifier
- Two push buttons

The system uses I2C for display and motion-sensor communication, and I2S for digital audio input and output.

## 2. Power Distribution

DeskBot uses a USB 5V power source.

In the working breadboard prototype, a USB-C cable connects a power bank to the ESP32-C3.

The PCB schematic distributes the power rails as follows:

| Power Rail | Connected Components |
|---|---|
| 5V | ESP32-C3 5V rail and MAX98357A |
| 3.3V | INMP441, SSD1306 OLED and MPU6050 |
| GND | Common ground across the circuit |

All peripherals share a common ground reference.

The MAX98357A receives its 5V supply separately from the 3.3V peripherals.

## 3. Audio Input — INMP441 Microphone

The INMP441 is a digital MEMS microphone that communicates with the ESP32-C3 through the I2S interface.

Its connections are:

| INMP441 Signal | ESP32-C3 Signal |
|---|---|
| VDD | 3V3 |
| GND | GND |
| SCK / BCLK | I2S_BCLK |
| WS | I2S_WS |
| SD | I2S_MIC_SD |
| L/R | GND |

The microphone's L/R pin is connected to GND to select its left-channel output.

The microphone receives the shared I2S clock signals and sends digital audio data to the ESP32-C3 through I2S_MIC_SD.

## 4. Audio Output — MAX98357A Amplifier

The MAX98357A converts digital I2S audio data into an amplified speaker output.

Its connections are:

| MAX98357A Signal | ESP32-C3 / Supply |
|---|---|
| VIN | 5V |
| GND | GND |
| BCLK | I2S_BCLK |
| LRC / WS | I2S_WS |
| DIN | I2S_SPK_DIN |

The amplifier shares the I2S clock signals with the microphone but uses a separate data line.

A 4Ω speaker is used with the amplifier in the working prototype.

The amplifier's SD and GAIN pins are not connected in the current PCB schematic.

## 5. I2C Bus — OLED and MPU6050

The SSD1306 OLED and MPU6050 share the same I2C communication bus.

Both peripherals connect to the ESP32-C3 through the following signals:

| Signal | Connection |
|---|---|
| SDA | ESP32-C3 SDA |
| SCL | ESP32-C3 SCL |
| VCC | 3V3 |
| GND | GND |

The OLED receives display data from the ESP32-C3, while the MPU6050 provides motion and orientation measurements.

Both devices communicate over the same two I2C signal lines.

## 6. Push Button Connections

Two push buttons are incorporated into the PCB.

| Button | ESP32-C3 GPIO | Other Connection |
|---|---|---|
| SW1 | GPIO7 | GND |
| SW2 | GPIO10 | GND |

When pressed, each button connects its corresponding GPIO signal to ground.

The firmware should configure the corresponding GPIO pins with suitable input and pull-up settings.

## 7. Communication Architecture

DeskBot uses two primary digital communication interfaces.

### I2S — Audio Communication

The I2S bus connects the microphone and amplifier to the ESP32-C3.

| Signal | Function |
|---|---|
| I2S_BCLK | Bit clock shared by microphone and amplifier |
| I2S_WS | Word-select clock shared by microphone and amplifier |
| I2S_MIC_SD | Digital audio input from microphone |
| I2S_SPK_DIN | Digital audio output to amplifier |

The microphone and amplifier share clock signals while maintaining independent audio-data connections.

### I2C — Peripheral Communication

The I2C bus connects the OLED display and MPU6050.

| Signal | Function |
|---|---|
| SDA | Serial data |
| SCL | Serial clock |

Both peripherals share the same I2C bus.

## 8. Hardware Interconnection Summary

| Peripheral | Interface | Power Supply |
|---|---|---|
| INMP441 | I2S | 3.3V |
| MAX98357A | I2S | 5V |
| SSD1306 OLED | I2C | 3.3V |
| MPU6050 | I2C | 3.3V |
| Push Buttons | GPIO | GPIO pull-up configuration |

## 9. Wiring and Assembly Notes

- Maintain a common ground between the ESP32-C3 and all connected peripherals.
- Verify connector pin orientation before connecting breakout modules.
- Keep audio signal connections secure and avoid loose ground connections.
- Ensure that the amplifier receives the correct 5V supply.
- Avoid applying 5V directly to 3.3V-only peripheral power inputs.
- Verify the firmware pin assignments against the PCB schematic before assembly.

---

**Reference:** DeskBot KiCad schematic (`pilot.kicad_sch`). For individual ESP32-C3 pin assignments, refer to [`pinout.md`](pinout.md).