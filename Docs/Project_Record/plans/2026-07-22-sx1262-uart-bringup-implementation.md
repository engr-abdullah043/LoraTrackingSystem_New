# SX1262 UART Bring-up Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox format so progress survives agent handoffs.

**Goal:** Add a bounded, observable power-on bring-up of the Seeed Wio-SX1262 over SPI1, reporting the outcome over USART1 without starting RF transmission or reception.

**Architecture:** Keep CubeMX-generated peripheral code unchanged except for protected `USER CODE` integration in `main.c`. Vendor the pinned Semtech SX126x driver, place STM32/HAL details behind a board adapter, keep UART output behind a small logger, and let one bring-up module coordinate reset, BUSY synchronization, standby, status validation, and PASS/FAIL reporting.

**Tech Stack:** STM32CubeIDE/CubeMX, STM32U5 HAL, C11-compatible embedded C, Semtech SWSD003 SX126x driver v2.4.0 (commit `08912a2324bfc931224d368984b58b4a853078ad`), USART1 at 115200 8-N-1.

**Global Constraints:** Preserve all CubeMX-generated sections; no `printf` retargeting; no dynamic allocation; no unbounded BUSY waits; DIO1 ISR only sets a flag; no LoRa modulation, packet, frequency, PA, TX, or RX configuration in this milestone.

---

## Task 1: Add the pinned Semtech driver and provenance

**Files:**
- Create: `Firmware/Main_Controller/Drivers/SX126x/Inc/sx126x.h`
- Create: `Firmware/Main_Controller/Drivers/SX126x/Inc/sx126x_hal.h`
- Create: `Firmware/Main_Controller/Drivers/SX126x/Inc/sx126x_regs.h`
- Create: `Firmware/Main_Controller/Drivers/SX126x/Src/sx126x.c`
- Create: `Firmware/Main_Controller/Drivers/SX126x/LICENSE.txt`
- Create: `Firmware/Main_Controller/Drivers/SX126x/SOURCE.md`
- Modify: `Firmware/Main_Controller/.cproject`

- [ ] Copy only the four required driver source/header files and license verbatim from SWSD003 v2.4.0.
- [ ] Record repository URL, tag, and full commit hash in `SOURCE.md`.
- [ ] Add `Drivers/SX126x/Inc` to both Debug and Release include paths in `.cproject`; do not edit generated `Debug/*.mk` files.
- [ ] Verify provenance and includes:

```powershell
rg -n "08912a2324bfc931224d368984b58b4a853078ad|v2.4.0" Firmware/Main_Controller/Drivers/SX126x/SOURCE.md
rg -n "Drivers/SX126x/Inc" Firmware/Main_Controller/.cproject
```

Expected: source metadata is found and the include path appears in each configuration.

- [ ] Commit: `git add Firmware/Main_Controller/Drivers/SX126x Firmware/Main_Controller/.cproject && git commit -m "vendor Semtech SX126x driver v2.4.0"`

## Task 2: Implement and test the UART logger

**Files:**
- Create: `Firmware/Main_Controller/Core/Inc/app_uart.h`
- Create: `Firmware/Main_Controller/Core/Src/app_uart.c`
- Create: `Firmware/Main_Controller/Tests/test_app_uart.c`
- Create: `Firmware/Main_Controller/Tests/fakes/usart.h`

- [ ] Write a host test with a fake `HAL_UART_Transmit` that checks: text is sent through `huart1`, CRLF lines are exact, hex bytes render as two uppercase digits, null input fails, and HAL failure propagates.
- [ ] Run the test build first and confirm it fails because `app_uart` does not exist.
- [ ] Implement a blocking logger with bounded HAL timeout and explicit `APP_UART_OK`/`APP_UART_ERROR`; do not add `_write`, `printf`, `sprintf`, or heap use.
- [ ] Re-run the host test and confirm all cases pass.
- [ ] Commit: `git add Firmware/Main_Controller/Core/Inc/app_uart.h Firmware/Main_Controller/Core/Src/app_uart.c Firmware/Main_Controller/Tests && git commit -m "add bounded USART1 logger"`

## Task 3: Implement and test the STM32 SX1262 board adapter

**Files:**
- Create: `Firmware/Main_Controller/Core/Inc/sx1262_board.h`
- Create: `Firmware/Main_Controller/Core/Src/sx1262_board.c`
- Create: `Firmware/Main_Controller/Tests/test_sx1262_board.c`
- Create/Modify: HAL fakes under `Firmware/Main_Controller/Tests/fakes/`

- [ ] Write fake-HAL tests covering NSS idle/high behavior, BUSY-low success, BUSY timeout, SPI transmit/read failure, reset low/high timing, wake-up framing, and DIO1 flag set/take behavior.
- [ ] Run the tests first and confirm failure because the adapter does not exist.
- [ ] Define a context containing `hspi1`, NSS/BUSY/RESET ports and pins, bounded timeouts, last board error, and the last received status byte.
- [ ] Implement Semtech-required functions with the exact `sx126x_hal.h` signatures: `sx126x_hal_write`, `sx126x_hal_read`, `sx126x_hal_reset`, and `sx126x_hal_wakeup`.
- [ ] For every SPI transaction: wait for BUSY low with a deadline, drive NSS low, transfer command/data, always restore NSS high, and map failures to an inspectable board error.
- [ ] Reset by driving RESET low for 1 ms, high, delaying 10 ms, then waiting for BUSY low. Wake with the Semtech GET_STATUS wake sequence and a bounded wait.
- [ ] Implement `HAL_GPIO_EXTI_Rising_Callback(uint16_t GPIO_Pin)` so only `DIO1_Pin` sets a `volatile` flag; expose a take-and-clear function outside interrupt context.
- [ ] Re-run tests and confirm all cases pass.
- [ ] Commit: `git add Firmware/Main_Controller/Core/Inc/sx1262_board.h Firmware/Main_Controller/Core/Src/sx1262_board.c Firmware/Main_Controller/Tests && git commit -m "add STM32 SX1262 board adapter"`

## Task 4: Implement and test the bring-up state sequence

**Files:**
- Create: `Firmware/Main_Controller/Core/Inc/sx1262_bringup.h`
- Create: `Firmware/Main_Controller/Core/Src/sx1262_bringup.c`
- Create: `Firmware/Main_Controller/Tests/test_sx1262_bringup.c`

- [ ] Write tests using fake driver/logger boundaries for the successful order (`reset -> standby RC -> get status`) and for reset, BUSY, SPI, driver-status, invalid-chip-status, and UART-output failures.
- [ ] Run the tests first and confirm failure because bring-up does not exist.
- [ ] Implement one `sx1262_bringup_run()` entry point returning a specific result enum; stop at the first failed operation.
- [ ] Emit deterministic UART lines: boot banner, reset result, standby result, raw status byte plus decoded chip/command status, and final `SX1262 BRING-UP: PASS` or `SX1262 BRING-UP: FAIL (<reason>)`.
- [ ] Validate that the post-command chip mode is RC standby and that the Semtech command status is a successful/valid state; do not continue into RF configuration.
- [ ] Re-run tests and confirm all success and failure-path assertions pass.
- [ ] Commit: `git add Firmware/Main_Controller/Core/Inc/sx1262_bringup.h Firmware/Main_Controller/Core/Src/sx1262_bringup.c Firmware/Main_Controller/Tests && git commit -m "add SX1262 UART bring-up sequence"`

## Task 5: Integrate through CubeMX-safe user sections

**Files:**
- Modify: `Firmware/Main_Controller/Core/Src/main.c`
- Modify: `Firmware/Main_Controller/.cproject` only if source discovery/include settings require it

- [ ] Add `#include "sx1262_bringup.h"` inside `USER CODE BEGIN Includes`.
- [ ] Call `sx1262_bringup_run()` once inside `USER CODE BEGIN 2`, after `MX_GPIO_Init`, `MX_SPI1_Init`, `MX_USART1_UART_Init`, and `MX_ICACHE_Init`.
- [ ] Leave the generated infinite loop empty for this milestone; errors must have already been reported and must not hang in a hidden retry loop.
- [ ] Regenerate once from CubeMX and verify the user-section integration survives.
- [ ] Clean and build Debug in STM32CubeIDE. Expected: zero errors and zero new warnings; new app and driver `.c` files appear in build output.
- [ ] Commit: `git add Firmware/Main_Controller/Core/Src/main.c Firmware/Main_Controller/.cproject && git commit -m "run SX1262 bring-up at startup"`

## Task 6: Hardware smoke test and durable project record

**Files:**
- Modify: `Docs/Project_Record/PROJECT_STATUS.md`
- Modify: `Docs/Project_Record/CHANGELOG.md`
- Modify: this plan (check completed boxes)

- [ ] Connect the serial monitor to USART1 at 115200 baud, 8 data bits, no parity, 1 stop bit, no flow control.
- [ ] Flash/reset the board and capture the full boot transcript.
- [ ] Expected success ending:

```text
SX1262 BRING-UP: PASS
```

- [ ] If it fails, record the exact reported stage and reason before changing code or wiring; BUSY timeout, SPI failure, and invalid status are intentionally distinguishable.
- [ ] Confirm with a logic analyzer if needed: NSS idles high, SPI mode 0, clock at 10 MHz, reset pulse occurs, and BUSY returns low.
- [ ] Update status/changelog with implementation files, pinned driver version, build result, hardware transcript/result, and the next milestone (RF configuration only after bring-up passes).
- [ ] Run final repository checks:

```powershell
rg -n "sx1262_bringup_run|HAL_GPIO_EXTI_Rising_Callback|SX1262 BRING-UP" Firmware/Main_Controller/Core
rg -n "Drivers/SX126x/Inc" Firmware/Main_Controller/.cproject
git status --short
```

- [ ] Commit: `git add Docs/Project_Record Firmware/Main_Controller && git commit -m "document SX1262 bring-up verification"`

## Completion Gate

- [ ] Host-side tests pass.
- [ ] STM32CubeIDE clean build passes.
- [ ] CubeMX regeneration preserves custom integration.
- [ ] UART reports a bounded, explicit result.
- [ ] Hardware reports PASS, or the exact hardware blocker is recorded without claiming completion.
- [ ] No TX/RX or regional RF parameters were introduced.
