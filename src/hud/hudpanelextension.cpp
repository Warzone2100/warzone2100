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

#include "hudpanelextension.h"

#include "lib/framework/frame.h"
#include "lib/ivis_opengl/pieclip.h"

#include "hudskin.h"

#include "../hci.h"

#include <algorithm>
#include <vector>

constexpr int CLOSE_INSET = 3;

static std::vector<std::weak_ptr<WIDGET>> movedCloseButtons;
static std::vector<std::weak_ptr<WIDGET>> movedPanelControls;
static std::vector<std::weak_ptr<WIDGET>> skinPlacedWidgets;
static std::vector<std::weak_ptr<WIDGET>> replacedPageTabs;
static std::vector<std::pair<std::weak_ptr<WIDGET>, std::function<int ()>>> panelTopInsets;

static bool containsWeak(const std::vector<std::weak_ptr<WIDGET>> &list, const WIDGET &widget)
{
	return std::any_of(list.begin(), list.end(), [&widget](const std::weak_ptr<WIDGET> &entry) {
		return entry.lock().get() == &widget;
	});
}

static void addWeak(std::vector<std::weak_ptr<WIDGET>> &list, const std::shared_ptr<WIDGET> &widget)
{
	list.erase(std::remove_if(list.begin(), list.end(), [](const std::weak_ptr<WIDGET> &entry) { return entry.expired(); }), list.end());
	if (widget != nullptr && !containsWeak(list, *widget))
	{
		list.push_back(widget);
	}
}

void hudExtendPanel(const std::shared_ptr<WIDGET> &panel, const std::shared_ptr<HudExtensionContent> &content, const HudPanelExtensionSpec &spec)
{
	ASSERT_OR_RETURN(, panel != nullptr && spec.classicRect, "Invalid panel extension");
	auto anchor = std::make_shared<WIDGET>();
	anchor->id = spec.contentId;
	anchor->setTransparentToMouse(true);
	const std::vector<std::shared_ptr<WIDGET>> children = panel->children();
	for (const auto &child : children)
	{
		panel->detach(child);
		if (child != spec.closeButton)
		{
			anchor->attach(child);
		}
	}
	panel->attach(anchor);
	if (content != nullptr)
	{
		panel->attach(content);
	}
	if (spec.closeButton != nullptr)
	{
		spec.closeButton->setCalcLayout(nullptr);
		panel->attach(spec.closeButton);
		addWeak(movedCloseButtons, spec.closeButton);
	}
	std::weak_ptr<WIDGET> weakCloseButton = spec.closeButton;
	if (spec.topInset)
	{
		panelTopInsets.erase(std::remove_if(panelTopInsets.begin(), panelTopInsets.end(), [](const std::pair<std::weak_ptr<WIDGET>, std::function<int ()>> &entry) {
			return entry.first.expired();
		}), panelTopInsets.end());
		panelTopInsets.emplace_back(panel, spec.topInset);
	}

	std::weak_ptr<WIDGET> weakAnchor = anchor;
	std::weak_ptr<HudExtensionContent> weakContent = content;
	const HudPanelExtensionSpec layoutSpec = spec;
	panel->setCalcLayout([weakAnchor, weakContent, weakCloseButton, layoutSpec](WIDGET *psWidget) {
		auto anchor = weakAnchor.lock();
		auto content = weakContent.lock();
		const WzRect full = layoutSpec.classicRect();
		const int inset = layoutSpec.topInset ? std::max(0, std::min(layoutSpec.topInset(), full.height())) : 0;
		const WzRect classic(full.x(), full.y() + inset, full.width(), full.height() - inset);
		const int above = layoutSpec.availableHeightAbove ? std::max(0, layoutSpec.availableHeightAbove(classic)) : 0;
		const int right = layoutSpec.availableWidthRight ? std::max(0, layoutSpec.availableWidthRight(classic)) : 0;
		HudExtensionFit fit;
		if (content != nullptr)
		{
			fit = content->fit(classic.width(), classic.height(), above + layoutSpec.bandOverlap, right);
			fit.top = std::max(0, std::min(fit.top, above + layoutSpec.bandOverlap));
			fit.right = std::max(0, std::min(fit.right, right));
		}
		// Without a band of its own above the contents, the panel still grows by what its frame takes at the top
		const int band = fit.top;
		fit.top = std::max(0, band - layoutSpec.bandOverlap);
		if (band == 0)
		{
			fit.top = std::min(std::max(0, currentHudSkin().panelTopPadding()), above);
		}
		psWidget->setGeometry(classic.x(), classic.y() - fit.top, classic.width() + fit.right, classic.height() + fit.top);
		if (anchor != nullptr)
		{
			anchor->setGeometry(0, fit.top - inset, full.width(), full.height());
		}
		if (content != nullptr)
		{
			if (band > 0)
			{
				content->setGeometry(0, 0, classic.width() + fit.right, band);
			}
			else
			{
				content->setGeometry(classic.width(), fit.top, fit.right, fit.right > 0 ? classic.height() : 0);
			}
		}
		if (auto closeButton = weakCloseButton.lock())
		{
			const int width = classic.width() + fit.right;
			if (band > 0)
			{
				closeButton->setGeometry(width - CLOSE_WIDTH - CLOSE_INSET, std::max(0, std::min((band - CLOSE_HEIGHT) / 2, CLOSE_INSET + 1)), CLOSE_WIDTH, CLOSE_HEIGHT);
			}
			else if (fit.right > 0)
			{
				closeButton->setGeometry(width - (fit.right + CLOSE_WIDTH) / 2, fit.top + CLOSE_INSET, CLOSE_WIDTH, CLOSE_HEIGHT);
			}
			else
			{
				closeButton->setGeometry(classic.width() - CLOSE_WIDTH, fit.top, CLOSE_WIDTH, CLOSE_HEIGHT);
			}
		}
		if (layoutSpec.afterLayout)
		{
			layoutSpec.afterLayout();
		}
	});
}

int hudCloseButtonGutter()
{
	return CLOSE_WIDTH + CLOSE_INSET * 2;
}

bool hudIsMovedCloseButton(const WIDGET &widget)
{
	return containsWeak(movedCloseButtons, widget);
}

void hudSetMovedPanelControl(const std::shared_ptr<WIDGET> &widget)
{
	addWeak(movedPanelControls, widget);
}

bool hudIsMovedPanelControl(const WIDGET &widget)
{
	return containsWeak(movedPanelControls, widget);
}

void hudSetSkinPlacedWidget(const std::shared_ptr<WIDGET> &widget)
{
	addWeak(skinPlacedWidgets, widget);
}

bool hudIsSkinPlacedWidget(const WIDGET &widget)
{
	return containsWeak(skinPlacedWidgets, widget);
}

int hudPanelTopInset(const WIDGET &panel)
{
	for (const auto &entry : panelTopInsets)
	{
		if (entry.first.lock().get() == &panel)
		{
			return std::max(0, entry.second());
		}
	}
	return 0;
}

void hudSetReplacedPageTabs(const std::shared_ptr<WIDGET> &tabs)
{
	addWeak(replacedPageTabs, tabs);
}

bool hudIsReplacedPageTabs(const WIDGET &widget)
{
	return containsWeak(replacedPageTabs, widget);
}

void hudRelayoutPanelOf(WIDGET &content)
{
	if (auto panel = content.parent())
	{
		panel->callCalcLayout();
	}
}

int hudTopBand()
{
	return std::min<int>(96, pie_GetVideoBufferHeight() / 6);
}
