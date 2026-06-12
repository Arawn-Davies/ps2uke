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
