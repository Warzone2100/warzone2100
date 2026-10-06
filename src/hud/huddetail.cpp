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

#include "huddetail.h"
#include "hudsettings.h"

#include "lib/framework/frame.h"
#include "lib/framework/input.h"
#include "lib/framework/wzapp.h"
#include "lib/ivis_opengl/pieclip.h"
#include "lib/widget/widget.h"
#include "lib/widget/form.h"

#include "../hci.h"
#include "../loop.h"
#include "../main.h"

#include <algorithm>
#include <vector>

namespace
{

constexpr uint32_t LEAVE_GRACE_MS = 150;
constexpr uint32_t REFRESH_MS = 500;
constexpr int GAP = 6;

std::shared_ptr<W_SCREEN> overlayScreen;
std::shared_ptr<W_FULLSCREENOVERLAY_CLICKFORM> overlayRoot;
std::shared_ptr<WIDGET> contents;
std::weak_ptr<WIDGET> anchor;
std::weak_ptr<WIDGET> restingOn;
uint32_t restingSince = 0;
uint32_t leftAt = 0;
uint32_t builtAt = 0;
bool dismissedUntilMoved = false;

void displayNothing(WIDGET *, UDWORD, UDWORD)
{
}

bool detailWanted()
{
	return hudDetailedTooltips() || (hudSettings().tooltips == HudTooltips::Simple && (keyDown(KEY_LSHIFT) || keyDown(KEY_RSHIFT)));
}

bool usable()
{
	return GetGameMode() == GS_NORMAL && psWScreen != nullptr && getWidgetsStatus() && !intHelpOverlayIsUp();
}

std::vector<std::pair<std::weak_ptr<WIDGET>, HudDetailProvider>> registered;

class RegisteredSource : public HudDetailSource
{
public:
	std::shared_ptr<WIDGET> makeHudDetail() override { return provider != nullptr && provider->make ? provider->make() : nullptr; }
	HudDetailPlacement hudDetailPlacement() const override { return provider != nullptr ? provider->placement : HudDetailPlacement::Above; }
	uint32_t hudDetailPanelId() const override { return provider != nullptr ? provider->panelId : 0; }
	const HudDetailProvider *provider = nullptr;
};

HudDetailSource *sourceOf(const WIDGET *widget)
{
	if (widget == nullptr)
	{
		return nullptr;
	}
	if (auto source = dynamic_cast<HudDetailSource *>(const_cast<WIDGET *>(widget)))
	{
		return source;
	}
	static RegisteredSource adapter;
	for (const auto &entry : registered)
	{
		if (entry.first.lock().get() == widget)
		{
			adapter.provider = &entry.second;
			return &adapter;
		}
	}
	return nullptr;
}

std::shared_ptr<WIDGET> describedAt(std::shared_ptr<WIDGET> widget)
{
	for (int depth = 0; widget != nullptr && depth < 4; ++depth, widget = widget->parent())
	{
		if (sourceOf(widget.get()) != nullptr)
		{
			return widget;
		}
	}
	return nullptr;
}

void ensureOverlay()
{
	if (overlayScreen != nullptr)
	{
		return;
	}
	overlayScreen = W_SCREEN::make();
	overlayScreen->psForm->hide();
	overlayRoot = W_FULLSCREENOVERLAY_CLICKFORM::make();
	overlayRoot->displayFunction = displayNothing;
	overlayRoot->backgroundColor = pal_RGBA(0, 0, 0, 0);
	overlayRoot->setTransparentToMouse(true);
	overlayRoot->setCustomHitTest([](const WIDGET *, int, int) -> bool {
		return false;
	});
	overlayRoot->swallowsInput = false;
	overlayRoot->onClickedFunc = nullptr;
	overlayRoot->onCancelPressed = nullptr;
	overlayScreen->psForm->attach(overlayRoot);
	widgRegisterOverlayScreenOnTopOfScreen(overlayScreen, psWScreen);
}

WzRect place(const WzRect &around, int w, int h, HudDetailPlacement placement)
{
	const int screenW = static_cast<int>(pie_GetVideoBufferWidth());
	const int screenH = static_cast<int>(pie_GetVideoBufferHeight());
	auto clampX = [screenW, w](int x) { return std::max(0, std::min(x, screenW - w)); };
	auto clampY = [screenH, h](int y) { return std::max(0, std::min(y, screenH - h)); };
	auto overlaps = [&around](const WzRect &r) {
		return r.x() < around.right() && around.x() < r.right() && r.y() < around.bottom() && around.y() < r.bottom();
	};

	std::vector<WzRect> candidates;
	const WzRect right(around.right() + GAP, clampY(around.y()), w, h);
	const WzRect left(around.x() - GAP - w, clampY(around.y()), w, h);
	const WzRect above(clampX(around.x() + around.width() / 2 - w / 2), around.y() - GAP - h, w, h);
	const WzRect below(clampX(around.x() + around.width() / 2 - w / 2), around.bottom() + GAP, w, h);
	if (placement == HudDetailPlacement::Right)
	{
		candidates = {right, left, above, below};
	}
	else
	{
		candidates = {above, below, right, left};
	}
	for (const auto &candidate : candidates)
	{
		if (candidate.x() >= 0 && candidate.y() >= 0 && candidate.right() <= screenW && candidate.bottom() <= screenH && !overlaps(candidate))
		{
			return candidate;
		}
	}
	const WzRect fallback = candidates.front();
	return WzRect(clampX(fallback.x()), clampY(fallback.y()), w, h);
}

WzRect besideRect(const WIDGET &widget, uint32_t panelId, HudDetailPlacement placement)
{
	const WzRect own = widget.screenGeometry();
	if (panelId != 0)
	{
		for (auto ancestor = widget.parent(); ancestor != nullptr; ancestor = ancestor->parent())
		{
			if (ancestor->id == panelId)
			{
				const WzRect panel = ancestor->screenGeometry();
				return placement == HudDetailPlacement::Right ? WzRect(panel.x(), own.y(), panel.width(), own.height()) : panel;
			}
		}
	}
	return own;
}

bool show(const std::shared_ptr<WIDGET> &widget)
{
	auto source = sourceOf(widget.get());
	if (source == nullptr)
	{
		return false;
	}
	auto newContents = source->makeHudDetail();
	if (newContents == nullptr)
	{
		return false;
	}
	ensureOverlay();
	if (contents != nullptr)
	{
		overlayRoot->detach(contents);
	}
	contents = newContents;
	contents->setTransparentToMouse(true);
	overlayRoot->attach(contents);
	const WzRect rect = place(besideRect(*widget, source->hudDetailPanelId(), source->hudDetailPlacement()), contents->width(), contents->height(), source->hudDetailPlacement());
	contents->move(rect.x(), rect.y());
	anchor = widget;
	builtAt = realTime;
	return true;
}

}

void hudDetailRegister(const std::shared_ptr<WIDGET> &widget, const HudDetailProvider &provider)
{
	registered.erase(std::remove_if(registered.begin(), registered.end(), [&widget](const std::pair<std::weak_ptr<WIDGET>, HudDetailProvider> &entry) {
		auto locked = entry.first.lock();
		return locked == nullptr || locked == widget;
	}), registered.end());
	registered.emplace_back(widget, provider);
}

void hudDetailClose()
{
	if (overlayScreen != nullptr)
	{
		widgRemoveOverlayScreen(overlayScreen);
	}
	overlayScreen.reset();
	overlayRoot.reset();
	contents.reset();
	anchor.reset();
}

bool hudDetailShowing()
{
	return contents != nullptr;
}

int hudDetailWidth()
{
	return std::max(220, std::min(300, static_cast<int>(pie_GetVideoBufferWidth()) / 3));
}

int hudDetailMaxHeight()
{
	return static_cast<int>(pie_GetVideoBufferHeight()) * 55 / 100;
}

void hudDetailUpdate()
{
	static bool installed = false;
	if (!installed)
	{
		installed = true;
		widgSetTipSuppressed([](const WIDGET &widget) {
			if (!usable() || !detailWanted())
			{
				return false;
			}
			auto described = describedAt(std::const_pointer_cast<WIDGET>(widget.shared_from_this()));
			auto source = sourceOf(described.get());
			return source != nullptr && source->hasHudDetail();
		});
	}

	if (!usable() || !detailWanted())
	{
		hudDetailClose();
		restingOn.reset();
		return;
	}

	auto over = describedAt(getMouseOverWidget().lock());
	auto source = sourceOf(over.get());
	if (mousePressed(MOUSE_LMB) || mousePressed(MOUSE_RMB))
	{
		hudDetailClose();
		dismissedUntilMoved = true;
		restingOn = over;
		return;
	}
	if (dismissedUntilMoved)
	{
		if (over == restingOn.lock())
		{
			return;
		}
		dismissedUntilMoved = false;
	}

	auto current = anchor.lock();
	if (contents != nullptr && (current == nullptr || !current->visible() || current->screenPointer.lock() == nullptr))
	{
		hudDetailClose();
		current.reset();
	}

	if (source == nullptr || !source->hasHudDetail())
	{
		restingOn.reset();
		if (contents != nullptr)
		{
			if (leftAt == 0)
			{
				leftAt = realTime;
			}
			if (realTime - leftAt >= LEAVE_GRACE_MS)
			{
				hudDetailClose();
				leftAt = 0;
			}
		}
		return;
	}
	leftAt = 0;

	if (contents != nullptr)
	{
		if (over != current)
		{
			if (!show(over))
			{
				hudDetailClose();
			}
		}
		else if (realTime - builtAt >= REFRESH_MS)
		{
			show(over);
		}
		return;
	}

	if (restingOn.lock() != over)
	{
		restingOn = over;
		restingSince = realTime;
		return;
	}
	if (realTime - restingSince >= hudSettings().tooltipDelay)
	{
		show(over);
	}
}
