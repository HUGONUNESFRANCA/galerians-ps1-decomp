#ifndef ENTITY_H
#define ENTITY_H

#include <stdint.h>

/* Renomeado de PlayerObject → EntityBase.
 * Inimigos provavelmente usam a mesma struct. */
typedef struct {
    int16_t  id;
    int16_t  pos_x;
    int16_t  pos_y;
    int16_t  pos_z;
    int16_t  entity_type;
    uint8_t  _pad_0x0A[18]; /* TODO: rot, vel, anim, flags */
    int16_t  hp;            /* 0x1C — verificar relação com GlobalCombatState->hp */
    int16_t  hp_max;
    uint8_t  _pad_0x20[24]; /* TODO: inventário, IA, timers */
} EntityBase;

#endif /* ENTITY_H */