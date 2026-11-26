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
 *  Definition for in game,multiplayer, interface.
 */

#ifndef __INCLUDED_SRC_MULTIMENU__
#define __INCLUDED_SRC_MULTIMENU__

#include "lib/widget/widgbase.h"
#include "lib/widget/multibutform.h"
#include "stringdef.h"

#include <string>

// requester
void addMultiRequest(const char *searchDir, const char *fileExtension, UDWORD id, UBYTE numPlayers, std::string const &searchString = std::string());
void closeMultiRequester();

extern bool multiRequestUp;
extern std::shared_ptr<W_SCREEN> psRScreen; // requester stuff.

bool runMultiRequester(UDWORD id, UDWORD *mode, WzString *chosen, LEVEL_DATASET **chosenValue, bool *isHoverPreview);
void displayRequestOption(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset);

// multimenu
bool intCloseMultiMenu();
void intCloseMultiMenuNoAnim();
bool intAddMultiMenu();

class IntFormAnimated;
std::shared_ptr<IntFormAnimated> intCreateMultiMenuForm(std::function<void ()> closeButtonHandler = nullptr);

class WzMultiMenuTabs : public MultichoiceWidget
{
public:
	WzMultiMenuTabs(int value = -1) : MultichoiceWidget(value) { }
	virtual void display(int xOffset, int yOffset) override { }
};

/// The multiplayer menu's rules, shared with the HUD's players view (src/hud/intel/playersview.cpp)
bool multiMenuListsPlayer(uint32_t player);
/// No units or structures left (the menu grays them)
bool multiMenuPlayerIsDead(uint32_t player);
enum class MultiMenuLastColumn
{
	Power,       ///< with the debug mappings
	Ping,        ///< in a networked game
	Structures
};
MultiMenuLastColumn multiMenuLastColumn();
std::string multiMenuScoreText(uint32_t player);
std::string multiMenuKillsText(uint32_t player);
/// The local player's and allies' only (anyone's with the debug mappings): empty otherwise
std::string multiMenuUnitsText(uint32_t player);
/// Empty where the menu shows nothing. A ping of PING_LIMIT or more is "∞"
std::string multiMenuLastColumnText(uint32_t player);
/// Whether the local player's chat goes to the player can be set
/// (not for oneself, and players don't set it for spectators)
bool multiMenuHasChannel(uint32_t player);
/// The alliance and gifts controls: when the game allows giving, for the other players (not spectators)
bool multiMenuHasAllianceControls(uint32_t player);
/// The alliance can be changed during the game (not with locked teams)
bool multiMenuCanChangeAlliance(uint32_t player);
/// The alliance button's click: requests an alliance, accepts an invitation, cancels a request, or breaks an alliance
void multiMenuToggleAlliance(uint32_t player);
/// The gifts are shown (to allies)
bool multiMenuShowsGifts(uint32_t player);
bool multiMenuCanGiveResearchAndRadar();
/// The host can start a kick vote (in a networked game, or a skirmish with cheats on)
bool multiMenuCanStartKickVote(uint32_t player);
bool multiMenuHasMute(uint32_t player);

void multiMenuScreenSizeDidChange(unsigned int oldWidth, unsigned int oldHeight, unsigned int newWidth, unsigned int newHeight);

extern bool		MultiMenuUp;

extern UDWORD		current_numplayers;

#define MULTIMENU				10600
#define MULTIMENU_FORM			MULTIMENU

#endif // __INCLUDED_SRC_MULTIMENU__
