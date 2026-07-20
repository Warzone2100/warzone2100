/*
	This file is part of Warzone 2100.
	Copyright (C) 2024  Warzone 2100 Project

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
/** \file
 *  Info Button
 */

#include "infobutton.h"
#include "src/frend.h"

WzInfoButton::WzInfoButton()
{ }

std::shared_ptr<WzInfoButton> WzInfoButton::make()
{
	class make_shared_enabler: public WzInfoButton {};
	auto widget = std::make_shared<make_shared_enabler>();
	widget->setImage(IMAGE_INFO_CIRCLE);
	widget->setBorderDrawMode(WzFrontendImageButton::BorderDrawMode::Highlighted);
	widget->setPadding(0, 0);
	return widget;
}
