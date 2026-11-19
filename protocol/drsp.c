#include "drsp.h"

#include <string.h>

uint16_t drsp_crc16_kermit(const uint8_t* data, size_t len) {
    uint16_t crc = 0;
    for(size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for(uint8_t bit = 0; bit < 8; bit++) {
            if(crc & 1u)
                crc = (uint16_t)((crc >> 1) ^ 0x8408u);
            else
                crc >>= 1;
        }
    }
    return crc;
}

DrspStatus drsp_chunk_unwrap(
    const uint8_t* wire,
    size_t wire_len,
    DrspChunkKind* kind,
    uint8_t* out,
    size_t out_cap,
    size_t* out_len) {
    if(!wire || wire_len < 2) return DrspErrShort;
    if(wire[0] != DRSP_CHUNK_OPCODE) return DrspErrOpcode;
    if(!out || !out_len) return DrspErrShort; /* invalid output arguments */

    size_t payload_len;
    DrspChunkKind k = DrspChunkContinuation;

    if(wire_len >= 4) {
        const uint16_t expect = (uint16_t)(wire[wire_len - 2] | (wire[wire_len - 1] << 8));
        if(drsp_crc16_kermit(wire + 1, wire_len - 3) == expect) {
            k = DrspChunkCrc;
            payload_len = wire_len - 3; /* drop opcode and CRC */
        } else {
            payload_len = wire_len - 1; /* MTU slice, no CRC */
        }
    } else {
        payload_len = wire_len - 1;
    }

    if(payload_len > out_cap) return DrspErrCapacity;

    for(size_t i = 0; i < payload_len; i++) {
        out[i] = (uint8_t)~wire[i + 1];
    }

    if(kind) *kind = k;
    *out_len = payload_len;
    return DrspOk;
}

size_t drsp_stream_feed(DrspStream* s, const uint8_t* data, size_t len) {
    if(!s || !data) return 0;
    const size_t requested = len;
    if(len > sizeof(s->buf) - s->len) len = sizeof(s->buf) - s->len;
    s->dropped_bytes += (uint32_t)(requested - len);
    memcpy(s->buf + s->len, data, len);
    s->len += len;
    return len;
}

void drsp_stream_reset(DrspStream* s) {
    if(!s) return;
    s->len = 0;
    s->chunks = 0;
    s->crc_chunks = 0;
    s->continuation_chunks = 0;
    s->resync_bytes = 0;
    s->unknown_records = 0;
    s->unknown_bytes = 0;
    s->dropped_bytes = 0;
    s->records = 0;
}

/*
 * Lengths measured in the corpus (DW-H5600-EMU, constat 53 / 54):
 *
 *   type   announced length   blocks
 *   0x00         15             1
 *   0x65         99             1
 *   0x50        301             2
 *   0x51        301             2
 *   0x52        745             4
 *   0x56       1115             6
 *   0x72       2373            many
 *
 * A header is only accepted when both the type and the length match a measured
 * pair. That is what makes resynchronisation after a desync deterministic.
 */
int drsp_record_plausible(uint8_t type, uint16_t length) {
    switch(type) {
    case DRSP_TYPE_ACTIVITY:
        return length == 15;
    case DRSP_TYPE_BLOB:
        return length == 99;
    case DRSP_TYPE_METRIC_A:
    case DRSP_TYPE_METRIC_B:
        return length == 301;
    case DRSP_TYPE_METRIC_C:
        return length == 745;
    case DRSP_TYPE_METRIC_D:
        return length == 1115;
    case DRSP_TYPE_METRIC_E:
        return length == 2373;
    default:
        return 0;
    }
}

static void stream_drop(DrspStream* s, size_t count) {
    if(count >= s->len) {
        s->len = 0;
        return;
    }
    memmove(s->buf, s->buf + count, s->len - count);
    s->len -= count;
}

int drsp_stream_next(DrspStream* s, DrspRecord* rec) {
    if(!s || !rec) return 0;

    while(s->len >= DRSP_RECORD_HEADER_SIZE) {
        const uint8_t type = s->buf[0];
        const uint16_t length = (uint16_t)(s->buf[1] | (s->buf[2] << 8));
        const uint16_t length_hi = (uint16_t)(s->buf[3] | (s->buf[4] << 8));

        /* A header of an unlisted type that is still well framed (metric-like
         * type nibble 0x5_/0x6_/0x7_, 32-bit length that fits the buffer) is
         * skipped whole once complete: dropping it byte by byte would scan its
         * body for phantom headers. Assumption, not measured: types outside the
         * corpus keep the [type][len32][body] framing. */
        if(length_hi == 0 && !drsp_record_plausible(type, length) && length > 0 &&
           (type >= 0x50 && type <= 0x7F) &&
           DRSP_RECORD_HEADER_SIZE + (size_t)length <= DRSP_STREAM_MAX) {
            const size_t total_unknown = DRSP_RECORD_HEADER_SIZE + (size_t)length;
            if(total_unknown > s->len) return 0; /* wait for the whole body */
            stream_drop(s, total_unknown);
            s->unknown_records++;
            s->unknown_bytes += (uint32_t)total_unknown;
            continue;
        }

        if(length_hi != 0 || !drsp_record_plausible(type, length)) {
            stream_drop(s, 1);
            s->resync_bytes++;
            continue;
        }

        const size_t total = DRSP_RECORD_HEADER_SIZE + (size_t)length;
        if(total > s->len) return 0; /* need more bytes */

        rec->type = type;
        rec->length = length;
        rec->body = s->buf + DRSP_RECORD_HEADER_SIZE;
        s->records++;

        /* rec->body points into s->buf and stays valid until the caller calls
         * drsp_stream_consume(), which drops the record. */
        return 1;
    }
    return 0;
}

/*
 * Consume the record returned by the previous drsp_stream_next() call.
 * Split from next() so the caller can read body[] without an extra copy.
 */
void drsp_stream_consume(DrspStream* s, const DrspRecord* rec) {
    if(!s || !rec) return;
    const size_t total = DRSP_RECORD_HEADER_SIZE + (size_t)rec->length;
    if(total <= s->len) stream_drop(s, total);
}

const char* drsp_command_name(uint8_t cmd) {
    switch(cmd) {
    case DRSP_CMD_CURRENT_TIME:
        return "CURRENT_TIME";
    case DRSP_CMD_ALERT:
        return "ALERT/FIND_PHONE";
    case DRSP_CMD_BLE_FEATURES:
        return "BLE_FEATURES";
    case DRSP_CMD_TIMER:
        return "TIMER";
    case DRSP_CMD_DST_STATE:
        return "DST_WATCH_STATE";
    case DRSP_CMD_DST_SETTING:
        return "DST_SETTING";
    case DRSP_CMD_WORLD_CITIES:
        return "WORLD_CITIES";
    case DRSP_CMD_VERSION:
        return "VERSION";
    case DRSP_CMD_APP_INFO:
        return "APP_INFO";
    case DRSP_CMD_WATCH_NAME:
        return "WATCH_NAME";
    case DRSP_CMD_HOME_TIME:
        return "HOME_TIME";
    case DRSP_CMD_ACTIVITY:
        return "ACTIVITY_RECORD";
    case DRSP_CMD_CONDITION:
        return "WATCH_CONDITION";
    case DRSP_CMD_SERVICE_DISCOVERY:
        return "SERVICE_DISCOVERY";
    case DRSP_CMD_ERROR:
        return "ERROR";
    default:
        return "UNKNOWN";
    }
}

const char* drsp_status_name(DrspStatus status) {
    switch(status) {
    case DrspOk:
        return "ok";
    case DrspErrShort:
        return "short";
    case DrspErrOpcode:
        return "bad-opcode";
    case DrspErrCapacity:
        return "capacity";
    default:
        return "?";
    }
}
