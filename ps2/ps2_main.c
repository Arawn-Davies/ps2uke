/*
 * ps2uke stage-6 test: draw a real Duke ART tile.
 *
 * Beyond the palette test: load the tile catalog from DUKE3D.GRP and draw the
 * Duke3D TITLE screen tile with the engine's own rotatesprite -- the same call
 * Duke uses for full-screen images (see premap.c). If this shows the title
 * screen, the ART/tile pipeline works, not just the palette.
 *
 * Follows Ken's TEST.C init order: initengine -> loadpics -> setgamemode, then
 * a clearview/rotatesprite/nextpage loop. The Duke game loop comes next.
 */

#include <stdio.h>

#include <sifrpc.h>
#include <iopcontrol.h>
#include <sbv_patches.h>
#include <libpad.h>             /* PAD_LEFT/PAD_RIGHT/PAD_CROSS */
#include <ps2_cdfs_driver.h>
#include "ps2_pad.h"

/* full-screen tiles from Duke's names.h */
#define TITLE       2486
#define BETASCREEN  2493
#define LOADSCREEN  3281

/* engine + driver entry points */
extern char  palette[768];
extern long  xdim, ydim;
extern int   initengine(void);
extern long  initgroupfile(const char *filename);
extern int   loadpics(char *filename);
extern int   setgamemode(char davidoption, long daxdim, long daydim);
extern void  clearview(long dacol);
extern void  rotatesprite(long sx, long sy, long z, short a, short picnum,
                          signed char dashade, char dapalnum, char dastat,
                          long cx1, long cy1, long cx2, long cy2);
extern void  nextpage(void);
extern int   VBE_setPalette(long start, long num, char *palettebuffer);

static void ps2_iop_init(void)
{
    SifInitRpc(0);
    while (!SifIopReset("", 0)) { }
    while (!SifIopSync()) { }
    SifInitRpc(0);
    sbv_patch_enable_lmb();
    sbv_patch_disable_prefix_check();
}

static void apply_engine_palette(void)
{
    /* engine palette is R,G,B (0..63); VBE_setPalette wants B,G,R,pad. */
    unsigned char pal[256 * 4];
    int i, nonzero = 0;
    for (i = 0; i < 768; i++) if (palette[i]) { nonzero = 1; break; }
    printf("palette from GRP: %s\n", nonzero ? "LOADED" : "EMPTY");
    for (i = 0; i < 256; i++)
    {
        pal[i * 4 + 0] = (unsigned char) palette[i * 3 + 2];
        pal[i * 4 + 1] = (unsigned char) palette[i * 3 + 1];
        pal[i * 4 + 2] = (unsigned char) palette[i * 3 + 0];
        pal[i * 4 + 3] = 0;
    }
    VBE_setPalette(0, 256, (char *) pal);
}

int main(int argc, char **argv)
{
    long grp;
    int  pv;

    (void) argc; (void) argv;

    printf("ps2uke: stage-6 ART tile test\n");

    ps2_iop_init();
    printf("cdfs: init_cdfs_driver() = %d\n", (int) init_cdfs_driver());
    fflush(stdout);

    grp = initgroupfile("cdfs:/DUKE3D.GRP");
    printf("initgroupfile(cdfs:/DUKE3D.GRP) = %ld\n", grp);
    fflush(stdout);

    initengine();                          /* loads PALETTE.DAT from the GRP */
    pv = loadpics("tiles000.art");          /* loads the tile catalog         */
    printf("loadpics(tiles000.art) = %d\n", pv);
    setgamemode(0, 320, 200);               /* engine view setup + our driver */
    apply_engine_palette();

    ps2pad_init();
    printf("ps2uke: D-pad Left/Right or Cross to cycle screens\n");
    fflush(stdout);

    {
        static const short tiles[] = { TITLE, LOADSCREEN, BETASCREEN };
        const int ntiles = (int) (sizeof(tiles) / sizeof(tiles[0]));
        int      idx  = 0;
        unsigned prev = 0;

        for (;;)
        {
            unsigned b = ps2pad_btns();
            unsigned pressed = b & ~prev;          /* rising edges only */
            prev = b;

            if (pressed & (PAD_RIGHT | PAD_CROSS)) idx = (idx + 1) % ntiles;
            if (pressed & PAD_LEFT)                idx = (idx + ntiles - 1) % ntiles;

            clearview(0L);
            rotatesprite(320 << 15, 200 << 15, 65536L, 0, tiles[idx],
                         0, 0, 2 + 8 + 64, 0L, 0L, xdim - 1, ydim - 1);
            nextpage();
        }
    }

    return 0;
}
