# lifelog_record.h

**Path:** `data/lifelog_record.h`

## Content (real state, 2026-11-18)

Type `0x00` LifeLog record (dated activity record).

```c
#define LIFELOG_RECORD00_LEN 15u

typedef struct {
    int year, month, day, hour, minute;
    int32_t value_a;   /* body[5..8],  semantics UNKNOWN */
    int32_t value_b;   /* body[9..12], semantics UNKNOWN */
    uint16_t sequence;
} CasioLifeLogRecord;

int  lifelog_record_decode(const uint8_t* body, size_t len, CasioLifeLogRecord* out);
void lifelog_record_format(const CasioLifeLogRecord* rec, char* out, size_t out_cap);
```

## Layout and provenance

Measured on capture -4 (2026-10-26 22:18:40), body after the 5-byte record header:

```
1a 0a 1a 16 12 | cf f5 00 00 | 70 02 00 00 | ef 04
-- date ------   -- value_a     -- value_b   sequence
```

- `[0]` year − 2000, **binary** (0x1a = 26 → 2026), not BCD
- `[1]` month 1..12, `[2]` day 1..31, `[3]` hour 0..23, `[4]` minute 0..59
- `[5..8]` int32 LE = 62927, `[9..12]` int32 LE = 624: **semantics unknown**
- `[13..14]` uint16 LE = 1263: increments by one per record in a run

Second measured sample (capture 2): `2026-08-29 22:33`, value_a 62892,
value_b 0, sequence 1391.

## Known limits

- `value_a` / `value_b` are decoded but NOT interpreted. No capture correlates
  them with steps, calories or heart rate, so they stay raw on purpose.
- `lifelog_record_decode` rejects impossible dates (month/day/hour/minute) and
  returns −2..−5; it does not validate days per month (no leap-year check).
- The record carries no year century: 2000 is added unconditionally.

## Status

- [x] Implemented
- [x] Tested — `tests/native/test_drsp.c` decodes the measured body and checks
      every field, including the formatted `2026-10-26 22:18`

## Errors

_none yet_

## Corrections

_none yet_
