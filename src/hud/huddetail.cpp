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
#include "hudlayout.h"
#include "hudsettings.h"
#include "hudskin.h"

#include "lib/framework/frame.h"
#include "lib/framework/input.h"
#include "lib/framework/wzapp.h"
#include "lib/ivis_opengl/pieclip.h"
#include "lib/widget/widget.h"
#include "lib/widget/form.h"
#include "lib/widget/button.h"
#include "lib/ivis_opengl/pieblitfunc.h"
#include "lib/ivis_opengl/textdraw.h"

#include "../hci.h"
#include "../loop.h"
#include "../main.h"
#include "../widgets/detailpanel/detailpanel.h"

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

/// A long-press's popover: opened while the finger is still down, pinned once it lifts
enum class Peek
{
	None,
	Held,
	Pinned,
};
Peek peek = Peek::None;
std::weak_ptr<WIDGET> peekRequested;
std::shared_ptr<WIDGET> peekRequestedContents;

constexpr int ACTION_HEIGHT = 26;
constexpr int ACTION_HEIGHT_LARGER_TARGETS = 36;
constexpr int ACTION_GAP = 6;
constexpr int ACTION_PAD = 9;
constexpr int ACTION_ICON = 13;

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
	std::vector<HudDetailAction> hudDetailActions() override { return provider != nullptr && provider->actions ? provider->actions() : std::vector<HudDetailAction>(); }
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

PIELIGHT scaledTone(PIELIGHT color)
{
	color.byte.a = static_cast<uint8_t>(color.byte.a * 55 / 100);
	return color;
}

/// One of a pinned popover's actions
class PeekActionButton : public W_BUTTON
{
public:
	explicit PeekActionButton(HudDetailAction action_) : action(std::move(action_))
	{
		labelText.setText(action.label, font_small);
		if (!action.note.isEmpty())
		{
			noteText.setText(action.note, font_small);
		}
		if (!action.enabled)
		{
			setState(WBUT_DISABLE);
		}
		addOnClickHandler([](W_BUTTON &button) {
			auto run = static_cast<PeekActionButton &>(button).action.run;
			widgScheduleTask([run]() {
				if (run)
				{
					run();
				}
				hudDetailClose();
			});
		});
	}

	int32_t idealWidth() override
	{
		const int icon = action.icon.empty() ? 0 : ACTION_ICON + 7;
		const int note = action.note.isEmpty() ? 0 : 8 + noteText.width();
		return ACTION_PAD + icon + labelText.fit().naturalWidth + note + ACTION_PAD;
	}

	void display(int xOffset, int yOffset) override
	{
		HudSkin &skin = currentHudSkin();
		const HudColors &colors = skin.colors();
		const int x0 = xOffset + x();
		const int y0 = yOffset + y();
		const bool down = (getState() & WBUT_DOWN) != 0;
		PIELIGHT tone = action.destructive ? colors.stateBad : colors.accent;
		if (!action.enabled || peek == Peek::Held)
		{
			// (while the finger is still down, the actions show what lifting it offers)
			tone = action.enabled ? scaledTone(tone) : colors.stateDisabled;
		}
		PIELIGHT fill = tone;
		fill.byte.a = down ? 80 : 36;
		pie_DrawRoundedBox(static_cast<float>(x0), static_cast<float>(y0), static_cast<float>(x0 + width()), static_cast<float>(y0 + height()), fill, 5.f);
		int tx = x0 + ACTION_PAD;
		if (!action.icon.empty() && skin.drawNamedIcon(action.icon, WzRect(tx, y0 + (height() - ACTION_ICON) / 2, ACTION_ICON, ACTION_ICON), tone))
		{
			tx += ACTION_ICON + 7;
		}
		const int right = x0 + width() - ACTION_PAD;
		const int noteWidth = action.note.isEmpty() ? 0 : noteText.width();
		const int baseline = y0 + (height() - labelText.lineSize()) / 2 - labelText.aboveBase();
		labelText.setText(labelText.getText(), labelText.getFontID(), std::max(10, right - tx - (noteWidth > 0 ? noteWidth + 8 : 0)));
		labelText.render(static_cast<float>(tx), static_cast<float>(baseline), tone);
		if (noteWidth > 0)
		{
			noteText.render(static_cast<float>(right - noteWidth), static_cast<float>(baseline), colors.textSecondary);
		}
	}

	HudDetailAction action;

private:
	WzText labelText;
	WzText noteText;
};

/// The actions side by side: the last takes the room the others leave (or all share it, when it's short)
class PeekActionRow : public WIDGET
{
public:
	int32_t idealHeight() override { return hudLargerTouchTargets() ? ACTION_HEIGHT_LARGER_TARGETS : ACTION_HEIGHT; }

	void geometryChanged() override
	{
		const auto &buttons = children();
		if (buttons.empty())
		{
			return;
		}
		const int count = static_cast<int>(buttons.size());
		const int gaps = (count - 1) * ACTION_GAP;
		int wanted = gaps;
		for (const auto &button : buttons)
		{
			wanted += button->idealWidth();
		}
		int at = 0;
		for (int i = 0; i < count; ++i)
		{
			int w = (width() - gaps) / count;
			if (wanted <= width())
			{
				w = i + 1 < count ? buttons[i]->idealWidth() : width() - at;
			}
			buttons[i]->setGeometry(at, 0, w, height());
			at += w + ACTION_GAP;
		}
	}
};

class PeekCloseButton : public W_BUTTON
{
public:
	PeekCloseButton()
	{
		addOnClickHandler([](W_BUTTON &) {
			widgScheduleTask([]() { hudDetailClose(); });
		});
	}
	int32_t idealWidth() override { return 18; }
	int32_t idealHeight() override { return 16; }
	void display(int xOffset, int yOffset) override
	{
		currentHudSkin().drawCloseButton(xOffset + x(), yOffset + y(), width(), height(), (getState() & WBUT_DOWN) != 0);
	}
};

/// A long-press's popover: the element's actions along the bottom, and a close button
void dressForPeek(const std::shared_ptr<WIDGET> &made, HudDetailSource &source)
{
	auto panel = std::dynamic_pointer_cast<DetailPanelContents>(made);
	if (panel == nullptr)
	{
		return;
	}
	auto actions = source.hudDetailActions();
	if (!actions.empty())
	{
		auto row = std::make_shared<PeekActionRow>();
		for (auto &action : actions)
		{
			row->attach(std::make_shared<PeekActionButton>(std::move(action)));
		}
		panel->setActions(row);
	}
	panel->setCorner(std::make_shared<PeekCloseButton>());
}

bool attachContents(const std::shared_ptr<WIDGET> &widget, HudDetailSource &source, std::shared_ptr<WIDGET> newContents)
{
	if (newContents == nullptr)
	{
		return false;
	}
	if (peek != Peek::None)
	{
		dressForPeek(newContents, source);
	}
	ensureOverlay();
	if (contents != nullptr)
	{
		overlayRoot->detach(contents);
	}
	contents = newContents;
	contents->setTransparentToMouse(peek != Peek::Pinned);
	overlayRoot->attach(contents);
	const WzRect rect = place(besideRect(*widget, source.hudDetailPanelId(), source.hudDetailPlacement()), contents->width(), contents->height(), source.hudDetailPlacement());
	contents->move(rect.x(), rect.y());
	anchor = widget;
	builtAt = realTime;
	return true;
}

bool show(const std::shared_ptr<WIDGET> &widget)
{
	auto source = sourceOf(widget.get());
	return source != nullptr && attachContents(widget, *source, source->makeHudDetail());
}

void closeLater()
{
	widgScheduleTask([]() { hudDetailClose(); });
}

/// The finger lifted: the popover stays, and takes input (a press anywhere else only closes it)
void pin()
{
	peek = Peek::Pinned;
	contents->setTransparentToMouse(false);
	overlayRoot->setTransparentToMouse(false);
	overlayRoot->setCustomHitTest(nullptr);
	overlayRoot->onClickedFunc = closeLater;
	overlayRoot->onCancelPressed = []() {
		inputLoseFocus();
		closeLater();
	};
}

/// A long-press took the press (from the widget library): its popover opens on the next update
bool requestPeek(WIDGET &widget)
{
	if (!usable() || hudSettings().skin != HudSkinId::Modern)
	{
		return false;
	}
	auto described = describedAt(widget.shared_from_this());
	auto source = sourceOf(described.get());
	if (source == nullptr || !source->hasHudDetail())
	{
		return false;
	}
	auto made = source->makeHudDetailPeek();
	if (made == nullptr)
	{
		return false;
	}
	peekRequested = described;
	peekRequestedContents = made;
	return true;
}

void updatePeek()
{
	auto current = anchor.lock();
	if (contents == nullptr || current == nullptr || !current->visible() || current->screenPointer.lock() == nullptr)
	{
		hudDetailClose();
		return;
	}
	if (peek == Peek::Held)
	{
		if (!mouseDown(MOUSE_LMB) || mouseReleased(MOUSE_LMB))
		{
			pin();
		}
		else if (!current->screenGeometry().contains(mouseX(), mouseY()))
		{
			hudDetailClose();
		}
		return;
	}
	if (!mouseDown(MOUSE_LMB) && realTime - builtAt >= REFRESH_MS)
	{
		auto source = sourceOf(current.get());
		if (source == nullptr || !attachContents(current, *source, source->makeHudDetailPeek()))
		{
			hudDetailClose();
		}
	}
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
	peek = Peek::None;
}

bool hudDetailShowing()
{
	return contents != nullptr;
}

bool hudDetailPeeking()
{
	return peek != Peek::None;
}

bool hudDetailPinned()
{
	return peek == Peek::Pinned;
}

std::vector<WzRect> hudDetailPeekActionRectsForTests()
{
	std::vector<WzRect> rects;
	std::function<void (const std::shared_ptr<WIDGET> &)> visit = [&](const std::shared_ptr<WIDGET> &widget) {
		if (dynamic_cast<PeekActionButton *>(widget.get()) != nullptr)
		{
			rects.push_back(widget->screenGeometry());
		}
		for (const auto &child : widget->children())
		{
			visit(child);
		}
	};
	if (contents != nullptr)
	{
		visit(contents);
	}
	return rects;
}

int hudDetailWidth()
{
	return std::max(220, std::min(300, static_cast<int>(pie_GetVideoBufferWidth()) / 3));
}

int hudDetailMaxHeight()
{
	return static_cast<int>(pie_GetVideoBufferHeight()) * 55 / 100;
}

bool hudDetailSmall()
{
	return hudCompactLayout();
}

void hudDetailUpdate()
{
	static bool installed = false;
	if (!installed)
	{
		installed = true;
		widgSetTipSuppressed([](const WIDGET &widget) {
			if (peek != Peek::None)
			{
				return true;
			}
			if (!usable() || !detailWanted())
			{
				return false;
			}
			auto described = describedAt(std::const_pointer_cast<WIDGET>(widget.shared_from_this()));
			auto source = sourceOf(described.get());
			return source != nullptr && source->hasHudDetail();
		});
		widgSetTouchLongPress([]() { return std::chrono::milliseconds(hudSettings().longPressDelay); }, requestPeek);
	}

	if (!usable())
	{
		hudDetailClose();
		restingOn.reset();
		peekRequested.reset();
		peekRequestedContents.reset();
		return;
	}
	if (auto requested = peekRequested.lock())
	{
		hudDetailClose();
		peek = Peek::Held;
		auto source = sourceOf(requested.get());
		if (source == nullptr || !attachContents(requested, *source, peekRequestedContents))
		{
			hudDetailClose();
		}
		dismissedUntilMoved = false;
	}
	peekRequested.reset();
	peekRequestedContents.reset();
	if (peek != Peek::None)
	{
		updatePeek();
		return;
	}
	// (with touch, nothing rests on anything: a long-press shows the popover instead)
	if (!detailWanted() || inputLastPointerInputWasTouch())
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
