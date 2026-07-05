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
 *  Gamepad button dispatch for in-game actions.
 */

#include "gamepadbindings.h"

#include "lib/framework/frame.h"
#include "lib/framework/gamepad_input.h"
#include "lib/framework/wzapp.h"
#include "../hci.h"
#include "../keybind.h"
#include "../levels.h"
#include "../multimenu.h"
#include "../multiplay.h"

// Holding a d-pad direction this long assigns the current selection to its
// group instead of recalling the group
static const uint32_t GAMEPAD_GROUP_ASSIGN_HOLD_MS = 400;

struct GroupButtonState
{
	unsigned int group = 0;   // latched at press, including the shoulder layer offset
	uint32_t pressTime = 0;
	bool held = false;
	bool assigned = false;
};

// D-pad directions map to unit groups clockwise from up (1, 2, 3, 4), with the
// left shoulder selecting groups 5 to 8
// - tap recalls the group (recalling again centers the camera on it)
// - hold assigns the current selection to the group
static void updateGroupButton(GAMEPAD_INPUT button, unsigned int baseGroup, GroupButtonState& state)
{
	if (gamepadButtonPressed(button))
	{
		state.held = true;
		state.assigned = false;
		state.pressTime = wzGetTicks();
		state.group = baseGroup + (gamepadButtonDown(GPAD_BTN_LEFT_SHOULDER) ? 4 : 0);
	}
	if (state.held && !state.assigned && gamepadButtonDown(button) && wzGetTicks() - state.pressTime >= GAMEPAD_GROUP_ASSIGN_HOLD_MS)
	{
		state.assigned = true;
		kf_AssignGrouping_N(state.group)();
	}
	if (gamepadButtonReleased(button) && state.held)
	{
		state.held = false;
		if (!state.assigned)
		{
			kf_SelectGrouping_N(state.group)();
		}
	}
}

void gamepadProcessBindings()
{
	static GroupButtonState dpadUp, dpadRight, dpadDown, dpadLeft;
	updateGroupButton(GPAD_BTN_DPAD_UP, 1, dpadUp);
	updateGroupButton(GPAD_BTN_DPAD_RIGHT, 2, dpadRight);
	updateGroupButton(GPAD_BTN_DPAD_DOWN, 3, dpadDown);
	updateGroupButton(GPAD_BTN_DPAD_LEFT, 4, dpadLeft);

	// clicking the right stick centers the camera on the last event
	if (gamepadButtonPressed(GPAD_BTN_RIGHT_STICK))
	{
		kf_MoveToLastMessagePos();
	}

	// holding the right shoulder chords the face buttons to the command panels
	// (click and key synthesis for these buttons is suppressed while it is held)
	const bool rightShoulderHeld = gamepadButtonDown(GPAD_BTN_RIGHT_SHOULDER);
	if (rightShoulderHeld)
	{
		if (gamepadButtonPressed(GPAD_BTN_SOUTH))
		{
			kf_ChooseBuild();
		}
		if (gamepadButtonPressed(GPAD_BTN_WEST))
		{
			kf_ChooseManufacture();
		}
		if (gamepadButtonPressed(GPAD_BTN_NORTH))
		{
			kf_ChooseResearch();
		}
		if (gamepadButtonPressed(GPAD_BTN_EAST))
		{
			kf_ChooseCommand();
		}
	}
	// north cycles through the player's factories
	else if (gamepadButtonPressed(GPAD_BTN_NORTH))
	{
		kf_SelectNextFactory(REF_FACTORY, true)();
	}

	// back toggles game info - the objectives screen in campaign and the
	// multiplayer options / alliances dialog in multiplayer
	if (gamepadButtonPressed(GPAD_BTN_BACK))
	{
		if (bMultiPlayer)
		{
			if (MultiMenuUp)
			{
				intCloseMultiMenu();
			}
			else
			{
				kf_addMultiMenu();
			}
		}
		else if (intMode == INT_INTELMAP)
		{
			intResetScreen(false);
		}
		else
		{
			kf_ChooseIntelligence();
		}
	}
}

// Opening the objectives screen in campaign pauses the game update, which
// stops the input processing that runs gamepadProcessBindings, so closing
// it again on back is handled here
void gamepadProcessPausedBindings()
{
	if (gamepadButtonPressed(GPAD_BTN_BACK) && intMode == INT_INTELMAP)
	{
		intResetScreen(false);
	}
}
