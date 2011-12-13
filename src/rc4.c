/*
 ** Copyright (©) 2003-2012 Teus Benschop.
 **  
 ** This program is free software; you can redistribute it and/or modify
 ** it under the terms of the GNU General Public License as published by
 ** the Free Software Foundation; either version 3 of the License, or
 ** (at your option) any later version.
 **  
 ** This program is distributed in the hope that it will be useful,
 ** but WITHOUT ANY WARRANTY; without even the implied warranty of
 ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 ** GNU General Public License for more details.
 **  
 ** You should have received a copy of the GNU General Public License
 ** along with this program; if not, write to the Free Software
 ** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 **  
 */

// The code was copied from software copyrighted (©) 2006 Olivier Gay <olivier.gay@a3.epfl.ch>

#include <assert.h>
#include <string.h>

#include "rc4.h"

static const uint8 rc4_table[256] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b,
    0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
    0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23,
    0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b,
    0x3c, 0x3d, 0x3e, 0x3f, 0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,
    0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f, 0x50, 0x51, 0x52, 0x53,
    0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f,
    0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b,
    0x6c, 0x6d, 0x6e, 0x6f, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77,
    0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f, 0x80, 0x81, 0x82, 0x83,
    0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f,
    0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0x9b,
    0x9c, 0x9d, 0x9e, 0x9f, 0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
    0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf, 0xb0, 0xb1, 0xb2, 0xb3,
    0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf,
    0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xcb,
    0xcc, 0xcd, 0xce, 0xcf, 0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7,
    0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf, 0xe0, 0xe1, 0xe2, 0xe3,
    0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef,
    0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa, 0xfb,
    0xfc, 0xfd, 0xfe, 0xff};

#define SWAP(x, y) \
  {                \
    tmp = x;       \
    x = y;         \
    y = tmp;       \
  }

#define RC4_CRYPT()                                           \
  {                                                           \
    t += s[(uint8)i];                                         \
    SWAP(s[(uint8)i], s[t]);                                  \
    new_dst[i] = new_src[i] ^ s[(uint8)(s[(uint8)i] + s[t])]; \
  }

void rc4_ks(rc4_ctx *ctx, const uint8 *key, uint32 key_len) {
  uint32 i;
  uint8 *s;
  uint8 t, tmp;

  t = 0;
  s = ctx->se;

  assert(key_len > 0 && key_len <= 256);

  ctx->pose = 1;
  ctx->posd = 1;
  ctx->te = 0;
  ctx->td = 0;

  memcpy(s, rc4_table, 256);

  for (i = 0; i < 256; i++) {
    t += s[i] + key[i % key_len];
    SWAP(s[i], s[t]);
  }

  memcpy(ctx->sd, s, 256);
}

void rc4_encrypt(rc4_ctx *ctx, const uint8 *src, uint8 *dst, uint32 len) {
  uint32 i;
  uint32 pos;
  const uint8 *new_src;
  uint8 *s, *new_dst;
  uint8 t, tmp;

  pos = ctx->pose;
  s = ctx->se;
  t = ctx->te;

  new_src = src - pos;
  new_dst = dst - pos;

  for (i = pos; i < len + pos; i++) {
    RC4_CRYPT();
  }

  ctx->pose = i;
  ctx->te = t;
}

void rc4_decrypt(rc4_ctx *ctx, const uint8 *src, uint8 *dst, uint32 len) {
  uint32 i;
  uint32 pos;
  const uint8 *new_src;
  uint8 *s, *new_dst;
  uint8 t, tmp;

  pos = ctx->posd;
  s = ctx->sd;
  t = ctx->td;

  new_src = src - pos;
  new_dst = dst - pos;

  for (i = pos; i < len + pos; i++) {
    RC4_CRYPT();
  }

  ctx->posd = i;
  ctx->td = t;
}

#ifdef TEST_VECTORS
/*
 * Test vectors from K. Kaukonen and R. Thayer (SSH) Internet Draft
 * for Arcfour algorithm
 */

/* Test vectors from [CRYPTLIB] */

static const uint8 pt1[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static const uint8 key1[8] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef};
static const uint8 ct1[8] = {0x74, 0x94, 0xc2, 0xe7, 0x10, 0x4b, 0x08, 0x79};

/* Test vectors from [COMMERCE] */

static const uint8 pt2[5] = {0xdc, 0xee, 0x4c, 0xf9, 0x2c};
static const uint8 key2[5] = {0x61, 0x8a, 0x63, 0xd2, 0xfb};
static const uint8 ct2[5] = {0xf1, 0x38, 0x29, 0xc9, 0xde};

/* Test vectors from [SSH ARCFOUR] */

static const uint8 pt3[309] = {
    0x52, 0x75, 0x69, 0x73, 0x6c, 0x69, 0x6e, 0x6e, 0x75, 0x6e, 0x20, 0x6c,
    0x61, 0x75, 0x6c, 0x75, 0x20, 0x6b, 0x6f, 0x72, 0x76, 0x69, 0x73, 0x73,
    0x73, 0x61, 0x6e, 0x69, 0x2c, 0x20, 0x74, 0xe4, 0x68, 0x6b, 0xe4, 0x70,
    0xe4, 0x69, 0x64, 0x65, 0x6e, 0x20, 0x70, 0xe4, 0xe4, 0x6c, 0x6c, 0xe4,
    0x20, 0x74, 0xe4, 0x79, 0x73, 0x69, 0x6b, 0x75, 0x75, 0x2e, 0x20, 0x4b,
    0x65, 0x73, 0xe4, 0x79, 0xf6, 0x6e, 0x20, 0x6f, 0x6e, 0x20, 0x6f, 0x6e,
    0x6e, 0x69, 0x20, 0x6f, 0x6d, 0x61, 0x6e, 0x61, 0x6e, 0x69, 0x2c, 0x20,
    0x6b, 0x61, 0x73, 0x6b, 0x69, 0x73, 0x61, 0x76, 0x75, 0x75, 0x6e, 0x20,
    0x6c, 0x61, 0x61, 0x6b, 0x73, 0x6f, 0x74, 0x20, 0x76, 0x65, 0x72, 0x68,
    0x6f, 0x75, 0x75, 0x2e, 0x20, 0x45, 0x6e, 0x20, 0x6d, 0x61, 0x20, 0x69,
    0x6c, 0x6f, 0x69, 0x74, 0x73, 0x65, 0x2c, 0x20, 0x73, 0x75, 0x72, 0x65,
    0x20, 0x68, 0x75, 0x6f, 0x6b, 0x61, 0x61, 0x2c, 0x20, 0x6d, 0x75, 0x74,
    0x74, 0x61, 0x20, 0x6d, 0x65, 0x74, 0x73, 0xe4, 0x6e, 0x20, 0x74, 0x75,
    0x6d, 0x6d, 0x75, 0x75, 0x73, 0x20, 0x6d, 0x75, 0x6c, 0x6c, 0x65, 0x20,
    0x74, 0x75, 0x6f, 0x6b, 0x61, 0x61, 0x2e, 0x20, 0x50, 0x75, 0x75, 0x6e,
    0x74, 0x6f, 0x20, 0x70, 0x69, 0x6c, 0x76, 0x65, 0x6e, 0x2c, 0x20, 0x6d,
    0x69, 0x20, 0x68, 0x75, 0x6b, 0x6b, 0x75, 0x75, 0x2c, 0x20, 0x73, 0x69,
    0x69, 0x6e, 0x74, 0x6f, 0x20, 0x76, 0x61, 0x72, 0x61, 0x6e, 0x20, 0x74,
    0x75, 0x75, 0x6c, 0x69, 0x73, 0x65, 0x6e, 0x2c, 0x20, 0x6d, 0x69, 0x20,
    0x6e, 0x75, 0x6b, 0x6b, 0x75, 0x75, 0x2e, 0x20, 0x54, 0x75, 0x6f, 0x6b,
    0x73, 0x75, 0x74, 0x20, 0x76, 0x61, 0x6e, 0x61, 0x6d, 0x6f, 0x6e, 0x20,
    0x6a, 0x61, 0x20, 0x76, 0x61, 0x72, 0x6a, 0x6f, 0x74, 0x20, 0x76, 0x65,
    0x65, 0x6e, 0x2c, 0x20, 0x6e, 0x69, 0x69, 0x73, 0x74, 0xe4, 0x20, 0x73,
    0x79, 0x64, 0xe4, 0x6d, 0x65, 0x6e, 0x69, 0x20, 0x6c, 0x61, 0x75, 0x6c,
    0x75, 0x6e, 0x20, 0x74, 0x65, 0x65, 0x6e, 0x2e, 0x20, 0x2d, 0x20, 0x45,
    0x69, 0x6e, 0x6f, 0x20, 0x4c, 0x65, 0x69, 0x6e, 0x6f};

static const uint8 key3[16] = {
    0x29, 0x04, 0x19, 0x72, 0xfb, 0x42, 0xba, 0x5f,
    0xc7, 0x12, 0x77, 0x12, 0xf1, 0x38, 0x29, 0xc9};

static const uint8 ct3[309] = {
    0x35, 0x81, 0x86, 0x99, 0x90, 0x01, 0xe6, 0xb5, 0xda, 0xf0, 0x5e, 0xce,
    0xeb, 0x7e, 0xee, 0x21, 0xe0, 0x68, 0x9c, 0x1f, 0x00, 0xee, 0xa8, 0x1f,
    0x7d, 0xd2, 0xca, 0xae, 0xe1, 0xd2, 0x76, 0x3e, 0x68, 0xaf, 0x0e, 0xad,
    0x33, 0xd6, 0x6c, 0x26, 0x8b, 0xc9, 0x46, 0xc4, 0x84, 0xfb, 0xe9, 0x4c,
    0x5f, 0x5e, 0x0b, 0x86, 0xa5, 0x92, 0x79, 0xe4, 0xf8, 0x24, 0xe7, 0xa6,
    0x40, 0xbd, 0x22, 0x32, 0x10, 0xb0, 0xa6, 0x11, 0x60, 0xb7, 0xbc, 0xe9,
    0x86, 0xea, 0x65, 0x68, 0x80, 0x03, 0x59, 0x6b, 0x63, 0x0a, 0x6b, 0x90,
    0xf8, 0xe0, 0xca, 0xf6, 0x91, 0x2a, 0x98, 0xeb, 0x87, 0x21, 0x76, 0xe8,
    0x3c, 0x20, 0x2c, 0xaa, 0x64, 0x16, 0x6d, 0x2c, 0xce, 0x57, 0xff, 0x1b,
    0xca, 0x57, 0xb2, 0x13, 0xf0, 0xed, 0x1a, 0xa7, 0x2f, 0xb8, 0xea, 0x52,
    0xb0, 0xbe, 0x01, 0xcd, 0x1e, 0x41, 0x28, 0x67, 0x72, 0x0b, 0x32, 0x6e,
    0xb3, 0x89, 0xd0, 0x11, 0xbd, 0x70, 0xd8, 0xaf, 0x03, 0x5f, 0xb0, 0xd8,
    0x58, 0x9d, 0xbc, 0xe3, 0xc6, 0x66, 0xf5, 0xea, 0x8d, 0x4c, 0x79, 0x54,
    0xc5, 0x0c, 0x3f, 0x34, 0x0b, 0x04, 0x67, 0xf8, 0x1b, 0x42, 0x59, 0x61,
    0xc1, 0x18, 0x43, 0x07, 0x4d, 0xf6, 0x20, 0xf2, 0x08, 0x40, 0x4b, 0x39,
    0x4c, 0xf9, 0xd3, 0x7f, 0xf5, 0x4b, 0x5f, 0x1a, 0xd8, 0xf6, 0xea, 0x7d,
    0xa3, 0xc5, 0x61, 0xdf, 0xa7, 0x28, 0x1f, 0x96, 0x44, 0x63, 0xd2, 0xcc,
    0x35, 0xa4, 0xd1, 0xb0, 0x34, 0x90, 0xde, 0xc5, 0x1b, 0x07, 0x11, 0xfb,
    0xd6, 0xf5, 0x5f, 0x79, 0x23, 0x4d, 0x5b, 0x7c, 0x76, 0x66, 0x22, 0xa6,
    0x6d, 0xe9, 0x2b, 0xe9, 0x96, 0x46, 0x1d, 0x5e, 0x4d, 0xc8, 0x78, 0xef,
    0x9b, 0xca, 0x03, 0x05, 0x21, 0xe8, 0x35, 0x1e, 0x4b, 0xae, 0xd2, 0xfd,
    0x04, 0xf9, 0x46, 0x73, 0x68, 0xc4, 0xad, 0x6a, 0xc1, 0x86, 0xd0, 0x82,
    0x45, 0xb2, 0x63, 0xa2, 0x66, 0x6d, 0x1f, 0x6c, 0x54, 0x20, 0xf1, 0x59,
    0x9d, 0xfd, 0x9f, 0x43, 0x89, 0x21, 0xc2, 0xf5, 0xa4, 0x63, 0x93, 0x8c,
    0xe0, 0x98, 0x22, 0x65, 0xee, 0xf7, 0x01, 0x79, 0xbc, 0x55, 0x3f, 0x33,
    0x9e, 0xb1, 0xa4, 0xc1, 0xaf, 0x5f, 0x6a, 0x54, 0x7f};

#include <stdio.h>
#include <stdlib.h>

/*
void test(const uint8 *pt, const uint8 *ct, uint32 msg_len,
          const uint8 *key, uint32 key_len)
{
    rc4_ctx ctx;
    uint8 dst[512];

    rc4_ks(&ctx, key, key_len);
    rc4_encrypt(&ctx, pt, dst, msg_len);

    if (memcmp(dst, ct, msg_len)) {
        fprintf(stderr, "Test failed.\n");
        exit(EXIT_FAILURE);
    }

    rc4_decrypt(&ctx, dst, dst, msg_len);

    if (memcmp(dst, pt, msg_len)) {
        fprintf(stderr, "Test failed.\n");
        exit(EXIT_FAILURE);
    }
}

int main()
{
    printf("RC4 Validation test\n\n");

    test(pt1, ct1, sizeof(pt1), key1, sizeof(key1));
    printf("Test vector 1: OK\n");

    test(pt2, ct2, sizeof(pt2), key2, sizeof(key2));
    printf("Test vector 2: OK\n");

    test(pt3, ct3, sizeof(pt3), key3, sizeof(key3));
    printf("Test vector 3: OK\n");

    printf("\nAll tests passed.\n");

    return 0;
}
*/

#endif /* TEST_VECTORS */
