#ifndef GTE_H
#define GTE_H

/* GTE register defines */
#define GTE_OFX 0xC000
#define GTE_OFY 0xC800

/* PORT NOTE: For widescreen, change g_ScreenWidth and g_ScreenHeight at
 * 0x80193E30 / 0x80193E34 before Engine_Init, or intercept
 * GTE_SetScreenCenter to pass new OpenGL viewport values. */
void GTE_SetScreenCenter(int x, int y); // 0x8018c008

#endif /* GTE_H */
