/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2015  Warzone 2100 Project

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
/*
 *  MultiMenu.c
 *  Handles the In Game MultiPlayer Screen, alliances etc...
 *  Also the selection of disk files..
 */
#include "lib/framework/frame.h"
#include "lib/framework/wzapp.h"
#include "lib/framework/strres.h"
#include "lib/framework/physfs_ext.h"
#include "lib/widget/button.h"
#include "lib/widget/widget.h"

#include "display3d.h"
#include "intdisplay.h"
// FIXME Direct iVis implementation include!
#include "lib/ivis_opengl/pieblitfunc.h"
#include "lib/ivis_opengl/piedef.h"
#include "lib/ivis_opengl/piepalette.h"
#include "lib/ivis_opengl/bitimage.h"
#include "lib/gamelib/gtime.h"
#include "lib/ivis_opengl/piematrix.h"
#include "levels.h"
#include "objmem.h"		 	//for droid lists.
#include "component.h"		// for disaplycomponentobj.
#include "hci.h"			// for wFont def.& intmode.
#include "init.h"
#include "power.h"
#include "loadsave.h"		// for drawbluebox
#include "console.h"
#include "ai.h"
#include "frend.h"
#include "lib/netplay/netplay.h"
#include "multiplay.h"
#include "multistat.h"
#include "multimenu.h"
#include "multiint.h"
#include "multigifts.h"
#include "multijoin.h"
#include "mission.h"
#include "scores.h"
#include "keymap.h"
#include "keybind.h"
#include "loop.h"
#include "frontend.h"

// ////////////////////////////////////////////////////////////////////////////
// defines

W_SCREEN  *psRScreen;			// requester stuff.

extern char	MultiCustomMapsPath[PATH_MAX];

bool	MultiMenuUp			= false;
static UDWORD	context = 0;
UDWORD	current_tech = 1;
UDWORD	current_numplayers = 4;
static std::string current_searchString;

#define MULTIMENU_FORM_X		10 + D_W
#define MULTIMENU_FORM_Y		23 + D_H
#define MULTIMENU_FORM_W		620

#define MULTIMENU_PLAYER_H		32
#define MULTIMENU_FONT_OSET		20

#define MULTIMENU_C0			(MULTIMENU_C2+95)
#define MULTIMENU_C1			30
#define MULTIMENU_C2			(MULTIMENU_C1+30)
#define MULTIMENU_C3			(MULTIMENU_C0+36)
#define MULTIMENU_C4			(MULTIMENU_C3+36)
#define MULTIMENU_C5			(MULTIMENU_C4+32)
#define MULTIMENU_C6			(MULTIMENU_C5+32)
#define MULTIMENU_C7			(MULTIMENU_C6+32)
#define MULTIMENU_C8			(MULTIMENU_C7+45)
#define MULTIMENU_C9			(MULTIMENU_C8+95)
#define MULTIMENU_C10			(MULTIMENU_C9+50)
#define MULTIMENU_C11			(MULTIMENU_C10+45)

#define MULTIMENU_CLOSE			(MULTIMENU+1)
#define MULTIMENU_PLAYER		(MULTIMENU+2)

#define	MULTIMENU_ALLIANCE_BASE (MULTIMENU_PLAYER        +MAX_PLAYERS)
#define	MULTIMENU_GIFT_RAD		(MULTIMENU_ALLIANCE_BASE +MAX_PLAYERS)
#define	MULTIMENU_GIFT_RES		(MULTIMENU_GIFT_RAD		 +MAX_PLAYERS)
#define	MULTIMENU_GIFT_DRO		(MULTIMENU_GIFT_RES		 +MAX_PLAYERS)
#define	MULTIMENU_GIFT_POW		(MULTIMENU_GIFT_DRO		 +MAX_PLAYERS)
#define MULTIMENU_CHANNEL		(MULTIMENU_GIFT_POW		 +MAX_PLAYERS)

/// requester stuff.
#define M_REQUEST_CLOSE (MULTIMENU+49)
#define M_REQUEST		(MULTIMENU+50)

#define M_REQUEST_C1	(MULTIMENU+61)
#define M_REQUEST_C2	(MULTIMENU+62)
#define M_REQUEST_C3	(MULTIMENU+63)

#define M_REQUEST_AP	(MULTIMENU+70)
#define M_REQUEST_2P	(MULTIMENU+71)
#define M_REQUEST_3P	(MULTIMENU+72)
#define M_REQUEST_4P	(MULTIMENU+73)
#define M_REQUEST_5P	(MULTIMENU+74)
#define M_REQUEST_6P	(MULTIMENU+75)
#define M_REQUEST_7P	(MULTIMENU+76)
#define M_REQUEST_8P	(MULTIMENU+77)
#define M_REQUEST_9P    (MULTIMENU+78)
#define M_REQUEST_10P   (MULTIMENU+79)
#define M_REQUEST_11P   (MULTIMENU+80)
#define M_REQUEST_12P   (MULTIMENU+81)
#define M_REQUEST_13P   (MULTIMENU+82)
#define M_REQUEST_14P   (MULTIMENU+83)
#define M_REQUEST_15P   (MULTIMENU+84)
#define M_REQUEST_16P   (MULTIMENU+85)
static const unsigned M_REQUEST_NP[] = {M_REQUEST_2P,    M_REQUEST_3P,    M_REQUEST_4P,    M_REQUEST_5P,    M_REQUEST_6P,    M_REQUEST_7P,    M_REQUEST_8P,    M_REQUEST_9P,    M_REQUEST_10P,    M_REQUEST_11P,    M_REQUEST_12P,    M_REQUEST_13P,    M_REQUEST_14P,    M_REQUEST_15P,    M_REQUEST_16P};

#define M_REQUEST_BUT	(MULTIMENU+100)		// allow loads of buttons.
#define M_REQUEST_BUTM	(MULTIMENU+1100)

#define M_REQUEST_X		MULTIOP_PLAYERSX
#define M_REQUEST_Y		MULTIOP_PLAYERSY
#define M_REQUEST_W		MULTIOP_PLAYERSW
#define M_REQUEST_H		MULTIOP_PLAYERSH

#define	R_BUT_W			105//112
#define R_BUT_H			30

#define HOVER_PREVIEW_TIME 300

bool			multiRequestUp = false;				//multimenu is up.
static unsigned         hoverPreviewId;
static bool		giftsUp[MAX_PLAYERS] = {true};		//gift buttons for player are up.

// ////////////////////////////////////////////////////////////////////////////
// Map / force / name load save stuff.

/* Sets the player's text color depending on if alliance formed, or if dead.
 * \param mode  the specified alliance
 * \param player the specified player
 */
static void SetPlayerTextColor( int mode, UDWORD player )
{
	// override color if they are dead...
	if (!apsDroidLists[player] && !apsStructLists[player])
	{
		iV_SetTextColour(WZCOL_GREY);			// dead text color
	}
	// the colors were chosen to match the FRIEND/FOE radar map colors.
	else if (mode == ALLIANCE_FORMED)
	{
		iV_SetTextColour(WZCOL_YELLOW);			// Human alliance text color
	}
	else if (isHumanPlayer(player))				// Human player, no alliance
	{
		iV_SetTextColour(WZCOL_TEXT_BRIGHT);	// Normal text color
	}
	else
	{
		iV_SetTextColour(WZCOL_RED);			// Enemy color
	}
}
// ////////////////////////////////////////////////////////////////////////////
// enumerates maps in the gamedesc file.
// returns only maps that are valid the right 'type'
static LEVEL_DATASET *enumerateMultiMaps(bool first, unsigned camToUse, unsigned numPlayers)
{
	static LEVEL_DATASET *lev;
	unsigned int cam;

	if(first)
	{
		lev = psLevels;
	}
	while(lev)
	{
		if (game.type == SKIRMISH)
		{
			if (lev->type == MULTI_SKIRMISH2)
			{
				cam = 2;
			}
			else if (lev->type == MULTI_SKIRMISH3)
			{
				cam = 3;
			}
			else
			{
				cam = 1;
			}
			if((lev->type == SKIRMISH || lev->type == MULTI_SKIRMISH2 || lev->type == MULTI_SKIRMISH3)
				&& (numPlayers == 0 || numPlayers == lev->players)
				&& cam == camToUse )
			{
				LEVEL_DATASET *found = lev;

				lev = lev->psNext;
				return found;
			}
		}
		lev = lev->psNext;
	}

	return NULL;
}

// ////////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////////
void displayRequestOption(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset)
{
	LEVEL_DATASET *mapData = (LEVEL_DATASET *)psWidget->pUserData;

	int x = xOffset + psWidget->x();
	int y = yOffset + psWidget->y();
	char  butString[255];

	sstrcpy(butString, ((W_BUTTON *)psWidget)->pTip.toUtf8().constData());

	drawBlueBox(x, y, psWidget->width(), psWidget->height());

	iV_SetFont(font_regular);					// font

	if (mapData && CheckForMod(mapData->realFileName))
	{
		iV_SetTextColour(WZCOL_RED);
	}
	else
	{
		iV_SetTextColour(WZCOL_TEXT_BRIGHT);
	}


	while (iV_GetTextWidth(butString) > psWidget->width() - 10)
	{
		butString[strlen(butString)-1]='\0';
	}

	iV_DrawText(butString, x + 6, y + 12);	//draw text

	if (mapData != NULL)
	{
		// Display map hash, so we can see the difference between identically named maps.
		Sha256 hash = mapData->realFileHash;  // levGetFileHash can be slightly expensive.
		static uint32_t lastHashTime = 0;
		if (lastHashTime != realTime && hash.isZero())
		{
			hash = levGetFileHash(mapData);
			if (!hash.isZero())
			{
				lastHashTime = realTime;  // We just calculated a hash. Don't calculate any more hashes this frame.
			}
		}
		if (!hash.isZero())
		{
			iV_SetFont(font_small);
			iV_SetTextColour(WZCOL_TEXT_DARK);
			sstrcpy(butString, hash.toString().c_str());
			while (iV_GetTextWidth(butString) > psWidget->width() - 10 - (8 + mapData->players*6))
			{
				butString[strlen(butString) - 1] = '\0';
			}
			iV_DrawText(butString, x + 6 + 8 + mapData->players*6, y + 26);
			iV_SetFont(font_regular);
		}

		// if map, then draw no. of players.
		for (int count = 0; count < mapData->players; ++count)
		{
			iV_DrawImage(FrontImages, IMAGE_WEE_GUY, x + 6*count + 6, y + 16);
		}
	}
}

// ////////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////////

static void displayCamTypeBut(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset)
{
	int x = xOffset + psWidget->x();
	int y = yOffset + psWidget->y();
	char buffer[8];

	iV_SetFont(font_regular);
	drawBlueBox(x, y, psWidget->width(), psWidget->height());
	sprintf(buffer, "T%i", (int)(psWidget->UserData));
	if ((unsigned int)(psWidget->UserData) == current_tech) {
		iV_SetTextColour(WZCOL_TEXT_BRIGHT);
	} else {
		iV_SetTextColour(WZCOL_TEXT_MEDIUM);
	}
	iV_DrawText(buffer, x+2, y+12);
}

static void displayNumPlayersBut(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset)
{
	int x = xOffset + psWidget->x();
	int y = yOffset + psWidget->y();
	char buffer[8];

	iV_SetFont(font_regular);
	drawBlueBox(x, y, psWidget->width(), psWidget->height());
	if ((unsigned int)(psWidget->UserData) == current_numplayers) {
		iV_SetTextColour(WZCOL_TEXT_BRIGHT);
	} else {
		iV_SetTextColour(WZCOL_TEXT_MEDIUM);
	}
	if ((unsigned int)(psWidget->UserData) == 0) {
		sprintf(buffer, " *");
	} else {
		sprintf(buffer, "%iP", (int)(psWidget->UserData));
		buffer[2] = '\0';  // Truncate 'P' if 2 digits, since there isn't room.
	}
	iV_DrawText(buffer, x+2, y+12);

}

static int stringRelevance(std::string const &string, std::string const &search)
{
	QString str = QString::fromUtf8(string.c_str()).normalized(QString::NormalizationForm_KD);
	QString sea = QString::fromUtf8(search.c_str()).normalized(QString::NormalizationForm_KD);
	int strDim = str.size() + 1;
	int seaDim = sea.size() + 1;

	if (strDim > 10000 || seaDim > 10000 || strDim*seaDim > 100000)
	{
		return 0;
	}

	std::vector<unsigned> scores(strDim*seaDim);
	for (int sum = 0; sum <= str.size() + sea.size(); ++sum)
		for (int iStr = std::max(0, sum - sea.size()); iStr <= std::min(str.size(), sum - 0); ++iStr)
	{
		int iSea = sum - iStr;
		unsigned score = 0;
		if (iStr > 0 && iSea > 0)
		{
			score = (scores[iStr - 1 + (iSea - 1)*strDim] + 1) | 1;
			QChar a = str[iStr - 1], b = sea[iSea - 1];
			if (a == b)
			{
				score += 100;
			}
			else if (a.toUpper() == b.toUpper())
			{
				score += 80;
			}
		}
		if (iStr > 0)
		{
			score = std::max(score, scores[iStr - 1 + iSea*strDim] & ~1);
		}
		if (iSea > 0)
		{
			score = std::max(score, scores[iStr + (iSea - 1)*strDim] & ~1);
		}
		scores[iStr + iSea*strDim] = score;
	}

	return scores[str.size() + sea.size()*strDim];
}

static bool reverseSortByFirst(std::pair<int, W_BUTTON *> const &a, std::pair<int, W_BUTTON *> const &b)
{
	return a.first > b.first;
}

/** Searches in the given search directory for files ending with the
 *  given extension. Then will create a window with buttons for each
 *  found file.
 *  \param searchDir the directory to search in
 *  \param fileExtension the extension files should end with, if the
 *         extension has a dot (.) then this dot _must_ be present as
 *         the first char in this parameter
 *  \param mode (purpose unknown)
 *  \param numPlayers (purpose unknown)
 */
void addMultiRequest(const char* searchDir, const char* fileExtension, UDWORD mode, UBYTE mapCam, UBYTE numPlayers, std::string const &searchString)
{
	const unsigned int extensionLength = strlen(fileExtension);
	const unsigned int buttonsX = (mode == MULTIOP_MAP) ? 22 : 17;

	context = mode;
	if(mode == MULTIOP_MAP)
	{	// only save these when they select MAP button
		current_tech = mapCam;
		current_numplayers = numPlayers;
		current_searchString = searchString;
	}
	char **fileList = PHYSFS_enumerateFiles(searchDir);

	psRScreen = new W_SCREEN; ///< move this to intinit or somewhere like that.. (close too.)

	WIDGET *parent = psRScreen->psForm;

	/* add a form to place the tabbed form on */
	IntFormAnimated *requestForm = new IntFormAnimated(parent);
	requestForm->id = M_REQUEST;
	requestForm->setGeometry(M_REQUEST_X + D_W, M_REQUEST_Y + D_H, M_REQUEST_W, M_REQUEST_H);

	// Add the button list.
	IntListTabWidget *requestList = new IntListTabWidget(requestForm);
	requestList->setChildSize(R_BUT_W, R_BUT_H);
	requestList->setChildSpacing(4, 4);
	requestList->setGeometry(2 + buttonsX, 2, M_REQUEST_W - buttonsX, M_REQUEST_H - 4);

	// Add the close button.
	W_BUTINIT sButInit;
	sButInit.formID = M_REQUEST;
	sButInit.id = M_REQUEST_CLOSE;
	sButInit.x = M_REQUEST_W - CLOSE_WIDTH - 3;
	sButInit.y = 0;
	sButInit.width = CLOSE_WIDTH;
	sButInit.height = CLOSE_HEIGHT;
	sButInit.pTip = _("Close");
	sButInit.pDisplay = intDisplayImageHilight;
	sButInit.UserData = PACKDWORD_TRI(0,IMAGE_CLOSEHILIGHT , IMAGE_CLOSE);
	widgAddButton(psRScreen, &sButInit);

	// Put the buttons on it.
	int nextButtonId = M_REQUEST_BUT;
	for (char **currFile = fileList; *currFile != NULL; ++currFile)
	{
		const unsigned int fileNameLength = strlen(*currFile);

		// Check to see if this file matches the given extension
		if (fileNameLength <= extensionLength
		 || strcmp(&(*currFile)[fileNameLength - extensionLength], fileExtension) != 0)
			continue;

		char *withoutExtension = strdup(*currFile);
		withoutExtension[fileNameLength - extensionLength] = '\0';

		// Set the tip and add the button
		W_BUTTON *button = new W_BUTTON(requestList);
		button->id = nextButtonId;
		button->setTip(withoutExtension);
		button->setString(withoutExtension);
		button->displayFunction = displayRequestOption;
		requestList->addWidgetToLayout(button);

		free(withoutExtension);

		/* Update the init struct for the next button */
		++nextButtonId;
	}

	// Make sure to return memory back to PhyscisFS
	PHYSFS_freeList(fileList);

	multiRequestUp = true;
	hoverPreviewId = 0;

	if(mode == MULTIOP_MAP)
	{
		LEVEL_DATASET *mapData;
		std::vector<std::pair<int, W_BUTTON *> > buttons;
		bool first = true;
		while ((mapData = enumerateMultiMaps(first, mapCam, numPlayers)) != NULL)
		{
			first = false;

			// add number of players to string.
			W_BUTTON *button = new W_BUTTON(requestList);
			button->id = nextButtonId;
			button->setTip(mapData->pName);
			button->setString(mapData->pName);
			button->pUserData = mapData;
			button->displayFunction = displayRequestOption;
			buttons.push_back(std::make_pair(stringRelevance(mapData->pName, searchString), button));

			++nextButtonId;
		}
		std::stable_sort(buttons.begin(), buttons.end(), reverseSortByFirst);
		for (std::vector<std::pair<int, W_BUTTON *> >::const_iterator i = buttons.begin(); i != buttons.end(); ++i)
		{
			requestList->addWidgetToLayout(i->second);
		}

		// if it's map select then add the cam style buttons.
		sButInit = W_BUTINIT();
		sButInit.formID		= M_REQUEST;
		sButInit.id		= M_REQUEST_C1;
		sButInit.x              = 3;
		sButInit.y              = 254;
		sButInit.width		= 17;
		sButInit.height		= 17;
		sButInit.UserData	= 1;
		sButInit.pTip		= _("Technology level 1");
		sButInit.pDisplay	= displayCamTypeBut;

		widgAddButton(psRScreen, &sButInit);

		sButInit.id		= M_REQUEST_C2;
		sButInit.y		+= 22;
		sButInit.UserData	= 2;
		sButInit.pTip		= _("Technology level 2");
		widgAddButton(psRScreen, &sButInit);

		sButInit.id		= M_REQUEST_C3;
		sButInit.y		+= 22;
		sButInit.UserData	= 3;
		sButInit.pTip		= _("Technology level 3");
		widgAddButton(psRScreen, &sButInit);

		sButInit.id		= M_REQUEST_AP;
		sButInit.y		= 17;
		sButInit.UserData	= 0;
		sButInit.pTip		= _("Any number of players");
		sButInit.pDisplay	= displayNumPlayersBut;
		widgAddButton(psRScreen, &sButInit);

		STATIC_ASSERT(MAX_PLAYERS_IN_GUI <= ARRAY_SIZE(M_REQUEST_NP) + 1);
		for (unsigned numPlayers = 2; numPlayers <= MAX_PLAYERS_IN_GUI; ++numPlayers)
		{
			static char ttip[MAX_PLAYERS_IN_GUI + 1][20];
			sButInit.id             = M_REQUEST_NP[numPlayers - 2];
			sButInit.y		+= 22;
			sButInit.UserData	= numPlayers;
			ssprintf(ttip[numPlayers], ngettext("%d player", "%d players", numPlayers), numPlayers);
			sButInit.pTip		= ttip[numPlayers];
			widgAddButton(psRScreen, &sButInit);
		}
	}
}

static void closeMultiRequester(void)
{
	multiRequestUp = false;
	resetReadyStatus(false);
	delete psRScreen;  // move this to the frontend shutdown...
	psRScreen = NULL;
	return;
}

bool runMultiRequester(UDWORD id, UDWORD *mode, QString *chosen, LEVEL_DATASET **chosenValue, bool *isHoverPreview)
{
	static unsigned hoverId = 0;
	static unsigned hoverStartTime = 0;

	if( (id == M_REQUEST_CLOSE) || CancelPressed() )			// user hit close box || hit the cancel key
	{
		closeMultiRequester();
		*mode = 0;
		return true;
	}

	bool hoverPreview = false;
	if (id == 0 && context == MULTIOP_MAP)
	{
		id = widgGetMouseOver(psRScreen);
		if (id != hoverId)
		{
			hoverId = id;
			hoverStartTime = wzGetTicks() + HOVER_PREVIEW_TIME;
		}
		if (id == hoverPreviewId || hoverStartTime > wzGetTicks())
		{
			id = 0;  // Don't re-render preview nor render preview before HOVER_PREVIEW_TIME.
		}
		hoverPreview = true;
	}
	if (id >= M_REQUEST_BUT && id <= M_REQUEST_BUTM)  // chose a file.
	{
		*chosen = ((W_BUTTON *)widgGetFromID(psRScreen,id))->pText;

		*chosenValue = (LEVEL_DATASET *)((W_BUTTON *)widgGetFromID(psRScreen,id))->pUserData;
		*mode = context;
		*isHoverPreview = hoverPreview;
		hoverPreviewId = id;
		if (!hoverPreview)
		{
			closeMultiRequester();
		}

		return true;
	}
	if (hoverPreview)
	{
		id = 0;
	}

	switch (id)
	{
		case M_REQUEST_C1:
			closeMultiRequester();
			addMultiRequest(MultiCustomMapsPath, ".wrf", MULTIOP_MAP, 1, current_numplayers, current_searchString);
			break;
		case M_REQUEST_C2:
			closeMultiRequester();
			addMultiRequest(MultiCustomMapsPath, ".wrf", MULTIOP_MAP, 2, current_numplayers, current_searchString);
			break;
		case M_REQUEST_C3:
			closeMultiRequester();
			addMultiRequest(MultiCustomMapsPath, ".wrf", MULTIOP_MAP, 3, current_numplayers, current_searchString);
			break;
		case M_REQUEST_AP:
			closeMultiRequester();
			addMultiRequest(MultiCustomMapsPath, ".wrf", MULTIOP_MAP, current_tech, 0, current_searchString);
			break;
		default:
			for (unsigned numPlayers = 2; numPlayers <= MAX_PLAYERS_IN_GUI; ++numPlayers)
			{
				if (id == M_REQUEST_NP[numPlayers - 2])
				{
					closeMultiRequester();
					addMultiRequest(MultiCustomMapsPath, ".wrf", MULTIOP_MAP, current_tech, numPlayers, current_searchString);
					break;
				}
			}
			break;
	}

	return false;
}


// ////////////////////////////////////////////////////////////////////////////
// Multiplayer in game menu stuff

// ////////////////////////////////////////////////////////////////////////////
// Display Functions


static void displayExtraGubbins(UDWORD height)
{
	char str[128];

	//draw grid
	iV_Line(MULTIMENU_FORM_X+MULTIMENU_C0 -6 , MULTIMENU_FORM_Y,
			MULTIMENU_FORM_X+MULTIMENU_C0 -6 , MULTIMENU_FORM_Y+height, WZCOL_BLACK);

	iV_Line(MULTIMENU_FORM_X+MULTIMENU_C8 -6 , MULTIMENU_FORM_Y,
			MULTIMENU_FORM_X+MULTIMENU_C8 -6 , MULTIMENU_FORM_Y+height, WZCOL_BLACK);

	iV_Line(MULTIMENU_FORM_X+MULTIMENU_C9 -6 , MULTIMENU_FORM_Y,
			MULTIMENU_FORM_X+MULTIMENU_C9 -6 , MULTIMENU_FORM_Y+height, WZCOL_BLACK);

	iV_Line(MULTIMENU_FORM_X+MULTIMENU_C10 -6 , MULTIMENU_FORM_Y,
			MULTIMENU_FORM_X+MULTIMENU_C10 -6 , MULTIMENU_FORM_Y+height, WZCOL_BLACK);

	iV_Line(MULTIMENU_FORM_X+MULTIMENU_C11 -6 , MULTIMENU_FORM_Y,
			MULTIMENU_FORM_X+MULTIMENU_C11 -6 , MULTIMENU_FORM_Y+height, WZCOL_BLACK);

	iV_Line(MULTIMENU_FORM_X				, MULTIMENU_FORM_Y+MULTIMENU_PLAYER_H,
			MULTIMENU_FORM_X+MULTIMENU_FORM_W, MULTIMENU_FORM_Y+MULTIMENU_PLAYER_H, WZCOL_BLACK);

	iV_SetFont(font_regular);						// font
	iV_SetTextColour(WZCOL_TEXT_BRIGHT);			// main wz text color

	// draw timer
	getAsciiTime(str, gameTime);
	iV_DrawText(str, MULTIMENU_FORM_X+MULTIMENU_C2, MULTIMENU_FORM_Y+MULTIMENU_FONT_OSET) ;

	// draw titles.
	iV_DrawText(_("Alliances"), MULTIMENU_FORM_X+MULTIMENU_C0, MULTIMENU_FORM_Y+MULTIMENU_FONT_OSET);
	iV_DrawText(_("Score"), MULTIMENU_FORM_X+MULTIMENU_C8, MULTIMENU_FORM_Y+MULTIMENU_FONT_OSET);
	iV_DrawText(_("Kills"), MULTIMENU_FORM_X+MULTIMENU_C9, MULTIMENU_FORM_Y+MULTIMENU_FONT_OSET);
	iV_DrawText(_("Units"), MULTIMENU_FORM_X+MULTIMENU_C10, MULTIMENU_FORM_Y+MULTIMENU_FONT_OSET);

	if (getDebugMappingStatus())
	{
		iV_DrawText(_("Power"), MULTIMENU_FORM_X+MULTIMENU_C11, MULTIMENU_FORM_Y+MULTIMENU_FONT_OSET);
	}
	else
	{
		// ping is useless for non MP games, so display something useful depending on mode.
		if (runningMultiplayer())
		{
			iV_DrawText(_("Ping"), MULTIMENU_FORM_X+MULTIMENU_C11, MULTIMENU_FORM_Y+MULTIMENU_FONT_OSET);
		}
		else
		{
			iV_DrawText(_("Structs"), MULTIMENU_FORM_X+MULTIMENU_C11, MULTIMENU_FORM_Y+MULTIMENU_FONT_OSET);
		}
	}

#ifdef DEBUG
	iV_SetFont(font_small);
	for (unsigned q = 0; q < 2; ++q)
	{
		unsigned xPos = 0;
		unsigned yPos = q*12;
		bool isTotal = q != 0;

		char const *srText[2] = {_("Sent/Received per sec —"), _("Total Sent/Received —")};
		sprintf(str, srText[q]);
		iV_DrawText(str, MULTIMENU_FORM_X + xPos, MULTIMENU_FORM_Y + height + yPos);
		xPos += iV_GetTextWidth(str) + 20;

		sprintf(str, _("Traf: %u/%u"), NETgetStatistic(NetStatisticRawBytes, true, isTotal), NETgetStatistic(NetStatisticRawBytes, false, isTotal));
		iV_DrawText(str, MULTIMENU_FORM_X + xPos, MULTIMENU_FORM_Y + height + yPos);
		xPos += iV_GetTextWidth(str) + 20;

		sprintf(str, _("Uncompressed: %u/%u"), NETgetStatistic(NetStatisticUncompressedBytes, true, isTotal), NETgetStatistic(NetStatisticUncompressedBytes, false, isTotal));
		iV_DrawText(str, MULTIMENU_FORM_X + xPos, MULTIMENU_FORM_Y + height + yPos);
		xPos += iV_GetTextWidth(str) + 20;

		sprintf(str, _("Pack: %u/%u"), NETgetStatistic(NetStatisticPackets, true, isTotal), NETgetStatistic(NetStatisticPackets, false, isTotal));
		iV_DrawText(str, MULTIMENU_FORM_X + xPos, MULTIMENU_FORM_Y + height + yPos);
	}
#endif
	return;
}


static void displayMultiPlayer(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset)
{
	char str[128];
	int x = xOffset + psWidget->x();
	int y = yOffset + psWidget->y();
	unsigned player = psWidget->UserData;  // Get the in game player number.

	if (responsibleFor(player, 0))
	{
		displayExtraGubbins(widgGetFromID(psWScreen,MULTIMENU_FORM)->height());
	}

	iV_SetFont(font_regular);  // font
	iV_SetTextColour(WZCOL_TEXT_BRIGHT);

	const bool isHuman = isHumanPlayer(player);
	const bool isAlly = aiCheckAlliances(selectedPlayer, player);
	const bool isSelectedPlayer = player == selectedPlayer;

	SetPlayerTextColor(alliances[selectedPlayer][player], player);

	if (isHuman || (game.type == SKIRMISH && player<game.maxPlayers) )
	{
		ssprintf(str, "%d: %s", NetPlay.players[player].position, getPlayerName(player));

		while (iV_GetTextWidth(str) >= MULTIMENU_C0 - MULTIMENU_C2 - 10)
		{
			str[strlen(str) - 1] = '\0';
		}
		iV_DrawText(str, x + MULTIMENU_C2, y + MULTIMENU_FONT_OSET);

		//c3-7 alliance
		//manage buttons by showing or hiding them. gifts only in campaign,
		if (alliancesCanGiveAnything(game.alliance))
		{
			if (isAlly && !isSelectedPlayer && !giftsUp[player] )
			{
				if (alliancesCanGiveResearchAndRadar(game.alliance))
				{
					widgReveal(psWScreen, MULTIMENU_GIFT_RAD + player);
					widgReveal(psWScreen, MULTIMENU_GIFT_RES + player);
				}
				widgReveal(psWScreen, MULTIMENU_GIFT_DRO + player);
				widgReveal(psWScreen, MULTIMENU_GIFT_POW + player);
				giftsUp[player] = true;
			}
			else if (!isAlly && !isSelectedPlayer && giftsUp[player])
			{
				if (alliancesCanGiveResearchAndRadar(game.alliance))
				{
					widgHide(psWScreen, MULTIMENU_GIFT_RAD + player);
					widgHide(psWScreen, MULTIMENU_GIFT_RES + player);
				}
				widgHide(psWScreen, MULTIMENU_GIFT_DRO + player);
				widgHide(psWScreen, MULTIMENU_GIFT_POW + player);
				giftsUp[player] = false;
			}
		}
	}

	// Let's use the real score for MP games
	if (NetPlay.bComms)
	{
		if (Cheated)
		{
			sprintf(str, "(cheated)");
		}
		else
		{
			sprintf(str, "%d", getMultiStats(player).recentScore);
		}
		iV_DrawText(str, x + MULTIMENU_C8, y + MULTIMENU_FONT_OSET);

		//c9:kills,
		sprintf(str, "%d", getMultiStats(player).recentKills);
		iV_DrawText(str, x + MULTIMENU_C9, y + MULTIMENU_FONT_OSET);
	}
	else
	{
		// estimate of score for skirmish games
		sprintf(str, "%d", ingame.skScores[player][0]);
		iV_DrawText(str, x + MULTIMENU_C8, y + MULTIMENU_FONT_OSET);
		// estimated kills
		sprintf(str, "%d", ingame.skScores[player][1]);
		iV_DrawText(str, x + MULTIMENU_C9, y + MULTIMENU_FONT_OSET);
	}

	//only show player's and allies' unit counts, and nobody elses.
	//c10:units
	if (isAlly || getDebugMappingStatus())
	{
		sprintf(str, "%d", getNumDroids(player) + getNumTransporterDroids(player));
		iV_DrawText(str, x + MULTIMENU_C10, y + MULTIMENU_FONT_OSET);
	}

	/* Display player power instead of number of played games
	  * and number of units instead of ping when in debug mode
	  */
	if (getDebugMappingStatus())  //Won't pass this when in both release and multiplayer modes
	{
		//c11: Player power
		sprintf(str, "%u", (int)getPower(player));
		iV_DrawText(str, MULTIMENU_FORM_X + MULTIMENU_C11, y + MULTIMENU_FONT_OSET);
	}
	else if (runningMultiplayer())
	{
		//c11:ping
		if (!isSelectedPlayer && isHuman)
		{
			if (ingame.PingTimes[player] < PING_LIMIT)
			{
				sprintf(str, "%03d", ingame.PingTimes[player]);
			}
			else
			{
				sprintf(str, "∞");
			}
			iV_DrawText(str, x + MULTIMENU_C11, y + MULTIMENU_FONT_OSET);
		}
	}
	else
	{
		//c11: Structures
		if (isAlly || getDebugMappingStatus())
		{
			// NOTE, This tallys up *all* the structures you have. Test out via 'start with no base'.
			int num = 0;
			for (STRUCTURE *temp = apsStructLists[player]; temp != NULL; temp = temp->psNext)
			{
				++num;
			}
			sprintf(str, "%d", num);
			iV_DrawText(str, x + MULTIMENU_C11, y + MULTIMENU_FONT_OSET);
		}
	}

	// a droid of theirs.
	DROID *displayDroid = apsDroidLists[player];
	while (displayDroid != NULL && !displayDroid->visible[selectedPlayer])
	{
		displayDroid = displayDroid->psNext;
	}
	if (displayDroid)
	{
		pie_SetGeometricOffset( MULTIMENU_FORM_X+MULTIMENU_C1 ,y+MULTIMENU_PLAYER_H);
		Vector3i rotation(-15, 45, 0);
		Position position(0, 0, BUTTON_DEPTH);  // Scale them.
		if (displayDroid->droidType == DROID_SUPERTRANSPORTER)
		{
			position.z = 7850;
		}
		else if (displayDroid->droidType == DROID_TRANSPORTER)
		{
			position.z = 4100;
		}

		displayComponentButtonObject(displayDroid, &rotation, &position, 100);
	}
	else if (apsDroidLists[player])
	{
		// Show that they have droids, but not which droids, since we can't see them.
		iV_DrawImageTc(IntImages, IMAGE_GENERIC_TANK, IMAGE_GENERIC_TANK_TC, MULTIMENU_FORM_X + MULTIMENU_C1 - iV_GetImageWidth(IntImages, IMAGE_GENERIC_TANK)/2, y + MULTIMENU_PLAYER_H - iV_GetImageHeight(IntImages, IMAGE_GENERIC_TANK), pal_GetTeamColour(getPlayerColour(player)));
	}

	// clean up widgets if player leaves while menu is up.
	if (!isHuman && !(game.type == SKIRMISH && player < game.maxPlayers))
	{
		if (widgGetFromID(psWScreen, MULTIMENU_CHANNEL + player) != NULL)
		{
			widgDelete(psWScreen, MULTIMENU_CHANNEL + player);
		}

		if (widgGetFromID(psWScreen, MULTIMENU_ALLIANCE_BASE + player) != NULL)
		{
			widgDelete(psWScreen, MULTIMENU_ALLIANCE_BASE + player);
			widgDelete(psWScreen, MULTIMENU_GIFT_RAD + player);
			widgDelete(psWScreen, MULTIMENU_GIFT_RES + player);
			widgDelete(psWScreen, MULTIMENU_GIFT_DRO + player);
			widgDelete(psWScreen, MULTIMENU_GIFT_POW + player);
			giftsUp[player] = false;
		}
	}
}

// ////////////////////////////////////////////////////////////////////////////
// alliance display funcs

static void displayAllianceState(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset)
{
	UDWORD a, b, c, player = psWidget->UserData;
	switch(alliances[selectedPlayer][player])
	{
	case ALLIANCE_BROKEN:
		a = 0;
		b = IMAGE_MULTI_NOAL_HI;
		c = IMAGE_MULTI_NOAL;					// replace with real gfx
		break;
	case ALLIANCE_FORMED:
		a = 0;
		b = IMAGE_MULTI_AL_HI;
		c = IMAGE_MULTI_AL;						// replace with real gfx
		break;
	case ALLIANCE_REQUESTED:
	case ALLIANCE_INVITATION:
		a = 0;
		b = IMAGE_MULTI_OFFAL_HI;
		c = IMAGE_MULTI_OFFAL;						// replace with real gfx
		break;
	default:
		a = 0;
		b = IMAGE_MULTI_NOAL_HI;
		c = IMAGE_MULTI_NOAL;
		break;
	}

	psWidget->UserData = PACKDWORD_TRI(a,b,c);
	intDisplayImageHilight(psWidget,  xOffset,  yOffset);
	psWidget->UserData = player;
}

static void displayChannelState(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset)
{
	UDWORD player = psWidget->UserData;

	if (openchannels[player])
	{
		psWidget->UserData = PACKDWORD_TRI(0, IMAGE_MULTI_CHAN, IMAGE_MULTI_CHAN);
	}
	else
	{
		psWidget->UserData = PACKDWORD_TRI(0, IMAGE_MULTI_NOCHAN, IMAGE_MULTI_NOCHAN);
	}
	intDisplayImageHilight(psWidget, xOffset, yOffset);
	psWidget->UserData = player;
}


// ////////////////////////////////////////////////////////////////////////////

static void addMultiPlayer(UDWORD player,UDWORD pos)
{
	UDWORD			y,id;
	y	= MULTIMENU_PLAYER_H*(pos+1);					// this is the top of the pos.
	id	= MULTIMENU_PLAYER+player;

	// add the whole thing.
	W_FORMINIT sFormInit;
	sFormInit.formID		  = MULTIMENU_FORM;
	sFormInit.id			  = id;
	sFormInit.style			  = WFORM_PLAIN;
	sFormInit.x				  = 2;
	sFormInit.y				  = (short)y;
	sFormInit.width			  = MULTIMENU_FORM_W -4;
	sFormInit.height		  = MULTIMENU_PLAYER_H;
	sFormInit.pDisplay		  = displayMultiPlayer;
	sFormInit.UserData		  = player;
	widgAddForm(psWScreen, &sFormInit);

	W_BUTINIT sButInit;

	// add channel opener.
	if(player != selectedPlayer)
	{
		sButInit.formID = id;
		sButInit.x		= MULTIMENU_C0;
		sButInit.y		= 5;
		sButInit.width	= 35;
		sButInit.height = 24;
		sButInit.id		= MULTIMENU_CHANNEL + player;
		sButInit.pTip	= _("Channel");
		sButInit.pDisplay = displayChannelState;
		sButInit.UserData = player;
		widgAddButton(psWScreen, &sButInit);
	}

	if (alliancesCanGiveAnything(game.alliance) && player != selectedPlayer)
	{
		//alliance
		sButInit.x		= MULTIMENU_C3;
		sButInit.y		= 5;
		sButInit.width	= 35;
		sButInit.height = 24;
		sButInit.id		= MULTIMENU_ALLIANCE_BASE + player;
		sButInit.pTip	= _("Toggle Alliance State");
		sButInit.pDisplay = displayAllianceState;
		sButInit.UserData = player;

		//can't break alliances in 'Locked Teams' mode
		if (!alliancesFixed(game.alliance))
		{
			widgAddButton(psWScreen, &sButInit);
		}

		sButInit.pDisplay = intDisplayImageHilight;

			// add the gift buttons.
			sButInit.y		+= 1;	// move down a wee bit.

		if (alliancesCanGiveResearchAndRadar(game.alliance))
		{
			sButInit.id		= MULTIMENU_GIFT_RAD+ player;
			sButInit.x		= MULTIMENU_C4;
			sButInit.pTip	= _("Give Visibility Report");
			sButInit.UserData = PACKDWORD_TRI(0,IMAGE_MULTI_VIS_HI, IMAGE_MULTI_VIS);
			widgAddButton(psWScreen, &sButInit);

			sButInit.id		= MULTIMENU_GIFT_RES + player;
			sButInit.x		= MULTIMENU_C5;
			sButInit.pTip	= _("Leak Technology Documents");
			sButInit.UserData = PACKDWORD_TRI(0,IMAGE_MULTI_TEK_HI , IMAGE_MULTI_TEK);
			widgAddButton(psWScreen, &sButInit);
		}

			sButInit.id		= MULTIMENU_GIFT_DRO + player;
			sButInit.x		= MULTIMENU_C6;
			sButInit.pTip	= _("Hand Over Selected Units");
			sButInit.UserData = PACKDWORD_TRI(0,IMAGE_MULTI_DRO_HI , IMAGE_MULTI_DRO);
			widgAddButton(psWScreen, &sButInit);

			sButInit.id		= MULTIMENU_GIFT_POW + player;
			sButInit.x		= MULTIMENU_C7;
			sButInit.pTip	= _("Give Power To Player");
			sButInit.UserData = PACKDWORD_TRI(0,IMAGE_MULTI_POW_HI , IMAGE_MULTI_POW);
			widgAddButton(psWScreen, &sButInit);

			giftsUp[player] = true;				// note buttons are up!
	}
}

bool intAddMultiMenu(void)
{
	UDWORD			i;

	//check for already open.
	if(widgGetFromID(psWScreen,MULTIMENU_FORM))
	{
		intCloseMultiMenu();
		return true;
	}

	if (intMode != INT_INTELMAP)
	{
		intResetScreen(false);
	}

	WIDGET *parent = psWScreen->psForm;

	// add form
	IntFormAnimated *multiMenuForm = new IntFormAnimated(parent);
	multiMenuForm->id = MULTIMENU_FORM;
	multiMenuForm->setGeometry(MULTIMENU_FORM_X, MULTIMENU_FORM_Y, MULTIMENU_FORM_W, MULTIMENU_PLAYER_H*game.maxPlayers + MULTIMENU_PLAYER_H + 7);

	// add any players
	for(i=0;i<MAX_PLAYERS;i++)
	{
		if(isHumanPlayer(i) || (game.type == SKIRMISH && i<game.maxPlayers && game.skDiff[i] ) )
		{
			addMultiPlayer(i,NetPlay.players[i].position);
		}
	}

	// Add the close button.
	W_BUTINIT sButInit;
	sButInit.formID = MULTIMENU_FORM;
	sButInit.id = MULTIMENU_CLOSE;
	sButInit.x = MULTIMENU_FORM_W - CLOSE_WIDTH;
	sButInit.y = 0;
	sButInit.width = CLOSE_WIDTH;
	sButInit.height = CLOSE_HEIGHT;
	sButInit.pTip = _("Close");
	sButInit.pDisplay = intDisplayImageHilight;
	sButInit.UserData = PACKDWORD_TRI(0,IMAGE_CLOSEHILIGHT , IMAGE_CLOSE);
	if (!widgAddButton(psWScreen, &sButInit))
	{
		return false;
	}

	intShowPowerBar();						// add power bar

	if (intMode != INT_INTELMAP)
	{
		intMode		= INT_MULTIMENU;			// change interface mode.
	}
	MultiMenuUp = true;
	return true;
}

// ////////////////////////////////////////////////////////////////////////////
void intCloseMultiMenuNoAnim(void)
{
	widgDelete(psWScreen, MULTIMENU_FORM);
	if (!MultiMenuUp)
	{
		return;
	}
	MultiMenuUp = false;
	if (intMode != INT_INTELMAP)
	{
		intMode		= INT_NORMAL;
	}
}


// ////////////////////////////////////////////////////////////////////////////
bool intCloseMultiMenu(void)
{
	if (!MultiMenuUp)
	{
		return true;
	}

	// Start the window close animation.
	IntFormAnimated *form = (IntFormAnimated *)widgGetFromID(psWScreen, MULTIMENU_FORM);
	if (form != nullptr)
	{
		form->closeAnimateDelete();
		MultiMenuUp  = false;
	}

	if (intMode != INT_INTELMAP)
	{
		intMode		= INT_NORMAL;
	}
	return true;
}

// ////////////////////////////////////////////////////////////////////////////
// In Game Options house keeping stuff.
bool intRunMultiMenu(void)
{
	return true;
}

// ////////////////////////////////////////////////////////////////////////////
// process clicks made by user.
void intProcessMultiMenu(UDWORD id)
{
	UBYTE	i;

	//close
	if (id == MULTIMENU_CLOSE)
	{
		intCloseMultiMenu();
	}

	//alliance button
	if(id >=MULTIMENU_ALLIANCE_BASE  &&  id<MULTIMENU_ALLIANCE_BASE+MAX_PLAYERS)
	{
		i =(UBYTE)( id - MULTIMENU_ALLIANCE_BASE);

		switch(alliances[selectedPlayer][i])
		{
		case ALLIANCE_BROKEN:
			requestAlliance((UBYTE)selectedPlayer,i,true,true);			// request an alliance
			break;
		case ALLIANCE_INVITATION:
			formAlliance((UBYTE)selectedPlayer,i,true,true,true);			// form an alliance
			break;
		case ALLIANCE_REQUESTED:
			breakAlliance((UBYTE)selectedPlayer,i,true,true);		// break an alliance
			break;

		case ALLIANCE_FORMED:
			breakAlliance((UBYTE)selectedPlayer,i,true,true);		// break an alliance
			break;
		default:
			break;
		}
	}


	//channel opens.
	if(id >=MULTIMENU_CHANNEL &&  id<MULTIMENU_CHANNEL+MAX_PLAYERS)
	{
		i = id - MULTIMENU_CHANNEL;
		openchannels[i] = !openchannels[i];

		if(mouseDown(MOUSE_RMB) && NetPlay.isHost) // both buttons....
			{
				char buf[250];

				// Allow the host to kick the AI only in a MP game, or if they activated cheats in a skirmish game
				if ((NetPlay.bComms || Cheated) && (NetPlay.players[i].allocated || (NetPlay.players[i].allocated == false && NetPlay.players[i].ai != AI_OPEN)))
				{
					inputLoseFocus();
					ssprintf(buf, _("The host has kicked %s from the game!"), getPlayerName((unsigned int) i));
					sendTextMessage(buf, true);
					ssprintf(buf, _("kicked %s : %s from the game, and added them to the banned list!"), getPlayerName((unsigned int) i), NetPlay.players[i].IPtextAddress);
					NETlogEntry(buf, SYNC_FLAG, (unsigned int) i);
					kickPlayer((unsigned int) i, "you are unwanted by the host.", ERROR_KICKED);
					return;
				}
			}
	}

	//radar gifts
	if(id >=  MULTIMENU_GIFT_RAD && id< MULTIMENU_GIFT_RAD +MAX_PLAYERS)
	{
		sendGift(RADAR_GIFT, id - MULTIMENU_GIFT_RAD);
	}

	// research gift
	if(id >= MULTIMENU_GIFT_RES && id<MULTIMENU_GIFT_RES  +MAX_PLAYERS)
	{
		sendGift(RESEARCH_GIFT, id - MULTIMENU_GIFT_RES);
	}

	//droid gift
	if(id >=  MULTIMENU_GIFT_DRO && id<  MULTIMENU_GIFT_DRO +MAX_PLAYERS)
	{
		sendGift(DROID_GIFT, id - MULTIMENU_GIFT_DRO);
	}

	//power gift
	if(id >=  MULTIMENU_GIFT_POW && id<  MULTIMENU_GIFT_POW +MAX_PLAYERS)
	{
		sendGift(POWER_GIFT, id - MULTIMENU_GIFT_POW);
	}
}
