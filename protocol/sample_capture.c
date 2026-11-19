#include "sample_capture.h"

#include <string.h>

#include "drsp.h"
#include "sample_data.h" /* generated from the captures, do not edit */
#include "data/battery_decoder.h"
#include "data/watch_name_decoder.h"

void sample_capture_run(CasioSampleResult* out) {
    if(!out) return;
    memset(out, 0, sizeof(*out));
    out->battery_raw = -1;

    static DrspStream stream;
    drsp_stream_reset(&stream);

    static uint8_t payload[512]; /* >= largest ATT notification; static: 4 KB app stack */

    for(uint32_t i = 0; i < CASIO_SAMPLE_FLOW_COUNT; i++) {
        const CasioSampleFrame* f = &kSampleFlowChunks[i];
        DrspChunkKind kind = DrspChunkContinuation;
        size_t n = 0;
        out->chunks++;
        if(drsp_chunk_unwrap(f->data, f->len, &kind, payload, sizeof(payload), &n) != DrspOk) {
            out->dropped_chunks++;
            continue;
        }
        if(kind == DrspChunkCrc)
            out->crc_chunks++;
        else
            out->continuation_chunks++;
        out->bytes_in += (uint32_t)n;
        drsp_stream_feed(&stream, payload, n);
    }

    DrspRecord rec;
    int guard = 0;
    while(drsp_stream_next(&stream, &rec) == 1 && guard++ < 64) {
        out->records++;
        if(rec.type == DRSP_TYPE_ACTIVITY) {
            out->records_type00++;
            if(!out->record_valid &&
               lifelog_record_decode(rec.body, rec.length, &out->record) == 0) {
                out->record_valid = 1;
            }
        } else if(rec.type == DRSP_TYPE_BLOB) {
            out->records_type65++;
        }
        drsp_stream_consume(&stream, &rec);
    }
    out->resync_bytes = stream.resync_bytes;
    out->dropped_bytes = stream.dropped_bytes;
    out->unknown_records = stream.unknown_records;

    for(uint32_t i = 0; i < CASIO_SAMPLE_NAME_COUNT; i++) {
        const CasioSampleFrame* f = &kSampleNames[i];
        if(watch_name_decode(f->data, f->len, out->name, sizeof(out->name)) == 0) {
            out->name_valid = 1;
        }
    }

    if(CASIO_SAMPLE_CONDITION_COUNT > 0) {
        const CasioSampleFrame* f = &kSampleConditions[0];
        CasioBattery b;
        if(battery_decode_full(
               f->data, f->len, CASIO_BATT_LOWER_GENERIC, CASIO_BATT_UPPER_GENERIC, &b) == 0) {
            out->battery_raw = b.raw_level;
            out->temperature_c = b.temperature_c;
            out->battery_pct = b.percent;
            out->condition_valid = 1;
        }
    }
}
