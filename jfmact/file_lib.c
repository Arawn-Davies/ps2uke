/*
 * file_lib.c
 * File functions to emulate MACT
 *
 * by Jonathon Fowler
 *
 * Since we weren't given the source for MACT386.LIB so I've had to do some
 * creative interpolation here.
 *
 */
//-------------------------------------------------------------------------
/*
Duke Nukem Copyright (C) 1996, 2003 3D Realms Entertainment

This file is part of Duke Nukem 3D version 1.5 - Atomic Edition

Duke Nukem 3D is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/
//-------------------------------------------------------------------------

#include <stdlib.h>
#ifndef _MSC_VER
#include <unistd.h>
#endif
#include <fcntl.h>
#include <errno.h>
#include "compat.h"
#include "types.h"
#include "file_lib.h"
#include "util_lib.h"
#include "cache1d.h"

#if defined(_EE) || defined(__PS2__)
/* This file opens via openfrompath (Bopen -> cdfs) but reads/closes with raw
   POSIX read()/close(), which don't know the tagged cdfs fd -> reads fail and
   SafeClose fatally Error()s. Route the raw I/O through the cdfs shim too.
   (After the system includes above so the declarations aren't macro-mangled.) */
#define read   Bread
#define close  Bclose
#define lseek  Blseek
#endif

#ifndef O_BINARY
#define  O_BINARY 0
#endif
#ifndef O_TEXT
#define  O_TEXT   0
#endif
#ifndef F_OK
#define F_OK 0
#endif

#define MaxFiles 20
static char *FileNames[MaxFiles];

int32 SafeOpen(const char *filename, int32 mode, int32 sharemode)
{
	int32 h;

	h = openfrompath(filename, mode, sharemode);
#if defined(_EE) || defined(__PS2__)
	/* The disc is read-only and optional files (config, saves) may be missing;
	   a failed open must not fatally Error() or the game can't boot. */
	if (h < 0) { printf("SafeOpen: '%s' failed (%s)\n", filename, strerror(errno)); return -1; }
#else
	if (h < 0) Error("Error opening %s: %s", filename, strerror(errno));
#endif

	if (h < MaxFiles) {
		if (FileNames[h]) SafeFree(FileNames[h]);
		FileNames[h] = (char*)SafeMalloc((int)strlen(filename)+1);
		if (FileNames[h]) strcpy(FileNames[h], filename);
	}

	return h;
}

int32 SafeOpenRead(const char *filename, int32 filetype)
{
	switch (filetype) {
		case filetype_binary:
			return SafeOpen(filename, O_RDONLY|O_BINARY, S_IREAD);
		case filetype_text:
			return SafeOpen(filename, O_RDONLY|O_TEXT, S_IREAD);
		default:
			Error("SafeOpenRead: Illegal filetype specified");
			return -1;
	}
}

void SafeClose ( int32 handle )
{
	if (handle < 0) return;
	if (close(handle) < 0) {
		if (handle < MaxFiles)
			Error("Unable to close file %s", FileNames[handle]);
		else
			Error("Unable to close file");
	}

	if (handle < MaxFiles && FileNames[handle]) {
		SafeFree(FileNames[handle]);
		FileNames[handle] = NULL;
	}
}

boolean SafeFileExists(const char *filename)
{
#if defined(_EE) || defined(__PS2__)
	/* access() uses POSIX open() which can't see the cdfs device; probe via
	   openfrompath (routed through the cdfs shim) instead. */
	int32 h = openfrompath(filename, O_RDONLY|O_BINARY, S_IREAD);
	if (h < 0) return false;
	close(h);
	return true;
#else
	if (!access(filename, F_OK)) return true;
	return false;
#endif
}

int32 SafeFileLength ( int32 handle )
{
	if (handle < 0) return -1;
	return (int32)Bfilelength(handle);
}

void SafeRead(int32 handle, void *buffer, int32 count)
{
	int32 b;

	b = (int32)read(handle, buffer, count);
	if (b != count) {
		close(handle);
		if (handle < MaxFiles)
			Error("File read failure %s reading %d bytes from file %s.",
				strerror(errno), count, FileNames[handle]);
		else
			Error("File read failure %s reading %d bytes.",
				strerror(errno), count);
	}
}


