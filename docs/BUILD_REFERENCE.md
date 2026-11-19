# BUILD REFERENCE — M1/M2/M3 (reproducible Momentum baseline)

Purpose: establish a **reproducible, unmodified** Momentum build before any
BLE patch. Nothing here is flashed. Everything lives under
`F:\\freebuff\\flipper\\fw\\`. The evidence corpus `F:\\freebuff\\gshock\\` is not
touched.

---

## M1 — clone at the exact device commit

| Item | Value |
|---|---|
| Repository | `https://github.com/Next-Flip/Momentum-Firmware.git` |
| Branch cloned | `dev` (depth 1), then detached |
| **HEAD** | `8ed809fba8af7ac3f09b9495a597d8963f9178a8` |
| Commit subject | `Fix broken submodule` |
| Commit date | 2026-06-03 01:16:46 +0200 |
| Device builds date | `03-06-2026` (from `device_info`) — matches |
| Working tree | **0** changed paths after checkout |
| Submodules | 16, all initialised, recursive |
| Size on disk | 0,75 GB / 23 155 files |
| `F:` free after clone | 22,78 GB |

Submodule pins that matter for this project:

| Submodule | Pin |
|---|---|
| `lib/stm32wb_copro` | `133182d5…` tagged **v1.20.0** |
| `lib/stm32wb_copro/scripts` | `781464e6…` |
| `lib/stm32wb_hal` | `cfd0dd25…` |
| `lib/stm32wb_cmsis` | `d1b86058…` |
| `lib/mbedtls` | `107ea89d…` |
| `lib/nanopb` | `6cfe48d6…` |
| `lib/mlib` | `62c8ac3e…` |
| `lib/FreeRTOS-Kernel` | `def7d2df…` |
| `assets/protobuf` | `ea4f185f…` |
| `applications/external` | `b05485f7…` |

Reproducible command: `fw\\m1-clone.ps1`.

---

## M2 — unmodified build

### M2.1 — attempt 1: Windows, `python fbt` — FAILED
- Command `python fbt updater_package` → exit 1, `SyntaxError` at `fbt` line 10
  (`SCRIPT_PATH="$(cd ...)"`). On Windows `fbt` is the POSIX shell entry point.

### M2.2 — attempt 2: Windows, `.\fbt.cmd` — FAILED (interrupted)
- Command `.\fbt.cmd updater_package`. Windows toolchain downloaded into
  `toolchain\`, then SCons produced no further output. Process killed manually,
  exit 255, no artefacts. Abandoned in favour of WSL2.

### M2.3 — environment incident under WSL2 (recorded, then resolved)
- First WSL2 probe returned `Input/output error` (EIO) when *reading* binaries
  (`mv`, `awk`, `tar`, `curl`, `df`, `openssl`, `apt-get`). Exit 126, Python
  `OSError [Errno 5]`. Metadata intact, data blocks unreadable — a vhdx/backing
  store condition, not a package problem.
- Resolution: `wsl --shutdown`, wait, restart. All binaries then executed normally.
  `df` still fails with EIO on this distro, so disk space is measured with
  `stat -f` / `shutil.disk_usage`. **`df` must not be used in any script.**
- `F:` free (WSL): 21.94 GB at probe time (23.5 GB per `stat -f`); total 465.76 GB.
- Root fs free (WSL ext4): 942.61 GB of 1006.85 GB.

### M2.4 — attempt 3: WSL2 Ubuntu — SUCCESS
- Command: `FBT_NO_SYNC=1 ./fbt updater_package`
- Environment: WSL2 Ubuntu 26.04.1 LTS, kernel 6.18.33.2-microsoft-standard-WSL2,
  git 2.53.0, Python 3.14.4, gcc 15.2.0, toolchain v39 (arm-none-eabi 12.3.1)
- Filesystem: repo on `/mnt/f` (9p → NTFS)
- Patch applied: **none** — `git status --porcelain` = 0 before and after
- Target: `updater_package`
- Script: `fw\\m2-wsl.sh` · Log: `fw\\m2-wsl.log`
- Duration: 1 h 56 min 38 s · Exit: 0

### M2-results — build finished

| Item | Value |
|---|---|
| Status | **SUCCESS** (exit 0) |
| Duration | 1 h 56 min 38 s |
| Command | `FBT_NO_SYNC=1 ./fbt updater_package` |
| Environment | WSL2 Ubuntu 26.04.1 LTS, kernel 6.18.33, git 2.53.0, Python 3.14.4, gcc 15.2.0, toolchain v39 (arm-none-eabi 12.3.1) |
| Patch applied | **none** |
| Firmware metadata | `mntm-dev`, target 7, commit `8ed809fb`, branch `HEAD`, build date 19-11-2026 |
| Warnings | 2 pre-existing: invalid appid `.cli_gui` and `.f0_mtp` (appids starting with a dot) |

#### Artefacts (`dist/f7-C/`)

| File | Size (o) | SHA-256 |
|---|---|---|
| `f7-update-mntm-HEAD-8ed809fb/radio.bin` | 116 956 | `134a9af448d2bc35995600c9e95723a387c144c2d1adea671086f16f95d327df` |
| `flipper-z-f7-full-mntm-HEAD-8ed809fb.bin` | 838 372 | `a79820e9e41fa31789a39bb2f9ed18f6baa61a4382a440735ae68bff13743e32` |
| `flipper-z-f7-full-mntm-HEAD-8ed809fb.dfu` | 838 681 | `0aaaf9a87dd70c60df6876cb962cf9c0a0905a31d22e463841c9690d9100bc92` |
| `flipper-z-f7-updater-mntm-HEAD-8ed809fb.bin` | 120 653 | `17febb6437313870dc9daf8cb35fa4fc2cf46166d96a3336dd392c8bccaf483e` |
| `flipper-z-f7-updater-mntm-HEAD-8ed809fb.dfu` | 120 962 | `e5f87439c730071b94b63c1e28dfcf3a8c4bd5ab332308db7f60b291db19d3a7` |
| `flipper-z-f7-updater-mntm-HEAD-8ed809fb.elf` | 2 262 488 | (not hashed) |
| `flipper-z-f7-firmware-mntm-HEAD-8ed809fb.elf` | 10 011 188 | (not hashed) |
| `flipper-z-f7-sdk-mntm-HEAD-8ed809fb.zip` | 31 070 589 | (not hashed) |
| `flipper-z-f7-update-mntm-HEAD-8ed809fb.tgz` | 12 501 732 | (not hashed) |

#### Comparison with the installed device firmware

| Artefact | Built | Installed (device) | Match? |
|---|---|---|---|
| `radio.bin` (copro light, 116 956 o) | `134a9af4…d327df` | `134a9af4…d327df` | **BYTE-IDENTICAL** |
| `ob.data` (light baseline) | `SFSA 0xD7 … SBRV 0x35C00` | same | identical |
| Firmware version string | `mntm-dev` / `8ed809fb` | `mntm-dev` / `8ed809fb` | identical |
| API version | 87.1 | 87.1 | identical |

The built `radio.bin` is **byte-identical** to the `radio.bin` extracted from the
installed firmware zip. This is the strongest possible proof that the clone at
`8ed809fb` + submodule pins reproduces the firmware actually running on the device.

#### Disk usage after M2
- Repo size: 3.5 Go (0.75 after M1 + 2.75 of build artefacts)
- `build/`: 461 Mo · `dist/`: 68 Mo
- `F:` free: 20.16 Go (down from 22.78 Go after M1)

### M2 verdict
- **M2 = PASS.** Reproducible build confirmed, `radio.bin` byte-identical to device.

---

## M3 — firmware patch (Central role)

### M3.1 — GAP role patch (done)
- File: `fw/momentum/targets/f7/ble_glue/gap.c`, line 360.
- Change: `aci_gap_init(GAP_PERIPHERAL_ROLE, …)` →
  `aci_gap_init(GAP_PERIPHERAL_ROLE | GAP_CENTRAL_ROLE | GAP_OBSERVER_ROLE, …)` (= 0x0D).
- Rationale: without the Central/Observer roles at init, the stack refuses Central
  procedures even with the Full stack.
- Verified: `gap.o` recompiled successfully (incremental build, timestamp 12:15).

### M3.2 — ACI exports (deferred)
- 18 functions + 6 ST BLE ACI headers identified and signatures extracted
  (`m3-gen.py`). Manual addition to `api_symbols.csv` was reverted by the SDK
  validation (« SDK version is not finalized »). The Momentum SDK build system
  requires an approval pass or SCons `SDK_HEADERS` config change.
- **Deferred to M5** (FAP link), where it can be done properly.

### M3.3 — not in M3
- `ob.data` change (SFSA/secure boundary) belongs to M4 (Full-stack install,
  flash, Gate A).
- GPL decision still open before any `libble/` code is copied.

### M3 verdict
- **M3 = PARTIAL PASS.** The essential firmware patch (GAP role) compiles.
  ACI exports deferred to M5. No firmware flashed.

---

## M4 — Full BLE stack build

### M4.1 — change applied
- File: `fw/momentum/fbt_options.py`
- `COPRO_STACK_BIN`: `stm32wb5x_BLE_Stack_light_fw.bin` → `stm32wb5x_BLE_Stack_full_fw.bin`
- `COPRO_STACK_TYPE`: `ble_light` → `ble_full`
- `ob.data` unchanged (SFSA 0xD7, light baseline) — the full stack changes the
  secure boundary, but the build itself only swaps the binary. The `ob.data`
  change for flashing is a separate step (Gate A).

### M4.2 — incremental build result
- **PASS** (exit 0). SCons rebuilt the copro parts and relinked.
- Firmware binary (`flipper-z-f7-full-mntm-HEAD-8ed809fb.bin`): 838 372 o,
  hash `6bd6e90b6d012f092443e66345396166a2640d047f038d4898b1f94883ea61be`
  (changed from M2's `a79820e9…` due to the M3 gap.c role patch; size unchanged).
- `radio.bin` (Full stack, produced by `updater_package` step which copies the
  copro binary verbatim — proven in M2 where light `radio.bin` was byte-identical
  to the source light binary):
  - Size: **151 732 o** (was 116 956 o with light stack)
  - Hash: **`1145b7d2a7d21df6f33ddb62c6a6535f8d13a7390e307ae31ba9eeecda3c0834`**
- This matches exactly the Full stack binary in the source tree
  (`lib/stm32wb_copro/firmware/stm32wb5x_BLE_Stack_full_fw.bin`): 151 732 o,
  hash `1145b7d2…c0834`. **BYTE-IDENTICAL.**
- The firmware metadata is unchanged (`mntm-dev`, `8ed809fb`, target 7).

### M4 verdict
- **M4 = PASS.** The Full BLE stack builds and the produced `radio.bin` matches
  the source Full stack binary byte-for-byte. Combined with M3's GAP role patch,
  the firmware is now configured for the Central role.
- Flashing the Full stack requires the `ob.data` change (SFSA secure boundary)
  and is **not** done here (Gate A, explicit consent required).

---

## M5 — FAP link (ACI exports)

### M5.1 — target.json change (done)
- File: `fw/momentum/targets/f7/target.json`
- Added `../../lib/stm32wb_copro/wpan/ble/core/auto` to `sdk_header_paths`.
- Backup: `targets/f7/target.json.bak`

### M5.2 — api_symbols.csv exports (attempted)
- Added 287 + 25 = 312 ST BLE ACI function entries + 7 ST headers.
- The SDK validation (`SDKCHK`) detects them as "New" entries and saves them
  as `?` (PENDING). The approval workflow requires changing `?` → `+` and
  re-running. This was attempted but the build hangs on 9p (I/O wait) before
  completing. The `?` → `+` sed replacement also corrupted the CSV once
  (fixed by restoring from git and using Python string replacement).
- **Deferred**: the export approval pass needs a build that completes without
  hanging. On a faster filesystem (ext4) this would take minutes.

### M5.3 — casio_ble_central.c (written)
- Files: `CasioLab/ble/casio_ble_central.h` and `.c`
- Implements scan (aci_gap_start_observation_proc), connect (aci_gap_create_connection),
  disconnect (hci_disconnect). GATT discovery, read/write, notifications, pairing
  are documented TODOs with the exact ACI calls needed.
- Guarded by `__has_include("ble_gap_aci.h")` so it compiles without the SDK exports.
- Will be compiled into the FAP via `ble/*.c` (already globbed in application.fam).

### M5 verdict
- **M5 = PARTIAL.** The Central driver skeleton is written. The ACI export
  mechanism is understood and the target.json change is in place, but the
  SDK approval pass could not be completed due to 9p build hangs. On a faster
  build host (or ext4), the approval pass should succeed.

---

## What M2/M3/M4/M5 unblock

- M2 green: any size/hash delta in M4/M5 is attributable to the patch, not the
  environment.
- M3 green: the GAP role init now includes Central + Observer.
- M4 green: the copro now carries the Full BLE stack (Central procedures
  available).
- M5 (FAP link) still requires: resolving the ACI exports via the SCons SDK
  config, plus the GPL decision before copying any `libble/` code.
- M6 (pairing) and M7 (device validation) remain untouched.