/*
 * ps2_audio_stub.c -- silent audio backend for the PS2 build.
 *
 * Chocolate's dummy_audiolib.c stubs are stale (their signatures predate the
 * headers + our int32->int32_t change), so we provide fresh no-op FX_ and MUSIC_
 * stubs matching the current fx_man.h / music.h. Real SPU2/audsrv sound later.
 */

#ifdef PLATFORM_PS2

#include <inttypes.h>
#include "audiolib/fx_man.h"
#include "audiolib/music.h"

char *FX_ErrorString(int ErrorNumber) { (void) ErrorNumber; static char s[] = ""; return s; }
int   FX_Init(int a, int b, int c, int d, unsigned e) { (void)a;(void)b;(void)c;(void)d;(void)e; return 0; }
int   FX_Shutdown(void) { return 0; }
int   FX_VoiceAvailable(int priority) { (void) priority; return 1; }
int   FX_GetReverseStereo(void) { return 0; }
void  FX_SetReverseStereo(int setting) { (void) setting; }
void  FX_SetVolume(int volume) { (void) volume; }
void  FX_SetReverb(int reverb) { (void) reverb; }
void  FX_SetReverbDelay(int delay) { (void) delay; }
int   FX_SetCallBack(void (*function)(int32_t)) { (void) function; return 0; }
int32_t FX_Pan3D(int handle, int angle, int distance) { (void)handle;(void)angle;(void)distance; return 0; }
int32_t FX_StopAllSounds(void) { return 0; }
int32_t FX_StopSound(int handle) { (void) handle; return 0; }
int   FX_SetupSoundBlaster(fx_blaster_config blaster, int *MaxVoices, int *MaxSampleBits, int *MaxChannels)
{ (void)blaster; if(MaxVoices)*MaxVoices=0; if(MaxSampleBits)*MaxSampleBits=0; if(MaxChannels)*MaxChannels=0; return 0; }

int FX_PlayLoopedVOC(uint8_t *ptr, int32_t loopstart, int32_t loopend, int32_t pitchoffset,
                     int32_t vol, int32_t left, int32_t right, int32_t priority, uint32_t callbackval)
{ (void)ptr;(void)loopstart;(void)loopend;(void)pitchoffset;(void)vol;(void)left;(void)right;(void)priority;(void)callbackval; return 0; }
int FX_PlayLoopedWAV(uint8_t *ptr, int32_t loopstart, int32_t loopend, int32_t pitchoffset,
                     int32_t vol, int32_t left, int32_t right, int32_t priority, uint32_t callbackval)
{ (void)ptr;(void)loopstart;(void)loopend;(void)pitchoffset;(void)vol;(void)left;(void)right;(void)priority;(void)callbackval; return 0; }
int FX_PlayVOC3D(uint8_t *ptr, int32_t pitchoffset, int32_t angle, int32_t distance,
                 int32_t priority, uint32_t callbackval)
{ (void)ptr;(void)pitchoffset;(void)angle;(void)distance;(void)priority;(void)callbackval; return 0; }
int FX_PlayWAV3D(uint8_t *ptr, int pitchoffset, int angle, int distance,
                 int priority, uint32_t callbackval)
{ (void)ptr;(void)pitchoffset;(void)angle;(void)distance;(void)priority;(void)callbackval; return 0; }

void MUSIC_Continue(void) {}
void MUSIC_Pause(void) {}
int  MUSIC_Init(int SoundCard, int Address) { (void)SoundCard;(void)Address; return 0; }
int  MUSIC_Shutdown(void) { return 0; }
int  MUSIC_StopSong(void) { return 0; }
void MUSIC_SetVolume(int volume) { (void) volume; }
void MUSIC_RegisterTimbreBank(unsigned char *timbres) { (void) timbres; }

void PlayMusic(char *filename) { (void) filename; }

#endif /* PLATFORM_PS2 */
