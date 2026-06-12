/*
 * PS2 controller reader for ps2uke (libpad). Returns a button bitmask; the
 * BUILD-key mapping for the real menu/game is layered on top later.
 */

#ifndef _INCLUDE_PS2_PAD_H_
#define _INCLUDE_PS2_PAD_H_

#ifdef PLATFORM_PS2

void     ps2pad_init(void);
unsigned ps2pad_btns(void);   /* active-high (bit set == pressed); 0 if not ready */

#endif /* PLATFORM_PS2 */

#endif /* _INCLUDE_PS2_PAD_H_ */
