# Galerians PS1 — Asset Format Documentation

## CD Filesystem (ISO9660, 216MB total)

| File | Size | LBA | Purpose |
|------|------|-----|---------|
| MOV/ | folder | 965 | FMV MDEC videos |
| MOV_D/ | folder | 966 | FMV duplicates/extras |
| BGTIM_A.CDB | 22.8MB | 2220 | Background textures zone A |
| BGTIM_B.CDB | 14.7MB | 13396 | Background textures zone B |
| BGTIM_C.CDB | 12.9MB | 20594 | Background textures zone C |
| BGTIM_D.CDB | 5.1MB | 26928 | Background textures zone D |
| DISPLAY.CDB | 2.5MB | 967 | UI/HUD textures |
| GE.CDB | 4KB | 29467 | Global effects (minimal) |
| ITEMTIM.CDB | 14.5MB | 29469 | Item textures |
| MENU.CDB | 2.6MB | 36589 | Menu textures |
| MODEL.CDB | 4.8MB | 38942 | 3D models |
| MODULE.BIN | 2.4MB | 40229 | Code overlays → RAM 0x801AD140 |
| MDT.CDB | 1.5MB | 41429 | Map/motion data |
| PSX_CD.DUM | 30.7MB | 243385 | CD padding dummy |
| SOUND.CDB | 15.9MB | 42203 | SFX + BGM audio |
| XA.MXA | 85MB | 49976 | XA streaming music/cutscenes |

## Asset Loader
Function: AssetLoader_Init (0x8011ce48)
- Always loads MODULE.BIN first to RAM 0x801AD140.
- Flag at 0x80193e08 selects load table: 0 = Table A (10 files from PTR_DAT_80190e0c), non-0 = Table B (7 files from PTR_DAT_80190e5c).
- Each table entry: {void* dest_ram, char* filename} 8 bytes.
- CD_LoadFile (0x8018e2c4): reads CD, decompresses, copies to RAM.
- CD_FinishLoad (0x80165528): finalizes loading.

## CDB Format
- Container format for multiple PS1 assets (LZSS compressed). Includes TIM and TMD/HMD.

## MODULE.BIN Format
- Code overlay system loaded to fixed address 0x801AD140. Each area/room has its own code module.

## MODULE.BIN Header Format

| Offset | Size | Value              | Description                          |
|--------|------|--------------------|--------------------------------------|
| 0x00   | 4    | `0x00000001`       | Version/type = 1                     |
| 0x04   | 16   | `"\T4\MODULE.BIN;1"` | ISO9660 full path (null terminated) |
| 0x14   | N    | `0x00...`          | Zero padding then overlay code       |

ISO9660 path structure: `\T4\` = track 4 (data track), `;1` = version.

## CDB Compression Detection

`CD_GetSize` returns two values:
- `sector_count`: number of CD sectors
- `compression_flag`: `0` = raw/uncompressed, non-zero = LZSS compressed

Buffer sizing:
- Uncompressed: `sector_count * 4 + 4` bytes
- Compressed:   `sector_count * 8 + 8` bytes (2× workspace for decompression)

## RAM Destination Map (Table A — Disc 1)

| File         | RAM Address  | Size estimate           |
|--------------|--------------|-------------------------|
| MODULE.BIN   | `0x801AD140` | 2.4 MB overlay code     |
| MODEL.CDB    | `0x801AD050` | 4.8 MB 3D models        |
| DISPLAY.CDB  | `0x801AD0C8` | 2.5 MB UI textures      |
| SOUND.CDB    | `0x801ACEA8` | 15.9 MB audio data      |
| ITEMTIM.CDB  | `0x801ACFD8` | 14.5 MB item textures   |
| MENU.CDB     | `0x801ACF60` | 2.6 MB menu textures    |

## XA.MXA Format
- XA audio interleaved with data. Streams directly from CD for cutscenes/ambient.