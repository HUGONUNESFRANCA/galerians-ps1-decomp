/*
 * HAL — Audio (OpenAL stub)
 *
 * Future home of the OpenAL-backed replacement for the PS1 SPU + SEQ
 * pipeline documented in include/ps1/audio.h and docs/MemoryMap.md
 * ("Audio System"). Empty translation unit for now — keeps CMake happy
 * until the SEQ/VAG path is wired up.
 *
 * Replacement plan (from docs/asset_format.md):
 *   SEQ (pQES) → FluidSynth or pre-rendered OGG
 *   SPU voices → alGenSources × 24
 *   XA stream  → SDL_Mixer streaming buffer + XA-ADPCM decoder
 */
