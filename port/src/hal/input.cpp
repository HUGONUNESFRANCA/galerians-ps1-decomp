/*
 * HAL — Input
 *
 * SDL2 keyboard polling mapped to the PS1 BTN_* bitmask format
 * documented in include/ps1/input.h.
 *
 * Stick state is kept simple for now: keyboard provides only a
 * centered analog value (0x80) since no SDL_GameController is
 * wired up yet. The function surface matches the PS1 channel API.
 */

#include "hal/input.h"
#include "ps1/input.h"

#include <SDL.h>
#include <cstring>

namespace {

constexpr int kPortCount = 2;

struct PortState {
    uint16_t buttons;
    uint8_t  lx, ly, rx, ry;
};
PortState g_ports[kPortCount];

bool g_quit_requested = false;

uint16_t MapKeyboardToButtons(void) {
    const Uint8 *ks = SDL_GetKeyboardState(nullptr);
    uint16_t b = 0;
    if (ks[SDL_SCANCODE_UP])        b |= BTN_UP;
    if (ks[SDL_SCANCODE_DOWN])      b |= BTN_DOWN;
    if (ks[SDL_SCANCODE_LEFT])      b |= BTN_LEFT;
    if (ks[SDL_SCANCODE_RIGHT])     b |= BTN_RIGHT;
    if (ks[SDL_SCANCODE_Z])         b |= BTN_CROSS;
    if (ks[SDL_SCANCODE_X])         b |= BTN_CIRCLE;
    if (ks[SDL_SCANCODE_A])         b |= BTN_SQUARE;
    if (ks[SDL_SCANCODE_S])         b |= BTN_TRIANGLE;
    if (ks[SDL_SCANCODE_Q])         b |= BTN_L1;
    if (ks[SDL_SCANCODE_W])         b |= BTN_R1;
    if (ks[SDL_SCANCODE_RETURN])    b |= BTN_START;
    if (ks[SDL_SCANCODE_BACKSPACE]) b |= BTN_SELECT;
    return b;
}

} /* namespace */

extern "C" void HAL_Input_Init(void) {
    std::memset(g_ports, 0, sizeof(g_ports));
    for (auto &p : g_ports) {
        p.lx = p.ly = p.rx = p.ry = 0x80;  /* centered */
    }
    g_quit_requested = false;
}

extern "C" void HAL_Input_Shutdown(void) {
    /* nothing to release until SDL_GameController is added */
}

extern "C" bool HAL_Input_Poll(void) {
    SDL_Event ev;
    while (SDL_PollEvent(&ev)) {
        if (ev.type == SDL_QUIT) g_quit_requested = true;
        if (ev.type == SDL_KEYDOWN && ev.key.keysym.sym == SDLK_ESCAPE)
            g_quit_requested = true;
    }
    g_ports[0].buttons = MapKeyboardToButtons();
    return !g_quit_requested;
}

extern "C" uint16_t HAL_GetButtons(int port) {
    if (port < 0 || port >= kPortCount) return 0;
    return g_ports[port].buttons;
}

extern "C" uint8_t HAL_GetAnalogX(int port, int stick) {
    if (port < 0 || port >= kPortCount) return 0x80;
    return stick == HAL_STICK_RIGHT ? g_ports[port].rx : g_ports[port].lx;
}

extern "C" uint8_t HAL_GetAnalogY(int port, int stick) {
    if (port < 0 || port >= kPortCount) return 0x80;
    return stick == HAL_STICK_RIGHT ? g_ports[port].ry : g_ports[port].ly;
}
