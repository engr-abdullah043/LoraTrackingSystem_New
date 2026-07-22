# SX1262 UART Bring-up Design

Date: 2026-07-22
Status: Approved for implementation planning

## Objective

Add the smallest maintainable firmware path that proves communication between the STM32U585CIU6 and the Seeed Studio Wio-SX1262. On every boot, firmware will reset the radio, wait for BUSY to deassert, place the radio in standby, read and validate its status, and print one clear PASS or FAIL report through USART1 at 115200 8N1.

## Scope

This milestone includes:

- A vendored, pinned release of Semtech's SX126x driver and its license files.
- An STM32 HAL adapter for SPI1, manual NSS, reset, BUSY, delays, wake-up, and DIO1 notification.
- A small blocking USART1 logger for fixed diagnostic strings and hexadecimal bytes.
- A one-shot startup bring-up coordinator.
- Time-bounded error handling and readable UART diagnostics.
- Build verification and a hardware serial-monitor smoke test.

This milestone does not configure LoRa modulation, RF frequency, packet format, transmit power, TCXO, DIO2 RF switching, transmission, reception, or LoRaWAN.

## Existing Generated Interface

- SPI1: PA5 SCK, PA6 MISO, PA7 MOSI; 8-bit mode 0; MSB first; 10 Mbit/s; software NSS.
- NSS: PA1 GPIO output, initially high.
- BUSY: PA2 GPIO input, no pull.
- NRST: PA3 GPIO output, initially high.
- DIO1: PA4 rising-edge EXTI4 input, no pull.
- USART1: PA9 TX, PA10 RX; 115200 baud, 8 data bits, no parity, 1 stop bit.
- Generated peripheral initialization is split into `gpio`, `spi`, `usart`, and `icache` source/header pairs.

## Architecture

### Semtech SX126x driver

The required SX126x driver files will be vendored under `Drivers/SX126x` from Semtech SWSD003 release `v2.4.0`. Vendoring makes builds reproducible and avoids runtime or build-time network dependencies. The upstream license and version/source note will be kept beside the code.

Only the SX126x driver portions required by the transceiver API will be added. Semtech examples, unrelated LR11xx support, board support packages, and application code will not be copied.

### STM32 board adapter

`Core/Inc/sx1262_board.h` and `Core/Src/sx1262_board.c` will implement the platform operations expected by the Semtech driver:

- Assert and release PA1/NSS around complete radio transactions.
- Transfer command and response bytes through `hspi1` with bounded HAL timeouts.
- Pulse PA3/NRST low and then release it high.
- Poll PA2/BUSY with a millisecond timeout based on `HAL_GetTick()`.
- Provide delays through `HAL_Delay()`.
- Perform the Semtech wake-up transaction when required.
- Store a `volatile` DIO1 event flag; no SPI or radio processing occurs inside the interrupt callback.

The adapter will use the generated handles and pin definitions from `spi.h` and `main.h`. It will not duplicate CubeMX pin constants.

### UART diagnostics

`Core/Inc/app_uart.h` and `Core/Src/app_uart.c` will provide a small blocking logger using `huart1` and `HAL_UART_Transmit()`.

The first milestone will not retarget `printf` or modify generated `syscalls.c`. The logger will support fixed strings, CRLF line endings, and two-digit hexadecimal byte output. Every write will have a bounded timeout.

### Bring-up coordinator

`Core/Inc/sx1262_bringup.h` and `Core/Src/sx1262_bringup.c` will own the one-shot sequence and return a typed result:

1. Print the firmware and UART banner.
2. Initialize the board adapter context.
3. Reset the SX1262.
4. Wait for BUSY to become low.
5. Request standby using the RC oscillator.
6. Read the radio status using the Semtech driver.
7. Reject transport errors and status values that cannot represent a valid chip/command state.
8. Print decoded status and a final PASS or FAIL line.
9. Leave the radio in standby and return to the existing main loop.

`main.c` will include and invoke the coordinator only inside CubeMX `USER CODE` sections after GPIO, SPI1, and USART1 initialization. CubeMX-generated initialization functions will remain unedited.

## Interrupt Flow

The existing generated `EXTI4_IRQHandler()` calls `HAL_GPIO_EXTI_IRQHandler(DIO1_Pin)`. Firmware will implement `HAL_GPIO_EXTI_Rising_Callback()` in a user-owned application source file. When the pin is DIO1, the callback sets a flag and returns immediately.

DIO1 is not expected during the reset/status smoke test, but this hook establishes the safe interrupt boundary required by later transmit/receive work.

## Error Handling

The bring-up result will distinguish at least:

- BUSY timeout before or after a command.
- SPI transmit/receive failure.
- Semtech driver command failure.
- Invalid or reserved status response.
- UART transmission failure.

BUSY polling and HAL operations must always be bounded. Failures print the most specific available error, leave NSS high and reset released, and then return to the main loop. Bring-up failure must not call `Error_Handler()` or block indefinitely.

## Expected Serial Output

Successful startup:

```text
[BOOT] LoRa Tracking System
[UART] USART1 115200 8N1
[SX1262] Resetting...
[SX1262] BUSY: LOW
[SX1262] SetStandby: OK
[SX1262] Status: 0xNN
[SX1262] Bring-up: PASS
```

Failure example:

```text
[SX1262] Bring-up: FAIL (BUSY_TIMEOUT)
```

The precise valid status byte depends on the radio state returned by hardware; the test will validate decoded fields rather than require one hard-coded byte.

## Verification

Software verification:

- Compile the complete Debug configuration with warnings enabled.
- Confirm the ELF links the Semtech driver, board adapter, UART logger, and bring-up coordinator exactly once.
- Check that the CubeMX-generated files contain no application edits outside `USER CODE` sections.
- Exercise status decoding and error-to-text mapping with deterministic host-side tests where practical.

Hardware verification:

- Connect a serial monitor to USART1 at 115200 8N1.
- Power-cycle or reset the MCU.
- Observe exactly one startup report.
- Confirm BUSY becomes low, standby succeeds, status is valid, and the final result is PASS.
- If the result is FAIL, use its explicit error code to diagnose reset, BUSY, NSS, SPI, or UART wiring before any RF configuration is attempted.

## Completion Criteria

This milestone is complete only when:

- The firmware builds cleanly from repository-contained sources.
- UART emits the complete one-shot diagnostic.
- The connected Wio-SX1262 returns a valid status and the diagnostic reports PASS.
- The result and any hardware observations are recorded in `PROJECT_STATUS.md` and `CHANGELOG.md`.
