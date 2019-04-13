#include <config.h>

#ifdef FEATURE_LIBNFC
#include "internal.h"

#include <nfc/nfc.h>
#include <stdlib.h>

#define dlog(f_, ...) \
  if (debug)          \
  fprintf (stderr, (f_), ##__VA_ARGS__)

u2fh_rc
card_transmit (nfc_device *pnd, const uint8_t *capdu, const size_t capdu_len,
               uint8_t *rapdu, size_t *rapdu_len)
{
  int res;
  int total_res = 0;
  uint8_t cmd_get_resp[4] = { capdu[0], 0xC0, 0x00, 0x00 };

  size_t pos;
  dlog ("=> ");
  for (pos = 0; pos < capdu_len; pos++)
    dlog ("%02x ", capdu[pos]);
  dlog ("\n");

  memset (rapdu, 0, *rapdu_len);
  while ((res = nfc_initiator_transceive_bytes (
           pnd, total_res > 0 ? cmd_get_resp : capdu, total_res > 0 ? 4 : capdu_len,
           &rapdu[total_res], *rapdu_len - total_res, 1000)) >= 0)
    {
      total_res += res - 2;
      if (rapdu[total_res] != 0x61)
        break;
    }
  if (res < 0)
    {
      return U2FH_TRANSPORT_ERROR;
    }
  *rapdu_len = total_res + 2;

  dlog ("<= ");
  for (pos = 0; pos < *rapdu_len; pos++)
    dlog ("%02x ", rapdu[pos]);
  dlog ("\n");

  return U2FH_OK;
}

u2fh_rc
send_apdu_nfc (nfc_device *pnd, uint8_t cla, uint8_t ins, uint8_t p1, uint8_t p2,
               const unsigned char *data, uint8_t data_len, uint8_t *resp, size_t *resp_len)
{
  uint8_t *msg = malloc (4 + 1 + data_len);
  int ret;
  msg[0] = cla;
  msg[1] = ins;
  msg[2] = p1;
  msg[3] = p2;
  msg[4] = data_len;
  memcpy (&msg[5], data, data_len);
  ret = card_transmit (pnd, msg, 4 + 1 + data_len, resp, resp_len);
  free (msg);
  if (ret != U2FH_OK)
    return ret;
  if (*resp_len < 2 || resp[*resp_len - 2] != 0x90 || resp[*resp_len - 1] != 0x00)
    {
      return U2FH_AUTHENTICATOR_ERROR;
    }
  return U2FH_OK;
}

u2fh_rc
parse_tlv (const uint8_t data[], const size_t data_len, const uint8_t tag,
           size_t *offset, size_t *len)
{
  size_t pos = 0;
  while (pos < data_len)
    {
      if (data[pos] == tag)
        {
          *len = data[pos + 1];
          *offset = pos + 2;
          return U2FH_OK;
        }
      pos += data[pos + 1] + 2;
    }
  return U2FH_AUTHENTICATOR_ERROR;
}

u2fh_rc
u2fh_nfc_devs_init (u2fh_nfc_devs **devs)
{
  u2fh_nfc_devs *devices;
  devices = malloc (sizeof (*devices));
  *devs = devices;

  size_t num_devs;
  size_t i;
  nfc_device *pnd;
  nfc_connstring nfc_connstrings[NFC_MAX_NUM_CONTROLLERS];
  nfc_context *context;
  struct u2fnfcdevice *dev;
  struct u2fnfcdevice *curr_dev;

  devices->max_id = 0;

  nfc_init (&context);
  devices->context = context;
  num_devs = nfc_list_devices (context, nfc_connstrings, NFC_MAX_NUM_CONTROLLERS);
  if (!num_devs)
    return U2FH_NO_U2F_DEVICE;

  const nfc_modulation nm = {
    .nmt = NMT_ISO14443A,
    .nbr = NBR_106,
  };
  nfc_target ant[1];

  if (num_devs > NFC_MAX_NUM_CONTROLLERS)
    num_devs = NFC_MAX_NUM_CONTROLLERS;
  for (i = 0; i < num_devs; i++)
    {
      pnd = nfc_open (context, nfc_connstrings[i]);
      if (pnd != NULL)
        {
          if (nfc_initiator_list_passive_targets (pnd, nm, ant, 1) > 0)
            {
              curr_dev = malloc (sizeof (*curr_dev));
              curr_dev->next = NULL;
              curr_dev->skipped = 0;
              curr_dev->pnd = pnd;
              if (devices->max_id)
                {
                  curr_dev->next = dev;
                }
              else
                {
                  devices->first = curr_dev;
                }
              dev = curr_dev;
              devices->max_id++;
            }
          else
            {
              nfc_close (pnd);
            }
        }
    }
  if (!devices->max_id)
    return U2FH_NO_U2F_DEVICE;
  return U2FH_OK;
}

u2fh_rc
u2fh_nfc_devs_discover (u2fh_nfc_devs *devices)
{
  struct u2fnfcdevice *dev;
  int discovered_devs = 0;

  uint8_t aid_oath[] = NFC_OATH_AID;
  uint8_t aid_u2f[] = NFC_U2F_AID;
  uint8_t resp[NFC_MAX_RESPONSE_LEN];
  size_t resp_len;
  size_t version_offset;
  size_t version_len;

  dev = devices->first;
  while (dev != NULL)
    {
      resp_len = NFC_MAX_RESPONSE_LEN;
      if (send_apdu_nfc (dev->pnd, 0x00, 0xA4, 0x04, 0x00, aid_oath,
                         sizeof (aid_oath), resp, &resp_len) < 0)
        {
          dev->skipped = 1;
          dev = dev->next;
          continue;
        }
      if (parse_tlv (resp, resp_len, NFC_TAG_VERSION, &version_offset, &version_len) < 0 ||
          version_len != 3)
        {
          dev->skipped = 1;
          dev = dev->next;
          continue;
        }
      dev->versionMajor = resp[version_offset++];
      dev->versionMinor = resp[version_offset++];
      dev->versionBuild = resp[version_offset];
      resp_len = NFC_MAX_RESPONSE_LEN;
      if (send_apdu_nfc (dev->pnd, 0x00, 0xA4, 0x04, 0x00, aid_u2f,
                         sizeof (aid_u2f), resp, &resp_len) < 0)
        {
          dev->skipped = 1;
        }
      discovered_devs++;
      dev = dev->next;
    }
  if (!discovered_devs)
    return U2FH_NO_U2F_DEVICE;
  return U2FH_OK;
}

struct u2fnfcdevice *
get_nfc_device (u2fh_nfc_devs *devices)
{
  struct u2fnfcdevice *dev;
  dev = devices->first;
  while (dev->skipped && dev->next != NULL)
    {
      dev = dev->next;
    }
  return dev;
}

static void
close_nfc_devices (u2fh_nfc_devs *devices)
{
  struct u2fnfcdevice *dev;
  struct u2fnfcdevice *curr;
  dev = devices->first;
  while (dev != NULL)
    {
      curr = dev;
      dev = dev->next;
      nfc_close (curr->pnd);
      free (curr);
    }
}

void
u2fh_nfc_devs_done (u2fh_nfc_devs *devices)
{
  close_nfc_devices (devices);
  nfc_exit (devices->context);

  free (devices);
}

#endif
