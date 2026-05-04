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
| MOT.CDB | 1.5MB | 41429 | Character animation system — 670 MOT anims + 160 TMD + 36 TIM + 8 HMD rigs |
| PSX_CD.DUM | 30.7MB | 243385 | CD padding dummy |
| SOUND.CDB | 15.9MB | 42203 | 95 SEQ (pQES) sequencer files — RAW, no LZSS |
| XA.MXA | 85MB | 49976 | XA streaming music/cutscenes |

## Asset Loader
Function: AssetLoader_Init (0x8011ce48)
- Always loads MODULE.BIN first to RAM 0x801AD140.
- Flag at 0x80193e08 selects load table: 0 = Table A (10 files from PTR_DAT_80190e0c), non-0 = Table B (7 files from PTR_DAT_80190e5c).
- Each table entry: {void* dest_ram, char* filename} 8 bytes.
- CD_LoadFile (0x8018e2c4): reads CD, decompresses, copies to RAM.
- CD_FinishLoad (0x80165528): finalizes loading.

## CDB Format
- Container format for multiple PS1 assets, optionally LZSS-compressed.
- Header is **8 bytes** (matches what `CD_GetSize` @ `0x8018e1f4` returns); body starts at offset `0x08`.
- There is **no embedded sub-file table** and **no `decompressed_size` field** — the LZSS stream simply runs to its natural end. Sub-files are located by scanning the decompressed payload for known 4-byte magics on word-aligned offsets.
- Sub-asset types observed in `MODEL.CDB`: TIM textures + TMD models (+ a few HMD).

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
- 85 MB, LBA 49976. XA-ADPCM sectors interleaved directly with data sectors on the CD.
- Streams in real time from the drive; never fully loaded into PS1 RAM.
- Used for cutscene audio and ambient/BGM streaming.
- PC port: decode XA-ADPCM sectors to PCM on the fly; feed via SDL_Mixer music channel or an OpenAL streaming buffer.

## SOUND.CDB Format (CONFIRMED)

- **Size:** 15,919,104 bytes (15.18 MB, 7773 CD sectors)
- **compression_flag:** `0x00000000` — RAW, no LZSS compression
- **Content:** 95 SEQ files in pQES format (PsyQ sequencer)
- **RAM destination:** `0x801ACEA8` (`g_SoundCDB_Base`)

### pQES SEQ Files

Galerians uses the PsyQ SEQ + SPU bank system rather than raw VAG samples:

| Property | Value |
|----------|-------|
| Magic | `70 51 45 53` (`pQES`) |
| Count | 95 files |
| Max size | ~209 KB (largest = main BGM themes) |
| Role | PS1 equivalent of MIDI — note events + timing + SPU commands |
| Player | `SEQ_Player` @ `0x80130764` ✅ |

SEQ files drive the SPU hardware with precise timing; they reference sample data
held in a separate SPU bank (not embedded in SOUND.CDB).

### SPU Sample Banks — NOT in SOUND.CDB

| Candidate | Size | Likely role |
|-----------|------|-------------|
| `XA.MXA`    | 85 MB  | XA-ADPCM streaming (cutscenes / BGM) |
| `MODULE.BIN` overlays | 2.4 MB | May pre-load per-area SPU RAM banks |

> ⚠️ `MOT.CDB` was previously listed here as a VH+VB candidate. **CONFIRMED WRONG** — MOT.CDB contains animation/motion data (see MOT.CDB section below). SPU sample bank source is still **unknown**.

## MOT.CDB Format (CONFIRMED)

- **Compressed:** 1.51 MB (LZSS) → **Decompressed:** 4.13 MB (2.73× ratio)
- **compression_flag:** `0x00060001` — LZSS, upper bits = variant 6
- **Payload header (first 4 bytes of decompressed body):** `00 03 00 00` = `0x300` = 768 (motion count field)

### Contents

| Count | Type | Notes |
|------:|------|-------|
| 670 | BIN | Proprietary MOT animation data — magic byte **unknown**, needs new extractor rule |
| 160 | TMD | 3D geometry (pose / reference meshes per animation) |
|  36 | TIM | Textures (likely character skin textures) |
|   8 | HMD | Skeletal rigs — same 8 characters as MODEL.CDB |
| **874** | **total** | |

> **HMD overlap:** MODEL.CDB carries the same 8 HMD rigs for rendering geometry;
> MOT.CDB's 8 HMDs are the skeletal definitions used at runtime to drive the 670 animations.

### Motion Data Pipeline

```
MOT.CDB  → 8 HMD rigs (skeleton def) + 670 BIN animations (keyframes)
MODEL.CDB → 8 HMD rigs (geometry)    + 676 TMD (room/scene models)
Together  → full character animation system
```

---

## Animation System

### Characters with Skeletal Rigs (8 HMD slots)

Both MODEL.CDB and MOT.CDB contain exactly 8 HMD files — one per character:

| Slot | Character | Status |
|-----:|-----------|--------|
| 0 | Rion (main character) | 🟡 likely |
| 1 | Rita | 🟡 likely |
| 2 | Dorothy | 🟡 likely |
| 3–7 | Key NPCs / enemies | 🔴 to confirm |

HMD role by CDB:
- **MODEL.CDB HMDs** — geometry meshes bound to a skeleton (used for rendering)
- **MOT.CDB HMDs** — skeleton definitions that bind to the 670 BIN motion clips at runtime

### MOT Binary Format (Proprietary — Needs Reverse Engineering)

| Offset | Type | Field | Notes |
|--------|------|-------|-------|
| `+0x00` | `uint32` | `motion_count` | `0x00000300` = 768 declared slots; 670 non-empty found by extractor |
| `+0x04` | … | motion entries | Format **unknown** — needs Ghidra analysis |

**Magic byte:** UNKNOWN. `cdb_extractor.py` cannot currently detect BIN/MOT files by magic.
The 670 BIN files are found by elimination after all known magics (TIM / TMD / HMD / SEQ) are matched.

> **Next step:** Open a BIN file in hex editor or Ghidra to find a repeating header pattern and define a new magic byte / detection heuristic for the extractor.

### Runtime Animation Pipeline (Hypothesized)

```
Load phase:
  MOT.CDB  ──→ 8 HMD (skeleton defs) + 670 BIN (motion keyframes)
  MODEL.CDB ──→ 8 HMD (geometry)     + 676 TMD (room/scene models)

Per-frame:
  SEQ_Player (FUN_80130764) drives timing
  BIN motion clip → bone transform array → HMD skeleton → GTE → GPU draw
```

---

## CDB Binary Format (CONFIRMED)

### Header — 8 bytes total

| Offset | Size | Field            | Notes                                                                 |
|--------|------|------------------|-----------------------------------------------------------------------|
| 0x00   | 4    | `sector_count`   | CD sectors of the *compressed* payload (e.g. `108` for MODEL.CDB).    |
| 0x04   | 4    | `compression_flag` | `0` = raw, non-zero = LZSS-compressed body. MODEL.CDB = `0x00130001`. |
| 0x08   | …    | body             | LZSS stream (or raw bytes) until end of file.                         |

> ⚠️ Earlier notes claimed a 12-byte header with `file_size` / `decompressed_size` /
> embedded `sub_file_table` fields starting at `0x0C`. That layout was wrong:
> the bytes at `0x08+` are LZSS payload, not a directory. There is no
> decompressed-size hint in the file — the stream just runs to its natural end.

### Sub-file discovery — magic-byte scan

Sub-files are located by scanning the *decompressed* payload at 4-byte
alignment for known PS1 asset magics, then slicing between consecutive hits:

| Magic (LE)      | Type | Ext    | Notes                                  |
|-----------------|------|--------|----------------------------------------|
| `10 00 00 00`   | TIM  | `.tim` | PS1 texture (4/8/16 bpp).              |
| `40 00 00 00`   | TMD  | `.tmd` | PS1 model.                             |
| `41 00 00 00`   | HMD  | `.hmd` | PS1 hierarchical model.                |
| `41 4C 54 00`   | ALT  | `.alt` | `'ALT\0'` container (not seen in MODEL.CDB). |
| `56 41 47 70`   | VAG  | `.vag` | `'VAGp'` ADPCM audio (SOUND.CDB).      |
| `70 51 45 53`   | SEQ  | `.seq` | `'pQES'` PsyQ sequence.                |
| `00 00 00 00`   | —    | skip   | Padding / empty slot.                  |

### LZSS Parameters (PS1 variant)
Window size: `0x1000` bytes (4096). Lookahead: `0x12` bytes (18). Flag byte: 8 bits, **LSB first**, `1`=literal, `0`=back-reference. Back-ref encoding: 12-bit window offset + 4-bit `(length - 3)`.
Compression ratio: MODEL.CDB ≈ 3.0× (4.6 MB → 13.4 MB).

### Known CDB Files and Contents

| File         | Compressed | Decompressed | Sub-files | Composition                  |
|--------------|-----------:|-------------:|----------:|------------------------------|
| MODEL.CDB    |    4.6 MB  |     13.4 MB  |    1050   | 366 TIM + 676 TMD + 8 HMD    |
| MOT.CDB      |    1.5 MB  |     4.1 MB   |     874   | 670 BIN (MOT anims) + 160 TMD + 36 TIM + 8 HMD |
| SOUND.CDB    |   15.9 MB  |   15.9 MB    |      95   | 95 SEQ (pQES sequences), RAW — no LZSS |
| DISPLAY.CDB  |    2.5 MB  |  *(pending)* |  *(pending)* | TIM UI textures           |

Inventories confirmed by running `tools/cdb_extractor.py` against the disk image and tallying magic-byte hits.

**HMD note:** Both MODEL.CDB and MOT.CDB contain 8 HMD files — these are the same 8 main characters (Rion, Rita, Dorothy, key NPCs). MODEL.CDB's HMDs carry the rendering geometry; MOT.CDB's HMDs are the skeletal rig definitions used to drive the 670 BIN animations at runtime.

**BIN/MOT note:** The 670 BIN files in MOT.CDB are proprietary animation keyframe data (internal name: MOT format). Their magic bytes are unknown — `cdb_extractor.py` needs a new detection rule for this type.

## PC Port Notes

For a native Windows port, replace the CD access stack with file I/O and pre-extract
all CDB assets at build time:

| PS1 Layer | PC Replacement |
|-----------|----------------|
| `CD_ReadSector` (BIOS Table A [0x27]) | `fread()` from file |
| `CD_Read` / `CD_LoadFile` | standard file open + read |
| LZSS decompressor at runtime | pre-extracted assets via `tools/cdb_extractor.py` |
| `MODEL.CDB` raw binary | `assets/MODEL/*.tmd`, `assets/MODEL/*.tim`, `assets/MODEL/*.hmd` |
| `SOUND.CDB` 95 pQES SEQ files | Option A: decode SEQ → standard MIDI, play with FluidSynth |
| | Option B: implement pQES interpreter with OpenAL backend |
| | Option C: pre-render all 95 tracks to OGG at build time (simplest) |
| `XA.MXA` XA-ADPCM streaming | Decode XA sectors to PCM on the fly → SDL_Mixer / OpenAL streaming buffer |
| `HMD` skeletal rigs (8 chars) | convert to GLTF 2.0 with skeleton at build time |
| `MOT` 670 BIN motion clips | decode MOT format → GLTF 2.0 animations (one `.glb` per clip) |
| Animation build step | combine HMD + BIN → one GLTF 2.0 file per character (mesh + skeleton + all animations) |
| Runtime animation | skeletal interpolation (lerp between keyframes); cgltf + custom bone stack, or ozz-animation |
| SPU sample banks | Source **unknown** (MOT.CDB is animation data, not audio) — locate via SPU RAM dump |
| TMD loader | convert to GLTF or custom binary at build time |
| TIM textures | convert to PNG/DDS at build time |
| VAG ADPCM audio | decode to WAV/OGG at build time |

Pre-extraction strategy: run `tools/cdb_extractor.py` on all CDBs once; ship the
decompressed sub-files. This eliminates the PS1 LZSS decompressor and CD polling
loop entirely from the port.

### Animation Port Strategy

| Step | Action |
|------|--------|
| Build-time 1 | Extract 8 HMD files from MODEL.CDB → convert to GLTF 2.0 with skeleton |
| Build-time 2 | Extract 670 BIN files from MOT.CDB → decode MOT format → GLTF 2.0 animation tracks |
| Build-time 3 | Combine per-character HMD + relevant BIN clips → one `.glb` per character |
| Runtime | Load `.glb`; play animations via skeletal interpolation (lerp between keyframes per bone) |

> **Blocker:** MOT binary format magic and internal layout are unknown. Reverse engineering required before build-time steps 2–3 can be implemented. See Animation System section above.