/*
 * ps2_display.c -- PlayStation 2 video for Chocolate Duke3D.
 *
 * Replaces Engine/src/display.c (the SDL driver). Implements display.h's
 * contract with gsKit: the engine renders into the 8-bit `frameplace`
 * framebuffer, which we present each frame as a GS PSMT8 texture + CT32 CLUT
 * (hardware indexed->RGB), vsync-flipped. Mirrors ps2oom's proven backend.
 *
 * Driver-owned globals only (frameplace/screen/xres/...); the engine owns
 * xdim/ydim/ylookup/bytesperline/qsetmode. See PORTING.md.
 */

#ifdef PLATFORM_PS2

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <unistd.h>
#include <time.h>
#include <gsKit.h>
#include <dmaKit.h>

#include "platform.h"
#include "build.h"
#include "display.h"

/* ---- driver-owned globals (declared extern in display.h) ---------------- */
uint8_t *frameplace = NULL, *frameoffset = NULL, *screen = NULL;
int32_t  bytesperline = 0;             /* display.c owned this; we replace it  */
uint8_t  lastPalette[768];             /* display.c's palette scratch buffer    */
int32_t  xres = 0, yres = 0, imageSize = 0, maxpages = 1;
int32_t  buffermode = 0, origbuffermode = 0, linearmode = 1;
uint8_t  permanentupdate = 0, vgacompatible = 1, vesachecked = 1;
int32_t  pageoffset = 0, ydim16 = 0;
int32_t  visualpage = 0, activepage = 0;
/* moustat, curbrightness, stereomode, whiteband, blackband: owned by engine.c */

/* ---- gsKit state (ps2oom PSMT8 backend) --------------------------------- */
static GSGLOBAL *gsGlobal = NULL;
static GSTEXTURE fbtex;
static unsigned char ps2pal[256][3];   /* live palette, R,G,B 0..255 */
static int video_ready = 0;

void _handle_events(void);             /* in ps2_input.c */

static void ps2_video_init(int32_t w, int32_t h)
{
    if (gsGlobal) return;

    gsGlobal = gsKit_init_global();
    gsGlobal->Mode       = GS_MODE_NTSC;
    gsGlobal->Interlace  = GS_INTERLACED;
    gsGlobal->Field      = GS_FIELD;
    gsGlobal->Width      = 640;
    gsGlobal->Height     = 448;
    gsGlobal->PSM        = GS_PSM_CT24;
    gsGlobal->PSMZ       = GS_PSMZ_16S;
    gsGlobal->ZBuffering = GS_SETTING_OFF;
    gsGlobal->DoubleBuffering = GS_SETTING_ON;

    dmaKit_init(D_CTRL_RELE_OFF, D_CTRL_MFD_OFF, D_CTRL_STS_UNSPEC,
                D_CTRL_STD_OFF, D_CTRL_RCYC_8, 1 << DMA_CHANNEL_GIF);
    dmaKit_chan_init(DMA_CHANNEL_GIF);

    gsKit_init_screen(gsGlobal);
    gsKit_mode_switch(gsGlobal, GS_ONESHOT);
    gsKit_TexManager_init(gsGlobal);

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

/* ---- mode set: alloc framebuffer + engine view setup -------------------- */

int32_t _setgamemode(uint8_t davidoption, int32_t daxdim, int32_t daydim)
{
    extern int32_t xdim, ydim, ylookup[], qsetmode, horizycent;
    extern int32_t oxdimen, oviewingrange, oxyaspect;
    extern void setview(int32_t, int32_t, int32_t, int32_t);
    extern void clearallviews(int32_t);
    int32_t i, j;

    if (screen) { free(screen); screen = NULL; }
    xdim = xres = daxdim;
    ydim = yres = daydim;
    bytesperline = daxdim;
    imageSize = bytesperline * daydim;
    screen = (uint8_t *) malloc(imageSize);
    if (screen) memset(screen, 0, imageSize);
    frameoffset = frameplace = screen;

    ps2_video_init(daxdim, daydim);

    for (i = 0, j = 0; i <= ydim; i++) { ylookup[i] = j; j += bytesperline; }
    horizycent = (ydim * 4) >> 1;
    oxyaspect = oxdimen = oviewingrange = -1;
    setview(0, 0, xdim - 1, ydim - 1);
    clearallviews(0);

    qsetmode = 200;
    return 0;
}

void setvmode(int mode) { (void) mode; }

/* BUILD validates the requested mode against this list; offer 320x200. */
void getvalidvesamodes(void)
{
    extern int32_t validmodecnt, validmodexdim[], validmodeydim[];
    validmodecnt = 1;
    validmodexdim[0] = 320;
    validmodeydim[0] = 200;
}

void _uninitengine(void)
{
    if (screen) { free(screen); screen = NULL; }
    frameplace = frameoffset = NULL;
}

void *_getVideoBase(void) { return (void *) screen; }

/* ---- present ------------------------------------------------------------ */

void _nextpage(void)
{
    u32 *clut;
    int i;

    _handle_events();

    if (!video_ready) return;

    /* CT32 CLUT from the live palette, CSM1-swizzled (bits 3/4 swapped). */
    clut = fbtex.Clut;
    for (i = 0; i < 256; i++)
    {
        int j = (i & ~0x18) | ((i & 0x08) << 1) | ((i & 0x10) >> 1);
        clut[j] = (u32) ps2pal[i][0]
                | ((u32) ps2pal[i][1] << 8)
                | ((u32) ps2pal[i][2] << 16)
                | (0x80u << 24);
    }

    if (screen)
        memcpy(fbtex.Mem, screen, (size_t) imageSize);

    gsKit_clear(gsGlobal, GS_SETREG_RGBAQ(0x00, 0x00, 0x00, 0x00, 0x00));
    gsKit_TexManager_invalidate(gsGlobal, &fbtex);
    gsKit_TexManager_bind(gsGlobal, &fbtex);
    gsKit_prim_sprite_texture(gsGlobal, &fbtex,
        0.0f, 0.0f, 0.0f, 0.0f,
        (float) gsGlobal->Width, (float) gsGlobal->Height,
        (float) fbtex.Width, (float) fbtex.Height,
        0, GS_SETREG_RGBAQ(0x80, 0x80, 0x80, 0x80, 0x00));
    gsKit_queue_exec(gsGlobal);
    gsKit_sync_flip(gsGlobal);
    gsKit_TexManager_nextFrame(gsGlobal);
}

void _updateScreenRect(int32_t x, int32_t y, int32_t w, int32_t h)
{ (void) x; (void) y; (void) w; (void) h; }

void clear2dscreen(void) { if (screen) memset(screen, 0, imageSize); }

/* ---- palette ------------------------------------------------------------ */

int VBE_setPalette(uint8_t *palettebuffer)
{
    uint8_t *p = palettebuffer;
    int i;
    for (i = 0; i < 256; i++)
    {
        unsigned b = *p++, g = *p++, r = *p++;
        p++;                            /* reserved */
        ps2pal[i][0] = (unsigned char) ((r << 2) | (r >> 4));   /* 0..63 -> 0..255 */
        ps2pal[i][1] = (unsigned char) ((g << 2) | (g >> 4));
        ps2pal[i][2] = (unsigned char) ((b << 2) | (b >> 4));
    }
    return 0;
}

int VBE_getPalette(int32_t start, int32_t num, uint8_t *dapal)
{
    int i;
    for (i = 0; i < num; i++)
    {
        *dapal++ = (uint8_t) (ps2pal[start + i][2] >> 2);
        *dapal++ = (uint8_t) (ps2pal[start + i][1] >> 2);
        *dapal++ = (uint8_t) (ps2pal[start + i][0] >> 2);
        *dapal++ = 0;
    }
    return 0;
}

/* ---- 8-bit pixel / 2D helpers ------------------------------------------- */

static uint8_t drawpixel_color = 0;

uint8_t readpixel(uint8_t *location) { return *location; }
void drawpixel(uint8_t *location, uint8_t pixel) { *location = pixel; }
void setcolor16(uint8_t color) { drawpixel_color = color; }
void drawpixel16(int32_t offset) { if (screen) screen[offset] = drawpixel_color; }

void drawline16(int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint8_t col)
{
    extern int32_t bytesperline;
    int32_t dx = x2 - x1, dy = y2 - y1, sx, sy, err, e2;
    if (!screen) return;
    sx = (dx < 0) ? -1 : 1; if (dx < 0) dx = -dx;
    sy = (dy < 0) ? -1 : 1; if (dy < 0) dy = -dy;
    err = (dx > dy ? dx : -dy) / 2;
    for (;;)
    {
        if ((uint32_t) x1 < (uint32_t) bytesperline && y1 >= 0 && y1 < yres)
            screen[y1 * bytesperline + x1] = col;
        if (x1 == x2 && y1 == y2) break;
        e2 = err;
        if (e2 > -dx) { err -= dy; x1 += sx; }
        if (e2 <  dy) { err += dx; y1 += sy; }
    }
}

void fillscreen16(int32_t a, int32_t b, int32_t c)
{ (void) a; (void) b; (void) c; if (screen) memset(screen, 0, imageSize); }

/* ---- timing ------------------------------------------------------------- */

uint32_t getticks(void)
{
    /* newlib clock() is wired to the EE kernel timer on ps2sdk. */
    return (uint32_t) (((unsigned long long) clock() * 1000ULL) / CLOCKS_PER_SEC);
}

/* The game polls sampletimer() to advance BUILD's 120 Hz clock. */
void sampletimer(void)
{
    extern volatile int32_t totalclock;
    totalclock = (int32_t) ((getticks() * 120ULL) / 1000ULL);
}

int screencapture(char *filename, uint8_t inverseit)
{
    (void) filename; (void) inverseit;
    return 0;                  /* TODO: GS framebuffer -> TGA later */
}

/* DOS-ism the filesystem expects (file size of an open fd). */
int32_t filelength(int32_t fd)
{
    extern int ps2_blseek(int, int, int);
    int cur = ps2_blseek(fd, 0, SEEK_CUR);
    int end = ps2_blseek(fd, 0, SEEK_END);
    ps2_blseek(fd, cur, SEEK_SET);
    return end;
}

#endif /* PLATFORM_PS2 */
