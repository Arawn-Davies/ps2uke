/*
 * driver_audsrv.h -- PS2 audsrv PCM output driver for jfaudiolib (SFX).
 */

int  AudSrvDrv_GetError(void);
const char *AudSrvDrv_ErrorString( int ErrorNumber );

int  AudSrvDrv_PCM_Init(int *mixrate, int *numchannels, int *samplebits, void *initdata);
void AudSrvDrv_PCM_Shutdown(void);
int  AudSrvDrv_PCM_BeginPlayback(char *BufferStart, int BufferSize,
              int NumDivisions, void ( *CallBackFunc )( void ) );
void AudSrvDrv_PCM_StopPlayback(void);
void AudSrvDrv_PCM_Lock(void);
void AudSrvDrv_PCM_Unlock(void);
