//
//  ps2_compat.h -- PlayStation 2 compatibility for Chocolate Duke3D.
//
//  Modeled on unix_compat.h but WITHOUT PLATFORM_SUPPORTS_SDL: the PS2 supplies
//  its own platform layer (ps2/platform: gsKit video, cdfs file I/O, libpad
//  input) instead of SDL. Networking/audio use the in-tree dummy backends for
//  now. See PORTING.md.
//

#ifndef Duke3D_ps2_compat_h
#define Duke3D_ps2_compat_h

#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#define kmalloc(x)  malloc(x)
#define kkmalloc(x) malloc(x)
#define kfree(x)    free(x)
#define kkfree(x)   free(x)

#ifdef FP_OFF
#undef FP_OFF
#endif
// Watcom-ism: cast a memory pointer to a 32-bit int. The EE n32 ABI has 32-bit
// pointers, so this is well-defined here.
#define FP_OFF(x) ((int32_t) (x))

#ifndef max
#define max(x, y)  (((x) > (y)) ? (x) : (y))
#endif
#ifndef min
#define min(x, y)  (((x) < (y)) ? (x) : (y))
#endif

#define __int64 int64_t
#define O_BINARY 0

// PS2 EE is little-endian; global.h #errors without BYTE_ORDER.
#ifndef LITTLE_ENDIAN
#define LITTLE_ENDIAN 1234
#endif
#ifndef BIG_ENDIAN
#define BIG_ENDIAN 4321
#endif
#ifndef BYTE_ORDER
#define BYTE_ORDER LITTLE_ENDIAN
#endif

// DOS/Watcom keywords GCC doesn't know.
#define cdecl
#define __far
#define __interrupt

#define stricmp strcasecmp
#define strcmpi strcasecmp

#ifndef S_IREAD
#define S_IREAD S_IRUSR
#endif

// Use the in-tree dummy backends (dummy_multi.c / dummy_audiolib.c) for now.
#define USER_DUMMY_NETWORK 1

#endif
