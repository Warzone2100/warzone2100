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

#include "wzbuildinfo.h"
#include "wzbindetails.h"

#include "lib/framework/frame.h"

#include "../wzpropertyproviders.h"

#include <nlohmann/json.hpp>

#include <utility>

namespace
{

std::string buildProperty(BuildPropertyProvider& provider, const char* name)
{
	std::string value;
	if (!provider.getPropertyValue(name, value))
	{
		value.clear();
	}
	return value;
}

nlohmann::ordered_json assembleBuildInfo(const ExeDetails& exeDetails)
{
	BuildPropertyProvider buildProps;

	nlohmann::ordered_json info = nlohmann::ordered_json::object();
	info["schema"] = 1;
	info["v"] = buildProperty(buildProps, "VERSION_STRING");
	info["p"] = buildProperty(buildProps, "PLATFORM");
	info["d"] = buildProperty(buildProps, "WZ_PACKAGE_DISTRIBUTOR");

	nlohmann::ordered_json git = nlohmann::ordered_json::object();
	std::string gitTag = buildProperty(buildProps, "GIT_TAG");
	if (!gitTag.empty())
	{
		git["t"] = gitTag;
	}
	git["c"] = buildProperty(buildProps, "GIT_FULL_HASH");
	std::string wcModified = buildProperty(buildProps, "GIT_WC_MODIFIED");
	if (!wcModified.empty() && wcModified != "0")
	{
		git["m"] = true;
	}
	info["git"] = std::move(git);

	nlohmann::ordered_json exe = nlohmann::ordered_json::object();
	if (exeDetails.rawHash.has_value())
	{
		exe["p"] = (exeDetails.pathConfidence == HashableFile::PathConfidence::KernelAuthoritative) ? "k" : "h";
		exe["size"] = exeDetails.fileSize.value_or(0);
		nlohmann::ordered_json hashes = nlohmann::ordered_json::object();
		hashes["sha256"] = exeDetails.rawHash.value().toString();
		if (!exeDetails.machoCanonicalSlices.empty())
		{
			nlohmann::ordered_json slices = nlohmann::ordered_json::object();
			for (const auto& slice : exeDetails.machoCanonicalSlices)
			{
				slices[slice.arch] = slice.sha256Hex;
			}
			hashes["macho-canonical"] = std::move(slices);
		}
		exe["hashes"] = std::move(hashes);
	}
	else
	{
		exe["error"] = exeDetails.errorDetails;
	}
	info["exe"] = std::move(exe);

	return info;
}

// NOTE: Main thread only.
optional<nlohmann::ordered_json>& buildInfoCache()
{
	static optional<nlohmann::ordered_json> cache;
	return cache;
}

} // anonymous namespace

void getBuildInfo(std::function<void(const nlohmann::ordered_json&)> resultFunc)
{
	ASSERT_OR_RETURN(, resultFunc != nullptr, "Null resultFunc");
	getSelfExecutableDetails([resultFunc](const ExeDetails& exeDetails) {
		auto& cache = buildInfoCache();
		if (!cache.has_value())
		{
			cache = assembleBuildInfo(exeDetails);
		}
		resultFunc(cache.value());
	});
}

optional<nlohmann::ordered_json> getBuildInfoCached()
{
	return buildInfoCache();
}
