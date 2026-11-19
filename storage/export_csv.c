#include "export_csv.h"

#include <stdio.h>

#include "../data/data_manager.h"
#include "export.h"

static bool file_sink_write(void* ctx, const char* data, size_t len) {
    File* file = ctx;
    return storage_file_write(file, data, len) == len;
}

bool export_csv_create(Storage* storage, const char* session_path) {
    if(!storage || !session_path) return false;

    char path[256];
    snprintf(path, sizeof(path), "%s/export_packets.csv", session_path);

    File* file = storage_file_alloc(storage);
    if(!storage_file_open(file, path, FSAM_WRITE, FSOM_CREATE_ALWAYS)) {
        storage_file_free(file);
        return false;
    }

    const CasioSink sink = {.ctx = file, .write = file_sink_write};
    const bool ok = export_csv_session(&sink, data_manager_get());

    storage_file_close(file);
    storage_file_free(file);
    return ok;
}
