/*
 * ps2_input.c -- PS2 input/timer/platform glue for Chocolate Duke3D.
 *
 * Provides the input/mouse/timer entry points display.h declares (the SDL
 * driver implemented them in display.c, which we don't build). Controller
 * reading is via ps2_pad.c; the pad->BUILD-key mapping is layered on once the
 * game boots to video.
 */

#ifdef PLATFORM_PS2

#include <stdio.h>
#include <sifrpc.h>
#include <iopcontrol.h>
#include <sbv_patches.h>
#include <ps2_cdfs_driver.h>
#include "platform.h"
#include "build.h"
#include "display.h"
#include "ps2_pad.h"

/* Must run before ANY file access (the game loads the GRP very early in main(),
   before _platform_init). Resets the IOP + enables load-module-from-buffer so
   ps2_drivers can install the embedded cdfs.irx (GRP = cdfs:/DUKE3D.GRP).
   Normally SDL2main does this; we don't link SDL. Called first thing in main(). */
void ps2_early_init(void)
{
    static int done = 0;
    if (done) return;
    done = 1;

    SifInitRpc(0);
    while (!SifIopReset("", 0)) { }
    while (!SifIopSync()) { }
    SifInitRpc(0);
    sbv_patch_enable_lmb();
    sbv_patch_disable_prefix_check();
    printf("cdfs: init_cdfs_driver() = %d\n", (int) init_cdfs_driver());
    ps2pad_init();
}

void _platform_init(int argc, char **argv, const char *title, const char *icon)
{
    (void) title; (void) icon;
    ps2_early_init();          /* idempotent; also called at the top of main() */
    _argc = argc;
    _argv = argv;
}

void _idle(void) {}

void _handle_events(void)
{
    /* Poll the pad each frame. Key mapping into BUILD's input is wired next. */
    (void) ps2pad_btns();
}

/* keyboard (keyhandler is defined by keyboard.c; timerhandler by game.c) */
void initkeys(void) {}
void uninitkeys(void) {}
uint8_t _readlastkeyhit(void) { return 0; }

/* mouse (no mouse on PS2) */
int  setupmouse(void) { return 0; }
void readmousexy(short *x, short *y) { if (x) *x = 0; if (y) *y = 0; }
void readmousebstatus(short *b) { if (b) *b = 0; }

/* timer: the game drives totalclock from getticks(); no callback needed. */
int  inittimer(int t) { (void) t; return 0; }
void uninittimer(void) {}

/* joystick (unused; pad goes through ps2_pad) */
void _joystick_init(void) {}
void _joystick_deinit(void) {}
int  _joystick_update(void) { return 0; }
int  _joystick_axis(int axis) { (void) axis; return 0; }
int  _joystick_hat(int hat) { (void) hat; return 0; }
int  _joystick_button(int button) { (void) button; return 0; }

#endif /* PLATFORM_PS2 */
