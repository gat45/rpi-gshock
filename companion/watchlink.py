#!/usr/bin/env python3
"""CasioLab watchlink - BLE transport for the Casio DW-H5600.

Why this exists
---------------
A Flipper FAP on Momentum (API 87.1) cannot be a BLE central: the exported API
contains GATT *server* symbols only (ble_gatt_service_add,
ble_gatt_characteristic_*), no GAP scan and no GATT client. The central role
therefore lives on a host with a Bluetooth radio: PC, phone or Raspberry Pi.

This module speaks the part of the protocol that was measured on the watch:

  write  0x0016  <- one command byte
  notify 0x0018  -> [command byte][payload...]

Commands used here are only the read-only ones observed in the captures:
  0x23 watch name, 0x28 watch condition, 0x26 activity record.

Two transports are provided:
  BleTransport     real watch, through bleak (needs a Bluetooth adapter)
  ReplayTransport  the ATT CSV exports, so the decoder can be tested with no
                   hardware and no radio

Usage:
    # decode a real capture, no Bluetooth needed
    python companion/watchlink.py --replay capture.csv --cmd all --verify

    # talk to the watch
    python companion/watchlink.py --ble --address E0:72:A1:F9:ED:D4 --cmd all

    # push the result to the Flipper as /ext/casiolab/live.txt (CLI over USB)
    python companion/watchlink.py --ble --cmd all --flipper COM8
"""
from __future__ import annotations

import argparse
import asyncio
import csv
import json
import sys
import time
from dataclasses import dataclass, field

# Handles measured on the DW-H5600 captures.
HANDLE_CMD = 0x0016  # central writes the command byte here
HANDLE_RSP = 0x0018  # watch notifies the answer here
HANDLE_FLOW = 0x0013  # LifeLog stream notifications

CMD_WATCH_NAME = 0x23
CMD_CONDITION = 0x28
CMD_ACTIVITY = 0x26

BATT_LOWER_GENERIC = 9
BATT_UPPER_GENERIC = 19

FLIPPER_LIVE_PATH = "/ext/casiolab/live.txt"


# --------------------------------------------------------------------------
# Pure protocol helpers
# --------------------------------------------------------------------------
def crc16_kermit(data: bytes) -> int:
    crc = 0
    for b in data:
        crc ^= b
        for _ in range(8):
            crc = (crc >> 1) ^ 0x8408 if crc & 1 else crc >> 1
    return crc


def unwrap_chunk(wire: bytes) -> tuple[bytes, str]:
    """Decode one 0x05 LifeLog notification.

    Returns (payload, kind) with kind in {"crc", "continuation"}.
    The payload is complemented byte-wise; when a valid CRC16 is present it is
    removed, otherwise the notification is an MTU slice of a larger frame.
    """
    if not wire or wire[0] != 0x05:
        raise ValueError(f"not a 0x05 chunk: {wire[:1].hex()}")
    if len(wire) >= 4 and crc16_kermit(wire[1:-2]) == int.from_bytes(wire[-2:], "little"):
        return bytes(b ^ 0xFF for b in wire[1:-2]), "crc"
    return bytes(b ^ 0xFF for b in wire[1:]), "continuation"


def decode_name(frame: bytes) -> str | None:
    if len(frame) < 2 or frame[0] != CMD_WATCH_NAME:
        return None
    out = []
    for b in frame[1:]:
        if b == 0 or b < 0x20 or b > 0x7E:
            break
        out.append(chr(b))
    name = "".join(out).rstrip()
    return name or None


def decode_condition(frame: bytes, lower: int = BATT_LOWER_GENERIC, upper: int = BATT_UPPER_GENERIC):
    if len(frame) < 3 or frame[0] != CMD_CONDITION:
        return None
    if upper <= lower:
        return None
    span = upper - lower
    multiplier = (100 + span // 2) // span
    percent = min(max((frame[1] - lower) * multiplier, 0), 100)
    return {"raw": frame[1], "temperature_c": frame[2], "percent": percent}


def parse_record(body: bytes) -> dict | None:
    """Type 0x00 LifeLog record body (15 bytes, measured)."""
    if len(body) < 15:
        return None
    year, month, day, hour, minute = body[0], body[1], body[2], body[3], body[4]
    if not (1 <= month <= 12 and 1 <= day <= 31 and hour <= 23 and minute <= 59):
        return None
    return {
        "year": 2000 + year,
        "month": month,
        "day": day,
        "hour": hour,
        "minute": minute,
        "value_a": int.from_bytes(body[5:9], "little", signed=True),
        "value_b": int.from_bytes(body[9:13], "little", signed=True),
        "sequence": int.from_bytes(body[13:15], "little"),
    }


# Measured (type, length) pairs, same table as protocol/drsp.c.
_PLAUSIBLE = {0x00: 15, 0x65: 99, 0x50: 301, 0x51: 301, 0x52: 745, 0x56: 1115, 0x72: 2373}


def parse_flow(chunks: list[bytes]) -> dict | None:
    """Rebuild the 0x0013 record stream and return the first type 0x00 record.

    Mirrors protocol/drsp.c: complemented payloads are concatenated, headers
    are [type][len32 LE], an unlisted but framed type (0x50..0x7F) is skipped
    whole, anything else is dropped one byte at a time. UNVERIFIED on hardware:
    which command makes the watch emit this flow is not known, so this only
    decodes what the watch sends while the other commands run.
    """
    stream = bytearray()
    for wire in chunks:
        try:
            payload, _kind = unwrap_chunk(wire)
        except ValueError:
            continue
        stream += payload
    while len(stream) >= 5:
        rtype = stream[0]
        length = int.from_bytes(stream[1:5], "little")
        if _PLAUSIBLE.get(rtype) == length:
            if 5 + length > len(stream):
                return None
            if rtype == 0x00:
                return parse_record(bytes(stream[5 : 5 + length]))
            del stream[: 5 + length]
        elif 0x50 <= rtype <= 0x7F and 0 < length <= 8187:
            if 5 + length > len(stream):
                return None
            del stream[: 5 + length]
        else:
            del stream[:1]
    return None


# --------------------------------------------------------------------------
# Transports
# --------------------------------------------------------------------------
@dataclass
class ReplayTransport:
    """Replays a btsnoop ATT export: a write to 0x0016 is answered by the next
    notification on 0x0018 that starts with the same command byte."""

    path: str
    _pairs: list[tuple[int, bytes]] = field(default_factory=list)

    def __post_init__(self) -> None:
        pending: list[int] = []
        with open(self.path, encoding="utf-8", errors="replace") as fh:
            for row in csv.DictReader(fh):
                raw = (row.get("valeur") or "").strip()
                if not raw:
                    continue
                handle = int(row.get("handle") or "0", 16)
                op = row.get("opcode")
                if row.get("dir") == "C->H" and handle == HANDLE_CMD and op in ("0x52", "0x12"):
                    pending.append(int(raw[:2], 16))
                elif row.get("dir") == "H->C" and handle == HANDLE_RSP and op == "0x1B":
                    cmd = int(raw[:2], 16)
                    if cmd in pending:
                        self._pairs.append((cmd, bytes.fromhex(raw)))
                        pending = pending[pending.index(cmd) + 1 :]

    def request(self, cmd: int) -> bytes | None:
        for c, frame in self._pairs:
            if c == cmd:
                return frame
        return None

    def close(self) -> None:
        pass


@dataclass
class BleTransport:
    address: str | None = None
    timeout: float = 12.0
    _client: object = None
    _responses: dict = field(default_factory=dict)
    flow_chunks: list = field(default_factory=list)

    async def connect(self) -> None:
        from bleak import BleakClient, BleakScanner  # imported lazily

        address = self.address
        if address is None:
            found = await BleakScanner.discover(timeout=8.0)
            for dev in found:
                name = (dev.name or "").upper()
                if "CASIO" in name or "DW-H5600" in name or "G-SHOCK" in name:
                    address = dev.address
                    break
            if address is None:
                raise RuntimeError("no CASIO device found in range")
        self.address = address

        self._client = BleakClient(address, timeout=self.timeout)

        def on_notify(_char, data: bytearray) -> None:
            if data:
                self._responses[int(data[0])] = bytes(data)

        def on_flow(_char, data: bytearray) -> None:
            if data:
                self.flow_chunks.append(bytes(data))

        await self._client.connect()
        await self._client.start_notify(HANDLE_RSP, on_notify)
        await self._client.start_notify(HANDLE_FLOW, on_flow)

    async def request(self, cmd: int, wait: float = 1.5) -> bytes | None:
        self._responses.pop(cmd, None)
        await self._client.write_gatt_char(HANDLE_CMD, bytes([cmd]), response=False)
        deadline = time.monotonic() + wait
        while time.monotonic() < deadline:
            if cmd in self._responses:
                return self._responses[cmd]
            await asyncio.sleep(0.05)
        return None

    async def close(self) -> None:
        """Disconnect on the SAME event loop that opened the connection."""
        if self._client is not None:
            try:
                await self._client.disconnect()
            except Exception as exc:  # report it: a silent failure leaves the link up
                print(f"[ble] disconnect failed: {exc}", file=sys.stderr)


# --------------------------------------------------------------------------
# Collection
# --------------------------------------------------------------------------
async def collect(transport, commands: list[int]) -> dict:
    result: dict = {"schema": "casiolab.live/1", "source": type(transport).__name__}

    for cmd in commands:
        if hasattr(transport, "connect"):
            frame = await transport.request(cmd)
        else:
            frame = transport.request(cmd)
        if frame is None:
            continue
        if cmd == CMD_WATCH_NAME:
            name = decode_name(frame)
            if name:
                result["name"] = name
        elif cmd == CMD_CONDITION:
            cond = decode_condition(frame)
            if cond:
                result.update(batt_raw=cond["raw"], temp_c=cond["temperature_c"], batt_pct=cond["percent"])
        elif cmd == CMD_ACTIVITY:
            result["activity_len"] = len(frame) - 1

    chunks = getattr(transport, "flow_chunks", None)
    if chunks:
        rec = parse_flow(chunks)
        if rec:
            result["record"] = "{year:04d}-{month:02d}-{day:02d} {hour:02d}:{minute:02d}".format(**rec)
            result.update(value_a=rec["value_a"], value_b=rec["value_b"], seq=rec["sequence"])

    return result


async def collect_ble(address: str | None, cmds: list[int]) -> dict:
    """connect + collect + disconnect inside ONE event loop (a BleakClient is
    bound to the loop that created it; two asyncio.run() calls break that)."""
    transport = BleTransport(address=address)
    await transport.connect()
    try:
        return await collect(transport, cmds)
    finally:
        await transport.close()


def to_kv(data: dict) -> str:
    """Flat key=value lines: trivial to parse on the Flipper, no JSON parser."""
    order = ["name", "batt_raw", "batt_pct", "temp_c", "record", "value_a", "value_b", "seq"]
    lines = ["v=1"]
    for key in order:
        if key in data and data[key] is not None:
            lines.append(f"{key}={data[key]}")
    return "\n".join(lines) + "\n"


def push_to_flipper(port: str, text: str) -> bool:
    """Append `text` as the newest document of live.txt and read it back.

    `storage write_chunk` APPENDS - it does not truncate - so the FAP parser
    treats every `v=` line as the start of a new document and keeps the last
    one (see data/live_state.h). The read-back is the actual proof the bytes
    reached the filesystem, not a guess based on the absence of an error.
    """
    import serial

    payload = text.encode()
    with serial.Serial(port, 115200, timeout=2) as ser:
        time.sleep(0.3)
        ser.reset_input_buffer()
        # Start from an empty file: write_chunk appends, so without this the
        # file grows forever and the read-back below would eventually exceed
        # what can be read in one second.
        ser.write(f"storage remove {FLIPPER_LIVE_PATH}\r\n".encode())
        time.sleep(0.3)
        ser.reset_input_buffer()
        ser.write(f"storage write_chunk {FLIPPER_LIVE_PATH} {len(payload)}\r\n".encode())
        time.sleep(0.25)
        ser.write(payload)
        time.sleep(0.8)
        reply = ser.read(8192).decode("utf-8", "replace")
        if "error" in reply.lower():
            return False

        ser.reset_input_buffer()
        ser.write(f"storage read {FLIPPER_LIVE_PATH}\r\n".encode())
        time.sleep(1.0)
        back = ser.read(65536).decode("utf-8", "replace")

    back = back.replace("\r\n", "\n")
    # Exactly one document on the card, and it is ours.
    return text.strip() in back and back.count("v=1") == 1


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    src = ap.add_mutually_exclusive_group(required=True)
    src.add_argument("--replay", metavar="CSV", help="decode a btsnoop ATT export")
    src.add_argument("--ble", action="store_true", help="talk to the real watch")
    ap.add_argument("--address", help="watch BLE address for --ble")
    ap.add_argument("--cmd", default="all", choices=["all", "name", "condition", "activity"])
    ap.add_argument("--flipper", metavar="PORT", help="push live.txt to the Flipper over USB")
    ap.add_argument("--json", action="store_true", help="print JSON instead of key=value")
    ap.add_argument("--verify", action="store_true", help="check decoded values against expectations")
    args = ap.parse_args()

    cmds = {
        "all": [CMD_WATCH_NAME, CMD_CONDITION, CMD_ACTIVITY],
        "name": [CMD_WATCH_NAME],
        "condition": [CMD_CONDITION],
        "activity": [CMD_ACTIVITY],
    }[args.cmd]

    if args.replay:
        transport = ReplayTransport(args.replay)
        data = asyncio.run(collect(transport, cmds))
    else:
        data = asyncio.run(collect_ble(args.address, cmds))

    if args.verify:
        failures = 0
        if data.get("name") != "CASIO DW-H5600" and data.get("name") != "CASIO GW-B5600":
            print(f"VERIFY FAIL name={data.get('name')!r}")
            failures += 1
        if "batt_raw" not in data:
            print("VERIFY FAIL no watch condition decoded")
            failures += 1
        print("VERIFY OK" if failures == 0 else f"VERIFY {failures} failure(s)")

    text = json.dumps(data, indent=2) if args.json else to_kv(data)
    print(text)

    rc = 1 if (args.verify and failures) else 0

    # Push after verification, never instead of it: --verify used to return
    # early, so "--verify --flipper" printed VERIFY OK without writing anything.
    if args.flipper:
        ok = push_to_flipper(args.flipper, to_kv(data))
        print(f"[flipper] {'written' if ok else 'FAILED'} {FLIPPER_LIVE_PATH}", file=sys.stderr)
        if not ok:
            rc = 1
    return rc


if __name__ == "__main__":
    raise SystemExit(main())
