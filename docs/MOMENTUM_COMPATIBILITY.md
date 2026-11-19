# MOMENTUM_COMPATIBILITY

## Firmware target: Momentum (user's device)

### BLE capabilities in Momentum (for FAP)
- Momentum keeps the Flipper's lightweight BLE stack.
- FAPs get **peripheral role** only.
- No Central GATT API exposed to FAPs (no scan, no connect, no read/write as client).
- Standard Flipper firmware has the same limitation.

### Impact on CasioLab
- Cannot connect to DW-H5600 directly from a Flipper FAP.
- Cannot discover GATT services/characteristics from the FAP.
- Cannot subscribe to notifications as a client.

### Adapted architecture
- Flipper (Momentum) FAP: UI, storage, session management, decoding, analysis, export.
- Raspberry Pi: GATT central (scan, connect, discover, read, notify, write).
- Common capture format shared between both.
- Proxy mode (bidirectional) deferred to later phase — requires dual radio role validation.

### Alternative
Use a custom firmware with full BLE stack (Blue++) for Central GATT, but requires leaving Momentum. Not recommended for now.

### Verified
- Momentum version: TBD (user to confirm in Settings → About)
- FAP API: standard Flipper FAP API (ufbt)

---

# Correction (2026-11-19)

## The limit is now proven, not assumed

`f7_sdk/targets/f7/api_symbols.csv` exports BLE **server** symbols only:
`ble_gatt_service_add`, `ble_gatt_characteristic_*`, profiles, advertising.
There is **no GAP scan and no GATT client**. A FAP therefore cannot be the
central, on this firmware, regardless of how the code is written.

Build facts: `ufbt` → `dist/casio_lab.fap`, `Target: 7, API: 87.1` (local
patch of `api_symbols.csv`; `ufbt update` reverts it).

## Architecture, corrected

- **Raspberry Pi as GATT central: rejected by the user.** It is not part of the
  design. The central is `CasioLab/companion/watchlink.py`, run on any host
  with a Bluetooth radio (PC, phone, RPi if one is at hand).
- The FAP decodes, stores, exports, and reads `/ext/casiolab/live.txt` that the
  companion pushes. It never opens a BLE link to the watch.
- "Proxy mode deferred" is in fact **impossible**, not deferred: a proxy needs
  both a central and a peripheral role in the same device.

## What "verified" means here

- The firmware limit: verified against the exported symbol list.
- The FAP: builds, deploys, runs, and has produced sessions
  `session_0001..0013` on the SD card.
- The companion: `ReplayTransport` verified against real captures;
  `BleTransport` written but **not exercised** (no Bluetooth adapter on the
  build host, `BleakBluetoothNotAvailableError`).

---

# Measured 2026-11-19 — the installed copro stack is BLE **Light**

This settles the "Momentum + BLE Full" question with bytes, not with README
claims.

## Device

```
firmware_version   mntm-dev          firmware_commit  8ed809fb
firmware_branch    dev               firmware_build   03-06-2026
firmware_api       87.1              firmware_target  7
radio_mode         Stack             radio_stack_type 3
radio_stack        1.20.0            radio_stack_flash 116K
radio_stack_sram2b 19K              radio_stack_sram2a 14K
```

## Proof of which stack runs

| Artefact | Size | SHA-256 |
|---|---|---|
| `radio.bin` inside `flipper-z-f7-update-mntm-012.zip` | 116 956 | `134a9af4…d327df` |
| ST CubeWB **v1.20.0** `stm32wb5x_BLE_Stack_light_fw.bin` | 116 956 | `134a9af4…d327df` |

**The two files are byte-identical** (`st == local` in the comparison, full
hash equality). The coprocessor therefore runs the **light** stack from
CubeWB 1.20.0, not the full one. For reference, the same release's full
binary is 151 732 bytes — nobody is running it here.

`radio_stack_type: 3` is the raw ST `StackType` value (`furi_hal_info.c` prints
`ble_c2_info->StackType` verbatim). In this package, light = `0x03`; the number
alone is not proof, which is why the hash comparison matters.

## What Momentum can build, and what it cannot

`fbt_options.py` (dev):

```python
COPRO_STACK_BIN  = "stm32wb5x_BLE_Stack_light_fw.bin"
COPRO_STACK_TYPE = "ble_light"
# Firmware also supports "ble_full", but it might not fit into debug builds
```

So building a **full-stack firmware** is a supported option, not a hack. But
the stack is only one of three missing layers.

## Layer 2 — the GAP port has no central code

`targets/f7/ble_glue/gap.h` exposes exactly:

```c
gap_init, gap_start_advertising, gap_stop_advertising,
gap_get_state, gap_thread_stop, gap_emit_ble_beacon_status_event
```

No scan, no connect, no service/characteristic discovery, no GATT client
read/write. Switching the copro to full does not create those functions: the
C side that drives them does not exist in Momentum.

## Layer 3 — the API table hides what does exist

`targets/f7/api_symbols.csv` (API 87.1) marks:

```
Function,-,gap_init,…
Function,-,gap_start_advertising,…
Function,-,hci_send_req,…
Function,-,ble_app_init,…
```

`-` means present in the firmware but **not resolvable by a FAP**. Exported
(`+`): `ble_gatt_service_add`, `ble_gatt_characteristic_init/update`,
`furi_hal_bt_start_app`, `furi_hal_bt_is_gatt_gap_supported`.

## Consequence for the plan

A Flipper-native central needs all three, in this order:

1. **Copro**: build with `ble_full`, which changes `radio.bin` and therefore
   requires flashing the C2 (DFU), not just installing a `.fap`.
2. **GAP/GATT client**: port the central side (Blue++ is the reference), or
   write it; Momentum has none.
3. **API export**: move the needed symbols from `-` to `+` in
   `api_symbols.csv` and rebuild the firmware + SDK.

Plus two limits that survive all three: the FAP must take ownership of the
radio (the BT service stops advertising), and central-side pairing/bonding is
not implemented in Blue++.

**Nothing has been flashed.** The installed `mntm-dev` is untouched; the
current CasioLab FAP was deployed over USB and runs.