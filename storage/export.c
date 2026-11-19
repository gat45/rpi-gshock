#include "export.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define EXPORT_LINE 192

static bool sink_str(const CasioSink* sink, const char* s) {
    if(!sink || !sink->write) return false;
    if(!s) s = "";
    return sink->write(sink->ctx, s, strlen(s));
}

static bool sink_printf(const CasioSink* sink, const char* fmt, ...) {
    char buf[EXPORT_LINE];
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    if(n < 0) return false;
    if((size_t)n >= sizeof(buf)) n = (int)sizeof(buf) - 1;
    return sink->write(sink->ctx, buf, (size_t)n);
}

/* RFC 4180 field: quoted (and quotes doubled) when it holds , " CR or LF. */
static bool sink_csv_field(const CasioSink* sink, const char* s) {
    if(!s) s = "";
    if(!strpbrk(s, ",\"\r\n")) return sink_str(sink, s);
    if(!sink_str(sink, "\"")) return false;
    for(const char* p = s; *p; p++) {
        if(*p == '"' && !sink_str(sink, "\"\"")) return false;
        if(*p != '"' && !sink->write(sink->ctx, p, 1)) return false;
    }
    return sink_str(sink, "\"");
}

/* Writes a naive ISO-8601 timestamp. The watch sends no time zone, so no "Z"
 * suffix is written: claiming UTC would be false. Empty when nothing decoded. */
static bool sink_timestamp(const CasioSink* sink, const CasioData* d) {
    if(d->record_valid) {
        return sink_printf(
            sink,
            "%04d-%02d-%02dT%02d:%02d:00",
            d->last_record.year,
            d->last_record.month,
            d->last_record.day,
            d->last_record.hour,
            d->last_record.minute);
    }
    if(d->time_valid) {
        return sink_printf(
            sink,
            "%04d-%02d-%02dT%02d:%02d:%02d",
            d->year,
            d->month,
            d->day,
            d->hour,
            d->minute,
            d->second);
    }
    return sink_str(sink, "");
}

bool export_csv_session(const CasioSink* sink, const CasioData* d) {
    if(!sink || !sink->write || !d) return false;

    if(!sink_str(
           sink,
           "timestamp,watch_name,battery_raw,battery_pct,temperature_c,"
           "record_time,value_a,value_b,sequence,frames_decoded,frames_rejected\n")) {
        return false;
    }

    if(!sink_timestamp(sink, d)) return false;

    if(!sink_str(sink, ",")) return false;
    if(!sink_csv_field(sink, d->name_valid ? d->watch_name : "")) return false;
    if(!sink_str(sink, ",")) return false;

    /* Each column is filled only when its own value exists. */
    if(d->battery_raw >= 0 && !sink_printf(sink, "%d", d->battery_raw)) return false;
    if(!sink_str(sink, ",")) return false;
    if(d->battery_raw >= 0 && d->battery_pct >= 0 && !sink_printf(sink, "%d", d->battery_pct)) {
        return false;
    }
    if(!sink_str(sink, ",")) return false;
    if(d->temp_valid && !sink_printf(sink, "%d", d->temperature_c)) return false;
    if(!sink_str(sink, ",")) return false;

    if(d->record_valid) {
        if(!sink_printf(
               sink,
               "%04d-%02d-%02d %02d:%02d,%ld,%ld,%u,",
               d->last_record.year,
               d->last_record.month,
               d->last_record.day,
               d->last_record.hour,
               d->last_record.minute,
               (long)d->last_record.value_a,
               (long)d->last_record.value_b,
               (unsigned)d->last_record.sequence)) {
            return false;
        }
    } else {
        if(!sink_str(sink, ",,,,")) return false;
    }

    return sink_printf(sink, "%lu,%lu\n", (unsigned long)d->frames_decoded, (unsigned long)d->frames_rejected);
}

bool export_json_session(const CasioSink* sink, const CasioData* d) {
    if(!sink || !sink->write || !d) return false;

    /* Escape the watch name for JSON so quotes/backslashes/control bytes in a
     * name cannot break the document (the decoder accepts 0x20..0x7E). */
    char escaped_name[64];
    size_t ei = 0;
    const char* src = d->name_valid ? d->watch_name : "";
    for (size_t i = 0; src[i] != '\0' && ei + 7 < sizeof(escaped_name); i++) {
        const unsigned char c = (unsigned char)src[i];
        if (c == '"' || c == '\\') {
            escaped_name[ei++] = '\\';
            escaped_name[ei++] = (char)c;
        } else if (c < 0x20) {
            int w = snprintf(escaped_name + ei, sizeof(escaped_name) - ei,
                             "\\u%04x", c);
            if (w < 0) break;
            ei += (size_t)w;
        } else {
            escaped_name[ei++] = (char)c;
        }
    }
    escaped_name[ei] = '\0';

    /* Provenance is part of the output on purpose: a consumer must be able to
     * tell a measured value from a derived one, and an absent value from a
     * measured one. It therefore follows the actual content of the document. */
    const char* batt_state = d->battery_raw >= 0 ? "measured" : "absent";
    const char* temp_state = d->temp_valid ? "measured" : "absent";

    if(!sink_str(sink, "{\"schema\":\"casiolab.session/1\",\"provenance\":{")) return false;
    if(!sink_printf(sink, "\"battery_raw\":\"%s\",\"temperature_c\":\"%s\",", batt_state, temp_state)) {
        return false;
    }
    if(!sink_printf(
           sink,
           "\"battery_pct\":\"%s\",\"record\":\"%s\",",
           d->battery_raw >= 0 ? "derived" : "absent",
           d->record_valid ? "measured" : "absent")) {
        return false;
    }
    if(!sink_str(sink, "\"current_time\":\"reference-layout-unverified\"},")) return false;

    if(!sink_str(sink, "\"timestamp\":\"")) return false;
    if(!sink_timestamp(sink, d)) return false;
    if(!sink_str(sink, "\",")) return false;

    if(!sink_printf(
           sink,
           "\"watch_name\":\"%s\",\"name_valid\":%s,",
           escaped_name,
           d->name_valid ? "true" : "false")) {
        return false;
    }

    /* Absent values are JSON null, never a sentinel such as -1 or 0. */
    if(!sink_str(sink, "\"battery\":{\"raw\":")) return false;
    if(!(d->battery_raw >= 0 ? sink_printf(sink, "%d", d->battery_raw) : sink_str(sink, "null"))) {
        return false;
    }
    if(!sink_str(sink, ",\"percent\":")) return false;
    if(!(d->battery_raw >= 0 && d->battery_pct >= 0 ? sink_printf(sink, "%d", d->battery_pct) :
                                                       sink_str(sink, "null"))) {
        return false;
    }
    if(!sink_str(sink, ",\"temperature_c\":")) return false;
    if(!(d->temp_valid ? sink_printf(sink, "%d", d->temperature_c) : sink_str(sink, "null"))) {
        return false;
    }
    if(!sink_str(sink, "},")) return false;

    if(d->record_valid) {
        if(!sink_printf(
               sink,
               "\"record\":{\"year\":%d,\"month\":%d,\"day\":%d,\"hour\":%d,\"minute\":%d,"
               "\"value_a\":%ld,\"value_b\":%ld,\"sequence\":%u},",
               d->last_record.year,
               d->last_record.month,
               d->last_record.day,
               d->last_record.hour,
               d->last_record.minute,
               (long)d->last_record.value_a,
               (long)d->last_record.value_b,
               (unsigned)d->last_record.sequence)) {
            return false;
        }
    } else {
        if(!sink_str(sink, "\"record\":null,")) return false;
    }

    return sink_printf(
        sink,
        "\"frames\":{\"decoded\":%lu,\"rejected\":%lu}}",
        (unsigned long)d->frames_decoded,
        (unsigned long)d->frames_rejected);
}
