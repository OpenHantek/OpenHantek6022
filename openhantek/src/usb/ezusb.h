#pragma once
/*
 * Copyright © 2001 Stephen Williams (steve@icarus.com)
 * Copyright © 2002 David Brownell (dbrownell@users.sourceforge.net)
 * Copyright © 2013 Federico Manzan (f.manzan@gmail.com)
 *
 *    This source code is free software; you can redistribute it
 *    and/or modify it in source code form under the terms of the GNU
 *    General Public License as published by the Free Software
 *    Foundation; either version 2 of the License, or (at your option)
 *    any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <inttypes.h>

struct libusb_device_handle;
#define FX_TYPE_FX2 2   /* USB 2.0 versions */
#define FX_TYPE_FX2LP 3 /* Updated FX2 */
#define FX_TYPE_FX3 4   /* USB 3.0 versions */

/*
 * This function uploads the firmware from the given file into RAM.
 * Stage == 0 means this is a single stage load (or the first of
 * two stages).  Otherwise it's the second of two stages; the
 * caller having preloaded the second stage loader.
 *
 * The target processor is reset at the end of this upload.
 */
extern int ezusb_load_ram( libusb_device_handle *device, const char *path, int fx_type, int stage );

// Verbosity level set by command line option --verbose
extern int verboseLevel;
