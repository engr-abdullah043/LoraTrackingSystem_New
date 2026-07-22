# Nodes LoRa Bring-Up Design

Date: 2026-07-22

## Goal

Give the existing `Firmware/Nodes` STM32 project the same verified SX1262 UART bring-up behavior as `Firmware/Tower`, so the project owner can build, flash, and verify both boards independently.

## Design

The Nodes project keeps its current `Nodes` folder, CubeMX file, CubeIDE project metadata, generated peripheral files, and linker scripts. Only the proven LoRa-specific implementation is copied and integrated.

The following Tower components will be mirrored into Nodes:

- Official Semtech SX126x driver under `Drivers/SX126x`.
- `app_uart.c/.h` for bounded USART1 diagnostic output.
- `sx1262_board.c/.h` for SPI1, NSS, BUSY, reset, RF switch, and DIO1 handling.
- `sx1262_bringup.c/.h` for reset, RC standby, status reporting, and PASS/FAIL validation.
- Existing host-side source tests and HAL fakes.

The Nodes `main.c` will call the same bring-up function after CubeMX peripheral initialization. Its `.cproject` will include the Semtech header directory and compile the copied source directory. CubeMX-owned initialization remains based on `Nodes.ioc`, whose MCU, PA0-PA7 LoRa mapping, SPI1 parameters, and USART1 configuration already match Tower.

## Runtime Flow

After startup, Nodes initializes GPIO, ICACHE, SPI1, and USART1, then runs the SX1262 bring-up once. UART should report reset success, RC standby success, status `0x22`, and `SX1262 BRING-UP: PASS`. The normal infinite loop then continues.

## Error Handling

The copied implementation retains the existing BUSY timeout, SPI error propagation, reset validation, and status checks. Failures remain visible over UART and do not silently report success.

## Verification Boundary

The assistant will compare the copied source and integration points with Tower but will not build, flash, test hardware, commit, or push. The project owner will clean/build Nodes in STM32CubeIDE, flash it, and confirm the UART bring-up output.

## Out of Scope

LoRa packet transmission, reception, RF parameters, protocol design, and application behavior are deferred to the next phase.
