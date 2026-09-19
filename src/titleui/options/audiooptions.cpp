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
 *  Audio Options form
 */

#include "optionsforms.h"

#include "../../warzoneconfig.h"
#include "lib/sound/mixer.h"
#include "lib/sound/tracklib.h"
#include "../../seqdisp.h"
#include "../../main.h"

#include "lib/ivis_opengl/pieblitfunc.h"
#include "../musicmanagertitleui.h"
#include "../../musicmanager.h"
#include "../../screens/ingameopscreen.h"

// MARK: -

OptionInfo::AvailabilityResult HRTFSupported(const OptionInfo&)
{
	OptionInfo::AvailabilityResult result;
	result.available = (sound_GetHRTFMode() != HRTFMode::Unsupported);
	result.localizedUnavailabilityReason = _("HRTF is not supported on your device / system / OpenAL library");
	return result;
}

static void doOpenMusicManager(const std::shared_ptr<W_SCREEN>& currentScreen)
{
	switch (GetGameMode())
	{
	case GS_NORMAL:
	{
		showInGameOptionsScreen(currentScreen, makeMusicManagerForm(true), []() {
			// no-op on close function
		});
		break;
	}
	case GS_TITLE_SCREEN:
		changeTitleUI(std::make_shared<WzMusicManagerTitleUI>(wzTitleUICurrent));
		break;
	default:
		break;
	}
}

// MARK: -

std::shared_ptr<OptionsForm> makeAudioOptionsForm()
{
	auto result = OptionsForm::make();

	// Volume:
	result->addSection(OptionsSection(N_("Volume"), ""), true);
	{
		auto optionInfo = OptionInfo("audio.voiceVol", N_("Voice Volume"), "");
		auto valueChanger = OptionsSlider::make(0, AUDIO_VOL_MAX, 1,
			[]() { return static_cast<int32_t>(sound_GetUIVolume() * 100.0f); },
			[](int32_t newValue) {
				sound_SetUIVolume((float)newValue / 100.0f);
			}, false
		);
		result->addOption(optionInfo, valueChanger, true);
	}
	{
		auto optionInfo = OptionInfo("audio.fxVol", N_("FX Volume"), "");
		auto valueChanger = OptionsSlider::make(0, AUDIO_VOL_MAX, 1,
			[]() { return static_cast<int32_t>(sound_GetEffectsVolume() * 100.0f); },
			[](int32_t newValue) {
				sound_SetEffectsVolume((float)newValue / 100.0f);
			}, false
		);
		result->addOption(optionInfo, valueChanger, true);
	}
	{
		auto optionInfo = OptionInfo("audio.musicVol", N_("Music Volume"), "");
		auto valueChanger = OptionsSlider::make(0, AUDIO_VOL_MAX, 1,
			[]() { return static_cast<int32_t>(sound_GetMusicVolume() * 100.0f); },
			[](int32_t newValue) {
				sound_SetMusicVolume((float)newValue / 100.0f);
			}, false
		);
		result->addOption(optionInfo, valueChanger, true);
	}

	// Output:
	result->addSection(OptionsSection(N_("Output"), ""), true);
	{
		auto optionInfo = OptionInfo("audio.hrtf", N_("HRTF"), N_("HRTF can enable enhanced spatialization when used with headphones or some stereo output."));
		optionInfo.addAvailabilityCondition(HRTFSupported);
		auto valueChanger = OptionsDropdown<HRTFMode>::make(
			[]() {
				OptionChoices<HRTFMode> result;
				// note: sound_GetHRTFMode() returns the current actual status (and never auto, even if the request was "auto")
				auto currentMode = sound_GetHRTFMode();
				if (currentMode == HRTFMode::Unsupported)
				{
					result.choices = { { _("Unsupported"), "", HRTFMode::Unsupported } };
					result.currentIdx = 0;
					return result;
				}
				result.choices = {
					{ _("Disabled"), "", HRTFMode::Disabled },
					{ _("Enabled"), "", HRTFMode::Enabled }
				};
				bool isAutoSetting = war_GetHRTFMode() == HRTFMode::Auto;
				if (isAutoSetting)
				{
					WzString currentModeStr;
					switch (currentMode)
					{
					case HRTFMode::Disabled:
						currentModeStr = _("Disabled");
						break;
					case HRTFMode::Enabled:
						currentModeStr = _("Enabled");
						break;
					default:
						break;
					}
					result.choices.push_back({ WzString(_("Auto")) + " (" + currentModeStr + ")", "", HRTFMode::Auto });
				}
				else
				{
					result.choices.push_back({ _("Auto"), "", HRTFMode::Auto });
				}
				result.setCurrentIdxForValue((isAutoSetting) ? HRTFMode::Auto : currentMode);
				return result;
			},
			[](const auto& newValue) -> bool {
				if (!sound_SetHRTFMode(newValue))
				{
					return false;
				}
				war_SetHRTFMode(newValue);
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

	// Music:
	result->addSection(OptionsSection(N_("Music"), ""), true);
	{
		auto optionInfo = OptionInfo("audio.music.manager", N_("Music Manager"), "");
		auto valueChanger = OptionsButton::make(
			[](OptionsButton& but) {
				but.setString(_("Open Music Manager"));
			}, false
		);
		valueChanger->addOnClickHandler([](W_BUTTON& but) {
			doOpenMusicManager(but.screenPointer.lock());
		});
		result->addOption(optionInfo, valueChanger, true);
	}

	// Audio Cues:
	result->addSection(OptionsSection(N_("Audio Cues"), ""), true);
	{
		auto optionInfo = OptionInfo("audio.cues.groupReporting", N_("Group Reporting"), "");
		auto valueChanger = OptionsDropdown<bool>::make(
			[]() {
				OptionChoices<bool> result;
				result.choices = {
					{ _("Off"), "", false },
					{ _("On"), "", true },
				};
				result.setCurrentIdxForValue(war_getPlayAudioCue_GroupReporting());
				return result;
			},
			[](const auto& newValue) -> bool {
				war_setPlayAudioCue_GroupReporting(newValue);
				return true;
			}, true
		);
		result->addOption(optionInfo, valueChanger, true);
	}

	return result;
}
