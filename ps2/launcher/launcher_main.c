/*
 * launcher_main.c -- ps2uke boot launcher (LAUNCH.ELF).
 *
 * A standalone ELF, booted first by SYSTEM.CNF, that shows the PS2 options
 * picker on the libdebug text screen and then chain-loads the game ELF
 * (PS2UKE.ELF) via LoadExecPS2, handing the chosen settings across in argv.
 *
 * This is the ps2quake / ps2oom launcher model. Doing the picker in its own ELF
 * (rather than inside the game) is what keeps it sane: the launcher owns the GS
 * (libdebug) and the pad exclusively, then closes the pad and hands the whole
 * machine to the game -- so there's no init_scr-vs-gsKit or double-open-pad
 * conflict with the engine (which broke the in-game attempt).
 *
 * Settings persist to a memory card when present (ps2_settings.c); the argv tag
 * makes them apply even with no card.
 */

#include <kernel.h>        /* LoadExecPS2 (noreturn), SleepThread */
#include <sifrpc.h>        /* SifInitRpc                          */
#include <loadfile.h>      /* SifLoadModule                       */
#include <libpad.h>        /* padInit/padPortOpen/padRead/...     */
#include <debug.h>         /* init_scr, scr_clear, scr_setXY, scr_printf */

#include "ps2_settings.h"

#define GAME_ELF  "cdrom0:\\PS2UKE.ELF;1"
#define MENU_TOP  3                         /* dodge the TV's top overscan */
#define NROWS     5
#define ROW_LAUNCH 4

static char pad_buf[256] __attribute__((aligned(64)));

static void busy_wait(volatile int n) { while (n-- > 0) __asm__ volatile ("nop"); }

/* Bounded wait for the pad to become readable; 0 if it never does. */
static int pad_wait_ready(void)
{
    int tries, s;
    for (tries = 0; tries < 250; tries++) {
        s = padGetState(0, 0);
        if (s == PAD_STATE_STABLE || s == PAD_STATE_FINDCTP1)
            return 1;
        if (s == PAD_STATE_DISCONN)
            return 0;
        busy_wait(2000000);
    }
    return 0;
}

static const char *val_str(int row)
{
    switch (row) {
        case 0: return ps2_video_name(ps2cfg.video);
        case 1: return ps2cfg.filter == PS2_FILTER_SMOOTH ? "Smooth"     : "Sharp";
        case 2: return ps2cfg.cap    == PS2_CAP_30        ? "30 fps"     : "60 fps";
        case 3: switch (ps2cfg.music) {
                    case 0:  return "Off";
                    case 1:  return "Low";
                    case 3:  return "Loud";
                    default: return "Normal";
                }
        case ROW_LAUNCH: return "";
    }
    return "";
}

static void adjust(int row, int dir)
{
    switch (row) {
        case 0: ps2cfg.video = (ps2cfg.video + dir + PS2_VIDEO_COUNT) % PS2_VIDEO_COUNT; break;
        case 1: ps2cfg.filter ^= 1; break;
        case 2: ps2cfg.cap    ^= 1; break;
        case 3: ps2cfg.music += dir;
                if (ps2cfg.music < 0) ps2cfg.music = 0;
                if (ps2cfg.music > 3) ps2cfg.music = 3;
                break;
    }
}

static void draw(int sel, int card_port)
{
    static const char *name[NROWS] =
        { "Video standard", "Texture filter", "Frame cap", "Music", "" };
    int i;

    scr_clear();
    scr_setXY(2, MENU_TOP);
    scr_printf("ps2uke -- PlayStation 2 options");
    scr_setXY(2, MENU_TOP + 1);
    scr_printf("-------------------------------");

    for (i = 0; i < 4; ++i) {
        scr_setXY(2, MENU_TOP + 3 + i);
        scr_printf("%s %-16s : %s", (i == sel) ? ">" : " ", name[i], val_str(i));
    }

    scr_setXY(2, MENU_TOP + 8);
    scr_printf("%s [ Launch Duke Nukem 3D ]", (sel == ROW_LAUNCH) ? ">" : " ");

    scr_setXY(2, MENU_TOP + 10);
    scr_printf("Up/Down: select   Left/Right: change");
    scr_setXY(2, MENU_TOP + 11);
    scr_printf("START or X: launch");
    scr_setXY(2, MENU_TOP + 13);
    if (card_port < 0)
        scr_printf("Memory card: none -- settings not saved");
    else
        scr_printf("Memory card: mc%d -- settings saved here", card_port);
}

int main(int argc, char **argv)
{
    struct padButtonStatus btn;
    int sel = ROW_LAUNCH, last = -1, last_card = -2, launch = 0;
    u16 prev = 0xFFFF;             /* active-low: all released */

    (void) argc; (void) argv;

    SifInitRpc(0);
    SifLoadModule("rom0:SIO2MAN", 0, NULL);
    SifLoadModule("rom0:PADMAN", 0, NULL);

    init_scr();
    scr_clear();
    scr_setXY(2, MENU_TOP);
    scr_printf("ps2uke launcher -- initialising...");

    ps2_mc_init();
    ps2_settings_load();           /* defaults, then card if present */

    padInit(0);
    padPortOpen(0, 0, pad_buf);

    if (pad_wait_ready()) {
        while (!launch) {
            int card_port = ps2_mc_card_port();
            if (sel != last || card_port != last_card) {
                draw(sel, card_port);
                last = sel;
                last_card = card_port;
            }
            if (padRead(0, 0, &btn) != 0) {
                u16 now     = btn.btns;
                u16 pressed = (prev & ~now);   /* 1->0 edge = pressed */
                prev = now;

                if (pressed & PAD_UP)    sel = (sel - 1 + NROWS) % NROWS;
                if (pressed & PAD_DOWN)  sel = (sel + 1) % NROWS;
                if (pressed & PAD_LEFT)  { adjust(sel, -1); last = -1; }
                if (pressed & PAD_RIGHT) { adjust(sel, +1); last = -1; }
                if (pressed & PAD_CROSS) {
                    if (sel == ROW_LAUNCH) launch = 1;
                    else { adjust(sel, +1); last = -1; }
                }
                if (pressed & PAD_START) launch = 1;
            }
            busy_wait(1500000);
        }
    } else {
        launch = 1;                /* no controller -> use saved/default */
    }

    /* Persist on the way out so the choices survive a cold boot. */
    if (ps2_mc_card_port() >= 0)
        ps2_settings_save();

    scr_clear();
    scr_setXY(2, MENU_TOP);
    scr_printf("Launching Duke Nukem 3D...");

    padPortClose(0, 0);
    padEnd();

    /* Hand the live toggles to the game via argv (applies with no card too). */
    {
        static char cfg[40];
        char *args[2];
        ps2_settings_format_argv(cfg, sizeof cfg);
        args[0] = (char *) GAME_ELF;
        args[1] = cfg;
        LoadExecPS2(GAME_ELF, 2, args);   /* noreturn */
    }
    SleepThread();
    return 0;
}
