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

#include "detailpanel.h"

#include "lib/widget/label.h"
#include "lib/widget/paragraph.h"
#include "lib/ivis_opengl/pieblitfunc.h"
#include "lib/ivis_opengl/piematrix.h"
#include "lib/ivis_opengl/piepalette.h"
#include "lib/framework/input.h"

#include "../../intimage.h"
#include "../../component.h"
#include "../../multiplay.h"

#include <algorithm>

static constexpr int MIN_BODY_HEIGHT = 60;
static constexpr int BODY_LINE_SPACING = DETAIL_PANEL_BODY_LINE_SPACING;
static constexpr int ACTIONS_HEIGHT = 18;
static constexpr int BADGE_HEIGHT = 15;
static constexpr int TITLE_HEIGHT = 18;
static constexpr int STATS_Y = 41;
static constexpr int COMPACT_STATS_Y = 24;
static constexpr int STATUS_HEIGHT = 14;
static constexpr float READABLE_COLOR_LUMINANCE = 140.f;
static constexpr int BADGE_GAP = 5;

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

PIELIGHT detailPanelHeadingColor() { return pal_Colour(255, 220, 140); }
PIELIGHT detailPanelBuffColor() { return pal_Colour(140, 210, 150); }
PIELIGHT detailPanelNerfColor() { return pal_Colour(215, 130, 120); }

DetailPanelStyle DetailPanelStyle::researchTree()
{
	DetailPanelStyle style;
	style.background = pal_RGBA(24, 24, 24, 255);
	style.border = panelBorderColor();
	style.divider = panelColor(90);
	style.cornerRadius = 6.f;
	style.restingAlpha = 232;
	style.solidWhenPointedAt = true;
	return style;
}

// A model (or anything) drawn fitted inside the box
class DetailPanelModel : public WIDGET
{
public:
	explicit DetailPanelModel(std::function<void (int, int, int, int)> drawModel): m_drawModel(std::move(drawModel)) {}

	void display(int xOffset, int yOffset) override
	{
		const int x0 = xOffset + x();
		const int y0 = yOffset + y();
		pie_UniTransBoxFill(static_cast<float>(x0), static_cast<float>(y0),
		                    static_cast<float>(x0 + width()), static_cast<float>(y0 + height()),
		                    pal_RGBA(0, 0, 0, 120));
		if (m_drawModel)
		{
			// Nearer than the models of the buttons the panel may cover
			gfx_api::context::get().set_depth_range(0.f, 0.01f);
			m_drawModel(x0, y0, width(), height());
			pie_BeginInterface();
		}
	}

private:
	std::function<void (int, int, int, int)> m_drawModel;
};

class DetailPanelTitle : public WIDGET
{
public:
	static std::shared_ptr<DetailPanelTitle> make(const WzString& text)
	{
		auto widget = std::make_shared<DetailPanelTitle>();
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

class DetailPanelStat : public WIDGET
{
public:
	static std::shared_ptr<DetailPanelStat> make(const WzString& label, const WzString& value,
	                                             optional<uint16_t> icon, const std::string& tip)
	{
		auto widget = std::make_shared<DetailPanelStat>();
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

	std::function<bool (uint16_t image, float x, float y, float size, PIELIGHT colour)> drawIcon;

	void display(int xOffset, int yOffset) override
	{
		const float x0 = static_cast<float>(xOffset + x());
		const float y0 = static_cast<float>(yOffset + y());
		pie_DrawRoundedBox(x0, y0, x0 + static_cast<float>(width()), y0 + static_cast<float>(height()),
		                   pal_RGBA(0, 0, 0, 0), 3.f, EDGE, 1.f);

		const int availableWidth = width() - PAD_X * 2;
		renderCut(m_labelText, x0 + PAD_X, y0 + PAD_TOP - m_labelText.aboveBase(), EDGE, availableWidth, y0);

		const float valueTop = y0 + PAD_TOP + static_cast<float>(m_labelText.lineSize());
		float at = x0 + PAD_X;
		const bool withIcon = m_icon.has_value()
			&& (iconSize() + ICON_GAP - static_cast<int>(ICON_NUDGE) + m_valueText.width()) <= availableWidth;
		if (withIcon)
		{
			// Centered on the ascent rather than the line, because a line reserves room
			// under the baseline for descenders and a row of digits has none.
			const float size = static_cast<float>(iconSize());
			const float ascent = static_cast<float>(-m_valueText.aboveBase());
			const float iconX = at - ICON_NUDGE;
			const float iconY = valueTop + (ascent - size) / 2.f;
			if (!drawIcon || !drawIcon(*m_icon, iconX, iconY, size, WZCOL_TEXT_BRIGHT))
			{
				iV_DrawImageTint(IntImages, *m_icon, iconX, iconY, WZCOL_TEXT_BRIGHT, Vector2f(size, size));
			}
			at += size + static_cast<float>(ICON_GAP) - ICON_NUDGE;
		}
		renderCut(m_valueText, at, valueTop - m_valueText.aboveBase(), WZCOL_TEXT_BRIGHT,
		          static_cast<int>(static_cast<float>(x0 + PAD_X) + static_cast<float>(availableWidth) - at), y0);
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

	void renderCut(WzText& text, float atX, float baseline, PIELIGHT color, int maxWidth, float boxTop)
	{
		if (maxWidth <= 0)
		{
			return;
		}
		const bool cut = text.width() > maxWidth;
		const int shown = cut ? maxWidth - iV_GetEllipsisWidth(text.getFontID()) - 2 : maxWidth;
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

const PIELIGHT DetailPanelStat::EDGE = pal_RGBA(191, 191, 191, 190);

// One wrapped line of the DetailPanelChecklist
class DetailPanelChecklistRow : public WIDGET
{
public:
	struct Entry
	{
		WzText text;
		bool done = false;
		int at = 0;		// where it starts, from the row's left edge
	};

	static std::shared_ptr<DetailPanelChecklistRow> make(std::vector<Entry> entries)
	{
		auto row = std::make_shared<DetailPanelChecklistRow>();
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
			const PIELIGHT color = entry.done ? detailPanelBuffColor() : WZCOL_TEXT_MEDIUM;
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

// What something requires, displayed as a checklist: a check for what is done and an empty
// circle for what is not. Wrapped across the width instead of just one to a line.
class DetailPanelChecklist : public WIDGET
{
public:
	static std::shared_ptr<DetailPanelChecklist> make(const WzString& heading, std::vector<std::pair<WzString, bool>> items)
	{
		auto widget = std::make_shared<DetailPanelChecklist>();
		widget->m_items = std::move(items);
		widget->m_metrics.setText(heading.toUpper(), font_small);
		widget->m_heading = std::make_shared<W_LABEL>();
		widget->m_heading->setFont(font_small, detailPanelHeadingColor());
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
		return static_cast<int>(DetailPanelChecklistRow::markPx()) + DetailPanelChecklistRow::MARK_GAP
			+ static_cast<int>(iV_GetTextWidth(name, font_small)) + DetailPanelChecklistRow::ITEM_GAP;
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

		const int availableWidth = std::max(20, forWidth);
		int at = 0;
		int rows = m_items.empty() ? 0 : 1;
		std::vector<DetailPanelChecklistRow::Entry> entries;
		const auto closeRow = [&]() {
			if (entries.empty())
			{
				return;
			}
			auto row = DetailPanelChecklistRow::make(std::move(entries));
			entries.clear();
			row->setGeometry(0, LEAD_GAP + rows * pitch, availableWidth, pitch);
			attach(row);
			m_rowWidgets.push_back(row);
		};
		for (const auto& item : m_items)
		{
			const int itemWidth = entryWidth(item.first);
			if (at > 0 && at + itemWidth > availableWidth)
			{
				closeRow();
				at = 0;
				++rows;
			}
			DetailPanelChecklistRow::Entry entry;
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
	std::vector<std::shared_ptr<DetailPanelChecklistRow>> m_rowWidgets;
	int m_height = 0;
	int m_rows = 0;
	int m_laidOutFor = -1;
};

// (Team colors are chosen to be told apart on the map, not necessarily easily read on a dark plate.)
PIELIGHT detailPanelReadableTeamColor(PIELIGHT color)
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

class DetailPanelStatusLine : public WIDGET
{
public:
	static std::shared_ptr<DetailPanelStatusLine> make(const WzString& standing, PIELIGHT color,
	                                                   const WzString& prefix,
	                                                   const std::vector<uint32_t>& players)
	{
		auto widget = std::make_shared<DetailPanelStatusLine>();
		widget->m_color = color;
		widget->m_standing.setText(standing, font_small);
		widget->m_prefix.setText(prefix, font_small);
		for (size_t i = 0; i < players.size(); ++i)
		{
			Name name;
			name.color = detailPanelReadableTeamColor(pal_GetTeamColour(getPlayerColour(players[i])));
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
			wanted += (m_standing.getText().isEmpty() ? 0 : static_cast<int32_t>(GAP)) + m_prefix.width() + static_cast<int32_t>(SPACE);
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
		WzText &metrics = m_standing.getText().isEmpty() ? m_prefix : m_standing;
		const float baseline = static_cast<float>(yOffset + y())
			+ (static_cast<float>(height()) - static_cast<float>(metrics.lineSize())) / 2.f
			- static_cast<float>(metrics.aboveBase());
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
		if (!m_standing.getText().isEmpty())
		{
			at += GAP;
		}
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

class DetailPanelBadge : public WIDGET
{
public:
	static std::shared_ptr<DetailPanelBadge> make(const WzString& text, PIELIGHT fill, PIELIGHT textColor)
	{
		class make_shared_enabler : public DetailPanelBadge {};
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

std::shared_ptr<DetailPanelContents> DetailPanelContents::make(std::shared_ptr<WIDGET> model, std::shared_ptr<WIDGET> title, std::shared_ptr<WIDGET> subtitle, std::shared_ptr<WIDGET> group, std::shared_ptr<WIDGET> status, std::vector<std::shared_ptr<WIDGET>> stats, std::shared_ptr<WIDGET> body, std::shared_ptr<WIDGET> actions, int32_t contentWidth, const DetailPanelStyle &panelStyle, DetailStatusPlacement statusPlacement, int modelSize)
{
	class make_shared_enabler : public DetailPanelContents {};
	auto widget = std::make_shared<make_shared_enabler>();
	widget->m_model = std::move(model);
	widget->m_title = std::move(title);
	widget->m_subtitle = std::move(subtitle);
	widget->m_group = std::move(group);
	widget->m_status = std::move(status);
	widget->m_stats = std::move(stats);
	widget->m_body = std::move(body);
	widget->m_actions = std::move(actions);
	widget->m_style = panelStyle;
	widget->m_statusPlacement = statusPlacement;
	widget->m_modelSize = modelSize;
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
		if (auto cell = std::dynamic_pointer_cast<DetailPanelStat>(stat))
		{
			cell->drawIcon = panelStyle.drawIcon;
		}
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

void DetailPanelContents::display(int xOffset, int yOffset)
{
	const int x0 = xOffset + x();
	const int y0 = yOffset + y();

	if (m_style.drawBackground)
	{
		m_style.drawBackground(WzRect(x0, y0, width(), height()));
	}
	else
	{
		// The popover may be click-through, in which case the widget system never reports the pointer as over it.
		// Compare against its own rect.
		const int pointerX = static_cast<int>(mouseX());
		const int pointerY = static_cast<int>(mouseY());
		const bool pointerOver = m_style.solidWhenPointedAt
			&& pointerX >= screenPosX() && pointerX < screenPosX() + width()
			&& pointerY >= screenPosY() && pointerY < screenPosY() + height();
		PIELIGHT fill = m_style.background;
		fill.byte.a = pointerOver ? 255 : m_style.restingAlpha;

		pie_DrawRoundedBox(static_cast<float>(x0), static_cast<float>(y0),
		                   static_cast<float>(x0 + width()), static_cast<float>(y0 + height()),
		                   fill, m_style.cornerRadius, m_style.border, 1.f);
	}
	if (anythingUnderHeader())
	{
		const int dividerY = y0 + PADDING + headerHeight() + PADDING / 2;
		iV_Line(x0 + PADDING, dividerY, x0 + width() - PADDING, dividerY, m_style.divider);
		// A band has a line under it as well as over it
		if (statusBarHeight() > 0)
		{
			const int closingY = dividerY + statusBarHeight();
			iV_Line(x0 + PADDING, closingY, x0 + width() - PADDING, closingY, m_style.divider);
		}
	}
}

void DetailPanelContents::geometryChanged()
{
	const int textX = PADDING + (m_model ? m_modelSize + PADDING : 0);
	const int textWidth = std::max(10, width() - textX - PADDING);
	if (m_model)
	{
		m_model->setGeometry(PADDING, PADDING, m_modelSize, m_modelSize);
	}
	if (m_title)
	{
		m_title->setGeometry(textX, PADDING + 2, textWidth, TITLE_HEIGHT);
	}
	// The line under the title names the group, if the subject is in one.
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
	// The stats under the group, then the state under those, so the header reads:
	// name, group, numbers, status.
	if (!m_stats.empty())
	{
		int wanted = 0;
		for (const auto& stat : m_stats)
		{
			wanted += stat->idealWidth();
		}
		const int gaps = static_cast<int>(m_stats.size() - 1) * BADGE_GAP;
		const int availableWidth = std::max(30, textWidth);
		int at = textX;
		for (size_t i = 0; i < m_stats.size(); ++i)
		{
			int statWidth = m_stats[i]->idealWidth();
			if (wanted + gaps > availableWidth)
			{
				statWidth = std::max(20, (availableWidth - gaps) / static_cast<int>(m_stats.size()));
			}
			m_stats[i]->setGeometry(at, PADDING + statsY(), statWidth, DetailPanelStat::STAT_HEIGHT);
			at += statWidth + BADGE_GAP;
		}
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
			m_status->setGeometry(textX, PADDING + statsY(),
			                      std::min(m_status->idealWidth(), std::max(10, width() - textX - PADDING)), BADGE_HEIGHT);
		}
	}
	const int bodyY = PADDING + headerHeight() + statusBarHeight() + PADDING;
	const int reservedActionsHeight = m_actions ? ACTIONS_HEIGHT + PADDING : 0;
	if (m_body)
	{
		m_body->setGeometry(PADDING, bodyY, std::max(10, width() - PADDING * 2), std::max(10, height() - bodyY - PADDING - reservedActionsHeight));
	}
	if (m_actions)
	{
		m_actions->setGeometry(PADDING, height() - PADDING - ACTIONS_HEIGHT, std::max(10, width() - PADDING * 2), ACTIONS_HEIGHT);
	}
}

void DetailPanelContents::setMaxHeight(int32_t maxHeight)
{
	m_maxHeight = maxHeight;
	recalcHeight();
}

bool DetailPanelContents::statusInBar() const
{
	switch (m_statusPlacement)
	{
	case DetailStatusPlacement::Band: return true;
	case DetailStatusPlacement::Badge: return false;
	case DetailStatusPlacement::Automatic: break;
	}
	// A subject with data in its header leaves no room there for its status, so that goes in a band of its own
	// underneath. A subject without data displays its status as a badge in the header instead.
	return !m_stats.empty();
}

int DetailPanelContents::headerHeight() const
{
	int text = m_title ? TITLE_HEIGHT : 0;
	if (!m_stats.empty())
	{
		text = statsY() + DetailPanelStat::STAT_HEIGHT;
	}
	else if (m_status && !statusInBar())
	{
		text = statsY() + BADGE_HEIGHT;
	}
	return std::max(m_model ? m_modelSize : 0, text);
}

int DetailPanelContents::statsY() const
{
	// Without a line under the title, the stats go right under it
	return (m_group || m_subtitle) ? STATS_Y : COMPACT_STATS_Y;
}

int DetailPanelContents::statusBarHeight() const
{
	return (m_status && statusInBar()) ? PADDING + STATUS_HEIGHT : 0;
}

bool DetailPanelContents::anythingUnderHeader() const
{
	return statusBarHeight() > 0 || m_actions != nullptr
		|| (m_body != nullptr && m_body->idealHeight() > 0);
}

void DetailPanelContents::recalcHeight()
{
	const int bodyY = PADDING + headerHeight() + statusBarHeight() + PADDING;
	const int reservedActionsHeight = m_actions ? ACTIONS_HEIGHT + PADDING : 0;
	if (!m_body || m_body->idealHeight() <= 0)
	{
		setGeometry(x(), y(), width(), bodyY + (m_actions ? PADDING / 2 + reservedActionsHeight : 0));
		return;
	}
	const int chrome = bodyY + PADDING + reservedActionsHeight;
	const int availableBodyHeight = std::max(MIN_BODY_HEIGHT, m_maxHeight - chrome);
	setGeometry(x(), y(), width(), chrome + std::min(m_body->idealHeight(), availableBodyHeight));
}

std::shared_ptr<WIDGET> makeDetailPanelTitle(const WzString& text)
{
	return DetailPanelTitle::make(text);
}

std::shared_ptr<WIDGET> makeDetailPanelStat(const WzString& label, const WzString& value, optional<uint16_t> icon, const std::string& tip)
{
	return DetailPanelStat::make(label, value, icon, tip);
}

std::shared_ptr<WIDGET> makeDetailPanelBadge(const WzString& text, PIELIGHT fill, PIELIGHT textColor)
{
	return DetailPanelBadge::make(text, fill, textColor);
}

std::shared_ptr<WIDGET> makeDetailPanelStatusLine(const WzString& standing, PIELIGHT color, const WzString& prefix, const std::vector<uint32_t>& players)
{
	return DetailPanelStatusLine::make(standing, color, prefix, players);
}

std::shared_ptr<WIDGET> makeDetailPanelChecklist(const WzString& heading, std::vector<std::pair<WzString, bool>> items)
{
	return DetailPanelChecklist::make(heading, std::move(items));
}

std::shared_ptr<WIDGET> makeDetailPanelModel(const std::function<void (int x0, int y0, int width, int height)>& drawModel)
{
	return std::make_shared<DetailPanelModel>(drawModel);
}

void detailPanelAddSection(const std::shared_ptr<Paragraph>& body, const WzString& heading)
{
	body->setFontColour(detailPanelHeadingColor());
	body->addText("\n" + heading.toUpper() + "\n");
	body->setFontColour(WZCOL_TEXT_MEDIUM);
}

std::shared_ptr<Paragraph> makeDetailPanelParagraph(int32_t width)
{
	auto paragraph = std::make_shared<Paragraph>();
	paragraph->setGeometry(0, 0, std::max(10, width), 10);
	paragraph->setFont(font_small);
	paragraph->setFontColour(WZCOL_TEXT_MEDIUM);
	paragraph->setLineSpacing(BODY_LINE_SPACING);
	return paragraph;
}

class DetailPanelActionButton : public W_BUTTON
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
	auto button = std::make_shared<DetailPanelActionButton>();
	button->setString(text);
	button->FontID = font_small;
	button->setTip(tip.toUtf8());
	button->setQuiet(quiet);
	return button;
}

std::shared_ptr<W_BUTTON> makeDetailPanelActionButton(const WzString& text, const WzString& tip)
{
	return makeButton(text, tip, false);
}

std::shared_ptr<W_BUTTON> makeDetailPanelQuietButton(const WzString& text, const WzString& tip)
{
	return makeButton(text, tip, true);
}

class DetailPanelActionRow : public WIDGET
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

std::shared_ptr<WIDGET> makeDetailPanelActionRow()
{
	return std::make_shared<DetailPanelActionRow>();
}
