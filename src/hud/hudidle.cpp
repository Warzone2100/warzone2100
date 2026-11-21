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

#include "hudidle.h"
#include "hudids.h"
#include "hudkeys.h"
#include "hudmodel.h"
#include "hudstatusline.h"
#include "hudunittypes.h"
#include "hudwidgets.h"

#include "lib/framework/frame.h"
#include "lib/ivis_opengl/pieclip.h"
#include "lib/widget/form.h"

#include "../keybind.h"

#include <array>

namespace
{

constexpr int CHIP_GAP = 4;
constexpr int ZONE_HEIGHT = 20;
constexpr int MIN_ZONE_WIDTH = 160;

enum class IdleKind : uint8_t
{
	Trucks,
	Factories,
	ResearchFacilities,
};
constexpr size_t IDLE_KIND_COUNT = 3;

class IdleIndicatorsModule : public HudModule
{
public:
	HudModuleId moduleId() const override { return HudModuleId::IdleIndicators; }

	void create(const std::shared_ptr<W_FORM> &root) override
	{
		container = std::make_shared<HudZoneForm>();
		container->id = IDHUD_IDLE_INDICATORS;
		container->setHelp(WidgetHelp()
			.setTitle(_("Idle Indicators"))
			.setDescription(_("Idle trucks, factories and research facilities. Click one to select the next idle one, or Shift-click the trucks to select them all.")));
		root->attach(container);
		for (size_t i = 0; i < IDLE_KIND_COUNT; ++i)
		{
			const IdleKind kind = static_cast<IdleKind>(i);
			auto chip = HudChip::make(WzString());
			chip->setTone(HudTone::Warn);
			chip->onPressed = [kind](HudChip &, bool shift) { select(kind, shift); };
			chip->hide();
			chips[i] = chip;
			container->attach(chip);
		}
		container->setCalcLayout([this](WIDGET *psWidget) {
			const WzRect line = hudStatusLineRect();
			leftAligned = line.width() >= MIN_ZONE_WIDTH;
			if (leftAligned)
			{
				psWidget->setGeometry(line.x(), line.y() - ZONE_HEIGHT - 2, line.width(), ZONE_HEIGHT);
			}
			else
			{
				const WzRect zone = hudRadarZone();
				const int availableWidth = static_cast<int>(pie_GetVideoBufferWidth());
				const int width = std::min(zone.width(), availableWidth);
				const int x = std::max(0, std::min(zone.x(), availableWidth - width));
				psWidget->setGeometry(x, std::max(0, zone.y()), width, zone.height());
			}
			layout();
		});
		update();
	}

	void destroy() override
	{
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
		container->callCalcLayout();
		container->show(shown && container->width() > 0);
		const auto &idle = hudModel().idle();
		counts = {idle.trucks, idle.factories, idle.researchFacilities};
		chips[0]->setText(WzString::fromUtf8(astringf("%s %u", hudUnitTypeSymbol(HudUnitType::Truck), counts[0])));
		chips[1]->setText(WzString::fromUtf8(astringf("%s %u", hudUnitTypeSymbol(HudUnitType::Tank), counts[1])));
		chips[2]->setText(WzString::fromUtf8(astringf("⚛ %u", counts[2])));
		chips[0]->setTip(hudTipWithKeyBinding(astringf(ngettext("%u idle truck: select the next one (Shift: all of them)", "%u idle trucks: select the next one (Shift: all of them)", counts[0]), counts[0]), "SelectNextIdleTruck"));
		chips[1]->setTip(hudTipWithKeyBinding(astringf(ngettext("%u idle factory: select the next one", "%u idle factories: select the next one", counts[1]), counts[1]), "SelectNextIdleFactory"));
		chips[2]->setTip(hudTipWithKeyBinding(astringf(ngettext("%u idle research facility: select the next one", "%u idle research facilities: select the next one", counts[2]), counts[2]), "SelectNextIdleResearch"));
		layout();
	}

private:
	static void select(IdleKind kind, bool all)
	{
		switch (kind)
		{
		case IdleKind::Trucks:
			if (all)
			{
				kf_SelectAllIdleTrucks();
			}
			else
			{
				kf_SelectNextIdleTruck(true)();
			}
			break;
		case IdleKind::Factories: kf_SelectNextIdleFactory(true)(); break;
		case IdleKind::ResearchFacilities: kf_SelectNextIdleResearch(true)(); break;
		}
	}

	void layout()
	{
		if (!container)
		{
			return;
		}
		int used = 0;
		std::array<bool, IDLE_KIND_COUNT> visible = {};
		for (size_t i = 0; i < IDLE_KIND_COUNT; ++i)
		{
			const int needed = used + (used > 0 ? CHIP_GAP : 0) + chips[i]->idealWidth();
			visible[i] = counts[i] > 0 && needed <= container->width();
			if (visible[i])
			{
				used = needed;
			}
		}
		int x = leftAligned ? 0 : container->width() - used;
		for (size_t i = 0; i < IDLE_KIND_COUNT; ++i)
		{
			auto &chip = chips[i];
			chip->show(visible[i]);
			if (!visible[i])
			{
				continue;
			}
			chip->setGeometry(x, (container->height() - HUD_CHIP_HEIGHT) / 2, chip->idealWidth(), HUD_CHIP_HEIGHT);
			x += chip->idealWidth() + CHIP_GAP;
		}
	}

	std::shared_ptr<W_FORM> container;
	std::array<std::shared_ptr<HudChip>, IDLE_KIND_COUNT> chips;
	std::array<unsigned, IDLE_KIND_COUNT> counts = {};
	bool shown = true;
	bool leftAligned = false;
};

}

std::unique_ptr<HudModule> hudMakeIdleIndicators()
{
	return std::make_unique<IdleIndicatorsModule>();
}
