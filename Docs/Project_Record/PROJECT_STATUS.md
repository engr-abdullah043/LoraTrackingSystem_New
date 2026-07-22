# LoRa Tracking System - Project Status

Last updated: 2026-07-22

## Purpose

This file is the current-state handoff for developers and AI agents. Keep it concise and update it whenever the firmware architecture, hardware mapping, verified behavior, or immediate next steps change. Record individual changes in `CHANGELOG.md`.

## Current Phase

STM32CubeMX LoRa interface configuration is statically verified. No LoRa driver or application behavior has been implemented yet.

## Hardware

- MCU: STM32U585CIU6, UFQFPN48
- LoRa module: Seeed Studio Wio-SX1262, 862-930 MHz, IPEX antenna connection
- Module supply: 3.3 V typical; datasheet operating range is 1.8-3.6 V
- CubeMX project: `Firmware/Main_Controller/Main_Controller.ioc`
- Module references: `Docs/Wio-SX1262_Module_Datasheet.pdf` and `Docs/Wio-SX1262-pin_map.png`

## LoRa Interface Mapping

| Module signal | STM32 pin | STM32 configuration | Signal direction relative to STM32 | Status |
|---|---:|---|---|---|
| RF_SW | PA0 | GPIO output, push-pull, initial low | Output | Direction verified |
| NSS | PA1 | GPIO output, push-pull, initial high | Output | Verified |
| BUSY | PA2 | GPIO input, no pull | Input | Verified |
| NRST | PA3 | GPIO output, push-pull, initial high | Output | Verified |
| DIO1 | PA4 | Rising-edge EXTI4 input, no pull | Input | Verified |
| SCK | PA5 | SPI1_SCK, AF5, high speed | Output | Verified |
| MISO | PA6 | SPI1_MISO, AF5, high speed | Input | Verified |
| MOSI | PA7 | SPI1_MOSI, AF5, high speed | Output | Verified |

Power and ground must be wired to the module's 3V3 and GND connections. The SMD radio module VCC has a 3.9 V absolute maximum; any 5 V header on an adapter board must not be confused with the module's VCC.

## Verified CubeMX Configuration

- SPI1 uses 8-bit frames, mode 0, MSB first, full-duplex master, software NSS, and CRC disabled.
- SPI1 uses a prescaler of 16, producing 10 Mbit/s from the 160 MHz SPI1 kernel clock.
- PA5-PA7 use high GPIO speed and no internal pull.
- PA4/DIO1 is a no-pull, rising-edge EXTI4 input with its NVIC interrupt enabled.
- PA2/BUSY is a normal GPIO input with no pull.
- PA1/NSS is a GPIO output that starts high and will be controlled by the driver.
- CubeMX generates `SPI_NSS_PULSE_ENABLE`, but this does not drive PA1 because SPI1 uses software NSS.

## Verification Notes

- The supplied Wio-SX1262 datasheet identifies BUSY as a module output and DIO1 as the generic IRQ line.
- NSS and NRST are driven by the host MCU.
- The module pin table marks RF_SW as a host-driven input, but its footnote says RF switching is determined by the SX1262's internally connected DIO2. Confirm its intended use before implementing RF switching.
- The Semtech SX1261/SX1262 SPI interface uses mode 0, 8-bit command/data bytes, and supports SCK up to 16 MHz.
- This review is static configuration verification, not yet a build, flash, electrical, or communication test.

## Immediate Next Steps

1. Implement a minimal SX1262 hardware-abstraction layer: NSS, reset, BUSY wait, SPI transfer, and DIO1 callback/flag.
2. Add the minimum SX1262 command layer needed to reset the radio, enter standby, and read device status.
3. Build and flash a non-RF hardware smoke test that confirms BUSY behavior and valid SPI responses.
4. Configure RF frequency, packet type, modulation, PA, DIO IRQ routing, and TCXO control before attempting transmit/receive.

## Maintenance Rule

After each meaningful change:

1. Update this file if the current state or next steps changed.
2. Append a dated entry to `CHANGELOG.md` describing the change, rationale, verification, and remaining work.

