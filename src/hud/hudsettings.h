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

/// The Modern skin's accent color (its panels are tinted with the same hue)
enum class HudAccent : uint8_t
{
	Blue = 0,
	Teal,
	Green,
	Purple,
	Magenta,
	Red,
	Orange,
	Gray,
};
constexpr size_t HUD_ACCENT_COUNT = static_cast<size_t>(HudAccent::Gray) + 1;

/// How much of the accent's hue the Modern skin's panels take
enum class HudPanelTint : uint8_t
{
	Full = 0,
	Subtle,
	None,
};

/// Where the HUD's elements go. Standard keeps them where they always were, Compact fits a height of 360, and Auto is
/// Compact below a height of 480 (510 with Larger Touch Targets)
enum class HudLayoutChoice : uint8_t
{
	Auto = 0,
	Standard = 1,
	Compact = 2,
};

/// The reticule. Full is the Classic cluster. Compact is the same hexes in a band two high, giving its height to the
/// stats panel in the Compact layout. Auto is Compact in the Compact layout
enum class HudReticuleChoice : uint8_t
{
	Auto = 0,
	Full = 1,
	Compact = 2,
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
	Objectives,
};
constexpr size_t HUD_MODULE_COUNT = static_cast<size_t>(HudModuleId::Objectives) + 1;

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
	HudAccent accent = HudAccent::Blue;
	HudPanelTint panelTint = HudPanelTint::Full;
	bool reduceMotion = false;
	bool rememberFilters = false;
	/// The HUD's notices (ex. a research report arrived), at the top of the screen
	bool toasts = true;
	/// With the objectives card off: the objectives pop up over the reticule when they change (else: only a dot on
	/// the intelligence button)
	bool objectivesPopUp = true;
	uint32_t tooltipDelay = 200;
	/// With touch, how long a press is held for the element's popover (the Modern skin's long-press)
	uint32_t longPressDelay = 400;
	HudLayoutChoice layout = HudLayoutChoice::Auto;
	HudReticuleChoice reticule = HudReticuleChoice::Auto;
	/// Taller panel header controls, and in the Compact layout one menu button for them (not tied to touch detection)
#if defined(__ANDROID__)
	bool largerTouchTargets = true;
#else
	bool largerTouchTargets = false;
#endif
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

/// How the objectives show (the Objectives option): the card (its module), a pop-up over the reticule, or neither
enum class HudObjectivesShow : uint8_t
{
	Card,
	PopUp,
	Off
};
HudObjectivesShow hudObjectivesShow();
void hudSetObjectivesShow(HudObjectivesShow show);

const char *hudModuleOptionKey(HudModuleId module);
/// A module's name and description, untranslated (for the options screen, which translates them itself)
const char *hudModuleNameMsgid(HudModuleId module);
const char *hudModuleDescriptionMsgid(HudModuleId module);
/// An accent's name, untranslated
const char *hudAccentNameMsgid(HudAccent accent);

/// The spectator strip's view (two lines per player, or one): remembered, but not one of the settings
bool hudSpectatorStripTwoLines();
void hudSetSpectatorStripTwoLines(bool twoLines);

/// Config file I/O (src/configuration.cpp)
void hudLoadSettings(const std::function<nonstd::optional<int> (const std::string &key)> &getInteger);
void hudSaveSettings(const std::function<void (const std::string &key, int value)> &setInteger);

#endif // __INCLUDED_SRC_HUD_HUDSETTINGS_H__
