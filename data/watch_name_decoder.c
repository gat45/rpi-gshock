#include "watch_name_decoder.h"

#include <string.h>

int watch_name_decode(const uint8_t* payload, size_t len, char* out, size_t out_cap) {
    if(!payload || !out || out_cap == 0) return -1;
    if(len < 2 || payload[0] != 0x23) return -2;

    size_t n = 0;
    for(size_t i = 1; i < len && n + 1 < out_cap; i++) {
        const uint8_t c = payload[i];
        if(c == 0x00) break;
        if(c < 0x20 || c > 0x7E) break;
        out[n++] = (char)c;
    }
    out[n] = '\0';

    /* Trim trailing spaces; the watch pads with 0x00, some models with 0x20. */
    while(n > 0 && out[n - 1] == ' ') {
        out[--n] = '\0';
    }

    return n > 0 ? 0 : -3;
}
