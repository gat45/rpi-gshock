#pragma once
#include <stdint.h>
#include <stddef.h>

/*
 * Type 0x00 LifeLog record ("activity record" anchored on a date).
 *
 * Body layout, measured on the real DW-H5600 corpus and confirmed by the DEX
 * disassembly (docs/LIFELOG_ANALYSIS.md, L12):
 *
 *   [0]     year - 2000      binary, not BCD
 *   [1]     month            1..12
 *   [2]     day              1..31
 *   [3]     hour             0..23
 *   [4]     minute           0..59
 *   [5..8]  int32 LE         semantics UNKNOWN (L13)
 *   [9..12] int32 LE         semantics UNKNOWN (L13)
 *   [13..14] uint16 LE       sequence, +1 per record emitted within a run
 *
 * Measured sample (capture -4, 2026-10-26 22:18:40):
 *   1a 0a 1a 16 12 | cf f5 00 00 | 70 02 00 00 | ef 04
 *   -> 2026-10-26 22:18, 62927, 624, 1263
 */

#define LIFELOG_RECORD00_LEN 15u

typedef struct {
    int year; /* full year (2000 + raw byte) */
    int month;
    int day;
    int hour;
    int minute;
    int32_t value_a; /* body[5..8], semantics UNKNOWN */
    int32_t value_b; /* body[9..12], semantics UNKNOWN */
    uint16_t sequence;
} CasioLifeLogRecord;

/* Returns 0 on success, negative on invalid body. */
int lifelog_record_decode(const uint8_t* body, size_t len, CasioLifeLogRecord* out);

/* "YYYY-MM-DD HH:MM" into out (needs >= 17 bytes). */
void lifelog_record_format(const CasioLifeLogRecord* rec, char* out, size_t out_cap);
