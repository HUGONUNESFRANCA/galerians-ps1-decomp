#!/usr/bin/env python3
"""
find_real_tmds.py - Scans MODEL.CDB for valid TMD models with strict validation.

TMD Header (12 bytes):
  uint32 magic  = 0x41000000
  uint32 flags  (bit 0: 0=absolute ptrs, 1=relative ptrs)
  uint32 nobj   (number of objects, 1-256)

Object descriptor (28 bytes each):
  uint32 vert_top, uint32 vert_n, uint32 norm_top, uint32 norm_n,
  uint32 prim_top, uint32 prim_n, int32 scale

Usage: python tools/find_real_tmds.py <FILE.CDB> [output_dir] [--count N]
       output_dir defaults to port/assets/models_real/
       --count N  extract first N valid TMDs (default: 20)
"""

import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))
from cdb_extractor import read_header, lzss_decompress

HEADER_SIZE = 8
TMD_MAGIC = 0x41000000


def is_valid_tmd(data: bytes, offset: int) -> bool:
    if offset + 12 > len(data):
        return False
    magic, flags, nobj = struct.unpack_from('<III', data, offset)
    if magic != TMD_MAGIC:
        return False
    if flags not in (0, 1):
        return False
    if nobj < 1 or nobj > 256:
        return False
    # Validate the first object descriptor
    obj_table_start = offset + 12
    if obj_table_start + 28 > len(data):
        return False
    vert_top, vert_n, norm_top, norm_n, prim_top, prim_n, scale = \
        struct.unpack_from('<IIIIIIi', data, obj_table_start)
    if vert_n < 1 or vert_n > 65535:
        return False
    if prim_n < 1 or prim_n > 65535:
        return False
    # For relative pointers, sanity-check that vert_top lands inside the blob
    if flags == 1:
        # vert_top is relative to the start of the object table
        abs_vert = obj_table_start + 4 + vert_top  # +4 past vert_top field itself
        if abs_vert + 12 >= len(data):
            return False
    return True


def tmd_info(data: bytes, offset: int) -> dict:
    magic, flags, nobj = struct.unpack_from('<III', data, offset)
    obj_table_start = offset + 12
    # Read all object descriptors
    objects = []
    for i in range(nobj):
        od = obj_table_start + i * 28
        if od + 28 > len(data):
            break
        vert_top, vert_n, norm_top, norm_n, prim_top, prim_n, scale = \
            struct.unpack_from('<IIIIIIi', data, od)
        objects.append({
            "vert_top": vert_top, "vert_n": vert_n,
            "norm_top": norm_top, "norm_n": norm_n,
            "prim_top": prim_top, "prim_n": prim_n,
            "scale": scale,
        })
    return {"offset": offset, "flags": flags, "nobj": nobj, "objects": objects}


def estimate_tmd_end(data: bytes, offset: int) -> int:
    """Best-effort estimate of where this TMD ends."""
    _, flags, nobj = struct.unpack_from('<III', data, offset)
    obj_table_start = offset + 12
    max_end = obj_table_start + nobj * 28

    for i in range(nobj):
        od = obj_table_start + i * 28
        if od + 28 > len(data):
            break
        vert_top, vert_n, norm_top, norm_n, prim_top, prim_n, _ = \
            struct.unpack_from('<IIIIIII', data, od)
        if flags == 1:
            base = obj_table_start + 4  # relative to after vert_top field
            # Use largest reasonable end estimate
            vert_end = base + vert_top + vert_n * 8
            norm_end = base + norm_top + norm_n * 8
            prim_end = base + prim_top + prim_n * 12  # rough max primitive size
            max_end = max(max_end, vert_end, norm_end, prim_end)
        else:
            max_end = max(max_end, vert_top + vert_n * 8,
                         norm_top + norm_n * 8,
                         prim_top + prim_n * 12)
    return min(max_end, len(data))


def scan_tmds(payload: bytes) -> list[dict]:
    results = []
    end = len(payload) - 4
    off = 0
    while off <= end:
        if payload[off] == 0x00 and payload[off+1:off+4] == b'\x00\x00\x41':
            # TMD magic is 0x41000000 = bytes 00 00 00 41 in LE
            pass
        # Check little-endian: 0x41000000 -> bytes [00, 00, 00, 41]
        if (payload[off] == 0x00 and payload[off+1] == 0x00 and
                payload[off+2] == 0x00 and payload[off+3] == 0x41):
            if is_valid_tmd(payload, off):
                info = tmd_info(payload, off)
                info["end"] = estimate_tmd_end(payload, off)
                results.append(info)
                off = info["end"]
                continue
        off += 4
    return results


def main():
    args = sys.argv[1:]
    count = 20
    if "--count" in args:
        idx = args.index("--count")
        count = int(args[idx + 1])
        args = args[:idx] + args[idx + 2:]

    if not args:
        print("Usage: python tools/find_real_tmds.py <FILE.CDB> [output_dir] [--count N]")
        sys.exit(1)

    cdb_path = Path(args[0])
    out_dir = Path(args[1]) if len(args) >= 2 else Path("port/assets/models_real")

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

    print("    scanning raw body ...", end=" ", flush=True)
    tmds = scan_tmds(body)
    if tmds:
        payload = body
        payload_desc = f"raw ({len(payload)} bytes)"
        print(f"{len(tmds)} valid TMD(s) found - using raw")
    else:
        print("0 found - trying LZSS decompression ...", end=" ", flush=True)
        payload = lzss_decompress(body)
        tmds = scan_tmds(payload)
        payload_desc = f"LZSS-decompressed ({len(payload)} bytes, ~{len(payload)/1024/1024:.1f} MB)"
        print(f"{len(tmds)} valid TMD(s) found")

    print(f"    payload source   : {payload_desc}")
    print(f"\n    total valid TMDs : {len(tmds)}\n")

    header = f"{'#':<5} {'offset':>10}  {'flags':>6}  {'nobj':>5}  {'vert_n':>7}  {'norm_n':>7}  {'prim_n':>7}"
    print(header)
    print("-" * len(header))
    for i, t in enumerate(tmds):
        first = t["objects"][0] if t["objects"] else {}
        vert_n = first.get("vert_n", 0)
        norm_n = first.get("norm_n", 0)
        prim_n = first.get("prim_n", 0)
        print(f"{i:<5} 0x{t['offset']:08X}  0x{t['flags']:04X}  {t['nobj']:>5}  {vert_n:>7}  {norm_n:>7}  {prim_n:>7}")

    extract = tmds[:count]
    if extract:
        out_dir.mkdir(parents=True, exist_ok=True)
        print(f"\n    extracting first {len(extract)} TMD(s) to {out_dir}/")
        for i, t in enumerate(extract):
            blob = payload[t["offset"]:t["end"]]
            name = f"REAL_{i:04d}_TMD_0x{t['offset']:08X}.tmd"
            (out_dir / name).write_bytes(blob)
            first = t["objects"][0] if t["objects"] else {}
            print(f"      [{i:02d}] {name}  ({len(blob)} bytes)"
                  f"  nobj={t['nobj']} vert_n={first.get('vert_n',0)}"
                  f" norm_n={first.get('norm_n',0)} prim_n={first.get('prim_n',0)}")

    print()


if __name__ == "__main__":
    main()
