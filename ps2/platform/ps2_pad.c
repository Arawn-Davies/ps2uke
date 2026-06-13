/*
 * PS2 DualShock input via libpad, modelled on ps2quake's pad.c / in_ps2.c.
 *
 * SIO2MAN/PADMAN are ROM IOP modules; the SIF/IOP must be up first (libSDL2main
 * resets the IOP before main()). padGetState/padRead never block, so this is
 * hang-proof: we deliberately do NOT busy-wait for the pad at init (that bit us
 * with mtapInit) -- ps2pad_btns() just returns 0 until the pad reaches a
 * readable state, retried each frame. btns is active-low; we return it
 * active-high so callers read "bit set == pressed". The BUILD-key mapping for
 * the menu/game lives in sdlayer2.c's handleevents() (see ps2_pad_inject()).
 */

#if defined(_EE) || defined(__PS2__) || defined(PLATFORM_PS2)

#include <tamtypes.h>
#include <loadfile.h>      /* SifLoadModule */
#include <libpad.h>

#include "ps2_pad.h"

static char padbuf[256] __attribute__((aligned(64)));
static int  inited = 0;        /* modules loaded + port opened */
static int  analog_locked = 0; /* DualShock (analog) mode requested once stable */

/* Last analog stick reading (0..255, 0x80 = centred), refreshed by ps2pad_btns().
   Default to centred so a digital pad / not-yet-ready state reads as no input. */
static unsigned char stick_lh = 0x80, stick_lv = 0x80, stick_rh = 0x80, stick_rv = 0x80;

void ps2pad_init(void)
{
    if (inited)
        return;

    SifLoadModule("rom0:SIO2MAN", 0, NULL);
    SifLoadModule("rom0:PADMAN", 0, NULL);

    padInit(0);
    if (padPortOpen(0, 0, padbuf) == 0)
        return;               /* not ready -- retried on the next ps2pad_btns() */

    inited = 1;
}

unsigned ps2pad_btns(void)
{
    struct padButtonStatus btn;
    int s;

    if (!inited) {
        ps2pad_init();
        if (!inited)
            return 0;
    }

    s = padGetState(0, 0);
    if (s != PAD_STATE_STABLE && s != PAD_STATE_FINDCTP1)
        return 0;             /* no controller yet / still detecting */

    /* Once the pad is first stable, lock it into DualShock (analog) mode if it
       supports it, so the sticks report -- mirrors ps2quake's Setup_Pad(). This
       is fire-and-forget; digital buttons work regardless. */
    if (!analog_locked) {
        int modes = padInfoMode(0, 0, PAD_MODETABLE, -1), i;
        for (i = 0; i < modes; i++) {
            if (padInfoMode(0, 0, PAD_MODETABLE, i) == PAD_TYPE_DUALSHOCK) {
                padSetMainMode(0, 0, PAD_MMODE_DUALSHOCK, PAD_MMODE_LOCK);
                break;
            }
        }
        analog_locked = 1;
    }

    if (padRead(0, 0, &btn) == 0)
        return 0;

    /* Stash the analog sticks (valid in DualShock mode; 0x80 when digital). */
    stick_lh = btn.ljoy_h; stick_lv = btn.ljoy_v;
    stick_rh = btn.rjoy_h; stick_rv = btn.rjoy_v;

    return ((unsigned) ~btn.btns) & 0xffff;   /* active-low -> active-high */
}

/* Latest analog stick positions (0..255, 0x80 = centred). Call ps2pad_btns()
   first each frame to refresh them. */
void ps2pad_sticks(int *lh, int *lv, int *rh, int *rv)
{
    if (lh) *lh = stick_lh;
    if (lv) *lv = stick_lv;
    if (rh) *rh = stick_rh;
    if (rv) *rv = stick_rv;
}

#endif /* PS2 */
