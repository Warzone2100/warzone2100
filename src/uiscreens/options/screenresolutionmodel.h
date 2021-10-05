#pragma once

#include <vector>
#include <tuple>

#include "videooptions.h"

#include "lib/framework/wzapp.h"

#include "../../warzoneconfig.h"

class ScreenResolutionsModel
{
public:
	typedef const std::vector<screeninfo>::iterator iterator;
	typedef const std::vector<screeninfo>::const_iterator const_iterator;

	ScreenResolutionsModel() : modes(ScreenResolutionsModel::loadModes())
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

	void selectAt(size_t index) const;

	static std::string currentResolutionString()
	{
		return ScreenResolutionsModel::resolutionString(getCurrentResolution());
	}

	static std::string resolutionString(const screeninfo& info)
	{
		return astringf("[%d] %d × %d", info.screen, info.width, info.height);
	}

private:
	const std::vector<screeninfo> modes;

	static screeninfo getCurrentResolution()
	{
		screeninfo info;
		info.screen = war_GetScreen();
		info.width = war_GetWidth();
		info.height = war_GetHeight();
		info.refresh_rate = -1;  // Unused.
		return info;
	}

	static std::vector<screeninfo> loadModes()
	{
		// Get resolutions, sorted with duplicates removed.
		std::vector<screeninfo> modes = wzAvailableResolutions();
		std::sort(modes.begin(), modes.end(), ScreenResolutionsModel::compareLess);
		modes.erase(std::unique(modes.begin(), modes.end(), ScreenResolutionsModel::compareEq), modes.end());

		return modes;
	}

	static std::tuple<int, int, int> compareKey(const screeninfo& x)
	{
		return std::make_tuple(x.screen, x.width, x.height);
	}

	static bool compareLess(const screeninfo& a, const screeninfo& b)
	{
		return ScreenResolutionsModel::compareKey(a) < ScreenResolutionsModel::compareKey(b);
	}

	static bool compareEq(const screeninfo& a, const screeninfo& b)
	{
		return ScreenResolutionsModel::compareKey(a) == ScreenResolutionsModel::compareKey(b);
	}
};

