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

#include "hudrowsummary.h"
#include "hudids.h"
#include "hudobjectinfo.h"
#include "hudpanelextension.h"
#include "hudsettings.h"
#include "hudunittypes.h"
#include "hudwidgets.h"

#include "lib/framework/frame.h"
#include "lib/gamelib/gtime.h"
#include "lib/ivis_opengl/textdraw.h"
#include "lib/widget/listwidget.h"

#include "../hci.h"
#include "../hci/build.h"
#include "../hci/commander.h"
#include "../hci/manufacture.h"
#include "../hci/objects_stats.h"
#include "../hci/research.h"
#include "../cmddroid.h"
#include "../droid.h"
#include "../group.h"
#include "../intdisplay.h"
#include "../selectionidle.h"
#include "../intimage.h"
#include "../keybind.h"
#include "../objmem.h"
#include "../structure.h"

#include <array>

namespace
{

enum class RowPanel : uint8_t
{
	Build,
	Manufacture,
	Research,
	Commanders,
};

/// The type chips' types: factory types (Manufacture), or trucks and cyborg engineers (Build)
enum class RowType : uint8_t
{
	Factory,
	CyborgFactory,
	VtolFactory,
	Truck,
	CyborgEngineer,
};
constexpr size_t ROW_TYPE_COUNT = static_cast<size_t>(RowType::CyborgEngineer) + 1;

std::array<optional<RowType>, 4> remembered;

constexpr int PAD = 4;
constexpr int GAP = 4;
// The page tabs: the full-size tabs' size and gap, and the narrowest the tabs can be
// (one small tab between scroll buttons)
constexpr int PAGE_TAB_WIDTH = 26;
constexpr int PAGE_TAB_HEIGHT = 11;
constexpr int PAGE_TAB_GAP = 2;
constexpr int PAGE_TABS_MIN_WIDTH = 7 + 2 + 13 + 2 + 7;
constexpr uint32_t REFRESH_MS = 250;

optional<RowType> rowTypeOf(BASE_OBJECT *object)
{
	if (object == nullptr)
	{
		return nullopt;
	}
	if (object->type == OBJ_STRUCTURE)
	{
		switch (static_cast<STRUCTURE *>(object)->pStructureType->type)
		{
		case REF_FACTORY: return RowType::Factory;
		case REF_CYBORG_FACTORY: return RowType::CyborgFactory;
		case REF_VTOL_FACTORY: return RowType::VtolFactory;
		default: return nullopt;
		}
	}
	if (object->type == OBJ_DROID)
	{
		const DROID *psDroid = static_cast<DROID *>(object);
		return psDroid->droidType == DROID_CYBORG_CONSTRUCT ? RowType::CyborgEngineer : RowType::Truck;
	}
	return nullopt;
}

const char *rowTypeSymbol(RowType type)
{
	switch (type)
	{
	case RowType::Factory: return hudUnitTypeSymbol(HudUnitType::Tank);
	case RowType::CyborgFactory:
	case RowType::CyborgEngineer: return hudUnitTypeSymbol(HudUnitType::Cyborg);
	case RowType::VtolFactory: return hudUnitTypeSymbol(HudUnitType::Vtol);
	case RowType::Truck: return hudUnitTypeSymbol(HudUnitType::Truck);
	}
	return "";
}

std::string rowTypeName(RowType type, unsigned count)
{
	switch (type)
	{
	case RowType::Factory: return ngettext("factory", "factories", count);
	case RowType::CyborgFactory: return ngettext("cyborg factory", "cyborg factories", count);
	case RowType::VtolFactory: return ngettext("VTOL factory", "VTOL factories", count);
	case RowType::Truck: return ngettext("truck", "trucks", count);
	case RowType::CyborgEngineer: return ngettext("cyborg engineer", "cyborg engineers", count);
	}
	return std::string();
}

class RowSummaryContent : public HudExtensionContent
{
public:
	static std::shared_ptr<RowSummaryContent> make(const std::shared_ptr<ObjectsForm> &form, RowPanel panel)
	{
		auto content = std::make_shared<RowSummaryContent>();
		content->id = IDHUD_OBJECTS_EXTENSION;
		content->form = form;
		content->panel = panel;
		if (hudSettings().rememberFilters)
		{
			content->filter = remembered[static_cast<size_t>(panel)];
		}
		content->initialize();
		return content;
	}

	HudExtensionFit fit(int panelWidth, int panelHeight, int availableHeightAbove, int availableWidthRight) override
	{
		HudExtensionFit result;
		const int needed = HUD_CHIP_HEIGHT + PAD * 2;
		if (availableHeightAbove >= needed)
		{
			result.top = needed;
		}
		return result;
	}

protected:
	void geometryChanged() override
	{
		// With room for the strip, its page tabs stand in for the list's (whose row it covers)
		const bool ownTabs = height() > 0;
		if (auto tabs = listTabs.lock())
		{
			tabs->show(!ownTabs);
		}
		pageTabs->show(ownTabs);
		layout();
	}

	void display(int xOffset, int yOffset) override
	{
		const uint32_t hovered = hoveredFactoryId();
		if (lastRefresh == 0 || realTime - lastRefresh >= REFRESH_MS || hovered != previewId)
		{
			lastRefresh = realTime;
			previewId = hovered;
			refresh();
		}
	}

private:
	void initialize()
	{
		countChip = HudChip::make(WzString(), _("Show all"));
		countChip->onPressed = [this](HudChip &, bool) { setFilter(nullopt); };
		attach(countChip);
		for (size_t i = 0; i < ROW_TYPE_COUNT; ++i)
		{
			const RowType rowType = static_cast<RowType>(i);
			auto chip = HudChip::make(WzString());
			chip->onPressed = [this, rowType](HudChip &, bool) { setFilter(filter == rowType ? optional<RowType>() : optional<RowType>(rowType)); };
			chip->hide();
			typeChips[i] = chip;
			attach(chip);
		}
		idleChip = HudChip::make(WzString());
		idleChip->setTone(HudTone::Warn);
		idleChip->onPressed = [this](HudChip &, bool shift) { cycleIdle(shift); };
		idleChip->hide();
		attach(idleChip);
		info = std::make_shared<HudOverlayText>();
		info->setTransparentToMouse(true);
		attach(info);
		addPageTabs();
		refresh();
		applyFilter();
	}

	void addPageTabs()
	{
		pageTabs = TabSelectionWidget::make();
		intSetupPageTabs(*pageTabs, TabAlignment::LeftAligned);
		intSetHudPageTabs(*pageTabs);
		attach(pageTabs);
		auto strongForm = form.lock();
		if (strongForm == nullptr || strongForm->objectsListWidget() == nullptr)
		{
			return;
		}
		auto list = strongForm->objectsListWidget();
		listTabs = std::static_pointer_cast<WIDGET>(list->tabWidget()->shared_from_this());
		hudSetReplacedPageTabs(listTabs.lock());
		pageTabs->setNumberOfTabs(list->pages());
		pageTabs->setTab(list->currentPage());
		std::weak_ptr<IntListTabWidget> weakList = list;
		pageTabs->addOnTabChangedHandler([weakList](TabSelectionWidget &, size_t tab) {
			if (auto strongList = weakList.lock())
			{
				strongList->setCurrentPage(tab);
			}
		});
		std::weak_ptr<RowSummaryContent> weakSelf = std::static_pointer_cast<RowSummaryContent>(shared_from_this());
		list->listWidget()->addOnCurrentPageChangedHandler([weakSelf](ListWidget &, size_t page) {
			if (auto self = weakSelf.lock())
			{
				self->pageTabs->setTab(page);
			}
		});
		list->listWidget()->addOnNumberOfPagesChangedHandler([weakSelf](ListWidget &, size_t pages) {
			if (auto self = weakSelf.lock())
			{
				self->pageTabs->setNumberOfTabs(pages);
				self->layout();
			}
		});
	}

	/// The page tabs' width at the strip's right end (0 with a single page), given where the chips before them end.
	/// While a factory's queue shows instead of the chips, they keep the width they had, so they don't jump.
	int pageTabsWidth(int chipsEnd)
	{
		if (previewId != 0 && tabsWidthBeforePreview >= 0)
		{
			return pageTabs->tabs() > 1 ? tabsWidthBeforePreview : 0;
		}
		tabsWidthBeforePreview = computePageTabsWidth(chipsEnd);
		return tabsWidthBeforePreview;
	}

	int computePageTabsWidth(int chipsEnd) const
	{
		const int pages = static_cast<int>(pageTabs->tabs());
		if (pages <= 1)
		{
			return 0;
		}
		const int ideal = pages * (PAGE_TAB_WIDTH + PAGE_TAB_GAP) - PAGE_TAB_GAP;
		const int availableWidth = width() - hudCloseButtonGutter() - chipsEnd - GAP;
		return std::max(PAGE_TABS_MIN_WIDTH, std::min(ideal, availableWidth));
	}

	void refresh()
	{
		auto strongForm = form.lock();
		if (strongForm == nullptr)
		{
			return;
		}
		auto &controller = strongForm->objectsController();
		const size_t count = controller.objectsSize();
		std::array<unsigned, ROW_TYPE_COUNT> typeCounts = {};
		unsigned idle = 0;
		unsigned waiting = 0;
		for (size_t i = 0; i < count; ++i)
		{
			BASE_OBJECT *object = controller.getObjectAt(i);
			if (object == nullptr || object->died)
			{
				continue;
			}
			if (auto rowType = rowTypeOf(object))
			{
				++typeCounts[static_cast<size_t>(rowType.value())];
			}
			if (object->type == OBJ_STRUCTURE)
			{
				STRUCTURE *psStruct = static_cast<STRUCTURE *>(object);
				idle += structureIsIdlePending(psStruct) ? 1 : 0;
				if (auto job = hudStructureJob(psStruct))
				{
					waiting += job->powerNeeded.has_value() && !job->onHold ? 1 : 0;
				}
			}
			else if (object->type == OBJ_DROID)
			{
				DROID *psDroid = static_cast<DROID *>(object);
				idle += droidIsIdleTruck(psDroid) ? 1 : 0;
			}
		}

		unsigned typesPresent = 0;
		for (unsigned typeCount : typeCounts)
		{
			typesPresent += typeCount > 0 ? 1 : 0;
		}
		for (size_t i = 0; i < ROW_TYPE_COUNT; ++i)
		{
			const RowType rowType = static_cast<RowType>(i);
			const bool shown = typesPresent >= 2 && typeCounts[i] > 0;
			typeChips[i]->show(shown);
			typeChips[i]->setText(WzString::fromUtf8(astringf("%s %u", rowTypeSymbol(rowType), typeCounts[i])));
			typeChips[i]->setTip(astringf(_("Show only the %s"), rowTypeName(rowType, typeCounts[i]).c_str()));
			typeChips[i]->setActive(filter == rowType);
		}
		if (filter.has_value() && (typesPresent < 2 || typeCounts[static_cast<size_t>(filter.value())] == 0))
		{
			setFilter(nullopt);
		}
		countChip->show(typesPresent >= 2);
		countChip->setText(WzString::fromUtf8(astringf("%s %u", _("All"), static_cast<unsigned>(count))));
		countChip->setActive(!filter.has_value());

		idleChip->show(idle > 0 && panel != RowPanel::Commanders);
		idleChip->setText(WzString::fromUtf8(astringf(_("● %u idle ▸"), idle)));
		idleChip->setTip(panel == RowPanel::Build ? _("Select the next idle truck (Shift: all of them)") : _("Select the next idle one"));

		std::string text;
		if (panel == RowPanel::Research && waiting > 0)
		{
			text = astringf(ngettext("⚡ %u waiting for power", "⚡ %u waiting for power", waiting), waiting);
		}
		info->setText(WzString::fromUtf8(text), currentHudSkin().colors().textSecondary);
		if (STRUCTURE *factory = previewFactory())
		{
			for (auto &chip : typeChips)
			{
				chip->hide();
			}
			idleChip->hide();
			countChip->hide();
			const int tabs = pageTabsWidth(PAD + 2);
			const int availableWidth = width() - PAD - hudCloseButtonGutter() - GAP - (tabs > 0 ? tabs + GAP : 0);
			info->setText(WzString::fromUtf8(queuePreview(factory, availableWidth)), currentHudSkin().colors().textPrimary);
		}
		layout();
	}

	uint32_t hoveredFactoryId() const
	{
		if (panel != RowPanel::Manufacture)
		{
			return 0;
		}
		auto strongForm = form.lock();
		BASE_OBJECT *object = strongForm != nullptr ? strongForm->objectUnderPointer() : nullptr;
		return object != nullptr && object->type == OBJ_STRUCTURE ? object->id : 0;
	}

	STRUCTURE *previewFactory() const
	{
		auto strongForm = form.lock();
		if (previewId == 0 || strongForm == nullptr)
		{
			return nullptr;
		}
		auto &controller = strongForm->objectsController();
		for (size_t i = 0; i < controller.objectsSize(); ++i)
		{
			BASE_OBJECT *object = controller.getObjectAt(i);
			if (object != nullptr && object->id == previewId && object->type == OBJ_STRUCTURE && !object->died)
			{
				return static_cast<STRUCTURE *>(object);
			}
		}
		return nullptr;
	}

	/// "Next: MG Viper ×3 · Truck ×1 +2 ↻", as much of it as fits in `maxWidth` pixels
	static std::string queuePreview(STRUCTURE *factory, int maxWidth)
	{
		const auto queue = hudFactoryQueue(factory);
		if (queue.empty())
		{
			return _("Nothing queued");
		}
		const bool loops = StructureGetFactory(factory)->productionLoops != 0;
		const std::string ending = loops ? " ↻" : "";
		std::string text = _("Next:");
		size_t shown = 0;
		for (const auto &entry : queue)
		{
			const std::string item = astringf("%s%s ×%u", shown == 0 ? " " : " · ", getLocalizedStatsName(entry.psTemplate), entry.remaining);
			const std::string rest = shown + 1 < queue.size() ? astringf(" +%zu", queue.size() - shown - 1) : std::string();
			if (shown > 0 && static_cast<int>(iV_GetTextWidth(WzString::fromUtf8(text + item + rest + ending), font_small)) > maxWidth)
			{
				break;
			}
			text += item;
			++shown;
		}
		if (shown < queue.size())
		{
			text += astringf(" +%zu", queue.size() - shown);
		}
		return text + ending;
	}

	void layout()
	{
		if (width() <= 0 || height() <= 0)
		{
			return;
		}
		std::vector<std::shared_ptr<WIDGET>> widgets = {countChip};
		for (auto &chip : typeChips)
		{
			widgets.push_back(chip);
		}
		widgets.push_back(idleChip);
		int chipsEnd = PAD + 2;
		for (auto &widget : widgets)
		{
			if (widget->visible())
			{
				chipsEnd += widget->idealWidth() + GAP;
			}
		}
		widgets.push_back(info);
		// The page tabs come first, at the right end, and the chips and the text fold before them
		int end = width() - hudCloseButtonGutter();
		const int tabs = pageTabsWidth(chipsEnd - GAP);
		if (tabs > 0)
		{
			end -= tabs;
			pageTabs->setGeometry(end, PAD + (HUD_CHIP_HEIGHT - PAGE_TAB_HEIGHT) / 2, tabs, PAGE_TAB_HEIGHT);
			end -= GAP;
		}
		else
		{
			pageTabs->setGeometry(end, PAD, 0, 0);
		}
		int x = PAD + 2;
		for (auto &widget : widgets)
		{
			if (!widget->visible())
			{
				continue;
			}
			const int w = widget->idealWidth();
			if (x + w > end)
			{
				widget->setGeometry(x, PAD, 0, 0);
				continue;
			}
			const int y = widget == info ? PAD + (HUD_CHIP_HEIGHT - widget->idealHeight()) / 2 : PAD;
			widget->setGeometry(x, y, w, widget == info ? widget->idealHeight() : HUD_CHIP_HEIGHT);
			x += w + GAP;
		}
	}

	void cycleIdle(bool all)
	{
		switch (panel)
		{
		case RowPanel::Build:
			if (all)
			{
				kf_SelectAllIdleTrucks();
			}
			else
			{
				kf_SelectNextIdleTruck(true)();
			}
			break;
		case RowPanel::Manufacture: kf_SelectNextIdleFactory(true)(); break;
		case RowPanel::Research: kf_SelectNextIdleResearch(true)(); break;
		case RowPanel::Commanders: break;
		}
	}

	void setFilter(optional<RowType> rowType)
	{
		if (filter == rowType)
		{
			return;
		}
		filter = rowType;
		remembered[static_cast<size_t>(panel)] = filter;
		for (size_t i = 0; i < ROW_TYPE_COUNT; ++i)
		{
			typeChips[i]->setActive(filter == static_cast<RowType>(i));
		}
		applyFilter();
	}

	void applyFilter()
	{
		auto strongForm = form.lock();
		if (strongForm == nullptr)
		{
			return;
		}
		if (!filter.has_value())
		{
			strongForm->setObjectFilter(nullptr);
			return;
		}
		const RowType rowType = filter.value();
		strongForm->setObjectFilter([rowType](BASE_OBJECT *object) {
			return rowTypeOf(object) == rowType;
		});
	}

	std::weak_ptr<ObjectsForm> form;
	RowPanel panel = RowPanel::Build;
	optional<RowType> filter;
	uint32_t lastRefresh = 0;
	uint32_t previewId = 0;
	std::shared_ptr<HudChip> countChip;
	std::array<std::shared_ptr<HudChip>, ROW_TYPE_COUNT> typeChips;
	std::shared_ptr<HudChip> idleChip;
	std::shared_ptr<HudOverlayText> info;
	std::shared_ptr<TabSelectionWidget> pageTabs;
	std::weak_ptr<WIDGET> listTabs;
	int tabsWidthBeforePreview = -1;
};

}

std::shared_ptr<HudExtensionContent> hudMakeRowSummary(const std::shared_ptr<ObjectsForm> &form)
{
	if (form == nullptr)
	{
		return nullptr;
	}
	auto &controller = form->objectsController();
	if (dynamic_cast<BuildController *>(&controller) != nullptr)
	{
		return RowSummaryContent::make(form, RowPanel::Build);
	}
	if (dynamic_cast<ManufactureController *>(&controller) != nullptr)
	{
		return RowSummaryContent::make(form, RowPanel::Manufacture);
	}
	if (dynamic_cast<ResearchController *>(&controller) != nullptr)
	{
		return RowSummaryContent::make(form, RowPanel::Research);
	}
	if (dynamic_cast<CommanderController *>(&controller) != nullptr)
	{
		return RowSummaryContent::make(form, RowPanel::Commanders);
	}
	return nullptr;
}

void hudResetRowFilters()
{
	remembered = {};
}
