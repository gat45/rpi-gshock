#include "session_manager.h"

#include <furi.h>
#include <storage/storage.h>
#include <stdio.h>
#include <string.h>

#define CASIOLAB_ROOT "/ext/casiolab"
#define SESSIONS_DIR  "/ext/casiolab/sessions"

FuriString* session_create(Storage* storage) {
    storage_common_mkdir(storage, CASIOLAB_ROOT);
    storage_common_mkdir(storage, SESSIONS_DIR);

    /* Find next session number */
    /* Start probing at the current count instead of 1: O(1) in the common
     * case instead of one SD lookup per existing session. Probing forward still
     * guarantees a free name even when numbers have gaps. */
    uint32_t n = 0;
    char path[128];
    uint32_t start = session_count(storage);
    if (start < 1) start = 1;
    for (uint32_t i = start; i < 10000; i++) {
        snprintf(path, sizeof(path), "%s/session_%04lu", SESSIONS_DIR, (unsigned long)i);
        if (!storage_dir_exists(storage, path)) {
            n = i;
            break;
        }
    }
    if (n == 0) return NULL; /* 9999 sessions: refuse instead of reusing a name */

    FuriString* session_path = furi_string_alloc();
    furi_string_printf(session_path, "%s/session_%04lu", SESSIONS_DIR, (unsigned long)n);

    if (storage_common_mkdir(storage, furi_string_get_cstr(session_path)) != FSE_OK) {
        furi_string_free(session_path);
        return NULL;
    }

    /* Write metadata.json */
    char meta_path[160];
    snprintf(meta_path, sizeof(meta_path), "%s/metadata.json", furi_string_get_cstr(session_path));
    File* f = storage_file_alloc(storage);
    if (storage_file_open(f, meta_path, FSAM_WRITE, FSOM_CREATE_ALWAYS)) {
        char buf[64];
        snprintf(buf, sizeof(buf), "{\"session\":%lu,\"status\":\"created\"}\n", (unsigned long)n);
        storage_file_write(f, buf, strlen(buf));
        storage_file_close(f);
    }
    storage_file_free(f);

    return session_path;
}

uint32_t session_count(Storage* storage) {
    uint32_t count = 0;
    FileInfo info;
    File* dir = storage_file_alloc(storage);
    if (storage_dir_open(dir, SESSIONS_DIR)) {
        char name[256];
        while (storage_dir_read(dir, &info, name, sizeof(name))) {
            if (info.flags & FSF_DIRECTORY) count++;
        }
        storage_dir_close(dir);
    }
    storage_file_free(dir);
    return count;
}