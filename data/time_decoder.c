#include "time_decoder.h"

/*
 * Reference-derived decoder (gshock_api TimeEncoderPure). See time_decoder.h:
 * this layout is NOT yet confirmed on a DW-H5600 capture. It is kept because
 * it is the documented wire format of the reference client, and it is tested
 * against the reference's own encoder output in tests/native/test_decoders.c.
 */
int time_decode(const uint8_t* payload, size_t len,
                int* year, int* month, int* day,
                int* hour, int* minute, int* second) {
    if(!payload || len < 11) return -1;
    if(payload[0] != 0x09) return -1;

    const int y = payload[1] | (payload[2] << 8);
    const int mo = payload[3];
    const int d = payload[4];
    const int h = payload[5];
    const int mi = payload[6];
    const int s = payload[7];

    if(y < 2000 || y > 2099) return -6;
    if(mo < 1 || mo > 12) return -2;
    if(d < 1 || d > 31) return -3;
    if(h > 23) return -4;
    if(mi > 59 || s > 60) return -5;

    if(year)   *year   = y;
    if(month)  *month  = mo;
    if(day)    *day    = d;
    if(hour)   *hour   = h;
    if(minute) *minute = mi;
    if(second) *second = s;
    return 0;
}
