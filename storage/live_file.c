#include "live_file.h"

#include <stdio.h>
#include <string.h>

size_t live_file_read(Storage* storage, char* buf, size_t cap) {
    if(!storage || !buf || cap < 2) return 0;

    buf[0] = '\0';

    File* file = storage_file_alloc(storage);
    if(!storage_file_open(file, CASIO_LIVE_PATH, FSAM_READ, FSOM_OPEN_EXISTING)) {
        storage_file_free(file);
        return 0;
    }

    /* The push command appends, so the file can be larger than the buffer.
     * The newest document is at the end: read the tail, never the head. */
    const uint32_t size = storage_file_size(file);
    const uint32_t window = (uint32_t)(cap - 1);
    const bool tail = size > window;
    if(tail) {
        storage_file_seek(file, (uint32_t)(size - window), true);
    }

    size_t read = storage_file_read(file, buf, cap - 1);
    buf[read] = '\0';

    if(tail) {
        /* The window can start in the middle of a line: drop that fragment so
         * the parser never sees a truncated key (e.g. "att_raw=7"). */
        char* nl = strchr(buf, '\n');
        if(!nl) {
            read = 0;
            buf[0] = '\0';
        } else {
            const size_t skip = (size_t)(nl - buf) + 1;
            memmove(buf, buf + skip, read - skip);
            read -= skip;
            buf[read] = '\0';
        }
    }

    storage_file_close(file);
    storage_file_free(file);
    return read;
}

bool live_file_load(Storage* storage, CasioLiveState* out) {
    if(!storage || !out) return false;

    live_state_reset(out);

    char buf[CASIO_LIVE_MAX];
    size_t read = live_file_read(storage, buf, sizeof(buf));
    if(read == 0) return false;

    /* rstrip whitespace: the CLI write path can leave a trailing CR. */
    while(read > 0 && (buf[read - 1] == '\n' || buf[read - 1] == '\r' || buf[read - 1] == ' ')) {
        buf[--read] = '\0';
    }
    if(read == 0) return false;

    if(live_state_parse(buf, read, out) != 0) {
        live_state_reset(out);
        return false;
    }
    return out->parsed != 0;
}
