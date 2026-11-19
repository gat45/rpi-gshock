#include "casiolab_test.h"

#include "data/battery_decoder.h"
#include "data/data_manager.h"
#include "data/time_decoder.h"
#include "data/watch_name_decoder.h"
#include "protocol/sample_data.h"

void test_decoders(void) {
    char name[32];

    /* Two measured 0x23 responses, from two different watches. */
    CK_INT(watch_name_decode(kSampleNames[0].data, kSampleNames[0].len, name, sizeof(name)), 0);
    CK_STR(name, "CASIO GW-B5600");
    CK_INT(watch_name_decode(kSampleNames[1].data, kSampleNames[1].len, name, sizeof(name)), 0);
    CK_STR(name, "CASIO DW-H5600");

    /* A name that fills the characteristic without a NUL must still stop. */
    const uint8_t no_nul[] = {0x23, 'A', 'B', 'C', 'D'};
    CK_INT(watch_name_decode(no_nul, sizeof(no_nul), name, sizeof(name)), 0);
    CK_STR(name, "ABCD");

    /* Wrong command byte and empty payload are refused. */
    CK_INT(watch_name_decode(kSampleNames[0].data, 1, name, sizeof(name)), -2);
    const uint8_t not_name[] = {0x28, 0x06, 0x1A};
    CK_INT(watch_name_decode(not_name, sizeof(not_name), name, sizeof(name)), -2);

    /* Measured 0x28 responses: raw level and temperature. */
    struct {
        int raw;
        int temp;
    } expected[] = {{6, 26}, {6, 27}};
    unsigned checked = 0;
    for(unsigned i = 0; i < CASIO_SAMPLE_CONDITION_COUNT && checked < 2; i++) {
        CasioBattery b;
        memset(&b, 0, sizeof(b));
        CK_INT(battery_decode_full(
                   kSampleConditions[i].data,
                   kSampleConditions[i].len,
                   CASIO_BATT_LOWER_GENERIC,
                   CASIO_BATT_UPPER_GENERIC,
                   &b),
               0);
        CK_INT(b.raw_level, expected[checked].raw);
        CK_INT(b.temperature_c, expected[checked].temp);
        checked++;
    }
    CK_INT(checked, 2);

    /* The percent mapping is derived, so it must follow the limits it is given. */
    const uint8_t cond[] = {0x28, 0x06, 0x1A};
    CasioBattery b;
    CK_INT(battery_decode_full(cond, sizeof(cond), 0, 10, &b), 0);
    CK_INT(b.percent, 60);
    CK_INT(battery_decode_full(cond, sizeof(cond), 9, 19, &b), 0);
    CK_INT(b.percent, 0); /* 6 is below the generic lower limit */
    CK_INT(battery_decode_full(cond, sizeof(cond), 10, 10, &b), -3);
    const uint8_t not_condition[] = {0x23, 0x06, 0x1A};
    CK_INT(battery_decode_full(not_condition, sizeof(not_condition), 0, 10, &b), -2);
    CK_INT(battery_decode(cond, sizeof(cond)), 0);

    /* 0x09: reference layout (year little-endian, months 1..12). */
    const uint8_t time_ref[] = {
        0x09, 0xEA, 0x07, 0x0B, 0x12, 0x16, 0x1E, 0x03, 0x00, 0x01, 0x01};
    int y, mo, d, h, mi, s;
    CK_INT(time_decode(time_ref, sizeof(time_ref), &y, &mo, &d, &h, &mi, &s), 0);
    CK_INT(y, 2026);
    CK_INT(mo, 11);
    CK_INT(d, 18);
    CK_INT(h, 22);
    CK_INT(mi, 30);
    CK_INT(s, 3);

    const uint8_t time_bad[] = {
        0x09, 0xEA, 0x07, 0x00, 0x12, 0x16, 0x1E, 0x03, 0x00, 0x01, 0x01};
    CK_INT(time_decode(time_bad, sizeof(time_bad), &y, &mo, &d, &h, &mi, &s), -2);
    CK_INT(time_decode(time_ref, 5, &y, &mo, &d, &h, &mi, &s), -1);

    /* data_manager dispatches on the command byte and keeps provenance. */
    data_reset();
    CasioData* dm = data_manager_get();
    CK_INT(dm->battery_raw, -1);
    CK_INT(dm->name_valid, 0);

    CK_INT(data_update_frame(kSampleNames[1].data, kSampleNames[1].len), 0);
    CK_INT(dm->name_valid, 1);
    CK_STR(dm->watch_name, "CASIO DW-H5600");

    CK_INT(data_update_frame(kSampleConditions[0].data, kSampleConditions[0].len), 0);
    CK_INT(dm->battery_raw, 6);
    CK_INT(dm->temperature_c, 26);
    CK_INT(dm->frames_decoded, 2);

    const uint8_t unknown[] = {0x7E, 0x00};
    CK_INT(data_update_frame(unknown, sizeof(unknown)), -1);
    CK_INT(dm->frames_rejected, 1);

    data_reset();
    CK_INT(dm->battery_raw, -1);
    CK_INT(dm->frames_decoded, 0);
}
