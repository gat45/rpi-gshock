# AUDIT_REPOSITORY — izivkov/gshock_api + GShockAPI

## Repositories
- https://github.com/izivkov/gshock_api (Python, MIT) — primary
- https://github.com/izivkov/GShockAPI (Kotlin, Android) — complementary

## Key findings
1. Transport: Bleak (Python) / Android BLE (Kotlin). Not portable to Flipper FAP.
2. Protocol: Casio proprietary over GATT characteristics 0x26eb0023-0030.
3. Command IDs: 0x09-0x47 (see PROTOCOL_CATALOG.md).
4. DW-H5600 confirmed support: app notifications (send_app_notification).
5. LifeLog: step_counter_io.py + h5600_lifelog.json — fragmentation via DRSP/Convoy.
6. Functional architecture (v2.0.38+): composable transforms, monoids.

## DW-H5600 specific
- App notifications: send_app_notification (to be fully analyzed).
- LifeLog step counter data via DRSP (0x26eb0023) + Convoy (0x26eb0024).

## Not confirmed for DW-H5600
- ABL-100 activity formats (different model family).
- All alarm/timer/world cities (generic, needs validation).

## License
MIT — preserve attribution in ported code.