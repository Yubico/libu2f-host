/*
  Copyright (C) 2013-2014 Yubico AB

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <config.h>
#include <u2f-host/u2f-host-version.h>

#include "check-version.h"

/**
 * u2fh_check_version:
 * @req_version: Required version number, or NULL.
 *
 * Check that the version of the library is at minimum the requested
 * one and return the version string; return NULL if the condition is
 * not satisfied.  If a NULL is passed to this function, no check is
 * done, but the version string is simply returned.
 *
 * See %U2FH_VERSION_STRING for a suitable @req_version string.
 *
 * Return value: Version string of run-time library, or NULL if the
 * run-time library does not meet the required version number.
 */
const char *
u2fh_check_version (const char *req_version)
{
  return check_version (req_version);
}
