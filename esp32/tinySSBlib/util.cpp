// util.cpp

// (c) 2022-2023 <christian.tschudin@unibas.ch>

#include <stdio.h>
#include <string.h>

#include "util.h"

static char hex_str[101];
static const char *hexdigits = "0123456789abcdef";

char* to_hex(unsigned char *buf, int bin_len, int add_colon)
{
  char *cp = hex_str;
  if (2*bin_len >= sizeof(hex_str))
    bin_len = sizeof(hex_str) / 2;
  for (int i = 0; i < bin_len; i++) {
    if (add_colon && i > 0) *cp++ = ':';
    *cp++ = hexdigits[buf[i] >> 4];
    *cp++ = hexdigits[buf[i] & 0x0f];
  }
  *cp = 0;
  return hex_str;
}

unsigned char* from_hex(char *hex, int len) // len = number of bytes to retrieve from hexstring
{
  static unsigned char buf[32];
  if (2*len > strlen(hex))
    return NULL;
  for (int cnt = 0; cnt < len; cnt++, hex += 2)
    sscanf(hex, "%2hhx", buf + cnt);
  return buf;
}
