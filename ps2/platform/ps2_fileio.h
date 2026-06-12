/*
 * PS2 file-I/O shim for the BUILD engine's cache1d.c.
 *
 * cache1d reads the GRP (and any external files) with POSIX open/read/lseek/
 * close. On the PS2 the game data lives on the disc, reached via cdfs.irx --
 * a LEGACY ioman device that newlib's open() can't see and that rejects
 * O_RDONLY (0), needing FIO_O_RDONLY (1). So read-only opens of disc paths are
 * routed through the fio API instead; everything else falls through to POSIX.
 *
 * cache1d.c #defines open/read/lseek/close to these (after its system includes,
 * so <fcntl.h>'s declarations aren't mangled). Returned descriptors carry a tag
 * bit so ps2_bread/blseek/bclose know which backend owns them. See PORTING.md.
 */

#ifndef _INCLUDE_PS2_FILEIO_H_
#define _INCLUDE_PS2_FILEIO_H_

#ifdef PLATFORM_PS2

int  ps2_bopen(const char *path, int flags, ...);
int  ps2_bread(int fd, void *buf, int len);
int  ps2_blseek(int fd, int off, int whence);
int  ps2_bclose(int fd);

#endif /* PLATFORM_PS2 */

#endif /* _INCLUDE_PS2_FILEIO_H_ */
