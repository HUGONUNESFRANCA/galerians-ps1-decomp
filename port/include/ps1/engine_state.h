#ifndef ENGINE_STATE_H
#define ENGINE_STATE_H

#include <stdint.h>

/* ─────────────────────────────────────────────────────────
 * EngineChannel — entrada no array de canais da engine
 * Stride: 0x10 bytes (16 bytes por canal)
 * 16 canais totais começando em 0x801AC900
 *
 * Tipos de canal (armazenados em channel_flags[]):
 *   0x00  canal livre
 *   0x04  tipo 4 — provavelmente áudio PCM/ADPCM
 *   0x10  tipo 7 — provavelmente vídeo MDEC ou CD-DA
 * ───────────────────────────────────────────────────────── */
typedef struct {
    uint16_t  field_0;    /* +0x00 — zerado no reset             */
    uint16_t  _pad_02;
    uint16_t  field_4;    /* +0x04 — zerado no reset             */
    uint16_t  _pad_06;
    uint16_t  field_8;    /* +0x08 — zerado no reset             */
    uint16_t  _pad_0A;
    uint16_t  sentinel;   /* +0x0C — 0xF9FF = canal ativo        */
    uint16_t  _pad_0E;
} EngineChannel;          /* sizeof = 0x10                       */

/* ─────────────────────────────────────────────────────────
 * EngineState — struct mestre da engine do Galerians
 * Início real: 0x801AC8D8
 * O ponteiro g_EngineBase (0x801AC900) aponta para +0x28
 * ───────────────────────────────────────────────────────── */
typedef struct {

    uint8_t       _unknown_0x00[0x28];  /* 0x801AC8D8–0x801AC8FF: desconhecido */

    /* +0x28 — array de canais (g_EngineBase aponta aqui) */
    EngineChannel channel_table[16];    /* 0x801AC900–0x801AC9FF: 16 × 0x10    */

    uint8_t       _unknown_0xA8[0x328]; /* 0x801ACA00–0x801ACD27: desconhecido */

    /* +0x358 — scheduler (offset de 0x801AC8D8) */
    uint16_t      state_slots_active;   /* 0x801ACB5C */
    uint16_t      active_state_index;   /* 0x801ACB5E */
    uint8_t       *scheduler_stack;     /* 0x801ACB60 */

    uint8_t       _unknown_rest[];      /* até offset +0x1858 e além           */

    /* +0x1880 — combate (offset de 0x801AC8D8) */
    /* int32_t  rion_hp_mirror;  → 0x801AE158                                 */
    /* uint32_t death_field_1;   → 0x801AE170                                 */
    /* uint32_t death_field_2;   → 0x801AE174                                 */
    /* uint32_t death_field_3;   → 0x801AE178                                 */

} EngineState;

/* Ponteiros de acesso direto */
#define ENGINE_STATE_BASE    ((EngineState *)0x801AC8D8)
#define ENGINE_BASE          ((EngineChannel *)0x801AC900)  /* campo +0x28 */
#define g_ChannelFlags       ((uint8_t *)0x801AC8D0)        /* flags[16]   */

/* Funções do gerenciador de canais */
/* 0x80185cc8  Channel_Manage(cmd, index, flags)                             */
/* 0x80185e14  Channel_InitType4(data, entry) — áudio PCM/ADPCM             */
/* 0x80185f68  Channel_InitType7(data, entry) — vídeo/CD-DA                 */
/* 0x8018635c  Channel_Register(index, entry) — registro comum              */

/* ─────────────────────────────────────────────────────────
 * Globais de CD / disco / módulo
 * ───────────────────────────────────────────────────────── */
extern uint32_t g_CDDriveReady;     /* 0x80193e00 */
extern uint32_t g_CDLoadReady;      /* 0x80193e04 */
extern uint32_t g_DiscVersion;      /* 0x80193e08 — 0=disc1, non-0=disc2  */
extern void    *g_ModuleBase;       /* 0x801AD140 — MODULE.BIN load addr  */

/* ─────────────────────────────────────────────────────────
 * Engine_Init — 0x80187DF4  ✅ Confirmado
 *
 * Ponto de entrada da engine inteira. Responsabilidades:
 *   1. Zera o scratchpad (0x1F800000, 0x2A0 bytes) — ver scratchpad.h
 *   2. Mapeia todos os subsistemas (engine state, câmera, SPU)
 *      preenchendo ponteiros para blocos dentro do scratchpad.
 *   3. Configura o renderer:
 *        Video_SetResolution(g_ScreenWidth/2, g_ScreenHeight/2)  [0x8018c008]
 *        Display_Enable(1)                                        [0x80178c84]
 *        SetDrawEnv(...)                                          [0x80178ea0]
 *   4. Obtém os bancos de voz do SPU via SPU_GetChannelBank
 *      (A = 0x801E2380 voices 0-15, B = 0x801E2470 voices 16-23).
 *   5. Chama FUN_80187420/FUN_80187450 com ponteiros para o
 *      scratchpad — provavelmente setup de SPU DMA / buffers.
 *   6. Dispara o primeiro VSync via Frame_First (0x8018669C).
 *
 * Observação histórica: este endereço foi inicialmente rotulado como
 * SPU_CoreDriver por causa dos 22 acessos a registradores SPU; o
 * mapeamento posterior revelou que os acessos são na inicialização
 * em bloco da SPU durante o boot, não num driver por frame.
 *
 * PORT NOTE (PC):
 *   Engine_Init é o entry point do port. Substituições:
 *     - scratchpad zeroing   → malloc(ScratchpadLayout) + memset(0)
 *     - Video_SetResolution  → SDL_CreateWindow / glViewport
 *     - Display_Enable       → SwapBuffers / SDL_ShowWindow
 *     - SetDrawEnv           → glClearColor / FBO setup
 *     - VSync                → timer de QueryPerformanceCounter
 *     - SPU channel banks    → alGenSources(24)
 *   O layout do scratchpad mapeia 1:1 para ScratchpadLayout em
 *   include/scratchpad.h — não existe indireção a manter.
 * ───────────────────────────────────────────────────────── */
/* void Engine_Init(void);  — stub em src/ a criar quando o dump for feito */

#endif /* ENGINE_STATE_H */