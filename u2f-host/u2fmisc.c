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

#include <json.h>

#include "sha256.h"

#define RESPHEAD_SIZE 7
#define HID_TIMEOUT 2
#define HID_MAX_TIMEOUT 4096

#ifdef HAVE_JSON_OBJECT_OBJECT_GET_EX
#define u2fh_json_object_object_get(obj, key, value) json_object_object_get_ex(obj, key, &value)
#else
typedef int json_bool;
#define u2fh_json_object_object_get(obj, key, value) (value = json_object_object_get(obj, key)) == NULL ? (json_bool)FALSE : (json_bool)TRUE
#endif

static void
dumpHex (unsigned char *data, int offs, int len)
{
  int i;
  for (i = offs; i < len; i++)
    {
      fprintf (stderr, "%02x", data[i] & 0xFF);
    }
  fprintf (stderr, "\n");
}

int
prepare_browserdata (const char *challenge, const char *origin,
		     const char *typstr, char *out, size_t * outlen)
{
  int rc = U2FH_JSON_ERROR;
  struct json_object *jo = NULL;
  struct json_object *chal = NULL, *orig = NULL, *typ = NULL;
  const char *buf;
  size_t len;

  chal = json_object_new_string (challenge);
  if (chal == NULL)
    goto done;

  orig = json_object_new_string (origin);
  if (orig == NULL)
    goto done;

  typ = json_object_new_string (typstr);
  if (typ == NULL)
    goto done;

  jo = json_object_new_object ();
  if (jo == NULL)
    goto done;

  json_object_object_add (jo, "challenge", chal);
  json_object_object_add (jo, "origin", orig);
  json_object_object_add (jo, "typ", typ);

  if (debug)
    {
      fprintf (stderr, "client data: %s\n", json_object_to_json_string (jo));
    }

  buf = json_object_to_json_string (jo);
  len = strlen (buf);
  if (len >= *outlen)
    {
      rc = U2FH_MEMORY_ERROR;
      goto done;
    }

  strcpy (out, buf);
  *outlen = len;
  rc = U2FH_OK;

done:
  json_object_put (jo);
  if (!jo)
    {
      json_object_put (chal);
      json_object_put (orig);
      json_object_put (typ);
    }
  return rc;
}

int
prepare_origin (const char *jsonstr, unsigned char *p)
{
  const char *app_id;
  struct json_object *jo;
  struct json_object *k;

  jo = json_tokener_parse (jsonstr);
  if (jo == NULL)
    return U2FH_MEMORY_ERROR;

  if (debug)
    fprintf (stderr, "JSON: %s\n", json_object_to_json_string (jo));

  if (u2fh_json_object_object_get (jo, "appId", k) == FALSE)
    return U2FH_JSON_ERROR;

  app_id = json_object_get_string (k);
  if (app_id == NULL)
    return U2FH_JSON_ERROR;

  if (debug)
    fprintf (stderr, "JSON app_id %s\n", app_id);

  sha256_buffer (app_id ? app_id : "", app_id ? strlen (app_id) : 0, p);

  json_object_put (jo);

  return U2FH_OK;
}

/**
 * u2fh_sendrecv:
 * @devs: device handle, from u2fh_devs_init().
 * @index: index of device
 * @cmd: command to run
 * @send: buffer of data to send
 * @sendlen: length of data to send
 * @recv: buffer of data to receive
 * @recvlen: length of data to receive
 *
 * Send a command with data to the device at @index.
 *
 * Returns: %U2FH_OK on success, another #u2fh_rc error code
 * otherwise.
 */
u2fh_rc
u2fh_sendrecv (u2fh_devs * devs, unsigned index, uint8_t cmd,
	       const unsigned char *send, uint16_t sendlen,
	       unsigned char *recv, size_t * recvlen)
{
  int datasent = 0;
  int sequence = 0;
  struct u2fdevice *dev = get_device (devs, index);

  if (!dev)
    {
      return U2FH_NO_U2F_DEVICE;
    }

  while (sendlen > datasent)
    {
      U2FHID_FRAME frame = { 0 };
      {
	int len = sendlen - datasent;
	unsigned int maxlen;
	unsigned char *data;
	frame.cid = dev->cid;
	if (datasent == 0)
	  {
	    frame.init.cmd = cmd;
	    frame.init.bcnth = (sendlen >> 8) & 0xff;
	    frame.init.bcntl = sendlen & 0xff;
	    data = frame.init.data;
	    maxlen = sizeof (frame.init.data);
	  }
	else
	  {
	    frame.cont.seq = sequence++;
	    data = frame.cont.data;
	    maxlen = sizeof (frame.cont.data);
	  }
	if (len > maxlen)
	  {
	    len = maxlen;
	  }
	memcpy (data, send + datasent, len);
	datasent += len;
      }

      {
	unsigned char data[sizeof (U2FHID_FRAME) + 1];
	int len;
	/* FIXME: add report as first byte, is report 0 correct? */
	data[0] = 0;
	memcpy (data + 1, &frame, sizeof (U2FHID_FRAME));
	if (debug)
	  {
	    fprintf (stderr, "USB send: ");
	    dumpHex (data, 0, sizeof (U2FHID_FRAME));
	  }

	len = hid_write (dev->devh, data, sizeof (U2FHID_FRAME) + 1);
	if (debug)
	  fprintf (stderr, "USB write returned %d\n", len);
	if (len < 0)
	  return U2FH_TRANSPORT_ERROR;
	if (sizeof (U2FHID_FRAME) + 1 != len)
	  return U2FH_TRANSPORT_ERROR;
      }
    }

  {
    U2FHID_FRAME frame;
    unsigned char data[HID_RPT_SIZE];
    int len = HID_RPT_SIZE;
    unsigned int maxlen = *recvlen;
    int recvddata = 0;
    unsigned short datalen;
    int timeout = HID_TIMEOUT;
    int rc = 0;

    while (rc == 0)
      {
	if (debug)
	  {
	    fprintf (stderr, "now trying with timeout %d\n", timeout);
	  }
	rc = hid_read_timeout (dev->devh, data, len, timeout);
	timeout *= 2;
	if (timeout > HID_MAX_TIMEOUT)
	  {
	    rc = -2;
	    break;
	  }
      }
    sequence = 0;

    if (debug)
      {
	fprintf (stderr, "USB read rc read %d\n", len);
	if (rc > 0)
	  {
	    fprintf (stderr, "USB recv: ");
	    dumpHex (data, 0, rc);
	  }
      }
    if (rc < 0)
      {
	return U2FH_TRANSPORT_ERROR;
      }

    memcpy (&frame, data, HID_RPT_SIZE);
    if (frame.cid != dev->cid || frame.init.cmd != cmd)
      {
	return U2FH_TRANSPORT_ERROR;
      }
    datalen = frame.init.bcnth << 8 | frame.init.bcntl;
    if (datalen + datalen % HID_RPT_SIZE > maxlen)
      {
	return U2FH_TRANSPORT_ERROR;
      }
    memcpy (recv, frame.init.data, sizeof (frame.init.data));
    recvddata = sizeof (frame.init.data);

    while (datalen > recvddata)
      {
	timeout = HID_TIMEOUT;
	rc = 0;
	while (rc == 0)
	  {
	    if (debug)
	      {
		fprintf (stderr, "now trying with timeout %d\n", timeout);
	      }
	    rc = hid_read_timeout (dev->devh, data, len, timeout);
	    timeout *= 2;
	    if (timeout > HID_MAX_TIMEOUT)
	      {
		rc = -2;
		break;
	      }
	  }
	if (debug)
	  {
	    fprintf (stderr, "USB read rc read %d\n", len);
	    if (rc > 0)
	      {
		fprintf (stderr, "USB recv: ");
		dumpHex (data, 0, rc);
	      }
	  }
	if (rc < 0)
	  {
	    return U2FH_TRANSPORT_ERROR;
	  }

	memcpy (&frame, data, HID_RPT_SIZE);
	if (frame.cid != dev->cid || frame.cont.seq != sequence++)
	  {
	    fprintf (stderr, "bar: %d %d %d %d\n", frame.cid, dev->cid,
		     frame.cont.seq, sequence);
	    return U2FH_TRANSPORT_ERROR;
	  }
	memcpy (recv + recvddata, frame.cont.data, sizeof (frame.cont.data));
	recvddata += sizeof (frame.cont.data);
      }
    *recvlen = datalen;
  }
  return U2FH_OK;
}

u2fh_rc
send_apdu (u2fh_devs * devs, int index, int cmd, const unsigned char *d,
	   size_t dlen, int p1, unsigned char *out, size_t * outlen)
{
  unsigned char data[2048];
  int rc;

  if (dlen > MAXDATASIZE)
    return U2FH_MEMORY_ERROR;

  sprintf (data, "%c%c%c%c%c%c%c", 0, cmd, p1, 0, 0, (dlen >> 8) & 0xff,
	   dlen & 0xff);
  memcpy (data + RESPHEAD_SIZE, d, dlen);
  memset (data + RESPHEAD_SIZE + dlen, 0, 2);

  rc =
    u2fh_sendrecv (devs, index, U2FHID_MSG, data, RESPHEAD_SIZE + dlen + 2,
		   out, outlen);
  if (rc != U2FH_OK)
    {
      if (debug)
	fprintf (stderr, "USB rc %d\n", rc);
      return rc;
    }
  if (*outlen < 2)
    {
      if (debug)
	fprintf (stderr, "USB read too short\n");
      return U2FH_TRANSPORT_ERROR;
    }

  if (*outlen > MAXDATASIZE)
    {
      if (debug)
	fprintf (stderr, "USB too large response?\n");
      return U2FH_MEMORY_ERROR;
    }

  /* FIXME: Improve APDU error handling? */

  if (debug)
    {
      fprintf (stderr, "USB data (len %zu): ", *outlen);
      dumpHex (out, 0, *outlen);
    }

  return U2FH_OK;
}

int
get_fixed_json_data (const char *jsonstr, const char *key, char *p,
		     size_t * len)
{
  struct json_object *jo;
  struct json_object *k;
  const char *urlb64;

  jo = json_tokener_parse (jsonstr);
  if (jo == NULL)
    return U2FH_JSON_ERROR;

  if (debug)
    fprintf (stderr, "JSON: %s\n", json_object_to_json_string (jo));

  if (u2fh_json_object_object_get (jo, key, k) == FALSE)
    return U2FH_JSON_ERROR;

  urlb64 = json_object_get_string (k);
  if (urlb64 == NULL)
    return U2FH_JSON_ERROR;

  if (debug)
    fprintf (stderr, "JSON %s URL-B64: %s\n", key, urlb64);

  if (strlen (urlb64) >= *len)
    {
      return U2FH_JSON_ERROR;
    }
  *len = strlen (urlb64);

  strcpy (p, urlb64);

  json_object_put (jo);

  return U2FH_OK;
}
