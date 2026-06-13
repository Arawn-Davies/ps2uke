/*
 * PS2 controller reader for ps2uke (libpad). Returns a button bitmask; the
 * BUILD-key mapping for the real menu/game is layered on top later.
 */

#ifndef _INCLUDE_PS2_PAD_H_
#define _INCLUDE_PS2_PAD_H_

#if defined(_EE) || defined(__PS2__) || defined(PLATFORM_PS2)

void     ps2pad_init(void);
unsigned ps2pad_btns(void);   /* active-high (bit set == pressed); 0 if not ready */
void     ps2pad_sticks(int *lh, int *lv, int *rh, int *rv);  /* 0..255, 0x80=centre */

#endif /* PS2 */

#endif /* _INCLUDE_PS2_PAD_H_ */
