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

/** @file
 *  A skirmish or multiplayer game's results in the Modern skin: the Overview (a table by group of figures), Charts
 *  (one at a time, the team lead, who led), Research (each side's timeline, who got there first, by weapon line) and
 *  the Timeline (phases and key moments).
 */

#include "hudresults.h"

#include "hudchart.h"
#include "hudresultsdata.h"
#include "hudskin.h"
#include "intel/intelkit.h"

#include "lib/framework/frame.h"
#include "lib/framework/input.h"
#include "lib/ivis_opengl/pieblitfunc.h"
#include "lib/ivis_opengl/pieshapes.h"
#include "lib/ivis_opengl/piestate.h"
#include "lib/ivis_opengl/screen.h"
#include "lib/netplay/netplay.h"
#include "lib/netplay/netreplay.h"
#include "lib/widget/button.h"

#include "../ai.h"
#include "../levels.h"
#include "../multiplay.h"
#include "../urlhelpers.h"

#include <algorithm>
#include <cmath>
#include <map>

using namespace intelkit;

std::shared_ptr<WIDGET> hudMakeResultsRoot(std::function<std::shared_ptr<HudResultsSheet> ()> makeSheet, uint8_t dim);

namespace
{

constexpr int PAD = 16;

const PIELIGHT SIDE_COLORS[] = {pal_RGBA(70, 175, 255, 255), pal_RGBA(232, 135, 90, 255), pal_RGBA(96, 205, 128, 255), pal_RGBA(176, 128, 236, 255),
                                pal_RGBA(226, 196, 74, 255), pal_RGBA(88, 200, 200, 255), pal_RGBA(220, 110, 170, 255), pal_RGBA(160, 170, 184, 255)};
/// Counts (kills, built, research): one fixed color, whatever the accent (which is for chosen things)
const PIELIGHT DATA_COLOR = pal_RGBA(70, 175, 255, 255);
const PIELIGHT KILL_COLOR = pal_RGBA(92, 196, 122, 255);
const PIELIGHT LOSS_COLOR = pal_RGBA(224, 96, 78, 255);
const PIELIGHT GOLD = pal_RGBA(232, 176, 64, 255);

PIELIGHT faint(uint8_t a = 170)
{
	return alpha(currentHudSkin().colors().textSecondary, a);
}

std::string upper(const std::string &text)
{
	return hudResultsCaps(text);
}

/// A text cache that grows: setText and render in one call
class Texts
{
public:
	void reset() { next = 0; }
	WzText &get()
	{
		if (next >= texts.size())
		{
			texts.resize(texts.size() + 16);
		}
		return texts[next++];
	}
	/// Renders `text` with its top at `top` (x the left, or the right when `right`), and returns its width
	int draw(const std::string &text, iV_fonts font, int x, int top, PIELIGHT color, bool right = false)
	{
		WzText &t = get();
		t.setText(WzString::fromUtf8(text), font);
		t.render(static_cast<float>(right ? x - t.width() : x), baselineAt(t, top), color);
		return t.width();
	}
	int width(const std::string &text, iV_fonts font)
	{
		WzText &t = get();
		t.setText(WzString::fromUtf8(text), font);
		return t.width();
	}

private:
	std::vector<WzText> texts;
	size_t next = 0;
};

PIELIGHT sideColor(const HudGameResults &data, size_t side)
{
	if (!data.teamGame && side < data.sides.size() && !data.sides[side].empty())
	{
		return hudReadableColor(data.players[data.sides[side].front()].color);
	}
	return SIDE_COLORS[side % (sizeof(SIDE_COLORS) / sizeof(SIDE_COLORS[0]))];
}

/// A button of a row of choices (tabs, groups, metrics, filters): its look is its parent's to draw
class ChoiceButton : public W_BUTTON
{
public:
	enum class Look { Tab, Segment, Chip, Metric };
	Look look = Look::Segment;
	bool small = false;
	std::string label;
	std::string icon;
	PIELIGHT iconColor = WZCOL_WHITE;
	bool chosen = false;
	bool underline = true;   ///< (a tab: its parent may draw the underline instead, to slide it)
	bool first = false;
	bool last = false;
	std::vector<float> spark;
	std::vector<float> rivalSpark;   ///< (dashed, on the same scale)
	PIELIGHT sparkColor = WZCOL_WHITE;
	PIELIGHT rivalColor = WZCOL_WHITE;
	const HudResearchLine *line = nullptr;   ///< (a chip of a research line: its picture)

	int32_t idealWidth() override
	{
		const int textW = look == Look::Tab ? static_cast<int>(iV_GetTextWidth(WzString::fromUtf8(upper(label)), RESULTS_TAB_FONT))
		                                    : static_cast<int>(iV_GetTextWidth(label.c_str(), font_small));
		switch (look)
		{
		case Look::Tab: return textW + (small ? 18 : 30);
		case Look::Segment: return textW + 22;
		case Look::Chip: return textW + (icon.empty() && !line ? 0 : 17) + 22;
		case Look::Metric: return 180;
		}
		return textW;
	}

	void display(int xOffset, int yOffset) override
	{
		const HudColors &colors = currentHudSkin().colors();
		const int x0 = xOffset + x();
		const int y0 = yOffset + y();
		const bool hover = isMouseOverWidget();
		cache.reset();
		switch (look)
		{
		case Look::Tab:
		{
			// (in capitals, centered on them, and fading in after the header)
			const float in = hudResultsAppear(320.f, 300.f);
			WzText &t = cache.get();
			t.setText(WzString::fromUtf8(upper(label)), RESULTS_TAB_FONT);
			const float cap = hudResultsCapHeight(RESULTS_TAB_FONT, small);
			const PIELIGHT color = chosen || hover ? colors.textPrimary : colors.textSecondary;
			t.render(static_cast<float>(x0 + (width() - t.width()) / 2), y0 + (height() + cap) / 2.f, alpha(color, static_cast<uint8_t>(color.byte.a * in)));
			if (chosen && underline)
			{
				pie_UniTransBoxFill(static_cast<float>(x0), static_cast<float>(y0 + height() - 3), static_cast<float>(x0 + width()), static_cast<float>(y0 + height()), colors.accent);
			}
			break;
		}
		case Look::Segment:
		{
			const glm::vec4 radii(first ? 4.f : 0.f, last ? 4.f : 0.f, first ? 4.f : 0.f, last ? 4.f : 0.f);
			pie_DrawRoundedBox(static_cast<float>(x0), static_cast<float>(y0), static_cast<float>(x0 + width()), static_cast<float>(y0 + height()),
			                   chosen ? hudResultsChosen(pal_RGBA(28, 53, 80, 255)) : hudResultsSurface(hover ? pal_RGBA(26, 33, 43, 255) : pal_RGBA(18, 24, 32, 255)), radii,
			                   hudResultsSurface(pal_RGBA(47, 58, 74, 255)), 1.f);
			cache.draw(label, font_small, x0 + 11, y0 + (height() - static_cast<int>(iV_GetTextLineSize(font_small))) / 2, chosen ? colors.textPrimary : colors.textSecondary);
			break;
		}
		case Look::Chip:
		{
			pie_DrawRoundedBox(static_cast<float>(x0), static_cast<float>(y0), static_cast<float>(x0 + width()), static_cast<float>(y0 + height()),
			                   chosen ? hudResultsChosen(pal_RGBA(28, 53, 80, 255)) : hudResultsSurface(hover ? pal_RGBA(26, 33, 43, 255) : pal_RGBA(22, 29, 39, 255)), currentHudSkin().metrics().cardRadius,
			                   chosen ? hudResultsChosen(pal_RGBA(61, 138, 208, 255)) : hudResultsSurface(pal_RGBA(38, 48, 64, 255)), 1.f);
			int tx = x0 + 11;
			if (line)
			{
				hudDrawResearchLineIcon(*line, WzRect(tx - 1, y0 + height() / 2 - 7, 14, 14));
				tx += 17;
			}
			else if (!icon.empty())
			{
				currentHudSkin().drawNamedIcon(icon, WzRect(tx, y0 + height() / 2 - 6, 12, 12), iconColor);
				tx += 17;
			}
			cache.draw(label, font_small, tx, y0 + (height() - static_cast<int>(iV_GetTextLineSize(font_small))) / 2, chosen ? colors.textPrimary : colors.textSecondary);
			break;
		}
		case Look::Metric:
		{
			if (chosen || hover)
			{
				pie_DrawRoundedBox(static_cast<float>(x0), static_cast<float>(y0), static_cast<float>(x0 + width()), static_cast<float>(y0 + height()),
				                   chosen ? hudResultsChosen(pal_RGBA(26, 44, 64, 255)) : hudResultsSurface(pal_RGBA(22, 29, 39, 255)), 5.f);
			}
			if (chosen)
			{
				pie_UniTransBoxFill(static_cast<float>(x0), static_cast<float>(y0 + 2), static_cast<float>(x0 + 3), static_cast<float>(y0 + height() - 2), colors.accent);
			}
			{
				// (in capitals, centered on them, cut short if they don't fit by the sparkline)
				WzText &t = cache.get();
				const int availableWidth = width() - 12 - (spark.size() >= 2 ? 62 : 12);
				t.setText(WzString::fromUtf8(hudCappedText(upper(label), RESULTS_RAIL_FONT, availableWidth)), RESULTS_RAIL_FONT);
				t.render(static_cast<float>(x0 + 12), y0 + (height() + hudResultsCapHeight(RESULTS_RAIL_FONT, small)) / 2.f, chosen ? colors.textPrimary : colors.textSecondary);
			}
			if (spark.size() >= 2)
			{
				if (sparkShapes.empty() || sparkW != width())
				{
					sparkW = width();
					sparkShapes.clear();
					float hi = 1.f, lo = 0.f;
					for (const std::vector<float> *values : {&spark, &rivalSpark})
					{
						for (float v : *values)
						{
							hi = std::max(hi, v);
							lo = std::min(lo, v);
						}
					}
					const float w = 48.f;
					const float h = 18.f;
					auto points = [&](const std::vector<float> &values) {
						std::vector<glm::vec2> pts;
						for (size_t i = 0; i < values.size(); ++i)
						{
							pts.emplace_back(w * i / (values.size() - 1), h - h * (values[i] - lo) / (hi - lo));
						}
						return pts;
					};
					const std::vector<glm::vec2> pts = points(spark);
					PIELIGHT top = sparkColor;
					top.byte.a = 45;
					PIELIGHT bottom = sparkColor;
					bottom.byte.a = 6;
					sparkShapes.addArea(pts, h, top, bottom);
					if (rivalSpark.size() >= 2)
					{
						sparkShapes.addPolyline(points(rivalSpark), 1.3f, rivalColor, 3.f, 2.f);
					}
					sparkShapes.addPolyline(pts, 1.5f, sparkColor);
				}
				sparkShapes.draw(static_cast<float>(x0 + width() - 56), static_cast<float>(y0 + (height() - 18) / 2));
			}
			break;
		}
		}
	}

	void setSparkColors(PIELIGHT c, PIELIGHT rival)
	{
		if (c.rgba() != sparkColor.rgba() || rival.rgba() != rivalColor.rgba())
		{
			sparkColor = c;
			rivalColor = rival;
			sparkShapes.clear();
		}
	}

private:
	Texts cache;
	PieShapes sparkShapes;
	int sparkW = 0;
};

std::shared_ptr<ChoiceButton> makeChoice(ChoiceButton::Look look, const std::string &label, const std::string &icon = std::string())
{
	auto b = std::make_shared<ChoiceButton>();
	b->look = look;
	b->label = label;
	b->icon = icon;
	return b;
}

// ---------------------------------------------------------------- Overview

enum class Group { Combat, Economy, Research, All };

class OverviewView : public WIDGET
{
public:
	void initialize(const HudGameResults *results, bool compactLayout)
	{
		data = results;
		compact = compactLayout;
		const char *names[] = {N_("Combat"), N_("Economy"), N_("Research"), N_("All")};
		const int groups = compact ? 3 : 4; // (All, with its pairs of numbers, needs the room)
		for (int g = 0; g < groups; ++g)
		{
			auto b = makeChoice(ChoiceButton::Look::Segment, gettext(names[g]));
			b->first = g == 0;
			b->last = g == groups - 1;
			b->chosen = g == 0;
			b->addOnClickHandler([this, g](W_BUTTON &) {
				group = static_cast<Group>(g);
				firstRow = 0;
				for (size_t k = 0; k < segments.size(); ++k)
				{
					segments[k]->chosen = static_cast<int>(k) == g;
				}
			});
			attach(b);
			segments.push_back(b);
		}
	}

	void geometryChanged() override
	{
		int x = compact ? 10 : PAD;
		for (auto &b : segments)
		{
			const int w = b->idealWidth();
			b->setGeometry(x, compact ? 6 : 12, w, compact ? 22 : 26);
			x += w;
		}
	}

	void run(W_CONTEXT *) override
	{
		// A flick: a line moved for each line's height traveled, slowing down
		if (flingVelocity != 0.f)
		{
			const float elapsed = static_cast<float>(std::min<uint32_t>(realTime - flingLastTime, 100));
			flingLastTime = realTime;
			flingTravel += flingVelocity * elapsed;
			flingVelocity *= std::exp(-elapsed / 325.f);
			const float step = static_cast<float>(std::max(1, lineStep));
			while (std::abs(flingTravel) >= step)
			{
				const int dir = flingTravel > 0.f ? 1 : -1;
				flingTravel -= static_cast<float>(dir) * step;
				if ((dir > 0 && hiddenRows <= 0) || (dir < 0 && firstRow <= 0))
				{
					flingVelocity = 0.f;
					break;
				}
				firstRow += dir;
				hiddenRows -= dir;
			}
			if (std::abs(flingVelocity) < 0.02f)
			{
				flingVelocity = 0.f;
			}
		}
		// More lines than fit: the wheel scrolls them, the bar drags them
		if (isMouseOverWidget() && (hiddenRows > 0 || firstRow > 0))
		{
			if (mousePressed(MOUSE_WDN) && hiddenRows > 0)
			{
				++firstRow;
			}
			else if (mousePressed(MOUSE_WUP) && firstRow > 0)
			{
				--firstRow;
			}
		}
		const int mx = mouseX() - screenPosX();
		const int my = mouseY() - screenPosY();
		if (mousePressed(MOUSE_LMB) && barRect.width() > 0 && barRect.contains(mx, my))
		{
			dragging = true;
		}
		if (dragging && !mouseDown(MOUSE_LMB))
		{
			dragging = false;
		}
		if (dragging && barRect.height() > 0 && lineCount > 0)
		{
			const float at = static_cast<float>(my - barRect.y()) / static_cast<float>(barRect.height());
			firstRow = std::max(0, static_cast<int>(std::lround(at * lineCount - linesShown / 2.f)));
		}
	}

	bool canConsumeWheelScroll() override { return hiddenRows > 0 || firstRow > 0; }

	// Touch: a drag moves the lines (whole lines, as the wheel), a flick goes on, a press stops it
	WzDragAxes touchDragAxes() override { return (hiddenRows > 0 || firstRow > 0) ? WzDragAxes::Vertical : WzDragAxes::None; }

	bool interceptsTouchPress() override
	{
		if (flingVelocity == 0.f)
		{
			return false;
		}
		flingVelocity = 0.f;
		return true;
	}

	bool isTouchScrolling() const override { return touchDragging || flingVelocity != 0.f; }

	void mouseDragged(WIDGET_KEY, W_CONTEXT *start, W_CONTEXT *current) override
	{
		if (!touchDragging)
		{
			touchDragging = true;
			flingVelocity = 0.f;
			dragStartFirst = firstRow;
			touchSamples.clear();
		}
		const int moved = static_cast<int>(std::lround(static_cast<float>(start->my - current->my) / static_cast<float>(std::max(1, lineStep))));
		firstRow = std::max(0, std::min(dragStartFirst + moved, lineCount - linesShown));
		touchSamples.emplace_back(realTime, current->my);
		while (touchSamples.size() > 2 && realTime - touchSamples.front().first > 100)
		{
			touchSamples.erase(touchSamples.begin());
		}
	}

	void released(W_CONTEXT *, WIDGET_KEY) override
	{
		if (!touchDragging)
		{
			return;
		}
		touchDragging = false;
		if (touchSamples.size() >= 2 && realTime - touchSamples.back().first < 60 && touchSamples.back().first > touchSamples.front().first)
		{
			const float velocity = -static_cast<float>(touchSamples.back().second - touchSamples.front().second) / static_cast<float>(touchSamples.back().first - touchSamples.front().first);
			if (std::abs(velocity) > 0.15f)
			{
				flingVelocity = std::max(-6.f, std::min(6.f, velocity));
				flingTravel = 0.f;
				flingLastTime = realTime;
			}
		}
		touchSamples.clear();
	}

	/// A group of rows: a team (its name, and won or lost), the winners or the losers of a free-for-all, or all
	struct RowGroup
	{
		std::string name;
		std::string state;
		bool won = false;
		std::vector<size_t> players;
	};

	std::vector<RowGroup> makeRowGroups() const
	{
		std::vector<RowGroup> out;
		if (data->teamGame)
		{
			for (size_t side = 0; side < data->sides.size(); ++side)
			{
				const bool lost = std::any_of(data->sides[side].begin(), data->sides[side].end(), [this](size_t p) { return data->players[p].lost; });
				out.push_back({data->sideNames[side], data->sideWon[side] ? _("won") : (lost ? _("lost") : ""), data->sideWon[side], data->sides[side]});
			}
			return out;
		}
		RowGroup all;
		RowGroup won{std::string(), _("won"), true, {}};
		RowGroup lost{std::string(), _("lost"), false, {}};
		for (const std::vector<size_t> &side : data->sides)
		{
			for (size_t p : side)
			{
				all.players.push_back(p);
				(data->sideWon[data->players[p].side] ? won : lost).players.push_back(p);
			}
		}
		if (!data->resultsKnown)
		{
			return {all};
		}
		for (RowGroup *g : {&won, &lost})
		{
			if (!g->players.empty())
			{
				out.push_back(*g);
			}
		}
		return out;
	}

	/// The standouts' block, under the rows: a gap, the heading (not compact), the cards
	int standoutsGap() const { return compact ? 4 : 18; }
	int standoutsHeadingH() const { return compact ? 0 : 18; }
	int standoutCardH() const { return compact ? 28 : 58; }
	int standoutsH() const { return standoutsGap() + standoutsHeadingH() + standoutCardH(); }

	/// The standouts: a card each (compact: two, on a line each), in standoutsH() from the top given,
	/// fading in from `delayMs`
	void displayStandouts(int left, int right, int top, float delayMs)
	{
		const HudColors &colors = currentHudSkin().colors();
		// (Compact: two, the second about other players than the first where possible)
		std::vector<size_t> shown;
		for (size_t k = 0; k < data->standouts.size() && shown.size() < 4; ++k)
		{
			shown.push_back(k);
		}
		if (compact && shown.size() > 2)
		{
			const std::vector<size_t> &firsts = data->standouts[0].players;
			size_t second = 1;
			for (size_t k = 1; k < data->standouts.size(); ++k)
			{
				const std::vector<size_t> &ps = data->standouts[k].players;
				if (std::none_of(ps.begin(), ps.end(), [&](size_t p) { return std::find(firsts.begin(), firsts.end(), p) != firsts.end(); }))
				{
					second = k;
					break;
				}
			}
			shown = {0, second};
		}
		const size_t count = shown.size();
		if (count == 0)
		{
			return;
		}
		top += standoutsGap();
		if (!compact)
		{
			texts.draw(upper(_("Standouts")), RESULTS_LABEL_FONT, left, top, faint());
		}
		top += standoutsHeadingH();
		const int gap = compact ? 6 : 10;
		const int cardW = (right - left - gap * static_cast<int>(count - 1)) / static_cast<int>(count);
		const int cardH = standoutCardH();
		if (standoutChips.size() < count * 2)
		{
			standoutChips.resize(count * 2);
			standoutLines.resize(count * 3);
		}
		for (size_t k = 0; k < count; ++k)
		{
			const HudResultsStandout &s = data->standouts[shown[k]];
			const float cardIn = hudResultsAppear(delayMs + 60.f * k, 320.f);
			const int cx = left + static_cast<int>(k) * (cardW + gap);
			pie_DrawRoundedBox(static_cast<float>(cx), static_cast<float>(top), static_cast<float>(cx + cardW), static_cast<float>(top + cardH),
			                   hudResultsSurface(pal_RGBA(20, 27, 37, 255)), compact ? 5.f : 7.f, hudResultsSurface(pal_RGBA(33, 44, 59, 255)), 1.f);
			// Its players' chips (two at most), then the title, who and how much
			const int chip = compact ? 18 : 26;
			int tx = cx + (compact ? 7 : 12);
			for (size_t q = 0; q < s.players.size() && q < 2; ++q)
			{
				drawPositionChip(standoutChips[k * 2 + q], data->players[s.players[q]].index, WzRect(tx, top + (cardH - chip) / 2, chip, chip));
				tx += chip + 3;
			}
			tx += compact ? 4 : 8;
			std::string who = data->players[s.players.front()].name;
			if (s.players.size() == 2)
			{
				who = astringf(_("%s and %s"), who.c_str(), data->players[s.players[1]].name.c_str());
			}
			else if (s.players.size() > 2)
			{
				who = astringf(_("%s and %zu others"), who.c_str(), s.players.size() - 1);
			}
			const int availableWidth = cx + cardW - 8 - tx;
			if (compact)
			{
				drawLine(standoutLines[k * 3], upper(s.title) + "  ·  " + who + "  ·  " + s.value, font_small, tx, top + 8, availableWidth, colors.textSecondary);
			}
			else
			{
				drawLine(standoutLines[k * 3], upper(s.title), RESULTS_LABEL_FONT, tx, top + 9, availableWidth, faint());
				drawLine(standoutLines[k * 3 + 1], who, font_regular_bold, tx, top + 22, availableWidth, colors.textPrimary);
				drawLine(standoutLines[k * 3 + 2], s.value, font_small, tx, top + 39, availableWidth, colors.textSecondary);
			}
			if (cardIn < 1.f)
			{
				pie_DrawRoundedBox(static_cast<float>(cx - 1), static_cast<float>(top - 1), static_cast<float>(cx + cardW + 1), static_cast<float>(top + cardH + 1),
				                   alpha(hudResultsSurface(pal_RGBA(14, 18, 24, 255)), static_cast<uint8_t>(255 * (1.f - cardIn))), compact ? 5.f : 7.f);
			}
		}
	}

	void display(int xOffset, int yOffset) override
	{
		const HudColors &colors = currentHudSkin().colors();
		texts.reset();
		const int x0 = xOffset + x();
		const int y0 = yOffset + y();
		const int left = x0 + (compact ? 10 : PAD);
		const int right = x0 + width() - (compact ? 10 : PAD);
		const int segRight = segments.empty() ? left : xOffset + x() + segments.back()->x() + segments.back()->width();
		if (group == Group::Research && !compact)
		{
			// The weapon lines' key, by the segments
			int lx = segRight + 24;
			for (const HudResearchLine &line : data->lines)
			{
				const int w = texts.width(line.name, font_small);
				if (lx + 12 + w > right)
				{
					break;
				}
				pie_DrawRoundedBox(static_cast<float>(lx), static_cast<float>(y0 + 21), static_cast<float>(lx + 8), static_cast<float>(y0 + 29), line.color, 2.f);
				texts.draw(line.name, font_small, lx + 12, y0 + 18, colors.textSecondary);
				lx += 12 + w + 12;
			}
		}

		struct Column
		{
			std::string label;
			std::string sub;
			std::function<double (const HudResultsPlayer &)> value;
			std::function<double (const HudResultsPlayer &)> second;   // (All: "a / b")
			enum Kind { Bar, Spark, Categories, Pair } kind;
			PIELIGHT color;
			bool lowerIsBetter;
			bool percent;
			float weight = 1.f;
		};
		std::vector<Column> cols;
		switch (group)
		{
		case Group::Combat:
			cols = {{_("Kills"), _("units"), [](const HudResultsPlayer &p) { return p.kills; }, nullptr, Column::Bar, DATA_COLOR, false, false},
			        {_("Kills"), _("structures"), [](const HudResultsPlayer &p) { return p.structureKills; }, nullptr, Column::Bar, DATA_COLOR, false, false},
			        {_("Units lost"), "", [](const HudResultsPlayer &p) { return p.lost_; }, nullptr, Column::Bar, LOSS_COLOR, true, false},
			        {_("Power destroyed"), "", [](const HudResultsPlayer &p) { return static_cast<double>(p.powerDestroyed); }, nullptr, Column::Bar, KILL_COLOR, false, false},
			        {_("Power lost"), "", [](const HudResultsPlayer &p) { return static_cast<double>(p.powerLost); }, nullptr, Column::Bar, LOSS_COLOR, true, false}};
			break;
		case Group::Economy:
			cols = {{_("Derricks"), _("at end"), [](const HudResultsPlayer &p) { return p.derricks; }, nullptr, Column::Bar, GOLD, false, false},
			        {_("Derricks"), _("peak"), [](const HudResultsPlayer &p) { return p.derricksPeak; }, nullptr, Column::Bar, GOLD, false, false},
			        {_("Derricks"), _("over the game"), nullptr, nullptr, Column::Spark, GOLD, false, false},
			        {_("Structures"), _("built"), [](const HudResultsPlayer &p) { return p.structuresBuilt; }, nullptr, Column::Bar, DATA_COLOR, false, false},
			        {_("Structures"), _("lost"), [](const HudResultsPlayer &p) { return p.structuresLost; }, nullptr, Column::Bar, LOSS_COLOR, true, false}};
			break;
		case Group::Research:
			cols = {{_("Topics"), _("by own labs"), [](const HudResultsPlayer &p) { return p.ownResearch; }, nullptr, Column::Bar, DATA_COLOR, false, false},
			        {_("Lab use"), "", [](const HudResultsPlayer &p) { return p.labUse; }, nullptr, Column::Bar, DATA_COLOR, false, true},
			        {_("Firsts"), _("main technologies"), [](const HudResultsPlayer &p) { return p.firsts; }, nullptr, Column::Bar, GOLD, false, false},
			        {_("By weapon line"), "", nullptr, nullptr, Column::Categories, DATA_COLOR, false, false}};
			break;
		case Group::All:
			cols = {{_("Kills"), _("units / structures"), [](const HudResultsPlayer &p) { return p.kills; }, [](const HudResultsPlayer &p) { return p.structureKills; }, Column::Pair, DATA_COLOR, false, false},
			        {_("Units"), _("built / lost"), [](const HudResultsPlayer &p) { return p.built; }, [](const HudResultsPlayer &p) { return p.lost_; }, Column::Pair, DATA_COLOR, false, false},
			        {_("Structures"), _("built / lost"), [](const HudResultsPlayer &p) { return p.structuresBuilt; }, [](const HudResultsPlayer &p) { return p.structuresLost; }, Column::Pair, DATA_COLOR, false, false},
			        {_("Power"), _("destroyed / lost"), [](const HudResultsPlayer &p) { return static_cast<double>(p.powerDestroyed); }, [](const HudResultsPlayer &p) { return static_cast<double>(p.powerLost); }, Column::Pair, DATA_COLOR, false, false},
			        {_("Derricks"), _("end / peak"), [](const HudResultsPlayer &p) { return p.derricks; }, [](const HudResultsPlayer &p) { return p.derricksPeak; }, Column::Pair, DATA_COLOR, false, false},
			        {_("Research"), _("own labs"), [](const HudResultsPlayer &p) { return p.ownResearch; }, nullptr, Column::Pair, DATA_COLOR, false, false},
			        {_("Lab use"), "", [](const HudResultsPlayer &p) { return p.labUse; }, nullptr, Column::Pair, DATA_COLOR, false, true}};
			break;
		}

		if (group == Group::All)
		{
			const float weights[] = {1.15f, 1.f, 1.f, 1.55f, 0.95f, 0.8f, 0.75f};
			for (size_t c = 0; c < cols.size(); ++c)
			{
				cols[c].weight = weights[c];
			}
		}

		// The rows, in groups (a team game's teams, or in a free-for-all who won and who lost, when known),
		// and the standouts under them when they leave the rows room for four lines (or all of them)
		const std::vector<RowGroup> rowGroups = makeRowGroups();
		struct Line
		{
			const RowGroup *group;
			nonstd::optional<size_t> player;   ///< (none: the group's heading)
		};
		std::vector<Line> lines;
		int headings = 0;
		for (const RowGroup &g : rowGroups)
		{
			if (!g.name.empty() || !g.state.empty())
			{
				lines.push_back({&g, nonstd::nullopt});
				++headings;
			}
			for (size_t p : g.players)
			{
				lines.push_back({&g, p});
			}
		}
		const int tableTop = y0 + (compact ? 32 : 52);
		const int headH = compact ? 28 : 40;
		const int headingH = compact ? 20 : 30;
		const int minRow = compact ? 22 : 30;
		const int players = std::max<int>(1, static_cast<int>(data->players.size()));
		const int availableBodyHeight = y0 + height() - tableTop - headH - 4;
		int standoutsHeight = data->standouts.empty() ? 0 : standoutsH();
		if (availableBodyHeight - standoutsHeight < std::min(headings * headingH + players * minRow, 4 * minRow))
		{
			standoutsHeight = 0;
		}
		const int avail = availableBodyHeight - standoutsHeight;
		const int rowH = std::max(minRow, std::min(46, (avail - headings * headingH) / players));
		lineStep = rowH;
		auto lineH = [&](const Line &l) { return l.player ? rowH : headingH; };
		// The lines that fit, from the first shown (no later than the last screen's)
		int lastFirst = static_cast<int>(lines.size());
		for (int availableHeight = avail; lastFirst > 0 && availableHeight - lineH(lines[static_cast<size_t>(lastFirst - 1)]) >= 0; --lastFirst)
		{
			availableHeight -= lineH(lines[static_cast<size_t>(lastFirst - 1)]);
		}
		firstRow = std::max(0, std::min(firstRow, lastFirst));
		int shownLines = 0;
		for (int availableHeight = avail, k = firstRow; k < static_cast<int>(lines.size()) && availableHeight - lineH(lines[static_cast<size_t>(k)]) >= 0; ++k, ++shownLines)
		{
			availableHeight -= lineH(lines[static_cast<size_t>(k)]);
		}
		hiddenRows = static_cast<int>(lines.size()) - firstRow - shownLines;
		lineCount = static_cast<int>(lines.size());
		linesShown = shownLines;
		const int nameW = compact ? 150 : (group == Group::All ? 210 : std::min(300, std::max(180, (right - left) / 4)));
		const int scoreW = compact ? 76 : (group == Group::All ? 96 : 110);
		const int colsX = left + nameW + scoreW;
		float weightSum = 0.f;
		for (const Column &c : cols)
		{
			weightSum += c.weight;
		}
		std::vector<int> colLeft(cols.size() + 1, colsX);
		for (size_t c = 0; c < cols.size(); ++c)
		{
			colLeft[c + 1] = colLeft[c] + static_cast<int>((right - colsX) * cols[c].weight / weightSum);
		}

		texts.draw(upper(_("Player")), RESULTS_LABEL_FONT, left + (compact ? 6 : 14), tableTop + (compact ? 14 : 20), faint());
		texts.draw(upper(_("Score")), RESULTS_LABEL_FONT, left + nameW + scoreW - 10, tableTop + (compact ? 14 : 20), faint(), true);
		for (size_t c = 0; c < cols.size(); ++c)
		{
			const int cr = colLeft[c + 1] - 10;
			std::string label = upper(cols[c].label);
			std::string sub = upper(cols[c].sub);
			PIELIGHT subColor = faint(110);
			// A heading wider than its column, without a second line: on two
			const size_t space = label.find(' ');
			if (sub.empty() && space != std::string::npos && texts.width(label, RESULTS_LABEL_FONT) > colLeft[c + 1] - colLeft[c] - 14)
			{
				sub = label.substr(space + 1);
				label = label.substr(0, space);
				subColor = faint();
			}
			texts.draw(label, RESULTS_LABEL_FONT, cr, tableTop + (compact ? 2 : 8), faint(), true);
			if (!sub.empty())
			{
				texts.draw(sub, RESULTS_LABEL_FONT, cr, tableTop + (compact ? 14 : 22), subColor, true);
			}
		}
		drawDivider(left, tableTop + headH - 1, right, tableTop + headH);

		std::vector<double> best(cols.size(), 0.0), most(cols.size(), 1.0), bestSecond(cols.size(), 0.0);
		for (size_t c = 0; c < cols.size(); ++c)
		{
			bool any = false;
			for (const HudResultsPlayer &p : data->players)
			{
				if (!cols[c].value)
				{
					continue;
				}
				const double v = cols[c].value(p);
				most[c] = std::max(most[c], v);
				best[c] = !any ? v : (cols[c].lowerIsBetter ? std::min(best[c], v) : std::max(best[c], v));
				if (cols[c].second)
				{
					const double s = cols[c].second(p);
					const bool lowerSecond = c != 4; // lower is better for losses (All: built / lost), higher for derricks
					bestSecond[c] = !any ? s : (lowerSecond ? std::min(bestSecond[c], s) : std::max(bestSecond[c], s));
				}
				any = true;
			}
		}
		// A bar sits by its column's numbers, right-aligned: just left of the widest of them
		auto barText = [](const Column &col, double v) {
			return col.percent ? astringf("%.1f%%", v) : hudChartNumber(static_cast<float>(v));
		};
		std::vector<int> numbersW(cols.size(), 0);
		for (size_t c = 0; c < cols.size(); ++c)
		{
			if (cols[c].kind != Column::Bar)
			{
				continue;
			}
			for (const HudResultsPlayer &p : data->players)
			{
				numbersW[c] = std::max(numbersW[c], texts.width(barText(cols[c], cols[c].value(p)), font_regular));
			}
		}
		int ry = tableTop + headH;
		size_t chipIndex = 0;
		for (int k = firstRow; k < firstRow + shownLines; ++k)
		{
			const Line &row = lines[static_cast<size_t>(k)];
			if (!row.player)
			{
				const RowGroup &g = *row.group;
				const int ty = ry + (compact ? 5 : 11);
				int w = 0;
				if (!g.name.empty())
				{
					w = texts.draw(upper(g.name) + (g.state.empty() ? "" : "  ·  "), RESULTS_LABEL_FONT, left + 14, ty, faint());
				}
				if (!g.state.empty())
				{
					texts.draw(upper(g.state), RESULTS_LABEL_FONT, left + 14 + w, ty, g.won ? colors.stateOk : faint());
				}
				ry += headingH;
				drawDivider(left, ry - 1, right, ry);
				continue;
			}
			{
				const size_t pi = *row.player;
				const HudResultsPlayer &p = data->players[pi];
				// On entering: the rows come in one after another, their bars growing and numbers counting up
				const int shownRow = k - firstRow;
				const float rowIn = hudResultsAppear(260.f + 60.f * shownRow, 320.f);
				const float grow = hudResultsAppear(380.f + 60.f * shownRow, 560.f);
				if (p.you)
				{
					pie_UniTransBoxFill(static_cast<float>(left), static_cast<float>(ry), static_cast<float>(right), static_cast<float>(ry + rowH), hudResultsChosen(pal_RGBA(21, 32, 45, 255)));
					pie_UniTransBoxFill(static_cast<float>(left), static_cast<float>(ry), static_cast<float>(left + 3), static_cast<float>(ry + rowH), colors.accent);
				}
				const int cy = ry + rowH / 2;
				if (chips.size() <= chipIndex)
				{
					chips.resize(chipIndex + 1);
				}
				const int chip = compact ? 18 : 22;
				drawPositionChip(chips[chipIndex++], p.index, WzRect(left + (compact ? 8 : 14), cy - chip / 2, chip, chip));
				const int nx = left + (compact ? 34 : 46);
				const int twoLines = !p.about.empty() && rowH >= 36;
				const int nameTop = twoLines ? cy - 15 : cy - static_cast<int>(iV_GetTextLineSize(font_regular_bold)) / 2;
				const iV_fonts scoreFont = compact ? font_regular_bold : font_medium_bold;
				const std::string score = hudFullNumber(p.score);
				const int scoreR = left + nameW + scoreW - 10;
				// A long name stops short of the score
				const int maxNameWidth = scoreR - texts.width(score, scoreFont) - 16 - nx;
				const int youW = p.you ? texts.width(_("YOU"), font_small) + 6 : 0;
				const int nameMax = std::max(30, maxNameWidth - youW);
				if (nameTexts.size() < data->players.size())
				{
					nameTexts.resize(data->players.size());
					abouts.resize(data->players.size());
				}
				drawLine(nameTexts[pi], p.name, font_regular_bold, nx, nameTop, nameMax, colors.textPrimary);
				if (p.you)
				{
					texts.draw(_("YOU"), font_small, nx + std::min(nameMax, texts.width(p.name, font_regular_bold)) + 6, nameTop + 3, colors.accent);
				}
				if (twoLines)
				{
					drawLine(abouts[pi], p.about, font_small, nx, cy + 3, std::max(30, maxNameWidth), faint(150));
				}
				texts.draw(grow < 1.f ? hudFullNumber(static_cast<long long>(std::llround(p.score * grow))) : score, scoreFont, scoreR, cy - static_cast<int>(iV_GetTextLineSize(scoreFont)) / 2, p.score < 0 ? faint(200) : colors.textPrimary, true);
				for (size_t c = 0; c < cols.size(); ++c)
				{
					const int cl = colLeft[c];
					const int cr = colLeft[c + 1] - 10;
					const Column &col = cols[c];
					switch (col.kind)
					{
					case Column::Bar:
					{
						const double v = col.value(p);
						texts.draw(barText(col, grow < 1.f ? std::round(v * grow) : v), font_regular, cr, cy - 8, v == best[c] ? colors.textPrimary : alpha(colors.textPrimary, 215), true);
						const int barR = cr - numbersW[c] - (compact ? 6 : 10);
						const int barW = std::min(70, barR - cl - (compact ? 8 : 16));
						if (barW > 16)
						{
							pie_DrawRoundedBox(static_cast<float>(barR - barW), static_cast<float>(cy - 3), static_cast<float>(barR), static_cast<float>(cy + 3), hudResultsSurface(pal_RGBA(28, 36, 48, 255)), 3.f);
							const float f = grow * static_cast<float>(std::min(1.0, v / (col.percent ? 100.0 : most[c])));
							if (f > 0.f)
							{
								pie_DrawRoundedBox(static_cast<float>(barR - barW), static_cast<float>(cy - 3), barR - barW + std::max(3.f, barW * f), static_cast<float>(cy + 3), col.color, 3.f);
							}
						}
						break;
					}
					case Column::Spark:
					{
						if (sparks.size() <= pi)
						{
							sparks.resize(data->players.size());
						}
						if (sparks[pi].empty())
						{
							const std::vector<float> &v = data->values(HudResultsMetric::Derricks, pi);
							float hi = 1.f;
							for (float f : v)
							{
								hi = std::max(hi, f);
							}
							std::vector<glm::vec2> pts;
							const float w = static_cast<float>(std::min(120, cr - cl - 4)), h = compact ? 16.f : 22.f;
							for (size_t i = 0; i < v.size(); ++i)
							{
								pts.emplace_back(w * i / std::max<size_t>(1, v.size() - 1), h - h * v[i] / hi);
							}
							PIELIGHT top = GOLD;
							top.byte.a = 45;
							PIELIGHT bottom = GOLD;
							bottom.byte.a = 6;
							sparks[pi].addArea(pts, h, top, bottom);
							sparks[pi].addPolyline(pts, 1.4f, GOLD);
						}
						sparks[pi].draw(static_cast<float>(cr - std::min(120, cr - cl - 4)), static_cast<float>(cy - (compact ? 8 : 11)));
						break;
					}
					case Column::Categories:
					{
						std::map<std::string, int> count;
						int total = 0;
						for (const HudResultsResearch &e : data->research)
						{
							if (e.player == pi)
							{
								count[e.line]++;
								total++;
							}
						}
						int mostTotal = 1;
						for (const HudResultsPlayer &q : data->players)
						{
							mostTotal = std::max(mostTotal, static_cast<int>(q.ownResearch));
						}
						const float full = static_cast<float>(std::min(190, cr - cl - 4));
						float bx = static_cast<float>(cr) - full * total / mostTotal;
						for (const HudResearchLine &line : data->lines)
						{
							const int n = count[line.key];
							if (!n)
							{
								continue;
							}
							const float w = full * n / mostTotal;
							pie_UniTransBoxFill(bx, static_cast<float>(cy - 4), bx + std::max(1.f, w - 1.f), static_cast<float>(cy + 4), line.color);
							bx += w;
						}
						break;
					}
					case Column::Pair:
					{
						const double a = col.value(p);
						const std::string at = col.percent ? astringf("%.1f%%", a) : hudChartNumber(static_cast<float>(a));
						if (col.second)
						{
							const double b = col.second(p);
							const int bw = texts.draw(hudChartNumber(static_cast<float>(b)), font_regular, cr, cy - 8, colors.textSecondary, true);
							if (b == bestSecond[c])
							{
								pie_DrawRoundedBox(static_cast<float>(cr + 3), static_cast<float>(cy - 2), static_cast<float>(cr + 8), static_cast<float>(cy + 3), GOLD, 2.5f);
							}
							const int sx = cr - bw - 8;
							texts.draw("/", font_regular, sx, cy - 8, hudResultsSurface(pal_RGBA(58, 70, 88, 255)), true);
							const int ar = sx - 14;
							texts.draw(at, font_regular, ar, cy - 8, colors.textPrimary, true);
							if (a == best[c])
							{
								pie_DrawRoundedBox(static_cast<float>(ar + 3), static_cast<float>(cy - 2), static_cast<float>(ar + 8), static_cast<float>(cy + 3), GOLD, 2.5f);
							}
						}
						else
						{
							texts.draw(at, font_regular, cr, cy - 8, colors.textPrimary, true);
							if (a == best[c])
							{
								pie_DrawRoundedBox(static_cast<float>(cr + 3), static_cast<float>(cy - 2), static_cast<float>(cr + 8), static_cast<float>(cy + 3), GOLD, 2.5f);
							}
						}
						break;
					}
					}
				}
				if (rowIn < 1.f)
				{
					pie_UniTransBoxFill(static_cast<float>(left), static_cast<float>(ry), static_cast<float>(right), static_cast<float>(ry + rowH),
					                    alpha(hudResultsSurface(pal_RGBA(14, 18, 24, 255)), static_cast<uint8_t>(255 * (1.f - rowIn))));
				}
				ry += rowH;
				drawDivider(left, ry - 1, right, ry);
			}
		}

		// More lines than fit: a thin bar by them, to drag (the wheel scrolls them too)
		barRect = WzRect();
		if (hiddenRows > 0 || firstRow > 0)
		{
			const int top = tableTop + headH + 2;
			const int h = avail - 4;
			const int bx = right + (compact ? 3 : 8);
			barRect = WzRect(bx - x0 - 4, top - y0, 11, h);
			pie_DrawRoundedBox(static_cast<float>(bx), static_cast<float>(top), static_cast<float>(bx + 3), static_cast<float>(top + h), alpha(colors.textPrimary, 16), 1.5f);
			const float total = static_cast<float>(lines.size());
			const float t0 = top + h * firstRow / total;
			const float t1 = top + h * (firstRow + shownLines) / total;
			pie_DrawRoundedBox(static_cast<float>(bx), t0, static_cast<float>(bx + 3), t1, alpha(colors.textSecondary, dragging ? 200 : 140), 1.5f);
		}

		// The standouts
		if (standoutsHeight > 0)
		{
			// (At most standoutsH() above the end of the rows' room, so within availableBodyHeight and above the footer.
			// They fade in after the rows, as the next line would)
			displayStandouts(left, right, std::min(ry, tableTop + headH + avail), 260.f + 60.f * shownLines);
		}
	}

private:
	const HudGameResults *data = nullptr;
	bool compact = false;
	int firstRow = 0;      ///< (the first line shown: a heading or a player)
	int hiddenRows = 0;    ///< (the lines below those shown)
	int lineCount = 0;
	int linesShown = 0;
	WzRect barRect;        ///< (the scroll bar's, in the view)
	bool dragging = false;
	// Touch scrolling (touchDragAxes)
	int lineStep = 30;     ///< (a row's height: a drag's or flick's travel per line)
	bool touchDragging = false;
	int dragStartFirst = 0;
	std::vector<std::pair<uint32_t, int>> touchSamples;
	float flingVelocity = 0.f;
	float flingTravel = 0.f;
	uint32_t flingLastTime = 0;
	std::vector<ChipText> standoutChips;
	std::vector<WzText> standoutLines;
	Group group = Group::Combat;
	std::vector<std::shared_ptr<ChoiceButton>> segments;
	Texts texts;
	std::vector<ChipText> chips;
	std::vector<PieShapes> sparks;
	std::vector<WzText> nameTexts;
	std::vector<WzText> abouts;
};

// ---------------------------------------------------------------- Charts

class ChartsView : public WIDGET
{
	static constexpr int RAIL_W = 232;

public:
	void initialize(const HudGameResults *results, bool compactLayout)
	{
		data = results;
		compact = compactLayout;
		const HudResultsMetric metrics[] = {HudResultsMetric::ArmyValue, HudResultsMetric::Army, HudResultsMetric::Derricks, HudResultsMetric::Kills, HudResultsMetric::PowerDestroyed,
		                                    HudResultsMetric::PowerLost, HudResultsMetric::Research, HudResultsMetric::LabUse, HudResultsMetric::Structures,
		                                    HudResultsMetric::Score};
		for (HudResultsMetric m : metrics)
		{
			auto b = makeChoice(ChoiceButton::Look::Metric, hudResultsMetricName(m));
			const auto sides = sparkSides(m);
			b->spark = data->sideValues(m, sides.first);
			if (sides.second)
			{
				b->rivalSpark = data->sideValues(m, *sides.second);
			}
			b->chosen = m == metric;
			setSparkColors(*b, m);
			b->addOnClickHandler([this, m](W_BUTTON &) { choose(m); });
			attach(b);
			rail.emplace_back(m, b);
			if (compact)
			{
				b->hide();
			}
		}
		if (compact)
		{
			// Instead of the list: the previous and next metric
			for (int k = 0; k < 2; ++k)
			{
				auto b = makeChoice(ChoiceButton::Look::Segment, k == 0 ? "‹" : "›");
				b->first = k == 0;
				b->last = k == 1;
				b->addOnClickHandler([this, k](W_BUTTON &) {
					size_t at = 0;
					for (size_t q = 0; q < rail.size(); ++q)
					{
						if (rail[q].first == metric)
						{
							at = q;
						}
					}
					at = (at + (k == 0 ? rail.size() - 1 : 1)) % rail.size();
					choose(rail[at].first);
				});
				attach(b);
				stepper.push_back(b);
			}
		}
		if (data->teamGame)
		{
			for (int k = 0; k < 2; ++k)
			{
				auto b = makeChoice(ChoiceButton::Look::Segment, k == 0 ? _("Players") : _("Teams"));
				b->first = k == 0;
				b->last = k == 1;
				b->chosen = k == 0;
				b->addOnClickHandler([this, k](W_BUTTON &) {
					byTeam = k == 1;
					for (size_t q = 0; q < teamSwitch.size(); ++q)
					{
						teamSwitch[q]->chosen = static_cast<int>(q) == k;
					}
					refresh();
				});
				attach(b);
				teamSwitch.push_back(b);
			}
		}
		chart = HudChart::make(HudChart::Mode::Lines);
		chart->setFill(true);
		attach(chart);
		if (data->teamGame && data->sides.size() == 2 && !compact)
		{
			lead = HudChart::make(HudChart::Mode::Lead);
			attach(lead);
		}
		band = HudTimeBand::make(HudTimeBand::Look::Band);
		attach(band);
		chart->onCursor = [this](nonstd::optional<size_t> index) {
			band->setCursorTime(index ? nonstd::optional<float>(data->times[*index]) : nonstd::nullopt);
			if (lead)
			{
				lead->setCursor(index);
			}
		};
		refresh();
	}

	/// The sides a metric's sparkline sets against each other: a player's against its strongest rival in it (the most on
	/// average over the game: the runner-up if the player's side had the most), a spectator's the side with the most
	/// against the next
	std::pair<size_t, nonstd::optional<size_t>> sparkSides(HudResultsMetric m) const
	{
		std::vector<std::pair<float, size_t>> byMean;
		for (size_t s = 0; s < data->sides.size(); ++s)
		{
			const std::vector<float> v = data->sideValues(m, s);
			float sum = 0.f;
			for (float x : v)
			{
				sum += x;
			}
			byMean.emplace_back(v.empty() ? 0.f : sum / v.size(), s);
		}
		std::stable_sort(byMean.begin(), byMean.end(), [](const std::pair<float, size_t> &a, const std::pair<float, size_t> &b) { return a.first > b.first; });
		if (byMean.empty())
		{
			return {0, nonstd::nullopt};
		}
		const size_t mine = data->youIndex ? data->players[*data->youIndex].side : byMean.front().second;
		for (const auto &e : byMean)
		{
			if (e.second != mine)
			{
				return {mine, e.second};
			}
		}
		return {mine, nonstd::nullopt};
	}

	/// The chosen metric's in the player's color (a spectator's: the leading side's), the rest in grays
	void setSparkColors(ChoiceButton &b, HudResultsMetric m) const
	{
		if (m != metric)
		{
			b.setSparkColors(pal_RGBA(120, 132, 148, 255), pal_RGBA(84, 94, 108, 255));
			return;
		}
		const PIELIGHT mine = data->youIndex ? hudReadableColor(data->players[*data->youIndex].color) : hudReadableColor(sideColor(*data, sparkSides(m).first));
		b.setSparkColors(mine, pal_RGBA(139, 149, 158, 255));
	}

	void choose(HudResultsMetric m)
	{
		metric = m;
		for (auto &r : rail)
		{
			r.second->chosen = r.first == m;
			setSparkColors(*r.second, r.first);
		}
		refresh();
		reveal();
	}

	/// Draws the charts in
	void reveal()
	{
		for (HudChart *c : {chart.get(), lead.get()})
		{
			if (c)
			{
				c->setBackground(hudResultsSurface(pal_RGBA(14, 18, 24, 255)));
				c->reveal();
			}
		}
	}

	/// Research completed, when allies share it: each ally's the team's, so by team only
	bool teamsOnly() const
	{
		return metric == HudResultsMetric::Research && data->sharedResearch;
	}

	void refresh()
	{
		for (auto &b : teamSwitch)
		{
			b->show(!teamsOnly());
		}
		const bool teams = byTeam || teamsOnly();
		std::vector<HudChartSeries> series;
		if (teams)
		{
			for (size_t s = 0; s < data->sides.size(); ++s)
			{
				HudChartSeries cs;
				cs.label = data->sideNames[s];
				cs.color = sideColor(*data, s);
				cs.values = data->sideValues(metric, s);
				cs.emphasized = data->youIndex && data->players[*data->youIndex].side == s;
				series.push_back(cs);
			}
		}
		else
		{
			for (size_t p = 0; p < data->players.size(); ++p)
			{
				HudChartSeries cs;
				cs.label = data->players[p].name;
				cs.color = data->players[p].color;
				cs.values = data->values(metric, p);
				cs.emphasized = data->players[p].you;
				cs.chip = data->players[p].index;
				cs.withChip = true;
				series.push_back(cs);
			}
		}
		chart->setTimes(data->times, data->end);
		chart->setSeries(series);
		chart->setUnit(hudResultsMetricUnit(metric));
		chart->setValueDecimals(metric == HudResultsMetric::LabUse ? 1 : 0);
		chart->setFixedMax(metric == HudResultsMetric::LabUse ? nonstd::optional<float>(100.f) : nonstd::nullopt);
		chart->setTooltipTitle(hudResultsMetricName(metric));
		if (data->teamGame && !teams)
		{
			chart->tooltipExtra = [this](size_t i) {
				std::vector<std::pair<std::string, std::string>> rows;
				for (size_t s = 0; s < data->sides.size(); ++s)
				{
					const std::vector<float> v = data->sideValues(metric, s);
					rows.emplace_back(data->sideNames[s], i < v.size() ? (metric == HudResultsMetric::LabUse ? astringf("%.1f", v[i]) : hudChartNumber(v[i])) + hudResultsMetricUnit(metric) : std::string());
				}
				return rows;
			};
		}
		else
		{
			chart->tooltipExtra = nullptr;
		}
		if (lead)
		{
			const std::vector<float> a = data->sideValues(metric, 0);
			const std::vector<float> b = data->sideValues(metric, 1);
			HudChartSeries diff;
			for (size_t i = 0; i < a.size() && i < b.size(); ++i)
			{
				diff.values.push_back(a[i] - b[i]);
			}
			lead->setTimes(data->times, data->end);
			lead->setSeries({diff});
			lead->setLeadColors(sideColor(*data, 0), sideColor(*data, 1), data->sideNames[0], data->sideNames[1]);
		}

		// Who led: the biggest, by at least a fifth (and, for the army, 4 units or HUD_LEAD_VALUE)
		float top = 1.f;
		for (size_t p = 0; p < data->players.size(); ++p)
		{
			for (float v : data->values(metric, p))
			{
				top = std::max(top, v);
			}
		}
		const float least = metric == HudResultsMetric::Army ? 4.f : metric == HudResultsMetric::ArmyValue ? HUD_LEAD_VALUE : 0.04f * top;
		std::vector<HudTimeBandStretch> stretches;
		const float step = data->times.size() > 1 ? data->times[1] - data->times[0] : 15.f;
		for (size_t i = 0; i < data->times.size(); ++i)
		{
			nonstd::optional<size_t> leader;
			float v1 = -1e30f, v2 = -1e30f;
			size_t who = 0;
			for (size_t p = 0; p < data->players.size(); ++p)
			{
				const std::vector<float> &v = data->values(metric, p);
				const float x = i < v.size() ? v[i] : 0.f;
				if (x > v1)
				{
					v2 = v1;
					v1 = x;
					who = p;
				}
				else if (x > v2)
				{
					v2 = x;
				}
			}
			if (data->players.size() > 1 && v1 - v2 >= least && v1 - v2 >= hudLeadShare(data->players.size()) * std::fabs(v1))
			{
				leader = who;
			}
			const float from = data->times[i];
			const float to = std::min(data->end, from + step);
			const bool same = !stretches.empty() && ((leader && stretches.back().color && stretches.back().label == data->players[*leader].name)
			                                        || (!leader && !stretches.back().color));
			if (same)
			{
				stretches.back().to = to;
			}
			else
			{
				HudTimeBandStretch s;
				s.from = from;
				s.to = to;
				if (leader)
				{
					s.color = data->players[*leader].color;
					s.label = data->players[*leader].name;
					s.labelColor = hudReadableColor(*s.color);
				}
				stretches.push_back(s);
			}
		}
		for (HudTimeBandStretch &s : stretches)
		{
			if (s.to - s.from < 90.f)
			{
				s.label.clear();
			}
		}
		anyLed = std::any_of(stretches.begin(), stretches.end(), [](const HudTimeBandStretch &s) { return s.color.has_value(); });
		band->setStretches(stretches, data->end);
		band->setMargins(HUD_CHART_LEFT, HUD_CHART_RIGHT_LABELS);
		band->show(anyLed);
		geometryChanged();
	}

	void geometryChanged() override
	{
		if (compact)
		{
			for (size_t k = 0; k < stepper.size(); ++k)
			{
				stepper[k]->setGeometry(10 + static_cast<int>(k) * 24, 6, 24, 22);
			}
			for (size_t k = 0; k < teamSwitch.size(); ++k)
			{
				const int w = teamSwitch[k]->idealWidth();
				const int right = width() - 10 - (k == 0 ? teamSwitch[1]->idealWidth() : 0);
				teamSwitch[k]->setGeometry(right - w, 6, w, 22);
			}
			chart->setGeometry(4, 34, width() - 8, std::max(80, height() - 34 - (anyLed ? 56 : 8)));
			band->setGeometry(4, chart->y() + chart->height() + 22, width() - 8, 34);
			return;
		}
		const int railW = RAIL_W;
		const int railStep = std::max(24, std::min(32, (height() - 34) / std::max<int>(1, static_cast<int>(rail.size()))));
		int y = 12;
		for (auto &r : rail)
		{
			r.second->setGeometry(8, y + 18, railW - 16, railStep - 2);
			y += railStep;
		}
		const int cx = railW + 12;
		const int cw = width() - cx - 12;
		int top = 44;
		for (size_t k = 0; k < teamSwitch.size(); ++k)
		{
			const int w = teamSwitch[k]->idealWidth();
			const int right = width() - PAD - (k == 0 ? teamSwitch[1]->idealWidth() : 0);
			teamSwitch[k]->setGeometry(right - w, 10, w, 24);
		}
		// (each smaller chart's heading 16 px under what's above it, and room under the lead chart when it's last)
		const int reservedBottom = (anyLed ? 62 : 0) + (lead ? 104 : 0) + (lead && !anyLed ? 10 : 0);
		chart->setGeometry(cx, top, cw, std::max(120, height() - top - reservedBottom - 8));
		top = chart->y() + chart->height() + 14;
		if (lead)
		{
			lead->setGeometry(cx, top + 22, cw, 70);
			top += 104;
		}
		band->setGeometry(cx, top + 22, cw, 34);
	}

	void display(int xOffset, int yOffset) override
	{
		const HudColors &colors = currentHudSkin().colors();
		texts.reset();
		const int x0 = xOffset + x();
		const int y0 = yOffset + y();
		if (compact)
		{
			texts.draw(upper(hudResultsMetricName(metric)), RESULTS_TAB_FONT, x0 + 66, y0 + 9, colors.textPrimary);
			if (anyLed)
			{
				texts.draw(upper(_("Who led")), RESULTS_LABEL_FONT, x0 + 4 + HUD_CHART_LEFT - 30, y0 + band->y() - 13, colors.textSecondary);
			}
			return;
		}
		pie_UniTransBoxFill(static_cast<float>(x0 + RAIL_W), static_cast<float>(y0), static_cast<float>(x0 + RAIL_W + 1), static_cast<float>(y0 + height()), hudResultsSurface(pal_RGBA(34, 43, 56, 255)));
		texts.draw(upper(_("Show")), RESULTS_LABEL_FONT, x0 + 20, y0 + 12, faint());
		// Under the list, what its sparklines' two lines are (where there's room)
		if (!rail.empty() && rail.front().second && sparkSides(metric).second)
		{
			const WIDGET &last = *rail.back().second;
			const int ky = y0 + last.y() + last.height() + 14;
			if (ky + 12 < y0 + height())
			{
				const std::string mine = data->youIndex ? (data->teamGame ? _("Your team") : _("You")) : _("Leader");
				const std::string rival = data->youIndex ? _("Top rival") : _("Next");
				int kx = x0 + 20;
				pie_UniTransBoxFill(static_cast<float>(kx), static_cast<float>(ky + 4), static_cast<float>(kx + 12), static_cast<float>(ky + 6), faint(200));
				kx += 17;
				kx += texts.draw(upper(mine), RESULTS_LABEL_FONT, kx, ky, faint()) + 14;
				for (int d = 0; d < 3; ++d)
				{
					pie_UniTransBoxFill(static_cast<float>(kx + d * 5), static_cast<float>(ky + 4), static_cast<float>(kx + d * 5 + 3), static_cast<float>(ky + 6), faint(160));
				}
				kx += 17;
				texts.draw(upper(rival), RESULTS_LABEL_FONT, kx, ky, faint());
			}
		}
		const int cx = x0 + RAIL_W + 12 + HUD_CHART_LEFT - 30;
		const int w = texts.draw(upper(hudResultsMetricName(metric)), RESULTS_TAB_FONT, cx, y0 + 13, colors.textPrimary);
		const char *about = "";
		switch (metric)
		{
		case HudResultsMetric::Army: about = N_("units on the field"); break;
		case HudResultsMetric::ArmyValue: about = N_("the power cost of the units on the field"); break;
		case HudResultsMetric::Derricks: about = N_("oil derricks"); break;
		case HudResultsMetric::Kills: about = N_("units and structures destroyed"); break;
		case HudResultsMetric::PowerDestroyed: about = N_("the value of what each destroyed"); break;
		case HudResultsMetric::PowerLost: about = N_("the value of what each lost"); break;
		case HudResultsMetric::Research: about = N_("topics, shared in a team"); break;
		case HudResultsMetric::LabUse: about = N_("labs busy, over the last minute"); break;
		case HudResultsMetric::Structures: about = N_("standing"); break;
		case HudResultsMetric::Score: about = N_("the game's score"); break;
		}
		texts.draw(gettext(about), font_small, cx + w + 10, y0 + 17, faint());
		if (lead)
		{
			const int top = y0 + lead->y() - 20;
			int kx = cx + texts.draw(upper(_("Team lead")), RESULTS_TAB_FONT, cx, top, colors.textPrimary) + 14;
			// Its key: which side is ahead above the line, which below (in the colors of the areas)
			for (size_t s = 0; s < 2; ++s)
			{
				const PIELIGHT c = sideColor(*data, s);
				pie_DrawRoundedBox(static_cast<float>(kx), static_cast<float>(top + 4), static_cast<float>(kx + 9), static_cast<float>(top + 13), alpha(c, 110), 2.f, c, 1.f);
				kx += 14 + texts.draw(astringf(_("%s ahead"), data->sideNames[s].c_str()), font_small, kx + 14, top + 3, colors.textSecondary) + 16;
			}
		}
		if (anyLed)
		{
			texts.draw(upper(_("Who led")), RESULTS_TAB_FONT, cx, y0 + band->y() - 20, colors.textPrimary);
		}
	}

private:
	const HudGameResults *data = nullptr;
	bool compact = false;
	std::vector<std::shared_ptr<ChoiceButton>> stepper;
	HudResultsMetric metric = HudResultsMetric::ArmyValue;
	bool byTeam = false;
	std::vector<std::pair<HudResultsMetric, std::shared_ptr<ChoiceButton>>> rail;
	std::vector<std::shared_ptr<ChoiceButton>> teamSwitch;
	std::shared_ptr<HudChart> chart;
	std::shared_ptr<HudChart> lead;
	std::shared_ptr<HudTimeBand> band;
	bool anyLed = true;
	Texts texts;
};

// ---------------------------------------------------------------- Research

class ResearchView : public WIDGET
{
public:
	void initialize(const HudGameResults *results, bool compactLayout)
	{
		data = results;
		compact = compactLayout;
		if (compact)
		{
			// One thing at a time: the timeline, who got there first, or by weapon line
			const char *names[] = {N_("Timeline"), N_("Firsts"), N_("Weapon lines")};
			for (int k = 0; k < 3; ++k)
			{
				auto b = makeChoice(ChoiceButton::Look::Segment, gettext(names[k]));
				b->first = k == 0;
				b->last = k == 2;
				b->chosen = k == 0;
				b->addOnClickHandler([this, k](W_BUTTON &) {
					page = k;
					for (size_t q = 0; q < pages.size(); ++q)
					{
						pages[q]->chosen = static_cast<int>(q) == k;
					}
					geometryChanged();
				});
				attach(b);
				pages.push_back(b);
			}
		}
		auto all = makeChoice(ChoiceButton::Look::Chip, astringf("%s  %zu", _("All"), data->research.size()));
		all->chosen = true;
		all->addOnClickHandler([this](W_BUTTON &) { filter(std::string()); });
		attach(all);
		chips.emplace_back(std::string(), all);
		for (const HudResearchLine &line : data->lines)
		{
			const std::string cat = line.key;
			auto b = makeChoice(ChoiceButton::Look::Chip, astringf("%s  %zu", line.name.c_str(), line.count));
			b->line = &line;
			b->addOnClickHandler([this, cat](W_BUTTON &) { filter(cat); });
			attach(b);
			chips.emplace_back(cat, b);
		}
	}

	void filter(const std::string &cat)
	{
		category = cat;
		for (auto &c : chips)
		{
			c.second->chosen = c.first == cat;
		}
	}

	void geometryChanged() override
	{
		int x = compact ? 10 : PAD;
		for (auto &b : pages)
		{
			const int w = b->idealWidth();
			b->setGeometry(x, 6, w, 22);
			x += w;
		}
		if (compact)
		{
			x += 10;
		}
		for (auto &c : chips)
		{
			const int w = c.second->idealWidth();
			const bool fits = x + w <= width() - (compact ? 10 : PAD) && (!compact || page == 0);
			c.second->show(fits);
			c.second->setGeometry(x, compact ? 6 : 12, w, compact ? 22 : 24);
			if (fits)
			{
				x += w + 6;
			}
		}
	}

	void run(W_CONTEXT *) override
	{
		hovered = nonstd::nullopt;
		hoveredHeading = nonstd::nullopt;
		if (!isMouseOverWidget())
		{
			return;
		}
		const int mx = mouseX() - screenPosX();
		const int my = mouseY() - screenPosY();
		float best = 8.f;
		for (size_t i = 0; i < dots.size(); ++i)
		{
			const float d = std::hypot(dots[i].x - mx, dots[i].y - my);
			if (d < best)
			{
				best = d;
				hovered = i;
			}
		}
		for (size_t i = 0; i < headingTips.size() && !hovered; ++i)
		{
			const WzRect &r = headingTips[i].first;
			if (mx >= r.x() - 3 && mx < r.right() + 3 && my >= r.y() - 3 && my < r.bottom() + 3)
			{
				hoveredHeading = i;
			}
		}
	}

	void display(int xOffset, int yOffset) override
	{
		const HudColors &colors = currentHudSkin().colors();
		texts.reset();
		headingTips.clear();
		const int x0 = xOffset + x();
		const int y0 = yOffset + y();
		const int left = x0 + (compact ? 10 : PAD);
		const int right = x0 + width() - (compact ? 10 : PAD);
		const bool showLanes = !compact || page == 0;

		// The lanes: a side's research, a dot each (stacked when close), its main technologies labeled
		const size_t sides = data->sides.size();
		const int lanesTop = y0 + (compact ? 34 : 48);
		const int lanesH = compact ? height() - 34 - 22 : std::max(140, std::min(270, (height() - 48) / 2 + 20));
		const int laneGap = compact || sides > 4 ? 3 : 6;
		const int laneH = std::max(compact || sides > 4 ? 20 : 36, (lanesH - laneGap * static_cast<int>(sides - 1)) / static_cast<int>(sides));
		const int lx = left + (compact ? 96 : 150);
		const int lw = right - lx - 10;
		const auto X = [&](float t) { return lx + lw * std::min(t, data->end) / data->end; };
		dots.clear();
		dotEvents.clear();
		std::map<std::string, std::map<size_t, const HudResultsResearch *>> firstBySide;
		for (const HudResultsResearch &e : data->research)
		{
			firstBySide[e.id].emplace(e.side, &e);
		}
		for (size_t s = 0; s < sides && showLanes; ++s)
		{
			const int ly = lanesTop + static_cast<int>(s) * (laneH + laneGap);
			pie_DrawRoundedBox(static_cast<float>(left), static_cast<float>(ly), static_cast<float>(right), static_cast<float>(ly + laneH), hudResultsSurface(pal_RGBA(18, 24, 33, 255)), 5.f);
			if (laneH >= 40)
			{
				texts.draw(data->sideNames[s], font_regular_bold, left + 12, ly + 8, colors.textPrimary);
			}
			else
			{
				drawLine(lines[s % lines.size()], data->sideNames[s], font_small, left + 10, ly + (laneH - static_cast<int>(iV_GetTextLineSize(font_small))) / 2, lx - left - 20,
				         data->teamGame ? colors.textPrimary : sideColor(*data, s));
			}
			int cx = left + 12;
			if (laneH >= 64)
			{
				for (size_t p : data->sides[s])
				{
					if (chipTexts.size() <= p)
					{
						chipTexts.resize(data->players.size());
					}
					drawPositionChip(chipTexts[p], data->players[p].index, WzRect(cx, ly + 30, 16, 16));
					cx += 20;
				}
			}
			const int topics = static_cast<int>(std::count_if(data->research.begin(), data->research.end(), [s](const HudResultsResearch &e) { return e.side == s; }));
			if (laneH >= 80)
			{
				texts.draw(astringf(_("%d topics"), topics), font_small, left + 12, ly + 52, colors.textSecondary);
			}
			const int base = ly + laneH - 13;
			std::vector<std::pair<float, int>> placed;
			for (const HudResultsResearch &e : data->research)
			{
				if (e.side != s || (!category.empty() && e.line != category))
				{
					continue;
				}
				const float x = X(e.time);
				int level = 0;
				while (std::any_of(placed.begin(), placed.end(), [x, level](const std::pair<float, int> &q) { return std::fabs(q.first - x) < 11.f && q.second == level; }))
				{
					++level;
				}
				placed.emplace_back(x, level);
				const float y = static_cast<float>(base - level * 11);
				if (y < ly + (laneH >= 40 ? 18 : 3))
				{
					continue;
				}
				const HudResearchLine *line = data->lineFor(e.line);
				pie_DrawRoundedBox(x - 5.f, y - 5.f, x + 5.f, y + 5.f, line ? line->color : hudResearchCategoryColor(e.icon), 2.5f, pal_RGBA(18, 24, 33, 255), 1.f);
				dots.emplace_back(x - xOffset - this->x(), y - yOffset - this->y());
				dotEvents.push_back(&e);
			}
			// Labels: the side's main technologies, each in the first of three rows with room for it
			std::array<int, 3> rowEnd = {{-100000, -100000, -100000}};
			for (const auto &tech : hudKeyTechnologies())
			{
				auto it = firstBySide.find(tech.first);
				if (it == firstBySide.end() || !it->second.count(s))
				{
					continue;
				}
				const HudResultsResearch *e = it->second[s];
				if (!category.empty() && e->line != category)
				{
					continue;
				}
				const int x = static_cast<int>(X(e->time));
				const std::string label = astringf("%s %s", gettext(tech.second.c_str()), hudClockText(static_cast<uint32_t>(e->time)).c_str());
				const int tw = texts.width(label, font_small);
				const bool flip = x + tw + 6 > right;
				const int from = flip ? x - 3 - tw : x + 3;
				const int maxRows = laneH >= 90 ? 3 : (laneH >= 60 ? 2 : (laneH >= 40 ? 1 : 0));
				int row = -1;
				for (int r = 0; r < maxRows; ++r)
				{
					if (from > rowEnd[static_cast<size_t>(r)] + 8)
					{
						row = r;
						break;
					}
				}
				if (row < 0)
				{
					continue;
				}
				rowEnd[static_cast<size_t>(row)] = from + tw;
				const int ty = ly + 4 + 13 * row;
				pie_UniTransBoxFill(static_cast<float>(x), static_cast<float>(ty + 3), static_cast<float>(x + 1), static_cast<float>(base - 8), pal_RGBA(58, 70, 88, 160));
				texts.draw(label, font_small, flip ? x - 3 : x + 3, ty, colors.textSecondary, flip);
			}
		}
		const int axisY = lanesTop + static_cast<int>(sides) * (laneH + laneGap);
		float step = 60.f;
		for (float st : {60.f, 120.f, 180.f, 300.f, 600.f, 900.f, 1800.f})
		{
			if (lw * st / data->end >= 46.f)
			{
				step = st;
				break;
			}
		}
		for (float t = 0.f; t <= data->end + 0.5f && showLanes; t += step)
		{
			const std::string label = hudClockText(static_cast<uint32_t>(t));
			const int tw = texts.width(label, font_small);
			texts.draw(label, font_small, static_cast<int>(X(t)) - tw / 2, axisY + 2, faint(150));
		}

		// Under them: who got there first, and the research by weapon line (Compact: one of them, in the whole room)
		const int boxTop = compact ? y0 + 34 : axisY + 22;
		const int boxH = y0 + height() - (compact ? 6 : 10) - boxTop;
		const bool showFirsts = !compact || page == 1;
		const bool showCategories = !compact || page == 2;
		const int split = compact ? (page == 1 ? right + 5 : left - 5) : left + (right - left) * 56 / 100;
		if (boxH > 80 && showFirsts)
		{
			pie_DrawRoundedBox(static_cast<float>(left), static_cast<float>(boxTop), static_cast<float>(split - 5), static_cast<float>(boxTop + boxH), hudResultsSurface(pal_RGBA(20, 26, 35, 255)), 6.f, hudResultsSurface(pal_RGBA(34, 43, 56, 255)), 1.f);
			texts.draw(upper(_("Who got there first")), RESULTS_LABEL_FONT, left + 14, boxTop + 10, faint());
			texts.draw(_("the biggest leads, on what more than one side researched"), font_small, left + 14 + texts.width(upper(_("Who got there first")), RESULTS_LABEL_FONT) + 10, boxTop + 10, faint(120));
			// Up to four sides: a column each, and the lead. More: who was first, who next, the lead, and how many got it
			const bool perSide = sides <= 4;
			const int leadW = 64;
			const int colW = perSide ? 72 : 120;
			const int tableR = split - 18;
			const int leadR = tableR;
			int ty = boxTop + 30;
			if (perSide)
			{
				// A player's column: their chip. A team's: its name, cut to the column. Either way the name on hover
				for (size_t s = 0; s < sides; ++s)
				{
					const int cr = tableR - leadW - colW * static_cast<int>(sides - 1 - s);
					if (data->sides[s].size() == 1)
					{
						const size_t p = data->sides[s].front();
						if (chipTexts.size() <= p)
						{
							chipTexts.resize(data->players.size());
						}
						drawPositionChip(chipTexts[p], data->players[p].index, WzRect(cr - 15, ty - 2, 15, 15));
						headingTips.push_back({WzRect(cr - 15 - x0, ty - 2 - y0, 15, 15), data->players[p].name});
						continue;
					}
					const std::string name = upper(data->sideNames[s]);
					const std::string shown = hudCappedText(name, RESULTS_LABEL_FONT, colW - 10);
					const int tw = texts.draw(shown, RESULTS_LABEL_FONT, cr, ty, faint(), true);
					if (shown != name)
					{
						headingTips.push_back({WzRect(cr - tw - x0, ty - 2 - y0, tw, 15), data->sideNames[s]});
					}
				}
			}
			else
			{
				texts.draw(upper(_("First")), RESULTS_LABEL_FONT, tableR - leadW - colW * 2, ty, faint(), true);
				texts.draw(upper(_("Next")), RESULTS_LABEL_FONT, tableR - leadW - colW, ty, faint(), true);
				texts.draw(upper(_("Got it")), RESULTS_LABEL_FONT, tableR - leadW - colW * 3 + 40, ty, faint(), true);
			}
			texts.draw(upper(_("Lead")), RESULTS_LABEL_FONT, leadR, ty, faint(), true);
			ty += 18;
			drawDivider(left + 12, ty - 2, split - 17, ty - 1);
			const int rows = std::max(0, (boxTop + boxH - 8 - ty) / 22);
			int shown = 0;
			if (data->races.empty())
			{
				texts.draw(_("No technology was researched by more than one side a while apart."), font_small, left + 14, ty + 4, faint());
			}
			for (const HudResultsRace &race : data->races)
			{
				if (shown >= rows || (!category.empty() && !std::any_of(data->research.begin(), data->research.end(), [&](const HudResultsResearch &e) { return e.id == race.id && e.line == category; })))
				{
					continue;
				}
				currentHudSkin().drawNamedIcon(race.icon, WzRect(left + 14, ty + 4, 13, 13), hudResearchCategoryColor(race.icon));
				const int nameRight = perSide ? tableR - leadW - colW * static_cast<int>(sides) : tableR - leadW - colW * 3 + 40 - 50;
				drawLine(raceNames[static_cast<size_t>(shown) % raceNames.size()], race.name, font_regular, left + 34, ty + 2, nameRight - left - 34, colors.textPrimary);
				auto timeCell = [&](int cr, float time, bool isFirst, nonstd::optional<size_t> side) {
					// (Right-aligned on its column, the first's star to its left,
					// and in a free-for-all, the player's chip)
					const std::string text = hudClockText(static_cast<uint32_t>(time));
					const iV_fonts font = isFirst ? font_regular_bold : font_regular;
					const int tw = texts.draw(text, font, cr, ty + 2, isFirst ? colors.textPrimary : colors.textSecondary, true);
					int textLeft = cr - tw;
					if (isFirst)
					{
						currentHudSkin().drawNamedIcon("favorite", WzRect(textLeft - 15, ty + 5, 11, 11), GOLD);
						textLeft -= 15;
					}
					if (side && !data->sides[*side].empty())
					{
						const size_t p = data->sides[*side].front();
						if (chipTexts.size() <= p)
						{
							chipTexts.resize(data->players.size());
						}
						drawPositionChip(chipTexts[p], data->players[p].index, WzRect(textLeft - 20, ty + 3, 15, 15));
					}
				};
				if (perSide)
				{
					for (size_t s = 0; s < sides; ++s)
					{
						const int cr = tableR - leadW - colW * static_cast<int>(sides - 1 - s);
						auto it = std::find_if(race.sides.begin(), race.sides.end(), [s](const std::pair<size_t, float> &p) { return p.first == s; });
						if (it == race.sides.end())
						{
							texts.draw("—", font_regular, cr, ty + 2, faint(120), true);
							continue;
						}
						timeCell(cr, it->second, it == race.sides.begin(), nonstd::nullopt);
					}
				}
				else
				{
					texts.draw(astringf("%zu/%zu", race.sides.size(), sides), font_regular, tableR - leadW - colW * 3 + 40, ty + 2, colors.textSecondary, true);
					timeCell(tableR - leadW - colW * 2, race.sides[0].second, true, race.sides[0].first);
					timeCell(tableR - leadW - colW, race.sides[1].second, false, race.sides[1].first);
				}
				texts.draw("+" + hudClockText(static_cast<uint32_t>(race.lead)), font_regular, leadR, ty + 2, GOLD, true);
				ty += 22;
				++shown;
			}
		}
		if (boxH > 80 && showCategories)
		{
			pie_DrawRoundedBox(static_cast<float>(split + 5), static_cast<float>(boxTop), static_cast<float>(right), static_cast<float>(boxTop + boxH), hudResultsSurface(pal_RGBA(20, 26, 35, 255)), 6.f, hudResultsSurface(pal_RGBA(34, 43, 56, 255)), 1.f);
			// By weapon line: two sides as a mirror, more as a bar each
			const int bx = split + 19;
			const int bw = right - 14 - bx;
			texts.draw(upper(_("By weapon line")), RESULTS_LABEL_FONT, bx, boxTop + 10, faint());
			std::vector<std::map<std::string, int>> counts(sides);
			int most = 1;
			for (const HudResultsResearch &e : data->research)
			{
				most = std::max(most, ++counts[e.side][e.line]);
			}
			int cy = boxTop + 32;
			if (sides == 2)
			{
				texts.draw(upper(data->sideNames[0]), RESULTS_LABEL_FONT, bx, cy - 4, sideColor(*data, 0));
				texts.draw(upper(data->sideNames[1]), RESULTS_LABEL_FONT, bx + bw, cy - 4, sideColor(*data, 1), true);
				cy += 16;
				const int mid = bx + bw / 2;
				const int nameW = 120;
				const int maxBarWidth = bw / 2 - nameW / 2 - 28;
				for (const HudResearchLine &line : data->lines)
				{
					const std::string &cat = line.key;
					if (cy + 16 > boxTop + boxH - 6)
					{
						break;
					}
					const int a = counts[0][cat];
					const int b = counts[1][cat];
					const std::string &name = line.name;
					const int nw = texts.width(name, font_small);
					hudDrawResearchLineIcon(line, WzRect(mid - nw / 2 - 18, cy - 1, 15, 15));
					texts.draw(name, font_small, mid - nw / 2 + 2, cy, colors.textSecondary);
					const float wa = static_cast<float>(maxBarWidth) * a / most;
					const float wb = static_cast<float>(maxBarWidth) * b / most;
					const float l0 = static_cast<float>(mid - nameW / 2 - 6);
					const float r0 = static_cast<float>(mid + nameW / 2 + 6);
					if (a)
					{
						pie_DrawRoundedBox(l0 - wa, static_cast<float>(cy + 4), l0, static_cast<float>(cy + 11), sideColor(*data, 0), glm::vec4(3.5f, 0.f, 3.5f, 0.f), sideColor(*data, 0), 0.f);
					}
					if (b)
					{
						pie_DrawRoundedBox(r0, static_cast<float>(cy + 4), r0 + wb, static_cast<float>(cy + 11), sideColor(*data, 1), glm::vec4(0.f, 3.5f, 0.f, 3.5f), sideColor(*data, 1), 0.f);
					}
					texts.draw(std::to_string(a), font_small, bx, cy, colors.textPrimary);
					texts.draw(std::to_string(b), font_small, bx + bw, cy, colors.textPrimary, true);
					cy += 19;
				}
			}
			else
			{
				// A table, as the spectator statistics': a row per side, a column per line (its picture over it),
				// the most in a column in bold
				const int nameW = 100;
				const int cols = static_cast<int>(data->lines.size());
				const int colW = std::max(24, std::min(46, (bw - nameW) / std::max(1, cols)));
				const int shown = std::min(cols, (bw - nameW) / colW);
				for (int c = 0; c < shown; ++c)
				{
					const HudResearchLine &line = data->lines[static_cast<size_t>(c)];
					const int cx = bx + nameW + c * colW + colW / 2;
					hudDrawResearchLineIcon(line, WzRect(cx - 9, cy - 6, 18, 18));
					pie_UniTransBoxFill(static_cast<float>(cx - 9), static_cast<float>(cy + 14), static_cast<float>(cx + 9), static_cast<float>(cy + 16), line.color);
				}
				cy += 22;
				for (size_t s = 0; s < sides; ++s)
				{
					if (cy + 16 > boxTop + boxH - 6)
					{
						texts.draw(astringf(_("and %zu more"), sides - s), font_small, bx, cy - 4, faint());
						break;
					}
					drawLine(lines[s % lines.size()], data->sideNames[s], font_small, bx, cy, nameW - 6, sideColor(*data, s));
					for (int c = 0; c < shown; ++c)
					{
						const std::string &key = data->lines[static_cast<size_t>(c)].key;
						const int n = counts[s][key];
						int top = 0;
						for (size_t o = 0; o < sides; ++o)
						{
							top = std::max(top, counts[o][key]);
						}
						const int cx = bx + nameW + c * colW + colW / 2;
						const std::string text = n ? std::to_string(n) : "·";
						texts.draw(text, n && n == top ? font_regular_bold : font_small, cx - texts.width(text, n && n == top ? font_regular_bold : font_small) / 2, cy - (n && n == top ? 2 : 0),
						           n ? (n == top ? colors.textPrimary : colors.textSecondary) : faint(90));
					}
					cy += 19;
				}
				if (shown < cols)
				{
					texts.draw(astringf(_("and %d more lines"), cols - shown), font_small, bx + bw, cy + 2, faint(), true);
				}
			}
		}

		// A dot's tooltip
		if (hovered && *hovered < dotEvents.size())
		{
			const HudResultsResearch &e = *dotEvents[*hovered];
			const int dx = xOffset + this->x() + static_cast<int>(dots[*hovered].x);
			const int dy = yOffset + this->y() + static_cast<int>(dots[*hovered].y);
			std::string other;
			auto it = firstBySide.find(e.id);
			if (it != firstBySide.end())
			{
				for (const auto &b : it->second)
				{
					if (b.first != e.side)
					{
						const float diff = b.second->time - e.time;
						other += (other.empty() ? "" : "  ·  ") + astringf(diff >= 0 ? _("%s: %s (%d s later)") : _("%s: %s (%d s earlier)"), data->sideNames[b.first].c_str(),
						                                                   hudClockText(static_cast<uint32_t>(b.second->time)).c_str(), static_cast<int>(std::fabs(diff)));
					}
				}
			}
			const std::string line2 = astringf(_("%s · researched by %s"), hudClockText(static_cast<uint32_t>(e.time)).c_str(), data->players[e.player].name.c_str());
			const int w = std::max({texts.width(e.name, font_regular_bold), texts.width(line2, font_small), texts.width(other, font_small)}) + 50;
			const int h = other.empty() ? 50 : 66;
			int tx = dx + 12;
			if (tx + w > x0 + width() - 8)
			{
				tx = dx - 12 - w;
			}
			const int ty = std::max(y0 + 4, dy - h - 8);
			pie_DrawRoundedBox(static_cast<float>(tx), static_cast<float>(ty), static_cast<float>(tx + w), static_cast<float>(ty + h), hudResultsSurface(pal_RGBA(13, 18, 24, 248)), 5.f, hudResultsSurface(pal_RGBA(58, 70, 88, 255)), 1.f);
			currentHudSkin().drawNamedIcon(e.icon, WzRect(tx + 12, ty + 12, 18, 18), hudResearchCategoryColor(e.icon));
			texts.draw(e.name, font_regular_bold, tx + 40, ty + 9, colors.textPrimary);
			texts.draw(line2, font_small, tx + 40, ty + 29, colors.textSecondary);
			if (!other.empty())
			{
				texts.draw(other, font_small, tx + 40, ty + 45, faint(150));
			}
			pie_DrawRoundedBox(static_cast<float>(dx - 7), static_cast<float>(dy - 7), static_cast<float>(dx + 7), static_cast<float>(dy + 7), pal_RGBA(0, 0, 0, 0), 4.f, WZCOL_WHITE, 2.f);
		}
		// A column heading's tooltip: the name, under it
		if (hoveredHeading && *hoveredHeading < headingTips.size())
		{
			const WzRect &r = headingTips[*hoveredHeading].first;
			const std::string &name = headingTips[*hoveredHeading].second;
			const int w = texts.width(name, font_regular_bold) + 20;
			const int tx = std::min(x0 + r.x() + r.width() / 2 - w / 2, x0 + width() - 8 - w);
			const int ty = y0 + r.bottom() + 6;
			pie_DrawRoundedBox(static_cast<float>(tx), static_cast<float>(ty), static_cast<float>(tx + w), static_cast<float>(ty + 26), hudResultsSurface(pal_RGBA(13, 18, 24, 248)), 5.f, hudResultsSurface(pal_RGBA(58, 70, 88, 255)), 1.f);
			texts.draw(name, font_regular_bold, tx + 10, ty + 5, colors.textPrimary);
		}
	}

private:
	const HudGameResults *data = nullptr;
	bool compact = false;
	int page = 0;
	std::vector<std::shared_ptr<ChoiceButton>> pages;
	std::string category;
	std::vector<std::pair<std::string, std::shared_ptr<ChoiceButton>>> chips;
	std::vector<glm::vec2> dots;
	std::vector<const HudResultsResearch *> dotEvents;
	nonstd::optional<size_t> hovered;
	std::vector<std::pair<WzRect, std::string>> headingTips;   ///< (the firsts' column headings: where, and the name)
	nonstd::optional<size_t> hoveredHeading;
	Texts texts;
	std::vector<ChipText> chipTexts;
	std::array<WzText, 16> lines;
	std::array<WzText, 16> raceNames;
};

// ---------------------------------------------------------------- Timeline

std::string momentIcon(const HudResultsMoment &m)
{
	if (m.kind == "fight") return "units";
	if (m.kind == "battle") return "damage";
	if (m.kind == "derricks") return "power";
	if (m.kind == "lead") return "upgrade";
	if (m.kind == "out") return "defeat";
	if (m.kind == "end") return m.players.empty() ? "objective-done" : "victory";
	if (m.kind == "vtol") return "research-droid";
	if (m.kind == "aa") return "research-defense";
	return "research-computer";
}

class TimelineView : public WIDGET
{
public:
	/// Draws its chart in
	void reveal()
	{
		chart->setBackground(hudResultsSurface(pal_RGBA(14, 18, 24, 255)));
		chart->reveal();
	}

	void initialize(const HudGameResults *results, bool compactLayout)
	{
		data = results;
		compact = compactLayout;
		phases = HudTimeBand::make(HudTimeBand::Look::Phases);
		attach(phases);
		chart = HudChart::make(HudChart::Mode::Lines);
		attach(chart);
		std::vector<HudChartSeries> series;
		for (size_t s = 0; s < data->sides.size(); ++s)
		{
			HudChartSeries cs;
			cs.label = data->sideNames[s];
			cs.color = data->teamGame ? sideColor(*data, s) : data->players[data->sides[s].front()].color;
			cs.values = data->sideValues(HudResultsMetric::ArmyValue, s);
			cs.emphasized = data->youIndex && data->players[*data->youIndex].side == s;
			if (!data->teamGame)
			{
				cs.chip = data->players[data->sides[s].front()].index;
				cs.withChip = true;
			}
			series.push_back(cs);
		}
		chart->setTimes(data->times, data->end);
		chart->setSeries(series);
		chart->setTooltipTitle(_("Army value"));
		std::vector<HudChartMarker> markers;
		for (const HudResultsMoment &m : data->moments)
		{
			markers.push_back({m.time, std::string(), momentColor(m)});
		}
		chart->setMarkers(markers);
		std::vector<HudTimeBandStretch> stretches;
		for (const HudResultsPhase &p : data->phases)
		{
			HudTimeBandStretch s;
			s.from = p.from;
			s.to = p.to;
			if (p.side)
			{
				s.color = sideColor(*data, *p.side);
				s.label = astringf(p.farAhead ? _("%s far ahead") : _("%s ahead"), data->sideNames[*p.side].c_str());
			}
			else
			{
				s.label = _("No clear leader");
			}
			stretches.push_back(s);
		}
		phases->setStretches(stretches, data->end);
		phases->setMargins(HUD_CHART_LEFT, HUD_CHART_RIGHT_LABELS);
	}

	/// A moment's number in its circle: the circle odd-sized, on whole pixels, so the digits' ink
	/// (most an odd number of pixels wide) centers exactly (1 and 10 sit as 4)
	void drawNumberedCircle(size_t number, float cx, float cy, float r, PIELIGHT fill, PIELIGHT ring)
	{
		const int x = static_cast<int>(std::floor(cx)) - static_cast<int>(r);
		const int y = static_cast<int>(std::floor(cy)) - static_cast<int>(r);
		const int d = static_cast<int>(r) * 2 + 1;
		pie_DrawRoundedBox(static_cast<float>(x), static_cast<float>(y), static_cast<float>(x + d), static_cast<float>(y + d), fill, d / 2.f, ring, 1.5f);
		if (numbers.size() <= number)
		{
			numbers.resize(number + 1);
		}
		numbers[number].setText(WzString::number(static_cast<int>(number)), font_small);
		digits.setText(WzString::fromUtf8("0123456789"), font_small);
		numbers[number].renderCenteredOnInk(WzRect(x, y, d, d), currentHudSkin().colors().textPrimary, &digits);
	}

	PIELIGHT momentColor(const HudResultsMoment &m) const
	{
		if (m.kind == "end")
		{
			return currentHudSkin().colors().stateOk;
		}
		if (m.kind == "battle")
		{
			return GOLD;
		}
		if (m.kind == "research" || m.kind == "vtol" || m.kind == "aa")
		{
			return m.side ? sideColor(*data, *m.side) : currentHudSkin().colors().textSecondary;
		}
		return m.side ? sideColor(*data, *m.side) : currentHudSkin().colors().textSecondary;
	}

	void geometryChanged() override
	{
		if (compact)
		{
			phases->setGeometry(4, 6, width() - 8, 18);
			chart->setGeometry(4, 26, width() - 8, std::max(70, height() - 26 - 22 - 5 * 20 - 6));
			return;
		}
		phases->setGeometry(PAD, 44, width() - PAD * 2, 18);
		// (leaving the list five rows of at least 30, under the moments' circles)
		const int chartH = std::max(80, std::min({210, height() / 2 - 70, height() - 66 - 36 - 5 * 30 - 14}));
		chart->setGeometry(PAD, 66, width() - PAD * 2, chartH);
	}

	void display(int xOffset, int yOffset) override
	{
		const HudColors &colors = currentHudSkin().colors();
		texts.reset();
		const int x0 = xOffset + x();
		const int y0 = yOffset + y();
		const int left = x0 + (compact ? 10 : PAD);
		const int right = x0 + width() - (compact ? 10 : PAD);
		if (!compact)
		{
			int lx = left + texts.draw(upper(_("How the game went")), RESULTS_TAB_FONT, left, y0 + 14, colors.textPrimary) + 12;
			lx += texts.draw(_("army value"), font_small, lx, y0 + 17, colors.textSecondary) + 16;
			size_t s = 0;
			for (; s < data->sides.size() && lx < right - 290; ++s)
			{
				pie_UniTransBoxFill(static_cast<float>(lx), static_cast<float>(y0 + 23), static_cast<float>(lx + 12), static_cast<float>(y0 + 26), sideColor(*data, s));
				lx += 17 + texts.draw(data->sideNames[s], font_small, lx + 17, y0 + 17, colors.textSecondary) + 12;
			}
			if (s < data->sides.size())
			{
				texts.draw(astringf(_("+%zu more"), data->sides.size() - s), font_small, lx, y0 + 17, faint());
			}
			const unsigned found = static_cast<unsigned>(data->momentsFound);
			const unsigned shown = static_cast<unsigned>(data->moments.empty() ? 0 : data->moments.size() - 1);
			texts.draw(astringf(ngettext("%u moment found", "%u moments found", found), found) + "  ·  " + astringf(_("%u shown"), shown), font_small, right,
			           y0 + 17, faint(), true);
		}

		// The moments' numbers under the chart, at their times
		const int plotL = x0 + chart->x() + HUD_CHART_LEFT;
		const int plotW = chart->width() - HUD_CHART_LEFT - HUD_CHART_RIGHT_LABELS;
		const int ny = y0 + chart->y() + chart->height() + (compact ? 8 : 10);
		const float mark = compact ? 7.f : 9.f;
		// At their times, but moved apart where they'd overlap
		const float apart = mark * 2.f + 3.f;
		std::vector<float> xs;
		for (const HudResultsMoment &m : data->moments)
		{
			xs.push_back(plotL + plotW * std::min(m.time, data->end) / data->end);
		}
		for (size_t i = 1; i < xs.size(); ++i)
		{
			xs[i] = std::max(xs[i], xs[i - 1] + apart);
		}
		for (size_t i = xs.size(); i-- > 0;)
		{
			xs[i] = std::min(xs[i], i + 1 < xs.size() ? xs[i + 1] - apart : static_cast<float>(plotL + plotW) + mark);
		}
		for (size_t i = 0; i < data->moments.size(); ++i)
		{
			drawNumberedCircle(i + 1, xs[i], static_cast<float>(ny), mark, hudResultsSurface(pal_RGBA(18, 24, 33, 255)), momentColor(data->moments[i]));
		}

		// The list, in two columns (compact: one line each, the detail in its tooltip)
		const int listTop = ny + (compact ? 14 : 20);
		const size_t half = (data->moments.size() + 1) / 2;
		const int gap = compact ? 12 : 20;
		const int colW = (right - left - gap) / 2;
		const int rowH = compact ? 20 : std::max(30, std::min(44, (y0 + height() - 6 - listTop) / std::max<int>(1, static_cast<int>(half))));
		const iV_fonts textFont = compact ? font_small : font_regular;
		const int timeX = compact ? 24 : 30;
		const int iconX = compact ? 58 : 74;
		const int textX = compact ? 76 : 96;
		const int chipsWidth = compact ? 42 : 60;
		for (size_t i = 0; i < data->moments.size(); ++i)
		{
			const HudResultsMoment &m = data->moments[i];
			const int cx = left + (i < half ? 0 : colW + gap);
			const int cy = listTop + static_cast<int>(i < half ? i : i - half) * rowH;
			const PIELIGHT c = momentColor(m);
			const float r = compact ? 8.f : 9.f;
			drawNumberedCircle(i + 1, cx + 11.f, cy + 11.f, r, pal_RGBA(0, 0, 0, 0), c);
			texts.draw(hudClockText(static_cast<uint32_t>(m.time)), font_small, cx + timeX, cy + 5, faint());
			currentHudSkin().drawNamedIcon(momentIcon(m), WzRect(cx + iconX, cy + 4, 13, 13), c);
			drawLine(lines[i % lines.size()], m.text, textFont, cx + textX, cy + (compact ? 4 : 1), colW - textX - chipsWidth, colors.textPrimary);
			if (!m.detail.empty() && rowH >= 36)
			{
				drawLine(details[i % details.size()], m.detail, font_small, cx + textX, cy + 19, colW - textX - chipsWidth, faint(150));
			}
			int px = cx + colW - 4;
			for (auto it = m.players.rbegin(); it != m.players.rend(); ++it)
			{
				if (px - 16 < cx + colW - chipsWidth)
				{
					break;
				}
				px -= 16;
				if (chipTexts.size() <= *it)
				{
					chipTexts.resize(data->players.size());
				}
				drawPositionChip(chipTexts[*it], data->players[*it].index, WzRect(px, cy + 3, 14, 14));
				px -= 3;
			}
		}
		pie_UniTransBoxFill(static_cast<float>(left + colW + gap / 2 - 1), static_cast<float>(listTop), static_cast<float>(left + colW + gap / 2),
		                    static_cast<float>(listTop + static_cast<int>(half) * rowH - 6), hudResultsSurface(pal_RGBA(34, 43, 56, 255)));
	}

private:
	const HudGameResults *data = nullptr;
	bool compact = false;
	std::vector<WzText> numbers;
	WzText digits;
	std::shared_ptr<HudTimeBand> phases;
	std::shared_ptr<HudChart> chart;
	Texts texts;
	std::vector<ChipText> chipTexts;
	std::array<WzText, 12> lines;
	std::array<WzText, 12> details;
};

// ---------------------------------------------------------------- the sheet

class GameSheet : public HudResultsSheet
{
public:
	/// (`collected` is the data as the game ended, so a sheet made again shows the same)
	void initialize(bool isSpectator, bool won, const std::string &note, const HudGameResults &collected)
	{
		spectator = isSpectator;
		compact = hudResultsCompact();
		data = collected;
		if (spectator)
		{
			data.youIndex = nonstd::nullopt;
			for (HudResultsPlayer &p : data.players)
			{
				p.you = false;
			}
		}
		const HudColors &colors = currentHudSkin().colors();

		bindFonts();

		// The title: Victory or Defeat for a player, or Game over for a spectator (who won then leads the subtitle)
		std::string sizes;
		if (data.teamGame)
		{
			for (size_t s = 0; s < data.sides.size(); ++s)
			{
				sizes += (s ? " v " : "") + std::to_string(data.sides[s].size());
			}
		}
		else
		{
			sizes = _("free for all");
		}
		const std::string mapName = mapNameWithoutTechlevel(getLevelName());
		const std::string clock = hudClockText(static_cast<uint32_t>(data.end));
		if (spectator)
		{
			subtitleRest = {mapName, sizes, clock};
			if (compact)
			{
				subtitleRest.push_back(NETisReplay() ? _("Replay") : _("Spectating")); // (compact has no line above the title to say it)
			}
			setSpectatorHeader();
		}
		else
		{
			const char *result = data.teamGame ? (won ? _("Your team won") : _("Your team lost")) : (won ? _("You won") : _("You lost"));
			setHeader(won ? "victory" : "defeat", won ? colors.stateOk : colors.stateBad, _("Match result"), won ? _("Victory") : _("Defeat"),
			          {result, mapName, sizes, clock}, true);
		}
		noteText = note;

		std::string settings;
		switch (game.base)
		{
		case CAMP_CLEAN: settings = _("No bases"); break;
		case CAMP_BASE: settings = _("Bases"); break;
		default: settings = _("Advanced bases"); break;
		}
		settings += "  ·  " + std::string(data.teamGame ? _("Fixed teams") : (alliancesFixed(game.alliance) ? _("Free for all") : _("Alliances allowed")));
		settings += "  ·  " + std::string(game.scavengers ? _("Scavengers") : _("No scavengers"));
		setSubtitleTip(settings);
		// The replay this game saved, and where
		const std::string &replay = NETreplaySaveFilename();
		if (!replay.empty() && !NETisReplay())
		{
#if defined(__EMSCRIPTEN__)
			setFooterNote("replay-play", _("Replay saved"));
#else
			const std::string folder = replay.substr(0, replay.rfind('/'));
			setFooterNote("replay-play", _("Replay saved"), _("Show in folder"), [folder]() {
				std::string path = std::string(PHYSFS_getWriteDir()) + PHYSFS_getDirSeparator() + folder;
				std::replace(path.begin() + static_cast<std::ptrdiff_t>(path.size() - folder.size()), path.end(), '/', PHYSFS_getDirSeparator()[0]);
				openFolderInDefaultFileManager(path.c_str());
			});
#endif
		}

		const char *names[] = {N_("Overview"), N_("Charts"), N_("Research"), N_("Timeline")};
		for (int k = 0; k < 4; ++k)
		{
			auto b = makeChoice(ChoiceButton::Look::Tab, gettext(names[k]));
			b->small = compact;
			b->chosen = k == 0;
			b->underline = false;
			b->addOnClickHandler([this, k](W_BUTTON &) { showTab(k); });
			attach(b);
			tabs.push_back(b);
		}
		if (data.empty())
		{
			return;
		}
		overview = std::make_shared<OverviewView>();
		overview->initialize(&data, compact);
		charts = std::make_shared<ChartsView>();
		charts->initialize(&data, compact);
		research = std::make_shared<ResearchView>();
		research->initialize(&data, compact);
		timeline = std::make_shared<TimelineView>();
		timeline->initialize(&data, compact);
		for (const auto &v : std::vector<std::shared_ptr<WIDGET>>{overview, charts, research, timeline})
		{
			attach(v);
		}
		showTab(0);
	}

	void showTab(int k)
	{
		if (k != tabShown)
		{
			tabFrom = tabShown;
			tabShown = k;
			tabMovedAt = realTime;
		}
		for (size_t q = 0; q < tabs.size(); ++q)
		{
			tabs[q]->chosen = static_cast<int>(q) == k;
		}
		if (data.empty())
		{
			return;
		}
		overview->show(k == 0);
		if (k == 1 && !charts->visible())
		{
			charts->reveal();
		}
		charts->show(k == 1);
		research->show(k == 2);
		if (k == 3 && !timeline->visible())
		{
			timeline->reveal();
		}
		timeline->show(k == 3);
	}

	/// A spectator's: who won leads the subtitle, as much of them as its room takes
	void setSpectatorHeader()
	{
		std::vector<std::string> sub = subtitleRest;
		const std::string winnerLine = winnersLine(sub);
		if (!winnerLine.empty())
		{
			sub.insert(sub.begin(), winnerLine);
		}
		setHeader(winnerLine.empty() ? "players" : "victory", pal_RGBA(120, 134, 152, 255), NETisReplay() ? _("Replay") : _("Spectating"), _("Game over"), sub);
	}

	/// Who won, for the subtitle, before its `rest`: the names share the room the rest leaves, up to a cap (a long or
	/// wide name, or a small screen, would push the "won" out)
	std::string winnersLine(const std::vector<std::string> &rest) const
	{
		std::vector<size_t> winners;
		for (size_t s = 0; s < data.sides.size(); ++s)
		{
			if (data.sideWon[s])
			{
				winners.push_back(s);
			}
		}
		if (winners.empty())
		{
			return std::string();
		}
		auto widthOf = [](const std::string &text) { return static_cast<int>(iV_GetTextWidth(WzString::fromUtf8(upper(text)), RESULTS_SUBTITLE_FONT)); };
		int restW = 0;
		for (const std::string &part : rest)
		{
			restW += widthOf(" / " + part);
		}
		const char *format = winners.size() == 1 ? _("%s won") : winners.size() == 2 ? _("%s and %s won") : _("%s and others won");
		const int availableWidth = availableSubtitleWidth() - restW - widthOf(astringf(format, "", ""));
		const int cap = compact ? 200 : 360;
		int nameMax = std::max(60, std::min(cap, availableWidth));
		if (winners.size() == 2)
		{
			// The shorter name takes what it needs, the longer the rest
			const int shorter = std::min(widthOf(data.sideNames[winners[0]]), widthOf(data.sideNames[winners[1]]));
			nameMax = std::max(60, std::min(cap, shorter * 2 <= availableWidth ? availableWidth - shorter : availableWidth / 2));
		}
		if (winners.size() > 1 && availableWidth / static_cast<int>(winners.size()) < 70)
		{
			// (too little room for their names to be read)
			return astringf(ngettext("%u player won", "%u players won", static_cast<unsigned>(winners.size())), static_cast<unsigned>(winners.size()));
		}
		auto name = [&](size_t side) {
			return hudCappedText(upper(data.sideNames[side]), RESULTS_SUBTITLE_FONT, nameMax);
		};
		if (winners.size() == 2)
		{
			return astringf(format, name(winners[0]).c_str(), name(winners[1]).c_str());
		}
		return astringf(format, name(winners.front()).c_str());
	}

	WzRect sheetRect(int screenW, int screenH) const override
	{
		const int margin = compact ? 4 : 12;
		const int w = std::min(1120, screenW - margin * 2);
		const int h = std::min(664, screenH - margin * 2);
		return WzRect((screenW - w) / 2, (screenH - h) / 2, w, h);
	}

	void geometryChanged() override
	{
		if (spectator)
		{
			setSpectatorHeader();
		}
		HudResultsSheet::geometryChanged();
		// (the first tab's text in line with the badge)
		int x = padW() - (compact ? 9 : 15);
		for (auto &t : tabs)
		{
			const int w = t->idealWidth();
			t->setGeometry(x, headerH(), w, tabsH());
			x += w + (compact ? 2 : 10);
		}
		const WzRect c = contentRect();
		for (const auto &v : std::vector<std::shared_ptr<WIDGET>>{overview, charts, research, timeline})
		{
			if (v)
			{
				v->setGeometry(c.x(), c.y(), c.width(), c.height());
			}
		}
	}

protected:
	int headerExtra() const override { return tabsH(); }
	int tabsH() const { return compact ? 30 : 44; }

	/// A player's own figures (the header's right, for a player)
	struct Figure
	{
		std::string label;
		double value;
		bool full;   ///< (every digit, else short as the charts' numbers)
		int width = 0;
		std::string text(double shown) const
		{
			return full ? hudFullNumber(static_cast<long long>(std::llround(shown))) : hudChartNumber(static_cast<float>(shown));
		}
	};
	std::vector<Figure> yourFigures() const
	{
		std::vector<Figure> out;
		if (spectator || data.empty() || !data.youIndex)
		{
			return out;
		}
		const HudResultsPlayer &p = data.players[*data.youIndex];
		out.push_back({upper(_("Your score")), static_cast<double>(p.score), true});
		out.back().width = figureWidth(out.back());
		if (!compact)
		{
			// The others while the title and subtitle still fit beside them (the table under them has them all)
			Figure more[] = {
				{upper(_("Units killed")), static_cast<double>(p.kills), false},
				{upper(_("Units lost")), static_cast<double>(p.lost_), false},
				{upper(_("Power destroyed")), static_cast<double>(p.powerDestroyed), false},
			};
			const int availableWidth = availableHeaderRightWidth();
			int w = out[0].width;
			for (Figure &f : more)
			{
				f.width = figureWidth(f);
				w += figureGap() + f.width;
				if (w > availableWidth)
				{
					break;
				}
				out.push_back(f);
			}
		}
		return out;
	}
	int figureGap() const { return compact ? 14 : 28; }
	int figureWidth(const Figure &f) const
	{
		return std::max(static_cast<int>(iV_GetTextWidth(WzString::fromUtf8(f.label), RESULTS_LABEL_FONT)),
		                static_cast<int>(iV_GetTextWidth(WzString::fromUtf8(f.text(f.value)), RESULTS_FIGURE_FONT)));
	}
	/// A side's group on a spectator's header: its name over its players' chips, then Won or Lost
	int sideGroupWidth(size_t side) const
	{
		const int chipsW = static_cast<int>(data.sides[side].size()) * 20;
		const int labelW = static_cast<int>(iV_GetTextWidth(WzString::fromUtf8(upper(data.sideNames[side])), RESULTS_LABEL_FONT));
		const int stateW = static_cast<int>(iV_GetTextWidth(WzString::fromUtf8(upper(data.sideWon[side] ? _("Won") : _("Lost"))), RESULTS_FIGURE_FONT));
		return std::max(chipsW, labelW) + 12 + stateW;
	}

	int headerRightWidth() const override
	{
		shownFigures = yourFigures();
		if (!shownFigures.empty())
		{
			int w = 0;
			for (const Figure &f : shownFigures)
			{
				w += f.width + figureGap();
			}
			return w - figureGap();
		}
		if (compact || data.empty() || !data.resultsKnown)
		{
			return 0;
		}
		if (data.teamGame && data.sides.size() <= 4)
		{
			int w = 0;
			for (size_t s = 0; s < data.sides.size(); ++s)
			{
				w += sideGroupWidth(s) + (s ? 25 : 0);
			}
			return w;
		}
		int w = 0;
		size_t winners = 0;
		for (const HudResultsPlayer &p : data.players)
		{
			winners += data.sideWon[p.side] ? 1 : 0;
		}
		for (const HudResultsPlayer &p : data.players)
		{
			const bool won = data.sideWon[p.side];
			w += won && winners <= 2 && !spectator ? static_cast<int>(iV_GetTextWidth(WzString::fromUtf8(p.name + ","), font_regular_bold)) + 34 : 20;
		}
		return w + 50 + static_cast<int>(iV_GetTextWidth(WzString::fromUtf8(upper(_("Won"))), RESULTS_FIGURE_FONT))
		       + static_cast<int>(iV_GetTextWidth(WzString::fromUtf8(upper(_("Lost"))), RESULTS_FIGURE_FONT));
	}

	void displayHeaderRight(int right, int cy) override
	{
		const HudColors &colors = currentHudSkin().colors();
		texts.reset();
		// A player: their own figures (the table marks who won), each label over its value, fading in one after another
		// as they count up. A spectator: who won and who lost
		const std::vector<Figure> &figures = shownFigures;
		const float labelCap = hudResultsCapHeight(RESULTS_LABEL_FONT, compact);
		const float valueCap = hudResultsCapHeight(RESULTS_FIGURE_FONT, compact);
		const float gap = compact ? 4.f : 8.f;
		const float top = cy - (labelCap + gap + valueCap) / 2.f;
		if (!figures.empty())
		{
			int x = right;
			for (size_t k = figures.size(); k-- > 0;)
			{
				const Figure &f = figures[k];
				const float in = hudResultsAppear(300.f + 80.f * k, 320.f);
				const float count = hudResultsAppear(340.f + 80.f * k, 760.f);
				const float rise = 6.f * (1.f - in);
				WzText &label = texts.get();
				label.setText(WzString::fromUtf8(f.label), RESULTS_LABEL_FONT);
				label.render(static_cast<float>(x - label.width()), top + labelCap + rise, alpha(faint(), static_cast<uint8_t>(170 * in)));
				WzText &value = texts.get();
				value.setText(WzString::fromUtf8(f.text(f.value * count)), RESULTS_FIGURE_FONT);
				const PIELIGHT valueColor = f.value < 0 ? faint(200) : colors.textPrimary;
				value.render(static_cast<float>(x - value.width()), top + labelCap + gap + valueCap + rise, alpha(valueColor, static_cast<uint8_t>(valueColor.byte.a * in)));
				x -= f.width + figureGap();
			}
			return;
		}
		if (data.empty() || compact || !data.resultsKnown)
		{
			return; // (compact: the title and subtitle say it. Unknown: the title gives the player's own result)
		}
		const float in = hudResultsAppear(300.f, 320.f);
		if (data.teamGame && data.sides.size() <= 4)
		{
			// Each side: its name over its chips, then Won or Lost
			int x = right;
			for (size_t s = data.sides.size(); s-- > 0;)
			{
				const bool won = data.sideWon[s];
				WzText &state = texts.get();
				state.setText(WzString::fromUtf8(upper(won ? _("Won") : _("Lost"))), RESULTS_FIGURE_FONT);
				const PIELIGHT stateColor = won ? colors.stateOk : faint();
				state.render(static_cast<float>(x - state.width()), cy + valueCap / 2.f, alpha(stateColor, static_cast<uint8_t>(stateColor.byte.a * in)));
				x -= state.width() + 12;
				const int chipsW = static_cast<int>(data.sides[s].size()) * 20;
				WzText &label = texts.get();
				label.setText(WzString::fromUtf8(upper(data.sideNames[s])), RESULTS_LABEL_FONT);
				const int w = std::max(chipsW, label.width());
				label.render(static_cast<float>(x - w), cy - 5.f, alpha(faint(), static_cast<uint8_t>(170 * in)));
				int cx = x - w;
				for (size_t p : data.sides[s])
				{
					if (chipTexts.size() <= p)
					{
						chipTexts.resize(data.players.size());
					}
					drawPositionChip(chipTexts[p], data.players[p].index, WzRect(cx, cy + 1, 18, 18));
					cx += 20;
				}
				x -= w + 12;
				if (s > 0)
				{
					pie_UniTransBoxFill(static_cast<float>(x), static_cast<float>(cy - 18), static_cast<float>(x + 1), static_cast<float>(cy + 20), pal_RGBA(255, 255, 255, 26));
					x -= 13;
				}
			}
		}
		else
		{
			// Who won, by name (the game's rules decide, not the score), and the rest by their chips
			std::vector<size_t> winners;
			std::vector<size_t> others;
			for (size_t i = 0; i < data.players.size(); ++i)
			{
				(data.sideWon[data.players[i].side] ? winners : others).push_back(i);
			}
			if (chipTexts.size() < data.players.size())
			{
				chipTexts.resize(data.players.size());
			}
			int x = right;
			auto drawChips = [&](const std::vector<size_t> &players) {
				x -= static_cast<int>(players.size()) * 20 - 2;
				for (size_t k = 0; k < players.size(); ++k)
				{
					drawPositionChip(chipTexts[players[k]], data.players[players[k]].index, WzRect(x + static_cast<int>(k) * 20, cy - 9, 18, 18));
				}
			};
			if (!others.empty())
			{
				WzText &lost = texts.get();
				lost.setText(WzString::fromUtf8(upper(_("Lost"))), RESULTS_FIGURE_FONT);
				lost.render(static_cast<float>(x - lost.width()), cy + valueCap / 2.f, alpha(faint(), static_cast<uint8_t>(170 * in)));
				x -= lost.width() + 12;
				drawChips(others);
				x -= 14;
				pie_UniTransBoxFill(static_cast<float>(x + 4), static_cast<float>(cy - 16), static_cast<float>(x + 5), static_cast<float>(cy + 18), pal_RGBA(255, 255, 255, 26));
				x -= 12;
			}
			WzText &wonText = texts.get();
			wonText.setText(WzString::fromUtf8(upper(_("Won"))), RESULTS_FIGURE_FONT);
			wonText.render(static_cast<float>(x - wonText.width()), cy + valueCap / 2.f, alpha(colors.stateOk, static_cast<uint8_t>(255 * in)));
			x -= wonText.width() + 12;
			if (winners.size() <= 2 && !spectator)   // (a spectator: the title names them)
			{
				for (size_t k = winners.size(); k-- > 0;)
				{
					const HudResultsPlayer &p = data.players[winners[k]];
					x -= texts.draw(p.name + (k + 1 < winners.size() ? "," : ""), font_regular_bold, x, cy - 8, colors.textPrimary, true) + 24;
					drawPositionChip(chipTexts[winners[k]], p.index, WzRect(x, cy - 9, 18, 18));
					x -= 10;
				}
			}
			else
			{
				drawChips(winners);
			}
		}
	}

	void displayContent(int x0, int y0) override
	{
		const HudColors &colors = currentHudSkin().colors();
		// The tabs' underline, sliding to the chosen tab
		if (static_cast<size_t>(tabShown) < tabs.size())
		{
			const WIDGET &to = *tabs[static_cast<size_t>(tabShown)];
			const WIDGET &from = *tabs[static_cast<size_t>(tabFrom)];
			const float t = hudAppearSince(tabMovedAt, 0.f, 220.f);
			const float l = from.x() + (to.x() - from.x()) * t;
			const float w = from.width() + (to.width() - from.width()) * t;
			const float by = static_cast<float>(y0 + to.y() + to.height());
			pie_UniTransBoxFill(x0 + l, by - (compact ? 2.f : 3.f), x0 + l + w, by, alpha(colors.accent, static_cast<uint8_t>(255 * hudResultsAppear(320.f, 300.f))));
		}
		if (!noteText.empty())
		{
			int tx = x0 + 12;
			for (auto &t : tabs)
			{
				tx = std::max(tx, x0 + t->x() + t->width());
			}
			drawTag(noteTag, noteText, tx + 16, y0 + headerH() + tabsH() / 2, colors.stateWarn, false);
		}
		if (data.empty())
		{
			texts.reset();
			texts.draw(_("No statistics were recorded for this game."), font_regular, x0 + PAD, y0 + headerH() + tabsH() + 20, colors.textSecondary);
		}
	}

private:
	bool spectator = false;
	mutable std::vector<Figure> shownFigures;   ///< (a player's, chosen when the header is laid out)
	std::vector<std::string> subtitleRest;   ///< (a spectator's subtitle, after who won)
	HudGameResults data;
	/// The weapon lines' pictures, kept while the sheet is up
	std::shared_ptr<HudWeaponSubclassTextures> subclassTextures = hudWeaponSubclassTextures();
	std::string noteText;
	WzText noteTag;
	std::vector<std::shared_ptr<ChoiceButton>> tabs;
	std::shared_ptr<OverviewView> overview;
	std::shared_ptr<ChartsView> charts;
	int tabShown = 0;
	int tabFrom = 0;
	uint32_t tabMovedAt = 0;
	std::shared_ptr<ResearchView> research;
	std::shared_ptr<TimelineView> timeline;
	Texts texts;
	std::vector<ChipText> chipTexts;
};

}

std::shared_ptr<WIDGET> hudMakeGameResults(bool spectator, bool won, const std::string &note, std::vector<HudResultsButton> buttons)
{
	auto collected = std::make_shared<const HudGameResults>(HudGameResults::collect(spectator ? nonstd::nullopt : nonstd::optional<bool>(won)));
	return hudMakeResultsRoot([spectator, won, note, buttons, collected]() {
		auto sheet = std::make_shared<GameSheet>();
		sheet->initialize(spectator, won, note, *collected);
		sheet->setButtons(buttons);
		return std::static_pointer_cast<HudResultsSheet>(sheet);
	}, 150);
}
