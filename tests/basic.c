/*
  Copyright (C) 2013-2015 Yubico AB

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
#include <u2f-host.h>

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int
main (void)
{
  u2fh_devs *devs;
  int rc;

  if (strcmp (U2FH_VERSION_STRING, u2fh_check_version (NULL)) != 0)
    {
      printf ("version mismatch %s != %s\n", U2FH_VERSION_STRING,
	      u2fh_check_version (NULL));
      return EXIT_FAILURE;
    }

  if (u2fh_check_version (U2FH_VERSION_STRING) == NULL)
    {
      printf ("version NULL?\n");
      return EXIT_FAILURE;
    }

  if (u2fh_check_version ("99.99.99") != NULL)
    {
      printf ("version not NULL?\n");
      return EXIT_FAILURE;
    }

  printf ("u2fh version: header %s library %s\n",
	  U2FH_VERSION_STRING, u2fh_check_version (NULL));

  rc = u2fh_global_init (0);
  if (rc != U2FH_OK)
    {
      printf ("u2fh_global_init rc %d\n", rc);
      return EXIT_FAILURE;
    }

  if (u2fh_strerror (U2FH_OK) == NULL)
    {
      printf ("u2fh_strerror NULL\n");
      return EXIT_FAILURE;
    }

  {
    const char *s;
    s = u2fh_strerror_name (U2FH_OK);
    if (s == NULL || strcmp (s, "U2FH_OK") != 0)
      {
	printf ("u2fh_strerror_name %s\n", s);
	return EXIT_FAILURE;
      }
  }

  rc = u2fh_devs_init (&devs);
  if (rc != U2FH_OK)
    {
      printf ("u2fh_devs_init %d\n", rc);
      return EXIT_FAILURE;
    }

  rc = u2fh_devs_discover (devs, NULL);
  if (rc == U2FH_OK)
    {
      printf ("Found U2F device\n");
      /* XXX: register+authenticate */
    }
  else if (rc != U2FH_NO_U2F_DEVICE)
    {
      printf ("u2fh_devs_discover %d\n", rc);
      return EXIT_FAILURE;
    }

  u2fh_devs_done (devs);

  u2fh_global_done ();

  return EXIT_SUCCESS;
}
