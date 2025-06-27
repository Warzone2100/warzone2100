// SPDX-License-Identifier: GPL-2.0-or-later

/*
	This file is part of Warzone 2100.
	Copyright (C) 2025  Warzone 2100 Project

	Warzone 2100 is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	Warzone 2100 is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Warzone 2100; if not, write to the Free Software
	Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
*/
/**
 * @file
 * Functions for popover.
 */

#include "popover.h"
#include "widgbase.h"
#include "form.h"
#include "lib/ivis_opengl/pieblitfunc.h"
#include "lib/framework/input.h"

PopoverWidget::PopoverWidget()
{ }

PopoverWidget::~PopoverWidget()
{
	if (overlayScreen.lock())
	{
		close();
	}
}

std::shared_ptr<PopoverWidget> PopoverWidget::makePopover(const std::shared_ptr<WIDGET>& parent, std::shared_ptr<WIDGET> popoverContents, PopoverWidget::Style style, Alignment align, Vector2i positionOffset, const OnPopoverCloseHandlerFunc& onCloseHandler)
{
	class make_shared_enabler: public PopoverWidget { };
	auto result = std::make_shared<make_shared_enabler>();
	result->style = style;
	result->align = align;
	result->positionOffset = positionOffset;
	result->setGeometry(0, 0, popoverContents->width(), popoverContents->height());
	result->popoverContents = popoverContents;
	result->onCloseHandler = onCloseHandler;
	result->attach(popoverContents);
	result->open(parent);
	return result;
}

void PopoverWidget::repositionOnScreenRelativeToParent(const std::shared_ptr<WIDGET>& parent)
{
	int popOverX0 = 0;
	switch (align)
	{
		case Alignment::LeftOfParent:
		{
			// (Ideally, with its left aligned with the left side of the "parent" widget, but ensure visibility on-screen)
			popOverX0 = parent->screenPosX();
			if (popOverX0 + width() > screenWidth)
			{
				popOverX0 = screenWidth - width();
			}
		}
		break;
		case Alignment::RightOfParent:
		{
			// (Ideally, with its right aligned with the right side of the "parent" widget, but ensure left side visibility on-screen)
			int parentX1 = parent->screenPosX() + parent->width();
			popOverX0 = parentX1 - width();
			if (popOverX0 < 0)
			{
				popOverX0 = 0;
			}
		}
		break;
	}
	// (Ideally, with its top aligned with the bottom of the "parent" widget, but ensure full visibility on-screen)
	int popOverY0 = parent->screenPosY() + parent->height();
	if (popOverY0 + height() > screenHeight)
	{
		popOverY0 = screenHeight - height();
	}
	move(popOverX0 + positionOffset.x, popOverY0 + positionOffset.y);
}

void PopoverWidget::geometryChanged()
{
	if (popoverContents)
	{
		popoverContents->setGeometry(0, 0, width(), height());
	}
}

void PopoverWidget::open(const std::shared_ptr<WIDGET>& parent)
{
	if (overlayScreen.lock() != nullptr) { return; }
	std::shared_ptr<PopoverWidget> popoverWidget(std::dynamic_pointer_cast<PopoverWidget>(shared_from_this()));
	std::weak_ptr<WIDGET> weakParent = parent;

	widgScheduleTask([popoverWidget, weakParent]() {
		auto strongParent = weakParent.lock();

		auto lockedScreen = (strongParent) ? strongParent->screenPointer.lock() : nullptr;
		ASSERT(lockedScreen != nullptr, "The parent does not have an associated screen pointer?");

		auto psOverlayScreen = W_SCREEN::make();
		psOverlayScreen->psForm->hide();

		auto newRootFrm = W_FULLSCREENOVERLAY_CLICKFORM::make();
		newRootFrm->displayFunction = displayChildDropShadows;
		std::weak_ptr<W_SCREEN> psWeakOverlayScreen(psOverlayScreen);
		std::weak_ptr<PopoverWidget> pWeakThis(popoverWidget);
		newRootFrm->onClickedFunc = [psWeakOverlayScreen, pWeakThis]() {
			if (auto popoverWidget = pWeakThis.lock())
			{
				popoverWidget->close();
			}
			if (auto psOverlayScreen = psWeakOverlayScreen.lock())
			{
				widgRemoveOverlayScreen(psOverlayScreen);
			}
		};
		newRootFrm->onCancelPressed = newRootFrm->onClickedFunc;
		if (popoverWidget->style == Style::NonInteractive)
		{
			newRootFrm->setTransparentToMouse(true);
			newRootFrm->setCustomHitTest([](const WIDGET *psWidget, int x, int y) -> bool {
				return false; // effectively: make this (and all of its children) unclickable
			});
		}
		psOverlayScreen->psForm->attach(newRootFrm);

		popoverWidget->overlayScreen = psOverlayScreen;
		newRootFrm->attach(popoverWidget);

		popoverWidget->setCalcLayout([weakParent](WIDGET *psWidget) {
			auto psParent = weakParent.lock();
			ASSERT_OR_RETURN(, psParent != nullptr, "parent is null");
			auto psPopoverWidget = std::static_pointer_cast<PopoverWidget>(psWidget->shared_from_this());
			psPopoverWidget->repositionOnScreenRelativeToParent(psParent);
		});

		widgRegisterOverlayScreenOnTopOfScreen(psOverlayScreen, lockedScreen);
	});
}

void PopoverWidget::closeImpl()
{
	auto strongOverlay = overlayScreen.lock();
	if (strongOverlay == nullptr)
	{
		return;
	}
	widgRemoveOverlayScreen(strongOverlay);
	overlayScreen.reset();
	if (onCloseHandler)
	{
		onCloseHandler();
	}
}

void PopoverWidget::close()
{
	// because open() uses widgScheduleTask to actually create the overlay screen,
	// widgScheduleTask must also be used to close it (otherwise, overlayScreen might be accessed before the scheduled open task creates it)
	std::shared_ptr<PopoverWidget> popoverWidget(std::dynamic_pointer_cast<PopoverWidget>(shared_from_this()));
	widgScheduleTask([popoverWidget]() {
		popoverWidget->closeImpl();
	});
}

void PopoverWidget::display(int xOffset, int yOffset)
{
	// currently, no-op
}

int32_t PopoverWidget::idealWidth()
{
	if (!popoverContents)
	{
		return 0;
	}
	return popoverContents->idealWidth();
}

int32_t PopoverWidget::idealHeight()
{
	if (!popoverContents)
	{
		return 0;
	}
	return popoverContents->idealHeight();
}
