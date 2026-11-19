# BLE_CENTRAL_PLAN — Flipper as a GATT central for the DW-H5600

**Status: proposal. Nothing built, nothing flashed.** The installed firmware
(`mntm-dev`, 8ed809fb) is untouched and the CasioLab FAP runs from the SD card.

This document replaces the earlier "RPi companion" plan and supersedes the
optimistic phase list in `PORTING_PLAN.md`. It is written so that every
statement can be checked against a file on disk or a command output.

---

## 1. Measured inputs

| Fact | Value | Where it comes from |
|---|---|---|
| Firmware | `mntm-dev`, commit `8ed809fb`, branch `dev`, built 03-06-2026 | `device_info` on the device |
| API | 87.1, target 7 | `api_symbols.csv`, `device_info` |
| Copro stack | **BLE Light** 1.20.0, flash 116K | see §2 |
| `ble_gatt_service_add`, `ble_gatt_characteristic_init/update` | exported (`+`) | `targets/f7/api_symbols.csv` |
| `gap_init`, `gap_start_advertising`, `hci_send_req`, `ble_app_init` | **not** exported (`-`) | same file |
| GAP functions implemented | `gap_init`, `gap_start_advertising`, `gap_stop_advertising`, `gap_get_state`, `gap_thread_stop`, beacon | `targets/f7/ble_glue/gap.h` |
| Watch pairing profile | legacy, bonding, **no MITM**, 16-byte key | `analysis/btsnoop/smp_pdu_bruts.txt` |

## 2. Proof that the copro runs the light stack

`radio.bin` from `flipper-z-f7-update-mntm-012.zip` and ST CubeWB **v1.20.0**
`stm32wb5x_BLE_Stack_light_fw.bin` are the same file:

```
size 116 956 bytes, sha256 134a9af4…d327df   (identical bytes)
same release, full binary: 151 732 bytes      (not what runs)
```

`radio_stack_type: 3` is the raw `BleGlueC2Info.StackType` printed verbatim by
`furi_hal_info.c`; in this package 3 = light. The number alone is not proof,
the hash is.

## 3. Three layers, not one flag

The phrase "switch Momentum to BLE Full" hides three separate jobs.

### A. Coprocessor stack — change + flash

`fbt_options.py` already documents the option:

```python
COPRO_STACK_BIN  = "stm32wb5x_BLE_Stack_light_fw.bin"
COPRO_STACK_TYPE = "ble_light"
# Firmware also supports "ble_full", but it might not fit into debug builds
```

Switching to `ble_full` changes `radio.bin`, 116 956 → 151 732 bytes (+34 776).
That is a **C2 flash**, not a `.fap` install: it goes through the firmware
update path (DFU/qFlipper), and the copro flash allocation in `ob.data` /
`COPRO_STACK_ADDR` must be checked first. **Gate A**: does the new image fit
the C2 flash window used by this board? Unverified.

### B. GAP / GATT client code — the real work

Switching the stack creates no C code in the firmware. `gap.h` has no scan, no
connect, no discovery, no read/write, no client-side subscription. What is
needed:

- scan start/stop + advertisement report events (observer);
- connect / disconnect + connection event handling;
- primary service / characteristic / descriptor discovery by UUID and by handle;
- attribute read, write, write-without-response;
- CCCD subscription and notification dispatch;
- a connection state machine per link, with error paths;
- security manager (see §4).

**Correction (2026-11-19, after the Blue++ audit):** this code does **not** go
into the firmware. Blue++ implements all of it in `libble/ble_central.c`
(19 808 B), shipped inside the **FAP**, calling the ST ACI command set directly
(`aci_gap_*`, `aci_gatt_*`, `hci_disconnect`) and receiving completions through
`ble_event_dispatcher_register_svc_handler` — which API 87.1 **already
exports**. The firmware side is the GAP role bitmap (`0x01` → `0x0D`), the SDK
headers, the symbol export and the copro image.

Reference implementation: **Blue++** (`andydixon/flipper-bluepp`, Official
1.4.3 base). **Gate B1 is done**: see `BLUE_PLUS_PLUS_AUDIT.md` for the full
patch inventory and the revised M1–M7 plan.

### C. API export — make it callable from a FAP

`api_symbols.csv` currently marks the whole gap/hci surface `-`. Moving
entries to `+` changes the firmware API table, so:

- firmware and SDK must be rebuilt together;
- the API version should be bumped (CasioLab targets 87.1 today);
- every existing FAP keeps working only if the bump is additive.

## 4. Pairing — what the corpus requires

`analysis/btsnoop/smp_pdu_bruts.txt` (5 913 bytes, hash attested) contains:

```
PAIRING_REQUEST   phone->watch  01 04 00 2D 10 0F 0F   ×6
                  io_cap=KeyboardDisplay, bonding=1, MITM=1, SC=1
PAIRING_RESPONSE  watch->phone  02 03 00 01 10 03 03   ×6
                  io_cap=NoInputNoOutput, bonding=1, MITM=0, SC=0
PAIRING_CONFIRM   17-byte PDUs (opcode + 16), ×6 per direction
```

**Correction (2026-11-19, code-level cross-check).** An earlier version of this
section claimed the 17-byte confirm PDU proved legacy pairing. That reasoning
was wrong: **Secure Connections also uses a 16-byte confirm.** The decisive
evidence is the **opcode census** of the whole corpus:

| Opcode | Meaning | Seen |
|---|---|---|
| 0x01 / 0x02 | Pairing Request / Response | 1 / 1 |
| 0x03 / 0x04 | Pairing Confirm / Random | 12 / 12 |
| 0x06 | Encryption Information (LTK) | 12 |
| 0x07 | Central Identification (EDIV+RAND) | 12 |
| 0x08 | Identity Information (IRK) | 2 |
| 0x09 | Identity Address Information | 7 |
| 0x0B | Security Request | **147** |
| 0x0C | **Pairing Public Key** | **0** |
| 0x0D | **Pairing DHKey Check** | **0** |

Secure Connections is **impossible** without 0x0C/0x0D. Zero of either, in
11 captures, settles it: legacy pairing. The `AuthReq` bit (watch answers
`0x01`, SC bit clear) agrees, but the opcode census is the stronger proof.

The `0x0B` count is operational, not cosmetic: the watch sends
`0B 01` (bonding, no MITM) **147 times** — it actively demands bonding and will
keep asking until the central answers.

So the central needs:

- legacy pairing with **Just Works** (no passkey, no OOB);
- confirm/random exchange, STK derivation, encryption start;
- LTK + IRK distribution and persistent storage (bonding);
- encryption of the link before the first protected attribute is read.

Blue++ does not implement central pairing. This module is **not optional** if
the watch requires encryption on the characteristics we need. Which
characteristics are actually protected is still unmeasured.

## 5. Radio ownership

Momentum's BT service owns GAP and keeps a profile running. Starting an app
(`furi_hal_bt_start_app`) stops advertising. A FAP that wants to be central must
take the radio away from the service, and the coexistence contract is not
documented.

Measured on API 87.1: `furi_hal_bt_stop_advertising`,
`furi_hal_bt_start_advertising` and `furi_hal_bt_is_active` are **already
exported to FAPs** (`Function,+`), and Blue++ uses exactly that pattern — stop
advertising on entry, restore on exit. **Gate B2** remains: measure what the
phone link and the BT service do across that transition, and whether the
dispatcher a FAP registers survives the restore.

### 5.1 Event path (read 2026-11-19, `AUDIT_CROSSCHECK.md` §2.2)

The HCI event chain is live at boot: `ble_app.c:88` → `:143` → `:148` →
`event_dispatcher.c`. `ble_event_dispatcher_process_event` calls **every**
registered handler with **every** event, then falls back to the firmware GAP
profile if nobody acks. Consequences a central implementation must respect:

- the callback runs on the **BLE glue thread** — non-blocking only (event
  flags, memcpy, return);
- `BleEventAckFlowEnable` **stops propagation**; a false ack on a peripheral
  event breaks the phone link;
- `ble_event_dispatcher_reset()` does `furi_check(size == 0)` — a FAP that
  exits without unregistering makes the next `gap_init`/`gap_thread_stop`
  **panic the device**;
- the BT service's peripheral profile and a FAP handler share the dispatcher,
  and `CFG_BLE_NUM_LINK` is 2 — a single-device proxy is not excluded by the
  code, only by the missing layers.

## 6. Phases and gates

| # | Step | Output | Gate to pass before next |
|---|---|---|---|
| P0 | inventory | this document | done |
| P1 | build unmodified `mntm-dev` from source | firmware tree + `.fuf` | builds, sizes sane, **not flashed** |
| P2 | audit Blue++ patch | `BLUE_PLUS_PLUS_AUDIT.md` | **done 2026-11-19**: all 7 hunks classified, 14 symbols identified |
| P3 | `ble_full` build | `radio.bin` 151 732 B | fits C2 window (Gate A) |
| P4 | central code in the FAP | `libble/ble_central.c` ported | scan → connect → discover → read `0x2A00` on a test peripheral |
| P5 | API export + SDK | 14 symbols `+`, SDK headers | CasioLab links against the new SDK |
| P6 | flash a **test** firmware | device runs, scan works | explicit user go, backup taken, rollback rehearsed |
| P7 | connect to DW-H5600 | name `0x23` over `0x0016`/`0x0018` | read-only; no write command sent |
| P8 | pairing/bonding | encrypted link | characteristics discovered as readable |
| P9 | LifeLog | DRSP stream from the watch | matches the captured bytes |
| P10 | revisit the 35 stubs | `ble/*`, `proxy/*`, `control/*` | only after P7 works |

The concrete task list for P1–P5 is **M1–M7** in `BLUE_PLUS_PLUS_AUDIT.md` §6.

P3–P5 do not touch the device. P6 is the first irreversible-ish step; DFU
recovery exists but must be rehearsed and documented before it is used.

## 7. Safety rules

- The original update zip and a full SD backup stay available before P6.
- No flash on low battery; no flash from a tree that has uncommitted local
  patches without a written diff.
- The test firmware is built from a clean tree and labelled as such; it is not
  presented as Momentum.
- Nothing in `F:\freebuff\gshock\` is modified: it is the evidence corpus.

## 8. What does not change

Even if all of the above succeeds:

- The host companion (`companion/watchlink.py`) remains the only **verified**
  central today, and stays the fallback.
- Decoders stay transport-agnostic: they take bytes, not a connection.
- The 35 stub modules stay stubs until the layer they need exists.
- Heart rate, sleep, workout still have no measurement — a working central
  does not invent their format.

## 9. Open questions

| Question | How to close it |
|---|---|
| Does the full stack fit the C2 flash? | build `ble_full`, inspect `ob.data` / stack address |
| Which characteristics require encryption? | read the GATT table of a real session, not the reference docs |
| Can the BT service be restored after a FAP takes the radio? | measure on device, P6 |
| Does Blue++'s GAP survive Momentum's event threading? | audit then build, P2–P4 |
| Does the FAP get enough RAM with GAP central? | P4 measurement, `free` before/after |

## 10. Decision log

- **RPi as core: rejected** by the user. Host companion kept as a fallback,
  not as the architecture.
- **Blue++ as base firmware: rejected.** It is a reference to port from, not a
  replacement to install.
- **Momentum stays the target firmware.** The variant, if it happens, is a
  branch of Momentum, built and flashed deliberately, never silently.
