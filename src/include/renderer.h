#ifndef RENDERER_H
#define RENDERER_H

#include <stdint.h>

/*
 * Sistema de Renderização — Galerians PS1
 * SDK: PsyQ v1.140 (12 Janeiro 1998 — Sony SCEE)
 *
 * Pipeline:
 *   1. Primitivas são adicionadas à Ordering Table (OT)
 *   2. OT é enviada ao GPU via DrawOTag
 *   3. DrawSync aguarda o GPU terminar
 *   4. Display buffers são alternados (double buffering)
 */

/* ── Ordering Table ────────────────────────────────────────── */
#define OT_BASE_ADDR     0x80193358u
#define OT_END_ADDR      0x80193888u
#define OT_TERMINATOR    0xFFFFFFFCu  /* tag de fim de primitiva PsyQ */

/* ── Vtable do Renderer ────────────────────────────────────── */
#define RENDERER_VTABLE  ((void **)0x8019b4c8)

/*
 * Offsets confirmados na vtable:
 * (base = 0x8019b4c8, acesso como undefined4*)
 */
#define VTBL_OFFSET_CLEAR_IMAGE   (-3)  /* -0x0C: ClearImage / ResetGraph  */
#define VTBL_OFFSET_SET_DISP_MASK (-2)  /* -0x08: SetDispMask              */
#define VTBL_OFFSET_DRAW_SYNC_A   (-1)  /* -0x04: DrawSync (direto)        */
#define VTBL_OFFSET_DRAW_SYNC_B   (15)  /* +0x3C: DrawSync (via callback)  */

/* ── Formatos de Textura ───────────────────────────────────── */
#define TEX_FORMAT_4BPP   0   /* paleta 16 cores  — size >> 2  */
#define TEX_FORMAT_8BPP   1   /* paleta 256 cores — size / 2   */
#define TEX_FORMAT_16BPP  2   /* cor direta       — size       */

/* ── Flags de Debug ────────────────────────────────────────── */
#define g_RendererDebugLevel  (*(uint16_t *)0x8019b4d2)
/* > 1 = imprime "DrawSync(%d)..." no console */

/*
 * Funções do Renderer (endereços PS1)
 *
 * 0x80178d1c  Renderer_Flush(mode)
 *             Aguarda GPU via DrawSync. Em debug, imprime log.
 *
 * 0x8017d150  Texture_Load(src, format, param3, x, y, w, tpage)
 *             Carrega textura na VRAM no formato especificado.
 *             Chama Engine_EnqueueLoadImage + SetTPage.
 *
 * 0x8017d240  SetTPage(format, param3, x, y)
 *             Configura página de textura na VRAM.
 *             Retorna TPage word para usar em primitivas.
 *
 * 0x8017afc4  ClearImage / ResetGraph
 * 0x8017albc  SetDispMask
 * 0x8017b114  DrawSync
 *
 * A IDENTIFICAR:
 * Engine_EnqueueLoadImage — endereço a confirmar
 * DrawOTag               — envio da OT ao GPU
 * ClearOTag              — inicialização da OT
 * VSync / display flip   — alternância de buffer
 */

/* ── Para o Port PC ────────────────────────────────────────── */
/*
 * Substituições necessárias:
 *
 * DrawSync()         → glFinish() / vkQueueWaitIdle()
 * DrawOTag()         → iterar OT e converter primitivas → draw calls
 * LoadImage()        → glTexImage2D() / vkCreateImage()
 * SetTPage()         → calcular UV offset na atlas de textura
 * ClearImage()       → glClear() / VkRenderPassBeginInfo
 * SetDispMask()      → controle de visibilidade do framebuffer
 * OT double buffer   → command buffers duplos (Vulkan) ou FBOs (OpenGL)
 */

#endif /* RENDERER_H */