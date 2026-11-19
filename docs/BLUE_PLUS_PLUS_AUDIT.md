# BLUE++ AUDIT — what it actually patches, and what it means for CasioLab

**Gate P2 of `BLE_CENTRAL_PLAN.md`. Read-only audit. Nothing cloned into the
build tree, nothing compiled, nothing flashed.**

Sources read: `github.com/andydixon/flipper-bluepp` @ `main`
(`pushed_at 2026-09-11T17:28:15Z`, repo size 150 KB, GNU GPL-3.0-or-later).
Comparison base: the local `E:\temp\opencode\Momentum-Firmware-dev` tree
(`mntm-dev`, API 87.1) and the installed device `8ed809fb`.

---

## 0. Correction to my own earlier plan

`BLE_CENTRAL_PLAN.md` §3 says the missing GAP/GATT client code must be
"ported into the firmware". **That is wrong, and this audit is why.**

Blue++ puts the central logic in the **FAP**, not in the firmware:

```
libble/ble_central.c   19 808 B   ← the whole central/GATT client
libble/ble_central.h    2 668 B
libble/ble_names.c     19 340 B   ← UI tables + decoders (NOT needed by CasioLab)
libble/ble_names.h      1 221 B
```

`ble_central.c` calls **no** `gap_*` function. It talks to the coprocessor with
the ST ACI command set (`aci_gap_*`, `aci_gatt_*`, `hci_disconnect`) and reads
completions through `ble_event_dispatcher_register_svc_handler`.

So the firmware work is **not** "write a GAP stack". It is: widen the GAP role
bitmap, compile the ST headers into the SDK, expose 14 already-linked symbols,
and flash a bigger coprocessor image. The protocol logic ships as app code.

That shrinks Phase H by an order of magnitude — and moves the real risk to the
**copro flash**, not to the C code.

---

## 1. What the repository is

| Claim | Measured |
|---|---|
| "custom firmware" | **No firmware tree** in the repo. `build.sh` clones `flipperdevices/flipperzero-firmware` tag `1.4.3` and patches it |
| "adds BLE Central" | True, but the central lives in `libble/` and compiles into the **app** |
| "validated on hardware" | README says the opposite: *"Everything here is compile-verified, not yet exercised on hardware"* |
| pairing | README: *"No pairing or bonding as central"* — confirmed by grep: **0** occurrences of `smp`/`pairing`/`bond`/`encrypt`/`ltk` in `ble_central.c` |

The 10 bundled apps are beside the point for CasioLab. Only `libble/` matters.

---

## 2. The complete patch set (7 items)

`build.sh` is 13 704 bytes; it is the entire "firmware". Every hunk, classified:

| # | File patched | What it does | Needed by CasioLab? |
|---|---|---|---|
| 1 | `targets/f7/ble_glue/gap.c` | `GAP_PERIPHERAL_ROLE` → `\| GAP_CENTRAL_ROLE \| GAP_OBSERVER_ROLE` | **Yes** — see §2.1. ST defines the role argument of `aci_gap_init` as a **bitmap**: `0x01` Peripheral, `0x02` Broadcaster, `0x04` Central, `0x08` Observer. Momentum passes `0x01` only, so the stack is initialised peripheral+broadcaster. **Correction to the first version of this audit, which called this patch cosmetic.** |
| 2 | `lib/stm32wb.scons` | appends 6 ST headers to `SDK_HEADERS` (`ble_hci_le.h`, `ble_gap_aci.h`, `ble_gatt_aci.h`, `ble_hal_aci.h`, `ble_l2cap_aci.h`, `ble_vs_codes.h`) before `Return("lib")` | **Yes** — without them the FAP cannot declare the calls |
| 3 | those 5 ST headers | inject `extern "C"` guards (the API table is C++) | **Yes** — otherwise link fails |
| 4 | `targets/f7/furi_hal/furi_hal_region.c` | sub-GHz region/TX unlock | **No** — unrelated, do not port |
| 5 | `scripts/ob.data` | comments out 9 CPU2 secure-region option bytes (`SFSA FSD DDS C2OPT NBRSD SNBRSA BRSD SBRSA SBRV`) so the updater stops comparing them to the Light-stack reference | **Yes for the flash step** — the full stack moves CPU2's secure boundary, so the stock compare fails |
| 6 | `applications/services/loader/application.fam` | adds internal loader headers to the SDK | **No** — CasioLab uses the public loader API |
| 7 | `targets/f7/api_symbols.csv` | enables the chosen `aci_*`/`hci_*` plus `EXPORT_ENABLE` regexes (`^gap_`, `^ble_glue_(start\|stop\|...)`, `^furi_hal_bt_init$`, `^furi_hal_power_insomnia_level$`, optionally `^furi_hal_flash_`) | **Yes, reduced** — see §4 |

Build flags, also part of the change:

```
COPRO_STACK_TYPE=ble_full
COPRO_STACK_BIN=stm32wb5x_BLE_Stack_full_fw.bin
COMPACT=1 DEBUG=0
```

### 2.1 Why patch #1 is required (self-correction)

`ble_central.c` never calls `gap_*`, which is why the first version of this
audit classified the `gap.c` hunk as cosmetic. That was wrong. The role is not
a GAP API argument — it is passed to the **stack** at init:

```
ST ble_gap_aci.h, aci_gap_init():
  @param Role Bitmap of allowed roles.
        0x01 Peripheral, 0x02 Broadcaster, 0x04 Central, 0x08 Observer

ST ble_defs.h:
  GAP_PERIPHERAL_ROLE 0x01
  GAP_BROADCASTER_ROLE 0x02
  GAP_CENTRAL_ROLE    0x04
  GAP_OBSERVER_ROLE   0x08
```

Momentum (`gap.c`, `aci_gap_init` call) passes `GAP_PERIPHERAL_ROLE` = `0x01`.
Blue++ passes `0x01 | 0x04 | 0x08` = `0x0D`. Observation and connection
procedures need the corresponding bits; without them the stack is initialised
peripheral-only and the ACI procedures have no role to run in.

So the patch is **one line and required**. Whether the ST stack rejects the
procedure outright or merely refuses to establish the link is a Gate A/B
measurement, but the role bitmap must be widened in any case.

`BLE_API=all|min|<list>` controls how many ST commands are exported. `min`
derives the list by regex-scanning the app sources for `aci_*`/`hci_*` calls —
the same method used in §4 below. README prices it at **~1.5 KB** of flash.

---

## 3. What Momentum already has (this is the good news)

Checked directly in `Momentum-Firmware-dev`:

| Capability | State in Momentum |
|---|---|
| ST ACI implementation compiled into `libstm32wb` | **Present**: `stm32wb_copro/wpan/ble/core/auto/ble_gap_aci.c`, `ble_gatt_aci.c`, `ble_hci_le.c`, `ble_hal_aci.c`, `ble_l2cap_aci.c` are all in `sources` in `lib/stm32wb.scons` |
| Firmware already calls ACI | **Yes**: `gap.c` calls `aci_hal_set_tx_power_level`, `aci_gatt_init`, `aci_gap_init`, `aci_gatt_update_char_value` |
| `ble_event_dispatcher_register_svc_handler` | `Function,+` — **already exportable to a FAP** |
| `furi_hal_bt_get_radio_stack` | `Function,+` — the FAP can test `FuriHalBtStackFull` |
| `furi_hal_bt_stop_advertising` / `_start_advertising` / `_is_active` | all `Function,+` — radio ownership is already controllable |
| `furi_ble/event_dispatcher.h` | `Header,+` — in the SDK |

So three of the four things `ble_central.c` needs at runtime are **already
exported in the current 87.1 firmware**. The missing piece is only the command
surface.

---

## 4. The exact missing surfaces

### 4.1 Symbols

`targets/f7/api_symbols.csv` (Momentum, 3 715 `Function` rows, 2 623 exported)
contains **zero** `aci_*` entries. The only ST-ish row is:

```
Function,-,hci_send_req,int,"hci_request*, uint8_t"
```

`ble_central.c` needs exactly 14, extracted by regex
`\b(aci_|hci_)\w+\s*\(`:

```
aci_gap_create_connection          aci_gatt_disc_all_primary_services
aci_gap_start_observation_proc     aci_gatt_disc_all_char_of_service
aci_gap_terminate_gap_proc         aci_gatt_disc_all_char_desc
aci_gatt_exchange_config           aci_gatt_read_char_value
aci_gatt_read_long_char_value      aci_gatt_write_char_value
aci_gatt_write_without_resp        aci_gatt_write_char_desc
aci_gatt_confirm_indication        hci_disconnect
```

CasioLab needs a subset — no indication confirm, no long read — but staying
with the tested 14 is the safer choice for a first port.

### 4.2 Headers

`lib/stm32wb_copro` is a submodule (`flipperdevices/stm32wb_copro.git`) and the
local extraction is **empty** (`0` entries). Nothing can be built until
`git submodule update --init lib/stm32wb_copro` runs. The headers CasioLab
needs are all under `wpan/ble/core/auto/`.

### 4.3 Copro image

`ble_full` = `stm32wb5x_BLE_Stack_full_fw.bin`, 151 732 B vs 116 956 B now
(+34 776 B). This is the only step that touches the device irreversibly.

---

## 5. Risk list, ranked by what can actually hurt

| Risk | Evidence | Mitigation |
|---|---|---|
| **Copro flash window** | +34 776 B into C2 flash; `COPRO_STACK_ADDR=0x0` is "let scripts calculate" | Gate A: build, then read the computed address and compare to `ob.data` SFSA before flashing |
| **Option-byte validation** | Blue++ had to disable 9 CPU2 OB comparisons; if we skip that step the update aborts *after* the radio is installed | Port patch #5 verbatim (it is data, not code) |
| **API version break** | Adding symbols changes the API hash; the FAP then only loads on this firmware | Rebuild SDK + FAP together; never ship the FAP alone |
| **Event dispatcher coexistence** | FAP registers a svc handler on a dispatcher the BT service also uses | Gate B2: measure with the phone link up, then down, then after FAP exit |
| **RAM** | Blue++ app uses `stack_size=4 * 1024`; Momentum free heap was 106 128 B | Measure `free` before/after `bc_alloc` |
| **No pairing** | 0 hits for smp/pairing/bond in `libble`; README confirms | Accept: first target is unencrypted name/`0x2A00`. Pairing is P8, separate module |
| **Two links max** | README limitations, firmware BLE config | Accept: CasioLab needs one |
| **License** | Blue++ GPL-3.0-or-later, Momentum GPL-3.0; CasioLab has **no LICENSE file** | If `libble` is copied, CasioLab must become GPL-3.0-or-later with attribution. Do not copy before the license is chosen |

---

## 6. Revised port plan (replaces §3.B of `BLE_CENTRAL_PLAN.md`)

| Step | Work | Touches device? |
|---|---|---|
| **M1** | `git submodule update --init lib/stm32wb_copro` — get the ST headers | no |
| **M2** | Build unmodified `mntm-dev` (Phase P1) — proves the toolchain before any patch | no |
| **M3** | Patch `gap.c` role bitmap (`0x01` → `0x0D`) + `lib/stm32wb.scons` (6 headers) + `extern "C"` guards + `api_check` + enable the 14 symbols. **No sub-GHz unlock, no loader headers** | no |
| **M4** | Build with `COPRO_STACK_TYPE=ble_full`; do not flash. Record computed `COPRO_STACK_ADDR` and the `ob.data` SFSA | no |
| **M5** | Port `libble/ble_central.{c,h}` into CasioLab as `ble/central.*` behind the existing `ble_backend` contract; **delete `ble_names`** (UI-only) | no |
| **M6** | Compile-check the FAP against the new SDK; link must resolve all 14 | no |
| **M7** | Flash: needs explicit user go + ob.data patch + rollback rehearsal | **yes** |

M1–M6 are reversible and cost nothing but disk. M7 is the gate.

---

## 7. What this audit does **not** establish

- That the full stack **fits** this board's C2 flash window (needs M4).
- That a FAP-owned dispatcher survives Momentum's BT service (needs M7).
- That the DW-H5600 accepts a connection without pairing (needs a real
  peripheral test; the SMP corpus suggests bonding is expected but does not
  say the characteristics are gated).
- That `libble` compiles unmodified against API 87.1 (it targets official
  1.4.3; Momentum's headers differ).

Each of these is a gate, not an assumption.

---

## 8. Verdict

Blue++ is **not** a firmware to install and **not** a codebase to port
wholesale. It is a small, readable patch set plus one 20 KB C file, and its
honest limitations (no pairing, compile-only validation) match what the SMP
corpus already told us.

The cheapest viable path to a Flipper-native central is:

**ST headers → 14 exported symbols → `ble_full` copro → `ble_central.c` in the
FAP.**

Three of those four are build-time only. The fourth is a copro flash that must
be measured before it is attempted.
