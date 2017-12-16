/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2017  Warzone 2100 Project

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

#include <functional>
#include <limits>
#include <memory>

//================================================================================
// MARK: - CRC
//================================================================================

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

//================================================================================
// MARK: - SHA256
//================================================================================

#include <sha2/sha2.h>
Sha256 sha256Sum(void const *data, size_t dataLen)
{
	static_assert(Sha256::Bytes == SHA256_DIGEST_SIZE, "Size mismatch.");

	Sha256 ret;
	if (dataLen > std::numeric_limits<unsigned long>::max())
	{
		debug(LOG_FATAL, "Attempting to calculate SHA256 on data length exceeding std::numeric_limits<unsigned long>::max()=(%lu)", std::numeric_limits<unsigned long>::max());
		ret.setZero();
		return ret;
	}

	sha256_ctx ctx[1];
	sha256_begin(ctx);
	sha256_hash((const unsigned char *)data, dataLen, ctx);
	sha256_end(ret.bytes, ctx);
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
	for (int n = 0; n < Bytes; ++n)
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

//================================================================================
// MARK: - ECDSA Tables / Helpers
//================================================================================

// Hard-coded ECPrivateKey tables for specific curves

struct wz_secp_privateKey_format {
	struct secp224r1 {
		const static uint8_t prelude[];
		const static uint8_t numPrivateKeyBytes;
		const static uint8_t ecDomainParameters[];
		const static uint8_t publicKeyPrelude[];
		const static uint8_t numPublicKeyBytes;
	};
	struct secp256r1 {
		const static uint8_t prelude[];
		const static uint8_t numPrivateKeyBytes;
		const static uint8_t ecDomainParameters[];
		const static uint8_t publicKeyPrelude[];
		const static uint8_t numPublicKeyBytes;
	};
};

// [secp224r1]

// specifies a SEQUENCE of defined length + an INTEGER (1) + the prefix for a 28-byte OCTET STRING
const uint8_t wz_secp_privateKey_format::secp224r1::prelude[] = { 0x30, 0x82, 0x01, 0x44, 0x02, 0x01, 0x01, 0x04, 0x1C };
//
const uint8_t wz_secp_privateKey_format::secp224r1::numPrivateKeyBytes = 28;
// The encoded ECDomainParameters for secp224r1
const uint8_t wz_secp_privateKey_format::secp224r1::ecDomainParameters[] = { 0xA0, 0x81, 0xE2, 0x30, 0x81, 0xDF, 0x02, 0x01, 0x01, 0x30, 0x28, 0x06, 0x07, 0x2A, 0x86, 0x48, 0xCE, 0x3D, 0x01, 0x01, 0x02, 0x1D, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x30, 0x53, 0x04, 0x1C, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFE, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFE, 0x04, 0x1C, 0xB4, 0x05, 0x0A, 0x85, 0x0C, 0x04, 0xB3, 0xAB, 0xF5, 0x41, 0x32, 0x56, 0x50, 0x44, 0xB0, 0xB7, 0xD7, 0xBF, 0xD8, 0xBA, 0x27, 0x0B, 0x39, 0x43, 0x23, 0x55, 0xFF, 0xB4, 0x03, 0x15, 0x00, 0xBD, 0x71, 0x34, 0x47, 0x99, 0xD5, 0xC7, 0xFC, 0xDC, 0x45, 0xB5, 0x9F, 0xA3, 0xB9, 0xAB, 0x8F, 0x6A, 0x94, 0x8B, 0xC5, 0x04, 0x39, 0x04, 0xB7, 0x0E, 0x0C, 0xBD, 0x6B, 0xB4, 0xBF, 0x7F, 0x32, 0x13, 0x90, 0xB9, 0x4A, 0x03, 0xC1, 0xD3, 0x56, 0xC2, 0x11, 0x22, 0x34, 0x32, 0x80, 0xD6, 0x11, 0x5C, 0x1D, 0x21, 0xBD, 0x37, 0x63, 0x88, 0xB5, 0xF7, 0x23, 0xFB, 0x4C, 0x22, 0xDF, 0xE6, 0xCD, 0x43, 0x75, 0xA0, 0x5A, 0x07, 0x47, 0x64, 0x44, 0xD5, 0x81, 0x99, 0x85, 0x00, 0x7E, 0x34, 0x02, 0x1D, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x16, 0xA2, 0xE0, 0xB8, 0xF0, 0x3E, 0x13, 0xDD, 0x29, 0x45, 0x5C, 0x5C, 0x2A, 0x3D, 0x02, 0x01, 0x01 };
// The prelude before the public key (1) elem + public key BIT STRING prelude
const uint8_t wz_secp_privateKey_format::secp224r1::publicKeyPrelude[] = { 0xA1, 0x3C, 0x03, 0x3A, 0x00 };
// 0x04 || X || Y  // (or, in other words, 0x04 + 28 byte X + 28 byte Y)
const uint8_t wz_secp_privateKey_format::secp224r1::numPublicKeyBytes = 57;


// [secp256r1]

// specifies a SEQUENCE of defined length + an INTEGER (1) + the prefix for a 32-byte OCTET STRING
const uint8_t wz_secp_privateKey_format::secp256r1::prelude[] = { 0x30, 0x82, 0x01, 0x68, 0x02, 0x01, 0x01, 0x04, 0x20 };
//
const uint8_t wz_secp_privateKey_format::secp256r1::numPrivateKeyBytes = 32;
// The encoded ECDomainParameters for secp256r1
const uint8_t wz_secp_privateKey_format::secp256r1::ecDomainParameters[] = { 0xA0, 0x81, 0xFA, 0x30, 0x81, 0xF7, 0x02, 0x01, 0x01, 0x30, 0x2C, 0x06, 0x07, 0x2A, 0x86, 0x48, 0xCE, 0x3D, 0x01, 0x01, 0x02, 0x21, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x30, 0x5B, 0x04, 0x20, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFC, 0x04, 0x20, 0x5A, 0xC6, 0x35, 0xD8, 0xAA, 0x3A, 0x93, 0xE7, 0xB3, 0xEB, 0xBD, 0x55, 0x76, 0x98, 0x86, 0xBC, 0x65, 0x1D, 0x06, 0xB0, 0xCC, 0x53, 0xB0, 0xF6, 0x3B, 0xCE, 0x3C, 0x3E, 0x27, 0xD2, 0x60, 0x4B, 0x03, 0x15, 0x00, 0xC4, 0x9D, 0x36, 0x08, 0x86, 0xE7, 0x04, 0x93, 0x6A, 0x66, 0x78, 0xE1, 0x13, 0x9D, 0x26, 0xB7, 0x81, 0x9F, 0x7E, 0x90, 0x04, 0x41, 0x04, 0x6B, 0x17, 0xD1, 0xF2, 0xE1, 0x2C, 0x42, 0x47, 0xF8, 0xBC, 0xE6, 0xE5, 0x63, 0xA4, 0x40, 0xF2, 0x77, 0x03, 0x7D, 0x81, 0x2D, 0xEB, 0x33, 0xA0, 0xF4, 0xA1, 0x39, 0x45, 0xD8, 0x98, 0xC2, 0x96, 0x4F, 0xE3, 0x42, 0xE2, 0xFE, 0x1A, 0x7F, 0x9B, 0x8E, 0xE7, 0xEB, 0x4A, 0x7C, 0x0F, 0x9E, 0x16, 0x2B, 0xCE, 0x33, 0x57, 0x6B, 0x31, 0x5E, 0xCE, 0xCB, 0xB6, 0x40, 0x68, 0x37, 0xBF, 0x51, 0xF5, 0x02, 0x21, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xBC, 0xE6, 0xFA, 0xAD, 0xA7, 0x17, 0x9E, 0x84, 0xF3, 0xB9, 0xCA, 0xC2, 0xFC, 0x63, 0x25, 0x51, 0x02, 0x01, 0x01 };
// The prelude before the public key (1) elem + public key BIT STRING prelude
const uint8_t wz_secp_privateKey_format::secp256r1::publicKeyPrelude[] = { 0xA1, 0x44, 0x03, 0x42, 0x00 };
// 0x04 || X || Y  // (or, in other words, 0x04 + 32 byte X + 32 byte Y)
const uint8_t wz_secp_privateKey_format::secp256r1::numPublicKeyBytes = 65;

// The curves currently supported by the code
enum CurveID: int {
	secp224r1 = 224,
	secp256r1 = 256
};

// Used to convert between raw private + public key bytes and the
// DER ECPrivateKey representation that prior versions of Wz used.
//
// The expected format of the input private + public key bytes is:
//  - Private Key Bytes: The raw encoded representation of the privateKey OCTET STRING as stored in the ECPrivateKey structure.
// - Public Key Bytes: The raw encoded representation of the uncompressed (0x04) public key, with the 0x04 byte at the front.
class ecPrivateKeyDERExternalRepresentation {
public:
	const std::vector<uint8_t> privateKeyBytes;
	const std::vector<uint8_t> publicKeyBytes;
	const CurveID curve;
private:
	ecPrivateKeyDERExternalRepresentation(const std::vector<uint8_t> &privateKeyBytes, const std::vector<uint8_t> &publicKeyBytes, CurveID curve)
	:   privateKeyBytes(privateKeyBytes),
	publicKeyBytes(publicKeyBytes),
	curve(curve)
	{
	}

	struct ecCurveData {
		CurveID curveID;
		const uint8_t* prelude;
		size_t prelude_len;
		uint8_t numPrivateKeyBytes;
		const uint8_t* ecDomainParameters;
		size_t ecDomainParameters_len;
		const uint8_t* publicKeyPrelude;
		size_t publicKeyPrelude_len;
		uint8_t numPublicKeyBytes;

		size_t totalSize() const
		{
			return prelude_len + numPrivateKeyBytes + ecDomainParameters_len + publicKeyPrelude_len + numPublicKeyBytes;
		}

		size_t bytesBeforePrivateKey() const
		{
			return prelude_len;
		}

		size_t bytesBeforeECDomainParameters() const
		{
			return prelude_len + numPrivateKeyBytes;
		}

		size_t bytesBeforePublicKeyPrelude() const
		{
			return bytesBeforeECDomainParameters() + ecDomainParameters_len;
		}

		size_t bytesBeforePublicKey() const
		{
			return bytesBeforePublicKeyPrelude() + publicKeyPrelude_len;
		}
	};
public:
	static std::shared_ptr<ecPrivateKeyDERExternalRepresentation> createFromRawKeyBytes(CurveID curve, const std::vector<uint8_t> &privateKeyBytes, const std::vector<uint8_t>& publicKeyBytes)
	{
		return _makeExternalRepresentation(getCurveData(curve), privateKeyBytes, publicKeyBytes);
	}

	static std::shared_ptr<ecPrivateKeyDERExternalRepresentation> fromExternalRepresentation(const std::vector<uint8_t>& derECPrivateKey)
	{
		// Read in the DER external ECPrivateKey representation
		// Only supported 2 curves, currently

		std::shared_ptr<ecPrivateKeyDERExternalRepresentation> result(nullptr);

		// secp224r1
		result = _fromExternalRepresentation(curveData_secp224r1(), derECPrivateKey);
		if (result) return result;

		// secp256r1
		result = _fromExternalRepresentation(curveData_secp256r1(), derECPrivateKey);

		return result;
	}
public:
	// Produces the DER-encoded ECPrivateKey representation of a private key
	// This includes the optional ECDomainParameters & PublicKey.
	std::vector<uint8_t> toExternalRepresentationBytes() const
	{
		ecCurveData curveData = getCurveData(curve);
		assert(curveData.curveID == curve);

		std::vector<uint8_t> derECPrivateKey(curveData.totalSize());

		// + prelude
		memcpy(&derECPrivateKey[0], curveData.prelude, curveData.prelude_len);

		// + private key bytes
		assert(privateKeyBytes.size() == curveData.numPrivateKeyBytes);
		memcpy(&derECPrivateKey[curveData.bytesBeforePrivateKey()], &privateKeyBytes[0], privateKeyBytes.size());

		// + ECDomainParameters
		memcpy(&derECPrivateKey[curveData.bytesBeforeECDomainParameters()], curveData.ecDomainParameters, curveData.ecDomainParameters_len);

		// + prelude to public key
		memcpy(&derECPrivateKey[curveData.bytesBeforePublicKeyPrelude()], curveData.publicKeyPrelude, curveData.publicKeyPrelude_len);

		// + public key bytes
		assert(publicKeyBytes.size() == curveData.numPublicKeyBytes);
		assert(publicKeyBytes[0] == 0x04); // Uncompressed public keys only
		memcpy(&derECPrivateKey[curveData.bytesBeforePublicKey()], &publicKeyBytes[0], publicKeyBytes.size());

		return derECPrivateKey;
	}
private:
	static ecCurveData getCurveData(CurveID curve)
	{
		switch (curve)
		{
			case secp224r1:
				return curveData_secp224r1();
			case secp256r1:
				return curveData_secp256r1();
			default:
				debug(LOG_FATAL, "Unimplemented curve ID");
				return ecCurveData { };
		}
	}
	static ecCurveData curveData_secp224r1()
	{
		ecCurveData data = { };
		data.curveID = secp224r1;
		data.prelude = wz_secp_privateKey_format::secp224r1::prelude;
		data.prelude_len = sizeof(wz_secp_privateKey_format::secp224r1::prelude);
		data.numPrivateKeyBytes = wz_secp_privateKey_format::secp224r1::numPrivateKeyBytes;
		data.ecDomainParameters = wz_secp_privateKey_format::secp224r1::ecDomainParameters;
		data.ecDomainParameters_len = sizeof(wz_secp_privateKey_format::secp224r1::ecDomainParameters);
		data.publicKeyPrelude = wz_secp_privateKey_format::secp224r1::publicKeyPrelude;
		data.publicKeyPrelude_len = sizeof(wz_secp_privateKey_format::secp224r1::publicKeyPrelude);
		data.numPublicKeyBytes = wz_secp_privateKey_format::secp224r1::numPublicKeyBytes;
		return data;
	}
	static ecCurveData curveData_secp256r1()
	{
		ecCurveData data = { };
		data.curveID = secp256r1;
		data.prelude = wz_secp_privateKey_format::secp256r1::prelude;
		data.prelude_len = sizeof(wz_secp_privateKey_format::secp256r1::prelude);
		data.numPrivateKeyBytes = wz_secp_privateKey_format::secp256r1::numPrivateKeyBytes;
		data.ecDomainParameters = wz_secp_privateKey_format::secp256r1::ecDomainParameters;
		data.ecDomainParameters_len = sizeof(wz_secp_privateKey_format::secp256r1::ecDomainParameters);
		data.publicKeyPrelude = wz_secp_privateKey_format::secp256r1::publicKeyPrelude;
		data.publicKeyPrelude_len = sizeof(wz_secp_privateKey_format::secp256r1::publicKeyPrelude);
		data.numPublicKeyBytes = wz_secp_privateKey_format::secp256r1::numPublicKeyBytes;
		return data;
	}

	static std::shared_ptr<ecPrivateKeyDERExternalRepresentation> _makeExternalRepresentation(const ecCurveData& curve, const std::vector<uint8_t>& privateKeyBytes, const std::vector<uint8_t>& publicKeyBytes)
	{
		if (curve.numPrivateKeyBytes != privateKeyBytes.size())
		{
			debug(LOG_ERROR, "Unexpected private key bytes length provided.");
			return nullptr;
		}
		if (curve.numPublicKeyBytes != publicKeyBytes.size())
		{
			debug(LOG_ERROR, "Unexpected public key bytes length provided.");
			return nullptr;
		}
		// verify that public key is uncompressed (should always start with byte 0x04)
		if (publicKeyBytes[0] != 0x04)
		{
			debug(LOG_ERROR, "Public key is compressed, or is in an unexpected format.");
			return nullptr;
		}

		return std::shared_ptr<ecPrivateKeyDERExternalRepresentation>(new ecPrivateKeyDERExternalRepresentation(privateKeyBytes, publicKeyBytes, curve.curveID));
	}

	static std::shared_ptr<ecPrivateKeyDERExternalRepresentation> _fromExternalRepresentation(const ecCurveData& curve, const std::vector<uint8_t>& derECPrivateKey)
	{
		if (derECPrivateKey.size() != curve.totalSize())
		{
			debug(LOG_ERROR, "Unexpected external private key format length.");
			return nullptr;
		}
		// check that the DER ECPrivateKey begins with the expected prelude bytes
		if (!match(curve.prelude, curve.prelude_len, derECPrivateKey.begin()))
		{
			debug(LOG_ERROR, "Unexpected external private key format: prelude bytes do not match");
			return nullptr;
		}
		// check that the DER ECPrivateKey has the expected ECDomainParameters
		if (!match(curve.ecDomainParameters, curve.ecDomainParameters_len,  derECPrivateKey.begin() + curve.bytesBeforeECDomainParameters()))
		{
			debug(LOG_ERROR, "Unexpected external private key format: ECDomainParameters do not match");
			return nullptr;
		}
		// check that the DER ECPrivateKey has the expected public key prelude
		if (!match(curve.publicKeyPrelude, curve.publicKeyPrelude_len,  derECPrivateKey.begin() + curve.bytesBeforePublicKeyPrelude()))
		{
			debug(LOG_ERROR, "Unexpected external private key format: public key prelude bytes do not match");
			return nullptr;
		}

		// get the private and public key bytes
		auto privateKeyBegin = derECPrivateKey.begin() + curve.bytesBeforePrivateKey();
		std::vector<uint8_t> privateKeyBytes(privateKeyBegin, privateKeyBegin + curve.numPrivateKeyBytes);
		auto publicKeyBegin = derECPrivateKey.begin() + curve.bytesBeforePublicKey();
		std::vector<uint8_t> publicKeyBytes(publicKeyBegin, publicKeyBegin + curve.numPublicKeyBytes);

		assert(privateKeyBytes.size() == curve.numPrivateKeyBytes);
		assert(publicKeyBytes.size() == curve.numPublicKeyBytes);

		return std::shared_ptr<ecPrivateKeyDERExternalRepresentation>(new ecPrivateKeyDERExternalRepresentation(privateKeyBytes, publicKeyBytes, curve.curveID));
	}

	static bool match(const uint8_t* matching, size_t matchingLen, std::vector<uint8_t>::const_iterator target)
	{
		size_t count = 0;
		while (count < matchingLen && *matching == *target) {
			++count; ++matching; ++target;
		}
		return count == matchingLen;
	}
};

//================================================================================
// MARK: - ECDSA
//================================================================================

#include <micro-ecc/uECC.h>

const CurveID curveID = secp224r1;
const int EcKey::curveId = curveID; // Not used for anything in this uECC implementation

uECC_Curve _currentCurve() {
	switch (curveID) {
		case secp224r1:
			return uECC_secp224r1();
		case secp256r1:
			return uECC_secp256r1();
		default:
			debug(LOG_ERROR, "Unsupported EC curve - falling back to secp224r1");
			return uECC_secp224r1();
	}
}

size_t _currentCurveSizeInBytes()
{
	return curveID / 8;
}

size_t _currentSignatureSizeInBytes()
{
	// signature is always 2 * curveSizeInBytes
	return 2 * _currentCurveSizeInBytes();
}

#define currentECCurve _currentCurve()
#define currentCurveSizeInBytes _currentCurveSizeInBytes()
#define currentSignatureSizeInBytes _currentSignatureSizeInBytes()

struct EC_KEY
{
	std::vector<uint8_t> privateKey;
	std::vector<uint8_t> publicKey;

	EC_KEY(const std::vector<uint8_t>& privateKey, const std::vector<uint8_t>& publicKey)
		: privateKey(privateKey)
		, publicKey(publicKey)
	{
	}

	EC_KEY(EC_KEY const &b)
		: privateKey(b.privateKey)
		, publicKey(b.publicKey)
	{
	}

	EC_KEY(EC_KEY &&b)
		: privateKey()
		, publicKey()
	{
		std::swap(privateKey, b.privateKey);
		std::swap(publicKey, b.publicKey);
	}

	// Automatically initializes an EC_KEY structure with empty private and public key
	// vectors that are of the appropriate size (based on EcKey::curveId).
	static EC_KEY createAndReserveForCurve(uECC_Curve curve)
	{
		return EC_KEY(std::vector<uint8_t>(uECC_curve_private_key_size(curve)), std::vector<uint8_t>(uECC_curve_public_key_size(curve)));
	}
};

// External public key format is:
//  0x04 || X || Y
// Internal public key format is:
//  X || Y
//
// Returns an EcKey::Key in the internal (uECC) expected format, or an empty EcKey::Key upon failure.
EcKey::Key ecPublicKey_ExternalToMicroECCFormat(const std::vector<uint8_t>& externalPublicKey)
{
	// External public key should have an additional byte at the beginning
	if (uECC_curve_public_key_size(currentECCurve) + 1 != externalPublicKey.size())
	{
		debug(LOG_ERROR, "Invalid public key length");
		return EcKey::Key();
	}

	// Check the first byte (only uncompressed public keys are supported)
	if (externalPublicKey[0] != 0x04)
	{
		debug(LOG_ERROR, "Only uncompressed public keys are supported.");
		return EcKey::Key();
	}

	// Remove the initial 0x04 byte
	EcKey::Key publicKeyInternalFormat;
	publicKeyInternalFormat.reserve(externalPublicKey.size() - 1);
	publicKeyInternalFormat.insert(std::end(publicKeyInternalFormat), std::begin(externalPublicKey) + 1, std::end(externalPublicKey));

	return publicKeyInternalFormat;
}

// Returns a std::vector<uint8_t> containing the public key in the external format
// (i.e. with a 0x04 byte added to the front to denote an uncompressed public key)
std::vector<uint8_t> ecPublicKey_MicroECCFormatToExternal(const EcKey::Key& internalPublicKey)
{
	// uECC's public keys are just the raw bytes of the uncompressed key, *without* an 0x04 at the front.
	// Thus we must add 0x04 in front of uECC's public key bytes.

	EcKey::Key outputPublicKey;
	outputPublicKey.reserve(internalPublicKey.size() + 1);
	outputPublicKey.push_back(0x04);
	outputPublicKey.insert(std::end(outputPublicKey), std::begin(internalPublicKey), std::end(internalPublicKey));

	return outputPublicKey;
}


#define EC_KEY_CAST(k) ((EC_KEY *)k)


EcKey::EcKey()
	: vKey(nullptr)
{}

EcKey::EcKey(EcKey const &b)
{
	vKey = b.vKey != nullptr ? new EC_KEY(*EC_KEY_CAST(b.vKey)) : nullptr;
}

EcKey::EcKey(EcKey &&b)
	: vKey(nullptr)
{
	std::swap(vKey, b.vKey);
}

EcKey::~EcKey()
{
	clear();
}

EcKey &EcKey::operator =(EcKey const &b)
{
	clear();
	vKey = b.vKey != nullptr ? new EC_KEY(*EC_KEY_CAST(b.vKey)) : nullptr;
	return *this;
}

EcKey &EcKey::operator =(EcKey &&b)
{
	std::swap(vKey, b.vKey);
	return *this;
}

void EcKey::clear()
{
	delete (EC_KEY *)vKey;
	vKey = nullptr;
}

bool EcKey::empty() const
{
	return vKey == nullptr;
}

bool EcKey::hasPrivate() const
{
	return vKey != nullptr && !((EC_KEY *)vKey)->privateKey.empty();
}

EcKey::Sig EcKey::sign(void const *data, size_t dataLen) const
{
	if (vKey == nullptr || EC_KEY_CAST(vKey)->privateKey.empty())
	{
		debug(LOG_ERROR, "No key");
		return Sig();
	}

	Sig signature(currentSignatureSizeInBytes);

	if (dataLen > std::numeric_limits<unsigned int>::max())
	{
		debug(LOG_ERROR, "Overflow. Data to sign has a greater length than supported. You should probably be hashing and signing the hash.");
		return Sig();
	}
	unsigned int uIntDataLen = (unsigned int)dataLen;

	auto privateKey = EC_KEY_CAST(vKey)->privateKey;
	auto publicKey = EC_KEY_CAST(vKey)->publicKey;

	int signSuccess = uECC_sign(&privateKey[0], (const uint8_t *)data, uIntDataLen, &signature[0], currentECCurve);

	if (signSuccess == 0)
	{
		debug(LOG_ERROR, "Creating a signature failed");
		return Sig();
	}

	assert(signSuccess == 1);

	return signature;
}

bool EcKey::verify(Sig const &sig, void const *data, size_t dataLen) const
{
	if (vKey == nullptr || EC_KEY_CAST(vKey)->publicKey.empty())
	{
		debug(LOG_ERROR, "No key");
		return false;
	}

	if (sig.size() != currentSignatureSizeInBytes)
	{
		debug(LOG_ERROR, "Signature is the wrong size");
		return false;
	}

	int verifyResult = uECC_verify(&(EC_KEY_CAST(vKey)->publicKey[0]), (const uint8_t *)data, dataLen, &sig[0], currentECCurve);
	if (verifyResult == 0)
	{
		debug(LOG_ERROR, "Invalid signature");
		return false;
	}

	assert(verifyResult == 1);
	return true;
}


EcKey::Key EcKey::toBytes(Privacy privacy) const
{
	if (empty())
	{
		debugBacktrace(LOG_ERROR, "No key");
		return Key();
	}
	assert(EC_KEY_CAST(vKey) != nullptr);
	assert(EC_KEY_CAST(vKey)->publicKey.size() == uECC_curve_public_key_size(currentECCurve));

	// Convert internal public key format to expected external format (0x04 || X || Y).
	EcKey::Key outputPublicKey = ecPublicKey_MicroECCFormatToExternal(EC_KEY_CAST(vKey)->publicKey);

	if (privacy == Public)
	{
		return outputPublicKey;
	}
	else if (privacy == Private)
	{
		std::shared_ptr<ecPrivateKeyDERExternalRepresentation> externalPrivateKey =  ecPrivateKeyDERExternalRepresentation::createFromRawKeyBytes(curveID, EC_KEY_CAST(vKey)->privateKey, outputPublicKey);

		if (!externalPrivateKey)
		{
			debug(LOG_ERROR, "Failed to create external representation of private key");
			return Key();
		}

		return externalPrivateKey->toExternalRepresentationBytes();
	}
	else
	{
		debug(LOG_FATAL, "Unsupported privacy level");
		return Key();
	}
}

void EcKey::fromBytes(EcKey::Key const &key, EcKey::Privacy privacy)
{
	clear();

	if (privacy == Public)
	{
		// Convert public key from external represenation to internal
		EcKey::Key publicKeyInternalFormat = ecPublicKey_ExternalToMicroECCFormat(key);
		if (publicKeyInternalFormat.empty())
		{
			debug(LOG_ERROR, "Failed to convert public key from external representation to internal format");
			return;
		}

		int validResult = uECC_valid_public_key(&publicKeyInternalFormat[0], currentECCurve);
		if (validResult == 0)
		{
			debug(LOG_ERROR, "Invalid public key");
			return;
		}
		assert(validResult == 1);

		vKey = new EC_KEY(std::vector<uint8_t>(0), publicKeyInternalFormat);
	}
	else if (privacy == Private)
	{
		// Process DER External ECPrivateKey representation
		std::shared_ptr<ecPrivateKeyDERExternalRepresentation> externalKey = ecPrivateKeyDERExternalRepresentation::fromExternalRepresentation(key);
		if (!externalKey)
		{
			// Invalid input
			debug(LOG_ERROR, "Failed to import private key from external representation");
			return;
		}

		// Verify the expected curve has been provided
		if (externalKey->curve != curveID)
		{
			debug(LOG_ERROR, "External key is not of the expected EC curve");
			return;
		}

		// Convert public key from external represenation to internal
		EcKey::Key publicKeyInternalFormat = ecPublicKey_ExternalToMicroECCFormat(externalKey->publicKeyBytes);
		if (publicKeyInternalFormat.empty())
		{
			debug(LOG_ERROR, "Failed to convert public key from external representation to internal format");
			return;
		}

		// Verify public key is valid
		if (uECC_curve_public_key_size(currentECCurve) != publicKeyInternalFormat.size())
		{
			debug(LOG_ERROR, "Invalid public key length");
			return;
		}
		int validResult = uECC_valid_public_key(&publicKeyInternalFormat[0], currentECCurve);
		if (validResult == 0)
		{
			debug(LOG_ERROR, "Invalid public key");
			return;
		}
		assert(validResult == 1);

		// Verify private key is expected length
		if (uECC_curve_private_key_size(currentECCurve) != externalKey->privateKeyBytes.size())
		{
			debug(LOG_ERROR, "Invalid private key length");
			return;
		}

		vKey = new EC_KEY(externalKey->privateKeyBytes, publicKeyInternalFormat);
	}
	else
	{
		debug(LOG_FATAL, "Unsupported privacy level");
	}
}

EcKey EcKey::generate()
{
	EC_KEY * key = new EC_KEY(EC_KEY::createAndReserveForCurve(currentECCurve));

	int genResult = uECC_make_key(&(key->publicKey[0]), &(key->privateKey[0]), currentECCurve);
	if (genResult == 0)
	{
		// uECC_make_key failed
		debug(LOG_ERROR, "Failed to generate key pair");
		delete key;
		return EcKey();
	}
	assert(genResult == 1);

	EcKey ret;
	ret.vKey = (void *)key;
	return ret;
}

//================================================================================
// MARK: - Base64
//================================================================================

std::string base64Encode(std::vector<uint8_t> const &bytes)
{
	std::string str((bytes.size() + 2) / 3 * 4, '\0');
	for (unsigned n = 0; n * 3 < bytes.size(); ++n)
	{
		unsigned rem = bytes.size() - n * 3;
		unsigned block = bytes[0 + n * 3] << 16 | (rem > 1 ? bytes[1 + n * 3] : 0) << 8 | (rem > 2 ? bytes[2 + n * 3] : 0);
		for (unsigned i = 0; i < 4; ++i)
		{
			str[i + n * 4] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"[(block >> (6 * (3 - i))) & 0x3F];
		}
		if (rem <= 2)
		{
			str[3 + n * 4] = '=';
			if (rem <= 1)
			{
				str[2 + n * 4] = '=';
			}
		}
	}
	return str;
}

std::vector<uint8_t> base64Decode(std::string const &str)
{
	std::vector<uint8_t> bytes(str.size() / 4 * 3);
	for (unsigned n = 0; n < str.size() / 4; ++n)
	{
		unsigned block = 0;
		for (unsigned i = 0; i < 4; ++i)
		{
			uint8_t ch = str[i + n * 4];
			unsigned val = ch >= 'A' && ch <= 'Z' ? ch - 'A' + 0 :
			               ch >= 'a' && ch <= 'z' ? ch - 'a' + 26 :
			               ch >= '0' && ch <= '9' ? ch - '0' + 52 :
			               ch == '+' ? 62 :
			               ch == '/' ? 63 :
			               0;
			block |= val << (6 * (3 - i));
		}
		bytes[0 + n * 3] = block >> 16;
		bytes[1 + n * 3] = block >> 8;
		bytes[2 + n * 3] = block;
	}
	if (str.size() >= 4)
	{
		if (str[str.size() / 4 * 4 - 2] == '=')
		{
			bytes.resize(bytes.size() - 2);
		}
		else if (str[str.size() / 4 * 4 - 1] == '=')
		{
			bytes.resize(bytes.size() - 1);
		}
	}
	return bytes;
}
