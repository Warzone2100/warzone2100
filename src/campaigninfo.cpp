/*
	This file is part of Warzone 2100.
	Copyright (C) 2005-2024  Warzone 2100 Project

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
/**
 * @file campaiginfo.cpp
 *
 * All the stuff relevant to campaign info.
 */

#include "campaigninfo.h"
#include "lib/framework/frame.h"
#include "lib/framework/physfs_ext.h"
#include "multiplay.h"
#include "qtscript.h"
#include "wzjsonhelpers.h"

#include <nonstd/optional.hpp>
using nonstd::optional;
using nonstd::nullopt;

/* Which campaign are we dealing with? */
static uint32_t camNumber = 1;
static optional<std::string> campaignName = nullopt;

/* cam tweak options */
static std::unordered_map<std::string, nlohmann::json> camTweakOptions;

void setCampaignNumber(uint32_t number)
{
	ASSERT(number < 4, "Campaign Number too high!");
	camNumber = number;

	if (!bMultiPlayer && !bInTutorial)
	{
		if (!campaignName.has_value())
		{
			campaignName = getCampaignNameFromCampaignNumber(number);
		}
	}
	else
	{
		campaignName.reset();
	}
}

uint32_t getCampaignNumber()
{
	return camNumber;
}

void setCampaignName(const std::string& newCampaignName)
{
	campaignName = newCampaignName;
}

void clearCampaignName()
{
	campaignName.reset();
}

std::string getCampaignName()
{
	return campaignName.value_or(std::string());
}

// Fallback for old campaign saves *ONLY*
std::string getCampaignNameFromCampaignNumber(uint32_t campaignNum)
{
	std::vector<CAMPAIGN_FILE> list = readCampaignFiles();

	if (campaignNum > 0 && campaignNum <= list.size())
	{
		return list[campaignNum - 1].name.toStdString();
	}
	return std::string();
}

std::vector<CAMPAIGN_FILE> readCampaignFiles()
{
	static std::vector<CAMPAIGN_FILE> result;
	if (!result.empty())
	{
		return result;
	}

	WZ_PHYSFS_enumerateFiles("campaigns", [&](const char *i) -> bool {
		CAMPAIGN_FILE c;
		WzString filename("campaigns/");
		filename += i;
		if (!filename.endsWith(".json"))
		{
			return true; // continue;
		}

		auto jsonObj = wzLoadJsonObjectFromFile(filename.toUtf8());
		if (!jsonObj.has_value())
		{
			return true; // continue;
		}

		try {
			c = jsonObj.value().get<CAMPAIGN_FILE>();
		}
		catch (const std::exception &e) {
			// Failed to parse the JSON
			debug(LOG_ERROR, "JSON document from %s is invalid: %s", filename.toUtf8().c_str(), e.what());
			return true; // continue;
		}

		result.push_back(c);
		return true; // continue
	});
	return result;
}

void from_json(const nlohmann::json& j, CAMPAIGN_FILE& v)
{
	// required properties
	v.name = j.at("name").get<WzString>();
	v.level = j.at("level").get<WzString>();

	// optional properties
	auto it = j.find("video");
	if (it != j.end())
	{
		v.video = it.value().get<WzString>();
	}
	it = j.find("captions");
	if (it != j.end())
	{
		v.captions = it.value().get<WzString>();
	}
}

void setCamTweakOptions(std::unordered_map<std::string, nlohmann::json> options)
{
	camTweakOptions = options;
}

void clearCamTweakOptions()
{
	camTweakOptions.clear();
}

const std::unordered_map<std::string, nlohmann::json>& getCamTweakOptions()
{
	return camTweakOptions;
}

nlohmann::json getCamTweakOptionsValue(const std::string& identifier, nlohmann::json defaultValue)
{
	auto it = camTweakOptions.find(identifier);
	if (it == camTweakOptions.end())
	{
		return defaultValue;
	}
	return it->second;
}

bool getCamTweakOption_AutosavesOnly()
{
	auto val = getCamTweakOptionsValue("autosavesOnly", false);
	if (!val.is_boolean())
	{
		return false;
	}
	return val.get<bool>();
}

bool getCamTweakOption_PS1Modifiers()
{
	auto val = getCamTweakOptionsValue("ps1Modifiers", false);
	if (!val.is_boolean())
	{
		return false;
	}
	return val.get<bool>();
}

bool getCamTweakOption_FastExp()
{
	auto val = getCamTweakOptionsValue("fastExp", false);
	if (!val.is_boolean())
	{
		return false;
	}
	return val.get<bool>();
}
