#ifndef PORT_CONFIG_H
#define PORT_CONFIG_H

/*
 * Galerians PC Port — Global Configuration
 *
 * Native PS1 framebuffer is 320x240. Target window is 1920x1080 with
 * uniform scale factors so the port can render at native resolution
 * to an offscreen target and upscale, or render directly at target
 * resolution by multiplying GTE projection by PORT_SCALE_X/Y.
 *
 * PS1 GTE uses 4096-fixed-point (Q3.12 / Q15.12 mixed). Float
 * conversions divide by PS1_FIXED_POINT.
 *
 * Frame-rate decoupling: game logic runs at PS1_FRAME_RATE (30 Hz),
 * render runs at PORT_FRAME_RATE (60 Hz unlocked) with interpolation
 * between two game-state snapshots.
 */

#define PORT_NATIVE_WIDTH   320
#define PORT_NATIVE_HEIGHT  240
#define PORT_TARGET_WIDTH   1920
#define PORT_TARGET_HEIGHT  1080
#define PORT_SCALE_X  (PORT_TARGET_WIDTH  / (float)PORT_NATIVE_WIDTH)
#define PORT_SCALE_Y  (PORT_TARGET_HEIGHT / (float)PORT_NATIVE_HEIGHT)
#define PS1_FIXED_POINT  4096.0f
#define PS1_FRAME_RATE   30
#define PORT_FRAME_RATE  60   /* unlocked target */

#endif /* PORT_CONFIG_H */
