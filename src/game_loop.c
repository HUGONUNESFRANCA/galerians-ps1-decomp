/*
 * game_loop.c
 *
 * Scheduler de Slots — Galerians PS1
 * Endereço original: 0x80185170
 *
 * ARQUITETURA REAL (descoberta via dump de RAM):
 *   Não é um while(true) clássico.
 *   É um scheduler cooperativo: processa UM slot por invocação.
 *   O orquestrador externo é FUN_80185510 (a documentar).
 *
 *   Ciclo completo = slots 0..15 processados em sequência.
 *   Slots inativos (não em g_StateSlotMask) são pulados.
 *   Slots busy (em g_StateSlotsActive) são pulados.
 *
 * TABELAS DE ESTADO (três, não duas):
 *   0x801E2198  g_StateTable_VsyncData   armazena retorno do VSync por slot
 *   0x801E21D8  g_StateTable_FuncPtrs    ponteiros das funções de estado
 *   0x801E2218  g_StateTable_EntryPtrs   entry points / dados dos slots
 *
 * PARA O PORT:
 *   FUN_8017e0fc(0xf2000001) = VSync_WaitFrameSync
 *   Substituir por timer de alta resolução do Windows (QueryPerformanceCounter)
 *   FUN_80185510 precisa ser analisada — é o orquestrador externo
 */

#include <stdint.h>
#include "engine_state.h"
#include "state_machine.h"

/* --- Funções externas --- */
extern uint32_t VSync_WaitFrameSync(uint32_t gpu_ctrl); /* 0x8017e0fc */
extern void     FrameCycle_Orchestrator(void);           /* 0x80185510 — analisar */

/*
 * g_StateTable_VsyncData
 * Terceira tabela da state machine — armazena o valor de retorno
 * do VSync para cada slot. Uso exato ainda a determinar.
 * Endereço: 0x801E2198
 */
#define g_StateTable_VsyncData  ((uint32_t *)0x801E2198)

/*
 * GPU_CTRL_VSYNC
 * Valor codificado passado ao VSync — referencia registrador de GPU do PS1.
 * 0xF200_0001 = endereço/flag de controle de display.
 */
#define GPU_CTRL_VSYNC  0xf2000001u

/*
 * StateSlot_Scheduler()
 * Endereço PS1: 0x80185170
 *
 * Processa o próximo slot ativo do ciclo atual.
 * Chamada repetidamente por FrameCycle_Orchestrator (FUN_80185510).
 */
void StateSlot_Scheduler(void)
{
    uint32_t  vsync_result;
    uint32_t  next_index;
    uint32_t  slot_bit;
    uint32_t *func_ptr;

    do {
        /* Aguarda VSync e captura dado de sincronização */
        vsync_result = VSync_WaitFrameSync(GPU_CTRL_VSYNC);

        /* Salva resultado na tabela de VSync do slot atual */
        g_StateTable_VsyncData[g_ActiveStateIndex] = vsync_result;

        /* Avança índice */
        next_index = g_ActiveStateIndex + 1;
        g_ActiveStateIndex = (uint16_t)next_index;

        /* Ciclo completo (slots 0..15 processados): fim do frame */
        if (next_index == STATE_SLOT_COUNT) {
            FrameCycle_Orchestrator();
            return;
        }

        /* Bitmask do slot candidato */
        slot_bit = 1u << (next_index & 0x1f);

    /* Pula slots não registrados ou ocupados */
    } while (((g_StateSlotMask   & slot_bit) == 0) ||
             ((g_StateSlotsActive & slot_bit) != 0));

    /*
     * Slot ativo encontrado — despacha sua função.
     * NOTA: FUN_80185510 é chamada ANTES do dispatch.
     * Hipótese: prepara contexto / flush de DMA antes de cada slot.
     */
    func_ptr = (uint32_t *)(&g_StateTable_RetAddrs[next_index & 0xffff]);
    FrameCycle_Orchestrator();
    ((void (*)(void))*func_ptr)();
}