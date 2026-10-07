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

#include "hudpanelfilters.h"

#include "lib/framework/frame.h"

#include "hudbuildcategories.h"
#include "hudids.h"
#include "hudpanelextension.h"
#include "hudsettings.h"
#include "hudskin.h"
#include "hudwidgets.h"

#include "lib/framework/wzapp.h"
#include "lib/widget/editbox.h"
#include "lib/widget/popovermenu.h"

#include "../hci.h"
#include "../hci/build.h"
#include "../hci/manufacture.h"
#include "../hci/objects_stats.h"
#include "../hci/research.h"
#include "../droid.h"
#include "../research.h"
#include "../researchtree/researchtreelayout.h"
#include "../titleui/widgets/optionsform.h"

#include <array>
#include <unordered_map>

namespace
{

enum class FilterPanel : uint8_t
{
	Build,
	Manufacture,
	Research,
};

struct FilterState
{
	std::string category;
	WzString search;
	optional<uint8_t> lane;

	bool isDefault() const
	{
		return category.empty() && search.isEmpty() && !lane.has_value();
	}
};

std::array<FilterState, 3> remembered;

constexpr int PAD = 4;
constexpr int GAP = 3;
constexpr int COMPACT_WIDTH = 22;
constexpr size_t MAX_CATEGORIES = 4;
constexpr uint32_t REFRESH_MS = 500;

void displaySearchBox(WIDGET *widget, UDWORD xOffset, UDWORD yOffset)
{
	auto box = static_cast<W_EDITBOX *>(widget);
	currentHudSkin().drawInputField(WzRect(static_cast<int>(xOffset) + box->x(), static_cast<int>(yOffset) + box->y(), box->width(), box->height()), box->isEditing());
}
/// (Demolish, which is listed whatever the category)
const char *const ANY_CATEGORY = "*";

struct CategoryChoice
{
	std::string id;
	WzString name;
	std::string icon;  ///< (HudChip::setIcon)
};

const char *templateCategory(const DROID_TEMPLATE *psTemplate)
{
	switch (droidTemplateType(psTemplate))
	{
	case DROID_CONSTRUCT:
	case DROID_CYBORG_CONSTRUCT:
		return "trucks";
	case DROID_SENSOR:
	case DROID_REPAIR:
	case DROID_CYBORG_REPAIR:
	case DROID_COMMAND:
		return "support";
	default:
		return "combat";
	}
}

std::vector<CategoryChoice> templateCategories()
{
	return {
		{"combat", WzString::fromUtf8(_("Combat")), "category-combat"},
		{"trucks", WzString::fromUtf8(_("Trucks")), "category-trucks"},
		{"support", WzString::fromUtf8(_("Support")), "category-support"},
	};
}

class PanelFiltersContent : public HudExtensionContent
{
public:
	static std::shared_ptr<PanelFiltersContent> make(const std::shared_ptr<StatsForm> &form, FilterPanel panel)
	{
		auto content = std::make_shared<PanelFiltersContent>();
		content->id = IDHUD_STATS_EXTENSION;
		content->form = form;
		content->panel = panel;
		content->state = hudSettings().rememberFilters ? remembered[static_cast<size_t>(panel)] : FilterState();
		content->initialize();
		return content;
	}

	HudExtensionFit fit(int panelWidth, int panelHeight, int availableHeightAbove, int availableWidthRight) override
	{
		int needed = 0;
		if (panel == FilterPanel::Research)
		{
			needed = HUD_CHIP_HEIGHT + PAD * 2;
		}
		else if (categories.size() >= 2)
		{
			needed = layoutChips(panelWidth) + PAD * 2;
		}
		compact = false;
		HudExtensionFit result;
		if (needed > 0 && needed <= availableHeightAbove)
		{
			result.top = needed;
		}
		else if (needed > 0 && availableWidthRight >= COMPACT_WIDTH)
		{
			compact = true;
			result.right = COMPACT_WIDTH;
		}
		return result;
	}

protected:
	void geometryChanged() override
	{
		const bool shown = width() > 0 && height() > 0;
		compactButton->show(shown && compact);
		for (auto &chip : chips)
		{
			chip->show(shown && !compact);
		}
		if (searchBox)
		{
			searchBox->show(shown && !compact);
			laneButton->show(shown && !compact);
		}
		if (!shown)
		{
			return;
		}
		if (compact)
		{
			compactButton->setGeometry(2, hudCloseButtonGutter(), COMPACT_WIDTH - 4, HUD_CHIP_HEIGHT);
			return;
		}
		if (searchBox)
		{
			const int laneWidth = HUD_CHIP_HEIGHT + 6;
			const int right = width() - hudCloseButtonGutter();
			searchBox->setGeometry(PAD, PAD, right - PAD - laneWidth - GAP, HUD_CHIP_HEIGHT);
			laneButton->setGeometry(right - laneWidth, PAD, laneWidth, HUD_CHIP_HEIGHT);
			return;
		}
		layoutChips(width());
	}

	void display(int xOffset, int yOffset) override
	{
		if (realTime - lastRefresh >= REFRESH_MS)
		{
			lastRefresh = realTime;
			refreshCategories();
		}
		if (searchBox && searchBox->getString() != state.search)
		{
			state.search = searchBox->getString();
			apply();
		}
	}

private:
	void initialize()
	{
		compactButton = HudChip::make(WzString::fromUtf8("▾"), _("Filter"));
		compactButton->onPressed = [this](HudChip &, bool) { openMenu(compactButton); };
		attach(compactButton);
		if (panel == FilterPanel::Research)
		{
			W_EDBINIT init;
			init.id = IDHUD_STATS_SEARCH;
			init.FontID = font_small;
			init.pBoxDisplay = displaySearchBox;
			searchBox = std::make_shared<W_EDITBOX>(&init);
			searchBox->setString(state.search);
			searchBox->setPlaceholder(WzString::fromUtf8(_("Search")));
			PIELIGHT muted = currentHudSkin().colors().textSecondary;
			muted.byte.a = 150;
			searchBox->setPlaceholderTextColor(muted);
			searchBox->setTip(_("Search by name"));
			searchBox->setMaxStringSize(40);
			searchBox->setOnEscapeHandler([](W_EDITBOX &box) {
				box.setString(WzString());
				box.stopEditing();
			});
			attach(searchBox);
			laneButton = HudChip::make(WzString::fromUtf8("▾"), _("Category"));
			laneButton->onPressed = [this](HudChip &, bool) { openMenu(laneButton); };
			attach(laneButton);
		}
		refreshCategories();
		apply();
	}

	std::vector<CategoryChoice> presentCategories()
	{
		std::vector<CategoryChoice> present;
		auto strongForm = form.lock();
		if (strongForm == nullptr)
		{
			return present;
		}
		auto &controller = strongForm->statsController();
		std::vector<CategoryChoice> all;
		if (panel == FilterPanel::Build)
		{
			for (const auto &category : hudBuildCategories())
			{
				all.push_back({category.id, category.name, category.icon});
			}
		}
		else if (panel == FilterPanel::Manufacture)
		{
			all = templateCategories();
		}
		else
		{
			std::vector<bool> visible(asResearch.size(), true);
			for (uint8_t lane : researchLanesInUse(visible))
			{
				all.push_back({std::to_string(lane), researchLaneName(lane), std::string()});
			}
		}
		std::vector<bool> used(all.size(), false);
		for (size_t i = 0; i < controller.statsSize(); ++i)
		{
			const std::string category = categoryOf(controller.getStatsAt(i));
			for (size_t c = 0; c < all.size(); ++c)
			{
				if (all[c].id == category)
				{
					used[c] = true;
				}
			}
		}
		for (size_t c = 0; c < all.size(); ++c)
		{
			if (used[c])
			{
				present.push_back(all[c]);
			}
		}
		return present;
	}

	const std::string &categoryOf(BASE_STATS *stats)
	{
		auto it = categoryCache.find(stats);
		if (it != categoryCache.end())
		{
			return it->second;
		}
		std::string category;
		if (stats != nullptr)
		{
			switch (panel)
			{
			case FilterPanel::Build:
				category = static_cast<STRUCTURE_STATS *>(stats)->type == REF_DEMOLISH ? ANY_CATEGORY : hudBuildCategoryOf(static_cast<STRUCTURE_STATS *>(stats));
				break;
			case FilterPanel::Manufacture: category = templateCategory(static_cast<DROID_TEMPLATE *>(stats)); break;
			case FilterPanel::Research: category = std::to_string(laneOfResearchIcon(static_cast<RESEARCH *>(stats)->iconID)); break;
			}
		}
		return categoryCache.emplace(stats, category).first->second;
	}

	const WzString &searchTextOf(BASE_STATS *stats)
	{
		auto it = searchCache.find(stats);
		if (it != searchCache.end())
		{
			return it->second;
		}
		WzString text = stats != nullptr ? (WzString::fromUtf8(getLocalizedStatsName(stats)) + " " + stats->id).toLower() : WzString();
		return searchCache.emplace(stats, text).first->second;
	}

	void refreshCategories()
	{
		auto present = presentCategories();
		if (panel != FilterPanel::Research && present.size() > MAX_CATEGORIES)
		{
			present.resize(MAX_CATEGORIES);
		}
		std::vector<std::string> ids;
		for (const auto &category : present)
		{
			ids.push_back(category.id);
		}
		if (ids == categoryIds)
		{
			return;
		}
		categoryIds = ids;
		categories = present;

		const std::string &selected = panel == FilterPanel::Research ? laneId() : state.category;
		if (!selected.empty() && std::find(ids.begin(), ids.end(), selected) == ids.end())
		{
			state.category.clear();
			state.lane.reset();
			apply();
		}
		if (panel != FilterPanel::Research)
		{
			rebuildChips();
		}
		hudRelayoutPanelOf(*this);
	}

	std::string laneId() const
	{
		return state.lane.has_value() ? std::to_string(state.lane.value()) : std::string();
	}

	void rebuildChips()
	{
		for (auto &chip : chips)
		{
			detach(chip);
		}
		chips.clear();
		if (categories.size() < 2)
		{
			return;
		}
		auto all = HudChip::make(WzString::fromUtf8(_("All")), _("Show everything"));
		all->setIcon("category-all");
		all->onPressed = [this](HudChip &, bool) { selectCategory(std::string()); };
		chips.push_back(all);
		for (const auto &category : categories)
		{
			auto chip = HudChip::make(category.name, category.icon.empty() ? std::string() : category.name.toUtf8());
			chip->setIcon(category.icon);
			const std::string categoryId = category.id;
			chip->onPressed = [this, categoryId](HudChip &, bool) { selectCategory(state.category == categoryId ? std::string() : categoryId); };
			chips.push_back(chip);
		}
		for (auto &chip : chips)
		{
			attach(chip);
		}
		updateChipStates();
	}

	void updateChipStates()
	{
		for (size_t i = 0; i < chips.size(); ++i)
		{
			chips[i]->setActive(i == 0 ? state.category.empty() : categories[i - 1].id == state.category);
		}
		if (laneButton)
		{
			laneButton->setActive(state.lane.has_value());
		}
		compactButton->setActive(!state.isDefault());
	}

	int layoutChips(int panelWidth)
	{
		std::vector<std::shared_ptr<WIDGET>> widgets(chips.begin(), chips.end());
		return hudFlowLayout(widgets, PAD, PAD, panelWidth - PAD - hudCloseButtonGutter(), GAP, HUD_CHIP_HEIGHT);
	}

	void selectCategory(const std::string &category)
	{
		if (panel == FilterPanel::Research)
		{
			state.lane = category.empty() ? optional<uint8_t>() : optional<uint8_t>(static_cast<uint8_t>(std::stoi(category)));
		}
		else
		{
			state.category = category;
		}
		apply();
	}

	void openMenu(const std::shared_ptr<WIDGET> &anchor)
	{
		auto menu = PopoverMenuWidget::make();
		std::weak_ptr<PanelFiltersContent> weakSelf = std::static_pointer_cast<PanelFiltersContent>(shared_from_this());
		auto addItem = [weakSelf, &menu](const WzString &text, const std::string &category, bool current) {
			auto item = WzClickableOptionsChoiceWidget::make(font_small);
			item->setString(current ? WzString::fromUtf8("✓ ") + text : text);
			item->addOnClickHandler([weakSelf, category](WzClickableOptionsChoiceWidget &, WIDGET_KEY) {
				if (auto self = weakSelf.lock())
				{
					self->selectCategory(category);
				}
			});
			item->setGeometry(0, 0, item->idealWidth(), item->idealHeight());
			menu->addMenuItem(item, true);
		};
		const std::string current = panel == FilterPanel::Research ? laneId() : state.category;
		addItem(WzString::fromUtf8(panel == FilterPanel::Research ? _("All categories") : _("All")), std::string(), current.empty());
		for (const auto &category : categories)
		{
			addItem(category.name, category.id, category.id == current);
		}
		hudOpenMenu(menu, anchor);
	}

	void apply()
	{
		remembered[static_cast<size_t>(panel)] = state;
		updateChipStates();
		auto strongForm = form.lock();
		if (strongForm == nullptr)
		{
			return;
		}
		if (state.isDefault())
		{
			strongForm->setOptionFilter(nullptr);
			return;
		}
		const std::string category = panel == FilterPanel::Research ? laneId() : state.category;
		const WzString needle = state.search.toLower();
		strongForm->setOptionFilter([this, category, needle](BASE_STATS *stats) {
			if (!category.empty() && categoryOf(stats) != category && categoryOf(stats) != ANY_CATEGORY)
			{
				return false;
			}
			return needle.isEmpty() || searchTextOf(stats).contains(needle);
		});
	}

	std::weak_ptr<StatsForm> form;
	FilterPanel panel = FilterPanel::Build;
	FilterState state;
	bool compact = false;
	uint32_t lastRefresh = 0;
	std::vector<CategoryChoice> categories;
	std::vector<std::string> categoryIds;
	std::vector<std::shared_ptr<HudChip>> chips;
	std::shared_ptr<HudChip> compactButton;
	std::shared_ptr<W_EDITBOX> searchBox;
	std::shared_ptr<HudChip> laneButton;
	std::unordered_map<BASE_STATS *, std::string> categoryCache;
	std::unordered_map<BASE_STATS *, WzString> searchCache;
};

}

std::shared_ptr<HudExtensionContent> hudMakePanelFilters(const std::shared_ptr<StatsForm> &form)
{
	if (form == nullptr)
	{
		return nullptr;
	}
	auto &controller = form->statsController();
	if (dynamic_cast<BuildController *>(&controller) != nullptr)
	{
		return PanelFiltersContent::make(form, FilterPanel::Build);
	}
	if (dynamic_cast<ManufactureController *>(&controller) != nullptr)
	{
		return PanelFiltersContent::make(form, FilterPanel::Manufacture);
	}
	if (dynamic_cast<ResearchController *>(&controller) != nullptr)
	{
		return PanelFiltersContent::make(form, FilterPanel::Research);
	}
	return nullptr;
}

void hudResetPanelFilters()
{
	remembered = {};
}

bool hudFocusPanelSearch()
{
	auto searchBox = dynamic_cast<W_EDITBOX *>(widgGetFromID(psWScreen, IDHUD_STATS_SEARCH));
	if (searchBox == nullptr || !searchBox->visible())
	{
		return false;
	}
	std::weak_ptr<W_EDITBOX> weakSearchBox = std::static_pointer_cast<W_EDITBOX>(searchBox->shared_from_this());
	widgScheduleTask([weakSearchBox]() {
		if (auto strongSearchBox = weakSearchBox.lock())
		{
			W_CONTEXT context = W_CONTEXT::ZeroContext();
			strongSearchBox->simulateClick(&context, true);
		}
	});
	return true;
}
