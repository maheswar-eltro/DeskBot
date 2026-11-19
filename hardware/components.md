# DeskBot — Hardware Components

## 1. Main Controller

| Component | Description |
|---|---|
| ESP32-C3 Super Mini | Main microcontroller responsible for firmware execution, sensor processing, display control and audio handling |

## 2. Display

| Component | Description |
|---|---|
| SSD1306 OLED Display | Monochrome OLED used for facial animations, audio visualisation, optical illusions and physics simulation |

## 3. Sensors and Audio Input

| Component | Description |
|---|---|
| MPU6050 | 6-axis accelerometer and gyroscope for motion detection and tilt-based interaction |
| INMP441 | Digital MEMS microphone for sound detection, audio processing and voice recording |

## 4. Audio Output

| Component | Description |
|---|---|
| MAX98357A | I²S digital audio amplifier |
| Speaker | Audio output for sound effects and voice playback |

## 5. User Interaction

| Component | Description |
|---|---|
| Push Buttons | Physical user-input controls |

## 6. Custom PCB

A custom carrier PCB designed in KiCad to integrate the DeskBot electronic modules.

The PCB design accommodates the ESP32-C3 Super Mini, OLED display, MPU6050, INMP441 microphone, MAX98357A amplifier, speaker connections and associated controls.

Refer to the `hardware/pcb/` directory for the editable KiCad project and manufacturing outputs.

## 7. Excluded from Current Design

The following components were considered in earlier design iterations but are not part of the current DeskBot configuration:

- TTP223 capacitive touch sensor
- Two SG90 servo motors