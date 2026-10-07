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

/** \file
 *  The HUD section of the Interface options form
 */

#include "hudoptions.h"
#include "optionsforms.h"

#include "../../hud/hudsettings.h"

void addHudOptions(const std::shared_ptr<OptionsForm> &form)
{
	form->addSection(OptionsSection(N_("HUD"), N_("The look of the in-game interface, and optional additions.")), true);
	{
		auto optionInfo = OptionInfo("interface.hud.preset", N_("HUD Preset"), "");
		auto valueChanger = OptionsDropdown<HudPreset>::make(
			[]() {
				OptionChoices<HudPreset> result;
				result.choices = {
					{ _("Classic"), _("The classic look. Pixel-art, core functionality. Lacking some new interface affordances and features."), HudPreset::Classic },
					{ _("Classic+"), _("The classic look, with detailed tooltips, Power Details, Object Badges and the Spectator Strip."), HudPreset::ClassicPlus },
					{ _("Modern"), _("The Modern skin, with detailed tooltips, Text Outline and every addition below."), HudPreset::Modern },
				};
				if (!result.setCurrentIdxForValue(hudCurrentPreset()))
				{
					result.choices.push_back({ _("Custom"), "", HudPreset::Custom });
					result.currentIdx = result.choices.size() - 1;
				}
				return result;
			},
			[](const auto& newValue) -> bool {
				hudApplyPreset(newValue);
				return true;
			}, true
		);
		form->addOption(optionInfo, valueChanger, true);
	}
	{
		auto optionInfo = OptionInfo("interface.hud.skin", N_("HUD Skin"), N_("The HUD appearance."));
		auto valueChanger = OptionsDropdown<HudSkinId>::make(
			[]() {
				OptionChoices<HudSkinId> result;
				result.choices = {
					{ _("Classic"), _("The interface's original look."), HudSkinId::Classic },
					{ _("Modern"), _("A dark, remastered look. Some of the options below require it."), HudSkinId::Modern },
				};
				result.setCurrentIdxForValue(hudSettings().skin);
				return result;
			},
			[](const auto& newValue) -> bool {
				hudSetSkin(newValue);
				return true;
			}, true
		);
		form->addOption(optionInfo, valueChanger, true);
	}
	{
		auto optionInfo = OptionInfo("interface.hud.tooltips", N_("Tooltips"), N_("What shows when the pointer rests on a button."));
		auto valueChanger = OptionsDropdown<HudTooltips>::make(
			[]() {
				OptionChoices<HudTooltips> result;
				result.choices = {
					{ _("Simple"), _("A short tooltip, as in the classic interface. Hold Shift for the details."), HudTooltips::Simple },
					{ _("Detailed"), _("Costs, times, limits and stats, in a panel beside what the pointer is on."), HudTooltips::Detailed },
				};
				result.setCurrentIdxForValue(hudSettings().tooltips);
				return result;
			},
			[](const auto& newValue) -> bool {
				HudSettings settings = hudSettings();
				settings.tooltips = newValue;
				hudSetSettings(settings);
				return true;
			}, true
		);
		form->addOption(optionInfo, valueChanger, true);
	}
	{
		auto optionInfo = OptionInfo("interface.hud.tooltipDelay", N_("Tooltip Delay"), N_("How long the pointer rests on something before its tooltip shows."));
		auto valueChanger = OptionsDropdown<uint32_t>::make(
			[]() {
				OptionChoices<uint32_t> result;
				result.choices = {
					{ _("Short (0.1 s)"), "", 100 },
					{ _("Normal (0.2 s)"), "", 200 },
					{ _("Long (0.4 s)"), "", 400 },
					{ _("Very long (0.8 s)"), "", 800 },
				};
				result.setCurrentIdxForValue(hudSettings().tooltipDelay);
				return result;
			},
			[](const auto& newValue) -> bool {
				HudSettings settings = hudSettings();
				settings.tooltipDelay = newValue;
				hudSetSettings(settings);
				return true;
			}, true
		);
		form->addOption(optionInfo, valueChanger, true);
	}
	for (size_t i = 0; i < HUD_MODULE_COUNT; ++i)
	{
		const auto module = static_cast<HudModuleId>(i);
		auto optionInfo = OptionInfo(WzString("interface.") + hudModuleOptionKey(module), hudModuleNameMsgid(module), hudModuleDescriptionMsgid(module));
		auto valueChanger = OptionsDropdown<bool>::make(
			[module]() {
				OptionChoices<bool> result;
				result.choices = {
					{ _("Off"), "", false },
					{ _("On"), "", true },
				};
				result.setCurrentIdxForValue(hudModuleEnabled(module));
				return result;
			},
			[module](const auto& newValue) -> bool {
				hudSetModuleEnabled(module, newValue);
				return true;
			}, true
		);
		form->addOption(optionInfo, valueChanger, true);
	}
	{
		auto optionInfo = OptionInfo("interface.hud.textOutline", N_("Text Outline"), N_("An outline around text drawn over the battlefield, to keep it readable."));
		auto valueChanger = OptionsDropdown<bool>::make(
			[]() {
				OptionChoices<bool> result;
				result.choices = {
					{ _("Off"), "", false },
					{ _("On"), "", true },
				};
				result.setCurrentIdxForValue(hudSettings().textOutline);
				return result;
			},
			[](const auto& newValue) -> bool {
				HudSettings settings = hudSettings();
				settings.textOutline = newValue;
				hudSetSettings(settings);
				return true;
			}, true
		);
		form->addOption(optionInfo, valueChanger, true);
	}
	{
		auto optionInfo = OptionInfo("interface.hud.accent", N_("Accent Color"), N_("The Modern skin's color for selected and active things. Its panels take a tint of the same hue (see Panel Tint)."));
		auto valueChanger = OptionsDropdown<HudAccent>::make(
			[]() {
				OptionChoices<HudAccent> result;
				for (size_t i = 0; i < HUD_ACCENT_COUNT; ++i)
				{
					const auto accent = static_cast<HudAccent>(i);
					result.choices.push_back({ gettext(hudAccentNameMsgid(accent)), "", accent });
				}
				result.setCurrentIdxForValue(hudSettings().accent);
				return result;
			},
			[](const auto& newValue) -> bool {
				HudSettings settings = hudSettings();
				settings.accent = newValue;
				hudSetSettings(settings);
				return true;
			}, true
		);
		form->addOption(optionInfo, valueChanger, true);
	}
	{
		auto optionInfo = OptionInfo("interface.hud.panelTint", N_("Panel Tint"), N_("How much of the accent color's hue the Modern skin's panels take."));
		auto valueChanger = OptionsDropdown<HudPanelTint>::make(
			[]() {
				OptionChoices<HudPanelTint> result;
				result.choices = {
					{ _("Full"), "", HudPanelTint::Full },
					{ _("Subtle"), "", HudPanelTint::Subtle },
					{ _("None"), "", HudPanelTint::None },
				};
				result.setCurrentIdxForValue(hudSettings().panelTint);
				return result;
			},
			[](const auto& newValue) -> bool {
				HudSettings settings = hudSettings();
				settings.panelTint = newValue;
				hudSetSettings(settings);
				return true;
			}, true
		);
		form->addOption(optionInfo, valueChanger, true);
	}
	{
		auto optionInfo = OptionInfo("interface.hud.reduceMotion", N_("Reduce Motion"), N_("No panel animations and no pulsing: flashing buttons get a steady highlight instead."));
		auto valueChanger = OptionsDropdown<bool>::make(
			[]() {
				OptionChoices<bool> result;
				result.choices = {
					{ _("Off"), "", false },
					{ _("On"), "", true },
				};
				result.setCurrentIdxForValue(hudSettings().reduceMotion);
				return result;
			},
			[](const auto& newValue) -> bool {
				HudSettings settings = hudSettings();
				settings.reduceMotion = newValue;
				hudSetSettings(settings);
				return true;
			}, true
		);
		form->addOption(optionInfo, valueChanger, true);
	}
	{
		auto optionInfo = OptionInfo("interface.hud.rememberFilters", N_("Remember Filters"), N_("Keep the panels' filters and search when a panel is opened again, for the rest of the game."));
		auto valueChanger = OptionsDropdown<bool>::make(
			[]() {
				OptionChoices<bool> result;
				result.choices = {
					{ _("Off"), "", false },
					{ _("On"), "", true },
				};
				result.setCurrentIdxForValue(hudSettings().rememberFilters);
				return result;
			},
			[](const auto& newValue) -> bool {
				HudSettings settings = hudSettings();
				settings.rememberFilters = newValue;
				hudSetSettings(settings);
				return true;
			}, true
		);
		form->addOption(optionInfo, valueChanger, true);
	}
}
