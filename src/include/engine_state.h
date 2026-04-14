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

#endif /* ENGINE_STATE_H */