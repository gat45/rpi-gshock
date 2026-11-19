# live_file.h

**Path:** `storage/live_file.h`

## Content (real state, 2026-11-19)

```c
#define CASIO_LIVE_PATH "/ext/casiolab/live.txt"
#define CASIO_LIVE_MAX  512u

bool   live_file_load(Storage* storage, CasioLiveState* out);
size_t live_file_read(Storage* storage, char* buf, size_t cap);
```

Reads the file written by `companion/watchlink.py` and parses it with
`data/live_state.h`. A missing file is normal (no companion session yet) and
returns false; the UI then falls back to the embedded capture sample.

## Behaviour that matters

- **Reads the tail, not the head.** Because the push appends, a file that grows
  past the 512-byte window would hide the newest document at the end. When
  `size > cap-1` the file is seeked to `size-(cap-1)` and the first partial line
  is dropped, so the parser never sees a cut key such as `att_raw=7`.
- Trailing `\n`, `\r` and spaces are stripped before parsing (the CLI path can
  leave a CR).
- On parse failure the state is reset, so a caller that ignores the return value
  cannot display stale or half-parsed values.

## Known limits

- The 512-byte window must hold at least one full document (~56 bytes). It does.
- The command does not truncate the file: it grows with every push. That is
  handled by the "last document wins" rule, not by housekeeping. There is no
  delete/rename command in the Flipper CLI to compact it.
- `live_file_load` returns false when the document is valid but contains no
  known key at all (`parsed == 0`).

## Status

- [x] Implemented
- [x] Tested on hardware: DATA and STORAGE read a real pushed document
      (`CASIO DW-H5600`, raw 7, 27 °C, `record:null`) and the fallback to the
      sample was observed with an invalid last document.

## Errors

_none yet_

## Corrections

- First version opened the file and read from offset 0 only. With an appended
  file the newest document was beyond the first 512 bytes and the parser kept
  an older one. Fixed by seeking to the end window (see above).
