#ifndef ENGINE_STATE_H
#define ENGINE_STATE_H

#include <stdint.h>

/*
 * EngineState — struct mestre da engine do Galerians.
 * Base address: 0x801AC900  (g_EngineBase)
 *
 * A maior parte dos campos ainda é desconhecida.
 * Documentar conforme análise avança.
 *
 * QUESTÃO ABERTA: offset 0x1858 contém HP do Rion (mirror de
 * GlobalCombatState->hp) ou é a fonte original?
 * Verificar com watchpoint duplo no DuckStation.
 */
typedef struct {

    uint8_t  _unknown_0x0000[0x1858]; /* TODO: campos da engine */

    int32_t  rion_hp_mirror;   /* +0x1858 — HP negativo = morte */

    uint8_t  _unknown_0x185C[0x14];

    uint32_t death_field_1;    /* +0x1870 — zerado por Engine_ResetDeathState */
    uint32_t death_field_2;    /* +0x1874 — zerado por Engine_ResetDeathState */
    uint32_t death_field_3;    /* +0x1878 — zerado por Engine_ResetDeathState */

} EngineState;

/* Ponteiro global para a engine */
#define ENGINE_BASE  ((EngineState *)0x801AC900)

#endif /* ENGINE_STATE_H */