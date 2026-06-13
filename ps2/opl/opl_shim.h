//
// opl_shim.h -- tiny compatibility layer so the OPL/MIDI sources lifted from
// the ps2oom (doomgeneric) tree build inside ps2uke without dragging in the
// whole Chocolate Doom support tree (doomtype/i_swap/m_misc/z_zone/...).
//
// Each Doom header the lifted .c files include (doomtype.h, i_swap.h, ...) is
// reduced to a one-line stub that pulls this in; everything those files
// actually reference is provided here. The PS2 EE is little-endian, so the
// byte-swap macros are identities, matching the SYS_LITTLE_ENDIAN path.
//

#ifndef PS2_OPL_SHIM_H
#define PS2_OPL_SHIM_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>

// ---- doomtype.h -------------------------------------------------------

#if defined(__GNUC__)
#define PACKEDATTR __attribute__((packed))
#else
#define PACKEDATTR
#endif

typedef uint8_t byte;

// Mirror Chocolate Doom's doomtype.h: boolean is always defined (int-sized, as
// Doom casts boolean* to int*); true/false come from <stdbool.h> when present.
#if defined(__cplusplus) || defined(__bool_true_false_are_defined)
typedef unsigned int boolean;
#else
typedef enum { false, true } boolean;
#endif

#ifndef arrlen
#define arrlen(array) (sizeof(array) / sizeof(*array))
#endif

// ---- i_swap.h (little-endian: identities) -----------------------------

#ifndef SHORT
#define SHORT(x)  ((signed short) (x))
#endif
#ifndef LONG
#define LONG(x)   ((signed int) (x))
#endif

// ---- z_zone.h ---------------------------------------------------------

#define PU_STATIC   1
#define PU_SOUND    9
#define PU_LEVEL    50
#define PU_CACHE    101

#define Z_Malloc(size, tag, user)  malloc((size_t) (size))
#define Z_Free(ptr)                free(ptr)
#define Z_Realloc(ptr, size, tag, user)  realloc((ptr), (size_t) (size))

// ---- deh_main.h / deh_str.h ------------------------------------------

#define DEH_String(x)  (x)

// ---- m_misc.h / i_system.h (inline shims) -----------------------------

static inline int M_snprintf(char *buf, size_t buf_len, const char *s, ...)
{
    va_list args;
    int result;
    va_start(args, s);
    result = vsnprintf(buf, buf_len, s, args);
    va_end(args);
    return result;
}

static inline boolean M_StringConcat(char *dest, const char *src, size_t dest_size)
{
    size_t offset = strlen(dest);
    if (offset > dest_size) offset = dest_size;
    M_snprintf(dest + offset, dest_size - offset, "%s", src);
    return strlen(dest) < dest_size - 1;
}

static inline FILE *M_fopen(const char *path, const char *mode)
{
    return fopen(path, mode);
}

static inline boolean M_WriteFile(char *name, void *source, int length)
{
    FILE *handle = fopen(name, "wb");
    int count;
    if (handle == NULL) return false;
    count = fwrite(source, 1, length, handle);
    fclose(handle);
    return count == length;
}

static inline int M_ReadFile(char *name, byte **buffer)
{
    FILE *handle;
    int count, length;
    byte *buf;

    handle = fopen(name, "rb");
    if (handle == NULL) return -1;

    fseek(handle, 0, SEEK_END);
    length = ftell(handle);
    fseek(handle, 0, SEEK_SET);

    buf = (byte *) malloc(length);
    count = fread(buf, 1, length, handle);
    fclose(handle);

    if (count < length) { free(buf); return -1; }

    *buffer = buf;
    return length;
}

static inline void I_Error(const char *error, ...)
{
    va_list args;
    va_start(args, error);
    vfprintf(stderr, error, args);
    va_end(args);
    fputc('\n', stderr);
}

#endif // PS2_OPL_SHIM_H
