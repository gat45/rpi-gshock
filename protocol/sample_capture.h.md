# sample_capture.h

**Path:** `protocol/sample_capture.h`

## Content (real state, 2026-11-18)

Offline decode of real captured frames, so the FAP can prove its decoders on the
device even though it cannot be a BLE central on Momentum.

```c
typedef struct {
    uint32_t chunks, crc_chunks, continuation_chunks, bytes_in;
    uint32_t records, records_type00, records_type65, resync_bytes;
    int record_valid;
    CasioLifeLogRecord record;
    char name[24];
    int name_valid;
    int battery_raw, temperature_c, battery_pct;
    int condition_valid;
} CasioSampleResult;

void sample_capture_run(CasioSampleResult* out);
```

The bytes themselves live in `protocol/sample_data.h`, which is **generated**:

```
python tools/gen_sample_capture.py
```

The generator parses the btsnoop ATT CSVs and keeps the first contiguous run of
notifications that covers a full `0x00` (15 B) + `0x65` (99 B) record pair, i.e.
124 payload bytes. It also keeps two measured watch names (`0x23`, one
GW-B5600 and one DW-H5600) and up to three distinct watch conditions (`0x28`).

## Provenance

- Frames: real notifications, kept byte for byte, CRC included.
- `CASIO_SAMPLE_FLOW_BYTES 124` is the size of the pair, checked by the test.
- 3 chunks (1 with CRC + 2 continuations) currently produce 305 wire bytes.

## Known limits

- This is a capture, not a live watch: it proves the framing and the field
  offsets, nothing about timing or session state.
- The generator silently produces fewer chunks if the corpus changes; the
  native test asserts the exact counts, so a regression fails the build.
- `sample_capture_run()` uses one static `DrspStream`, so it is not reentrant
  and not thread-safe. Fine for the FAP UI, which is single-threaded.

## Status

- [x] Implemented
- [x] Tested — `tests/native/test_drsp.c` asserts chunk counts, byte count,
      record type counts and that the record fields are in valid ranges

## Errors

_none yet_

## Corrections

_none yet_
