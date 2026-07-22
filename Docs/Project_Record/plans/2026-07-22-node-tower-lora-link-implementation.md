# Node-to-Tower LoRa Link Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Implement an acknowledged 915 MHz LoRa link in which Nodes sends an identifiable alive packet, Tower returns a matching ACK, and both roles report link quality and failures over UART.

**Architecture:** Mirror a small `lora_protocol` and `lora_radio` layer in both STM32 projects, then add independent non-blocking `node_app` and `tower_app` state machines. Existing board HAL and Semtech driver remain unchanged; each role is called from its CubeMX-protected `main.c` sections.

**Tech Stack:** STM32U585 HAL, SPI1, USART1, GPIO/EXTI DIO1, Semtech SWSD003 SX126x v2.4.0, SX1262 LoRa at 915 MHz.

## Global Constraints

- Carrier: 915,000,000 Hz; LoRa SF12; BW125; CR4/8; LDRO enabled; 12-symbol preamble.
- Explicit header, payload CRC enabled, standard IQ, private sync word `0x12`.
- Both radios use +22 dBm, SX1262 PA settings `{ 0x04, 0x07, 0x00, 0x01 }`, and 200 us ramp.
- Enable automatic DIO2 RF-switch control and keep PA0 `RF_SW` inactive-low.
- DATA is `NODE01|I am alive|000001`; ACK is `ACK|NODE01|000001`.
- Three total DATA attempts, 5,000 ms ACK window per attempt, then 10,000 ms before the next new transaction.
- Sequence range is `000001`-`999999`; retries reuse the sequence and the next transaction increments it.
- No dynamic allocation, unbounded waits, `printf` retargeting, ESP32 forwarding, multi-node scheduling, or mesh behavior.
- Preserve CubeMX-generated code outside `USER CODE` sections.
- The assistant performs source edits and short static checks only; the project owner builds, flashes, and tests hardware.
- Do not commit or push.

---

### Task 1: Add bounded protocol formatting and parsing

**Files:**

- Create: `Firmware/Nodes/Core/Inc/lora_protocol.h`
- Create: `Firmware/Nodes/Core/Src/lora_protocol.c`
- Create: `Firmware/Nodes/Tests/test_lora_protocol.c`
- Mirror after completion: the same three files under `Firmware/Tower`

**Interfaces:**

```c
#define LORA_PROTOCOL_NODE_ID_MAX_LEN 6U
#define LORA_PROTOCOL_PACKET_MAX_LEN  64U

typedef struct
{
    char     node_id[LORA_PROTOCOL_NODE_ID_MAX_LEN + 1U];
    uint32_t sequence;
} lora_protocol_message_t;

bool lora_protocol_format_data( const char* node_id, uint32_t sequence,
                                uint8_t* output, uint8_t capacity, uint8_t* output_length );
bool lora_protocol_parse_data( const uint8_t* payload, uint8_t length,
                               lora_protocol_message_t* message );
bool lora_protocol_format_ack( const char* node_id, uint32_t sequence,
                               uint8_t* output, uint8_t capacity, uint8_t* output_length );
bool lora_protocol_ack_matches( const uint8_t* payload, uint8_t length,
                                const char* expected_node_id, uint32_t expected_sequence );
uint32_t lora_protocol_next_sequence( uint32_t current );
```

- [ ] **Step 1: Add protocol tests before implementation.**

Tests cover exact DATA/ACK bytes, six-digit zero padding, `999999 -> 000001`, short output buffers, missing/extra separators, wrong message text, invalid node IDs, non-decimal sequences, sequence `000000`, oversized input, ACK ID mismatch, and ACK sequence mismatch.

Representative assertions:

```c
assert( lora_protocol_format_data( "NODE01", 1U, buffer, sizeof( buffer ), &length ) );
assert( length == 24U );
assert( memcmp( buffer, "NODE01|I am alive|000001", 24U ) == 0 );
assert( lora_protocol_format_ack( "NODE01", 1U, buffer, sizeof( buffer ), &length ) );
assert( memcmp( buffer, "ACK|NODE01|000001", 17U ) == 0 );
assert( lora_protocol_ack_matches( buffer, 17U, "NODE01", 1U ) );
assert( lora_protocol_next_sequence( 999999U ) == 1U );
```

- [ ] **Step 2: Implement strict bounded ASCII handling.**

Accept exactly six uppercase letters/digits for a node ID and exactly six decimal sequence characters in the range 1-999999. Build digits manually, operate on explicit lengths, and never read beyond the supplied payload.

- [ ] **Step 3: Mirror the completed protocol files into Tower.**

The Nodes and Tower `lora_protocol.c/.h` files must remain byte-for-byte identical. Mirror the protocol test as the behavioral contract.

### Task 2: Extend bounded UART diagnostics

**Files:**

- Modify: `Firmware/Nodes/Core/Inc/app_uart.h`
- Modify: `Firmware/Nodes/Core/Src/app_uart.c`
- Modify: `Firmware/Nodes/Tests/test_app_uart.c`
- Mirror: the same changes under `Firmware/Tower`

**Interfaces:**

```c
app_uart_status_t app_uart_write_u32( uint32_t value );
app_uart_status_t app_uart_write_i8( int8_t value );
app_uart_status_t app_uart_write_bytes( const uint8_t* data, uint8_t length );
```

- [ ] **Step 1: Add tests for decimal and bounded byte output.**

Cover `0`, `1`, `999999`, `UINT32_MAX`, `INT8_MIN`, `-1`, `0`, `INT8_MAX`, zero-length byte output, null input rejection, and exact-length non-null-terminated payloads.

- [ ] **Step 2: Implement conversions without formatted I/O.**

Use stack buffers, repeated division for unsigned decimal, an `int16_t` intermediate for safe `INT8_MIN` negation, and the existing bounded HAL UART transmit path.

- [ ] **Step 3: Mirror and compare both UART implementations.**

Nodes and Tower `app_uart.c/.h` and their tests must be byte-for-byte identical.

### Task 3: Add the common SX1262 radio layer

**Files:**

- Create: `Firmware/Nodes/Core/Inc/lora_radio.h`
- Create: `Firmware/Nodes/Core/Src/lora_radio.c`
- Create: `Firmware/Nodes/Tests/test_lora_radio.c`
- Create/modify: Nodes test fakes needed to capture Semtech calls
- Mirror after completion: the radio files and tests under `Firmware/Tower`

**Interfaces:**

```c
typedef enum
{
    LORA_RADIO_OK = 0,
    LORA_RADIO_INVALID_ARGUMENT,
    LORA_RADIO_DRIVER_ERROR,
    LORA_RADIO_BOARD_ERROR
} lora_radio_result_t;

typedef struct
{
    int8_t rssi_dbm;
    int8_t snr_db;
} lora_radio_packet_status_t;

lora_radio_result_t lora_radio_init( void );
lora_radio_result_t lora_radio_start_tx( const uint8_t* payload, uint8_t length );
lora_radio_result_t lora_radio_start_rx( uint32_t timeout_ms );
lora_radio_result_t lora_radio_start_continuous_rx( void );
lora_radio_result_t lora_radio_take_irq( bool* has_event, sx126x_irq_mask_t* irq );
lora_radio_result_t lora_radio_read_packet( uint8_t* payload, uint8_t capacity,
                                             uint8_t* length, lora_radio_packet_status_t* status );
```

- [ ] **Step 1: Add call-order and error-propagation tests.**

Verify the initialization sequence and constants, TX buffer bounds, RX length bounds, DIO1-without-event behavior, IRQ read-and-clear behavior, and immediate return of the first Semtech/board failure.

- [ ] **Step 2: Implement the exact common initialization sequence.**

After successful existing bring-up, call in order:

```text
set_standby(STDBY_RC)
set_reg_mode(DCDC)
set_dio2_as_rf_sw_ctrl(true)
cal_img(0xE1, 0xE9)
set_pkt_type(LORA)
set_rf_freq(915000000)
set_pa_cfg({0x04, 0x07, 0x00, 0x01})
set_tx_params(22, SX126X_RAMP_200_US)
set_lora_sync_word(0x12)
set_lora_mod_params({SF12, BW125, CR4/8, ldro=1})
set_lora_pkt_params({12, EXPLICIT, 255, crc=true, invert_iq=false})
set_buffer_base_address(0x00, 0x80)
clear_irq_status(SX126X_IRQ_ALL)
```

Drive PA0 `RF_SW` low before enabling DIO2 automatic switching.

- [ ] **Step 3: Implement bounded transmit.**

Reject null/zero/oversized payloads. Set LoRa payload length to the exact TX length, write at offset `0x00`, map `TX_DONE | TIMEOUT` to DIO1, clear all IRQs, then call `sx126x_set_tx(..., 5000U)`.

- [ ] **Step 4: Implement finite and continuous receive.**

Set RX maximum payload length to 64, map `RX_DONE | CRC_ERROR | HEADER_ERROR | TIMEOUT` to DIO1, clear all IRQs, and call `sx126x_set_rx()` with either the requested finite timeout or `SX126X_RX_CONTINUOUS`.

- [ ] **Step 5: Implement IRQ and packet reading.**

Set `*has_event = false` when `sx1262_board_take_dio1_event()` reports no event. Otherwise read then clear the returned mask, set `*has_event = true`, and propagate any driver error through `lora_radio_result_t`. For a received packet, call `sx126x_get_rx_buffer_status`, reject lengths above capacity or 64, read from `buffer_start_pointer`, then call `sx126x_get_lora_pkt_status` and return integer RSSI/SNR.

- [ ] **Step 6: Mirror and compare the common radio layer.**

Nodes and Tower radio files must be byte-for-byte identical. No copied common file may contain a Tower/Nodes project path or role-specific behavior.

### Task 4: Implement the Nodes acknowledged-transmit state machine

**Files:**

- Create: `Firmware/Nodes/Core/Inc/node_app.h`
- Create: `Firmware/Nodes/Core/Src/node_app.c`
- Create: `Firmware/Nodes/Tests/test_node_app.c`
- Modify: `Firmware/Nodes/Core/Src/main.c`

**Interfaces:**

```c
bool node_app_init( void );
void node_app_process( void );
```

- [ ] **Step 1: Add deterministic state-machine tests.**

With fake tick, radio, protocol, and UART interfaces, cover initialization failure, first-attempt success, ACK mismatch followed by a valid ACK in the same window, ACK timeout and retry, success on third attempt, three failed attempts, reuse of sequence during retries, sequence increment after a transaction, and the 10,000 ms post-transaction delay.

- [ ] **Step 2: Implement initialization.**

Call `sx1262_bringup_run()` and require `SX1262_BRINGUP_OK`, then call `lora_radio_init()`. Initialize sequence to `1`, counters to zero, and schedule the first DATA transmission immediately.

- [ ] **Step 3: Implement non-blocking transaction states.**

Use explicit states `NODE_APP_START_TX`, `NODE_APP_WAIT_TX_DONE`, `NODE_APP_WAIT_ACK`, and `NODE_APP_WAIT_NEXT`. A transaction formats `NODE01|I am alive|NNNNNN`; attempts are numbered 1-3. After `TX_DONE`, start a 5,000 ms ACK window. Invalid/mismatched packets are logged and RX restarts for the wrap-safe remaining window. Timeout or hard error retries the same sequence; success or the third failure schedules the next transaction for `HAL_GetTick() + 10000U`.

- [ ] **Step 4: Integrate Nodes main.c.**

Replace the bring-up include/call with:

```c
/* USER CODE BEGIN Includes */
#include "node_app.h"
/* USER CODE END Includes */
```

```c
/* USER CODE BEGIN 2 */
( void ) node_app_init();
/* USER CODE END 2 */
```

```c
/* USER CODE BEGIN WHILE */
while (1)
{
  node_app_process();
/* USER CODE END WHILE */
```

Keep the generated loop closing structure intact.

### Task 5: Implement the Tower receive-and-ACK state machine

**Files:**

- Create: `Firmware/Tower/Core/Inc/tower_app.h`
- Create: `Firmware/Tower/Core/Src/tower_app.c`
- Create: `Firmware/Tower/Tests/test_tower_app.c`
- Modify: `Firmware/Tower/Core/Src/main.c`

**Interfaces:**

```c
bool tower_app_init( void );
void tower_app_process( void );
```

- [ ] **Step 1: Add deterministic Tower tests.**

Cover initialization failure, valid DATA reception, RSSI/SNR logging, ACK format, ACK TX completion followed by continuous RX, duplicate DATA acknowledged but counted once, malformed DATA, CRC/header errors, RX timeout, ACK TX timeout, and recovery to continuous RX.

- [ ] **Step 2: Implement initialization and receive state.**

Require successful bring-up and common radio initialization, clear counters/cache, then start continuous RX. Use states `TOWER_APP_RECEIVING` and `TOWER_APP_WAIT_ACK_TX_DONE`.

- [ ] **Step 3: Implement DATA handling and duplicate suppression.**

On `RX_DONE`, read and strictly parse DATA. A new `(node_id, sequence)` is logged with RSSI/SNR and stored as the most recently accepted packet. The same pair is logged as a duplicate. Both cases format and transmit `ACK|node_id|sequence`. Invalid packets receive no ACK.

- [ ] **Step 4: Implement recovery.**

After ACK `TX_DONE`, ACK timeout, CRC/header error, invalid packet, or recoverable radio error, clear the condition and restore continuous RX. Never remain in standby after a completed ACK attempt.

- [ ] **Step 5: Integrate Tower main.c.**

Use the same protected-section structure as Nodes, with `#include "tower_app.h"`, one `( void ) tower_app_init();` call after peripheral initialization, and `tower_app_process();` inside the generated infinite loop.

### Task 6: Update project metadata and engineering records

**Files:**

- Modify: `Firmware/Nodes/.cproject`
- Modify: `Firmware/Tower/.cproject`
- Modify: `Docs/Project_Record/PROJECT_STATUS.md`
- Modify: `Docs/Project_Record/CHANGELOG.md`

- [ ] **Step 1: Confirm source discovery and includes.**

Both projects already discover `Core` recursively and include `Core/Inc`; retain those settings. Add no path unless static inspection proves a new directory is outside current discovery.

- [ ] **Step 2: Record the implementation state.**

Document the exact radio profile, DATA/ACK format, retry policy, role state machines, diagnostics, and that owner build/flash verification is pending.

- [ ] **Step 3: Perform short source-level verification.**

Confirm:

```text
Common protocol/radio/UART files match across Nodes and Tower.
Nodes main.c contains one node_app_init and one node_app_process call.
Tower main.c contains one tower_app_init and one tower_app_process call.
915000000, SF12, BW125, CR4/8, LDRO=1, +22 dBm, DIO2 RF switch, CRC, and sync word 0x12 appear in the radio configuration.
DATA/ACK formats, 5000 ms ACK window, three attempts, and 10000 ms interval appear in role code/tests.
No ESP32 forwarding, mesh routing, dynamic allocation, project-name leakage, or generated Debug artifacts are added.
```

- [ ] **Step 4: Project-owner verification.**

The owner cleans/builds and flashes both projects, monitors USART1 at 115200 8-N-1, verifies increasing acknowledged sequences, powers Tower off to observe exactly three attempts and final failure, then powers Tower on to verify recovery. Record the complete UART transcripts before proceeding to ESP32 forwarding or multi-node work.

