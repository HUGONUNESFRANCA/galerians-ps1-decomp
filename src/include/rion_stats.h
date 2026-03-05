#ifndef RION_STATS_H
#define RION_STATS_H

#include "ghidra_types.h"

// Estrutura global de status do Rion
// Base Address na memoria RAM: 0x801C2F9C
typedef struct {
    short HP;     // Offset 0x00 (0x801C2F9C)
    short AP;     // Offset 0x02 (0x801C2F9E)
    short Max_HP;         // Offset 0x04 (0x801C2FA0)
    short Max_AP;         // Offset 0x06 (0x801C2FA2)
    short Action_State;   // Offset 0x08 (0x801C2FA4) - Controla a pose/colisão (Afundou o Rion)
    short Attack_Charge;  // Offset 0x0A (0x801C2FA6) - Nível de carga do ataque (Tiro Infinito)
    short Unknown_Timer;  // Offset 0x0C (0x801C2FA8) - (Talvez tempo de invencibilidade após dano?)
} GlobalCombatState;

#endif // RION_STATS_H