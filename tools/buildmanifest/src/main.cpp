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

// wzbuildmanifest - generates, signs, and verifies build manifests

#include "wzbuildcert.h"

#include <sodium.h>
#include <nlohmann/json.hpp>

#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#if !defined(_WIN32)
# include <fcntl.h>
# include <sys/stat.h>
# include <unistd.h>
#endif

using wzbuildcert::optional;
using wzbuildcert::nullopt;

namespace
{

int usage()
{
	fprintf(stderr,
		"Usage:\n"
		"  wzbuildmanifest genkey --out-secret <file> --out-public <file>\n"
		"  wzbuildmanifest sign --manifest <json file> (--key <secret key file> | --key-env <env var>)\n"
		"                   --key-id <id> --out <file>\n"
		"                   (--artifacts-dir <dir> [--exe <path>] | --allow-unverified-artifacts)\n"
		"  wzbuildmanifest verify --envelope <file> --public-key <base64 | @file> [--print-payload]\n"
		"  wzbuildmanifest hash-exe <file>\n");
	return 2;
}

optional<std::string> readFile(const std::string& path)
{
	std::ifstream file(path, std::ios::binary);
	if (!file)
	{
		return nullopt;
	}
	std::ostringstream contents;
	contents << file.rdbuf();
	if (!file.good() && !file.eof())
	{
		return nullopt;
	}
	return contents.str();
}

bool writeFile(const std::string& path, const std::string& contents)
{
	std::ofstream file(path, std::ios::binary | std::ios::trunc);
	if (!file)
	{
		return false;
	}
	file << contents;
	return file.good();
}

// Writes a file readable only by the current user (on Windows, it inherits the directory's ACLs)
bool writePrivateFile(const std::string& path, const std::string& contents)
{
#if defined(_WIN32)
	return writeFile(path, contents);
#else
	int fd = open(path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
	if (fd < 0)
	{
		return false;
	}
	// open() leaves an existing file's permissions alone
	bool ok = (fchmod(fd, S_IRUSR | S_IWUSR) == 0);
	size_t written = 0;
	while (ok && written < contents.size())
	{
		ssize_t result = write(fd, contents.data() + written, contents.size() - written);
		if (result < 0)
		{
			ok = (errno == EINTR);
			continue;
		}
		written += static_cast<size_t>(result);
	}
	return (close(fd) == 0) && ok;
#endif
}

struct FileHash
{
	std::string sha256Hex;
	uint64_t size = 0;
};

optional<FileHash> hashFileSha256(const std::string& path)
{
	std::ifstream file(path, std::ios::binary);
	if (!file)
	{
		return nullopt;
	}
	crypto_hash_sha256_state state;
	crypto_hash_sha256_init(&state);
	std::vector<char> buffer(1024 * 1024);
	uint64_t totalBytes = 0;
	while (file)
	{
		file.read(buffer.data(), buffer.size());
		std::streamsize bytesRead = file.gcount();
		if (bytesRead > 0)
		{
			crypto_hash_sha256_update(&state, reinterpret_cast<const unsigned char*>(buffer.data()), static_cast<size_t>(bytesRead));
			totalBytes += static_cast<uint64_t>(bytesRead);
		}
	}
	if (!file.eof())
	{
		return nullopt;
	}
	unsigned char digest[crypto_hash_sha256_BYTES];
	crypto_hash_sha256_final(&state, digest);
	char hex[crypto_hash_sha256_BYTES * 2 + 1];
	sodium_bin2hex(hex, sizeof(hex), digest, sizeof(digest));
	FileHash result;
	result.sha256Hex = hex;
	result.size = totalBytes;
	return result;
}

// Simple flag parser: --name value
optional<std::string> getArg(int argc, char** argv, const char* name)
{
	for (int i = 2; i < argc - 1; ++i)
	{
		if (strcmp(argv[i], name) == 0)
		{
			return std::string(argv[i + 1]);
		}
	}
	return nullopt;
}

bool hasFlag(int argc, char** argv, const char* name)
{
	for (int i = 2; i < argc; ++i)
	{
		if (strcmp(argv[i], name) == 0)
		{
			return true;
		}
	}
	return false;
}

int cmdGenKey(int argc, char** argv)
{
	auto outSecret = getArg(argc, argv, "--out-secret");
	auto outPublic = getArg(argc, argv, "--out-public");
	if (!outSecret || !outPublic)
	{
		return usage();
	}
	unsigned char pk[crypto_sign_PUBLICKEYBYTES];
	unsigned char sk[crypto_sign_SECRETKEYBYTES];
	crypto_sign_keypair(pk, sk);
	if (!writePrivateFile(outSecret.value(), wzbuildcert::base64Encode(sk, sizeof(sk)) + "\n"))
	{
		fprintf(stderr, "Failed to write: %s\n", outSecret.value().c_str());
		return 1;
	}
	if (!writeFile(outPublic.value(), wzbuildcert::base64Encode(pk, sizeof(pk)) + "\n"))
	{
		fprintf(stderr, "Failed to write: %s\n", outPublic.value().c_str());
		return 1;
	}
	printf("public key: %s\n", wzbuildcert::base64Encode(pk, sizeof(pk)).c_str());
	return 0;
}

std::string trimmed(const std::string& str)
{
	size_t begin = str.find_first_not_of(" \t\r\n");
	size_t end = str.find_last_not_of(" \t\r\n");
	if (begin == std::string::npos)
	{
		return "";
	}
	return str.substr(begin, end - begin + 1);
}

// Verifies the manifest's artifact hashes against the files on disk
bool verifyManifestArtifacts(const nlohmann::json& manifest, const std::string& artifactsDir, const optional<std::string>& exePath)
{
	auto it = manifest.find("artifacts");
	if (it == manifest.end() || !it->is_object())
	{
		fprintf(stderr, "Manifest has no artifacts object\n");
		return false;
	}
	for (auto& entry : it->items())
	{
		const auto& artifact = entry.value();
		std::string path;
		if (entry.key() == "exe" && exePath.has_value())
		{
			path = exePath.value();
		}
		else
		{
			path = artifactsDir + "/" + entry.key();
		}
		if (!artifact.contains("sha256"))
		{
			fprintf(stderr, "warning: skipping artifact with no raw sha256: %s\n", entry.key().c_str());
			continue;
		}
		auto measured = hashFileSha256(path);
		if (!measured.has_value())
		{
			fprintf(stderr, "Failed to read artifact: %s\n", path.c_str());
			return false;
		}
		if (measured.value().sha256Hex != artifact.at("sha256").get<std::string>()
			|| (artifact.contains("size") && measured.value().size != artifact.at("size").get<uint64_t>()))
		{
			fprintf(stderr, "Artifact mismatch: %s (%s)\n", entry.key().c_str(), path.c_str());
			return false;
		}
	}
	return true;
}

int cmdSign(int argc, char** argv)
{
	auto manifestPath = getArg(argc, argv, "--manifest");
	auto keyPath = getArg(argc, argv, "--key");
	auto keyEnvVar = getArg(argc, argv, "--key-env");
	auto keyId = getArg(argc, argv, "--key-id");
	auto outPath = getArg(argc, argv, "--out");
	auto artifactsDir = getArg(argc, argv, "--artifacts-dir");
	auto exePath = getArg(argc, argv, "--exe");
	bool allowUnverified = hasFlag(argc, argv, "--allow-unverified-artifacts");
	if (!manifestPath || !keyId || !outPath || (!artifactsDir && !allowUnverified))
	{
		return usage();
	}
	if (keyPath.has_value() == keyEnvVar.has_value())
	{
		fprintf(stderr, "Specify exactly one of --key or --key-env\n");
		return usage();
	}

	auto payload = readFile(manifestPath.value());
	if (!payload)
	{
		fprintf(stderr, "Failed to read: %s\n", manifestPath.value().c_str());
		return 1;
	}
	nlohmann::json manifest;
	try
	{
		manifest = nlohmann::json::parse(payload.value());
	}
	catch (const std::exception& e)
	{
		fprintf(stderr, "Manifest is not valid JSON: %s\n", e.what());
		return 1;
	}
	if (artifactsDir.has_value() && !verifyManifestArtifacts(manifest, artifactsDir.value(), exePath))
	{
		fprintf(stderr, "Refusing to sign: artifact verification failed\n");
		return 1;
	}

	std::string encodedKey;
	if (keyPath.has_value())
	{
		auto keyFileContents = readFile(keyPath.value());
		if (!keyFileContents)
		{
			fprintf(stderr, "Failed to read key file: %s\n", keyPath.value().c_str());
			return 1;
		}
		encodedKey = trimmed(keyFileContents.value());
	}
	else
	{
		const char* keyEnvValue = getenv(keyEnvVar.value().c_str());
		if (keyEnvValue == nullptr)
		{
			fprintf(stderr, "Environment variable is not set: %s\n", keyEnvVar.value().c_str());
			return 1;
		}
		encodedKey = trimmed(keyEnvValue);
	}
	auto secretKey = wzbuildcert::base64Decode(encodedKey);
	if (!secretKey || secretKey.value().size() != wzbuildcert::SECRETKEY_BYTES)
	{
		fprintf(stderr, "Invalid secret key\n");
		return 1;
	}

	std::string errorDetails;
	auto envelope = wzbuildcert::signAndBuildEnvelope(payload.value(), keyId.value(), secretKey.value().data(), errorDetails);
	sodium_memzero(secretKey.value().data(), secretKey.value().size());
	if (!envelope)
	{
		fprintf(stderr, "Signing failed: %s\n", errorDetails.c_str());
		return 1;
	}
	if (!writeFile(outPath.value(), envelope.value()))
	{
		fprintf(stderr, "Failed to write: %s\n", outPath.value().c_str());
		return 1;
	}
	printf("signed: %s (key_id: %s)\n", outPath.value().c_str(), keyId.value().c_str());
	return 0;
}

int cmdVerify(int argc, char** argv)
{
	auto envelopePath = getArg(argc, argv, "--envelope");
	auto publicKeyArg = getArg(argc, argv, "--public-key");
	if (!envelopePath || !publicKeyArg)
	{
		return usage();
	}
	std::string publicKeyB64 = publicKeyArg.value();
	if (!publicKeyB64.empty() && publicKeyB64.front() == '@')
	{
		auto keyFileContents = readFile(publicKeyB64.substr(1));
		if (!keyFileContents)
		{
			fprintf(stderr, "Failed to read public key file\n");
			return 1;
		}
		publicKeyB64 = trimmed(keyFileContents.value());
	}
	auto publicKey = wzbuildcert::base64Decode(publicKeyB64);
	if (!publicKey || publicKey.value().size() != wzbuildcert::PUBLICKEY_BYTES)
	{
		fprintf(stderr, "Invalid public key\n");
		return 1;
	}

	auto contents = readFile(envelopePath.value());
	if (!contents)
	{
		fprintf(stderr, "Failed to read: %s\n", envelopePath.value().c_str());
		return 1;
	}
	std::string errorDetails;
	auto envelope = wzbuildcert::parseEnvelope(contents.value(), errorDetails);
	if (!envelope)
	{
		fprintf(stderr, "Malformed envelope: %s\n", errorDetails.c_str());
		return 1;
	}
	if (!wzbuildcert::verifySignature(envelope.value(), publicKey.value().data()))
	{
		fprintf(stderr, "SIGNATURE INVALID (key_id: %s)\n", envelope.value().keyId.c_str());
		return 1;
	}
	printf("signature valid (key_id: %s)\n", envelope.value().keyId.c_str());
	if (hasFlag(argc, argv, "--print-payload"))
	{
		fwrite(envelope.value().payload.data(), 1, envelope.value().payload.size(), stdout);
		printf("\n");
	}
	return 0;
}

int cmdHashExe(int argc, char** argv)
{
	if (argc < 3)
	{
		return usage();
	}
	auto result = hashFileSha256(argv[2]);
	if (!result)
	{
		fprintf(stderr, "Failed to read: %s\n", argv[2]);
		return 1;
	}
	printf("%s  %llu\n", result.value().sha256Hex.c_str(), static_cast<unsigned long long>(result.value().size));
	return 0;
}

} // anonymous namespace

int main(int argc, char** argv)
{
	if (sodium_init() < 0)
	{
		fprintf(stderr, "Failed to initialize libsodium\n");
		return 1;
	}
	if (argc < 2)
	{
		return usage();
	}
	if (strcmp(argv[1], "genkey") == 0)
	{
		return cmdGenKey(argc, argv);
	}
	if (strcmp(argv[1], "sign") == 0)
	{
		return cmdSign(argc, argv);
	}
	if (strcmp(argv[1], "verify") == 0)
	{
		return cmdVerify(argc, argv);
	}
	if (strcmp(argv[1], "hash-exe") == 0)
	{
		return cmdHashExe(argc, argv);
	}
	return usage();
}
