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

#include "teamstrategyview.h"
#include "intelkit.h"

#include "../hudpagecontrol.h"

#include "lib/framework/frame.h"
#include "lib/ivis_opengl/pieblitfunc.h"
#include "lib/ivis_opengl/textdraw.h"
#include "lib/netplay/netplay.h"
#include "lib/widget/button.h"
#include "lib/widget/scrollablelist.h"

#include "../../multiplay.h"
#include "../../notifications.h"
#include "../../stats.h"

#include <algorithm>

using namespace intelkit;

namespace
{

constexpr int FACTS_HEIGHT = 34;
constexpr int HEADER_HEIGHT = 130;
constexpr int HEADER_HEIGHT_COMPACT = 40;
constexpr int LEGEND_HEIGHT = 44;
constexpr int LEGEND_HEIGHT_COMPACT = 40;
constexpr int NAME_WIDTH = 196;
constexpr int GROUP_GAP = 24;
constexpr int ROW_TALL = 48;
constexpr int ROW_SHORT = 38;
constexpr int TALL_ROWS_UP_TO = 5;
constexpr int ROW_COMPACT = 44;
constexpr int SECTION_COMPACT = 26;
constexpr int COLUMN_COMPACT_NAMED = 120;
constexpr int COLUMN_COMPACT = 44;
constexpr int NAMED_HEADS_UP_TO = 3;
constexpr int PAD = 16;
constexpr int PAD_COMPACT = 8;
constexpr int SCROLLBAR = 9;
constexpr int CHIP = 20;
constexpr int CHIP_COMPACT = 24;

const char *stateName(WzStrategyPlanningState state)
{
	switch (state)
	{
	case WzStrategyPlanningState::NotSpecified: return _("Not specified");
	case WzStrategyPlanningState::Planned: return _("Planned");
	case WzStrategyPlanningState::HeavilyPlanned: return _("Main focus");
	case WzStrategyPlanningState::Never: return _("Never");
	}
	return "";
}

/// A plan's tile: empty (not specified), the class's picture on blue (planned) or amber with a corner mark (main focus),
/// or a red cross (never)
void drawTile(gfx_api::texture *image, const WzRect &rect, WzStrategyPlanningState state, bool disabled, bool outlined)
{
	const HudColors &colors = skin().colors();
	const float x0 = static_cast<float>(rect.x());
	const float y0 = static_cast<float>(rect.y());
	const float x1 = static_cast<float>(rect.right());
	const float y1 = static_cast<float>(rect.bottom());
	const float radius = 5.f;
	PIELIGHT fill = pal_RGBA(12, 17, 25, 200);
	PIELIGHT border = pal_RGBA(120, 150, 190, 46);
	switch (state)
	{
	case WzStrategyPlanningState::NotSpecified:
		break;
	case WzStrategyPlanningState::Planned:
		fill = alpha(colors.accent, 52);
		border = alpha(colors.accent, 210);
		break;
	case WzStrategyPlanningState::HeavilyPlanned:
		fill = alpha(colors.stateWarn, 60);
		border = alpha(colors.stateWarn, 230);
		break;
	case WzStrategyPlanningState::Never:
		fill = alpha(colors.stateBad, 30);
		border = alpha(colors.stateBad, 150);
		break;
	}
	if (disabled)
	{
		fill.byte.a = static_cast<uint8_t>(fill.byte.a / 3);
		border.byte.a = static_cast<uint8_t>(border.byte.a / 3);
	}
	if (outlined)
	{
		border = colors.textPrimary;
	}
	pie_DrawRoundedBox(x0, y0, x1, y1, fill, radius, border, outlined ? 1.5f : 1.f);
	if (state == WzStrategyPlanningState::Planned || state == WzStrategyPlanningState::HeavilyPlanned)
	{
		if (image != nullptr)
		{
			const int inset = std::max(3, rect.width() / 8);
			const int size = rect.width() - 2 * inset;
			iV_DrawImageAnisotropic(*image, Vector2i(rect.x() + inset, rect.y() + (rect.height() - size) / 2), Vector2f(0, 0), Vector2f(size, size), 0.f,
			                        pal_RGBA(255, 255, 255, disabled ? 90 : 235));
		}
		if (state == WzStrategyPlanningState::HeavilyPlanned)
		{
			// The corner mark: a triangle in the top right corner
			const int mark = std::max(6, rect.width() / 4);
			for (int i = 0; i < mark; ++i)
			{
				pie_BoxFill(rect.right() - mark + i, rect.y() + 1 + i, rect.right() - 1, rect.y() + 2 + i, alpha(colors.stateWarn, 235));
			}
		}
	}
	else if (state == WzStrategyPlanningState::Never)
	{
		const int s = std::max(10, rect.width() / 2 - 2);
		skin().drawNamedIcon("never", WzRect(rect.x() + (rect.width() - s) / 2, rect.y() + (rect.height() - s) / 2, s, s), alpha(colors.stateBad, disabled ? 90 : 230));
	}
}

/// One player's plan for one class: theirs to read, or (the local player's) to step through
class PlanCell : public W_BUTTON
{
public:
	PlanCell(const std::shared_ptr<HudTeamStrategyView> &view, uint32_t player, size_t index): view(view), player(player), index(index)
	{
		style |= WBUT_SECONDARY;
		if (HudTeamStrategyView::classDisabled(index))
		{
			setState(WBUT_DISABLE);
		}
	}

	void released(W_CONTEXT *context, WIDGET_KEY mouseButton) override
	{
		const bool clickAndReleaseOnThisButton = (state & WBUT_DOWN) != 0;
		W_BUTTON::released(context, mouseButton);
		if (!clickAndReleaseOnThisButton || heldStepped)
		{
			heldStepped = false;
			return;
		}
		if (auto strong = view.lock())
		{
			if (player == selectedPlayer && !HudTeamStrategyView::classDisabled(index))
			{
				strong->step(index, mouseButton == WKEY_PRIMARY);
			}
		}
	}

	bool clickHeld(W_CONTEXT *, WIDGET_KEY key) override
	{
		// (As in the Classic view, a held click steps back)
		auto strong = view.lock();
		if (key != WKEY_PRIMARY || !strong || player != selectedPlayer || HudTeamStrategyView::classDisabled(index))
		{
			return false;
		}
		strong->step(index, false);
		heldStepped = true;
		return true;
	}

	void run(W_CONTEXT *) override
	{
		const WzStrategyPlanningState current = HudTeamStrategyView::state(player, index);
		const std::string name = HudTeamStrategyView::className(index, false);
		std::string tip;
		if (player == selectedPlayer)
		{
			if (HudTeamStrategyView::classDisabled(index))
			{
				tip = astringf(_("%s · not in this game"), name.c_str());
			}
			else
			{
				const char *next = stateName(teamStrategyNextState(current, true));
				tip = hudPointerIsTouch() ? astringf(_("%s · %s · tap for %s"), name.c_str(), stateName(current), next)
				                          : astringf(_("%s · %s · click for %s"), name.c_str(), stateName(current), next);
			}
		}
		else
		{
			tip = astringf("%s · %s · %s", getPlayerName(player), name.c_str(), stateName(current));
		}
		if (tip != lastTip)
		{
			setTip(tip);
			lastTip = tip;
		}
		if (isHighlighted())
		{
			if (auto strong = view.lock())
			{
				strong->setHovered(index);
			}
		}
	}

	void display(int xOffset, int yOffset) override
	{
		auto strong = view.lock();
		if (!strong)
		{
			return;
		}
		const int tile = strong->tileSize();
		const WzRect rect(xOffset + x() + (width() - tile) / 2, yOffset + y() + (height() - tile) / 2, tile, tile);
		const bool own = player == selectedPlayer;
		const bool outlined = own && ((isHighlighted() && !HudTeamStrategyView::classDisabled(index)) || strong->isFocused(player, index));
		drawTile(strong->image(index), rect, HudTeamStrategyView::state(player, index), HudTeamStrategyView::classDisabled(index), outlined);
	}

private:
	std::weak_ptr<HudTeamStrategyView> view;
	uint32_t player;
	size_t index;
	std::string lastTip;
	bool heldStepped = false;
};

/// (Full layout) A player's row: the chip and name ("Your plan" under the local player's), and the cells
class PlanRow : public WIDGET
{
public:
	PlanRow(const std::shared_ptr<HudTeamStrategyView> &view, uint32_t player, size_t rowIndex): view(view), player(player), rowIndex(rowIndex) {}

	void initialize(const std::shared_ptr<HudTeamStrategyView> &owner)
	{
		for (size_t i = 0; i < HudTeamStrategyView::classCount(); ++i)
		{
			auto cell = std::make_shared<PlanCell>(owner, player, i);
			cells.push_back(cell);
			attach(cell);
		}
	}

	void geometryChanged() override
	{
		auto strong = view.lock();
		if (!strong)
		{
			return;
		}
		const HudTeamStrategyView::Layout &l = strong->layout();
		for (size_t i = 0; i < cells.size(); ++i)
		{
			cells[i]->setGeometry(l.columnX(i), 0, l.pitch, height());
		}
	}

	void display(int xOffset, int yOffset) override
	{
		const WzRect rect(xOffset + x(), yOffset + y(), width(), height());
		const HudColors &colors = skin().colors();
		const bool you = player == selectedPlayer;
		if (you)
		{
			pie_UniTransBoxFill(static_cast<float>(rect.x()), static_cast<float>(rect.y()), static_cast<float>(rect.right()), static_cast<float>(rect.bottom()), alpha(colors.accent, 22));
			pie_BoxFill(rect.x(), rect.y() + 1, rect.x() + 3, rect.bottom() - 1, colors.accent);
		}
		else if (rowIndex % 2 == 0)
		{
			pie_UniTransBoxFill(static_cast<float>(rect.x()), static_cast<float>(rect.y()), static_cast<float>(rect.right()), static_cast<float>(rect.bottom()), pal_RGBA(255, 255, 255, 7));
		}
		const int cy = rect.y() + rect.height() / 2;
		drawPositionChip(chipText, player, WzRect(rect.x() + 12, cy - CHIP / 2, CHIP, CHIP));
		const int x0 = rect.x() + 12 + CHIP + 10;
		auto strong = view.lock();
		const int nameWidth = (strong ? strong->layout().nameWidth : NAME_WIDTH) - (x0 - rect.x()) - 8;
		const iV_fonts font = you ? font_regular_bold : font_regular;
		const int lineSize = static_cast<int>(iV_GetTextLineSize(font));
		if (you && rect.height() >= ROW_TALL)
		{
			drawLine(name, getPlayerName(player), font, x0, cy - lineSize + 2, nameWidth, colors.textPrimary);
			drawLine(sub, _("Your plan"), font_small, x0, cy + 3, nameWidth, colors.accent);
		}
		else
		{
			drawLine(name, getPlayerName(player), font, x0, cy - lineSize / 2, nameWidth, colors.textPrimary);
		}
	}

private:
	std::weak_ptr<HudTeamStrategyView> view;
	uint32_t player;
	size_t rowIndex;
	std::vector<std::shared_ptr<PlanCell>> cells;
	ChipText chipText;
	WzText name;
	WzText sub;
};

/// (Compact) A class's row: its picture and full name, and each player's cell, or a section's heading
class ClassRow : public WIDGET
{
public:
	ClassRow(const std::shared_ptr<HudTeamStrategyView> &view, nonstd::optional<size_t> index, std::string heading): view(view), index(index), heading(std::move(heading)) {}

	void initialize(const std::shared_ptr<HudTeamStrategyView> &owner)
	{
		if (index.has_value())
		{
			for (uint32_t player : owner->rowPlayers())
			{
				auto cell = std::make_shared<PlanCell>(owner, player, *index);
				cells.push_back(cell);
				attach(cell);
			}
		}
	}

	void geometryChanged() override
	{
		auto strong = view.lock();
		if (!strong)
		{
			return;
		}
		const HudTeamStrategyView::Layout &l = strong->layout();
		for (size_t i = 0; i < cells.size(); ++i)
		{
			cells[i]->setGeometry(l.nameWidth + static_cast<int>(i) * l.pitch, 0, l.pitch, height());
		}
	}

	void display(int xOffset, int yOffset) override
	{
		auto strong = view.lock();
		if (!strong)
		{
			return;
		}
		const WzRect rect(xOffset + x(), yOffset + y(), width(), height());
		const HudColors &colors = skin().colors();
		const HudTeamStrategyView::Layout &l = strong->layout();
		if (!index.has_value())
		{
			drawLine(name, heading, font_regular_bold, rect.x() + 10, rect.y() + (rect.height() - static_cast<int>(iV_GetTextLineSize(font_regular_bold))) / 2,
			         l.nameWidth - 20, colors.textSecondary);
			return;
		}
		if (*index % 2 == 1)
		{
			pie_UniTransBoxFill(static_cast<float>(rect.x()), static_cast<float>(rect.y()), static_cast<float>(rect.right()), static_cast<float>(rect.bottom()), pal_RGBA(255, 255, 255, 7));
		}
		// The local player's column, tinted
		pie_UniTransBoxFill(static_cast<float>(rect.x() + l.nameWidth), static_cast<float>(rect.y()), static_cast<float>(rect.x() + l.nameWidth + l.pitch),
		                    static_cast<float>(rect.bottom()), alpha(colors.accent, 18));
		const int s = 24;
		if (auto image = strong->image(*index))
		{
			iV_DrawImageAnisotropic(*image, Vector2i(rect.x() + 10, rect.y() + (rect.height() - s) / 2), Vector2f(0, 0), Vector2f(s, s), 0.f,
			                        pal_RGBA(255, 255, 255, HudTeamStrategyView::classDisabled(*index) ? 90 : 235));
		}
		drawLine(name, HudTeamStrategyView::className(*index, false), font_regular, rect.x() + 10 + s + 10,
		         rect.y() + (rect.height() - static_cast<int>(iV_GetTextLineSize(font_regular))) / 2, l.nameWidth - (10 + s + 10) - 8,
		         HudTeamStrategyView::classDisabled(*index) ? colors.stateDisabled : colors.textPrimary);
	}

private:
	std::weak_ptr<HudTeamStrategyView> view;
	nonstd::optional<size_t> index;
	std::string heading;
	std::vector<std::shared_ptr<PlanCell>> cells;
	WzText name;
};

/// (Compact) A player's column head: the chip, and the name when there's room (holding or pointing at it names the
/// player)
class PlayerHead : public WIDGET
{
public:
	PlayerHead(uint32_t player, bool named): player(player), named(named)
	{
		const unsigned position = player < MAX_CONNECTED_PLAYERS ? NetPlay.players[player].position : 0;
		tip = astringf("%u · %s", position, getPlayerName(player));
	}

	std::string getTip() override { return tip; }

	void display(int xOffset, int yOffset) override
	{
		const WzRect rect(xOffset + x(), yOffset + y(), width(), height());
		const int cy = rect.y() + rect.height() / 2;
		if (named)
		{
			drawPositionChip(chipText, player, WzRect(rect.x() + 8, cy - CHIP_COMPACT / 2, CHIP_COMPACT, CHIP_COMPACT));
			const bool you = player == selectedPlayer;
			const iV_fonts font = you ? font_regular_bold : font_regular;
			drawLine(name, you ? std::string(_("You")) : std::string(getPlayerName(player)), font, rect.x() + 8 + CHIP_COMPACT + 8,
			         cy - static_cast<int>(iV_GetTextLineSize(font)) / 2, rect.width() - (8 + CHIP_COMPACT + 8) - 4, skin().colors().textPrimary);
		}
		else
		{
			drawPositionChip(chipText, player, WzRect(rect.x() + (rect.width() - CHIP_COMPACT) / 2, cy - CHIP_COMPACT / 2, CHIP_COMPACT, CHIP_COMPACT));
		}
	}

private:
	uint32_t player;
	bool named;
	std::string tip;
	ChipText chipText;
	WzText name;
};

}

int HudTeamStrategyView::Layout::columnX(size_t index) const
{
	const size_t weapons = static_cast<size_t>(WSC_NUM_WEAPON_SUBCLASSES);
	return nameWidth + static_cast<int>(index) * pitch + (index >= weapons ? groupGap : 0);
}

size_t HudTeamStrategyView::classCount()
{
	return static_cast<size_t>(WSC_NUM_WEAPON_SUBCLASSES) + WzStrategyPlanningUnitTypes_NUM_TYPES;
}

HudTeamStrategyView::PlanClass HudTeamStrategyView::planClass(size_t index)
{
	PlanClass result;
	if (index < static_cast<size_t>(WSC_NUM_WEAPON_SUBCLASSES))
	{
		result.weapon = static_cast<WEAPON_SUBCLASS>(index);
	}
	else
	{
		result.unit = true;
		result.unitType = static_cast<WzStrategyPlanningUnitTypes>(index - static_cast<size_t>(WSC_NUM_WEAPON_SUBCLASSES));
	}
	return result;
}

std::string HudTeamStrategyView::className(size_t index, bool shortForm)
{
	const PlanClass c = planClass(index);
	return c.unit ? teamStrategyUnitTypeName(c.unitType) : getWeaponSubClassDisplayName(c.weapon, shortForm);
}

WzStrategyPlanningState HudTeamStrategyView::state(uint32_t player, size_t index)
{
	const PlanClass c = planClass(index);
	return c.unit ? teamStrategyUnitPlan(player, c.unitType) : teamStrategyWeaponPlan(player, c.weapon);
}

bool HudTeamStrategyView::classDisabled(size_t index)
{
	const PlanClass c = planClass(index);
	return c.unit && teamStrategyUnitTypeDisabled(c.unitType);
}

std::shared_ptr<HudTeamStrategyView> HudTeamStrategyView::make(bool compact)
{
	auto view = std::make_shared<HudTeamStrategyView>();
	view->initialize(compact);
	return view;
}

HudTeamStrategyView::~HudTeamStrategyView()
{
	for (auto image : images)
	{
		delete image;
	}
}

gfx_api::texture *HudTeamStrategyView::image(size_t index)
{
	if (index >= images.size())
	{
		return nullptr;
	}
	if (images[index] == nullptr)
	{
		const PlanClass c = planClass(index);
		if (c.unit)
		{
			const char *path = teamStrategyUnitTypeImagePath(c.unitType);
			if (path != nullptr)
			{
				images[index] = WZ_Notification_Image(path).loadImageToTexture();
			}
		}
		else
		{
			images[index] = loadImageForWeapSubclass(c.weapon);
		}
	}
	return images[index];
}

void HudTeamStrategyView::initialize(bool compactLayout)
{
	compact = compactLayout;
	images.assign(classCount(), nullptr);
	columnNames.resize(classCount());
	players.clear();
	if (selectedPlayer < MAX_PLAYERS)
	{
		players.push_back(selectedPlayer);
	}
	for (uint32_t player : teamStrategyTeammates(!NetPlay.bComms))
	{
		players.push_back(player);
	}
	// (As the Classic view in game: the local plan goes to the team as it opens)
	teamStrategySendLocalPlan();

	list = ScrollableListWidget::make();
	list->setScrollbarWidth(SCROLLBAR);
	list->setScrollbarDisplay(displayScrollbar);
	list->setExpandWhenScrollbarInvisible(false);
	attach(list);
	auto self = std::static_pointer_cast<HudTeamStrategyView>(shared_from_this());
	if (compact)
	{
		const bool named = players.size() <= NAMED_HEADS_UP_TO;
		for (uint32_t player : players)
		{
			auto head = std::make_shared<PlayerHead>(player, named);
			heads.push_back(head);
			attach(head);
		}
		const size_t weapons = static_cast<size_t>(WSC_NUM_WEAPON_SUBCLASSES);
		const auto addRow = [this, &self](nonstd::optional<size_t> index, const std::string &heading) {
			auto row = std::make_shared<ClassRow>(self, index, heading);
			row->initialize(self);
			rows.push_back(row);
		};
		addRow(nonstd::nullopt, _("Weapons"));
		for (size_t i = 0; i < classCount(); ++i)
		{
			if (i == weapons)
			{
				addRow(nonstd::nullopt, _("Units"));
			}
			addRow(i, std::string());
		}
	}
	else
	{
		for (size_t i = 0; i < players.size(); ++i)
		{
			auto row = std::make_shared<PlanRow>(self, players[i], i);
			row->initialize(self);
			rows.push_back(row);
		}
	}
	for (auto &row : rows)
	{
		list->addItem(row);
	}
}

int HudTeamStrategyView::tileSize() const
{
	if (compact)
	{
		return 36;
	}
	return lay.rowHeight >= ROW_TALL ? 34 : 30;
}

WzRect HudTeamStrategyView::tableRect() const
{
	const int pad = compact ? PAD_COMPACT : PAD;
	const int top = compact ? 4 : FACTS_HEIGHT;
	const int legend = compact ? LEGEND_HEIGHT_COMPACT : LEGEND_HEIGHT;
	return WzRect(pad, top, std::max(0, width() - 2 * pad), std::max(0, height() - top - legend));
}

int HudTeamStrategyView::heightFor(int width) const
{
	if (compact)
	{
		return 4 + HEADER_HEIGHT_COMPACT + 2 * SECTION_COMPACT + static_cast<int>(classCount()) * ROW_COMPACT + LEGEND_HEIGHT_COMPACT;
	}
	const int rowHeight = static_cast<int>(players.size()) <= TALL_ROWS_UP_TO ? ROW_TALL : ROW_SHORT;
	return FACTS_HEIGHT + HEADER_HEIGHT + static_cast<int>(std::max<size_t>(players.size(), 1)) * rowHeight + LEGEND_HEIGHT;
}

void HudTeamStrategyView::layoutChildren()
{
	const WzRect table = tableRect();
	const int rowWidth = std::max(0, table.width() - SCROLLBAR);
	Layout l;
	if (compact)
	{
		l.header = HEADER_HEIGHT_COMPACT;
		l.pitch = players.size() <= NAMED_HEADS_UP_TO ? COLUMN_COMPACT_NAMED : COLUMN_COMPACT;
		l.nameWidth = std::max(120, rowWidth - static_cast<int>(players.size()) * l.pitch);
		l.pitch = std::max(40, std::min(l.pitch, (rowWidth - 120) / std::max<int>(1, static_cast<int>(players.size()))));
		l.nameWidth = rowWidth - static_cast<int>(players.size()) * l.pitch;
		l.rowHeight = ROW_COMPACT;
	}
	else
	{
		l.header = HEADER_HEIGHT;
		l.nameWidth = NAME_WIDTH;
		l.groupGap = GROUP_GAP;
		l.pitch = std::max(34, std::min(46, (rowWidth - NAME_WIDTH - GROUP_GAP - 8) / static_cast<int>(classCount())));
		l.rowHeight = static_cast<int>(players.size()) <= TALL_ROWS_UP_TO ? ROW_TALL : ROW_SHORT;
	}
	lay = l;
	list->setGeometry(table.x(), table.y() + l.header, table.width(), std::max(0, table.height() - l.header));
	for (auto &row : rows)
	{
		int h = l.rowHeight;
		if (compact)
		{
			h = row->children().empty() ? SECTION_COMPACT : ROW_COMPACT;
		}
		row->setGeometry(row->x(), row->y(), rowWidth, h);
	}
	list->setLayoutDirty();
	for (size_t i = 0; i < heads.size(); ++i)
	{
		heads[i]->setGeometry(table.x() + l.nameWidth + static_cast<int>(i) * l.pitch, table.y(), l.pitch, l.header);
	}
}

void HudTeamStrategyView::geometryChanged()
{
	layoutChildren();
}

void HudTeamStrategyView::run(W_CONTEXT *)
{
	hovered.reset();
}

bool HudTeamStrategyView::isFocused(uint32_t player, size_t index) const
{
	return focusShown && player == selectedPlayer && index == focus;
}

void HudTeamStrategyView::step(size_t index, bool forward)
{
	const PlanClass c = planClass(index);
	if (c.unit)
	{
		teamStrategyStepUnitPlan(c.unitType, forward, true);
	}
	else
	{
		teamStrategyStepWeaponPlan(c.weapon, forward, true);
	}
}

void HudTeamStrategyView::moveFocus(int delta)
{
	if (!focusShown)
	{
		focusShown = true;
		return;
	}
	const int last = static_cast<int>(classCount()) - 1;
	int next = static_cast<int>(focus);
	do
	{
		next = std::max(0, std::min(last, next + delta));
	}
	while (classDisabled(static_cast<size_t>(next)) && next != 0 && next != last);
	if (!classDisabled(static_cast<size_t>(next)))
	{
		focus = static_cast<size_t>(next);
	}
	if (compact)
	{
		// (The class's row: after the Weapons heading, and the Units heading for the unit types)
		const size_t row = focus + 1 + (focus >= static_cast<size_t>(WSC_NUM_WEAPON_SUBCLASSES) ? 1 : 0);
		list->scrollEnsureItemVisible(row);
	}
}

void HudTeamStrategyView::stepFocus(bool forward)
{
	if (!focusShown || players.empty() || players.front() != selectedPlayer || classDisabled(focus))
	{
		focusShown = true;
		return;
	}
	step(focus, forward);
}

bool HudTeamStrategyView::applyForTests(const std::string &key, const std::string &value)
{
	if (key == "focus")
	{
		const size_t index = static_cast<size_t>(std::strtoul(value.c_str(), nullptr, 10));
		if (index >= classCount())
		{
			return false;
		}
		focus = index;
		focusShown = true;
		return true;
	}
	return false;
}

void HudTeamStrategyView::display(int xOffset, int yOffset)
{
	const HudColors &colors = skin().colors();
	const WzRect rect(xOffset + x(), yOffset + y(), width(), height());
	const WzRect table = tableRect();
	const int tx = rect.x() + table.x();
	const int ty = rect.y() + table.y();
	if (!compact)
	{
		const size_t teammates = players.empty() ? 0 : players.size() - 1;
		const std::string line = teammates == 0 ? std::string(_("No teammates."))
		                                        : astringf(_("%zu on your team · what each of you plans to build"), players.size());
		drawLine(factsText, line, font_small, tx, rect.y() + 10, table.width(), colors.textSecondary);

		// The header band: the groups, and each column's name (sideways) over its picture
		const Layout &l = lay;
		const size_t weapons = static_cast<size_t>(WSC_NUM_WEAPON_SUBCLASSES);
		drawLine(groupTexts[0], _("WEAPONS"), font_small, tx + l.columnX(0) + 4, ty + 4, l.pitch * 6, colors.textSecondary);
		drawLine(groupTexts[1], _("UNITS"), font_small, tx + l.columnX(weapons) + 4, ty + 4, l.pitch * 3, colors.textSecondary);
		drawLine(groupTexts[2], _("Player"), font_small, tx + 12, ty + l.header - 22, l.nameWidth - 20, colors.textSecondary);
		const int dividerX = tx + l.columnX(weapons) - l.groupGap / 2;
		drawDivider(dividerX, ty, dividerX + 1, rect.bottom() - (compact ? LEGEND_HEIGHT_COMPACT : LEGEND_HEIGHT) - 4);
		const int icon = 22;
		for (size_t i = 0; i < classCount(); ++i)
		{
			const int cx = tx + l.columnX(i) + l.pitch / 2;
			const bool disabled = classDisabled(i);
			if (auto img = image(i))
			{
				iV_DrawImageAnisotropic(*img, Vector2i(cx - icon / 2, ty + l.header - 6 - icon), Vector2f(0, 0), Vector2f(icon, icon), 0.f,
				                        pal_RGBA(255, 255, 255, disabled ? 90 : 235));
			}
			const bool hot = hovered.has_value() && *hovered == i;
			WzText &text = columnNames[i];
			const int bottom = ty + l.header - 6 - icon - 6;
			const int availableHeight = bottom - (ty + 22);
			text.setText(WzString::fromUtf8(className(i, true)), font_small, std::max(availableHeight, 1));
			const float textX = static_cast<float>(cx) - text.lineSize() / 2.f - static_cast<float>(text.aboveBase());
			text.render(textX, static_cast<float>(bottom), disabled ? colors.stateDisabled : (hot ? colors.textPrimary : colors.textSecondary), 270.f);
		}
		drawDivider(tx, ty + l.header - 1, tx + table.width(), ty + l.header);
	}
	else
	{
		drawLine(titleText, _("Plans"), font_regular_bold, tx + 10, ty + (HEADER_HEIGHT_COMPACT - static_cast<int>(iV_GetTextLineSize(font_regular_bold))) / 2,
		         lay.nameWidth - 20, colors.textSecondary);
		pie_UniTransBoxFill(static_cast<float>(tx + lay.nameWidth), static_cast<float>(ty), static_cast<float>(tx + lay.nameWidth + lay.pitch),
		                    static_cast<float>(ty + HEADER_HEIGHT_COMPACT), alpha(colors.accent, 18));
		drawDivider(tx, ty + HEADER_HEIGHT_COMPACT - 1, tx + table.width(), ty + HEADER_HEIGHT_COMPACT);
	}

	// The legend
	const int legendHeight = compact ? LEGEND_HEIGHT_COMPACT : LEGEND_HEIGHT;
	const int cy = rect.bottom() - legendHeight / 2 - 2;
	int lx = tx;
	const WzStrategyPlanningState states[] = {WzStrategyPlanningState::NotSpecified, WzStrategyPlanningState::Planned,
	                                          WzStrategyPlanningState::HeavilyPlanned, WzStrategyPlanningState::Never};
	const int sample = compact ? 22 : 22;
	for (size_t i = 0; i < 4; ++i)
	{
		drawTile(image(1), WzRect(lx, cy - sample / 2, sample, sample), states[i], false, false);
		lx += sample + 8;
		legendTexts[i].setText(WzString::fromUtf8(stateName(states[i])), font_small);
		legendTexts[i].render(static_cast<float>(lx), std::round(static_cast<float>(cy) - legendTexts[i].lineSize() / 2.f - legendTexts[i].aboveBase()), colors.textPrimary);
		lx += legendTexts[i].width() + 22;
	}
	legendTexts[4].setText(WzString::fromUtf8(hudPointerIsTouch() ? _("Tap: next · hold: back") : _("Click: next · right-click or hold: back")), font_small);
	if (lx + legendTexts[4].width() <= rect.right() - 8)
	{
		legendTexts[4].render(static_cast<float>(lx), std::round(static_cast<float>(cy) - legendTexts[4].lineSize() / 2.f - legendTexts[4].aboveBase()), colors.textSecondary);
	}
}
