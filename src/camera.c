#include "include/camera.h"
#include "include/ghidra_types.h"

#include <stddef.h>

/*
 * ──────────────────────────────────────────────────────────────
 * Galerians PS1 — Sistema de Câmera
 * ──────────────────────────────────────────────────────────────
 *
 * Este arquivo reconstrói as funções de câmera identificadas no
 * dump de RAM (DuckStation → Ghidra). Todos os endereços absolutos
 * referenciam o espaço de RAM PS1 (0x80000000 – 0x801FFFFF) e a
 * scratchpad COP2 em 0x1F800168.
 *
 *   0x80187320  Camera_LoadToGTE    ✅
 *   0x8013b584  Camera_Manager      ✅
 *   0x80187350  Camera_RecordFrame  ✅
 *
 * Port Notes (Fase 2 — Windows/OpenGL) estão no final de cada função.
 * ──────────────────────────────────────────────────────────────
 */


/* ────────────────────────────────────────────────────────────────
 * Camera_LoadToGTE — 0x80187320  ✅ Confirmado
 *
 * Copia 32 bytes (8 × uint32) do buffer global g_CameraBuffer
 * (0x801eb560) para a scratchpad GTE (param_1, tipicamente
 * 0x1F800168). Esse bloco alimenta os registradores COP2
 * (R11..R33 + TRX/TRY/TRZ) no próximo ciclo de transformação.
 *
 * Pseudocódigo do dump:
 *     for (i = 0; i < 8; i++)
 *         dst[i] = g_CameraBuffer[i];
 *
 * PORT NOTE (PC):
 *     A scratchpad COP2 não existe fora do PS1. Em OpenGL/Vulkan,
 *     a matriz de visão equivalente deve ser enviada ao shader
 *     como uniform:
 *         glUniformMatrix4fv(u_view, 1, GL_FALSE, view_matrix);
 *     Em Vulkan, via UBO / Push Constant.
 * ─────────────────────────────────────────────────────────────── */
void Camera_LoadToGTE(uint32_t *dst)
{
    uint32_t *src = g_CameraBuffer;   /* 0x801eb560 */
    int i;

    /* Loop fiel ao dump: 8 iterações copiando uint32 por vez. */
    for (i = 0; i < 8; i++) {
        dst[i] = src[i];
    }
}


/* ────────────────────────────────────────────────────────────────
 * Camera_Manager — 0x8013b584  ✅ Confirmado
 *
 * Reseta o estado dos 4 slots de câmera residentes em
 * g_CameraSlots (0x801C1778) e zera o ponteiro de função global
 * armazenado em PTR_FUN_801c1768.
 *
 * Para cada slot (stride 0xC60):
 *   slot->active_flag   = 0      (campo +0xC3E)
 *   slot->func_ptr      = NULL   (campo +0x00)
 *
 * Pseudocódigo do dump:
 *     for (i = 0; i < 4; i++) {
 *         CameraSlot *s = &g_CameraSlots[i];
 *         s->active_flag = 0;
 *         s->func_ptr    = NULL;
 *     }
 *     *(void **)0x801c1768 = NULL;
 *
 * PORT NOTE (PC):
 *     Equivalente a resetar o array de câmeras ao carregar uma
 *     nova cena/sala. Uma implementação PC limpa seria:
 *         memset(scene->cameras, 0, sizeof(scene->cameras));
 *         scene->active_camera_cb = NULL;
 * ─────────────────────────────────────────────────────────────── */
void Camera_Manager(void)
{
    CameraSlot *slot = g_CameraSlots; /* 0x801C1778 */
    int i;

    for (i = 0; i < CAMERA_SLOT_COUNT; i++) {
        slot->func_ptr    = NULL;   /* +0x00    */
        slot->active_flag = 0;      /* +0xC3E   */

        /* Avança manualmente pelo stride confirmado (0xC60). */
        slot = (CameraSlot *)((uint8_t *)slot + CAMERA_SLOT_STRIDE);
    }

    /* Ponteiro global PTR_FUN_801c1768 — função corrente é invalidada. */
    *(void **)0x801c1768 = NULL;
}


/* ────────────────────────────────────────────────────────────────
 * Camera_RecordFrame — 0x80187350  ✅ Confirmado
 *
 * Grava o estado atual de g_CameraBuffer (0x801eb560) no ring buffer
 * de histórico de câmera apontado por g_CameraHistoryPtr (0x801eb544).
 *
 * Layout do buffer apontado por g_CameraHistoryPtr:
 *   +0x00  void*    (ponteiro ou cabeçalho)
 *   +0x02  int16_t  frame_index — índice de escrita atual
 *   +0x04  uint32_t[8][N]  — entradas (stride 0x20 por frame)
 *
 * Pseudocódigo do dump:
 *     base  = g_CameraHistoryPtr
 *     idx   = *(int16_t *)(base + 2)
 *     dst   = (uint32_t *)(base + 0x04 + idx * 0x20)
 *     for (i = 0; i < 8; i++) dst[i] = g_CameraBuffer[i]
 *     *(int16_t *)(base + 2) = idx + 1
 *
 * PORT NOTE (PC — Freecam):
 *     Para implementar freecam, interceptar Camera_LoadToGTE e
 *     substituir o conteúdo de g_CameraBuffer pela view matrix
 *     construída a partir da posição e ângulos controlados pelo
 *     jogador antes que Camera_RecordFrame seja chamada.
 *     O ring buffer de histórico pode ser ignorado completamente
 *     ou reaproveitado para suavizar transições de sala/cutscene
 *     via interpolação linear entre frames consecutivos.
 * ─────────────────────────────────────────────────────────────── */
void Camera_RecordFrame(void)
{
    uint8_t  *base = (uint8_t *)g_CameraHistoryPtr;
    int16_t   idx  = *(int16_t *)(base + 2);
    uint32_t *dst  = (uint32_t *)(base + CAMERA_HISTORY_DATA_OFFSET
                                       + idx * CAMERA_HISTORY_STRIDE);
    uint32_t *src  = g_CameraBuffer;
    int i;

    for (i = 0; i < 8; i++)
        dst[i] = src[i];

    *(int16_t *)(base + 2) = idx + 1;
}
