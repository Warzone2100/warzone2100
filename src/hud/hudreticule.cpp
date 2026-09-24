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

#include "hudreticule.h"
#include "hudkeys.h"

#include "lib/framework/frame.h"

#include "../hci.h"

static constexpr int RETICULE_BUTTONS = RETBUT_COMMAND + 1;

static const char *const reticuleKeyFunctions[RETICULE_BUTTONS] =
{
	nullptr,
	"ChooseManufacture",
	"ChooseResearch",
	"ChooseBuild",
	"ChooseDesign",
	"ChooseIntelligence",
	"ChooseCommand",
};

WidgetHelp reticuleButtonHelp(int slot)
{
	WidgetHelp help;
	switch (slot)
	{
	case RETBUT_CANCEL:
		help.setTitle(_("Close")).setDescription(_("Closes the open panel."));
		break;
	case RETBUT_FACTORY:
		help.setTitle(_("Manufacture")).setDescription(_("Shows your factories and the units they can produce."));
		break;
	case RETBUT_RESEARCH:
		help.setTitle(_("Research")).setDescription(_("Shows your research facilities and the topics they can research."));
		break;
	case RETBUT_BUILD:
		help.setTitle(_("Build")).setDescription(_("Shows your trucks and the structures they can build."));
		break;
	case RETBUT_DESIGN:
		help.setTitle(_("Design")).setDescription(_("Opens the design screen, to design new units."));
		break;
	case RETBUT_INTELMAP:
		help.setTitle(_("Intelligence Display")).setDescription(_("Shows messages, research results and mission briefings."));
		break;
	case RETBUT_COMMAND:
		help.setTitle(_("Commanders")).setDescription(_("Shows your commanders and their orders."));
		break;
	default:
		break;
	}
	if (slot >= 0 && slot < RETICULE_BUTTONS && reticuleKeyFunctions[slot] != nullptr)
	{
		help.addRelatedKeybinding(reticuleKeyFunctions[slot]);
	}
	return help;
}

class ReticuleButton : public W_BUTTON
{
public:
	using W_BUTTON::W_BUTTON;

	std::string getTip() override
	{
		std::string tip = W_BUTTON::getTip();
		if (UserData >= RETICULE_BUTTONS || reticuleKeyFunctions[UserData] == nullptr || tip.empty() || !intCheckReticuleButEnabled(id))
		{
			return tip;
		}
		return hudTipWithKeyBinding(tip, reticuleKeyFunctions[UserData]);
	}
};

std::shared_ptr<W_BUTTON> makeReticuleButton(const W_BUTINIT *init)
{
	return std::make_shared<ReticuleButton>(init);
}
