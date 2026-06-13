/*
 * ps2_settings.c -- shared PS2 options state (launcher + game ELFs).
 *
 * Holds the live ps2cfg, the argv bridge the launcher uses to hand settings to
 * the game (so a choice applies even with no memory card), and best-effort
 * memory-card load/save via libmc's own RPC (independent of the cdfs/fio disc
 * path). Every mc* call is paired with a blocking mcSync(MC_WAIT,...).
 */

#if defined(_EE) || defined(__PS2__) || defined(PLATFORM_PS2)

#include <libmc.h>
#include <loadfile.h>
#include <fcntl.h>          /* O_RDONLY / O_WRONLY / O_CREAT / O_TRUNC */
#include <string.h>
#include <stdio.h>

#include "ps2_settings.h"

ps2_settings_t ps2cfg;

#define SAVE_DIR   "PS2UKE"
#define SAVE_FILE  "PS2UKE/SETTINGS.BIN"
#define CFG_MAGIC   0x554b3250u      /* 'P2KU' */
#define CFG_VERSION 3                /* v3: video ladder = 5 modes (480i..720p) */
#define ARGV_TAG    "ps2uke="

typedef struct {
    unsigned int   magic;
    unsigned int   version;
    ps2_settings_t cfg;
} mc_record_t;

static int mc_ready = 0;

const char *ps2_video_name(int idx)
{
    switch (idx) {
        case PS2_VIDEO_NTSC_480I: return "NTSC 480i";
        case PS2_VIDEO_NTSC_480P: return "NTSC 480p";
        case PS2_VIDEO_PAL_576I:  return "PAL 576i";
        case PS2_VIDEO_PAL_576P:  return "PAL 576p";
        case PS2_VIDEO_720P:      return "720p (exp)";
    }
    return "?";
}

void ps2_settings_defaults(void)
{
    ps2cfg.video  = PS2_VIDEO_NTSC_480I;
    ps2cfg.filter = PS2_FILTER_SHARP;
    ps2cfg.cap    = PS2_CAP_60;
    ps2cfg.music  = 2;               /* normal */
}

int ps2_settings_music_gain(void)
{
    int m = ps2cfg.music;
    if (m < 0) m = 0;
    if (m > 3) m = 3;
    return m;                        /* 0 (off), 1, 2, 3 */
}

/* ---- argv bridge ------------------------------------------------------ */

void ps2_settings_format_argv(char *buf, int buflen)
{
    snprintf(buf, buflen, ARGV_TAG "%d.%d.%d.%d",
             ps2cfg.video, ps2cfg.filter, ps2cfg.cap, ps2cfg.music);
}

void ps2_settings_apply_argv(int argc, char **argv)
{
    int i, vid, fil, cap, mus;

    for (i = 0; i < argc; i++) {
        if (!argv[i] || strncmp(argv[i], ARGV_TAG, sizeof(ARGV_TAG) - 1) != 0)
            continue;

        if (sscanf(argv[i] + sizeof(ARGV_TAG) - 1, "%d.%d.%d.%d",
                   &vid, &fil, &cap, &mus) == 4) {
            if (vid < 0 || vid >= PS2_VIDEO_COUNT) vid = PS2_VIDEO_NTSC_480I;
            ps2cfg.video  = vid;
            ps2cfg.filter = fil ? PS2_FILTER_SMOOTH: PS2_FILTER_SHARP;
            ps2cfg.cap    = cap ? PS2_CAP_30       : PS2_CAP_60;
            if (mus < 0) mus = 0;
            if (mus > 3) mus = 3;
            ps2cfg.music  = mus;
        }
        return;
    }
}

/* ---- memory card ------------------------------------------------------ */

void ps2_mc_init(void)
{
    if (mc_ready)
        return;
    /* No-ops if already resident. */
    SifLoadModule("rom0:SIO2MAN", 0, NULL);
    SifLoadModule("rom0:MCMAN",   0, NULL);
    SifLoadModule("rom0:MCSERV",  0, NULL);
    if (mcInit(MC_TYPE_MC) >= 0)
        mc_ready = 1;
}

int ps2_mc_card_port(void)
{
    int port, type, freec, format, ret;

    if (!mc_ready)
        return -1;

    for (port = 0; port < 2; port++) {
        mcGetInfo(port, 0, &type, &freec, &format);
        mcSync(MC_WAIT, NULL, &ret);
        /* 0 = same card, -1 = formatted card just inserted -> usable. */
        if (ret == 0 || ret == -1)
            return port;
    }
    return -1;
}

int ps2_settings_load(void)
{
    mc_record_t rec;
    int port, fd, r;

    ps2_settings_defaults();

    port = ps2_mc_card_port();
    if (port < 0)
        return 0;

    mcOpen(port, 0, SAVE_FILE, O_RDONLY);
    mcSync(MC_WAIT, NULL, &fd);
    if (fd < 0)
        return 0;

    memset(&rec, 0, sizeof rec);
    mcRead(fd, &rec, sizeof rec);
    mcSync(MC_WAIT, NULL, &r);

    mcClose(fd);
    mcSync(MC_WAIT, NULL, &fd);

    if (r >= (int) sizeof rec
     && rec.magic == CFG_MAGIC && rec.version == CFG_VERSION) {
        ps2cfg = rec.cfg;
        return 1;
    }
    return 0;
}

int ps2_settings_save(void)
{
    mc_record_t rec;
    int port, fd, r;

    port = ps2_mc_card_port();
    if (port < 0)
        return -1;

    mcMkDir(port, 0, SAVE_DIR);
    mcSync(MC_WAIT, NULL, &r);

    mcOpen(port, 0, SAVE_FILE, O_WRONLY | O_CREAT | O_TRUNC);
    mcSync(MC_WAIT, NULL, &fd);
    if (fd < 0)
        return -2;

    rec.magic   = CFG_MAGIC;
    rec.version = CFG_VERSION;
    rec.cfg     = ps2cfg;

    mcWrite(fd, &rec, sizeof rec);
    mcSync(MC_WAIT, NULL, &r);

    mcClose(fd);
    mcSync(MC_WAIT, NULL, &fd);

    return (r == (int) sizeof rec) ? 0 : -3;
}

#endif /* PS2 */
