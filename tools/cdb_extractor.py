#!/usr/bin/env python3
"""
cdb_extractor.py — Galerians PS1 .CDB container extractor.

Reads a .CDB file, detects compression via the CD_GetSize-style header,
optionally LZSS-decompresses the body (PS1 LZSS variant), enumerates
sub-files, and writes them to an output directory.

Usage:
    python tools/cdb_extractor.py MODEL.CDB ./extracted/

Header layout (mirrors what CD_GetSize @ 0x8018e1f4 returns):
    +0x00  uint32_t  sector_count        number of CD sectors of payload
    +0x04  uint32_t  compression_flag    0 = raw, non-zero = LZSS

Buffer sizing follows the PS1 driver:
    Uncompressed: sector_count * 4 + 4 bytes
    Compressed  : sector_count * 8 + 8 bytes (2x workspace)

LZSS PS1 variant parameters:
    window size  : 0x1000 (4096 bytes)
    lookahead    : 0x12   (18 bytes)
    flag byte    : 8 bits, bit=1 -> literal, bit=0 -> back-reference
    back-ref     : 2 bytes; high nibble of byte 2 = (length - 3),
                   remaining 12 bits = window offset
"""

from __future__ import annotations

import argparse
import struct
import sys
from pathlib import Path

CD_SECTOR_SIZE = 0x800  # 2048 bytes per CD sector

LZSS_WINDOW_SIZE = 0x1000  # 4096
LZSS_LOOKAHEAD = 0x12      # 18
LZSS_THRESHOLD = 3         # min match length encoded as (len - 3)


def read_header(data: bytes) -> tuple[int, int]:
    """Return (sector_count, compression_flag) from the first 8 bytes."""
    if len(data) < 8:
        raise ValueError("file too small to contain CDB header")
    sector_count, compression_flag = struct.unpack_from("<II", data, 0)
    return sector_count, compression_flag


def expected_buffer_size(sector_count: int, compressed: bool) -> int:
    """Mirror the malloc size used by the PS1 CD driver."""
    if compressed:
        return sector_count * 8 + 8
    return sector_count * 4 + 4


def lzss_decompress(src: bytes, expected_size: int | None = None) -> bytes:
    """
    Decompress an LZSS stream using the PS1-style variant.

    Stream format:
        [flag_byte][unit_0][unit_1]...[unit_7]
        flag_byte bit i (LSB first): 1 -> next unit is a literal byte
                                     0 -> next unit is 2 bytes (back-reference)

    Back-reference layout (2 little-endian bytes b0, b1):
        offset = b0 | ((b1 & 0xF0) << 4)   -> 12-bit window offset
        length = (b1 & 0x0F) + LZSS_THRESHOLD
        Copy `length` bytes from `out[-window_size + offset .. ]`.

    Stops when src is exhausted or expected_size bytes are produced.
    """
    out = bytearray()
    window = bytearray(LZSS_WINDOW_SIZE)
    win_pos = LZSS_WINDOW_SIZE - LZSS_LOOKAHEAD

    i = 0
    end = len(src)
    while i < end:
        flags = src[i]
        i += 1
        for bit in range(8):
            if i >= end:
                return bytes(out)
            if expected_size is not None and len(out) >= expected_size:
                return bytes(out)
            if flags & (1 << bit):
                # literal
                byte = src[i]
                i += 1
                out.append(byte)
                window[win_pos] = byte
                win_pos = (win_pos + 1) % LZSS_WINDOW_SIZE
            else:
                if i + 1 >= end:
                    return bytes(out)
                b0 = src[i]
                b1 = src[i + 1]
                i += 2
                offset = b0 | ((b1 & 0xF0) << 4)
                length = (b1 & 0x0F) + LZSS_THRESHOLD
                for k in range(length):
                    byte = window[(offset + k) % LZSS_WINDOW_SIZE]
                    out.append(byte)
                    window[win_pos] = byte
                    win_pos = (win_pos + 1) % LZSS_WINDOW_SIZE
                    if expected_size is not None and len(out) >= expected_size:
                        return bytes(out)
    return bytes(out)


# Magic numbers for known PS1 sub-asset formats found inside CDBs.
# (signature_bytes, signature_offset, name, extension)
SIGNATURES: list[tuple[bytes, int, str, str]] = [
    (b"\x10\x00\x00\x00", 0, "TIM",  ".tim"),   # PS1 TIM image
    (b"\x40\x00\x00\x00", 0, "TMD",  ".tmd"),   # PS1 TMD model
    (b"\x41\x00\x00\x00", 0, "HMD",  ".hmd"),   # PS1 HMD model
    (b"VAGp",             0, "VAG",  ".vag"),   # PS1 VAG audio
    (b"pQES",             0, "SEQ",  ".seq"),   # PsyQ sequence
]


def find_subfiles(payload: bytes) -> list[tuple[int, str, str]]:
    """
    Scan `payload` for known PS1 asset signatures.

    Returns a list of (offset, name, extension) tuples.
    Detection is best-effort — the CDB index format itself has not yet been
    fully reverse engineered.
    """
    hits: list[tuple[int, str, str]] = []
    # Walk in 4-byte stride; PS1 assets are word-aligned in CDB containers.
    for off in range(0, max(0, len(payload) - 4), 4):
        for sig, sig_off, name, ext in SIGNATURES:
            if payload[off + sig_off : off + sig_off + len(sig)] == sig:
                hits.append((off, name, ext))
                break
    return hits


def slice_subfiles(
    payload: bytes, hits: list[tuple[int, str, str]]
) -> list[tuple[int, str, str, bytes]]:
    """Slice the payload into sub-file byte ranges using consecutive offsets."""
    sliced: list[tuple[int, str, str, bytes]] = []
    for idx, (off, name, ext) in enumerate(hits):
        next_off = hits[idx + 1][0] if idx + 1 < len(hits) else len(payload)
        sliced.append((off, name, ext, payload[off:next_off]))
    return sliced


def extract(cdb_path: Path, out_dir: Path) -> int:
    data = cdb_path.read_bytes()
    sector_count, compression_flag = read_header(data)
    compressed = compression_flag != 0

    print(f"[+] {cdb_path.name}")
    print(f"    size            : {len(data)} bytes")
    print(f"    sector_count    : {sector_count} ({sector_count * CD_SECTOR_SIZE} raw bytes)")
    print(f"    compression_flag: 0x{compression_flag:08x} ({'LZSS' if compressed else 'raw'})")
    print(f"    expected buffer : {expected_buffer_size(sector_count, compressed)} bytes")

    body = data[8:]
    if compressed:
        try:
            payload = lzss_decompress(body)
            print(f"    decompressed    : {len(payload)} bytes")
        except Exception as exc:
            print(f"    [!] LZSS decompression failed: {exc}", file=sys.stderr)
            print(f"    [!] falling back to raw body", file=sys.stderr)
            payload = body
    else:
        payload = body

    hits = find_subfiles(payload)
    print(f"    sub-files found : {len(hits)}")
    if not hits:
        print("    [!] no known signatures matched — dumping full payload only")

    out_dir.mkdir(parents=True, exist_ok=True)
    stem = cdb_path.stem

    # Always dump the (possibly decompressed) payload for inspection.
    payload_path = out_dir / f"{stem}.payload.bin"
    payload_path.write_bytes(payload)
    print(f"    -> {payload_path}")

    written = 0
    for idx, (off, name, ext, blob) in enumerate(slice_subfiles(payload, hits)):
        sub_path = out_dir / f"{stem}_{idx:04d}_{name}_0x{off:06x}{ext}"
        sub_path.write_bytes(blob)
        print(f"    -> {sub_path.name}  ({len(blob)} bytes)")
        written += 1
    return written


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(description="Galerians PS1 CDB extractor")
    parser.add_argument("cdb", type=Path, help="path to a .CDB file")
    parser.add_argument(
        "out_dir", type=Path, help="directory to write extracted assets into"
    )
    args = parser.parse_args(argv)

    if not args.cdb.is_file():
        print(f"error: {args.cdb} not found", file=sys.stderr)
        return 1
    extract(args.cdb, args.out_dir)
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
