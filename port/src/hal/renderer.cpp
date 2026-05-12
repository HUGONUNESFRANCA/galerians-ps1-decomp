/*
 * HAL — Renderer (OpenGL 3.3 core stub)
 *
 * Stubs out the PsyQ rendering surface for the port. Window/GL context
 * creation lives in main.cpp for now; this module is the long-term home
 * for the OT-replacement command-list and the SwapBuffers call.
 */

#include "hal/renderer.h"
#include "port_config.h"

#include <glad/glad.h>
#include <SDL.h>
#include <cstdio>

namespace {
SDL_Window *g_window = nullptr;
int g_center_x = PORT_TARGET_WIDTH / 2;
int g_center_y = PORT_TARGET_HEIGHT / 2;
}

extern "C" bool HAL_Renderer_Init(int width, int height) {
    /* PORT_NOTE: replaces Engine_Init's renderer-setup block on PS1
     * (GTE_SetScreenCenter + Display_Enable + SetDrawEnv).
     *
     * The actual window + GL context creation currently happens in
     * main.cpp; this entry point will own it once the bootstrap path
     * is consolidated. */
    (void)width;
    (void)height;
    g_window = SDL_GL_GetCurrentWindow();
    return g_window != nullptr;
}

extern "C" void HAL_Renderer_Shutdown(void) {
    g_window = nullptr;
}

extern "C" void HAL_Renderer_BeginFrame(void) {
    /* PORT_NOTE: replaces PS1 ClearOTag + ClearImage at frame start. */
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

extern "C" void HAL_Renderer_EndFrame(void) {
    /* PORT_NOTE: replaces Frame_Flip (0x8017b06c) which on PS1 calls
     * PutDispEnv + DrawOTag + clears the GPU DMA control register. */
    if (g_window) SDL_GL_SwapWindow(g_window);
}

extern "C" int HAL_DrawSync(int mode) {
    /* PORT_NOTE: replaces DrawSync (0x8017b114).
     *   mode == 0 → blocking wait → glFinish
     *   mode != 0 → non-blocking status poll → return 0 (idle) */
    if (mode == 0) {
        glFinish();
    }
    return 0;
}

extern "C" void HAL_SetScreenCenter(int x, int y) {
    /* PORT_NOTE: replaces GTE_SetScreenCenter (0x8018c008) writing
     * OFX/OFY GTE control registers. On PC we re-center the viewport
     * for the upscale stage; HAL_GTE_SetScreenCenter also receives
     * the same values and rebuilds the projection matrix. */
    g_center_x = x;
    g_center_y = y;
    glViewport(0, 0, PORT_TARGET_WIDTH, PORT_TARGET_HEIGHT);
}

extern "C" void *HAL_Renderer_GetWindow(void) {
    return g_window;
}
