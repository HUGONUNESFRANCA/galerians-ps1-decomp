#ifndef HAL_RENDERER_H
#define HAL_RENDERER_H

/*
 * HAL — Renderer
 *
 * Maps PS1 PsyQ rendering calls onto OpenGL 3.3 core. The functions
 * below are 1:1 with the PsyQ calls documented in docs/MemoryMap.md
 * under "Renderer — Pipeline PsyQ".
 *
 * PORT_NOTE: PS1 issues GPU primitives via an Ordering Table (DrawOTag).
 * On PC we replace that linked-list traversal with a per-frame command
 * list flushed in HAL_Renderer_EndFrame.
 */

#ifdef __cplusplus
extern "C" {
#endif

/* PORT_NOTE: replaces PsyQ Engine_Init's renderer-setup block
 * (Display_Enable + SetDrawEnv + GTE_SetScreenCenter). */
bool HAL_Renderer_Init(int width, int height);

void HAL_Renderer_Shutdown(void);

/* PORT_NOTE: replaces ClearOTag + ClearImage prologue of a PS1 frame. */
void HAL_Renderer_BeginFrame(void);

/* PORT_NOTE: replaces Frame_Flip (0x8017b06c)
 * = PutDispEnv + DrawOTag + clear GPU DMA register
 * On PC this is just SDL_GL_SwapWindow(). */
void HAL_Renderer_EndFrame(void);

/* PORT_NOTE: replaces DrawSync (0x8017b114)
 *   mode == 0 → blocking wait (glFinish)
 *   mode != 0 → non-blocking status poll (returns 0 always for now) */
int HAL_DrawSync(int mode);

/* PORT_NOTE: replaces GTE_SetScreenCenter (0x8018c008) which writes
 * GTE OFX/OFY registers. On PC this becomes a glViewport adjustment
 * and is also fed into the projection matrix in the GTE HAL. */
void HAL_SetScreenCenter(int x, int y);

/* Returns the SDL_GLContext-backed window pointer (opaque). */
void *HAL_Renderer_GetWindow(void);

#ifdef __cplusplus
}
#endif

#endif /* HAL_RENDERER_H */
