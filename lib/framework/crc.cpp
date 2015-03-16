/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2013  Warzone 2100 Project

	Warzone 2100 is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	Warzone 2100 is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Warzone 2100; if not, write to the Free Software
	Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
*/

#include "crc.h"
#include "lib/netplay/netsocket.h"  // For htonl

// Invariant:
// crcTable[0] = 0;
// crcTable[1] = 0x04C11DB7;
// crcTable[i] = crcTable[i>>1]<<1 ^ ((crcTable[i>>1]>>31 ^ (i & 0x01))*crcTable[1]);
static const uint32_t crcTable[256] = {0x00000000, 0x04C11DB7, 0x09823B6E, 0x0D4326D9, 0x130476DC, 0x17C56B6B, 0x1A864DB2, 0x1E475005, 0x2608EDB8, 0x22C9F00F, 0x2F8AD6D6, 0x2B4BCB61, 0x350C9B64, 0x31CD86D3, 0x3C8EA00A, 0x384FBDBD, 0x4C11DB70, 0x48D0C6C7, 0x4593E01E, 0x4152FDA9, 0x5F15ADAC, 0x5BD4B01B, 0x569796C2, 0x52568B75, 0x6A1936C8, 0x6ED82B7F, 0x639B0DA6, 0x675A1011, 0x791D4014, 0x7DDC5DA3, 0x709F7B7A, 0x745E66CD, 0x9823B6E0, 0x9CE2AB57, 0x91A18D8E, 0x95609039, 0x8B27C03C, 0x8FE6DD8B, 0x82A5FB52, 0x8664E6E5, 0xBE2B5B58, 0xBAEA46EF, 0xB7A96036, 0xB3687D81, 0xAD2F2D84, 0xA9EE3033, 0xA4AD16EA, 0xA06C0B5D, 0xD4326D90, 0xD0F37027, 0xDDB056FE, 0xD9714B49, 0xC7361B4C, 0xC3F706FB, 0xCEB42022, 0xCA753D95, 0xF23A8028, 0xF6FB9D9F, 0xFBB8BB46, 0xFF79A6F1, 0xE13EF6F4, 0xE5FFEB43, 0xE8BCCD9A, 0xEC7DD02D, 0x34867077, 0x30476DC0, 0x3D044B19, 0x39C556AE, 0x278206AB, 0x23431B1C, 0x2E003DC5, 0x2AC12072, 0x128E9DCF, 0x164F8078, 0x1B0CA6A1, 0x1FCDBB16, 0x018AEB13, 0x054BF6A4, 0x0808D07D, 0x0CC9CDCA, 0x7897AB07, 0x7C56B6B0, 0x71159069, 0x75D48DDE, 0x6B93DDDB, 0x6F52C06C, 0x6211E6B5, 0x66D0FB02, 0x5E9F46BF, 0x5A5E5B08, 0x571D7DD1, 0x53DC6066, 0x4D9B3063, 0x495A2DD4, 0x44190B0D, 0x40D816BA, 0xACA5C697, 0xA864DB20, 0xA527FDF9, 0xA1E6E04E, 0xBFA1B04B, 0xBB60ADFC, 0xB6238B25, 0xB2E29692, 0x8AAD2B2F, 0x8E6C3698, 0x832F1041, 0x87EE0DF6, 0x99A95DF3, 0x9D684044, 0x902B669D, 0x94EA7B2A, 0xE0B41DE7, 0xE4750050, 0xE9362689, 0xEDF73B3E, 0xF3B06B3B, 0xF771768C, 0xFA325055, 0xFEF34DE2, 0xC6BCF05F, 0xC27DEDE8, 0xCF3ECB31, 0xCBFFD686, 0xD5B88683, 0xD1799B34, 0xDC3ABDED, 0xD8FBA05A, 0x690CE0EE, 0x6DCDFD59, 0x608EDB80, 0x644FC637, 0x7A089632, 0x7EC98B85, 0x738AAD5C, 0x774BB0EB, 0x4F040D56, 0x4BC510E1, 0x46863638, 0x42472B8F, 0x5C007B8A, 0x58C1663D, 0x558240E4, 0x51435D53, 0x251D3B9E, 0x21DC2629, 0x2C9F00F0, 0x285E1D47, 0x36194D42, 0x32D850F5, 0x3F9B762C, 0x3B5A6B9B, 0x0315D626, 0x07D4CB91, 0x0A97ED48, 0x0E56F0FF, 0x1011A0FA, 0x14D0BD4D, 0x19939B94, 0x1D528623, 0xF12F560E, 0xF5EE4BB9, 0xF8AD6D60, 0xFC6C70D7, 0xE22B20D2, 0xE6EA3D65, 0xEBA91BBC, 0xEF68060B, 0xD727BBB6, 0xD3E6A601, 0xDEA580D8, 0xDA649D6F, 0xC423CD6A, 0xC0E2D0DD, 0xCDA1F604, 0xC960EBB3, 0xBD3E8D7E, 0xB9FF90C9, 0xB4BCB610, 0xB07DABA7, 0xAE3AFBA2, 0xAAFBE615, 0xA7B8C0CC, 0xA379DD7B, 0x9B3660C6, 0x9FF77D71, 0x92B45BA8, 0x9675461F, 0x8832161A, 0x8CF30BAD, 0x81B02D74, 0x857130C3, 0x5D8A9099, 0x594B8D2E, 0x5408ABF7, 0x50C9B640, 0x4E8EE645, 0x4A4FFBF2, 0x470CDD2B, 0x43CDC09C, 0x7B827D21, 0x7F436096, 0x7200464F, 0x76C15BF8, 0x68860BFD, 0x6C47164A, 0x61043093, 0x65C52D24, 0x119B4BE9, 0x155A565E, 0x18197087, 0x1CD86D30, 0x029F3D35, 0x065E2082, 0x0B1D065B, 0x0FDC1BEC, 0x3793A651, 0x3352BBE6, 0x3E119D3F, 0x3AD08088, 0x2497D08D, 0x2056CD3A, 0x2D15EBE3, 0x29D4F654, 0xC5A92679, 0xC1683BCE, 0xCC2B1D17, 0xC8EA00A0, 0xD6AD50A5, 0xD26C4D12, 0xDF2F6BCB, 0xDBEE767C, 0xE3A1CBC1, 0xE760D676, 0xEA23F0AF, 0xEEE2ED18, 0xF0A5BD1D, 0xF464A0AA, 0xF9278673, 0xFDE69BC4, 0x89B8FD09, 0x8D79E0BE, 0x803AC667, 0x84FBDBD0, 0x9ABC8BD5, 0x9E7D9662, 0x933EB0BB, 0x97FFAD0C, 0xAFB010B1, 0xAB710D06, 0xA6322BDF, 0xA2F33668, 0xBCB4666D, 0xB8757BDA, 0xB5365D03, 0xB1F740B4};

uint32_t crcSum(uint32_t crc, const void *data_, size_t dataLen)
{
	const char *data = (const char *)data_;  // Aliasing rules say that this must be read as a char, not as an an uint8_t.

	while (dataLen-- > 0)
	{
		crc = crc << 8 ^ crcTable[crc>>24 ^ (uint8_t) * data++];
	}

	return crc;
}

uint32_t crcSumU16(uint32_t crc, const uint16_t *data, size_t dataLen)
{
	while (dataLen-- > 0)
	{
		crc = crc << 8 ^ crcTable[crc>>24 ^ (uint8_t)(*data >> 8)];
		crc = crc << 8 ^ crcTable[crc>>24 ^ (uint8_t) * data++];
	}

	return crc;
}

uint32_t crcSumVector2i(uint32_t crc, const Vector2i *data, size_t dataLen)
{
	while (dataLen-- > 0)
	{
		crc = crc << 8 ^ crcTable[crc>>24 ^ (uint8_t)(data->x >> 24)];
		crc = crc << 8 ^ crcTable[crc>>24 ^ (uint8_t)(data->x >> 16)];
		crc = crc << 8 ^ crcTable[crc>>24 ^ (uint8_t)(data->x >> 8)];
		crc = crc << 8 ^ crcTable[crc>>24 ^ (uint8_t)data->x];
		crc = crc << 8 ^ crcTable[crc>>24 ^ (uint8_t)(data->y >> 24)];
		crc = crc << 8 ^ crcTable[crc>>24 ^ (uint8_t)(data->y >> 16)];
		crc = crc << 8 ^ crcTable[crc>>24 ^ (uint8_t)(data->y >> 8)];
		crc = crc << 8 ^ crcTable[crc>>24 ^ (uint8_t)data++->y];
	}

	return crc;
}

// Let primes[n] = {2, 3, 5, 7, 11, ...}.
// Invariant: sha256TableH[n] = sqrt(primes[n]) << 32
static const uint32_t sha256TableH[8] = {0x6A09E667, 0xBB67AE85, 0x3C6EF372, 0xA54FF53A, 0x510E527F, 0x9B05688C, 0x1F83D9AB, 0x5BE0CD19};
// Invariant: sha256TableK[n] = pow(primes[n], 1/3.) << 32
static const uint32_t sha256TableK[64] = {0x428A2F98, 0x71374491, 0xB5C0FBCF, 0xE9B5DBA5, 0x3956C25B, 0x59F111F1, 0x923F82A4, 0xAB1C5ED5, 0xD807AA98, 0x12835B01, 0x243185BE, 0x550C7DC3, 0x72BE5D74, 0x80DEB1FE, 0x9BDC06A7, 0xC19BF174, 0xE49B69C1, 0xEFBE4786, 0x0FC19DC6, 0x240CA1CC, 0x2DE92C6F, 0x4A7484AA, 0x5CB0A9DC, 0x76F988DA, 0x983E5152, 0xA831C66D, 0xB00327C8, 0xBF597FC7, 0xC6E00BF3, 0xD5A79147, 0x06CA6351, 0x14292967, 0x27B70A85, 0x2E1B2138, 0x4D2C6DFC, 0x53380D13, 0x650A7354, 0x766A0ABB, 0x81C2C92E, 0x92722C85, 0xA2BFE8A1, 0xA81A664B, 0xC24B8B70, 0xC76C51A3, 0xD192E819, 0xD6990624, 0xF40E3585, 0x106AA070, 0x19A4C116, 0x1E376C08, 0x2748774C, 0x34B0BCB5, 0x391C0CB3, 0x4ED8AA4A, 0x5B9CCA4F, 0x682E6FF3, 0x748F82EE, 0x78A5636F, 0x84C87814, 0x8CC70208, 0x90BEFFFA, 0xA4506CEB, 0xBEF9A3F7, 0xC67178F2};

// Rotate right.
static inline uint32_t ror(uint32_t a, int shift)
{
	return (a >> shift) | (a << (32 - shift));  // This is correctly optimised to a rotate-right instruction, in GCC, at least.
}

static void sha256SumBlock(uint32_t *w, uint32_t *h)
{
	for (unsigned i = 0; i < 16; ++i)
	{
		w[i] = ntohl(w[i]);  // Interpret data as big-endian 32-bit numbers.
	}

	// Extend w block from length 16 to 64.
	for (unsigned i = 16; i < 64; ++i)
	{
		uint32_t s0 = ror(w[i - 15], 7) ^ ror(w[i - 15], 18) ^ (w[i - 15] >> 3);
		uint32_t s1 = ror(w[i - 2], 17) ^ ror(w[i - 2], 19) ^ (w[i - 2] >> 10);
		w[i] = w[i - 16] + s0 + w[i - 7] + s1;
	}

	uint32_t a = h[0], b = h[1], c = h[2], d = h[3], e = h[4], f = h[5], g = h[6], j = h[7];
	for (unsigned i = 0; i < 64; ++i)
	{
		uint32_t s0 = ror(a, 2) ^ ror(a, 13) ^ ror(a, 22);
		uint32_t maj = (a & b) ^ (a & c) ^ (b & c);  // Can be simplified to ((a ^ b) & c) ^ (a & b), but GCC does this at compile time.
		uint32_t t2 = s0 + maj;
		uint32_t s1 = ror(e, 6) ^ ror(e, 11) ^ ror(e, 25);
		uint32_t ch = (e & (f ^ g)) ^ g;  // Equivalent to (e & f) ^ (~e & g), but saves a 'notl' instruction.
		uint32_t t1 = j + s1 + ch + sha256TableK[i] + w[i];

		j = g;
		g = f;
		f = e;
		e = d + t1;
		d = c;
		c = b;
		b = a;
		a = t1 + t2;
	}

	h[0] += a;
	h[1] += b;
	h[2] += c;
	h[3] += d;
	h[4] += e;
	h[5] += f;
	h[6] += g;
	h[7] += j;
}

Sha256 sha256Sum(void const *data, size_t dataLen)
{
	uint32_t h[8];
	std::copy(sha256TableH, sha256TableH + 8, h);

	// Add the suffix to the data.
	char suffix[64 + 8];
	size_t suffixLen = ((-9 - dataLen) & 63) + 9;
	suffix[0] = 0x80;
	memset(suffix + 1, 0x00, suffixLen - 1 - 8);
	uint32_t size[2] = {htonl(dataLen >> 29), htonl(dataLen << 3)};  // Convert dataLen to number of bits, in big-endian format.
	memcpy(suffix + suffixLen - 8, (char *)size, 8);

	// Hash the data.
	uint32_t w[64];  // 16 elements are passed to sha256SumBlock(), rest is used as scratch space by sha256SumBlock().
	for (size_t z = 0; z + 63 < dataLen; z += 64)
	{
		memcpy((char *)w, (char const *)data + z, 64);
		sha256SumBlock(w, h);
	}
	memcpy((char *)w, (char const *)data + (dataLen & ~63), dataLen & 63);
	memcpy((char *)w + (dataLen & 63), suffix, ((suffixLen - 1) & 63) + 1);
	sha256SumBlock(w, h);
	if (suffixLen > 64)
	{
		memcpy((char *)w, suffix + (suffixLen & 63), 64);
		sha256SumBlock(w, h);
	}

	// Convert result to big-endian, and return it.
	for (unsigned i = 0; i < 8; ++i)
	{
		h[i] = htonl(h[i]);
	}
	Sha256 ret;
	memcpy(ret.bytes, h, 32);
	return ret;
}

bool Sha256::operator ==(Sha256 const &b) const
{
	return memcmp(bytes, b.bytes, Bytes) == 0;
}

bool Sha256::isZero() const
{
	return bytes[0] == 0x00 && memcmp(bytes, bytes + 1, Bytes - 1) == 0;
}

void Sha256::setZero()
{
	memset(bytes, 0x00, Bytes);
}

std::string Sha256::toString() const
{
	std::string str;
	str.resize(Bytes * 2);
	char const *hexDigits = "0123456789abcdef";
	for (unsigned n = 0; n < Bytes; ++n)
	{
		str[n * 2    ] = hexDigits[bytes[n] >> 4];
		str[n * 2 + 1] = hexDigits[bytes[n] & 15];
	}
	return str;
}

void Sha256::fromString(std::string const &s)
{
	setZero();
	unsigned nChars = std::min<unsigned>(Bytes * 2, s.size());
	for (unsigned n = 0; n < nChars; ++n)
	{
		unsigned h;
		unsigned c = s[n];
		if (c >= '0' && c <= '9')
		{
			h = c - '0';
		}
		else if (c >= 'a' && c <= 'f')
		{
			h = c - 'a' + 10;
		}
		else if (c >= 'A' && c <= 'F')
		{
			h = c - 'A' + 10;
		}
		else
		{
			break;
		}
		bytes[n / 2] |= h << (n % 2 ? 0 : 4);
	}
}
