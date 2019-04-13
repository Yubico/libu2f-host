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

#ifndef INTERNAL_H
#define INTERNAL_H

#include <u2f-host.h>
#include <hidapi.h>
#include <stdio.h>

#ifdef FEATURE_LIBNFC
#include <nfc/nfc.h>
#endif

#include "inc/u2f.h"
#include "inc/u2f_hid.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#define Sleep(x) (usleep((x) * 1000))
#endif

struct u2fdevice
{
  struct u2fdevice *next;
  hid_device *devh;
  unsigned id;
  uint32_t cid;
  char *device_string;
  char *device_path;
  int skipped;
  uint8_t versionInterface;	// Interface version
  uint8_t versionMajor;		// Major version number
  uint8_t versionMinor;		// Minor version number
  uint8_t versionBuild;		// Build version number
  uint8_t capFlags;		// Capabilities flags
};

struct u2fh_devs
{
  unsigned max_id;
  struct u2fdevice *first;
};

#ifdef FEATURE_LIBNFC

#define NFC_MAX_NUM_CONTROLLERS 4
#define NFC_MAX_RESPONSE_LEN 2048
#define NFC_TAG_NAME 0x71
#define NFC_TAG_VERSION 0x79
#define NFC_U2F_AID                                \
  {                                                \
    0xA0, 0x00, 0x00, 0x06, 0x47, 0x2F, 0x00, 0x01 \
  }
#define NFC_OATH_AID                               \
  {                                                \
    0xA0, 0x00, 0x00, 0x05, 0x27, 0x21, 0x01, 0x01 \
  }

struct u2fnfcdevice
{
  struct u2fnfcdevice *next;
  nfc_device *pnd;
  int skipped;
  uint8_t versionMajor;
  uint8_t versionMinor;
  uint8_t versionBuild;
};

struct u2fh_nfc_devs
{
  unsigned max_id;
  nfc_context *context;
  struct u2fnfcdevice *first;
};

u2fh_rc card_transmit (nfc_device *pnd, const uint8_t *capdu,
                       const size_t capdu_len, uint8_t *rapdu, size_t *rapdu_len);
u2fh_rc send_apdu_nfc_compat (struct u2fnfcdevice *dev, int cmd,
                              const unsigned char *data, size_t data_len,
                              int p1, unsigned char *out, size_t *out_len);
u2fh_rc send_apdu_nfc (nfc_device *pnd, uint8_t cla, uint8_t ins, uint8_t p1,
                       uint8_t p2, const unsigned char *data, uint8_t data_len,
                       uint8_t *resp, size_t *resp_len);

#endif

extern int debug;

#define MAXDATASIZE 16384

#define MAXFIXEDLEN 1024

#define REGISTER_TYP "navigator.id.finishEnrollment"
#define AUTHENTICATE_TYP "navigator.id.getAssertion"

#define CTAPHID_KEEPALIVE        (TYPE_INIT | 0x3b)	// Keepalive response

int prepare_browserdata (const char *challenge, const char *origin,
			 const char *typstr, char *out, size_t * outlen);
int prepare_origin (const char *origin, unsigned char *p);
u2fh_rc send_apdu (u2fh_devs * devs, int index, int cmd,
		   const unsigned char *d, size_t dlen, int p1,
		   unsigned char *out, size_t * outlen);
int get_fixed_json_data (const char *jsonstr, const char *key, char *p,
			 size_t * len);
int hash_data (const char *in, size_t len, unsigned char *out);

struct u2fdevice *get_device (u2fh_devs * devs, unsigned index);

#endif
