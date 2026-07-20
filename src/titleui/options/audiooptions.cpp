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

class OptionsButton : public W_BUTTON, public OptionValueChangerInterface
{
protected:
	OptionsButton();
public:
	typedef std::function<void(OptionsButton&)> UpdateButtonFunc;
public:
	static std::shared_ptr<OptionsButton> make(UpdateButtonFunc updateButtonFunc, bool needsStateUpdates);

	void setString(WzString string) override;
	void setFont(iV_fonts font);

	void display(int xOffset, int yOffset) override;
	void geometryChanged() override;
	int32_t idealWidth() override;
	int32_t idealHeight() override;

	void update(bool force) override;
	void informAvailable(bool isAvailable) override;
	void addOnChangeHandler(std::function<void(WIDGET&)> handler) override;

private:
	void handleSliderPosChanged();
	void updateSliderValueText();
	void cacheIdealTextSize();
	size_t maxNumValueCharacters();

private:
	WzCachedText wzText;
	iV_fonts FontID = font_regular;
	PIELIGHT bindingColor = WZCOL_FORM_TEXT;
	PIELIGHT highlightBackgroundColor;
	int32_t cachedIdealTextWidth = 0;
	int32_t lastWidgetWidth = 0;
	int verticalPadding = 10;
	int horizontalPadding = 14;
	bool isTruncated = false;
	UpdateButtonFunc updateButtonFunc;
	bool needsStateUpdates;
};

OptionsButton::OptionsButton()
{
	highlightBackgroundColor = WZCOL_MENU_SCORE_BUILT;
}

std::shared_ptr<OptionsButton> OptionsButton::make(UpdateButtonFunc updateButtonFunc, bool needsStateUpdates)
{
	ASSERT_OR_RETURN(nullptr, updateButtonFunc != nullptr, "Missing required updateButtonFunc");

	class make_shared_enabler : public OptionsButton { };
	auto result = std::make_shared<make_shared_enabler>();
	result->updateButtonFunc = updateButtonFunc;
	result->needsStateUpdates = needsStateUpdates;

	updateButtonFunc(*result);

	return result;
}

void OptionsButton::setString(WzString string)
{
	W_BUTTON::setString(string);
	cachedIdealTextWidth = iV_GetTextWidth(pText, FontID);
}

void OptionsButton::setFont(iV_fonts font)
{
	if (font == FontID) { return; }
	FontID = font;
	cachedIdealTextWidth = iV_GetTextWidth(pText, FontID);
}

void OptionsButton::display(int xOffset, int yOffset)
{
	int x0 = x() + xOffset;
	int y0 = y() + yOffset;
	int w = width();
	int h = height();

	bool isDown = (getState() & (WBUT_DOWN | WBUT_LOCK | WBUT_CLICKLOCK)) != 0;
	bool isDisabled = (getState() & WBUT_DISABLE) != 0;
	bool isHighlight = (getState() & WBUT_HIGHLIGHT) != 0;

	iV_fonts fontID = wzText.getFontID();
	if (fontID == font_count || lastWidgetWidth != width() || wzText.getText() != pText)
	{
		fontID = FontID;
		wzText.setText(pText, fontID);
	}

	const int maxTextDisplayableWidth  = w - (horizontalPadding * 2);
	if (wzText->width() > maxTextDisplayableWidth)
	{
		// text would exceed the bounds of the button area
		// try to shrink font so it fits
		do {
			if (fontID == font_small || fontID == font_bar)
			{
				break;
			}
			auto result = iV_ShrinkFont(fontID);
			if (!result.has_value())
			{
				break;
			}
			fontID = result.value();
			wzText.setText(pText, fontID);
		} while (wzText->width() > maxTextDisplayableWidth);
	}
	lastWidgetWidth = width();

	// Draw background
	int backX0 = x0;
	int backY0 = y0;
	int backX1 = x0 + w - 1;
	int backY1 = y0 + h - 1;
	const bool drawHighlightBorder = isHighlight;
	if (drawHighlightBorder)
	{
		pie_UniTransBoxFill(backX0, backY0, backX1, backY1, pal_RGBA(255,255,255,225));
	}
	backX0 += 1;
	backY0 += 1;
	backX1 -= 1;
	backY1 -= 1;
	PIELIGHT backgroundColor;
	if (isDown)
	{
		backgroundColor = WZCOL_FORM_DARK;
	}
	else if (isHighlight && !highlightBackgroundColor.isTransparent())
	{
		backgroundColor = highlightBackgroundColor;
	}
	else
	{
		backgroundColor = WZCOL_FORM_DARK;
		backgroundColor.byte.a = static_cast<uint8_t>(backgroundColor.byte.a / 3);
	}
	pie_UniTransBoxFill(backX0, backY0, backX1, backY1, backgroundColor);

	// Draw the main text
	PIELIGHT textColor = (!isDisabled) ? WZCOL_TEXT_BRIGHT : WZCOL_TEXT_MEDIUM;
	if (isDisabled)
	{
		textColor.byte.a = (textColor.byte.a / 2);
	}

	int maxDisplayableMainTextWidth = maxTextDisplayableWidth;
	isTruncated = maxDisplayableMainTextWidth < wzText->width();
	if (isTruncated)
	{
		maxDisplayableMainTextWidth -= (iV_GetEllipsisWidth(wzText.getFontID()) + 2);
	}
	int textX0 = x0 + (w - std::min(maxDisplayableMainTextWidth, wzText->width())) / 2;
	int textY0 = y0 + (h - wzText->lineSize()) / 2 - wzText->aboveBase();
	wzText->render(textX0, textY0, textColor, 0.0f, maxDisplayableMainTextWidth);
	if (isTruncated)
	{
		// Render ellipsis
		iV_DrawEllipsis(wzText.getFontID(), Vector2f(textX0 + maxDisplayableMainTextWidth + 2, textY0), textColor);
	}
}

void OptionsButton::geometryChanged()
{
	// currently no-op
}

int32_t OptionsButton::idealWidth()
{
	return (horizontalPadding * 2) + cachedIdealTextWidth;
}

int32_t OptionsButton::idealHeight()
{
	return (verticalPadding * 2) + iV_GetTextLineSize(FontID);
}

void OptionsButton::update(bool force)
{
	if (!force && !needsStateUpdates)
	{
		return;
	}

	updateButtonFunc(*this);
}

void OptionsButton::informAvailable(bool isAvailable)
{
	setState((!isAvailable) ? WBUT_DISABLE : 0);
}

void OptionsButton::addOnChangeHandler(std::function<void(WIDGET&)> handler)
{
	// no-op for buttons - expectation is that an onclick-handler will be added
}

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
