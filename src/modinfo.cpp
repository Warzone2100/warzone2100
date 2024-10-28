/*
	This file is part of Warzone 2100.
	Copyright (C) 2024  Warzone 2100 Project

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

#include "modinfo.h"
#include "lib/framework/physfs_ext.h"
#include "lib/framework/file.h"
#include "lib/framework/wzpaths.h"
#include "lib/framework/i18n.h"
#include "wzjsonhelpers.h"
#include "campaigninfo.h"
#include "version.h"

#include <nlohmann/json.hpp>

static WzModCompatibilityResult isWZModVersionCompatible(const WZModInfo& modInfo)
{
	const auto tagResult = version_extractVersionNumberFromTag(version_getLatestTag());
	ASSERT(tagResult.has_value(), "No extractable latest tag?? - Please try re-downloading the latest official source bundle");
	const TagVer latestTaggedVersion = tagResult.value_or(TagVer());

	// minVersionSupported incompatibility check
	const auto minVerResult = version_extractVersionNumberFromTag(modInfo.minVersionSupported);
	if (minVerResult.has_value())
	{
		if (latestTaggedVersion < minVerResult.value())
		{
			return WzModCompatibilityResult::NOT_COMPATIBLE;
		}
		// otherwise, fall-through and do additional checks
	}
	else
	{
		// input minVersionSupported didn't seem to be a version string...
		return WzModCompatibilityResult::NOT_COMPATIBLE;
	}

	// additional checks
	const auto maxVerTestedResult = version_extractVersionNumberFromTag(modInfo.maxVersionTested);
	auto buildVersionIdentifier = version_getBuildIdentifierReleaseEnvironment();
	bool isOnReleaseBuild = (buildVersionIdentifier == "release" || buildVersionIdentifier == "preview");
	if (isOnReleaseBuild && (latestTaggedVersion == minVerResult.value()))
	{
		return WzModCompatibilityResult::OK;
	}

	// latestTaggedVersion is >= minVersionSupported - check maxVersionTested
	if (!maxVerTestedResult.has_value())
	{
		// no maxVersionTested? - _might_ be compatible
		return WzModCompatibilityResult::MAYBE_COMPATIBLE;
	}

	// check maxTestedVersion
	if (latestTaggedVersion < maxVerTestedResult.value())
	{
		return WzModCompatibilityResult::OK;
	}
	else if (latestTaggedVersion == maxVerTestedResult.value())
	{
		// if latestTaggedVersion == maxVersionTested on a RELEASE build, then mark as compatible
		// if not on a release build, it's likely that changes have been made since the latestTaggedVersion, so mark as maybe_compatible
		return (isOnReleaseBuild) ? WzModCompatibilityResult::OK : WzModCompatibilityResult::MAYBE_COMPATIBLE;
	}
	else
	{
		return WzModCompatibilityResult::MAYBE_COMPATIBLE;
	}
}

WzModCompatibilityResult WZModInfo::getModCompatibility() const
{
	return isWZModVersionCompatible(*this);
}

void from_json(const nlohmann::json& j, WzModType& v)
{
	auto str = j.get<std::string>();
	if (str == "alternateCampaign")
	{
		v = WzModType::AlternateCampaign;
		return;
	}
	else if (str == "campaignBalanceMod")
	{
		v = WzModType::CampaignBalanceMod;
		return;
	}
	throw nlohmann::json::type_error::create(302, "type value is unknown: \"" + str + "\"", &j);
}

void loadBaseModInfo(const nlohmann::json& j, WZModInfo& v)
{
	if (!j.is_object())
	{
		throw nlohmann::json::type_error::create(302, "type must be an object, but is " + std::string(j.type_name()), &j);
	}

	v.name = j.at("name").get<std::string>();
	v.author = j.at("author").get<std::string>();
	v.type = j.at("type").get<WzModType>();
	v.description = j.at("description").get<WzJsonLocalizedString>();
	v.license = j.at("license").get<std::string>();
	auto it = j.find("version");
	if (it != j.end())
	{
		v.version = it.value().get<std::string>();
	}
	it = j.find("updatesUrl");
	if (it != j.end())
	{
		v.updatesUrl = it.value().get<std::string>();
	}
	v.minVersionSupported = j.at("minVersionSupported").get<std::string>();
	v.maxVersionTested = j.at("maxVersionTested").get<std::string>();
}

void from_json(const nlohmann::json& j, DIFFICULTY_LEVEL& v)
{
	auto str = j.get<std::string>();
	if (str == "super easy")
	{
		v = DL_SUPER_EASY;
		return;
	}
	else if (str == "easy")
	{
		v = DL_EASY;
		return;
	}
	else if (str == "normal")
	{
		v = DL_NORMAL;
		return;
	}
	else if (str == "hard")
	{
		v = DL_HARD;
		return;
	}
	else if (str == "insane")
	{
		v = DL_INSANE;
		return;
	}
	throw nlohmann::json::type_error::create(302, "difficultyLevel value is unknown: \"" + str + "\"", &j);
}

void from_json(const nlohmann::json& j, WzCampaignUniverse& v)
{
	auto str = j.get<std::string>();
	if (str == "wz2100")
	{
		v = WzCampaignUniverse::WZ2100;
		return;
	}
	else if (str == "wz2100extended")
	{
		v = WzCampaignUniverse::WZ2100Extended;
		return;
	}
	else if (str == "unique")
	{
		v = WzCampaignUniverse::Unique;
		return;
	}
	throw nlohmann::json::type_error::create(302, "universe value is unknown: \"" + str + "\"", &j);
}

void from_json(const nlohmann::json& j, WzCampaignTweakOption::Type& v)
{
	auto str = j.get<std::string>();
	if (str == "bool")
	{
		v = WzCampaignTweakOption::Type::Bool;
		return;
	}
	throw nlohmann::json::type_error::create(302, "type value is unknown: \"" + str + "\"", &j);
}

WzCampaignTweakOption builtinTweakOptionFromJSON(const nlohmann::json& j)
{
	WzCampaignTweakOption v;
	v.uniqueIdentifier = j.at("id").get<std::string>();
	v.type = WzCampaignTweakOption::Type::Bool; // always a bool
	v.enabled = j.at("default").get<bool>();
	auto it = j.find("userEditable");
	if (it != j.end())
	{
		v.userEditable = j.at("userEditable").get<bool>();
	}
	else
	{
		v.userEditable = true;
	}
	return v;
}

WzCampaignTweakOption customTweakOptionFromJSON(const nlohmann::json& j)
{
	WzCampaignTweakOption v;
	v.uniqueIdentifier = j.at("id").get<std::string>();
	v.type = j.at("type").get<WzCampaignTweakOption::Type>();
	v.enabled = j.at("default").get<bool>();
	v.userEditable = true;
	v.displayName = j.at("displayName").get<WzJsonLocalizedString>();
	v.description = j.at("description").get<WzJsonLocalizedString>();
	return v;
}

WzCampaignModInfo loadCampaignModInfo(const nlohmann::json& j, const std::string& baseDir)
{
	WzCampaignModInfo v;

	if (!j.is_object())
	{
		throw nlohmann::json::type_error::create(302, "type must be an object, but is " + std::string(j.type_name()), &j);
	}
	loadBaseModInfo(j, v);

	if (v.type == WzModType::AlternateCampaign)
	{
		// campaigns
		std::vector<std::string> campaignJSONList;
		auto& campaigns = j.at("campaigns");
		if (!campaigns.is_array())
		{
			throw nlohmann::json::type_error::create(302, "campaigns type must be an array, but is " + std::string(campaigns.type_name()), &j);
		}
		campaignJSONList = campaigns.get<std::vector<std::string>>();
		for (const auto& jsonName : campaignJSONList)
		{
			std::string fullJSONPath = baseDir + "/campaigns/" + jsonName;
			auto jsonObj = wzLoadJsonObjectFromFile(fullJSONPath.c_str());
			if (!jsonObj.has_value())
			{
				debug(LOG_ERROR, "Failed to find: campaigns/%s", jsonName.c_str());
				continue;
			}

			CAMPAIGN_FILE c;
			try {
				c = jsonObj.value().get<CAMPAIGN_FILE>();
			}
			catch (const std::exception &e) {
				// Failed to parse the JSON
				debug(LOG_ERROR, "JSON document from %s is invalid: %s", jsonName.c_str(), e.what());
				continue;
			}

			v.campaignFiles.push_back(c);
		}
		if (v.campaignFiles.empty())
		{
			throw nlohmann::json::parse_error::create(302, 0, "\"campaigns\" must list one or more valid campaign JSON files", &j);
		}
	}

	// difficultyLevels
	auto& difficultyLevels = j.at("difficultyLevels");
	if (difficultyLevels.is_array())
	{
		for (const auto& i : difficultyLevels)
		{
			v.difficultyLevels.insert(i.get<DIFFICULTY_LEVEL>());
		}
	}
	else if (difficultyLevels.is_string())
	{
		if (difficultyLevels.get<std::string>() == "default")
		{
			v.difficultyLevels.clear();
		}
		else
		{
			throw nlohmann::json::type_error::create(302, "difficultyLevels is a string, but is not \"default\"", &j);
		}
	}
	else
	{
		throw nlohmann::json::type_error::create(302, "difficultyLevels is not an array or string, but is " + std::string(difficultyLevels.type_name()), &j);
	}

	// universe
	v.universe = j.at("universe").get<WzCampaignUniverse>();

	// camTweakOptions (optional)
	v.camTweakOptions.clear();
	auto it = j.find("camTweakOptions");
	if (it != j.end())
	{
		auto& tweakOptions = it.value();
		if (!tweakOptions.is_array())
		{
			throw nlohmann::json::type_error::create(302, "camTweakOptions type must be an array, but is " + std::string(tweakOptions.type_name()), &j);
		}

		v.camTweakOptions.reserve(tweakOptions.size());
		for (auto& opt : tweakOptions)
		{
			v.camTweakOptions.push_back(builtinTweakOptionFromJSON(opt));
		}
	}

	// customTweakOptions (optional)
	v.customTweakOptions.clear();
	it = j.find("customTweakOptions");
	if (it != j.end())
	{
		auto& tweakOptions = it.value();
		if (!tweakOptions.is_array())
		{
			throw nlohmann::json::type_error::create(302, "customTweakOptions type must be an array, but is " + std::string(tweakOptions.type_name()), &j);
		}

		v.customTweakOptions.reserve(tweakOptions.size());
		for (auto& opt : tweakOptions)
		{
			v.customTweakOptions.push_back(customTweakOptionFromJSON(opt));
		}
	}

	return v;
}

optional<WzCampaignModInfo> loadCampaignModInfoFromFile(const std::string& filePath, const std::string& baseDir)
{
	auto jsonObj = wzLoadJsonObjectFromFile(filePath);
	if (!jsonObj.has_value())
	{
		return nullopt;
	}

	// from_json
	WzCampaignModInfo result;
	try {
		result = loadCampaignModInfo(jsonObj.value(), baseDir);
	}
	catch (const std::exception &e) {
		// Failed to parse the JSON
		debug(LOG_ERROR, "JSON document from %s is invalid: %s", filePath.c_str(), e.what());
		return nullopt;
	}
	return result;
}
