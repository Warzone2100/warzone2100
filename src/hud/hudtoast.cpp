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

#include "hudtoast.h"
#include "hudids.h"
#include "hudsettings.h"
#include "hudskin.h"
#include "hudspectator.h"
#include "hudwidgets.h"

#include "lib/framework/frame.h"
#include "lib/framework/input.h"
#include "lib/gamelib/gtime.h"
#include "lib/ivis_opengl/pieblitfunc.h"
#include "lib/ivis_opengl/screen.h"
#include "lib/ivis_opengl/textdraw.h"
#include "lib/widget/button.h"
#include "lib/widget/form.h"
#include "lib/widget/widget.h"

#include "../hci.h"

#include <algorithm>
#include <deque>

namespace
{

constexpr uint32_t SHOW_MS = 6000;
constexpr uint32_t SLIDE_MS = 180;
constexpr int WIDTH = 400;
constexpr int HEIGHT = 48;
constexpr int WIDTH_COMPACT = 300;
constexpr int HEIGHT_COMPACT = 36;
constexpr int COMPACT_BELOW_WIDTH = 960;
constexpr int COMPACT_BELOW_HEIGHT = 600;
constexpr int TOP_GAP = 8;

struct Pending
{
	HudToast toast;
	size_t count = 1;
};

std::deque<Pending> waiting;

bool compactLayout()
{
	return screenWidth < COMPACT_BELOW_WIDTH || screenHeight < COMPACT_BELOW_HEIGHT;
}

PIELIGHT withAlpha(PIELIGHT color, uint8_t a)
{
	color.byte.a = a;
	return color;
}

std::string titleOf(const Pending &pending)
{
	return pending.count > 1 && pending.toast.groupTitle ? pending.toast.groupTitle(pending.count) : pending.toast.title;
}

/// The action's button: its label on the accent's outline
class ToastAction : public W_BUTTON
{
public:
	std::string label;

	int32_t idealWidth() override
	{
		return static_cast<int>(iV_GetTextWidth(WzString::fromUtf8(label), font_small)) + 24;
	}

protected:
	void display(int xOffset, int yOffset) override
	{
		HudSkin &skin = currentHudSkin();
		const HudColors &colors = skin.colors();
		const WzRect rect(xOffset + x(), yOffset + y(), width(), height());
		pie_DrawRoundedBox(static_cast<float>(rect.x()), static_cast<float>(rect.y()), static_cast<float>(rect.right()), static_cast<float>(rect.bottom()),
		                   withAlpha(colors.accent, isHighlighted() ? 110 : 60), skin.metrics().cardRadius, colors.accent, 1.f);
		text.setText(WzString::fromUtf8(label), font_small);
		const float base = static_cast<float>(rect.y()) + (static_cast<float>(rect.height()) - text.lineSize()) / 2.f - text.aboveBase();
		text.render(static_cast<float>(rect.x() + (rect.width() - text.width()) / 2), std::round(base), colors.textPrimary);
	}

private:
	WzText text;
};

/// The close button: a cross
class ToastClose : public W_BUTTON
{
protected:
	void display(int xOffset, int yOffset) override
	{
		HudSkin &skin = currentHudSkin();
		const WzRect rect(xOffset + x(), yOffset + y(), width(), height());
		if (isHighlighted())
		{
			skin.drawCard(rect, HudCardState::Highlighted);
		}
		const int s = std::min(rect.width(), rect.height()) - 6;
		skin.drawNamedIcon("never", WzRect(rect.x() + (rect.width() - s) / 2, rect.y() + (rect.height() - s) / 2, s, s), skin.colors().textSecondary);
	}
};

class ToastWidget : public HudZoneForm
{
public:
	void initialize()
	{
		// (So it takes the clicks over it)
		style &= ~WFORM_INVISIBLE;
		setTransparentToMouse(false);
		action = std::make_shared<ToastAction>();
		action->addOnClickHandler([](W_BUTTON &button) {
			auto *self = static_cast<ToastWidget *>(button.parent().get());
			auto onAction = self->current.toast.onAction;
			self->next();
			if (onAction)
			{
				widgScheduleTask(onAction);
			}
		});
		attach(action);
		close = std::make_shared<ToastClose>();
		close->setTip(_("Close"));
		close->addOnClickHandler([](W_BUTTON &button) {
			static_cast<ToastWidget *>(button.parent().get())->next();
		});
		attach(close);
	}

	/// Shows `pending` (or merges it with the one shown, of the same group). False when it has to wait.
	bool offer(const Pending &pending)
	{
		if (!showing)
		{
			start(pending);
			return true;
		}
		if (!leaving && !pending.toast.group.empty() && pending.toast.group == current.toast.group)
		{
			const size_t count = current.count + pending.count;
			current = pending;
			current.count = count;
			shownTime = 0;
			layoutChildren();
			return true;
		}
		return false;
	}

	void start(const Pending &pending)
	{
		current = pending;
		showing = true;
		leaving = false;
		shownTime = 0;
		lastTick = realTime;
		startedAt = realTime;
		layoutChildren();
		place();
	}

	/// Shows the next one waiting, or hides the toast when nothing is left
	void next()
	{
		if (!waiting.empty())
		{
			Pending pending = waiting.front();
			waiting.pop_front();
			start(pending);
			return;
		}
		showing = false;
		hide();
	}

	void run(W_CONTEXT *) override
	{
		if (!showing)
		{
			return;
		}
		const uint32_t now = realTime;
		const WzRect rect(screenPosX(), screenPosY(), width(), height());
		// (The time a toast shows doesn't run while the pointer is on it)
		if (!rect.contains(mouseX(), mouseY()))
		{
			shownTime += now - lastTick;
		}
		lastTick = now;
		if (!leaving && shownTime >= SHOW_MS)
		{
			leaving = true;
			leftAt = now;
		}
		if (leaving && (hudReduceMotion() || now - leftAt >= SLIDE_MS))
		{
			next();
			return;
		}
		place();
	}

	void place()
	{
		const bool compact = compactLayout();
		const int w = compact ? WIDTH_COMPACT : WIDTH;
		const int h = compact ? HEIGHT_COMPACT : HEIGHT;
		const int top = std::max(TOP_GAP, hudSpectatorStripBottom() + 6);
		float shown = 1.f;
		if (!hudReduceMotion())
		{
			if (leaving)
			{
				shown = 1.f - std::min(1.f, static_cast<float>(realTime - leftAt) / SLIDE_MS);
			}
			else
			{
				shown = std::min(1.f, static_cast<float>(realTime - startedAt) / SLIDE_MS);
			}
		}
		const int y0 = static_cast<int>(std::round(static_cast<float>(-h) + static_cast<float>(top + h) * shown));
		show(true);
		setGeometry((static_cast<int>(screenWidth) - w) / 2, y0, w, h);
	}

	void geometryChanged() override
	{
		layoutChildren();
	}

	void layoutChildren()
	{
		if (!action)
		{
			return;
		}
		const bool compact = height() < HEIGHT;
		const int closeSize = compact ? 22 : 20;
		close->setGeometry(width() - 8 - closeSize, (height() - closeSize) / 2, closeSize, closeSize);
		action->label = current.toast.action;
		if (action->label.empty())
		{
			action->hide();
			return;
		}
		action->show();
		const int actionHeight = compact ? 24 : 26;
		const int actionWidth = action->idealWidth();
		action->setGeometry(close->x() - 8 - actionWidth, (height() - actionHeight) / 2, actionWidth, actionHeight);
	}

	void display(int xOffset, int yOffset) override
	{
		if (!showing)
		{
			return;
		}
		HudSkin &skin = currentHudSkin();
		const HudColors &colors = skin.colors();
		const WzRect rect(xOffset + x(), yOffset + y(), width(), height());
		const bool compact = rect.height() < HEIGHT;
		skin.drawPopoverFrame(rect);
		pie_BoxFill(rect.x() + 1, rect.y() + 1, rect.x() + 4, rect.bottom() - 1, colors.accent);
		const int picture = rect.height() - (compact ? 8 : 12);
		const WzRect well(rect.x() + 10, rect.y() + (rect.height() - picture) / 2, picture, picture);
		pie_DrawRoundedBox(static_cast<float>(well.x()), static_cast<float>(well.y()), static_cast<float>(well.right()), static_cast<float>(well.bottom()),
		                   pal_RGBA(6, 9, 14, 230), 4.f, withAlpha(colors.panelBorder, 150), 1.f);
		if (current.toast.drawPicture)
		{
			current.toast.drawPicture(well);
		}
		const int textX = well.right() + 10;
		const int textRight = action->visible() ? action->x() + rect.x() - 10 : close->x() + rect.x() - 10;
		const int textWidth = std::max(0, textRight - textX);
		const std::string title = titleOf(current);
		if (compact)
		{
			drawLine(titleText, title, font_regular, textX, rect.y() + (rect.height() - static_cast<int>(iV_GetTextLineSize(font_regular))) / 2,
			         textWidth, colors.textPrimary);
			return;
		}
		drawLine(kindText, current.toast.kind, font_small, textX, rect.y() + 7, textWidth, colors.accent);
		drawLine(titleText, title, font_regular_bold, textX, rect.y() + 23, textWidth, colors.textPrimary);
	}

	bool isShowing() const { return showing; }

private:
	/// One line, cut with an ellipsis
	static void drawLine(WzText &text, const std::string &string, iV_fonts font, int x, int top, int maxWidth, PIELIGHT color)
	{
		text.setText(WzString::fromUtf8(string), font, std::max(maxWidth, 1));
		hudDrawTextLine(text, static_cast<float>(x), static_cast<float>(top - text.aboveBase()), maxWidth, color);
	}

	Pending current;
	bool showing = false;
	bool leaving = false;
	uint32_t shownTime = 0;
	uint32_t lastTick = 0;
	uint32_t startedAt = 0;
	uint32_t leftAt = 0;
	std::shared_ptr<ToastAction> action;
	std::shared_ptr<ToastClose> close;
	WzText kindText;
	WzText titleText;
};

ToastWidget *findToast()
{
	return psWScreen ? dynamic_cast<ToastWidget *>(widgGetFromID(psWScreen, IDHUD_TOAST)) : nullptr;
}

}

void hudToastShow(HudToast toast)
{
	if (psWScreen == nullptr)
	{
		return;
	}
	ToastWidget *widget = findToast();
	if (widget == nullptr)
	{
		auto created = std::make_shared<ToastWidget>();
		created->id = IDHUD_TOAST;
		created->setHelp(WidgetHelp()
			.setTitle(_("Notices"))
			.setDescription(_("What just arrived, ex. a research report: View opens it. They can be turned off in the options.")));
		psWScreen->psForm->attach(created);
		created->initialize();
		created->hide();
		widget = created.get();
	}
	Pending pending;
	pending.toast = std::move(toast);
	if (widget->offer(pending))
	{
		return;
	}
	// (Several of a group waiting together make one)
	for (auto &item : waiting)
	{
		if (!pending.toast.group.empty() && item.toast.group == pending.toast.group)
		{
			const size_t count = item.count + 1;
			item = pending;
			item.count = count;
			return;
		}
	}
	waiting.push_back(std::move(pending));
}

void hudToastClearAll()
{
	waiting.clear();
	if (psWScreen != nullptr && findToast() != nullptr)
	{
		widgDelete(psWScreen, IDHUD_TOAST);
	}
}

bool hudToastShown()
{
	ToastWidget *widget = findToast();
	return widget != nullptr && widget->isShowing();
}
