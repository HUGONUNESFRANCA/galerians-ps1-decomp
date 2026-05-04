#!/usr/bin/env python3
"""
cdb_extractor.py — Galerians PS1 .CDB container extractor.

Reads a .CDB file, parses the 8-byte CD_GetSize-style header, LZSS-
decompresses the body (PS1 LZSS variant) when flagged, scans the
decompressed payload for known PS1 asset magics (word-aligned), and
writes each sub-file to the output directory.

Usage:
    python tools/cdb_extractor.py MODEL.CDB tools/extracted/MODEL/

CDB Header (8 bytes — confirmed by CD_GetSize @ 0x8018e1f4 and by
matching the disk-image extraction of MODEL.CDB to its known asset
inventory):

    +0x00  uint32  sector_count       Total CD sectors of payload (108 for MODEL.CDB)
    +0x04  uint32  compression_flag   0 = raw, non-zero = LZSS
    +0x08  ...     LZSS stream (or raw payload)

There is NO embedded sub-file table and NO `decompressed_size` field.
The LZSS stream simply runs to its natural end (~14 MB for MODEL.CDB).
Sub-files inside the decompressed payload are located by scanning for
4-byte magics on word-aligned offsets and slicing between consecutive
hits.

LZSS PS1 variant parameters:
    window size  : 0x1000 (4096 bytes)
    lookahead    : 0x12   (18 bytes)
    flag byte    : 8 bits, LSB first; bit=1 -> literal, bit=0 -> back-ref
    back-ref     : 2 bytes; high nibble of byte 2 = (length - 3),
                   remaining 12 bits = window offset
"""

from __future__ import annotations

import argparse
import struct
import sys
from pathlib import Path

CD_SECTOR_SIZE = 0x800     # 2048 bytes per CD sector
HEADER_SIZE = 8            # uint32 sector_count + uint32 compression_flag

LZSS_WINDOW_SIZE = 0x1000  # 4096
LZSS_LOOKAHEAD = 0x12      # 18
LZSS_THRESHOLD = 3         # min match length encoded as (len - 3)


def read_header(data: bytes) -> tuple[int, int]:
    """Return (sector_count, compression_flag) from the first 8 bytes."""
    if len(data) < HEADER_SIZE:
        raise ValueError("file too small to contain CDB header")
    sector_count, compression_flag = struct.unpack_from("<II", data, 0)
    return sector_count, compression_flag


def lzss_decompress(src: bytes) -> bytes:
    """
    Decompress a PS1-style LZSS stream until input is exhausted.

    Stream format:
        [flag_byte][unit_0][unit_1]...[unit_7]
        flag_byte bit i (LSB first): 1 -> next unit is a literal byte
                                     0 -> next unit is a 2-byte back-reference

    Back-reference layout (2 little-endian bytes b0, b1):
        offset = b0 | ((b1 & 0xF0) << 4)   -> 12-bit window offset
        length = (b1 & 0x0F) + LZSS_THRESHOLD
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
    return bytes(out)


# Known 4-byte magics for PS1 sub-asset formats found in CDB containers.
# (signature_bytes, name, extension)  — extension None means "skip this blob".
SIGNATURES: list[tuple[bytes, str, str | None]] = [
    (b"\x10\x00\x00\x00", "TIM",   ".tim"),  # PS1 TIM image (MODEL.CDB, DISPLAY.CDB)
    (b"\x40\x00\x00\x00", "TMD",   ".tmd"),  # PS1 TMD model (MODEL.CDB)
    (b"\x41\x00\x00\x00", "HMD",   ".hmd"),  # PS1 HMD model
    (b"\x41\x4C\x54\x00", "ALT",   ".alt"),  # 'ALT\0' container
    (b"VAGp",             "VAG",   ".vag"),  # PS1 VAG audio (SOUND.CDB)
    (b"pQES",             "SEQ",   ".seq"),  # PsyQ sequence
    (b"\x00\x00\x00\x00", "EMPTY", None),    # padding / empty slot — skip
]

REAL_SIGS = [(sig, name, ext) for sig, name, ext in SIGNATURES if ext is not None]

# TIM flag field (offset 4, uint32) must be one of these values.
# Bits 0-2 encode bpp (0=4, 1=8, 2=16, 3=24); bit 3 = has CLUT.
VALID_TIM_FLAGS = {0, 1, 2, 3, 8, 9, 10, 11}


def is_valid_tim(blob: bytes) -> bool:
    """Return False when TIM magic is a false positive.

    Many zero-runs and palette data happen to start with 0x10000000.
    Three quick checks reject the vast majority of false hits:
      1. blob must be at least 16 bytes
      2. flag uint32 at offset 4 must be a known bpp/CLUT combination
      3. the first block's width and height fields (offsets 0x10/0x12)
         must both be non-zero
    """
    if len(blob) < 16:
        return False
    flag = struct.unpack_from("<I", blob, 4)[0]
    if flag not in VALID_TIM_FLAGS:
        return False
    w, h = struct.unpack_from("<HH", blob, 0x10)
    if w == 0 or h == 0:
        return False
    return True


def identify_blob(blob: bytes) -> tuple[str, str | None]:
    """Return (type_name, extension) for a sub-file blob.

    extension is None when the blob is empty/padding (4 leading zero bytes
    or shorter than 4 bytes); the caller should skip these.
    Unknown magics fall through to ('BIN', '.bin').
    """
    if len(blob) < 4:
        return "EMPTY", None
    head = bytes(blob[:4])
    for sig, name, ext in SIGNATURES:
        if head == sig:
            if name == "TIM" and not is_valid_tim(blob):
                return "BIN", ".bin"
            return name, ext
    return "BIN", ".bin"


def scan_subfiles_by_magic(payload: bytes) -> list[tuple[int, int]]:
    """Word-aligned scan for known asset signatures; slice between hits.

    Returns a list of (offset, size) tuples covering [hit_i, hit_{i+1}).
    The empty-padding magic (00 00 00 00) is *not* used as a slice marker —
    it would split every zero-padded run in the payload.
    """
    hits: list[int] = []
    end = len(payload) - 4
    for off in range(0, max(0, end) + 1, 4):
        head = payload[off : off + 4]
        for sig, _name, _ext in REAL_SIGS:
            if head == sig:
                hits.append(off)
                break
    sliced: list[tuple[int, int]] = []
    for idx, off in enumerate(hits):
        next_off = hits[idx + 1] if idx + 1 < len(hits) else len(payload)
        sliced.append((off, next_off - off))
    return sliced


def extract(cdb_path: Path, out_dir: Path) -> int:
    data = cdb_path.read_bytes()
    sector_count, compression_flag = read_header(data)
    compressed = compression_flag != 0

    print(f"[+] {cdb_path.name}")
    print(f"    file size on disk: {len(data)} bytes")
    print(f"    sector_count     : {sector_count} "
          f"({sector_count * CD_SECTOR_SIZE} bytes raw)")
    print(f"    compression_flag : 0x{compression_flag:08x} "
          f"({'LZSS' if compressed else 'raw'})")

    body = data[HEADER_SIZE:]
    if compressed:
        try:
            payload = lzss_decompress(body)
            print(f"    decompressed     : {len(payload)} bytes "
                  f"(~{len(payload)/1024/1024:.1f} MB)")
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

    entries = scan_subfiles_by_magic(payload)
    print(f"    magic scan hits  : {len(entries)}")

    type_counts: dict[str, int] = {}
    written = 0
    skipped_empty = 0
    for idx, (off, size) in enumerate(entries):
        blob = payload[off : off + size]
        name, ext = identify_blob(blob)
        type_counts[name] = type_counts.get(name, 0) + 1
        if ext is None:
            skipped_empty += 1
            continue
        sub_path = out_dir / f"{stem}_{idx:04d}_{name}{ext}"
        sub_path.write_bytes(blob)
        written += 1

    print()
    print(f"[=] Summary for {cdb_path.name}")
    print(f"    sub-files written: {written}")
    print(f"    skipped (empty)  : {skipped_empty}")
    print(f"    by type:")
    for name in sorted(type_counts):
        print(f"      {name:<6s} : {type_counts[name]}")
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
