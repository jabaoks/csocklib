#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

static const char b64_table[] = {
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
    'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
    'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
    'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/'};

#define TMP2BUFD \
    buf[0] = (tmp[0] << 2) + ((tmp[1] & 0x30) >> 4); \
    buf[1] = ((tmp[1] & 0xf) << 4) + ((tmp[2] & 0x3c) >> 2); \
    buf[2] = ((tmp[2] & 0x3) << 6) + tmp[3];

unsigned char *base64_dec(char *dst, const char *src, int len) {
  int i = 0, j = 0, l;
  int size = 0;
  unsigned char *dec = dst;
  unsigned char buf[3];
  unsigned char tmp[4];

  while (len--) {
    if ('=' == src[j]) {
      break;
    }
    if (!(isalnum(src[j]) || '+' == src[j] || '/' == src[j])) {
      break;
    }
    tmp[i++] = src[j++];
    if (4 == i) {
      for (i = 0; i < 4; ++i) {
        for (l = 0; l < 64; ++l) {
          if (tmp[i] == b64_table[l]) {
            tmp[i] = l;
            break;
          }
        }
      }
      TMP2BUFD;
      for (i = 0; i < 3; ++i) dec[size++] = buf[i];
      i = 0;
    }
  }
  if (i > 0) {
    for (j = i; j < 4; ++j) {
      tmp[j] = '\0';
    }
    for (j = 0; j < 4; ++j) {
      for (l = 0; l < 64; ++l) {
        if (tmp[j] == b64_table[l]) {
          tmp[j] = l;
          break;
        }
      }
    }
    TMP2BUFD;
    for (j = 0; (j < i - 1); ++j) dec[size++] = buf[j];
  }
  dec[size] = '\0';
  return dec;
}

#define TMP2BUFE \
      buf[0] = (tmp[0] & 0xfc) >> 2; \
      buf[1] = ((tmp[0] & 0x03) << 4) + ((tmp[1] & 0xf0) >> 4); \
      buf[2] = ((tmp[1] & 0x0f) << 2) + ((tmp[2] & 0xc0) >> 6); \
      buf[3] = tmp[2] & 0x3f;

unsigned char *base64_enc(unsigned char *dst, const unsigned char *src,
                          int len) {
  int i = 0, j = 0;
  char *enc = dst;
  int size = 0;
  unsigned char buf[4];
  unsigned char tmp[3];

  while (len--) {
    tmp[i++] = *(src++);

    if (3 == i) {
      TMP2BUFE;
      for (i = 0; i < 4; ++i) {
        enc[size++] = b64_table[buf[i]];
      }
      i = 0;
    }
  }
  if (i > 0) {
    for (j = i; j < 3; ++j) {
      tmp[j] = '\0';
    }
    TMP2BUFE;
    for (j = 0; (j < i + 1); ++j) {
      enc[size++] = b64_table[buf[j]];
    }
    while ((i++ < 3)) {
      enc[size++] = '=';
    }
  }
  enc[size] = '\0';
  return enc;
}
