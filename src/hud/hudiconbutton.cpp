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

#include "hudiconbutton.h"

bool hudDrawIconButton(const W_BUTTON &button, HudIcon icon, bool active, HudIconButtonStyle style, int xOffset, int yOffset)
{
	HudIconButtonLook look;
	look.rect = WzRect(xOffset + button.x(), yOffset + button.y(), button.width(), button.height());
	look.icon = icon;
	look.style = style;
	look.down = (button.getState() & WBUT_DOWN) != 0;
	look.highlighted = (button.getState() & WBUT_HIGHLIGHT) != 0;
	look.active = active;
	look.disabled = (button.getState() & WBUT_DISABLE) != 0;
	return currentHudSkin().drawIconButton(look);
}

void HudIconChoiceButton::setIcon(unsigned stateValue, HudIcon icon, bool active)
{
	icons[stateValue] = {icon, active};
}

void HudIconChoiceButton::display(int xOffset, int yOffset)
{
	auto it = icons.find(getChoice());
	if (it == icons.end() || !hudDrawIconButton(*this, it->second.icon, it->second.active, iconStyle, xOffset, yOffset))
	{
		MultipleChoiceButton::display(xOffset, yOffset);
	}
}
