#ifndef _HASHCONV_H_
#define _HASHCONV_H_

#ifdef __cplusplus
extern "C" {
#endif

#define SHA1_DIGEST_SIZE 20
void sha1sum(unsigned char *dst, const void *ptr, int len);
unsigned char *base64_dec(unsigned char *dst, const char *src, int len);
unsigned char *base64_enc(unsigned char *dst, const unsigned char *src,
                          int len);

#ifdef __cplusplus
}
#endif

#endif
