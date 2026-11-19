# DeskBot — PCB Resources

This directory contains the editable KiCad project and manufacturing exports for the custom DeskBot PCB.

## Directory Structure

```text
pcb/
│
├── README.md
│
├── kicad/
│   ├── pilot.kicad_pro
│   ├── pilot.kicad_sch
│   └── pilot.kicad_pcb
│
└── gerbers/
    └── Manufacturing files
```

## 1. KiCad Project

The `kicad/` directory contains the editable PCB design files.

| File | Description |
|---|---|
| `pilot.kicad_pro` | KiCad project configuration |
| `pilot.kicad_sch` | Electrical schematic |
| `pilot.kicad_pcb` | PCB layout, footprints and routing |

These files can be opened using KiCad for further schematic modifications and PCB layout development.

## 2. Gerber and Drill Files

The `gerbers/` directory contains the exported manufacturing files.

The package includes:

- Front and back copper layers.
- Front and back solder-mask layers.
- Front and back silkscreen layers.
- Board outline.
- Plated through-hole drill data.
- Non-plated through-hole drill data.
- Gerber job information.

These files describe the PCB layers and drilling information required by a PCB manufacturer.

## 3. PCB Specifications

| Parameter | Specification |
|---|---|
| Design software | KiCad 10 |
| Board dimensions | 74.55 × 60.55 mm |
| Copper layers | 2 |
| Board thickness | 1.6 mm |
| Substrate | FR4 |

## 4. Design Status

The schematic and PCB layout have been developed, and manufacturing exports have been generated.

Initial KiCad DRC results:

- 0 PCB violations.
- 0 unconnected items.
- 1 extra-footprint schematic-parity warning.

PCB fabrication, physical assembly and hardware validation are pending.

For the complete design explanation, refer to [`../../documentation/pcb-design.md`](../../documentation/pcb-design.md).