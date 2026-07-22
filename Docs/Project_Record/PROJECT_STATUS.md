# LoRa Tracking System - Project Status

Last updated: 2026-07-22

## Purpose

This file is the current-state handoff for developers and AI agents. Keep it concise and update it whenever the firmware architecture, hardware mapping, verified behavior, or immediate next steps change. Record individual changes in `CHANGELOG.md`.

## Current Phase

The minimal SX1262 reset/standby/status bring-up is implemented on branch `feature/sx1262-uart-bringup` and passes a clean STM32CubeIDE Debug build. Flashing and UART/SPI verification on physical hardware are still pending. RF configuration, transmit, and receive are intentionally out of scope for this milestone.

## Hardware

- MCU: STM32U585CIU6, UFQFPN48
- LoRa module: Seeed Studio Wio-SX1262, 862-930 MHz, IPEX antenna connection
- Module supply: 3.3 V typical; datasheet operating range is 1.8-3.6 V
- CubeMX project: `Firmware/Main_Controller/Main_Controller.ioc`
- Module references: `Docs/Wio-SX1262_Module_Datasheet.pdf` and `Docs/Wio-SX1262-pin_map.png`

## LoRa Interface Mapping

| Module signal | STM32 pin | STM32 configuration | Signal direction relative to STM32 | Status |
|---|---:|---|---|---|
| RF_SW | PA0 | GPIO output, push-pull, initial low | Output | Direction verified; unused by bring-up |
| NSS | PA1 | GPIO output, push-pull, initial high | Output | Implemented |
| BUSY | PA2 | GPIO input, no pull | Input | Implemented with 100 ms timeout |
| NRST | PA3 | GPIO output, push-pull, initial high | Output | Implemented |
| DIO1 | PA4 | Rising-edge EXTI4 input, no pull | Input | ISR flag implemented |
| SCK | PA5 | SPI1_SCK, AF5, high speed | Output | Implemented |
| MISO | PA6 | SPI1_MISO, AF5, high speed | Input | Implemented |
| MOSI | PA7 | SPI1_MOSI, AF5, high speed | Output | Implemented |

Power and ground must be wired to the module's 3V3 and GND connections. The SMD radio module VCC has a 3.9 V absolute maximum; any 5 V header on an adapter board must not be confused with the module's VCC.

## Verified CubeMX Configuration

- SPI1 uses 8-bit frames, mode 0, MSB first, full-duplex master, software NSS, and CRC disabled.
- SPI1 uses a prescaler of 16, producing 10 Mbit/s from the 160 MHz SPI1 kernel clock.
- PA5-PA7 use high GPIO speed and no internal pull.
- PA4/DIO1 is a no-pull, rising-edge EXTI4 input with its NVIC interrupt enabled.
- PA2/BUSY is a normal GPIO input with no pull.
- PA1/NSS is a GPIO output that starts high and is controlled by the board adapter.
- USART1 uses PA9/PA10 at 115200 baud, 8 data bits, no parity, and 1 stop bit.
- CubeMX generates `SPI_NSS_PULSE_ENABLE`, but this does not drive PA1 because SPI1 uses software NSS.

## Firmware Implementation

- Official Semtech SWSD003 SX126x driver v2.4.0 is pinned under `Firmware/Main_Controller/Drivers/SX126x` at commit `08912a2324bfc931224d368984b58b4a853078ad`.
- `app_uart.*` provides bounded blocking USART1 text, CRLF, and hexadecimal output without `printf` retargeting.
- `sx1262_board.*` implements Semtech HAL write/read/reset/wakeup functions, bounded BUSY waits, NSS handling, SPI error mapping, raw status capture, and the DIO1 event flag.
- `sx1262_bringup.*` performs reset, RC standby, and status validation. If the first command status is RFU (`0x01`), it performs one immediate second status read before reporting PASS/FAIL.
- `main.c` calls bring-up once after all CubeMX peripheral initialization using protected `USER CODE` sections.
- No frequency, modulation, packet, PA, IRQ-routing, TX, or RX configuration is present.

## Verification Status

- STM32CubeIDE 2.2.0 headless clean Debug build: PASS, 0 errors and 0 warnings.
- Build size: text 31,464 bytes; data 49 bytes; BSS 1,868 bytes.
- Fake-HAL test harnesses were written test-first and compile/link successfully as ARM ELFs.
- The installed toolchain has no native C runner or ARM simulator, so those retained harnesses have not been executed.
- Physical flash, UART transcript, SPI response, BUSY timing, and CubeMX-regeneration survival remain unverified.
- The next hardware run should print `Status #1`; if it is RFU (`0x01`), it will also print `Status #2`. Expected successful final line: `SX1262 BRING-UP: PASS`.

## Immediate Next Steps

1. Open the project in STM32CubeIDE, flash the board, and monitor USART1 at 115200 8-N-1 with no flow control.
2. Capture the complete UART transcript and record PASS or the exact reported failure reason.
3. If needed, verify NSS, SCK, RESET, and BUSY with a logic analyzer.
4. Regenerate once from CubeMX and confirm the `main.c` user sections and SX126x include path remain intact.
5. Only after bring-up passes, design the regional RF, modulation, packet, PA, TCXO/RF-switch, IRQ, TX, and RX configuration.

## Maintenance Rule

After each meaningful change:

1. Update this file if the current state or next steps changed.
2. Append a dated entry to `CHANGELOG.md` describing the change, rationale, verification, and remaining work.
