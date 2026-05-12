/*
 * Engine — Game Loop
 *
 * PC replacement for the PS1 game loop at 0x80185170 (StateSlot_Scheduler).
 * Will decouple update (30 Hz fixed) from render (60+ Hz unlocked) with
 * interpolation between two game-state snapshots — see PORT_FRAME_RATE
 * in include/port_config.h.
 *
 * Empty translation unit until the update/render split is implemented.
 */
