/*
 * coroutine.c
 * Sistema de corrotinas cooperativas do Galerians PS1.
 */

#include <stdint.h>
#include "coroutine.h"
#include "state_machine.h"

extern void  StateSlot_Execute(void);   /* 0x801854d4 */
extern void  Context_Restore(void);     /* 0x80185510 */
extern void  Trigger_GameOver(void);    /* 0x8018539c */
extern int   GetFreeStackSpace(void);   /* 0x8018549c — a confirmar */

/*
 * Coroutine_Spawn()
 * Endereço PS1: 0x80184dc4
 *
 * Cria uma nova corrotina no canal especificado.
 *
 * param_1  — função de entrada da corrotina (vira RA ao iniciar)
 * param_2  — índice do canal (0-15)
 * param_3  — argumento a0 passado para a corrotina ao iniciar
 * param_4  — argumento a1 passado para a corrotina ao iniciar
 *
 * Retorno: handle = (canal << 16) | slot_index
 *          ou valor anterior de v0 se canal cheio
 */
uint32_t Coroutine_Spawn(uint32_t entry_fn,
                         int      channel,
                         uint32_t arg0,
                         uint32_t arg1)
{
    /* Lê contador de sub-entradas do canal */
    uint32_t count = *(uint16_t *)(*(int *)(g_CoroutineContextTable
                                   + channel * 4) + 2);

    /* Busca primeira sub-entrada livre (saved_sp == 0) */
    int slot;
    int32_t base;
    do {
        count--;
        if ((int32_t)count < 0)
            return v0_prev; /* canal cheio — retorna sem criar */

        base = *(int *)(g_CoroutineContextTable + channel * 4)
               - 0x38
               + (int32_t)count * -COROUTINE_STRIDE;

    } while (*(int *)(base - 0x23C) != COROUTINE_SLOT_FREE);

    /* Inicializa contexto da nova corrotina */
    *(uint32_t *)(base - 0x004) = arg0;       /* a0 inicial */
    *(uint32_t *)(base - 0x008) = arg1;       /* a1 inicial */
    *(uint32_t *)(base - 0x00C) = 0;          /* a2 = 0 */
    *(uint32_t *)(base - 0x010) = 0;          /* a3 = 0 */
    /* s0-s7 e fp herdados do caller (unaff) */
    *(uint32_t *)(base - 0x034) = 0;          /* fp = 0 */
    *(uint32_t *)(base - 0x23C) = base - 0x34; /* SP aponta para bloco */
    *(uint32_t *)(base - 0x238) = entry_fn;   /* RA = função de entrada */

    /* Retorna handle codificado */
    return ((uint32_t)channel << 16) | count;
}

/*
 * CoroutineSlot_Init()
 * Endereço PS1: 0x80184f4c
 *
 * Inicializa o pool de corrotinas de um slot/canal.
 * Registra o slot na tabela, calcula quantas sub-entradas
 * cabem no espaço disponível e marca todas como livres.
 */
void CoroutineSlot_Init(void)
{
    /* Registra este slot na tabela */
    *(uint8_t **)(&g_CoroutineContextTable[g_ActiveStateIndex])
        = &local_stack_top;

    /* Marca canal como ativo */
    g_CoroutineSlotMask |= (uint16_t)(1 << g_ActiveStateIndex);

    /* Calcula capacidade: (espaço livre - header) / stride */
    int free_mem = GetFreeStackSpace();
    int capacity = (free_mem - COROUTINE_HEADER_SIZE) / COROUTINE_STRIDE;

    /* Zera saved_sp de todas as sub-entradas (marca como livres) */
    uint8_t *entry = &local_stack_top;
    for (int i = capacity; i >= 0; i--) {
        *(uint32_t *)(entry - 0x23C) = COROUTINE_SLOT_FREE;
        entry -= COROUTINE_STRIDE;
    }

    /* Entra no loop de yield/timeout do canal */
    /* ... (loop de Coroutine_Yield) */
}