#pragma once
#include <furi.h>
#include <storage/storage.h>

/* Write <session_path>/export_decoded.json from the decoded session data.
 * Returns true when the file was written completely. */
bool export_json_create(Storage* storage, const char* session_path);
