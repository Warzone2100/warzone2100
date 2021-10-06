#include "gmcommon.h"

#include "lib/netplay/netplay.h"

#include "../../multiplay.h"
#include "../../warzoneconfig.h"
#include "../../component.h"
#include "../../map.h"
#include "../../loadsave.h"
#include "../common.h"

char			aLevelName[MAX_LEVEL_NAME_SIZE + 1];	//256];			// vital! the wrf file to use.
bool			bLimiterLoaded = false;

void SPinit(LEVEL_TYPE gameType)
{
	uint8_t playercolor;

	NetPlay.bComms = false;
	bMultiPlayer = false;
	bMultiMessages = false;
	game.type = gameType;
	NET_InitPlayers();
	NetPlay.players[0].allocated = true;
	NetPlay.players[0].autoGame = false;
	NetPlay.players[0].difficulty = AIDifficulty::HUMAN;
	game.maxPlayers = MAX_PLAYERS - 1;	// Currently, 0 - 10 for a total of MAX_PLAYERS
	// make sure we have a valid color choice for our SP game. Valid values are 0, 4-7
	playercolor = war_GetSPcolor();

	if (playercolor >= 1 && playercolor <= 3)
	{
		playercolor = 0;	// default is green
	}
	setPlayerColour(0, playercolor);
	game.hash.setZero();	// must reset this to zero
	builtInMap = true;
}

void loadOK()
{
	if (strlen(sRequestResult))
	{
		sstrcpy(saveGameName, sRequestResult);
		changeTitleMode(LOADSAVEGAME);
	}
}
