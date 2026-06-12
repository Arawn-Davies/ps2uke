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
#include <malloc.h>           /* memalign (GS DMA wants 128-byte-aligned data) */
#include <gsKit.h>
#include <dmaKit.h>
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

void faketimerhandler(void);   /* defined below; called from _nextpage */

/* ---- gsKit state (mirrors ps2oom's proven PSMT8 backend) ---------------- */
static GSGLOBAL *gsGlobal = NULL;
static GSTEXTURE fbtex;                  /* T8 texture; Mem is an aligned copy */
static unsigned char ps2pal[256][3];     /* live palette (R,G,B 0..255)        */
static int video_ready = 0;

static void ps2_video_init(long w, long h)
{
    if (gsGlobal) return;

    gsGlobal = gsKit_init_global();
    gsGlobal->Mode      = GS_MODE_NTSC;   /* deterministic, region-independent */
    gsGlobal->Interlace = GS_INTERLACED;
    gsGlobal->Field     = GS_FIELD;
    gsGlobal->Width     = 640;
    gsGlobal->Height    = 448;
    gsGlobal->PSM            = GS_PSM_CT24;
    gsGlobal->PSMZ           = GS_PSMZ_16S;
    gsGlobal->ZBuffering     = GS_SETTING_OFF;
    gsGlobal->DoubleBuffering = GS_SETTING_ON;

    dmaKit_init(D_CTRL_RELE_OFF, D_CTRL_MFD_OFF, D_CTRL_STS_UNSPEC,
                D_CTRL_STD_OFF, D_CTRL_RCYC_8, 1 << DMA_CHANNEL_GIF);
    dmaKit_chan_init(DMA_CHANNEL_GIF);

    gsKit_init_screen(gsGlobal);
    gsKit_mode_switch(gsGlobal, GS_ONESHOT);   /* queue + flip every frame */
    gsKit_TexManager_init(gsGlobal);

    /* 8-bit indexed texture + 256-entry CT32 CLUT. Mem is a 128-aligned upload
       buffer we memcpy `screen` into each frame (screen is malloc'd, unaligned;
       the GS DMA wants aligned source data). TexManager owns the VRAM. */
    fbtex.Width    = w;
    fbtex.Height   = h;
    fbtex.PSM      = GS_PSM_T8;
    fbtex.ClutPSM  = GS_PSM_CT32;
    fbtex.Filter   = GS_FILTER_NEAREST;
    fbtex.Delayed  = 0;
    fbtex.Vram     = 0;
    fbtex.VramClut = 0;
    fbtex.Mem  = (u32 *) memalign(128, gsKit_texture_size_ee(w, h, GS_PSM_T8));
    fbtex.Clut = (u32 *) memalign(128, gsKit_texture_size_ee(256, 1, GS_PSM_CT32));

    video_ready = 1;
}

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
    ps2_video_init(daxdim, daydim);
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
    u32 *clut;
    int i;

    if (!video_ready) return;

    /* Rebuild the CT32 CLUT from the live palette every frame. The GS stores a
       256-entry CSM1 CLUT swizzled: palette-index bits 3 and 4 are swapped in
       VRAM, and gsKit doesn't fix it -- so write each colour to its swizzled
       slot, else grays/browns tint green. RGBA32 = R|G<<8|B<<16|A<<24, A=0x80. */
    clut = fbtex.Clut;
    for (i = 0; i < 256; i++)
    {
        int j = (i & ~0x18) | ((i & 0x08) << 1) | ((i & 0x10) >> 1);
        clut[j] = (u32) ps2pal[i][0]
                | ((u32) ps2pal[i][1] << 8)
                | ((u32) ps2pal[i][2] << 16)
                | (0x80u << 24);
    }

    /* Copy the engine's 8-bit frame into the aligned upload buffer. */
    if (screen)
        memcpy(fbtex.Mem, screen, (size_t) imageSize);

    gsKit_clear(gsGlobal, GS_SETREG_RGBAQ(0x00, 0x00, 0x00, 0x00, 0x00));

    gsKit_TexManager_invalidate(gsGlobal, &fbtex);   /* frame changed; re-upload */
    gsKit_TexManager_bind(gsGlobal, &fbtex);
    gsKit_prim_sprite_texture(gsGlobal, &fbtex,
        0.0f, 0.0f,                                            /* screen x1,y1 */
        0.0f, 0.0f,                                            /* tex u1,v1    */
        (float) gsGlobal->Width, (float) gsGlobal->Height,    /* x2,y2 (fill) */
        (float) fbtex.Width, (float) fbtex.Height,            /* u2,v2        */
        0,                                                     /* z            */
        GS_SETREG_RGBAQ(0x80, 0x80, 0x80, 0x80, 0x00));       /* 1.0 modulate */

    gsKit_queue_exec(gsGlobal);
    gsKit_sync_flip(gsGlobal);
    gsKit_TexManager_nextFrame(gsGlobal);

    faketimerhandler();   /* keep BUILD's frame/timer bookkeeping advancing */
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
        int idx = (int) (start + i);
        /* sdl_driver.c hands the palette over as B,G,R,pad per entry. Scale
           BUILD's 0..63 channels to 0..255; _nextpage builds the GS CLUT. */
        unsigned int b = *p++;
        unsigned int g = *p++;
        unsigned int r = *p++;
        p++;                            /* pad */
        ps2pal[idx][0] = (unsigned char) ((r << 2) | (r >> 4));
        ps2pal[idx][1] = (unsigned char) ((g << 2) | (g >> 4));
        ps2pal[idx][2] = (unsigned char) ((b << 2) | (b >> 4));
    }
    return 0;
}

int VBE_getPalette(long start, long num, char *dapal)
{
    long i;
    unsigned char *p = (unsigned char *) dapal;
    for (i = 0; i < num; i++)
    {
        *p++ = (unsigned char) (ps2pal[start + i][2] >> 2);   /* back to 0..63 */
        *p++ = (unsigned char) (ps2pal[start + i][1] >> 2);
        *p++ = (unsigned char) (ps2pal[start + i][0] >> 2);
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
