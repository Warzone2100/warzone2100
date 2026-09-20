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

#include "lib/framework/frame.h"
#include "lib/framework/input.h"
#include "lib/gamelib/gtime.h"
#include "lib/ivis_opengl/pieblitfunc.h"
#include "lib/ivis_opengl/piepalette.h"
#include "lib/ivis_opengl/textdraw.h"
#include "lib/sound/audio.h"
#include "lib/sound/audio_id.h"
#include "lib/widget/button.h"
#include "lib/widget/form.h"
#include "lib/widget/paneltabbutton.h"
#include "lib/widget/paragraph.h"
#include "lib/widget/scrollbar.h"

#include "../challenge.h"
#include "../console.h"
#include "../hci.h"
#include "../input/manager.h"
#include "../loop.h"
#include "../mission.h"
#include "../multimenu.h"
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
// The history's scroll bar, kept clear of the text whether it shows or not
static constexpr int HISTORY_SCROLLBAR_WIDTH = 10;
static constexpr int HISTORY_SCROLLBAR_GAP = 4;
static constexpr int HISTORY_WHEEL_LINES = 3;
static constexpr int PANEL_CORNER_RADIUS = 6;

static const PIELIGHT PANEL_FILL_COLOR = pal_RGBA(10, 10, 40, 225);
static const PIELIGHT PANEL_BORDER_COLOR = pal_RGBA(100, 100, 255, 120);
static const PIELIGHT MARKER_FILL_COLOR = pal_RGBA(25, 0, 110, 235);
static const PIELIGHT MARKER_BORDER_COLOR = pal_RGBA(255, 255, 255, 150);

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

static void setFragmentTexts(std::vector<ConsoleTextFragment> &texts, const std::vector<WzTextLayoutFragment> &fragments)
{
	texts.resize(fragments.size());
	for (size_t i = 0; i < fragments.size(); ++i)
	{
		texts[i].text.setText(fragments[i].text, font_regular);
		texts[i].offset = static_cast<int>(fragments[i].offset);
	}
}

// MARK: - The history

/// The history's lines, wrapped to the view's width. Only the visible lines have text objects,
/// from a pool sized to the view, which are refilled as it scrolls.
class ConsoleHistoryView : public WIDGET
{
protected:
	ConsoleHistoryView(): WIDGET() {}
	void initialize();

public:
	static std::shared_ptr<ConsoleHistoryView> make()
	{
		class make_shared_enabler: public ConsoleHistoryView {};
		auto widget = std::make_shared<make_shared_enabler>();
		widget->initialize();
		return widget;
	}

	ConsoleHistoryFilter filter() const
	{
		return currentFilter;
	}
	void setFilter(ConsoleHistoryFilter filter);
	void scroll(ConsoleScroll scroll);
	/// Whether messages arrived below the view since the player scrolled up
	bool hasUnseenMessages();

	void display(int xOffset, int yOffset) override;
	void geometryChanged() override;
	std::shared_ptr<WIDGET> findMouseTargetRecursive(W_CONTEXT *psContext, WIDGET_KEY key, bool wasPressed) override;
	bool canConsumeWheelScroll() override;

private:
	void update();
	bool atBottom() const;
	int textWidth() const;

	struct WrappedEntry
	{
		uint32_t id;
		std::vector<ConsoleWrappedLine> lines;
	};
	struct Row
	{
		uint32_t entryId;
		uint32_t line;
		size_t entryIndex;	///< Into wrapped, and the history's entries
	};
	struct RowSlot
	{
		bool filled = false;
		uint32_t entryId = 0;
		uint32_t line = 0;
		std::vector<ConsoleTextFragment> fragments;
	};

	std::shared_ptr<ScrollBarWidget> scrollBar;
	ConsoleHistoryFilter currentFilter = ConsoleHistoryFilter::All;
	std::deque<WrappedEntry> wrapped;	///< One for each of the history's entries, in the same order
	std::vector<Row> rows;				///< The wrapped lines of the entries in the filter
	std::vector<RowSlot> slots;
	bool rowsValid = false;
	bool unseenMessages = false;
};

void ConsoleHistoryView::initialize()
{
	scrollBar = ScrollBarWidget::make();
	attach(scrollBar);
	scrollBar->setStickToBottom(true);
}

int ConsoleHistoryView::textWidth() const
{
	return std::max(1, width() - HISTORY_SCROLLBAR_WIDTH - HISTORY_SCROLLBAR_GAP);
}

void ConsoleHistoryView::geometryChanged()
{
	scrollBar->setGeometry(width() - HISTORY_SCROLLBAR_WIDTH, 0, HISTORY_SCROLLBAR_WIDTH, height());
	scrollBar->setViewSize(static_cast<uint32_t>(std::max(height(), 0)));
	wrapped.clear();
	rowsValid = false;
	slots.clear();
	slots.resize(static_cast<size_t>(std::max(height(), 0) / consoleLinePitch() + 2));
}

bool ConsoleHistoryView::atBottom() const
{
	const int64_t contentHeight = static_cast<int64_t>(rows.size()) * consoleLinePitch();
	return scrollBar->position() + static_cast<int64_t>(height()) >= contentHeight;
}

void ConsoleHistoryView::setFilter(ConsoleHistoryFilter filter)
{
	if (filter == currentFilter)
	{
		return;
	}
	currentFilter = filter;
	rowsValid = false;
	update();
	scroll(ConsoleScroll::Newest);
}

void ConsoleHistoryView::scroll(ConsoleScroll scroll)
{
	update();
	const int page = std::max(height() - consoleLinePitch(), consoleLinePitch());
	switch (scroll)
	{
	case ConsoleScroll::PageUp:
		scrollBar->incrementPosition(-page);
		break;
	case ConsoleScroll::PageDown:
		scrollBar->incrementPosition(page);
		break;
	case ConsoleScroll::Oldest:
		scrollBar->setPosition(0);
		break;
	case ConsoleScroll::Newest:
		scrollBar->setPosition(UINT32_MAX);
		break;
	}
	if (atBottom())
	{
		unseenMessages = false;
	}
}

bool ConsoleHistoryView::hasUnseenMessages()
{
	update();
	return unseenMessages;
}

void ConsoleHistoryView::update()
{
	const auto &entries = getConsoleHistory().entries();
	const bool historyChanged = wrapped.size() != entries.size()
		|| (!entries.empty() && (wrapped.front().id != entries.front().id || wrapped.back().id != entries.back().id));
	if (rowsValid && !historyChanged)
	{
		return;
	}

	const int pitch = consoleLinePitch();
	const bool wasAtBottom = rows.empty() || atBottom();
	const uint32_t lastRowEntryId = rows.empty() ? 0 : rows.back().entryId;
	const bool hadRows = !rows.empty();

	// Keep the line at the top of the view where it is, if it's still there
	optional<Row> anchor;
	int anchorOffset = 0;
	if (!wasAtBottom)
	{
		const size_t firstRow = scrollBar->position() / pitch;
		if (firstRow < rows.size())
		{
			anchor = rows[firstRow];
			anchorOffset = static_cast<int>(scrollBar->position() % pitch);
		}
	}

	// Both are in the order the entries arrived, so only the dropped ones and the new ones change
	while (!wrapped.empty() && (entries.empty() || wrapped.front().id < entries.front().id))
	{
		wrapped.pop_front();
	}
	for (size_t i = wrapped.size(); i < entries.size(); ++i)
	{
		wrapped.push_back({entries[i].id, wrapConsoleText(entries[i].text, textWidth())});
	}

	rows.clear();
	for (size_t i = 0; i < entries.size(); ++i)
	{
		if (!consoleHistoryFilterIncludes(currentFilter, entries[i].channel))
		{
			continue;
		}
		for (uint32_t line = 0; line < wrapped[i].lines.size(); ++line)
		{
			rows.push_back({entries[i].id, line, i});
		}
	}

	scrollBar->setScrollableSize(static_cast<uint32_t>(rows.size() * pitch));
	if (wasAtBottom)
	{
		scrollBar->setPosition(UINT32_MAX);
	}
	else if (anchor.has_value())
	{
		auto it = std::lower_bound(rows.begin(), rows.end(), *anchor, [](const Row &a, const Row &b) {
			return a.entryId < b.entryId || (a.entryId == b.entryId && a.line < b.line);
		});
		scrollBar->setPosition(static_cast<uint32_t>((it - rows.begin()) * pitch + anchorOffset));
		if (hadRows && !rows.empty() && rows.back().entryId > lastRowEntryId)
		{
			unseenMessages = true;
		}
	}
	if (atBottom())
	{
		unseenMessages = false;
	}
	scrollBar->show(rows.size() * pitch > static_cast<size_t>(std::max(height(), 0)));
	rowsValid = true;
}

std::shared_ptr<WIDGET> ConsoleHistoryView::findMouseTargetRecursive(W_CONTEXT *psContext, WIDGET_KEY key, bool wasPressed)
{
	auto result = WIDGET::findMouseTargetRecursive(psContext, key, wasPressed);
	// The hover pass runs once a frame, so the wheel only scrolls once
	if (key == WKEY_NONE && getMouseWheelSpeed().y != 0)
	{
		update();
		scrollBar->incrementPosition(-getMouseWheelSpeed().y * HISTORY_WHEEL_LINES * consoleLinePitch());
		if (atBottom())
		{
			unseenMessages = false;
		}
	}
	return result;
}

bool ConsoleHistoryView::canConsumeWheelScroll()
{
	return true;
}

void ConsoleHistoryView::display(int xOffset, int yOffset)
{
	update();
	if (rows.empty() || slots.empty())
	{
		return;
	}

	const auto &entries = getConsoleHistory().entries();
	const int pitch = consoleLinePitch();
	const int aboveBase = iV_GetTextAboveBase(font_regular);
	const int x0 = xOffset + x();
	const int y0 = yOffset + y();
	const int position = static_cast<int>(scrollBar->position());
	const WzRect clip(x0, y0, textWidth(), height());

	const size_t first = static_cast<size_t>(position / pitch);
	const size_t last = std::min(rows.size(), static_cast<size_t>((position + height() + pitch - 1) / pitch));
	for (size_t i = first; i < last; ++i)
	{
		const Row &row = rows[i];
		const ConsoleWrappedLine &line = wrapped[row.entryIndex].lines[row.line];
		RowSlot &slot = slots[i % slots.size()];
		if (!slot.filled || slot.entryId != row.entryId || slot.line != row.line)
		{
			setFragmentTexts(slot.fragments, line.fragments);
			slot.entryId = row.entryId;
			slot.line = row.line;
			slot.filled = true;
		}

		const ConsoleHistoryEntry &entry = entries[row.entryIndex];
		const PIELIGHT color = (currentFilter == ConsoleHistoryFilter::Team) ? WZCOL_CONS_TEXT_USER_ALLY : getConsoleTextColor(entry.player);
		const int lineX = justifiedX(entry.justification, line.width, x0, x0 + textWidth());
		const int baseline = y0 + static_cast<int>(i) * pitch - position + LINE_PADDING_Y - aboveBase;
		for (auto &fragment : slot.fragments)
		{
			fragment.text.renderClipped(Vector2f(lineX + fragment.offset, baseline), color, clip);
		}
	}
}

/// Shows while messages arrived below the view, and jumps to them
class ConsoleNewMessagesButton : public W_BUTTON
{
public:
	void display(int xOffset, int yOffset) override
	{
		const int x0 = x() + xOffset;
		const int y0 = y() + yOffset;
		const bool isHighlight = (getState() & WBUT_HIGHLIGHT) != 0;
		pie_DrawRoundedBox(x0, y0, x0 + width(), y0 + height(), MARKER_FILL_COLOR, height() / 2.f, isHighlight ? WZCOL_TEXT_BRIGHT : MARKER_BORDER_COLOR, 1.f);
		text.setText(pText, FontID);
		const int textX = x0 + (width() - text.width()) / 2;
		const int textY = y0 + (height() - text.lineSize()) / 2 - text.aboveBase();
		text.render(textX, textY, WZCOL_TEXT_BRIGHT);
	}

private:
	WzText text;
};

/// The expanded console: a backdrop with the filter tabs above the history
class ConsoleHistoryPanel : public WIDGET
{
protected:
	ConsoleHistoryPanel(): WIDGET() {}
	void initialize();

public:
	static std::shared_ptr<ConsoleHistoryPanel> make()
	{
		class make_shared_enabler: public ConsoleHistoryPanel {};
		auto widget = std::make_shared<make_shared_enabler>();
		widget->initialize();
		return widget;
	}

	ConsoleHistoryFilter filter() const
	{
		return view->filter();
	}
	void setFilter(ConsoleHistoryFilter filter);
	void scroll(ConsoleScroll scroll)
	{
		view->scroll(scroll);
	}

	void display(int xOffset, int yOffset) override;
	void geometryChanged() override;

private:
	std::shared_ptr<WzMultiMenuTabs> tabs;
	std::shared_ptr<ConsoleHistoryView> view;
	std::shared_ptr<ConsoleNewMessagesButton> newMessages;
};

void ConsoleHistoryPanel::initialize()
{
	tabs = std::make_shared<WzMultiMenuTabs>(static_cast<int>(ConsoleHistoryFilter::All));
	attach(tabs);
	tabs->addButton(static_cast<int>(ConsoleHistoryFilter::All), WzPanelTabButton::make(_("All")));
	tabs->addButton(static_cast<int>(ConsoleHistoryFilter::Team), WzPanelTabButton::make(_("Team")));
	tabs->choose(static_cast<int>(ConsoleHistoryFilter::All));

	view = ConsoleHistoryView::make();
	attach(view);

	newMessages = std::make_shared<ConsoleNewMessagesButton>();
	attach(newMessages);
	newMessages->setString(_("New messages"));
	newMessages->FontID = font_regular;
	newMessages->hide();

	std::weak_ptr<ConsoleHistoryView> weakView = view;
	tabs->addOnChooseHandler([weakView](MultibuttonWidget &, int newValue) {
		if (auto strongView = weakView.lock())
		{
			strongView->setFilter(static_cast<ConsoleHistoryFilter>(newValue));
		}
	});
	newMessages->addOnClickHandler([weakView](W_BUTTON &) {
		if (auto strongView = weakView.lock())
		{
			strongView->scroll(ConsoleScroll::Newest);
		}
	});
}

void ConsoleHistoryPanel::setFilter(ConsoleHistoryFilter filter)
{
	tabs->choose(static_cast<int>(filter));
}

void ConsoleHistoryPanel::geometryChanged()
{
	const int inner = width() - 2 * CONSOLE_PANEL_PADDING;
	tabs->setGeometry(CONSOLE_PANEL_PADDING, CONSOLE_PANEL_PADDING, tabs->idealWidth(), CONSOLE_TAB_HEIGHT);
	const int viewTop = CONSOLE_PANEL_PADDING + CONSOLE_TAB_HEIGHT + CONSOLE_TAB_GAP;
	view->setGeometry(CONSOLE_PANEL_PADDING + LINE_PADDING_X, viewTop, inner - LINE_PADDING_X, height() - viewTop - CONSOLE_PANEL_PADDING);

	const int markerHeight = iV_GetTextLineSize(font_regular) + 4;
	const int markerWidth = iV_GetTextWidth(newMessages->pText, font_regular) + 2 * markerHeight;
	newMessages->setGeometry((width() - markerWidth) / 2, view->y() + view->height() - markerHeight - 2, markerWidth, markerHeight);
}

void ConsoleHistoryPanel::display(int xOffset, int yOffset)
{
	const int x0 = x() + xOffset;
	const int y0 = y() + yOffset;
	// The panel hangs from the top of the screen, so only its bottom corners are rounded
	pie_DrawRoundedBox(x0, y0, x0 + width(), y0 + height(), PANEL_FILL_COLOR, glm::vec4(0.f, 0.f, PANEL_CORNER_RADIUS, PANEL_CORNER_RADIUS), PANEL_BORDER_COLOR, 1.f);
	newMessages->show(view->hasUnseenMessages());
}

// MARK: - The console

/// The feed and, when expanded, the history panel in its place.
/// In the feed only the lines take the mouse, and a press on one clears the feed.
class InGameConsoleWidget : public WIDGET
{
protected:
	InGameConsoleWidget(): WIDGET() {}
	void initialize();

public:
	static std::shared_ptr<InGameConsoleWidget> make()
	{
		class make_shared_enabler: public InGameConsoleWidget {};
		auto widget = std::make_shared<make_shared_enabler>();
		widget->initialize();
		return widget;
	}

	bool isExpanded() const
	{
		return expanded;
	}
	ConsoleHistoryFilter filter() const
	{
		return panel->filter();
	}
	void setExpanded(bool value, ConsoleHistoryFilter filter);
	void scroll(ConsoleScroll scroll)
	{
		panel->scroll(scroll);
	}

	bool hitTest(int x, int y) const override;
	void clicked(W_CONTEXT *psContext, WIDGET_KEY key) override;
	void display(int xOffset, int yOffset) override;
	void geometryChanged() override;

private:
	bool consoleShown() const;
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

	std::shared_ptr<ConsoleHistoryPanel> panel;
	bool expanded = false;
	std::vector<FeedMessage> feed;	///< One for each of getConsoleFeed()'s messages, in the same order
	uint32_t feedRevision = 0;
	bool feedLaidOut = false;
};

static std::weak_ptr<InGameConsoleWidget> currentConsole;

void InGameConsoleWidget::initialize()
{
	panel = ConsoleHistoryPanel::make();
	attach(panel);
	panel->hide();
	panel->setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
		const ConsoleRect rect = inGameConsoleExpandedRect();
		psWidget->setGeometry(rect.x, rect.y, rect.width, rect.height);
	}));
	setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
		psWidget->setGeometry(0, 0, screenWidth, screenHeight);
	}));
}

bool InGameConsoleWidget::consoleShown() const
{
	return intConsoleShown() && getConsoleDisplayStatus();
}

void InGameConsoleWidget::setExpanded(bool value, ConsoleHistoryFilter filter)
{
	panel->setFilter(filter);
	if (value == expanded)
	{
		return;
	}
	expanded = value;
	panel->show(expanded);
	if (expanded)
	{
		panel->scroll(ConsoleScroll::Newest);
	}
	// Page Up, Page Down, Home, End and Esc act on the history while it's open
	gInputManager.contexts().set(InputContext::CONSOLE, expanded ? InputContext::State::ACTIVE : InputContext::State::INACTIVE);
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
			FeedMessage wrappedMessage;
			wrappedMessage.id = message.id;
			for (auto &line : wrapConsoleText(message.text, wrapWidth))
			{
				FeedLine feedLine;
				setFragmentTexts(feedLine.fragments, line.fragments);
				const int plateWidth = (line.width > 0) ? line.width + 2 * LINE_PADDING_X : 0;
				feedLine.plate = WzRect(0, 0, plateWidth, pitch);
				wrappedMessage.lines.push_back(std::move(feedLine));
			}
			updated.push_back(std::move(wrappedMessage));
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
	if (!consoleShown())
	{
		return false;
	}
	if (expanded)
	{
		return panel->geometry().contains(x - this->x(), y - this->y());
	}
	// Lines laid out for an older feed may no longer be there
	if (!feedLaidOut || feedRevision != getConsoleFeedRevision())
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
	if (key == WKEY_PRIMARY && !expanded)
	{
		clearActiveConsole();
	}
}

void InGameConsoleWidget::display(int xOffset, int yOffset)
{
	const bool shown = consoleShown();
	// The panel is drawn after this, as a child
	panel->show(shown && expanded);
	if (!shown || expanded)
	{
		// Every line in the feed is also in the history
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
			for (auto &fragment : line.fragments)
			{
				fragment.text.render(x0 + LINE_PADDING_X + fragment.offset, y0 + LINE_PADDING_Y - aboveBase, textColor);
			}
		}
	}
}

// MARK: - The console's interface

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

ConsoleRect inGameConsoleExpandedRect()
{
	return consoleExpandedRect(screenWidth, screenHeight, consoleLinePitch());
}

bool inGameConsoleIsExpanded()
{
	auto console = currentConsole.lock();
	return console && console->isExpanded();
}

ConsoleHistoryFilter inGameConsoleFilter()
{
	auto console = currentConsole.lock();
	return console ? console->filter() : ConsoleHistoryFilter::All;
}

void inGameConsoleExpand(ConsoleHistoryFilter filter)
{
	if (auto console = currentConsole.lock())
	{
		console->setExpanded(true, filter);
	}
}

void inGameConsoleCollapse()
{
	if (auto console = currentConsole.lock())
	{
		console->setExpanded(false, console->filter());
	}
}

void inGameConsoleToggle(ConsoleHistoryFilter filter)
{
	auto console = currentConsole.lock();
	if (!console)
	{
		return;
	}
	if (!console->isExpanded())
	{
		console->setExpanded(true, filter);
		audio_PlayTrack(ID_SOUND_WINDOWOPEN);
	}
	else if (console->filter() != filter)
	{
		console->setExpanded(true, filter);
	}
	else
	{
		console->setExpanded(false, filter);
		audio_PlayTrack(ID_SOUND_WINDOWCLOSE);
	}
}

void inGameConsoleScroll(ConsoleScroll scroll)
{
	auto console = currentConsole.lock();
	if (console && console->isExpanded())
	{
		console->scroll(scroll);
	}
}
