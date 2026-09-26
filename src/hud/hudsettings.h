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
	additional terms, or (at your option) any later version, as published
	by the Warzone 2100 Project. They are provided in COPYING.SECTION7 in
	the top-level directory of the source distribution. This notice must
	be kept intact.

	Warzone 2100 is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Warzone 2100. If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef __INCLUDED_SRC_HUD_HUDSETTINGS_H__
#define __INCLUDED_SRC_HUD_HUDSETTINGS_H__

#include <array>
#include <cstdint>
#include <functional>
#include <string>

#include <nonstd/optional.hpp>

/// The look of the in-game HUD. Every skin keeps every existing element where it is (tests/hud).
enum class HudSkinId : uint8_t
{
	Classic = 0,
	Modern = 1,
};

enum class HudTooltips : uint8_t
{
	Simple = 0,
	Detailed = 1,
};

/// Optional HUD additions, each its own on/off option, usable with any skin
enum class HudModuleId : uint8_t
{
	PowerDetails,
	StatusLine,
	ObjectBadges,
	IdleIndicators,
	RowSummary,
	SelectionReadout,
	ReadableBars,
	PanelFilters,
	SpectatorStrip,
};
constexpr size_t HUD_MODULE_COUNT = static_cast<size_t>(HudModuleId::SpectatorStrip) + 1;

/// A preset sets the skin and the toggles (it's derived from them, never stored)
enum class HudPreset : uint8_t
{
	Classic = 0,
	ClassicPlus = 1,
	Modern = 2,
	Custom = 3,
};

struct HudSettings
{
	HudSkinId skin = HudSkinId::Classic;
	HudTooltips tooltips = HudTooltips::Simple;
	std::array<bool, HUD_MODULE_COUNT> modules = {};
	bool textOutline = false;
	bool reduceMotion = false;
	bool rememberFilters = false;
	uint32_t tooltipDelay = 200;
	bool showModulesInTutorial = false;

	bool operator==(const HudSettings &other) const;
	bool operator!=(const HudSettings &other) const { return !(*this == other); }
};

const HudSettings &hudSettings();
HudSettings hudPresetSettings(HudPreset preset);
/// The preset whose skin and toggles are the current ones, or Custom
HudPreset hudCurrentPreset();

/// Change the settings. Whatever needs the HUD rebuilt (the modules) is rebuilt when it's safe (hudRequestRebuild())
void hudSetSettings(const HudSettings &settings);
void hudApplyPreset(HudPreset preset);
void hudSetSkin(HudSkinId skin);
void hudSetModuleEnabled(HudModuleId module, bool enabled);

bool hudModuleEnabled(HudModuleId module);
/// Whether an enabled module is shown now: not in the tutorial (unless asked for), and not before the HUD exists
bool hudModuleActive(HudModuleId module);
bool hudDetailedTooltips();
bool hudReduceMotion();

const char *hudModuleOptionKey(HudModuleId module);
/// A module's name and description, untranslated (for the options screen, which translates them itself)
const char *hudModuleNameMsgid(HudModuleId module);
const char *hudModuleDescriptionMsgid(HudModuleId module);

/// Config file I/O (src/configuration.cpp)
void hudLoadSettings(const std::function<nonstd::optional<int> (const std::string &key)> &getInteger);
void hudSaveSettings(const std::function<void (const std::string &key, int value)> &setInteger);

#endif // __INCLUDED_SRC_HUD_HUDSETTINGS_H__
