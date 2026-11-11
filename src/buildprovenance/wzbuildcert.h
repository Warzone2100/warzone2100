// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-WZ2100-GPL-3.0-Section-7-1.0-or-later

/*
	This file is part of Warzone 2100.
	Copyright (C) 2026  Warzone 2100 Project (https://github.com/Warzone2100)

	Warzone 2100 is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Additional terms under Section 7 of the GNU General Public License
	version 3 apply to this file: version 1.0 of the Warzone 2100
	additional terms, or (at your option) any later version published by
	the Warzone 2100 Project, set out in COPYING.SECTION7 in the
	top-level directory of the source distribution. This notice must be
	kept intact.

	Warzone 2100 is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Warzone 2100. If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include <nonstd/optional.hpp>

// Build manifest clearsign envelope format:
//
//   -----BEGIN WZ2100 BUILD MANIFEST-----
//   { ...manifest JSON... }
//   -----WZ2100 SIGNATURE-----
//   key_id: <key id>
//   ed25519: <base64 signature over the exact payload bytes>
//   -----END WZ2100 BUILD MANIFEST-----
//
// The payload is signed verbatim (no JSON canonicalization).
// Framework-free, so the wzbuildmanifest tool can share it. Callers must have initialized libsodium.

namespace wzbuildcert
{

using nonstd::optional;
using nonstd::nullopt;

constexpr size_t PUBLICKEY_BYTES = 32;
constexpr size_t SECRETKEY_BYTES = 64;
constexpr size_t SIGNATURE_BYTES = 64;

struct ParsedEnvelope
{
	std::string payload;			// the exact signed bytes
	std::string keyId;
	std::vector<unsigned char> signature;	// SIGNATURE_BYTES
};

optional<ParsedEnvelope> parseEnvelope(const std::string& fileContents, std::string& errorDetails);

bool verifySignature(const ParsedEnvelope& envelope, const unsigned char* publicKey /* PUBLICKEY_BYTES */);

optional<std::string> signAndBuildEnvelope(const std::string& payload, const std::string& keyId, const unsigned char* secretKey /* SECRETKEY_BYTES */, std::string& errorDetails);

std::string base64Encode(const unsigned char* data, size_t dataLen);
optional<std::vector<unsigned char>> base64Decode(const std::string& b64);

} // namespace wzbuildcert
