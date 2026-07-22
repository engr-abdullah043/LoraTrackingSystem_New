# LoRa Tracking System - Firmware Change Log

This is an append-only engineering record. Add the newest dated entry at the top, immediately below this introduction.

## 2026-07-22 - CubeMX LoRa interface configuration verified

### Changes verified

- SPI1 uses 8-bit mode 0 at 10 Mbit/s with MSB first and software NSS.
- PA5/SCK, PA6/MISO, and PA7/MOSI now use high GPIO speed.
- PA4/DIO1 uses a rising-edge EXTI4 input with no pull, and the generated interrupt handler dispatches `DIO1_Pin`.
- PA2/BUSY remains a no-pull GPIO input.

### Notes

- CubeMX still generates `SPI_NSS_PULSE_ENABLE`. This is acceptable because SPI1 uses software NSS and PA1 is controlled as an ordinary GPIO.
- Static configuration verification is complete. No build, flash, or hardware SPI test has been performed yet.

### Next

- Implement the minimal SX1262 hardware-abstraction and command layer, then perform a reset/status-read smoke test.

## 2026-07-22 - Initial pin-configuration review

### Scope

- Inspected `Main_Controller.ioc` and CubeMX-generated GPIO/SPI initialization.
- Compared the configuration with the supplied Seeed Studio Wio-SX1262 module datasheet and pin map.
- No firmware or CubeMX configuration was edited during this review.

### Findings

- Verified correct signal directions for RF_SW, NSS, BUSY, NRST, DIO1, SCK, MISO, and MOSI.
- Verified that NSS and NRST start high; RF_SW starts low.
- Found SPI1 configured for 4-bit frames at 80 Mbit/s. The SX1262 requires byte-oriented transfers and supports an SPI clock up to 16 MHz.
- Found DIO1 configured as a plain GPIO input. The module uses DIO1 as its IRQ line, so a rising-edge external interrupt is recommended.
- BUSY was correctly configured as a normal GPIO input.

### Decision

- The project owner corrected SPI1 and DIO1 in STM32CubeMX and regenerated the firmware.
- Documentation uses a current-state file (`PROJECT_STATUS.md`) plus this chronological log.

### Verification

- Static review only; no build, flash, or hardware communication test was performed.

### Next

- Re-review regenerated CubeMX output, then begin the minimal SX1262 driver/HAL integration.
