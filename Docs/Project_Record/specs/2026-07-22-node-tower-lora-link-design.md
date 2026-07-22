# Node-to-Tower LoRa Link Design

Date: 2026-07-22

## Goal

Establish a reliable, measurable point-to-point LoRa link in which one Nodes board sends an alive packet, Tower acknowledges it, and both devices report communication status over UART.

## Scope

This milestone supports one node and one tower. ESP32-S3 forwarding, multiple active nodes, collision avoidance, routing, and mesh relaying are deferred.

## Radio Profiles

### Active bench-reliability profile

- Carrier frequency: 915,000,000 Hz.
- Packet type: LoRa.
- Spreading factor: SF9.
- Bandwidth: 125 kHz.
- Coding rate: 4/5.
- Low-data-rate optimization: disabled.
- Preamble: 12 symbols.
- Header: explicit.
- Payload CRC: enabled.
- IQ: standard, not inverted.
- Sync word: private LoRa network value `0x12`.
- Transmit power: 0 dBm on both Node and Tower.
- RF switch: automatic control through SX1262 DIO2; PA0 `RF_SW` remains inactive-low.
- TCXO: DIO3 at 3.0 V with a 5 ms startup timeout.
- Image calibration band: 902-928 MHz.

This profile is selected for clean bench verification with matched 915 MHz antennas separated by at least 1-3 metres. SF9 reduces DATA and ACK airtime substantially while retaining generous margin at the measured approximately -48 dBm RSSI and positive SNR.

### Reserved maximum-range profile

The later outdoor range profile remains 915 MHz, SF12, BW125, CR4/8, LDRO enabled, 12-symbol preamble, CRC, and +22 dBm. It must not be used at breadboard distance and must never transmit without an antenna.

## Software Boundaries

Each STM32 project retains its independent source tree and CubeMX configuration.

- `lora_radio.*` owns common SX1262 initialization, radio parameters, IRQ retrieval/clearing, payload transmission, continuous reception, packet reading, RSSI, and SNR.
- `lora_protocol.*` formats and parses bounded ASCII DATA and ACK packets without dynamic allocation or `printf` retargeting.
- `node_app.*` owns the Node transmit, ACK-wait, retry, sequence, and interval state machine.
- `tower_app.*` owns Tower continuous reception, validation, duplicate detection, ACK transmission, and return-to-receive behavior.
- Existing `sx1262_board.*`, `app_uart.*`, and the Semtech driver remain the hardware and transport foundations.

Only `node_app.*` is present in `Firmware/Nodes`; only `tower_app.*` is present in `Firmware/Tower`. The common radio and protocol files are mirrored in both projects.

## Packet Protocol

The Node ID is the compile-time string `NODE01`. The sequence is a six-digit decimal value from `000001` through `999999`, then wraps to `000001`.

DATA packet:

```text
NODE01|I am alive|000001
```

ACK packet:

```text
ACK|NODE01|000001
```

Packets with an invalid field count, unexpected Node ID, malformed sequence, excessive length, CRC error, or unrelated message type are rejected. The Node accepts an ACK only when both the Node ID and sequence match its outstanding DATA packet.

## Node State Machine

1. Run the existing SX1262 bring-up. Do not start the link if bring-up fails.
2. Configure the common 915 MHz maximum-range profile.
3. Format the current DATA packet and transmit it.
4. Wait for `TX_DONE`, then enter receive mode and wait up to 5,000 ms for the matching ACK.
5. On a matching ACK, record success and finish the transaction.
6. Ignore CRC-invalid, malformed, or mismatched packets while the 5,000 ms ACK window remains open. On ACK timeout or a hard radio error, retry the same sequence up to two times. This gives three total transmission attempts.
7. After success or final failure, wait 10,000 ms before starting a new sequence.
8. Increment the sequence only when starting the next new transaction; all retries reuse the outstanding sequence.

Timing uses wrap-safe comparisons against `HAL_GetTick()`. Radio work occurs in the main loop; the DIO1 interrupt only records an event flag.

## Tower State Machine

1. Run the existing SX1262 bring-up. Do not start the link if bring-up fails.
2. Configure the identical common radio profile and enter continuous receive mode.
3. On `RX_DONE`, read and validate the payload and packet status.
4. For a new valid DATA packet, print its Node ID, sequence, RSSI, and SNR, then wait 250 ms before transmitting the matching ACK so Nodes has completed its TX-to-RX transition.
5. For a valid duplicate of the most recently accepted Node ID and sequence, increment the duplicate count and schedule the same ACK after the same 250 ms guard without reporting it as new application data.
6. On `TX_DONE`, immediately return to continuous receive mode.
7. Clear and report CRC, header, timeout, and radio errors, then restore continuous receive mode when required.

Duplicate handling ensures that a lost ACK does not cause the retried DATA packet to be processed twice.

## UART Diagnostics

Nodes reports initialization, sequence, attempt number, TX completion, ACK match, ACK mismatch, timeout, retry, final success, and final failure.

Tower reports initialization, valid DATA, duplicate DATA, invalid payload, CRC/header error, ACK transmission, return to receive mode, RSSI, and SNR.

Both roles maintain counters suitable for UART inspection: DATA transmissions, valid receptions, successful acknowledgments, retries, timeouts, CRC/header errors, invalid packets, duplicates, and radio errors.

## Error Handling

- Every Semtech driver call is checked.
- BUSY waits retain their existing bounded timeout.
- IRQ status is read and cleared before the next radio operation.
- Payload buffers are fixed-size and always length-checked.
- A failure is printed with its stage and status; it is never converted into a false success.
- Tower restores continuous RX after recoverable errors.
- Node completes the current attempt as failed and follows the bounded retry policy.

## Verification

The project owner builds and flashes both projects. Success requires:

- Both boards report successful SX1262 bring-up and the active SF9/BW125/CR4/5/0 dBm bench profile.
- Tower prints the exact Node ID and sequence received from Nodes.
- Nodes prints a matching ACK for the same ID and sequence.
- Subsequent transactions use increasing sequences and begin 10 seconds after the preceding transaction completes.
- Temporarily disabling Tower causes Nodes to show two retries and one final failure for the same sequence.
- Re-enabling Tower restores successful acknowledged transactions.
- Tower UART reports RSSI and SNR for each valid or duplicate DATA packet.

The assistant performs source-level checks only. Build, flash, hardware testing, Git commit, and Git push remain with the project owner.


