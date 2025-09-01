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
 *  Defaults Options form
 */

#include "optionsforms.h"

#include "../../warzoneconfig.h"
#include "lib/ivis_opengl/bitimage.h"
#include "../../difficulty.h"
#include "../../multiint.h"
#include "../../multiplay.h"
#include "../../clparse.h"
#include "../../frend.h"
#include "../campaign.h"

#include <fmt/core.h>

// MARK: -

class PlayerColorOptionsSelector : public WIDGET, public OptionValueChangerInterface
{
protected:
	PlayerColorOptionsSelector() {}
public:
	typedef std::function<optional<unsigned>()> GetCurrentValueFunc;
	typedef std::function<bool (optional<unsigned> tc)> OnSelectColorFunc; // note: return true if able to select item, false if a failure
public:
	static std::shared_ptr<PlayerColorOptionsSelector> make(GetCurrentValueFunc getCurrVal, OnSelectColorFunc onSelect);

	void addColor(optional<unsigned> tc);

	void display(int xOffset, int yOffset) override;
	void geometryChanged() override;
	int32_t idealWidth() override;
	int32_t idealHeight() override;
	int32_t heightRequiredForWidth(int32_t w);

	void update(bool force) override;
	void informAvailable(bool isAvailable) override;
	void addOnChangeHandler(std::function<void(WIDGET&)> handler) override;

private:
	void updateSelectedItem(optional<unsigned> tc);
	int32_t rowsRequiredForInnerWidth(int32_t w);
	int32_t innerHeightRequiredForNumRows(int32_t rows);
	void handleColorPressed(optional<unsigned> tc);
	unsigned optTcToMultiButTc(optional<unsigned> tc);

private:
	GetCurrentValueFunc getCurrentValueFunc;
	OnSelectColorFunc onSelectFunc;
	std::vector<std::function<void(WIDGET&)>> onChangeHandlers;
	std::vector<std::shared_ptr<WzMultiButton>> colorButtons;
	int32_t buttonWidth = 0;
	int32_t buttonHeight = 0;
	int32_t buttonMargin = 5;
	const int32_t outerPaddingX = 8;
	const int32_t outerPaddingY = 5;
};

std::shared_ptr<PlayerColorOptionsSelector> PlayerColorOptionsSelector::make(GetCurrentValueFunc getCurrVal, OnSelectColorFunc onSelect)
{
	class make_shared_enabler : public PlayerColorOptionsSelector { };
	auto result = std::make_shared<make_shared_enabler>();
	result->getCurrentValueFunc = getCurrVal;
	result->onSelectFunc = onSelect;
	result->buttonWidth = iV_GetImageWidth(FrontImages, IMAGE_PLAYERN);
	result->buttonHeight = iV_GetImageHeight(FrontImages, IMAGE_PLAYERN);
	return result;
}

unsigned PlayerColorOptionsSelector::optTcToMultiButTc(optional<unsigned> tc)
{
	return tc.value_or(MAX_PLAYERS + 1);
}

void PlayerColorOptionsSelector::addColor(optional<unsigned> tc)
{
	auto button = makeMultiBut(0, buttonWidth, buttonHeight, nullptr, IMAGE_PLAYERN, IMAGE_PLAYERX, true, optTcToMultiButTc(tc));
	attach(button);
	auto weakSelf = std::weak_ptr<PlayerColorOptionsSelector>(std::dynamic_pointer_cast<PlayerColorOptionsSelector>(shared_from_this()));
	button->addOnClickHandler([weakSelf, tc](W_BUTTON&) {
		auto strongSelf = weakSelf.lock();
		ASSERT_OR_RETURN(, strongSelf != nullptr, "Parent gone?");
		strongSelf->handleColorPressed(tc);
	});
	colorButtons.push_back(button);
	updateSelectedItem(getCurrentValueFunc());
}

void PlayerColorOptionsSelector::display(int xOffset, int yOffset)
{
	// no-op
}

void PlayerColorOptionsSelector::geometryChanged()
{
	int w = width();
	int h = height();

	if (w == 0 || h == 0)
	{
		return;
	}

	int availableWidth = w - (outerPaddingX * 2);
	int availableHeight = h - (outerPaddingY * 2);

	int rowsRequired = rowsRequiredForInnerWidth(w);
	int neededHeight = innerHeightRequiredForNumRows(rowsRequired);

	int buttonX0 = 0; // TODO: Right-align everything?
	int buttonY0 = std::max<int>(0, (availableHeight - neededHeight) / 2);
	for (const auto& b : colorButtons)
	{
		b->setGeometry(outerPaddingX + buttonX0, outerPaddingY + buttonY0, buttonWidth, buttonHeight);
		buttonX0 += buttonWidth + buttonMargin;
		if (buttonX0 + buttonWidth > availableWidth)
		{
			buttonY0 += buttonHeight + buttonMargin;
			buttonX0 = 0;
		}
	}
}

int32_t PlayerColorOptionsSelector::idealWidth()
{
	return (outerPaddingX * 2) + 157;
}

int32_t PlayerColorOptionsSelector::idealHeight()
{
	return (outerPaddingY * 2) + heightRequiredForWidth(157);
}

int32_t PlayerColorOptionsSelector::rowsRequiredForInnerWidth(int32_t w)
{
	int32_t totalRequiredWidthForButtons = 0;
	if (colorButtons.size())
	{
		totalRequiredWidthForButtons = (buttonWidth * colorButtons.size()) + (buttonMargin * (colorButtons.size() - 1));
	}
	return ((totalRequiredWidthForButtons - 1) / w) + 1;
}

int32_t PlayerColorOptionsSelector::innerHeightRequiredForNumRows(int32_t rows)
{
	int32_t result = (rows * buttonHeight);
	if (rows > 1)
	{
		result += (rows - 1) * buttonMargin;
	}
	return result;
}

int32_t PlayerColorOptionsSelector::heightRequiredForWidth(int32_t w)
{
	return innerHeightRequiredForNumRows(rowsRequiredForInnerWidth(w));
}

void PlayerColorOptionsSelector::updateSelectedItem(optional<unsigned> tc)
{
	for (const auto& b : colorButtons)
	{
		b->setState((b->tc == optTcToMultiButTc(tc)) ? WBUT_LOCK : 0);
	}
}

void PlayerColorOptionsSelector::handleColorPressed(optional<unsigned> tc)
{
	if (tc == getCurrentValueFunc())
	{
		return;
	}

	if (!onSelectFunc(tc))
	{
		return;
	}
	updateSelectedItem(tc);

	for (const auto& onChangeFunc : onChangeHandlers)
	{
		onChangeFunc(*this);
	}
}

void PlayerColorOptionsSelector::update(bool force)
{
	updateSelectedItem(getCurrentValueFunc());
}

void PlayerColorOptionsSelector::informAvailable(bool isAvailable)
{
	// future todo: could disable the buttons
}

void PlayerColorOptionsSelector::addOnChangeHandler(std::function<void(WIDGET&)> handler)
{
	onChangeHandlers.push_back(handler);
}

// MARK: -

std::shared_ptr<OptionsForm> makeDefaultsOptionsForm()
{
	auto result = OptionsForm::make();

	// Campaign:
	result->addSection(OptionsSection(N_("Campaign"), ""), true);
	{
		auto optionInfo = OptionInfo("defaults.campaign.unitColor", N_("Unit Color"), "");
		// Valid colors for SP are 0, 4-7. (Some are reserved for AI players.)
		std::vector<unsigned> validSPColors = {0, 4, 5, 6, 7};
		auto valueChanger = PlayerColorOptionsSelector::make(
			[validSPColors]() -> optional<unsigned> {
				auto currValue = war_GetSPcolor();
				if (std::find(validSPColors.begin(), validSPColors.end(), currValue) == validSPColors.end())
				{
					currValue = 0;
				}
				return currValue;
			},
			[](optional<unsigned> tc) -> bool {
				if (!tc.has_value()) { return false; }
				war_SetSPcolor(tc.value());
				return true;
			}
		);
		for (auto tc : validSPColors)
		{
			valueChanger->addColor(tc);
		}
		result->addOption(optionInfo, valueChanger, true);
	}
	{
		auto optionInfo = OptionInfo("defaults.campaign.difficultyLevel", N_("Difficulty Level"), "");
		auto valueChanger = OptionsDropdown<DIFFICULTY_LEVEL>::make(
			[]() {
				OptionChoices<DIFFICULTY_LEVEL> result;
				std::vector<DIFFICULTY_LEVEL> availableDifficultyLevels = { DL_SUPER_EASY, DL_EASY, DL_NORMAL, DL_HARD, DL_INSANE };
				for (const auto& level : availableDifficultyLevels)
				{
					result.choices.push_back({ difficultyLevelToString(level), getCampaignDifficultyDescriptionString(level), level });
				}
				result.setCurrentIdxForValue(getDifficultyLevel());
				return result;
			},
			[](const auto& newValue) -> bool {
				setDifficultyLevel(newValue);
				return true;
			}, true
		);
		result->addOption(optionInfo, valueChanger, true);
	}

	// Multiplayer:
	result->addSection(OptionsSection(N_("Multiplayer"), ""), true);
	{
		auto optionInfo = OptionInfo("defaults.multiplayer.unitColor", N_("Unit Color"), "");
		auto valueChanger = PlayerColorOptionsSelector::make(
			[]() -> optional<unsigned> {
				auto currValue = war_getMPcolour();
				if (currValue < 0)
				{
					return nullopt;
				}
				return static_cast<unsigned>(currValue);
			},
			[](optional<unsigned> tc) -> bool {
				war_setMPcolour(tc.value_or(-1));
				return true;
			}
		);
		valueChanger->addColor(nullopt);
		for (unsigned tc = 0; tc < MAX_PLAYERS_IN_GUI; ++tc)
		{
			valueChanger->addColor(tc);
		}
		result->addOption(optionInfo, valueChanger, true);
	}
	{
		auto optionInfo = OptionInfo("defaults.multiplayer.defaultAI", N_("Default AI"), "");
		optionInfo.setRequiresRestart(true);
		auto valueChanger = OptionsDropdown<WzString>::make(
			[]() {
				OptionChoices<WzString> result;
				const auto& aidata = getAIData();
				for (size_t i = 0; i < aidata.size(); ++i)
				{
					result.choices.push_back({ aidata[i].name, "", aidata[i].js });
				}
				result.setCurrentIdxForValue(WzString::fromUtf8(getDefaultSkirmishAI()));
				return result;
			},
			[](const auto& newValue) -> bool {
				setDefaultSkirmishAI(newValue.toUtf8());
				return true;
			}, true
		);
		result->addOption(optionInfo, valueChanger, true);
	}

	// Hosting Options:
	result->addSection(OptionsSection(N_("Hosting Options"), ""), true);
	{
		auto optionInfo = OptionInfo("defaults.hosting.portMapping", N_("Port Mapping"), N_("Use PCP, NAT-PMP, or UPnP to help configure your router / firewall to allow connections while hosting."));
		auto valueChanger = OptionsDropdown<bool>::make(
			[]() {
				OptionChoices<bool> result;
				result.choices = {
					{ _("Off"), "", false },
					{ _("On"), "", true },
				};
				result.setCurrentIdxForValue(NetPlay.isPortMappingEnabled);
				return result;
			},
			[](const auto& newValue) -> bool {
				NetPlay.isPortMappingEnabled = newValue;
				return true;
			}, false
		);
		result->addOption(optionInfo, valueChanger, true);
	}
	{
		auto optionInfo = OptionInfo("defaults.hosting.chat", N_("Chat"), "");
		auto valueChanger = OptionsDropdown<bool>::make(
			[]() {
				OptionChoices<bool> result;
				result.choices = {
					{ _("Quick Chat Only"), "", false },
					{ _("Allow All"), "", true },
				};
				result.setCurrentIdxForValue(NETgetDefaultMPHostFreeChatPreference());
				return result;
			},
			[](const auto& newValue) -> bool {
				NETsetDefaultMPHostFreeChatPreference(newValue);
				return true;
			}, false
		);
		result->addOption(optionInfo, valueChanger, true);
	}
	{
		auto optionInfo = OptionInfo("defaults.hosting.inactivityTimeout", N_("Inactivity Timeout"), "");
		optionInfo.addAvailabilityCondition(IsNotInGame);
		auto valueChanger = OptionsDropdown<uint32_t>::make(
			[]() {
				OptionChoices<uint32_t> result;
				result.choices = {
					{ _("Off"), "", 0 }
				};
				for (uint32_t inactivityMinutes = MIN_MPINACTIVITY_MINUTES; inactivityMinutes <= (MIN_MPINACTIVITY_MINUTES + 6); inactivityMinutes++)
				{
					result.choices.push_back({ WzString::fromUtf8(astringf(_("%u minutes"), inactivityMinutes)), "", inactivityMinutes });
				}
				auto currValue = war_getMPInactivityMinutes();
				if (!result.setCurrentIdxForValue(currValue))
				{
					// add "Custom" item
					result.choices.push_back({ WzString::fromUtf8(astringf(_("%u minutes"), currValue)), "", currValue });
					result.currentIdx = result.choices.size() - 1;
				}
				return result;
			},
			[](const auto& newValue) -> bool {
				war_setMPInactivityMinutes(newValue);
				game.inactivityMinutes = war_getMPInactivityMinutes();
				return true;
			}, false
		);
		result->addOption(optionInfo, valueChanger, true);
	}
	{
		auto optionInfo = OptionInfo("defaults.hosting.lagKick", N_("Lag Kick"), "");
		auto valueChanger = OptionsDropdown<int>::make(
			[]() {
				OptionChoices<int> result;
				result.choices = {
					{ _("Off"), "", 0 }
				};
				for (int lagKickSeconds = 60; lagKickSeconds <= 120; lagKickSeconds += 30)
				{
					result.choices.push_back({ WzString::fromUtf8(astringf(_("%u seconds"), lagKickSeconds)), "", lagKickSeconds });
				}
				auto currValue = war_getAutoLagKickSeconds();
				if (!result.setCurrentIdxForValue(currValue))
				{
					// add "Custom" item
					result.choices.push_back({ WzString::fromUtf8(astringf(_("%u seconds"), currValue)), "", currValue });
					result.currentIdx = result.choices.size() - 1;
				}
				return result;
			},
			[](const auto& newValue) -> bool {
				war_setAutoLagKickSeconds(newValue);
				return true;
			}, false
		);
		result->addOption(optionInfo, valueChanger, true);
	}
	{
		auto optionInfo = OptionInfo("defaults.hosting.spectatorSlots", N_("Spectator Slots"), "");
		optionInfo.addAvailabilityCondition(IsNotInGame);
		auto valueChanger = OptionsDropdown<uint32_t>::make(
			[]() {
				OptionChoices<uint32_t> result;
				result.choices = {
					{ _("None"), "", 0 }
				};
				for (uint32_t openSpectatorSlots = 1; openSpectatorSlots <= MAX_SPECTATOR_SLOTS; openSpectatorSlots++)
				{
					result.choices.push_back({ WzString::number(openSpectatorSlots), "", openSpectatorSlots });
				}
				result.setCurrentIdxForValue(war_getMPopenSpectatorSlots());
				return result;
			},
			[](const auto& newValue) -> bool {
				war_setMPopenSpectatorSlots(newValue);
				return true;
			}, false
		);
		result->addOption(optionInfo, valueChanger, true);
	}
	{
		auto optionInfo = OptionInfo("defaults.hosting.onPlayerLeave", N_("On Player Leave"), "");
		optionInfo.addAvailabilityCondition(IsNotInGame);
		auto valueChanger = OptionsDropdown<PLAYER_LEAVE_MODE>::make(
			[]() {
				OptionChoices<PLAYER_LEAVE_MODE> result;
				result.choices = {
					{ _("Distribute to Team"), "", PLAYER_LEAVE_MODE::SPLIT_WITH_TEAM },
					{ _("Destroy (Classic)"), "", PLAYER_LEAVE_MODE::DESTROY_RESOURCES },
				};
				result.setCurrentIdxForValue(war_getMPPlayerLeaveMode());
				return result;
			},
			[](const auto& newValue) -> bool {
				war_setMPPlayerLeaveMode(newValue);
				return true;
			}, false
		);
		result->addOption(optionInfo, valueChanger, true);
	}
	{
		auto optionInfo = OptionInfo("defaults.hosting.gameTimeLimit", N_("Game Time Limit"), "");
		optionInfo.addAvailabilityCondition(IsNotInGame);
		auto valueChanger = OptionsDropdown<uint32_t>::make(
			[]() {
				OptionChoices<uint32_t> result;
				result.choices = {
					{ _("Off"), "", 0 }
				};
				auto addGameTimeLimitMinutesOption = [&](uint32_t gameTimeLimitMinutes) {
					std::string buttonText;
					if (gameTimeLimitMinutes >= 120)
					{
						std::string hoursFloatStr = fmt::format("{:#}", static_cast<float>(gameTimeLimitMinutes) / 60.f);
						buttonText = astringf(_("%s hours"), hoursFloatStr.c_str());
					}
					else
					{
						buttonText = astringf(_("%u minutes"), gameTimeLimitMinutes);
					}
					result.choices.push_back({ WzString::fromUtf8(buttonText), "", gameTimeLimitMinutes });
				};
				for (uint32_t gameTimeLimitMinutes = MIN_MPGAMETIMELIMIT_MINUTES; gameTimeLimitMinutes <= (MIN_MPGAMETIMELIMIT_MINUTES + (15 * 10)); gameTimeLimitMinutes += 15)
				{
					addGameTimeLimitMinutesOption(gameTimeLimitMinutes);
				}
				auto currValue = war_getMPGameTimeLimitMinutes();
				if (!result.setCurrentIdxForValue(currValue))
				{
					// add custom value
					addGameTimeLimitMinutesOption(currValue);
					result.currentIdx = result.choices.size() - 1;
				}
				return result;
			},
			[](const auto& newValue) -> bool {
				war_setMPGameTimeLimitMinutes(newValue);
				game.gameTimeLimitMinutes = war_getMPGameTimeLimitMinutes();
				return true;
			}, false
		);
		result->addOption(optionInfo, valueChanger, true);
	}

	return result;
}
