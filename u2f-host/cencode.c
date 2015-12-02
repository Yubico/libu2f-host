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

/*
cencoder.c - c source to a base64 encoding algorithm implementation

This is part of the libb64 project, and has been placed in the public domain.
For details, see http://sourceforge.net/projects/libb64
*/

#include <b64/cencode.h>

const int CHARS_PER_LINE = 72;

void
base64_init_encodestate (base64_encodestate * state_in)
{
  state_in->step = step_A;
  state_in->result = 0;
  state_in->stepcount = 0;
}

char
base64_encode_value (char value_in)
{
  static const char *encoding =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
  if (value_in > 63)
    return '=';
  return encoding[(int) value_in];
}

int
base64_encode_block (const char *plaintext_in, int length_in, char *code_out,
		     base64_encodestate * state_in)
{
  const char *plainchar = plaintext_in;
  const char *const plaintextend = plaintext_in + length_in;
  char *codechar = code_out;
  char result;
  char fragment;

  result = state_in->result;

  switch (state_in->step)
    {
      while (1)
	{
    case step_A:
	  if (plainchar == plaintextend)
	    {
	      state_in->result = result;
	      state_in->step = step_A;
	      return codechar - code_out;
	    }
	  fragment = *plainchar++;
	  result = (fragment & 0x0fc) >> 2;
	  *codechar++ = base64_encode_value (result);
	  result = (fragment & 0x003) << 4;
    case step_B:
	  if (plainchar == plaintextend)
	    {
	      state_in->result = result;
	      state_in->step = step_B;
	      return codechar - code_out;
	    }
	  fragment = *plainchar++;
	  result |= (fragment & 0x0f0) >> 4;
	  *codechar++ = base64_encode_value (result);
	  result = (fragment & 0x00f) << 2;
    case step_C:
	  if (plainchar == plaintextend)
	    {
	      state_in->result = result;
	      state_in->step = step_C;
	      return codechar - code_out;
	    }
	  fragment = *plainchar++;
	  result |= (fragment & 0x0c0) >> 6;
	  *codechar++ = base64_encode_value (result);
	  result = (fragment & 0x03f) >> 0;
	  *codechar++ = base64_encode_value (result);

	  ++(state_in->stepcount);
	  if (state_in->stepcount == CHARS_PER_LINE / 4)
	    {
	      /* *codechar++ = '\n'; */
	      state_in->stepcount = 0;
	    }
	}
    }
  /* control should not reach here */
  return codechar - code_out;
}

int
base64_encode_blockend (char *code_out, base64_encodestate * state_in)
{
  char *codechar = code_out;

  switch (state_in->step)
    {
    case step_B:
    case step_C:
      *codechar++ = base64_encode_value (state_in->result);
      break;
    case step_A:
      break;
    }
  *codechar++ = '\0';

  return codechar - code_out;
}
