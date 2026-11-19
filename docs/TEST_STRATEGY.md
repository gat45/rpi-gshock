# TEST_STRATEGY

## Principle
Every C function ported must be compared to Python/Kotlin reference with identical inputs.

## Test types
1. Encoder tests: input → expected bytes
2. Decoder tests: bytes → expected struct
3. Fragmentation/reassembly tests
4. Length validation (short/long/invalid)
5. Unexpected response handling
6. Regression tests with captured vectors

## Test vectors
- Store in `test_data/` (hex dumps from captures)
- Include `failed_step_payload_20260902_125923.hex` and `step-output.txt`
- Include `h5600_lifelog.json`

## No hardware required
All unit tests run on host (native) via ufbt test or custom harness.

---

# Correction (2026-11-19) — what actually exists

The section above was written in Phase 0 and describes an intention. Some of
it never became true; this section states the measured reality. Nothing above
is deleted.

## What is false above

- **"Every C function ported must be compared to Python/Kotlin reference"** —
  no: the decoders are compared to **real btsnoop captures**, not to the
  Python reference. Where only the reference exists (`0x09` current time) the
  decoder is explicitly marked *not measured* and must not be called validated.
- **"via `ufbt test`"** — `ufbt` is not used for tests. There is a plain `gcc`
  harness in `tests/native/`.
- **"`test_data/`"** — that directory does not exist in this project. The
  vectors live in two **generated headers** (`protocol/sample_data.h`,
  `casio/command_observed.h`) produced from the raw CSVs by the scripts in
  `tools/`.

## Real harness

`tests/native/` — no framework, no build system, no Flipper SDK. Assertions
come from `tests/native/casiolab_test.h` (`CK`, `CK_INT`, `CK_STR`,
`CK_BYTES`); a failing check increments a counter and prints
`FAIL file:line`, it does not abort, so one run shows every failure.

```bash
cd CasioLab
gcc -std=c11 -O1 -Wall -Wextra -I . -o dist/native/t.exe \
  protocol/drsp.c protocol/sample_capture.c data/lifelog_record.c \
  data/watch_name_decoder.c data/battery_decoder.c data/time_decoder.c \
  data/live_state.c data/data_manager.c storage/export.c \
  tests/native/main.c tests/native/test_drsp.c tests/native/test_decoders.c \
  tests/native/test_export.c tests/native/test_live.c
./dist/native/t.exe
```

Last run (`2026-11-19`):

```
CasioLab native tests
---------------------
drsp        39 checks, 0 failures
decoders    44 checks, 0 failures
export      31 checks, 0 failures
live        72 checks, 0 failures
---------------------
186 checks, 0 failures
```

`main.c` prints a per-suite line so a failure is attributable; the exit code is
non-zero if any check failed.

## What each suite covers

| Suite | File | Coverage |
|---|---|---|
| `drsp` | `test_drsp.c` | CRC16-KERMIT check value `0x2189`, frame `0x2C44`, chunk `05` unwrap (CRC and MTU slice), `DrspStream` feed/next/consume, resync counted byte-by-byte, generated sample counts (3 chunks, 305 bytes, 2 records, 1 resync) |
| `decoders` | `test_decoders.c` | record `0x00` (2 measured dates), name `0x23` (`DW-H5600`, `GW-B5600`, no NUL, wrong command), battery `0x28` (raw/temp, injected limits, invalid limits), time `0x09` (reference layout + error cases), dispatch, frame counters, reset |
| `export` | `test_export.c` | CSV header + line, JSON schema `casiolab.session/1`, `provenance` follows content (`measured`/`derived`/`absent`), `record:null`, empty state, brace balance, dead sink refused |
| `live` | `test_live.c` | exact 56 bytes read from the SD card, CRLF, missing version, unknown keys, garbage integers, impossible record, negative metrics, appended documents (last wins), truncated input, `live_file` tail read |

## Measured vs reference — the rule that matters

A test passing proves the code matches the **input it was given**, not that the
input is real. So every suite carries its provenance:

- **measured**: record `0x00`, name `0x23`, battery `0x28`, DRSP framing. The
  vectors are raw bytes from the 5 btsnoop captures.
- **reference-derived, not measured**: `0x09` current time. Tested against a
  frame built from the reference layout; must stay labelled unverified.
- **not implemented, not tested**: heart rate, sleep, workout, activity,
  unknown decoders. No measurement exists, so no decoder is written.

## On-hardware checks (not automatable)

These were run manually and are the only proof the FAP works with real SD and
GUI code:

- `ufbt launch` → app starts, `loader info` reports it running.
- Menu navigation, BACK returns to the menu then exits.
- DATA / STORAGE with a live document pushed from capture -4 → `CASIO DW-H5600`,
  raw 7, 27 °C, `record: null`, `frames: 0`.
- DATA / STORAGE with an invalid last document → sample fallback, record
  present, `frames: 6`.
- `/ext/casiolab/sessions/session_0001..0013/` created with
  `export_packets.csv` and `export_decoded.json`.

## Not covered anywhere (honest gaps)

- Any module that needs the Flipper `Storage`/GUI API is not compiled on the
  host: `session_manager`, `history_db`, `export_csv`, `export_json`,
  `live_file` (only its parser `live_state` is tested), `ui/main_menu`.
- No fuzzing. A malformed chunk larger than the stream buffer is handled by
  resync, but not exhaustively.
- The companion's `BleTransport` is untested (no Bluetooth adapter on the
  build host); only `ReplayTransport` is verified against real captures.
- No test asserts the `command_catalog` safety policy.
