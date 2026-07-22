# Nodes LoRa Bring-Up Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Give `Firmware/Nodes` the same verified SX1262 reset, standby, status, and UART bring-up behavior as `Firmware/Tower`.

**Architecture:** Preserve the Nodes CubeMX/CubeIDE project and copy only the proven LoRa driver, board adapter, UART logger, bring-up module, and tests. Integrate the copied layer through the existing Nodes `main.c` user sections and compiler include paths.

**Tech Stack:** STM32U585 HAL, SPI1, USART1, GPIO/EXTI, Semtech SWSD003 SX126x driver v2.4.0, STM32CubeIDE managed build.

## Global Constraints

- Keep the folder, CubeMX file, and CubeIDE project name as `Nodes`.
- Preserve `Firmware/Nodes/Nodes.ioc` and all CubeMX-generated peripheral initialization.
- Do not copy Tower project metadata, linker scripts, or Debug artifacts.
- Do not add transmit, receive, RF configuration, or application protocol behavior.
- The assistant must not build, flash, commit, or push; the project owner performs hardware verification.

---

### Task 1: Mirror the proven LoRa modules

**Files:**

- Create: `Firmware/Nodes/Drivers/SX126x/Inc/sx126x.h`
- Create: `Firmware/Nodes/Drivers/SX126x/Inc/sx126x_hal.h`
- Create: `Firmware/Nodes/Drivers/SX126x/Inc/sx126x_regs.h`
- Create: `Firmware/Nodes/Drivers/SX126x/Src/sx126x.c`
- Create: `Firmware/Nodes/Drivers/SX126x/LICENSE.txt`
- Create: `Firmware/Nodes/Drivers/SX126x/SOURCE.md`
- Create: `Firmware/Nodes/Core/Inc/app_uart.h`
- Create: `Firmware/Nodes/Core/Src/app_uart.c`
- Create: `Firmware/Nodes/Core/Inc/sx1262_board.h`
- Create: `Firmware/Nodes/Core/Src/sx1262_board.c`
- Create: `Firmware/Nodes/Core/Inc/sx1262_bringup.h`
- Create: `Firmware/Nodes/Core/Src/sx1262_bringup.c`
- Create: `Firmware/Nodes/Tests/test_app_uart.c`
- Create: `Firmware/Nodes/Tests/test_sx1262_board.c`
- Create: `Firmware/Nodes/Tests/test_sx1262_bringup.c`
- Create: `Firmware/Nodes/Tests/fakes/main.h`
- Create: `Firmware/Nodes/Tests/fakes/spi.h`
- Create: `Firmware/Nodes/Tests/fakes/usart.h`

**Interfaces:**

- Consumes: Nodes-generated `hspi1`, `huart1`, and PA0-PA4 GPIO definitions.
- Produces: `bool sx1262_bringup_run(void)` and the Semtech `sx126x_hal_*` board interface.

- [ ] **Step 1: Copy the complete SX126x driver directory from Tower to Nodes.**

Copy `Firmware/Tower/Drivers/SX126x` byte-for-byte to `Firmware/Nodes/Drivers/SX126x`, retaining the Semtech source provenance and license.

- [ ] **Step 2: Copy the application modules from Tower to Nodes.**

Copy these six files without behavior changes:

```text
Core/Inc/app_uart.h
Core/Src/app_uart.c
Core/Inc/sx1262_board.h
Core/Src/sx1262_board.c
Core/Inc/sx1262_bringup.h
Core/Src/sx1262_bringup.c
```

- [ ] **Step 3: Copy the existing host-side tests and fakes.**

Copy `Firmware/Tower/Tests` byte-for-byte to `Firmware/Nodes/Tests` so both firmware projects retain the same behavioral reference tests.

- [ ] **Step 4: Compare the copied files.**

Run read-only comparisons between each Tower source tree and its Nodes counterpart. Expected result: no differences.

### Task 2: Integrate bring-up into the Nodes project

**Files:**

- Modify: `Firmware/Nodes/Core/Src/main.c`
- Modify: `Firmware/Nodes/.cproject`

**Interfaces:**

- Consumes: `bool sx1262_bringup_run(void)` from Task 1.
- Produces: one SX1262 bring-up attempt after GPIO, ICACHE, SPI1, and USART1 initialization.

- [ ] **Step 1: Add the bring-up header inside the CubeMX Includes user section.**

```c
/* USER CODE BEGIN Includes */
#include "sx1262_bringup.h"
/* USER CODE END Includes */
```

- [ ] **Step 2: Run bring-up inside the CubeMX user initialization section.**

```c
/* USER CODE BEGIN 2 */
( void ) sx1262_bringup_run();
/* USER CODE END 2 */
```

- [ ] **Step 3: Add the Semtech include directory to both Debug and Release compiler settings.**

Add the following entry to each C compiler include-path option in `Firmware/Nodes/.cproject`:

```xml
<listOptionValue builtIn="false" value="../Drivers/SX126x/Inc"/>
```

Also include `../Drivers/SX126x/Inc` in both generated Defaults option summaries. Keep all Nodes project IDs, names, build paths, and linker paths unchanged. The existing `Drivers` source entry already discovers `Drivers/SX126x/Src/sx126x.c`.

- [ ] **Step 4: Perform static integration checks.**

Confirm `main.c` contains exactly one include and one call, `.cproject` contains the SX126x include path for Debug and Release, and no copied file refers to `Firmware/Tower` or the Tower project name.

### Task 3: Record the Nodes integration and owner verification

**Files:**

- Modify: `Docs/Project_Record/PROJECT_STATUS.md`
- Modify: `Docs/Project_Record/CHANGELOG.md`

**Interfaces:**

- Consumes: completed Nodes integration from Tasks 1-2.
- Produces: an accurate handoff describing both firmware projects and the pending physical test.

- [ ] **Step 1: Update current project status.**

Record that Tower hardware bring-up is verified, Nodes now contains the same bring-up implementation, and Nodes build/flash/UART verification is pending with the project owner.

- [ ] **Step 2: Add the newest changelog entry.**

List the copied driver, board adapter, UART logger, bring-up module, test files, `main.c` integration, and `.cproject` include-path update. State explicitly that the assistant did not build, flash, commit, or push.

- [ ] **Step 3: Project-owner hardware verification.**

The project owner cleans and builds `Firmware/Nodes` in STM32CubeIDE, flashes it, and confirms this UART result:

```text
=== SX1262 UART BRING-UP ===
Reset: OK
Standby RC: OK
Status: 0x22, chip_mode=0x02, cmd_status=0x01
SX1262 BRING-UP: PASS
```

If the UART result differs, retain the exact output for diagnosis before adding transmit or receive functionality.
