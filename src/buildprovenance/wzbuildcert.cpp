// SPDX-License-Identifier: GPL-2.0-or-later

/*
	This file is part of Warzone 2100.
	Copyright (C) 2026  Warzone 2100 Project (https://github.com/Warzone2100)

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

#include "wzbuildcert.h"

#include <sodium.h>

#include <cstring>

namespace wzbuildcert
{

namespace
{

constexpr const char BEGIN_MARKER[] = "-----BEGIN WZ2100 BUILD MANIFEST-----";
constexpr const char SIG_MARKER[] = "-----WZ2100 SIGNATURE-----";
constexpr const char END_MARKER[] = "-----END WZ2100 BUILD MANIFEST-----";
constexpr const char KEYID_PREFIX[] = "key_id: ";
constexpr const char SIG_PREFIX[] = "ed25519: ";

// Returns the line (sans trailing \n, tolerating one trailing \r), advancing pos past it
optional<std::string> takeLine(const std::string& contents, size_t& pos)
{
	if (pos >= contents.size())
	{
		return nullopt;
	}
	size_t eol = contents.find('\n', pos);
	std::string line = (eol == std::string::npos) ? contents.substr(pos) : contents.substr(pos, eol - pos);
	pos = (eol == std::string::npos) ? contents.size() : eol + 1;
	if (!line.empty() && line.back() == '\r')
	{
		line.pop_back();
	}
	return line;
}

bool hasPrefix(const std::string& str, const char* prefix)
{
	return str.compare(0, strlen(prefix), prefix) == 0;
}

} // anonymous namespace

optional<ParsedEnvelope> parseEnvelope(const std::string& fileContents, std::string& errorDetails)
{
	size_t pos = 0;
	optional<std::string> line = takeLine(fileContents, pos);
	if (!line.has_value() || line.value() != BEGIN_MARKER)
	{
		errorDetails = "Missing begin marker";
		return nullopt;
	}
	size_t payloadStart = pos;
	// The payload is everything between the begin marker and the line before the signature marker
	std::string sigMarkerLine = std::string("\n") + SIG_MARKER + "\n";
	size_t sigMarkerPos = fileContents.find(sigMarkerLine, payloadStart);
	if (sigMarkerPos == std::string::npos || sigMarkerPos < payloadStart)
	{
		errorDetails = "Missing signature marker";
		return nullopt;
	}
	ParsedEnvelope result;
	result.payload = fileContents.substr(payloadStart, sigMarkerPos - payloadStart);
	pos = sigMarkerPos + sigMarkerLine.size();

	line = takeLine(fileContents, pos);
	if (!line.has_value() || !hasPrefix(line.value(), KEYID_PREFIX))
	{
		errorDetails = "Missing key_id";
		return nullopt;
	}
	result.keyId = line.value().substr(strlen(KEYID_PREFIX));
	if (result.keyId.empty())
	{
		errorDetails = "Empty key_id";
		return nullopt;
	}

	line = takeLine(fileContents, pos);
	if (!line.has_value() || !hasPrefix(line.value(), SIG_PREFIX))
	{
		errorDetails = "Missing signature";
		return nullopt;
	}
	optional<std::vector<unsigned char>> signature = base64Decode(line.value().substr(strlen(SIG_PREFIX)));
	if (!signature.has_value() || signature.value().size() != SIGNATURE_BYTES)
	{
		errorDetails = "Invalid signature encoding";
		return nullopt;
	}
	result.signature = std::move(signature.value());

	line = takeLine(fileContents, pos);
	if (!line.has_value() || line.value() != END_MARKER)
	{
		errorDetails = "Missing end marker";
		return nullopt;
	}

	return result;
}

bool verifySignature(const ParsedEnvelope& envelope, const unsigned char* publicKey)
{
	if (envelope.signature.size() != SIGNATURE_BYTES || publicKey == nullptr)
	{
		return false;
	}
	static_assert(SIGNATURE_BYTES == crypto_sign_BYTES, "Size mismatch");
	static_assert(PUBLICKEY_BYTES == crypto_sign_PUBLICKEYBYTES, "Size mismatch");
	return crypto_sign_verify_detached(envelope.signature.data(),
			reinterpret_cast<const unsigned char*>(envelope.payload.data()),
			envelope.payload.size(), publicKey) == 0;
}

optional<std::string> signAndBuildEnvelope(const std::string& payload, const std::string& keyId, const unsigned char* secretKey, std::string& errorDetails)
{
	if (keyId.empty() || keyId.find_first_of("\r\n") != std::string::npos)
	{
		errorDetails = "Invalid key_id";
		return nullopt;
	}
	if (secretKey == nullptr)
	{
		errorDetails = "Missing secret key";
		return nullopt;
	}
	static_assert(SECRETKEY_BYTES == crypto_sign_SECRETKEYBYTES, "Size mismatch");
	unsigned char signature[crypto_sign_BYTES];
	if (crypto_sign_detached(signature, nullptr, reinterpret_cast<const unsigned char*>(payload.data()), payload.size(), secretKey) != 0)
	{
		errorDetails = "Signing failed";
		return nullopt;
	}
	std::string envelope;
	envelope.reserve(payload.size() + 256);
	envelope += BEGIN_MARKER;
	envelope += '\n';
	envelope += payload;
	envelope += '\n';
	envelope += SIG_MARKER;
	envelope += '\n';
	envelope += KEYID_PREFIX;
	envelope += keyId;
	envelope += '\n';
	envelope += SIG_PREFIX;
	envelope += base64Encode(signature, sizeof(signature));
	envelope += '\n';
	envelope += END_MARKER;
	envelope += '\n';
	return envelope;
}

std::string base64Encode(const unsigned char* data, size_t dataLen)
{
	std::string result(sodium_base64_encoded_len(dataLen, sodium_base64_VARIANT_ORIGINAL), '\0');
	sodium_bin2base64(&result[0], result.size(), data, dataLen, sodium_base64_VARIANT_ORIGINAL);
	result.resize(strlen(result.c_str()));
	return result;
}

optional<std::vector<unsigned char>> base64Decode(const std::string& b64)
{
	std::vector<unsigned char> result(b64.size(), 0);
	size_t decodedLen = 0;
	if (sodium_base642bin(result.data(), result.size(), b64.c_str(), b64.size(), nullptr, &decodedLen, nullptr, sodium_base64_VARIANT_ORIGINAL) != 0)
	{
		return nullopt;
	}
	result.resize(decodedLen);
	return result;
}

} // namespace wzbuildcert
