# DeskBot — Hardware Pinout

This document describes the ESP32-C3 pin assignments and peripheral connections used in the DeskBot custom PCB.

The pin mapping is based on the KiCad hardware schematic.

## 1. ESP32-C3 Super Mini

The ESP32-C3 module connects to the custom PCB through two 1×8 headers.

### ESP32_LEFT1

| Header Pin | Signal | Connected To |
|---|---|---|
| 1 | I2S_WS | INMP441 and MAX98357A |
| 2 | I2S_SPK_DIN | MAX98357A data input |
| 3 | GPIO7 | Push Button SW1 |
| 4 | GPIO8 | Not connected |
| 5 | GPIO9 | Not connected |
| 6 | GPIO10 | Push Button SW2 |
| 7 | GPIO20 | Exposed on header |
| 8 | GPIO21 | Not connected |

### ESP32_RIGHT1

| Header Pin | Signal | Connected To |
|---|---|---|
| 1 | 5V | 5V power rail |
| 2 | GND | Ground |
| 3 | 3V3 | 3.3V power rail |
| 4 | I2S_BCLK | INMP441 and MAX98357A |
| 5 | I2S_MIC_SD | INMP441 data output |
| 6 | GPIO2 | Not connected |
| 7 | SCL | OLED and MPU6050 |
| 8 | SDA | OLED and MPU6050 |

## 2. INMP441 — I2S Microphone

The INMP441 microphone is connected through two separate 1×3 headers.

### J1 — INMP441_LEFT

| Pin | Signal |
|---|---|
| 1 | L/R |
| 2 | I2S_WS |
| 3 | I2S_BCLK |

### J2 — INMP441_RIGHT

| Pin | Signal |
|---|---|
| 1 | GND |
| 2 | 3V3 |
| 3 | I2S_MIC_SD |

The L/R pin is connected to GND, selecting the left-channel output of the microphone.

The microphone communicates with the ESP32-C3 using the I2S protocol.

## 3. MAX98357A — Audio Amplifier

The MAX98357A receives digital audio from the ESP32-C3 and drives the connected speaker.

### J3 — MAX98357A

| Pin | Signal |
|---|---|
| 1 | 5V |
| 2 | GND |
| 3 | SD |
| 4 | GAIN |
| 5 | I2S_SPK_DIN |
| 6 | I2S_BCLK |
| 7 | I2S_WS |

The amplifier receives its audio data through I2S.

The SD and GAIN pins are not connected in the current schematic.

## 4. OLED Display — SSD1306

The OLED display communicates with the ESP32-C3 through the I2C interface.

### J5 — OLED

| Pin | Signal |
|---|---|
| 1 | 3V3 |
| 2 | GND |
| 3 | SCL |
| 4 | SDA |

The OLED shares the I2C bus with the MPU6050 sensor.

## 5. MPU6050 — Accelerometer and Gyroscope

The MPU6050 provides motion and orientation data through I2C communication.

### MPU-1 — MPU6050

| Pin | Signal |
|---|---|
| 1 | 3V3 |
| 2 | GND |
| 3 | SCL |
| 4 | SDA |
| 5 | XDA |
| 6 | XCL |
| 7 | AD0 |
| 8 | INT |

XDA, XCL, AD0 and INT are not connected in the current schematic.

## 6. Push Buttons

Two push buttons are incorporated into the PCB.

| Reference | ESP32-C3 Pin | Connection |
|---|---|---|
| SW1 | GPIO7 | GPIO7 to GND |
| SW2 | GPIO10 | GPIO10 to GND |

The buttons connect their respective GPIO signals to ground when pressed.

## 7. Communication Interfaces

DeskBot uses two primary communication interfaces.

### I2S — Digital Audio

The I2S interface is shared between the microphone and audio amplifier.

| Signal | Direction / Connection |
|---|---|
| I2S_BCLK | Shared clock for microphone and amplifier |
| I2S_WS | Shared word-select clock |
| I2S_MIC_SD | Microphone → ESP32-C3 |
| I2S_SPK_DIN | ESP32-C3 → MAX98357A |

### I2C — Peripheral Communication

The I2C bus connects the OLED display and MPU6050 sensor.

| Signal | Connected Devices |
|---|---|
| SCL | OLED and MPU6050 |
| SDA | OLED and MPU6050 |

Both peripherals share the same I2C communication lines.

## 8. Power Connections

| Supply | Connected Components |
|---|---|
| 5V | ESP32-C3 5V rail and MAX98357A |
| 3V3 | INMP441, OLED and MPU6050 |
| GND | Common ground across the circuit |

The prototype is powered using a USB-C connection to the ESP32-C3, supplied by a USB power bank.

---

**Note:** This document describes the PCB schematic connections. Verify the firmware GPIO configuration and physical connector orientation before assembling or modifying the hardware.