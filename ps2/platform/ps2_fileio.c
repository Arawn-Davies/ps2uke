/*
 * PS2 file-I/O shim. jfbuild's Bopen/Bread/Blseek/Bclose are routed here
 * (see jfbuild/include/compat.h): the GRP and all data live on the boot disc,
 * reached via cdfs.irx -- a legacy ioman device POSIX open() can't see and that
 * needs FIO_O_RDONLY, so disc reads go through the fio API. cdfs is brought up
 * lazily on first use (init_cdfs_driver, from ps2_drivers).
 */

#if defined(_EE) || defined(__PS2__) || defined(PLATFORM_PS2)

#define NEWLIB_PORT_AWARE      /* we deliberately use fio for the cdfs device */

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <fileio.h>             /* fioOpen/fioRead/fioLseek/fioClose, FIO_O_RDONLY */
#include <sbv_patches.h>        /* sbv_patch_enable_lmb (load IRX from EE buffer) */
#include <ps2_cdfs_driver.h>    /* init_cdfs_driver (libps2_drivers) */

/* High bit tags a descriptor as fio-owned (vs a newlib POSIX fd). */
#define PS2_FIO_TAG   0x40000000

static int is_disc_path(const char *p)
{
    return (strncmp(p, "cdfs", 4) == 0) || (strncmp(p, "cdrom", 5) == 0);
}

static void ensure_cdfs(void)
{
    static int inited = 0;
    if (inited) return;
    inited = 1;
    /* ps2_drivers loads the embedded cdfs.irx via SifExecModuleBuffer, which
       needs load-module-from-buffer enabled first. SDL2main reset the IOP but
       didn't apply these, so without them init_cdfs_driver() returns 0 yet the
       'cdfs' device never registers ("Unknown device 'cdfs'"). */
    sbv_patch_enable_lmb();
    sbv_patch_disable_prefix_check();
    printf("cdfs: init_cdfs_driver() = %d\n", (int) init_cdfs_driver());
    fflush(stdout);
}

int ps2_bopen(const char *path, int flags, ...)
{
    int  rdonly = ((flags & O_ACCMODE) == O_RDONLY);
    char buf[96];
    const char *p = path;

    /* Strip leading "./" search-path noise so cdfs gets a clean path
       ("./duke3d.cfg" -> "duke3d.cfg" -> cdfs:/duke3d.cfg). */
    while (path[0] == '.' && path[1] == '/')
        path += 2;
    p = path;

    if (rdonly)
    {
        /* A bare filename ("duke3d.grp") means "the data disc". */
        if (strchr(path, ':') == NULL)
        {
            snprintf(buf, sizeof buf, "cdfs:/%s", path);
            p = buf;
        }
        if (is_disc_path(p))
        {
            int fd;
            ensure_cdfs();
            fd = fioOpen(p, FIO_O_RDONLY);
            return (fd < 0) ? -1 : (fd | PS2_FIO_TAG);
        }
        return open(p, flags);            /* host:/mc0: read via POSIX */
    }

    return open(path, flags, 0666);       /* writes never go to the disc */
}

int ps2_bread(int fd, void *buf, int len)
{
    if (fd & PS2_FIO_TAG)
    {
        /* fioRead over cdfs can return short (sector-sized) reads; loop so the
           caller gets the full request (the engine's kread expects exact). */
        int rfd = fd & ~PS2_FIO_TAG, total = 0;
        char *p = (char *) buf;
        while (total < len)
        {
            int n = fioRead(rfd, p + total, len - total);
            if (n <= 0) break;
            total += n;
        }
        return total;
    }
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

#endif /* PS2 */
