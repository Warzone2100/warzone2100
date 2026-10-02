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

#include "hud.h"
#include "hudmodel.h"
#include "hudskin.h"
#include "hudmodules.h"

#include "lib/framework/frame.h"
#include "lib/ivis_opengl/pieclip.h"
#include "lib/widget/widget.h"
#include "lib/widget/form.h"

#include "../hci.h"
#include "../ingameop.h"
#include "../mission.h"
#include "../radar.h"

#include <vector>

namespace
{

std::vector<std::unique_ptr<HudModule>> modules;
bool initialized = false;
bool rebuildPending = false;
bool interfaceShown = true;

bool safeToRebuild()
{
	if (psWScreen == nullptr || !initialized)
	{
		return false;
	}
	if (InGameOpUp || MissionResUp)
	{
		return false;
	}
	switch (intMode)
	{
	case INT_DESIGN:
	case INT_INTELMAP:
	case INT_INGAMEOP:
	case INT_MISSIONRES:
	case INT_TRANSPORTER:
		return false;
	default:
		return true;
	}
}

void destroyModules()
{
	for (auto &module : modules)
	{
		module->destroy();
	}
	modules.clear();
}

void createModules()
{
	for (size_t i = 0; i < HUD_MODULE_COUNT; ++i)
	{
		const auto id = static_cast<HudModuleId>(i);
		if (!hudModuleActive(id))
		{
			continue;
		}
		auto module = hudMakeModule(id);
		if (module)
		{
			module->create(psWScreen->psForm);
			module->setVisible(interfaceShown);
			modules.push_back(std::move(module));
		}
	}
}

void rebuildNow()
{
	rebuildPending = false;
	const OBJECT_MODE openPanel = intOpenObjectPanel();
	hudCloseTransientUi();
	destroyModules();
	if (openPanel != IOBJ_NONE)
	{
		intResetScreen(true);
	}
	createModules();
	if (openPanel != IOBJ_NONE)
	{
		intReopenObjectPanel(openPanel);
	}
}

}

void hudInitialize()
{
	initialized = true;
	interfaceShown = true;
	rebuildPending = false;
	hudSkinChanged();
	createModules();
}

void hudShutdown()
{
	hudCloseTransientUi();
	destroyModules();
	modernHudSkin().releaseCaches();
	initialized = false;
	rebuildPending = false;
}

void hudUpdate()
{
	if (rebuildPending && safeToRebuild())
	{
		rebuildNow();
	}
	for (auto &module : modules)
	{
		module->update();
	}
}

void hudInterfaceShown(bool shown)
{
	interfaceShown = shown;
	for (auto &module : modules)
	{
		module->setVisible(shown);
	}
}

void hudRequestRebuild()
{
	if (!initialized)
	{
		return;
	}
	rebuildPending = true;
	if (safeToRebuild())
	{
		widgScheduleTask([]() {
			if (rebuildPending && safeToRebuild())
			{
				rebuildNow();
			}
		});
	}
}

bool hudRebuildPending()
{
	return rebuildPending;
}

void hudSkinChanged()
{
	widgSetTipStyle(currentHudSkin().tooltipStyle());
	widgSetTipDelay(hudSettings().tooltipDelay);
}

WzRect hudStatusZone()
{
	const int x0 = RET_X + RET_FORMWIDTH + 4;
	const int x1 = OBJ_BACKX - 4;
	const int y0 = RET_Y + 4;
	const int y1 = pie_GetVideoBufferHeight() - 2;
	if (x1 - x0 < 60)
	{
		return WzRect();
	}
	return WzRect(x0, y0, x1 - x0, y1 - y0);
}

WzRect hudTopZone()
{
	constexpr int margin = 60;
	const int width = static_cast<int>(pie_GetVideoBufferWidth());
	return WzRect(margin, 3, std::max(0, width - margin * 2), 30);
}

WzRect hudRadarZone()
{
	auto radar = getRadarWidget();
	if (radar == nullptr)
	{
		return WzRect();
	}
	const WzRect radarRect = radar->geometry();
	const int height = 20;
	const int width = std::min<int>(radarRect.width() + 40, pie_GetVideoBufferWidth() / 3);
	return WzRect(radarRect.right() - width, radarRect.y() - height - 4, width, height);
}
