/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2020  Warzone 2100 Project

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
/** @file
 *  Functions for the MultibuttonWidget widget
 */

#include "lib/framework/frame.h"
#include "widget.h"
#include "widgint.h"
#include "multibutform.h"
#include "button.h"
#include "tip.h"
#include "lib/ivis_opengl/pieblitfunc.h"

MultibuttonWidget::MultibuttonWidget(int value)
	: W_FORM()
	, label(nullptr)
	, currentValue_(value)
	, disabled(false)
	, gap_(3)
	, lockCurrent(false)
{
}

void MultibuttonWidget::display(int xOffset, int yOffset)
{
	iV_ShadowBox(xOffset + x(), yOffset + y(), xOffset + x() + width() - 1, yOffset + y() + height() - 1, 0, WZCOL_MENU_BORDER, WZCOL_MENU_BORDER, WZCOL_MENU_BACKGROUND);
}

void MultibuttonWidget::geometryChanged()
{
	int s = width() - gap_;
	switch (butAlign)
	{
		case ButtonAlignment::CENTER_ALIGN:
		{
			int width_of_all_buttons = 0;
			for (size_t i = 0; i < buttons.size(); ++i)
			{
				if (i > 0)
				{
					width_of_all_buttons += gap_;
				}
				width_of_all_buttons += buttons[i].first->width();
			}
			if (width_of_all_buttons < width())
			{
				s = width() - gap_ - ((width() - width_of_all_buttons) / 2);
			}
		}
		// fall-through
		case ButtonAlignment::RIGHT_ALIGN:
			for (auto i = buttons.crbegin(); i != buttons.crend(); ++i)
			{
				i->first->move(s - i->first->width(), (height() - i->first->height()) / 2);
				s -= i->first->width() + gap_;
			}
			break;
	}
	if (label != nullptr)
	{
		label->setGeometry(gap_, 0, s - gap_, height());
	}
}

void MultibuttonWidget::setLabel(char const *text)
{
	attach(label = std::make_shared<W_LABEL>());
	label->setString(text);
	label->setCacheNeverExpires(true);

	geometryChanged();
}

void MultibuttonWidget::addButton(int value, const std::shared_ptr<W_BUTTON>& button)
{
	this->attach(button);
	button->setState(value == currentValue_ && lockCurrent ? WBUT_LOCK : disabled ? WBUT_DISABLE : 0);
	buttons.push_back(std::make_pair(button, value));

	button->addOnClickHandler([value](W_BUTTON& button) {
		auto pParent = std::static_pointer_cast<MultibuttonWidget>(button.parent());
		assert(pParent != nullptr);
		pParent->choose(value);
	});

	geometryChanged();
}

void MultibuttonWidget::setButtonMinClickInterval(UDWORD interval)
{
	for (auto& button_pair : buttons)
	{
		if (button_pair.first)
		{
			button_pair.first->minClickInterval = interval;
		}
	}
}

void MultibuttonWidget::setButtonAlignment(MultibuttonWidget::ButtonAlignment newAlignment)
{
	if (newAlignment == butAlign)
	{
		return;
	}
	butAlign = newAlignment;
	geometryChanged();
}

void MultibuttonWidget::enable(bool enabled)
{
	if (!enabled == disabled)
	{
		return;
	}

	disabled = !enabled;
	stateChanged();
}

void MultibuttonWidget::setGap(int gap)
{
	if (gap == gap_)
	{
		return;
	}

	gap_ = gap;
	geometryChanged();
}

void MultibuttonWidget::addOnChooseHandler(const W_ON_CHOOSE_FUNC& onChooseFunc)
{
	onChooseHandlers.push_back(onChooseFunc);
}

void MultibuttonWidget::choose(int value)
{
	if (value == currentValue_ && lockCurrent)
	{
		return;
	}

	currentValue_ = value;
	stateChanged();

	/* Call all onChoose event handlers */
	for (auto it = onChooseHandlers.begin(); it != onChooseHandlers.end(); it++)
	{
		auto onChoose = *it;
		if (onChoose)
		{
			onChoose(*this, currentValue_);
		}
	}

	if (auto lockedScreen = screenPointer.lock())
	{
		lockedScreen->setReturn(shared_from_this());
	}
}

void MultibuttonWidget::stateChanged()
{
	for (auto i = buttons.cbegin(); i != buttons.cend(); ++i)
	{
		i->first->setState(i->second == currentValue_ && lockCurrent ? WBUT_LOCK : disabled ? WBUT_DISABLE : 0);
	}
}

MultichoiceWidget::MultichoiceWidget(int value)
	: MultibuttonWidget(value)
{
	lockCurrent = true;
}

