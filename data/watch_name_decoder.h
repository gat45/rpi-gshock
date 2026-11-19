#pragma once
#include <stdint.h>
#include <stddef.h>

/*
 * Command 0x23 — watch name.
 *
 * Request  : single byte 0x23 written to handle 0x0016
 * Response : 0x23 + ASCII name + 0x00 padding, notified on handle 0x0018
 *
 * Measured responses (corpus 2026-08-21 .. 2026-10-26):
 *   23 "CASIO DW-H5600" 00 00 00 00 00     (DW-H5600, 5 occurrences)
 *   23 "CASIO GW-B5600" 00 00 00 00 00     (GW-B5600,  2 occurrences)
 *
 * The name is NOT NUL-terminated by the watch when the payload fills the whole
 * characteristic, so the decoder also stops on the first non-printable byte.
 */

/* Returns 0 on success and writes a NUL-terminated name into out. */
int watch_name_decode(const uint8_t* payload, size_t len, char* out, size_t out_cap);
