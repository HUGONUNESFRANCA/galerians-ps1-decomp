#ifndef GTE_H
#define GTE_H

/* GTE register defines */
#define GTE_OFX 0xC000  /* COP2 screen offset X — value written is (x << 16) */
#define GTE_OFY 0xC800  /* COP2 screen offset Y — value written is (y << 16) */

/* GTE_SetScreenCenter writes OFX (COP2 0xC000) and OFY (COP2 0xC800) using
 * a left shift of << 16 (i.e. the integer center is placed in the high 16
 * bits, leaving the low 16 bits as the fractional part expected by the GTE).
 *
 * PORT NOTE: For widescreen support, intercept the call to
 * GTE_SetScreenCenter and pass new center values calculated for the PC
 * viewport.
 *
 * WIDESCREEN IMPLEMENTATION:
 * - To enable widescreen: write new width to 0x80193E30 and height to
 *   0x80193E34 before Engine_Init.
 * - GTE_SetScreenCenter (0x8018c008) will auto-use these new values.
 * - Common targets:
 *     1280x720   -> OFX=640,  OFY=360
 *     1920x1080  -> OFX=960,  OFY=540
 *     2560x1440  -> OFX=1280, OFY=720
 *     3840x2160  -> OFX=1920, OFY=1080
 * - ASPECT RATIO NOTE: PS1 native is 4:3. Widescreen (16:9) will stretch
 *   geometry natively. True widescreen requires adjusting the GTE projection
 *   distance register (H) at COP2 control reg 0xE800 to fix FOV. */
void GTE_SetScreenCenter(int x, int y); // 0x8018c008

#endif /* GTE_H */
