# SOURCE_MAP — gshock_api (izivkov)

## Repo: https://github.com/izivkov/gshock_api (Python, MIT)

### Top-level
| Path | Role |
|---|---|
| `src/gshock_api/` | Core library |
| `src/examples/` | Usage examples (api_tests, h5600_health, app_notifications_tests, step_counter) |
| `tests/` | Unit tests |
| `test_data/` | Test vectors |
| `docs/` | Sphinx docs |
| `h5600_lifelog.json` | LifeLog sample data |
| `failed_step_payload_20260902_125923.hex` | Failed payload capture |
| `step-output.txt` | Step counter output sample |
| `KOTLIN_PORTING_GUIDE.md` | Architecture notes |
| `KOTLIN_LIFELOG_PORTING_GUIDE.md` | LifeLog format notes |
| `KOTLIN_CHANGE_SUMMARY_029d440.md` | Change summary |
| `CHANGELOG.rst`, `RELEASE_NOTES.md` | History |

### Core modules (src/gshock_api/)
| File | Lines | Role |
|---|---|---|
| `connection.py` | 171 | BLE connection via Bleak, scan, subscribe to all notify/indicate chars |
| `scanner.py` | 116 | Device scan + filter |
| `message_dispatcher.py` | 120 | Routes incoming packets by first byte (command key) |
| `casio_constants.py` | 79 | UUIDs, handles, command IDs (CHARACTERISTICS dict) |
| `gshock_api.py` | 137 | High-level API facade |
| `watch_info.py` | 668 | Watch info model (battery, name, etc.) |
| `casio_time_zone_helper.py` | 295 | Time zone / DST helpers |
| `app_notification.py` | 54 | App notification construction |
| `utils.py` | 171 | Helpers (to_casio_cmd, etc.) |
| `logger.py` | 46 | Logging |
| `exceptions.py` | 15 | Exception types |
| `cancelable_result.py` | 40 | Async result wrapper |
| `always_connected_watch_filter.py` | 46 | Filter for always-connected watches |

### iolib/ (protocol I/O)
| File | Lines | Role |
|---|---|---|
| `packet.py` | 54 | Packet framing |
| `standard_protocol.py` | 268 | Standard protocol encode/decode |
| `watch_protocol.py` | 172 | Base protocol |
| `analogue_protocol.py` | 57 | Analogue protocol |
| `mip_protocol.py` | 11 | MIP protocol (stub) |
| `connection_protocol.py` | 9 | Connection protocol |
| `message_dispatcher` (top-level) | 120 | Dispatch by command key |
| `time_io.py` | 142 | Time read/write |
| `home_time_io.py` | 83 | Home time |
| `second_dial_io.py` | 97 | Second dial |
| `timer_io.py` | 117 | Timer |
| `alarms_io.py` | 286 | Alarms |
| `events_io.py` | 355 | Events/reminders |
| `exercise_io.py` | 102 | Exercise |
| `step_counter_io.py` | 356 | Step counter + LifeLog (DRSP/Convoy) |
| `watch_condition_io.py` | 91 | Battery/conditions |
| `watch_name_io.py` | 53 | Watch name |
| `app_info_io.py` | 83 | App info |
| `app_notification_io.py` | 184 | App notifications (DW-H5600) |
| `settings_io.py` | 244 | Settings |
| `world_cities_io.py` | 68 | World cities |
| `dst_watch_state_io.py` | 64 | DST state |
| `dst_for_world_cities_io.py` | 61 | DST for cities |
| `gw_bx5600_time_io.py` | 201 | GW-BX5600 time |
| `time_adjustment_io.py` | 108 | Time adjustment |
| `button_pressed_io.py` | 110 | Button pressed |
| `exercise_profile.py` | 9 | Exercise profile |
| `error_io.py`, `unknown_io.py` | 7 each | Error/unknown |

### model/
| File | Lines | Role |
|---|---|---|
| `event.py` | 203 | Event model |
| `alarms.py` | 40 | Alarms model |
| `settings.py` | 19 | Settings model |
| `step_counter_data.py` | 57 | Step counter data |
| `exercise_profile.py` | 9 | Exercise profile |

### protocols/
| File | Lines | Role |
|---|---|---|
| `watch_protocol.py` | 172 | Base protocol |
| `standard_protocol.py` | 268 | Standard protocol encode/decode |
| `analogue_protocol.py` | 57 | Analogue protocol |
| `mip_protocol.py` | 11 | MIP protocol (minimal) |
| `connection_protocol.py` | 9 | Connection protocol |

### Complementary repo: izivkov/GShockAPI (Kotlin)
- WatchProtocol, StandardProtocol, AnalogueProtocol, MipProtocol
- MessageDispatcher, Connection, GShockScanner
- IO classes, data models, reassembly, sync

---

# Correction (2026-11-19) — local map

The map above describes the **upstream Python repo**. It says nothing about
what exists in this project, which is where the confusion came from: several
modules named after upstream files are still empty shells. This section is the
real map of `CasioLab/`.

## Implemented (code + measurements + tests)

| Local file | Upstream origin | State |
|---|---|---|
| `protocol/drsp.{h,c}` | `step_counter_io.py` (DRSP/Convoy) | framing, XOR 0xFF, CRC16-KERMIT LE, stream + resync |
| `protocol/sample_capture.{h,c}` | — | replays embedded real chunks through the decoders |
| `protocol/sample_data.h` | — | **generated** by `tools/gen_sample_capture.py` |
| `data/lifelog_record.{h,c}` | `step_counter_io.py` record `0x00` | 15-byte record, binary date, 2×int32 (semantics unknown) |
| `data/watch_name_decoder.{h,c}` | `watch_name_io.py` (`0x23`) | printable-string, trimmed |
| `data/battery_decoder.{h,c}` | `watch_condition_io.py` (`0x28`) | raw + temp measured, percent derived |
| `data/time_decoder.{h,c}` | `time_io.py` (`0x09`) | **reference layout only, not measured** |
| `data/live_state.{h,c}` | — | `key=value` parser, append-safe |
| `data/data_manager.{h,c}` | `message_dispatcher.py` | single decoded state + provenance |
| `storage/live_file.{h,c}` | — | reads `/ext/casiolab/live.txt` from the tail |
| `storage/export.{h,c}` | — | CSV/JSON sinks, pure C |
| `storage/export_csv.{h,c}` | — | Flipper wrapper over `export_csv_session` |
| `storage/export_json.{h,c}` | — | Flipper wrapper over `export_json_session` |
| `storage/session_manager.{h,c}` | — | `session_NNNN/` directories |
| `storage/history_db.{h,c}` | — | raw packet CSV (not wired to the UI) |
| `casio/command_observed.h` | `casio_constants.py` (ids only) | **generated**, 42 measured ids |
| `casio/command_catalog.{h,c}` | — | hand-written safety policy, 27 entries |
| `casio/protocol_state.{h,c}` | — | display-only state |
| `casio_lab.{h,c}`, `ui/main_menu.{h,c}` | — | app shell, 8-entry menu |
| `companion/watchlink.py` | `connection.py`, `scanner.py` | **host** BLE central + replay + push |
| `tools/gen_*.py` | — | regenerate the two measured headers |

## Still empty shells (honest list)

These files contain only a stub and must not be described as working:

```
ble/          ble_manager, ble_security, central, connection, gatt_discovery, subscriptions
casio/        dw_h5600_profile, gatt_map, initialization, synchronization
control/      command_executor, write_manager, response_tracker, safety_guard
data/         activity_decoder, heart_rate_decoder, sleep_decoder, workout_decoder, unknown_decoder
monitor/      connection_monitor, event_logger, statistics
proxy/        central_client, peripheral_server, packet_router, packet_filter, packet_rewriter, proxy_manager
reverse/      raw_capture, protocol_logger, packet_comparator, sequence_analyzer, replay_engine
storage/      deduplication, export_bin
```

Two groups, two different reasons:

- **`ble/` and `proxy/`**: impossible on the **installed** Momentum
  (`mntm-dev`, API 87.1). `api_symbols.csv` exports BLE **server** symbols only
  — no GAP scan, no GATT client. A FAP cannot be the central. The code belongs
  on the host (`companion/watchlink.py`). **Correction (2026-11-19):** this is
  a limit of the current build, not of the hardware. Making the Flipper a
  central requires three separate changes — copro stack `ble_full`, GAP/GATT
  client code, API export. See `BLE_CENTRAL_PLAN.md`; the stubs stay stubs
  until those layers exist.
- **`data/` decoders (HR, sleep, workout, activity)**: possible but not
  measured. No capture maps those payloads to values, so writing a decoder
  would mean inventing the format. They stay unimplemented by rule.
- **`control/`, `reverse/`, `monitor/`, `storage/` shells**: plain
  unimplemented work, no measurement blocked them; they were never written.

## Where the constants come from

| Constant | Source |
|---|---|
| Handles `0x0016`→`0x0018`, LifeLog `0x0013`, `0x0010`, GW-B5600 `0x000C`/`0x000E` | 5 ATT captures in `F:\freebuff\gshock\DW-H5600-EMU\analysis\btsnoop\` |
| `kObservedCommands[]` (42 ids, counts, handles, samples) | `tools/gen_command_observed.py` over those CSVs |
| `kSampleFlowChunks[]`, `kSampleNames[]`, `kSampleConditions[]` | `tools/gen_sample_capture.py` over those CSVs |
| `kF06B0`, LifeLog tables | `DW-H5600-EMU\v8\src\lifelog_frames.h` |
| `0x09` layout | `gshock_api/iolib/time_io.py` — **reference, not measured** |
| Pairing profile (legacy, bonding, no MITM, 16-byte key) | `analysis\btsnoop\smp_pdu_bruts.txt` |

### Evidence corpus — verified paths and hashes (2026-11-19)

The evidence tree `F:\freebuff\gshock\` is **read-only** for this project:
only `Get-FileHash` / reads are performed, nothing is written there.

The 5 ATT CSVs live in `DW-H5600-EMU\analysis\btsnoop\` and match
`SHA256_LIVRABLES.txt` byte for byte (size and SHA-256 checked):

| File | Size | SHA-256 |
|---|---|---|
| `btsnoop_hci-DW-H5600-1_att.csv` | 39 427 | `8a59c0a2a23c3a071dcf0f103e26103d33cfc870914ca605c471c7d2cdebdf81` |
| `btsnoop_hci_DW-H5600-2_att.csv` | 177 843 | `81edc8b1c758c7889b2d10c11749a31ca18e4684a708764b49842fddc3589172` |
| `btsnoop_hci_DW-H5600-3_att.csv` | 55 554 | `2d8719709d9d5782c86b8a4be32c6d8cac99463b5c74b91f8ae8fe1a3eed9c34` |
| `btsnoop_hci_DW-H5600-4_att.csv` | 187 827 | `5ff60ccb8ca178146464751f81fcd5797ad21127953c6f0c8d5babfcc04ef3e3` |
| `btsnoop_hci_DW-H5600_att.csv` | 52 717 | `c098d702db5e926defd7e998d7de753bd632f4966be3e73ee6b0311add49edff` |

Two derived artefacts in the same folder:

| File | Size | SHA-256 | Use |
|---|---|---|---|
| `btsnoop_synthese.json` | 3 069 911 | `79125d0ad430caa24e22203447743366cb8df43b2c148634f442072b46b5f0cd` | index of **11** captures (hci / smp / att / services / chars) |
| `smp_pdu_bruts.txt` | 5 913 | `f47fb886c3151319f1eec9a878bdcf9958fa04b14925e73aabec4ca68ebe5126` | every SMP PDU, direction and decoding |

**Correction to an earlier path claim**: the raw `btsnoop_hci*.log` files are
**not** under `DW-H5600-EMU\`; `DW-H5600-EMU\casio-chk` does not exist. They
are one level up:

```
F:\freebuff\gshock\casio-chk\py\test_data\
  btsnoop_hci-DW-H5600-1.log   1 292 506
  btsnoop_hci_DW-H5600-2.log   2 130 563
  btsnoop_hci_DW-H5600-3.log   2 741 913
  btsnoop_hci_DW-H5600-4.log     288 680
  btsnoop_hci_DW-H5600.log     1 364 783
```

The CSVs are the derived, audited form; the `.log` files are the raw captures
and are kept as the fallback source if a CSV ever needs regeneration.