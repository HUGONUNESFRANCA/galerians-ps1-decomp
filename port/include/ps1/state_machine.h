#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

#include <stdint.h>

#define STATE_SLOT_COUNT     16
#define STATE_SLOT_DATA_SIZE 0x1000

/*
 * StateSlot — entrada na tabela de estados da engine.
 * Cada slot representa um sistema ativo por frame
 * (gameplay, áudio, FMV, etc.).
 *
 * Slots conhecidos:
 *   0  — vídeo / FMV
 *   4  — música BGM
 *   5  — SFX
 *   6  — voz / diálogo
 *   ?  — gameplay (contém Handle_Combat_State)
 */
typedef struct {
    uint32_t  param_id;          /* +0x000 — ID do canal/estado */
    uint8_t   _pad[0x010];
    void     *init_data;         /* +0x010 — dado de inicialização */
    void     *channel_flags_ptr; /* +0x014 — ponteiro para g_StateChannelFlags */
    uint8_t   _fields[0x03C];
    uint32_t  prev_state;        /* +0x050 */
    uint32_t  slot_mask;         /* +0x054 — bitmask: 1 << slot_index */
    uint32_t  new_state;         /* +0x058 */
    uint32_t  slot_bit;          /* +0x05C */
} StateSlot;

/* Tabelas globais */
#define g_StateTable_EntryPtrs  ((void **)0x801E2218)
#define g_StateTable_RetAddrs   ((void **)0x801E21D8)
#define g_ActiveStateIndex      (*(uint16_t *)0x801ACB5E)
#define g_StateSlotsActive      (*(uint16_t *)0x801ACB5C)
#define g_DeathCount            (*(uint32_t *)0x801CB3C0)
#define g_ContinuesLeft         (*(uint16_t *)0x801CB3C4)

#endif /* STATE_MACHINE_H */