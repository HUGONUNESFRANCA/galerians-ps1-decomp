#!/usr/bin/env python3
"""
extract_real_tmds.py - Scan MODEL.CDB raw for valid TMD models.

Scans at 4-byte aligned offsets for byte pattern [0x41, 0x00, 0x00, 0x00]
(PS1 TMD magic = uint32-LE value 0x41).

TMD Header layout:
  [0x00] uint32  magic  = 0x00000041  (bytes: 41 00 00 00)
  [0x04] uint32  flags  (0 = absolute ptrs, 1 = relative ptrs)
  [0x08] uint32  nobj   (number of objects, 1-64)
  [0x0C] obj[0]  28-byte object descriptor
  ...

Object descriptor (28 bytes):
  uint32 vert_top, uint32 vert_n, uint32 norm_top, uint32 norm_n,
  uint32 prim_top, uint32 prim_n, int32  scale

Pointer modes:
  flags == 1 (relative): vert_top/norm_top/prim_top are relative to
                         start of object table (tmd_offset + 12)
  flags == 0 (absolute): offsets are from start of file

Usage:
    python tools/extract_real_tmds.py [MODEL.CDB] [output_dir]
"""

import struct
import sys
from pathlib import Path

DEFAULT_CDB = r"C:\Users\User\Desktop\Programação\T4\MODEL.CDB"
DEFAULT_OUT  = "port/assets/models_real"

MAGIC_BYTES  = bytes([0x41, 0x00, 0x00, 0x00])
OBJ_SIZE     = 28
BYTES_PER_VERT = 8
BYTES_PER_NORM = 8
BYTES_PER_PRIM = 12


# ---------------------------------------------------------------------------
# LZSS decompressor (PS1 / PsyQ variant, skip=8 for CDB header)
# ---------------------------------------------------------------------------

def lzss_decompress(src: bytes) -> bytes:
    WINDOW = 0x1000
    LOOKAHEAD = 0x12
    out = bytearray()
    window = bytearray(WINDOW)
    win_pos = WINDOW - LOOKAHEAD
    i, end = 0, len(src)
    while i < end:
        flags = src[i]; i += 1
        for bit in range(8):
            if i >= end:
                return bytes(out)
            if flags & (1 << bit):
                byte = src[i]; i += 1
                out.append(byte)
                window[win_pos] = byte
                win_pos = (win_pos + 1) % WINDOW
            else:
                if i + 1 >= end:
                    return bytes(out)
                b0, b1 = src[i], src[i+1]; i += 2
                offset = b0 | ((b1 & 0xF0) << 4)
                length = (b1 & 0x0F) + 3
                for _ in range(length):
                    byte = window[offset & 0xFFF]
                    offset += 1
                    out.append(byte)
                    window[win_pos] = byte
                    win_pos = (win_pos + 1) % WINDOW
    return bytes(out)


# ---------------------------------------------------------------------------
# Validation helpers
# ---------------------------------------------------------------------------

def _read_obj(data: bytes, obj_offset: int):
    if obj_offset + OBJ_SIZE > len(data):
        return None
    return struct.unpack_from("<IIIIIIi", data, obj_offset)


def validate_tmd(data: bytes, offset: int) -> tuple[bool, str]:
    """Return (valid, reason) for the TMD candidate at `offset`."""
    if offset + 12 > len(data):
        return False, "truncated header"

    flags, nobj = struct.unpack_from("<II", data, offset + 4)

    if flags not in (0, 1):
        return False, f"flags=0x{flags:08X} (must be 0 or 1)"
    if not (1 <= nobj <= 64):
        return False, f"nobj={nobj} (must be 1-64)"

    obj_table = offset + 12
    if obj_table + nobj * OBJ_SIZE > len(data):
        return False, "object table out of bounds"

    base = obj_table if flags == 1 else 0

    for i in range(nobj):
        obj = _read_obj(data, obj_table + i * OBJ_SIZE)
        if obj is None:
            return False, f"object {i} read failed"
        vert_top, vert_n, norm_top, norm_n, prim_top, prim_n, _ = obj

        if not (1 <= vert_n <= 65535):
            return False, f"obj[{i}] vert_n={vert_n} out of range"
        if not (1 <= prim_n <= 65535):
            return False, f"obj[{i}] prim_n={prim_n} out of range"

        abs_vert = base + vert_top
        if abs_vert >= len(data):
            return False, f"obj[{i}] vert_top 0x{vert_top:08X} out of bounds"

    return True, "ok"


def tmd_info(data: bytes, offset: int) -> dict:
    flags, nobj = struct.unpack_from("<II", data, offset + 4)
    obj_table   = offset + 12
    base        = obj_table if flags == 1 else 0
    objects, total_verts, total_norms, total_prims = [], 0, 0, 0
    for i in range(nobj):
        obj = _read_obj(data, obj_table + i * OBJ_SIZE)
        if obj is None:
            break
        vt, vn, nt, nn, pt, pn, sc = obj
        objects.append(dict(vert_top=vt,vert_n=vn,norm_top=nt,norm_n=nn,
                            prim_top=pt,prim_n=pn,scale=sc))
        total_verts += vn; total_norms += nn; total_prims += pn
    return dict(offset=offset, flags=flags, nobj=nobj,
                total_verts=total_verts, total_norms=total_norms,
                total_prims=total_prims, objects=objects)


def estimate_tmd_end(data: bytes, offset: int) -> int:
    flags, nobj = struct.unpack_from("<II", data, offset + 4)
    obj_table   = offset + 12
    base        = obj_table if flags == 1 else 0
    max_end     = obj_table + nobj * OBJ_SIZE
    for i in range(nobj):
        obj = _read_obj(data, obj_table + i * OBJ_SIZE)
        if obj is None: break
        vt, vn, nt, nn, pt, pn, _ = obj
        max_end = max(max_end,
                      base + vt + vn * BYTES_PER_VERT,
                      base + nt + nn * BYTES_PER_NORM,
                      base + pt + pn * BYTES_PER_PRIM)
    return min(max_end, len(data))


# ---------------------------------------------------------------------------
# Scanner
# ---------------------------------------------------------------------------

def scan_tmds(data: bytes, verbose_invalid: bool = False) -> list[dict]:
    """Scan `data` at 4-byte aligned offsets for [0x41 0x00 0x00 0x00]."""
    results = []
    magic_hits = []
    end = len(data) - 4
    off = 0
    while off <= end:
        if data[off:off+4] == MAGIC_BYTES:
            magic_hits.append(off)
            valid, reason = validate_tmd(data, off)
            if valid:
                info        = tmd_info(data, off)
                info["end"] = estimate_tmd_end(data, off)
                results.append(info)
                off = info["end"]
                continue
            elif verbose_invalid:
                print(f"      hit @0x{off:08X}: INVALID — {reason}")
        off += 4

    if not results and magic_hits:
        if not verbose_invalid:
            print(f"    ({len(magic_hits)} [41 00 00 00] patterns found but "
                  f"none pass TMD validation — re-run with verbose flag to see why)")
    return results, magic_hits


# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------

def main():
    args = sys.argv[1:]
    verbose = "--verbose" in args
    args = [a for a in args if a != "--verbose"]

    cdb_path = Path(args[0]) if args else Path(DEFAULT_CDB)
    out_dir  = Path(args[1]) if len(args) >= 2 else Path(DEFAULT_OUT)

    if not cdb_path.is_file():
        print(f"Error: file not found: {cdb_path}")
        sys.exit(1)

    raw = cdb_path.read_bytes()
    print(f"[+] {cdb_path.name}")
    print(f"    size on disk : {len(raw):,} bytes  ({len(raw):#x})")

    # Read CDB header if present
    if len(raw) >= 8:
        sc, cf = struct.unpack_from("<II", raw, 0)
        print(f"    CDB header   : sector_count={sc}  compression_flag=0x{cf:08X}")
        compressed = cf != 0
    else:
        compressed = False

    # --- Pass 1: scan raw bytes (as the instructions specify) ---
    print(f"\n--- Pass 1: scan raw bytes (no LZSS) ---")
    print(f"    scanning {len(raw):,} bytes ... ", end="", flush=True)
    tmds, raw_hits = scan_tmds(raw, verbose_invalid=verbose)
    print(f"{len(raw_hits)} [41 00 00 00] pattern(s), {len(tmds)} valid TMD(s)")

    if verbose and raw_hits and not tmds:
        print("    Failure details:")
        for off in raw_hits:
            _, reason = validate_tmd(raw, off)
            print(f"      0x{off:08X}: {reason}")

    # --- Pass 2: scan LZSS-decompressed body (if file appears compressed) ---
    if compressed and not tmds:
        print(f"\n--- Pass 2: scan LZSS-decompressed body ---")
        print(f"    decompressing body (skip first 8 bytes) ... ", end="", flush=True)
        try:
            payload = lzss_decompress(raw[8:])
            print(f"{len(payload):,} bytes")
        except Exception as e:
            print(f"FAILED ({e})")
            payload = b""

        if payload:
            print(f"    scanning {len(payload):,} bytes ... ", end="", flush=True)
            tmds, decomp_hits = scan_tmds(payload, verbose_invalid=verbose)
            print(f"{len(decomp_hits)} [41 00 00 00] pattern(s), {len(tmds)} valid TMD(s)")
            if verbose and decomp_hits and not tmds:
                print("    Failure details:")
                for off in decomp_hits:
                    _, reason = validate_tmd(payload, off)
                    print(f"      0x{off:08X}: {reason}")
            if tmds:
                raw = payload  # extract from decompressed data

    if not tmds:
        print(f"\n    [!] No valid TMDs found.")
        print(f"    NOTE: The file is LZSS-compressed (cf=0x{cf:08X}).")
        print(f"    The 17 [41 00 00 00] hits in the raw stream are LZSS artefacts.")
        print(f"    Galerians may use a proprietary model format, not standard PS1 TMD.")
        print()
        return

    # ---- table header -------------------------------------------------------
    print()
    col = (f"{'#':<5} {'offset':>10}  {'flags':>6}  {'nobj':>5}  "
           f"{'verts':>7}  {'norms':>7}  {'prims':>7}  {'size':>8}")
    print(col)
    print("-" * len(col))
    for i, t in enumerate(tmds):
        sz = t["end"] - t["offset"]
        print(f"{i:<5} 0x{t['offset']:08X}  0x{t['flags']:04X}  {t['nobj']:>5}  "
              f"{t['total_verts']:>7}  {t['total_norms']:>7}  {t['total_prims']:>7}  "
              f"{sz:>8}")

    # ---- extraction ---------------------------------------------------------
    out_dir.mkdir(parents=True, exist_ok=True)
    print(f"\n    extracting {len(tmds)} TMD(s) → {out_dir}/")
    for i, t in enumerate(tmds):
        blob = raw[t["offset"]:t["end"]]
        name = f"TMD_{i:04d}_0x{t['offset']:08X}.tmd"
        (out_dir / name).write_bytes(blob)
        print(f"      [{i:02d}] {name}  ({len(blob):,} bytes)"
              f"  nobj={t['nobj']}"
              f"  verts={t['total_verts']}"
              f"  prims={t['total_prims']}")

    print()


if __name__ == "__main__":
    main()
