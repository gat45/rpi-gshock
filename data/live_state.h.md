# live_state.h

**Path:** `data/live_state.h`

## Content (real state, 2026-11-19)

Parser for the file the companion writes on the SD card. Pure C, no Flipper
dependency, so it runs in the FAP and in the host tests.

```c
#define LIVE_NAME_MAX 24
#define LIVE_RECORD_MAX 20

typedef struct {
    uint32_t version;        /* 0 = untrusted */
    int parsed;
    char name[LIVE_NAME_MAX];
    int name_valid;
    int batt_raw, batt_pct, temp_c, batt_valid;
    char record[LIVE_RECORD_MAX];
    int record_valid;
    int record_year, record_month, record_day, record_hour, record_minute;
    int record_fields_valid;
    int value_a, value_b;
    unsigned seq;
    int metrics_valid;
} CasioLiveState;

void live_state_reset(CasioLiveState* out);
int  live_state_parse(const char* text, size_t len, CasioLiveState* out);
void live_state_summary(const CasioLiveState* st, char* out, size_t out_cap);
```

## Why a flat file

The FAP cannot be a BLE central on Momentum (no GAP scan / GATT client in the
exported API). The central lives on a host (`companion/watchlink.py`), decodes
the watch answers, and pushes this file. A flat `key=value` format needs no
JSON parser and no allocation on the device.

Format, one key per line, LF or CRLF, unknown keys ignored:

```
v=1
name=CASIO DW-H5600
batt_raw=7
batt_pct=0
temp_c=27
record=2026-10-26 22:18
value_a=62927
value_b=624
seq=1263
```

## Append-safe by construction

The Flipper CLI command used to push the file, `storage write_chunk`,
**appends**; it does not truncate. A second push therefore leaves two documents
in the file. Each `v=` line starts a new document and discards everything parsed
before it, so the **last** document always wins. A malformed `v=` line
invalidates that document instead of resurrecting the previous one, and on
failure the whole state is cleared so a caller ignoring the return value cannot
read a half-parsed document.

## Known limits

- `v=` must exist and be > 0, otherwise `live_state_parse` returns −1.
- Integers are parsed manually; anything non-numeric is ignored rather than
  turned into 0 (`batt_raw=abc` leaves −1).
- `record=` is trusted as a date only when it is exactly `YYYY-MM-DD HH:MM`
  with valid ranges; otherwise the text is kept but the numeric fields are not.
- `live_state_summary` truncates through `snprintf`; no overflow.

## Status

- [x] Implemented
- [x] Tested — `tests/native/test_live.c`: the exact 56 bytes that were on the
      card, CRLF, missing version, unknown keys, garbage values, impossible
      record, negative metrics, appended documents, truncated input

## Errors

_none yet_

## Corrections

_none yet_
