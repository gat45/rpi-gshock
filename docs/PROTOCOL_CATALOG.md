# PROTOCOL_CATALOG — gshock_api (DW-H5600 relevant)

## BLE Transport (Bleak)
- Library: `bleak` (BleakClient, BleakGATTCharacteristic)
- Scan: `scanner.scan(device_address, watch_filter, timeout)`
- Connect: `BleakClient(address).connect()`
- Service/char discovery: `client.services` → `service.characteristics`
- Subscribe: `client.start_notify(uuid, notification_handler)` for all notify/indicate chars
- Read/Write: `client.read_gatt_char`, `client.write_gatt_char`

## Casio proprietary UUIDs (from casio_constants.py)
| UUID | Name | Handle |
|---|---|---|
| `26eb002c-b012-49a8-b1f8-394fb2032b0f` | CASIO_READ_REQUEST_FOR_ALL_FEATURES | 0x0C |
| `26eb002d-b012-49a8-b1f8-394fb2032b0f` | CASIO_ALL_FEATURES | 0x0D (notify) |
| `26eb0030-b012-49a8-b1f8-394fb2032b0f` | CASIO_NOTIFICATION | — |
| `26eb0023-b012-49a8-b1f8-394fb2032b0f` | CASIO_DATA_REQUEST_SP (DRSP) | 0x11 |
| `26eb0024-b012-49a8-b1f8-394fb2032b0f` | CASIO_CONVOY | 0x14 |
| `26eb002e-b012-49a8-b1f8-394fb2032b0f` | CASIO_SET_CONFIGURATION (SP_REQUEST) | 0x17 |
| `26eb002f-b012-49a8-b1f8-394fb2032b0f` | CASIO_GET_CONFIGURATION (SP_DATA) | 0x19 |
| `00002a00-...` | Device Name | 0x04/0x07 |
| `00002a01-...` | Appearance | 0x06 |
| `00002a07-...` | TX Power | 0x09 |
| `00002a25-...` | Serial Number | — |

Handles requiring write-without-response: 0x0C, 0x0D, 0x14, 0x17

## Command/Feature IDs (CHARACTERISTICS dict)
| ID | Name |
|---|---|
| 0x23 | CASIO_WATCH_NAME |
| 0x22 | CASIO_APP_INFORMATION |
| 0x10 | CASIO_BLE_FEATURES |
| 0x11 | CASIO_SETTING_FOR_BLE |
| 0x3B | CASIO_ADVERTISE_PARAMETER_MANAGER |
| 0x3A | CASIO_CONNECTION_PARAMETER_MANAGER |
| 0x26 | CASIO_ACTIVITY_RECORD |
| 0x28 | CASIO_WATCH_CONDITION (battery %) |
| 0x20 | CASIO_VERSION_INFORMATION |
| 0x1D | CASIO_DST_WATCH_STATE |
| 0x1E | CASIO_DST_SETTING |
| 0x47 | CASIO_SERVICE_DISCOVERY_MANAGER |
| 0x09 | CASIO_CURRENT_TIME |
| 0x45 | CASIO_SETTING_FOR_USER_PROFILE |
| 0x43 | CASIO_SETTING_FOR_TARGET_VALUE |
| 0x0A | ALERT_LEVEL |
| 0x15 | CASIO_SETTING_FOR_ALM |
| 0x16 | CASIO_SETTING_FOR_ALM2 |
| 0x13 | CASIO_SETTING_FOR_BASIC |
| 0x2D | CASIO_SETTING_FOR_EXERCISE |
| 0x39 | CASIO_CURRENT_TIME_MANAGER |
| 0x1F | CASIO_WORLD_CITIES |
| 0x30 | CASIO_REMINDER_TITLE |
| 0x31 | CASIO_REMINDER_TIME |
| 0x18 | CASIO_TIMER |
| 0xFF | ERROR |
| 0x24 | CASIO_HOME_TIME |
| 0x03/0x05/0x06 | GW_BX5600_SP_DATA_HEADER_* |

## Message dispatcher
`message_dispatcher.MessageDispatcher.on_received(data)` routes by first byte (command key).

## Notification handler (connection.py)
- DRSP (`26eb0023`) → `StepCounterIO.on_drsp_received`
- Convoy (`26eb0024`) → `StepCounterIO.on_received`
- Others → `MessageDispatcher.on_received`

## Status legend
- IMPLEMENTED: in code
- TESTED: has test/execution evidence
- DW_H5600_CONFIRMED: demonstrated on DW-H5600 specifically
- GENERIC: common across models
- PARTIAL: incomplete
- UNKNOWN: insufficient info
- NOT_SUPPORTED: absent/incompatible

---

# Correction (2026-11-19)

This catalogue is the **upstream / reference** view. It must not be read as the
handles the DW-H5600 actually uses: the measured captures disagree with it.

## Reference vs measured handles

| Purpose | This document | Measured on DW-H5600 |
|---|---|---|
| Command write | `0x17` (`SET_CONFIGURATION`) | **`0x0016`** |
| Command notify | `0x19` (`GET_CONFIGURATION`) | **`0x0018`** |
| DRSP | `0x11` (`CASIO_DATA_REQUEST_SP`) | LifeLog stream on **`0x0013`** |
| Convoy | `0x14` | not observed as a separate handle in the 5 captures |
| All features | `0x0D` | `0x0010` carries the `09 1D` DST frames |
| GW-B5600 pair | — | **`0x000C` / `0x000E`** |

The measured values come from `casio/command_observed.h`, generated from the 5
ATT captures, and they are what the companion and the docs use.

## Consequence for C code

Do not build `casio/gatt_map.c` from the table above: it would encode the wrong
handles. Only the measured values are usable, and they are referenced directly
where they are needed.

## Command ids

The table above lists the reference ids (`0x09`–`0x47`). The measured corpus
shows **42 distinct ids**, with per-id request/response counts — that is the
number quoted in the FAP (`cmd_observed_count()`), not the reference list size.