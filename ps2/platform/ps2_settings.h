/*
 * ps2_settings.h -- PS2-specific options that have no home in duke3d.cfg.
 *
 * GS/console knobs (video standard, texture filter, frame cap, music boost)
 * chosen by the standalone boot launcher (ps2/launcher/) and consumed by the
 * game ELF: ps2_gs.c (video) and driver_audsrv.c (music gain). The launcher
 * hands them to the game via an argv tag (so they survive even without a memory
 * card) and also persists them to a card when one is present (mc0: then mc1:).
 *
 * This header + ps2_settings.c are shared between the launcher and game ELFs.
 */

#ifndef _INCLUDE_PS2_SETTINGS_H_
#define _INCLUDE_PS2_SETTINGS_H_

#if defined(_EE) || defined(__PS2__) || defined(PLATFORM_PS2)

/* Video modes, in picker order. The PS2 GS has only 4 MB of VRAM, so 720p drops
   to 16-bit to fit (see ps2_gs.c). 480p/576p/720p are progressive and need
   component/VGA output on real hardware. 720p is the practical ceiling: gsKit's
   1080i display setup doesn't come up on this pipeline (black), and interlaced
   1080i would be worse than progressive 720p anyway. */
enum {
    PS2_VIDEO_NTSC_480I = 0,
    PS2_VIDEO_NTSC_480P = 1,
    PS2_VIDEO_PAL_576I  = 2,
    PS2_VIDEO_PAL_576P  = 3,
    PS2_VIDEO_720P      = 4,
    PS2_VIDEO_COUNT     = 5
};

const char *ps2_video_name(int idx);   /* picker label for a video mode */
enum { PS2_FILTER_SHARP = 0, PS2_FILTER_SMOOTH = 1 };
enum { PS2_CAP_60 = 0, PS2_CAP_30 = 1 };
/* music: 0 = off, 1 = low, 2 = normal, 3 = loud (mix gain) */

typedef struct {
    int video;    /* PS2_VIDEO_*   */
    int filter;   /* PS2_FILTER_*  */
    int cap;      /* PS2_CAP_*     */
    int music;    /* 0..3 boost    */
} ps2_settings_t;

/* Live settings: defaulted at load, overwritten by argv (game) or the picker
   (launcher). Read directly by ps2_gs.c / driver_audsrv.c. */
extern ps2_settings_t ps2cfg;

void ps2_settings_defaults(void);

/* Music mix gain for the current setting (0 = off). Read by driver_audsrv.c. */
int  ps2_settings_music_gain(void);

/* argv bridge between the launcher and the game ELF. */
void ps2_settings_format_argv(char *buf, int buflen);   /* launcher -> "ps2uke=..." */
void ps2_settings_apply_argv(int argc, char **argv);     /* game: parse the tag    */

/* Memory-card persistence (best-effort; no-ops without a card). */
void ps2_mc_init(void);
int  ps2_mc_card_port(void);     /* 0/1, or -1 if no usable card */
int  ps2_settings_load(void);    /* card -> ps2cfg (after defaults); 1 if loaded */
int  ps2_settings_save(void);    /* ps2cfg -> card; 0 on success */

/* Game-only: reset the IOP and re-launch the boot launcher (quit -> picker). */
void ps2_reboot(void);

#endif /* PS2 */

#endif /* _INCLUDE_PS2_SETTINGS_H_ */
