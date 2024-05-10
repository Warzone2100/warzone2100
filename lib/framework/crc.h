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
#ifndef _CRC_H_
#define _CRC_H_

#include "types.h"
#include "vector.h"
#include <vector>
#include <string>
#include <functional>
#include <nlohmann/json_fwd.hpp>

namespace wz {
	uint32_t crc_init();
	uint32_t crc_update(uint32_t crc, const void *data, size_t dataLen);
}

// older crc functions - do not combine with the ones above
uint32_t crcSumU16(uint32_t crc, const uint16_t *data, size_t dataLen);
uint32_t crcSumI16(uint32_t crc, const int16_t *data, size_t dataLen);
uint32_t crcSumVector2i(uint32_t crc, const Vector2i *data, size_t dataLen);

struct Sha256
{
	static const int Bytes = 32;

	bool operator ==(Sha256 const &b) const;
	bool operator !=(Sha256 const &b) const { return !(*this == b); }
	bool isZero() const;
	void setZero();
	std::string toString() const;
	void fromString(std::string const &s);

	uint8_t bytes[Bytes] = {0};
};
Sha256 sha256Sum(void const *data, size_t dataLen);
template <>
struct std::hash<Sha256>
{
	std::size_t operator()(const Sha256& k) const
	{
		return std::hash<std::string>{}(k.toString());
	}
};
void to_json(nlohmann::json& j, const Sha256& k);

class EcKey
{
public:
	typedef std::vector<unsigned char> Sig;
	typedef std::vector<unsigned char> Key;
	enum Privacy { Public, Private };

	EcKey();
	EcKey(EcKey const &b);
	EcKey(EcKey &&b);
	~EcKey();
	EcKey &operator =(EcKey const &b);
	EcKey &operator =(EcKey &&b);

	void clear();

	bool empty() const;
	bool hasPrivate() const;

	Sig sign(void const *data, size_t dataLen) const;
	bool verify(Sig const &sig, void const *data, size_t dataLen) const;

	// Exports the specified key (public / private) bytes as a Key (vector of uint8_t).
	//
	// For the public key, the format follows the ANSI X9.63 standard using a byte string of 0x04 || X || Y.
	// (This is the format expected by OpenSSL's o2i_ECPublicKey function, and Apple's SecKeyCreateWithData
	// function for EC public keys.)
	//
	// For the private key, the format is the DER-encoded ECPrivateKey SEQUENCE.
	Key toBytes(Privacy privacy) const;

	// Imports the specified (public / private) Key previously exported via toBytes().
	//
	// See the documentation for toBytes() for the expected format.
	//
	// If supplied the exported public key, the EcKey object supports verify.
	// If supplied the exported private key, the EcKey object supports sign & verify.
	bool fromBytes(Key const &key, Privacy privacy);

	static EcKey generate();

	// Returns the SHA256 hash of the public key
	std::string publicHashString(size_t truncateToLength = 0) const;

	// Returns the public key, hex-encoded
	std::string publicKeyHexString(size_t truncateToLength = 0) const;

private:
	void *vKey;
	static const int curveId;
};

class SessionKeys
{
public:
	static constexpr size_t NonceSize = 24;
public:
	SessionKeys(EcKey const &me, uint32_t me_playerIdx, EcKey const &other, uint32_t other_playerIdx);

public:
	std::vector<uint8_t> encryptMessageForOther(void const *data, size_t dataLen); // not thread-safe
	bool decryptMessageFromOther(void const *data, size_t dataLen, std::vector<uint8_t>& outputDecrypted);

private:
	std::vector<unsigned char> receiveKey;
	std::vector<unsigned char> sendKey;
private:
	// to avoid some repeated allocations
	std::vector<unsigned char> buffer;
};

std::string base64Encode(std::vector<uint8_t> const &bytes);
std::vector<uint8_t> base64Decode(std::string const &str);

std::string b64Tob64UrlSafe(const std::string& inputb64);
std::string b64UrlSafeTob64(const std::string& inputb64urlsafe);

std::vector<uint8_t> genSecRandomBytes(size_t numBytes);
void genSecRandomBytes(void * const buf, const size_t size);

#endif //_CRC_H_
