# LoRa Tracking System - Firmware Change Log

This is an append-only engineering record. Add the newest dated entry at the top, immediately below this introduction.

## 2026-07-22 - Renamed STM32 firmware project to Tower

- Renamed the STM32 firmware directory from `Firmware/Towers` to `Firmware/Tower`.
- Renamed the CubeMX project to `Tower.ioc` and the CubeIDE project/debug configuration to `Tower`.
- Updated internal build, launch, and documentation paths from `Main_Controller` to `Tower`.
- Recorded successful physical bring-up output: status `0x22` and `SX1262 BRING-UP: PASS`.
- No firmware behavior was changed; build and regeneration will be performed by the project owner.

## 2026-07-22 - Corrected SX1262 bring-up status validation

- Removed the temporary second `GetStatus` diagnostic after hardware returned the same stable `0x22` response twice.
- Bring-up now accepts RC standby when command status is RFU (`0x01`) or another non-error value.
- Command timeout (`0x03`), processing error (`0x04`), and execution failure (`0x05`) still fail validation.
- Build and hardware verification are pending and will be performed by the project owner.


## 2026-07-22 - Added second SX1262 status diagnostic

- When the first status reports command status `0x01` (RFU), firmware now performs and prints one immediate second `GetStatus` read.
- PASS/FAIL validation uses the final status read and still requires RC standby plus documented `DATA_AVAILABLE (0x02)`; RFU is not silently accepted.
- Build and hardware verification are pending and will be performed by the project owner.



## 2026-07-22 - SX1262 UART bring-up implemented

### Changes

- Vendored the official Semtech SWSD003 SX126x driver v2.4.0 at commit `08912a2324bfc931224d368984b58b4a853078ad`.
- Added a bounded blocking USART1 logger without `printf` retargeting.
- Added the STM32 board adapter for NSS, BUSY, reset, SPI read/write, wake-up, raw status capture, and DIO1 event flagging.
- Added a one-shot reset ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€šÃ‚Â ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬ÃƒÂ¢Ã¢â‚¬Å¾Ã‚Â¢ RC standby ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€šÃ‚Â ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬ÃƒÂ¢Ã¢â‚¬Å¾Ã‚Â¢ status bring-up coordinator with deterministic UART PASS/FAIL reporting.
- Integrated the bring-up call in protected CubeMX `USER CODE` sections after peripheral initialization.
- Added retained fake-HAL test harnesses for the UART, board-adapter, and bring-up boundaries.

### Verification

- Each harness was created before its production module and first failed because the interface was absent.
- All harnesses compile and link as ARM test ELFs; execution is pending because no native C runner or ARM simulator is installed.
- STM32CubeIDE 2.2.0 headless clean Debug build passed with 0 errors and 0 warnings.
- Final build size: text 31,464 bytes; data 49 bytes; BSS 1,868 bytes.
- Generated Debug artifacts were removed/restored after verification and were not committed.

### Remaining

- Flash the board and capture USART1 output at 115200 8-N-1.
- Confirm `SX1262 BRING-UP: PASS`, or record the exact BUSY/SPI/status failure.
- Verify CubeMX regeneration preserves the user sections and SX126x include path.
- RF configuration, transmit, and receive remain intentionally unimplemented.

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

