#include "data_manager.h"

#include <string.h>

#include "battery_decoder.h"
#include "time_decoder.h"
#include "watch_name_decoder.h"

static CasioData g_data = {
    .battery_pct = -1,
    .battery_raw = -1,
    .temperature_c = 0,
    .temp_valid = 0,
    .time_valid = 0,
    .name_valid = 0,
    .record_valid = 0,
    .frames_decoded = 0,
    .frames_rejected = 0,
};

CasioData* data_manager_get(void) {
    return &g_data;
}

void data_update_battery(const uint8_t* payload, size_t len) {
    CasioBattery b;
    if(battery_decode_full(
           payload, len, CASIO_BATT_LOWER_GENERIC, CASIO_BATT_UPPER_GENERIC, &b) == 0) {
        g_data.battery_raw = b.raw_level;
        g_data.temperature_c = b.temperature_c;
        g_data.temp_valid = 1;
        g_data.battery_pct = b.percent;
        g_data.frames_decoded++;
    } else {
        g_data.frames_rejected++;
    }
}

void data_update_time(const uint8_t* payload, size_t len) {
    int y, m, d, h, mi, s;
    if(time_decode(payload, len, &y, &m, &d, &h, &mi, &s) == 0) {
        g_data.year = y;
        g_data.month = m;
        g_data.day = d;
        g_data.hour = h;
        g_data.minute = mi;
        g_data.second = s;
        g_data.time_valid = 1;
        g_data.frames_decoded++;
    } else {
        g_data.frames_rejected++;
    }
}

void data_update_name(const uint8_t* payload, size_t len) {
    if(watch_name_decode(payload, len, g_data.watch_name, sizeof(g_data.watch_name)) == 0) {
        g_data.name_valid = 1;
        g_data.frames_decoded++;
    } else {
        g_data.frames_rejected++;
    }
}

void data_update_record(const CasioLifeLogRecord* rec) {
    if(!rec) return;
    g_data.last_record = *rec;
    g_data.record_valid = 1;
    g_data.frames_decoded++;
}

int data_apply_live(const CasioLiveState* live) {
    if(!live) return 0;

    int applied = 0;

    if(live->name_valid) {
        size_t n = strlen(live->name);
        if(n >= sizeof(g_data.watch_name)) n = sizeof(g_data.watch_name) - 1;
        memcpy(g_data.watch_name, live->name, n);
        g_data.watch_name[n] = '\0';
        g_data.name_valid = 1;
        applied++;
    }

    if(live->batt_raw >= 0) {
        g_data.battery_raw = live->batt_raw;
        if(live->batt_pct >= 0) {
            g_data.battery_pct = live->batt_pct;
        } else {
            g_data.battery_pct = battery_percent_from_raw(
                live->batt_raw, CASIO_BATT_LOWER_GENERIC, CASIO_BATT_UPPER_GENERIC);
        }
        applied++;
    }

    /* temp_c is independent of batt_raw: a push carrying only the temperature
     * must not be silently dropped. */
    if(live->temp_valid) {
        g_data.temperature_c = live->temp_c;
        g_data.temp_valid = 1;
        applied++;
    }

    if(live->record_fields_valid) {
        g_data.last_record.year = live->record_year;
        g_data.last_record.month = live->record_month;
        g_data.last_record.day = live->record_day;
        g_data.last_record.hour = live->record_hour;
        g_data.last_record.minute = live->record_minute;
        if(live->metrics_valid) {
            g_data.last_record.value_a = (int32_t)live->value_a;
            g_data.last_record.value_b = (int32_t)live->value_b;
            g_data.last_record.sequence = (uint16_t)live->seq;
        }
        g_data.record_valid = 1;
        applied++;
    }

    return applied;
}

int data_update_frame(const uint8_t* payload, size_t len) {
    if(!payload || len < 1) return -1;

    switch(payload[0]) {
    case 0x28:
        data_update_battery(payload, len);
        return 0;
    case 0x23:
        data_update_name(payload, len);
        return 0;
    case 0x09:
        data_update_time(payload, len);
        return 0;
    default:
        g_data.frames_rejected++;
        return -1;
    }
}

void data_reset(void) {
    g_data.battery_pct = -1;
    g_data.battery_raw = -1;
    g_data.temperature_c = 0;
    g_data.temp_valid = 0;
    g_data.time_valid = 0;
    g_data.name_valid = 0;
    g_data.record_valid = 0;
    g_data.watch_name[0] = '\0';
    memset(&g_data.last_record, 0, sizeof(g_data.last_record));
    g_data.frames_decoded = 0;
    g_data.frames_rejected = 0;
}
