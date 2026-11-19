#pragma once
#include <stdint.h>
#include <stddef.h>

/*
 * DRSP / LifeLog framing for the Casio DW-H5600.
 *
 * Everything below is derived from the btsnoop captures in
 * DW-H5600-EMU/analysis/btsnoop (5 sessions, 2026-08-21 .. 2026-10-26)
 * and cross-checked against the CASIO WATCHES DEX (see docs/LIFELOG_ANALYSIS.md).
 *
 * Wire chunk (one BLE notification on handle 0x0013):
 *
 *     [0x05] [ payload, complemented byte-wise ] [ crc16_kermit(payload), LE ]
 *
 *   - The last two bytes are the CRC of the payload, stored little-endian.
 *     Verified on 36/408 chunks of capture -4.
 *   - The 372 remaining chunks are *continuations*: they carry no CRC (the
 *     notification is a slice of a frame larger than the MTU). For those the
 *     whole chunk after the opcode is payload.
 *   - The payload is complemented (XOR 0xFF), i.e. 0x00 -> 0xFF.
 *
 * Payload stream = concatenation of every unwrapped chunk. It is a sequence of
 *
 *     [type:8] [length:32 LE] [body:length]
 *
 * Boundaries are NOT aligned on notifications: a record can start in one
 * notification and end in the next.
 *
 * Resynchronisation: the corpus shows a real desync at stream offset 124
 * (after one 0x00 + one 0x65 record). The stream therefore resyncs by dropping
 * one byte at a time until a plausible (type, length) pair is found, and
 * counts every drop.
 */

#define DRSP_CHUNK_OPCODE 0x05u
#define DRSP_RECORD_HEADER_SIZE 5u
#define DRSP_STREAM_MAX 8192u

/* Record types observed in the corpus. */
#define DRSP_TYPE_ACTIVITY 0x00u /* 15-byte dated record */
#define DRSP_TYPE_METRIC_A 0x50u
#define DRSP_TYPE_METRIC_B 0x51u
#define DRSP_TYPE_METRIC_C 0x52u
#define DRSP_TYPE_METRIC_D 0x56u
#define DRSP_TYPE_BLOB 0x65u /* 99 bytes, no date of its own */
#define DRSP_TYPE_METRIC_E 0x72u

/* Command ids (first byte of a frame on the 0x0016/0x0018 channel). */
#define DRSP_CMD_CURRENT_TIME 0x09u
#define DRSP_CMD_ALERT 0x0Au
#define DRSP_CMD_BLE_FEATURES 0x10u
#define DRSP_CMD_TIMER 0x18u
#define DRSP_CMD_DST_STATE 0x1Du
#define DRSP_CMD_DST_SETTING 0x1Eu
#define DRSP_CMD_WORLD_CITIES 0x1Fu
#define DRSP_CMD_VERSION 0x20u
#define DRSP_CMD_APP_INFO 0x22u
#define DRSP_CMD_WATCH_NAME 0x23u
#define DRSP_CMD_HOME_TIME 0x24u
#define DRSP_CMD_ACTIVITY 0x26u
#define DRSP_CMD_CONDITION 0x28u
#define DRSP_CMD_SERVICE_DISCOVERY 0x47u
#define DRSP_CMD_ERROR 0xFFu

typedef enum {
    DrspOk = 0,
    DrspErrShort, /* fewer bytes than the format needs */
    DrspErrOpcode, /* chunk does not start with 0x05 */
    DrspErrCapacity, /* output buffer too small */
} DrspStatus;

typedef enum {
    DrspChunkCrc = 0, /* payload followed by a valid CRC16 */
    DrspChunkContinuation, /* MTU slice, no CRC */
} DrspChunkKind;

typedef struct {
    uint8_t type;
    uint16_t length;
    const uint8_t* body; /* points into DrspStream.buf */
} DrspRecord;

typedef struct {
    uint8_t buf[DRSP_STREAM_MAX];
    size_t len;
    uint32_t chunks;
    uint32_t crc_chunks;
    uint32_t continuation_chunks;
    uint32_t resync_bytes; /* bytes dropped to recover framing */
    uint32_t unknown_records; /* framed records of an unlisted type, skipped whole */
    uint32_t unknown_bytes; /* bytes skipped that way (not counted as resync) */
    uint32_t dropped_bytes; /* bytes refused by drsp_stream_feed (buffer full) */
    uint32_t records;
} DrspStream;

/* CRC-16/KERMIT (reflected 0x1021, poly 0x8408, init 0). */
uint16_t drsp_crc16_kermit(const uint8_t* data, size_t len);

/*
 * Unwrap one BLE notification.
 * Returns DrspOk and fills kind/out/out_len. `out` holds the decoded payload
 * (complemented back, CRC removed when present).
 */
DrspStatus drsp_chunk_unwrap(
    const uint8_t* wire,
    size_t wire_len,
    DrspChunkKind* kind,
    uint8_t* out,
    size_t out_cap,
    size_t* out_len);

/* Append payload bytes to the record stream. Returns bytes accepted. */
size_t drsp_stream_feed(DrspStream* s, const uint8_t* data, size_t len);

void drsp_stream_reset(DrspStream* s);

/*
 * Try to extract the next record.
 *  1  -> record filled (call drsp_stream_consume() afterwards)
 *  0  -> need more bytes (feed another chunk)
 * Desync repair (byte drops, unknown-type skips) is done internally and is
 * visible only through the counters; this function never returns -1.
 *
 * rec->body stays valid until the caller calls drsp_stream_consume().
 */
int drsp_stream_next(DrspStream* s, DrspRecord* rec);

/* Drop the record last returned by drsp_stream_next(). */
void drsp_stream_consume(DrspStream* s, const DrspRecord* rec);

/* True when (type, length) is a pair measured in the corpus. */
int drsp_record_plausible(uint8_t type, uint16_t length);

const char* drsp_command_name(uint8_t cmd);
const char* drsp_status_name(DrspStatus status);
