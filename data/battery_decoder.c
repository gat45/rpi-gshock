#include "battery_decoder.h"

int battery_percent_from_raw(int raw_level, int lower_limit, int upper_limit) {
    if(upper_limit <= lower_limit) return -1;

    const int range = upper_limit - lower_limit;
    /* round(100/range), but never collapse to 0 for wide ranges. */
    int multiplier = (100 + range / 2) / range;
    if (multiplier < 1) multiplier = 1;
    int pct = (raw_level - lower_limit) * multiplier;
    if(pct < 0) pct = 0;
    if(pct > 100) pct = 100;
    return pct;
}

int battery_decode_full(
    const uint8_t* payload,
    size_t len,
    int lower_limit,
    int upper_limit,
    CasioBattery* out) {
    if(!payload || !out || len < 3) return -1;
    if(payload[0] != 0x28) return -2;
    if(upper_limit <= lower_limit) return -3;

    out->raw_level = payload[1];
    out->temperature_c = payload[2];
    out->percent = battery_percent_from_raw(payload[1], lower_limit, upper_limit);
    return 0;
}

int battery_decode(const uint8_t* payload, size_t len) {
    CasioBattery b;
    if(battery_decode_full(
           payload, len, CASIO_BATT_LOWER_GENERIC, CASIO_BATT_UPPER_GENERIC, &b) != 0) {
        return -1;
    }
    return b.percent;
}
