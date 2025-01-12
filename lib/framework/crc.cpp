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
#include <array>

#include <nlohmann/json.hpp>

//================================================================================
// MARK: - Old CRCSum functions
//================================================================================

// Invariant:
// crcTable[0] = 0;
// crcTable[1] = 0x04C11DB7;
// crcTable[i] = crcTable[i>>1]<<1 ^ ((crcTable[i>>1]>>31 ^ (i & 0x01))*crcTable[1]);
static const uint32_t crcTable[256] = {0x00000000, 0x04C11DB7, 0x09823B6E, 0x0D4326D9, 0x130476DC, 0x17C56B6B, 0x1A864DB2, 0x1E475005, 0x2608EDB8, 0x22C9F00F, 0x2F8AD6D6, 0x2B4BCB61, 0x350C9B64, 0x31CD86D3, 0x3C8EA00A, 0x384FBDBD, 0x4C11DB70, 0x48D0C6C7, 0x4593E01E, 0x4152FDA9, 0x5F15ADAC, 0x5BD4B01B, 0x569796C2, 0x52568B75, 0x6A1936C8, 0x6ED82B7F, 0x639B0DA6, 0x675A1011, 0x791D4014, 0x7DDC5DA3, 0x709F7B7A, 0x745E66CD, 0x9823B6E0, 0x9CE2AB57, 0x91A18D8E, 0x95609039, 0x8B27C03C, 0x8FE6DD8B, 0x82A5FB52, 0x8664E6E5, 0xBE2B5B58, 0xBAEA46EF, 0xB7A96036, 0xB3687D81, 0xAD2F2D84, 0xA9EE3033, 0xA4AD16EA, 0xA06C0B5D, 0xD4326D90, 0xD0F37027, 0xDDB056FE, 0xD9714B49, 0xC7361B4C, 0xC3F706FB, 0xCEB42022, 0xCA753D95, 0xF23A8028, 0xF6FB9D9F, 0xFBB8BB46, 0xFF79A6F1, 0xE13EF6F4, 0xE5FFEB43, 0xE8BCCD9A, 0xEC7DD02D, 0x34867077, 0x30476DC0, 0x3D044B19, 0x39C556AE, 0x278206AB, 0x23431B1C, 0x2E003DC5, 0x2AC12072, 0x128E9DCF, 0x164F8078, 0x1B0CA6A1, 0x1FCDBB16, 0x018AEB13, 0x054BF6A4, 0x0808D07D, 0x0CC9CDCA, 0x7897AB07, 0x7C56B6B0, 0x71159069, 0x75D48DDE, 0x6B93DDDB, 0x6F52C06C, 0x6211E6B5, 0x66D0FB02, 0x5E9F46BF, 0x5A5E5B08, 0x571D7DD1, 0x53DC6066, 0x4D9B3063, 0x495A2DD4, 0x44190B0D, 0x40D816BA, 0xACA5C697, 0xA864DB20, 0xA527FDF9, 0xA1E6E04E, 0xBFA1B04B, 0xBB60ADFC, 0xB6238B25, 0xB2E29692, 0x8AAD2B2F, 0x8E6C3698, 0x832F1041, 0x87EE0DF6, 0x99A95DF3, 0x9D684044, 0x902B669D, 0x94EA7B2A, 0xE0B41DE7, 0xE4750050, 0xE9362689, 0xEDF73B3E, 0xF3B06B3B, 0xF771768C, 0xFA325055, 0xFEF34DE2, 0xC6BCF05F, 0xC27DEDE8, 0xCF3ECB31, 0xCBFFD686, 0xD5B88683, 0xD1799B34, 0xDC3ABDED, 0xD8FBA05A, 0x690CE0EE, 0x6DCDFD59, 0x608EDB80, 0x644FC637, 0x7A089632, 0x7EC98B85, 0x738AAD5C, 0x774BB0EB, 0x4F040D56, 0x4BC510E1, 0x46863638, 0x42472B8F, 0x5C007B8A, 0x58C1663D, 0x558240E4, 0x51435D53, 0x251D3B9E, 0x21DC2629, 0x2C9F00F0, 0x285E1D47, 0x36194D42, 0x32D850F5, 0x3F9B762C, 0x3B5A6B9B, 0x0315D626, 0x07D4CB91, 0x0A97ED48, 0x0E56F0FF, 0x1011A0FA, 0x14D0BD4D, 0x19939B94, 0x1D528623, 0xF12F560E, 0xF5EE4BB9, 0xF8AD6D60, 0xFC6C70D7, 0xE22B20D2, 0xE6EA3D65, 0xEBA91BBC, 0xEF68060B, 0xD727BBB6, 0xD3E6A601, 0xDEA580D8, 0xDA649D6F, 0xC423CD6A, 0xC0E2D0DD, 0xCDA1F604, 0xC960EBB3, 0xBD3E8D7E, 0xB9FF90C9, 0xB4BCB610, 0xB07DABA7, 0xAE3AFBA2, 0xAAFBE615, 0xA7B8C0CC, 0xA379DD7B, 0x9B3660C6, 0x9FF77D71, 0x92B45BA8, 0x9675461F, 0x8832161A, 0x8CF30BAD, 0x81B02D74, 0x857130C3, 0x5D8A9099, 0x594B8D2E, 0x5408ABF7, 0x50C9B640, 0x4E8EE645, 0x4A4FFBF2, 0x470CDD2B, 0x43CDC09C, 0x7B827D21, 0x7F436096, 0x7200464F, 0x76C15BF8, 0x68860BFD, 0x6C47164A, 0x61043093, 0x65C52D24, 0x119B4BE9, 0x155A565E, 0x18197087, 0x1CD86D30, 0x029F3D35, 0x065E2082, 0x0B1D065B, 0x0FDC1BEC, 0x3793A651, 0x3352BBE6, 0x3E119D3F, 0x3AD08088, 0x2497D08D, 0x2056CD3A, 0x2D15EBE3, 0x29D4F654, 0xC5A92679, 0xC1683BCE, 0xCC2B1D17, 0xC8EA00A0, 0xD6AD50A5, 0xD26C4D12, 0xDF2F6BCB, 0xDBEE767C, 0xE3A1CBC1, 0xE760D676, 0xEA23F0AF, 0xEEE2ED18, 0xF0A5BD1D, 0xF464A0AA, 0xF9278673, 0xFDE69BC4, 0x89B8FD09, 0x8D79E0BE, 0x803AC667, 0x84FBDBD0, 0x9ABC8BD5, 0x9E7D9662, 0x933EB0BB, 0x97FFAD0C, 0xAFB010B1, 0xAB710D06, 0xA6322BDF, 0xA2F33668, 0xBCB4666D, 0xB8757BDA, 0xB5365D03, 0xB1F740B4};


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

inline void to_json(nlohmann::json& j, const Sha256& k)
{
	if (k.isZero())
	{
		j = "";
		return;
	}
	j = k.toString();
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

#if SIZE_MAX > ULLONG_MAX
	if (dataLen > std::numeric_limits<unsigned long long>::max())
	{
		debug(LOG_ERROR, "Overflow. Data to sign has a greater length than supported. You should probably be hashing and signing the hash.");
		return Sig();
	}
#endif
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
		debug(LOG_WZ, "No key");
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

bool EcKey::fromBytes(EcKey::Key const &key, EcKey::Privacy privacy)
{
	clear();

	if (privacy == Public)
	{
		if (key.size() != crypto_sign_ed25519_PUBLICKEYBYTES)
		{
			debug(LOG_ERROR, "Invalid public key");
			return false;
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
			return false;
		}

		// Compute public key from private key
		std::vector<unsigned char> publicKey(crypto_sign_ed25519_PUBLICKEYBYTES);
		if (crypto_sign_ed25519_sk_to_pk(&publicKey[0], &key[0]) != 0)
		{
			// Failed to compute public key from private key
			debug(LOG_ERROR, "Invalid private key");
			return false;
		}

		vKey = new EC_KEY(key, publicKey);
	}
	else
	{
		debug(LOG_FATAL, "Unsupported privacy level");
		return false;
	}

	return true;
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

//================================================================================
// MARK: - SessionKeys
//================================================================================

static_assert(SessionKeys::NonceSize == crypto_aead_xchacha20poly1305_ietf_NPUBBYTES, "Mismatched nonce size");

SessionKeys::SessionKeys(EcKey const &me, uint32_t me_playerIdx, EcKey const &other, uint32_t other_playerIdx)
{
	if (!me.hasPrivate())
	{
		throw std::invalid_argument("Invalid \"me\" key");
	}

	// The libsodium API expects a deterministic "client" and "server"
	// To do so, sort the EcKeys by public key hex string
	// If they are somehow both equal (probably someone testing locally with the same config dir), use the player index instead
	auto me_publicKeyStr = me.publicKeyHexString();
	auto other_publicKeyStr = other.publicKeyHexString();
	bool bIsClient = (me_publicKeyStr != other_publicKeyStr) ? (me_publicKeyStr < other_publicKeyStr) : (me_playerIdx < other_playerIdx);

	std::vector<unsigned char> ed25519_pk = me.toBytes(EcKey::Public);
	std::vector<unsigned char> ed25519_sk = me.toBytes(EcKey::Private);
	std::vector<unsigned char> me_pk(crypto_scalarmult_curve25519_BYTES, 0);
	std::vector<unsigned char> me_sk(crypto_scalarmult_curve25519_BYTES, 0);
	if (crypto_sign_ed25519_pk_to_curve25519(&me_pk[0], ed25519_pk.data()) != 0)
	{
		throw std::invalid_argument("Invalid public key");
	}
	if (crypto_sign_ed25519_sk_to_curve25519(&me_sk[0], ed25519_sk.data()) != 0)
	{
		throw std::invalid_argument("Invalid private key");
	}
	sodium_memzero(&ed25519_sk[0], ed25519_sk.size());
	ed25519_sk.clear();

	std::vector<unsigned char> other_ed25519_pk = other.toBytes(EcKey::Public);
	std::vector<unsigned char> other_pk(crypto_scalarmult_curve25519_BYTES, 0);
	if (crypto_sign_ed25519_pk_to_curve25519(&other_pk[0], other_ed25519_pk.data()) != 0)
	{
		throw std::invalid_argument("Invalid \"other\" public key");
	}

	receiveKey.resize(crypto_kx_SESSIONKEYBYTES);
	sendKey.resize(crypto_kx_SESSIONKEYBYTES);

	if (bIsClient)
	{
		if (crypto_kx_client_session_keys(&receiveKey[0], &sendKey[0],
										  me_pk.data(), me_sk.data(), other_pk.data()) != 0)
		{
			// Suspicious other public key
			throw std::invalid_argument("Invalid \"other\" public key");
		}
	}
	else
	{
		if (crypto_kx_server_session_keys(&receiveKey[0], &sendKey[0],
										  me_pk.data(), me_sk.data(), other_pk.data()) != 0)
		{
			// Suspicious other public key
			throw std::invalid_argument("Invalid \"other\" public key");
		}
	}

	sodium_memzero(&me_sk[0], me_sk.size());
	me_sk.clear();
}

constexpr size_t sessionMessageBlockSize = 64;

std::vector<uint8_t> SessionKeys::encryptMessageForOther(void const *data, size_t dataLen)
{
#if SIZE_MAX > UINT32_MAX
	if (dataLen > static_cast<size_t>(std::numeric_limits<uint32_t>::max()))
	{
		debug(LOG_ERROR, "Attempting to encrypt message of data length (%zu) exceeding std::numeric_limits<uint32_t>::max()", dataLen);
		return {};
	}
#endif
	size_t bufferUnpaddedLen = dataLen;
	size_t bufferPaddedLen = 0;
	size_t bufferMaxLen = dataLen + (sessionMessageBlockSize - (dataLen % sessionMessageBlockSize));
	buffer.clear();
	buffer.reserve(bufferMaxLen);
	std::copy((const unsigned char*)data, (const unsigned char*)data + dataLen, std::back_inserter(buffer));
	buffer.resize(bufferMaxLen, 0);
	// Round the length of the buffer to a multiple of `sessionMessageBlockSize` by appending padding data
	if (sodium_pad(&bufferPaddedLen, &buffer[0], dataLen, sessionMessageBlockSize, buffer.size()) != 0)
	{
		ASSERT(false, "Not large enough output buffer for padding");
		return {};
	}
	ASSERT(bufferPaddedLen >= bufferUnpaddedLen, "Should not be smaller?");
	buffer.resize(bufferPaddedLen);

	std::vector<unsigned char> ciphertext(bufferPaddedLen + crypto_aead_xchacha20poly1305_ietf_ABYTES, 0);
	unsigned long long ciphertext_len = 0;

	std::array<unsigned char, NonceSize> nonce;
	randombytes_buf(&nonce[0], nonce.size());

	if (crypto_aead_xchacha20poly1305_ietf_encrypt(&ciphertext[0], &ciphertext_len,
												   buffer.data(), static_cast<unsigned long long>(buffer.size()),
											   NULL, 0,
											   NULL, nonce.data(), sendKey.data()) != 0)
	{
		// failed to encrypt
		debug(LOG_INFO, "Encryption failed?");
		return {};
	}

	// Return <nonce><ciphertext>
	std::vector<uint8_t> fullResult;
	fullResult.reserve(nonce.size() + ciphertext.size());
	std::copy(nonce.begin(), nonce.end(), std::back_inserter(fullResult));
	std::copy(ciphertext.begin(), ciphertext.end(), std::back_inserter(fullResult));
	return fullResult;
}

bool SessionKeys::decryptMessageFromOther(void const *data, size_t dataLen, std::vector<uint8_t>& decrypted)
{
	ASSERT_OR_RETURN(false, dataLen > crypto_aead_xchacha20poly1305_ietf_NPUBBYTES, "Invalid dataLen");
	ASSERT_OR_RETURN(false, dataLen < std::numeric_limits<uint32_t>::max(), "Invalid dataLen");

	// Split data: <nonce><ciphertext>
	const unsigned char *npub = (const unsigned char *)data;
	const unsigned char *ciphertext = ((const unsigned char *)data) + crypto_aead_xchacha20poly1305_ietf_NPUBBYTES;
	size_t ciphertextLen = dataLen - crypto_aead_xchacha20poly1305_ietf_NPUBBYTES;
	ASSERT_OR_RETURN({}, ciphertextLen >= crypto_aead_xchacha20poly1305_ietf_ABYTES, "Invalid dataLen");

	decrypted.resize(ciphertextLen, 0);
	unsigned long long decryptedLen = 0;

	if (crypto_aead_xchacha20poly1305_ietf_decrypt(&decrypted[0], &decryptedLen,
												   NULL,
												   ciphertext, static_cast<unsigned long long>(ciphertextLen),
												   NULL, 0,
												   npub, receiveKey.data()) != 0)
	{
		// Invalid / forged message
		ASSERT(false, "Invalid encrypted message");
		return false;
	}

	// Compute the original, unpadded length
	size_t bufferPaddedLen = static_cast<size_t>(decryptedLen);
	size_t bufferUnpaddedLen = 0;
	if (sodium_unpad(&bufferUnpaddedLen, decrypted.data(), bufferPaddedLen, sessionMessageBlockSize) != 0)
	{
		ASSERT(false, "Incorrect padding");
		return false;
	}

	decrypted.resize(bufferUnpaddedLen);
	return true;
}

//================================================================================
// MARK: - CRC
//================================================================================

// Endianness detection

#if !defined(WZ_IS_BIG_ENDIAN)
// Prefer the built-in endian macros provided by GCC 4.6+ and Clang 3.2+
#  if defined(__ORDER_LITTLE_ENDIAN__) && defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
#      define WZ_IS_BIG_ENDIAN 0
#  elif defined(__ORDER_BIG_ENDIAN__) && defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
#      define WZ_IS_BIG_ENDIAN 1
// Otherwise, if Windows, assume little endian
#  elif defined(_WIN32)
#    define WZ_IS_BIG_ENDIAN 0
// Otherwise, could fallback to other platform-specific things, but just output an error and expect WZ_IS_BIG_ENDIAN to be manually set
#  else
#    error Unable to detect endianness - manually define WZ_IS_BIG_ENDIAN to 0 (if little endian) or 1 (if big endian)
#  endif
#endif

typedef uint32_t crc_t;

/**
 * crc-32 model
 * using the configuration:
 *  - Width         = 32
 *  - Poly          = 0x04c11db7
 *  - XorIn         = 0xffffffff
 *  - ReflectIn     = True
 *  - XorOut        = 0xffffffff
 *  - ReflectOut    = True
 *  - Algorithm     = table-driven
 *  - SliceBy       = 8
 */

static const crc_t crc_table[8][256] = {
	{
		0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f, 0xe963a535, 0x9e6495a3,
		0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988, 0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91,
		0x1db71064, 0x6ab020f2, 0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
		0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9, 0xfa0f3d63, 0x8d080df5,
		0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172, 0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,
		0x35b5a8fa, 0x42b2986c, 0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
		0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423, 0xcfba9599, 0xb8bda50f,
		0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924, 0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d,
		0x76dc4190, 0x01db7106, 0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
		0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d, 0x91646c97, 0xe6635c01,
		0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e, 0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457,
		0x65b0d9c6, 0x12b7e950, 0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
		0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7, 0xa4d1c46d, 0xd3d6f4fb,
		0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0, 0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9,
		0x5005713c, 0x270241aa, 0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
		0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81, 0xb7bd5c3b, 0xc0ba6cad,
		0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a, 0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683,
		0xe3630b12, 0x94643b84, 0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
		0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb, 0x196c3671, 0x6e6b06e7,
		0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc, 0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5,
		0xd6d6a3e8, 0xa1d1937e, 0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
		0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55, 0x316e8eef, 0x4669be79,
		0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236, 0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f,
		0xc5ba3bbe, 0xb2bd0b28, 0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
		0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f, 0x72076785, 0x05005713,
		0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38, 0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21,
		0x86d3d2d4, 0xf1d4e242, 0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
		0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69, 0x616bffd3, 0x166ccf45,
		0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2, 0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db,
		0xaed16a4a, 0xd9d65adc, 0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
		0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693, 0x54de5729, 0x23d967bf,
		0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94, 0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
	},
	{
		0x00000000, 0x191b3141, 0x32366282, 0x2b2d53c3, 0x646cc504, 0x7d77f445, 0x565aa786, 0x4f4196c7,
		0xc8d98a08, 0xd1c2bb49, 0xfaefe88a, 0xe3f4d9cb, 0xacb54f0c, 0xb5ae7e4d, 0x9e832d8e, 0x87981ccf,
		0x4ac21251, 0x53d92310, 0x78f470d3, 0x61ef4192, 0x2eaed755, 0x37b5e614, 0x1c98b5d7, 0x05838496,
		0x821b9859, 0x9b00a918, 0xb02dfadb, 0xa936cb9a, 0xe6775d5d, 0xff6c6c1c, 0xd4413fdf, 0xcd5a0e9e,
		0x958424a2, 0x8c9f15e3, 0xa7b24620, 0xbea97761, 0xf1e8e1a6, 0xe8f3d0e7, 0xc3de8324, 0xdac5b265,
		0x5d5daeaa, 0x44469feb, 0x6f6bcc28, 0x7670fd69, 0x39316bae, 0x202a5aef, 0x0b07092c, 0x121c386d,
		0xdf4636f3, 0xc65d07b2, 0xed705471, 0xf46b6530, 0xbb2af3f7, 0xa231c2b6, 0x891c9175, 0x9007a034,
		0x179fbcfb, 0x0e848dba, 0x25a9de79, 0x3cb2ef38, 0x73f379ff, 0x6ae848be, 0x41c51b7d, 0x58de2a3c,
		0xf0794f05, 0xe9627e44, 0xc24f2d87, 0xdb541cc6, 0x94158a01, 0x8d0ebb40, 0xa623e883, 0xbf38d9c2,
		0x38a0c50d, 0x21bbf44c, 0x0a96a78f, 0x138d96ce, 0x5ccc0009, 0x45d73148, 0x6efa628b, 0x77e153ca,
		0xbabb5d54, 0xa3a06c15, 0x888d3fd6, 0x91960e97, 0xded79850, 0xc7cca911, 0xece1fad2, 0xf5facb93,
		0x7262d75c, 0x6b79e61d, 0x4054b5de, 0x594f849f, 0x160e1258, 0x0f152319, 0x243870da, 0x3d23419b,
		0x65fd6ba7, 0x7ce65ae6, 0x57cb0925, 0x4ed03864, 0x0191aea3, 0x188a9fe2, 0x33a7cc21, 0x2abcfd60,
		0xad24e1af, 0xb43fd0ee, 0x9f12832d, 0x8609b26c, 0xc94824ab, 0xd05315ea, 0xfb7e4629, 0xe2657768,
		0x2f3f79f6, 0x362448b7, 0x1d091b74, 0x04122a35, 0x4b53bcf2, 0x52488db3, 0x7965de70, 0x607eef31,
		0xe7e6f3fe, 0xfefdc2bf, 0xd5d0917c, 0xcccba03d, 0x838a36fa, 0x9a9107bb, 0xb1bc5478, 0xa8a76539,
		0x3b83984b, 0x2298a90a, 0x09b5fac9, 0x10aecb88, 0x5fef5d4f, 0x46f46c0e, 0x6dd93fcd, 0x74c20e8c,
		0xf35a1243, 0xea412302, 0xc16c70c1, 0xd8774180, 0x9736d747, 0x8e2de606, 0xa500b5c5, 0xbc1b8484,
		0x71418a1a, 0x685abb5b, 0x4377e898, 0x5a6cd9d9, 0x152d4f1e, 0x0c367e5f, 0x271b2d9c, 0x3e001cdd,
		0xb9980012, 0xa0833153, 0x8bae6290, 0x92b553d1, 0xddf4c516, 0xc4eff457, 0xefc2a794, 0xf6d996d5,
		0xae07bce9, 0xb71c8da8, 0x9c31de6b, 0x852aef2a, 0xca6b79ed, 0xd37048ac, 0xf85d1b6f, 0xe1462a2e,
		0x66de36e1, 0x7fc507a0, 0x54e85463, 0x4df36522, 0x02b2f3e5, 0x1ba9c2a4, 0x30849167, 0x299fa026,
		0xe4c5aeb8, 0xfdde9ff9, 0xd6f3cc3a, 0xcfe8fd7b, 0x80a96bbc, 0x99b25afd, 0xb29f093e, 0xab84387f,
		0x2c1c24b0, 0x350715f1, 0x1e2a4632, 0x07317773, 0x4870e1b4, 0x516bd0f5, 0x7a468336, 0x635db277,
		0xcbfad74e, 0xd2e1e60f, 0xf9ccb5cc, 0xe0d7848d, 0xaf96124a, 0xb68d230b, 0x9da070c8, 0x84bb4189,
		0x03235d46, 0x1a386c07, 0x31153fc4, 0x280e0e85, 0x674f9842, 0x7e54a903, 0x5579fac0, 0x4c62cb81,
		0x8138c51f, 0x9823f45e, 0xb30ea79d, 0xaa1596dc, 0xe554001b, 0xfc4f315a, 0xd7626299, 0xce7953d8,
		0x49e14f17, 0x50fa7e56, 0x7bd72d95, 0x62cc1cd4, 0x2d8d8a13, 0x3496bb52, 0x1fbbe891, 0x06a0d9d0,
		0x5e7ef3ec, 0x4765c2ad, 0x6c48916e, 0x7553a02f, 0x3a1236e8, 0x230907a9, 0x0824546a, 0x113f652b,
		0x96a779e4, 0x8fbc48a5, 0xa4911b66, 0xbd8a2a27, 0xf2cbbce0, 0xebd08da1, 0xc0fdde62, 0xd9e6ef23,
		0x14bce1bd, 0x0da7d0fc, 0x268a833f, 0x3f91b27e, 0x70d024b9, 0x69cb15f8, 0x42e6463b, 0x5bfd777a,
		0xdc656bb5, 0xc57e5af4, 0xee530937, 0xf7483876, 0xb809aeb1, 0xa1129ff0, 0x8a3fcc33, 0x9324fd72
	},
	{
		0x00000000, 0x01c26a37, 0x0384d46e, 0x0246be59, 0x0709a8dc, 0x06cbc2eb, 0x048d7cb2, 0x054f1685,
		0x0e1351b8, 0x0fd13b8f, 0x0d9785d6, 0x0c55efe1, 0x091af964, 0x08d89353, 0x0a9e2d0a, 0x0b5c473d,
		0x1c26a370, 0x1de4c947, 0x1fa2771e, 0x1e601d29, 0x1b2f0bac, 0x1aed619b, 0x18abdfc2, 0x1969b5f5,
		0x1235f2c8, 0x13f798ff, 0x11b126a6, 0x10734c91, 0x153c5a14, 0x14fe3023, 0x16b88e7a, 0x177ae44d,
		0x384d46e0, 0x398f2cd7, 0x3bc9928e, 0x3a0bf8b9, 0x3f44ee3c, 0x3e86840b, 0x3cc03a52, 0x3d025065,
		0x365e1758, 0x379c7d6f, 0x35dac336, 0x3418a901, 0x3157bf84, 0x3095d5b3, 0x32d36bea, 0x331101dd,
		0x246be590, 0x25a98fa7, 0x27ef31fe, 0x262d5bc9, 0x23624d4c, 0x22a0277b, 0x20e69922, 0x2124f315,
		0x2a78b428, 0x2bbade1f, 0x29fc6046, 0x283e0a71, 0x2d711cf4, 0x2cb376c3, 0x2ef5c89a, 0x2f37a2ad,
		0x709a8dc0, 0x7158e7f7, 0x731e59ae, 0x72dc3399, 0x7793251c, 0x76514f2b, 0x7417f172, 0x75d59b45,
		0x7e89dc78, 0x7f4bb64f, 0x7d0d0816, 0x7ccf6221, 0x798074a4, 0x78421e93, 0x7a04a0ca, 0x7bc6cafd,
		0x6cbc2eb0, 0x6d7e4487, 0x6f38fade, 0x6efa90e9, 0x6bb5866c, 0x6a77ec5b, 0x68315202, 0x69f33835,
		0x62af7f08, 0x636d153f, 0x612bab66, 0x60e9c151, 0x65a6d7d4, 0x6464bde3, 0x662203ba, 0x67e0698d,
		0x48d7cb20, 0x4915a117, 0x4b531f4e, 0x4a917579, 0x4fde63fc, 0x4e1c09cb, 0x4c5ab792, 0x4d98dda5,
		0x46c49a98, 0x4706f0af, 0x45404ef6, 0x448224c1, 0x41cd3244, 0x400f5873, 0x4249e62a, 0x438b8c1d,
		0x54f16850, 0x55330267, 0x5775bc3e, 0x56b7d609, 0x53f8c08c, 0x523aaabb, 0x507c14e2, 0x51be7ed5,
		0x5ae239e8, 0x5b2053df, 0x5966ed86, 0x58a487b1, 0x5deb9134, 0x5c29fb03, 0x5e6f455a, 0x5fad2f6d,
		0xe1351b80, 0xe0f771b7, 0xe2b1cfee, 0xe373a5d9, 0xe63cb35c, 0xe7fed96b, 0xe5b86732, 0xe47a0d05,
		0xef264a38, 0xeee4200f, 0xeca29e56, 0xed60f461, 0xe82fe2e4, 0xe9ed88d3, 0xebab368a, 0xea695cbd,
		0xfd13b8f0, 0xfcd1d2c7, 0xfe976c9e, 0xff5506a9, 0xfa1a102c, 0xfbd87a1b, 0xf99ec442, 0xf85cae75,
		0xf300e948, 0xf2c2837f, 0xf0843d26, 0xf1465711, 0xf4094194, 0xf5cb2ba3, 0xf78d95fa, 0xf64fffcd,
		0xd9785d60, 0xd8ba3757, 0xdafc890e, 0xdb3ee339, 0xde71f5bc, 0xdfb39f8b, 0xddf521d2, 0xdc374be5,
		0xd76b0cd8, 0xd6a966ef, 0xd4efd8b6, 0xd52db281, 0xd062a404, 0xd1a0ce33, 0xd3e6706a, 0xd2241a5d,
		0xc55efe10, 0xc49c9427, 0xc6da2a7e, 0xc7184049, 0xc25756cc, 0xc3953cfb, 0xc1d382a2, 0xc011e895,
		0xcb4dafa8, 0xca8fc59f, 0xc8c97bc6, 0xc90b11f1, 0xcc440774, 0xcd866d43, 0xcfc0d31a, 0xce02b92d,
		0x91af9640, 0x906dfc77, 0x922b422e, 0x93e92819, 0x96a63e9c, 0x976454ab, 0x9522eaf2, 0x94e080c5,
		0x9fbcc7f8, 0x9e7eadcf, 0x9c381396, 0x9dfa79a1, 0x98b56f24, 0x99770513, 0x9b31bb4a, 0x9af3d17d,
		0x8d893530, 0x8c4b5f07, 0x8e0de15e, 0x8fcf8b69, 0x8a809dec, 0x8b42f7db, 0x89044982, 0x88c623b5,
		0x839a6488, 0x82580ebf, 0x801eb0e6, 0x81dcdad1, 0x8493cc54, 0x8551a663, 0x8717183a, 0x86d5720d,
		0xa9e2d0a0, 0xa820ba97, 0xaa6604ce, 0xaba46ef9, 0xaeeb787c, 0xaf29124b, 0xad6fac12, 0xacadc625,
		0xa7f18118, 0xa633eb2f, 0xa4755576, 0xa5b73f41, 0xa0f829c4, 0xa13a43f3, 0xa37cfdaa, 0xa2be979d,
		0xb5c473d0, 0xb40619e7, 0xb640a7be, 0xb782cd89, 0xb2cddb0c, 0xb30fb13b, 0xb1490f62, 0xb08b6555,
		0xbbd72268, 0xba15485f, 0xb853f606, 0xb9919c31, 0xbcde8ab4, 0xbd1ce083, 0xbf5a5eda, 0xbe9834ed
	},
	{
		0x00000000, 0xb8bc6765, 0xaa09c88b, 0x12b5afee, 0x8f629757, 0x37def032, 0x256b5fdc, 0x9dd738b9,
		0xc5b428ef, 0x7d084f8a, 0x6fbde064, 0xd7018701, 0x4ad6bfb8, 0xf26ad8dd, 0xe0df7733, 0x58631056,
		0x5019579f, 0xe8a530fa, 0xfa109f14, 0x42acf871, 0xdf7bc0c8, 0x67c7a7ad, 0x75720843, 0xcdce6f26,
		0x95ad7f70, 0x2d111815, 0x3fa4b7fb, 0x8718d09e, 0x1acfe827, 0xa2738f42, 0xb0c620ac, 0x087a47c9,
		0xa032af3e, 0x188ec85b, 0x0a3b67b5, 0xb28700d0, 0x2f503869, 0x97ec5f0c, 0x8559f0e2, 0x3de59787,
		0x658687d1, 0xdd3ae0b4, 0xcf8f4f5a, 0x7733283f, 0xeae41086, 0x525877e3, 0x40edd80d, 0xf851bf68,
		0xf02bf8a1, 0x48979fc4, 0x5a22302a, 0xe29e574f, 0x7f496ff6, 0xc7f50893, 0xd540a77d, 0x6dfcc018,
		0x359fd04e, 0x8d23b72b, 0x9f9618c5, 0x272a7fa0, 0xbafd4719, 0x0241207c, 0x10f48f92, 0xa848e8f7,
		0x9b14583d, 0x23a83f58, 0x311d90b6, 0x89a1f7d3, 0x1476cf6a, 0xaccaa80f, 0xbe7f07e1, 0x06c36084,
		0x5ea070d2, 0xe61c17b7, 0xf4a9b859, 0x4c15df3c, 0xd1c2e785, 0x697e80e0, 0x7bcb2f0e, 0xc377486b,
		0xcb0d0fa2, 0x73b168c7, 0x6104c729, 0xd9b8a04c, 0x446f98f5, 0xfcd3ff90, 0xee66507e, 0x56da371b,
		0x0eb9274d, 0xb6054028, 0xa4b0efc6, 0x1c0c88a3, 0x81dbb01a, 0x3967d77f, 0x2bd27891, 0x936e1ff4,
		0x3b26f703, 0x839a9066, 0x912f3f88, 0x299358ed, 0xb4446054, 0x0cf80731, 0x1e4da8df, 0xa6f1cfba,
		0xfe92dfec, 0x462eb889, 0x549b1767, 0xec277002, 0x71f048bb, 0xc94c2fde, 0xdbf98030, 0x6345e755,
		0x6b3fa09c, 0xd383c7f9, 0xc1366817, 0x798a0f72, 0xe45d37cb, 0x5ce150ae, 0x4e54ff40, 0xf6e89825,
		0xae8b8873, 0x1637ef16, 0x048240f8, 0xbc3e279d, 0x21e91f24, 0x99557841, 0x8be0d7af, 0x335cb0ca,
		0xed59b63b, 0x55e5d15e, 0x47507eb0, 0xffec19d5, 0x623b216c, 0xda874609, 0xc832e9e7, 0x708e8e82,
		0x28ed9ed4, 0x9051f9b1, 0x82e4565f, 0x3a58313a, 0xa78f0983, 0x1f336ee6, 0x0d86c108, 0xb53aa66d,
		0xbd40e1a4, 0x05fc86c1, 0x1749292f, 0xaff54e4a, 0x322276f3, 0x8a9e1196, 0x982bbe78, 0x2097d91d,
		0x78f4c94b, 0xc048ae2e, 0xd2fd01c0, 0x6a4166a5, 0xf7965e1c, 0x4f2a3979, 0x5d9f9697, 0xe523f1f2,
		0x4d6b1905, 0xf5d77e60, 0xe762d18e, 0x5fdeb6eb, 0xc2098e52, 0x7ab5e937, 0x680046d9, 0xd0bc21bc,
		0x88df31ea, 0x3063568f, 0x22d6f961, 0x9a6a9e04, 0x07bda6bd, 0xbf01c1d8, 0xadb46e36, 0x15080953,
		0x1d724e9a, 0xa5ce29ff, 0xb77b8611, 0x0fc7e174, 0x9210d9cd, 0x2aacbea8, 0x38191146, 0x80a57623,
		0xd8c66675, 0x607a0110, 0x72cfaefe, 0xca73c99b, 0x57a4f122, 0xef189647, 0xfdad39a9, 0x45115ecc,
		0x764dee06, 0xcef18963, 0xdc44268d, 0x64f841e8, 0xf92f7951, 0x41931e34, 0x5326b1da, 0xeb9ad6bf,
		0xb3f9c6e9, 0x0b45a18c, 0x19f00e62, 0xa14c6907, 0x3c9b51be, 0x842736db, 0x96929935, 0x2e2efe50,
		0x2654b999, 0x9ee8defc, 0x8c5d7112, 0x34e11677, 0xa9362ece, 0x118a49ab, 0x033fe645, 0xbb838120,
		0xe3e09176, 0x5b5cf613, 0x49e959fd, 0xf1553e98, 0x6c820621, 0xd43e6144, 0xc68bceaa, 0x7e37a9cf,
		0xd67f4138, 0x6ec3265d, 0x7c7689b3, 0xc4caeed6, 0x591dd66f, 0xe1a1b10a, 0xf3141ee4, 0x4ba87981,
		0x13cb69d7, 0xab770eb2, 0xb9c2a15c, 0x017ec639, 0x9ca9fe80, 0x241599e5, 0x36a0360b, 0x8e1c516e,
		0x866616a7, 0x3eda71c2, 0x2c6fde2c, 0x94d3b949, 0x090481f0, 0xb1b8e695, 0xa30d497b, 0x1bb12e1e,
		0x43d23e48, 0xfb6e592d, 0xe9dbf6c3, 0x516791a6, 0xccb0a91f, 0x740cce7a, 0x66b96194, 0xde0506f1
	},
	{
		0x00000000, 0x3d6029b0, 0x7ac05360, 0x47a07ad0, 0xf580a6c0, 0xc8e08f70, 0x8f40f5a0, 0xb220dc10,
		0x30704bc1, 0x0d106271, 0x4ab018a1, 0x77d03111, 0xc5f0ed01, 0xf890c4b1, 0xbf30be61, 0x825097d1,
		0x60e09782, 0x5d80be32, 0x1a20c4e2, 0x2740ed52, 0x95603142, 0xa80018f2, 0xefa06222, 0xd2c04b92,
		0x5090dc43, 0x6df0f5f3, 0x2a508f23, 0x1730a693, 0xa5107a83, 0x98705333, 0xdfd029e3, 0xe2b00053,
		0xc1c12f04, 0xfca106b4, 0xbb017c64, 0x866155d4, 0x344189c4, 0x0921a074, 0x4e81daa4, 0x73e1f314,
		0xf1b164c5, 0xccd14d75, 0x8b7137a5, 0xb6111e15, 0x0431c205, 0x3951ebb5, 0x7ef19165, 0x4391b8d5,
		0xa121b886, 0x9c419136, 0xdbe1ebe6, 0xe681c256, 0x54a11e46, 0x69c137f6, 0x2e614d26, 0x13016496,
		0x9151f347, 0xac31daf7, 0xeb91a027, 0xd6f18997, 0x64d15587, 0x59b17c37, 0x1e1106e7, 0x23712f57,
		0x58f35849, 0x659371f9, 0x22330b29, 0x1f532299, 0xad73fe89, 0x9013d739, 0xd7b3ade9, 0xead38459,
		0x68831388, 0x55e33a38, 0x124340e8, 0x2f236958, 0x9d03b548, 0xa0639cf8, 0xe7c3e628, 0xdaa3cf98,
		0x3813cfcb, 0x0573e67b, 0x42d39cab, 0x7fb3b51b, 0xcd93690b, 0xf0f340bb, 0xb7533a6b, 0x8a3313db,
		0x0863840a, 0x3503adba, 0x72a3d76a, 0x4fc3feda, 0xfde322ca, 0xc0830b7a, 0x872371aa, 0xba43581a,
		0x9932774d, 0xa4525efd, 0xe3f2242d, 0xde920d9d, 0x6cb2d18d, 0x51d2f83d, 0x167282ed, 0x2b12ab5d,
		0xa9423c8c, 0x9422153c, 0xd3826fec, 0xeee2465c, 0x5cc29a4c, 0x61a2b3fc, 0x2602c92c, 0x1b62e09c,
		0xf9d2e0cf, 0xc4b2c97f, 0x8312b3af, 0xbe729a1f, 0x0c52460f, 0x31326fbf, 0x7692156f, 0x4bf23cdf,
		0xc9a2ab0e, 0xf4c282be, 0xb362f86e, 0x8e02d1de, 0x3c220dce, 0x0142247e, 0x46e25eae, 0x7b82771e,
		0xb1e6b092, 0x8c869922, 0xcb26e3f2, 0xf646ca42, 0x44661652, 0x79063fe2, 0x3ea64532, 0x03c66c82,
		0x8196fb53, 0xbcf6d2e3, 0xfb56a833, 0xc6368183, 0x74165d93, 0x49767423, 0x0ed60ef3, 0x33b62743,
		0xd1062710, 0xec660ea0, 0xabc67470, 0x96a65dc0, 0x248681d0, 0x19e6a860, 0x5e46d2b0, 0x6326fb00,
		0xe1766cd1, 0xdc164561, 0x9bb63fb1, 0xa6d61601, 0x14f6ca11, 0x2996e3a1, 0x6e369971, 0x5356b0c1,
		0x70279f96, 0x4d47b626, 0x0ae7ccf6, 0x3787e546, 0x85a73956, 0xb8c710e6, 0xff676a36, 0xc2074386,
		0x4057d457, 0x7d37fde7, 0x3a978737, 0x07f7ae87, 0xb5d77297, 0x88b75b27, 0xcf1721f7, 0xf2770847,
		0x10c70814, 0x2da721a4, 0x6a075b74, 0x576772c4, 0xe547aed4, 0xd8278764, 0x9f87fdb4, 0xa2e7d404,
		0x20b743d5, 0x1dd76a65, 0x5a7710b5, 0x67173905, 0xd537e515, 0xe857cca5, 0xaff7b675, 0x92979fc5,
		0xe915e8db, 0xd475c16b, 0x93d5bbbb, 0xaeb5920b, 0x1c954e1b, 0x21f567ab, 0x66551d7b, 0x5b3534cb,
		0xd965a31a, 0xe4058aaa, 0xa3a5f07a, 0x9ec5d9ca, 0x2ce505da, 0x11852c6a, 0x562556ba, 0x6b457f0a,
		0x89f57f59, 0xb49556e9, 0xf3352c39, 0xce550589, 0x7c75d999, 0x4115f029, 0x06b58af9, 0x3bd5a349,
		0xb9853498, 0x84e51d28, 0xc34567f8, 0xfe254e48, 0x4c059258, 0x7165bbe8, 0x36c5c138, 0x0ba5e888,
		0x28d4c7df, 0x15b4ee6f, 0x521494bf, 0x6f74bd0f, 0xdd54611f, 0xe03448af, 0xa794327f, 0x9af41bcf,
		0x18a48c1e, 0x25c4a5ae, 0x6264df7e, 0x5f04f6ce, 0xed242ade, 0xd044036e, 0x97e479be, 0xaa84500e,
		0x4834505d, 0x755479ed, 0x32f4033d, 0x0f942a8d, 0xbdb4f69d, 0x80d4df2d, 0xc774a5fd, 0xfa148c4d,
		0x78441b9c, 0x4524322c, 0x028448fc, 0x3fe4614c, 0x8dc4bd5c, 0xb0a494ec, 0xf704ee3c, 0xca64c78c
	},
	{
		0x00000000, 0xcb5cd3a5, 0x4dc8a10b, 0x869472ae, 0x9b914216, 0x50cd91b3, 0xd659e31d, 0x1d0530b8,
		0xec53826d, 0x270f51c8, 0xa19b2366, 0x6ac7f0c3, 0x77c2c07b, 0xbc9e13de, 0x3a0a6170, 0xf156b2d5,
		0x03d6029b, 0xc88ad13e, 0x4e1ea390, 0x85427035, 0x9847408d, 0x531b9328, 0xd58fe186, 0x1ed33223,
		0xef8580f6, 0x24d95353, 0xa24d21fd, 0x6911f258, 0x7414c2e0, 0xbf481145, 0x39dc63eb, 0xf280b04e,
		0x07ac0536, 0xccf0d693, 0x4a64a43d, 0x81387798, 0x9c3d4720, 0x57619485, 0xd1f5e62b, 0x1aa9358e,
		0xebff875b, 0x20a354fe, 0xa6372650, 0x6d6bf5f5, 0x706ec54d, 0xbb3216e8, 0x3da66446, 0xf6fab7e3,
		0x047a07ad, 0xcf26d408, 0x49b2a6a6, 0x82ee7503, 0x9feb45bb, 0x54b7961e, 0xd223e4b0, 0x197f3715,
		0xe82985c0, 0x23755665, 0xa5e124cb, 0x6ebdf76e, 0x73b8c7d6, 0xb8e41473, 0x3e7066dd, 0xf52cb578,
		0x0f580a6c, 0xc404d9c9, 0x4290ab67, 0x89cc78c2, 0x94c9487a, 0x5f959bdf, 0xd901e971, 0x125d3ad4,
		0xe30b8801, 0x28575ba4, 0xaec3290a, 0x659ffaaf, 0x789aca17, 0xb3c619b2, 0x35526b1c, 0xfe0eb8b9,
		0x0c8e08f7, 0xc7d2db52, 0x4146a9fc, 0x8a1a7a59, 0x971f4ae1, 0x5c439944, 0xdad7ebea, 0x118b384f,
		0xe0dd8a9a, 0x2b81593f, 0xad152b91, 0x6649f834, 0x7b4cc88c, 0xb0101b29, 0x36846987, 0xfdd8ba22,
		0x08f40f5a, 0xc3a8dcff, 0x453cae51, 0x8e607df4, 0x93654d4c, 0x58399ee9, 0xdeadec47, 0x15f13fe2,
		0xe4a78d37, 0x2ffb5e92, 0xa96f2c3c, 0x6233ff99, 0x7f36cf21, 0xb46a1c84, 0x32fe6e2a, 0xf9a2bd8f,
		0x0b220dc1, 0xc07ede64, 0x46eaacca, 0x8db67f6f, 0x90b34fd7, 0x5bef9c72, 0xdd7beedc, 0x16273d79,
		0xe7718fac, 0x2c2d5c09, 0xaab92ea7, 0x61e5fd02, 0x7ce0cdba, 0xb7bc1e1f, 0x31286cb1, 0xfa74bf14,
		0x1eb014d8, 0xd5ecc77d, 0x5378b5d3, 0x98246676, 0x852156ce, 0x4e7d856b, 0xc8e9f7c5, 0x03b52460,
		0xf2e396b5, 0x39bf4510, 0xbf2b37be, 0x7477e41b, 0x6972d4a3, 0xa22e0706, 0x24ba75a8, 0xefe6a60d,
		0x1d661643, 0xd63ac5e6, 0x50aeb748, 0x9bf264ed, 0x86f75455, 0x4dab87f0, 0xcb3ff55e, 0x006326fb,
		0xf135942e, 0x3a69478b, 0xbcfd3525, 0x77a1e680, 0x6aa4d638, 0xa1f8059d, 0x276c7733, 0xec30a496,
		0x191c11ee, 0xd240c24b, 0x54d4b0e5, 0x9f886340, 0x828d53f8, 0x49d1805d, 0xcf45f2f3, 0x04192156,
		0xf54f9383, 0x3e134026, 0xb8873288, 0x73dbe12d, 0x6eded195, 0xa5820230, 0x2316709e, 0xe84aa33b,
		0x1aca1375, 0xd196c0d0, 0x5702b27e, 0x9c5e61db, 0x815b5163, 0x4a0782c6, 0xcc93f068, 0x07cf23cd,
		0xf6999118, 0x3dc542bd, 0xbb513013, 0x700de3b6, 0x6d08d30e, 0xa65400ab, 0x20c07205, 0xeb9ca1a0,
		0x11e81eb4, 0xdab4cd11, 0x5c20bfbf, 0x977c6c1a, 0x8a795ca2, 0x41258f07, 0xc7b1fda9, 0x0ced2e0c,
		0xfdbb9cd9, 0x36e74f7c, 0xb0733dd2, 0x7b2fee77, 0x662adecf, 0xad760d6a, 0x2be27fc4, 0xe0beac61,
		0x123e1c2f, 0xd962cf8a, 0x5ff6bd24, 0x94aa6e81, 0x89af5e39, 0x42f38d9c, 0xc467ff32, 0x0f3b2c97,
		0xfe6d9e42, 0x35314de7, 0xb3a53f49, 0x78f9ecec, 0x65fcdc54, 0xaea00ff1, 0x28347d5f, 0xe368aefa,
		0x16441b82, 0xdd18c827, 0x5b8cba89, 0x90d0692c, 0x8dd55994, 0x46898a31, 0xc01df89f, 0x0b412b3a,
		0xfa1799ef, 0x314b4a4a, 0xb7df38e4, 0x7c83eb41, 0x6186dbf9, 0xaada085c, 0x2c4e7af2, 0xe712a957,
		0x15921919, 0xdececabc, 0x585ab812, 0x93066bb7, 0x8e035b0f, 0x455f88aa, 0xc3cbfa04, 0x089729a1,
		0xf9c19b74, 0x329d48d1, 0xb4093a7f, 0x7f55e9da, 0x6250d962, 0xa90c0ac7, 0x2f987869, 0xe4c4abcc
	},
	{
		0x00000000, 0xa6770bb4, 0x979f1129, 0x31e81a9d, 0xf44f2413, 0x52382fa7, 0x63d0353a, 0xc5a73e8e,
		0x33ef4e67, 0x959845d3, 0xa4705f4e, 0x020754fa, 0xc7a06a74, 0x61d761c0, 0x503f7b5d, 0xf64870e9,
		0x67de9cce, 0xc1a9977a, 0xf0418de7, 0x56368653, 0x9391b8dd, 0x35e6b369, 0x040ea9f4, 0xa279a240,
		0x5431d2a9, 0xf246d91d, 0xc3aec380, 0x65d9c834, 0xa07ef6ba, 0x0609fd0e, 0x37e1e793, 0x9196ec27,
		0xcfbd399c, 0x69ca3228, 0x582228b5, 0xfe552301, 0x3bf21d8f, 0x9d85163b, 0xac6d0ca6, 0x0a1a0712,
		0xfc5277fb, 0x5a257c4f, 0x6bcd66d2, 0xcdba6d66, 0x081d53e8, 0xae6a585c, 0x9f8242c1, 0x39f54975,
		0xa863a552, 0x0e14aee6, 0x3ffcb47b, 0x998bbfcf, 0x5c2c8141, 0xfa5b8af5, 0xcbb39068, 0x6dc49bdc,
		0x9b8ceb35, 0x3dfbe081, 0x0c13fa1c, 0xaa64f1a8, 0x6fc3cf26, 0xc9b4c492, 0xf85cde0f, 0x5e2bd5bb,
		0x440b7579, 0xe27c7ecd, 0xd3946450, 0x75e36fe4, 0xb044516a, 0x16335ade, 0x27db4043, 0x81ac4bf7,
		0x77e43b1e, 0xd19330aa, 0xe07b2a37, 0x460c2183, 0x83ab1f0d, 0x25dc14b9, 0x14340e24, 0xb2430590,
		0x23d5e9b7, 0x85a2e203, 0xb44af89e, 0x123df32a, 0xd79acda4, 0x71edc610, 0x4005dc8d, 0xe672d739,
		0x103aa7d0, 0xb64dac64, 0x87a5b6f9, 0x21d2bd4d, 0xe47583c3, 0x42028877, 0x73ea92ea, 0xd59d995e,
		0x8bb64ce5, 0x2dc14751, 0x1c295dcc, 0xba5e5678, 0x7ff968f6, 0xd98e6342, 0xe86679df, 0x4e11726b,
		0xb8590282, 0x1e2e0936, 0x2fc613ab, 0x89b1181f, 0x4c162691, 0xea612d25, 0xdb8937b8, 0x7dfe3c0c,
		0xec68d02b, 0x4a1fdb9f, 0x7bf7c102, 0xdd80cab6, 0x1827f438, 0xbe50ff8c, 0x8fb8e511, 0x29cfeea5,
		0xdf879e4c, 0x79f095f8, 0x48188f65, 0xee6f84d1, 0x2bc8ba5f, 0x8dbfb1eb, 0xbc57ab76, 0x1a20a0c2,
		0x8816eaf2, 0x2e61e146, 0x1f89fbdb, 0xb9fef06f, 0x7c59cee1, 0xda2ec555, 0xebc6dfc8, 0x4db1d47c,
		0xbbf9a495, 0x1d8eaf21, 0x2c66b5bc, 0x8a11be08, 0x4fb68086, 0xe9c18b32, 0xd82991af, 0x7e5e9a1b,
		0xefc8763c, 0x49bf7d88, 0x78576715, 0xde206ca1, 0x1b87522f, 0xbdf0599b, 0x8c184306, 0x2a6f48b2,
		0xdc27385b, 0x7a5033ef, 0x4bb82972, 0xedcf22c6, 0x28681c48, 0x8e1f17fc, 0xbff70d61, 0x198006d5,
		0x47abd36e, 0xe1dcd8da, 0xd034c247, 0x7643c9f3, 0xb3e4f77d, 0x1593fcc9, 0x247be654, 0x820cede0,
		0x74449d09, 0xd23396bd, 0xe3db8c20, 0x45ac8794, 0x800bb91a, 0x267cb2ae, 0x1794a833, 0xb1e3a387,
		0x20754fa0, 0x86024414, 0xb7ea5e89, 0x119d553d, 0xd43a6bb3, 0x724d6007, 0x43a57a9a, 0xe5d2712e,
		0x139a01c7, 0xb5ed0a73, 0x840510ee, 0x22721b5a, 0xe7d525d4, 0x41a22e60, 0x704a34fd, 0xd63d3f49,
		0xcc1d9f8b, 0x6a6a943f, 0x5b828ea2, 0xfdf58516, 0x3852bb98, 0x9e25b02c, 0xafcdaab1, 0x09baa105,
		0xfff2d1ec, 0x5985da58, 0x686dc0c5, 0xce1acb71, 0x0bbdf5ff, 0xadcafe4b, 0x9c22e4d6, 0x3a55ef62,
		0xabc30345, 0x0db408f1, 0x3c5c126c, 0x9a2b19d8, 0x5f8c2756, 0xf9fb2ce2, 0xc813367f, 0x6e643dcb,
		0x982c4d22, 0x3e5b4696, 0x0fb35c0b, 0xa9c457bf, 0x6c636931, 0xca146285, 0xfbfc7818, 0x5d8b73ac,
		0x03a0a617, 0xa5d7ada3, 0x943fb73e, 0x3248bc8a, 0xf7ef8204, 0x519889b0, 0x6070932d, 0xc6079899,
		0x304fe870, 0x9638e3c4, 0xa7d0f959, 0x01a7f2ed, 0xc400cc63, 0x6277c7d7, 0x539fdd4a, 0xf5e8d6fe,
		0x647e3ad9, 0xc209316d, 0xf3e12bf0, 0x55962044, 0x90311eca, 0x3646157e, 0x07ae0fe3, 0xa1d90457,
		0x579174be, 0xf1e67f0a, 0xc00e6597, 0x66796e23, 0xa3de50ad, 0x05a95b19, 0x34414184, 0x92364a30
	},
	{
		0x00000000, 0xccaa009e, 0x4225077d, 0x8e8f07e3, 0x844a0efa, 0x48e00e64, 0xc66f0987, 0x0ac50919,
		0xd3e51bb5, 0x1f4f1b2b, 0x91c01cc8, 0x5d6a1c56, 0x57af154f, 0x9b0515d1, 0x158a1232, 0xd92012ac,
		0x7cbb312b, 0xb01131b5, 0x3e9e3656, 0xf23436c8, 0xf8f13fd1, 0x345b3f4f, 0xbad438ac, 0x767e3832,
		0xaf5e2a9e, 0x63f42a00, 0xed7b2de3, 0x21d12d7d, 0x2b142464, 0xe7be24fa, 0x69312319, 0xa59b2387,
		0xf9766256, 0x35dc62c8, 0xbb53652b, 0x77f965b5, 0x7d3c6cac, 0xb1966c32, 0x3f196bd1, 0xf3b36b4f,
		0x2a9379e3, 0xe639797d, 0x68b67e9e, 0xa41c7e00, 0xaed97719, 0x62737787, 0xecfc7064, 0x205670fa,
		0x85cd537d, 0x496753e3, 0xc7e85400, 0x0b42549e, 0x01875d87, 0xcd2d5d19, 0x43a25afa, 0x8f085a64,
		0x562848c8, 0x9a824856, 0x140d4fb5, 0xd8a74f2b, 0xd2624632, 0x1ec846ac, 0x9047414f, 0x5ced41d1,
		0x299dc2ed, 0xe537c273, 0x6bb8c590, 0xa712c50e, 0xadd7cc17, 0x617dcc89, 0xeff2cb6a, 0x2358cbf4,
		0xfa78d958, 0x36d2d9c6, 0xb85dde25, 0x74f7debb, 0x7e32d7a2, 0xb298d73c, 0x3c17d0df, 0xf0bdd041,
		0x5526f3c6, 0x998cf358, 0x1703f4bb, 0xdba9f425, 0xd16cfd3c, 0x1dc6fda2, 0x9349fa41, 0x5fe3fadf,
		0x86c3e873, 0x4a69e8ed, 0xc4e6ef0e, 0x084cef90, 0x0289e689, 0xce23e617, 0x40ace1f4, 0x8c06e16a,
		0xd0eba0bb, 0x1c41a025, 0x92cea7c6, 0x5e64a758, 0x54a1ae41, 0x980baedf, 0x1684a93c, 0xda2ea9a2,
		0x030ebb0e, 0xcfa4bb90, 0x412bbc73, 0x8d81bced, 0x8744b5f4, 0x4beeb56a, 0xc561b289, 0x09cbb217,
		0xac509190, 0x60fa910e, 0xee7596ed, 0x22df9673, 0x281a9f6a, 0xe4b09ff4, 0x6a3f9817, 0xa6959889,
		0x7fb58a25, 0xb31f8abb, 0x3d908d58, 0xf13a8dc6, 0xfbff84df, 0x37558441, 0xb9da83a2, 0x7570833c,
		0x533b85da, 0x9f918544, 0x111e82a7, 0xddb48239, 0xd7718b20, 0x1bdb8bbe, 0x95548c5d, 0x59fe8cc3,
		0x80de9e6f, 0x4c749ef1, 0xc2fb9912, 0x0e51998c, 0x04949095, 0xc83e900b, 0x46b197e8, 0x8a1b9776,
		0x2f80b4f1, 0xe32ab46f, 0x6da5b38c, 0xa10fb312, 0xabcaba0b, 0x6760ba95, 0xe9efbd76, 0x2545bde8,
		0xfc65af44, 0x30cfafda, 0xbe40a839, 0x72eaa8a7, 0x782fa1be, 0xb485a120, 0x3a0aa6c3, 0xf6a0a65d,
		0xaa4de78c, 0x66e7e712, 0xe868e0f1, 0x24c2e06f, 0x2e07e976, 0xe2ade9e8, 0x6c22ee0b, 0xa088ee95,
		0x79a8fc39, 0xb502fca7, 0x3b8dfb44, 0xf727fbda, 0xfde2f2c3, 0x3148f25d, 0xbfc7f5be, 0x736df520,
		0xd6f6d6a7, 0x1a5cd639, 0x94d3d1da, 0x5879d144, 0x52bcd85d, 0x9e16d8c3, 0x1099df20, 0xdc33dfbe,
		0x0513cd12, 0xc9b9cd8c, 0x4736ca6f, 0x8b9ccaf1, 0x8159c3e8, 0x4df3c376, 0xc37cc495, 0x0fd6c40b,
		0x7aa64737, 0xb60c47a9, 0x3883404a, 0xf42940d4, 0xfeec49cd, 0x32464953, 0xbcc94eb0, 0x70634e2e,
		0xa9435c82, 0x65e95c1c, 0xeb665bff, 0x27cc5b61, 0x2d095278, 0xe1a352e6, 0x6f2c5505, 0xa386559b,
		0x061d761c, 0xcab77682, 0x44387161, 0x889271ff, 0x825778e6, 0x4efd7878, 0xc0727f9b, 0x0cd87f05,
		0xd5f86da9, 0x19526d37, 0x97dd6ad4, 0x5b776a4a, 0x51b26353, 0x9d1863cd, 0x1397642e, 0xdf3d64b0,
		0x83d02561, 0x4f7a25ff, 0xc1f5221c, 0x0d5f2282, 0x079a2b9b, 0xcb302b05, 0x45bf2ce6, 0x89152c78,
		0x50353ed4, 0x9c9f3e4a, 0x121039a9, 0xdeba3937, 0xd47f302e, 0x18d530b0, 0x965a3753, 0x5af037cd,
		0xff6b144a, 0x33c114d4, 0xbd4e1337, 0x71e413a9, 0x7b211ab0, 0xb78b1a2e, 0x39041dcd, 0xf5ae1d53,
		0x2c8e0fff, 0xe0240f61, 0x6eab0882, 0xa201081c, 0xa8c40105, 0x646e019b, 0xeae10678, 0x264b06e6
	}
};

namespace wz {

uint32_t crc_init()
{
	return 0xffffffff;
}

#if defined(__clang__)
#  pragma clang diagnostic push
#  pragma clang diagnostic ignored "-Wcast-align"
#elif defined(__GNUC__)
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wcast-align"
#endif

crc_t crc_update(crc_t crc, const void *data, size_t data_len)
{
	const unsigned char *d = (const unsigned char *)data;
	unsigned int tbl_idx;

	/* Align to a multiple of 8 bytes */
	while (data_len && (((uintptr_t)(const void *)d) % 8 != 0)) {
		tbl_idx = (crc ^ *d) & 0xff;
		crc = (crc_table[0][tbl_idx] ^ (crc >> 8)) & 0xffffffff;
		d++;
		data_len--;
	}

	const uint32_t *d32 = (const uint32_t *)d;
	while (data_len >= 8)
	{
#if WZ_IS_BIG_ENDIAN == 1
		crc_t d1 = *d32++ ^ le16toh(crc);
		crc_t d2 = *d32++;
		crc  =
			crc_table[0][d2 & 0xffu] ^
			crc_table[1][(d2 >> 8) & 0xffu] ^
			crc_table[2][(d2 >> 16) & 0xffu] ^
			crc_table[3][(d2 >> 24) & 0xffu] ^
			crc_table[4][d1 & 0xffu] ^
			crc_table[5][(d1 >> 8) & 0xffu] ^
			crc_table[6][(d1 >> 16) & 0xffu] ^
			crc_table[7][(d1 >> 24) & 0xffu];
#else
		crc_t d1 = *d32++ ^ crc;
		crc_t d2 = *d32++;
		crc  =
			crc_table[0][(d2 >> 24) & 0xffu] ^
			crc_table[1][(d2 >> 16) & 0xffu] ^
			crc_table[2][(d2 >> 8) & 0xffu] ^
			crc_table[3][d2 & 0xffu] ^
			crc_table[4][(d1 >> 24) & 0xffu] ^
			crc_table[5][(d1 >> 16) & 0xffu] ^
			crc_table[6][(d1 >> 8) & 0xffu] ^
			crc_table[7][d1 & 0xffu];
#endif

		data_len -= 8;
	}

	/* Remaining bytes with the standard algorithm */
	d = (const unsigned char *)d32;
	while (data_len--) {
		tbl_idx = (crc ^ *d) & 0xff;
		crc = (crc_table[0][tbl_idx] ^ (crc >> 8)) & 0xffffffff;
		d++;
	}
	return crc & 0xffffffff;
}

#if defined(__clang__)
#  pragma clang diagnostic pop
#elif defined(__GNUC__)
#  pragma GCC diagnostic pop
#endif

}
