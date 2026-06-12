/*
 * PS2 file-I/O shim for cache1d.c -- see ps2_fileio.h for the why.
 *
 * "Build Engine & Tools" Copyright (c) 1993-1997 Ken Silverman.
 * See BUILDLIC.TXT. This file is not part of Ken's original release.
 */

#ifdef PLATFORM_PS2

#define NEWLIB_PORT_AWARE      /* we deliberately use fio for the cdfs device */

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <fileio.h>            /* fioOpen/fioRead/fioLseek/fioClose, FIO_O_RDONLY */

#include "ps2_fileio.h"

/* High bit tags a descriptor as fio-owned (vs a newlib POSIX fd). */
#define PS2_FIO_TAG   0x40000000

static int is_disc_path(const char *p)
{
    return (strncmp(p, "cdfs", 4) == 0) || (strncmp(p, "cdrom", 5) == 0);
}

int ps2_bopen(const char *path, int flags, ...)
{
    int  rdonly = ((flags & O_ACCMODE) == O_RDONLY);
    char buf[96];
    const char *p = path;

    if (rdonly)
    {
        /* A bare filename ("DUKE3D.GRP") means "the data disc". */
        if (strchr(path, ':') == NULL)
        {
            snprintf(buf, sizeof buf, "cdfs:/%s", path);
            p = buf;
        }
        if (is_disc_path(p))
        {
            int fd = fioOpen(p, FIO_O_RDONLY);
            return (fd < 0) ? -1 : (fd | PS2_FIO_TAG);
        }
        return open(p, flags);            /* host:/mc0: read via POSIX */
    }

    return open(path, flags, 0666);       /* writes never go to the disc */
}

int ps2_bread(int fd, void *buf, int len)
{
    if (fd & PS2_FIO_TAG)
        return fioRead(fd & ~PS2_FIO_TAG, buf, len);
    return read(fd, buf, len);
}

int ps2_blseek(int fd, int off, int whence)
{
    if (fd & PS2_FIO_TAG)
        return fioLseek(fd & ~PS2_FIO_TAG, off, whence);
    return lseek(fd, off, whence);
}

int ps2_bclose(int fd)
{
    if (fd & PS2_FIO_TAG)
        return fioClose(fd & ~PS2_FIO_TAG);
    return close(fd);
}

#endif /* PLATFORM_PS2 */
