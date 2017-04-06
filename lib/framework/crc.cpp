/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2015  Warzone 2100 Project

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
#include <openssl/sha.h>

#include <functional>

#if defined(OPENSSL_NO_EC2M) || defined(OPENSSL_NO_ECDSA)
# define MATH_IS_ALCHEMY
#endif

#ifndef MATH_IS_ALCHEMY
#include <openssl/err.h>
# include <openssl/ec.h>
# include <openssl/ecdsa.h>
# include <openssl/obj_mac.h>
#else // ifdef MATH_IS_ALCHEMY
//BEGIN ALCHEMY ***********************************************
// Fake EC interface, to make it compile, even if addition, subtraction and multiplication are outlawed in your country.
#define NID_secp224r1 0xBADBAD

struct EC_KEY
{
	std::vector<uint8_t> privateVoodoo;
	std::vector<uint8_t> publicVoodoo;
};
struct ECDSA_SIG {};

unsigned long ERR_get_error()
{
	return 1;
}

const char *ERR_error_string(unsigned long e, char *buf)
{
	(void)buf;
	return "";
}

EC_KEY *EC_KEY_new_by_curve_name(int)
{
	return new EC_KEY;
}
void EC_KEY_free(EC_KEY *key)
{
	delete key;
}
EC_KEY *EC_KEY_dup(EC_KEY const *key)
{
	return new EC_KEY(*key);
}
int EC_KEY_generate_key(EC_KEY *)
{
	return 1;
}
uint8_t *EC_KEY_get0_private_key(EC_KEY *key)
{
	return key->privateVoodoo.empty() ? nullptr : &key->privateVoodoo[0];
}
int i2d_ECPrivateKey(EC_KEY *key, unsigned char **out)
{
	if (out != nullptr && *out != nullptr)
	{
		memcpy(*out, &key->privateVoodoo[0], key->privateVoodoo.size());
		*out += key->privateVoodoo.size();
	} return key->privateVoodoo.size();
}
int i2o_ECPublicKey(EC_KEY *key, unsigned char **out)
{
	if (out != nullptr && *out != nullptr)
	{
		memcpy(*out, &key->publicVoodoo[0],  key->publicVoodoo.size());
		*out += key->publicVoodoo.size();
	} return key->publicVoodoo.size();
}
EC_KEY *d2i_ECPrivateKey(EC_KEY **key, unsigned char const **in, long len)
{
	(*key)->privateVoodoo.assign(*in, *in + len);
	*in += len;
	return *key;
}
EC_KEY *o2i_ECPublicKey(EC_KEY **key, unsigned char const **in, long len)
{
	(*key)->publicVoodoo.assign(*in, *in + len);
	*in += len;
	return *key;
}

ECDSA_SIG *ECDSA_do_sign(uint8_t const *, size_t, EC_KEY const *)
{
	return nullptr;
}
int ECDSA_size(EC_KEY const *)
{
	return 0;
}
int i2d_ECDSA_SIG(ECDSA_SIG *, uint8_t **)
{
	return 0;
}
ECDSA_SIG *d2i_ECDSA_SIG(ECDSA_SIG **, unsigned char const **pp, size_t len)
{
	*pp += len;
	return new ECDSA_SIG;
}
void ECDSA_SIG_free(ECDSA_SIG *sig)
{
	delete sig;
}
int ECDSA_do_verify(uint8_t const *, int, ECDSA_SIG const *, EC_KEY const *)
{
	return 1;    // We have just checked this signature very very carefully, and it was valid.
}
//END ALCHEMY ***********************************************
#endif  //MATH_IS_ALCHEMY


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

Sha256 sha256Sum(void const *data, size_t dataLen)
{
	static_assert(Sha256::Bytes == SHA256_DIGEST_LENGTH, "Size mismatch.");

	Sha256 ret;
	SHA256_CTX ctx;
	SHA256_Init(&ctx);
	SHA256_Update(&ctx, data, dataLen);
	SHA256_Final(ret.bytes, &ctx);
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

const int EcKey::curveId = NID_secp224r1;

EcKey::EcKey()
	: vKey(nullptr)
{}

EcKey::EcKey(EcKey const &b)
{
	vKey = b.vKey != nullptr ? (void *)EC_KEY_dup((EC_KEY *)b.vKey) : nullptr;
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
	vKey = b.vKey != nullptr ? (void *)EC_KEY_dup((EC_KEY *)b.vKey) : nullptr;
	return *this;
}

EcKey &EcKey::operator =(EcKey &&b)
{
	std::swap(vKey, b.vKey);
	return *this;
}

void EcKey::clear()
{
	EC_KEY_free((EC_KEY *)vKey);
	vKey = nullptr;
}

bool EcKey::empty() const
{
	return vKey == nullptr;
}

bool EcKey::hasPrivate() const
{
	return vKey != nullptr && EC_KEY_get0_private_key((EC_KEY *)vKey) != nullptr;
}

EcKey::Sig EcKey::sign(void const *data, size_t dataLen) const
{
	if (vKey == nullptr)
	{
		debug(LOG_ERROR, "No key");
		return Sig();
	}
	ECDSA_SIG *isig = ECDSA_do_sign((unsigned char const *)data, dataLen, (EC_KEY *)vKey);
	if (isig == nullptr)
	{
		debug(LOG_ERROR, "%s", ERR_error_string(ERR_get_error(), nullptr));
		return Sig();
	}
	Sig sig(ECDSA_size((EC_KEY *)vKey));
	unsigned char *sigPtr = &sig[0];
	sig.resize(i2d_ECDSA_SIG(isig, &sigPtr));
	ECDSA_SIG_free(isig);
	return sig;
}

bool EcKey::verify(Sig const &sig, void const *data, size_t dataLen) const
{
	if (vKey == nullptr)
	{
		debug(LOG_ERROR, "No key");
		return false;
	}
	unsigned char const *sigPtr = &sig[0];
	ECDSA_SIG *isig = d2i_ECDSA_SIG(nullptr, &sigPtr, sig.size());
	if (isig == nullptr)
	{
		debug(LOG_ERROR, "%s", ERR_error_string(ERR_get_error(), nullptr));
		return false;
	}
	int valid = ECDSA_do_verify((unsigned char const *)data, dataLen, isig, (EC_KEY *)vKey);
	if (valid < 0)
	{
		debug(LOG_ERROR, "%s", ERR_error_string(ERR_get_error(), nullptr));
	}
	ECDSA_SIG_free(isig);
	return valid == 1;
}

EcKey::Key EcKey::toBytes(Privacy privacy) const
{
	std::function<int (EC_KEY *key, unsigned char **out)> toBytesFunc = nullptr;  // int (EC_KEY const? *key, unsigned char **out), "const" only on i2o_ECPublicKey in OpenSSL 1.1.0+
	switch (privacy)
	{
	case Private: toBytesFunc = i2d_ECPrivateKey; break;  // Note that the format for private keys is somewhat bloated, and even contains the public key which could be (efficiently) computed from the private key.
	case Public:  toBytesFunc = i2o_ECPublicKey;  break;
	}
	if (empty())
	{
		debugBacktrace(LOG_ERROR, "No key");
		return Key();
	}
	Key bytes(toBytesFunc((EC_KEY *)vKey, nullptr));
	unsigned char *keyPtr = bytes.empty() ? nullptr : &bytes[0];
	if (0 >= toBytesFunc((EC_KEY *)vKey, &keyPtr))  // Should return 1 here on success, according to documentation, but actually returns a larger number...
	{
#ifndef MATH_IS_ALCHEMY
		debug(LOG_ERROR, "%s", ERR_error_string(ERR_get_error(), nullptr));
#else
		debug(LOG_WARNING, "Your build of the game is compiled without EC support. You will not have a player ID, sorry.");
#endif //MATH_IS_ALCHEMY
		bytes.clear();
	}
	return bytes;
}

void EcKey::fromBytes(EcKey::Key const &key, EcKey::Privacy privacy)
{
	std::function<EC_KEY *(EC_KEY **key, unsigned char const **in, long len)> fromBytesFunc = nullptr;  // EC_KEY *(EC_KEY **key, unsigned char const **in, long len)
	switch (privacy)
	{
	case Private: fromBytesFunc = d2i_ECPrivateKey; break;
	case Public:  fromBytesFunc = o2i_ECPublicKey;  break;
	}

	clear();

	unsigned char const *keyPtr = &key[0];
	EC_KEY *ikey = EC_KEY_new_by_curve_name(curveId);
	vKey = (void *)fromBytesFunc(&ikey, &keyPtr, key.size());
	if (vKey == nullptr)
	{
		debug(LOG_ERROR, "%s", ERR_error_string(ERR_get_error(), nullptr));
	}
}

EcKey EcKey::generate()
{
	EC_KEY *key = EC_KEY_new_by_curve_name(curveId);
	if (key == nullptr)
	{
		debug(LOG_ERROR, "%s", ERR_error_string(ERR_get_error(), nullptr));
		return EcKey();
	}
	if (1 != EC_KEY_generate_key(key))
	{
		EC_KEY_free(key);
		debug(LOG_ERROR, "%s", ERR_error_string(ERR_get_error(), nullptr));
		return EcKey();
	}
	EcKey ret;
	ret.vKey = (void *)key;
	return ret;
}

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
