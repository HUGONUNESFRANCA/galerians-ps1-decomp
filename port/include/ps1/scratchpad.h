#ifndef SCRATCHPAD_H
#define SCRATCHPAD_H

#include <stdint.h>

/*
 * ──────────────────────────────────────────────────────────────
 * PS1 Scratchpad (Data Cache) Layout — Galerians
 * ──────────────────────────────────────────────────────────────
 *
 * Endereço:  0x1F800000
 * Tamanho:   1 KB (0x400 bytes) — hardware
 * Usado:     0x2A0 bytes pelo jogo
 *
 * A scratchpad do PS1 é 1 KB de memória alocada no data cache do
 * R3000A — acesso de ~1 ciclo comparado a ~4 ciclos para main RAM.
 * O jogo usa esta região como storage rápido para:
 *   - buffer de histórico de câmera (ring buffer)
 *   - shadow state da engine e da câmera ativa
 *   - buffers passados para rotinas de SPU DMA
 *
 * Toda essa região é zerada por Engine_Init (0x80187DF4), depois
 * preenchida por ponteiros de subsistemas (câmera, engine, SPU).
 *
 * PORT NOTE (PC):
 *   A scratchpad não existe em hardware PC. Alocar como uma struct
 *   global estática (ou malloc) — os ponteiros `DAT_801eb*` / `DAT_801e2*`
 *   passam a apontar para campos dessa struct.
 * ─────────────────────────────────────────────────────────────── */

#define SCRATCHPAD_BASE_ADDR     0x1F800000u
#define SCRATCHPAD_SIZE          0x400u      /* 1 KB hardware total */
#define SCRATCHPAD_USED          0x2A0u      /* bytes ativamente usados */

/* Ring buffer de histórico de câmera: stride 0x20 por frame.
 * Os primeiros 2 bytes do bloco armazenam o índice do frame (int16_t),
 * seguidos pelos slots de histórico. Gerenciado por Camera_RecordFrame
 * (0x80187350) via g_CameraHistoryPtr (0x801eb544). */
#define CAMERA_HISTORY_STRIDE    0x20u
#define CAMERA_HISTORY_BYTES     0x144u

/* Buffer ativo da câmera GTE: 8 × uint32 copiados para scratchpad COP2
 * (0x1F800168) por Camera_LoadToGTE (0x80187320). Shadow de DAT_801eb560. */
#define CAMERA_BUFFER_OFFSET     0x168u
#define CAMERA_BUFFER_WORDS      8u

/* Cada bloco de shadow state da engine/SPU ocupa 0x20 bytes. */
#define SCRATCHPAD_BLOCK_SIZE    0x20u

/* ─── Layout do Scratchpad ─────────────────────────────────────── */
typedef struct {
    /* +0x000 — Camera history ring buffer.
     * Gerenciado por Camera_RecordFrame; stride 0x20 por frame. */
    uint8_t  camera_history[CAMERA_HISTORY_BYTES];      /* 0x000 – 0x143 */

    /* +0x144 — DAT_801e2590. Propósito ainda não mapeado. */
    uint32_t dat_801e2590;                              /* 0x144 – 0x147 */

    /* +0x148 — Shadow do alvo de g_EngineStatePtr (DAT_801eb574). */
    uint8_t  engine_state_shadow[SCRATCHPAD_BLOCK_SIZE];/* 0x148 – 0x167 */

    /* +0x168 — g_CameraBuffer: matriz GTE ativa (shadow de DAT_801eb560).
     * Fonte da cópia de Camera_LoadToGTE. */
    uint32_t camera_buffer[CAMERA_BUFFER_WORDS];        /* 0x168 – 0x187 */

    /* +0x188 — DAT_801eb56c. Propósito não mapeado. */
    uint8_t  unknown_01eb56c[SCRATCHPAD_BLOCK_SIZE];    /* 0x188 – 0x1A7 */

    /* +0x1A8 — DAT_801eb55c → passado para FUN_80187450.
     * Provavelmente buffer de SPU (candidato a buffer de voz/XA). */
    uint8_t  spu_buffer_55c[SCRATCHPAD_BLOCK_SIZE];     /* 0x1A8 – 0x1C7 */

    /* +0x1C8 — DAT_801eb558 → passado para FUN_80187420.
     * Provavelmente descritor de SPU DMA. */
    uint8_t  spu_dma_558[SCRATCHPAD_BLOCK_SIZE];        /* 0x1C8 – 0x1E7 */

    /* +0x1E8 — DAT_801eb554 → passado para FUN_80187450. */
    uint8_t  spu_buffer_554[SCRATCHPAD_BLOCK_SIZE];     /* 0x1E8 – 0x207 */

    /* +0x208 — DAT_801e2588 → passado para FUN_80187420. */
    uint8_t  spu_dma_2588[SCRATCHPAD_BLOCK_SIZE];       /* 0x208 – 0x227 */

    /* +0x228 — 10 globals mapeados com propósito ainda desconhecido.
     * Preenchem o restante do footprint usado (0x228 – 0x29F). */
    uint8_t  unknown_globals[0x78];                     /* 0x228 – 0x29F */

    /* +0x2A0 — 1 KB - 0x2A0 = 0x160 bytes não usados pelo jogo. */
    uint8_t  _unused[SCRATCHPAD_SIZE - SCRATCHPAD_USED]; /* 0x2A0 – 0x3FF */
} ScratchpadLayout;  /* sizeof = 0x400 (hardware total) */

/* Acesso direto ao scratchpad mapeado em hardware. */
#define SCRATCHPAD   (*(volatile ScratchpadLayout *)SCRATCHPAD_BASE_ADDR)

/* Ponteiros globais que apontam para dentro da scratchpad (main RAM
 * mantém um shadow; o scratchpad é a cópia hot). Ver MemoryMap.md. */
/* 0x801eb560  g_CameraBuffer      → SCRATCHPAD.camera_buffer        */
/* 0x801eb574  g_EngineStatePtr    → SCRATCHPAD.engine_state_shadow  */
/* 0x801eb56c  DAT_801eb56c        → SCRATCHPAD.unknown_01eb56c      */
/* 0x801eb55c  DAT_801eb55c        → SCRATCHPAD.spu_buffer_55c       */
/* 0x801eb558  DAT_801eb558        → SCRATCHPAD.spu_dma_558          */
/* 0x801eb554  DAT_801eb554        → SCRATCHPAD.spu_buffer_554       */
/* 0x801e2588  DAT_801e2588        → SCRATCHPAD.spu_dma_2588         */
/* 0x801e2590  DAT_801e2590        → SCRATCHPAD.dat_801e2590         */

/* Verificação do layout em tempo de compilação (C11+). */
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
_Static_assert(sizeof(ScratchpadLayout) == SCRATCHPAD_SIZE,
               "ScratchpadLayout must be exactly 1 KB");
#endif

#endif /* SCRATCHPAD_H */
