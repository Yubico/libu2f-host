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
#include <u2f-host.h>

#define ERR(name, desc) { name, #name, desc }

typedef struct
{
  int rc;
  const char *name;
  const char *description;
} err_t;

static const err_t errors[] = {
  ERR (U2FH_OK, "successful return"),
  ERR (U2FH_MEMORY_ERROR, "out of memory or other memory error"),
  ERR (U2FH_TRANSPORT_ERROR, "error in transport layer"),
  ERR (U2FH_JSON_ERROR, "error in JSON handling"),
  ERR (U2FH_BASE64_ERROR, "base64 error"),
  ERR (U2FH_NO_U2F_DEVICE, "cannot find U2F device"),
  ERR (U2FH_AUTHENTICATOR_ERROR, "authenticator error"),
  ERR (U2FH_TIMEOUT_ERROR, "timeout error"),
};

/**
 * u2fh_strerror:
 * @err: error code
 *
 * Convert return code to human readable string explanation of the
 * reason for the particular error code.
 *
 * This string can be used to output a diagnostic message to the user.
 *
 * This function is one of few in the library that can be used without
 * a successful call to u2fh_global_init().
 *
 * Return value: Returns a pointer to a statically allocated string
 *   containing an explanation of the error code @err.
 **/
const char *
u2fh_strerror (int err)
{
  static const char *unknown = "Unknown libu2f-host error";
  const char *p;

  if (-err < 0 || -err >= (int) (sizeof (errors) / sizeof (errors[0])))
    return unknown;

  p = errors[-err].description;
  if (!p)
    p = unknown;

  return p;
}

/**
 * u2fh_strerror_name:
 * @err: error code
 *
 * Convert return code to human readable string representing the error
 * code symbol itself.  For example, u2fh_strerror_name(%U2FH_OK)
 * returns the string "U2FH_OK".
 *
 * This string can be used to output a diagnostic message to the user.
 *
 * This function is one of few in the library that can be used without
 * a successful call to u2fh_global_init().
 *
 * Return value: Returns a pointer to a statically allocated string
 *   containing a string version of the error code @err, or NULL if
 *   the error code is not known.
 **/
const char *
u2fh_strerror_name (int err)
{
  if (-err < 0 || -err >= (int) (sizeof (errors) / sizeof (errors[0])))
    return NULL;

  return errors[-err].name;
}
