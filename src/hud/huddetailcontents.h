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
/** @file
 *  What the HUD's detail popovers say about each kind of element, built from the detail-panel kit.
 *  They describe the local player's own things only, with the local player's own upgrades.
 */

#ifndef __INCLUDED_SRC_HUD_HUDDETAILCONTENTS_H__
#define __INCLUDED_SRC_HUD_HUDDETAILCONTENTS_H__

#include <memory>
#include <string>

class WIDGET;
struct BASE_OBJECT;
struct DROID_TEMPLATE;
struct RESEARCH;
struct STRUCTURE;
struct STRUCTURE_STATS;

/// A structure in the build panel. constructionRate: the build points per second of the trucks that would build it
std::shared_ptr<WIDGET> hudStructureDetail(STRUCTURE_STATS *stats, unsigned constructionRate);
/// A design in the manufacture panel, at the factory (if any) it would be made in
std::shared_ptr<WIDGET> hudTemplateDetail(DROID_TEMPLATE *psTemplate, STRUCTURE *factory);
/// A topic in the research panel
std::shared_ptr<WIDGET> hudResearchDetail(RESEARCH *research);
/// A factory, research facility, truck or commander in the object row
std::shared_ptr<WIDGET> hudObjectDetail(BASE_OBJECT *object);
/// A reticule button: its name (the script's tooltip), what it opens, its key, and what waits there
std::shared_ptr<WIDGET> hudReticuleDetail(int slot, const std::string &tip, const std::string &keyFunction);
std::shared_ptr<WIDGET> hudPowerDetail();
/// A group (1-9, 0) of the groups bar
std::shared_ptr<WIDGET> hudGroupDetail(unsigned group);
/// An empty group, for a long-press (its name only: the popover offers to assign the selection to it)
std::shared_ptr<WIDGET> hudGroupEmptyDetail(unsigned group);

#endif // __INCLUDED_SRC_HUD_HUDDETAILCONTENTS_H__
