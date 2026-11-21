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

#include "hudunittypes.h"

#include "lib/framework/frame.h"

#include "../droid.h"

HudUnitType hudUnitTypeOf(const DROID *psDroid)
{
	if (psDroid->isTransporter())
	{
		return HudUnitType::Transport;
	}
	switch (psDroid->droidType)
	{
	case DROID_CONSTRUCT:
	case DROID_CYBORG_CONSTRUCT:
		return HudUnitType::Truck;
	case DROID_SENSOR:
		return HudUnitType::Sensor;
	case DROID_REPAIR:
	case DROID_CYBORG_REPAIR:
		return HudUnitType::Repair;
	case DROID_COMMAND:
		return HudUnitType::Commander;
	default:
		break;
	}
	if (psDroid->isVtol())
	{
		return HudUnitType::Vtol;
	}
	if (psDroid->isCyborg())
	{
		return HudUnitType::Cyborg;
	}
	return HudUnitType::Tank;
}

const char *hudUnitTypeName(HudUnitType type, unsigned count)
{
	switch (type)
	{
	case HudUnitType::Tank: return ngettext("tank", "tanks", count);
	case HudUnitType::Cyborg: return ngettext("cyborg", "cyborgs", count);
	case HudUnitType::Vtol: return ngettext("VTOL unit", "VTOL units", count);
	case HudUnitType::Truck: return ngettext("truck", "trucks", count);
	case HudUnitType::Sensor: return ngettext("sensor", "sensors", count);
	case HudUnitType::Repair: return ngettext("repair unit", "repair units", count);
	case HudUnitType::Commander: return ngettext("commander", "commanders", count);
	case HudUnitType::Transport: return ngettext("transport", "transports", count);
	}
	return "";
}

const char *hudUnitTypeSymbol(HudUnitType type)
{
	switch (type)
	{
	case HudUnitType::Tank: return "▣";
	case HudUnitType::Cyborg: return "◆";
	case HudUnitType::Vtol: return "✈";
	case HudUnitType::Truck: return "⚒";
	case HudUnitType::Sensor: return "◎";
	case HudUnitType::Repair: return "+";
	case HudUnitType::Commander: return "★";
	case HudUnitType::Transport: return "▱";
	}
	return "";
}
