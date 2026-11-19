# AUDIT CROSS-CHECK — every claim re-tested against code

Third-party audits are treated here like any other source: re-checked, not
relayed. Each row was verified against a file on disk or a raw upstream file,
fetched on 2026-11-19. Verdicts are `CONFIRMED`, `WRONG`, `INCOMPLETE` or
`UNVERIFIED`.

Repos read (no clone, no write): `Next-Flip/Momentum-Firmware@dev` (local tree
`E:\temp\opencode\Momentum-Firmware-dev`, API 87.1), `andydixon/flipper-bluepp@main`,
`RfidResearchGroup/ChameleonUltra@main`, `flipperdevices/stm32wb_copro@master`.
Evidence corpus read-only: `F:\freebuff\gshock\DW-H5600-EMU\analysis\btsnoop\`.

---

## 1. Momentum — claims

| # | Claim | Verdict | Evidence |
|---|---|---|---|
| M1 | Exported `aci_*` = 0, exported `hci_*` = 0 | **CONFIRMED** | `api_symbols.csv`: sole ST-ish row is `Function,-,hci_send_req,int,"hci_request*, uint8_t"`. No `aci_*` row exists at all |
| M2 | `gap_init`, `hci_send_req`, `ble_app_init` are `-` | **CONFIRMED** | same file, three `Function,-` rows |
| M3 | `ble_gatt_service_add`, `ble_gatt_characteristic_init/update` are `+` | **CONFIRMED** | `Function,+,ble_gatt_service_add,_Bool,"uint8_t, const Service_UUID_t*, uint8_t, uint8_t, uint16_t*"` and the two characteristic rows |
| M4 | `ble_gatt_service_add_characteristic` exported | **WRONG** | No such symbol. The real set is `ble_gatt_characteristic_init`, `_update`, `_delete`. The audit invented a function name |
| M5 | Momentum initialises GAP as Peripheral only | **CONFIRMED** | `targets/f7/ble_glue/gap.c`: `aci_gap_init(GAP_PERIPHERAL_ROLE, 0, strlen(name), …)` |
| M6 | `GapConfig` has `bonding_mode` and `pairing_method`, no role field | **CONFIRMED** | `gap.h:81` `bool bonding_mode;`, `:82` `GapPairing pairing_method;`, `:84` `char adv_name[…]`; struct ends at `:86`, no role member |
| M7 | Key storage exists and is exported | **CONFIRMED** | 11 rows `Function,+,bt_keys_storage_*` (`alloc`, `free`, `load`, `update`, `delete`, `is_changed`, `get_root_keys`, `set_*_path`, `set_ram_params`) + `Header,+,…/bt_keys_storage.h` |
| M8 | Momentum sends a **slave** security request | **CONFIRMED** | `gap.c:198`: `if(gap->config->pairing_method != GapPairingNone) { aci_gap_slave_security_req(event->Connection_Handle); }` |
| M9 | The audit does not say which SC mode Momentum requests | **INCOMPLETE** | `app_conf.h:23`: `#define CFG_SC_SUPPORT SC_PAIRING_OPTIONAL` — the stack **permits** SC and negotiates down. A central port does not need to disable it; the watch already answers SC=0 |

### 1.1 Exact `GapPairing` enum (missing from the audit)

```c
typedef enum {
    GapPairingNone,                 // Just Works path
    GapPairingPinCodeShow,
    GapPairingPinCodeVerifyYesNo,
    GapPairingCount,
} GapPairing;
```

`GapPairingNone` is the branch that sets `MITM_PROTECTION_NOT_REQUIRED` and
`USE_FIXED_PIN_FOR_PAIRING_ALLOWED` (`gap.c` ~400) — i.e. the Flipper already
contains a Just Works code path, on the **peripheral** side, behind `gap_init`,
which a FAP cannot call. Worth reusing as a reference when the central pairing
module is written.

---

## 2. Blue++ — claims

| # | Claim | Verdict | Evidence |
|---|---|---|---|
| B1 | `bc_scan_start` calls `aci_gap_start_observation_proc(0x0050, 0x0030, 0x01, OWN_ADDR_TYPE, 0x00, 0x00)` | **CONFIRMED** verbatim | `libble/ble_central.c` |
| B2 | `bc_connect` calls `aci_gap_create_connection(0x60, 0x30, addr_type, addr, OWN_ADDR_TYPE, 0x18, 0x28, 0, 0x01F4, 0, 0)` | **CONFIRMED** verbatim | same |
| B3 | Role filter `if(e->Status == 0 && e->Role != 0) return BleEventNotAck;` | **CONFIRMED** verbatim | same |
| B4 | 14 ST commands are used (scan, connect, discover, read, write, CCCD) | **CONFIRMED** | regex census in §4 |
| B5 | Blue++ has no central pairing | **CONFIRMED** | grep: **0** hits for `smp`, `pairing`, `bond`, `encrypt`, `mitm`, `passkey`, `ltk`, `security` in `ble_central.c`; README limitations say the same |
| B6 | Blue++ copies "all roles" into `gap.c` | **CONFIRMED** and **more important than the audit says** — see §2.1 |
| B7 | Blue++ is GATT-server "fixed serial service, echoes writes" | **CONFIRMED** | README app docs; `ble_gattsrv` is out of scope for CasioLab |

### 2.1 New finding — the role patch is not cosmetic

ST documents the parameter, and the constants are hard numbers:

```
ble_gap_aci.h, aci_gap_init():
  @param Role Bitmap of allowed roles.
        0x01 Peripheral, 0x02 Broadcaster, 0x04 Central, 0x08 Observer

ble_defs.h:
  GAP_PERIPHERAL_ROLE 0x01
  GAP_BROADCASTER_ROLE 0x02
  GAP_CENTRAL_ROLE    0x04
  GAP_OBSERVER_ROLE   0x08
```

Momentum passes `0x01`. Blue++ passes `0x01 | 0x04 | 0x08` = `0x0D`. An
audit that lists the role change as "one of four optional things" is wrong:
without bits `0x04`/`0x08`, the stack is initialised peripheral-only and the
observation/connection procedures have no role to execute in. **This corrects
my own `BLUE_PLUS_PLUS_AUDIT.md`, which had classified that hunk as "not
needed".**

---

## 2.2 Event plumbing — the deepest unverified layer, now read

This is where a port usually dies, so it was read line by line.

**Who pumps events.** `ble_app.c:88` installs the handler with
`hci_init(ble_app_hci_event_handler, &hci_tl_config)`; the handler is defined at
`:143` and calls `ble_event_dispatcher_process_event` at `:148`. It is installed
by `ble_app_init()`, which `ble_glue.c:235` calls during stack start — i.e. at
boot, before any FAP can run. The chain is therefore already live when the app
starts.

**What the dispatcher does.** `furi_ble/event_dispatcher.c` keeps a plain list
and calls **every** registered handler with **every** HCI event:

```c
for(GapSvcEventHandlerList_it(it, handlers); …; GapSvcEventHandlerList_next(it)) {
    ack_status = item->callback(payload, item->context);
    if(ack_status == BleEventNotAck) continue;      // keep going
    else if(ack_status == BleEventAckFlowEnable || BleEventAckFlowDisable) break;
}
/* Handlers for client-mode events are also to be implemented here. But not today. */
if(ack_status == BleEventNotAck) return ble_event_app_notification(payload);
```

**Reading of that comment.** It does **not** say client-mode events are
dropped. It says the *routing/filter* stage is absent — every handler sees
everything, and the handler is responsible for recognising its own events.
That is exactly what Blue++ does: `if(!bc->connected || ch != bc->conn_handle)
return BleEventNotAck;`, and it only acks on `HCI_LE_META_EVT_CODE`
(advertising report, connection complete) and ACI vendor events.

So the ACI event path to a FAP **exists today**. What does not exist is the
role, the stack and the exported commands.

Four consequences that belong in the port plan:

| # | Fact | Consequence |
|---|---|---|
| E1 | The callback runs on the **BLE glue thread**, not the FAP thread | Handler must be non-blocking: set event flags, copy data, return. Blue++ does exactly this |
| E2 | `BleEventAckFlowEnable` **stops propagation** — the firmware GAP profile never sees the event | Acks must be provably ours. A false ack on a peripheral link breaks the phone connection |
| E3 | `ble_event_dispatcher_reset()` contains `furi_check(GapSvcEventHandlerList_size(handlers) == 0)` | A FAP that exits (or crashes) without unregistering makes the next `gap_init`/`gap_thread_stop` **panic the device**. Unregister on every path, including error paths |
| E4 | The BT service's peripheral profile and a FAP handler coexist on the same dispatcher; `CFG_BLE_NUM_LINK 2` (`app_conf.h:43`) | A Flipper-side proxy (peripheral to phone **and** central to watch) is architecturally plausible **on the Flipper alone** — see §6.1 |

E4 is an architectural correction to the "two devices required" verdict, not a
capability claim: it is a gate, not a result.

---

## 3. Chameleon Ultra — claims

Repo `RfidResearchGroup/ChameleonUltra@main` (pushed 2026-11-11).

| # | Claim | Verdict | Evidence |
|---|---|---|---|
| C1 | `NRF_SDH_BLE_PERIPHERAL_LINK_COUNT 1` | **CONFIRMED** | `firmware/application/src/sdk_config.h` |
| C2 | `NRF_SDH_BLE_CENTRAL_LINK_COUNT 0` | **CONFIRMED** | same |
| C3 | `NRF_SDH_BLE_TOTAL_LINK_COUNT 1` | **CONFIRMED** | same |
| C4 | `BLE_NUS_C_ENABLED 0` | **CONFIRMED** | same |
| C5 | `PM_CENTRAL_ENABLED 1` but no central link | **CONFIRMED** | same file; peer manager compiled, link count 0 |
| C6 | Entry point is `ble_slave_init()` with `ble_stack_init / gap_params_init / gatt_init / services_init / advertising_init / conn_params_init / peer_manager_init` | **CONFIRMED** verbatim | `ble_main.c` |
| C7 | `BLE_NUS_DEF(m_nus, NRF_SDH_BLE_TOTAL_LINK_COUNT)` | **CONFIRMED** verbatim | same |
| C8 | `SEC_PARAMS_BOND 1`, `SEC_PARAMS_MITM 1`, `SEC_PARAMS_LESC 1`, IO caps `DISPLAY_ONLY` | **CONFIRMED** (MITM=1 is the live branch: `#define LESC_MITM_NC 1`) | same |
| C9 | The app never calls a central primitive | **CONFIRMED** | call census in `ble_main.c`: `sd_ble_gap_adv_stop`, `device_name_set`, `disconnect`, `phy_update`, `ppcp_set`, `sec_params_reply`, `gatts_sys_attr_set`, `opt_set`. **No** `sd_ble_gap_connect`, **no** `sd_ble_gattc_*`, **no** `ble_db_discovery_*`, **no** `nrf_ble_gattc_*`, **no** `nrf_ble_scan_*` |
| C10 | "Central générique à valider" | **WRONG as phrased** | It is not "to validate": it is **not present in the active build**. The SDK ships `nrf_ble_scan/`, `ble_nus_c/`, and `sd_ble_gap_connect` exists, but no application path uses them |

`ble_main.c` contains **0** occurrences of `ble_nus_c` — the client is compiled
into the SDK, never referenced by the firmware.

---

## 4. The 14 ST commands Blue++ needs (census)

```
aci_gap_start_observation_proc     aci_gatt_disc_all_primary_services
aci_gap_create_connection          aci_gatt_disc_all_char_of_service
aci_gap_terminate_gap_proc         aci_gatt_disc_all_char_desc
aci_gatt_exchange_config           aci_gatt_read_char_value
aci_gatt_read_long_char_value      aci_gatt_write_char_value
aci_gatt_write_without_resp        aci_gatt_write_char_desc
aci_gatt_confirm_indication        hci_disconnect
```

All 14 are absent from Momentum's `api_symbols.csv`. The `.c` files that
define them (`ble_gap_aci.c`, `ble_gatt_aci.c`, `ble_hci_le.c`,
`ble_hal_aci.c`, `ble_l2cap_aci.c`) are **already in `lib/stm32wb.scons`'s
`sources`**, so the code is linked into the firmware today — only the table
entry is missing.

---

## 5. SMP — the audit's correction is right, its proof is weak

| # | Claim | Verdict | Evidence |
|---|---|---|---|
| S1 | 16-byte `PAIRING_CONFIRM` does not by itself distinguish legacy from SC | **CORRECT** | Secure Connections also uses a 16-byte confirm; the public-key difference is carried by separate PDUs |
| S2 | `AuthReq = 0x01` in the response is the decisive proof | **INCOMPLETE** | True but weaker than available. See S3 |
| S3 | The corpus contains **no** 0x0C / 0x0D | **NEW, decisive** | Opcode census of `smp_pdu_bruts.txt` below |

### 5.1 Opcode census (the strongest available proof)

| Opcode | Name | Count |
|---|---|---|
| 0x01 | Pairing Request | 1 |
| 0x02 | Pairing Response | 1 |
| 0x03 | Pairing Confirm | 12 |
| 0x04 | Pairing Random | 12 |
| 0x06 | Encryption Information (LTK) | 12 |
| 0x07 | Central Identification (EDIV + RAND) | 12 |
| 0x08 | Identity Information (IRK) | 2 |
| 0x09 | Identity Address Information | 7 |
| 0x0B | Security Request | **147** |
| 0x0C | Pairing Public Key | **0** |
| 0x0D | Pairing DHKey Check | **0** |

Secure Connections cannot complete without 0x0C and 0x0D. Zero occurrences in
11 captures is a direct, structural proof of **legacy pairing** — stronger
than reading one bit.

### 5.2 New operational finding — the watch nags

`0x0B` appears **147 times**, always `0B 01`, always **watch → phone**
(auth_req `0x01` = bonding, no MITM, no SC). The watch repeatedly requests
security until the peer starts pairing. A central that ignores that PDU will
be asked 147 times in these captures and, in practice, will be disconnected or
left unable to use protected attributes.

### 5.3 What the audit said correctly about the parameters

```
PAIRING_REQUEST   01 04 00 2D 10 0F 0F
  io_cap 0x04 KeyboardDisplay, auth 0x2D (bonding|MITM|SC), key 16, dist 0x0F

PAIRING_RESPONSE  02 03 00 01 10 03 03
  io_cap 0x03 NoInputNoOutput,  auth 0x01 (bonding only), key 16, dist 0x03
```

`0x03` = EncKey | IdKey. KeyboardDisplay vs NoInputNoOutput with MITM absent
on the responder ⇒ **Just Works**. Confirmed.

---

## 6. Proxy section — claims

| # | Claim | Verdict | Evidence |
|---|---|---|---|
| P1 | A proxy needs two independent links | **CONFIRMED** by construction | it is an architectural statement, not a code fact |
| P2 | Encrypted ATT payloads cannot be relayed opaquely | **CONFIRMED** | each link derives its own session keys from its own SMP; ciphertext is link-local |
| P3 | Flipper as proxy needs Peripheral + Central at once | **CONFIRMED as requirement**, **UNVERIFIED as capability** | `CFG_BLE_NUM_LINK 2` (`app_conf.h:43`) allows 2 links; whether Momentum's profile model can host a GATT server and a central concurrently is a Gate B2 measurement |
| P4 | Blue++ cannot be the proxy today | **CONFIRMED** | its BLE apps stop advertising and drop the phone link while they own the radio (README + `bt_inspector.c` lines 1251–1269) |

### 6.1 The "two devices" verdict is too strong for the Flipper

The audit concludes a proxy needs Flipper **plus** Chameleon because "the
Flipper unique solution is not viable with the current Momentum APIs". The
"current APIs" part is true. The "unique solution is not viable" part does not
follow from the code:

- `CFG_BLE_NUM_LINK 2` — the controller is configured for two links
  (`app_conf.h:43`).
- The BT service keeps its peripheral profile running independently of any FAP.
- A FAP registers on the same dispatcher and receives **all** events (§2.2).

So the shape "Flipper = peripheral to the phone (BT service serial profile)
**and** central to the watch (FAP handler)" is not excluded by anything read.
It is excluded **today** only by the three missing layers (stack, roles,
symbols). Whether the two roles truly coexist without starving each other is a
measurement — Gate B2 — not a design impossibility.

What *is* certain: encrypted ATT payloads still cannot be relayed opaquely
(P2), so the proxy must terminate both links at the application level.

---

## 7. Corrections this cross-check forces on our own docs

| Doc | Was | Now |
|---|---|---|
| `BLUE_PLUS_PLUS_AUDIT.md` §2 | `gap.c` role patch = "No" | **Yes**, required: `0x01` → `0x0D`, with ST constants as proof |
| `BLUE_PLUS_PLUS_AUDIT.md` §6 M3 | "no `gap.c` patch" | role bitmap patch included |
| `BLE_CENTRAL_PLAN.md` §4 | "confirm PDUs are 17 bytes ⇒ legacy" | opcode census: no 0x0C/0x0D ⇒ legacy; 0x0B ×147 documented |
| `BLE_CENTRAL_PLAN.md` §3.B | "only headers + export + copro" | adds the role bitmap |
| `PORTING_PLAN.md` Phase H | "`gap.c` does not need to be touched" | corrected |
| both `SKILL.md` | same confirm-length claim | corrected to the opcode census |

---

## 8. Still unverified — do not state these as facts

| Question | Why it is still open | How to close |
|---|---|---|
| Does Momentum's profile model accept a GATT server **and** a central at once? | `CFG_BLE_NUM_LINK 2` only says the controller allows two links; nothing in the code proves the service can host both roles | Gate B2 on device |
| Does a FAP handler survive the BT service's own event traffic without breaking the phone link? | the dispatcher is shared and a wrong ack swallows events (E2) | Gate B2: phone connected, FAP active, measure disconnects |
| Can a FAP reliably unregister on crash paths? | `reset()` panics if the list is not empty (E3) | M7: test an intentional abort mid-session |
| Do the 14 symbols link into a FAP without pulling 20 KB of dead code? | export granularity is per symbol; `BLE_API=min` prices it at ~1.5 KB, unmeasured here | M6 link map |
| Does the DW-H5600 gate its characteristics behind encryption? | the captures show pairing, not which attributes require it | read the ATT error codes in the 5 CSVs, or test after P8 |
| Does `libble` compile against API 87.1? | it targets official 1.4.3 | M6 |
| Does the full stack fit the C2 flash window? | needs a real build with `ob.data` inspected | M4 |

---

## 9. Net effect on the plan

No phase changes. Three corrections:

1. **Bigger on the firmware side** — the GAP role bitmap (`0x01` → `0x0D`) is
   mandatory, not cosmetic. I had it wrong in the first Blue++ audit.
2. **Smaller on the evidence side** — pairing is legacy by *structural*
   absence of 0x0C/0x0D, not by confirm length. The earlier "17 bytes ⇒
   legacy" argument was invalid and is now removed everywhere.
3. **New architectural option** — the dispatcher model and
   `CFG_BLE_NUM_LINK 2` leave open a **single-device** Flipper proxy. It is a
   gate, not a promise, but the "two devices mandatory" verdict does not follow
   from the code.

The 14-symbol list, the SDK header list, the `ob.data` secure-region patch, the
role bitmap and the `ble_full` copro image are the full firmware delta.
