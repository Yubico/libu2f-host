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
#include "internal.h"

#include <unistd.h>
#include <json.h>
#include "b64/cencode.h"
#include "b64/cdecode.h"
#include "sha256.h"

static int
prepare_response2 (const char *encstr, const char *bdstr, const char *input,
		   char **response)
{
  int rc = U2FH_JSON_ERROR;
  struct json_object *jo = NULL, *enc = NULL, *bd = NULL, *key = NULL;
  char keyb64[256];
  size_t keylen = sizeof (keyb64);

  enc = json_object_new_string (encstr);
  if (enc == NULL)
    goto done;
  bd = json_object_new_string (bdstr);
  if (bd == NULL)
    goto done;

  rc = get_fixed_json_data (input, "keyHandle", keyb64, &keylen);
  if (rc != U2FH_OK)
    {
      goto done;
    }

  rc = U2FH_JSON_ERROR;
  key = json_object_new_string (keyb64);
  if (key == NULL)
    goto done;

  jo = json_object_new_object ();
  if (jo == NULL)
    goto done;

  json_object_object_add (jo, "signatureData", enc);
  json_object_object_add (jo, "clientData", bd);
  json_object_object_add (jo, "keyHandle", key);

  *response = strdup (json_object_to_json_string (jo));
  if (*response == NULL)
    rc = U2FH_MEMORY_ERROR;
  else
    rc = U2FH_OK;

done:
  json_object_put (jo);
  json_object_put (enc);
  json_object_put (bd);
  json_object_put (key);

  return rc;
}

static int
prepare_response (const unsigned char *buf, int len, const char *bd,
		  const char *input, char **response)
{
  base64_encodestate b64ctx;
  char b64enc[2048];
  char bdstr[2048];
  int cnt;

  if (len > 2048)
    return U2FH_MEMORY_ERROR;
  if (strlen (bd) > 2048)
    return U2FH_MEMORY_ERROR;

  base64_init_encodestate (&b64ctx);
  cnt = base64_encode_block (buf, len, b64enc, &b64ctx);
  base64_encode_blockend (b64enc + cnt, &b64ctx);

  base64_init_encodestate (&b64ctx);
  cnt = base64_encode_block (bd, strlen (bd), bdstr, &b64ctx);
  base64_encode_blockend (bdstr + cnt, &b64ctx);

  return prepare_response2 (b64enc, bdstr, input, response);
}

#define CHALLBINLEN 32
#define HOSIZE 32
#define MAXKHLEN 128
#define NOTSATISFIED "\x69\x85"

/**
 * u2fh_authenticate:
 * @devs: a device handle, from u2fh_devs_init() and u2fh_devs_discover().
 * @challenge: string with JSON data containing the challenge.
 * @origin: U2F origin URL.
 * @response: pointer to output string with JSON data.
 * @flags: set of ORed #u2fh_cmdflags values.
 *
 * Perform the U2F Authenticate operation.
 *
 * Returns: On success %U2FH_OK (integer 0) is returned, and on errors
 * an #u2fh_rc error code.
 */
u2fh_rc
u2fh_authenticate (u2fh_devs * devs,
		   const char *challenge,
		   const char *origin, char **response, u2fh_cmdflags flags)
{
  unsigned char data[CHALLBINLEN + HOSIZE + MAXKHLEN + 1];
  unsigned char buf[MAXDATASIZE];
  char bd[2048];
  size_t bdlen = sizeof (bd);
  size_t len;
  int rc;
  char chalb64[256];
  size_t challen = sizeof (chalb64);
  char khb64[256];
  size_t kh64len = sizeof (khb64);
  base64_decodestate b64;
  size_t khlen;

  rc = get_fixed_json_data (challenge, "challenge", chalb64, &challen);
  if (rc != U2FH_OK)
    return rc;

  rc = prepare_browserdata (chalb64, origin, AUTHENTICATE_TYP, bd, &bdlen);
  if (rc != U2FH_OK)
    return rc;

  sha256_buffer (bd, bdlen, data);

  prepare_origin (challenge, data + CHALLBINLEN);

  /* confusion between key_handle and keyHandle */
  rc = get_fixed_json_data (challenge, "keyHandle", khb64, &kh64len);
  if (rc != U2FH_OK)
    return rc;

  base64_init_decodestate (&b64);
  khlen = base64_decode_block (khb64, kh64len,
			       data + HOSIZE + CHALLBINLEN + 1, &b64);
  data[HOSIZE + CHALLBINLEN] = khlen;

  /* FIXME: Support asynchronous usage, through a new u2fh_cmdflags
     flag. */

  do
    {
      int i;
      for (i = 0; i < devs->num_devices; i++)
	{
          if(!devs->devs[i].is_alive) {
            continue;
          }
	  len = MAXDATASIZE;
	  rc = send_apdu (devs, i, U2F_AUTHENTICATE, data,
			  HOSIZE + CHALLBINLEN + khlen + 1,
			  flags & U2FH_REQUEST_USER_PRESENCE ? 3 : 7, buf,
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

      sleep (1);
    }
  while ((flags & U2FH_REQUEST_USER_PRESENCE)
	 && len == 2 && memcmp (buf, NOTSATISFIED, 2) == 0);

  prepare_response (buf, len - 2, bd, challenge, response);

  return U2FH_OK;
}
