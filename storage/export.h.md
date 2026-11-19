# export.h

**Path:** `storage/export.h`

## Content (real state, 2026-11-18)

Serialisation of a decoded session, deliberately independent of the Flipper
storage API so it can be compiled and tested on the host.

```c
typedef bool (*CasioSinkWrite)(void* ctx, const char* data, size_t len);

typedef struct {
    void* ctx;
    CasioSinkWrite write;
} CasioSink;

bool export_csv_session(const CasioSink* sink, const CasioData* data);
bool export_json_session(const CasioSink* sink, const CasioData* data);
```

The Flipper wrappers (`export_csv.c`, `export_json.c`) plug a `File*` into the
sink; `tests/native/test_export.c` plugs a memory buffer.

## Output

CSV — one header line + one data line:

```
timestamp,watch_name,battery_raw,battery_pct,temperature_c,record_time,value_a,value_b,sequence,frames_decoded,frames_rejected
```

JSON — one object, schema `casiolab.session/1`, with a `provenance` block that
follows the actual content:

```json
"provenance":{"battery_raw":"measured","temperature_c":"measured",
              "battery_pct":"derived","record":"absent",
              "current_time":"reference-layout-unverified"}
```

`absent` means the field is not in the document (`record` is `null`), so a
consumer can never mistake an empty value for a measured one.

## Known limits

- The timestamp prefers a LifeLog record; it falls back to the `0x09` current
  time, whose layout is reference-derived and **not measured** on a DW-H5600.
- No escaping is performed on `watch_name` in JSON. The decoder only lets
  printable ASCII through, so a quote or backslash cannot reach the output;
  if that ever changes, escaping must be added here.
- The CSV has no quoted fields for the same reason.

## Status

- [x] Implemented
- [x] Tested — `tests/native/test_export.c`: both formats, provenance fields,
      null record, empty state, brace balance of the JSON, NULL sink refused

## Errors

_none yet_

## Corrections

_none yet_
