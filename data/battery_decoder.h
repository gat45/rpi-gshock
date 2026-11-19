#pragma once
#include <stdint.h>
#include <stddef.h>

/*
 * Command 0x28 — watch condition (battery + temperature).
 *
 * Request  : single byte 0x28 written to handle 0x0016
 * Response : 0x28 [raw_level] [temperature_c] ... notified on handle 0x0018
 *
 * Measured responses (corpus, DW-H5600, 39 occurrences):
 *   28 06 1A 00 00 FA 00 00 00 00     raw 6, 26 C
 *   28 07 1B 00 00 FA 00 00 00 00     raw 7, 27 C
 *   28 04 1A 00 00 FA 00 00 00 01 01  raw 4, 26 C
 *
 * `raw_level` is a model-specific step index, not a percentage. The reference
 * clients map it with  percent = (raw - lower) * round(100/(upper-lower)),
 * clamped to 0..100. The (lower, upper) pair for the DW-H5600 is NOT yet
 * confirmed by a capture with a known battery state, so the caller must pass
 * it explicitly. battery_decode() keeps the historical signature and uses the
 * generic G-Shock pair (9, 19) of the reference implementation.
 *
 * Everything here is measured except the percent mapping, which is derived.
 */

#define CASIO_BATT_LOWER_GENERIC 9
#define CASIO_BATT_UPPER_GENERIC 19

typedef struct {
    int raw_level; /* measured step index */
    int temperature_c; /* measured, degrees Celsius */
    int percent; /* derived from raw_level and the limits */
} CasioBattery;

/* Full decode. Returns 0 on success, negative on invalid frame. */
int battery_decode_full(
    const uint8_t* payload,
    size_t len,
    int lower_limit,
    int upper_limit,
    CasioBattery* out);

/* Historical API: battery percentage using the generic limits, or -1. */
int battery_decode(const uint8_t* payload, size_t len);

/*
 * Same mapping as battery_decode_full(), for a raw level that did not come from
 * a 0x28 frame (e.g. a value pushed by the companion). Returns 0..100, or -1
 * when upper <= lower.
 */
int battery_percent_from_raw(int raw_level, int lower_limit, int upper_limit);
