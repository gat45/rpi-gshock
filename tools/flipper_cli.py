#!/usr/bin/env python3
"""Minimal Flipper CLI helper over USB serial.

Used to drive the device from the host without ufbt (which takes the port).

    python tools/flipper_cli.py --port COM8 storage list /ext/casiolab
    python tools/flipper_cli.py --port COM8 input send down press
"""
from __future__ import annotations

import argparse
import sys
import time

import serial


def run(port: str, commands: list[str], wait: float = 0.6) -> str:
    ser = serial.Serial(port, 115200, timeout=0.4)
    time.sleep(0.3)
    ser.reset_input_buffer()
    out = []
    for cmd in commands:
        ser.write((cmd + "\r\n").encode())
        time.sleep(wait)
        out.append(ser.read(16384).decode("utf-8", "replace"))
    ser.close()
    return "\n".join(out)


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--port", default="COM8")
    ap.add_argument("--wait", type=float, default=0.6)
    ap.add_argument("commands", nargs="+")
    args = ap.parse_args()
    try:
        text = run(args.port, args.commands, args.wait)
    except Exception as exc:  # pragma: no cover - hardware path
        print(f"serial error: {exc}", file=sys.stderr)
        return 2
    print(text)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
