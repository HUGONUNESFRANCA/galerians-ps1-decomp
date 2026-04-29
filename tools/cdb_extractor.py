#!/usr/bin/env python3
"""
cdb_extractor.py — Galerians PS1 .CDB container extractor.

Reads a .CDB file, parses the confirmed header layout, LZSS-decompresses
the payload (PS1 LZSS variant), enumerates sub-files via the embedded
sub-file table (and/or magic-byte scanning), and writes them out.

Usage:
    python tools/cdb_extractor.py MODEL.CDB ./extracted/
    python tools/cdb_extractor.py --subfile-count 1050 MODEL.CDB ./extracted/

CDB Header (CONFIRMED):
    +0x00  uint32  file_size            Total compressed file size
    +0x04  uint16  sector_count         CD sectors (108 for MODEL.CDB)
    +0x06  uint16  compression_flags    Low byte: 1=LZSS, High byte: variant/meta
    +0x08  uint32  decompressed_size    Target size after LZSS expand
    +0x0C  N       sub_file_table       Array of {offset, size} pairs
                                        (1050 entries for MODEL.CDB)

Notes:
- compression_flags is masked with 0xFFFF when checking for LZSS — the upper
  bits (e.g. 0x0013 = 19 in MODEL.CDB) appear to encode a variant or meta
  field (possibly sub-file count table size). Do NOT use sector_count to
  size the decompression buffer in this variant — use decompressed_size.
- MODEL.CDB stats: 4.6MB compressed → 14MB decompressed (≈3× ratio),
  1050 sub-files (TIM textures + TMD models).

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

HEADER_SIZE = 0x0C         # file_size + sector_count + comp_flags + decompressed_size
SUBFILE_ENTRY_SIZE = 8     # {uint32 offset, uint32 size}

LZSS_FLAG_MASK = 0xFFFF    # mask out variant bits in compression_flags


def read_header(data: bytes) -> dict:
    """Parse the 12-byte CDB header."""
    if len(data) < HEADER_SIZE:
        raise ValueError("file too small to contain CDB header")
    file_size, sector_count, comp_flags, decompressed_size = struct.unpack_from(
        "<IHHI", data, 0
    )
    lzss_flag = comp_flags & LZSS_FLAG_MASK
    variant = (comp_flags >> 8) & 0xFF
    return {
        "file_size": file_size,
        "sector_count": sector_count,
        "compression_flags": comp_flags,
        "lzss_flag": lzss_flag,
        "variant": variant,
        "decompressed_size": decompressed_size,
    }


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
    (b"\x10\x00\x00\x00", 0, "TIM",  ".tim"),   # PS1 TIM image (CONFIRMED in MODEL.CDB)
    (b"\x40\x00\x00\x00", 0, "TMD",  ".tmd"),   # PS1 TMD model
    (b"\x41\x00\x00\x00", 0, "HMD",  ".hmd"),   # PS1 HMD model
    (b"VAGp",             0, "VAG",  ".vag"),   # PS1 VAG audio (SOUND.CDB)
    (b"pQES",             0, "SEQ",  ".seq"),   # PsyQ sequence
]


def identify_blob(blob: bytes) -> tuple[str, str]:
    """Return (type_name, extension) for a sub-file blob, or ('BIN', '.bin')."""
    for sig, sig_off, name, ext in SIGNATURES:
        if blob[sig_off : sig_off + len(sig)] == sig:
            return name, ext
    return "BIN", ".bin"


def parse_subfile_table(payload: bytes, count: int) -> list[tuple[int, int]]:
    """
    Read `count` sub-file table entries from the start of `payload`.

    Each entry is {uint32 offset, uint32 size}. Offsets are interpreted as
    absolute positions inside `payload` (decompressed image).
    Returns the list of (offset, size) pairs that look in-range.
    """
    entries: list[tuple[int, int]] = []
    table_bytes = count * SUBFILE_ENTRY_SIZE
    if table_bytes > len(payload):
        return entries
    for i in range(count):
        off, size = struct.unpack_from("<II", payload, i * SUBFILE_ENTRY_SIZE)
        if off == 0 and size == 0:
            continue
        if off >= len(payload) or size == 0 or off + size > len(payload):
            continue
        entries.append((off, size))
    return entries


def scan_subfiles_by_magic(payload: bytes) -> list[tuple[int, int]]:
    """Fallback: word-aligned scan for known signatures, slice between hits."""
    hits: list[int] = []
    for off in range(0, max(0, len(payload) - 4), 4):
        for sig, sig_off, _name, _ext in SIGNATURES:
            if payload[off + sig_off : off + sig_off + len(sig)] == sig:
                hits.append(off)
                break
    sliced: list[tuple[int, int]] = []
    for idx, off in enumerate(hits):
        next_off = hits[idx + 1] if idx + 1 < len(hits) else len(payload)
        sliced.append((off, next_off - off))
    return sliced


def extract(cdb_path: Path, out_dir: Path, subfile_count: int | None) -> int:
    data = cdb_path.read_bytes()
    hdr = read_header(data)
    compressed = hdr["lzss_flag"] != 0

    print(f"[+] {cdb_path.name}")
    print(f"    file size on disk: {len(data)} bytes")
    print(f"    header.file_size : {hdr['file_size']} bytes")
    print(f"    sector_count     : {hdr['sector_count']} "
          f"(CD raw = {hdr['sector_count'] * CD_SECTOR_SIZE} bytes — informational only)")
    print(f"    compression_flags: 0x{hdr['compression_flags']:04x} "
          f"(lzss=0x{hdr['lzss_flag']:04x}, variant=0x{hdr['variant']:02x})")
    print(f"    decompressed_size: {hdr['decompressed_size']} bytes "
          f"({'LZSS' if compressed else 'raw'})")

    body = data[HEADER_SIZE:]
    if compressed:
        try:
            payload = lzss_decompress(body, expected_size=hdr["decompressed_size"])
            print(f"    decompressed     : {len(payload)} bytes "
                  f"(target {hdr['decompressed_size']})")
        except Exception as exc:
            print(f"    [!] LZSS decompression failed: {exc}", file=sys.stderr)
            print(f"    [!] falling back to raw body", file=sys.stderr)
            payload = body
    else:
        payload = body

    out_dir.mkdir(parents=True, exist_ok=True)
    stem = cdb_path.stem

    payload_path = out_dir / f"{stem}.payload.bin"
    payload_path.write_bytes(payload)
    print(f"    -> {payload_path}")

    entries: list[tuple[int, int]] = []
    if subfile_count is not None and subfile_count > 0:
        entries = parse_subfile_table(payload, subfile_count)
        print(f"    sub-file table   : {len(entries)}/{subfile_count} valid entries")

    if not entries:
        entries = scan_subfiles_by_magic(payload)
        print(f"    magic scan hits  : {len(entries)} (fallback)")

    written = 0
    for idx, (off, size) in enumerate(entries):
        blob = payload[off : off + size]
        name, ext = identify_blob(blob)
        sub_path = out_dir / f"{stem}_{idx:04d}_{name}_0x{off:06x}{ext}"
        sub_path.write_bytes(blob)
        written += 1
    print(f"    sub-files written: {written}")
    return written


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(description="Galerians PS1 CDB extractor")
    parser.add_argument("cdb", type=Path, help="path to a .CDB file")
    parser.add_argument(
        "out_dir", type=Path, help="directory to write extracted assets into"
    )
    parser.add_argument(
        "--subfile-count",
        type=int,
        default=None,
        help="known sub-file table entry count (e.g. 1050 for MODEL.CDB). "
             "If omitted, falls back to magic-byte scanning.",
    )
    args = parser.parse_args(argv)

    if not args.cdb.is_file():
        print(f"error: {args.cdb} not found", file=sys.stderr)
        return 1
    extract(args.cdb, args.out_dir, args.subfile_count)
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
