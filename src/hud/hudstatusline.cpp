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

#include "hudstatusline.h"
#include "hudids.h"
#include "hudmodel.h"
#include "hudtext.h"
#include "hudunittypes.h"
#include "hudwidgets.h"

#include "lib/framework/frame.h"
#include "lib/framework/input.h"
#include "lib/gamelib/gtime.h"
#include "lib/netplay/netplay.h"
#include "lib/widget/form.h"
#include "lib/widget/popovermenu.h"

#include "../droid.h"
#include "../game_world.h"
#include "../hci.h"
#include "../levels.h"
#include "../loop.h"
#include "../multiplay.h"
#include "../objmem.h"
#include "../research.h"
#include "../scores.h"
#include "../selection.h"
#include "../titleui/widgets/optionsform.h"

#include <array>

constexpr int LINE_HEIGHT = 13;
constexpr int SEGMENT_GAP = 10;
constexpr int CHIP_GAP = 4;
constexpr size_t MAX_CHIPS = 4;

static bool statusLineVisible = false;
static bool readoutVisible = false;

static bool localPlayerIsSpectating()
{
	return selectedPlayer >= MAX_PLAYERS || NETisReplay() || (bMultiPlayer && NetPlay.players[selectedPlayer].isSpectator);
}

/// The top of the two lines, where Classic draws the level name and the clock
WzRect hudStatusLineRect()
{
	const WzRect zone = hudStatusZone();
	if (zone.width() <= 0)
	{
		return WzRect();
	}
	const int x = std::max(zone.x(), RET_X + 134);
	const int y = 400 + E_H;
	return WzRect(x, y, zone.right() - x, LINE_HEIGHT * 2);
}

namespace
{

WzRect readoutRect()
{
	const WzRect line = hudStatusLineRect();
	if (line.width() <= 0)
	{
		return WzRect();
	}
	return WzRect(line.x(), line.y() + LINE_HEIGHT - 2, line.width(), HUD_CHIP_HEIGHT);
}

/// Text segments in one line, left to right
class SegmentLine : public WIDGET
{
public:
	void setSegments(const std::vector<std::pair<std::string, PIELIGHT>> &segments)
	{
		while (texts.size() < segments.size())
		{
			auto text = std::make_shared<HudOverlayText>();
			text->setTransparentToMouse(true);
			attach(text);
			texts.push_back(text);
		}
		int x = 0;
		for (size_t i = 0; i < texts.size(); ++i)
		{
			if (i >= segments.size())
			{
				texts[i]->hide();
				continue;
			}
			texts[i]->show();
			texts[i]->setText(WzString::fromUtf8(segments[i].first), segments[i].second);
			const int w = texts[i]->idealWidth();
			const bool fits = x + w <= width();
			texts[i]->show(fits);
			texts[i]->setGeometry(x, 0, w, LINE_HEIGHT);
			x += w + SEGMENT_GAP;
		}
	}

private:
	std::vector<std::shared_ptr<HudOverlayText>> texts;
};

class StatusLineModule : public HudModule
{
public:
	HudModuleId moduleId() const override { return HudModuleId::StatusLine; }

	void create(const std::shared_ptr<W_FORM> &root) override
	{
		container = std::make_shared<HudZoneForm>();
		container->id = IDHUD_STATUS_LINE;
		container->setHelp(WidgetHelp()
			.setTitle(_("Status Line"))
			.setDescription(_("The game clock and speed, your units versus the unit limit, and what your research facilities will finish next.")));
		root->attach(container);
		for (auto &line : lines)
		{
			line = std::make_shared<SegmentLine>();
			line->setTransparentToMouse(true);
			container->attach(line);
		}
		container->setCalcLayout([this](WIDGET *psWidget) {
			const WzRect rect = hudStatusLineRect();
			psWidget->setGeometry(rect.x(), rect.y(), rect.width(), rect.height());
			for (size_t i = 0; i < lines.size(); ++i)
			{
				lines[i]->setGeometry(0, static_cast<int>(i) * LINE_HEIGHT, rect.width(), LINE_HEIGHT);
			}
		});
		update();
	}

	void destroy() override
	{
		statusLineVisible = false;
		if (container)
		{
			if (auto parent = container->parent())
			{
				parent->detach(container);
			}
			container.reset();
		}
	}

	void setVisible(bool visible) override
	{
		shown = visible;
		update();
	}

	void update() override
	{
		if (!container)
		{
			return;
		}
		const bool zoneUsable = container->width() > 0;
		container->show(shown && zoneUsable);
		statusLineVisible = shown && zoneUsable;
		if (!statusLineVisible)
		{
			return;
		}
		auto &skin = currentHudSkin();
		const PIELIGHT normal = WZCOL_TEXT_MEDIUM;
		const PIELIGHT warn = skin.toneColor(HudTone::Warn);

		std::vector<std::pair<std::string, PIELIGHT>> first;
		char clock[255];
		getAsciiTime(clock, gameTime, false);
		std::string clockText = clock;
		const double speed = gameTimeGetMod().asDouble();
		if (gamePaused())
		{
			clockText += " ‖";
		}
		else if (speed != 1.0)
		{
			clockText += astringf(" ×%g", speed);
		}
		first.emplace_back(clockText, normal);

		std::vector<std::pair<std::string, PIELIGHT>> second;
		if (!localPlayerIsSpectating())
		{
			const auto &units = hudModel().units();
			first.emplace_back(astringf(_("Units %u/%u"), units.count, units.limit), units.atLimit ? warn : normal);
			if (hudModuleActive(HudModuleId::PowerDetails))
			{
				const auto &power = hudModel().power();
				if (power.incomePerMinute.has_value())
				{
					first.emplace_back(astringf(_("⚡ +%d/min"), power.incomePerMinute.value()), normal);
				}
				if (power.unusedDerricks > 0)
				{
					first.emplace_back(astringf(ngettext("%u derrick unpowered", "%u derricks unpowered", power.unusedDerricks), power.unusedDerricks), warn);
				}
			}
			if (!readoutVisible)
			{
				researchSegments(second, normal, warn);
			}
		}
		first.emplace_back(mapNameWithoutTechlevel(getLevelName()), WZCOL_TEXT_DARK);
		lines[0]->setSegments(first);
		lines[1]->setSegments(second);
	}

private:
	void researchSegments(std::vector<std::pair<std::string, PIELIGHT>> &segments, PIELIGHT normal, PIELIGHT warn)
	{
		const auto &research = hudModel().research();
		if (research.facilities == 0)
		{
			return;
		}
		if (research.active == 0)
		{
			segments.emplace_back(astringf(ngettext("%u lab, not researching", "%u labs, none researching", research.facilities), research.facilities), warn);
			return;
		}
		std::string text = astringf(ngettext("%u lab", "%u labs", research.active), research.active);
		if (research.nextTopic.has_value() && research.nextTopic.value() < asResearch.size())
		{
			text += " · " + astringf(_("next: %s %s"), getLocalizedStatsName(&asResearch[research.nextTopic.value()]), hudFormatDuration(research.nextSeconds).c_str());
		}
		segments.emplace_back(text, normal);
		if (research.waitingForPower > 0)
		{
			segments.emplace_back(astringf(ngettext("⚡ %u waiting for power", "⚡ %u waiting for power", research.waitingForPower), research.waitingForPower), warn);
		}
		if (research.onHold > 0)
		{
			segments.emplace_back(astringf(ngettext("‖ %u on hold", "‖ %u on hold", research.onHold), research.onHold), warn);
		}
	}

	std::shared_ptr<W_FORM> container;
	std::array<std::shared_ptr<SegmentLine>, 2> lines;
	bool shown = true;
};

class SelectionReadoutModule : public HudModule
{
public:
	HudModuleId moduleId() const override { return HudModuleId::SelectionReadout; }

	void create(const std::shared_ptr<W_FORM> &root) override
	{
		container = std::make_shared<HudZoneForm>();
		container->id = IDHUD_SELECTION_READOUT;
		container->setHelp(WidgetHelp()
			.setTitle(_("Selection"))
			.setDescription(_("What the selected units are made of. Click a type to keep only those units, or Shift-click it to drop them.")));
		root->attach(container);
		summary = std::make_shared<HudOverlayText>();
		summary->setTransparentToMouse(true);
		container->attach(summary);
		for (size_t i = 0; i < HUD_UNIT_TYPE_COUNT; ++i)
		{
			const HudUnitType type = static_cast<HudUnitType>(i);
			auto chip = HudChip::make(WzString());
			chip->onPressed = [type](HudChip &, bool shift) { refineSelection(type, shift); };
			chip->hide();
			chips[i] = chip;
			container->attach(chip);
		}
		moreChip = HudChip::make(WzString(), _("More types"));
		moreChip->onPressed = [this](HudChip &, bool) { openMoreMenu(); };
		moreChip->hide();
		container->attach(moreChip);
		health = std::make_shared<HudOverlayText>();
		health->setTransparentToMouse(true);
		container->attach(health);
		container->setCalcLayout([this](WIDGET *psWidget) {
			const WzRect rect = readoutRect();
			psWidget->setGeometry(rect.x(), rect.y(), rect.width(), rect.height());
			layout();
		});
		update();
	}

	void destroy() override
	{
		readoutVisible = false;
		if (container)
		{
			if (auto parent = container->parent())
			{
				parent->detach(container);
			}
			container.reset();
		}
	}

	void setVisible(bool visible) override
	{
		shown = visible;
		update();
	}

	void update() override
	{
		if (!container)
		{
			return;
		}
		counts = {};
		unsigned total = 0;
		unsigned body = 0;
		unsigned originalBody = 0;
		const DROID *mostExperienced = nullptr;
		if (selectedPlayer < MAX_PLAYERS && !localPlayerIsSpectating())
		{
			for (const DROID *psDroid : gameWorld.objects.droids[selectedPlayer])
			{
				if (!psDroid->selected || psDroid->died)
				{
					continue;
				}
				++total;
				++counts[static_cast<size_t>(hudUnitTypeOf(psDroid))];
				body += psDroid->body;
				originalBody += psDroid->originalBody;
				if (mostExperienced == nullptr || psDroid->experience > mostExperienced->experience)
				{
					mostExperienced = psDroid;
				}
			}
		}
		const bool visible = shown && total > 0 && intOpenObjectPanel() == IOBJ_NONE && container->width() > 0;
		container->show(visible);
		readoutVisible = visible;
		if (!visible)
		{
			return;
		}
		summary->setText(WzString::fromUtf8(astringf(ngettext("%u selected", "%u selected", total), total)), WZCOL_TEXT_BRIGHT);
		std::string healthText = astringf(_("♥ %u%%"), originalBody > 0 ? body * 100 / originalBody : 0);
		if (mostExperienced != nullptr && getDroidLevel(mostExperienced) > 0)
		{
			healthText += astringf(" · ★ %s", getDroidLevelName(mostExperienced));
		}
		health->setText(WzString::fromUtf8(healthText), WZCOL_TEXT_MEDIUM);
		for (size_t i = 0; i < HUD_UNIT_TYPE_COUNT; ++i)
		{
			const HudUnitType type = static_cast<HudUnitType>(i);
			chips[i]->setText(WzString::fromUtf8(astringf("%s %u", hudUnitTypeSymbol(type), counts[i])));
			chips[i]->setTip(astringf(_("%u %s: click to keep only these, Shift-click to drop them"), counts[i], hudUnitTypeName(type, counts[i])));
		}
		layout();
	}

private:
	static void refineSelection(HudUnitType type, bool drop)
	{
		if (selectedPlayer >= MAX_PLAYERS)
		{
			return;
		}
		selSetDroidSelection(gameWorld.objects, selectedPlayer, [type, drop](const DROID *psDroid) {
			return psDroid->selected && ((hudUnitTypeOf(psDroid) == type) != drop);
		});
	}

	std::vector<size_t> presentTypes() const
	{
		std::vector<size_t> present;
		for (size_t i = 0; i < HUD_UNIT_TYPE_COUNT; ++i)
		{
			if (counts[i] > 0)
			{
				present.push_back(i);
			}
		}
		return present;
	}

	void layout()
	{
		if (!container || container->width() <= 0)
		{
			return;
		}
		const int width = container->width();
		const auto present = presentTypes();
		int x = 0;
		const int textY = (HUD_CHIP_HEIGHT - summary->idealHeight()) / 2;
		summary->setGeometry(x, textY, summary->idealWidth(), summary->idealHeight());
		x += summary->idealWidth() + CHIP_GAP * 2;

		const size_t shownChips = present.size() > MAX_CHIPS ? MAX_CHIPS - 1 : present.size();
		int chipsWidth = 0;
		for (size_t n = 0; n < shownChips; ++n)
		{
			chipsWidth += chips[present[n]]->idealWidth() + CHIP_GAP;
		}
		moreChip->setText(WzString::fromUtf8(astringf("+%zu", present.size() - shownChips)));
		const bool more = present.size() > shownChips;
		if (more)
		{
			chipsWidth += moreChip->idealWidth() + CHIP_GAP;
		}
		const bool chipsFit = present.size() >= 2 && x + chipsWidth + health->idealWidth() <= width;
		for (auto &chip : chips)
		{
			chip->hide();
		}
		moreChip->hide();
		if (chipsFit)
		{
			for (size_t n = 0; n < shownChips; ++n)
			{
				auto &chip = chips[present[n]];
				chip->show();
				chip->setGeometry(x, 0, chip->idealWidth(), HUD_CHIP_HEIGHT);
				x += chip->idealWidth() + CHIP_GAP;
			}
			if (more)
			{
				moreChip->show();
				moreChip->setGeometry(x, 0, moreChip->idealWidth(), HUD_CHIP_HEIGHT);
				x += moreChip->idealWidth() + CHIP_GAP;
			}
			x += CHIP_GAP;
		}
		const int healthWidth = std::min(health->idealWidth(), std::max(0, width - x));
		health->setGeometry(x, textY, healthWidth, health->idealHeight());
		health->show(healthWidth == health->idealWidth());
	}

	void openMoreMenu()
	{
		const auto present = presentTypes();
		auto menu = PopoverMenuWidget::make();
		for (size_t n = MAX_CHIPS - 1; n < present.size(); ++n)
		{
			const HudUnitType type = static_cast<HudUnitType>(present[n]);
			auto item = WzClickableOptionsChoiceWidget::make(font_small);
			item->setString(WzString::fromUtf8(astringf("%s %u %s", hudUnitTypeSymbol(type), counts[present[n]], hudUnitTypeName(type, counts[present[n]]))));
			item->addOnClickHandler([type](WzClickableOptionsChoiceWidget &, WIDGET_KEY) {
				refineSelection(type, keyDown(KEY_LSHIFT) || keyDown(KEY_RSHIFT));
			});
			item->setGeometry(0, 0, item->idealWidth(), item->idealHeight());
			menu->addMenuItem(item, true);
		}
		hudOpenMenu(menu, moreChip);
	}

	std::shared_ptr<W_FORM> container;
	std::shared_ptr<HudOverlayText> summary;
	std::array<std::shared_ptr<HudChip>, HUD_UNIT_TYPE_COUNT> chips;
	std::shared_ptr<HudChip> moreChip;
	std::shared_ptr<HudOverlayText> health;
	HudUnitCounts counts = {};
	bool shown = true;
};

}

std::unique_ptr<HudModule> hudMakeStatusLine()
{
	return std::make_unique<StatusLineModule>();
}

std::unique_ptr<HudModule> hudMakeSelectionReadout()
{
	return std::make_unique<SelectionReadoutModule>();
}

bool hudStatusLineShown()
{
	return statusLineVisible;
}
