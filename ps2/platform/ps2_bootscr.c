/*
 * ps2_bootscr.c -- on-screen boot text console (adapted from ps2oom).
 *
 * ps2sdk's default printf goes to the SIO serial port (only emulators echo it),
 * so the game's boot log is invisible on real hardware and vanishes on reboot.
 * This brings up libdebug's GS text console and overrides the newlib _write
 * syscall so stdout/stderr land on screen -- until our gsKit driver takes the
 * GS for the game framebuffer (BootScr_End, called from ps2_display.c).
 */

#ifdef PLATFORM_PS2

#include <stdio.h>
#include <string.h>
#include <debug.h>      // init_scr, scr_printf, scr_setXY

static int g_scr_active = 0;

void BootScr_Begin(void)
{
    init_scr();
    scr_setXY(0, 2);    // nudge down past the TV's top-overscan clip
    g_scr_active = 1;
    scr_printf("ps2uke boot console\n");
}

void BootScr_End(void)
{
    g_scr_active = 0;
}

/* Redirect stdout/stderr to the GS text console while the boot screen is up.
   scr_printf needs a NUL-terminated string, so copy the payload in chunks. */
int _write(int fd, const void *buf, size_t nbyte)
{
    if ((fd == 1 || fd == 2) && g_scr_active)
    {
        const char *p = (const char *) buf;
        char tmp[120];
        size_t i = 0;
        while (i < nbyte)
        {
            size_t n = nbyte - i;
            if (n > sizeof(tmp) - 1)
                n = sizeof(tmp) - 1;
            memcpy(tmp, p + i, n);
            tmp[n] = '\0';
            scr_printf("%s", tmp);
            i += n;
        }
    }
    return (int) nbyte;
}

#endif /* PLATFORM_PS2 */
