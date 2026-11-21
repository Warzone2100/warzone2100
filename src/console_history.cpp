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

#include "console_history.h"

ConsoleChannel consoleChannelOf(int player, bool team)
{
	if (team)
	{
		return ConsoleChannel::Team;
	}
	switch (player)
	{
	case SYSTEM_MESSAGE:
		return ConsoleChannel::System;
	case NOTIFY_MESSAGE:
		return ConsoleChannel::Notify;
	default:
		return ConsoleChannel::Global;
	}
}

bool consoleHistoryFilterIncludes(ConsoleHistoryFilter filter, ConsoleChannel channel)
{
	switch (filter)
	{
	case ConsoleHistoryFilter::All:
		return true;
	case ConsoleHistoryFilter::Team:
		return channel == ConsoleChannel::Team;
	}
	return true;
}

ConsoleHistory::ConsoleHistory(size_t capacity)
	: maxEntries(capacity)
{ }

void ConsoleHistory::add(const WzString &text, CONSOLE_TEXT_JUSTIFICATION justification, int player, bool team, uint32_t gameTimeAdded)
{
	history.push_back({nextId++, text, justification, player, consoleChannelOf(player, team), gameTimeAdded});
	while (history.size() > maxEntries)
	{
		history.pop_front();
	}
}

void ConsoleHistory::clear()
{
	// nextId keeps counting, so a view can't mistake new entries for ones it saw before the clear
	history.clear();
}
