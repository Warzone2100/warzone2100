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

#include "ingameconsole.h"
#include "ingameconsole_layout.h"

#include "lib/framework/frame.h"
#include "lib/gamelib/gtime.h"
#include "lib/ivis_opengl/pieblitfunc.h"
#include "lib/ivis_opengl/piepalette.h"
#include "lib/ivis_opengl/textdraw.h"
#include "lib/widget/form.h"
#include "lib/widget/paragraph.h"

#include "../challenge.h"
#include "../console.h"
#include "../hci.h"
#include "../loop.h"
#include "../mission.h"
#include "../multiplay.h"

#include <algorithm>
#include <vector>

// How long a feed line takes to fade out at the end of its time
static constexpr UDWORD FEED_FADE_DURATION = GAME_TICKS_PER_SEC / 2;
// The top of the feed's first line, below the row the mission and transporter timers use
static constexpr int FEED_TOP_SKIRMISH = 20;
static constexpr int FEED_TOP_WITH_TIMERS = FEED_TOP_SKIRMISH + TIMER_Y;
// Space between a line's text and the edges of its backplate
static constexpr int LINE_PADDING_X = 4;
static constexpr int LINE_PADDING_Y = 1;

static std::weak_ptr<WIDGET> currentConsole;

struct ConsoleTextFragment
{
	WzText text;
	int offset;
};

/// One wrapped line of a message, as fragments of text in the same direction
struct ConsoleWrappedLine
{
	std::vector<WzTextLayoutFragment> fragments;
	int width = 0;
};

static std::vector<ConsoleWrappedLine> wrapConsoleText(const WzString &text, int maxWidth)
{
	std::vector<ConsoleWrappedLine> lines;
	for (auto &fragments : wzLayoutTextLines(text, font_regular, static_cast<unsigned int>(std::max(maxWidth, 1))))
	{
		ConsoleWrappedLine line;
		for (const auto &fragment : fragments)
		{
			line.width = std::max(line.width, static_cast<int>(fragment.offset + fragment.width));
		}
		line.fragments = std::move(fragments);
		lines.push_back(std::move(line));
	}
	if (lines.empty())
	{
		// An empty message still takes a line
		lines.emplace_back();
	}
	return lines;
}

static int consoleLinePitch()
{
	return iV_GetTextLineSize(font_regular) + 2 * LINE_PADDING_Y;
}

/// Where a line of the given width starts within [left, right)
static int justifiedX(CONSOLE_TEXT_JUSTIFICATION justification, int lineWidth, int left, int right)
{
	switch (justification)
	{
	case RIGHT_JUSTIFY:
		return right - lineWidth;
	case CENTRE_JUSTIFY:
		return left + (right - left - lineWidth) / 2;
	case LEFT_JUSTIFY:
		break;
	}
	return left;
}

static float feedLineOpacity(const ConsoleFeedMessage &message)
{
	if (message.duration == MAX_CONSOLE_MESSAGE_DURATION || consolePaused())
	{
		// While the console is paused, lines don't expire, so they don't fade either
		return 1.f;
	}
	const UDWORD age = realTime - message.timeAdded;
	if (age >= message.duration)
	{
		return 0.f;
	}
	return std::min(1.f, static_cast<float>(message.duration - age) / FEED_FADE_DURATION);
}

static PIELIGHT withOpacity(PIELIGHT color, float opacity)
{
	color.byte.a = static_cast<uint8_t>(color.byte.a * opacity);
	return color;
}

static void drawConsoleTextLine(std::vector<ConsoleTextFragment> &fragments, int x, int y, PIELIGHT color)
{
	for (auto &fragment : fragments)
	{
		fragment.text.render(x + fragment.offset, y, color);
	}
}

/// The feed: the recent messages, each line on its own backplate, below the timers' row.
/// Only the lines take the mouse, and a press on one clears the feed.
class InGameConsoleWidget : public WIDGET
{
protected:
	InGameConsoleWidget(): WIDGET() {}

public:
	static std::shared_ptr<InGameConsoleWidget> make()
	{
		class make_shared_enabler: public InGameConsoleWidget {};
		auto widget = std::make_shared<make_shared_enabler>();
		widget->setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
			psWidget->setGeometry(0, 0, screenWidth, screenHeight);
		}));
		return widget;
	}

	bool hitTest(int x, int y) const override;
	void clicked(W_CONTEXT *psContext, WIDGET_KEY key) override;
	void display(int xOffset, int yOffset) override;
	void geometryChanged() override;

private:
	bool feedShown() const;
	void updateFeed();

	struct FeedLine
	{
		std::vector<ConsoleTextFragment> fragments;
		WzRect plate;	///< Relative to this widget
	};
	struct FeedMessage
	{
		uint32_t id;
		std::vector<FeedLine> lines;
	};

	std::vector<FeedMessage> feed;	///< One for each of getConsoleFeed()'s messages, in the same order
	uint32_t feedRevision = 0;
	bool feedLaidOut = false;
};

bool InGameConsoleWidget::feedShown() const
{
	return intConsoleShown() && getConsoleDisplayStatus();
}

void InGameConsoleWidget::geometryChanged()
{
	feed.clear();
	feedLaidOut = false;
}

void InGameConsoleWidget::updateFeed()
{
	if (feedLaidOut && feedRevision == getConsoleFeedRevision())
	{
		return;
	}

	const auto &messages = getConsoleFeed();
	const int wrapWidth = consoleColumnWidth(width()) - 2 * LINE_PADDING_X;
	const int left = CONSOLE_SIDE_MARGIN;
	const int right = width() - CONSOLE_SIDE_MARGIN;
	const int pitch = consoleLinePitch();
	int top = (!challengeActive && game.type == LEVEL_TYPE::SKIRMISH) ? FEED_TOP_SKIRMISH : FEED_TOP_WITH_TIMERS;

	// Both lists are in the order the messages arrived, so the lines of the ones still there can be kept
	std::vector<FeedMessage> updated;
	updated.reserve(messages.size());
	auto previous = feed.begin();
	for (const auto &message : messages)
	{
		while (previous != feed.end() && previous->id < message.id)
		{
			++previous;
		}
		if (previous != feed.end() && previous->id == message.id)
		{
			updated.push_back(std::move(*previous));
		}
		else
		{
			FeedMessage wrapped;
			wrapped.id = message.id;
			for (auto &line : wrapConsoleText(message.text, wrapWidth))
			{
				FeedLine feedLine;
				for (const auto &fragment : line.fragments)
				{
					feedLine.fragments.push_back({WzText(fragment.text, font_regular), static_cast<int>(fragment.offset)});
				}
				const int plateWidth = (line.width > 0) ? line.width + 2 * LINE_PADDING_X : 0;
				feedLine.plate = WzRect(0, 0, plateWidth, pitch);
				wrapped.lines.push_back(std::move(feedLine));
			}
			updated.push_back(std::move(wrapped));
		}

		for (auto &line : updated.back().lines)
		{
			const int plateWidth = line.plate.width();
			line.plate = WzRect(justifiedX(message.justification, plateWidth, left, right), top, plateWidth, pitch);
			top += pitch;
		}
	}

	feed = std::move(updated);
	feedRevision = getConsoleFeedRevision();
	feedLaidOut = true;
}

bool InGameConsoleWidget::hitTest(int x, int y) const
{
	// Lines laid out for an older feed may no longer be there
	if (!feedShown() || !feedLaidOut || feedRevision != getConsoleFeedRevision())
	{
		return false;
	}
	const int localX = x - this->x();
	const int localY = y - this->y();
	for (const auto &message : feed)
	{
		for (const auto &line : message.lines)
		{
			if (line.plate.contains(localX, localY))
			{
				return true;
			}
		}
	}
	return false;
}

void InGameConsoleWidget::clicked(W_CONTEXT *, WIDGET_KEY key)
{
	if (key == WKEY_PRIMARY)
	{
		clearActiveConsole();
	}
}

void InGameConsoleWidget::display(int xOffset, int yOffset)
{
	if (!feedShown())
	{
		return;
	}
	updateFeed();

	const auto &messages = getConsoleFeed();
	const int aboveBase = iV_GetTextAboveBase(font_regular);
	for (size_t i = 0; i < feed.size() && i < messages.size(); ++i)
	{
		const float opacity = feedLineOpacity(messages[i]);
		if (opacity <= 0.f)
		{
			continue;
		}
		const PIELIGHT textColor = withOpacity(getConsoleTextColor(messages[i].player), opacity);
		const PIELIGHT plateColor = withOpacity(WZCOL_TRANSPARENT_BOX, opacity);
		for (auto &line : feed[i].lines)
		{
			const int x0 = xOffset + x() + line.plate.x();
			const int y0 = yOffset + y() + line.plate.y();
			if (line.plate.width() > 0)
			{
				pie_UniTransBoxFill(x0, y0, x0 + line.plate.width(), y0 + line.plate.height(), plateColor);
			}
			drawConsoleTextLine(line.fragments, x0 + LINE_PADDING_X, y0 + LINE_PADDING_Y - aboveBase, textColor);
		}
	}
}

void inGameConsoleAdd(const std::shared_ptr<W_SCREEN> &screen)
{
	ASSERT_OR_RETURN(, screen != nullptr, "No screen");
	auto console = InGameConsoleWidget::make();
	screen->psForm->attach(console);
	currentConsole = console;
}

void inGameConsoleKeepOnTop()
{
	auto console = currentConsole.lock();
	if (!console)
	{
		return;
	}
	auto parent = console->parent();
	if (!parent || parent->children().back() == console)
	{
		return;
	}
	parent->detach(console);
	parent->attach(console);
}
