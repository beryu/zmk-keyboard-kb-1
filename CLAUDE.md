# CLAUDE.md

## Project overview

This repository builds ZMK firmware for the kb-1 split keyboard. The hardware source of truth is the KiCad project at https://github.com/beryu/kb-1.

## Hardware

- Controller: Seeed Studio XIAO nRF52840 Plus (`xiao_ble`)
- Matrix: 5 rows x 7 columns per half, with 49 populated keys total
- Pointing device: PAW3222 over three-wire SPI
- Power: one AAA cell per half with an external 5 V boost circuit
- Battery sensing: external divider connected to D0/P0.02/AIN0

Do not reuse BMP Boost GPIO assignments. Derive any hardware changes from the kb-1 PCB and generated schematics.

## Build variants

- `kb_1_left_central` with PAW3222
- `kb_1_right_peripheral`
- `kb_1_right_central` with PAW3222
- `kb_1_left_peripheral`
- `settings_reset`

The trackball side is the ZMK split central.

## Validation

- Run `git diff --check`.
- Build every entry in `build.yaml` through the reusable ZMK GitHub Actions workflow.
- Review generated Kconfig and devicetree artifacts when changing GPIO or power behavior.
- Hardware validation is required for matrix positions, split pairing, PAW3222 motion and battery reporting.
