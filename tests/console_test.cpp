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

// Unit tests for the in-game console's history (src/console_history.cpp) and layout (src/hci/ingameconsole_layout.h).
// Build with -DWZ_BUILD_CONSOLE_TEST=ON (target: console_test). Exits nonzero on failure.

#include "console_history.h"
#include "hci/ingameconsole_layout.h"
#include "lib/framework/wzapp.h"

#include <cstdio>

// Stubs for the SDL-backend symbols framework/debug.cpp references (as in tests/framework_linktest.cpp)
bool wzIsFullscreen()
{
	return false;
}

bool wzChangeWindowMode(WINDOW_MODE, bool)
{
	return true;
}

void wzDisplayDialog(DialogType, const char*, const char*)
{
}

static int failures = 0;
static int checks = 0;

#define CHECK_TRUE(cond, ...) \
	do { \
		checks++; \
		if (!(cond)) { \
			failures++; \
			std::printf("FAIL %s:%d: ", __FILE__, __LINE__); \
			std::printf(__VA_ARGS__); \
			std::printf("\n"); \
		} \
	} while (0)

#define CHECK_EQ(actual, expected, ...) \
	do { \
		checks++; \
		long long a_ = (actual), e_ = (expected); \
		if (a_ != e_) { \
			failures++; \
			std::printf("FAIL %s:%d: %lld != %lld: ", __FILE__, __LINE__, a_, e_); \
			std::printf(__VA_ARGS__); \
			std::printf("\n"); \
		} \
	} while (0)

// The quick chat form below the chat box, as measured at 640x480 and 1280x720
static constexpr int QUICK_CHAT_HEIGHT = 343;
// What the chat screen stacks under the expanded console: a gap, the chat box, a gap and quick chat
static constexpr int CHAT_BELOW_CONSOLE = 4 + 20 + 4 + QUICK_CHAT_HEIGHT;

static void testChannels()
{
	CHECK_TRUE(consoleChannelOf(SYSTEM_MESSAGE, false) == ConsoleChannel::System, "system message");
	CHECK_TRUE(consoleChannelOf(NOTIFY_MESSAGE, false) == ConsoleChannel::Notify, "notification");
	CHECK_TRUE(consoleChannelOf(3, false) == ConsoleChannel::Global, "a player's message to everyone");
	CHECK_TRUE(consoleChannelOf(3, true) == ConsoleChannel::Team, "a player's message to the team");
	CHECK_TRUE(consoleChannelOf(SYSTEM_MESSAGE, true) == ConsoleChannel::Team, "a system message to the team");

	for (auto channel : {ConsoleChannel::System, ConsoleChannel::Notify, ConsoleChannel::Team, ConsoleChannel::Global})
	{
		CHECK_TRUE(consoleHistoryFilterIncludes(ConsoleHistoryFilter::All, channel), "All includes channel %d", static_cast<int>(channel));
		CHECK_TRUE(consoleHistoryFilterIncludes(ConsoleHistoryFilter::Team, channel) == (channel == ConsoleChannel::Team), "Team includes only Team, channel %d", static_cast<int>(channel));
	}
}

static void testHistoryCap()
{
	ConsoleHistory history(500);
	for (int i = 0; i < 600; ++i)
	{
		history.add(WzString::number(i), LEFT_JUSTIFY, i % 4, i % 3 == 0, static_cast<uint32_t>(i) * 10);
	}
	const auto &entries = history.entries();
	CHECK_EQ(entries.size(), 500, "the history keeps its capacity");
	CHECK_EQ(entries.front().id, 100, "the oldest entries are the ones dropped");
	CHECK_EQ(entries.back().id, 599, "the newest entry is last");
	CHECK_TRUE(entries.front().text == WzString::number(100), "an entry keeps its text");
	CHECK_EQ(entries.front().gameTimeAdded, 1000, "an entry keeps its arrival time");
	CHECK_TRUE(entries.front().channel == ConsoleChannel::Global, "entry 100 isn't a team message");
	CHECK_TRUE(entries[2].channel == ConsoleChannel::Team, "entry 102 is a team message");
	for (size_t i = 1; i < entries.size(); ++i)
	{
		if (entries[i].id != entries[i - 1].id + 1)
		{
			CHECK_TRUE(false, "ids increase by one, at %zu", i);
			break;
		}
	}
}

static void testHistoryClear()
{
	ConsoleHistory history(10);
	history.add(WzString::fromUtf8("before"), LEFT_JUSTIFY, SYSTEM_MESSAGE, false, 0);
	history.add(WzString::fromUtf8("before"), LEFT_JUSTIFY, SYSTEM_MESSAGE, false, 0);
	history.clear();
	CHECK_TRUE(history.entries().empty(), "clear() empties the history");
	history.add(WzString::fromUtf8("after"), LEFT_JUSTIFY, SYSTEM_MESSAGE, false, 0);
	CHECK_EQ(history.entries().front().id, 2, "ids keep counting across a clear");
}

static void testColumnWidth()
{
	CHECK_EQ(consoleColumnWidth(640), 608, "a narrow screen keeps the side margins");
	CHECK_EQ(consoleColumnWidth(932), 900, "the width reaches the maximum");
	CHECK_EQ(consoleColumnWidth(2560), 900, "and stays there");
	CHECK_EQ(consoleColumnWidth(10), 0, "never negative");
}

static void testExpandedRect()
{
	struct Case
	{
		int width;
		int height;
		int expectedX;
		int expectedHeight;
	};
	// With an 18 pixel line pitch: 4 lines (the minimum) at 480, then a third of the screen in whole lines
	const int pitch = 18;
	const Case cases[] = {
		{640, 480, 16, CONSOLE_PANEL_CHROME_HEIGHT + 4 * pitch},
		{1280, 720, 190, CONSOLE_PANEL_CHROME_HEIGHT + 11 * pitch},
		{1920, 1080, 510, CONSOLE_PANEL_CHROME_HEIGHT + 18 * pitch},
		{2560, 1440, 830, CONSOLE_PANEL_CHROME_HEIGHT + 24 * pitch},
	};
	for (const auto &c : cases)
	{
		const ConsoleRect rect = consoleExpandedRect(c.width, c.height, pitch);
		CHECK_EQ(rect.x, c.expectedX, "%dx%d: centered", c.width, c.height);
		CHECK_EQ(rect.y, 0, "%dx%d: at the top", c.width, c.height);
		CHECK_EQ(rect.width, consoleColumnWidth(c.width), "%dx%d: as wide as the column", c.width, c.height);
		CHECK_EQ(rect.height, c.expectedHeight, "%dx%d: height", c.width, c.height);
	}

	// At any line pitch a normal font gives, the chat fits below, and the panel is no taller than a third of the
	// screen unless that's less than the minimum lines
	for (int linePitch = 14; linePitch <= 18; ++linePitch)
	{
		for (int height = 480; height <= 2160; height += 8)
		{
			const ConsoleRect rect = consoleExpandedRect(1280, height, linePitch);
			const int lines = (rect.height - CONSOLE_PANEL_CHROME_HEIGHT) / linePitch;
			CHECK_EQ((rect.height - CONSOLE_PANEL_CHROME_HEIGHT) % linePitch, 0, "whole lines, pitch %d, height %d", linePitch, height);
			CHECK_TRUE(lines >= CONSOLE_MIN_LINES, "at least the minimum lines, pitch %d, height %d", linePitch, height);
			CHECK_TRUE(rect.bottom() + CHAT_BELOW_CONSOLE <= height, "the chat fits below, pitch %d, height %d: ends at %d", linePitch, height, rect.bottom() + CHAT_BELOW_CONSOLE);
			CHECK_TRUE(rect.height <= height / 3 || lines == CONSOLE_MIN_LINES, "a third of the screen at most, pitch %d, height %d", linePitch, height);
		}
	}
}

int main()
{
	testChannels();
	testHistoryCap();
	testHistoryClear();
	testColumnWidth();
	testExpandedRect();

	std::printf("%s: %d checks, %d failures\n", failures == 0 ? "PASS" : "FAIL", checks, failures);
	return failures == 0 ? 0 : 1;
}
