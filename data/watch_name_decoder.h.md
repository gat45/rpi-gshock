# watch_name_decoder.h

**Path:** `data/watch_name_decoder.h`

## Content (real state, 2026-11-18)

```c
int watch_name_decode(const uint8_t* payload, size_t len, char* out, size_t out_cap);
```

Decodes command `0x23` (watch name): request is the single byte `0x23` on handle
`0x0016`, the answer is notified on `0x0018`.

## Provenance

Measured responses in the corpus:

```
23 "CASIO DW-H5600" 00 00 00 00 00     (5 occurrences)
23 "CASIO GW-B5600" 00 00 00 00 00     (2 occurrences)
```

## Known limits

- The watch does not NUL-terminate when the name fills the characteristic, so
  the decoder also stops on the first non-printable byte.
- Trailing spaces are trimmed (some models pad with `0x20` instead of `0x00`).
- Returns −2 when the first byte is not `0x23`, −3 when nothing printable was
  found. Never returns a partially trusted string on failure.

## Status

- [x] Implemented
- [x] Tested — `tests/native/test_decoders.c` decodes both measured names, a
      name without NUL, a wrong command byte and a 1-byte payload

## Errors

_none yet_

## Corrections

_none yet_
