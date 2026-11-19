#pragma once
#include <stdint.h>

#include "data/lifelog_record.h"

/*
 * Offline decode of real captured frames.
 *
 * The FAP cannot be a BLE central on Momentum (no GAP scan / GATT client in the
 * exported API), so the only way to prove the decoders on the device is to run
 * them over bytes that were captured from the real watch. sample_capture.c does
 * exactly that with protocol/sample_capture.h, which is generated from the
 * btsnoop corpus by tools/gen_sample_capture.py.
 *
 * tests/native/test_decoders.c asserts the same numbers on the host.
 */

typedef struct {
    uint32_t chunks;
    uint32_t crc_chunks;
    uint32_t continuation_chunks;
    uint32_t bytes_in;
    uint32_t records;
    uint32_t records_type00;
    uint32_t records_type65;
    uint32_t resync_bytes;
    uint32_t dropped_chunks; /* chunks rejected by drsp_chunk_unwrap */
    uint32_t dropped_bytes; /* bytes refused by the stream buffer */
    uint32_t unknown_records;

    int record_valid;
    CasioLifeLogRecord record;

    char name[24];
    int name_valid;

    int battery_raw;
    int temperature_c;
    int battery_pct;
    int condition_valid;
} CasioSampleResult;

/* Decode the embedded capture samples. Always fills out. */
void sample_capture_run(CasioSampleResult* out);
