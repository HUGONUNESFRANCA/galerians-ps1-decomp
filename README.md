# Galerians (PS1) — Reverse Engineering & Decompilation

## Sobre o Projeto
Educational reverse engineering of Galerians (PS1, 1999) 
targeting a native Windows port with enhancements.
SDK: PsyQ v1.140 (Sony SCEE, January 1998)
CPU: MIPS R3000A, Little Endian, 32-bit

## Status
WORK IN PROGRESS — Active research phase

## Ferramentas
- Ghidra (static analysis + decompilation)
- DuckStation (emulation + CPU debugger + memory viewer)
- CDMage (CD image extraction)
- Claude Code (automation + documentation)
- Custom Python tools (tools/)

## Sistemas Mapeados

### ✅ Game Loop & Scheduler (100%)
- Game loop at 0x80185170 — cooperative coroutine scheduler
- 16 slots × 7 coroutines = 112 simultaneous coroutines max
- VSync at 0x8017e0fc — frame pacing (30fps logic / 60fps video)
- Full coroutine context save/restore confirmed

### ✅ Input System (100%)
- Controller base: 0x801AC900, stride 0x40 per port
- Digital (type 4) and DualShock analog (type 7) fully mapped
- All button bitmasks, analog thresholds, rumble system documented
- DualShock rumble via SPU: SetRumble (0x80182bdc)

### ✅ Camera System (100%)
- Camera table: 0x801C3200 (pre-defined angles per room)
- Active camera matrix: 0x801BFD10 (updates during movement)
- Camera_LoadToGTE (0x80187320) — copies 32 bytes to GTE scratchpad
- Camera_RecordFrame (0x80187350) — circular frame history buffer
- Camera_Manager (0x8013b584) — resets 4 camera slots (stride 0xC60)
- Freecam implementation path: intercept Camera_LoadToGTE

### ✅ Engine Init & Scratchpad (100%)
- Engine_Init at 0x80187df4 — zeros scratchpad, maps all subsystems
- Full 1KB scratchpad layout documented (0x1F800000)
- Native resolution: 320×240 confirmed (g_ScreenWidth/Height)
- Widescreen: change 0x80193E30/E34 before Engine_Init

### ✅ Combat & Death System (100%)
- Rion stats: GlobalCombatState at 0x801C2F9C
- Full death chain: damage → GameOver → death counter
- AP critical threshold: 17 (Shorting/Addiction mechanic)
- g_DeathCount at 0x801CB3C0

### ✅ Audio System (100%)
- 95 SEQ files (pQES/MIDI format) in SOUND.CDB (raw, no compression)
- MIDI commands: 0xB0 (BGM), 0xB2 (SFX)
- SPU voice table: 0x801bfa30, stride 0x1C, 24 voices max
- SPU Sound RAM limit: 0x19000 bytes (102KB)
- XA streaming states: Sleep→Ready→XaSeek→XaWaitPly→XaPlaying
- Full voice pipeline: Sound_GetEntry→SEQ_MIDIVoiceAlloc→SPU_VoiceUpdate

### ✅ Asset Loading & CD Pipeline (100%)
- CD pipeline: AssetLoader_Init→CD_LoadFile→CD_Read→CD_ReadSector→BIOS A(0x27)
- BIOS CdGetSector confirmed at hardware level
- ISO9660 path format: \T4\FILENAME.EXT;1
- MODULE.BIN always loaded first to 0x801AD140

### ✅ CDB Asset Format (100%)
- 8-byte header: {sector_count, compression_flag}
- LZSS compression (window 0x1000, lookahead 0x12)
- MODEL.CDB: 676 TMD + 366 TIM + 8 HMD = 1050 assets (4.6MB→14MB)
- SOUND.CDB: 95 SEQ files, raw (no compression), 15.9MB
- MOT.CDB:  670 BIN animations + 8 HMD rigs + 196 TMD/TIM (1.5MB→4.1MB)
- CDB extractor tool: tools/cdb_extractor.py

### ✅ Renderer (100% — was 70%)
- PsyQ v1.140 DrawOTag confirmed at PTR_PsyQ_DrawOTag_801d04a0
- 6 Ordering Table buffers mapped (0x801932b4 to 0x80193520)
- Frame_Flip (0x8017b06c): PutDispEnv + DrawOTag + GPU DMA clear
- DrawSync (0x8017b114): blocking/non-blocking GPU wait, confirmed complete
- GPU hardware registers: 0x1F8010A8 (display), 0x1F801814 (DMA)
- BattleTransition_Render (0x8014d2cc): combat screen renderer
- DispEnv/DrawEnv double-buffer (Display_InitBuffers): g_DispEnv_0/1, g_DrawEnv_0/1 mapped
- PsyQ_SetDispEnv (0x8018d4d8) and PsyQ_SetDrawEnv (0x8018d598) confirmed
- Port path: DrawOTag → iterate OT → OpenGL/Vulkan draw calls

### ✅ FMV/MDEC System (90%)
- MDEC_StartDecode (0x8017e574): DMA0+DMA1+MDEC command pipeline
- MDEC_WaitReady (0x8017e690): status polling with timeout
- Full hardware register map: 7 DMA/MDEC registers documented
- FMV pipeline: XA streaming → MDEC decode → framebuffer → GPU
- Port: replace with libavcodec MPEG1 + SDL_Mixer XA decode

### ✅ Map/Area Overlays (80%)
- MODULE.BIN: monolithic code at 0x801AD140, never reloaded per-area
- Room data in MODEL.CDB: camera angles, geometry, ambient color
- Room_SetupCameraSlots (0x8018b320): initializes 4 camera angles per room
- Each room: up to 4 camera angles with frame animation + geometry + RGB ambient
- PsyQ ResetGraph (0x801789a0) manages rendering slot reallocation
- Missing: final room index table linking area IDs to descriptor pointers

## Estrutura do Repositório
galerians-ps1-decomp/
├── docs/
│   ├── memory_map.md      # Complete address reference
│   └── asset_format.md    # CDB format, audio, animation
├── include/
│   ├── engine_state.h     # EngineState struct
│   ├── state_machine.h    # Scheduler & coroutines
│   ├── input.h            # Controller system
│   ├── camera.h           # Camera system
│   ├── audio.h            # SPU & SEQ system
│   ├── renderer.h         # GPU & PsyQ rendering
│   ├── coroutine.h        # Coroutine context layout
│   ├── gte.h              # GTE coprocessor
│   ├── rion_stats.h       # Combat stats
│   ├── entity.h           # Entity base struct
│   └── ghidra_types.h     # Type mappings
├── src/
│   ├── game_loop.c        # Scheduler implementation
│   ├── combat_state.c     # Combat state machine
│   ├── damage_calc.c      # Damage formula
│   ├── camera.c           # Camera system
│   ├── audio.c            # Audio pipeline
│   └── coroutine.c        # Coroutine system
└── tools/
├── cdb_extractor.py   # CDB extraction + LZSS decompress
├── camera_search.py   # RAM scanner for camera structs
└── audio_search.py    # RAM scanner for audio data

## Aviso Legal
Educational project. No game files included.
Requires a legal copy of Galerians (PS1) to use these tools.