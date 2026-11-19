#pragma once
#include <stdbool.h>
#include <stddef.h>

#include "../data/data_manager.h"

/*
 * Session serialisation, independent from the Flipper storage API so that it
 * can be compiled and tested on the host (tests/native/test_export.c).
 *
 * A sink is a write callback plus an opaque context. The Flipper wrapper in
 * export_csv.c / export_json.c plugs a File into it; the test plugs a buffer.
 */

typedef bool (*CasioSinkWrite)(void* ctx, const char* data, size_t len);

typedef struct {
    void* ctx;
    CasioSinkWrite write;
} CasioSink;

/* CSV: one header line + one data line. Returns false if the sink failed. */
bool export_csv_session(const CasioSink* sink, const CasioData* data);

/* JSON: one object, no trailing newline. Returns false if the sink failed. */
bool export_json_session(const CasioSink* sink, const CasioData* data);
