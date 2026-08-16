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

#include "wzmanifest.h"
#include "wzbindetails.h"
#include "wzbuildcert.h"

#include "lib/framework/frame.h"
#include "lib/framework/mac_wrapper.h"
#include "lib/framework/wzpaths.h"

#include <nlohmann/json.hpp>

#include <fstream>
#include <sstream>
#include <vector>

namespace
{

constexpr size_t MAX_MANIFEST_SIZE = 1024 * 1024;
constexpr const char MANIFEST_EXTENSION[] = ".buildmanifest";

// NOTE: Main thread only.
bool cachedComputed = false;
optional<std::string> cachedManifestBytes;

std::vector<std::string> manifestPathCandidates(const std::string& exePath)
{
	size_t dirSep = exePath.find_last_of("/\\");
	std::string dir = (dirSep != std::string::npos) ? exePath.substr(0, dirSep + 1) : std::string();
	std::string basename = (dirSep != std::string::npos) ? exePath.substr(dirSep + 1) : exePath;
	size_t extPos = basename.rfind(".exe");
	if (extPos != std::string::npos && extPos == basename.size() - 4)
	{
		basename.resize(extPos);
	}
	std::string filename = basename + MANIFEST_EXTENSION;
	std::vector<std::string> candidates;
#if defined(WZ_OS_MAC)
	// Bundles keep non-code files out of Contents/MacOS
	auto resourceDirPath = wzMacAppBundleGetResourceDirectoryPath();
	if (resourceDirPath.has_value())
	{
		candidates.push_back(resourceDirPath.value().toStdString() + "/" + filename);
	}
#endif
#if defined(WZ_BUILDMANIFEST_DIR) && defined(WZ_BUILDMANIFEST_DIR_ISABSOLUTE)
	candidates.push_back(std::string(WZ_BUILDMANIFEST_DIR) + "/" + filename);
#elif defined(WZ_BUILDMANIFEST_DIR)
	candidates.push_back(getWZInstallPrefix() + "/" + WZ_BUILDMANIFEST_DIR + "/" + filename);
#endif
	candidates.push_back(dir + filename);
	return candidates;
}

optional<std::string> readSmallFile(const std::string& path)
{
	std::ifstream file(path, std::ios::binary);
	if (!file)
	{
		return nullopt;
	}
	std::ostringstream contents;
	contents << file.rdbuf();
	if ((!file.good() && !file.eof()) || contents.str().size() > MAX_MANIFEST_SIZE)
	{
		return nullopt;
	}
	return contents.str();
}

bool exeEntryMatchesSelf(const nlohmann::json& exeEntry, const ExeDetails& details)
{
	auto hashModeIt = exeEntry.find("hash_mode");
	if (hashModeIt != exeEntry.end() && hashModeIt->get<std::string>() == "macho-canonical")
	{
		auto slicesIt = exeEntry.find("slices");
		if (slicesIt == exeEntry.end() || !slicesIt->is_object() || details.machoCanonicalSlices.empty()
			|| slicesIt->size() != details.machoCanonicalSlices.size())
		{
			return false;
		}
		for (const auto& measured : details.machoCanonicalSlices)
		{
			auto archIt = slicesIt->find(measured.arch);
			if (archIt == slicesIt->end() || archIt->get<std::string>() != measured.sha256Hex)
			{
				return false;
			}
		}
		return true;
	}
	if (!details.rawHash.has_value() || !exeEntry.contains("sha256"))
	{
		return false;
	}
	if (exeEntry.at("sha256").get<std::string>() != details.rawHash.value().toString())
	{
		return false;
	}
	if (exeEntry.contains("size") && exeEntry.at("size").get<uint64_t>() != details.fileSize.value_or(0))
	{
		return false;
	}
	return true;
}

// NOTE: Main thread only, and called once.
void computeValidatedManifest(const ExeDetails& details)
{
	cachedComputed = true;
	if (details.path.empty())
	{
		return;
	}
	std::string manifestPath;
	optional<std::string> contents;
	for (const auto& candidate : manifestPathCandidates(details.path))
	{
		contents = readSmallFile(candidate);
		if (contents.has_value())
		{
			manifestPath = candidate;
			break;
		}
	}
	if (!contents.has_value())
	{
		debug(LOG_WZ, "No build manifest found for: %s", details.path.c_str());
		return;
	}
	std::string parseError;
	auto envelope = wzbuildcert::parseEnvelope(contents.value(), parseError);
	bool isSignedEnvelope = envelope.has_value();
	const std::string& manifestText = isSignedEnvelope ? envelope.value().payload : contents.value();
	nlohmann::json manifest;
	try
	{
		manifest = nlohmann::json::parse(manifestText);
	}
	catch (const std::exception& e)
	{
		if (isSignedEnvelope)
		{
			debug(LOG_WZ, "Build manifest payload is not valid JSON (%s): %s", e.what(), manifestPath.c_str());
		}
		else if (contents.value().rfind("-----", 0) == 0)
		{
			debug(LOG_WZ, "Build manifest envelope is malformed (%s): %s", parseError.c_str(), manifestPath.c_str());
		}
		else
		{
			debug(LOG_WZ, "Build manifest is not valid JSON (%s): %s", e.what(), manifestPath.c_str());
		}
		return;
	}
	auto artifactsIt = manifest.find("artifacts");
	if (artifactsIt == manifest.end() || !artifactsIt->is_object() || !artifactsIt->contains("exe"))
	{
		debug(LOG_WZ, "Build manifest has no exe artifact: %s", manifestPath.c_str());
		return;
	}
	if (!exeEntryMatchesSelf(artifactsIt->at("exe"), details))
	{
		debug(LOG_INFO, "Build manifest does not match this executable (stale?): %s", manifestPath.c_str());
		return;
	}
	if (isSignedEnvelope)
	{
		debug(LOG_WZ, "Build manifest (signed, key_id: %s) matches this executable: %s",
			  envelope.value().keyId.c_str(), manifestPath.c_str());
	}
	else
	{
		debug(LOG_WZ, "Build manifest (unsigned) matches this executable: %s", manifestPath.c_str());
	}
	cachedManifestBytes = std::move(contents.value());
}

} // anonymous namespace

void getValidatedManifestBytes(std::function<void(const optional<std::string>&)> resultFunc)
{
	ASSERT_OR_RETURN(, resultFunc != nullptr, "Null resultFunc");
	getSelfExecutableDetails([resultFunc](const ExeDetails& details) {
		if (!cachedComputed)
		{
			computeValidatedManifest(details);
		}
		resultFunc(cachedManifestBytes);
	});
}
