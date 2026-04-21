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
 *   0x80187320  Camera_LoadToGTE  ✅
 *   0x8013b584  Camera_Manager    ✅
 *   0x80187350  Camera_Select     🟠 (stub — 20 XREFs, a confirmar)
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
 * Camera_Select — 0x80187350  🟠 Stub (hipótese)
 *
 * 20 XREFs apontam para esta função, o que sugere fortemente que
 * seja o ponto de entrada para troca de câmera (triggers de sala,
 * cutscenes, transições). Nome e assinatura ainda a confirmar.
 *
 * TODO:
 *   - Decompilar no Ghidra e validar a assinatura.
 *   - Confirmar se copia g_CameraTable[index] → g_ActiveCameraMatrix.
 *   - Verificar se aciona Camera_LoadToGTE após a cópia.
 *   - Mapear callers para identificar convenções de índice.
 *
 * PORT NOTE (PC):
 *     Quando confirmado, deve traduzir-se em selecionar um preset
 *     de câmera e recomputar a view matrix da cena.
 * ─────────────────────────────────────────────────────────────── */
void Camera_Select(int index)
{
    /* TODO: reconstrução pendente. */
    (void)index;
}
