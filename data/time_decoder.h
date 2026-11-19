#pragma once
#include <stdint.h>
#include <stddef.h>

/*
 * Command 0x09 — current time.
 *
 * Format taken from the reference implementation
 * (gshock_api/iolib/time_io.py, TimeEncoderPure.encode_current_time):
 *
 *   [0]     0x09 command
 *   [1..2]  year, little-endian
 *   [3]     month
 *   [4]     day
 *   [5]     hour
 *   [6]     minute
 *   [7]     second
 *   [8]     weekday, 1 = Monday .. 7 = Sunday (isoweekday)
 *   [9]     sub-second byte (nanoseconds * 256 / 1e9)
 *   [10]    flag, always 0x01
 *
 * STATUS: reference-derived, NOT yet measured on a DW-H5600.
 * The corpus only contains `09 1D ...` frames on handle 0x0010, which are
 * DST-watch-state frames (opcode 0x09 / command 0x1D), not current time.
 * Do not present this decoder as validated until a SET_TIME write from the
 * official app is captured. The measured timestamps available today come from
 * the LifeLog 0x00 records (see data/lifelog_record.h).
 */

/* Returns 0 on success, negative on invalid payload. */
int time_decode(const uint8_t* payload, size_t len,
                int* year, int* month, int* day,
                int* hour, int* minute, int* second);
