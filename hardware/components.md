# Hardware Components — DeskBot

This document lists the hardware components used in the DeskBot project, their functions, communication interfaces, and relevant integration notes.

## 1. Main Controller

### ESP32-C3 Development Board

**Purpose:** Main controller of the robot.

* Executes the robot's firmware and coordinates its peripherals.
* Reads sensor data and processes touch and motion inputs.
* Controls OLED expressions, servo movement, and buzzer output.
* Communicates with digital audio peripherals.

**Interfaces used:**

* I²C for the OLED and MPU6050.
* I²S for the INMP441 microphone and MAX98357A audio amplifier.
* Digital GPIO for touch detection and buzzer control.
* PWM-capable GPIO for servo control.

**Important notes:**

* Confirm the exact board pinout before assigning GPIOs.
* Check the board's supported logic voltage and each module's supply requirements.
* Do not power servos directly from GPIO pins.

## 2. Display

### SSD1306 OLED Display — 0.96-inch, 128×64

**Purpose:** Displays the robot's eyes, expressions, and simple visual animations.

**Interface:** I²C.

**Main connections:**

* VCC — supply voltage supported by the module.
* GND — ground.
* SDA — I²C data.
* SCL — I²C clock.

**Important notes:**

* The SSD1306 is the display controller; the OLED panel produces the visible light.
* The display is monochrome, with 128 × 64 = 8,192 pixels.
* Common I²C addresses are `0x3C` and `0x3D`; verify the actual module.
* Confirm the module's supported supply voltage before connecting it.

## 3. Motion Sensor

### MPU6050 — 6-Axis IMU

**Purpose:** Measures acceleration and rotational motion to help the robot detect movement and estimate orientation.

**Sensors:**

* 3-axis accelerometer.
* 3-axis gyroscope.

**Interface:** I²C.

**Main connections:**

* VCC — supported supply voltage.
* GND — ground.
* SDA — I²C data.
* SCL — I²C clock.

**Important notes:**

* The accelerometer measures acceleration, including the effect of gravity.
* The gyroscope measures angular velocity.
* Combining both measurements can produce a more useful orientation estimate than relying on either sensor alone.
* The usual 7-bit I²C address is `0x68` or `0x69`, depending on the AD0 pin.
* Sensor readings can contain bias and noise; calibration and filtering may be required.

## 4. Touch Sensor

### TTP223 Capacitive Touch Module

**Purpose:** Detects touch to trigger interactions such as changing expressions or playing a sound.

**Interface:** Digital GPIO.

**Main connections:**

* VCC — supported supply voltage.
* GND — ground.
* OUT — digital touch-detection signal.

**Important notes:**

* Detects changes in capacitance caused by a nearby finger.
* It is a touch detector, not a pressure sensor.
* Output polarity and behaviour can depend on the module configuration.
* Verify the output level is safe for the ESP32-C3 before connecting it.

## 5. Audio Input

### INMP441 Digital MEMS Microphone

**Purpose:** Captures environmental sound for audio-reactive behaviour and future sound-processing features.

**Interface:** I²S digital audio.

**Main connections:**

* VDD — microphone supply.
* GND — ground.
* SCK — I²S serial clock.
* WS — word-select or left/right clock.
* SD — serial audio data.
* L/R — selects the left or right channel.

**Important notes:**

* The INMP441 produces digital audio samples; it does not perform speech recognition by itself.
* It uses I²S, not I²C.
* The ESP32-C3 must be configured to receive I²S audio data.
* Check the microphone module's supply and logic requirements before wiring.

## 6. Audio Output

### MAX98357A I²S Class-D Amplifier

**Purpose:** Converts digital audio received from the ESP32-C3 into an amplified signal for the speaker.

**Interface:** I²S digital audio input.

**Typical connections:**

* VIN — amplifier supply.
* GND — ground.
* BCLK — I²S bit clock.
* LRC — I²S word-select clock.
* DIN — digital audio data input.
* Speaker terminals — connect to the speaker as specified by the module documentation.

**Important notes:**

* The MAX98357A is an amplifier, not an audio-file player or speech synthesizer.
* The ESP32-C3 supplies the digital audio stream.
* Class-D speaker outputs may be bridge-driven; do not connect a speaker terminal to ground unless the module documentation explicitly permits it.
* Confirm the supply voltage and speaker compatibility.

### Speaker — 4 Ω, 3 W

**Purpose:** Produces audible sounds, alerts, and audio responses.

**Connection:** Speaker output terminals on the MAX98357A amplifier.

**Important notes:**

* The amplifier and speaker must be electrically compatible.
* The speaker's rated power does not mean the amplifier will always deliver 3 W.
* Avoid excessive volume and prolonged distortion.

## 7. Head Movement

### 2 × SG90-Type Servo Motors

**Purpose:** Provide expressive movement, such as turning or tilting the robot's head.

**Interface:** PWM-style servo control signals.

**Typical connections:**

* Power — suitable regulated servo supply.
* GND — servo supply ground.
* Signal — assigned ESP32-C3 GPIO.

**Important notes:**

* Confirm the required voltage and stall current for the actual servo models.
* Do not power the servos directly from ESP32 GPIO pins.
* The servo supply must handle startup and movement current.
* Connect the servo supply ground to the controller ground when required by the signal interface.
* Consider a bulk capacitor across the servo supply rails if testing reveals voltage dips or resets.
* Final power and GPIO assignments will be documented after testing.

## 8. Sound Effects

### Buzzer

**Purpose:** Produces simple beeps and alerts for events such as touch detection or interaction feedback.

**Interface:** Digital GPIO; tone generation depends on the buzzer type.

**Important notes:**

* An active buzzer generally sounds when supplied with its required DC voltage.
* A passive buzzer generally needs a switching or tone signal.
* Identify the buzzer type and verify its current requirements before connecting it directly to a GPIO.

## 9. Prototyping and Assembly

### Breadboard and Jumper Wires

**Purpose:** Allow circuit connections and testing before permanent assembly.

* Used during individual component tests.
* Make it easier to change wiring during development.
* Check connections and power rails before applying power.

### Perfboard

**Purpose:** Supports the final soldered circuit after the prototype has been tested.

* Transfer only verified connections.
* Keep power and ground wiring appropriate for the expected current.
* Avoid accidental shorts between adjacent solder joints.

### ESD-Shielding Bags

**Purpose:** Help protect sensitive electronic components from electrostatic discharge during storage.

* Store loose modules in suitable ESD-safe packaging.
* Handle circuit boards carefully, especially around exposed pins.

## 10. Interface Summary

| Component            | Interface          | Primary function                           |
| -------------------- | ------------------ | ------------------------------------------ |
| ESP32-C3             | Controller         | Runs firmware                              |
| SSD1306 OLED         | I²C                | Displays expressions                       |
| MPU6050              | I²C                | Measures acceleration and angular velocity |
| TTP223               | Digital GPIO       | Detects touch                              |
| INMP441              | I²S input          | Captures digital audio                     |
| MAX98357A            | I²S input          | Amplifies digital audio                    |
| 4 Ω, 3 W speaker     | Amplifier output   | Produces sound                             |
| 2 × SG90-type servos | PWM-style signals  | Moves the head                             |
| Buzzer               | GPIO / tone signal | Produces alerts                            |

## 11. Power and Wiring Checklist

* [ ] Confirm the exact ESP32-C3 board and available GPIO pins.
* [ ] Verify the supply-voltage requirements of every module.
* [ ] Verify I²C addresses for the OLED and MPU6050.
* [ ] Confirm I²S pin assignments and channel configuration for the microphone.
* [ ] Confirm I²S connections and speaker wiring for the amplifier.
* [ ] Select a suitable servo supply based on measured or documented current requirements.
* [ ] Establish the required common-ground connections.
* [ ] Check polarity and continuity before powering the circuit.
* [ ] Document final wiring in `hardware/wiring.md`.

*This document describes the planned hardware configuration. Actual pin assignments, power connections, and tested behaviour should be updated as the robot is assembled.*
