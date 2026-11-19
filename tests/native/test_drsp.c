#include "casiolab_test.h"

#include "protocol/drsp.h"
#include "protocol/sample_capture.h"
#include "protocol/sample_data.h"
#include "data/lifelog_record.h"

/*
 * kF06B0 is the first 0x00 LifeLog chunk of capture -4, copied byte for byte
 * from the emulator's measured table (DW-H5600-EMU/v8/src/lifelog_frames.h:37),
 * itself generated from the capture. It carries the record dated
 * 2026-10-26 22:18.
 *
 *   05 | ff f0 ff ff ff e5 f5 e5 e9 ed 30 0a ff ff 8f fd ff ff 10 fb | 44 2c
 *   ^    ^ payload = complement of 00 0f 00 00 00 1a 0a 1a 16 12 cf f5 ..    ^ CRC LE
 *   opcode                                                                    crc = 0x2c44
 */
static const uint8_t kF06B0[23] = {
    0x05, 0xFF, 0xF0, 0xFF, 0xFF, 0xFF, 0xE5, 0xF5, 0xE5, 0xE9, 0xED, 0x30,
    0x0A, 0xFF, 0xFF, 0x8F, 0xFD, 0xFF, 0xFF, 0x10, 0xFB, 0x44, 0x2C,
};

static const uint8_t kExpectedPayload[20] = {
    0x00, 0x0F, 0x00, 0x00, 0x00, 0x1A, 0x0A, 0x1A, 0x16, 0x12,
    0xCF, 0xF5, 0x00, 0x00, 0x70, 0x02, 0x00, 0x00, 0xEF, 0x04,
};

void test_drsp(void) {
    /* Published CRC-16/KERMIT check value. */
    CK_INT(drsp_crc16_kermit((const uint8_t*)"123456789", 9), 0x2189);

    /* The CRC really is in the frame, little-endian. */
    CK_INT(drsp_crc16_kermit(kF06B0 + 1, sizeof(kF06B0) - 3), 0x2C44);

    uint8_t out[256];
    size_t n = 0;
    DrspChunkKind kind = DrspChunkContinuation;

    CK_INT(drsp_chunk_unwrap(kF06B0, sizeof(kF06B0), &kind, out, sizeof(out), &n), DrspOk);
    CK_INT(kind, DrspChunkCrc);
    CK_INT(n, 20);
    CK_BYTES(out, kExpectedPayload, 20);

    /* Bad opcode and short frames are rejected, never silently accepted. */
    uint8_t bad[23];
    memcpy(bad, kF06B0, sizeof(bad));
    bad[0] = 0x04;
    CK_INT(drsp_chunk_unwrap(bad, sizeof(bad), &kind, out, sizeof(out), &n), DrspErrOpcode);
    CK_INT(drsp_chunk_unwrap(kF06B0, 1, &kind, out, sizeof(out), &n), DrspErrShort);

    /* A frame whose trailing CRC does not match is treated as an MTU slice. */
    memcpy(bad, kF06B0, sizeof(bad));
    bad[sizeof(bad) - 1] ^= 0x01;
    CK_INT(drsp_chunk_unwrap(bad, sizeof(bad), &kind, out, sizeof(out), &n), DrspOk);
    CK_INT(kind, DrspChunkContinuation);
    CK_INT(n, 22);

    /* Record decode of the measured body. */
    CasioLifeLogRecord rec;
    memset(&rec, 0, sizeof(rec));
    CK_INT(lifelog_record_decode(kExpectedPayload + DRSP_RECORD_HEADER_SIZE, 15, &rec), 0);
    CK_INT(rec.year, 2026);
    CK_INT(rec.month, 10);
    CK_INT(rec.day, 26);
    CK_INT(rec.hour, 22);
    CK_INT(rec.minute, 18);
    CK_INT(rec.value_a, 62927);
    CK_INT(rec.value_b, 624);
    CK_INT(rec.sequence, 1263);

    char ts[24];
    lifelog_record_format(&rec, ts, sizeof(ts));
    CK_STR(ts, "2026-10-26 22:18");

    /* The real generated sample must yield exactly the documented pair. */
    CasioSampleResult sample;
    sample_capture_run(&sample);
    CK_INT(sample.chunks, 3);
    CK_INT(sample.crc_chunks, 1);
    CK_INT(sample.continuation_chunks, 2);
    CK_INT(sample.bytes_in >= CASIO_SAMPLE_FLOW_BYTES, 1);
    CK_INT(sample.records, 2);
    CK_INT(sample.records_type00, 1);
    CK_INT(sample.records_type65, 1);
    CK_INT(sample.record_valid, 1);
    CK_INT(sample.record.month >= 1 && sample.record.month <= 12, 1);
    CK_INT(sample.record.day >= 1 && sample.record.day <= 31, 1);
    CK_INT(sample.record.year >= 2020 && sample.record.year <= 2030, 1);

    /* Resynchronisation: junk bytes are dropped one at a time and counted. */
    DrspStream stream;
    drsp_stream_reset(&stream);
    const uint8_t junk[3] = {0xAA, 0x55, 0x01};
    drsp_stream_feed(&stream, junk, sizeof(junk));
    drsp_stream_feed(&stream, kExpectedPayload, sizeof(kExpectedPayload));

    DrspRecord r;
    CK_INT(drsp_stream_next(&stream, &r), 1);
    CK_INT(stream.resync_bytes, 3);
    CK_INT(r.type, DRSP_TYPE_ACTIVITY);
    CK_INT(r.length, 15);
    drsp_stream_consume(&stream, &r);
    CK_INT(drsp_stream_next(&stream, &r), 0);

    /* A truncated record is "need more", not an error and not a record. */
    drsp_stream_reset(&stream);
    drsp_stream_feed(&stream, kExpectedPayload, 10);
    CK_INT(drsp_stream_next(&stream, &r), 0);
    CK_INT(stream.records, 0);
}
