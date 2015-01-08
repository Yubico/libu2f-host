/*
  Copyright (C) 2013-2015 Yubico AB

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU Lesser General Public License as published by
  the Free Software Foundation; either version 2.1, or (at your option) any
  later version.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
  General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with this program; if not, see <http://www.gnu.org/licenses/>.
*/

#include <config.h>
#include "internal.h"

int debug;

/**
 * u2fh_global_init:
 * @flags: initialization flags, ORed #u2fh_initflags.
 *
 * Initialize the library.  This function is not guaranteed to be
 * thread safe and must be invoked on application startup.
 *
 * Returns: On success %U2FH_OK (integer 0) is returned, and on errors
 * an #u2fh_rc error code.
 */
u2fh_rc
u2fh_global_init (u2fh_initflags flags)
{
  if (flags & U2FH_DEBUG)
    debug = 1;

  return U2FH_OK;
}

/**
 * u2fh_global_done:
 *
 * Release all resources from the library.  Call this function when no
 * further use of the library is needed.
 */
void
u2fh_global_done (void)
{
  debug = 0;
}
