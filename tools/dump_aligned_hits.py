"""
dump_aligned_hits.py - Scan decompressed MODEL.CDB for TMD/HMD/40-magic headers
at 4-byte aligned offsets and report the most common 4-byte patterns.

Usage: python tools/dump_aligned_hits.py [path/to/MODEL.CDB]
"""

import sys
import os
import struct
from collections import Counter


# ---------------------------------------------------------------------------
# LZSS decompressor (PS1 / PsyQ variant)
# ---------------------------------------------------------------------------

def lzss_decompress(data: bytes) -> bytes:
    """Decompress a PsyQ/PS1 LZSS stream.

    Header layout:
      [0..3]  magic / flags  (varies by encoder; we skip until 0x00 is found
              or treat the first 4 bytes as a size hint if they look like one)
    This implementation is a plain sliding-window LZSS decoder that mirrors
    the decompressor used in Galerians (no header consumed – caller passes
    the raw payload).
    """
    out = bytearray()
    ring = bytearray(0x1000)
    ring_pos = 0xFEE          # PS1 default fill position
    i = 0
    n = len(data)

    while i < n:
        flags = data[i]
        i += 1
        for bit in range(8):
            if i >= n:
                break
            if flags & (1 << bit):
                # Literal byte
                byte = data[i]
                i += 1
                out.append(byte)
                ring[ring_pos] = byte
                ring_pos = (ring_pos + 1) & 0xFFF
            else:
                # Back-reference
                if i + 1 >= n:
                    break
                lo = data[i];     i += 1
                hi = data[i];     i += 1
                offset = lo | ((hi & 0xF0) << 4)
                length = (hi & 0x0F) + 3
                for _ in range(length):
                    byte = ring[offset & 0xFFF]
                    offset += 1
                    out.append(byte)
                    ring[ring_pos] = byte
                    ring_pos = (ring_pos + 1) & 0xFFF

    return bytes(out)


def try_decompress(path: str) -> bytes:
    """Read and attempt LZSS decompression.  Falls back to raw if it fails."""
    raw = open(path, "rb").read()

    # Many PS1 CDB files have a 4-byte uncompressed-size header first.
    # Try skipping it; if decompression produces nothing sensible, use raw.
    for skip in (0, 4, 8):
        try:
            result = lzss_decompress(raw[skip:])
            if len(result) >= 16:
                print(f"[+] LZSS decompressed (skip={skip}): "
                      f"{len(raw)} → {len(result)} bytes")
                return result
        except Exception:
            pass

    print(f"[!] LZSS decompression failed or produced empty output; "
          f"scanning raw bytes ({len(raw)} bytes).")
    return raw


# ---------------------------------------------------------------------------
# Scanner
# ---------------------------------------------------------------------------

MAGIC_LABELS = {
    0x41: "TMD hit",
    0x40: "40-magic hit",
    0x50: "HMD hit",
}

DUMP_LIMIT = 5          # first N hits of each type to dump
DUMP_BYTES = 48         # bytes to hex-dump per hit
TOP_PATTERNS = 10       # how many frequent 4-byte patterns to print


def hex_dump(data: bytes, base_offset: int) -> None:
    """Print 'data' as 16-bytes-per-row hex + ASCII."""
    for row in range(0, len(data), 16):
        chunk = data[row:row + 16]
        hex_part  = " ".join(f"{b:02X}" for b in chunk)
        ascii_part = "".join(chr(b) if 0x20 <= b < 0x7F else "." for b in chunk)
        print(f"  {base_offset + row:08X}: {hex_part:<48}  {ascii_part}")


def scan(data: bytes) -> None:
    n = len(data)
    hits: dict[int, list[int]] = {k: [] for k in MAGIC_LABELS}
    pattern_counter: Counter = Counter()

    for off in range(0, n - 3, 4):
        b0, b1, b2, b3 = data[off], data[off+1], data[off+2], data[off+3]

        # Frequency counter for all aligned 4-byte words
        word = (b0, b1, b2, b3)
        pattern_counter[word] += 1

        # Magic detection: first byte is one of our targets AND next 3 are 0x00
        if b1 == 0 and b2 == 0 and b3 == 0 and b0 in MAGIC_LABELS:
            hits[b0].append(off)

    # ---- Report hits -------------------------------------------------------
    for magic, label in MAGIC_LABELS.items():
        offsets = hits[magic]
        print(f"\n{'='*60}")
        print(f"  {label}  (magic=0x{magic:02X})  —  {len(offsets)} hits total")
        print(f"{'='*60}")
        if not offsets:
            print("  (none)")
            continue
        for off in offsets[:DUMP_LIMIT]:
            chunk = data[off: off + DUMP_BYTES]
            print(f"\n  @ offset 0x{off:08X}  ({DUMP_BYTES} bytes):")
            hex_dump(chunk, off)

    # ---- Top N most common 4-byte patterns ---------------------------------
    print(f"\n{'='*60}")
    print(f"  Top {TOP_PATTERNS} most common aligned 4-byte patterns")
    print(f"{'='*60}")
    for rank, (pat, count) in enumerate(pattern_counter.most_common(TOP_PATTERNS), 1):
        hex_str  = " ".join(f"{b:02X}" for b in pat)
        ascii_str = "".join(chr(b) if 0x20 <= b < 0x7F else "." for b in pat)
        print(f"  #{rank:2d}  [{hex_str}]  '{ascii_str}'  — {count} occurrences")


# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------

def main():
    if len(sys.argv) < 2:
        # Try the default T4 location the user mentioned
        default = r"C:\Users\User\Desktop\Programação\T4\MODEL.CDB"
        if os.path.exists(default):
            path = default
            print(f"[*] Using default path: {path}")
        else:
            print("Usage: python tools/dump_aligned_hits.py <path/to/MODEL.CDB>")
            sys.exit(1)
    else:
        path = sys.argv[1]

    if not os.path.exists(path):
        print(f"[!] File not found: {path}")
        sys.exit(1)

    print(f"[*] Reading: {path}")
    data = try_decompress(path)
    print(f"[*] Scanning {len(data)} bytes at 4-byte aligned offsets …\n")
    scan(data)


if __name__ == "__main__":
    main()
