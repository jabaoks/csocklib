#include "hashconv.h"
#include <string.h>

#define cpu_to_be32(x) __builtin_bswap32(x)
#define be32_to_cpu(x) __builtin_bswap32(x)
#define cpu_to_be64(x) __builtin_bswap64(x)
#define be64_to_cpu(x) __builtin_bswap64(x)

static inline unsigned int rol(unsigned int x, int n) {
  return (x << n) | (x >> (-n & 31));
}

typedef struct {
  unsigned int count;
  union {
    struct {
      unsigned int h0, h1, h2, h3, h4;
    };
    unsigned int h[5];
  };
  unsigned char buf[64];
} SHA1_CONTEXT;

static void sha1_init(SHA1_CONTEXT *hd) {
  *hd = (SHA1_CONTEXT){
      .h0 = 0x67452301,
      .h1 = 0xefcdab89,
      .h2 = 0x98badcfe,
      .h3 = 0x10325476,
      .h4 = 0xc3d2e1f0,
  };
}

static unsigned int sha1_blend(unsigned int *x, unsigned int i) {
#define X(i) x[(i)&15]

  return X(i) = rol(X(i) ^ X(i - 14) ^ X(i - 8) ^ X(i - 3), 1);

#undef X
}

static void sha1_transform(SHA1_CONTEXT *hd, const void *_data) {
  const unsigned int *data = _data;
  unsigned int a, b, c, d, e;
  unsigned int x[16];
  int i;

  a = hd->h0;
  b = hd->h1;
  c = hd->h2;
  d = hd->h3;
  e = hd->h4;

  for (i = 0; i < 16; ++i)
    x[i] = cpu_to_be32(data[i]);

#define K1 0x5A827999L
#define K2 0x6ED9EBA1L
#define K3 0x8F1BBCDCL
#define K4 0xCA62C1D6L
#define F1(x, y, z) (z ^ (x & (y ^ z)))
#define F2(x, y, z) (x ^ y ^ z)
#define F3(x, y, z) ((x & y) | (z & (x | y)))
#define F4(x, y, z) (x ^ y ^ z)

#define M(i) sha1_blend(x, i)
#define R(a, b, c, d, e, f, k, m)                                              \
  do {                                                                         \
    e += rol(a, 5) + f(b, c, d) + k + m;                                       \
    b = rol(b, 30);                                                            \
  } while (0)

  for (i = 0; i < 15; i += 5) {
    R(a, b, c, d, e, F1, K1, x[i + 0]);
    R(e, a, b, c, d, F1, K1, x[i + 1]);
    R(d, e, a, b, c, F1, K1, x[i + 2]);
    R(c, d, e, a, b, F1, K1, x[i + 3]);
    R(b, c, d, e, a, F1, K1, x[i + 4]);
  }

  R(a, b, c, d, e, F1, K1, x[15]);
  R(e, a, b, c, d, F1, K1, M(16));
  R(d, e, a, b, c, F1, K1, M(17));
  R(c, d, e, a, b, F1, K1, M(18));
  R(b, c, d, e, a, F1, K1, M(19));

  for (i = 20; i < 40; i += 5) {
    R(a, b, c, d, e, F2, K2, M(i + 0));
    R(e, a, b, c, d, F2, K2, M(i + 1));
    R(d, e, a, b, c, F2, K2, M(i + 2));
    R(c, d, e, a, b, F2, K2, M(i + 3));
    R(b, c, d, e, a, F2, K2, M(i + 4));
  }

  for (; i < 60; i += 5) {
    R(a, b, c, d, e, F3, K3, M(i + 0));
    R(e, a, b, c, d, F3, K3, M(i + 1));
    R(d, e, a, b, c, F3, K3, M(i + 2));
    R(c, d, e, a, b, F3, K3, M(i + 3));
    R(b, c, d, e, a, F3, K3, M(i + 4));
  }

  for (; i < 80; i += 5) {
    R(a, b, c, d, e, F4, K4, M(i + 0));
    R(e, a, b, c, d, F4, K4, M(i + 1));
    R(d, e, a, b, c, F4, K4, M(i + 2));
    R(c, d, e, a, b, F4, K4, M(i + 3));
    R(b, c, d, e, a, F4, K4, M(i + 4));
  }

  hd->h0 += a;
  hd->h1 += b;
  hd->h2 += c;
  hd->h3 += d;
  hd->h4 += e;
}

static void sha1_once(SHA1_CONTEXT *hd, const void *data, unsigned int len) {
  hd->count = len;
  for (; len >= 64; data += 64, len -= 64)
    sha1_transform(hd, data);

  memcpy(hd->buf, data, len);
}

static void sha1_final(SHA1_CONTEXT *hd, unsigned char hash[SHA1_DIGEST_SIZE]) {
  unsigned int partial = hd->count & 0x3f;

  hd->buf[partial++] = 0x80;

  if (partial > 56) {
    memset(hd->buf + partial, 0, 64 - partial);
    sha1_transform(hd, hd->buf);
    partial = 0;
  }
  memset(hd->buf + partial, 0, 56 - partial);

  unsigned long long *count = (void *)&hd->buf[56];
  *count = cpu_to_be64((unsigned long long)hd->count << 3);
  sha1_transform(hd, hd->buf);

  unsigned int *p = (void *)hash;
  for (int i = 0; i < 5; ++i)
    p[i] = be32_to_cpu(hd->h[i]);
}

void sha1sum(unsigned char *dst, const void *ptr, int len) {
  SHA1_CONTEXT ctx;

  sha1_init(&ctx);
  sha1_once(&ctx, ptr, len);
  sha1_final(&ctx, dst);
}
