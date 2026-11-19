# PORTING_PLAN

## Phases (aligned with CasioLab plan)
1. Phase 0 — Socle (DONE: skeleton compiles)
2. Phase A — Repo cartography (DONE: SOURCE_MAP, AUDIT)
3. Phase B — BLE + protocol analysis (DONE: PROTOCOL_CATALOG, PYTHON_TO_C_MAPPING)
4. Phase C — DW-H5600 features (app notifications, LifeLog)
5. Phase D — LifeLog analysis (LIFELOG_ANALYSIS)
6. Phase E — Python/Kotlin comparison
7. Phase F — Momentum compatibility (DONE: MOMENTUM_COMPATIBILITY)
8. Phase G — Test strategy (DONE: TEST_STRATEGY)

## Next concrete work
1. Analyze app_notification_io.py fully (send_app_notification)
2. Analyze step_counter_io.py (LifeLog fragmentation)
3. Implement encoders/decoders in C
4. Build RPi companion for GATT central
5. Validate against real captures

---

# Correction (2026-11-19)

The phase list above was a plan written before any code existed, and several
`DONE` labels were optimistic. Nothing above is deleted; here is the measured
state.

## Phases, re-labelled

| Phase | Claimed | Real |
|---|---|---|
| 0 — socle | DONE | **done**: builds with `ufbt`, API 87.1, runs on device |
| A — cartography | DONE | **done**: `SOURCE_MAP.md` (with its own correction) |
| B — BLE + protocol | DONE | **partial**: protocol *analysis* exists; no BLE code can exist on the FAP |
| C — DW-H5600 features | — | **not done**: app notifications not implemented |
| D — LifeLog analysis | listed as phase | **done for framing**, not for HR/sleep/workout |
| E — Python/Kotlin comparison | — | **not done**: no systematic comparison |
| F — Momentum compatibility | DONE | **done**: the limit is verified against `api_symbols.csv` |
| G — test strategy | DONE | **wrong at the time**: the doc described `ufbt test`, which was never used. Real harness: `tests/native/`, 186 checks |

## Next concrete work, re-labelled

1. ~~Analyze `app_notification_io.py`~~ — **not started**.
2. ~~Analyze `step_counter_io.py`~~ — **done for the framing**: DRSP (chunk
   `05`, XOR 0xFF, CRC16-KERMIT LE) and record `0x00` are decoded and tested.
3. ~~Implement encoders/decoders in C~~ — **done for** name `0x23`, battery
   `0x28`, record `0x00`, time `0x09` (reference layout only). **Not done** for
   heart rate, sleep, workout, steps: no measurement exists.
4. ~~Build RPi companion for GATT central~~ — **cancelled**. The user rejected
   the RPi as the core of the project; the central is
   `companion/watchlink.py` on a host with a radio. It is written; only its
   replay path is verified (no Bluetooth adapter on the build host).
5. ~~Validate against real captures~~ — **done**: `kObservedCommands[]`
   (42 ids) and `kSampleFlowChunks[]` are generated from the 5 btsnoop
   captures, and the native tests assert the decoded values.

---

# Phase H — BLE central (added 2026-11-19)

Not a porting phase in the Python→C sense: it is a **firmware** phase. It is
listed here because it changes the answer to "can the FAP talk to the watch".

| Item | Status |
|---|---|
| Installed copro stack | **BLE Light**, CubeWB 1.20.0 — proven by sha256 `134a9af4…d327df` / 116 956 B, identical to ST's `stm32wb5x_BLE_Stack_light_fw.bin` |
| GAP central code in Momentum | **absent**: `gap.h` has no scan/connect/discover/read/write |
| `gap_*`, `hci_send_req`, `ble_app_init` in `api_symbols.csv` | `Function,-` (not callable from a FAP) |
| Watch pairing profile | legacy, bonding, **no MITM**, 16-byte key (`smp_pdu_bruts.txt`) |

Full plan, measurements and gates P0–P10: **`BLE_CENTRAL_PLAN.md`**.
Related measurement detail: `MOMENTUM_COMPATIBILITY.md` § "Measured
2026-11-19".

**Blue++ audit done (P2)** — `BLUE_PLUS_PLUS_AUDIT.md`. Result that changes the
work estimate: the GAP/GATT client is **app-side code** (`libble/ble_central.c`,
19 808 B), not firmware code. The firmware change is the GAP role bitmap
(`0x01` → `0x0D`), 6 SDK headers, 14 exported `aci_*`/`hci_*` symbols, and the
`ble_full` copro image. The sub-GHz unlock and loader-header hunks are not
relevant.

Ordering rule: Phase H does **not** unblock the `ble/` and `proxy/` stubs by
itself. They stay unimplemented until the three missing layers (copro
`ble_full`, GAP/GATT client code, API export) exist **and** a connection to the
watch has been measured. Until then the only verified central remains
`companion/watchlink.py`.

Nothing has been flashed. `mntm-dev` (`8ed809fb`, API 87.1) is untouched.