#ifndef COROUTINE_H
#define COROUTINE_H

#include <stdint.h>

/*
 * Sistema de Corrotinas Cooperativas — Galerians PS1
 *
 * CAPACIDADE:
 *   16 slots × 7 corrotinas = 112 corrotinas simultâneas (máximo)
 *
 * LAYOUT DE MEMÓRIA:
 *   Base global:       0x801D2198
 *   Por slot:          0x1000 bytes  (4096)
 *   Por corrotina:     0x240  bytes  (576)
 *   Stride de yield:   0x120  bytes  (288 — navegação interna)
 *
 * HANDLE DE CORROTINA (uint32_t):
 *   bits 31-16 = índice do slot (canal)
 *   bits 15-0  = índice da corrotina no slot
 */

#define COROUTINE_POOL_BASE      0x801D2198u
#define COROUTINE_SLOT_STRIDE    0x1000       /* bytes por slot           */
#define COROUTINE_ALLOC_STRIDE   0x240        /* bytes por corrotina      */
#define COROUTINE_YIELD_STRIDE   0x120        /* stride de navegação      */
#define COROUTINE_HEADER_SIZE    0x038        /* header antes das entradas*/
#define COROUTINE_SLOT_FREE      0            /* saved_sp == 0 → livre    */
#define COROUTINE_MAX_PER_SLOT   7            /* (4096-56)/576            */
#define COROUTINE_SLOT_COUNT     16
#define COROUTINE_MAX_TOTAL      112          /* 16 × 7                   */

/* Offsets dentro do bloco de contexto (negativos a partir do topo) */
#define CTX_OFFSET_ARG0      (-0x004)   /* a0 inicial                    */
#define CTX_OFFSET_ARG1      (-0x008)   /* a1 inicial                    */
#define CTX_OFFSET_A2        (-0x00C)
#define CTX_OFFSET_A3        (-0x010)
#define CTX_OFFSET_S0        (-0x014)
#define CTX_OFFSET_S1        (-0x018)
#define CTX_OFFSET_S2        (-0x01C)
#define CTX_OFFSET_S3        (-0x020)
#define CTX_OFFSET_S4        (-0x024)
#define CTX_OFFSET_S5        (-0x028)
#define CTX_OFFSET_S6        (-0x02C)
#define CTX_OFFSET_S7        (-0x030)
#define CTX_OFFSET_FP        (-0x034)
#define CTX_OFFSET_SAVED_SP  (-0x23C)   /* 0 = slot livre                */
#define CTX_OFFSET_ENTRY_FN  (-0x238)   /* RA = função de entrada        */
#define CTX_OFFSET_NEXT_CTX  (-0x11E)   /* próxima corrotina (NULL=fim)  */
#define CTX_OFFSET_NEXT_FN   (-0x11C)   /* função da próxima corrotina   */

/* Globais do scheduler */
#define g_CoroutineContextTable ((uint32_t *)0x801D2158)
#define g_CurrentCoroutineCtx   (*(uint8_t  **)0x801ACB58)
#define g_SchedulerStack        (*(uint16_t **)0x801ACB60)
#define g_CoroutineSlotMask     (*(uint16_t  *)0x801ACB54)

#endif /* COROUTINE_H */