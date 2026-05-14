#!/usr/bin/env python3
"""
inspect_exe.py - Scan a PS1 executable for TMD / HMD / TIM asset blobs.

Scans at 4-byte aligned offsets for:
  [41 00 00 00] -> TMD magic  (PS1 TMD id = 0x41)
  [50 00 00 00] -> HMD magic  (PS1 HMD id = 0x50)
  [10 00 00 00] -> TIM magic  (PS1 TIM id = 0x10)

TMD validation:
  flags   uint32 LE  must be 0 or 1
  nobj    uint32 LE  must be 1-64
  obj[0]  28-byte descriptor:
    vert_n  must be 1-65535
    prim_n  must be 1-65535
    vert_top must lie within file

TIM validation:
  flags   uint32 LE  must be in {0..3, 8..11}  (bpp + CLUT bit)
  pixel block width/height both > 0 and <= 1024

Usage:
    python tools/inspect_exe.py [path/to/SLUS_010.99]
"""

import struct
import sys
from pathlib import Path

DEFAULT_EXE = r"C:\Users\User\Desktop\Programação\T4\SLUS_010.99"

MAGIC_TMD = bytes([0x41, 0x00, 0x00, 0x00])
MAGIC_HMD = bytes([0x50, 0x00, 0x00, 0x00])
MAGIC_TIM = bytes([0x10, 0x00, 0x00, 0x00])

VALID_TIM_FLAGS = set(range(4)) | set(range(8, 12))   # 0-3 and 8-11
OBJ_SIZE        = 28


# ---------------------------------------------------------------------------
# TMD validation
# ---------------------------------------------------------------------------

def validate_tmd(data: bytes, offset: int) -> tuple[bool, str]:
    n = len(data)
    if offset + 12 > n:
        return False, "truncated header"

    flags, nobj = struct.unpack_from("<II", data, offset + 4)
    if flags not in (0, 1):
        return False, f"flags=0x{flags:08X}"
    if not (1 <= nobj <= 64):
        return False, f"nobj={nobj}"

    obj_table = offset + 12
    if obj_table + OBJ_SIZE > n:
        return False, "object table truncated"

    base = obj_table if flags == 1 else 0

    for i in range(nobj):
        od = obj_table + i * OBJ_SIZE
        if od + OBJ_SIZE > n:
            return False, f"obj[{i}] truncated"
        vt, vn, nt, nn, pt, pn, _ = struct.unpack_from("<IIIIIIi", data, od)
        if not (1 <= vn <= 65535):
            return False, f"obj[{i}].vert_n={vn}"
        if not (1 <= pn <= 65535):
            return False, f"obj[{i}].prim_n={pn}"
        if base + vt >= n:
            return False, f"obj[{i}].vert_top=0x{vt:X} out of bounds"

    return True, "ok"


def tmd_summary(data: bytes, offset: int) -> str:
    flags, nobj = struct.unpack_from("<II", data, offset + 4)
    obj_table = offset + 12
    total_v = total_p = 0
    for i in range(nobj):
        od = obj_table + i * OBJ_SIZE
        _, vn, _, nn, _, pn, _ = struct.unpack_from("<IIIIIIi", data, od)
        total_v += vn
        total_p += pn
    return f"flags={flags} nobj={nobj} total_verts={total_v} total_prims={total_p}"


# ---------------------------------------------------------------------------
# TIM validation
# ---------------------------------------------------------------------------

def validate_tim(data: bytes, offset: int) -> tuple[bool, str]:
    n = len(data)
    if offset + 20 > n:
        return False, "truncated"

    flags = struct.unpack_from("<I", data, offset + 4)[0]
    if flags not in VALID_TIM_FLAGS:
        return False, f"flags=0x{flags:X}"

    has_clut = bool(flags & 0x08)

    if has_clut:
        if offset + 20 > n:
            return False, "CLUT header truncated"
        clut_sz = struct.unpack_from("<I", data, offset + 8)[0]
        if clut_sz < 12 or offset + 8 + clut_sz > n:
            return False, f"CLUT block size={clut_sz}"
        pix_off = offset + 8 + clut_sz
    else:
        pix_off = offset + 8

    if pix_off + 12 > n:
        return False, "pixel block truncated"

    pix_sz = struct.unpack_from("<I", data, pix_off)[0]
    if pix_sz < 12 or pix_off + pix_sz > n:
        return False, f"pix block size={pix_sz}"

    w, h = struct.unpack_from("<HH", data, pix_off + 8)
    if w == 0 or h == 0:
        return False, f"dim={w}x{h}"
    if w > 1024 or h > 1024:
        return False, f"dim={w}x{h} too large"

    return True, "ok"


def tim_summary(data: bytes, offset: int) -> str:
    flags = struct.unpack_from("<I", data, offset + 4)[0]
    bpp_s = ["4bpp", "8bpp", "16bpp", "24bpp"][flags & 3]
    has_clut = bool(flags & 8)
    clut_sz = struct.unpack_from("<I", data, offset + 8)[0] if has_clut else 0
    pix_off = (offset + 8 + clut_sz) if has_clut else (offset + 8)
    px, py, pw, ph = struct.unpack_from("<HHHH", data, pix_off + 4)
    return f"{bpp_s} clut={has_clut} pix={pw}x{ph} @VRAM({px},{py})"


# ---------------------------------------------------------------------------
# HMD validation  (PS1 HMD id=0x50, version 1-3)
# ---------------------------------------------------------------------------

def validate_hmd(data: bytes, offset: int) -> tuple[bool, str]:
    n = len(data)
    if offset + 16 > n:
        return False, "truncated header"

    version, prim_cnt, coord_cnt = struct.unpack_from("<III", data, offset + 4)
    if version not in (1, 2, 3):
        return False, f"version={version}"
    if prim_cnt == 0 or prim_cnt > 10000:
        return False, f"prim_cnt={prim_cnt}"
    # coord_cnt 0 is allowed (static model with no hierarchy)
    if coord_cnt > 1000:
        return False, f"coord_cnt={coord_cnt}"
    # Primitive header table follows at offset+16: each entry is 8 bytes
    ph_table = offset + 16
    if ph_table + prim_cnt * 8 > n:
        return False, "prim_header table truncated"
    return True, "ok"


def hmd_summary(data: bytes, offset: int) -> str:
    version, prim_cnt, coord_cnt = struct.unpack_from("<III", data, offset + 4)
    return f"version={version} prim_cnt={prim_cnt} coord_cnt={coord_cnt}"


# ---------------------------------------------------------------------------
# Scanner
# ---------------------------------------------------------------------------

def scan(data: bytes) -> dict:
    n = len(data)
    results: dict[str, dict] = {
        "TMD": {"raw": [], "valid": []},
        "HMD": {"raw": [], "valid": []},
        "TIM": {"raw": [], "valid": []},
    }

    for off in range(0, n - 4, 4):
        b = data[off: off + 4]
        if b == MAGIC_TMD:
            results["TMD"]["raw"].append(off)
            ok, _ = validate_tmd(data, off)
            if ok:
                results["TMD"]["valid"].append(off)
        elif b == MAGIC_HMD:
            results["HMD"]["raw"].append(off)
            ok, _ = validate_hmd(data, off)
            if ok:
                results["HMD"]["valid"].append(off)
        elif b == MAGIC_TIM:
            results["TIM"]["raw"].append(off)
            ok, _ = validate_tim(data, off)
            if ok:
                results["TIM"]["valid"].append(off)

    return results


# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------

def main():
    path = Path(sys.argv[1]) if len(sys.argv) > 1 else Path(DEFAULT_EXE)

    if not path.is_file():
        print(f"Error: not found: {path}")
        sys.exit(1)

    data = path.read_bytes()
    n = len(data)

    print(f"[+] {path.name}")
    print(f"    size: {n:,} bytes  ({n / 1024:.1f} KB  /  {n / 1048576:.2f} MB)")
    print()

    print("    scanning ... ", end="", flush=True)
    res = scan(data)
    print("done\n")

    # ── TMD ────────────────────────────────────────────────────────────────
    tmd_raw   = res["TMD"]["raw"]
    tmd_valid = res["TMD"]["valid"]
    print(f"TMD  [41 00 00 00]: {len(tmd_raw):4d} raw hits  =>  {len(tmd_valid)} valid")
    if tmd_valid:
        show = tmd_valid[:5]
        print(f"  {'#':<4} {'offset':>10}  details")
        print(f"  {'-'*60}")
        for i, off in enumerate(show):
            summary = tmd_summary(data, off)
            print(f"  {i:<4} 0x{off:08X}  {summary}")
        if len(tmd_valid) > 5:
            print(f"  ... and {len(tmd_valid) - 5} more")

    # ── HMD ────────────────────────────────────────────────────────────────
    hmd_raw   = res["HMD"]["raw"]
    hmd_valid = res["HMD"]["valid"]
    print()
    print(f"HMD  [50 00 00 00]: {len(hmd_raw):4d} raw hits  =>  {len(hmd_valid)} valid")
    if hmd_valid:
        show = hmd_valid[:5]
        print(f"  {'#':<4} {'offset':>10}  details")
        print(f"  {'-'*60}")
        for i, off in enumerate(show):
            summary = hmd_summary(data, off)
            print(f"  {i:<4} 0x{off:08X}  {summary}")
        if len(hmd_valid) > 5:
            print(f"  ... and {len(hmd_valid) - 5} more")
    elif hmd_raw:
        print(f"  (first 5 raw offsets: {[hex(o) for o in hmd_raw[:5]]})")
        for off in hmd_raw[:3]:
            ok, reason = validate_hmd(data, off)
            print(f"    0x{off:08X}: INVALID ({reason})  raw={data[off:off+16].hex()}")

    # ── TIM ────────────────────────────────────────────────────────────────
    tim_raw   = res["TIM"]["raw"]
    tim_valid = res["TIM"]["valid"]
    print()
    print(f"TIM  [10 00 00 00]: {len(tim_raw):4d} raw hits  =>  {len(tim_valid)} valid")
    if tim_valid:
        show = tim_valid[:5]
        print(f"  {'#':<4} {'offset':>10}  details")
        print(f"  {'-'*60}")
        for i, off in enumerate(show):
            summary = tim_summary(data, off)
            print(f"  {i:<4} 0x{off:08X}  {summary}")
        if len(tim_valid) > 5:
            print(f"  ... and {len(tim_valid) - 5} more")

    print()
    print("-" * 50)
    print(f"  TMD raw={len(tmd_raw)}  valid={len(tmd_valid)}")
    print(f"  HMD raw={len(hmd_raw)}  valid={len(hmd_valid)}")
    print(f"  TIM raw={len(tim_raw)}  valid={len(tim_valid)}")
    print()


if __name__ == "__main__":
    main()
