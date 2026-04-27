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
 * viewport. */
void GTE_SetScreenCenter(int x, int y); // 0x8018c008

#endif /* GTE_H */
