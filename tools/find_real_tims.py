#!/usr/bin/env python3
"""
find_real_tims.py - Scans a .CDB for valid TIM textures with strict validation.

Payload detection: tries the raw body first (no decompression); if no valid
TIMs are found, falls back to LZSS decompression.

Usage: python tools/find_real_tims.py <FILE.CDB> [output_dir] [--count N]
       output_dir defaults to port/assets/model_real/
       --count N  extract first N valid TIMs (default: 10)
"""

import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))
from cdb_extractor import read_header, lzss_decompress

HEADER_SIZE = 8
PMODE_NAMES = {0: "4bpp", 1: "8bpp", 2: "16bpp", 3: "24bpp"}


def is_valid_tim(data, offset):
    if offset + 12 > len(data): return False
    if data[offset:offset+4] != b'\x10\x00\x00\x00': return False
    flags = struct.unpack_from('<I', data, offset+4)[0]
    if flags > 0x0B or (flags & 0xF0) != 0: return False
    pmode = flags & 0x07
    has_clut = (flags >> 3) & 1

    pos = offset + 8
    if has_clut:
        if pos + 4 > len(data): return False
        clut_size = struct.unpack_from('<I', data, pos)[0]
        if clut_size < 12 or clut_size > 0x10000: return False
        pos += clut_size

    if pos + 12 > len(data): return False
    img_size = struct.unpack_from('<I', data, pos)[0]
    w = struct.unpack_from('<H', data, pos+8)[0]
    h = struct.unpack_from('<H', data, pos+10)[0]

    if w == 0 or h == 0 or w > 1024 or h > 1024: return False
    bpp_divisor = [4, 2, 1, 1][pmode]  # noqa: F841 — pixels per 16bpp unit
    expected = w * h * 2 + 12
    if abs(img_size - expected) > 4: return False

    return True


def tim_end_offset(data, offset):
    """Return the byte offset just past this TIM's data (best-effort)."""
    flags = struct.unpack_from('<I', data, offset+4)[0]
    has_clut = (flags >> 3) & 1
    pos = offset + 8
    if has_clut:
        clut_size = struct.unpack_from('<I', data, pos)[0]
        pos += clut_size
    img_size = struct.unpack_from('<I', data, pos)[0]
    return pos + img_size


def scan_tims(payload: bytes) -> list[dict]:
    results = []
    end = len(payload) - 4
    off = 0
    while off <= end:
        if payload[off] == 0x10 and payload[off+1:off+4] == b'\x00\x00\x00':
            if is_valid_tim(payload, off):
                flags = struct.unpack_from('<I', payload, off+4)[0]
                pmode = flags & 0x07
                has_clut = (flags >> 3) & 1
                pos = off + 8
                if has_clut:
                    pos += struct.unpack_from('<I', payload, pos)[0]
                w = struct.unpack_from('<H', payload, pos+8)[0]
                h = struct.unpack_from('<H', payload, pos+10)[0]
                img_size = struct.unpack_from('<I', payload, pos)[0]
                end_off = pos + img_size
                results.append({
                    "offset": off,
                    "flags": flags,
                    "pmode": pmode,
                    "width": w,
                    "height": h,
                    "has_clut": has_clut,
                    "end": end_off,
                })
                off = end_off  # skip past this TIM's data
                continue
        off += 4
    return results


def main():
    args = sys.argv[1:]
    count = 10
    if "--count" in args:
        idx = args.index("--count")
        count = int(args[idx + 1])
        args = args[:idx] + args[idx + 2:]

    if not args:
        print("Usage: python tools/find_real_tims.py <FILE.CDB> [output_dir] [--count N]")
        sys.exit(1)

    cdb_path = Path(args[0])
    out_dir = Path(args[1]) if len(args) >= 2 else Path("port/assets/model_real")

    if not cdb_path.is_file():
        print(f"Error: file not found: {cdb_path}")
        sys.exit(1)

    raw = cdb_path.read_bytes()
    sector_count, compression_flag = read_header(raw)

    print(f"[+] {cdb_path.name}")
    print(f"    size on disk     : {len(raw)} bytes")
    print(f"    sector_count     : {sector_count}")
    print(f"    compression_flag : 0x{compression_flag:08X}")

    body = raw[HEADER_SIZE:]

    # Try raw body first; fall back to LZSS if no valid TIMs found.
    print("    scanning raw body ...", end=" ", flush=True)
    tims = scan_tims(body)
    if tims:
        payload = body
        payload_desc = f"raw ({len(payload)} bytes)"
        print(f"{len(tims)} valid TIM(s) found - using raw")
    else:
        print("0 found - trying LZSS decompression ...", end=" ", flush=True)
        payload = lzss_decompress(body)
        tims = scan_tims(payload)
        payload_desc = f"LZSS-decompressed ({len(payload)} bytes, ~{len(payload)/1024/1024:.1f} MB)"
        print(f"{len(tims)} valid TIM(s) found")

    print(f"    payload source   : {payload_desc}")
    print(f"\n    total valid TIMs : {len(tims)}\n")

    header = f"{'#':<5} {'offset':>10}  {'flags':>6}  {'mode':<6}  {'w':>5}  {'h':>5}  clut"
    print(header)
    print("-" * len(header))
    for i, t in enumerate(tims):
        clut_str = "yes" if t["has_clut"] else "no"
        mode_str = PMODE_NAMES.get(t["pmode"], f"mode{t['pmode']}")
        print(f"{i:<5} 0x{t['offset']:08X}  0x{t['flags']:04X}  {mode_str:<6}  {t['width']:>5}  {t['height']:>5}  {clut_str}")

    extract = tims[:count]
    if extract:
        out_dir.mkdir(parents=True, exist_ok=True)
        print(f"\n    extracting first {len(extract)} TIM(s) to {out_dir}/")
        for i, t in enumerate(extract):
            blob = payload[t["offset"]:t["end"]]
            name = f"REAL_{i:04d}_TIM_0x{t['offset']:08X}.tim"
            (out_dir / name).write_bytes(blob)
            print(f"      [{i:02d}] {name}  ({len(blob)} bytes)")

    print()


if __name__ == "__main__":
    main()
