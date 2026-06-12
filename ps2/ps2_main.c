/*
 * ps2uke stage-5 test: read the real Duke data off the disc via cdfs.
 *
 * Brings up cdfs, opens DUKE3D.GRP on the boot disc, inits the BUILD engine
 * (which loads PALETTE.DAT out of the GRP), then renders the colour-bar test
 * pattern through the engine's REAL palette. If the disc/GRP path works you see
 * the bars in Duke's actual colours -- visual proof the whole file stack
 * (cdfs.irx -> fio shim -> cache1d -> GRP -> PALETTE.DAT) is live.
 *
 * The Duke game loop itself is wired in a later stage; see PORTING.md.
 */

#include <stdio.h>

#include <ps2_cdfs_driver.h>   /* init_cdfs_driver() (libps2_drivers) */

/* Driver + engine entry points (buildengine/display.h, cache1d.c, engine.c). */
extern char *screen;
extern long  xres, yres, bytesperline;
extern char  palette[768];                 /* engine's live palette (R,G,B 0..63) */

extern int   _setgamemode(char davidoption, long daxdim, long daydim);
extern int   VBE_setPalette(long start, long num, char *palettebuffer);
extern void  _nextpage(void);

extern int   initengine(void);
extern long  initgroupfile(const char *filename);

static void apply_engine_palette(void)
{
    /* BUILD palette is R,G,B (0..63). VBE_setPalette wants B,G,R,pad per entry. */
    unsigned char pal[256 * 4];
    int i;
    for (i = 0; i < 256; i++)
    {
        pal[i * 4 + 0] = (unsigned char) palette[i * 3 + 2];   /* B */
        pal[i * 4 + 1] = (unsigned char) palette[i * 3 + 1];   /* G */
        pal[i * 4 + 2] = (unsigned char) palette[i * 3 + 0];   /* R */
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
        int band = (y * 16) / yres;
        for (x = 0; x < bytesperline; x++)
        {
            int bar = (x * 16) / bytesperline;
            row[x] = (unsigned char) ((bar << 4) | band);
        }
    }
}

int main(int argc, char **argv)
{
    long grp;

    (void) argc;
    (void) argv;

    printf("ps2uke: stage-5 cdfs/GRP test\n");

    _setgamemode(0, 320, 200);          /* alloc screen + bring up gsKit */

    printf("cdfs: init_cdfs_driver() = %d\n", (int) init_cdfs_driver());
    fflush(stdout);

    grp = initgroupfile("cdfs:/DUKE3D.GRP");
    printf("initgroupfile(cdfs:/DUKE3D.GRP) = %ld\n", grp);
    if (grp < 0)
        printf("  !! GRP not found on disc -- is it on the ISO as DUKE3D.GRP?\n");
    fflush(stdout);

    initengine();                        /* sets up tables; loads PALETTE.DAT */
    apply_engine_palette();              /* push the real Duke palette to the GS */

    draw_test_pattern();

    printf("ps2uke: presenting (colour bars in Duke's real palette)\n");
    fflush(stdout);

    for (;;)
        _nextpage();

    return 0;
}
