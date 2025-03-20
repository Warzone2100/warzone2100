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
/** @file
 * Campaign info defines for the game
 */

#ifndef __INCLUDED_SRC_CAMPAIGNINFO_H__
#define __INCLUDED_SRC_CAMPAIGNINFO_H__

#include <string>
#include <vector>
#include <cstdint>
#include <unordered_map>
#include "lib/framework/wzstring.h"
#include <nlohmann/json_fwd.hpp>

uint32_t getCampaignNumber();
void setCampaignNumber(uint32_t number);

void setCampaignName(const std::string& campaignName);
void clearCampaignName();
std::string getCampaignName();

std::string getCampaignNameFromCampaignNumber(uint32_t number);

struct CAMPAIGN_FILE
{
	WzString name;
	WzString level;
	WzString video;
	WzString captions;
};

void from_json(const nlohmann::json& j, CAMPAIGN_FILE& v);

std::vector<CAMPAIGN_FILE> readCampaignFiles();


void setCamTweakOptions(std::unordered_map<std::string, nlohmann::json> options);
void clearCamTweakOptions();
const std::unordered_map<std::string, nlohmann::json>& getCamTweakOptions();
nlohmann::json getCamTweakOptionsValue(const std::string& identifier, nlohmann::json defaultValue);
bool getCamTweakOption_AutosavesOnly();
bool getCamTweakOption_PS1Modifiers();
bool getCamTweakOption_FastExp();

#endif // __INCLUDED_SRC_CAMPAIGNINFO_H__
