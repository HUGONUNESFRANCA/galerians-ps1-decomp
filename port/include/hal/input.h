#ifndef HAL_INPUT_H
#define HAL_INPUT_H

#include <stdint.h>

/*
 * HAL — Input
 *
 * Maps SDL2 input onto the PS1 controller bitmask format documented
 * in include/ps1/input.h (BTN_CROSS, BTN_CIRCLE, BTN_UP, …).
 *
 * Keyboard mapping (port 0):
 *   Arrow keys → D-pad
 *   Z          → BTN_CROSS
 *   X          → BTN_CIRCLE
 *   A          → BTN_SQUARE
 *   S          → BTN_TRIANGLE
 *   Q          → BTN_L1
 *   W          → BTN_R1
 *   Enter      → BTN_START
 *   Backspace  → BTN_SELECT
 *
 * Analog sticks: keyboard provides 0x00/0x80/0xFF tri-state per axis;
 * a real SDL_GameController hookup will land later.
 */

#ifdef __cplusplus
extern "C" {
#endif

enum HALStick { HAL_STICK_LEFT = 0, HAL_STICK_RIGHT = 1 };

void HAL_Input_Init(void);
void HAL_Input_Shutdown(void);

/* PORT_NOTE: replaces the controller polling done implicitly by PsyQ
 * (BIOS-level controller read in Controller_Update @ 0x80185cc8).
 * Returns false if the user requested quit (window-close / ESC). */
bool HAL_Input_Poll(void);

/* PS1-format bitmask (BTN_* constants from include/ps1/input.h) */
uint16_t HAL_GetButtons(int port);

/* Analog axis 0x00..0xFF, 0x80 = centered (PS1 convention). */
uint8_t HAL_GetAnalogX(int port, int stick);
uint8_t HAL_GetAnalogY(int port, int stick);

#ifdef __cplusplus
}
#endif

#endif /* HAL_INPUT_H */
