/*
 * ps2_reboot.c -- "quit" handler for the game ELF: drop back to the launcher.
 *
 * There is no "DOS" to exit to on a console, and the engine's full EE/IOP
 * teardown (audsrv + cdfs) deadlocks on the way out ("callnull" spam). Instead,
 * following ps2quake's Sys_Quit, we reset the IOP cleanly and LoadExecPS2() the
 * boot launcher (LAUNCH.ELF) -- which re-shows the options picker. The launcher
 * then chain-loads the game again. LoadExecPS2 does not return.
 */

#if defined(_EE) || defined(__PS2__) || defined(PLATFORM_PS2)

#define NEWLIB_PORT_AWARE       /* allow <fileio.h> (fioExit) in a newlib TU */

#include <kernel.h>         /* LoadExecPS2, FlushCache        */
#include <sifrpc.h>         /* SifInitRpc/SifExitRpc/SifExitCmd */
#include <iopcontrol.h>     /* SifIopReset/SifIopSync         */
#include <iopheap.h>        /* SifExitIopHeap                 */
#include <loadfile.h>       /* SifLoadFileExit                */
#include <fileio.h>         /* fioExit                        */

#include "ps2_settings.h"

/* Reset the IOP to a clean ROM state -- verbatim from ps2quake's IOP_reset.
 * The crucial bit my first attempt got wrong: after tearing the SIF services
 * down, RPC *and* the load-file service must be brought back up, or the
 * following LoadExecPS2 has no working RPC to load the next ELF (-> black
 * screen). The double reset is ps2quake's hd-loader hack. */
static void iop_reset(void)
{
    SifInitRpc(0);
    while (!SifIopReset("rom0:UDNL rom0:EELOADCNF", 0)) ;
    while (!SifIopSync()) ;
    fioExit();
    SifExitIopHeap();
    SifLoadFileExit();
    SifExitRpc();
    SifExitCmd();
    SifInitRpc(0);
    FlushCache(0);
    FlushCache(2);

    while (!SifIopReset("rom0:UDNL rom0:EELOADCNF", 0)) ;
    while (!SifIopSync()) ;
    fioExit();
    SifExitIopHeap();
    SifLoadFileExit();
    SifExitRpc();
    SifExitCmd();
    SifInitRpc(0);
    FlushCache(0);
    FlushCache(2);

    SifLoadFileInit();
}

void ps2_reboot(void)
{
    iop_reset();

    LoadExecPS2("cdrom0:\\LAUNCH.ELF;1", 0, NULL);

    /* Should never get here; if the re-exec failed, terminate cleanly. */
    __asm__ __volatile__("li $3, 0x04; syscall; nop;");
}

#endif /* PS2 */
