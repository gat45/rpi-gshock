#include "casiolab_test.h"

#include "data/data_manager.h"
#include "data/lifelog_record.h"
#include "protocol/sample_capture.h"
#include "protocol/sample_data.h"
#include "storage/export.h"

typedef struct {
    char buf[4096];
    size_t len;
    int writes;
} Buffer;

static bool buffer_write(void* ctx, const char* data, size_t len) {
    Buffer* b = ctx;
    if(b->len + len >= sizeof(b->buf)) return false;
    memcpy(b->buf + b->len, data, len);
    b->len += len;
    b->buf[b->len] = '\0';
    b->writes++;
    return true;
}

static bool contains(const char* haystack, const char* needle) {
    return strstr(haystack, needle) != NULL;
}

static int brace_balance(const char* s) {
    int depth = 0;
    for(; *s; s++) {
        if(*s == '{') depth++;
        if(*s == '}') depth--;
    }
    return depth;
}

static void populate(void) {
    data_reset();
    /* Documented 15-byte body of the first 0x00 record (capture -4). */
    static const uint8_t body[15] = {
        0x1A, 0x0A, 0x1A, 0x16, 0x12, 0xCF, 0xF5, 0x00, 0x00,
        0x70, 0x02, 0x00, 0x00, 0xEF, 0x04};
    CasioLifeLogRecord rec;
    memset(&rec, 0, sizeof(rec));
    lifelog_record_decode(body, sizeof(body), &rec);
    data_update_name(kSampleNames[1].data, kSampleNames[1].len);
    data_update_battery(kSampleConditions[0].data, kSampleConditions[0].len);
    data_update_record(&rec);
}

void test_export(void) {
    populate();

    Buffer csv = {0};
    const CasioSink csv_sink = {.ctx = &csv, .write = buffer_write};
    CK(export_csv_session(&csv_sink, data_manager_get()));
    CK(csv.len > 0);
    CK(csv.writes > 1); /* streamed, not a single big write */
    CK(contains(csv.buf, "timestamp,watch_name,battery_raw,battery_pct"));
    CK(contains(csv.buf, "CASIO DW-H5600"));
    CK(contains(csv.buf, ",6,0,26,")); /* raw, percent (generic limits), temp */
    CK(contains(csv.buf, "2026-10-26 22:18"));
    CK(contains(csv.buf, ",62927,624,1263,"));

    Buffer json = {0};
    const CasioSink json_sink = {.ctx = &json, .write = buffer_write};
    CK(export_json_session(&json_sink, data_manager_get()));
    CK(json.len > 0);
    CK(contains(json.buf, "\"schema\":\"casiolab.session/1\""));
    CK(contains(json.buf, "\"provenance\""));
    CK(contains(json.buf, "\"battery_pct\":\"derived\""));
    CK(contains(json.buf, "\"current_time\":\"reference-layout-unverified\""));
    CK(contains(json.buf, "\"watch_name\":\"CASIO DW-H5600\""));
    CK(contains(json.buf, "\"sequence\":1263"));
    CK(contains(json.buf, "\"temperature_c\":26"));
    CK_INT(brace_balance(json.buf), 0);

    /* Empty state must still produce a valid document, with nulls not garbage. */
    data_reset();
    Buffer csv_empty = {0};
    const CasioSink e_sink = {.ctx = &csv_empty, .write = buffer_write};
    CK(export_csv_session(&e_sink, data_manager_get()));
    CK(contains(csv_empty.buf, "timestamp,watch_name"));
    CK(!contains(csv_empty.buf, "CASIO"));

    Buffer json_empty = {0};
    const CasioSink je_sink = {.ctx = &json_empty, .write = buffer_write};
    CK(export_json_session(&je_sink, data_manager_get()));
    CK(contains(json_empty.buf, "\"record\":null"));
    CK(contains(json_empty.buf, "\"name_valid\":false"));
    CK(contains(json_empty.buf, "\"record\":\"absent\""));
    CK(contains(json_empty.buf, "\"battery_pct\":\"absent\""));
    CK_INT(brace_balance(json_empty.buf), 0);

    /* A failing sink is reported, not ignored. */
    const CasioSink dead_sink = {.ctx = NULL, .write = NULL};
    CK(!export_csv_session(&dead_sink, data_manager_get()));
    CK(!export_json_session(&dead_sink, data_manager_get()));
    CK(!export_csv_session(NULL, data_manager_get()));
    CK(!export_json_session(&je_sink, NULL));
}
