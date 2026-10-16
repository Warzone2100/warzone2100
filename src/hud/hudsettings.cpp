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

#include "hudsettings.h"
#include "hud.h"

#include "lib/framework/frame.h"

#include "../main.h"
#include "../qtscript.h"

namespace
{

HudSettings currentSettings;
bool spectatorStripTwoLines = true;

struct ModuleInfo
{
	const char *key;
	const char *name;
	const char *description;
};

const ModuleInfo moduleInfo[HUD_MODULE_COUNT] = {
	{"hud.powerDetails", N_("Power Details"), N_("Queued power and the derricks in the power bar's tooltip, and income per minute and unpowered derricks in the status line.")},
	{"hud.statusLine", N_("Status Line"), N_("Beside the reticule: the game clock and speed, your units against the unit limit, and the research to finish next.")},
	{"hud.objectBadges", N_("Object Badges"), N_("On factory and research facility buttons: how many are queued, whether production repeats, and the time left.")},
	{"hud.idleIndicators", N_("Idle Indicators"), N_("Counts of idle trucks, factories and research facilities. Click one to select the next idle one.")},
	{"hud.rowSummary", N_("Row Summary"), N_("A slim line above the row of trucks, factories, research facilities or commanders: how many there are, how many are idle, and a filter for each type.")},
	{"hud.selectionReadout", N_("Selection Readout"), N_("What the selected units are made of, by type. Click a type to keep only those units.")},
	{"hud.readableBars", N_("Readable Health Bars"), N_("Thicker health bars, with an outline and segment ticks.")},
	{"hud.panelFilters", N_("Panel Filters"), N_("Category filters and a search box above the build, manufacture and research panels.")},
	{"hud.spectatorStrip", N_("Spectator Strip"), N_("For spectators: each player's power and units, at the top of the screen.")},
	{"hud.objectives", N_("Objectives"), N_("How a mission's objectives show with the Modern skin. The intelligence screen always lists them.")},
};

bool sameLook(const HudSettings &a, const HudSettings &b)
{
	return a.skin == b.skin && a.tooltips == b.tooltips && a.modules == b.modules && a.textOutline == b.textOutline;
}

}

bool HudSettings::operator==(const HudSettings &other) const
{
	return sameLook(*this, other) && accent == other.accent && panelTint == other.panelTint && reduceMotion == other.reduceMotion && rememberFilters == other.rememberFilters && toasts == other.toasts
		&& objectivesPopUp == other.objectivesPopUp && tooltipDelay == other.tooltipDelay && showModulesInTutorial == other.showModulesInTutorial;
}

const HudSettings &hudSettings()
{
	return currentSettings;
}

HudSettings hudPresetSettings(HudPreset preset)
{
	HudSettings settings = currentSettings;
	settings.modules.fill(false);
	settings.skin = HudSkinId::Classic;
	settings.tooltips = HudTooltips::Simple;
	settings.textOutline = false;
	auto enable = [&settings](HudModuleId module) { settings.modules[static_cast<size_t>(module)] = true; };
	switch (preset)
	{
	case HudPreset::Classic:
	case HudPreset::Custom:
		break;
	case HudPreset::ClassicPlus:
		settings.tooltips = HudTooltips::Detailed;
		enable(HudModuleId::PowerDetails);
		enable(HudModuleId::ObjectBadges);
		enable(HudModuleId::SpectatorStrip);
		break;
	case HudPreset::Modern:
		settings.skin = HudSkinId::Modern;
		settings.tooltips = HudTooltips::Detailed;
		settings.textOutline = true;
		enable(HudModuleId::PowerDetails);
		enable(HudModuleId::StatusLine);
		enable(HudModuleId::ObjectBadges);
		enable(HudModuleId::IdleIndicators);
		enable(HudModuleId::RowSummary);
		enable(HudModuleId::SelectionReadout);
		enable(HudModuleId::ReadableBars);
		enable(HudModuleId::PanelFilters);
		enable(HudModuleId::SpectatorStrip);
		enable(HudModuleId::Objectives);
		break;
	}
	return settings;
}

HudPreset hudCurrentPreset()
{
	for (HudPreset preset : {HudPreset::Classic, HudPreset::ClassicPlus, HudPreset::Modern})
	{
		if (sameLook(hudPresetSettings(preset), currentSettings))
		{
			return preset;
		}
	}
	return HudPreset::Custom;
}

void hudSetSettings(const HudSettings &settings)
{
	const HudSettings previous = currentSettings;
	currentSettings = settings;
	if (previous.skin != settings.skin || previous.tooltipDelay != settings.tooltipDelay)
	{
		hudSkinChanged();
	}
	if (previous.skin != settings.skin || previous.modules != settings.modules || previous.tooltips != settings.tooltips
		|| previous.showModulesInTutorial != settings.showModulesInTutorial)
	{
		hudRequestRebuild();
	}
}

void hudApplyPreset(HudPreset preset)
{
	if (preset != HudPreset::Custom)
	{
		hudSetSettings(hudPresetSettings(preset));
	}
}

void hudSetSkin(HudSkinId skin)
{
	HudSettings settings = currentSettings;
	settings.skin = skin;
	hudSetSettings(settings);
}

void hudSetModuleEnabled(HudModuleId module, bool enabled)
{
	HudSettings settings = currentSettings;
	settings.modules[static_cast<size_t>(module)] = enabled;
	hudSetSettings(settings);
}

bool hudModuleEnabled(HudModuleId module)
{
	return currentSettings.modules[static_cast<size_t>(module)];
}

bool hudModuleActive(HudModuleId module)
{
	if (!hudModuleEnabled(module) || GetGameMode() != GS_NORMAL)
	{
		return false;
	}
	return !bInTutorial || currentSettings.showModulesInTutorial;
}

bool hudDetailedTooltips()
{
	return currentSettings.tooltips == HudTooltips::Detailed && (!bInTutorial || currentSettings.showModulesInTutorial);
}

bool hudReduceMotion()
{
	return currentSettings.reduceMotion;
}

const char *hudModuleOptionKey(HudModuleId module)
{
	return moduleInfo[static_cast<size_t>(module)].key;
}

const char *hudModuleNameMsgid(HudModuleId module)
{
	return moduleInfo[static_cast<size_t>(module)].name;
}

const char *hudModuleDescriptionMsgid(HudModuleId module)
{
	return moduleInfo[static_cast<size_t>(module)].description;
}

const char *hudAccentNameMsgid(HudAccent accent)
{
	switch (accent)
	{
	case HudAccent::Blue: return N_("Blue");
	case HudAccent::Teal: return N_("Teal");
	case HudAccent::Green: return N_("Green");
	case HudAccent::Purple: return N_("Purple");
	case HudAccent::Magenta: return N_("Magenta");
	case HudAccent::Red: return N_("Red");
	case HudAccent::Orange: return N_("Orange");
	case HudAccent::Gray: return N_("Gray");
	}
	return N_("Blue");
}

bool hudSpectatorStripTwoLines()
{
	return spectatorStripTwoLines;
}

void hudSetSpectatorStripTwoLines(bool twoLines)
{
	spectatorStripTwoLines = twoLines;
}

HudObjectivesShow hudObjectivesShow()
{
	if (currentSettings.modules[static_cast<size_t>(HudModuleId::Objectives)])
	{
		return HudObjectivesShow::Card;
	}
	return currentSettings.objectivesPopUp ? HudObjectivesShow::PopUp : HudObjectivesShow::Off;
}

void hudSetObjectivesShow(HudObjectivesShow show)
{
	HudSettings settings = currentSettings;
	settings.modules[static_cast<size_t>(HudModuleId::Objectives)] = show == HudObjectivesShow::Card;
	if (show != HudObjectivesShow::Card)
	{
		settings.objectivesPopUp = show == HudObjectivesShow::PopUp;
	}
	hudSetSettings(settings);
}

void hudLoadSettings(const std::function<nonstd::optional<int> (const std::string &key)> &getInteger)
{
	HudSettings settings;
	auto get = [&getInteger](const char *key, int fallback) {
		auto value = getInteger(key);
		return value.has_value() ? value.value() : fallback;
	};
	settings.skin = get("hudSkin", 0) == 1 ? HudSkinId::Modern : HudSkinId::Classic;
	settings.tooltips = get("hudTooltips", 0) == 1 ? HudTooltips::Detailed : HudTooltips::Simple;
	for (size_t i = 0; i < HUD_MODULE_COUNT; ++i)
	{
		// (Configs from before the Objectives option: the card is on with the Modern skin)
		const bool onByDefault = static_cast<HudModuleId>(i) == HudModuleId::Objectives && settings.skin == HudSkinId::Modern;
		settings.modules[i] = get(moduleInfo[i].key, onByDefault ? 1 : 0) != 0;
	}
	settings.textOutline = get("hudTextOutline", 0) != 0;
	settings.accent = static_cast<HudAccent>(std::max(0, std::min(get("hudAccent", 0), static_cast<int>(HUD_ACCENT_COUNT) - 1)));
	settings.panelTint = static_cast<HudPanelTint>(std::max(0, std::min(get("hudPanelTint", 0), static_cast<int>(HudPanelTint::None))));
	settings.reduceMotion = get("hudReduceMotion", 0) != 0;
	settings.rememberFilters = get("hudRememberFilters", 0) != 0;
	settings.toasts = get("hudToasts", 1) != 0;
	settings.objectivesPopUp = get("hudObjectivesPopUp", 1) != 0;
	settings.tooltipDelay = static_cast<uint32_t>(std::max(0, std::min(get("hudTooltipDelay", 200), 2000)));
	settings.showModulesInTutorial = get("hudModulesInTutorial", 0) != 0;
	hudSetSettings(settings);
	spectatorStripTwoLines = get("hudSpectatorStripLines", 2) != 1;
}

void hudSaveSettings(const std::function<void (const std::string &key, int value)> &setInteger)
{
	setInteger("hudSkin", static_cast<int>(currentSettings.skin));
	setInteger("hudTooltips", static_cast<int>(currentSettings.tooltips));
	for (size_t i = 0; i < HUD_MODULE_COUNT; ++i)
	{
		setInteger(moduleInfo[i].key, currentSettings.modules[i] ? 1 : 0);
	}
	setInteger("hudTextOutline", currentSettings.textOutline ? 1 : 0);
	setInteger("hudAccent", static_cast<int>(currentSettings.accent));
	setInteger("hudPanelTint", static_cast<int>(currentSettings.panelTint));
	setInteger("hudReduceMotion", currentSettings.reduceMotion ? 1 : 0);
	setInteger("hudRememberFilters", currentSettings.rememberFilters ? 1 : 0);
	setInteger("hudToasts", currentSettings.toasts ? 1 : 0);
	setInteger("hudObjectivesPopUp", currentSettings.objectivesPopUp ? 1 : 0);
	setInteger("hudTooltipDelay", static_cast<int>(currentSettings.tooltipDelay));
	setInteger("hudModulesInTutorial", currentSettings.showModulesInTutorial ? 1 : 0);
	setInteger("hudSpectatorStripLines", spectatorStripTwoLines ? 2 : 1);
}
