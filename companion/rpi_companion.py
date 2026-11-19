#!/usr/bin/env python3
"""CasioLab RPi companion - periodic BLE poll of the DW-H5600 pushed to the Flipper.

Thin loop over companion/watchlink.py so there is ONE wire format: the flat
`v=1` key=value document that the FAP reads from /ext/casiolab/live.txt.
(The previous version sent JSON to the Flipper CLI, which the FAP never reads.)

Usage:
    python3 rpi_companion.py --flipper-port /dev/ttyACM0 [--watch-address XX:XX:XX:XX:XX:XX]

Requirements:
    pip install bleak pyserial
"""
import argparse
import asyncio
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import watchlink as wl  # noqa: E402

COMMANDS = [wl.CMD_WATCH_NAME, wl.CMD_CONDITION, wl.CMD_ACTIVITY]


async def poll_forever(port: str, address: str | None, interval: float) -> None:
    failures = 0
    while True:
        try:
            data = await wl.collect_ble(address, COMMANDS)
            ok = await asyncio.to_thread(wl.push_to_flipper, port, wl.to_kv(data))
            print(f"[push] {'ok' if ok else 'FAILED'}: {sorted(data)}")
            failures = 0 if ok else failures + 1
        except Exception as exc:  # BLE drop, serial error: log and retry
            failures += 1
            print(f"[error] {type(exc).__name__}: {exc}", file=sys.stderr)
        # linear back-off, capped, so a missing watch does not spin the radio
        await asyncio.sleep(min(interval * (1 + failures), 300))


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--flipper-port", default="/dev/ttyACM0")
    ap.add_argument("--watch-address", default=None, help="omit to scan for a CASIO device")
    ap.add_argument("--interval", type=float, default=30.0, help="seconds between polls")
    args = ap.parse_args()
    try:
        asyncio.run(poll_forever(args.flipper_port, args.watch_address, args.interval))
    except KeyboardInterrupt:
        pass
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
