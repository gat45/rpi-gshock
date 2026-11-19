#pragma once

#include <furi.h>
#include <storage/storage.h>

/* Create a new session directory under /ext/casiolab/sessions/session_NNNN.
   Returns FuriString with the path, or NULL on failure. Caller frees. */
FuriString* session_create(Storage* storage);

/* List existing session directories. Returns count. */
uint32_t session_count(Storage* storage);