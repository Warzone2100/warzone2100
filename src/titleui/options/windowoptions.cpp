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
 *  Window Options form
 */

#include "optionsforms.h"

#include "../../warzoneconfig.h"
#include "lib/framework/wzapp.h"

// MARK: -

OptionInfo::AvailabilityResult IsInFullscreenMode(const OptionInfo&)
{
	OptionInfo::AvailabilityResult result;
	result.available = (war_getWindowMode() == WINDOW_MODE::fullscreen);
	switch (war_getWindowMode())
	{
		case WINDOW_MODE::desktop_fullscreen:
			result.localizedUnavailabilityReason = _("In Desktop Fullscreen mode, the resolution matches that of your desktop \n(or what the window manager allows).");
			break;
		case WINDOW_MODE::windowed:
			result.localizedUnavailabilityReason = _("You can change the resolution by resizing the window normally. (Try dragging a corner / edge.)");
			break;
		default:
			break;
	}
	return result;
}

// MARK: -

class ScreenResolutionsModel
{
public:
	typedef const std::vector<screeninfo>::iterator iterator;
	typedef const std::vector<screeninfo>::const_iterator const_iterator;

	ScreenResolutionsModel(): modes(ScreenResolutionsModel::loadModes())
	{
	}

	const_iterator begin() const
	{
		return modes.begin();
	}

	const_iterator end() const
	{
		return modes.end();
	}

	const_iterator findResolutionClosestToCurrent() const
	{
		auto currentResolution = getCurrentResolution();
		auto closest = std::lower_bound(modes.begin(), modes.end(), currentResolution, compareLess);
		if (closest != modes.end() && compareEq(*closest, currentResolution))
		{
			return closest;
		}

		if (closest != modes.begin())
		{
			--closest;  // If current resolution doesn't exist, round down to next-highest one.
		}

		return closest;
	}

	bool selectAt(size_t index) const
	{
		ASSERT_OR_RETURN(false, wzGetCurrentWindowMode() == WINDOW_MODE::fullscreen, "Attempt to change resolution in windowed-mode / desktop-fullscreen mode");

		ASSERT_OR_RETURN(false, index < modes.size(), "Invalid mode index passed to ScreenResolutionsModel::selectAt");

		auto selectedResolution = modes.at(index);

		auto currentResolution = getCurrentResolution();
		// Attempt to change the resolution
		if (!wzChangeFullscreenDisplayMode(selectedResolution.screen, selectedResolution.width, selectedResolution.height))
		{
			debug(
				LOG_WARNING,
				"Failed to change active resolution from: %s to: %s",
				ScreenResolutionsModel::resolutionString(currentResolution).toUtf8().c_str(),
				ScreenResolutionsModel::resolutionString(selectedResolution).toUtf8().c_str()
			);
			return false;
		}

		// Store the new width and height
		war_SetFullscreenModeScreen(selectedResolution.screen);
		war_SetFullscreenModeWidth(selectedResolution.width);
		war_SetFullscreenModeHeight(selectedResolution.height);

		return true;
	}

	static WzString currentResolutionString()
	{
		return ScreenResolutionsModel::resolutionString(getCurrentResolution());
	}

	static WzString resolutionString(const screeninfo &info)
	{
		return WzString::fromUtf8(astringf("[%d] %d × %d", info.screen, info.width, info.height));
	}

private:
	const std::vector<screeninfo> modes;

	static screeninfo getCurrentWindowedResolution()
	{
		int screen = 0;
		unsigned int windowWidth = 0, windowHeight = 0;
		wzGetWindowResolution(&screen, &windowWidth, &windowHeight);
		screeninfo info;
		info.screen = screen;
		info.width = windowWidth;
		info.height = windowHeight;
		info.refresh_rate = -1;  // Unused.
		return info;
	}

	static screeninfo getCurrentResolution(optional<WINDOW_MODE> modeOverride = nullopt)
	{
		return (modeOverride.value_or(wzGetCurrentWindowMode()) == WINDOW_MODE::fullscreen) ? wzGetCurrentFullscreenDisplayMode() : getCurrentWindowedResolution();
	}

	static std::vector<screeninfo> loadModes()
	{
		// Get resolutions, sorted with duplicates removed.
		std::vector<screeninfo> modes = wzAvailableResolutions();
		std::sort(modes.begin(), modes.end(), ScreenResolutionsModel::compareLess);
		modes.erase(std::unique(modes.begin(), modes.end(), ScreenResolutionsModel::compareEq), modes.end());

		return modes;
	}

	static std::tuple<int, int, int> compareKey(const screeninfo &x)
	{
		return std::make_tuple(x.screen, x.width, x.height);
	}

	static bool compareLess(const screeninfo &a, const screeninfo &b)
	{
		return ScreenResolutionsModel::compareKey(a) < ScreenResolutionsModel::compareKey(b);
	}

	static bool compareEq(const screeninfo &a, const screeninfo &b)
	{
		return ScreenResolutionsModel::compareKey(a) == ScreenResolutionsModel::compareKey(b);
	}
};

// MARK: -

std::shared_ptr<OptionsForm> makeWindowOptionsForm()
{
	auto result = OptionsForm::make();
	result->setRefreshOptionsOnScreenSizeChange(true);

	result->addSection(OptionsSection(N_("Window"), ""), true);
	{
		auto optionInfo = OptionInfo("window.mode", N_("Window Mode"), "");
		auto valueChanger = OptionsDropdown<WINDOW_MODE>::make(
			[]() {
				OptionChoices<WINDOW_MODE> result;
				result.choices = {
					{_("Windowed"), "", WINDOW_MODE::windowed},
					{_("Desktop Full"), "", WINDOW_MODE::desktop_fullscreen},
					{_("Fullscreen"), "", WINDOW_MODE::fullscreen},
				};
				result.choices.erase(std::remove_if(result.choices.begin(), result.choices.end(), [](const auto& item) -> bool {
					return !wzIsSupportedWindowMode(item.value);
				}), result.choices.end());
				result.setCurrentIdxForValue(war_getWindowMode());
				return result;
			},
			[](const auto& newMode) -> bool {
				if (newMode == wzGetCurrentWindowMode())
				{
					return true;
				}
				if (!wzChangeWindowMode(newMode))
				{
					// unable to change to this fullscreen mode, so disable the widget
					debug(LOG_ERROR, "Failed to set fullscreen mode: %s", to_display_string(newMode).c_str());
					return false;
				}
				war_setWindowMode(newMode); // persist config change
				return true;
			}, true
		);
		result->addOption(optionInfo, valueChanger, true);
	}
	auto screenResolutionsModel = std::make_shared<ScreenResolutionsModel>();
	{
		auto optionInfo = OptionInfo("window.resolution", N_("Resolution"), "");
		optionInfo.addAvailabilityCondition(IsInFullscreenMode);
		auto valueChanger = OptionsDropdown<size_t>::make(
			[screenResolutionsModel]() {
				OptionChoices<size_t> result;
				if (war_getWindowMode() != WINDOW_MODE::fullscreen)
				{
					// for now, only allow editing the fullscreen mode when in fullscreen mode
					// just display the current resolution
					result.choices.push_back({ScreenResolutionsModel::currentResolutionString(), "", 0});
					return result;
				}
				size_t idx = 0;
				for (auto resolution: *screenResolutionsModel)
				{
					result.choices.push_back({ScreenResolutionsModel::resolutionString(resolution), "", idx});
					++idx;
				}
				auto closestResolution = screenResolutionsModel->findResolutionClosestToCurrent();
				if (closestResolution != screenResolutionsModel->end())
				{
					result.currentIdx = (closestResolution - screenResolutionsModel->begin());
				}
				return result;
			},
			[screenResolutionsModel](const auto& newResolutionIdx) -> bool {
				if (war_getWindowMode() != WINDOW_MODE::fullscreen)
				{
					return true;
				}
				if (!screenResolutionsModel->selectAt(newResolutionIdx))
				{
					return false;
				}
				return true;
			}, true
		);
		result->addOption(optionInfo, valueChanger, true);
	}
#if !defined(__EMSCRIPTEN__)
	result->addSection(OptionsSection(N_("Behaviors"), ""), true);
	{
		auto optionInfo = OptionInfo("window.minimizeOnFocusLoss", N_("Minimize on Focus Loss"), N_("Whether the window should auto-minimize on focus loss"));
		auto valueChanger = OptionsDropdown<MinimizeOnFocusLossBehavior>::make(
			[]() {
				OptionChoices<MinimizeOnFocusLossBehavior> result;
				result.choices = {
					{_("Auto"), "", MinimizeOnFocusLossBehavior::Auto},
					{_("Off"), "", MinimizeOnFocusLossBehavior::Off},
					{_("On (Fullscreen)"), "", MinimizeOnFocusLossBehavior::On_Fullscreen},
				};
				result.setCurrentIdxForValue(wzGetCurrentMinimizeOnFocusLossBehavior());
				return result;
			},
			[](const auto& newMode) -> bool {
				wzSetMinimizeOnFocusLoss(newMode);
				war_setMinimizeOnFocusLoss(static_cast<int>(newMode));
				return true;
			}, true
		);
		result->addOption(optionInfo, valueChanger, true);
	}
#endif

	return result;
}
