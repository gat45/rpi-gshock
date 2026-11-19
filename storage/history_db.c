#include "history_db.h"

#include <furi.h>
#include <storage/storage.h>
#include <stdio.h>
#include <string.h>

/* Appends `src` to line[*pos] as an RFC 4180 field; never overruns `cap`. */
static void csv_put(char* line, int cap, int* pos, const char* src) {
    if(!src) src = "";
    const bool quote = strpbrk(src, ",\"\r\n") != NULL;
    if(quote && *pos < cap - 1) line[(*pos)++] = '"';
    for(; *src && *pos < cap - 3; src++) {
        if(*src == '"') line[(*pos)++] = '"';
        line[(*pos)++] = *src;
    }
    if(quote && *pos < cap - 1) line[(*pos)++] = '"';
    line[*pos] = '\0';
}

void history_append_packet(Storage* storage, const char* session_path,
                          const char* timestamp, const char* direction,
                          const char* uuid, uint16_t handle, const uint8_t* payload,
                          size_t len, const char* event_type) {
    char path[256];
    snprintf(path, sizeof(path), "%s/packets.csv", session_path);

    File* f = storage_file_alloc(storage);
    /* Append: open existing or create. */
    if (!storage_file_open(f, path, FSAM_WRITE, FSOM_OPEN_APPEND)) {
        if (!storage_file_open(f, path, FSAM_WRITE, FSOM_CREATE_ALWAYS)) {
            storage_file_free(f);
            return;
        }
        const char* header = "timestamp,direction,uuid,handle,payload,len,event_type\n";
        if (storage_file_write(f, header, strlen(header)) != strlen(header)) {
            storage_file_close(f);
            storage_file_free(f);
            return;
        }
    }

    char line[512];
    int pos = 0;
    line[0] = '\0';
    csv_put(line, (int)sizeof(line), &pos, timestamp);
    line[pos++] = ',';
    csv_put(line, (int)sizeof(line), &pos, direction);
    line[pos++] = ',';
    csv_put(line, (int)sizeof(line), &pos, uuid);
    line[pos] = '\0';
    {
        int w = snprintf(line + pos, sizeof(line) - (size_t)pos, ",0x%04X,", handle);
        if (w < 0) w = 0;
        pos += w;
        if (pos >= (int)sizeof(line)) pos = (int)sizeof(line) - 1;
    }
    if (!payload) len = 0;
    /* payload as hex */
    for (size_t i = 0; i < len && pos < (int)sizeof(line) - 4; i++) {
        int w = snprintf(line + pos, (size_t)((int)sizeof(line) - pos), "%02X", payload[i]);
        if (w < 0) break;
        pos += w;
        if (pos >= (int)sizeof(line)) { pos = (int)sizeof(line) - 1; break; }
    }
    if (pos >= 0 && pos < (int)sizeof(line)) {
        snprintf(line + pos, (size_t)((int)sizeof(line) - pos), ",%zu,", len);
        pos += (int)strlen(line + pos);
        csv_put(line, (int)sizeof(line), &pos, event_type);
        if (pos < (int)sizeof(line) - 1) { line[pos++] = '\n'; line[pos] = '\0'; }
    }
    line[sizeof(line) - 1] = '\0';
    /* A short write is reported to the log instead of passing for success. */
    if (storage_file_write(f, line, strlen(line)) != strlen(line)) {
        FURI_LOG_E("CasioLab", "history: short write on packets.csv");
    }
    storage_file_close(f);
    storage_file_free(f);
}

void history_write_decoded(Storage* storage, const char* session_path,
                          const char* json_entry) {
    char path[256];
    snprintf(path, sizeof(path), "%s/decoded.json", session_path);

    File* f = storage_file_alloc(storage);
    if (storage_file_open(f, path, FSAM_WRITE, FSOM_CREATE_ALWAYS)) {
        if (json_entry) storage_file_write(f, json_entry, strlen(json_entry));
        storage_file_close(f);
    }
    storage_file_free(f);
}

uint32_t history_session_count(Storage* storage) {
    uint32_t count = 0;
    FileInfo info;
    File* dir = storage_file_alloc(storage);
    if (storage_dir_open(dir, "/ext/casiolab/sessions")) {
        char name[256];
        while (storage_dir_read(dir, &info, name, sizeof(name))) {
            if (info.flags & FSF_DIRECTORY) count++;
        }
        storage_dir_close(dir);
    }
    storage_file_free(dir);
    return count;
}