#ifndef RENDERER_H
#define RENDERER_H

#include <stdint.h>

/*
 * Sistema de Renderização — Galerians PS1  (COMPLETE)
 * SDK: PsyQ v1.140 (12 Janeiro 1998 — Sony SCEE)
 *
 * Pipeline por frame:
 *   1. ClearOTag()   — inicializa Ordering Table
 *   2. AddPrim()     — primitivas adicionadas à OT
 *   3. DrawOTag()    — envia OT ao GPU (linked list de primitivas)
 *   4. DrawSync()    — aguarda GPU finalizar
 *   5. VSync()       — aguarda VBlank
 *   6. Frame_Flip()  — PutDispEnv + DrawOTag + clears GPU DMA register
 */

/* ── Ordering Table — 6 buffers (COMPLETE) ─────────────────── */
#define OT_BUFFER_2      0x801932b4u  /* buffer 2                   */
#define OT_BUFFER_0      0x80193330u  /* buffer 0 — main gameplay   */
#define OT_BUFFER_1      0x801933acu  /* buffer 1 — secondary       */
#define OT_BUFFER_3      0x80193428u  /* buffer 3                   */
#define OT_BUFFER_4      0x801934a4u  /* buffer 4                   */
#define OT_BUFFER_5      0x80193520u  /* buffer 5                   */
#define OT_TERMINATOR    0xFFFFFFFCu  /* PsyQ end-of-OT tag         */

/* ── GPU Hardware Registers ────────────────────────────────── */
#define GPU_DISPLAY_REG  0x1F8010A8u  /* GPU Display Mode register (PutDispEnv) */
#define GPU_DMA_REG      0x1F801814u  /* GPU DMA Control register — cleared on flip */
#define GPU_STATUS_BUSY_BIT  24       /* bit 24: GPU busy (drawing) */
#define GPU_STATUS_DMA_BIT   26       /* bit 26: DMA transfer active */

/* ── Globals ────────────────────────────────────────────────── */
#define g_GPUStatusPtr       (*(void **)0x8019b5e4)  /* ptr to GPU status   */
#define g_DMAStatusPtr       (*(void **)0x8019b5d8)  /* ptr to DMA status   */
#define g_RenderQueueWrite   (*(uint32_t *)0x8019b5f8)
#define g_RenderQueueRead    (*(uint32_t *)0x8019b5fc)
#define g_DrawOTagBase       (*(void **)0x801d04a0)  /* OT base — PTR_PsyQ_DrawOTag */
#define g_RendererVtable     ((void **)0x8019b4c8)
#define g_RendererDebugLevel (*(uint16_t *)0x8019b4d2)  /* >1 = debug log */

/* ── Vtable offsets (base = g_RendererVtable, as uint32_t*) ── */
#define VTBL_OFFSET_CLEAR_IMAGE   (-3)  /* -0x0C: ClearImage / ResetGraph (0x8017afc4) */
#define VTBL_OFFSET_SET_DISP_MASK (-2)  /* -0x08: SetDispMask             (0x8017a1bc) */
#define VTBL_OFFSET_DRAW_SYNC_A   (-1)  /* -0x04: DrawSync direct         (0x8017b114) */
#define VTBL_OFFSET_DRAW_SYNC_B   (15)  /* +0x3C: DrawSync via callback   ← Renderer_Flush */

/* ── Formatos de Textura ───────────────────────────────────── */
#define TEX_FORMAT_4BPP   0   /* paleta 16 cores  */
#define TEX_FORMAT_8BPP   1   /* paleta 256 cores */
#define TEX_FORMAT_16BPP  2   /* cor direta       */

/*
 * Funções Confirmadas (endereços PS1)
 *
 * 0x8017b06c  Frame_Flip()
 *             Writes GPU Display Mode to GPU_DISPLAY_REG (PutDispEnv).
 *             Calls PsyQ DrawOTag via g_DrawOTagBase (PTR_PsyQ_DrawOTag_801d04a0).
 *             Clears GPU DMA control register (GPU_DMA_REG = 0).
 *
 * 0x8017b114  DrawSync(mode)
 *             mode=0: blocking wait (VSync + GPU drain).
 *             mode≠0: non-blocking status check.
 *             Returns: 0=success, 0xFFFFFFFF=error, 1=pending.
 *
 * 0x8017b8a8  PsyQ_DrawOTag / PutDispEnv
 *             Called with different args per operation.
 *             OT base pointer: g_DrawOTagBase (auto-labeled by Ghidra).
 *
 * 0x80183a00  RenderQueue_Dispatch
 *             Sends GPU command queue.
 *
 * 0x8017b284  GPU_CheckTimeout
 *             Monitors GPU hang condition.
 *
 * 0x8014d2cc  BattleTransition_Render(ot_case, param_2)
 *             Renders combat transition/result screen.
 *             Iterates 4 camera slots, 6 OT buffer cases.
 *             Ends with Trigger_GameOver + State_ResetBeforeDeath.
 *
 * 0x80178d1c  Renderer_Flush(mode)
 *             Calls DrawSync via vtable (+0x3C). Logs in debug mode.
 *
 * 0x8017d150  Texture_Load(src, format, param3, x, y, w, tpage)
 *             Carrega textura na VRAM. Chama Engine_EnqueueLoadImage + SetTPage.
 *
 * 0x8017d240  SetTPage(format, param3, x, y)
 *             Configura página de textura na VRAM.
 *
 * 0x8017afc4  ClearImage / ResetGraph
 * 0x8017a1bc  SetDispMask
 *
 * PENDENTE:
 * Engine_EnqueueLoadImage — endereço a confirmar
 * ClearOTag               — endereço a confirmar
 */

/* ── Port PC ────────────────────────────────────────────────── */
/*
 * Frame_Flip          → glSwapBuffers() / vkQueuePresentKHR()
 * DrawOTag            → iterate OT linked list → convert prims → draw calls
 * DrawSync            → glFinish() / vkWaitForFences()
 * PutDispEnv          → glViewport() + framebuffer bind
 * RenderQueue_Dispatch→ OpenGL command buffer / Vulkan command buffer
 * LoadImage           → glTexImage2D() / vkCreateImage()
 * SetTPage            → UV offset na atlas de textura
 * ClearImage          → glClear() / VkRenderPassBeginInfo
 * SetDispMask         → controle de visibilidade do framebuffer
 * OT 6 buffers        → 6 command buffers (Vulkan) ou 6 FBOs (OpenGL)
 */

#endif /* RENDERER_H */
