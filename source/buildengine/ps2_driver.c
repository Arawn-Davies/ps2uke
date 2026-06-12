/*
 * PlayStation 2 platform driver for the BUILD engine.
 *
 * This is the PS2 analog of sdl_driver.c / dos_drvr.c: it owns the 8-bit
 * palettized framebuffer the engine renders into (`screen`) and the small set
 * of entry points the engine calls to set a video mode, set the palette, and
 * present a frame. See display.h for the contract and PORTING.md for the plan.
 *
 * STAGE 3 (this file): a correct, linkable software framebuffer in EE RAM, with
 * presentation/palette/input stubbed. STAGE 4+ swaps the stubs for real gsKit
 * (CLUT-expand `screen` -> GS texture, vsync flip) and PS2 pad reads.
 *
 * "Build Engine & Tools" Copyright (c) 1993-1997 Ken Silverman.
 * See BUILDLIC.TXT. This file is not part of Ken's original release.
 */

#ifdef PLATFORM_PS2

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "platform.h"
#include "build.h"
#include "display.h"

/* ---- driver-owned globals (declared extern in display.h) ---------------- */
char *screen = NULL;
long xres = 0, yres = 0;
long bytesperline = 0;
long imageSize = 0;
long frameplace = 0;
long buffermode = 0;
long origbuffermode = 0;
char permanentupdate = 0;

/* 256-entry BUILD palette (R,G,B, 0..63 each), kept for the gsKit CLUT later. */
static unsigned char ps2pal[256][3];

/* ---- framebuffer lifecycle ---------------------------------------------- */

static void ps2_alloc_framebuffer(long w, long h)
{
    if (screen) { free(screen); screen = NULL; }
    xres = w;
    yres = h;
    bytesperline = w;
    imageSize = bytesperline * h;
    screen = (char *) malloc(imageSize);
    if (screen) memset(screen, 0, imageSize);
    frameplace = (long) screen;
}

int _setgamemode(char davidoption, long daxdim, long daydim)
{
    extern long qsetmode;
    ps2_alloc_framebuffer(daxdim, daydim);
    qsetmode = 200;           /* matches BUILD's "in 3D game mode" sentinel */
    return 0;
}

void setvmode(int mode)
{
    /* Text/again handled by the engine; nothing to program on PS2 here. */
    (void) mode;
}

void _uninitengine(void)
{
    if (screen) { free(screen); screen = NULL; }
    frameplace = 0;
}

void *_getVideoBase(void) { return (void *) screen; }

/* ---- presentation (STUB: gsKit blit + vsync flip lands in stage 4) ------- */

void _nextpage(void)
{
    /* TODO(stage4): CLUT-expand `screen` through ps2pal into a GS texture and
       flip on vsync. For now the frame is rendered into EE RAM and dropped. */
}

void _updateScreenRect(long x, long y, long w, long h)
{
    (void) x; (void) y; (void) w; (void) h;
}

void setactivepage(long dapagenum) { (void) dapagenum; }
void clear2dscreen(void)
{
    if (screen) memset(screen, 0, imageSize);
}

/* ---- palette (STUB: upload to GS CLUT in stage 4) ------------------------ */

int VBE_setPalette(long start, long num, char *palettebuffer)
{
    long i;
    unsigned char *p = (unsigned char *) palettebuffer;
    for (i = 0; i < num; i++)
    {
        /* sdl_driver.c stores the SDL palette as B,G,R,pad per entry. */
        ps2pal[start + i][2] = *p++;   /* B */
        ps2pal[start + i][1] = *p++;   /* G */
        ps2pal[start + i][0] = *p++;   /* R */
        p++;                            /* pad */
    }
    return 0;
}

int VBE_getPalette(long start, long num, char *dapal)
{
    long i;
    unsigned char *p = (unsigned char *) dapal;
    for (i = 0; i < num; i++)
    {
        *p++ = ps2pal[start + i][2];
        *p++ = ps2pal[start + i][1];
        *p++ = ps2pal[start + i][0];
        *p++ = 0;
    }
    return 0;
}

/* ---- pixel pokes (used by the 2D/menu paths) ---------------------------- */

unsigned char readpixel(long offset) { return *(unsigned char *)(offset); }
void drawpixel(long offset, Uint8 pixel) { *(unsigned char *)(offset) = pixel; }
void drawpixels(long offset, Uint16 pixels) { *(Uint16 *)(offset) = pixels; }
void drawpixelses(long offset, Uint32 pixelses) { *(Uint32 *)(offset) = pixelses; }

/* 16-colour 2D primitives: minimal, write straight into the 8-bit buffer. */
static long color16 = 0;
void setcolor16(int i1) { color16 = i1; }
void drawpixel16(long offset)
{
    if (screen) screen[offset] = (char) color16;
}
void drawline16(long XStart, long YStart, long XEnd, long YEnd, char Color)
{
    /* Bresenham into the 8-bit buffer; good enough for editor/menu lines. */
    long dx = XEnd - XStart, dy = YEnd - YStart, sx, sy, err, e2;
    if (!screen) return;
    sx = (dx < 0) ? -1 : 1; if (dx < 0) dx = -dx;
    sy = (dy < 0) ? -1 : 1; if (dy < 0) dy = -dy;
    err = (dx > dy ? dx : -dy) / 2;
    for (;;)
    {
        if ((unsigned long) XStart < (unsigned long) bytesperline)
            screen[YStart * bytesperline + XStart] = Color;
        if (XStart == XEnd && YStart == YEnd) break;
        e2 = err;
        if (e2 > -dx) { err -= dy; XStart += sx; }
        if (e2 <  dy) { err += dx; YStart += sy; }
    }
}

void fillscreen16(long input1, long input2, long input3)
{
    (void) input1; (void) input2; (void) input3;
    if (screen) memset(screen, 0, imageSize);
}

void limitrate(void) {}

/* ---- timer / mouse (STUB: real EE timer + PS2 pad in stage 4) ----------- */

void faketimerhandler(void) {}

unsigned long getticks(void) { return 0; }

int setupmouse(void) { return 0; }
void readmousexy(short *x, short *y) { if (x) *x = 0; if (y) *y = 0; }
void readmousebstatus(short *bstatus) { if (bstatus) *bstatus = 0; }

/* ---- DOS-ism implementations the engine/cache1d expect ------------------ */

long filelength(int fhandle)
{
    long cur = lseek(fhandle, 0, SEEK_CUR);
    long end = lseek(fhandle, 0, SEEK_END);
    lseek(fhandle, cur, SEEK_SET);
    return end;
}

int stricmp(const char *x, const char *y)
{
    return strcasecmp(x, y);
}

#endif /* PLATFORM_PS2 */
