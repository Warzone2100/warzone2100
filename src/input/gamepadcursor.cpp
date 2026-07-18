/*
	This file is part of Warzone 2100.
	Copyright (C) 2026  Warzone 2100 Project (https://github.com/Warzone2100)

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
 *  Cursor drawing for gamepad input.
 *
 *  While the gamepad is the active input source the system pointer is hidden,
 *  so the current cursor is drawn as part of the frame instead - via a
 *  topmost overlay screen whose sole child draws the cursor image.
 */

#include "gamepadcursor.h"

#include "lib/framework/frame.h"
#include "lib/framework/gamepad_input.h"
#include "lib/framework/input.h"
#include "lib/framework/wzapp.h"
#include "lib/widget/widget.h"
#include "lib/widget/form.h"
#include "lib/ivis_opengl/pieblitfunc.h"
#include "lib/ivis_opengl/piepalette.h"
#include "lib/ivis_opengl/pietypes.h"
#include "lib/ivis_opengl/screen.h"

#include "../warzoneconfig.h"
#include "../main.h"
#include "../loop.h"

#include <array>
#include <memory>
#include <limits>

// How far a stick flick may hop the cursor to reach the next widget. With a
// running simulation there are no world-object flick targets, so long hops
// would grab the cursor away toward distant widgets when flicking near them -
// keep in-game hops short
static const int GAMEPAD_FLICK_HOP_RANGE = 220;
static const int GAMEPAD_FLICK_HOP_RANGE_INGAME = 100;

static std::shared_ptr<W_SCREEN> gamepadCursorOverlayScreen = nullptr;
static std::array<std::unique_ptr<gfx_api::texture>, CURSOR_MAX> cursorTextures;
static unsigned int cursorTexturesGeneration = 0;

static gfx_api::texture* getCursorTexture(CURSOR cur, const iV_Image& image)
{
	// the cursor image set is rebuilt on coloured/mono or scale changes - drop stale textures
	if (cursorTexturesGeneration != wzGetCursorImagesGeneration())
	{
		for (auto& texture : cursorTextures)
		{
			texture.reset();
		}
		cursorTexturesGeneration = wzGetCursorImagesGeneration();
	}
	if (!cursorTextures[cur])
	{
		iV_Image imageCopy;
		if (!imageCopy.duplicate(image))
		{
			return nullptr;
		}
		cursorTextures[cur] = std::unique_ptr<gfx_api::texture>(gfx_api::context::get().loadTextureFromUncompressedImage(std::move(imageCopy), gfx_api::texture_type::user_interface, "gamepadcursor"));
	}
	return cursorTextures[cur].get();
}

class GamepadCursorWidget : public WIDGET
{
public:
	GamepadCursorWidget() : WIDGET()
	{
		setTransparentToMouse(true);
	}

	void display(int xOffset, int yOffset) override
	{
		if (!isGamepadActiveInput())
		{
			return;
		}
		const CURSOR cur = wzGetCursor();
		if (cur >= CURSOR_MAX)
		{
			return;
		}
		int hotX = 0;
		int hotY = 0;
		const iV_Image* image = wzGetCursorImage(cur, hotX, hotY);
		if (!image)
		{
			return;
		}
		gfx_api::texture* texture = getCursorTexture(cur, *image);
		if (!texture)
		{
			return;
		}
		iV_DrawImageAnisotropic(*texture, Vector2i(mouseX() - hotX, mouseY() - hotY), Vector2f(0.f, 0.f), Vector2f(image->width(), image->height()), 0.f, WZCOL_WHITE);
	}
};

bool gamepadCursorInit()
{
	gamepadCursorOverlayScreen = W_SCREEN::make();
	// hiding the root form prevents it from accepting mouse events without stopping child display
	gamepadCursorOverlayScreen->psForm->hide();

	auto cursorWidget = std::make_shared<GamepadCursorWidget>();
	gamepadCursorOverlayScreen->psForm->attach(cursorWidget);
	cursorWidget->setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
		psWidget->setGeometry(0, 0, screenWidth, screenHeight);
	}));

	// must be registered before other max z-order overlays (e.g. notifications) so the
	// cursor draws above them
	widgRegisterOverlayScreen(gamepadCursorOverlayScreen, std::numeric_limits<uint16_t>::max());
	return true;
}

void gamepadCursorShutdown()
{
	if (gamepadCursorOverlayScreen)
	{
		widgRemoveOverlayScreen(gamepadCursorOverlayScreen);
		gamepadCursorOverlayScreen = nullptr;
	}
	for (auto& texture : cursorTextures)
	{
		texture.reset();
	}
}

void gamepadCursorUpdateWidgetMagnet()
{
	if (!isGamepadActiveInput() || war_GetGamepadCursorMagnetism() == 0 || gamepadInCaptureMode())
	{
		return;
	}
	// never tug the cursor while a button is held, e.g. dragging a slider -
	// the synthetic click check catches double-click presses, which age out
	// of the mouse state while the button remains held
	if (mouseDown(MOUSE_LMB) || mouseDown(MOUSE_RMB) || gamepadSyntheticClickHeld())
	{
		return;
	}

	// a stick flick hops the cursor to the next widget in the flicked
	// direction, searching from where the gesture began, skipping the widget
	// it began inside, and staying on the screen the cursor is over
	const bool simulationRunning = (GetGameMode() == GS_NORMAL) && !gamePaused();
	const int hopRange = simulationRunning ? GAMEPAD_FLICK_HOP_RANGE_INGAME : GAMEPAD_FLICK_HOP_RANGE;
	float flickDirX = 0.f;
	float flickDirY = 0.f;
	int flickStartX = 0;
	int flickStartY = 0;
	if (gamepadConsumePendingCursorFlick(flickDirX, flickDirY, flickStartX, flickStartY))
	{
		const auto hopTarget = widgFindGamepadCursorMagnetTarget(widgGetLastRunScreen(), Vector2i(flickStartX, flickStartY), Vector2f(flickDirX, flickDirY), hopRange, 0.5f, true, widgGetMouseOverScreen());
		if (hopTarget.has_value())
		{
			gamepadCursorHopTo(hopTarget->screenPos.x, hopTarget->screenPos.y);
			return;
		}
	}
	else if (gamepadGetActiveCursorFlick(flickDirX, flickDirY, flickStartX, flickStartY))
	{
		// while the gesture may still become a flick, anchor the prospective
		// hop target so the cursor parks on it instead of overshooting
		const auto anchorTarget = widgFindGamepadCursorMagnetTarget(widgGetLastRunScreen(), Vector2i(flickStartX, flickStartY), Vector2f(flickDirX, flickDirY), hopRange, 0.5f, true, widgGetMouseOverScreen());
		if (anchorTarget.has_value())
		{
			gamepadSetCursorFlickAnchor(anchorTarget->screenPos.x, anchorTarget->screenPos.y);
		}
	}

	// walk the screen currently processing input, so title screens hosting
	// their own W_SCREEN attract the cursor too
	const Vector2f moveDir(gamepadAxis(GPAD_AXIS_LEFT_X), gamepadAxis(GPAD_AXIS_LEFT_Y));
	const auto target = widgFindGamepadCursorMagnetTarget(widgGetLastRunScreen(), Vector2i(mouseX(), mouseY()), moveDir, 48);
	if (target.has_value())
	{
		gamepadSetCursorMagnetTarget(target->screenPos.x, target->screenPos.y, target->screenRadius);
	}
}
