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

/* ── MDEC Hardware Registers ────────────────────────────────── */
#define MDEC_CMD_DATA_REG   0x1F801820u  /* MDEC command/data */
#define MDEC_STATUS_REG     0x1F801824u  /* MDEC status; bit 29 = busy */
#define MDEC_STATUS_BUSY    0x20000000u

/* ── DMA Channel Registers ──────────────────────────────────── */
#define DMA_DPCR_REG    0x1F8010F0u  /* channel priority/enable */
#define DMA0_MADR_REG   0x1F801080u  /* DMA0 source address (MDEC-in) */
#define DMA0_BCR_REG    0x1F801084u  /* DMA0 block count/size */
#define DMA0_CHCR_REG   0x1F801088u  /* DMA0 channel control (start to MDEC) */
#define DMA1_MADR_REG   0x1F801090u  /* DMA1 destination address (MDEC-out) */

/* ── MDEC/DMA Globals (Ghidra: DAT_8019b7xx — shadow HW reg ptrs) */
#define g_MDEC_Cmd    (*(uint32_t *)0x8019b790u)  /* → MDEC_CMD_DATA_REG */
#define g_MDEC_Status (*(uint32_t *)0x8019b794u)  /* → MDEC_STATUS_REG   */
#define g_DMA_DPCR    (*(uint32_t *)0x8019b798u)  /* → DMA_DPCR_REG      */
#define g_DMA0_MADR   (*(uint32_t *)0x8019b760u)  /* → DMA0_MADR_REG     */
#define g_DMA0_BCR    (*(uint32_t *)0x8019b764u)  /* → DMA0_BCR_REG      */
#define g_DMA0_CHCR   (*(uint32_t *)0x8019b768u)  /* → DMA0_CHCR_REG     */
#define g_DMA1_MADR   (*(uint32_t *)0x8019b76cu)  /* → DMA1_MADR_REG     */

/* ── Globals ────────────────────────────────────────────────── */
#define g_GPUStatusPtr       (*(void **)0x8019b5e4)  /* ptr to GPU status   */
#define g_DMAStatusPtr       (*(void **)0x8019b5d8)  /* ptr to DMA status   */
#define g_RenderQueueWrite   (*(uint32_t *)0x8019b5f8)
#define g_RenderQueueRead    (*(uint32_t *)0x8019b5fc)
#define g_DrawOTagBase       (*(void **)0x801d04a0)  /* OT base — PTR_PsyQ_DrawOTag */
#define g_RendererVtable     ((void **)0x8019b4c8)
#define g_RendererDebugLevel (*(uint16_t *)0x8019b4d2)  /* >1 = debug log */

/* ── DispEnv / DrawEnv Double-Buffer (Display_InitBuffers) ─── */
#define g_DispEnv_0     ((void *)0x801ac9b8u)  /* PsyQ DISPENV buffer 0 */
#define g_DispEnv_1     ((void *)0x801aca30u)  /* PsyQ DISPENV buffer 1 */
#define g_DrawEnv_0     ((void *)0x801aca14u)  /* PsyQ DRAWENV buffer 0 */
#define g_DrawEnv_1     ((void *)0x801aca8cu)  /* PsyQ DRAWENV buffer 1 */
#define g_DrawEnvActive ((void *)0x801acab8u)  /* active DrawEnv pointer */

/* ── Display Buffer Coordinates ─────────────────────────────── */
#define g_DispX0      (*(int16_t *)0x80193e20u)  /* display buffer 0 X */
#define g_DispX1      (*(int16_t *)0x80193e24u)  /* display buffer 1 X */
#define g_DispY0      (*(int16_t *)0x80193e28u)  /* display buffer 0 Y */
#define g_DispY1      (*(int16_t *)0x80193e2cu)  /* display buffer 1 Y */
#define g_DrawHeight  (*(int16_t *)0x80193e38u)  /* draw area height */
#define g_DrawWidth   (*(int16_t *)0x80193e3cu)  /* draw area width */
#define g_ScreenWidth  (*(int32_t *)0x80193E30u) /* native: 320 */
#define g_ScreenHeight (*(int32_t *)0x80193E34u) /* native: 240 */

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
 * 0x8018d4d8  PsyQ_SetDispEnv()
 *             Confirmed ✅ — sets PsyQ DISPENV (display environment).
 *
 * 0x8018d598  PsyQ_SetDrawEnv()
 *             Confirmed ✅ — sets PsyQ DRAWENV (drawing environment).
 *
 * PENDENTE:
 * Engine_EnqueueLoadImage — endereço a confirmar
 * ClearOTag               — endereço a confirmar
 */

/*
 * FMV/MDEC Pipeline Functions
 *
 * 0x8017e574  MDEC_StartDecode(uint32_t *cmd, uint size)
 *             Calls MDEC_WaitReady, configures DMA0+DMA1, sends MDEC command.
 *             5 callers (different decode modes).
 *
 * 0x8017e690  MDEC_WaitReady()
 *             Polls MDEC_STATUS_BUSY (bit 29) with timeout 0x100000.
 *             Returns 0=ready, 0xFFFFFFFF=timeout error.
 *             On timeout: calls Debug_PrintError("MDEC_in_sync").
 *
 * 0x8017e7d0  Debug_PrintError(string)
 *             Same pattern as "VSync: timeout" printer.
 *             Used for MDEC and other hardware timeout errors.
 *
 * Debug strings:
 *   0x8011bdb4  "MDEC_in_sync"  — MDEC timeout error
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
 *
 * MDEC/FMV replacement:
 * MDEC_WaitReady      → no-op (software decoder handles sync)
 * MDEC_StartDecode    → libavcodec / custom MPEG1 decoder
 * DMA0/DMA1           → memcpy to/from decoder buffers
 * XA streaming        → file I/O + XA-ADPCM decoder
 */

#endif /* RENDERER_H */
