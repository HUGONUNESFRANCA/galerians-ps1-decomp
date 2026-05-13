#!/usr/bin/env python3
"""
validate_tims.py - Validates PS1 TIM texture files and copies valid ones to output dir.

Usage: python tools/validate_tims.py <input_dir> <output_dir>
"""

import struct
import shutil
import sys
from pathlib import Path
from collections import defaultdict

PIXEL_MODES = {0: "4bpp", 1: "8bpp", 2: "16bpp", 3: "24bpp"}
VALID_FLAGS = {0, 1, 2, 3, 8, 9, 10, 11}
CLUT_WIDTH_FOR_MODE = {0: 16, 1: 256}  # 4bpp→16, 8bpp→256


def validate_tim(path: Path) -> tuple[bool, str, int | None]:
    """
    Returns (is_valid, reason, pixel_mode).
    pixel_mode is None on early failure.
    """
    data = path.read_bytes()
    size = len(data)

    if size < 8:
        return False, "file too small for header", None

    if data[0] != 0x10 or data[1] != 0x00 or data[2] != 0x00 or data[3] != 0x00:
        return False, f"invalid magic: {data[:4].hex()}", None

    flags = struct.unpack_from("<I", data, 4)[0]
    if flags not in VALID_FLAGS:
        return False, f"invalid flags: 0x{flags:08X}", None

    pixel_mode = flags & 0x03
    has_clut = bool(flags & 0x08)

    offset = 8

    if has_clut:
        if offset + 12 > size:
            return False, "file too small for CLUT header", pixel_mode

        clut_block_size = struct.unpack_from("<I", data, offset)[0]
        clut_w = struct.unpack_from("<H", data, offset + 8)[0]
        clut_h = struct.unpack_from("<H", data, offset + 10)[0]

        expected_clut_w = CLUT_WIDTH_FOR_MODE.get(pixel_mode)
        if expected_clut_w is not None and clut_w != expected_clut_w:
            return False, f"CLUT width {clut_w} invalid for {PIXEL_MODES[pixel_mode]}", pixel_mode

        if not (1 <= clut_h <= 256):
            return False, f"CLUT height {clut_h} out of range [1,256]", pixel_mode

        clut_data_bytes = clut_w * clut_h * 2
        expected_block_size = 12 + clut_data_bytes
        if clut_block_size < expected_block_size:
            return False, f"CLUT block_size {clut_block_size} < expected {expected_block_size}", pixel_mode

        if offset + clut_block_size > size:
            return False, "CLUT data extends beyond file", pixel_mode

        offset += clut_block_size

    if offset + 12 > size:
        return False, "file too small for image header", pixel_mode

    img_block_size = struct.unpack_from("<I", data, offset)[0]
    img_w = struct.unpack_from("<H", data, offset + 8)[0]
    img_h = struct.unpack_from("<H", data, offset + 10)[0]

    if not (1 <= img_w <= 512):
        return False, f"image width_units {img_w} out of range [1,512]", pixel_mode

    if not (1 <= img_h <= 512):
        return False, f"image height {img_h} out of range [1,512]", pixel_mode

    img_data_bytes = img_w * img_h * 2
    expected_img_block = 12 + img_data_bytes
    if img_block_size < expected_img_block:
        return False, f"image block_size {img_block_size} < expected {expected_img_block}", pixel_mode

    if offset + img_block_size > size:
        return False, "image data extends beyond file", pixel_mode

    return True, "ok", pixel_mode


def main():
    if len(sys.argv) != 3:
        print("Usage: python tools/validate_tims.py <input_dir> <output_dir>")
        sys.exit(1)

    input_dir = Path(sys.argv[1])
    output_dir = Path(sys.argv[2])

    if not input_dir.is_dir():
        print(f"Error: input directory not found: {input_dir}")
        sys.exit(1)

    tim_files = sorted(input_dir.rglob("*.tim")) + sorted(input_dir.rglob("*.TIM"))
    tim_files = list(dict.fromkeys(tim_files))  # deduplicate (case-insensitive FS edge case)

    if not tim_files:
        print(f"No .tim files found in {input_dir}")
        sys.exit(0)

    output_dir.mkdir(parents=True, exist_ok=True)

    total = len(tim_files)
    valid_count = 0
    invalid_count = 0
    mode_counts: dict[int, int] = defaultdict(int)
    invalid_files: list[tuple[str, str]] = []

    for path in tim_files:
        is_valid, reason, pixel_mode = validate_tim(path)

        if is_valid:
            valid_count += 1
            mode_counts[pixel_mode] += 1
            dest = output_dir / path.name
            # Avoid collisions from different subdirs
            if dest.exists() and dest.resolve() != path.resolve():
                stem = path.stem
                suffix = path.suffix
                counter = 1
                while dest.exists():
                    dest = output_dir / f"{stem}_{counter}{suffix}"
                    counter += 1
            shutil.copy2(path, dest)
        else:
            invalid_count += 1
            invalid_files.append((path.name, reason))

    sep  = "=" * 50
    dash = "-" * 50
    print(f"\n{sep}")
    print(f"TIM Validation Report")
    print(f"{sep}")
    print(f"Input:    {input_dir}")
    print(f"Output:   {output_dir}")
    print(dash)
    print(f"Total scanned : {total}")
    print(f"Valid         : {valid_count}")
    print(f"Invalid       : {invalid_count}")
    print(dash)
    print("Breakdown by pixel mode (valid files):")
    for mode_id, label in sorted(PIXEL_MODES.items()):
        count = mode_counts.get(mode_id, 0)
        print(f"  {label:6s} : {count}")
    if invalid_files:
        print(dash)
        print("Invalid files:")
        for name, reason in invalid_files:
            print(f"  {name}: {reason}")
    print(f"{sep}\n")


if __name__ == "__main__":
    main()
