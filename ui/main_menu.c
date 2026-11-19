#include "main_menu.h"

#include <gui/modules/submenu.h>
#include <gui/modules/widget.h>
#include <furi.h>
#include <storage/storage.h>
#include <stdio.h>
#include <string.h>

#include "../storage/session_manager.h"
#include "../storage/export_csv.h"
#include "../storage/export_json.h"
#include "../storage/live_file.h"
#include "../casio/command_catalog.h"
#include "../casio/protocol_state.h"
#include "../data/data_manager.h"
#include "../protocol/sample_capture.h"
#include "../protocol/sample_data.h"

static const char* menu_items[] = {
    "CasioLab v0.2",
    "REVERSE",
    "DATA",
    "CONTROL",
    "MONITOR",
    "PROXY",
    "SAMPLE",
    "STORAGE",
};

#define MENU_COUNT (sizeof(menu_items) / sizeof(menu_items[0]))

static void show(CasioLabApp* app, const char* text) {
    widget_reset(app->widget);
    widget_add_string_multiline_element(
        app->widget, 2, 2, AlignLeft, AlignTop, FontSecondary, text);
    app->current_view = CasioLabViewMode;
    view_dispatcher_switch_to_view(app->view_dispatcher, CasioLabViewMode);
}

/* Fill data_manager with the frames captured from the real watch. Used by
 * SAMPLE, and by STORAGE when no live data exists (there is no BLE central on
 * Momentum, so this is the only source of real decoded values on device). */
static void load_sample(void) {
    CasioSampleResult s;
    sample_capture_run(&s);
    data_reset();
    for(uint32_t i = 0; i < CASIO_SAMPLE_NAME_COUNT; i++) {
        data_update_name(kSampleNames[i].data, kSampleNames[i].len);
    }
    /* First condition only, like sample_capture_run(): DATA, SAMPLE and
     * MONITOR must show the same battery/temperature. */
    if(CASIO_SAMPLE_CONDITION_COUNT > 0) {
        data_update_battery(kSampleConditions[0].data, kSampleConditions[0].len);
    }
    if(s.record_valid) data_update_record(&s.record);
}

/*
 * Load /ext/casiolab/live.txt (written by companion/watchlink.py from a real
 * BLE session) and merge it into data_manager.
 *
 * A successful live document is authoritative: the decoded state is cleared
 * first, so DATA never mixes a live name/temperature with a record left over
 * from the embedded sample. Otherwise a stale sample record would be exported
 * as if the companion had sent it.
 *
 * Returns true when at least one field was applied.
 */
static bool load_live(void) {
    Storage* storage = furi_record_open(RECORD_STORAGE);
    CasioLiveState live;
    bool ok = live_file_load(storage, &live);
    if(ok) {
        data_reset();
        ok = data_apply_live(&live) > 0;
    }
    furi_record_close(RECORD_STORAGE);
    return ok;
}

static void submenu_callback(void* context, uint32_t index) {
    CasioLabApp* app = context;
    CasioData* d = data_manager_get();
    char buf[320];

    if(index == 0) {
        snprintf(
            buf,
            sizeof(buf),
            "CasioLab v0.2\n"
            "Casio DW-H5600 lab\n"
            "Decodes captured\n"
            "watch bytes.\n"
            "No BLE central here.\n"
            "Cmds seen: %lu",
            (unsigned long)cmd_observed_count());
        show(app, buf);
        return;
    }

    switch(index) {
    case 1: {
        uint32_t req = 0, rsp = 0;
        for(uint32_t i = 0; i < cmd_observed_count(); i++) {
            req += kObservedCommands[i].requests;
            rsp += kObservedCommands[i].responses;
        }
        snprintf(
            buf,
            sizeof(buf),
            "REVERSE\n\n"
            "Captures analysed: 5\n"
            "Commands seen: %lu\n"
            "Frames C->W: %lu\n"
            "Frames W->C: %lu\n"
            "Link: offline",
            (unsigned long)cmd_observed_count(),
            (unsigned long)req,
            (unsigned long)rsp);
        break;
    }
    case 2: {
        /* Prefer the live file (real BLE session pushed by the companion) and
         * fall back to the embedded capture sample so the view is never empty.
         * There is no BLE central inside the FAP on Momentum, so these are the
         * only two sources of real bytes on the device. */
        const bool live_ok = load_live();
        if(!live_ok) load_sample();
        d = data_manager_get();

        char rec[24] = "none";
        if(d->record_valid) {
            snprintf(
                rec,
                sizeof(rec),
                "%04d-%02d-%02d %02d:%02d",
                d->last_record.year,
                d->last_record.month,
                d->last_record.day,
                d->last_record.hour,
                d->last_record.minute);
        }
        char batt[24] = "unknown";
        if(d->battery_raw >= 0) {
            if(d->battery_pct >= 0)
                snprintf(batt, sizeof(batt), "%d (%d%%)", d->battery_raw, d->battery_pct);
            else
                snprintf(batt, sizeof(batt), "%d", d->battery_raw);
        }
        char temp[16] = "unknown";
        if(d->temp_valid) snprintf(temp, sizeof(temp), "%d C", d->temperature_c);
        snprintf(
            buf,
            sizeof(buf),
            "DATA (%s)\n"
            "Name: %s\n"
            "Batt raw: %s\n"
            "Temp: %s\n"
            "Record: %s\n"
            "a=%ld b=%ld seq=%u",
            live_ok ? "live.txt" : "sample",
            d->name_valid ? d->watch_name : "unknown",
            batt,
            temp,
            rec,
            d->record_valid ? (long)d->last_record.value_a : 0l,
            d->record_valid ? (long)d->last_record.value_b : 0l,
            d->record_valid ? (unsigned)d->last_record.sequence : 0u);
        break;
    }
    case 3: {
        char lines[5][32];
        for(uint32_t i = 0; i < 5; i++) {
            if(i < cmd_observed_count()) {
                const CasioObservedCommand* o = &kObservedCommands[i];
                const CmdEntry* e = cmd_lookup(o->id);
                snprintf(
                    lines[i],
                    sizeof(lines[i]),
                    "%02X %-10.10s %lu/%lu",
                    o->id,
                    e ? e->name : "?",
                    (unsigned long)o->requests,
                    (unsigned long)o->responses);
            } else {
                lines[i][0] = '\0';
            }
        }
        snprintf(
            buf,
            sizeof(buf),
            "CONTROL (%lu cmds)\n"
            "%s\n%s\n%s\n%s\n%s",
            (unsigned long)cmd_count(),
            lines[0],
            lines[1],
            lines[2],
            lines[3],
            lines[4]);
        break;
    }
    case 4: {
        CasioSampleResult s;
        sample_capture_run(&s);
        snprintf(
            buf,
            sizeof(buf),
            "MONITOR (sample)\n"
            "Chunks: %lu (crc %lu)\n"
            "Continuations: %lu\n"
            "Records: %lu\n"
            "Resync bytes: %lu\n"
            "Dropped: %lu chunks",
            (unsigned long)s.chunks,
            (unsigned long)s.crc_chunks,
            (unsigned long)s.continuation_chunks,
            (unsigned long)s.records,
            (unsigned long)s.resync_bytes,
            (unsigned long)s.dropped_chunks);
        break;
    }
    case 5:
        snprintf(
            buf,
            sizeof(buf),
            "PROXY: unavailable\n"
            "Momentum API gives\n"
            "a GATT server only:\n"
            "no scan, no client.\n"
            "Use the host\n"
            "companion (watchlink).");
        break;
    case 6: {
        CasioSampleResult s;
        sample_capture_run(&s);
        load_sample();
        char rec[24] = "none";
        if(s.record_valid) {
            snprintf(
                rec,
                sizeof(rec),
                "%04d-%02d-%02d %02d:%02d",
                s.record.year,
                s.record.month,
                s.record.day,
                s.record.hour,
                s.record.minute);
        }
        snprintf(
            buf,
            sizeof(buf),
            "SAMPLE (capture)\n"
            "%lu chunks, %lu crc\n"
            "%lu records decoded\n"
            "Record: %s\n"
            "a=%ld b=%ld seq=%u",
            (unsigned long)s.chunks,
            (unsigned long)s.crc_chunks,
            (unsigned long)s.records,
            rec,
            (long)s.record.value_a,
            (long)s.record.value_b,
            (unsigned)s.record.sequence);
        break;
    }
    case 7: {
        /* Load the data first, then create the session: a failed load never
         * leaves an empty session directory behind. */
        const bool live_ok = load_live();
        d = data_manager_get();
        if(!live_ok && !d->record_valid && !d->name_valid) load_sample();

        Storage* storage = furi_record_open(RECORD_STORAGE);
        FuriString* sp = session_create(storage);
        if(sp) {
            const bool csv_ok = export_csv_create(storage, furi_string_get_cstr(sp));
            const bool json_ok = export_json_create(storage, furi_string_get_cstr(sp));
            if(!csv_ok && !json_ok) {
                /* Nothing was exported: do not accumulate empty sessions. */
                storage_simply_remove_recursive(storage, furi_string_get_cstr(sp));
            }
            const uint32_t cnt = session_count(storage);
            snprintf(
                buf,
                sizeof(buf),
                "STORAGE (%s)\n"
                "%s\n"
                "Sessions: %lu\n"
                "CSV: %s\n"
                "JSON: %s",
                live_ok ? "live.txt" : "sample",
                furi_string_get_cstr(sp),
                (unsigned long)cnt,
                csv_ok ? "ok" : "fail",
                json_ok ? "ok" : "fail");
            furi_string_free(sp);
        } else {
            snprintf(
                buf,
                sizeof(buf),
                "STORAGE\nCreate failed\nSessions: %lu",
                (unsigned long)session_count(storage));
        }
        furi_record_close(RECORD_STORAGE);
        break;
    }
    default:
        snprintf(buf, sizeof(buf), "Unknown entry %lu", (unsigned long)index);
        break;
    }

    show(app, buf);
    FURI_LOG_I("CasioLab", "Selected %lu", (unsigned long)index);
}

void casio_lab_menu_setup(CasioLabApp* app) {
    submenu_reset(app->submenu);
    for(size_t i = 0; i < MENU_COUNT; i++) {
        submenu_add_item(app->submenu, menu_items[i], i, submenu_callback, app);
    }
    app->current_view = CasioLabViewSubmenu;
    view_dispatcher_switch_to_view(app->view_dispatcher, CasioLabViewSubmenu);
}
