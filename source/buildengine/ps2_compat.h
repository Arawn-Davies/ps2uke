/*
 * PlayStation 2 compatibility header for the BUILD engine.
 *
 * Models the proven unix_compat.h (the PS2 EE toolchain is newlib-based and
 * largely POSIX-ish), but WITHOUT pulling in SDL: the PS2 supplies its own
 * driver (ps2_driver.c) for video/input/timing, the way sdl_driver.c does on
 * desktop. See PORTING.md.
 *
 * "Build Engine & Tools" Copyright (c) 1993-1997 Ken Silverman
 * See BUILDLIC.TXT for license info. Not part of Ken's original release.
 */

#ifndef _INCLUDE_PS2_COMPAT_H_
#define _INCLUDE_PS2_COMPAT_H_

#if (!defined PLATFORM_PS2)
#error PLATFORM_PS2 is not defined.
#endif

#define __int64 long long

/* display.h declares the driver interface (drawpixel, ...) in SDL's Uint*
   spelling even off the SDL path; provide them like doscmpat.h does. */
typedef unsigned long  Uint32;
typedef unsigned short Uint16;
typedef unsigned char  Uint8;

/*
 * NOT PLATFORM_SUPPORTS_SDL: ps2_driver.c is the platform layer. We still owe
 * the engine a timer rate (platform.h #errors without it). Duke/BUILD drove the
 * DOS timer at 120Hz; ps2_driver.c reproduces that cadence.
 */
#define PLATFORM_TIMER_HZ 120

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

/* DOS-ism shims (same shape as unix_compat.h). */
#define __far
#define __interrupt
#define interrupt
#define far
#define cdecl                 /* Watcom/DOS calling-convention keyword */
#define kmalloc(x)  malloc(x)
#define kkmalloc(x) malloc(x)
#define kfree(x)    free(x)
#define kkfree(x)   free(x)
#define FP_OFF(x)   ((long) (x))

#ifndef O_BINARY
#define O_BINARY 0
#endif

/* PC I/O-port pokes have no meaning on the PS2; swallow them. The portable
   engine keeps real video behind the driver, so these only catch stragglers. */
#define outp(x, y)   do { (void)(x); (void)(y); } while (0)
#define koutp(x, y)  do { (void)(x); (void)(y); } while (0)
#define outpw(x, y)  do { (void)(x); (void)(y); } while (0)
#define koutpw(x, y) do { (void)(x); (void)(y); } while (0)
#define outb(x, y)   do { (void)(x); (void)(y); } while (0)
#define koutb(x, y)  do { (void)(x); (void)(y); } while (0)
#define inp(x)       (0)
#define kinp(x)      (0)

/* ANSI strips these from the namespace; BUILD expects them. */
int stricmp(const char *x, const char *y);
long filelength(int fhandle);

#define printext16 printext256
#define printext16_noupdate printext256_noupdate

#ifndef max
#define max(x, y)  (((x) > (y)) ? (x) : (y))
#endif
#ifndef min
#define min(x, y)  (((x) < (y)) ? (x) : (y))
#endif

#endif  /* !defined _INCLUDE_PS2_COMPAT_H_ */

/* end of ps2_compat.h ... */
