#pragma once
#include <stdint.h>
#include <stddef.h>

#include "lifelog_record.h"
#include "live_state.h"

/*
 * Aggregated state decoded from the frames the watch sends.
 *
 * Every field is tagged by how it was obtained:
 *   measured  -> the bytes come from a real capture and the decoder is tested
 *                against them (tests/native/test_decoders.c)
 *   derived   -> computed from measured bytes with a formula from the reference
 *                clients (battery percent)
 *   unverified-> layout comes from the reference implementation only
 *                (current time, 0x09)
 */

#define CASIO_NAME_MAX 24

typedef struct {
    /* 0x28 watch condition */
    int battery_pct; /* derived, -1 = unknown */
    int battery_raw; /* measured, -1 = unknown */
    int temperature_c; /* measured */
    int temp_valid; /* 1 once a temperature was decoded or pushed */

    /* 0x09 current time (reference layout, not yet measured on DW-H5600) */
    int year, month, day;
    int hour, minute, second;
    int time_valid;

    /* 0x23 watch name */
    char watch_name[CASIO_NAME_MAX];
    int name_valid;

    /* 0x00 LifeLog record */
    CasioLifeLogRecord last_record;
    int record_valid;

    uint32_t frames_decoded;
    uint32_t frames_rejected;
} CasioData;

/* Global data instance. */
CasioData* data_manager_get(void);

/* Update from a 0x28 payload (frame starts with 0x28). */
void data_update_battery(const uint8_t* payload, size_t len);

/* Update from a 0x09 payload (frame starts with 0x09). */
void data_update_time(const uint8_t* payload, size_t len);

/* Update from a 0x23 payload (frame starts with 0x23). */
void data_update_name(const uint8_t* payload, size_t len);

/* Update from a decoded type 0x00 LifeLog record. */
void data_update_record(const CasioLifeLogRecord* rec);

/*
 * Apply a state parsed from /ext/casiolab/live.txt (see data/live_state.h).
 * Only the fields actually present in the file are touched, so a companion that
 * pushed a name only will not erase a battery reading decoded earlier.
 *
 * Returns the number of fields applied (0 when nothing usable was in the file).
 */
int data_apply_live(const CasioLiveState* live);

/*
 * Dispatch a single frame from the 0x0016/0x0018 channel on its command byte.
 * Returns 0 when the command was recognised, -1 otherwise.
 */
int data_update_frame(const uint8_t* payload, size_t len);

/* Reset all data. */
void data_reset(void);
