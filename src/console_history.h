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
	The in-game console's message history, kept apart from its drawing.
*/

#ifndef __INCLUDED_SRC_CONSOLE_HISTORY_H__
#define __INCLUDED_SRC_CONSOLE_HISTORY_H__

#include "lib/framework/wzstring.h"

#include <cstddef>
#include <cstdint>
#include <deque>

/* ID to use for addConsoleMessage() in case of a system message */
#define	SYSTEM_MESSAGE				(-1)
#define NOTIFY_MESSAGE				(-2)	// mainly used for lobby & error messages
#define INFO_MESSAGE				(-3)	// This type is not stored, it is used for simple messages

enum CONSOLE_TEXT_JUSTIFICATION
{
	LEFT_JUSTIFY,
	DEFAULT_JUSTIFY = LEFT_JUSTIFY,
	RIGHT_JUSTIFY,
	CENTRE_JUSTIFY
};

enum class ConsoleChannel : uint8_t
{
	System,	///< SYSTEM_MESSAGE
	Notify,	///< NOTIFY_MESSAGE
	Team,	///< Added with `team`, which includes private messages
	Global	///< Any other player's message
};

ConsoleChannel consoleChannelOf(int player, bool team);

enum class ConsoleHistoryFilter : uint8_t
{
	All,
	Team
};

bool consoleHistoryFilterIncludes(ConsoleHistoryFilter filter, ConsoleChannel channel);

struct ConsoleHistoryEntry
{
	uint32_t id;	///< One more than the entry added before it
	WzString text;
	CONSOLE_TEXT_JUSTIFICATION justification;
	int player;
	ConsoleChannel channel;
	uint32_t gameTimeAdded;
};

/// The newest entries, oldest first, up to a capacity
class ConsoleHistory
{
public:
	explicit ConsoleHistory(size_t capacity);

	void add(const WzString &text, CONSOLE_TEXT_JUSTIFICATION justification, int player, bool team, uint32_t gameTimeAdded);
	void clear();

	const std::deque<ConsoleHistoryEntry> &entries() const
	{
		return history;
	}
	size_t capacity() const
	{
		return maxEntries;
	}

private:
	std::deque<ConsoleHistoryEntry> history;
	size_t maxEntries;
	uint32_t nextId = 0;
};

#endif // __INCLUDED_SRC_CONSOLE_HISTORY_H__
