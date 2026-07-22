# LoRa Bench Reliability Tuning Implementation Plan

**Goal:** Improve short-range Node/Tower packet and ACK reliability while preserving a documented maximum-range profile for later field testing.

**Constraints:** Source and documentation edits only. The user will build, flash, and test. No Git operations.

## Active bench profile

- Frequency: 915 MHz
- Spreading factor: SF9
- Bandwidth: 125 kHz
- Coding rate: 4/5
- Low-data-rate optimization: disabled
- Preamble: 12 symbols
- Header: explicit
- Payload CRC: enabled
- IQ: standard
- Private sync word: 0x12
- TX power: 0 dBm
- Tower ACK guard delay: 250 ms

## Implementation tasks

1. Update the mirrored Node and Tower radio modulation configuration in `Core/Src/lora_radio.c` from SF12/CR4/8/LDRO-on to SF9/CR4/5/LDRO-off.
2. Keep the existing 915 MHz frequency, preamble, explicit header, CRC, standard IQ, private sync word, DIO2 RF-switch control, DIO3 TCXO setup, calibration band, and 0 dBm power.
3. Add a 250 ms Tower ACK guard immediately before ACK transmission so the Node can complete its TX-to-RX transition.
4. Retain duplicate DATA acknowledgement behavior. Convert the guard to a non-blocking scheduled state before introducing multiple simultaneous nodes.
5. Update Node and Tower UART readiness messages so the printed profile matches the programmed radio settings.
6. Update `PROJECT_STATUS.md` and `CHANGELOG.md` with the active bench profile, rationale, and reserved maximum-range profile.

## User verification

1. Build and flash both projects.
2. Power the Tower first, then the Node, with both antennas attached and separated by approximately 1–3 metres.
3. Confirm each Node DATA transmission reaches `TX done`, receives a matching ACK, and advances its sequence number.
4. Confirm the Tower prints valid DATA, waits before ACK TX, and returns to continuous RX.
5. Record packet retries, header/CRC errors, RSSI, and SNR over at least 20 sequences.

## Reserved maximum-range profile

For later outdoor range testing: SF12, BW125, CR4/8, LDRO enabled, and +22 dBm with suitable antennas and regional/legal confirmation.
