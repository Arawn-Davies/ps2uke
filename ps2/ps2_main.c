/*
 * ps2uke link/boot smoke-test.
 *
 * STAGE 2/3 milestone: prove the portable BUILD engine + ps2_driver.c link into
 * a PS2 ELF under the ps2dev toolchain. This is NOT the game yet -- it exercises
 * the engine entry points so the linker pulls the whole engine in. The Duke game
 * loop (the Duke source) is wired in a later stage; see PORTING.md.
 */

#include <stdio.h>

/* BUILD engine entry points (engine.c). */
extern int  initengine(void);
extern int  setgamemode(char davidoption, long daxdim, long daydim);
extern void nextpage(void);
extern void uninitengine(void);

int main(int argc, char **argv)
{
    (void) argc;
    (void) argv;

    printf("ps2uke: BUILD engine PS2 link smoke-test\n");

    initengine();
    setgamemode(0, 320, 200);
    nextpage();
    uninitengine();

    printf("ps2uke: engine init/teardown returned cleanly\n");
    return 0;
}
