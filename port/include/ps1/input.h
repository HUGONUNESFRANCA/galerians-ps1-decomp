#ifndef INPUT_H
#define INPUT_H

#include <stdint.h>

/*
 * Sistema de Input — Galerians PS1
 *
 * 16 portas × 0x40 bytes = 0x400 bytes
 * Base: 0x801AC900  (campo +0x28 da EngineState)
 *
 * Tipos de controle (channel_flags em 0x801AC8D0):
 *   0x00  porta vazia
 *   0x04  controle digital (D-Pad apenas)
 *   0x10  DualShock analógico com rumble
 */

#define INPUT_CHANNEL_COUNT   16
#define INPUT_CHANNEL_STRIDE  0x40
#define INPUT_BASE_ADDR       0x801AC900u
#define INPUT_FLAGS_ADDR      0x801AC8D0u

/* Thresholds dos analógicos */
#define ANALOG_THRESHOLD_LOW  0x40   /* < 64  = esquerda/cima  */
#define ANALOG_THRESHOLD_HIGH 0xC1   /* > 192 = direita/baixo  */
#define ANALOG_DEADZONE_MIN   0x40
#define ANALOG_DEADZONE_MAX   0xC0

/* Bitmasks dos botões analógicos */
#define ANALOG_LEFT_LEFT    0x8000
#define ANALOG_LEFT_RIGHT   0x2000
#define ANALOG_LEFT_UP      0x1000
#define ANALOG_LEFT_DOWN    0x4000

/* Bitmasks dos botões digitais PS1 */
#define BTN_SELECT    0x0001
#define BTN_L3        0x0002
#define BTN_R3        0x0004
#define BTN_START     0x0008
#define BTN_UP        0x0010
#define BTN_RIGHT     0x0020
#define BTN_DOWN      0x0040
#define BTN_LEFT      0x0080
#define BTN_L2        0x0100
#define BTN_R2        0x0200
#define BTN_L1        0x0400
#define BTN_R1        0x0800
#define BTN_TRIANGLE  0x1000
#define BTN_CIRCLE    0x2000
#define BTN_CROSS     0x4000
#define BTN_SQUARE    0x8000

typedef struct {
    uint16_t  buttons_current;
    uint16_t  _pad_02;
    uint16_t  buttons_pressed;
    uint16_t  _pad_06;
    uint16_t  buttons_released;
    uint16_t  _pad_0A;
    uint16_t  buttons_prev;
    uint16_t  button_mask;
    uint8_t   _pad_10;
    uint8_t   turbo_limit;
    uint8_t   repeat_counter;
    uint8_t   repeat_timer;
    uint8_t   analog_left_x;
    uint8_t   _pad_15[3];
    uint8_t   analog_left_y;
    uint8_t   _pad_19[3];
    uint8_t   analog_right_x;
    uint8_t   _pad_1D[3];
    uint8_t   analog_right_y;
    uint8_t   _pad_21[3];
    uint32_t  rumble_data;
    uint8_t   _pad_28[4];
    uint16_t  analog_buttons;
    uint16_t  _pad_2E;
    uint16_t  analog_pressed;
    uint16_t  _pad_32;
    uint16_t  analog_released;
    uint16_t  _pad_36;
    uint8_t   vibration_active;
    uint8_t   analog_repeat;
    uint8_t   analog_timer;
    uint8_t   combine_flag;
    uint16_t  analog_buttons_prev;
    uint16_t  _pad_3E;
} ControllerChannel;

#define g_Controllers    ((ControllerChannel *)INPUT_BASE_ADDR)
#define g_ControllerFlags ((uint8_t *)INPUT_FLAGS_ADDR)

/* Macros de acesso rápido */
#define CTRL(n)              (g_Controllers[n])
#define BTN_DOWN_NOW(n, b)   (g_Controllers[n].buttons_current  & (b))
#define BTN_JUST_PRESSED(n,b)(g_Controllers[n].buttons_pressed  & (b))
#define BTN_JUST_RELEASED(n,b)(g_Controllers[n].buttons_released & (b))

#endif /* INPUT_H */