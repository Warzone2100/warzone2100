/*
	This file is part of Warzone 2100.
	Copyright (C) 2026  Warzone 2100 Project (https://github.com/Warzone2100)

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
/** @file
 *  The research tree view widget.
 */

#include "researchtreeview.h"

#include "lib/widget/paneltabbutton.h"
#include "lib/widget/editbox.h"
#include "lib/widget/label.h"
#include "lib/widget/scrollablelist.h"
#include "lib/ivis_opengl/pieblitfunc.h"
#include "lib/framework/input.h"

#include "researchtreecanvas.h"
#include "../titleui/widgets/optionsform.h"	// for WzOptionsDropdownWidget
#include "../hci.h"
#include "../research.h"
#include "../structure.h"
#include "../researchlogviewer.h"
#include "../console.h"

static constexpr int TOP_ROW_HEIGHT = 20;
static constexpr int TOP_ROW_GAP = 4;
static constexpr int TOOLBAR_PAD = 8;
// Equivalent to the band the in-game options browser puts behind its form switcher
static const PIELIGHT TOOLBAR_FILL = pal_RGBA(10, 10, 59, 255);
// Slightly brighter than the canvas - a traced path's heading belonging to the tree (instead of the window around it)
static const PIELIGHT HEADING_BAR_FILL = pal_RGBA(24, 24, 24, 255);
static constexpr int SEARCH_WIDTH = 240;
static constexpr int RESULT_ROW_HEIGHT = 18;
static constexpr size_t MAX_RESULTS = 40;
static constexpr int RESULTS_MAX_HEIGHT = RESULT_ROW_HEIGHT * 12;
static constexpr int RESULTS_PADDING = 3;
static constexpr int RESULTS_MAX_WIDTH = 420;
static constexpr int TRACE_BAR_HEIGHT = 50;
static constexpr int TRACE_EXIT_WIDTH = 104;
static constexpr int SWITCHER_TEXT_INSET = 5; // How far in the seat switcher writes its own text
static constexpr int OVERLAY_INSET = DETAIL_CORNER_INSET + SWITCHER_TEXT_INSET;
static constexpr int NAMES_BUTTON_WIDTH = 76;
static constexpr int IDLE_CHIP_WIDTH = 96;
static constexpr int WASTE_CHIP_WIDTH = 150;
static constexpr int LAB_SLOT_WIDTH = 128;
static constexpr int LAB_SLOT_GAP = 4;
static constexpr int LEGEND_ROW_HEIGHT = 14;
static constexpr int LEGEND_SWATCH = 8;
static constexpr int LEGEND_TEXT_GAP = 5;
static constexpr int LEGEND_ITEM_GAP = 16;
static constexpr int LEGEND_PAD = 6;

static PIELIGHT borderColor()
{
	PIELIGHT color = WZCOL_TEXT_MEDIUM;
	color.byte.a = 140;
	return color;
}
static PIELIGHT legendHintColor()
{
	PIELIGHT color = WZCOL_TEXT_MEDIUM;
	color.byte.a = 170;
	return color;
}
static const PIELIGHT TRACE_BAR_FILL = HEADING_BAR_FILL;
static constexpr size_t PERSPECTIVE_ROWS_SHOWN = 12;
static constexpr int OWN_SEAT_CHIP_WIDTH = 92;
static constexpr const char *BACK_ARROW = "\u2190 ";

// Throttle live state read rate
static constexpr uint32_t LAB_REFRESH_MS = 250;

// MARK: - WzResearchLabButton

class WzResearchLabButton : public W_BUTTON
{
public:
	void setLab(const ResearchLabOption& lab)
	{
		m_lab = lab;
		const WzString wanted = lab.currentSubject.isEmpty() ? WzString::fromUtf8(_("Idle")) : lab.currentSubject;
		if (wanted != m_shown)
		{
			m_shown = wanted;
			m_text.setText(wanted, font_small);
		}
	}

	const ResearchLabOption& lab() const { return m_lab; }

	void setOnHoverChanged(std::function<void(bool)> handler) { m_onHoverChanged = std::move(handler); }

	void highlight(W_CONTEXT *psContext) override
	{
		W_BUTTON::highlight(psContext);
		if (!m_over)
		{
			m_over = true;
			if (m_onHoverChanged) { m_onHoverChanged(true); }
		}
	}

	void highlightLost() override
	{
		W_BUTTON::highlightLost();
		if (m_over)
		{
			m_over = false;
			if (m_onHoverChanged) { m_onHoverChanged(false); }
		}
	}

	void display(int xOffset, int yOffset) override
	{
		const float x0 = static_cast<float>(x() + xOffset);
		const float y0 = static_cast<float>(y() + yOffset);
		const float x1 = x0 + static_cast<float>(width());
		const float y1 = y0 + static_cast<float>(height());

		const bool over = (getState() & WBUT_HIGHLIGHT) != 0;
		const PIELIGHT fill = m_lab.idle ? pal_Colour(58, 46, 20) : pal_Colour(30, 30, 30);
		const PIELIGHT edge = m_lab.idle ? pal_Colour(210, 170, 70) : (over ? WZCOL_TEXT_BRIGHT : borderColor());
		pie_DrawRoundedBox(x0, y0, x1, y1, fill, 3.f, edge, 1.f);

		const PIELIGHT textColor = m_lab.idle ? pal_Colour(240, 215, 150)
			: (over ? WZCOL_TEXT_BRIGHT : WZCOL_FORM_TEXT);
		const float textY = y0 + (static_cast<float>(height()) - m_text.lineSize()) / 2.f
			- static_cast<float>(m_text.aboveBase());
		const int room = width() - 10;
		if (m_text.width() <= room)
		{
			m_text.render(x0 + 5.f, textY, textColor);
		}
		else
		{
			const int shown = room - iV_GetEllipsisWidth(font_small) - 2;
			if (shown > 0)
			{
				m_text.renderClipped(Vector2f(x0 + 5.f, textY), textColor,
				                     WzRect(static_cast<int>(x0) + 5, static_cast<int>(y0), shown, height()), shown);
				iV_DrawEllipsis(font_small, Vector2f(x0 + 5.f + static_cast<float>(shown) + 2.f, textY), textColor);
			}
		}

		if (!m_lab.idle && m_lab.currentPercent > 0)
		{
			const float done = (x1 - x0 - 4.f) * static_cast<float>(m_lab.currentPercent) / 100.f;
			pie_DrawRoundedBox(x0 + 2.f, y1 - 3.f, x0 + 2.f + done, y1 - 1.f,
			                   researchInProgressColor(), 1.f, pal_RGBA(0, 0, 0, 0), 0.f);
		}
	}

private:
	ResearchLabOption m_lab;
	bool m_over = false;
	WzString m_shown;
	WzText m_text;
	std::function<void(bool)> m_onHoverChanged;
};

// MARK: - WzResearchLabStrip

// The lab buttons, in a row, so the view shows what all of them are doing without the player leaving it.
// Idle labs are called out.
class WzResearchLabStrip : public WIDGET
{
public:
	void setPlayer(uint32_t player)
	{
		m_player = player;
		m_lastRefresh = 0;
	}

	size_t labCount() const { return m_labs.size(); }

	size_t idleCount() const
	{
		size_t idle = 0;
		for (const auto& lab : m_labs)
		{
			idle += lab.idle ? 1 : 0;
		}
		return idle;
	}

	optional<uint16_t> subjectAt(size_t slot) const
	{
		if (slot >= m_labs.size() || m_labs[slot].idle || m_labs[slot].subjectIndex >= asResearch.size())
		{
			return nullopt;
		}
		return m_labs[slot].subjectIndex;
	}

	std::shared_ptr<WIDGET> slotWidget(size_t slot) const
	{
		return (slot < m_buttons.size()) ? m_buttons[slot] : nullptr;
	}

	void setOnSlotHovered(std::function<void(optional<size_t>)> handler) { m_onSlotHovered = std::move(handler); }
	void setOnSlotClicked(std::function<void(size_t)> handler) { m_onSlotClicked = std::move(handler); }

	int32_t idealWidth() override
	{
		return static_cast<int32_t>(m_labs.size()) * (LAB_SLOT_WIDTH + LAB_SLOT_GAP);
	}

	void geometryChanged() override
	{
		doLayout();
	}

	void run(W_CONTEXT *) override
	{
		if (realTime - m_lastRefresh < LAB_REFRESH_MS)
		{
			return;
		}
		m_lastRefresh = realTime;
		m_labs = researchLabsFor(m_player);
		rebuild();
	}

	std::string describe() const
	{
		std::string text;
		for (size_t i = 0; i < m_labs.size(); ++i)
		{
			const auto& lab = m_labs[i];
			text += astringf(_("Lab %u"), static_cast<unsigned>(i + 1));
			if (lab.idle)
			{
				text += std::string(" - ") + _("no research assigned");
			}
			else if (lab.onHold)
			{
				text += std::string(" - ") + _("on hold");
			}
			else if (lab.waitingForPower)
			{
				text += std::string(" - ") + _("waiting for power");
			}
			else
			{
				text += " - " + lab.currentSubject.toUtf8() + astringf(" (%d%%)", lab.currentPercent);
			}
			text += "\n";
		}
		return text;
	}

private:
	void rebuild()
	{
		std::weak_ptr<WzResearchLabStrip> weakSelf = std::dynamic_pointer_cast<WzResearchLabStrip>(shared_from_this());
		while (m_buttons.size() > m_labs.size())
		{
			detach(m_buttons.back());
			m_buttons.pop_back();
		}
		while (m_buttons.size() < m_labs.size())
		{
			const size_t slot = m_buttons.size();
			auto button = std::make_shared<WzResearchLabButton>();
			attach(button);
			button->setOnHoverChanged([weakSelf, slot](bool over) {
				auto self = weakSelf.lock();
				if (self && self->m_onSlotHovered) { self->m_onSlotHovered(over ? optional<size_t>(slot) : nullopt); }
			});
			button->addOnClickHandler([weakSelf, slot](W_BUTTON&) {
				auto self = weakSelf.lock();
				if (self && self->m_onSlotClicked) { self->m_onSlotClicked(slot); }
			});
			m_buttons.push_back(std::move(button));
		}
		for (size_t i = 0; i < m_labs.size(); ++i)
		{
			m_buttons[i]->setLab(m_labs[i]);
		}
		doLayout();
	}

	// Lay out however many lab buttons fit
	void doLayout()
	{
		for (size_t i = 0; i < m_buttons.size(); ++i)
		{
			const int at = static_cast<int>(i) * (LAB_SLOT_WIDTH + LAB_SLOT_GAP);
			if (at + LAB_SLOT_WIDTH > width())
			{
				m_buttons[i]->hide();
				continue;
			}
			m_buttons[i]->show();
			m_buttons[i]->setGeometry(at, 0, LAB_SLOT_WIDTH, height());
		}
	}

	uint32_t m_player = 0;
	uint32_t m_lastRefresh = 0;
	std::vector<ResearchLabOption> m_labs;
	std::vector<std::shared_ptr<WzResearchLabButton>> m_buttons;
	std::function<void(optional<size_t>)> m_onSlotHovered;
	std::function<void(size_t)> m_onSlotClicked;
};

// MARK: - WzResearchTraceBar

class WzResearchTraceBar : public WIDGET
{
public:
	void setTrace(const WzString& subject, const WzString& detail)
	{
		if (subject != m_subject)
		{
			m_subject = subject;
			m_subjectText.setText(subject, font_regular_bold);
		}
		if (detail != m_detail)
		{
			m_detail = detail;
			m_detailText.setText(detail, font_small);
		}
	}

	void display(int xOffset, int yOffset) override
	{
		const float x0 = static_cast<float>(xOffset + x());
		const float y0 = static_cast<float>(yOffset + y());
		pie_UniTransBoxFill(x0, y0, x0 + static_cast<float>(width()), y0 + static_cast<float>(height()),
		                    TRACE_BAR_FILL);

		const int room = std::max(10, width() - OVERLAY_INSET * 2 - TRACE_EXIT_WIDTH - TOP_ROW_GAP);
		float at = y0 + 4.f;
		m_headingText.render(x0 + OVERLAY_INSET, at - m_headingText.aboveBase(), WZCOL_FORM_TEXT);
		at += static_cast<float>(m_headingText.lineSize());
		m_subjectText.renderClipped(Vector2f(x0 + OVERLAY_INSET, at - m_subjectText.aboveBase()), WZCOL_TEXT_BRIGHT,
		                            WzRect(static_cast<int>(x0) + OVERLAY_INSET, static_cast<int>(y0), room, height()), room);

		at += static_cast<float>(m_subjectText.lineSize()) + 1.f;
		m_detailText.renderClipped(Vector2f(x0 + OVERLAY_INSET, at - m_detailText.aboveBase()), WZCOL_TEXT_MEDIUM,
		                           WzRect(static_cast<int>(x0) + OVERLAY_INSET, static_cast<int>(y0), room, height()), room);
	}

private:
	WzString m_subject;
	WzString m_detail;
	WzText m_headingText = []() { WzText t; t.setText(WzString::fromUtf8(_("Trace Path")), font_small); return t; }();
	WzText m_subjectText;
	WzText m_detailText;
};

// MARK: - WzResearchLegend

class WzResearchLegend : public WIDGET
{
public:
	void setLanes(const std::vector<uint8_t>& lanes)
	{
		m_entries.clear();
		for (const auto lane : lanes)
		{
			Entry entry;
			entry.color = researchLaneSwatchColor(lane);
			entry.name = researchLaneName(lane);
			entry.text.setText(entry.name, font_small);
			m_entries.push_back(std::move(entry));
		}
	}

	void setHint(const WzString& text)
	{
		m_hintText = text;
		m_hint.setText(text, font_small);
	}

	int heightForWidth(int forWidth)
	{
		int used = rows(roomIn(forWidth));
		if (used == 0 && hintRoom(usableIn(forWidth)) > 0)
		{
			used = 1;
		}
		return used * LEGEND_ROW_HEIGHT + LEGEND_PAD * 2;
	}

	void display(int xOffset, int yOffset) override
	{
		const int boxX = xOffset + x();
		const int boxY = yOffset + y();
		pie_UniTransBoxFill(static_cast<float>(boxX), static_cast<float>(boxY),
		                    static_cast<float>(boxX + width()), static_cast<float>(boxY + height()),
		                    researchTreeCanvasGround());

		const int originX = boxX + OVERLAY_INSET;
		const int originY = boxY + LEGEND_PAD;
		const int room = roomIn(width());
		int at = 0;
		int row = 0;
		for (auto& entry : m_entries)
		{
			const int itemWidth = entryWidth(entry);
			if (at > 0 && at + itemWidth > room)
			{
				at = 0;
				++row;
			}
			const float x0 = static_cast<float>(originX + at);
			const float y0 = static_cast<float>(originY + row * LEGEND_ROW_HEIGHT);
			const float mid = y0 + static_cast<float>(LEGEND_ROW_HEIGHT) / 2.f;
			pie_DrawRoundedBox(x0, mid - LEGEND_SWATCH / 2.f, x0 + LEGEND_SWATCH, mid + LEGEND_SWATCH / 2.f,
			                   entry.color, LEGEND_SWATCH / 2.f, pal_RGBA(0, 0, 0, 0), 0.f);
			entry.text.render(x0 + LEGEND_SWATCH + LEGEND_TEXT_GAP,
			                  mid - static_cast<float>(entry.text.aboveBase()) / 2.f, WZCOL_TEXT_MEDIUM);
			at += itemWidth;
		}

		// Against the right edge + on the last row the entries reached
		const int hintW = hintRoom(usableIn(width()));
		if (hintW > 0)
		{
			const int hintRoomForText = hintW - LEGEND_ITEM_GAP;
			const int hintTop = originY + std::max(0, rows(room) - 1) * LEGEND_ROW_HEIGHT;
			const float hintBase = static_cast<float>(hintTop) + static_cast<float>(LEGEND_ROW_HEIGHT) / 2.f
				- static_cast<float>(m_hint.aboveBase()) / 2.f;
			const float right = static_cast<float>(boxX + width() - OVERLAY_INSET);
			if (m_hint.width() <= hintRoomForText)
			{
				m_hint.render(right - static_cast<float>(m_hint.width()), hintBase, legendHintColor());
			}
			else
			{
				const float left = right - static_cast<float>(hintRoomForText);
				const int shown = hintRoomForText - iV_GetEllipsisWidth(font_small) - 2;
				if (shown > 0)
				{
					m_hint.renderClipped(Vector2f(left, hintBase), legendHintColor(),
					                     WzRect(static_cast<int>(left), hintTop, shown, LEGEND_ROW_HEIGHT), shown);
					iV_DrawEllipsis(font_small, Vector2f(left + static_cast<float>(shown) + 2.f, hintBase), legendHintColor());
				}
			}
		}
	}

private:
	struct Entry
	{
		PIELIGHT color;
		WzString name;
		WzText text;
	};

	int entryWidth(Entry& entry)
	{
		return LEGEND_SWATCH + LEGEND_TEXT_GAP + entry.text.width() + LEGEND_ITEM_GAP;
	}

	static int usableIn(int forWidth) { return std::max(10, forWidth - OVERLAY_INSET * 2); }

	int hintRoom(int usable)
	{
		if (m_hintText.isEmpty())
		{
			return 0;
		}
		return std::min(m_hint.width() + LEGEND_ITEM_GAP, usable / 2); // never more than half
	}

	int roomIn(int forWidth)
	{
		const int usable = usableIn(forWidth);
		return std::max(10, usable - hintRoom(usable));
	}

	int rows(int forWidth)
	{
		int at = 0;
		int used = 1;
		for (auto& entry : m_entries)
		{
			const int itemWidth = entryWidth(entry);
			if (at > 0 && at + itemWidth > forWidth)
			{
				at = 0;
				++used;
			}
			at += itemWidth;
		}
		return m_entries.empty() ? 0 : used;
	}

	std::vector<Entry> m_entries;
	WzString m_hintText;
	WzText m_hint;
};

std::shared_ptr<WzResearchTreeView> WzResearchTreeView::make(const ResearchTreeContext& context)
{
	auto view = std::make_shared<WzResearchTreeView>();
	view->initialize(context);
	return view;
}

void WzResearchTreeView::initialize(const ResearchTreeContext& context)
{
	m_context = context;
	// The seat the viewer actually holds, and what they may do from it
	// (Not the seat on show, which for a spectator is someone else's)
	m_ownPlayer = context.viewer;
	m_ownAllowAssignment = context.allowAssignment;

	m_canvas = WzResearchTreeCanvas::make(m_context);
	attach(m_canvas);
	{
		std::weak_ptr<WzResearchTreeView> weakSelf = std::dynamic_pointer_cast<WzResearchTreeView>(shared_from_this());
		m_canvas->setOnFocusRequested([weakSelf]() {
			if (auto self = weakSelf.lock())
			{
				self->toggleFocus();
			}
		});
	}

	addPerspectives();
	addSearch();

	m_legend = std::make_shared<WzResearchLegend>();
	m_legend->setHint(WzString::fromUtf8(_("Drag to pan, wheel to zoom, Esc to close")));
	attach(m_legend);

	// Filled immediately
	m_lanesShown = m_canvas->lanesShown();
	m_legend->setLanes(m_lanesShown);
}

void WzResearchTreeView::addPerspectives()
{
	// Live game:
	// - The seats the viewer is allowed to look at (for a player, themselves and their teammates)
	// Finished game:
	// - The seats the research log shows (one per team when teams are fixed + shared res)
	std::vector<ResearchPerspective> seats;
	if (m_context.source == ResearchTreeContext::Source::GameLog)
	{
		for (const auto& column : buildResearchLogColumnDefs())
		{
			ResearchPerspective seat;
			seat.player = static_cast<uint32_t>(column.sourcePlayerIdx);
			seat.title = column.title;
			seats.push_back(std::move(seat));
		}
	}
	else
	{
		seats = researchPerspectivesFor(m_context.viewer);
	}
	if (seats.size() < 2)
	{
		return;
	}

	// Which seat the viewer holds, and which one was asked for. (The same for a player opening their own tree.)
	optional<size_t> asked;
	for (size_t i = 0; i < seats.size(); ++i)
	{
		if (seats[i].kind != ResearchPerspective::Kind::Seat)
		{
			continue;
		}
		if (!m_ownSeat.has_value() && seats[i].player == m_ownPlayer)
		{
			m_ownSeat = i;
		}
		if (!asked.has_value() && seats[i].player == m_context.player)
		{
			asked = i;
		}
	}
	const int chosen = static_cast<int>(asked.value_or(m_ownSeat.value_or(0)));
	m_seats = seats;
	m_seatAt = static_cast<size_t>(chosen);
	setPerspective(seats[chosen]);

	// Not attached here - it goes on the title bar (which is the parent's to place)
	auto picker = std::make_shared<WzOptionsDropdownWidget>();
	picker->setTextAlignment(WLAB_ALIGNCENTRE);
	picker->setCurrentChoicePadding(SWITCHER_TEXT_INSET, 8);
	int itemHeight = 0;
	for (size_t i = 0; i < seats.size(); ++i)
	{
		auto item = WzOptionsChoiceWidget::make(seats[i].depth > 0 ? font_regular : font_regular_bold);
		item->setString(seats[i].title);
		// Members of a team are listed under the team option, and set in from it
		item->setPadding(SWITCHER_TEXT_INSET + seats[i].depth * 14, 4);
		item->setGeometry(0, 0, item->idealWidth(), item->idealHeight());
		itemHeight = std::max(itemHeight, item->idealHeight());
		picker->addItem(item);
	}
	const Padding& listPadding = picker->getDropdownMenuOuterPadding();
	picker->setListHeight(itemHeight * static_cast<uint32_t>(std::min<size_t>(PERSPECTIVE_ROWS_SHOWN, seats.size()))
	                      + listPadding.top + listPadding.bottom);
	picker->setSelectedIndex(static_cast<size_t>(chosen));

	std::weak_ptr<WzResearchTreeView> weakSelf = std::dynamic_pointer_cast<WzResearchTreeView>(shared_from_this());
	picker->setOnChange([weakSelf, seats](DropdownWidget& widget) {
		auto self = weakSelf.lock();
		const auto at = widget.getSelectedIndex();
		if (self == nullptr || !at.has_value() || *at >= seats.size())
		{
			return;
		}
		self->m_seatAt = *at;
		self->setPerspective(seats[*at]);
	});

	m_perspectives = picker;
}

void WzResearchTreeView::showOwnSeat()
{
	if (!m_ownSeat.has_value() || *m_ownSeat >= m_seats.size())
	{
		return;
	}
	m_seatAt = *m_ownSeat;
	if (m_perspectives)
	{
		m_perspectives->setSelectedIndex(m_seatAt);
	}
	setPerspective(m_seats[m_seatAt]);
}

void WzResearchTreeView::cyclePerspective(int delta)
{
	if (m_seats.size() < 2 || delta == 0)
	{
		return;
	}
	const int64_t count = static_cast<int64_t>(m_seats.size());
	const int64_t at = static_cast<int64_t>(m_seatAt);
	m_seatAt = static_cast<size_t>(((at + delta) % count + count) % count);
	if (m_perspectives)
	{
		m_perspectives->setSelectedIndex(m_seatAt);
	}
	setPerspective(m_seats[m_seatAt]);
}

void WzResearchTreeView::setPerspective(const ResearchPerspective& perspective)
{
	const bool team = perspective.kind == ResearchPerspective::Kind::Team;
	std::vector<uint32_t> aggregate = team ? perspective.members : std::vector<uint32_t>();
	if (m_context.player == perspective.player && m_context.aggregate == aggregate)
	{
		return;
	}
	m_context.player = perspective.player;
	m_context.aggregate = std::move(aggregate);
	// Only allow assignment for our own view (of course) - not other players' views or team views
	m_context.allowAssignment = m_ownAllowAssignment && !team && (perspective.player == m_ownPlayer);
	refreshState();
}

void WzResearchTreeView::refreshState()
{
	if (m_ownSeatChip)
	{
		if (m_ownSeat.has_value() && m_seatAt != *m_ownSeat) { m_ownSeatChip->show(); }
		else { m_ownSeatChip->hide(); }
		geometryChanged();
	}
	if (m_canvas)
	{
		m_canvas->setPerspective(m_context.player, m_context.aggregate, m_context.allowAssignment);
	}
	if (m_labStrip)
	{
		if (m_context.allowAssignment) { m_labStrip->show(); } else { m_labStrip->hide(); }
	}
	if (m_idleChip && !m_context.allowAssignment)
	{
		m_idleChip->hide();
	}
	// Always last, so it's triggered once everything has settled
	if (m_onSeatChanged)
	{
		m_onSeatChanged();
	}
}

class WzSearchResultsPanel : public WIDGET
{
public:
	void display(int xOffset, int yOffset) override
	{
		const float x0 = static_cast<float>(xOffset + x());
		const float y0 = static_cast<float>(yOffset + y());
		pie_DrawRoundedBox(x0, y0, x0 + static_cast<float>(width()), y0 + static_cast<float>(height()),
		                   pal_RGBA(24, 24, 24, 240), 6.f, borderColor(), 1.f);
	}

	void geometryChanged() override
	{
		for (auto& child : children())
		{
			child->setGeometry(RESULTS_PADDING, RESULTS_PADDING,
			                   std::max(1, width() - RESULTS_PADDING * 2), std::max(1, height() - RESULTS_PADDING * 2));
		}
	}
};

class WzSearchResultRow : public W_BUTTON
{
public:
	static std::shared_ptr<WzSearchResultRow> make(const WzString& text)
	{
		class make_shared_enabler : public WzSearchResultRow {};
		auto row = std::make_shared<make_shared_enabler>();
		row->FontID = font_small;
		row->m_text.setText(text, font_small);
		return row;
	}

	int32_t idealWidth() override { return m_text.width(); }

	void display(int xOffset, int yOffset) override
	{
		const int x0 = xOffset + x();
		const int y0 = yOffset + y();
		const bool over = isMouseOverWidget();
		if (over)
		{
			pie_UniTransBoxFill(static_cast<float>(x0), static_cast<float>(y0),
			                    static_cast<float>(x0 + width()), static_cast<float>(y0 + height()),
			                    pal_RGBA(70, 80, 100, 110));
		}

		const int textY = y0 + (height() - m_text.lineSize()) / 2 - m_text.aboveBase();
		const PIELIGHT textColor = over ? WZCOL_TEXT_BRIGHT : WZCOL_TEXT_MEDIUM;
		int room = width();
		const bool truncated = m_text.width() > room;
		if (truncated)
		{
			room -= iV_GetEllipsisWidth(m_text.getFontID()) + 2;
		}
		m_text.render(x0, textY, textColor, 0.f, room);
		if (truncated)
		{
			iV_DrawEllipsis(m_text.getFontID(), Vector2f(x0 + room + 2, textY), textColor);
		}
	}

private:
	WzText m_text;
};

static std::shared_ptr<W_BUTTON> makeResultRow(const ResearchSearchHit& hit, const std::function<void()>& onClick)
{
	WzString text = WzString::fromUtf8(getLocalizedStatsName(&asResearch[hit.researchIndex]));
	if (!hit.via.isEmpty())
	{
		text += WzString::fromUtf8(" - ") + hit.via;
	}
	auto row = WzSearchResultRow::make(text);
	row->setGeometry(0, 0, 10, RESULT_ROW_HEIGHT);
	row->addOnClickHandler([onClick](W_BUTTON&) {
		onClick();
	});
	return row;
}

static void displayResearchSearchBox(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset)
{
	W_EDITBOX *editBox = static_cast<W_EDITBOX *>(psWidget);
	const float x0 = static_cast<float>(editBox->x() + xOffset);
	const float y0 = static_cast<float>(editBox->y() + yOffset);
	const PIELIGHT border = editBox->isEditing() ? WZCOL_TEXT_BRIGHT : borderColor();
	pie_DrawRoundedBox(x0, y0, x0 + static_cast<float>(editBox->width()), y0 + static_cast<float>(editBox->height()),
	                   pal_RGBA(255, 255, 255, 10), 4.f, border, 1.f);
}

void WzResearchTreeView::addSearch()
{
	// What the player knows, which is not necessarily what the tree draws
	// (A campaign showing the shape of what has not been found may still not name it yet)
	const std::vector<bool> known = researchKnownTo(m_context);
	m_searchIndex.build(known.empty() ? nullptr : &known);

	m_searchBox = std::make_shared<W_EDITBOX>();
	attach(m_searchBox);
	m_searchBox->setPlaceholder(_("Search"));
	m_searchBox->FontID = font_small;
	m_searchBox->pBoxDisplay = displayResearchSearchBox;
	m_searchBox->setPlaceholderTextColor(WZCOL_TEXT_MEDIUM);
	m_searchBox->setOnReturnHandler([](W_EDITBOX& box) {
		auto view = std::dynamic_pointer_cast<WzResearchTreeView>(box.parent());
		if (view != nullptr)
		{
			view->revealHit(0);
		}
	});
	m_searchBox->setOnEscapeHandler([](W_EDITBOX& box) {
		// Clear the search
		box.setString(WzString());
		box.stopEditing();
		auto view = std::dynamic_pointer_cast<WzResearchTreeView>(box.parent());
		if (view != nullptr)
		{
			view->m_escapeHandled = true;
			view->updateResults();
		}
	});

	m_names = makeResearchActionButton(_("Names"), _("Show a name on every topic"));
	attach(m_names);
	m_names->addOnClickHandler([](W_BUTTON& button) {
		auto view = std::dynamic_pointer_cast<WzResearchTreeView>(button.parent());
		if (view != nullptr)
		{
			view->toggleLabeled();
		}
	});

	if (m_ownSeat.has_value())
	{
		// Beside the switcher on the title bar, so not attached here
		m_ownSeatChip = makeResearchActionButton(WzString::fromUtf8(BACK_ARROW) + WzString::fromUtf8(_("My tree")),
		                                         WzString::fromUtf8(_("Go back to your own research")));
		m_ownSeatChip->hide();
		std::weak_ptr<WzResearchTreeView> weakSelf = std::dynamic_pointer_cast<WzResearchTreeView>(shared_from_this());
		m_ownSeatChip->addOnClickHandler([weakSelf](W_BUTTON&) {
			if (auto view = weakSelf.lock())
			{
				view->showOwnSeat();
			}
		});
	}

	if (m_context.allowAssignment)
	{
		m_labStrip = std::make_shared<WzResearchLabStrip>();
		attach(m_labStrip);
		m_labStrip->setPlayer(m_context.player);

		std::weak_ptr<WzResearchTreeView> weakSelf = std::dynamic_pointer_cast<WzResearchTreeView>(shared_from_this());
		m_labStrip->setOnSlotHovered([weakSelf](optional<size_t> slot) {
			if (auto self = weakSelf.lock())
			{
				self->describeLab(slot, false);
			}
		});
		m_labStrip->setOnSlotClicked([weakSelf](size_t slot) {
			if (auto self = weakSelf.lock())
			{
				self->describeLab(slot, true);
			}
		});

		m_idleChip = makeResearchActionButton(WzString(), WzString());
		attach(m_idleChip);
		m_idleChip->hide();
		m_idleChip->addOnClickHandler([](W_BUTTON& button) {
			auto view = std::dynamic_pointer_cast<WzResearchTreeView>(button.parent());
			if (view != nullptr)
			{
				view->revealSomethingToResearch();
			}
		});

		m_wasteChip = makeResearchActionButton(WzString(), WzString());
		attach(m_wasteChip);
		m_wasteChip->hide();
		m_wasteChip->addOnClickHandler([](W_BUTTON& button) {
			auto view = std::dynamic_pointer_cast<WzResearchTreeView>(button.parent());
			if (view != nullptr)
			{
				view->revealDuplicatedResearch();
			}
		});
	}

	m_traceBar = std::make_shared<WzResearchTraceBar>();
	attach(m_traceBar);
	m_traceBar->hide();

	m_traceExit = makeResearchQuietButton(WzString::fromUtf8(BACK_ARROW) + WzString::fromUtf8(_("Tree View")),
	                                       WzString::fromUtf8(_("Go back to the whole tree")));
	attach(m_traceExit);
	m_traceExit->hide();
	m_traceExit->addOnClickHandler([](W_BUTTON& button) {
		auto view = std::dynamic_pointer_cast<WzResearchTreeView>(button.parent());
		if (view != nullptr)
		{
			view->toggleFocus();
		}
	});

	// Last, so it is drawn over everything else
	m_resultsPanel = std::make_shared<WzSearchResultsPanel>();
	attach(m_resultsPanel);
	m_resultsPanel->hide();

	m_results = ScrollableListWidget::make();
	m_resultsPanel->attach(m_results);
}

void WzResearchTreeView::updateResults()
{
	const WzString query = m_searchBox ? m_searchBox->getString() : WzString();
	m_hits = m_searchIndex.find(query, MAX_RESULTS);

	m_results->clear();

	int widest = 0;
	std::vector<std::shared_ptr<W_BUTTON>> rows;
	for (size_t i = 0; i < m_hits.size(); ++i)
	{
		std::weak_ptr<WzResearchTreeView> weakSelf = std::dynamic_pointer_cast<WzResearchTreeView>(shared_from_this());
		auto row = makeResultRow(m_hits[i], [weakSelf, i]() {
			if (auto self = weakSelf.lock())
			{
				self->revealHit(i);
			}
		});
		widest = std::max(widest, row->idealWidth());
		rows.push_back(std::move(row));
	}
	const int chrome = RESULTS_PADDING * 2 + m_results->getScrollbarWidth();
	m_resultsWidth = std::max(SEARCH_WIDTH, std::min(RESULTS_MAX_WIDTH, widest + chrome));
	const int rowWidth = std::max(1, m_resultsWidth - chrome);
	for (auto& row : rows)
	{
		row->setGeometry(0, 0, rowWidth, RESULT_ROW_HEIGHT);
		m_results->addItem(row);
	}
	m_resultsPanel->show(!m_hits.empty() && !m_resultsPutAway);
	callCalcLayout();
	geometryChanged();
}

bool WzResearchTreeView::pointerOverResults() const
{
	if (m_resultsPanel == nullptr || !m_resultsPanel->visible())
	{
		return false;
	}
	const int pointerX = static_cast<int>(mouseX());
	const int pointerY = static_cast<int>(mouseY());
	return pointerX >= m_resultsPanel->screenPosX() && pointerX < m_resultsPanel->screenPosX() + m_resultsPanel->width()
		&& pointerY >= m_resultsPanel->screenPosY() && pointerY < m_resultsPanel->screenPosY() + m_resultsPanel->height();
}

void WzResearchTreeView::revealHit(size_t resultIndex)
{
	if (resultIndex >= m_hits.size() || m_canvas == nullptr)
	{
		return;
	}
	// A focused view holds one path, so a topic outside it can only be shown by going back to the whole tree first
	if (!m_canvas->revealResearch(m_hits[resultIndex].researchIndex) && m_canvas->isFocused())
	{
		m_canvas->toggleFocusOnSelection();
		updateTraceBar();
		geometryChanged();
		m_canvas->revealResearch(m_hits[resultIndex].researchIndex);
	}
	// Hide the results list, but keep it around (for later resuming of the search)
	m_resultsPutAway = true;
	if (m_resultsPanel)
	{
		m_resultsPanel->show(false);
	}
	geometryChanged();
	if (m_searchBox)
	{
		m_searchBox->stopEditing();
	}
}

// Points per second across every research facility the player has finished
static int researchOutputPerSecond(const ResearchTreeContext& context)
{
	if (context.source != ResearchTreeContext::Source::LivePlayerState || context.player != selectedPlayer)
	{
		return 0;
	}
	const StructureList *structures = interfaceStructList();
	if (structures == nullptr)
	{
		return 0;
	}
	int points = 0;
	for (const STRUCTURE *psStruct : *structures)
	{
		if (psStruct->pStructureType->type == REF_RESEARCH && psStruct->status == SS_BUILT)
		{
			points += getBuildingResearchPoints(psStruct);
		}
	}
	return points;
}

void WzResearchTreeView::toggleFocus()
{
	if (m_canvas == nullptr)
	{
		return;
	}
	m_canvas->toggleFocusOnSelection();
	updateTraceBar();
	geometryChanged();
}

void WzResearchTreeView::stepSelection(int delta)
{
	if (m_canvas)
	{
		m_canvas->stepSelection(delta);
	}
}

void WzResearchTreeView::updateTraceBar()
{
	if (m_traceBar == nullptr || m_canvas == nullptr)
	{
		return;
	}
	if (!m_canvas->isFocused() || !m_canvas->focusTarget())
	{
		m_traceBar->hide();
		if (m_traceExit) { m_traceExit->hide(); }
		return;
	}

	const ResearchFocus& focus = m_canvas->focus();
	WzString detail;
	if (focus.remaining == 0)
	{
		detail = WzString::format(_("Researched, %u topics in all"), static_cast<unsigned>(focus.topics));
	}
	else
	{
		detail = WzString::format(_("%u of %u topics left, %u power, %u research points, %d deep"),
		                          static_cast<unsigned>(focus.remaining), static_cast<unsigned>(focus.topics), static_cast<unsigned>(focus.remainingPower), static_cast<unsigned>(focus.remainingPoints), focus.criticalPath);
		const int rate = researchOutputPerSecond(m_context);
		if (rate > 0)
		{
			const uint64_t seconds = focus.remainingPoints / static_cast<uint64_t>(rate);
			auto timeStr = WzString::format("%u:%02u", static_cast<unsigned>(seconds / 60), static_cast<unsigned>(seconds % 60));
			detail += " - ";
			detail += WzString::format(_("about %s at the current %d/sec"),
			                          timeStr.toUtf8().c_str(), rate);
		}
	}
	m_traceBar->setTrace(WzString::fromUtf8(getLocalizedStatsName(&asResearch[*m_canvas->focusTarget()])), detail);
	m_traceBar->show();
	if (m_traceExit) { m_traceExit->show(); }
}

void WzResearchTreeView::focusSearch()
{
	if (m_searchBox == nullptr || m_searchBox->isEditing())
	{
		return;
	}
	W_CONTEXT context = W_CONTEXT::ZeroContext();
	m_searchBox->clicked(&context);
}

bool WzResearchTreeView::searchIsFocused() const
{
	return m_searchBox != nullptr && m_searchBox->isEditing();
}

void WzResearchTreeView::describeLab(optional<size_t> slot, bool pinned)
{
	if (m_canvas == nullptr || m_labStrip == nullptr)
	{
		return;
	}
	if (m_labPinned && !m_canvas->detailIsOpen())
	{
		m_labPinned = false;
	}
	// Requesting the one already pinned - toggle
	if (pinned && m_labPinned && slot == m_labPinnedSlot)
	{
		m_labPinned = false;
		m_canvas->closePopovers();
		return;
	}
	if (!slot.has_value())
	{
		if (!m_labPinned)
		{
			m_canvas->closePopovers();
		}
		return;
	}
	const auto subject = m_labStrip->subjectAt(slot.value());
	if (!subject.has_value())
	{
		if (!m_labPinned)
		{
			m_canvas->closePopovers();
		}
		return;
	}
	if (!pinned && m_labPinned)
	{
		return;
	}
	// Anchor the panel based on the lab button
	auto anchor = m_labStrip->slotWidget(slot.value());
	if (anchor == nullptr)
	{
		return;
	}
	m_labPinned = pinned;
	m_labPinnedSlot = pinned ? slot : nullopt;
	m_canvas->openDetailBeside(subject.value(), anchor, pinned);
}

void WzResearchTreeView::closePopovers()
{
	m_labPinned = false;
	if (m_canvas)
	{
		m_canvas->closePopovers();
	}
}

bool WzResearchTreeView::backOut()
{
	if (m_escapeHandled)
	{
		m_escapeHandled = false; // the search box cleared itself
		return true;
	}
	if (m_canvas == nullptr)
	{
		return false;
	}
	if (m_canvas->isFocused())
	{
		toggleFocus();
		return true;
	}
	if (m_canvas->selectedUnit())
	{
		m_canvas->clearSelection();
		return true;
	}
	return false;
}

void WzResearchTreeView::run(W_CONTEXT *)
{
	// The legend follows what the canvas holds (campaigns may only show found topics)
	if (m_legend && m_canvas && m_canvas->lanesShown() != m_lanesShown)
	{
		m_lanesShown = m_canvas->lanesShown();
		m_legend->setLanes(m_lanesShown);
		geometryChanged();
	}

	// The button follows the canvas (since, for example, the canvas picks its own density on open)
	if (m_names && m_canvas)
	{
		const bool labeled = m_canvas->isLabeled();
		if (labeled != m_namesShown)
		{
			m_namesShown = labeled;
			m_names->setState(labeled ? WBUT_CLICKLOCK : 0);
		}
	}

	if (m_searchBox == nullptr)
	{
		return;
	}
	const WzString query = m_searchBox->getString();
	if (query != m_lastQuery)
	{
		m_lastQuery = query;
		m_resultsPutAway = false;
		updateResults();
	}

	// If returning to the search box, restore the prior list results
	const bool editing = m_searchBox->isEditing();
	if (editing && !m_searchWasEditing && m_resultsPutAway)
	{
		m_resultsPutAway = false;
		updateResults();
	}
	// The results list belongs to the search box, so it goes away with it - but not while the pointer is on the results list itself
	// (Pressing a row takes the focus off the box, so we can't rely on that alone)
	if (!editing && !m_resultsPutAway && m_resultsPanel && m_resultsPanel->visible()
		&& !pointerOverResults())
	{
		m_resultsPutAway = true;
		m_resultsPanel->show(false);
		geometryChanged();
	}
	m_searchWasEditing = editing;


	if (m_labStrip && m_idleChip)
	{
		const size_t idle = m_labStrip->idleCount();
		const size_t labs = m_labStrip->labCount();
		if (idle != m_idleShown || labs != m_labsShown)
		{
			m_idleShown = idle;
			m_labsShown = labs;
			if (idle == 0)
			{
				m_idleChip->hide();
			}
			else
			{
				m_idleChip->setString(WzString::fromUtf8(astringf(ngettext("%u lab idle", "%u labs idle", idle), static_cast<unsigned>(idle))));
				m_idleChip->show();
			}
			geometryChanged();
		}
		m_idleChip->setTip(m_labStrip->describe());
	}

	if (m_wasteChip && m_canvas)
	{
		const size_t wasted = m_canvas->duplicatedTopics().size();
		if (wasted != m_wastedShown)
		{
			m_wastedShown = wasted;
			if (wasted == 0)
			{
				m_wasteChip->hide();
			}
			else
			{
				m_wasteChip->setString(WzString::fromUtf8(astringf(
					ngettext("%u topic doubled up", "%u topics doubled up", wasted), static_cast<unsigned>(wasted))));
				m_wasteChip->setTip(_("Two labs on one topic research it no faster. Click to find one."));
				m_wasteChip->show();
			}
			geometryChanged();
		}
	}
}

// Takes the player to the first thing they could start
void WzResearchTreeView::revealSomethingToResearch()
{
	if (m_canvas == nullptr)
	{
		return;
	}
	if (const auto topic = m_canvas->firstStartableTopic())
	{
		m_canvas->revealResearch(*topic);
	}
	else
	{
		addConsoleMessage(_("Nothing can be started right now"), DEFAULT_JUSTIFY, SYSTEM_MESSAGE);
	}
}

void WzResearchTreeView::revealDuplicatedResearch()
{
	if (m_canvas == nullptr || m_canvas->duplicatedTopics().empty())
	{
		return;
	}
	m_canvas->revealResearch(m_canvas->duplicatedTopics().front());
}

void WzResearchTreeView::setLabeled(bool labeled)
{
	if (m_canvas == nullptr)
	{
		return;
	}
	m_canvas->setLabeled(labeled);
}

void WzResearchTreeView::toggleLabeled()
{
	if (m_canvas)
	{
		setLabeled(!m_canvas->isLabeled());
	}
}

void WzResearchTreeView::display(int xOffset, int yOffset)
{
	if (m_toolbarHeight <= 0)
	{
		return;
	}
	const float x0 = static_cast<float>(xOffset + x());
	const float y0 = static_cast<float>(yOffset + y());
	const float x1 = x0 + static_cast<float>(width());
	const float y1 = y0 + static_cast<float>(m_toolbarHeight);
	pie_UniTransBoxFill(x0, y0, x1, y1, TOOLBAR_FILL);
	iV_Line(static_cast<int>(x0), static_cast<int>(y1), static_cast<int>(x1), static_cast<int>(y1), pal_RGBA(0, 0, 0, 150));
}

void WzResearchTreeView::geometryChanged()
{
	const int padX = OVERLAY_INSET;
	const int row = TOOLBAR_PAD;
	int top = 0;
	const int searchWidth = std::min(SEARCH_WIDTH, std::max(10, width() - padX * 2));
	if (m_searchBox)
	{
		m_searchBox->setGeometry(padX, row, searchWidth, TOP_ROW_HEIGHT);
	}
	if (m_names)
	{
		m_names->setGeometry(std::max(padX, width() - NAMES_BUTTON_WIDTH - padX),
		                     row, std::min(NAMES_BUTTON_WIDTH, width()), TOP_ROW_HEIGHT);
	}
	if (m_labStrip)
	{
		// Take what is left between the search box and the names toggle
		int at = padX + searchWidth + TOP_ROW_GAP * 2;
		if (m_idleChip && m_idleChip->visible())
		{
			m_idleChip->setGeometry(at, row, IDLE_CHIP_WIDTH, TOP_ROW_HEIGHT);
			at += IDLE_CHIP_WIDTH + TOP_ROW_GAP;
		}
		if (m_wasteChip && m_wasteChip->visible())
		{
			m_wasteChip->setGeometry(at, row, WASTE_CHIP_WIDTH, TOP_ROW_HEIGHT);
			at += WASTE_CHIP_WIDTH + TOP_ROW_GAP;
		}
		const int room = std::max(0, width() - NAMES_BUTTON_WIDTH - padX - TOP_ROW_GAP - at);
		m_labStrip->setGeometry(at, row, std::min(room, m_labStrip->idealWidth()), TOP_ROW_HEIGHT);
	}
	if (m_searchBox || m_perspectives || m_names || m_labStrip)
	{
		top = row + TOP_ROW_HEIGHT + TOOLBAR_PAD;
	}
	m_toolbarHeight = top;

	if (m_traceBar && m_traceBar->visible())
	{
		m_traceBar->setGeometry(0, top, width(), TRACE_BAR_HEIGHT);
		if (m_traceExit && m_traceExit->visible())
		{
			m_traceExit->setGeometry(std::max(0, width() - TRACE_EXIT_WIDTH - OVERLAY_INSET),
			                         top + (TRACE_BAR_HEIGHT - TOP_ROW_HEIGHT) / 2,
			                         std::min(TRACE_EXIT_WIDTH, width()), TOP_ROW_HEIGHT);
		}
		top += TRACE_BAR_HEIGHT;
	}
	int bottom = height();
	if (m_legend)
	{
		const int legendHeight = m_legend->heightForWidth(width());
		bottom = std::max(top, bottom - legendHeight);
		m_legend->setGeometry(0, bottom, width(), legendHeight);
	}
	if (m_canvas)
	{
		m_canvas->setGeometry(0, top, width(), std::max(0, bottom - top));
	}
	if (m_resultsPanel)
	{
		const int rows = static_cast<int>(m_results ? m_results->numItems() : 0);
		const int listHeight = std::min(RESULTS_MAX_HEIGHT, std::max(RESULT_ROW_HEIGHT, rows * RESULT_ROW_HEIGHT)) + RESULTS_PADDING * 2;
		m_resultsPanel->setGeometry(padX, m_toolbarHeight, std::min(std::max(m_resultsWidth, SEARCH_WIDTH), width()),
		                            std::min(listHeight, std::max(0, height() - m_toolbarHeight)));
	}
}

int researchTreeContentInset()
{
	return OVERLAY_INSET;
}

int researchTreeSwitcherTextInset()
{
	return SWITCHER_TEXT_INSET;
}

std::shared_ptr<WIDGET> WzResearchTreeView::seatSwitcher() const
{
	return m_perspectives;
}

int WzResearchTreeView::ownSeatChipWidth()
{
	return OWN_SEAT_CHIP_WIDTH;
}

int32_t WzResearchTreeView::idealWidth()
{
	return SEARCH_WIDTH;
}

int32_t WzResearchTreeView::idealHeight()
{
	return TOP_ROW_HEIGHT + TOP_ROW_GAP;
}
