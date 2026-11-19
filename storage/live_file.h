#pragma once
#include <furi.h>
#include <storage/storage.h>

#include "../data/live_state.h"

/*
 * Reads the file the companion leaves on the SD card:
 *
 *     /ext/casiolab/live.txt
 *
 * Written by companion/watchlink.py (--flipper COM8) from a real BLE session,
 * or by any host that speaks the format documented in data/live_state.h.
 *
 * Returns true only when the file existed and carried a valid version line.
 * A missing file is normal: it means no companion session has been pushed yet,
 * and the UI then falls back to the embedded capture sample.
 */

#define CASIO_LIVE_PATH "/ext/casiolab/live.txt"
#define CASIO_LIVE_MAX 512u

/* Load and parse. `out` is reset first, so it is safe on every call. */
bool live_file_load(Storage* storage, CasioLiveState* out);

/* Read the file into `buf` (NUL-terminated) without parsing it.
 * Returns the number of bytes read, 0 when absent. */
size_t live_file_read(Storage* storage, char* buf, size_t cap);
