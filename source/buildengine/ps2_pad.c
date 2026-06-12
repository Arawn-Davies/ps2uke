/*
 * PS2 controller input via libpad (mirrors ps2oom's pad path). SIO2MAN/PADMAN
 * are ROM IOP modules loaded by path -- needs the SIF/IOP brought up first
 * (ps2_iop_init in ps2_main). btns is active-low (0 bit == pressed); we return
 * it active-high so callers read "bit set == pressed".
 */

#ifdef PLATFORM_PS2

#include <tamtypes.h>
#include <loadfile.h>      /* SifLoadModule */
#include <libpad.h>

#include "ps2_pad.h"

static char padbuf[256] __attribute__((aligned(64)));
static int  inited = 0;

void ps2pad_init(void)
{
    if (inited)
        return;

    SifLoadModule("rom0:SIO2MAN", 0, NULL);
    SifLoadModule("rom0:PADMAN", 0, NULL);
    padInit(0);
    padPortOpen(0, 0, padbuf);
    inited = 1;
}

unsigned ps2pad_btns(void)
{
    struct padButtonStatus btn;
    int s;

    if (!inited)
        ps2pad_init();

    s = padGetState(0, 0);
    if (s != PAD_STATE_STABLE && s != PAD_STATE_FINDCTP1)
        return 0;                        /* no controller / still detecting */

    if (padRead(0, 0, &btn) == 0)
        return 0;

    return ((unsigned) ~btn.btns) & 0xffff;   /* active-low -> active-high */
}

#endif /* PLATFORM_PS2 */
