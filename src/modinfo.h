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

#ifndef __INCLUDED_SRC_MODINFO_H__
#define __INCLUDED_SRC_MODINFO_H__

#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <nlohmann/json_fwd.hpp>
#include "difficulty.h"
#include "campaigninfo.h"
#include "wzjsonlocalizedstring.h"

#include <nonstd/optional.hpp>
using nonstd::optional;
using nonstd::nullopt;

enum class WzModType
{
	CampaignBalanceMod,
	AlternateCampaign,
};

enum class WzModCompatibilityResult
{
	NOT_COMPATIBLE,
	OK,
	MAYBE_COMPATIBLE
};

class WZModInfo
{
public:
	std::string name;
	std::string author;
	WzJsonLocalizedString description;
	WzModType type;
	std::string license;
	optional<std::string> version;
	optional<std::string> updatesUrl;
	// compatibility
	std::string minVersionSupported;
	std::string maxVersionTested;
	// from mod file itself
	std::string modFilename;
	std::string modPath;
	std::vector<unsigned char> modBannerPNGData;
public:
	WzModCompatibilityResult getModCompatibility() const;
};

enum class WzCampaignUniverse
{
	WZ2100,
	WZ2100Extended,
	Unique
};

struct WzCampaignTweakOption
{
	enum class Type
	{
		Bool
	};

	std::string uniqueIdentifier;
	Type type;
	bool enabled = false;
	bool userEditable = true;
	WzJsonLocalizedString displayName;
	WzJsonLocalizedString description;
};

class WzCampaignModInfo : public WZModInfo
{
public:
	std::vector<CAMPAIGN_FILE> campaignFiles;
	std::unordered_set<DIFFICULTY_LEVEL> difficultyLevels;
	std::vector<WzCampaignTweakOption> camTweakOptions;
	std::vector<WzCampaignTweakOption> customTweakOptions;
	WzCampaignUniverse universe = WzCampaignUniverse::WZ2100Extended;
};

optional<WzCampaignModInfo> loadCampaignModInfoFromFile(const std::string& filePath, const std::string& baseDir);

#endif // __INCLUDED_SRC_MODINFO_H__
