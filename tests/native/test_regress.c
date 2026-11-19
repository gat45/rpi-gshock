#include "casiolab_test.h"

#include "data/data_manager.h"
#include "data/lifelog_record.h"
#include "data/live_state.h"
#include "protocol/drsp.h"
#include "storage/export.h"

/* Regression tests for the bugs found by the audit of 2026-11-19. */

typedef struct {
    char buf[2048];
    size_t len;
} Buf;

static bool wr(void* ctx, const char* d, size_t l) {
    Buf* b = ctx;
    if(b->len + l >= sizeof(b->buf)) return false;
    memcpy(b->buf + b->len, d, l);
    b->len += l;
    b->buf[b->len] = '\0';
    return true;
}

static int count_char(const char* s, char c) {
    int n = 0;
    for(; *s; s++) n += (*s == c);
    return n;
}

static void live_fields(void) {
    CasioLiveState st;
    CasioData* d = data_manager_get();

    /* temp_c alone must not be dropped */
    const char t1[] = "v=1\ntemp_c=27\n";
    CK_INT(live_state_parse(t1, sizeof(t1) - 1, &st), 0);
    data_reset();
    CK_INT(data_apply_live(&st), 1);
    CK_INT(d->temp_valid, 1);
    CK_INT(d->temperature_c, 27);

    /* out-of-range values are rejected, not displayed */
    const char t2[] = "v=1\nbatt_raw=7\nbatt_pct=500\ntemp_c=-300\n";
    live_state_parse(t2, sizeof(t2) - 1, &st);
    CK_INT(st.batt_pct, -1);
    CK_INT(st.temp_valid, 0);

    /* seq beyond uint16 is rejected instead of truncated (70000 -> 4464) */
    const char t3[] = "v=1\nrecord=2026-10-26 22:18\nseq=70000\n";
    live_state_parse(t3, sizeof(t3) - 1, &st);
    CK_INT(st.metrics_valid, 0);

    /* calendar: Feb 31 refused, Feb 29 only on leap years */
    const char t4[] = "v=1\nrecord=2026-02-31 12:30\n";
    live_state_parse(t4, sizeof(t4) - 1, &st);
    CK_INT(st.record_fields_valid, 0);
    const char t5[] = "v=1\nrecord=2028-02-29 12:30\n";
    live_state_parse(t5, sizeof(t5) - 1, &st);
    CK_INT(st.record_fields_valid, 1);
    const char t6[] = "v=1\nrecord=2027-02-29 12:30\n";
    live_state_parse(t6, sizeof(t6) - 1, &st);
    CK_INT(st.record_fields_valid, 0);

    uint8_t body[15] = {26, 2, 31, 12, 30, 0};
    CasioLifeLogRecord r;
    CK(lifelog_record_decode(body, sizeof(body), &r) < 0);
    body[2] = 28;
    CK_INT(lifelog_record_decode(body, sizeof(body), &r), 0);
    body[0] = 200; /* year 2200 */
    CK(lifelog_record_decode(body, sizeof(body), &r) < 0);
}

static void export_format(void) {
    CasioLiveState st;
    CasioData* d = data_manager_get();
    Buf b;
    CasioSink sink = {&b, wr};

    /* CSV: comma and quote in the name keep 11 columns once parsed */
    const char t[] = "v=1\nname=A,B\"C\nbatt_raw=7\ntemp_c=27\n";
    live_state_parse(t, sizeof(t) - 1, &st);
    data_reset();
    data_apply_live(&st);
    memset(&b, 0, sizeof(b));
    CK(export_csv_session(&sink, d));
    const char* row = strchr(b.buf, '\n') + 1;
    CK(strstr(row, "\"A,B\"\"C\"") != NULL);
    /* commas outside the quoted field: 10 separators for 11 columns */
    CK_INT(count_char(row, ',') - 1, 10);

    /* JSON with nothing decoded: null, never -1 / 0 sentinels */
    data_reset();
    memset(&b, 0, sizeof(b));
    CK(export_json_session(&sink, d));
    CK(strstr(b.buf, "\"battery\":{\"raw\":null,\"percent\":null,\"temperature_c\":null}") != NULL);

    /* no false UTC claim */
    CasioLifeLogRecord rec = {2026, 10, 26, 22, 18, 1, 2, 3};
    data_reset();
    data_update_record(&rec);
    memset(&b, 0, sizeof(b));
    CK(export_csv_session(&sink, d));
    CK(strstr(b.buf, "2026-10-26T22:18:00,") != NULL);
    CK(strstr(b.buf, "00Z") == NULL);
}

static void drsp_robustness(void) {
    static DrspStream s;
    DrspRecord rec;
    drsp_stream_reset(&s);

    /* unknown but framed record (0x53, 301 bytes) followed by a valid 0x00 */
    uint8_t unk[5 + 301];
    memset(unk, 0xAA, sizeof(unk));
    unk[0] = 0x53; unk[1] = 0x2D; unk[2] = 0x01; unk[3] = 0; unk[4] = 0;
    const uint8_t good[20] = {0, 15, 0, 0, 0, 26, 10, 26, 22, 18, 1, 0, 0, 0, 2, 0, 0, 0, 3, 0};
    drsp_stream_feed(&s, unk, sizeof(unk));
    drsp_stream_feed(&s, good, sizeof(good));
    int n = 0;
    while(drsp_stream_next(&s, &rec) == 1) {
        n++;
        drsp_stream_consume(&s, &rec);
    }
    CK_INT(n, 1);
    CK_INT(s.resync_bytes, 0);
    CK_INT(s.unknown_records, 1);
    CK_INT(s.unknown_bytes, 306);

    /* an unknown body that contains a fake 0x00/15 header yields no phantom */
    drsp_stream_reset(&s);
    uint8_t unk2[5 + 40];
    memset(unk2, 0x11, sizeof(unk2));
    unk2[0] = 0x53; unk2[1] = 40; unk2[2] = 0; unk2[3] = 0; unk2[4] = 0;
    memcpy(unk2 + 10, good, sizeof(good));
    drsp_stream_feed(&s, unk2, sizeof(unk2));
    n = 0;
    while(drsp_stream_next(&s, &rec) == 1) {
        n++;
        drsp_stream_consume(&s, &rec);
    }
    CK_INT(n, 0);

    /* a full buffer is reported, not silently truncated */
    drsp_stream_reset(&s);
    static uint8_t big[DRSP_STREAM_MAX + 100];
    memset(big, 0, sizeof(big));
    CK_INT(drsp_stream_feed(&s, big, sizeof(big)), DRSP_STREAM_MAX);
    CK_INT(s.dropped_bytes, 100);
}

void test_regress(void) {
    live_fields();
    export_format();
    drsp_robustness();
}
