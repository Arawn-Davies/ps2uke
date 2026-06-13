/*
 * ps2_gs.c -- gsKit PSMT8 + CT32 CLUT present for jfbuild's PS2 video seam.
 *
 * The engine renders its 8-bit indexed frame into `frame`; we upload that as a
 * GS PSMT8 texture with a hardware CLUT built from the engine's curpalettefaded,
 * so the GS does the indexed->RGB expansion AND the 320x200 -> 640x448 upscale.
 * This replaces SDL2's renderer on PS2, which made the EE expand 8->32 and push
 * a 256 KB RGBA upload every single frame (slow even on the logo/menu).
 *
 * gsKit is ours alone: SDL2's PS2 port only ever calls gsKit_init_global from
 * its *renderer* (SDL_render_ps2.c) -- which sdlayer2.c now skips on PS2 -- so
 * nothing else owns the GS. Derived from ps2oom / the Chocolate-era
 * ps2_display.c backend.
 */

#if defined(_EE) || defined(__PS2__) || defined(PLATFORM_PS2)

#include <malloc.h>
#include <string.h>
#include <gsKit.h>
#include <dmaKit.h>

#include "ps2_settings.h"   /* ps2cfg: video standard / filter / frame cap */

/* jfbuild's faded palette is palette_t curpalettefaded[256] = { r, g, b, f }. */
extern unsigned char curpalettefaded[256][4];

static GSGLOBAL *gsGlobal = NULL;
static GSTEXTURE fbtex;
static int       fb_w = 0, fb_h = 0;
static int       fb_double = 1;        /* is the framebuffer double-buffered? */

void ps2gs_init(int w, int h)
{
    if (gsGlobal) return;                  /* once */

    gsGlobal = gsKit_init_global();

    /* Video mode from the boot picker, in PS2_VIDEO_* order. The GS has only
       4 MB of VRAM; a double-buffered CT24 framebuffer is fine up to 576p
       (640x512x4x2 ~= 2.6 MB), but 720p must drop to 16-bit to fit alongside
       our T8 texture. Progressive (480p/576p/720p) needs component/VGA output
       on real hardware; interlaced (480i/576i) works over composite. */
    static const struct {
        int mode, interlace, field, width, height, psm, dbuf;
    } modes[PS2_VIDEO_COUNT] = {
        { GS_MODE_NTSC,      GS_INTERLACED,    GS_FIELD,  640,  448, GS_PSM_CT24, GS_SETTING_ON },
        { GS_MODE_DTV_480P,  GS_NONINTERLACED, GS_FRAME,  640,  480, GS_PSM_CT24, GS_SETTING_ON },
        { GS_MODE_PAL,       GS_INTERLACED,    GS_FIELD,  640,  512, GS_PSM_CT24, GS_SETTING_ON },
        { GS_MODE_DTV_576P,  GS_NONINTERLACED, GS_FRAME,  640,  512, GS_PSM_CT24, GS_SETTING_ON },
        { GS_MODE_DTV_720P,  GS_NONINTERLACED, GS_FRAME, 1280,  720, GS_PSM_CT16, GS_SETTING_ON },
    };
    int v = ps2cfg.video;
    if (v < 0 || v >= PS2_VIDEO_COUNT) v = PS2_VIDEO_NTSC_480I;

    gsGlobal->Mode            = modes[v].mode;
    gsGlobal->Interlace       = modes[v].interlace;
    gsGlobal->Field           = modes[v].field;
    gsGlobal->Width           = modes[v].width;
    gsGlobal->Height          = modes[v].height;
    gsGlobal->PSM             = modes[v].psm;
    gsGlobal->PSMZ            = GS_PSMZ_16S;
    gsGlobal->ZBuffering      = GS_SETTING_OFF;
    gsGlobal->DoubleBuffering = modes[v].dbuf;
    fb_double                 = (modes[v].dbuf == GS_SETTING_ON);

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
    fbtex.Filter   = (ps2cfg.filter == PS2_FILTER_SMOOTH)
                   ? GS_FILTER_LINEAR : GS_FILTER_NEAREST;
    fbtex.Delayed  = 0;
    fbtex.Vram     = 0;
    fbtex.VramClut = 0;
    fbtex.Mem  = (u32 *) memalign(128, gsKit_texture_size_ee(w, h, GS_PSM_T8));
    fbtex.Clut = (u32 *) memalign(128, gsKit_texture_size_ee(256, 1, GS_PSM_CT32));

    fb_w = w;
    fb_h = h;
}

void ps2gs_present(const void *frame8, int pitch)
{
    u32 *clut;
    int  i, y;

    if (!gsGlobal || !fbtex.Mem) return;

    /* CT32 CLUT from the live engine palette, CSM1-swizzled (bits 3/4 swapped). */
    clut = fbtex.Clut;
    for (i = 0; i < 256; i++) {
        int j = (i & ~0x18) | ((i & 0x08) << 1) | ((i & 0x10) >> 1);
        clut[j] = (u32) curpalettefaded[i][0]
                | ((u32) curpalettefaded[i][1] << 8)
                | ((u32) curpalettefaded[i][2] << 16)
                | (0x80u << 24);
    }

    /* Copy the engine's 8-bit frame into the T8 texture. The frame pitch may be
       wider than the visible width (jfbuild rounds it up), so go row by row
       unless they match exactly. */
    if (pitch == fb_w) {
        memcpy(fbtex.Mem, frame8, (size_t) fb_w * fb_h);
    } else {
        const unsigned char *src = (const unsigned char *) frame8;
        unsigned char       *dst = (unsigned char *) fbtex.Mem;
        for (y = 0; y < fb_h; y++)
            memcpy(dst + (size_t) y * fb_w, src + (size_t) y * pitch, (size_t) fb_w);
    }

    gsKit_clear(gsGlobal, GS_SETREG_RGBAQ(0, 0, 0, 0, 0));
    gsKit_TexManager_invalidate(gsGlobal, &fbtex);
    gsKit_TexManager_bind(gsGlobal, &fbtex);
    gsKit_prim_sprite_texture(gsGlobal, &fbtex,
        0.0f, 0.0f,                                           /* x1, y1 */
        0.0f, 0.0f,                                           /* u1, v1 */
        (float) gsGlobal->Width, (float) gsGlobal->Height,    /* x2, y2 (scaled) */
        (float) fbtex.Width, (float) fbtex.Height,            /* u2, v2 */
        0, GS_SETREG_RGBAQ(0x80, 0x80, 0x80, 0x80, 0x00));
    gsKit_queue_exec(gsGlobal);
    if (fb_double)
        gsKit_sync_flip(gsGlobal);      /* vsync + flip between the two buffers */
    else
        gsKit_vsync_wait();             /* single buffer (1080i): just pace, no flip */
    if (ps2cfg.cap == PS2_CAP_30)
        gsKit_vsync_wait();             /* one more field -> ~30 fps cap */
    gsKit_TexManager_nextFrame(gsGlobal);
}

#endif /* PS2 */
