# LIFELOG_ANALYSIS — DW-H5600 Reverse Engineering Report

Source: analysis/firmware_reverse/05_protocols/ble/ (8 reports, 2026-11-18)
Status: Analysis complete, cross-referenced with gshock_api and CasioLab FAP.

## Key findings

### 1. Framing (0x05 flow)
- First notification = 0x05 + 22 bytes, dual CRC (crc16_kermit) validated
- No length prefix, no CRC chain after header
- MTU-imposed chunking: 244/23/13/102 byte sizes
- No standard codec (zlib/gzip/deflate/xz) decodes payload

### 2. Records 0x00 and 0x65
- 12 anchored pairs (not 4) across 4 captures
- 0x00 = 15 bytes: [year-2000][month][day][hour][minute] binary + int32 + int32 + u16
- 0x65 = 99 bytes, 97% constant intra-session, no date (dated by preceding 0x00)
- Cadence: 60s dominant, gaps counted by u16 counter
- u16 counter at body[18..19], increments by 1 per record

### 3. Emulator fidelity
- Cycle 1 = 372 blocks, types 00 65 50 51 52 56 72
- 0x00/0x65 pairs: 1 per cycle (5 pairs / 5 cycles in capture -4)
- 00 01 04 = dominant measured form (132/19 occurrences) — emulator serves it
- 47 01: 8 occurrences in 3 captures, all on 0x0018 H->C, 232-379ms after echo
- 5A 01: 16 writes C->H on 0x0016, 16 responses on 0x0018 (1:1, response not beacon)
- 3D: 5 forms, lengths 17/18 o (not 18/19 as previously documented)

### 4. Application (Casio WATCHES)
- QW3515 path IS the DW-H5600 path (no model exclusion)
- 00 01 04 causes app to abandon (startState=1, startReason=4)
- No successful LifeLog transfer to DW-H5600 in corpus
- App polls every ~30s

### 5. Momentum compatibility
- Momentum does not expose Central GATT to FAPs
- Flipper FAP = UI, storage, decoding, analysis
- RPi companion = GATT central (scan, connect, discover, read, notify)

### 6. Command IDs (from gshock_api)
See PROTOCOL_CATALOG.md for full list (0x09-0x47).

### 7. Status legend
- VALIDATED: measured, reproducible
- VALIDATED-RESTRICTED: true within stated limits
- CONTRADICTED: measurement says otherwise
- NON-ATTRIBUTED: source ambiguous (emulator in bootloader)
- TO-ARBITRATE: test not yet exists

### 8. Open items
- T-B: context of 00 01 04 (abandon vs re-poll) — needs BLE replay
- T-C: 47 01 delay on real card — needs BLE session (no BT adapter on host)
- T-F: origin of 0x56 gap (H2 transport vs H3 emitter) — needs HCI capture
- T-G: 0x3D semantics — needs watch variation
- T-H: CASIO WATCHES app vs emulator — needs real phone connection
- T-I: 2026-11-18 measurements — needs awake watch

---

# Correction (2026-11-19)

## "Analysis complete" was too strong

Nothing above is deleted. But the status line was wrong: this is a working
notebook of observations, not a finished analysis. What is actually settled,
and what is not:

**Settled, reproduced in this project's tests:**

- Framing of the `0x05` flow: chunk `05` + payload complemented (XOR 0xFF) +
  CRC16-KERMIT stored little-endian at the end, or the same chunk without the
  trailing CRC when it was split to the MTU.
- The record stream `[type][len u32 LE][body]`, and record `0x00` as 15 bytes
  `[year-2000][month][day][hour][minute]` + int32 + int32 + u16.
- A real desynchronisation at offset 124 (one `0x00`+`0x65` pair), handled by
  resync byte by byte and counted (`resync_bytes`).

These live in `protocol/drsp.c`, `data/lifelog_record.c`,
`protocol/sample_capture.c`, and are asserted by `tests/native/` (186 checks).

**Still not settled:**

- The **meaning** of the two int32 of record `0x00`. Values such as 62927 /
  624 / sequence 1263 are stored, compared and exported, but no field is named
  `steps` or `distance` because nothing measures that mapping.
- Cadence ("60 s dominant"), the `u16` counter position, the `0x65` body
  (99 bytes, "97 % constant"), the `0x3D` forms: these are observations of a
  corpus, not verified semantics. No decoder relies on them.
- **No LifeLog transfer from the official app to a DW-H5600 succeeded in the
  corpus.** So there is no end-to-end validation of the whole sequence, only of
  fragments.

## Emulator findings are not this project

Section 3 describes an **emulator** (a separate project, `DW-H5600-EMU`). Facts
like "emulator serves it" say what that emulator does, not what the watch does.
They are kept because they explain the corpus, but they must not be cited as
watch behaviour.

## Architecture

"RPi companion = GATT central" is outdated: the RPi was rejected. The central
is `companion/watchlink.py` on any host with a radio. On Momentum, the FAP
cannot be a central at all — verified against `api_symbols.csv`.

## Open items T-B … T-I

All still open. None of them can be closed from a Windows host without a
Bluetooth adapter or the watch; they are listed as blocked, not as done.