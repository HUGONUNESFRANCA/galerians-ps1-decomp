"""
camera_search.py — Galerians PS1 camera struct finder

Scans a PS1 RAM dump for clusters of signed 16-bit values that match
the layout of a GTE camera/transform struct:
  - GTE rotation matrix: 3x3 int16_t, all in [-4096, 4096] (4096 = 1.0 fixed-point)
  - Translation / position: 3x int32_t or int16_t (world-space coords)
  - Euler angles: int16_t in [0, 4095] (PS1: 4096 units = 360 degrees)

Usage:
    python tools/camera_search.py [--dump PATH] [--out PATH] [--addr-start HEX] [--addr-end HEX]

Defaults:
    --dump  "C:/Users/User/Desktop/Coisas sobre Jogos/Emuladores/PS1/Dump/ram.bin"
    --out   tools/camera_search_results.txt
    --addr-start  0x80000000  (full RAM)
    --addr-end    0x801FFFFF

PS1 RAM base: 0x80000000 (file offset 0 in raw dump)
"""

import argparse
import struct
import sys
from dataclasses import dataclass, field
from pathlib import Path
from typing import List, Tuple

# ── Constants ──────────────────────────────────────────────────────────────────

PS1_RAM_BASE   = 0x80000000
PS1_RAM_SIZE   = 0x200000   # 2 MB
GTE_FIXED_ONE  = 4096       # 1.0 in GTE fixed-point

# Signed int16 range that valid GTE matrix entries must satisfy
GTE_MAT_MIN = -GTE_FIXED_ONE
GTE_MAT_MAX =  GTE_FIXED_ONE

# Angle range (PS1 uses 12-bit angles: 0 = 0°, 4096 = 360°)
ANGLE_MIN =    0
ANGLE_MAX = 4095

# Known unmapped gap inside EngineState — highest-priority scan region
ENGINE_STATE_GAP_START = 0x801ACB66
ENGINE_STATE_GAP_END   = 0x801AE158

# Minimum consecutive "good" int16s to report a cluster
CLUSTER_MIN_GOOD  = 6
# Window size in int16s for scoring
CLUSTER_WINDOW    = 16

# ── Helpers ────────────────────────────────────────────────────────────────────

def ps1_to_offset(addr: int) -> int:
    return addr - PS1_RAM_BASE

def offset_to_ps1(offset: int) -> int:
    return PS1_RAM_BASE + offset

def read_s16(data: bytes, offset: int) -> int:
    return struct.unpack_from("<h", data, offset)[0]

def read_s32(data: bytes, offset: int) -> int:
    return struct.unpack_from("<i", data, offset)[0]

def is_gte_value(v: int) -> bool:
    return GTE_MAT_MIN <= v <= GTE_MAT_MAX

def is_angle(v: int) -> bool:
    return ANGLE_MIN <= v <= ANGLE_MAX

def gte_matrix_score(vals: List[int]) -> float:
    """
    Score a 3x3 GTE rotation matrix (9 int16 values).
    A valid rotation matrix in GTE fixed-point:
      - All entries in [-4096, 4096]
      - Each row vector has magnitude ≈ 4096 (unit vector × 4096)
    Returns 0.0..1.0.
    """
    if len(vals) < 9:
        return 0.0
    if not all(is_gte_value(v) for v in vals[:9]):
        return 0.0

    score = 1.0
    for row in range(3):
        r = vals[row * 3: row * 3 + 3]
        mag_sq = sum(x * x for x in r)
        # Ideal mag_sq = 4096^2 = 16,777,216
        ideal = GTE_FIXED_ONE * GTE_FIXED_ONE
        ratio = mag_sq / ideal if ideal else 0
        # Penalty: ratio should be near 1.0
        if 0.5 <= ratio <= 2.0:
            score *= (1.0 - abs(1.0 - ratio) * 0.5)
        else:
            score *= 0.1
    return score

# ── Result dataclass ───────────────────────────────────────────────────────────

@dataclass
class CandidateResult:
    ps1_addr:    int
    score:       float
    kind:        str           # "gte_matrix", "cluster", "angle_cluster"
    values:      List[int]
    in_gap:      bool = False
    note:        str  = ""

    def fmt_vals(self, n: int = 12) -> str:
        return "  ".join(f"{v:6d}" for v in self.values[:n])

    def __lt__(self, other):
        return self.score > other.score   # sort descending

# ── Scanners ───────────────────────────────────────────────────────────────────

def scan_gte_matrices(data: bytes, start_off: int, end_off: int) -> List[CandidateResult]:
    """Find 3×3 GTE rotation matrices (9 consecutive valid int16 values)."""
    results = []
    stride = 2  # int16 aligned
    end = min(end_off, len(data) - 18)  # 9 × 2 bytes

    for off in range(start_off, end, stride):
        vals = [read_s16(data, off + i * 2) for i in range(9)]
        sc = gte_matrix_score(vals)
        if sc >= 0.4:
            ps1 = offset_to_ps1(off)
            in_gap = ENGINE_STATE_GAP_START <= ps1 < ENGINE_STATE_GAP_END
            # Read 6 more int16s after the matrix (possible translation/angles)
            extra = [read_s16(data, off + 18 + i * 2) for i in range(6)
                     if off + 18 + i * 2 < len(data)]
            results.append(CandidateResult(
                ps1_addr=ps1,
                score=sc + (0.3 if in_gap else 0.0),
                kind="gte_matrix",
                values=vals + extra,
                in_gap=in_gap,
                note=f"GTE matrix score={sc:.2f}",
            ))
    return results


def scan_int16_clusters(data: bytes, start_off: int, end_off: int) -> List[CandidateResult]:
    """
    Slide a window and score regions where many consecutive int16s
    are in [-4096, 4096]. Catches position/angle arrays that aren't
    pure rotation matrices.
    """
    results = []
    win = CLUSTER_WINDOW
    stride = 2
    end = min(end_off, len(data) - win * 2)

    prev_reported = -1

    for off in range(start_off, end, stride):
        vals = []
        for i in range(win):
            o = off + i * 2
            if o + 2 > len(data):
                break
            vals.append(read_s16(data, o))

        good = sum(1 for v in vals if is_gte_value(v))
        if good < CLUSTER_MIN_GOOD:
            continue

        # Avoid reporting overlapping windows
        if off - prev_reported < CLUSTER_WINDOW * 2:
            continue

        score = good / win
        ps1 = offset_to_ps1(off)
        in_gap = ENGINE_STATE_GAP_START <= ps1 < ENGINE_STATE_GAP_END
        results.append(CandidateResult(
            ps1_addr=ps1,
            score=score * 0.7 + (0.3 if in_gap else 0.0),
            kind="cluster",
            values=vals,
            in_gap=in_gap,
            note=f"{good}/{win} values in [-4096,4096]",
        ))
        prev_reported = off

    return results


def scan_angle_clusters(data: bytes, start_off: int, end_off: int) -> List[CandidateResult]:
    """
    Scan for 3+ consecutive int16s that look like PS1 Euler angles [0, 4095].
    Camera orientation is often stored as (rotX, rotY, rotZ).
    """
    results = []
    stride = 2
    end = min(end_off, len(data) - 8)
    prev_reported = -1

    for off in range(start_off, end, stride):
        vals = [read_s16(data, off + i * 2) for i in range(4)
                if off + i * 2 + 2 <= len(data)]
        good = sum(1 for v in vals if is_angle(v))
        if good < 3:
            continue
        if off - prev_reported < 8:
            continue

        ps1 = offset_to_ps1(off)
        in_gap = ENGINE_STATE_GAP_START <= ps1 < ENGINE_STATE_GAP_END
        # Read up to 12 int16s for context
        ctx = [read_s16(data, off + i * 2) for i in range(12)
               if off + i * 2 + 2 <= len(data)]
        score = (good / 4) * 0.5 + (0.3 if in_gap else 0.0)
        results.append(CandidateResult(
            ps1_addr=ps1,
            score=score,
            kind="angle_cluster",
            values=ctx,
            in_gap=in_gap,
            note=f"{good}/4 values in angle range [0,4095]",
        ))
        prev_reported = off

    return results

# ── Dedup & rank ───────────────────────────────────────────────────────────────

def deduplicate(results: List[CandidateResult], window: int = 0x40) -> List[CandidateResult]:
    """Keep only the highest-scoring candidate within each address window."""
    results.sort()
    kept = []
    for r in results:
        if any(abs(r.ps1_addr - k.ps1_addr) < window for k in kept):
            continue
        kept.append(r)
    return kept

# ── Report writer ──────────────────────────────────────────────────────────────

def write_report(results: List[CandidateResult], out_path: Path,
                 scan_start: int, scan_end: int):
    gap_hits  = [r for r in results if r.in_gap]
    other     = [r for r in results if not r.in_gap]

    with open(out_path, "w", encoding="utf-8") as f:
        f.write("Galerians PS1 — Camera Struct Search Results\n")
        f.write("=" * 60 + "\n")
        f.write(f"Scan range:   0x{scan_start:08X} – 0x{scan_end:08X}\n")
        f.write(f"Priority gap: 0x{ENGINE_STATE_GAP_START:08X} – 0x{ENGINE_STATE_GAP_END:08X}"
                " (EngineState unmapped region)\n")
        f.write(f"Total candidates: {len(results)}\n\n")

        sections = [
            ("*** PRIORITY: Inside EngineState gap ***", gap_hits),
            ("Other candidates", other),
        ]

        for title, group in sections:
            if not group:
                continue
            f.write(f"{'─' * 60}\n{title}  ({len(group)} hits)\n{'─' * 60}\n\n")
            for i, r in enumerate(group[:50], 1):
                f.write(f"[{i:3d}] 0x{r.ps1_addr:08X}  score={r.score:.3f}"
                        f"  type={r.kind}\n")
                f.write(f"      {r.note}\n")
                f.write(f"      int16 values: {r.fmt_vals(12)}\n")
                # Interpret as possible matrix rows if gte_matrix
                if r.kind == "gte_matrix" and len(r.values) >= 9:
                    for row in range(3):
                        rv = r.values[row * 3: row * 3 + 3]
                        mag = (sum(x*x for x in rv) ** 0.5)
                        f.write(f"      row {row}: [{rv[0]:6d} {rv[1]:6d} {rv[2]:6d}]"
                                f"  |mag|={mag:.1f} (ideal={GTE_FIXED_ONE})\n")
                    if len(r.values) >= 12:
                        extra = r.values[9:12]
                        f.write(f"      +0x12 (pos/angles?): {extra}\n")
                f.write("\n")

        f.write("\n" + "=" * 60 + "\n")
        f.write("Next steps:\n")
        f.write("  1. Open top hits in DuckStation Memory viewer\n")
        f.write("  2. Set Write watchpoints on the 3 most promising addresses\n")
        f.write("  3. Move camera in-game — watchpoint should fire\n")
        f.write("  4. Check XREFs to the writing function in Ghidra\n")
        f.write("  5. Confirm struct layout using 'gte_matrix' hits first\n")
        f.write("     (GTE matrix entries change continuously during 3D movement)\n")

# ── Main ───────────────────────────────────────────────────────────────────────

def main():
    default_dump = (
        r"C:\Users\User\Desktop\Coisas sobre Jogos\Emuladores\PS1\Dump\ram.bin"
    )
    default_out = Path(__file__).parent / "camera_search_results.txt"

    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--dump",       default=default_dump,
                    help="Path to raw PS1 RAM dump (default: %(default)s)")
    ap.add_argument("--out",        default=str(default_out),
                    help="Output report path (default: %(default)s)")
    ap.add_argument("--addr-start", default="0x80000000",
                    help="PS1 address to start scan (hex)")
    ap.add_argument("--addr-end",   default="0x801FFFFF",
                    help="PS1 address to end scan (hex)")
    args = ap.parse_args()

    dump_path = Path(args.dump)
    out_path  = Path(args.out)
    scan_start = int(args.addr_start, 16)
    scan_end   = int(args.addr_end,   16)

    # ── Load dump ──────────────────────────────────────────────────────────────
    if not dump_path.exists():
        sys.exit(f"ERROR: dump not found: {dump_path}")

    data = dump_path.read_bytes()
    if len(data) < 0x100000:
        sys.exit(f"ERROR: dump too small ({len(data)} bytes) — expected 2MB PS1 RAM dump")

    print(f"Loaded {len(data):,} bytes from {dump_path.name}")

    start_off = max(0, ps1_to_offset(scan_start))
    end_off   = min(len(data), ps1_to_offset(scan_end))

    print(f"Scan range: 0x{scan_start:08X} – 0x{scan_end:08X}"
          f"  (file offsets 0x{start_off:06X}–0x{end_off:06X})")

    # ── Run scanners ───────────────────────────────────────────────────────────
    print("Scanning for GTE rotation matrices...")
    gte = scan_gte_matrices(data, start_off, end_off)
    print(f"  {len(gte)} raw GTE matrix candidates")

    print("Scanning for int16 clusters...")
    clusters = scan_int16_clusters(data, start_off, end_off)
    print(f"  {len(clusters)} raw cluster candidates")

    print("Scanning for angle triplets (rotX/rotY/rotZ)...")
    angles = scan_angle_clusters(data, start_off, end_off)
    print(f"  {len(angles)} raw angle candidates")

    # ── Merge, dedup, rank ─────────────────────────────────────────────────────
    all_results = gte + clusters + angles
    deduped = deduplicate(all_results, window=0x20)
    deduped.sort()   # highest score first

    print(f"\nAfter dedup: {len(deduped)} unique candidates")
    gap_count = sum(1 for r in deduped if r.in_gap)
    print(f"  Inside EngineState gap: {gap_count}")
    print(f"  Elsewhere:              {len(deduped) - gap_count}")

    # ── Write report ───────────────────────────────────────────────────────────
    write_report(deduped, out_path, scan_start, scan_end)
    print(f"\nReport saved to: {out_path}")

    # ── Print top 10 to console ────────────────────────────────────────────────
    print("\nTop 10 candidates:")
    print(f"  {'Address':12s}  {'Score':6s}  {'Type':14s}  {'In gap?':8s}  Note")
    print("  " + "-" * 72)
    for r in deduped[:10]:
        gap_mark = "<<< GAP" if r.in_gap else ""
        print(f"  0x{r.ps1_addr:08X}  {r.score:.3f}  {r.kind:14s}  {gap_mark:8s}  {r.note}")


if __name__ == "__main__":
    main()
