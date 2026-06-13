//
// ps2_opl.h -- public entry points for the OPL (FM-synth) music player on PS2.
//
// Duke's sounds.c calls these instead of jfaudiolib's MUSIC_* MIDI path, and
// the audsrv feed thread (driver_audsrv.c) mixes the synth output in via
// OPL_PS2_Render(). Implemented across i_oplmusic.c (MIDI->OPL sequencer) and
// opl_ps2.c (DBOPL software FM rendered on demand).
//

#ifndef PS2_OPL_H
#define PS2_OPL_H

#ifdef __cplusplus
extern "C" {
#endif

// Bring the OPL synth up at the given output sample rate (Hz). Must match the
// rate of the buffer OPL_PS2_Render() fills. Safe to call once at startup.
void PS2OPL_Init(int samplerate);

// Tear the synth down.
void PS2OPL_Shutdown(void);

// Register `len` bytes of standard MIDI (or MUS) at `data` and start it.
// `looping` non-zero restarts the song at end of track.
void PS2OPL_PlaySong(void *data, int len, int looping);

// Stop and unregister the current song.
void PS2OPL_StopSong(void);

// Set music volume on Duke's 0..255 scale (mapped to the OPL 0..127 range).
void PS2OPL_SetVolume(int duke_volume);

// Pause (non-zero) / resume (zero) the current song.
void PS2OPL_Pause(int paused);

// Non-zero while a song is playing.
int PS2OPL_IsPlaying(void);

// Render `nframes` of interleaved stereo int16 OPL music (silence when idle).
// Lives in opl_ps2.c; declared here for the audsrv feed thread.
void OPL_PS2_Render(short *stereo_out, int nframes);

#ifdef __cplusplus
}
#endif

#endif // PS2_OPL_H
