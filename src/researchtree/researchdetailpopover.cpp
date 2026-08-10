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
 *  Research detail popover contents.
 */

#include "researchdetailpopover.h"

#include "lib/widget/button.h"
#include "lib/widget/label.h"
#include "lib/widget/paragraph.h"
#include "lib/widget/scrollablelist.h"
#include "lib/ivis_opengl/pieblitfunc.h"
#include "lib/ivis_opengl/piepalette.h"
#include "lib/framework/input.h"

#include "../research.h"
#include "../component.h"
#include "../multiplay.h"
#include "../mapdisplay.h"
#include "../structure.h"
#include "../game_world.h"
#include "../intimage.h"
#include "researcheffecttext.h"
#include "../messagedef.h"

#include <algorithm>

static constexpr int PADDING = 10;
static constexpr int MODEL_SIZE = 74;
static constexpr int MIN_BODY_HEIGHT = 60;
static constexpr int BODY_LINE_SPACING = 2;

static PIELIGHT restingButtonFill()
{
	PIELIGHT fill = WZCOL_TRANSPARENT_BOX;
	const float coat = static_cast<float>(fill.byte.a) / 255.f;
	fill.byte.a = static_cast<uint8_t>((1.f - (1.f - coat) * (1.f - coat)) * 255.f);
	return fill;
}

static PIELIGHT panelColor(uint8_t alpha)
{
	PIELIGHT color = WZCOL_FORM_TEXT;
	color.byte.a = alpha;
	return color;
}

static PIELIGHT panelBorderColor() { return panelColor(140); }
static constexpr int ACTIONS_HEIGHT = 18;
static constexpr int BADGE_HEIGHT = 15;
static constexpr int TITLE_HEIGHT = 18;
static constexpr int STATS_Y = 41;
static constexpr int STATUS_HEIGHT = 14;
static constexpr float READABLE_COLOR_LUMINANCE = 140.f;
static constexpr int BADGE_GAP = 5;

// Rotating 3d model of what the topic produces
class WzResearchModel : public WIDGET
{
public:
	static std::shared_ptr<WzResearchModel> make(uint16_t researchIndex)
	{
		class make_shared_enabler : public WzResearchModel {};
		auto widget = std::make_shared<make_shared_enabler>();
		widget->m_researchIndex = researchIndex;
		return widget;
	}

	void display(int xOffset, int yOffset) override
	{
		const int x0 = xOffset + x();
		const int y0 = yOffset + y();
		pie_UniTransBoxFill(static_cast<float>(x0), static_cast<float>(y0),
		                    static_cast<float>(x0 + width()), static_cast<float>(y0 + height()),
		                    pal_RGBA(0, 0, 0, 120));
		if (m_researchIndex < asResearch.size())
		{
			renderResearchFittedToBuffer(&asResearch[m_researchIndex], x0 + width() / 2, y0 + height() / 2,
			                             static_cast<UDWORD>(std::min(width(), height())));
		}
	}

private:
	uint16_t m_researchIndex = 0;
};

class WzResearchTitle : public WIDGET
{
public:
	static std::shared_ptr<WzResearchTitle> make(const WzString& text)
	{
		auto widget = std::make_shared<WzResearchTitle>();
		widget->m_string = text;
		widget->m_text.setText(text, TITLE_FONT);
		// Stored before anything shrinks it, since the panel sizes itself against this
		widget->m_natural = widget->m_text.width();
		return widget;
	}

	int32_t idealWidth() override { return m_natural; }
	int32_t idealHeight() override { return TITLE_HEIGHT; }

	void setTip(std::string string) override { m_tip = std::move(string); }
	std::string getTip() override { return m_tip; }

	void display(int xOffset, int yOffset) override
	{
		if (m_fittedTo != width())
		{
			m_fittedTo = width();
			iV_fonts font = TITLE_FONT;
			m_text.setText(m_string, font);
			while (m_text.width() > width())
			{
				const auto smaller = iV_ShrinkFont(font);
				if (!smaller.has_value())
				{
					break;
				}
				font = smaller.value();
				m_text.setText(m_string, font);
			}
			m_cut = m_text.width() > width();
			setTip(m_cut ? m_string.toUtf8() : std::string());
		}

		const float baseline = static_cast<float>(yOffset + y())
			+ (static_cast<float>(height()) - m_text.lineSize()) / 2.f - m_text.aboveBase();
		const float left = static_cast<float>(xOffset + x());
		if (!m_cut)
		{
			m_text.render(left, baseline, WZCOL_TEXT_BRIGHT);
			return;
		}
		const int shown = width() - iV_GetEllipsisWidth(m_text.getFontID()) - 2;
		if (shown <= 0)
		{
			return;
		}
		m_text.renderClipped(Vector2f(left, baseline), WZCOL_TEXT_BRIGHT,
		                     WzRect(static_cast<int>(left), yOffset + y(), shown, height()), shown);
		iV_DrawEllipsis(m_text.getFontID(), Vector2f(left + static_cast<float>(shown) + 2.f, baseline), WZCOL_TEXT_BRIGHT);
	}

private:
	static constexpr iV_fonts TITLE_FONT = font_regular_bold;

	WzString m_string;
	WzText m_text;
	int m_natural = 0;
	int m_fittedTo = -1;
	bool m_cut = false;
	std::string m_tip;
};

class WzResearchStat : public WIDGET
{
public:
	static std::shared_ptr<WzResearchStat> make(const WzString& label, const WzString& value,
	                                            optional<uint16_t> icon, const std::string& tip)
	{
		auto widget = std::make_shared<WzResearchStat>();
		widget->m_labelText.setText(label, LABEL_FONT);
		widget->m_valueText.setText(value, VALUE_FONT);
		widget->m_icon = icon;
		widget->m_tip = tip;
		return widget;
	}

	int32_t idealWidth() override
	{
		const int value = iconSize() + (m_icon.has_value() ? ICON_GAP : 0) + m_valueText.width();
		return std::max(m_labelText.width(), value) + PAD_X * 2;
	}
	int32_t idealHeight() override { return STAT_HEIGHT; }

	void setTip(std::string string) override { m_tip = std::move(string); }
	std::string getTip() override { return m_tip; }

	void display(int xOffset, int yOffset) override
	{
		const float x0 = static_cast<float>(xOffset + x());
		const float y0 = static_cast<float>(yOffset + y());
		pie_DrawRoundedBox(x0, y0, x0 + static_cast<float>(width()), y0 + static_cast<float>(height()),
		                   pal_RGBA(0, 0, 0, 0), 3.f, EDGE, 1.f);

		const int room = width() - PAD_X * 2;
		renderCut(m_labelText, x0 + PAD_X, y0 + PAD_TOP - m_labelText.aboveBase(), EDGE, room, y0);

		const float valueTop = y0 + PAD_TOP + static_cast<float>(m_labelText.lineSize());
		float at = x0 + PAD_X;
		const bool withIcon = m_icon.has_value()
			&& (iconSize() + ICON_GAP - static_cast<int>(ICON_NUDGE) + m_valueText.width()) <= room;
		if (withIcon)
		{
			// Centered on the ascent rather than the line, because a line reserves room
			// under the baseline for descenders and a row of digits has none.
			const float size = static_cast<float>(iconSize());
			const float ascent = static_cast<float>(-m_valueText.aboveBase());
			iV_DrawImageTint(IntImages, *m_icon, at - ICON_NUDGE,
			                 valueTop + (ascent - size) / 2.f,
			                 WZCOL_TEXT_BRIGHT, Vector2f(size, size));
			at += size + static_cast<float>(ICON_GAP) - ICON_NUDGE;
		}
		renderCut(m_valueText, at, valueTop - m_valueText.aboveBase(), WZCOL_TEXT_BRIGHT,
		          static_cast<int>(static_cast<float>(x0 + PAD_X) + static_cast<float>(room) - at), y0);
	}

	static constexpr int STAT_HEIGHT = 28;

private:
	static constexpr iV_fonts LABEL_FONT = font_bar;
	static constexpr iV_fonts VALUE_FONT = font_small;
	static constexpr int PAD_X = 5;
	static constexpr int PAD_TOP = 2;
	static constexpr int ICON_GAP = 4;
	static constexpr float ICON_NUDGE = 2.f;

	int iconSize() { return m_icon.has_value() ? m_valueText.lineSize() - 3 : 0; }

	void renderCut(WzText& text, float atX, float baseline, PIELIGHT color, int room, float boxTop)
	{
		if (room <= 0)
		{
			return;
		}
		const bool cut = text.width() > room;
		const int shown = cut ? room - iV_GetEllipsisWidth(text.getFontID()) - 2 : room;
		if (shown <= 0)
		{
			return;
		}
		text.renderClipped(Vector2f(atX, baseline), color,
		                   WzRect(static_cast<int>(atX), static_cast<int>(boxTop), shown, height()), shown);
		if (cut)
		{
			iV_DrawEllipsis(text.getFontID(), Vector2f(atX + static_cast<float>(shown) + 2.f, baseline), color);
		}
	}

	static const PIELIGHT EDGE;

	WzText m_labelText;
	WzText m_valueText;
	optional<uint16_t> m_icon;
	std::string m_tip;
};

const PIELIGHT WzResearchStat::EDGE = pal_RGBA(191, 191, 191, 190);

// One wrapped line of the WzResearchChecklist
class WzResearchChecklistRow : public WIDGET
{
public:
	struct Entry
	{
		WzText text;
		bool done = false;
		int at = 0;		// where it starts, from the row's left edge
	};

	static std::shared_ptr<WzResearchChecklistRow> make(std::vector<Entry> entries)
	{
		auto row = std::make_shared<WzResearchChecklistRow>();
		row->m_entries = std::move(entries);
		return row;
	}

	// Text height of a capital, the band a line of text reads as filling. No font metric
	// provides it, and the ascent reaches well above the capitals, so a mark placed by that
	// stands over the text rather than in it. The measurement rounds up, hence the adjustment.
	static float capPx() { return static_cast<float>(iV_GetTextHeight("H", font_small)) - 1.f; }

	// A ring around that band rather than inside it, so it encloses the line.
	static float markPx() { return capPx() + 2.f; }
	static float markTopPx(float baseline) { return baseline - capPx() - 1.f; }

	static constexpr int MARK_GAP = 4;
	static constexpr int ITEM_GAP = 14;
	static const PIELIGHT DONE_COLOR;

	void display(int xOffset, int yOffset) override
	{
		const float x0 = static_cast<float>(xOffset + x());
		const float y0 = static_cast<float>(yOffset + y());
		for (auto& entry : m_entries)
		{
			const float left = x0 + static_cast<float>(entry.at);
			const float baseline = y0 - static_cast<float>(entry.text.aboveBase());
			const float mark = markPx();
			const float markTop = markTopPx(baseline);
			const PIELIGHT color = entry.done ? DONE_COLOR : WZCOL_TEXT_MEDIUM;
			pie_DrawRoundedBox(left, markTop, left + mark, markTop + mark,
			                   pal_RGBA(0, 0, 0, 0), mark / 2.f, color, 1.f);
			if (entry.done)
			{
				iV_DrawImageTint(IntImages, IMAGE_INTFAC_CHECK_CIRCLE, left, markTop,
				                 color, Vector2f(mark, mark));
			}
			entry.text.render(left + mark + MARK_GAP, baseline, color);
		}
	}

private:
	std::vector<Entry> m_entries;
};

const PIELIGHT WzResearchChecklistRow::DONE_COLOR = pal_Colour(140, 210, 150);

// What a topic requires, displayed as a checklist: a check for what is done and an empty
// circle for what is not. Wrapped across the width instead of just one to a line.
class WzResearchChecklist : public WIDGET
{
public:
	static std::shared_ptr<WzResearchChecklist> make(const WzString& heading, std::vector<std::pair<WzString, bool>> items)
	{
		auto widget = std::make_shared<WzResearchChecklist>();
		widget->m_items = std::move(items);
		widget->m_metrics.setText(heading.toUpper(), font_small);
		widget->m_heading = std::make_shared<W_LABEL>();
		widget->m_heading->setFont(font_small, pal_Colour(255, 220, 140));
		widget->m_heading->setString(heading.toUpper());
		widget->m_heading->setTransparentToMouse(true);
		widget->attach(widget->m_heading);
		return widget;
	}

	int32_t idealHeight() override { return m_height; }

	// Enable scrolling by internal line
	nonstd::optional<std::vector<uint32_t>> getScrollSnapOffsets() override
	{
		std::vector<uint32_t> offsets = { 0 };
		for (int row = 0; row < m_rows; ++row)
		{
			offsets.push_back(static_cast<uint32_t>(LEAD_GAP + (row + 1) * linePitch()));
		}
		return offsets;
	}

	void displayRecursive(WidgetGraphicsContext const& context) override
	{
		WIDGET::displayRecursive(context.setAllowChildDisplayRecursiveIfSelfClipped(true));
	}

	void geometryChanged() override
	{
		if (width() == m_laidOutFor)
		{
			return;
		}
		// The list this sits in only sets a child's width and keeps the height it finds,
		// so the height is updated here after reflowing.
		m_laidOutFor = width();
		m_height = reflow(width());
		if (height() != m_height)
		{
			setGeometry(x(), y(), width(), m_height);
		}
	}

private:
	// A blank line before the heading, as the running text puts before its own.
	// Two line spacings: one closing the item above, one for the blank line itself.
	static constexpr int LEAD_GAP = BODY_LINE_SPACING * 2;

	// The pitch the body's prose sets its lines at, so a checklist line lands where a line of text would.
	int linePitch()
	{
		// Paragraph spaces by the full text height plus its line spacing.
		return -m_metrics.aboveBase() - m_metrics.belowBase() + BODY_LINE_SPACING;
	}

	int entryWidth(const WzString& name)
	{
		return static_cast<int>(WzResearchChecklistRow::markPx()) + WzResearchChecklistRow::MARK_GAP
			+ static_cast<int>(iV_GetTextWidth(name, font_small)) + WzResearchChecklistRow::ITEM_GAP;
	}

	// Wrap the items across the width and give each line a widget.
	int reflow(int forWidth)
	{
		for (const auto& row : m_rowWidgets)
		{
			detach(row);
		}
		m_rowWidgets.clear();

		const int pitch = linePitch();
		m_heading->setGeometry(0, LEAD_GAP, std::max(10, forWidth),
		                       -m_metrics.aboveBase() - m_metrics.belowBase());

		const int room = std::max(20, forWidth);
		int at = 0;
		int rows = m_items.empty() ? 0 : 1;
		std::vector<WzResearchChecklistRow::Entry> entries;
		const auto closeRow = [&]() {
			if (entries.empty())
			{
				return;
			}
			auto row = WzResearchChecklistRow::make(std::move(entries));
			entries.clear();
			row->setGeometry(0, LEAD_GAP + rows * pitch, room, pitch);
			attach(row);
			m_rowWidgets.push_back(row);
		};
		for (const auto& item : m_items)
		{
			const int itemWidth = entryWidth(item.first);
			if (at > 0 && at + itemWidth > room)
			{
				closeRow();
				at = 0;
				++rows;
			}
			WzResearchChecklistRow::Entry entry;
			entry.done = item.second;
			entry.at = at;
			entry.text.setText(item.first, font_small);
			entries.push_back(std::move(entry));
			at += itemWidth;
		}
		closeRow();
		m_rows = rows;
		return LEAD_GAP + (rows + 1) * pitch;
	}

	std::vector<std::pair<WzString, bool>> m_items;
	WzText m_metrics;
	std::shared_ptr<W_LABEL> m_heading;
	std::vector<std::shared_ptr<WzResearchChecklistRow>> m_rowWidgets;
	int m_height = 0;
	int m_rows = 0;
	int m_laidOutFor = -1;
};

// A player's color, lifted until it can be read as text.
// (Team colors are chosen to be told apart on the map, not necessarily easily read on a dark plate.)
static PIELIGHT readableTeamColor(PIELIGHT color)
{
	const float lum = 0.2126f * color.byte.r + 0.7152f * color.byte.g + 0.0722f * color.byte.b;
	if (lum >= READABLE_COLOR_LUMINANCE)
	{
		return color;
	}
	const float towards = (READABLE_COLOR_LUMINANCE - lum) / (255.f - lum);
	const auto lift = [towards](uint8_t channel) {
		return static_cast<uint8_t>(channel + towards * (255.f - static_cast<float>(channel)));
	};
	return pal_RGBA(lift(color.byte.r), lift(color.byte.g), lift(color.byte.b), color.byte.a);
}

class WzResearchStatusLine : public WIDGET
{
public:
	static std::shared_ptr<WzResearchStatusLine> make(const WzString& standing, PIELIGHT color,
	                                                  const WzString& prefix,
	                                                  const std::vector<uint32_t>& players)
	{
		auto widget = std::make_shared<WzResearchStatusLine>();
		widget->m_color = color;
		widget->m_standing.setText(standing, font_small);
		widget->m_prefix.setText(prefix, font_small);
		for (size_t i = 0; i < players.size(); ++i)
		{
			Name name;
			name.color = readableTeamColor(pal_GetTeamColour(getPlayerColour(players[i])));
			// The separator belongs to the name before it, so it takes that player's color
			// and sits against the name rather than a space away
			WzString text = WzString::fromUtf8(getPlayerName(players[i]));
			if (i + 1 < players.size())
			{
				text += WzString::fromUtf8(", ");
			}
			name.text.setText(text, font_small);
			widget->m_names.push_back(std::move(name));
		}
		widget->m_crowd.setText(WzString::fromUtf8(
			astringf(ngettext("%u player", "%u players", players.size()), static_cast<unsigned>(players.size()))), font_small);
		return widget;
	}

	int32_t idealHeight() override { return STATUS_HEIGHT; }

	int32_t idealWidth() override
	{
		int32_t wanted = m_standing.width();
		if (!m_names.empty())
		{
			wanted += static_cast<int32_t>(GAP) + m_prefix.width() + static_cast<int32_t>(SPACE);
			for (auto& name : m_names)
			{
				wanted += name.text.width();
			}
		}
		return wanted;
	}

	void display(int xOffset, int yOffset) override
	{
		const float left = static_cast<float>(xOffset + x());
		const float right = left + static_cast<float>(width());
		const float baseline = static_cast<float>(yOffset + y())
			+ (static_cast<float>(height()) - static_cast<float>(m_standing.lineSize())) / 2.f
			- static_cast<float>(m_standing.aboveBase());
		float at = left;
		const auto put = [&](WzText& text, PIELIGHT color) {
			text.render(at, baseline, color);
			at += static_cast<float>(text.width());
		};
		const auto fits = [&](const WzText& text) {
			return at + static_cast<float>(const_cast<WzText&>(text).width()) <= right;
		};
		if (!fits(m_standing))
		{
			m_standing.renderClipped(Vector2f(at, baseline), m_color,
			                         WzRect(static_cast<int>(at), 0, static_cast<int>(right - at), height()));
			return;
		}
		put(m_standing, m_color);
		if (m_names.empty())
		{
			return;
		}
		at += GAP;
		if (!fits(m_prefix))
		{
			return;
		}
		put(m_prefix, WZCOL_TEXT_BRIGHT);
		at += SPACE;
		float wanted = 0.f;
		for (auto& name : m_names)
		{
			wanted += static_cast<float>(name.text.width());
		}
		if (at + wanted <= right)
		{
			for (auto& name : m_names)
			{
				put(name.text, name.color);
			}
			return;
		}
		put(m_crowd, WZCOL_TEXT_BRIGHT);
	}

private:
	struct Name
	{
		WzText text;
		PIELIGHT color;
	};

	static constexpr float GAP = 8.f;
	static constexpr float SPACE = 4.f;

	WzText m_standing;
	PIELIGHT m_color = WZCOL_TEXT_MEDIUM;
	WzText m_prefix;
	WzText m_crowd;			// the count, for when the names will not fit
	std::vector<Name> m_names;
};

class WzResearchBadge : public WIDGET
{
public:
	static std::shared_ptr<WzResearchBadge> make(const WzString& text, PIELIGHT fill, PIELIGHT textColor)
	{
		class make_shared_enabler : public WzResearchBadge {};
		auto widget = std::make_shared<make_shared_enabler>();
		widget->m_fill = fill;
		widget->m_textColor = textColor;
		widget->m_text.setText(text, font_small);
		return widget;
	}

	int32_t idealWidth() override { return m_text.width() + 14; }

	void display(int xOffset, int yOffset) override
	{
		const float x0 = static_cast<float>(x() + xOffset);
		const float y0 = static_cast<float>(y() + yOffset);
		const float x1 = x0 + static_cast<float>(width());
		const float y1 = y0 + static_cast<float>(height());
		pie_DrawRoundedBox(x0, y0, x1, y1, m_fill, static_cast<float>(height()) / 2.f, m_textColor, 1.f);
		m_text.renderClipped(Vector2f(x0 + 7.f, y0 + (static_cast<float>(height()) - static_cast<float>(m_text.lineSize())) / 2.f
		                                        - static_cast<float>(m_text.aboveBase())),
		                     m_textColor, WzRect(static_cast<int>(x0) + 4, static_cast<int>(y0), width() - 8, height()));
	}

private:
	PIELIGHT m_fill;
	PIELIGHT m_textColor;
	WzText m_text;
};

std::shared_ptr<WzResearchDetailContents> WzResearchDetailContents::make(std::shared_ptr<WIDGET> model, std::shared_ptr<WIDGET> title, std::shared_ptr<WIDGET> subtitle, std::shared_ptr<WIDGET> group, std::shared_ptr<WIDGET> status, std::vector<std::shared_ptr<WIDGET>> stats, std::shared_ptr<WIDGET> body, std::shared_ptr<WIDGET> actions, int32_t contentWidth)
{
	class make_shared_enabler : public WzResearchDetailContents {};
	auto widget = std::make_shared<make_shared_enabler>();
	widget->m_model = std::move(model);
	widget->m_title = std::move(title);
	widget->m_subtitle = std::move(subtitle);
	widget->m_group = std::move(group);
	widget->m_status = std::move(status);
	widget->m_stats = std::move(stats);
	widget->m_body = std::move(body);
	widget->m_actions = std::move(actions);
	// A panel with nothing to say about its subject has no model, no line under the title, and no body
	if (widget->m_model)
	{
		widget->attach(widget->m_model);
	}
	if (widget->m_title)
	{
		widget->attach(widget->m_title);
	}
	if (widget->m_subtitle)
	{
		widget->attach(widget->m_subtitle);
	}
	if (widget->m_group)
	{
		widget->attach(widget->m_group);
	}
	if (widget->m_status)
	{
		widget->attach(widget->m_status);
	}
	for (const auto& stat : widget->m_stats)
	{
		widget->attach(stat);
	}
	if (widget->m_body)
	{
		widget->attach(widget->m_body);
	}
	if (widget->m_actions)
	{
		widget->attach(widget->m_actions);
	}
	widget->setGeometry(0, 0, contentWidth, 10);
	widget->recalcHeight();
	return widget;
}

void WzResearchDetailContents::display(int xOffset, int yOffset)
{
	const int x0 = xOffset + x();
	const int y0 = yOffset + y();

	// The popover may be click-through, in which case the widget system never reports the pointer as over it.
	// Compare against its own rect.
	const int pointerX = static_cast<int>(mouseX());
	const int pointerY = static_cast<int>(mouseY());
	const bool pointerOver = pointerX >= screenPosX() && pointerX < screenPosX() + width()
		&& pointerY >= screenPosY() && pointerY < screenPosY() + height();
	const uint8_t alpha = pointerOver ? 255 : m_restingAlpha;

	pie_DrawRoundedBox(static_cast<float>(x0), static_cast<float>(y0),
	                   static_cast<float>(x0 + width()), static_cast<float>(y0 + height()),
	                   pal_RGBA(24, 24, 24, alpha), 6.f, panelBorderColor(), 1.f);
	if (anythingUnderHeader())
	{
		const int dividerY = y0 + PADDING + headerHeight() + PADDING / 2;
		iV_Line(x0 + PADDING, dividerY, x0 + width() - PADDING, dividerY, panelColor(90));
		// A band has a line under it as well as over it
		if (statusBarHeight() > 0)
		{
			const int closingY = dividerY + statusBarHeight();
			iV_Line(x0 + PADDING, closingY, x0 + width() - PADDING, closingY, panelColor(90));
		}
	}
}

void WzResearchDetailContents::geometryChanged()
{
	const int textX = PADDING + (m_model ? MODEL_SIZE + PADDING : 0);
	const int textWidth = std::max(10, width() - textX - PADDING);
	if (m_model)
	{
		m_model->setGeometry(PADDING, PADDING, MODEL_SIZE, MODEL_SIZE);
	}
	if (m_title)
	{
		m_title->setGeometry(textX, PADDING + 2, textWidth, TITLE_HEIGHT);
	}
	// The line under the title names the progression, if the topic is in one.
	// The badge leads and the subtitle (if any) follows it.
	int subtitleX = textX;
	const int subtitleY = PADDING + 21;
	if (m_group)
	{
		const int groupWidth = std::min(m_group->idealWidth(), std::max(10, width() - subtitleX - PADDING));
		m_group->setGeometry(subtitleX, subtitleY, groupWidth, BADGE_HEIGHT);
		subtitleX += groupWidth + BADGE_GAP;
	}
	if (m_subtitle)
	{
		m_subtitle->setGeometry(subtitleX, subtitleY + 1, std::max(10, width() - subtitleX - PADDING), 14);
	}
	// The stats under the progression, then the state under those, so the header reads:
	// name, progression, numbers, status.
	int statsBottom = PADDING + STATS_Y;
	if (!m_stats.empty())
	{
		int wanted = 0;
		for (const auto& stat : m_stats)
		{
			wanted += stat->idealWidth();
		}
		const int gaps = static_cast<int>(m_stats.size() - 1) * BADGE_GAP;
		const int room = std::max(30, textWidth);
		int at = textX;
		for (size_t i = 0; i < m_stats.size(); ++i)
		{
			int statWidth = m_stats[i]->idealWidth();
			if (wanted + gaps > room)
			{
				statWidth = std::max(20, (room - gaps) / static_cast<int>(m_stats.size()));
			}
			m_stats[i]->setGeometry(at, PADDING + STATS_Y, statWidth, WzResearchStat::STAT_HEIGHT);
			at += statWidth + BADGE_GAP;
		}
		statsBottom = PADDING + STATS_Y + WzResearchStat::STAT_HEIGHT;
	}
	if (m_status)
	{
		if (statusInBar())
		{
			m_status->setGeometry(PADDING, PADDING + headerHeight() + PADDING,
			                      std::max(10, width() - PADDING * 2), STATUS_HEIGHT);
		}
		else
		{
			m_status->setGeometry(textX, PADDING + STATS_Y,
			                      std::min(m_status->idealWidth(), std::max(10, width() - textX - PADDING)), BADGE_HEIGHT);
		}
	}
	(void)statsBottom;
	const int bodyY = PADDING + headerHeight() + statusBarHeight() + PADDING;
	const int actionsRoom = m_actions ? ACTIONS_HEIGHT + PADDING : 0;
	if (m_body)
	{
		m_body->setGeometry(PADDING, bodyY, std::max(10, width() - PADDING * 2), std::max(10, height() - bodyY - PADDING - actionsRoom));
	}
	if (m_actions)
	{
		m_actions->setGeometry(PADDING, height() - PADDING - ACTIONS_HEIGHT, std::max(10, width() - PADDING * 2), ACTIONS_HEIGHT);
	}
}

void WzResearchDetailContents::setMaxHeight(int32_t maxHeight)
{
	m_maxHeight = maxHeight;
	recalcHeight();
}

int WzResearchDetailContents::headerHeight() const
{
	int text = m_title ? TITLE_HEIGHT : 0;
	if (!m_stats.empty())
	{
		text = STATS_Y + WzResearchStat::STAT_HEIGHT;
	}
	else if (m_status)
	{
		text = STATS_Y + BADGE_HEIGHT;
	}
	return std::max(m_model ? MODEL_SIZE : 0, text);
}

int WzResearchDetailContents::statusBarHeight() const
{
	return (m_status && statusInBar()) ? PADDING + STATUS_HEIGHT : 0;
}

bool WzResearchDetailContents::anythingUnderHeader() const
{
	return statusBarHeight() > 0 || m_actions != nullptr
		|| (m_body != nullptr && m_body->idealHeight() > 0);
}

void WzResearchDetailContents::recalcHeight()
{
	const int bodyY = PADDING + headerHeight() + statusBarHeight() + PADDING;
	const int actionsRoom = m_actions ? ACTIONS_HEIGHT + PADDING : 0;
	if (!m_body || m_body->idealHeight() <= 0)
	{
		setGeometry(x(), y(), width(), bodyY + (m_actions ? PADDING / 2 + actionsRoom : 0));
		return;
	}
	const int chrome = bodyY + PADDING + actionsRoom;
	const int bodyRoom = std::max(MIN_BODY_HEIGHT, m_maxHeight - chrome);
	setGeometry(x(), y(), width(), chrome + std::min(m_body->idealHeight(), bodyRoom));
}

// ---------------------------------------------------------------------------
// MARK: - Text
// ---------------------------------------------------------------------------

// Combined output of every research facility the player has running
static int playerResearchRate(uint32_t player)
{
	if (player >= MAX_PLAYERS)
	{
		return 0;
	}
	int rate = 0;
	for (const STRUCTURE *psStruct : gameWorld.objects.structures[player])
	{
		if (psStruct->pStructureType->type == REF_RESEARCH && psStruct->status == SS_BUILT)
		{
			rate += getBuildingResearchPoints(psStruct);
		}
	}
	return rate;
}

static WzString formatDuration(uint32_t seconds)
{
	return WzString::format("%u:%02u", seconds / 60, seconds % 60);
}

static void addSection(const std::shared_ptr<Paragraph>& body, const WzString& heading)
{
	body->setFontColour(pal_Colour(255, 220, 140));
	body->addText("\n" + heading.toUpper() + "\n");
	body->setFontColour(WZCOL_TEXT_MEDIUM);
}

class WzResearchActionButton : public W_BUTTON
{
public:
	void display(int xOffset, int yOffset) override
	{
		const float x0 = static_cast<float>(x() + xOffset);
		const float y0 = static_cast<float>(y() + yOffset);
		const float x1 = x0 + static_cast<float>(width());
		const float y1 = y0 + static_cast<float>(height());

		const bool disabled = (getState() & WBUT_DISABLE) != 0;
		const bool down = !disabled && (getState() & (WBUT_DOWN | WBUT_LOCK | WBUT_CLICKLOCK)) != 0;
		const bool highlight = !disabled && (getState() & WBUT_HIGHLIGHT) != 0;

		PIELIGHT fill;
		if (m_quiet)
		{
			fill = down ? panelColor(70) : (highlight ? panelColor(40) : panelColor(18));
		}
		else
		{
			fill = down ? WZCOL_MENU_SCORE_BUILT : (highlight ? pal_RGBA(39, 49, 185, 150) : restingButtonFill());
		}
		PIELIGHT border = down || highlight ? WZCOL_TEXT_BRIGHT : panelBorderColor();
		PIELIGHT text = highlight || down ? WZCOL_TEXT_BRIGHT : WZCOL_FORM_TEXT;
		if (disabled)
		{
			fill = WZCOL_TRANSPARENT_BOX;
			border = panelColor(70);
			text.byte.a = text.byte.a / 2;
		}
		pie_DrawRoundedBox(x0, y0, x1, y1, fill, 4.f, border, 1.f);

		if (!pText.isEmpty())
		{
			m_text.setText(pText, FontID);
			m_text.render(x0 + (static_cast<float>(width()) - static_cast<float>(m_text.width())) / 2.f,
			              y0 + (static_cast<float>(height()) - static_cast<float>(m_text.lineSize())) / 2.f - static_cast<float>(m_text.aboveBase()),
			              text);
		}
	}

	void setQuiet(bool quiet) { m_quiet = quiet; }

private:
	WzText m_text;
	bool m_quiet = false;
};

static std::shared_ptr<W_BUTTON> makeButton(const WzString& text, const WzString& tip, bool quiet)
{
	auto button = std::make_shared<WzResearchActionButton>();
	button->setString(text);
	button->FontID = font_small;
	button->setTip(tip.toUtf8());
	button->setQuiet(quiet);
	return button;
}

std::shared_ptr<W_BUTTON> makeResearchActionButton(const WzString& text, const WzString& tip)
{
	return makeButton(text, tip, false);
}

std::shared_ptr<W_BUTTON> makeResearchQuietButton(const WzString& text, const WzString& tip)
{
	return makeButton(text, tip, true);
}

class WzResearchActionRow : public WIDGET
{
public:
	void geometryChanged() override
	{
		int at = 0;
		for (auto& child : children())
		{
			const int childWidth = std::min(BUTTON_WIDTH, std::max(0, width() - at));
			child->setGeometry(at, 0, childWidth, height());
			at += childWidth + BUTTON_GAP;
		}
	}

private:
	static constexpr int BUTTON_WIDTH = 110;
	static constexpr int BUTTON_GAP = 6;
};

// A panel with nothing to offer returns nullptr.
static std::shared_ptr<WIDGET> makeActionRow(const ResearchDetailActions& actions)
{
	if (!actions.onTracePath && !actions.onResearch)
	{
		return nullptr;
	}
	auto row = std::make_shared<WzResearchActionRow>();

	if (actions.onResearch)
	{
		auto research = makeResearchActionButton(_("Research"), _("Start this in a research facility"));
		row->attach(research);
		const auto onResearch = actions.onResearch;
		research->addOnClickHandler([onResearch](W_BUTTON& button) {
			onResearch(button.shared_from_this());
		});
	}

	if (actions.onTracePath)
	{
		auto path = makeResearchQuietButton(actions.tracingPath ? _("Whole tree") : _("Trace path"),
		                                    actions.tracingPath ? _("Show the whole tree again") : _("Show only what this topic needs"));
		row->attach(path);
		const auto onTracePath = actions.onTracePath;
		path->addOnClickHandler([onTracePath](W_BUTTON&) {
			onTracePath();
		});
	}
	return row;
}

std::shared_ptr<WzResearchDetailContents> makeResearchDetailContents(const ResearchGraph& graph, const LayoutUnit& unit, const ResearchTreeContext& context, int32_t width, int32_t maxWidth, const ResearchDetailActions& actions, optional<uint16_t> subjectAsked, bool known)
{
	if (unit.members.empty() || unit.members.front() >= asResearch.size())
	{
		return nullptr;
	}

	// Describe whichever step was asked about.
	// Defaults to the one the player looks at next, which for a progression is the first non-researched.
	uint16_t subject = unit.members.front();
	for (const auto member : unit.members)
	{
		const auto node = graph.nodeForResearchIndex(member);
		if (node && graph.nodes()[*node].state == NodeState::Researched)
		{
			subject = member;
		}
	}
	for (const auto member : unit.members)
	{
		const auto node = graph.nodeForResearchIndex(member);
		if (node && graph.nodes()[*node].state != NodeState::Researched)
		{
			subject = member;
			break;
		}
	}
	if (subjectAsked && std::find(unit.members.begin(), unit.members.end(), *subjectAsked) != unit.members.end())
	{
		subject = *subjectAsked;
	}

	if (!known)
	{
		// All the panel says is that there is something there - no details
		auto unknown = std::make_shared<W_LABEL>();
		unknown->setFont(font_regular, WZCOL_TEXT_MEDIUM);
		unknown->setString(_("Undiscovered"));
		unknown->setGeometry(0, 0, width, iV_GetTextLineSize(font_regular));
		return WzResearchDetailContents::make(nullptr, unknown, nullptr, nullptr, nullptr, {}, nullptr, nullptr, width);
	}

	const RESEARCH& research = asResearch[subject];
	const auto subjectNode = graph.nodeForResearchIndex(subject);
	const NodeState state = subjectNode ? graph.nodes()[*subjectNode].state : NodeState::Locked;

	auto title = WzResearchTitle::make(WzString::fromUtf8(getLocalizedStatsName(&research)));

	auto subtitle = std::make_shared<W_LABEL>();
	subtitle->setFont(font_small, pal_RGBA(150, 190, 235, 255));

	WzString groupName = (unit.members.size() > 1) ? unit.name : WzString();
	if (groupName.isEmpty())
	{
		groupName = research.category;
	}

	// A knot has no order to count along, so nothing there is a step.
	WzString stepText;
	if (!unit.merged && unit.members.size() > 1)
	{
		// The subject's own place in the progression
		const size_t step = static_cast<size_t>(
			std::find(unit.members.begin(), unit.members.end(), subject) - unit.members.begin());
		stepText = WzString::format(_("step %zu of %zu"), step + 1, unit.members.size());
	}

	WzString subtitleText = groupName.isEmpty() ? researchTechCategoryName(research) : WzString();
	if (!stepText.isEmpty())
	{
		subtitleText = subtitleText.isEmpty() ? stepText
			: (subtitleText + WzString::fromUtf8(" - ") + stepText);
	}
	subtitle->setString(subtitleText);
	subtitle->setCanTruncate(true);

	WzString standing;
	PIELIGHT standingFill = pal_RGBA(40, 40, 40, 255);
	PIELIGHT standingTextColor = pal_Colour(190, 200, 215);
	switch (state)
	{
	case NodeState::Researched:
		standing = WzString::fromUtf8(_("Researched"));
		standingFill = pal_RGBA(26, 56, 32, 255);
		standingTextColor = pal_Colour(150, 220, 165);
		break;
	case NodeState::InProgress:
		standing = WzString::fromUtf8(_("Being researched"));
		standingFill = pal_RGBA(70, 58, 18, 255);
		standingTextColor = pal_Colour(245, 205, 120);
		break;
	case NodeState::Available:
		standing = WzString::fromUtf8(_("Available now"));
		standingFill = pal_RGBA(20, 58, 84, 255);
		standingTextColor = pal_Colour(110, 210, 255);
		break;
	case NodeState::Disabled:
		standing = WzString::fromUtf8(_("Not in this game"));
		standingFill = pal_RGBA(52, 28, 28, 255);
		standingTextColor = pal_Colour(225, 150, 150);
		break;
	default:
		standing = WzString::fromUtf8(_("Not yet available"));
		break;
	}

	if (!context.aggregate.empty() && state != NodeState::Disabled)
	{
		const uint32_t held = researchHeldCount(context, subject);
		const uint32_t members = static_cast<uint32_t>(context.aggregate.size());
		if (held == 0)
		{
			standing = WzString::fromUtf8(astringf(_("Held by none of %u"), static_cast<unsigned>(members)));
			standingFill = pal_RGBA(40, 40, 40, 255);
			standingTextColor = pal_Colour(190, 200, 215);
		}
		else if (held >= members)
		{
			standing = WzString::fromUtf8(astringf(_("Held by all %u"), static_cast<unsigned>(members)));
		}
		else
		{
			standing = WzString::fromUtf8(astringf(_("Held by %u of %u"), static_cast<unsigned>(held), static_cast<unsigned>(members)));
			standingFill = pal_RGBA(33, 50, 43, 255);
			standingTextColor = pal_Colour(120, 180, 150);
		}
	}

	auto onIt = alliesResearching(context, subject);
	if (state == NodeState::Researched)
	{
		onIt.clear();
	}
	bool viewerHasIt = (state == NodeState::Researched);
	if (!context.aggregate.empty())
	{
		viewerHasIt = context.player < MAX_PLAYERS && subject < asPlayerResList[context.player].size()
			&& IsResearchCompleted(&asPlayerResList[context.player][subject]);
	}

	// Stats (cost, time, etc)
	std::vector<std::shared_ptr<WIDGET>> stats;
	if (!viewerHasIt && state != NodeState::Disabled)
	{
		stats.push_back(WzResearchStat::make(WzString::fromUtf8(_("Cost")),
		                                    WzString::format("%u", research.researchPower),
		                                    IMAGE_DES_POWER, _("Power to start it")));
		stats.push_back(WzResearchStat::make(WzString::fromUtf8(_("Research")),
		                                    WzString::format(_("%u pts"), static_cast<unsigned>(research.researchPoints)),
		                                    nullopt, _("Research points to finish it")));
		const int rate = playerResearchRate(context.player);
		if (rate > 0 && context.source == ResearchTreeContext::Source::LivePlayerState)
		{
			uint32_t remaining = research.researchPoints;
			if (context.player < MAX_PLAYERS && subject < asPlayerResList[context.player].size())
			{
				const uint32_t already = asPlayerResList[context.player][subject].currentPoints;
				remaining = (already < remaining) ? remaining - already : 0;
			}
			stats.push_back(WzResearchStat::make(WzString::fromUtf8(_("Time")),
			                                     WzString::fromUtf8("\u2248 ") + formatDuration(remaining / static_cast<uint32_t>(rate)),
			                                     nullopt,
			                                     astringf(_("At the %d points a second the labs make now"), rate)));
		}
	}

	std::shared_ptr<WIDGET> status;
	if (!onIt.empty())
	{
		status = WzResearchStatusLine::make(standing, standingTextColor,
		                                    WzString::fromUtf8(_("Researching this now:")), onIt);
	}
	else if (stats.empty())
	{
		status = WzResearchBadge::make(standing, standingFill, standingTextColor);
	}
	else
	{
		auto line = std::make_shared<W_LABEL>();
		line->setFont(font_small, standingTextColor);
		line->setString(standing);
		line->setCanTruncate(true);
		status = std::move(line);
	}

	// Which progression a topic sits in (if it has one)
	std::shared_ptr<WIDGET> group;
	if (!groupName.isEmpty())
	{
		group = WzResearchBadge::make(groupName, pal_RGBA(40, 40, 40, 255), WZCOL_FORM_TEXT);
	}

	// The title and the badges beside the model do not wrap, and a long progression name next to a
	// long status runs off the end of the header without this.
	{
		const int32_t asked = width;
		const int textX = PADDING + MODEL_SIZE + PADDING;
		// A status shown as a bar spans the panel rather than sharing the header row, so
		// what it wants bounds the panel and not that row
		const bool statusIsBar = !stats.empty();
		int badges = statusIsBar ? 0 : status->idealWidth();
		if (group)
		{
			badges += (badges > 0 ? BADGE_GAP : 0) + group->idealWidth();
		}
		width = std::max(width, textX + std::max(title->idealWidth(), badges) + PADDING);
		if (statusIsBar)
		{
			width = std::max(width, PADDING + status->idealWidth() + PADDING);
		}
		width = std::min(width, std::max(maxWidth, asked));
	}

	auto paragraph = std::make_shared<Paragraph>();
	paragraph->setGeometry(0, 0, std::max(10, width - PADDING * 2), 10);
	paragraph->setFont(font_small);
	paragraph->setFontColour(WZCOL_TEXT_MEDIUM);
	paragraph->setLineSpacing(BODY_LINE_SPACING);

	const auto held = researchHeldBy(context, subject);
	if (!held.empty())
	{
		WzString names;
		bool viewerAmong = false;
		for (const auto member : held)
		{
			if (!names.isEmpty())
			{
				names += WzString::fromUtf8(", ");
			}
			names += WzString::fromUtf8(getPlayerName(member));
			viewerAmong = viewerAmong || member == context.player;
		}
		const bool aboutTheViewer = context.player == selectedPlayer;
		const bool leftOut = aboutTheViewer && !viewerAmong;
		paragraph->setFontColour(leftOut ? pal_Colour(110, 210, 255) : WZCOL_TEXT_MEDIUM);
		paragraph->addText(leftOut
			? WzString::format(_("Held by %s, not by you"), names.toUtf8().c_str()) + "\n"
			: WzString::format(_("Held by: %s"), names.toUtf8().c_str()) + "\n");
		paragraph->setFontColour(WZCOL_TEXT_MEDIUM);
	}

	// Flag duplicated research (in shared research mode)
	const auto allies = alliesResearching(context, subject);
	if (!allies.empty())
	{
		const auto duplicated = duplicatedResearch(context);
		if (std::find(duplicated.begin(), duplicated.end(), subject) != duplicated.end())
		{
			paragraph->setFontColour(pal_Colour(240, 120, 70));
			paragraph->addText(WzString::fromUtf8(_("Doubled up - a second lab on this researches it no faster")) + "\n");
			paragraph->setFontColour(WZCOL_TEXT_MEDIUM);
		}
	}

	// The authored description that the intelligence screen already shows for this topic
	if (research.pViewData != nullptr && !research.pViewData->textMsg.empty())
	{
		paragraph->addText("\n");
		for (const auto& line : research.pViewData->textMsg)
		{
			paragraph->addText(line + "\n");
		}
	}

	// What it actually changes
	const auto effects = describeResearchEffects(research);
	if (!effects.empty())
	{
		addSection(paragraph, WzString::fromUtf8(_("Effects")));
		for (const auto& effect : effects)
		{
			paragraph->setFontColour(effect.isBuff ? pal_Colour(140, 210, 150) : pal_Colour(215, 130, 120));
			paragraph->addText(effect.text + "\n");
		}
		paragraph->setFontColour(WZCOL_TEXT_MEDIUM);
	}

	// Prereqs, as a checklist
	std::vector<std::pair<WzString, bool>> prereqs;
	for (const auto prereqIndex : research.pPRList)
	{
		if (prereqIndex >= asResearch.size())
		{
			continue;
		}
		const auto prereqNode = graph.nodeForResearchIndex(prereqIndex);
		const bool done = prereqNode && graph.nodes()[*prereqNode].state == NodeState::Researched;
		prereqs.push_back({WzString::fromUtf8(getLocalizedStatsName(&asResearch[prereqIndex])), done});
	}
	std::shared_ptr<WzResearchChecklist> checklist;
	if (!prereqs.empty())
	{
		checklist = WzResearchChecklist::make(WzString::fromUtf8(_("Requires")), std::move(prereqs));
	}

	// Resume the running text after the checklist widget
	auto after = std::make_shared<Paragraph>();
	after->setGeometry(0, 0, std::max(10, width - PADDING * 2), 10);
	after->setFont(font_small);
	after->setFontColour(WZCOL_TEXT_MEDIUM);
	after->setLineSpacing(BODY_LINE_SPACING);

	if (!research.componentResults.empty() || !research.pStructureResults.empty())
	{
		addSection(after, WzString::fromUtf8(_("Unlocks")));
		for (const auto *component : research.componentResults)
		{
			if (component != nullptr)
			{
				after->addText(WzString::fromUtf8(getLocalizedStatsName(component)) + "\n");
			}
		}
		for (const auto structIndex : research.pStructureResults)
		{
			if (structIndex < numStructureStats)
			{
				after->addText(WzString::fromUtf8(getLocalizedStatsName(&asStructureStats[structIndex])) + "\n");
			}
		}
	}

	if (!research.pRedArtefacts.empty() || !research.pRedStructs.empty())
	{
		addSection(after, WzString::fromUtf8(_("Replaces")));
		for (const auto *component : research.pRedArtefacts)
		{
			if (component != nullptr)
			{
				after->addText(WzString::fromUtf8(getLocalizedStatsName(component)) + "\n");
			}
		}
		for (const auto structIndex : research.pRedStructs)
		{
			if (structIndex < numStructureStats)
			{
				after->addText(WzString::fromUtf8(getLocalizedStatsName(&asStructureStats[structIndex])) + "\n");
			}
		}
	}

	auto body = ScrollableListWidget::make();
	body->addItem(paragraph);
	if (checklist)
	{
		body->addItem(checklist);
	}
	body->addItem(after);

	auto model = WzResearchModel::make(subject);
	return WzResearchDetailContents::make(std::move(model), std::move(title), std::move(subtitle), std::move(group), std::move(status), std::move(stats), std::move(body), makeActionRow(actions), width);
}
