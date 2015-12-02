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

#include <unistd.h>
#include <json.h>
#include "b64/cencode.h"
#include "sha256.h"

static int
prepare_response2 (const char *respstr, const char *bdstr, char **response,
		   size_t * response_len)
{
  int rc = U2FH_JSON_ERROR;
  struct json_object *jo = NULL, *resp = NULL, *bd = NULL;
  const char *reply;

  bd = json_object_new_string (bdstr);
  if (bd == NULL)
    goto done;
  resp = json_object_new_string (respstr);
  if (resp == NULL)
    goto done;

  jo = json_object_new_object ();
  if (jo == NULL)
    goto done;

  json_object_object_add (jo, "registrationData", resp);
  json_object_object_add (jo, "clientData", bd);

  reply = json_object_to_json_string (jo);
  if (*response == NULL)
    {
      *response = strdup (reply);
    }
  else
    {
      if (strlen (reply) >= *response_len)
	{
	  rc = U2FH_SIZE_ERROR;
	  goto done;
	}
      strncpy (*response, reply, *response_len);
    }
  *response_len = strlen (reply);
  if (*response == NULL)
    rc = U2FH_MEMORY_ERROR;
  else
    rc = U2FH_OK;

done:
  json_object_put (jo);
  if (!jo) {
    json_object_put (resp);
    json_object_put (bd);
  }

  return rc;
}

static int
prepare_response (const unsigned char *buf, int len, const char *bd,
		  char **response, size_t * response_len)
{
  base64_encodestate b64ctx;
  char b64resp[2048];
  char bdstr[2048];
  int cnt;

  if (len > 2048)
    return U2FH_MEMORY_ERROR;
  if (strlen (bd) > 2048)
    return U2FH_MEMORY_ERROR;

  base64_init_encodestate (&b64ctx);
  cnt = base64_encode_block (buf, len, b64resp, &b64ctx);
  base64_encode_blockend (b64resp + cnt, &b64ctx);

  base64_init_encodestate (&b64ctx);
  cnt = base64_encode_block (bd, strlen (bd), bdstr, &b64ctx);
  base64_encode_blockend (bdstr + cnt, &b64ctx);

  return prepare_response2 (b64resp, bdstr, response, response_len);
}

#define V2CHALLEN 32

#define HOSIZE 32
#define NOTSATISFIED "\x69\x85"

static u2fh_rc
_u2fh_register (u2fh_devs * devs,
		const char *challenge,
		const char *origin, char **response, size_t * response_len,
		u2fh_cmdflags flags)
{
  unsigned char data[V2CHALLEN + HOSIZE];
  unsigned char buf[MAXDATASIZE];
  char bd[2048];
  size_t bdlen = sizeof (bd);
  size_t len;
  int rc = U2FH_JSON_ERROR;
  char chalb64[256];
  size_t challen = sizeof (chalb64);
  int iterations = 0;

  rc = get_fixed_json_data (challenge, "challenge", chalb64, &challen);
  if (rc != U2FH_OK)
    {
      return rc;
    }

  rc = prepare_browserdata (chalb64, origin, REGISTER_TYP, bd, &bdlen);
  if (rc != U2FH_OK)
    return rc;

  sha256_buffer (bd, bdlen, data);

  prepare_origin (challenge, data + V2CHALLEN);

  /* FIXME: Support asynchronous usage, through a new u2fh_cmdflags
     flag. */

  do
    {
      int i;
      if (iterations++ > 15)
	{
	  return U2FH_TIMEOUT_ERROR;
	}
      for (i = 0; i < devs->num_devices; i++)
	{
	  if (!devs->devs[i].is_alive)
	    {
	      continue;
	    }
	  len = MAXDATASIZE;
	  rc = send_apdu (devs, i, U2F_REGISTER, data, sizeof (data),
			  flags & U2FH_REQUEST_USER_PRESENCE ? 3 : 0, buf,
			  &len);
	  if (rc != U2FH_OK)
	    {
	      return rc;
	    }
	  else if (len != 2)
	    {
	      break;
	    }
	}
      if (len != 2)
	{
	  break;
	}
      Sleep (1000);
    }
  while ((flags & U2FH_REQUEST_USER_PRESENCE)
	 && len == 2 && memcmp (buf, NOTSATISFIED, 2) == 0);

  if (len != 2)
    {
      prepare_response (buf, len - 2, bd, response, response_len);
      return U2FH_OK;
    }
  return U2FH_TRANSPORT_ERROR;
}

/**
 * u2fh_register2:
 * @devs: a device set handle, from u2fh_devs_init() and u2fh_devs_discover().
 * @challenge: string with JSON data containing the challenge.
 * @origin: U2F origin URL.
 * @response: pointer to output string with JSON data.
 * @response_len: pointer to length of @response
 * @flags: set of ORed #u2fh_cmdflags values.
 *
 * Perform the U2F Register operation.
 *
 * Returns: On success %U2FH_OK (integer 0) is returned, and on errors
 * an #u2fh_rc error code.
 */
u2fh_rc
u2fh_register2 (u2fh_devs * devs,
		const char *challenge,
		const char *origin, char *response, size_t * response_len,
		u2fh_cmdflags flags)
{
  return _u2fh_register (devs, challenge, origin, &response, response_len,
			 flags);
}

/**
 * u2fh_register:
 * @devs: a device set handle, from u2fh_devs_init() and u2fh_devs_discover().
 * @challenge: string with JSON data containing the challenge.
 * @origin: U2F origin URL.
 * @response: pointer to pointer for output data
 * @flags: set of ORed #u2fh_cmdflags values.
 *
 * Perform the U2F Register operation.
 *
 * Returns: On success %U2FH_OK (integer 0) is returned, and on errors
 * an #u2fh_rc error code.
 */
u2fh_rc
u2fh_register (u2fh_devs * devs,
	       const char *challenge,
	       const char *origin, char **response, u2fh_cmdflags flags)
{
  size_t response_len = 0;
  *response = NULL;
  return _u2fh_register (devs, challenge, origin, response, &response_len,
			 flags);
}
