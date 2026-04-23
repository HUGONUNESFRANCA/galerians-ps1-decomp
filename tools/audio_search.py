"""
audio_search.py — Galerians PS1 audio system finder

Scans a PS1 RAM dump for audio-related structures and code references:

  1. SPU register references — finds the SPU base address (0x1F801C00)
     as a 4-byte literal AND as a MIPS lui $rX, 0x1F80 instruction.
     Reports the inferred enclosing function for each hit.

  2. VAG audio headers — PS1 audio files begin with the "VAGp" magic.
     Parses version, data length, sample rate, and name from the header.

  3. XA CD-ROM sector markers — raw CD sectors (sync: 00 FF×10 00) found
     in RAM indicate XA-ADPCM audio was DMA'd in for FMV streaming.

  4. Sound effect pointer tables — arrays of 4+ consecutive pointers in
     PS1 RAM range (0x80100000–0x801FFFFF). Flags entries whose targets
     contain a VAGp header.

Usage:
    python tools/audio_search.py [--dump PATH] [--out PATH]

Defaults:
    --dump  "C:/Users/User/Desktop/Coisas sobre Jogos/Emuladores/PS1/Dump/ram.bin"
    --out   tools/audio_search_results.txt

PS1 RAM base: 0x80000000 (file offset 0 in raw dump)
"""

import argparse
import struct
import sys
from dataclasses import dataclass, field
from pathlib import Path
from typing import List, Optional

# ── Constants ──────────────────────────────────────────────────────────────────

PS1_RAM_BASE   = 0x80000000
PS1_RAM_SIZE   = 0x200000   # 2 MB

# SPU hardware address space
SPU_BASE       = 0x1F801C00
SPU_BASE_LE    = b'\x00\x1C\x80\x1F'  # full address in little-endian
SPU_HI_BYTE01  = bytes([0x80, 0x1F])  # upper 16-bit half (lui immediate, LE bytes 0-1)

# VAG audio file magic
VAG_MAGIC      = b'VAGp'

# CD-ROM XA sector sync: 00 FF FF FF FF FF FF FF FF FF FF 00
XA_SYNC        = b'\x00' + b'\xFF' * 10 + b'\x00'
XA_SECTOR_SIZE = 2352  # raw CD sector size

# Pointer-table detection
PTR_LO         = 0x80100000
PTR_HI         = 0x801FFFFF
TABLE_MIN_PTRS = 4     # minimum consecutive pointers to flag as a table

# MIPS addiu $sp,$sp,-N prologue (function entry heuristic)
ADDIU_SP_BYTE2 = 0xBD   # byte[2] of LE addiu $sp,$sp,-N  (rt and rs = $sp = 29)
ADDIU_SP_BYTE3 = 0x27   # byte[3] = opcode byte (addiu = 0x09, upper bits in LE word)
PROLOGUE_SEARCH_BACK = 0x400  # max bytes to scan backwards for a prologue

# ── Helpers ────────────────────────────────────────────────────────────────────

def ps1_to_offset(addr: int) -> int:
    return addr - PS1_RAM_BASE

def offset_to_ps1(offset: int) -> int:
    return PS1_RAM_BASE + offset

def read_u32_le(data: bytes, offset: int) -> int:
    return struct.unpack_from("<I", data, offset)[0]

def read_u32_be(data: bytes, offset: int) -> int:
    return struct.unpack_from(">I", data, offset)[0]

def is_ps1_ptr(val: int) -> bool:
    return PTR_LO <= val <= PTR_HI

def in_bounds(data: bytes, offset: int, size: int = 1) -> bool:
    return 0 <= offset and offset + size <= len(data)

# ── Function-start heuristic ───────────────────────────────────────────────────

def find_enclosing_function(data: bytes, ref_offset: int) -> Optional[int]:
    """
    Scan backwards from ref_offset for a MIPS function prologue.
    Pattern: addiu $sp, $sp, -N (N > 0)
      LE encoding: [imm_lo] [imm_hi≥0x80] [0xBD] [0x27]
      (opcode=0x27 ← bits 31:26=001001; rs=rt=$sp=29 ← byte[2]=0xBD;
       negative imm ← byte[1] bit 7 set)
    Returns the PS1 address of the prologue instruction, or None.
    """
    base = max(0, ref_offset - PROLOGUE_SEARCH_BACK) & ~3
    for off in range(ref_offset & ~3, base, -4):
        if off + 4 > len(data):
            continue
        b = data[off:off + 4]
        if b[2] == ADDIU_SP_BYTE2 and b[3] == ADDIU_SP_BYTE3 and (b[1] & 0x80):
            return offset_to_ps1(off)
    return None

# ── Result dataclasses ─────────────────────────────────────────────────────────

@dataclass
class SpuRef:
    ps1_addr:   int
    kind:       str            # "direct_ptr" | "lui_instr"
    func_start: Optional[int]  # enclosing function (heuristic)
    note:       str

@dataclass
class VagHeader:
    ps1_addr:    int
    version:     int
    data_len:    int
    sample_rate: int
    name:        str

@dataclass
class XaMarker:
    ps1_addr:   int
    raw_header: bytes  # 16 bytes after sync (address, mode, subheader)

@dataclass
class SoundTable:
    ps1_addr:  int
    ptr_count: int
    pointers:  List[int]       # capped at 16 for the report
    vag_count: int             # how many pointers target a VAGp header
    note:      str

# ── Scanners ───────────────────────────────────────────────────────────────────

def scan_spu_refs(data: bytes) -> List[SpuRef]:
    """
    Two passes:
      A) Direct literal: 4-byte value 0x1F801C00 anywhere (data tables, code).
      B) MIPS instruction: lui $rX, 0x1F80
           LE bytes: [0x80] [0x1F] [rt_field] [0x3C]
           (opcode byte 0x3C ← bits 31:26=001111; rs=0; rt in byte[2] bits 4:0)
         Scanned at 4-byte-aligned offsets only (MIPS ISA guarantee).
    """
    results = []
    seen = set()

    # Pass A — direct literal
    pos = 0
    while True:
        idx = data.find(SPU_BASE_LE, pos)
        if idx < 0:
            break
        ps1 = offset_to_ps1(idx)
        if ps1 not in seen:
            seen.add(ps1)
            func = find_enclosing_function(data, idx)
            results.append(SpuRef(
                ps1_addr=ps1,
                kind="direct_ptr",
                func_start=func,
                note=f"0x1F801C00 literal",
            ))
        pos = idx + 4

    # Pass B — lui $rX, 0x1F80 instruction
    # Instruction layout (32-bit LE): byte[0]=0x80  byte[1]=0x1F  byte[2]=rt  byte[3]=0x3C
    # Additionally: upper 3 bits of byte[2] must be 0 (they encode rs bits 23:21; lui has rs=0)
    for off in range(0, len(data) - 4, 4):
        b = data[off:off + 4]
        if (b[0] == 0x80 and b[1] == 0x1F
                and b[3] == 0x3C
                and (b[2] >> 5) == 0):        # rs=0 sanity check
            ps1 = offset_to_ps1(off)
            if ps1 not in seen:
                seen.add(ps1)
                rt = b[2] & 0x1F              # destination register
                func = find_enclosing_function(data, off)
                results.append(SpuRef(
                    ps1_addr=ps1,
                    kind="lui_instr",
                    func_start=func,
                    note=f"lui $r{rt}, 0x1F80  (SPU base upper half)",
                ))

    return sorted(results, key=lambda r: r.ps1_addr)


def scan_vag_headers(data: bytes) -> List[VagHeader]:
    """
    Find VAG audio headers (magic bytes 'VAGp' = 56 41 47 70).
    VAG header layout (fields are big-endian):
      +0x00  4  magic   "VAGp"
      +0x04  4  version
      +0x08  4  reserved
      +0x0C  4  data length (bytes)
      +0x10  4  sample rate (Hz)
      +0x14  12 reserved
      +0x20  16 name (ASCII, null-padded)
      +0x30  …  ADPCM data
    """
    results = []
    pos = 0
    while True:
        idx = data.find(VAG_MAGIC, pos)
        if idx < 0:
            break
        if in_bounds(data, idx, 0x30):
            try:
                version     = read_u32_be(data, idx + 0x04)
                data_len    = read_u32_be(data, idx + 0x0C)
                sample_rate = read_u32_be(data, idx + 0x10)
                raw_name    = data[idx + 0x20: idx + 0x30]
                name        = raw_name.split(b'\x00')[0].decode('ascii', errors='replace')
            except Exception:
                pos = idx + 1
                continue
            results.append(VagHeader(
                ps1_addr=offset_to_ps1(idx),
                version=version,
                data_len=data_len,
                sample_rate=sample_rate,
                name=name,
            ))
        pos = idx + 4
    return results


def scan_xa_markers(data: bytes) -> List[XaMarker]:
    """
    Find CD-ROM XA sector sync patterns in RAM (00 FF×10 00).
    Presence in main RAM means raw sectors were DMA'd in for streaming.
    The 16 bytes following the sync contain the sector address, mode,
    and XA subheader (file number, channel, coding info).
    """
    results = []
    pos = 0
    while True:
        idx = data.find(XA_SYNC, pos)
        if idx < 0:
            break
        raw_hdr = data[idx + 12: idx + 28] if in_bounds(data, idx, 28) else b''
        results.append(XaMarker(
            ps1_addr=offset_to_ps1(idx),
            raw_header=raw_hdr,
        ))
        # Sectors are at least 2048 bytes apart; advance past this hit
        pos = idx + len(XA_SYNC)
    return results


def scan_sound_tables(data: bytes, vag_offsets: set) -> List[SoundTable]:
    """
    Slide over 4-byte-aligned positions; collect runs of TABLE_MIN_PTRS or
    more consecutive uint32 values all in the PS1 RAM pointer range.
    Cross-checks each pointer target against known VAGp offsets.
    """
    results = []
    off = 0
    # Align to start
    if off % 4:
        off += 4 - (off % 4)

    while off + 4 <= len(data):
        val = read_u32_le(data, off)
        if not is_ps1_ptr(val):
            off += 4
            continue

        # Collect consecutive valid pointers
        table_start = off
        ptrs: List[int] = []
        tmp = off
        while tmp + 4 <= len(data):
            v = read_u32_le(data, tmp)
            if not is_ps1_ptr(v):
                break
            ptrs.append(v)
            tmp += 4

        if len(ptrs) >= TABLE_MIN_PTRS:
            ps1 = offset_to_ps1(table_start)
            # How many pointers lead directly to a VAGp header?
            vag_hits = sum(
                1 for p in ptrs
                if ps1_to_offset(p) in vag_offsets
            )
            note_parts = [f"{len(ptrs)} consecutive PS1 pointers"]
            if vag_hits:
                note_parts.append(f"{vag_hits} → VAGp")
            results.append(SoundTable(
                ps1_addr=ps1,
                ptr_count=len(ptrs),
                pointers=ptrs[:16],
                vag_count=vag_hits,
                note=", ".join(note_parts),
            ))
            off = tmp   # jump past the table
        else:
            off += 4

    # Sort: VAGp tables first, then by table size descending
    results.sort(key=lambda t: (0 if t.vag_count else 1, -t.ptr_count))
    return results

# ── Report writer ──────────────────────────────────────────────────────────────

def write_report(
    spu_refs:     List[SpuRef],
    vag_headers:  List[VagHeader],
    xa_markers:   List[XaMarker],
    sound_tables: List[SoundTable],
    out_path:     Path,
) -> None:

    with open(out_path, "w", encoding="utf-8") as f:

        f.write("Galerians PS1 — Audio System Search Results\n")
        f.write("=" * 62 + "\n\n")
        f.write(f"  SPU register references : {len(spu_refs)}\n")
        f.write(f"  VAG audio headers       : {len(vag_headers)}\n")
        f.write(f"  XA CD-ROM markers       : {len(xa_markers)}\n")
        f.write(f"  Sound pointer tables    : {len(sound_tables)}\n\n")

        # ── Section 1: SPU refs ───────────────────────────────────────────────
        f.write("─" * 62 + "\n")
        f.write(f"1. SPU REGISTER REFERENCES (0x1F801C00)  —  {len(spu_refs)} hits\n")
        f.write("─" * 62 + "\n\n")

        if spu_refs:
            f.write(f"  {'Address':12s}  {'Kind':12s}  {'Func ~':12s}  Note\n")
            f.write(f"  {'-------':12s}  {'----':12s}  {'------':12s}  ----\n")
            for r in spu_refs:
                fn = f"0x{r.func_start:08X}" if r.func_start else "unknown"
                f.write(f"  0x{r.ps1_addr:08X}  {r.kind:12s}  {fn:12s}  {r.note}\n")

            # Group by enclosing function
            func_map: dict = {}
            for r in spu_refs:
                key = r.func_start if r.func_start else 0
                func_map.setdefault(key, []).append(r)

            f.write(f"\n  Functions touching SPU ({len(func_map)} unique):\n")
            for fn_addr, refs in sorted(func_map.items()):
                fn_str = f"0x{fn_addr:08X}" if fn_addr else "(unknown)"
                kinds = ", ".join(sorted({r.kind for r in refs}))
                f.write(f"    {fn_str}  ({len(refs)} ref{'s' if len(refs)>1 else ''}: {kinds})\n")
        else:
            f.write("  (none found)\n")
        f.write("\n")

        # ── Section 2: VAG headers ────────────────────────────────────────────
        f.write("─" * 62 + "\n")
        f.write(f"2. VAG AUDIO HEADERS ('VAGp')  —  {len(vag_headers)} hits\n")
        f.write("─" * 62 + "\n\n")

        if vag_headers:
            f.write(f"  {'Address':12s}  {'Rate':8s}  {'Len (bytes)':12s}  {'Ver':10s}  Name\n")
            f.write(f"  {'-------':12s}  {'----':8s}  {'-----------':12s}  {'---':10s}  ----\n")
            for v in vag_headers:
                f.write(
                    f"  0x{v.ps1_addr:08X}  "
                    f"{v.sample_rate:>6} Hz  "
                    f"0x{v.data_len:08X}    "
                    f"0x{v.version:08X}  "
                    f"'{v.name}'\n"
                )
        else:
            f.write("  (none found)\n")
        f.write("\n")

        # ── Section 3: XA markers ─────────────────────────────────────────────
        f.write("─" * 62 + "\n")
        f.write(f"3. XA CD-ROM SECTOR MARKERS  —  {len(xa_markers)} hits\n")
        f.write("─" * 62 + "\n\n")

        if xa_markers:
            for x in xa_markers:
                hdr = x.raw_header
                if len(hdr) >= 4:
                    # Bytes 0-2: address (min/sec/sector); byte 3: mode
                    addr_str = f"{hdr[0]:02X}:{hdr[1]:02X}:{hdr[2]:02X}"
                    mode     = hdr[3]
                    subhdr   = hdr[4:8].hex(' ') if len(hdr) >= 8 else "?"
                    f.write(
                        f"  0x{x.ps1_addr:08X}  "
                        f"CD addr {addr_str}  mode={mode}  subheader: {subhdr}\n"
                    )
                else:
                    f.write(f"  0x{x.ps1_addr:08X}  (header truncated)\n")
        else:
            f.write("  (none found)\n")
        f.write("\n")

        # ── Section 4: Sound tables ───────────────────────────────────────────
        f.write("─" * 62 + "\n")
        f.write(f"4. SOUND EFFECT POINTER TABLES  —  {len(sound_tables)} candidates\n")
        f.write("─" * 62 + "\n\n")

        if sound_tables:
            for t in sound_tables:
                marker = " *** VAGp" if t.vag_count else ""
                f.write(f"  0x{t.ps1_addr:08X}  {t.note}{marker}\n")
                shown = t.pointers[:8]
                ptr_str = "  ".join(f"0x{p:08X}" for p in shown)
                f.write(f"    [{ptr_str}")
                if t.ptr_count > 8:
                    f.write(f"  ... +{t.ptr_count - 8} more")
                f.write("]\n\n")
        else:
            f.write("  (none found)\n")
        f.write("\n")

        # ── Next steps ────────────────────────────────────────────────────────
        f.write("=" * 62 + "\n")
        f.write("NEXT STEPS\n")
        f.write("=" * 62 + "\n")
        f.write("""
SPU Register References:
  1. Open each 'lui_instr' address in Ghidra (G → address).
     Each lui $rX, 0x1F80 starts a SPU register access sequence.
  2. The following ori/addiu narrows the offset to the specific
     register (SPUCNT, voice pitch, volume, ADSR, etc.).
     Cross-reference with audio.h SPU_* constants.
  3. 'direct_ptr' hits are likely in data tables (jump tables,
     DMA descriptors) — check context for surrounding pointers.
  4. Functions found by the prologue heuristic: verify in Ghidra
     by checking they have a matching epilogue (jr $ra / addiu $sp).

VAG Audio Headers:
  1. VAG headers in main RAM = audio was loaded from CD into a staging
     buffer before being transferred to SPU RAM via DMA.
  2. The 'name' field often matches the original sound file name.
  3. Set a DuckStation Write watchpoint on the VAG address to catch
     the exact moment the audio loader copies it in.
  4. From the watchpoint, trace back to find Audio_SetChannel or
     any helper that calls SpuWrite / DMA transfer to SPU RAM.

XA CD-ROM Markers:
  1. XA sync bytes in RAM = raw Mode 2/Form 2 sectors were DMA'd
     directly into memory for FMV audio streaming.
  2. Subheader bytes encode: [file number] [channel] [submode] [coding].
     Submode 0x64 = XA audio; submode 0x21 = video (MDEC).
  3. This is the FMV streaming ring buffer — document its base and
     size for the port's XA → OpenAL streaming replacement.

Sound Tables:
  1. Tables flagged '→ VAGp' are the highest-priority audio assets.
  2. Open the table base in Ghidra, find XREFs to locate the loader
     function that indexes the table (likely called from Audio_SetChannel
     or a coroutine spawned by it).
  3. The entry count gives the number of SFX/BGM clips per category.
  4. For the port: each table entry becomes an OpenAL buffer handle.
""")

# ── Main ───────────────────────────────────────────────────────────────────────

def main() -> None:
    default_dump = (
        r"C:\Users\User\Desktop\Coisas sobre Jogos\Emuladores\PS1\Dump\ram.bin"
    )
    default_out = Path(__file__).parent / "audio_search_results.txt"

    ap = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    ap.add_argument("--dump", default=default_dump,
                    help="Path to raw PS1 RAM dump  (default: %(default)s)")
    ap.add_argument("--out",  default=str(default_out),
                    help="Output report path  (default: %(default)s)")
    args = ap.parse_args()

    dump_path = Path(args.dump)
    out_path  = Path(args.out)

    if not dump_path.exists():
        sys.exit(f"ERROR: dump not found: {dump_path}")

    data = dump_path.read_bytes()
    if len(data) < 0x100000:
        sys.exit(
            f"ERROR: dump too small ({len(data)} bytes) — expected 2 MB PS1 RAM dump"
        )

    print(f"Loaded {len(data):,} bytes from {dump_path.name}")

    print("Scanning for SPU register references (0x1F801C00 / lui 0x1F80)...")
    spu_refs = scan_spu_refs(data)
    lui_count = sum(1 for r in spu_refs if r.kind == "lui_instr")
    ptr_count = sum(1 for r in spu_refs if r.kind == "direct_ptr")
    print(f"  {len(spu_refs)} total  ({lui_count} lui instructions, {ptr_count} direct pointers)")

    print("Scanning for VAG audio headers ('VAGp')...")
    vag_headers = scan_vag_headers(data)
    print(f"  {len(vag_headers)} VAG headers found")

    print("Scanning for XA CD-ROM sector markers...")
    xa_markers = scan_xa_markers(data)
    print(f"  {len(xa_markers)} XA markers found")

    print("Scanning for sound effect pointer tables...")
    vag_offsets = {ps1_to_offset(v.ps1_addr) for v in vag_headers}
    sound_tables = scan_sound_tables(data, vag_offsets)
    vag_tables = sum(1 for t in sound_tables if t.vag_count)
    print(f"  {len(sound_tables)} candidate tables  ({vag_tables} with VAGp pointers)")

    write_report(spu_refs, vag_headers, xa_markers, sound_tables, out_path)
    print(f"\nReport saved to: {out_path}")

    # Console summary
    print("\n-- Summary --------------------------------------------------")
    print(f"  SPU refs     : {len(spu_refs):4d}  "
          f"({lui_count} lui / {ptr_count} direct)")
    print(f"  VAG headers  : {len(vag_headers):4d}")
    print(f"  XA markers   : {len(xa_markers):4d}")
    print(f"  Sound tables : {len(sound_tables):4d}  "
          f"({vag_tables} with VAGp pointers)")

    if spu_refs:
        func_addrs = sorted({r.func_start for r in spu_refs if r.func_start})
        print(f"\nInferred SPU functions ({len(func_addrs)}):")
        for fa in func_addrs[:8]:
            count = sum(1 for r in spu_refs if r.func_start == fa)
            print(f"  0x{fa:08X}  ({count} ref{'s' if count > 1 else ''})")

    if vag_headers:
        print(f"\nVAG headers (first 5):")
        for v in vag_headers[:5]:
            print(f"  0x{v.ps1_addr:08X}  '{v.name}'  "
                  f"{v.sample_rate} Hz  0x{v.data_len:X} bytes")

    if sound_tables:
        best = [t for t in sound_tables if t.vag_count] or sound_tables
        print(f"\nTop sound tables:")
        for t in best[:5]:
            print(f"  0x{t.ps1_addr:08X}  {t.note}")


if __name__ == "__main__":
    main()
