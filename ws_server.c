#include <stdio.h>
#include <string.h>

#include "csocklib.h"
#include "hashconv.h"

struct WsHead {
  unsigned char opcode : 4;
  unsigned char res3 : 1;
  unsigned char res2 : 1;
  unsigned char res1 : 1;
  unsigned char fin : 1;

  unsigned char payload_len : 7;
  unsigned char mask : 1;
};

int ws_encode_msg(unsigned char *out, unsigned char *inp) {
  unsigned char *pb = out;
  int len = strlen(inp);
  struct WsHead *ph = (struct WsHead *)pb;
  unsigned char *mask;
  *pb = 0;
  pb += 1;
  if (len <= 125) {
    *(pb++) = len;
  } else if (len <= 65535) {
    *(pb++) = 126;
    *(pb++) = (len >> 8) & 0xff;
    *(pb++) = len & 0xff;
  } else {
    *(pb++) = 127;
    *(pb++) = (len >> 24) & 0xff;
    *(pb++) = (len >> 16) & 0xff;
    *(pb++) = (len >> 8) & 0xff;
    *(pb++) = len & 0xff;
  }
  ph->opcode = 1; // text
  ph->fin = 1;
  ph->mask = 0;
  if (ph->mask) {
    mask = pb;
    mask[0] = 0xaa;
    mask[1] = 0x55;
    mask[2] = 0xaa;
    mask[3] = 0x55;
    pb += 4;
    for (int i = 0; i < len; i++)
      pb[i] = *(inp++) ^ mask[i % 4];
  } else {
    for (int i = 0; i < len; i++)
      pb[i] = *(inp++);
  }
  return len + (pb - out);
}

int ws_decode_msg(unsigned char *out, unsigned char *inp) {
  unsigned char *pb = inp;
  unsigned char *mask;
  unsigned char *dst = out;
  struct WsHead *ph = (struct WsHead *)pb;
  // for(int i=0; i<8; i++) sys_printf("%d %2x\n", i, inp[i]);
  if (*pb != 0x81)
    return 0;
  pb++;
  if (*pb < 128) {
    printf("no mask flag from client\n");
    return 0;
  }
  int len = *pb & 127;
  pb++;
  if (len == 126) {
    len = (pb[0] << 8) | pb[1];
    pb += 2;
  } else if (len == 127) {
    len = (pb[0] << 24) | (pb[1] << 16) | (pb[2] << 8) | pb[3];
    pb += 4;
  }
  mask = pb;
  pb += 4;
  for (int i = 0; i < len; i++) {
    *(dst++) = pb[i] ^ mask[i % 4];
  }
  out[len] = 0;
  // sys_printf("payload_len %d [%s]\n", len, out);
  return len;
}

static int get_key(const char *buf, const char *key, char *value) {
  char *ps;
  char *ps2 = value;
  ps = strstr(buf, key);
  if (ps) {
    ps += strlen(key);
    while (*ps == ' ')
      ps++;
    while (*ps >= ' ')
      *(ps2++) = *(ps++);
    *(ps2) = 0;
    return ps2 - key;
  }
  return 0;
}

int ws_io(void **ptr, char *data, int len) {
  char *ps;
  char buf[0x1000];
  int ret;
  // printf("web [%s] %d\n", data, len);
  if (strstr(data, "Connection: Upgrade")) {
    unsigned char key_in[200];
    char host[200];
    printf("web Connection: Upgrade\n");
    if (get_key(data, "Sec-WebSocket-Key:", key_in) &&
        get_key(data, "Host:", host)) {
      static const char *ws_UUID = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

      unsigned char key_sha1[200];
      unsigned char key_base64[200];

      strcat(key_in, ws_UUID);
      sha1sum(key_sha1, key_in, strlen(key_in));
      ps = buf;

      base64_enc(key_base64, key_sha1, SHA1_DIGEST_SIZE);

      ret = sprintf(buf,
                    "HTTP/1.1 101 Switching Protocols\n"
                    "Upgrade: websocket\n"
                    "Host: %s\n"
                    "Connection: Upgrade\n"
                    "Sec-WebSocket-Accept: %s\n"
                    "Sec-WebSocket-Version: 13\n"
                    "Sec-WebSocket-Protocol: chat\n"
                    "\n",
                    host, key_base64);

      printf("web write to scok [%s]\n", buf);

      csock_write(ptr, buf, ret);
    }
    return 1;
  }
  {
    char msg[1000];
    int len;
    len = ws_decode_msg(msg, data);
    if (len > 0) {
      char buf[1100];
      data[len] = 0;
      printf("ws:[%s]\n", msg);
      sprintf(buf, "echo from ws_server: %s", msg);
      len = ws_encode_msg(msg, buf);
      csock_write(ptr, msg, len);

      return len;
    }
  }
  return 0;
}
