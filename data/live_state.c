#include "live_state.h"

#include <stdio.h>
#include <string.h>

void live_state_reset(CasioLiveState* out) {
    if(!out) return;
    memset(out, 0, sizeof(*out));
    out->batt_raw = -1;
    out->batt_pct = -1;
}

/* Accepts an optional leading '-' then decimal digits. Rejects anything else. */
static int parse_int(const char* s, size_t len, int* out) {
    if(len == 0 || !out) return -1;

    size_t i = 0;
    int sign = 1;
    if(s[0] == '-') {
        sign = -1;
        i = 1;
        if(len == 1) return -1;
    }

    long value = 0;
    for(; i < len; i++) {
        if(s[i] < '0' || s[i] > '9') return -1;
        /* Guard against overflow on 32-bit targets (Flipper is Cortex-M4). */
        if(value > (100000000L - (s[i] - '0')) / 10) return -1;
        value = value * 10 + (s[i] - '0');
    }

    *out = (int)(sign * value);
    return 0;
}

/* True when s starts with prefix (prefix contains the '='). */
static int has_prefix(const char* s, size_t len, const char* prefix) {
    const size_t n = strlen(prefix);
    return len >= n && memcmp(s, prefix, n) == 0;
}

static int days_in_month(int y, int m) {
    static const int dm[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    const int leap = (y % 4 == 0 && y % 100 != 0) || y % 400 == 0;
    return (m == 2 && leap) ? 29 : dm[m - 1];
}

/* Parses exactly "YYYY-MM-DD HH:MM" (16 chars). Returns 1 on success. */
static int parse_record_time(
    const char* s,
    int* year,
    int* month,
    int* day,
    int* hour,
    int* minute) {
    if(!s || !year || !month || !day || !hour || !minute) return 0;
    if(strlen(s) != 16) return 0;
    if(s[4] != '-' || s[7] != '-' || s[10] != ' ' || s[13] != ':') return 0;

    int y, mo, d, h, mi;
    if(parse_int(s, 4, &y) != 0) return 0;
    if(parse_int(s + 5, 2, &mo) != 0) return 0;
    if(parse_int(s + 8, 2, &d) != 0) return 0;
    if(parse_int(s + 11, 2, &h) != 0) return 0;
    if(parse_int(s + 14, 2, &mi) != 0) return 0;

    if(y < 2000 || y > 2099) return 0;
    if(mo < 1 || mo > 12) return 0;
    if(d < 1 || d > days_in_month(y, mo)) return 0;
    if(h > 23 || mi > 59) return 0;

    *year = y;
    *month = mo;
    *day = d;
    *hour = h;
    *minute = mi;
    return 1;
}

static void live_state_parse_line(CasioLiveState* st, const char* line, size_t len) {
    if(!st || !line || len == 0) return;

    int value = 0;

    if(has_prefix(line, len, "v=")) {
        /* Any v= line starts a new document: drop everything accumulated from
         * the previous push, so an appended file behaves like a fresh one.
         * A malformed version therefore invalidates the whole document instead
         * of silently keeping the older one alive. */
        live_state_reset(st);
        if(parse_int(line + 2, len - 2, &value) == 0 && value > 0) {
            st->version = (uint32_t)value;
        }
        return;
    }
    if(has_prefix(line, len, "name=")) {
        const char* v = line + 5;
        size_t n = len - 5;
        size_t w = 0;
        for(size_t i = 0; i < n && w + 1 < sizeof(st->name); i++) {
            const unsigned char c = (unsigned char)v[i];
            /* The companion only sends ASCII, but the SD card can hold anything. */
            if(c < 0x20 || c > 0x7E) break;
            st->name[w++] = (char)c;
        }
        st->name[w] = '\0';
        st->name_valid = w > 0;
        st->parsed = 1;
        return;
    }
    if(has_prefix(line, len, "batt_raw=")) {
        if(parse_int(line + 9, len - 9, &value) == 0 && value >= 0 && value <= 255) {
            st->batt_raw = value;
            st->batt_valid = 1;
            st->parsed = 1;
        }
        return;
    }
    if(has_prefix(line, len, "batt_pct=")) {
        if(parse_int(line + 9, len - 9, &value) == 0 && value >= 0 && value <= 100) {
            st->batt_pct = value;
            st->parsed = 1;
        }
        return;
    }
    if(has_prefix(line, len, "temp_c=")) {
        if(parse_int(line + 7, len - 7, &value) == 0 && value >= -60 && value <= 150) {
            st->temp_c = value;
            st->temp_valid = 1;
            st->parsed = 1;
        }
        return;
    }
    if(has_prefix(line, len, "record=")) {
        size_t n = len - 7;
        if(n >= sizeof(st->record)) n = sizeof(st->record) - 1;
        memcpy(st->record, line + 7, n);
        st->record[n] = '\0';
        st->record_valid = n > 0;
        st->parsed = 1;
        st->record_fields_valid = parse_record_time(
            st->record,
            &st->record_year,
            &st->record_month,
            &st->record_day,
            &st->record_hour,
            &st->record_minute);
        return;
    }
    if(has_prefix(line, len, "value_a=")) {
        if(parse_int(line + 8, len - 8, &value) == 0) {
            st->value_a = value;
            st->metrics_valid = 1;
            st->parsed = 1;
        }
        return;
    }
    if(has_prefix(line, len, "value_b=")) {
        if(parse_int(line + 8, len - 8, &value) == 0) {
            st->value_b = value;
            st->metrics_valid = 1;
            st->parsed = 1;
        }
        return;
    }
    if(has_prefix(line, len, "seq=")) {
        /* seq is a uint16 on the wire: reject what would be truncated. */
        if(parse_int(line + 4, len - 4, &value) == 0 && value >= 0 && value <= 65535) {
            st->seq = (unsigned)value;
            st->metrics_valid = 1;
            st->parsed = 1;
        }
        return;
    }
    /* Unknown key: ignored deliberately, see live_state.h. */
}

int live_state_parse(const char* text, size_t len, CasioLiveState* out) {
    if(!text || !out) return -2;

    live_state_reset(out);

    size_t start = 0;
    for(size_t i = 0; i <= len; i++) {
        const int at_end = (i == len);
        if(!at_end && text[i] != '\n') continue;

        size_t end = i;
        if(end > start && text[end - 1] == '\r') end--; /* CRLF from a PC host */
        if(end > start) live_state_parse_line(out, text + start, end - start);
        start = i + 1;
    }

    /* Without a version line the content is untrusted: an old or foreign file
     * must never be displayed as if the companion wrote it. The fields are
     * cleared too, so a caller that ignores the return value cannot read a
     * half-parsed document. */
    if(out->version == 0) {
        live_state_reset(out);
        return -1;
    }
    return 0;
}

void live_state_summary(const CasioLiveState* state, char* out, size_t out_cap) {
    if(!out || out_cap == 0) return;
    out[0] = '\0';
    if(!state) return;

    const char* name = state->name_valid ? state->name : "?";
    /* Built with snprintf so the FAP and the host tests produce the same text;
     * the buffer is always truncated safely when out_cap is small. */
    snprintf(
        out,
        out_cap,
        "live v%lu %s raw=%d pct=%d %dC",
        (unsigned long)state->version,
        name,
        state->batt_raw,
        state->batt_pct,
        state->temp_c);
}
