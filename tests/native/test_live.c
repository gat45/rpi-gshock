#include "casiolab_test.h"

#include <string.h>

#include "data/data_manager.h"
#include "data/live_state.h"

/*
 * The bytes below are the ones actually written to /ext/casiolab/live.txt on the
 * device by `python companion/watchlink.py --replay ... --flipper COM8`, read
 * back with `storage read /ext/casiolab/live.txt` (56 bytes, verbatim).
 */
static const char kPushed[] =
    "v=1\n"
    "name=CASIO DW-H5600\n"
    "batt_raw=7\n"
    "batt_pct=0\n"
    "temp_c=27\n";

void test_live(void) {
    CasioLiveState st;

    /* Exact content of the file that was on the SD card. */
    CK_INT(live_state_parse(kPushed, sizeof(kPushed) - 1, &st), 0);
    CK_INT(st.version, 1);
    CK_INT(st.parsed, 1);
    CK_INT(st.name_valid, 1);
    CK_STR(st.name, "CASIO DW-H5600");
    CK_INT(st.batt_raw, 7);
    CK_INT(st.batt_pct, 0);
    CK_INT(st.temp_c, 27);
    CK_INT(st.record_valid, 0);

    /* CRLF line endings (a Windows host can produce them). */
    static const char crlf[] = "v=1\r\nname=CASIO GW-B5600\r\nbatt_raw=6\r\n";
    CK_INT(live_state_parse(crlf, sizeof(crlf) - 1, &st), 0);
    CK_STR(st.name, "CASIO GW-B5600");
    CK_INT(st.batt_raw, 6);
    CK_INT(st.batt_pct, -1); /* absent, not zero */

    /* Untrusted content is refused: no version line. */
    static const char no_version[] = "name=CASIO DW-H5600\nbatt_raw=7\n";
    CK_INT(live_state_parse(no_version, sizeof(no_version) - 1, &st), -1);
    CK_INT(st.version, 0);

    /* Unknown keys must not break parsing (forward compatibility). */
    static const char future[] = "v=1\nfirmware=3.2\nname=CASIO DW-H5600\nble_rssi=-62\n";
    CK_INT(live_state_parse(future, sizeof(future) - 1, &st), 0);
    CK_STR(st.name, "CASIO DW-H5600");

    /* Garbage values are ignored, not converted to 0. */
    static const char garbage[] = "v=1\nbatt_raw=abc\nbatt_pct=\ntemp_c=--4\n";
    CK_INT(live_state_parse(garbage, sizeof(garbage) - 1, &st), 0);
    CK_INT(st.batt_raw, -1);
    CK_INT(st.batt_pct, -1);
    CK_INT(st.temp_c, 0);

    /* A record line with the documented shape fills the numeric fields. */
    static const char record[] =
        "v=1\nrecord=2026-10-26 22:18\nvalue_a=62927\nvalue_b=624\nseq=1263\n";
    CK_INT(live_state_parse(record, sizeof(record) - 1, &st), 0);
    CK_INT(st.record_valid, 1);
    CK_INT(st.record_fields_valid, 1);
    CK_INT(st.record_year, 2026);
    CK_INT(st.record_month, 10);
    CK_INT(st.record_day, 26);
    CK_INT(st.record_hour, 22);
    CK_INT(st.record_minute, 18);
    CK_INT(st.value_a, 62927);
    CK_INT(st.value_b, 624);
    CK_INT(st.seq, 1263);

    /* An impossible record is kept as text but never trusted as a date. */
    static const char bad_record[] = "v=1\nrecord=2026-13-45 99:99\n";
    CK_INT(live_state_parse(bad_record, sizeof(bad_record) - 1, &st), 0);
    CK_INT(st.record_valid, 1);
    CK_INT(st.record_fields_valid, 0);

    /* Negative metrics are legal (value_b was signed in the samples). */
    static const char negative[] = "v=1\nvalue_a=-1\nseq=0\n";
    CK_INT(live_state_parse(negative, sizeof(negative) - 1, &st), 0);
    CK_INT(st.value_a, -1);
    CK_INT(st.seq, 0);

    /* Truncated input must not read past the buffer. */
    CK_INT(live_state_parse("v=", 2, &st), -1);
    CK_INT(live_state_parse(NULL, 0, &st), -2);
    CK_INT(live_state_parse(kPushed, 0, &st), -1);

    /* Merging into data_manager only touches the fields present. */
    data_reset();
    CasioData* d = data_manager_get();
    const uint8_t name_frame[] = {0x23, 'X', 0x00};
    data_update_name(name_frame, sizeof(name_frame));

    CK_INT(live_state_parse(kPushed, sizeof(kPushed) - 1, &st), 0);
    CK_INT(data_apply_live(&st), 3); /* name + battery + temperature */
    CK_STR(d->watch_name, "CASIO DW-H5600");
    CK_INT(d->battery_raw, 7);
    CK_INT(d->battery_pct, 0);
    CK_INT(d->temperature_c, 27);
    CK_INT(d->record_valid, 0); /* the file had no record line */

    /* A file without a record must not erase an existing decoded record. */
    static const uint8_t body[15] = {
        0x1A, 0x0A, 0x1A, 0x16, 0x12, 0xCF, 0xF5, 0x00, 0x00,
        0x70, 0x02, 0x00, 0x00, 0xEF, 0x04};
    CasioLifeLogRecord rec;
    memset(&rec, 0, sizeof(rec));
    lifelog_record_decode(body, sizeof(body), &rec);
    data_update_record(&rec);
    CK_INT(d->record_valid, 1);
    CK_INT(data_apply_live(&st), 3); /* name + battery + temperature are re-applied */
    CK_INT(d->record_valid, 1);
    CK_INT(d->last_record.sequence, 1263);

    /* A record line does overwrite the stored record. */
    CK_INT(live_state_parse(record, sizeof(record) - 1, &st), 0);
    CK_INT(data_apply_live(&st), 1);
    CK_INT(d->record_valid, 1);
    CK_INT(d->last_record.year, 2026);
    CK_INT(d->last_record.value_a, 62927);
    CK_INT(d->last_record.sequence, 1263);

    /* Nothing usable -> nothing applied, no crash. */
    data_reset();
    CasioLiveState empty;
    live_state_reset(&empty);
    CK_INT(data_apply_live(&empty), 0);
    CK_INT(data_apply_live(NULL), 0);
    CK_INT(d->name_valid, 0);

    /* Appended pushes: each v= line starts a new document, so the LAST one
     * wins even when the older one is still in the file (the CLI append). */
    static const char appended[] =
        "v=1\nname=CASIO GW-B5600\nbatt_raw=6\ntemp_c=25\n"
        "v=1\nname=CASIO DW-H5600\nbatt_raw=7\ntemp_c=27\n";
    CK_INT(live_state_parse(appended, sizeof(appended) - 1, &st), 0);
    CK_STR(st.name, "CASIO DW-H5600");
    CK_INT(st.batt_raw, 7);
    CK_INT(st.temp_c, 27);
    CK_INT(st.record_valid, 0); /* keys of the older document must not leak */

    /* A corrupted last document invalidates it instead of resurrecting the
     * previous one: v=0 is not a version this parser accepts. */
    static const char appended_bad[] =
        "v=1\nname=CASIO DW-H5600\nbatt_raw=7\n"
        "v=0\nname=BOGUS\n";
    CK_INT(live_state_parse(appended_bad, sizeof(appended_bad) - 1, &st), -1);
    CK_INT(st.version, 0);
    CK_INT(st.name_valid, 0);

    /* Summary is a single line and never overruns a small buffer. */
    char small[8];
    memset(small, 'Z', sizeof(small));
    live_state_reset(&st);
    live_state_summary(&st, small, sizeof(small));
    CK(small[sizeof(small) - 1] == '\0' || strlen(small) < sizeof(small));
    char full[64];
    CK_INT(live_state_parse(kPushed, sizeof(kPushed) - 1, &st), 0);
    live_state_summary(&st, full, sizeof(full));
    CK_STR(full, "live v1 CASIO DW-H5600 raw=7 pct=0 27C");
}
