# drsp.h

**Path:** `protocol/drsp.h`

## Content (real state, 2026-11-18)

DRSP / LifeLog framing, derived from the five btsnoop captures in
`F:/freebuff/gshock/DW-H5600-EMU/analysis/btsnoop`.

Wire chunk (one BLE notification on handle `0x0013`):

```
[0x05] [ payload XOR 0xFF ] [ crc16_kermit(payload), little-endian ]
```

API:

```c
uint16_t drsp_crc16_kermit(const uint8_t* data, size_t len);
DrspStatus drsp_chunk_unwrap(const uint8_t* wire, size_t wire_len,
                            DrspChunkKind* kind,
                            uint8_t* out, size_t out_cap, size_t* out_len);
size_t drsp_stream_feed(DrspStream* s, const uint8_t* data, size_t len);
void   drsp_stream_reset(DrspStream* s);
int    drsp_stream_next(DrspStream* s, DrspRecord* rec);   /* 1 record, 0 need more */
void   drsp_stream_consume(DrspStream* s, const DrspRecord* rec);
int    drsp_record_plausible(uint8_t type, uint16_t length);
const char* drsp_command_name(uint8_t cmd);
const char* drsp_status_name(DrspStatus status);
```

Record stream after unwrapping: `[type:8][length:32 LE][body:length]`.
Boundaries are **not** aligned with notifications, so `DrspStream` buffers.

## Provenance of the constants

| Constant | How it was obtained |
|---|---|
| `DRSP_CHUNK_OPCODE 0x05` | Every notification on `0x0013` starts with `05` in all 5 captures |
| Complement `XOR 0xFF` | `05 ff f0 ff ff ff e5 f5 …` decodes to `00 0f 00 00 00 1a 0a …` |
| CRC16-KERMIT, LE | `drsp_crc16_kermit` on `kF06B0[1..-2]` = `0x2C44`, wire bytes `44 2c` |
| Type/length pairs | measured: `0x00/15`, `0x65/99`, `0x50/301`, `0x51/301`, `0x52/745`, `0x56/1115`, `0x72/2373` |
| Resync | real desync at stream offset 124 in capture -4 (one `0x00` + one `0x65` record, then garbage) |

## Known limits

- 408 chunks of capture -4: 36 carry a valid CRC (end of frame), 372 are MTU
  slices without CRC. `drsp_chunk_unwrap` decides by testing the CRC; a slice
  whose last two bytes accidentally form a valid CRC would be misread as a
  frame end. Probability 2^-16 per chunk; no occurrence in the corpus.
- `drsp_record_plausible` only accepts the measured pairs. A firmware update
  that changes a record length would make the stream resync (bytes counted in
  `resync_bytes`) instead of decoding.
- `DrspStream.buf` is 8 KiB; the largest measured frame is 2373 bytes, so a
  full frame plus slack fits, but a stream fed without consuming will stop
  accepting bytes (feed returns less than `len`).

## Status

- [x] Implemented
- [x] Tested — `tests/native/test_drsp.c` (CRC, unwrap, both chunk kinds,
      record decode of the measured body, resync, truncated frame)

## Errors

_none yet_

## Corrections

_none yet_
