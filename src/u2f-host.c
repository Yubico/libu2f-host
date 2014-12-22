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
#include <u2f-host/u2f-host.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "cmdline.h"

int
main (int argc, char *argv[])
{
  int exit_code = EXIT_FAILURE;
  struct gengetopt_args_info args_info;
  char challenge[BUFSIZ];
  size_t chal_len;
  char *response = NULL;
  u2fh_devs *devs = NULL;
  u2fh_cmdflags flags = 0;
  u2fh_rc rc;

  if (cmdline_parser (argc, argv, &args_info) != 0)
    exit (EXIT_FAILURE);

  if (args_info.help_given)
    {
      cmdline_parser_print_help ();
      printf ("\nReport bugs at <https://github.com/Yubico/libu2f-host>.\n");
      exit (EXIT_SUCCESS);
    }

  chal_len = fread (challenge, 1, sizeof (challenge), stdin);
  if (!feof (stdin) || ferror (stdin))
    {
      perror ("read");
      exit (EXIT_FAILURE);
    }

  rc = u2fh_global_init (args_info.debug_flag ? U2FH_DEBUG : 0);
  if (rc != U2FH_OK)
    {
      fprintf (stderr, "error: u2fh_global_init (%d): %s\n", rc, u2fh_strerror (rc));
      exit (EXIT_FAILURE);
    }

  rc = u2fh_devs_init (&devs);
  if (rc != U2FH_OK)
    {
      fprintf (stderr, "error: u2fh_devs_init (%d): %s\n", rc, u2fh_strerror (rc));
      goto done;
    }

  rc = u2fh_devs_discover (devs, NULL);
  if (rc != U2FH_OK)
    {
      fprintf (stderr, "error: u2fh_devs_discover (%d): %s\n", rc, u2fh_strerror (rc));
      goto done;
    }

  switch (args_info.action_arg)
    {
    case action_arg_register:
    case action_arg_authenticate:
      if (args_info.origin_arg == NULL)
	{
	  fprintf (stderr, "error: origin URL empty, use -o to specify it\n");
	  exit (EXIT_FAILURE);
	}
      else if (strncmp ("http://", args_info.origin_arg, 7) != 0
	       && strncmp ("https://", args_info.origin_arg, 8) != 0)
	{
	  fprintf (stderr, "error: origin must be http or https\n");
	  exit (EXIT_FAILURE);
	}

      if (args_info.action_arg == action_arg_register)
	{
	  rc = u2fh_register (devs, challenge, args_info.origin_arg,
			      &response,
			      args_info.touch_flag ? 0 :
			      U2FH_REQUEST_USER_PRESENCE);
	}
      else
	{
	  rc = u2fh_authenticate (devs, challenge, args_info.origin_arg,
				  &response, args_info.touch_flag ? 0 :
				  U2FH_REQUEST_USER_PRESENCE);
	}
      break;
    case action_arg_sendrecv:
      {
	uint8_t command;
	unsigned char out[2048];
	size_t outlen = sizeof (out);
	if (args_info.command_arg == NULL)
	  {
	    fprintf (stderr, "error: empty sendrecv command.\n");
	    exit (EXIT_FAILURE);
	  }
	sscanf (args_info.command_arg, "%hhx", &command);
	rc =
	  u2fh_sendrecv (devs, 0, command, challenge, chal_len - 1, out,
			 &outlen);
      }
      break;
    case action__NULL:
    default:
      fprintf (stderr, "error: unknown action.\n");
      goto done;
    }
  if (rc != U2FH_OK || response == NULL)
    {
      fprintf (stderr, "error (%d): %s\n", rc, u2fh_strerror (rc));
      goto done;
    }

  printf ("%s\n", response);

  exit_code = EXIT_SUCCESS;

done:
  u2fh_devs_done (devs);
  u2fh_global_done ();

  exit (exit_code);
}
