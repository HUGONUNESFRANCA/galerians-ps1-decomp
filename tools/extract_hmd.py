#!/usr/bin/env python3
"""
extract_hmd.py - Extract and parse the 2 valid HMD blobs from SLUS_010.99.

HMD format observed in Galerians PS1 (SLUS_010.99):

  HMD Header (16 bytes):
    uint32  magic      = 0x00000050
    uint32  version    (1 or 2)
    uint32  prim_cnt   (total primitive count across all sections)
    uint32  coord_cnt  (= 0 in both blobs, no coordinate hierarchy)

  Object Entry Table (16 bytes each, until terminator):
    uint32  vert_count   (vertices for this object)
    uint32  flag         (usually 1; terminator when >= 3 or last entry)
    uint32  prim_count   (primitives for this object)
    uint32  reserved     (= 0)

  Data section (after table):
    - UV/texture layout header (16 bytes)
    - PS1 RAM pointer table: pairs of (ps1_address, count)
    - Primitive packets (sprite-like 12-byte records):
        int16  x, y    (screen position)
        uint8  r,g,b   (color)
        uint8  mode    (render flags)
        uint32 aux     (texture/UV data or zero)

Pointer address space:
  0x8019xxxx -> CLUT/palette data within the file (load_base=0x80010000)
  0x801ACCxx -> zero-initialised runtime transform buffers
  0x801ECxxx -> GPU DMA scratch buffers (beyond file end, runtime only)

Usage:
    python tools/extract_hmd.py [SLUS_010.99] [output_dir]
"""

import struct
import sys
from pathlib import Path

DEFAULT_EXE  = r"C:\Users\User\Desktop\Programação\T4\SLUS_010.99"
DEFAULT_OUT  = "port/assets/models_real"
LOAD_BASE    = 0x80010000   # PS1 executable load address


# ---------------------------------------------------------------------------
# Known HMD locations
# ---------------------------------------------------------------------------
KNOWN_HMDS = [
    (0x0016E6FC, "HMD_A", 0x0016E87C),   # (offset, name, end_exclusive)
    (0x0016E87C, "HMD_B", 0x0016F2E8),   # ends just before jump-table at 0x16F2E8
]


# ---------------------------------------------------------------------------
# HMD parsing
# ---------------------------------------------------------------------------

def parse_obj_table(data: bytes, offset: int) -> list[dict]:
    """Read 16-byte object entries until a terminator (flag >= 3 or runs out)."""
    entries = []
    pos = offset + 16          # skip header
    while pos + 16 <= len(data):
        vc, flag, pc, res = struct.unpack_from("<IIII", data, pos)
        entries.append({"vert_count": vc, "flag": flag,
                        "prim_count": pc, "reserved": res,
                        "file_offset": pos})
        pos += 16
        # Terminator: flag value >= 3 or we've hit the next HMD magic
        if flag >= 3:
            break
        # Safety: stop if we stumble on another HMD or clearly bad data
        if pos + 4 <= len(data) and data[pos:pos+4] == b"\x50\x00\x00\x00":
            break
    return entries


def collect_pointers(data: bytes, hmd_start: int, hmd_end: int) -> list[dict]:
    """Collect all PS1 RAM pointers embedded in the HMD region."""
    ptrs = []
    seen = set()
    for i in range(hmd_start, min(hmd_end, len(data)) - 4, 4):
        v = struct.unpack_from("<I", data, i)[0]
        if 0x80100000 <= v < 0x80210000 and v not in seen:
            seen.add(v)
            foff = v - LOAD_BASE
            in_file = 0 < foff < len(data)
            ptrs.append({"ps1": v, "file_offset": foff,
                         "hmd_relative": i - hmd_start,
                         "in_file": in_file})
    return sorted(ptrs, key=lambda p: p["ps1"])


def parse_prims(data: bytes, prim_start: int, prim_end: int) -> list[dict]:
    """Parse the primitive data section as 12-byte sprite records."""
    prims = []
    pos = prim_start
    while pos + 12 <= prim_end:
        x, y = struct.unpack_from("<hh", data, pos)
        r, g, b, mode = data[pos+4], data[pos+5], data[pos+6], data[pos+7]
        aux = struct.unpack_from("<I", data, pos + 8)[0]
        # Skip clearly invalid or all-zero records
        if (x, y) == (0, 0) and (r, g, b) == (0, 0, 0):
            pos += 4
            continue
        if not (-512 <= x <= 512 and -512 <= y <= 512):
            pos += 4
            continue
        prims.append({"x": x, "y": y, "r": r, "g": g, "b": b,
                      "mode": mode, "aux": aux, "file_offset": pos})
        pos += 12
    return prims


def find_ptr_table_end(data: bytes, ptr_start: int, blob_end: int) -> int:
    """Scan past the (ps1_addr, count) pointer table entries.

    Each pair is 8 bytes.  The table ends when we hit 8+ consecutive zero bytes
    or encounter data that no longer has PS1 addresses (0x8010_0000+ in col 0).
    """
    pos = ptr_start
    while pos + 8 <= blob_end:
        v0 = struct.unpack_from("<I", data, pos)[0]
        # If col-0 is a PS1 RAM address or a small index, we're still in the table
        if v0 >= 0x80100000 or v0 < 0x10000:
            pos += 8
        else:
            # Might be start of actual data
            break
    # Skip any padding zeros after the table
    while pos + 4 <= blob_end and data[pos:pos+4] == b"\x00\x00\x00\x00":
        pos += 4
    return pos


def find_prim_section(data: bytes, hmd_start: int, obj_entries: list,
                      blob_end: int) -> int:
    """Locate the start of the primitive data section.

    Layout inside blob:
      [header 16B] [obj table N*16B] [UV header 16B] [ptr table] [prim data]
    """
    table_end = hmd_start + 16 + len(obj_entries) * 16
    uv_end    = table_end + 16          # UV layout header
    return find_ptr_table_end(data, uv_end, blob_end)


# ---------------------------------------------------------------------------
# OBJ export (vertex-only, no face data — geometry not separately stored)
# ---------------------------------------------------------------------------

def export_obj(prims: list[dict], path: Path, name: str) -> None:
    """Write a minimal OBJ with sprite-centre positions as point cloud."""
    with open(path, "w") as f:
        f.write(f"# Galerians PS1 HMD: {name}\n")
        f.write(f"# {len(prims)} sprite records (2D screen-space)\n")
        f.write("o " + name + "\n")
        for p in prims:
            # Use x,y as 2D coords; z=0 (no depth data)
            f.write(f"v {p['x']}.0 {p['y']}.0 0.0\n")
        # No faces — these are point records
        for i in range(1, len(prims) + 1):
            f.write(f"p {i}\n")


# ---------------------------------------------------------------------------
# Main extraction
# ---------------------------------------------------------------------------

def extract_hmd(data: bytes, hmd_offset: int, name: str,
                blob_end: int, out_dir: Path) -> None:
    magic, version, prim_cnt, coord_cnt = \
        struct.unpack_from("<IIII", data, hmd_offset)

    print(f"\n{'='*60}")
    print(f"  {name}  @  0x{hmd_offset:08X}")
    print(f"{'='*60}")
    print(f"  magic=0x{magic:08X}  version={version}"
          f"  prim_cnt={prim_cnt}  coord_cnt={coord_cnt}")

    # --- Object entry table ---
    entries = parse_obj_table(data, hmd_offset)
    table_end = hmd_offset + 16 + len(entries) * 16
    total_verts = sum(e["vert_count"] for e in entries)
    total_prims = sum(e["prim_count"] for e in entries)

    print(f"\n  Object table: {len(entries)} entries  (ends 0x{table_end:08X})")
    print(f"  {'#':<4} {'verts':>6} {'flag':>5} {'prims':>6}  {'file_offset':>12}")
    print(f"  {'-'*46}")
    for i, e in enumerate(entries):
        term = " <-- terminator" if e["flag"] >= 3 else ""
        print(f"  {i:<4} {e['vert_count']:>6} {e['flag']:>5} {e['prim_count']:>6}"
              f"  0x{e['file_offset']:08X}{term}")
    print(f"  {'tot':4} {total_verts:>6} {'':>5} {total_prims:>6}")

    blob_size = blob_end - hmd_offset
    print(f"\n  Blob: 0x{hmd_offset:08X} - 0x{blob_end:08X}  ({blob_size} bytes)")

    # --- PS1 RAM pointers in blob ---
    ptrs    = collect_pointers(data, hmd_offset, blob_end)
    in_file = [p for p in ptrs if p["in_file"]]
    print(f"\n  PS1 RAM pointers: {len(ptrs)} unique  "
          f"({len(in_file)} resolve inside file)")
    for p in in_file[:10]:
        foff  = p["file_offset"]
        chunk = data[foff:foff+8] if foff + 8 <= len(data) else b""
        vals  = list(struct.unpack_from("<4h", chunk)) if len(chunk) == 8 else []
        if all(v in (0, -512, 6, 2047) for v in vals):
            tag = "clut/palette"
        elif all(v == 0 for v in vals):
            tag = "zeros/runtime-buf"
        else:
            tag = f"data{vals}"
        print(f"    0x{p['ps1']:08X} -> file 0x{foff:06X}"
              f"  HMD+0x{p['hmd_relative']:04X}  [{tag}]")
    runtime_only = [p for p in ptrs if not p["in_file"]]
    if runtime_only:
        print(f"    ({len(runtime_only)} pointers beyond file end = GPU/runtime buffers)")

    # --- Primitive / sprite section ---
    prim_start = find_prim_section(data, hmd_offset, entries, blob_end)
    prims = parse_prims(data, prim_start, blob_end)
    prim_bytes = blob_end - prim_start
    print(f"\n  Primitive section: 0x{prim_start:08X}  "
          f"({prim_start - hmd_offset} bytes into blob, {prim_bytes} bytes remain)")
    if prims:
        print(f"  Sprite records: {len(prims)}")
        print(f"  {'#':<4} {'x':>5} {'y':>5}  {'rgb':<14} {'mode':>4}  {'aux':>10}")
        print(f"  {'-'*54}")
        for i, p in enumerate(prims[:20]):
            rgb = f"({p['r']},{p['g']},{p['b']})"
            print(f"  {i:<4} {p['x']:>5} {p['y']:>5}  {rgb:<14} 0x{p['mode']:02X}"
                  f"  0x{p['aux']:08X}")
        if len(prims) > 20:
            print(f"  ... ({len(prims)-20} more records)")
    else:
        print("  (no sprite records matched the filter)")

    # --- Save ---
    out_dir.mkdir(parents=True, exist_ok=True)

    raw_path = out_dir / f"{name}.hmd"
    blob = data[hmd_offset:blob_end]
    raw_path.write_bytes(blob)
    print(f"\n  Saved: {raw_path}  ({len(blob)} bytes)")

    if prims:
        obj_path = out_dir / f"{name}.obj"
        export_obj(prims, obj_path, name)
        print(f"  Saved: {obj_path}  ({len(prims)} points)")


def main():
    exe_path = Path(sys.argv[1]) if len(sys.argv) > 1 else Path(DEFAULT_EXE)
    out_dir  = Path(sys.argv[2]) if len(sys.argv) > 2 else Path(DEFAULT_OUT)

    if not exe_path.is_file():
        print(f"Error: not found: {exe_path}")
        sys.exit(1)

    data = exe_path.read_bytes()
    print(f"[+] {exe_path.name}  ({len(data):,} bytes)")

    for offset, name, end in KNOWN_HMDS:
        extract_hmd(data, offset, name, end, out_dir)

    print()


if __name__ == "__main__":
    main()
