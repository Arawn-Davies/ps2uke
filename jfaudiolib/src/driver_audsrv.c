/*
 * driver_audsrv.c -- jfaudiolib PCM output via the PS2 audsrv IOP module.
 *
 * MultiVoc mixes 16-bit stereo into a buffer split into NumDivisions divisions;
 * a dedicated, higher-priority EE thread streams one division at a time to
 * audsrv_play_audio(), self-pacing on audsrv_wait_audio() (which blocks until the
 * IOP ring has room). The IOP audio modules are brought up by libps2_drivers'
 * init_audio_driver(), mirroring ps2quake's snd_ps2.c.
 *
 * A semaphore serialises the mixer callback (run on the feed thread) against the
 * game thread's MultiVoc calls -- that's what FX/MV_Lock()/Unlock() guard.
 *
 * Music: Duke's MIDI is rendered by the OPL (DBOPL) software synth and mixed in
 * here, right before each division is pushed -- OPL_PS2_Render() pulls exactly
 * one division's worth of stereo samples each loop, so the synth's clock paces
 * to the same audsrv output it shares with the SFX (see ps2/opl/).
 */

#if defined(_EE) || defined(__PS2__) || defined(PLATFORM_PS2)

#include <kernel.h>
#include <string.h>
#include <audsrv.h>
#include <ps2_audio_driver.h>

#include "driver_audsrv.h"
#include "ps2_opl.h"        /* OPL_PS2_Render (ps2/opl)        */
#include "ps2_settings.h"   /* ps2_settings_music_gain() (boot picker) */

/* OPL music is boosted before mixing: the DBOPL output sits well below full
   scale, and the final clamp catches the rare loud peak. The boost amount is
   the boot picker's "Music" setting (0 = off .. 3 = loud). */
#define OPL_SCRATCH_FRAMES 1024            /* stereo frames per render block */

enum {
    AudSrvErr_Warning = -2,
    AudSrvErr_Error   = -1,
    AudSrvErr_Ok      = 0,
    AudSrvErr_AudioDriver,
    AudSrvErr_SetFormat,
    AudSrvErr_Thread,
};

static int ErrorCode    = AudSrvErr_Ok;
static int Initialised  = 0;

static char *MixBuffer       = NULL;   /* total = MixBufferSize * MixBufferCount  */
static int   MixBufferSize   = 0;      /* one division, in bytes                  */
static int   MixBufferCount  = 0;      /* number of divisions                     */
static int   MixBufferCurrent = 0;
static void (*MixCallBack)(void) = NULL;

static volatile int playing  = 0;
static int  feed_tid  = -1;
static int  mix_sema  = -1;
static int  MixStereo = 1;             /* output is interleaved stereo?       */
static char feed_stack[16 * 1024] __attribute__((aligned(16)));

int AudSrvDrv_GetError(void) { return ErrorCode; }

/* Render OPL music over a just-mixed division and add it in. `buf` is the
   division about to be played (signed 16-bit; stereo interleaved when
   MixStereo). Rendered in blocks so the scratch buffer stays small. */
static void MixOplMusic(short *buf, int total_frames)
{
    static short opl[OPL_SCRATCH_FRAMES * 2];   /* stereo scratch */
    int done = 0;
    int gain = ps2_settings_music_gain();       /* 0 = music off */

    if (gain <= 0)
        return;                                 /* picker set Music = Off */

    while (done < total_frames) {
        int n = total_frames - done;
        int i, v;
        if (n > OPL_SCRATCH_FRAMES) n = OPL_SCRATCH_FRAMES;

        OPL_PS2_Render(opl, n);                 /* interleaved L,R int16 */

        if (MixStereo) {
            short *dst = buf + done * 2;
            for (i = 0; i < n * 2; ++i) {
                v = dst[i] + opl[i] * gain;
                if (v >  32767) v =  32767;
                if (v < -32768) v = -32768;
                dst[i] = (short) v;
            }
        } else {
            short *dst = buf + done;            /* mono: downmix L+R */
            for (i = 0; i < n; ++i) {
                int m = (opl[2 * i] + opl[2 * i + 1]) >> 1;
                v = dst[i] + m * gain;
                if (v >  32767) v =  32767;
                if (v < -32768) v = -32768;
                dst[i] = (short) v;
            }
        }
        done += n;
    }
}

const char *AudSrvDrv_ErrorString(int ErrorNumber)
{
    switch (ErrorNumber) {
        case AudSrvErr_Warning:
        case AudSrvErr_Error:       return AudSrvDrv_ErrorString(ErrorCode);
        case AudSrvErr_Ok:          return "audsrv ok.";
        case AudSrvErr_AudioDriver: return "audsrv: init_audio_driver failed.";
        case AudSrvErr_SetFormat:   return "audsrv: audsrv_set_format failed.";
        case AudSrvErr_Thread:      return "audsrv: could not create feed thread.";
        default:                    return "audsrv: unknown error.";
    }
}

/* Stream one division at a time. audsrv_wait_audio blocks until the IOP ring can
   take MixBufferSize bytes, so this self-paces to the output rate and yields. */
static void FeedThread(void *arg)
{
    (void) arg;
    while (playing) {
        audsrv_wait_audio(MixBufferSize);
        WaitSema(mix_sema);
        /* Add the OPL music onto the SFX mix for this division, then push it.
           Frames = bytes / (2 bytes * channels). */
        MixOplMusic((short *)(MixBuffer + MixBufferCurrent * MixBufferSize),
                    MixBufferSize / (MixStereo ? 4 : 2));
        audsrv_play_audio(MixBuffer + MixBufferCurrent * MixBufferSize, MixBufferSize);
        if (++MixBufferCurrent >= MixBufferCount) MixBufferCurrent = 0;
        if (MixCallBack) MixCallBack();        /* mix the next division */
        SignalSema(mix_sema);
    }
    ExitThread();
}

int AudSrvDrv_PCM_Init(int *mixrate, int *numchannels, int *samplebits, void *initdata)
{
    struct audsrv_fmt_t fmt;
    (void) initdata;

    if (Initialised) AudSrvDrv_PCM_Shutdown();

    if (init_audio_driver() != AUDIO_INIT_STATUS_OK) {
        ErrorCode = AudSrvErr_AudioDriver;
        return AudSrvErr_Error;
    }

    fmt.freq     = *mixrate;
    fmt.bits     = 16;                         /* audsrv + MultiVoc: signed 16-bit */
    fmt.channels = (*numchannels >= 2) ? 2 : 1;
    if (audsrv_set_format(&fmt) != 0) {
        ErrorCode = AudSrvErr_SetFormat;
        deinit_audio_driver();
        return AudSrvErr_Error;
    }
    audsrv_set_volume(MAX_VOLUME);

    *samplebits  = 16;
    *numchannels = fmt.channels;
    MixStereo    = (fmt.channels >= 2);
    /* *mixrate is honoured as requested */

    Initialised = 1;
    return AudSrvErr_Ok;
}

void AudSrvDrv_PCM_Shutdown(void)
{
    if (!Initialised) return;
    AudSrvDrv_PCM_StopPlayback();
    deinit_audio_driver();
    Initialised = 0;
}

int AudSrvDrv_PCM_BeginPlayback(char *BufferStart, int BufferSize,
                                int NumDivisions, void (*CallBackFunc)(void))
{
    ee_sema_t sema;
    ee_thread_t th;
    void *gp;

    if (!Initialised) { ErrorCode = AudSrvErr_Error; return AudSrvErr_Error; }
    if (playing) AudSrvDrv_PCM_StopPlayback();

    MixBuffer        = BufferStart;
    MixBufferSize    = BufferSize;
    MixBufferCount   = NumDivisions;
    MixBufferCurrent = 0;
    MixCallBack      = CallBackFunc;

    if (MixCallBack) MixCallBack();            /* prime division 0 */

    memset(&sema, 0, sizeof(sema));
    sema.init_count = 1;
    sema.max_count  = 1;
    mix_sema = CreateSema(&sema);
    if (mix_sema < 0) { ErrorCode = AudSrvErr_Thread; return AudSrvErr_Error; }

    playing = 1;

    /* Run the feed thread above the game thread so chunks are pushed the instant
       audsrv needs them, even during render-heavy frames (ps2quake's split). */
    ChangeThreadPriority(GetThreadId(), 0x40);

    __asm__ volatile ("move %0, $28" : "=r"(gp));
    memset(&th, 0, sizeof(th));
    th.func             = (void *) FeedThread;
    th.stack            = feed_stack;
    th.stack_size       = sizeof(feed_stack);
    th.gp_reg           = gp;
    th.initial_priority = 0x20;
    feed_tid = CreateThread(&th);
    if (feed_tid < 0) {
        playing = 0;
        DeleteSema(mix_sema); mix_sema = -1;
        ErrorCode = AudSrvErr_Thread;
        return AudSrvErr_Error;
    }
    StartThread(feed_tid, NULL);

    return AudSrvErr_Ok;
}

void AudSrvDrv_PCM_StopPlayback(void)
{
    if (!playing) return;
    playing = 0;
    audsrv_stop_audio();                       /* unblock audsrv_wait_audio */
    if (feed_tid >= 0) { TerminateThread(feed_tid); DeleteThread(feed_tid); feed_tid = -1; }
    if (mix_sema >= 0) { DeleteSema(mix_sema); mix_sema = -1; }
}

void AudSrvDrv_PCM_Lock(void)   { if (mix_sema >= 0) WaitSema(mix_sema); }
void AudSrvDrv_PCM_Unlock(void) { if (mix_sema >= 0) SignalSema(mix_sema); }

#endif /* PS2 */
