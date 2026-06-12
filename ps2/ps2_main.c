/*
 * ps2uke stage-4 framebuffer test.
 *
 * Brings up the PS2 video path WITHOUT any Duke data: it drives ps2_driver.c
 * directly (no engine.initengine / no PALETTE.DAT / no ART), fills the engine's
 * 8-bit `screen` buffer with a colour-bar + gradient test pattern, installs a
 * synthetic 256-entry palette, and presents it through gsKit every frame.
 *
 * If the pipeline (EE 8-bit buffer -> CT32 CLUT -> GS T8 texture -> display)
 * works, you see vertical colour bars over a vertical brightness gradient.
 *
 * The engine entry points are still referenced (below) so the whole BUILD
 * engine continues to link; the Duke game loop is wired in a later stage.
 */

#include <stdio.h>

/* Driver-owned globals + entry points (see buildengine/display.h). */
extern char *screen;
extern long  xres, yres, bytesperline;
extern int   _setgamemode(char davidoption, long daxdim, long daydim);
extern int   VBE_setPalette(long start, long num, char *palettebuffer);
extern void  _nextpage(void);

/* Keep the engine in the link (proves it still builds); not executed. */
extern int  initengine(void);
extern void uninitengine(void);
static void *engine_link_anchor[] = {
    (void *) initengine, (void *) uninitengine
};

static void make_test_palette(void)
{
    /* 256 entries, BUILD palette format: B,G,R,pad, channels 0..63.
       Build a 16x16 hue/!value ramp so every index is a distinct colour. */
    unsigned char pal[256 * 4];
    int i;
    for (i = 0; i < 256; i++)
    {
        int hi = i >> 4;            /* 0..15 -> colour family */
        int lo = i & 15;            /* 0..15 -> brightness    */
        int r = (hi & 4) ? (lo * 4) : (hi * 4);
        int g = (hi & 2) ? (lo * 4) : (lo * 2);
        int b = (hi & 1) ? (lo * 4) : ((15 - lo) * 4);
        pal[i * 4 + 0] = (unsigned char)(b & 63);
        pal[i * 4 + 1] = (unsigned char)(g & 63);
        pal[i * 4 + 2] = (unsigned char)(r & 63);
        pal[i * 4 + 3] = 0;
    }
    VBE_setPalette(0, 256, (char *) pal);
}

static void draw_test_pattern(void)
{
    int x, y;
    if (!screen) return;
    for (y = 0; y < yres; y++)
    {
        unsigned char *row = (unsigned char *) screen + y * bytesperline;
        int band = (y * 16) / yres;          /* 0..15 vertical brightness    */
        for (x = 0; x < bytesperline; x++)
        {
            int bar = (x * 16) / bytesperline;  /* 0..15 vertical colour bars */
            row[x] = (unsigned char) ((bar << 4) | band);
        }
    }
}

int main(int argc, char **argv)
{
    (void) argc;
    (void) argv;
    (void) engine_link_anchor;

    printf("ps2uke: stage-4 framebuffer test (no Duke data needed)\n");

    _setgamemode(0, 320, 200);   /* alloc `screen`, bring up gsKit */
    make_test_palette();
    draw_test_pattern();

    for (;;)
        _nextpage();             /* present the test pattern, vsync-locked */

    return 0;
}
