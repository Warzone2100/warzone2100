// SPDX-License-Identifier: GPL-2.0-or-later

/*
	This file is part of Warzone 2100.
	Copyright (C) 2025  Warzone 2100 Project

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
/** \file
 *  Interface Options form
 */

#include "optionsforms.h"

#include "../../warzoneconfig.h"
#include "../../display3d.h"
#include "../../radar.h"
#include "../../hci/groups.h"
#include "../../seqdisp.h"
#include "lib/ivis_opengl/bitimage.h"
#include "lib/framework/wzapp.h"
#include "../../difficulty.h"
#include "../../multiint.h"
#include "../../multiplay.h"
#include "../../clparse.h"
#include "../../frend.h"
#include "../../main.h"

#include <fmt/core.h>

// MARK: -

static std::map<WzString, const char *> languageIconsMap()
{
	const std::map<WzString, const char *> iconsMap = {
		{"", "icon-system.png"},
		{"ar_SA", "lang-AR.png"},
		{"bg", "flag-BG.png"},
		{"ca_ES", "flag-ES.png"},
		{"cs_CZ", "flag-CZ.png"},
		{"da", "flag-DK.png"},
		{"de", "flag-DE.png"},
		{"el_GR", "flag-GR.png"},
		{"en", "flag-US.png"},
		{"en_GB", "flag-GB.png"},
		{"es", "flag-ES.png"},
		{"et_EE", "flag-EE.png"},
		{"eu_ES", "flag-ES.png"},
		{"fi", "flag-FI.png"},
		{"fr", "flag-FR.png"},
		{"fy_NL", "flag-NL.png"},
		{"ga_IE", "flag-IE.png"},
		{"hr", "flag-HR.png"},
		{"hu", "flag-HU.png"},
		{"id", "flag-ID.png"},
		{"it", "flag-IT.png"},
		{"ko_KR", "flag-KR.png"},
		{"la", "flag-VA.png"},
		{"lt", "flag-LT.png"},
		{"lv", "flag-LV.png"},
		{"nb_NO", "flag-NO.png"},
		{"nn_NO", "flag-NO.png"},
		{"nl", "flag-NL.png"},
		{"pl", "flag-PL.png"},
		{"pt_BR", "flag-BR.png"},
		{"pt", "flag-PT.png"},
		{"ro", "flag-RO.png"},
		{"ru", "flag-RU.png"},
		{"sk", "flag-SK.png"},
		{"sl_SI", "flag-SI.png"},
		{"sv_SE", "flag-SE.png"},
		{"sv", "flag-SE.png"},
		{"tr", "flag-TR.png"},
		{"uz", "flag-UZ.png"},
		{"uk_UA", "flag-UA.png"},
		{"zh_CN", "flag-CN.png"},
		{"zh_TW", "flag-TW.png"},
	};
	return iconsMap;
}

std::shared_ptr<OptionsForm> makeInterfaceOptionsForm(const std::function<void()> languageDidChangeHandler)
{
	auto result = OptionsForm::make();
	result->setRefreshOptionsOnScreenSizeChange(true);

	result->addSection(OptionsSection(N_("General"), ""), true);
	{
		auto optionInfo = OptionInfo("interface.language", N_("Language"), "");
		optionInfo.addAvailabilityCondition(IsNotInGame);
		// NOTE: By capturing a copy of pFlagsImages in the populate handler, we ensure it stays around until the OptionsDropdown is deleted
		std::shared_ptr<IMAGEFILE> flagsImagesCopy = (GetGameMode() != GS_NORMAL) ? getFlagsImages() : nullptr;
		auto valueChanger = OptionsDropdown<WzString>::make(
			[flagsImagesCopy]() {
				const bool bIsInGame = (GetGameMode() == GS_NORMAL);
				if (bIsInGame)
				{
					// shortcut to avoid loading and processing lots of strings - just populate with the currrent language (can't be changed in-game anyway)
					OptionChoices<WzString> result;
					result.choices.push_back({ getLanguageName(), "", getLanguage(), false });
					result.currentIdx = 0;
					return result;
				}
				auto locales = getLocales();
				auto iconsMap = languageIconsMap();
				OptionChoices<WzString> result;
				for (size_t i = 0; i < locales.size(); i++)
				{
					AtlasImageDef *icon = nullptr;
					auto mapIcon = iconsMap.find(locales[i].code);
					if (mapIcon != iconsMap.end())
					{
						icon = (flagsImagesCopy) ? flagsImagesCopy->find(mapIcon->second) : nullptr;
					}
					result.choices.push_back({ locales[i].name, "", locales[i].code, false, icon });
				}
				auto currentCode = WzString::fromUtf8(getLanguage());
				result.setCurrentIdxForValue(currentCode);
				return result;
			},
			[languageDidChangeHandler](const auto& newValue) -> bool {
				if (!setLanguage(newValue.toUtf8().c_str()))
				{
					return false;
				}
				// hack to update translations of AI names and tooltips
				readAIs();
				// call the language change handler
				if (languageDidChangeHandler)
				{
					languageDidChangeHandler();
				}
				return true;
			}, false
		);
		result->addOption(optionInfo, valueChanger, true);
	}
	{
		auto weakOptionsForm = std::weak_ptr<OptionsForm>(result);
		auto optionInfo = OptionInfo("interface.displayscale", N_("Display Scale"), "");
		auto valueChanger = OptionsDropdown<unsigned int>::make(
			[]() {
				std::vector<unsigned int> displayScales = availableDisplayScalesSorted();
				unsigned int maximumDisplayScale = wzGetMaximumDisplayScaleForCurrentWindowSize();
				OptionChoices<unsigned int> result;
				for (unsigned int displayScale : displayScales)
				{
					if (displayScale > maximumDisplayScale)
					{
						continue;
					}
					result.choices.push_back({ WzString::number(displayScale) + "%", "", displayScale });
				}
				unsigned int currValue = war_GetDisplayScale();
				if (!result.setCurrentIdxForValue(currValue))
				{
					// add "Custom" item
					result.choices.push_back({ WzString::number(currValue) + "%", "", currValue });
					result.currentIdx = result.choices.size() - 1;
				}
				return result;
			},
			[weakOptionsForm](const auto& newValue) -> bool {
				if (!wzChangeDisplayScale(newValue))
				{
					debug(LOG_WARNING, "Failed to change display scale from: %d to: %d", war_GetDisplayScale(), newValue);
					return false;
				}
				war_SetDisplayScale(newValue);
				widgScheduleTask([weakOptionsForm]() {
					auto strongOptionsForm = weakOptionsForm.lock();
					if (strongOptionsForm)
					{
						// trigger layout recalc because of the display scaling change
						strongOptionsForm->refreshOptions(true);
					}
				});
				return true;
			}, true
		);
		result->addOption(optionInfo, valueChanger, true);
	}

	result->addSection(OptionsSection(N_("Game UI"), ""), true);
	{
		auto optionInfo = OptionInfo("interface.game.showFPS", N_("Show FPS"), "");
		auto valueChanger = OptionsDropdown<bool>::make(
			[]() {
				OptionChoices<bool> result;
				result.choices = {
					{ _("Off"), "", false },
					{ _("On"), "", true },
				};
				result.setCurrentIdxForValue(showFPS);
				return result;
			},
			[](const auto& newValue) -> bool {
				showFPS = newValue;
				return true;
			}, true
		);
		result->addOption(optionInfo, valueChanger, true);
	}
	{
		auto optionInfo = OptionInfo("interface.game.showCounts", N_("Show Unit Counts"), "");
		auto valueChanger = OptionsDropdown<bool>::make(
			[]() {
				OptionChoices<bool> result;
				result.choices = {
					{ _("Off"), "", false },
					{ _("On"), "", true },
				};
				result.setCurrentIdxForValue(showUNITCOUNT);
				return result;
			},
			[](const auto& newValue) -> bool {
				showUNITCOUNT = newValue;
				return true;
			}, true
		);
		result->addOption(optionInfo, valueChanger, true);
	}
	{
		auto optionInfo = OptionInfo("interface.game.groupsMenu", N_("Groups Menu"), "");
		auto valueChanger = OptionsDropdown<bool>::make(
			[]() {
				OptionChoices<bool> result;
				result.choices = {
					{ _("Off"), "", false },
					{ _("On"), "", true },
				};
				result.setCurrentIdxForValue(getGroupButtonEnabled());
				return result;
			},
			[](const auto& newValue) -> bool {
				setGroupButtonEnabled(newValue);
				war_setGroupsMenuEnabled(getGroupButtonEnabled()); // persist
				return true;
			}, true
		);
		result->addOption(optionInfo, valueChanger, true);
	}
	{
		auto optionInfo = OptionInfo("interface.game.optionsButton", N_("Options Button"), "");
		auto valueChanger = OptionsDropdown<uint8_t>::make(
			[]() {
				OptionChoices<uint8_t> result;
				result.choices = {
					{ _("Off"), "", 0 },
					{ _("Opacity: 50%"), "", 50 },
					{ _("On"), "", 100 },
				};
				auto currValue = war_getOptionsButtonVisibility();
				if (!result.setCurrentIdxForValue(currValue))
				{
					// add "Custom" item
					result.choices.push_back({ WzString::fromUtf8(astringf("(Custom: %u)", currValue)), "", currValue });
					result.currentIdx = result.choices.size() - 1;
				}
				return result;
			},
			[](const auto& newValue) -> bool {
				war_setOptionsButtonVisibility(newValue);
				intAddInGameOptionsButton(); // properly refresh button state
				return true;
			}, true
		);
		result->addOption(optionInfo, valueChanger, true);
	}

	result->addSection(OptionsSection(N_("Radar"), ""), true);
	{
		auto optionInfo = OptionInfo("interface.game.rotateRadar", N_("Radar Orientation"), "");
		auto valueChanger = OptionsDropdown<bool>::make(
			[]() {
				OptionChoices<bool> result;
				result.choices = {
					{ _("Fixed"), "", false },
					{ _("Rotating"), "", true },
				};
				result.setCurrentIdxForValue(rotateRadar);
				return result;
			},
			[](const auto& newValue) -> bool {
				rotateRadar = newValue;
				return true;
			}, true
		);
		result->addOption(optionInfo, valueChanger, true);
	}
	{
		auto optionInfo = OptionInfo("interface.game.radarJump", N_("Radar Jump"), "");
		auto valueChanger = OptionsDropdown<bool>::make(
			[]() {
				OptionChoices<bool> result;
				result.choices = {
					{ _("Instant"), "", true },
					{ _("Tracked"), "", false },
				};
				result.setCurrentIdxForValue(war_GetRadarJump());
				return result;
			},
			[](const auto& newValue) -> bool {
				war_SetRadarJump(newValue);
				return true;
			}, true
		);
		result->addOption(optionInfo, valueChanger, true);
	}
	{
		auto optionInfo = OptionInfo("interface.game.radarZoom", N_("Radar Zoom"), "");
		auto valueChanger = OptionsSlider::make(MIN_RADARZOOM, MAX_RADARZOOM, RADARZOOM_STEP,
			[]() { return war_GetRadarZoom(); },
			[](int32_t newValue) {
				war_SetRadarZoom(newValue);
			}, false
		);
		result->addOption(optionInfo, valueChanger, true);
	}

	result->addSection(OptionsSection(N_("Video Playback"), ""), true);
	{
		auto optionInfo = OptionInfo("interface.video.videoSize", N_("Video Size"), "");
		auto valueChanger = OptionsDropdown<FMV_MODE>::make(
			[]() {
				OptionChoices<FMV_MODE> result;
				result.choices = {
					{ _("1×"), "", FMV_1X },
					{ _("2×"), "", FMV_2X },
					{ _("Fullscreen"), "", FMV_FULLSCREEN },
				};
				result.setCurrentIdxForValue(war_GetFMVmode());
				return result;
			},
			[](const auto& newValue) -> bool {
				war_SetFMVmode(newValue);
				return true;
			}, true
		);
		result->addOption(optionInfo, valueChanger, true);
	}
	{
		auto optionInfo = OptionInfo("interface.video.scanlines", N_("Scanlines"), "");
		auto valueChanger = OptionsDropdown<SCANLINE_MODE>::make(
			[]() {
				OptionChoices<SCANLINE_MODE> result;
				result.choices = {
					{ _("Off"), "", SCANLINES_OFF },
					{ _("50%"), "", SCANLINES_50 },
					{ _("Black"), "", SCANLINES_BLACK },
				};
				result.setCurrentIdxForValue(war_getScanlineMode());
				return result;
			},
			[](const auto& newValue) -> bool {
				war_setScanlineMode(newValue);
				return true;
			}, true
		);
		result->addOption(optionInfo, valueChanger, true);
	}
	{
		auto optionInfo = OptionInfo("interface.video.subtitles", N_("Subtitles"), "");
		auto valueChanger = OptionsDropdown<bool>::make(
			[]() {
				OptionChoices<bool> result;
				result.choices = {
					{ _("Off"), "", false },
					{ _("On"), "", true },
				};
				result.setCurrentIdxForValue(seq_GetSubtitles());
				return result;
			},
			[](const auto& newValue) -> bool {
				seq_SetSubtitles(newValue);
				return true;
			}, true
		);
		result->addOption(optionInfo, valueChanger, true);
	}

	return result;
}
