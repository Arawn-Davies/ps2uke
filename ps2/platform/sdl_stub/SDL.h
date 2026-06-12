/*
 * Minimal no-op SDL shim for the PS2 build of Chocolate Duke3D.
 *
 * A few game files (game.c, global.c, menues.c) include "SDL.h" for window-
 * grab / cursor / quit calls that are meaningless without a desktop window
 * manager. On PS2 those are genuine no-ops, so we satisfy the references here
 * instead of pulling in SDL. Real video/input/audio come from ps2/platform.
 */

#ifndef _PS2_SDL_STUB_H_
#define _PS2_SDL_STUB_H_

typedef struct SDL_Surface { void *pixels; int w, h; } SDL_Surface;

#define SDL_INIT_VIDEO 0x00000020
#define SDL_GRAB_QUERY 0
#define SDL_GRAB_OFF   0
#define SDL_GRAB_ON    1

static __inline void SDL_Quit(void) {}
static __inline void SDL_QuitSubSystem(unsigned int flags) { (void) flags; }
static __inline int  SDL_ShowCursor(int toggle) { (void) toggle; return 0; }
static __inline int  SDL_WM_GrabInput(int mode) { (void) mode; return SDL_GRAB_OFF; }

#endif /* _PS2_SDL_STUB_H_ */
