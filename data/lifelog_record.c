#include "lifelog_record.h"

#include <stdio.h>

int lifelog_record_decode(const uint8_t* body, size_t len, CasioLifeLogRecord* out) {
    if(!body || !out || len < LIFELOG_RECORD00_LEN) return -1;

    const int year = 2000 + body[0];
    const int month = body[1];
    const int day = body[2];
    const int hour = body[3];
    const int minute = body[4];

    if(month < 1 || month > 12) return -2;
    if(year > 2099) return -6;
    {
        static const int dm[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
        const int leap = (year % 4 == 0 && year % 100 != 0) || year % 400 == 0;
        const int maxd = (month == 2 && leap) ? 29 : dm[month - 1];
        if(day < 1 || day > maxd) return -3;
    }
    if(hour > 23) return -4;
    if(minute > 59) return -5;

    out->year = year;
    out->month = month;
    out->day = day;
    out->hour = hour;
    out->minute = minute;
    out->value_a = (int32_t)((uint32_t)body[5] | ((uint32_t)body[6] << 8) |
                             ((uint32_t)body[7] << 16) | ((uint32_t)body[8] << 24));
    out->value_b = (int32_t)((uint32_t)body[9] | ((uint32_t)body[10] << 8) |
                             ((uint32_t)body[11] << 16) | ((uint32_t)body[12] << 24));
    out->sequence = (uint16_t)(body[13] | (body[14] << 8));
    return 0;
}

void lifelog_record_format(const CasioLifeLogRecord* rec, char* out, size_t out_cap) {
    if(!rec || !out || out_cap < 17) {
        if(out && out_cap) out[0] = '\0';
        return;
    }
    snprintf(
        out,
        out_cap,
        "%04d-%02d-%02d %02d:%02d",
        rec->year,
        rec->month,
        rec->day,
        rec->hour,
        rec->minute);
}
