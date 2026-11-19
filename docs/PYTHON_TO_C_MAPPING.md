# PYTHON_TO_C_MAPPING

## BLE transport (NOT portable directly — depends on Bleak)
| Python | C (Flipper/Momentum) | Notes |
|---|---|---|
| `bleak.BleakClient` | Momentum BLE API (peripheral only) | Momentum has no Central GATT → use RPi |
| `scanner.scan()` | N/A on Flipper FAP | Use RPi scanner |
| `client.start_notify()` | N/A on Flipper (peripheral role) | Proxy mode needs dual role |
| `client.read_gatt_char()` | N/A on Flipper FAP | Use RPi |
| `client.write_gatt_char()` | N/A on Flipper FAP | Use RPi |

## Logic (portable)
| Python module | C target file | Status |
|---|---|---|
| `casio_constants.py` | `casio/dw_h5600_profile.c` | Mapped |
| `message_dispatcher.py` | `casio/command_catalog.c` | Mapped |
| `packet.py` | `casio/gatt_map.c` | Mapped |
| `standard_protocol.py` | `casio/protocol_state.c` | Mapped |
| `watch_protocol.py` | `casio/protocol_state.c` | Mapped |
| `app_notification.py` + `app_notification_io.py` | `control/command_executor.c` | Mapped |
| `step_counter_io.py` | `data/activity_decoder.c` | Mapped |
| `time_io.py` | `data/time_decoder.c` | Mapped |
| `home_time_io.py` | `data/time_decoder.c` | Mapped |
| `alarms_io.py` | `data/` (future) | Mapped |
| `events_io.py` | `data/` (future) | Mapped |
| `watch_condition_io.py` | `data/battery_decoder.c` | Mapped |
| `watch_name_io.py` | `data/` (future) | Mapped |
| `utils.py` | `casio/` helpers | Mapped |
| `connection.py` (BLE) | `ble/ble_manager.c` (RPi-side) | Not portable to Flipper |
| `scanner.py` | RPi-side | Not portable |

## Key rule
BLE transport depends on Bleak → cannot be ported to Flipper FAP. The Flipper FAP handles UI, storage, analysis, decoding. The RPi handles GATT central operations.

---

# Correction (2026-11-19)

The table above is a **mapping intention**, written before any C existed. Its
`Status: Mapped` column means "a file with that name exists", not "the port is
done". Several targets were wrong, and several modules it maps to are still
empty shells.

## Real targets and real status

| Python module | Real C target | Status |
|---|---|---|
| `step_counter_io.py` (DRSP/Convoy, record `0x00`) | `protocol/drsp.c` + `data/lifelog_record.c` | **implemented + tested** (was mapped to `data/activity_decoder.c`, still an empty shell) |
| `watch_name_io.py` (`0x23`) | `data/watch_name_decoder.c` | **implemented + tested** (was "future") |
| `watch_condition_io.py` (`0x28`) | `data/battery_decoder.c` | **implemented + tested**; percent derived, limits unconfirmed |
| `time_io.py` (`0x09`) | `data/time_decoder.c` | **implemented, not measured**: layout from the reference only |
| `message_dispatcher.py` | `data/data_manager.c` | **implemented + tested** (was mapped to `command_catalog.c`, which is a safety policy) |
| `casio_constants.py` | `casio/command_observed.h` (**generated**) + `casio/command_catalog.c` | **measured table 42 ids**, policy 27 entries; `dw_h5600_profile.c` unchanged (stub) |
| `packet.py` / `standard_protocol.py` | `protocol/drsp.c` | **implemented + tested**; `casio/gatt_map.c` and `casio/protocol_state.c` were never the right targets (`protocol_state` is display-only) |
| `app_notification_io.py` | — | **not implemented**; `control/command_executor.c` is an empty shell |
| `alarms_io.py`, `events_io.py`, `home_time_io.py` | — | **not implemented** |
| `connection.py`, `scanner.py` | `companion/watchlink.py` (host) | **written**; replay verified, BLE transport untested |

## What "not portable" really means

Not portable *to the FAP*. It is portable to the host companion, which is a
normal Python process — `watchlink.py` uses `bleak` exactly like upstream. The
RPi is optional; any host with a radio works.