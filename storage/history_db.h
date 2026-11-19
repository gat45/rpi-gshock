#pragma once
#include <furi.h>
#include <storage/storage.h>

/* Append a packet record to packets.csv in a session directory. */
void history_append_packet(Storage* storage, const char* session_path,
                          const char* timestamp, const char* direction,
                          const char* uuid, uint16_t handle, const uint8_t* payload,
                          size_t len, const char* event_type);

/* Write decoded.json with a single decoded entry. */
void history_write_decoded(Storage* storage, const char* session_path,
                          const char* json_entry);

/* Read session count. */
uint32_t history_session_count(Storage* storage);