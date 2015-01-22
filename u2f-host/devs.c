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

#include <stdlib.h>
#include <unistd.h>
#ifdef __linux
#include <string.h>
#include <sys/ioctl.h>
#include <sys/fcntl.h>
#include <linux/hidraw.h>
#endif

#ifdef __linux
static uint32_t
get_bytes (uint8_t * rpt, size_t len, size_t num_bytes, size_t cur)
{
  /* Return if there aren't enough bytes. */
  if (cur + num_bytes >= len)
    return 0;

  if (num_bytes == 0)
    return 0;
  else if (num_bytes == 1)
    {
      return rpt[cur + 1];
    }
  else if (num_bytes == 2)
    {
      return (rpt[cur + 2] * 256 + rpt[cur + 1]);
    }
  else
    return 0;
}

static int
get_usage (uint8_t * report_descriptor, size_t size,
	   unsigned short *usage_page, unsigned short *usage)
{
  size_t i = 0;
  int size_code;
  int data_len, key_size;
  int usage_found = 0, usage_page_found = 0;

  while (i < size)
    {
      int key = report_descriptor[i];
      int key_cmd = key & 0xfc;

      if ((key & 0xf0) == 0xf0)
	{
	  fprintf (stderr, "invalid data received.\n");
	  return -1;
	}
      else
	{
	  size_code = key & 0x3;
	  switch (size_code)
	    {
	    case 0:
	    case 1:
	    case 2:
	      data_len = size_code;
	      break;
	    case 3:
	      data_len = 4;
	      break;
	    default:
	      /* Can't ever happen since size_code is & 0x3 */
	      data_len = 0;
	      break;
	    };
	  key_size = 1;
	}

      if (key_cmd == 0x4)
	{
	  *usage_page = get_bytes (report_descriptor, size, data_len, i);
	  usage_page_found = 1;
	}
      if (key_cmd == 0x8)
	{
	  *usage = get_bytes (report_descriptor, size, data_len, i);
	  usage_found = 1;
	}

      if (usage_page_found && usage_found)
	return 0;		/* success */

      i += data_len + key_size;
    }

  return -1;			/* failure */
}
#endif

static int
get_usages (struct hid_device_info *dev, unsigned short *usage_page,
	    unsigned short *usage)
{
#ifdef __linux
  int res, desc_size;
  int ret = U2FH_TRANSPORT_ERROR;
  struct hidraw_report_descriptor rpt_desc;
  int handle = open (dev->path, O_RDWR);
  if (handle > 0)
    {
      memset (&rpt_desc, 0, sizeof (rpt_desc));
      res = ioctl (handle, HIDIOCGRDESCSIZE, &desc_size);
      if (res >= 0)
	{
	  rpt_desc.size = desc_size;
	  res = ioctl (handle, HIDIOCGRDESC, &rpt_desc);
	  if (res >= 0)
	    {
	      res =
		get_usage (rpt_desc.value, rpt_desc.size, usage_page, usage);
	      if (res >= 0)
		{
		  ret = U2FH_OK;
		}
	    }
	}
      close (handle);
    }
  return ret;
#else
  *usage_page = dev->usage_page;
  *usage = dev->usage;
  return U2FH_OK;
#endif
}

static void
close_device (struct u2fdevice *dev)
{
  hid_close (dev->devh);
  free (dev->device_path);
  free (dev->device_string);
  dev->is_alive = 0;
}

static void
close_devices (u2fh_devs * devs)
{
  int i;
  if (devs == NULL || devs->num_devices == 0)
    {
      return;
    }

  for (i = 0; i < devs->num_devices; i++)
    {
      struct u2fdevice *dev = &devs->devs[i];
      if (dev->is_alive)
	{
	  close_device (dev);
	}
    }
  devs->num_devices = 0;
}

static int
init_device (u2fh_devs * devs, unsigned index)
{
  unsigned char resp[1024];
  /* FIXME: use something slightly more random as nonce */
  unsigned char nonce[] = { 0x8, 0x7, 0x6, 0x5, 0x4, 0x3, 0x2, 0x1 };
  size_t resplen = sizeof (resp);
  devs->devs[index].cid = CID_BROADCAST;

  if (u2fh_sendrecv
      (devs, index, U2FHID_INIT, nonce, sizeof (nonce), resp,
       &resplen) == U2FH_OK)
    {
      U2FHID_INIT_RESP initresp;
      memcpy (&initresp, resp, resplen);
      devs->devs[index].cid = initresp.cid;
      devs->devs[index].versionInterface = initresp.versionInterface;
      devs->devs[index].versionMajor = initresp.versionMajor;
      devs->devs[index].versionMinor = initresp.versionMinor;
      devs->devs[index].capFlags = initresp.capFlags;
    }
  else
    {
      return U2FH_TRANSPORT_ERROR;
    }
  return U2FH_OK;
}

static int
ping_device (u2fh_devs * devs, unsigned index)
{
  unsigned char data[1] = { 0 };
  unsigned char resp[1024];
  size_t resplen = sizeof (resp);
  return u2fh_sendrecv (devs, index, U2FHID_PING, data, sizeof (data), resp,
			&resplen);
}


/**
 * u2fh_devs_init:
 * @devs: pointer to #u2fh_devs type to initialize.
 *
 * Initialize device handle.
 *
 * Returns: On success %U2FH_OK (integer 0) is returned, on memory
 * allocation errors %U2FH_MEMORY_ERROR is returned, or another
 * #u2fh_rc error code is returned.
 */
u2fh_rc
u2fh_devs_init (u2fh_devs ** devs)
{
  u2fh_devs *d;
  int rc;

  d = malloc (sizeof (*d));
  if (d == NULL)
    return U2FH_MEMORY_ERROR;

  memset (d, 0, sizeof (*d));

  rc = hid_init ();
  if (rc != 0)
    {
      free (d);
      return U2FH_TRANSPORT_ERROR;
    }

  *devs = d;

  return U2FH_OK;
}

/**
 * u2fh_devs_discover:
 * @devs: device handle, from u2fh_devs_init().
 * @max_index: will on return be set to the maximum index, may be NULL; if
 *   there is 1 device this will be 0, if there are 2 devices this
 *   will be 1, and so on.
 *
 * Discover and open new devices.  This function can safely be called
 * several times and will free resources associated with unplugged
 * devices and open new.
 *
 * Returns: On success, %U2FH_OK (integer 0) is returned, when no U2F
 *   device could be found %U2FH_NO_U2F_DEVICE is returned, or another
 *   #u2fh_rc error code.
 */
u2fh_rc
u2fh_devs_discover (u2fh_devs * devs, unsigned *max_index)
{
  struct hid_device_info *di, *cur_dev;
  u2fh_rc res = U2FH_NO_U2F_DEVICE;
  unsigned index;

  di = hid_enumerate (0, 0);
  for (cur_dev = di; cur_dev; cur_dev = cur_dev->next)
    {
      int found = 0;
      unsigned short usage_page = 0, usage = 0;

      for (index = 0; index < devs->num_devices; index++)
	{
	  struct u2fdevice *dev = &devs->devs[index];
	  if (!dev->is_alive)
	    {
	      continue;
	    }
	  if (strcmp (dev->device_path, cur_dev->path) == 0)
	    {
	      if (ping_device (devs, index) == U2FH_OK)
		{
		  found = 1;
		  res = U2FH_OK;
		  break;
		}
	      else
		{
		  if (debug)
		    {
		      fprintf (stderr, "Device %s failed ping, dead.\n",
			       dev->device_path);
		    }
		  close_device (dev);
		  break;
		}
	    }
	}
      if (found)
	{
	  continue;
	}

      get_usages (cur_dev, &usage_page, &usage);
      if (usage_page == FIDO_USAGE_PAGE && usage == FIDO_USAGE_U2FHID)
	{
	  devs->devs =
	    realloc (devs->devs,
		     (devs->num_devices + 1) * sizeof (struct u2fdevice));
	  struct u2fdevice *dev = &devs->devs[devs->num_devices];
	  memset (dev, 0, sizeof (struct u2fdevice));
	  dev->devh = hid_open_path (cur_dev->path);
	  if (dev->devh != NULL)
	    {
	      dev->device_path = strdup (cur_dev->path);
	      if (dev->device_path == NULL)
		{
		  close_device (dev);
		  return U2FH_MEMORY_ERROR;
		}
	      index = devs->num_devices++;
	      dev->is_alive = 1;
	      if (init_device (devs, index) == U2FH_OK)
		{
		  if (cur_dev->product_string)
		    {
		      size_t len =
			wcstombs (NULL, cur_dev->product_string, 0);
		      dev->device_string = malloc (len + 1);
		      if (dev->device_string == NULL)
			{
			  close_device (dev);
			  return U2FH_MEMORY_ERROR;
			}
		      memset (dev->device_string, 0, len + 1);
		      wcstombs (dev->device_string, cur_dev->product_string,
				len);
		      if (debug)
			{
			  fprintf (stderr, "device %s discovered as '%s'\n",
				   dev->device_path, dev->device_string);
			  fprintf (stderr,
				   "  version (Interface, Major, "
				   "Minor, Build): %d, %d, "
				   "%d, %d  capFlags: %d\n",
				   dev->versionInterface,
				   dev->versionMajor,
				   dev->versionMinor,
				   dev->versionBuild, dev->capFlags);
			}
		    }
		  res = U2FH_OK;
		}
	      else
		{
		  close_device (dev);
		  devs->num_devices--;
		}
	    }
	}
    }

  for (index = 0; index < devs->num_devices; index++)
    {
      struct u2fdevice *dev = &devs->devs[index];
      int found = 0;

      if (!dev->is_alive)
	{
	  continue;
	}
      for (cur_dev = di; cur_dev; cur_dev = cur_dev->next)
	{
	  if (strcmp (cur_dev->path, dev->device_path) == 0)
	    {
	      found = 1;
	    }
	}
      if (!found)
	{
	  if (debug)
	    {
	      fprintf (stderr, "device %s looks dead.\n", dev->device_path);
	    }
	  close_device (dev);
	}
    }

  hid_free_enumeration (di);
  if (res == U2FH_OK && max_index)
    *max_index = devs->num_devices - 1;

  return res;
}

/**
 * u2fh_devs_done:
 * @devs: device handle, from u2fh_devs_init().
 *
 * Release all resources associated with @devs.  This function must be
 * called when you are finished with a device handle.
 */
void
u2fh_devs_done (u2fh_devs * devs)
{
  if (devs == NULL)
    return;

  close_devices (devs);
  free (devs->devs);
  hid_exit ();

  free (devs);
}

/**
 * u2fh_get_device_description:
 * @devs: device_handle, from u2fh_devs_init().
 * @index: index of device
 * @out: buffer for storing device description
 * @len: maximum amount of data to store in @out. Will be updated.
 *
 * Get the device description of the device at @index. Stores the
 * string in @out.
 *
 * Returns: %U2FH_OK on success.
 */
u2fh_rc
u2fh_get_device_description (u2fh_devs * devs, unsigned index, char *out,
			     size_t * len)
{
  struct u2fdevice *dev;
  size_t i;

  if (index >= devs->num_devices || !devs->devs[index].is_alive)
    {
      return U2FH_NO_U2F_DEVICE;
    }

  dev = &devs->devs[index];
  i = strlen (dev->device_string);
  if (i < *len)
    {
      *len = i;
    }
  else
    {
      return U2FH_MEMORY_ERROR;
    }
  strcpy (out, dev->device_string);
  return U2FH_OK;
}

/**
 * u2fh_is_alive:
 * @devs: device_handle, from u2fh_devs_init().
 * @index: index of device
 *
 * Get the liveliness of the device @index.
 *
 * Returns: 1 if the device is considered alive, 0 otherwise.
 */
int
u2fh_is_alive (u2fh_devs * devs, unsigned index)
{
  if (index >= devs->num_devices || !devs->devs[index].is_alive)
    return 0;
  return 1;
}
