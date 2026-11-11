// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-WZ2100-GPL-3.0-Section-7-1.0-or-later

/*
	This file is part of Warzone 2100.
	Copyright (C) 2024-2026  Warzone 2100 Project (https://github.com/Warzone2100)

	Warzone 2100 is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Additional terms under Section 7 of the GNU General Public License
	version 3 apply to this file: version 1.0 of the Warzone 2100
	additional terms, or (at your option) any later version published by
	the Warzone 2100 Project, set out in COPYING.SECTION7 in the
	top-level directory of the source distribution. This notice must be
	kept intact.

	Warzone 2100 is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Warzone 2100. If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

#include "lib/widget/widget.h"
#include "lib/widget/form.h"
#include "../basedef.h"

#include <functional>
#include <unordered_set>
#include <string>

bool hasGameGuideTopics();
bool showGuideScreen(std::function<void ()> onCloseFunc, bool expandedSidebarByDefault);
void closeGuideScreen();
void shutdownGameGuide();

std::vector<std::string> saveLoadedGuideTopics();
bool restoreLoadedGuideTopics(const std::vector<std::string>& topicIds);

bool getGameGuideDisableTopicPopups();
void setGameGuideDisableTopicPopups(bool disablePopups);

// flags to control whether the topic / guide is automatically & immediately opened (displayed to the user)
enum class ShowTopicFlags: int {
	None = 0,
	FirstAdd = 1 << 0,		// open guide only if this topic is newly-added (this playthrough) - if topic was already added, the guide won't be automatically displayed
	NeverViewed = 1 << 1		// open guide only if this topic has never been viewed by the player before (in any playthrough)
};
inline constexpr ShowTopicFlags operator|(ShowTopicFlags a, ShowTopicFlags b) {
	return static_cast<ShowTopicFlags>(static_cast<std::underlying_type<ShowTopicFlags>::type>(a) | static_cast<std::underlying_type<ShowTopicFlags>::type>(b));
}
inline constexpr bool operator&(ShowTopicFlags a, ShowTopicFlags b) {
  return static_cast<ShowTopicFlags>(static_cast<std::underlying_type<ShowTopicFlags>::type>(a) & static_cast<std::underlying_type<ShowTopicFlags>::type>(b)) != ShowTopicFlags::None;
}

void addGuideTopic(const std::string& guideTopicId, ShowTopicFlags showFlags = ShowTopicFlags::None, const std::unordered_set<std::string>& excludedTopicIDs = {});
