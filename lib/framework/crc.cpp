/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2020  Warzone 2100 Project

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
#include <algorithm>

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
		crc = crc << 8 ^ crcTable[crc>>24 ^ uint8_t(*data >> 8)];
		crc = crc << 8 ^ crcTable[crc>>24 ^ uint8_t(*data++)];
	}

	return crc;
}

uint32_t crcSumI16(uint32_t crc, const int16_t *data, size_t dataLen)
{
	while (dataLen-- > 0)
	{
		crc = crc << 8 ^ crcTable[crc>>24 ^ uint8_t(*data >> 8)];
		crc = crc << 8 ^ crcTable[crc>>24 ^ uint8_t(*data++)];
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

#include <sodium.h>
#include <climits>
Sha256 sha256Sum(void const *data, size_t dataLen)
{
	static_assert(Sha256::Bytes == crypto_hash_sha256_BYTES, "Size mismatch.");

	Sha256 ret;
	if (data == nullptr)
	{
		ASSERT(data != nullptr, "Called with null data pointer");
		ret.setZero();
		return ret;
	}
#if SIZE_MAX > ULLONG_MAX
	if (dataLen > std::numeric_limits<unsigned long long>::max())
	{
		debug(LOG_FATAL, "Attempting to calculate SHA256 on data length exceeding std::numeric_limits<unsigned long long>::max()=(%llu)", std::numeric_limits<unsigned long long>::max());
		ret.setZero();
		return ret;
	}
#endif

	crypto_hash_sha256_state state;
	crypto_hash_sha256_init(&state);
	crypto_hash_sha256_update(&state, (const unsigned char *)data, dataLen);
	crypto_hash_sha256_final(&state, ret.bytes);
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

static inline std::string bytesToHex(const uint8_t* bytes, size_t numBytes)
{
	std::string str;
	str.resize(numBytes * 2);
	char const *hexDigits = "0123456789abcdef";
	for (size_t n = 0; n < numBytes; ++n)
	{
		str[n * 2    ] = hexDigits[bytes[n] >> 4];
		str[n * 2 + 1] = hexDigits[bytes[n] & 15];
	}
	return str;
}

std::string Sha256::toString() const
{
	return bytesToHex(bytes, Bytes);
}

void Sha256::fromString(std::string const &s)
{
	setZero();
	size_t nChars = std::min<size_t>(Bytes * 2, s.size());
	for (size_t n = 0; n < nChars; ++n)
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
// MARK: - ECDSA
//================================================================================

size_t _currentSignatureSizeInBytes()
{
	return crypto_sign_ed25519_BYTES;
}

#define currentSignatureSizeInBytes _currentSignatureSizeInBytes()

struct EC_KEY
{
	std::vector<unsigned char> privateKey;
	std::vector<unsigned char> publicKey;

	EC_KEY(const std::vector<unsigned char>& privateKey, const std::vector<unsigned char>& publicKey)
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
	// vectors that are of the appropriate size.
	static EC_KEY createAndReserveForCurve()
	{
		return EC_KEY(std::vector<unsigned char>(crypto_sign_ed25519_SECRETKEYBYTES), std::vector<unsigned char>(crypto_sign_ed25519_PUBLICKEYBYTES));
	}
};

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

	if (dataLen > std::numeric_limits<unsigned long long>::max())
	{
		debug(LOG_ERROR, "Overflow. Data to sign has a greater length than supported. You should probably be hashing and signing the hash.");
		return Sig();
	}
	unsigned long long uLLDataLen = (unsigned long long)dataLen;

	auto privateKey = EC_KEY_CAST(vKey)->privateKey;

	int signSuccess = crypto_sign_ed25519_detached(&signature[0], nullptr, (const unsigned char *)data, uLLDataLen, &privateKey[0]);
	if (signSuccess != 0)
	{
		debug(LOG_ERROR, "Creating a signature failed");
		return Sig();
	}

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
		debug(LOG_INFO, "Signature is the wrong size (received: %zu, expecting: %zu)", sig.size(), currentSignatureSizeInBytes);
		return false;
	}

#if SIZE_MAX > ULLONG_MAX
	if (dataLen > std::numeric_limits<unsigned long long>::max())
	{
		debug(LOG_ERROR, "Attempting to verify signature on data length (%zu) exceeding std::numeric_limits<unsigned long long>::max()=(%ull)", dataLen, std::numeric_limits<unsigned long long>::max());
		return false;
	}
#endif

	int verifyResult = crypto_sign_ed25519_verify_detached(&sig[0], (const unsigned char *)data, static_cast<unsigned long long>(dataLen), &(EC_KEY_CAST(vKey)->publicKey[0]));
	if (verifyResult != 0)
	{
		return false;
	}

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
	assert(EC_KEY_CAST(vKey)->publicKey.size() == crypto_sign_ed25519_PUBLICKEYBYTES);

	if (privacy == Public)
	{
		return EC_KEY_CAST(vKey)->publicKey;
	}
	else if (privacy == Private)
	{
		if (EC_KEY_CAST(vKey)->privateKey.size() != crypto_sign_ed25519_SECRETKEYBYTES)
		{
			debug(LOG_ERROR, "Failed to create external representation of private key");
			return Key();
		}

		return EC_KEY_CAST(vKey)->privateKey;
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
		if (key.size() != crypto_sign_ed25519_PUBLICKEYBYTES)
		{
			debug(LOG_ERROR, "Invalid public key");
			return;
		}

		vKey = new EC_KEY(std::vector<unsigned char>(0), key);
	}
	else if (privacy == Private)
	{
		// Verify private key is expected length
		if (key.size() != crypto_sign_ed25519_SECRETKEYBYTES)
		{
			// Invalid input
			debug(LOG_ERROR, "Invalid private key length");
			return;
		}

		// Compute public key from private key
		std::vector<unsigned char> publicKey(crypto_sign_ed25519_PUBLICKEYBYTES);
		if (crypto_sign_ed25519_sk_to_pk(&publicKey[0], &key[0]) != 0)
		{
			// Failed to compute public key from private key
			debug(LOG_ERROR, "Invalid private key");
			return;
		}

		vKey = new EC_KEY(key, publicKey);
	}
	else
	{
		debug(LOG_FATAL, "Unsupported privacy level");
	}
}

EcKey EcKey::generate()
{
	EC_KEY * key = new EC_KEY(EC_KEY::createAndReserveForCurve());

	int genResult = crypto_sign_ed25519_keypair(&(key->publicKey[0]), &(key->privateKey[0]));
	if (genResult != 0)
	{
		// crypto_sign_ed25519_keypair failed
		debug(LOG_ERROR, "Failed to generate key pair");
		delete key;
		return EcKey();
	}

	EcKey ret;
	ret.vKey = (void *)key;
	return ret;
}

std::string EcKey::publicHashString(size_t truncateToLength /*= 0*/) const
{
	auto bytes = toBytes(EcKey::Public);
	if (bytes.empty())
	{
		return std::string{};
	}
	auto shaStr = sha256Sum(&bytes[0], bytes.size()).toString();
	if (truncateToLength > 0 && truncateToLength < shaStr.length())
	{
		return shaStr.substr(0, truncateToLength);
	}
	return shaStr;
}

std::string EcKey::publicKeyHexString(size_t truncateToLength /*= 0*/) const
{
	auto bytes = toBytes(EcKey::Public);
	if (bytes.empty())
	{
		return std::string{};
	}
	return bytesToHex(bytes.data(), bytes.size());
}


//================================================================================
// MARK: - Base64
//================================================================================

std::string base64Encode(std::vector<uint8_t> const &bytes)
{
	std::string str((bytes.size() + 2) / 3 * 4, '\0');
	for (unsigned n = 0; n * 3 < bytes.size(); ++n)
	{
		unsigned rem = static_cast<unsigned>(bytes.size()) - n * 3;
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
		bytes[0 + n * 3] = static_cast<uint8_t>(block >> 16);
		bytes[1 + n * 3] = static_cast<uint8_t>(block >> 8);
		bytes[2 + n * 3] = static_cast<uint8_t>(block);
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

std::string b64Tob64UrlSafe(const std::string& inputb64)
{
	std::string b64urlsafe = inputb64;
	std::replace(b64urlsafe.begin(), b64urlsafe.end(), '+', '-');
	std::replace(b64urlsafe.begin(), b64urlsafe.end(), '/', '_');
	return b64urlsafe;
}

std::string b64UrlSafeTob64(const std::string& inputb64urlsafe)
{
	std::string b64 = inputb64urlsafe;
	std::replace(b64.begin(), b64.end(), '-', '+');
	std::replace(b64.begin(), b64.end(), '_', '/');
	return b64;
}

//================================================================================
// MARK: - Generating Random Data
//================================================================================

std::vector<uint8_t> genSecRandomBytes(size_t numBytes)
{
	std::vector<uint8_t> result(numBytes);
	randombytes_buf(&result[0], result.size());
	return result;
}

void genSecRandomBytes(void * const buf, const size_t size)
{
	ASSERT_OR_RETURN(, buf != nullptr, "Null buffer!!");
	randombytes_buf(buf, size);
}
