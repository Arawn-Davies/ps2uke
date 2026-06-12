/*
 * ps2_stubs.c -- small symbols jfbuild/JFDuke3D expect that are normally
 * supplied by generated/desktop files we don't build on PS2:
 *   - appicon_bmp[]      : the window icon (no window manager on PS2)
 *   - build_version/...  : normally version-auto.c (generated at build time)
 */

/* Empty app icon: sdlayer2.c only sets a window icon when size > 0. */
const unsigned char appicon_bmp[1] = { 0 };
const int appicon_bmp_size = 0;

/* Build banner strings (engine.c / baselayer reference these). */
const char *build_version = "ps2uke";
const char *build_date = __DATE__;
const char *build_time = __TIME__;

/* Override libps2_drivers' init_joystick_driver(): its mtapInit() spins forever
   on the mtapman IOP module (a deadlock under PCSX2). We don't need the
   multitap, so load just the pad modules (SIO2MAN/PADMAN) here -- enough for
   SDL2's PS2 joystick backend to padInit/padRead the controller -- and skip
   mtap entirely. (Same override trick ps2oom uses for init_audio_driver.) */
#include <stdbool.h>
#include <loadfile.h>     /* SifLoadModule */

int init_joystick_driver(bool init_dependencies)
{
    (void) init_dependencies;
    /* Harmless if already loaded (SifLoadModule just errors). */
    SifLoadModule("rom0:SIO2MAN", 0, NULL);
    SifLoadModule("rom0:PADMAN", 0, NULL);
    return 0;   /* JOYSTICK_INIT_STATUS_OK */
}
void deinit_joystick_driver(bool deinit_dependencies)
{
    (void) deinit_dependencies;
}
