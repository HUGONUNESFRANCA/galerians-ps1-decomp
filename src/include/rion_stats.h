#ifndef RION_STATS_H
#define RION_STATS_H

#include <stdint.h>

#define RION_STATS_BASE_ADDR  0x801C2F9C

/* Threshold de AP crítico — confirmado em combat_state.c e damage_calc.c */
#define AP_CRITICAL_THRESHOLD 0x11  /* 17 — nível de Shorting/Addiction */

typedef struct {
    int16_t  hp;             /* +0x00 (0x801C2F9C) */
    int16_t  ap;             /* +0x02 (0x801C2F9E) */
    int16_t  hp_max;         /* +0x04 (0x801C2FA0) */
    int16_t  ap_max;         /* +0x06 (0x801C2FA2) */
    int16_t  action_state;   /* +0x08 (0x801C2FA4) — CUIDADO: afunda Rion se errado */
    int16_t  attack_charge;  /* +0x0A (0x801C2FA6) — tiro infinito quando alterado */
    int16_t  unknown_timer;  /* +0x0C (0x801C2FA8) — hipótese: i-frames */
} GlobalCombatState;

#define RION_STATS  ((GlobalCombatState *)RION_STATS_BASE_ADDR)

#endif /* RION_STATS_H */