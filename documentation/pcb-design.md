# DeskBot — Custom PCB Design

## 1. Introduction

DeskBot initially used a breadboard-based hardware configuration for developing and testing its individual peripherals.

As the project evolved into an integrated interactive system, a dedicated PCB was designed to consolidate the electrical connections between the ESP32-C3 and its peripheral modules.

The custom PCB provides organised connections for the display, motion sensor, microphone, audio amplifier, power distribution and physical input buttons.

The PCB was designed using **KiCad 10**.

## 2. PCB Design Specifications

| Parameter | Specification |
|---|---|
| Project | DeskBot |
| Design software | KiCad 10 |
| PCB dimensions | 74.55 × 60.55 mm |
| Copper layers | 2 |
| Board thickness | 1.6 mm |
| Substrate | FR4 |
| Copper layers used | Front Copper (F.Cu), Back Copper (B.Cu) |
| Power architecture | 5V and 3.3V rails |
| Grounding | GND copper zones on both layers |

## 3. PCB Architecture

The PCB is designed around the ESP32-C3 Super Mini, which acts as the central controller.

Instead of directly soldering every peripheral module onto the board, dedicated connectors provide interfaces for the individual modules.

### Main Hardware Connections

| Component | PCB Reference | Interface |
|---|---|---|
| ESP32-C3 Super Mini | ESP32_LEFT1 / ESP32_RIGHT1 | 2 × 1×8 headers |
| SSD1306 OLED | J5 | 1×4 header |
| INMP441 microphone | J1 / J2 | 2 × 1×3 headers |
| MAX98357A amplifier | J3 | 1×7 header |
| MPU6050 | MPU-1 | 1×8 header |
| Push Button 1 | SW1 | GPIO7 |
| Push Button 2 | SW2 | GPIO10 |

The two microphone connectors expose the six connections required by the INMP441 breakout.

The ESP32-C3 connects to the PCB through two headers, allowing the controller module to be connected separately from the main PCB.

## 4. Communication Interfaces

The PCB incorporates two primary digital communication interfaces.

### I2C Interface

The I2C bus is shared between the OLED display and MPU6050 motion sensor.

- SDA: Serial data.
- SCL: Serial clock.

Both peripherals connect to the ESP32-C3 through the same I2C lines.

### I2S Interface

The I2S interface handles digital audio input and output.

| Signal | Function |
|---|---|
| I2S_BCLK | Shared bit clock |
| I2S_WS | Shared word-select clock |
| I2S_MIC_SD | Microphone data input |
| I2S_SPK_DIN | Amplifier data input |

The microphone and amplifier share the clock signals while maintaining separate data connections.

## 5. Power Distribution and Grounding

The PCB provides separate 5V and 3.3V power connections for the connected modules.

| Power Rail | Intended Connections |
|---|---|
| 5V | ESP32-C3 5V rail and MAX98357A |
| 3.3V | INMP441, OLED and MPU6050 |
| GND | Common ground |

The working breadboard prototype is powered using a USB power bank connected to the ESP32-C3 through USB-C.

### Ground Plane

GND copper zones are assigned to both copper layers:

- Front Copper (F.Cu)
- Back Copper (B.Cu)

These zones provide a common ground plane and help reduce the need for individual ground-routing tracks.

Copper zones are refilled in KiCad to update their connections after PCB layout modifications.

## 6. PCB Layout and Routing

The PCB uses a two-layer routing arrangement.

- Front Copper (F.Cu)
- Back Copper (B.Cu)

Using both copper layers provides additional routing flexibility and allows connections to cross between layers using vias.

The component placement groups the peripheral connectors around the central controller connections.

The board also incorporates a KiCad logo as part of its visual design.

### Layout Considerations

The following aspects are considered during PCB development:

- Accessibility of peripheral connectors.
- Separation and organisation of power and signal connections.
- Routing flexibility across both copper layers.
- Common grounding through copper zones.
- Physical compatibility with the peripheral breakout modules.
- Connector orientation and accessibility during assembly.

## 7. Design Rule Check (DRC)

The PCB was subjected to a Design Rule Check using KiCad.

### Recorded DRC Results

| Check | Result |
|---|---:|
| PCB violations | 0 |
| Unconnected items | 0 |
| Schematic-parity warnings | 1 |
| Ignored tests | 5 |

The remaining schematic-parity warning concerns an extra footprint with the reference `REF**`, associated with the PCB logo.

The five ignored tests are excluded checks and should not automatically be interpreted as errors.

### DRC Interpretation

A zero-violation result indicates that the board passed the enabled PCB design-rule checks.

Zero unconnected items indicates that all required PCB connections are routed according to the current netlist.

However, DRC does not independently verify:

- Electrical correctness of the circuit.
- Actual current-carrying capacity of power tracks.
- Physical compatibility of breakout modules.
- Connector orientation during assembly.
- Thermal performance.
- Real-world hardware operation.

These aspects require separate design reviews and physical validation.

## 8. Manufacturing Files

The PCB design includes Gerber and drill files generated for manufacturing.

The exported fabrication package contains:

| File Type | Purpose |
|---|---|
| F.Cu | Front copper layer |
| B.Cu | Back copper layer |
| F.Mask | Front solder mask |
| B.Mask | Back solder mask |
| F.Silkscreen | Front component markings |
| B.Silkscreen | Back component markings |
| Edge.Cuts | PCB board outline |
| PTH Drill | Plated through-hole drilling data |
| NPTH Drill | Non-plated through-hole drilling data |
| Gerber Job | Manufacturing layer information |

The Gerber package represents the PCB layout in a format suitable for fabrication.

## 9. KiCad Project Files

The editable PCB design is maintained separately from the exported manufacturing files.

The main KiCad project contains:

```text
pilot/
│
├── pilot.kicad_pro
├── pilot.kicad_sch
└── pilot.kicad_pcb
```

| File | Description |
|---|---|
| `pilot.kicad_pro` | KiCad project configuration |
| `pilot.kicad_sch` | Circuit schematic |
| `pilot.kicad_pcb` | PCB layout and routing |

These files preserve the editable design and allow further modifications to the schematic and PCB.

## 10. Current PCB Status

The custom PCB schematic and layout have been developed.

Initial DRC checks report zero PCB violations and zero unconnected items, with one extra-footprint warning associated with the PCB logo.

The manufacturing files have also been exported.

**Current stage:** PCB design and initial verification completed.

**Pending stages:**
- Final design review.
- PCB fabrication.
- Physical component assembly.
- Power distribution testing.
- Peripheral connectivity testing.
- Complete system validation on the manufactured PCB.

The working breadboard prototype and the custom PCB are separate development stages. Successful operation of the breadboard prototype does not, by itself, establish successful operation of the manufactured PCB.

---

**Related documentation:**

- [`components.md`](../hardware/components.md) — Hardware inventory.
- [`pinout.md`](../hardware/pinout.md) — ESP32-C3 pin assignments.
- [`wiring.md`](../hardware/wiring.md) — Electrical interconnections.
- [`system-architecture.md`](system-architecture.md) — Firmware and system architecture.