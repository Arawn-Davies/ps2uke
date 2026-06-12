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
   on the mtapman IOP module (a deadlock under PCSX2), and SDL2's PS2 port has no
   joystick backend anyway. We read the DualShock ourselves with libpad
   (ps2_pad.c, which loads SIO2MAN/PADMAN and owns padInit), so make this a
   no-op. (Same override trick ps2oom uses for init_audio_driver.) */
#include <stdbool.h>

int init_joystick_driver(bool init_dependencies)
{
    (void) init_dependencies;
    return 0;   /* JOYSTICK_INIT_STATUS_OK */
}
void deinit_joystick_driver(bool deinit_dependencies)
{
    (void) deinit_dependencies;
}
