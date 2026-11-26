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
#include "../hud/intel/intelkit.h"
#include "../hud/hudchatbar.h"
#include "../hud/hudconsole.h"
#include "../hud/hudskin.h"
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
		HudConsolePrefix prefix;	///< (in a skin that draws the console)
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
		HudConsolePrefixTexts prefixTexts;
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
	if (hudConsoleSkinned())
	{
		scrollBar->setSliderDisplay(intelkit::displayScrollbar);
	}
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
	const bool skinned = hudConsoleSkinned();
	for (size_t i = wrapped.size(); i < entries.size(); ++i)
	{
		if (skinned)
		{
			// As a chat line, the text wrapped after its prefix
			WzString text;
			HudConsolePrefix prefix = hudConsolePrefix(entries[i].text, entries[i].player, entries[i].channel == ConsoleChannel::Team, pitch, text);
			wrapped.push_back({entries[i].id, wrapConsoleText(text, textWidth() - prefix.width), std::move(prefix)});
		}
		else
		{
			wrapped.push_back({entries[i].id, wrapConsoleText(entries[i].text, textWidth()), {}});
		}
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

	const bool skinned = hudConsoleSkinned();
	// (In the skin's card, a few lines sit at the bottom, by the chat bar)
	const int bottomGap = skinned ? std::max(0, height() - static_cast<int>(rows.size()) * pitch) : 0;
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
		const int rowTop = y0 + bottomGap + static_cast<int>(i) * pitch - position;
		const int baseline = rowTop + LINE_PADDING_Y - aboveBase;
		if (skinned)
		{
			const HudConsolePrefix &prefix = wrapped[row.entryIndex].prefix;
			// (Chat lines start at the left. The console's own notices keep their justification)
			const int lineX = prefix.name.empty() ? justifiedX(entry.justification, prefix.width + line.width, x0, x0 + textWidth()) : x0;
			// (A prefix cut off at the view's edge isn't drawn, as its text isn't)
			if (row.line == 0 && rowTop >= y0 && rowTop + pitch <= y0 + height())
			{
				hudConsoleDrawPrefix(slot.prefixTexts, prefix, lineX, rowTop, pitch, 1.f, false);
			}
			const PIELIGHT textColor = hudConsoleTextColor(entry.player, 1.f);
			for (auto &fragment : slot.fragments)
			{
				fragment.text.renderClipped(Vector2f(lineX + prefix.width + fragment.offset, baseline), textColor, clip);
			}
			continue;
		}
		const PIELIGHT color = (currentFilter == ConsoleHistoryFilter::Team) ? WZCOL_CONS_TEXT_USER_ALLY : getConsoleTextColor(entry.player);
		const int lineX = justifiedX(entry.justification, line.width, x0, x0 + textWidth());
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
		text.setText(pText, FontID);
		const int textY = y0 + (height() - text.lineSize()) / 2 - text.aboveBase();
		if (hudConsoleSkinned())
		{
			// The skin's accent button, an arrow down before the text
			HudSkin &skin = currentHudSkin();
			const HudColors &colors = skin.colors();
			pie_DrawRoundedBox(x0, y0, x0 + width(), y0 + height(), isHighlight ? pal_RGBA(26, 70, 112, 245) : pal_RGBA(18, 52, 86, 240), skin.metrics().cardRadius, colors.accent, 1.f);
			const int icon = height() - 8;
			const int contentX = x0 + (width() - icon - 4 - text.width()) / 2;
			skin.drawNamedIcon("chevron-down", WzRect(contentX, y0 + 4, icon, icon), colors.textPrimary);
			text.render(contentX + icon + 4, textY, colors.textPrimary);
			return;
		}
		pie_DrawRoundedBox(x0, y0, x0 + width(), y0 + height(), MARKER_FILL_COLOR, height() / 2.f, isHighlight ? WZCOL_TEXT_BRIGHT : MARKER_BORDER_COLOR, 1.f);
		const int textX = x0 + (width() - text.width()) / 2;
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
	/// Whether it was built for a skin that draws the console
	bool skinned() const
	{
		return chatRow != nullptr;
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
	/// In a skin that draws the console: the chat bar as the bottom row, in place of the tabs (while chat is open,
	/// the chat screen's own bar takes its place)
	std::shared_ptr<HudChatBar> chatRow;
};

static bool consoleChatOpen = false;
/// The chat screen's quick chat strip, under the card's bottom row while chatting
static int consoleChatExtra = 0;

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

	if (hudConsoleSkinned())
	{
		tabs->hide();
		chatRow = HudChatBar::make(screenWidth < HUD_CHAT_BAR_COMPACT_WIDTH, WzChatMode::Glob);
		chatRow->setEmbedded(true);
		auto chatMode = [](const std::shared_ptr<HudChatBar> &bar) {
			return (bar && bar->mode() == WzChatMode::Team) ? CHAT_TEAM : CHAT_GLOB;
		};
		std::weak_ptr<HudChatBar> weakRow = chatRow;
		chatRow->setIdle([weakRow, chatMode]() {
			chatDialog(chatMode(weakRow.lock()), false);
		}, [weakRow, chatMode]() {
			chatDialog(chatMode(weakRow.lock()), true);
		});
		std::weak_ptr<ConsoleHistoryPanel> weakPanel = std::static_pointer_cast<ConsoleHistoryPanel>(shared_from_this());
		chatRow->onModeChanged = [weakPanel](WzChatMode mode) {
			if (auto panel = weakPanel.lock())
			{
				panel->setFilter(mode == WzChatMode::Team ? ConsoleHistoryFilter::Team : ConsoleHistoryFilter::All);
			}
		};
		attach(chatRow);
	}

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
	if (chatRow)
	{
		chatRow->setMode(filter == ConsoleHistoryFilter::Team ? WzChatMode::Team : WzChatMode::Glob);
	}
}

void ConsoleHistoryPanel::geometryChanged()
{
	const int markerHeight = iV_GetTextLineSize(font_regular) + 4;
	if (chatRow)
	{
		const int row = hudConsoleChatRowHeight();
		view->setGeometry(HUD_CONSOLE_PADDING, HUD_CONSOLE_PADDING, width() - 2 * HUD_CONSOLE_PADDING, height() - 2 * HUD_CONSOLE_PADDING - row);
		chatRow->setGeometry(0, height() - row, width(), row);
		const int markerWidth = iV_GetTextWidth(newMessages->pText, font_regular) + 2 * markerHeight;
		newMessages->setGeometry((width() - markerWidth) / 2, view->y() + view->height() - markerHeight - 2, markerWidth, markerHeight);
		return;
	}
	const int inner = width() - 2 * CONSOLE_PANEL_PADDING;
	tabs->setGeometry(CONSOLE_PANEL_PADDING, CONSOLE_PANEL_PADDING, tabs->idealWidth(), CONSOLE_TAB_HEIGHT);
	const int viewTop = CONSOLE_PANEL_PADDING + CONSOLE_TAB_HEIGHT + CONSOLE_TAB_GAP;
	view->setGeometry(CONSOLE_PANEL_PADDING + LINE_PADDING_X, viewTop, inner - LINE_PADDING_X, height() - viewTop - CONSOLE_PANEL_PADDING);

	const int markerWidth = iV_GetTextWidth(newMessages->pText, font_regular) + 2 * markerHeight;
	newMessages->setGeometry((width() - markerWidth) / 2, view->y() + view->height() - markerHeight - 2, markerWidth, markerHeight);
}

void ConsoleHistoryPanel::display(int xOffset, int yOffset)
{
	const int x0 = x() + xOffset;
	const int y0 = y() + yOffset;
	if (chatRow)
	{
		// One card: the lines, a divider, the chat bar (and while chatting, quick chat under it)
		const WzRect card(x0, y0, width(), height() + (consoleChatOpen ? consoleChatExtra : 0));
		currentHudSkin().drawPanelFrame(card, card);
		const int dividerY = y0 + height() - hudConsoleChatRowHeight();
		intelkit::drawDivider(x0 + 1, dividerY, x0 + width() - 1, dividerY);
		chatRow->show(!consoleChatOpen);
		newMessages->show(view->hasUnseenMessages());
		return;
	}
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
	/// After a change of skin: the feed laid out again, the history built for the skin
	void skinChanged();
	void scroll(ConsoleScroll scroll)
	{
		panel->scroll(scroll);
	}
	std::shared_ptr<WIDGET> historyPanel() const
	{
		return panel;
	}

	bool hitTest(int x, int y) const override;
	void clicked(W_CONTEXT *psContext, WIDGET_KEY key) override;
	void display(int xOffset, int yOffset) override;
	void geometryChanged() override;

private:
	bool consoleShown() const;
	void makePanel();
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
		HudConsolePrefix prefix;	///< (in a skin that draws the console)
		HudConsolePrefixTexts prefixTexts;
	};

	std::shared_ptr<ConsoleHistoryPanel> panel;
	bool expanded = false;
	std::vector<FeedMessage> feed;	///< One for each of getConsoleFeed()'s messages, in the same order
	uint32_t feedRevision = 0;
	bool feedLaidOut = false;
};

static std::weak_ptr<InGameConsoleWidget> currentConsole;

void InGameConsoleWidget::makePanel()
{
	panel = ConsoleHistoryPanel::make();
	attach(panel);
	panel->hide();
	panel->setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
		const ConsoleRect rect = inGameConsoleExpandedRect();
		psWidget->setGeometry(rect.x, rect.y, rect.width, rect.height);
	}));
}

void InGameConsoleWidget::skinChanged()
{
	feed.clear();
	feedLaidOut = false;
	if (panel->skinned() == hudConsoleSkinned())
	{
		return;
	}
	const bool wasExpanded = expanded;
	const ConsoleHistoryFilter shownFilter = panel->filter();
	setExpanded(false, shownFilter);
	detach(panel);
	makePanel();
	if (wasExpanded)
	{
		setExpanded(true, shownFilter);
	}
}

void InGameConsoleWidget::initialize()
{
	makePanel();
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
		if (hudConsoleSkinned())
		{
			// (Its place depends on the options button's, which comes after the console)
			panel->callCalcLayout();
		}
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
	int wrapWidth = consoleColumnWidth(width()) - 2 * LINE_PADDING_X;
	const int left = CONSOLE_SIDE_MARGIN;
	const int right = width() - CONSOLE_SIDE_MARGIN;
	const int pitch = consoleLinePitch();
	const bool skinned = hudConsoleSkinned();
	int top = (!challengeActive && game.type == LEVEL_TYPE::SKIRMISH) ? FEED_TOP_SKIRMISH : FEED_TOP_WITH_TIMERS;
	// (In the skin, the feed stops short of the options button)
	int clearRight = right;
	if (skinned)
	{
		const int screenLeft = screenPosX() + left;
		wrapWidth = std::min(wrapWidth, hudConsoleRight(screenLeft, screenPosY() + top, screenLeft + wrapWidth + 2 * LINE_PADDING_X) - screenLeft - 2 * LINE_PADDING_X);
		clearRight = hudConsoleRight(screenLeft, screenPosY() + top, screenPosX() + right) - screenPosX();
	}

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
			WzString text = message.text;
			if (skinned)
			{
				// As a chat line, the text wrapped after its prefix
				wrappedMessage.prefix = hudConsolePrefix(message.text, message.player, message.team, pitch, text);
			}
			const int indent = wrappedMessage.prefix.width;
			for (auto &line : wrapConsoleText(text, wrapWidth - indent))
			{
				FeedLine feedLine;
				setFragmentTexts(feedLine.fragments, line.fragments);
				const int plateWidth = (line.width > 0 || indent > 0) ? indent + line.width + 2 * LINE_PADDING_X : 0;
				feedLine.plate = WzRect(0, 0, plateWidth, pitch);
				wrappedMessage.lines.push_back(std::move(feedLine));
			}
			updated.push_back(std::move(wrappedMessage));
		}

		for (auto &line : updated.back().lines)
		{
			const int plateWidth = line.plate.width();
			// (Chat lines start at the left. The console's own notices keep their justification)
			int plateX = (skinned && !updated.back().prefix.name.empty()) ? left : justifiedX(message.justification, plateWidth, left, right);
			if (plateX + plateWidth > clearRight)
			{
				plateX = std::max(left, clearRight - plateWidth);
			}
			line.plate = WzRect(plateX, top, plateWidth, pitch);
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
	const bool skinned = hudConsoleSkinned();
	for (size_t i = 0; i < feed.size() && i < messages.size(); ++i)
	{
		const float opacity = feedLineOpacity(messages[i]);
		if (opacity <= 0.f)
		{
			continue;
		}
		if (skinned)
		{
			// No backplates: the text is outlined over the 3D view, like the skin's other overlay text
			const PIELIGHT textColor = hudConsoleTextColor(messages[i].player, opacity);
			const int indent = feed[i].prefix.width;
			for (size_t lineIndex = 0; lineIndex < feed[i].lines.size(); ++lineIndex)
			{
				auto &line = feed[i].lines[lineIndex];
				const int x0 = xOffset + x() + line.plate.x() + LINE_PADDING_X;
				const int y0 = yOffset + y() + line.plate.y();
				if (lineIndex == 0)
				{
					hudConsoleDrawPrefix(feed[i].prefixTexts, feed[i].prefix, x0, y0, line.plate.height(), opacity, true);
				}
				for (auto &fragment : line.fragments)
				{
					currentHudSkin().drawOverlayText(fragment.text, x0 + indent + fragment.offset, y0 + LINE_PADDING_Y - aboveBase, textColor);
				}
			}
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
				currentHudSkin().drawOverlayText(fragment.text, x0 + LINE_PADDING_X + fragment.offset, y0 + LINE_PADDING_Y - aboveBase, textColor);
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

void inGameConsoleKeepOnTop(const std::shared_ptr<WIDGET> &under)
{
	auto console = currentConsole.lock();
	if (!console)
	{
		return;
	}
	auto parent = console->parent();
	if (!parent)
	{
		return;
	}
	const auto &children = parent->children();
	auto at = under && under->parent() == parent && !console->isExpanded() ? std::find(children.begin(), children.end(), under) : children.end();
	if (at == children.end())
	{
		if (children.back() != console)
		{
			parent->detach(console);
			parent->attach(console);
		}
		return;
	}
	if (at != children.begin() && *(at - 1) == console)
	{
		return;
	}
	// Attach them again in order: the console, then `under` and everything above it
	const std::vector<std::shared_ptr<WIDGET>> over(at, children.end());
	parent->detach(console);
	for (const auto &w : over)
	{
		parent->detach(w);
	}
	parent->attach(console);
	for (const auto &w : over)
	{
		parent->attach(w);
	}
}

ConsoleRect inGameConsoleExpandedRect()
{
	const ConsoleRect rect = consoleExpandedRect(screenWidth, screenHeight, consoleLinePitch());
	return hudConsoleSkinned() ? hudConsoleExpandedRect(rect) : rect;
}

void inGameConsoleSetChatOpen(bool open)
{
	consoleChatOpen = open;
	if (!open)
	{
		consoleChatExtra = 0;
	}
}

void inGameConsoleSetChatExtra(int height)
{
	consoleChatExtra = std::max(0, height);
}

void inGameConsoleSkinChanged()
{
	if (auto console = currentConsole.lock())
	{
		console->skinChanged();
	}
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

std::shared_ptr<WIDGET> inGameConsolePanel()
{
	auto console = currentConsole.lock();
	return console ? console->historyPanel() : nullptr;
}
