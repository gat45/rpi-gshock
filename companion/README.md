# RPi Companion — BLE Central for CasioLab

## Purpose
The Flipper Zero (Momentum firmware) cannot act as a BLE Central.
This companion runs on a Raspberry Pi and handles all BLE communication
with the DW-H5600, forwarding data to the Flipper via serial USB.

## Setup
```bash
pip install gshock-api bleak pyserial
python3 rpi_companion.py --flipper-port /dev/ttyACM0
```

## Architecture
```
DW-H5600 <--BLE--> Raspberry Pi <--USB serial--> Flipper Zero (CasioLab FAP)
```

## Data flow
1. RPi scans and connects to DW-H5600
2. RPi discovers GATT services/characteristics
3. RPi subscribes to notifications (HR, activity, etc.)
4. RPi sends JSON data to Flipper via serial
5. Flipper FAP displays, stores, decodes, exports

## Files
- `rpi_companion.py` — main companion script
- Uses `gshock_api` (izivkov) for protocol implementation