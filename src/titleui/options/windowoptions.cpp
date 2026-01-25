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

#include <fmt/core.h>

// MARK: -

OptionInfo::AvailabilityResult NotEditableCondition(const OptionInfo&)
{
	OptionInfo::AvailabilityResult result;
	result.available = false;
	return result;
}

// MARK: -

class ScreenResolutionsModel
{
public:
	typedef const std::vector<optional<screeninfo>>::iterator iterator;
	typedef const std::vector<optional<screeninfo>>::const_iterator const_iterator;

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
		ASSERT_OR_RETURN(false, index < modes.size(), "Invalid mode index passed to ScreenResolutionsModel::selectAt");

		auto selectedResolution = modes.at(index);

		auto currentResolution = getCurrentResolution();
		// Attempt to change the resolution
		if (!wzChangeFullscreenDisplayMode(selectedResolution))
		{
			debug(
				LOG_WARNING,
				"Failed to change fullscreen mode from: %s to: %s",
				ScreenResolutionsModel::resolutionString(currentResolution).toUtf8().c_str(),
				ScreenResolutionsModel::resolutionString(selectedResolution).toUtf8().c_str()
			);
			return false;
		}

		// NOTE: Settings are saved by wzChangeFullscreenDisplayMode on success

		return true;
	}

	static WzString currentResolutionString()
	{
		return ScreenResolutionsModel::resolutionString(getCurrentResolution());
	}

	static WzString resolutionString(const optional<screeninfo> &optInfo)
	{
		if (!optInfo.has_value())
		{
			return _("Desktop Full");
		}
		const screeninfo& info = optInfo.value();
		auto str = WzString::format("[%d] %d × %d @ %.2fhz", info.screen, info.width, info.height, info.refresh_rate);
		if (info.pixel_density > 1.f)
		{
			std::string dpiStr = fmt::format("{:g}", info.pixel_density);
			str.append(WzString::format(" [%sx DPI]", dpiStr.c_str()));
		}
		return str;
	}

private:
	const std::vector<optional<screeninfo>> modes;

	static screeninfo getCurrentWindowedResolution()
	{
		int screen = 0;
		unsigned int windowWidth = 0, windowHeight = 0;
		wzGetWindowResolution(&screen, &windowWidth, &windowHeight);
		screeninfo info;
		info.screen = screen;
		info.width = windowWidth;
		info.height = windowHeight;
		info.pixel_density = 0.f; // Unused.
		info.refresh_rate = -1.f;  // Unused.
		return info;
	}

	static optional<screeninfo> getCurrentResolution()
	{
		return wzGetCurrentFullscreenDisplayMode();
	}

	static std::vector<optional<screeninfo>> loadModes()
	{
		// Get resolutions, sorted with duplicates removed.
		std::vector<optional<screeninfo>> modes = wzAvailableResolutions();
		std::sort(modes.begin(), modes.end(), ScreenResolutionsModel::compareLess);
		modes.erase(std::unique(modes.begin(), modes.end(), ScreenResolutionsModel::compareEq), modes.end());

		return modes;
	}

	static std::tuple<int, int, int, float, float> compareKey(const screeninfo &x)
	{
		return std::make_tuple(x.screen, x.width, x.height, x.refresh_rate, x.pixel_density);
	}

	static bool compareLess(const optional<screeninfo> &a, const optional<screeninfo> &b)
	{
		if (!a.has_value())
		{
			return b.has_value();
		}
		if (!b.has_value())
		{
			return false;
		}
		return ScreenResolutionsModel::compareKey(a.value()) < ScreenResolutionsModel::compareKey(b.value());
	}

	static bool compareEq(const optional<screeninfo> &a, const optional<screeninfo> &b)
	{
		if (!a.has_value() || !b.has_value())
		{
			return a.has_value() == b.has_value();
		}
		return ScreenResolutionsModel::compareKey(a.value()) == ScreenResolutionsModel::compareKey(b.value());
	}
};

// MARK: -

std::shared_ptr<OptionsForm> makeWindowOptionsForm()
{
	auto result = OptionsForm::make();
	result->setRefreshOptionsOnScreenSizeChange(true);

	auto screenResolutionsModel = std::make_shared<ScreenResolutionsModel>();

	result->addSection(OptionsSection(N_("Window"), ""), true);
	{
		auto optionInfo = OptionInfo("window.mode", N_("Window Mode"), "");
		auto valueChanger = OptionsDropdown<WINDOW_MODE>::make(
			[]() {
				OptionChoices<WINDOW_MODE> result;
				result.choices = {
					{_("Windowed"), "", WINDOW_MODE::windowed},
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

	result->addSection(OptionsSection(N_("Fullscreen"), ""), true);
	{
		auto optionInfo = OptionInfo("fullscreen.mode", N_("Fullscreen Mode"), "");
		auto valueChanger = OptionsDropdown<size_t>::make(
			[screenResolutionsModel]() {
				OptionChoices<size_t> result;
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
				return screenResolutionsModel->selectAt(newResolutionIdx);
			}, true, 15
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
