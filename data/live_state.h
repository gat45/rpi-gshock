#pragma once
#include <stdint.h>
#include <stddef.h>

/*
 * Parser for the file the companion writes on the Flipper SD card.
 *
 * Why a flat key=value file
 * -------------------------
 * The FAP cannot be a BLE central on Momentum (no GAP scan / GATT client in the
 * exported API). The central therefore runs on a host (companion/watchlink.py),
 * decodes the watch answers, and pushes a tiny text file to
 *
 *     /ext/casiolab/live.txt
 *
 * A flat format keeps the on-device reader ~100 lines with no JSON parser, no
 * allocation and no Flipper dependency, so it is unit-tested on the host
 * (tests/native/test_live.c) with the exact bytes that were written to the card.
 *
 * Format (one `key=value` per line, LF or CRLF, unknown keys ignored):
 *
 *     v=1                  schema version, required
 *     name=CASIO DW-H5600  watch name from command 0x23
 *     batt_raw=7           step index from command 0x28 (measured)
 *     batt_pct=0           percent (derived by the companion)
 *     temp_c=27            temperature, degrees Celsius (measured)
 *     record=2026-10-26 22:18   optional, from a type 0x00 LifeLog record
 *     value_a=62927        optional, semantics UNKNOWN
 *     value_b=624          optional, semantics UNKNOWN
 *     seq=1263             optional, record sequence
 *
 * Append-safe by construction: the Flipper CLI command used to push the file
 * (`storage write_chunk`) APPENDS to whatever is already there - it does not
 * truncate. A push therefore leaves several documents in the file. Each `v=`
 * line starts a new document and discards everything parsed before it, so the
 * last complete document always wins. live_file_read() reads the tail of the
 * file for the same reason.
 *
 * Anything else is ignored on purpose: the companion may grow new keys without
 * breaking an older FAP.
 */

#define LIVE_NAME_MAX 24
#define LIVE_RECORD_MAX 20

typedef struct {
    uint32_t version; /* 0 = the file had no v= line, so it is not trusted */
    int parsed; /* 1 when at least one known key was understood */

    char name[LIVE_NAME_MAX];
    int name_valid;

    int batt_raw; /* -1 when absent */
    int batt_pct; /* -1 when absent */
    int temp_c;
    int temp_valid; /* 1 when temp_c= was present and in range */
    int batt_valid;

    char record[LIVE_RECORD_MAX]; /* "YYYY-MM-DD HH:MM" or empty */
    int record_valid;
    int record_year, record_month, record_day, record_hour, record_minute;
    int record_fields_valid; /* 1 only when `record` had the documented shape */

    int value_a;
    int value_b;
    unsigned seq;
    int metrics_valid;
} CasioLiveState;

/* Reset to the empty state (all flags 0, batt_raw -1). */
void live_state_reset(CasioLiveState* out);

/*
 * Parse `text` (not required to be NUL-terminated, `len` is authoritative).
 * Returns 0 when a trusted file was parsed, -1 when the version line is missing
 * or does not start with 'v', -2 on invalid arguments.
 * A valid version with no known key still returns 0 but leaves parsed = 0.
 */
int live_state_parse(const char* text, size_t len, CasioLiveState* out);

/* Human-readable one-liner for the UI: "live v1 name=... raw=...". */
void live_state_summary(const CasioLiveState* state, char* out, size_t out_cap);
