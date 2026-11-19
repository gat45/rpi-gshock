# CODE AUDIT — CasioLab (FAP + companion)

Scope: the modules that actually contain code (not the 2-line stubs). Native
host tests pass: **186 checks, 0 failures** (ASan/UBSan clean on the same
build). The findings below are what the tests do **not** cover: Flipper-side
code, storage paths, JSON/CSV escaping, and the Python companion.

## 1. Real bugs (fix before relying on the output)

### 1.1 `storage/history_db.c` — unchecked open + potential UB
- Line 20: the fallback `storage_file_open(f, path, FSAM_WRITE, FSOM_CREATE_ALWAYS)`
  return value is **not checked**. If it fails the file handle is left unopened
  and the following `storage_file_write` operates on a dead handle.
- Line 45 (`history_write_decoded`): same unchecked `storage_file_open`.
- Lines 22/34/46: `storage_file_write` return values ignored (minor, but a
  short write is silently reported as success).
- **Potential undefined behaviour**: in `history_append_packet`, `pos` is the
  return of `snprintf`. If that `snprintf` fails it returns a negative value;
  the loop condition `pos < (int)sizeof(line) - 4` is then true and
  `snprintf(line + pos, ...)` writes at `line - 1`. Add
  `if (pos < 0) return;` (or at least `if (pos < 0) pos = 0;`) before the loop.

### 1.2 `storage/export.c` (JSON) — unescaped `watch_name`
`export_json_session` prints `watch_name` with `%s` into a JSON string. The
watch name comes from `watch_name_decode`, which accepts the whole printable
ASCII range `0x20..0x7E`, **including `"` and `\`**. A name containing either
produces malformed JSON that a consumer cannot parse. Same class of bug in CSV:
a name containing `,`, `\n` or `"` corrupts the row. Mitigation: escape in the
sink, or reject names containing those bytes at decode time.

### 1.3 `data/battery_decoder.c` — degenerate range
`battery_percent_from_raw`: `multiplier = (100 + range/2) / range`. For
`range > 200` integer division yields `0`, so every battery reads `0 %`.
The guard `upper_limit <= lower_limit` does not cover `range > 200`. Add
`if (range > 100) ...` handling or clamp `multiplier` to at least 1.

### 1.4 `ui/main_menu.c` case 7 — nested storage + ordering
`session_create`/`session_count` and `load_live()` each call
`furi_record_open(RECORD_STORAGE)` independently. `furi_record_open` is
reference-counted so this is not a crash today, but it is fragile: if
`load_live()` ever fails to close on an error path, the count leaks. Also
`export_csv_create`/`export_json_create` are called with `furi_string_get_cstr(sp)`
while `sp` is still alive — fine, but the order (create session, then load
live, then export) means a failed live load silently falls back to the sample
without telling the user. Add an explicit status line.

## 2. Latent / robustness issues

- **No JSON/CSV escaping** anywhere in `export.c` (see 1.2). Also `history_db.c`
  writes `timestamp,direction,uuid,...` with raw strings — same injection class.
- **`time_decode`** does not bound `year` (payload[1]|payload[2]<<8 can be
  0..65535). Downstream `last_record.year` is printed `%04d`; a 5-digit year
  overruns the 4-digit field. Add `y < 2000 || y > 2099` style check.
- **`data_manager` is not thread-safe**: `g_data` is a plain static with no
  lock. Today the FAP is single-threaded (no BLE central yet), so it works,
  but the moment a BLE callback thread writes to `g_data` while the UI reads
  it, this becomes a data race. Plan a mutex before M6.
- **`protocol_state`** has the same global-mutable, no-lock problem.
- **`casio_lab_app_alloc`** does not check `malloc`/`view_dispatcher_alloc`/
  `submenu_alloc`/`widget_alloc` returns. Standard Flipper code assumes success,
  but a failed alloc here dereferences NULL. Low priority (OOM on a 4 KB stack
  FAP is unlikely), but worth a `furi_check` for debug builds.
- **`live_file.c` tail path**: `memmove(buf, buf + skip, read - skip)` is
  safe only because `skip <= read` is guaranteed by `strchr` returning a
  pointer inside `buf[0..read-1]`. The invariant holds, but it is implicit;
  a one-line comment would prevent a future regression.

## 3. Documented omissions (35 of 56 modules are stubs)

These are intentional (Phase 0), but they are the real feature gap. Listed by
layer:

- **BLE central (all stubs):** `ble/central.c`, `ble/ble_manager.c`,
  `ble/connection.c`, `ble/gatt_discovery.c`, `ble/subscriptions.c`,
  `ble/ble_security.c`. The whole central role is unimplemented — this is the
  M3–M7 work, not a bug.
- **Control (all stubs):** `control/command_executor.c`, `response_tracker.c`,
  `safety_guard.c`, `write_manager.c`. There is no path yet from a UI action to
  a write on the watch, and no safety gate on dangerous commands.
- **Monitor (all stubs):** `connection_monitor.c`, `event_logger.c`,
  `statistics.c`.
- **Proxy (all stubs):** the entire `proxy/*` layer. The PROXY menu entry
  already says "Not available", consistent.
- **Reverse (all stubs):** `packet_comparator.c`, `protocol_logger.c`,
  `raw_capture.c`, `replay_engine.c`, `sequence_analyzer.c`.
- **Decoders (stubs):** `data/activity_decoder.c`, `heart_rate_decoder.c`,
  `sleep_decoder.c`, `workout_decoder.c`, `unknown_decoder.c`. Only battery,
  time, watch-name and LifeLog record 0x00 are implemented.
- **Casio profile (stubs):** `casio/dw_h5600_profile.c`, `gatt_map.c`,
  `initialization.c`, `synchronization.c`.
- **Storage (stubs):** `storage/deduplication.c`, `export_bin.c`.

## 4. Data-flow blind spots

- `data_update_frame()` only dispatches `0x28`, `0x23`, `0x09`. LifeLog frames
  (type `0x00`, handled by `lifelog_record_decode`) are never routed through
  it — `sample_capture.c` decodes them directly. So on-device DATA shows
  battery/name/time but never the activity record unless it comes through the
  live file.
- `data_apply_live()` applies `value_a/value_b/seq` only when
  `metrics_valid` is set, but never validates the record's date against the
  `record_fields_valid` flag beyond what `live_state` already did. Acceptable,
  but the two flags are easy to confuse; a short comment in
  `data_manager.h` would help.
- `live_state_parse` resets the whole state on every `v=` line. Correct for
  appended files, but it means a truncated final document wipes the previous
  good one. The test suite pins this behaviour (`appended_bad`), so it is a
  deliberate choice, not a bug — but it is a *product* decision worth revisiting
  if partial pushes become common.

## 5. Companion (`companion/watchlink.py`)

- `push_to_flipper` uses a substring check (`text.strip() in back`) to confirm
  the write reached the filesystem. A partial match or an echo of the command
  itself can produce a false positive. Prefer checking that the read-back
  contains the *last* `v=` document verbatim.
- `BleTransport.connect` constructs `BleakClient(address, timeout=...)`. On
  bleak >= 0.21 the constructor signature is
  `BleakClient(address, timeout=...)` (still works) but `start_notify` by
  handle (int) is deprecated in favour of the characteristic UUID/string. Works
  today, watch for breakage on bleak upgrade.
- `collect()` calls `transport.request(cmd)` without `await` for the replay
  path (correct, it is sync) and with `await` for BLE (correct). The
  `hasattr(transport, "connect")` discriminator is fragile — a future transport
  that exposes `connect` but is sync would break. Minor.

## 6. Suggested priority

1. Fix 1.1 and 1.2 before the next STORAGE/JSON export is trusted.
2. Fix 1.3 (one-line clamp).
3. Add a mutex plan for `g_data` before any BLE callback thread lands (M6).
4. Decide whether `data_update_frame` should route LifeLog frames (4.1) —
   this is the difference between "DATA shows a record" and "DATA never shows
   a record" on device.
