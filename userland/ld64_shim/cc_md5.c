/* Real MD5 (RFC 1321) backing CommonCrypto/CommonDigest.h's CC_MD5_*
 * API. AsterOS has no CommonCrypto dylib to link against; ld64's
 * OutputFile.cpp only uses CC_MD5 to compute a binary's LC_UUID, which
 * just needs to be a stable, deterministic 16-byte digest -- not a
 * defense against adversarial input -- so a straightforward public
 * textbook implementation is appropriate here.
 */
#include <CommonCrypto/CommonDigest.h>
#include <string.h>

static uint32_t
leftrotate(uint32_t x, uint32_t c)
{
	return (x << c) | (x >> (32 - c));
}

static const uint32_t K[64] = {
	0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee,
	0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501,
	0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be,
	0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821,
	0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa,
	0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8,
	0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed,
	0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a,
	0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c,
	0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70,
	0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05,
	0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665,
	0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039,
	0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1,
	0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1,
	0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391,
};

static const uint32_t S[64] = {
	7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22,
	5,  9, 14, 20, 5,  9, 14, 20, 5,  9, 14, 20, 5,  9, 14, 20,
	4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23,
	6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21,
};

static void
md5_process_block(CC_MD5_CTX *c, const uint8_t block[64])
{
	uint32_t M[16];
	for (int i = 0; i < 16; i++) {
		M[i] = (uint32_t)block[i * 4] | ((uint32_t)block[i * 4 + 1] << 8) |
		    ((uint32_t)block[i * 4 + 2] << 16) | ((uint32_t)block[i * 4 + 3] << 24);
	}

	uint32_t A = c->A, B = c->B, Cc = c->C, D = c->D;

	for (int i = 0; i < 64; i++) {
		uint32_t F;
		int g;
		if (i < 16) {
			F = (B & Cc) | (~B & D);
			g = i;
		} else if (i < 32) {
			F = (D & B) | (~D & Cc);
			g = (5 * i + 1) % 16;
		} else if (i < 48) {
			F = B ^ Cc ^ D;
			g = (3 * i + 5) % 16;
		} else {
			F = Cc ^ (B | ~D);
			g = (7 * i) % 16;
		}
		uint32_t tmp = D;
		D = Cc;
		Cc = B;
		B = B + leftrotate(A + F + K[i] + M[g], S[i]);
		A = tmp;
	}

	c->A += A;
	c->B += B;
	c->C += Cc;
	c->D += D;
}

int
CC_MD5_Init(CC_MD5_CTX *c)
{
	c->A = 0x67452301;
	c->B = 0xefcdab89;
	c->C = 0x98badcfe;
	c->D = 0x10325476;
	c->Nl = 0;
	c->Nh = 0;
	c->num = 0;
	return 1;
}

int
CC_MD5_Update(CC_MD5_CTX *c, const void *data, CC_LONG len)
{
	const uint8_t *p = (const uint8_t *)data;
	uint8_t *buf = (uint8_t *)c->data;

	uint64_t total = ((uint64_t)c->Nh << 32) | c->Nl;
	total += len;
	c->Nl = (uint32_t)total;
	c->Nh = (uint32_t)(total >> 32);

	while (len > 0) {
		size_t space = 64 - (size_t)c->num;
		size_t take = (size_t)len < space ? (size_t)len : space;
		memcpy(buf + c->num, p, take);
		c->num += (int)take;
		p += take;
		len -= (CC_LONG)take;
		if (c->num == 64) {
			md5_process_block(c, buf);
			c->num = 0;
		}
	}
	return 1;
}

int
CC_MD5_Final(unsigned char *md, CC_MD5_CTX *c)
{
	uint64_t bitlen = (((uint64_t)c->Nh << 32) | c->Nl) * 8;
	uint8_t *buf = (uint8_t *)c->data;

	buf[c->num++] = 0x80;
	if (c->num > 56) {
		while (c->num < 64) {
			buf[c->num++] = 0;
		}
		md5_process_block(c, buf);
		c->num = 0;
	}
	while (c->num < 56) {
		buf[c->num++] = 0;
	}
	for (int i = 0; i < 8; i++) {
		buf[56 + i] = (uint8_t)(bitlen >> (8 * i));
	}
	md5_process_block(c, buf);

	uint32_t words[4] = { c->A, c->B, c->C, c->D };
	for (int i = 0; i < 4; i++) {
		md[i * 4 + 0] = (uint8_t)(words[i]);
		md[i * 4 + 1] = (uint8_t)(words[i] >> 8);
		md[i * 4 + 2] = (uint8_t)(words[i] >> 16);
		md[i * 4 + 3] = (uint8_t)(words[i] >> 24);
	}
	return 1;
}

unsigned char *
CC_MD5(const void *data, CC_LONG len, unsigned char *md)
{
	CC_MD5_CTX c;
	CC_MD5_Init(&c);
	CC_MD5_Update(&c, data, len);
	CC_MD5_Final(md, &c);
	return md;
}
