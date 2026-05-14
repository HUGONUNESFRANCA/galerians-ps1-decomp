#!/usr/bin/env python3
"""
inspect_model.py — Locate and hex-dump the first TMD (0x40) and HMD (0x41)
magic-word occurrences in the decompressed MODEL.CDB payload.

Usage:
    python tools/inspect_model.py <path/to/MODEL.CDB>
"""

import struct
import sys
from pathlib import Path

# Re-use the LZSS decompressor from cdb_extractor without re-implementing it.
sys.path.insert(0, str(Path(__file__).parent))
from cdb_extractor import lzss_decompress, read_header, HEADER_SIZE


def hex_dump(data: bytes, offset: int, length: int = 64) -> str:
    chunk = data[offset : offset + length]
    lines = []
    for i in range(0, len(chunk), 16):
        row = chunk[i : i + 16]
        hex_part = " ".join(f"{b:02X}" for b in row)
        asc_part = "".join(chr(b) if 32 <= b < 127 else "." for b in row)
        lines.append(f"  {offset + i:08X}  {hex_part:<47s}  {asc_part}")
    return "\n".join(lines)


def find_and_report(payload: bytes, needle: bytes, label: str) -> None:
    idx = payload.find(needle)
    if idx == -1:
        print(f"[{label}] NOT FOUND in decompressed payload")
        return
    print(f"[{label}] First occurrence at offset 0x{idx:08X} ({idx})")
    print(hex_dump(payload, idx, 64))
    print()


def main() -> int:
    if len(sys.argv) < 2:
        print("usage: python tools/inspect_model.py <MODEL.CDB>", file=sys.stderr)
        return 1

    cdb_path = Path(sys.argv[1])
    if not cdb_path.is_file():
        print(f"error: {cdb_path} not found", file=sys.stderr)
        return 1

    data = cdb_path.read_bytes()
    sector_count, compression_flag = read_header(data)
    compressed = compression_flag != 0

    print(f"[+] {cdb_path.name}  ({len(data)} bytes on disk)")
    print(f"    sector_count={sector_count}  compression_flag=0x{compression_flag:08X}")

    body = data[HEADER_SIZE:]
    if compressed:
        payload = lzss_decompress(body)
        print(f"    decompressed: {len(payload)} bytes\n")
    else:
        payload = body
        print(f"    raw payload: {len(payload)} bytes\n")

    find_and_report(payload, b"\x40\x00\x00\x00", "TMD 0x40000000")
    find_and_report(payload, b"\x41\x00\x00\x00", "HMD 0x41000000")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
