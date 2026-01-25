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
/*
 *  MultiMenu.c
 *  Handles the In Game MultiPlayer Screen, alliances etc...
 *  Also the selection of disk files..
 */
#include "lib/framework/frame.h"
#include "lib/framework/wzapp.h"
#include "lib/framework/strres.h"
#include "lib/framework/physfs_ext.h"
#include "lib/widget/gridlayout.h"
#include "lib/widget/alignment.h"
#include "lib/widget/resize.h"
#include "lib/widget/margin.h"
#include "lib/widget/button.h"
#include "lib/widget/label.h"
#include "lib/widget/widget.h"
#include "lib/widget/multibutform.h"
#include "lib/widget/paneltabbutton.h"
#include "lib/widget/editbox.h"

#include "display3d.h"
#include "intdisplay.h"
// FIXME Direct iVis implementation include!
#include "lib/ivis_opengl/pieblitfunc.h"
#include "lib/ivis_opengl/pietypes.h"
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
#include "keybind.h"
#include "loop.h"
#include "frontend.h"
#include "hci/teamstrategy.h"
#include "multivote.h"

// ////////////////////////////////////////////////////////////////////////////
// defines

std::shared_ptr<W_SCREEN> psRScreen = nullptr; // requester stuff.

extern char	MultiCustomMapsPath[PATH_MAX];

bool	MultiMenuUp			= false;
static UDWORD	current_context = 0;
UDWORD	current_numplayers = 4;
static std::string current_searchString;

#define MULTIMENU_FORM_Y		25

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

#define MULTIMENU_PLAYER_MUTE_START M_REQUEST_BUTM + 1

#define M_REQUEST_X		MULTIOP_PLAYERSX
#define M_REQUEST_Y		MULTIOP_PLAYERSY
#define M_REQUEST_W		MULTIOP_PLAYERSW
#define M_REQUEST_H		MULTIOP_PLAYERSH

#define	R_BUT_W			120//105
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
static PIELIGHT GetPlayerTextColor(int mode, UDWORD player)
{
	// override color if they are dead...
	if (player >= MAX_PLAYERS || (apsDroidLists[player].empty() && apsStructLists[player].empty()))
	{
		return WZCOL_GREY;			// dead text color
	}
	// the colors were chosen to match the FRIEND/FOE radar map colors.
	else if (mode == ALLIANCE_FORMED)
	{
		return WZCOL_YELLOW;		// Human alliance text color
	}
	else if (isHumanPlayer(player))		// Human player, no alliance
	{
		return WZCOL_TEXT_BRIGHT;	// Normal text color
	}
	else
	{
		return WZCOL_RED;			// Enemy color
	}
}

// ////////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////////

struct DisplayRequestOptionCache {
	Sha256 hash;
	WzText wzHashText;

private:
	WzText wzText;
	std::string _fullButString;
	int _widgetWidth = 0;

public:
	// For utilizing the cached (main) display text
	bool canUseCachedText(const std::string &fullButString, int widgetWidth)
	{
		return (_widgetWidth == widgetWidth) && (_fullButString == fullButString);
	}
	void setCachedText(const std::string &text, iV_fonts fontID, const std::string &fullButString, int widgetWidth)
	{
		wzText.setText(WzString::fromUtf8(text), fontID);
		_fullButString = fullButString;
		_widgetWidth = widgetWidth;
	}
	void renderCachedText(Vector2f position, PIELIGHT colour, float rotation = 0.0f)
	{
		wzText.render(position, colour, rotation);
	}
	void renderCachedText(int x, int y, PIELIGHT colour, float rotation = 0.0f)
	{
		renderCachedText(Vector2f{x,y}, colour, rotation);
	}
};

struct DisplayRequestOptionData {

	DisplayRequestOptionData(LEVEL_DATASET *pMapData = nullptr)
	: pMapData(pMapData)
	, cache()
	{ }

	LEVEL_DATASET				*pMapData;
	DisplayRequestOptionCache	cache;
};

void displayRequestOption(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset)
{
	// Any widget using displayRequestOption must have its pUserData initialized to a (DisplayRequestOptionData*)
	assert(psWidget->pUserData != nullptr);
	DisplayRequestOptionData& data = *static_cast<DisplayRequestOptionData *>(psWidget->pUserData);

	LEVEL_DATASET *mapData = data.pMapData;

	int x = xOffset + psWidget->x();
	int y = yOffset + psWidget->y();
	char  butString[255];

	sstrcpy(butString, ((W_BUTTON *)psWidget)->pTip.c_str());

	drawBlueBox(x, y, psWidget->width(), psWidget->height());

	PIELIGHT colour;
	if (mapData && CheckForMod(mapData->realFileName))
	{
		colour = WZCOL_RED;
	}
	else
	{
		colour = WZCOL_TEXT_BRIGHT;
	}

	if (!data.cache.canUseCachedText(butString, psWidget->width()))
	{
		std::string fullButString = butString;
		while ((int)iV_GetTextWidth(butString, font_regular) > psWidget->width() - 10)
		{
			butString[strlen(butString) - 1] = '\0';
		}
		data.cache.setCachedText(butString, font_regular, fullButString, psWidget->width());
	}

	data.cache.renderCachedText(x + 6, y + 12, colour);	//draw text

	if (mapData != nullptr)
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
			if (hash != data.cache.hash)
			{
				sstrcpy(butString, hash.toString().c_str());
				while ((int)iV_GetTextWidth(butString, font_small) > psWidget->width() - 10 - (8 + mapData->players * 6))
				{
					butString[strlen(butString) - 1] = '\0';
				}

				// Update the cached hash text
				data.cache.hash = hash;
				data.cache.wzHashText.setText(butString, font_small);
			}
			data.cache.wzHashText.render(x + 6 + 8 + mapData->players * 6, y + 26, WZCOL_TEXT_DARK);
		}

		// if map, then draw no. of players.
		for (int count = 0; count < mapData->players; ++count)
		{
			iV_DrawImage(FrontImages, IMAGE_WEE_GUY, x + 6 * count + 6, y + 16);
		}
		if (CheckForRandom(mapData->realFileName, mapData->apDataFiles[0].c_str()))
		{
			iV_DrawImage(FrontImages, IMAGE_WEE_DIE, x + 80 + 6, y + 15);
		}
	}
}

// ////////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////////

struct DisplayNumPlayersButCache {
	WzText wzText;
};

static void displayNumPlayersBut(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset)
{
	// Any widget using displayNumPlayersBut must have its pUserData initialized to a (DisplayNumPlayersButCache*)
	assert(psWidget->pUserData != nullptr);
	DisplayNumPlayersButCache& cache = *static_cast<DisplayNumPlayersButCache*>(psWidget->pUserData);

	int x = xOffset + psWidget->x();
	int y = yOffset + psWidget->y();
	char buffer[8];

	drawBlueBox(x, y, psWidget->width(), psWidget->height());

	PIELIGHT colour;
	if ((unsigned int)(psWidget->UserData) == current_numplayers)
	{
		colour = WZCOL_TEXT_BRIGHT;
	}
	else
	{
		colour = WZCOL_TEXT_MEDIUM;
	}
	if ((unsigned int)(psWidget->UserData) == 0)
	{
		sstrcpy(buffer, " *");
	}
	else
	{
		ssprintf(buffer, "%iP", (int)(psWidget->UserData));
		buffer[2] = '\0';  // Truncate 'P' if 2 digits, since there isn't room.
	}
	cache.wzText.setText(buffer, font_regular);
	cache.wzText.render(x + 2, y + 12, colour);
}

static int stringRelevance(std::string const &string, std::string const &search)
{
	WzString str = WzString::fromUtf8(string).normalized(WzString::NormalizationForm_KD);
	WzString sea = WzString::fromUtf8(search).normalized(WzString::NormalizationForm_KD);
	size_t strDim = static_cast<size_t>(str.size()) + 1;
	size_t seaDim = static_cast<size_t>(sea.size()) + 1;

	if (strDim > 10000 || seaDim > 10000 || strDim * seaDim > 100000)
	{
		return 0;
	}

	std::vector<unsigned> scores(strDim * seaDim);
	for (int sum = 0; sum <= str.size() + sea.size(); ++sum)
		for (int iStr = std::max(0, sum - sea.size()); iStr <= std::min(str.size(), sum - 0); ++iStr)
		{
			int iSea = sum - iStr;
			unsigned score = 0;
			if (iStr > 0 && iSea > 0)
			{
				score = (scores[iStr - 1 + (iSea - 1) * strDim] + 1) | 1;
				WzString::WzUniCodepointRef a = str[iStr - 1], b = sea[iSea - 1];
				if (a == b)
				{
					score += 100;
				}
				else if (a.value().caseFolded() == b.value().caseFolded())
				{
					score += 80;
				}
			}
			if (iStr > 0)
			{
				score = std::max(score, scores[iStr - 1 + iSea * strDim] & ~1);
			}
			if (iSea > 0)
			{
				score = std::max(score, scores[iStr + (iSea - 1) * strDim] & ~1);
			}
			scores[iStr + iSea * strDim] = score;
		}

	return scores[str.size() + sea.size() * strDim];
}

void multiMenuScreenSizeDidChange(unsigned int oldWidth, unsigned int oldHeight, unsigned int newWidth, unsigned int newHeight)
{
	if (psRScreen == nullptr) return;
	psRScreen->screenSizeDidChange(oldWidth, oldHeight, newWidth, newHeight);
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
void addMultiRequest(const char *searchDir, const char *fileExtension, UDWORD mode, UBYTE numPlayers, std::string const &searchString)
{
	const size_t extensionLength = strlen(fileExtension);
	const unsigned int buttonsX = (mode == MULTIOP_MAP) ? 22 : 17;

	current_context = mode;
	if (mode == MULTIOP_MAP)
	{
		// only save these when they select MAP button
		current_numplayers = numPlayers;
		current_searchString = searchString;
	}

	psRScreen = W_SCREEN::make(); ///< move this to intinit or somewhere like that.. (close too.)

	auto backdrop = addBackdrop(psRScreen);
	backdrop->setCalcLayout(calcBackdropLayoutForMultiplayerOptionsTitleUI);

	/* add a form to place the tabbed form on */
	auto requestForm = std::make_shared<IntFormAnimated>();
	backdrop->attach(requestForm);
	requestForm->id = M_REQUEST;
	requestForm->setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
		psWidget->setGeometry(M_REQUEST_X, M_REQUEST_Y, M_REQUEST_W, M_REQUEST_H);
	}));

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
	sButInit.UserData = PACKDWORD_TRI(0, IMAGE_CLOSEHILIGHT , IMAGE_CLOSE);
	widgAddButton(psRScreen, &sButInit);

	// Add the button list.
	auto requestList = IntListTabWidget::make();
	requestForm->attach(requestList);
	requestList->setChildSize(R_BUT_W, R_BUT_H);
	requestList->setChildSpacing(4, 4);
	requestList->setGeometry(2 + buttonsX, 2, sButInit.x - buttonsX - 8, M_REQUEST_H - 4);

	// Put the buttons on it.
	int nextButtonId = M_REQUEST_BUT;
	WZ_PHYSFS_enumerateFiles(searchDir, [&](const char *currFile) -> bool {
		const size_t fileNameLength = strlen(currFile);

		// Check to see if this file matches the given extension
		if (fileNameLength <= extensionLength
		    || strcmp(&(currFile)[fileNameLength - extensionLength], fileExtension) != 0)
		{
			return true; //continue
		}

		char *withoutExtension = strdup(currFile);
		withoutExtension[fileNameLength - extensionLength] = '\0';
		std::string withoutTechlevel = mapNameWithoutTechlevel(withoutExtension);
		free(withoutExtension);

		// Set the tip and add the button
		auto button = std::make_shared<W_BUTTON>();
		requestList->attach(button);
		button->id = nextButtonId;
		button->setTip(withoutTechlevel);
		button->setString(WzString::fromUtf8(withoutTechlevel));
		button->displayFunction = displayRequestOption;
		button->pUserData = new DisplayRequestOptionData();
		button->setOnDelete([](WIDGET *psWidget) {
			assert(psWidget->pUserData != nullptr);
			delete static_cast<DisplayRequestOptionData *>(psWidget->pUserData);
			psWidget->pUserData = nullptr;
		});
		requestList->addWidgetToLayout(button);

		/* Update the init struct for the next button */
		++nextButtonId;
		return true; // continue
	});

	multiRequestUp = true;
	hoverPreviewId = 0;

	if (mode == MULTIOP_MAP)
	{
		// Add the search edit box
		auto searchBox = std::make_shared<W_EDITBOX>();
		requestForm->attach(searchBox);
		searchBox->setGeometry(3, requestForm->height() - MULTIOP_SEARCHBOXH - 3, requestForm->width() - 6, MULTIOP_SEARCHBOXH);
		searchBox->setBoxColours(WZCOL_MENU_BORDER, WZCOL_MENU_BORDER, WZCOL_MENU_BACKGROUND);
		searchBox->setPlaceholder(_("Search for map"));
		searchBox->setString(WzString::fromUtf8(current_searchString));

		searchBox->setOnEditingStoppedHandler([searchDir, fileExtension, mode, numPlayers](W_EDITBOX& widg) {
			std::string value = widg.getString().toUtf8();
			if (value == current_searchString) {
				return;
			}
			closeMultiRequester();
			addMultiRequest(searchDir, fileExtension, mode, numPlayers, value);
		});

		LEVEL_LIST levels = enumerateMultiMaps(game.techLevel, numPlayers);
		using Pair = std::pair<int, std::shared_ptr<W_BUTTON>>;
		std::vector<Pair> buttons;

		for (auto mapData : levels)
		{
			std::string withoutTechlevel = mapNameWithoutTechlevel(mapData->pName.c_str());
			// add number of players to string.
			auto button = std::make_shared<W_BUTTON>();
			requestList->attach(button);
			button->id = nextButtonId;
			button->setTip(withoutTechlevel);
			button->setString(WzString::fromUtf8(withoutTechlevel));
			button->displayFunction = displayRequestOption;
			button->pUserData = new DisplayRequestOptionData(mapData);
			button->setOnDelete([](WIDGET *psWidget) {
				assert(psWidget->pUserData != nullptr);
				delete static_cast<DisplayRequestOptionData *>(psWidget->pUserData);
				psWidget->pUserData = nullptr;
			});
			buttons.push_back({stringRelevance(mapData->pName, searchString), button});

			++nextButtonId;
		}
		std::stable_sort(buttons.begin(), buttons.end(), [](Pair const &a, Pair const &b) {
			return a.first > b.first;
		});
		for (Pair const &p : buttons)
		{
			requestList->addWidgetToLayout(p.second);
		}

		// if it's map select then add the cam style buttons.
		sButInit = W_BUTINIT();
		sButInit.formID		= M_REQUEST;
		sButInit.x              = 3;
		sButInit.width		= 17;
		sButInit.height		= 17;

		sButInit.id		= M_REQUEST_AP;
		sButInit.y		= 17;
		sButInit.UserData	= 0;
		sButInit.pTip		= _("Any number of players");
		sButInit.pDisplay	= displayNumPlayersBut;
		sButInit.initPUserDataFunc = []() -> void * { return new DisplayNumPlayersButCache(); };
		sButInit.onDelete = [](WIDGET *psWidget) {
			assert(psWidget->pUserData != nullptr);
			delete static_cast<DisplayNumPlayersButCache *>(psWidget->pUserData);
			psWidget->pUserData = nullptr;
		};
		widgAddButton(psRScreen, &sButInit);

		STATIC_ASSERT(MAX_PLAYERS_IN_GUI <= ARRAY_SIZE(M_REQUEST_NP) + 1);
		for (unsigned mapNumPlayers = 2; mapNumPlayers <= MAX_PLAYERS_IN_GUI; ++mapNumPlayers)
		{
			char ttip[100] = {0};
			sButInit.id             = M_REQUEST_NP[mapNumPlayers - 2];
			sButInit.y		+= 22;
			sButInit.UserData	= mapNumPlayers;
			ssprintf(ttip, ngettext("%d player", "%d players", mapNumPlayers), mapNumPlayers);
			sButInit.pTip		= ttip;
			widgAddButton(psRScreen, &sButInit);
		}
	}
}

void closeMultiRequester()
{
	multiRequestUp = false;
	resetReadyStatus(false);
	psRScreen = nullptr;
	return;
}

bool runMultiRequester(UDWORD id, UDWORD *mode, WzString *chosen, LEVEL_DATASET **chosenValue, bool *isHoverPreview)
{
	static unsigned hoverId = 0;
	static unsigned hoverStartTime = 0;

	*isHoverPreview = false;

	if ((id == M_REQUEST_CLOSE) || CancelPressed())			// user hit close box || hit the cancel key
	{
		closeMultiRequester();
		*mode = 0;
		return true;
	}

	bool hoverPreview = false;
	if (id == 0 && current_context == MULTIOP_MAP)
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
		*chosen = ((W_BUTTON *)widgGetFromID(psRScreen, id))->pText;

		DisplayRequestOptionData * pData = static_cast<DisplayRequestOptionData *>(((W_BUTTON *)widgGetFromID(psRScreen, id))->pUserData);
		ASSERT_OR_RETURN(false, pData != nullptr, "Unable to get map data pointer for: %s", chosen->toUtf8().c_str());
		if (current_context == MULTIOP_MAP)
		{
			ASSERT_OR_RETURN(false, pData->pMapData != nullptr, "Unable to get map data: %s", chosen->toUtf8().c_str());
			*chosenValue = (LEVEL_DATASET *)pData->pMapData;
		}
		else
		{
			*chosenValue = nullptr;
		}
		*mode = current_context;
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
	case M_REQUEST_AP:
		closeMultiRequester();
		addMultiRequest(MultiCustomMapsPath, ".wrf", MULTIOP_MAP, 0, current_searchString);
		break;
	default:
		for (unsigned numPlayers = 2; numPlayers <= MAX_PLAYERS_IN_GUI; ++numPlayers)
		{
			if (id == M_REQUEST_NP[numPlayers - 2])
			{
				closeMultiRequester();
				addMultiRequest(MultiCustomMapsPath, ".wrf", MULTIOP_MAP, numPlayers, current_searchString);
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
// alliance display funcs

static void displayAllianceState(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset)
{
	if (selectedPlayer >= MAX_PLAYERS) { return; }

	UDWORD a, b, c, player = psWidget->UserData;
	switch (alliances[selectedPlayer][player])
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

	psWidget->UserData = PACKDWORD_TRI(a, b, c);
	intDisplayImageHilight(psWidget,  xOffset,  yOffset);
	psWidget->UserData = player;
}

static void displayChannelState(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset)
{
	UDWORD player = psWidget->UserData;
	ASSERT_OR_RETURN(, player < MAX_CONNECTED_PLAYERS, "invalid player: %" PRIu32 "", player);

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

static void displayMuteState(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset)
{
	UDWORD player = psWidget->UserData;
	ASSERT_OR_RETURN(, player < MAX_CONNECTED_PLAYERS, "invalid player: %" PRIu32 "", player);

	UWORD ImageID = IMAGE_INTFAC_VOLUME_UP;
	if (!ingame.muteChat[player])
	{
		ImageID = IMAGE_INTFAC_VOLUME_UP;
	}
	else
	{
		ImageID = IMAGE_INTFAC_VOLUME_MUTE;
	}
	int x = xOffset + psWidget->x();
	int y = yOffset + psWidget->y();
	bool isHighlighted = (psWidget->getState() & WBUT_HIGHLIGHT) != 0;
	bool isDown = (psWidget->getState() & WBUT_DOWN) != 0;
	if (isHighlighted || isDown)
	{
		pie_UniTransBoxFill(x-2, y-2, x + 16 + 4, y + 16 + 4, (isDown) ? WZCOL_TRANSPARENT_BOX : pal_RGBA(255, 255, 255, 65));
	}
	iV_DrawImageFileAnisotropic(IntImages, ImageID, x + 1, y + 1, Vector2f{16,16});
}


// ////////////////////////////////////////////////////////////////////////////

class MultiMenuDroidView: public WIDGET
{
public:
	MultiMenuDroidView(uint32_t player): player(player)
	{
		setGeometry(0, 0, 60, 32);
	}

	void display(int xOffset, int yOffset) override
	{
		// a droid of theirs.
		DroidList* displayDroidList = (player < MAX_PLAYERS) ? &apsDroidLists[player] : nullptr;
		if (!displayDroidList)
		{
			return;
		}
		auto displayDroidIt = std::find_if(displayDroidList->begin(), displayDroidList->end(), [](DROID* d)
		{
			return d->visibleForLocalDisplay();
		});

		auto centerX = xOffset + x() + width() / 2;
		auto y0 = yOffset + y();
		if (displayDroidIt != displayDroidList->end())
		{
			DROID* displayDroid = *displayDroidIt;
			pie_SetGeometricOffset(centerX, y0 + height() * 3 / 4);
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
		else if ((player < MAX_PLAYERS) && !apsDroidLists[player].empty())
		{
			// Show that they have droids, but not which droids, since we can't see them.
			iV_DrawImageTc(
				IntImages,
				IMAGE_GENERIC_TANK,
				IMAGE_GENERIC_TANK_TC,
				centerX - iV_GetImageWidth(IntImages, IMAGE_GENERIC_TANK) / 2,
				y0 + height() / 2 - iV_GetImageHeight(IntImages, IMAGE_GENERIC_TANK) / 2,
				pal_GetTeamColour(getPlayerColour(player))
			);
		}
	}

private:
	uint32_t player;
};

class MultiMenuGrid: public GridLayout
{
protected:
	MultiMenuGrid(): GridLayout() {}

public:
	static std::shared_ptr<MultiMenuGrid> make()
	{
		class make_shared_enabler: public MultiMenuGrid {};
		auto widget = std::make_shared<make_shared_enabler>();
		widget->initialize();
		return widget;
	}

private:
	const int32_t CELL_HORIZONTAL_PADDING = 8;

	struct PlayerWidgets
	{
		uint32_t player;
		std::shared_ptr<W_LABEL> name;
		std::shared_ptr<W_LABEL> score;
		std::shared_ptr<W_LABEL> kills;
		std::shared_ptr<W_LABEL> units;
		std::shared_ptr<W_LABEL> lastColumn;
	};

	void initialize()
	{
		auto margin = Margin(10, CELL_HORIZONTAL_PADDING);
		auto lastMargin = margin;
		lastMargin.right = CLOSE_WIDTH + 5;
		timerLabel = makeLabel("");
		timerLabel->setCanTruncate(false);
		powerLabel = makeLabel(_("Power"));
		pingLabel = makeLabel(_("Ping"));
		structsLabel = makeLabel(_("Structs"));
		place({0}, {0}, margin.wrap(Alignment::center().wrap(timerLabel)));
		place({1}, {0}, margin.wrap(makeLabel(_("Alliances"))));
		place({2}, {0}, Resize::width(100).wrap(margin.wrap(makeLabel(_("Score")))));
		place({3}, {0}, Resize::width(50).wrap(margin.wrap(makeLabel(_("Kills")))));
		place({4}, {0}, Resize::width(50).wrap(margin.wrap(makeLabel(_("Units")))));
		place({5}, {0}, Resize::width(50).wrap(lastMargin.wrap(powerLabel)));
		place({5}, {0}, Resize::width(50).wrap(lastMargin.wrap(pingLabel)));
		place({5}, {0}, Resize::width(50).wrap(lastMargin.wrap(structsLabel)));
		place({6}, {0}, Resize::width(16).wrap(lastMargin.wrap(makeLabel(""))));

		for (auto player = 0; player < MAX_CONNECTED_PLAYERS; player++)
		{
			if (isHumanPlayer(player) || (game.type == LEVEL_TYPE::SKIRMISH && player < game.maxPlayers && NetPlay.players[player].difficulty != AIDifficulty::DISABLED))
			{
				addPlayerWidgets(player, 1 + NetPlay.players[player].position);
			}
		}
	}

	std::shared_ptr<W_LABEL> makeLabel(const char *text)
	{
		auto label = std::make_shared<W_LABEL>();
		label->setString(text);
		label->setCanTruncate(true);
		label->setFontColour(WZCOL_TEXT_BRIGHT);
		return label;
	}

	std::shared_ptr<WIDGET> wrapGift(std::shared_ptr<WIDGET> gift)
	{
		auto alignment = Alignment(Alignment::Vertical::Center, Alignment::Horizontal::Center);
		return alignment.wrap(Margin(0, 1, 0, 0).wrap(gift));
	}

	void addPlayerWidgets(uint32_t player, uint32_t row)
	{
		auto nameGrid = std::make_shared<GridLayout>();
		char name[128];
		ssprintf(name, "%d: %s", NetPlay.players[player].position, getPlayerName(player));
		nameGrid->place({0, 1, false}, {0}, std::make_shared<MultiMenuDroidView>(player));
		auto nameLabel = makeLabel(name);
		nameGrid->place({1}, {0}, nameLabel);

		auto alliancesGrid = std::make_shared<GridLayout>();

		bool selectedPlayerIsSpectator = NetPlay.players[selectedPlayer].isSpectator;

		// add channel opener.
		// - not configurable for self (obviously)
		// - players shouldn't configure channel to spectators
		if ((player != selectedPlayer) && (selectedPlayerIsSpectator || !NetPlay.players[player].isSpectator))
		{
			W_BUTINIT sButInit;
			sButInit.width	= 35;
			sButInit.height = 24;
			sButInit.id		= MULTIMENU_CHANNEL + player;
			sButInit.pTip	= _("Channel");
			sButInit.pDisplay = displayChannelState;
			sButInit.UserData = player;

			auto channelButton = std::make_shared<W_BUTTON>(&sButInit);
			channelButton->addOnClickHandler([player](W_BUTTON&) {
				UBYTE i = (UBYTE)player;
				openchannels[i] = !openchannels[i];

				if (mouseDown(MOUSE_RMB) && NetPlay.isHost) // both buttons....
				{
					// Allow the host to kick the AI only in a MP game, or if they activated cheats in a skirmish game
					if ((NetPlay.bComms || Cheated) && (NetPlay.players[i].allocated || (NetPlay.players[i].allocated == false && NetPlay.players[i].ai != AI_OPEN)))
					{
						inputLoseFocus();
						startKickVote(static_cast<uint32_t>(i));
						return;
					}
				}
			});
			alliancesGrid->place({0}, {0}, Margin(0, 1, 0, 0).wrap(channelButton));
		}

		if (alliancesCanGiveAnything(game.alliance) && player != selectedPlayer && player < MAX_PLAYERS && !NetPlay.players[player].isSpectator)
		{
			W_BUTINIT sButInit;
			//alliance
			sButInit.id		= MULTIMENU_ALLIANCE_BASE + player;
			sButInit.pTip	= _("Toggle Alliance State");
			sButInit.pDisplay = displayAllianceState;
			sButInit.UserData = player;
			sButInit.width	= 35;
			sButInit.height = 24;

			//can't break alliances in 'Locked Teams' mode
			if (!alliancesFixed(game.alliance))
			{
				auto allianceButton = std::make_shared<W_BUTTON>(&sButInit);
				allianceButton->addOnClickHandler([player](W_BUTTON& widg){
					UBYTE i = (UBYTE)(player);

					switch (alliances[selectedPlayer][i])
					{
					case ALLIANCE_BROKEN:
						requestAlliance((UBYTE)selectedPlayer, i, true, true);			// request an alliance
						break;
					case ALLIANCE_INVITATION:
						formAlliance((UBYTE)selectedPlayer, i, true, true, true);			// form an alliance
						break;
					case ALLIANCE_REQUESTED:
						breakAlliance((UBYTE)selectedPlayer, i, true, true);		// break an alliance
						break;
					case ALLIANCE_FORMED:
						breakAlliance((UBYTE)selectedPlayer, i, true, true);		// break an alliance
						break;
					default:
						break;
					}
				});
				alliancesGrid->place({1}, {0}, Margin(0, 1, 0, 0).wrap(allianceButton));
			}

			sButInit.pDisplay = intDisplayImageHilight;

			// add the gift buttons.
			sButInit.width	= 31;
			sButInit.height = 21;

			if (alliancesCanGiveResearchAndRadar(game.alliance))
			{
				sButInit.id		= MULTIMENU_GIFT_RAD + player;
				sButInit.pTip	= _("Give Visibility Report");
				sButInit.UserData = PACKDWORD_TRI(0, IMAGE_MULTI_VIS_HI, IMAGE_MULTI_VIS);
				auto visGiftButton = std::make_shared<W_BUTTON>(&sButInit);
				visGiftButton->addOnClickHandler([player](W_BUTTON&) {
					sendGift(RADAR_GIFT, static_cast<uint8_t>(player));
				});
				alliancesGrid->place({2}, {0}, wrapGift(visGiftButton));

				sButInit.id		= MULTIMENU_GIFT_RES + player;
				sButInit.pTip	= _("Leak Technology Documents");
				sButInit.UserData = PACKDWORD_TRI(0, IMAGE_MULTI_TEK_HI , IMAGE_MULTI_TEK);
				auto resGiftButton = std::make_shared<W_BUTTON>(&sButInit);
				resGiftButton->addOnClickHandler([player](W_BUTTON&) {
					sendGift(RESEARCH_GIFT, static_cast<uint8_t>(player));
				});
				alliancesGrid->place({3}, {0}, wrapGift(resGiftButton));
			}

			sButInit.id		= MULTIMENU_GIFT_DRO + player;
			sButInit.pTip	= _("Hand Over Selected Units");
			sButInit.UserData = PACKDWORD_TRI(0, IMAGE_MULTI_DRO_HI , IMAGE_MULTI_DRO);
			auto droidGiftButton = std::make_shared<W_BUTTON>(&sButInit);
			droidGiftButton->addOnClickHandler([player](W_BUTTON&) {
				sendGift(DROID_GIFT, static_cast<uint8_t>(player));
			});
			alliancesGrid->place({4}, {0}, wrapGift(droidGiftButton));

			sButInit.id		= MULTIMENU_GIFT_POW + player;
			sButInit.pTip	= _("Give Power To Player");
			sButInit.UserData = PACKDWORD_TRI(0, IMAGE_MULTI_POW_HI , IMAGE_MULTI_POW);
			auto pwrGiftButton = std::make_shared<W_BUTTON>(&sButInit);
			pwrGiftButton->addOnClickHandler([player](W_BUTTON&) {
				sendGift(POWER_GIFT, static_cast<uint8_t>(player));
			});
			alliancesGrid->place({5}, {0}, wrapGift(pwrGiftButton));

			giftsUp[player] = true;				// note buttons are up!
		}

		auto scoreLabel = makeLabel("");
		scoreLabel->setTextAlignment(WzTextAlignment::WLAB_ALIGNRIGHT);
		auto killsLabel = makeLabel("");
		killsLabel->setTextAlignment(WzTextAlignment::WLAB_ALIGNRIGHT);
		auto unitsLabel = makeLabel("");
		unitsLabel->setTextAlignment(WzTextAlignment::WLAB_ALIGNRIGHT);
		auto lastColumnLabel = makeLabel("");
		lastColumnLabel->setTextAlignment(WzTextAlignment::WLAB_ALIGNRIGHT);
		playersWidgets.push_back({
			player,
			nameLabel,
			scoreLabel,
			killsLabel,
			unitsLabel,
			lastColumnLabel,
		});

		// chat mute button
		std::shared_ptr<WIDGET> muteWidget;
		if (player != selectedPlayer)
		{
			W_BUTINIT sButInit;
			sButInit.id		= MULTIMENU_PLAYER_MUTE_START + player;
			sButInit.pTip	= _("Toggle Chat Mute");
			sButInit.pDisplay = displayMuteState;
			sButInit.UserData = player;
			sButInit.width	= 18;
			sButInit.height = 18;
			auto muteButton = std::make_shared<W_BUTTON>(&sButInit);
			muteButton->addOnClickHandler([](W_BUTTON& button) {
				auto playerIdx = button.UserData;
				ASSERT_OR_RETURN(, playerIdx < MAX_CONNECTED_PLAYERS, "Invalid playerIdx: %" PRIu32, playerIdx);
				setPlayerMuted(playerIdx, !ingame.muteChat[playerIdx]);
			});
			muteWidget = muteButton;
		}
		else
		{
			muteWidget = std::make_shared<WIDGET>();
			muteWidget->setGeometry(0, 0, 0, 0);
		}

		Margin margin(0, CELL_HORIZONTAL_PADDING);
		place({0}, {row}, Margin(0, CELL_HORIZONTAL_PADDING, 0, 0).wrap(nameGrid));
		place({1}, {row}, margin.wrap(Alignment::center().wrap(alliancesGrid)));
		place({2}, {row}, margin.wrap(scoreLabel));
		place({3}, {row}, margin.wrap(killsLabel));
		place({4}, {row}, margin.wrap(unitsLabel));
		place({5}, {row}, margin.wrap(lastColumnLabel));
		place({6}, {row}, margin.wrap(Alignment::center().wrap(muteWidget)));
	}

	void updatePlayersWidgets()
	{
		for (auto &playerWidget: playersWidgets)
		{
			const auto color = GetPlayerTextColor((selectedPlayer < MAX_PLAYERS && playerWidget.player < MAX_PLAYERS) ? alliances[selectedPlayer][playerWidget.player] : 0, playerWidget.player);
			const bool isHuman = isHumanPlayer(playerWidget.player);
			const bool isAlly = (selectedPlayer < MAX_PLAYERS && playerWidget.player < MAX_PLAYERS) && aiCheckAlliances(selectedPlayer, playerWidget.player);
			const bool isSelectedPlayer = playerWidget.player == selectedPlayer;

			playerWidget.name->setFontColour(color);
			playerWidget.score->setFontColour(color);
			playerWidget.kills->setFontColour(color);
			playerWidget.units->setFontColour(color);
			playerWidget.lastColumn->setFontColour(color);

			if (playerWidget.player >= MAX_PLAYERS)
			{
				continue;
			}

			char scoreString[128];
			char killsString[20];
			char unitsString[20] = {0};
			char lastString[20] = {0};

			// Let's use the real score for MP games
			if (Cheated && NetPlay.bComms)
			{
				sstrcpy(scoreString, "(cheated)");
			}
			else
			{
				ssprintf(scoreString, "%d", getMultiStats(playerWidget.player).recentScore);
			}
			ssprintf(killsString, "%d", getMultiStats(playerWidget.player).recentKills);

			//only show player's and allies' unit counts, and nobody elses.
			if (isAlly || gInputManager.debugManager().debugMappingsAllowed())
			{
				ssprintf(unitsString, "%d", getNumDroids(playerWidget.player) + getNumTransporterDroids(playerWidget.player));
			}

			/* Display player power instead of number of played games
			 * and number of units instead of ping when in debug mode
			 */
			if (gInputManager.debugManager().debugMappingsAllowed())  //Won't pass this when in both release and multiplayer modes
			{
				// Player power
				ssprintf(lastString, "%u", (int)getPower(playerWidget.player));
			}
			else if (runningMultiplayer())
			{
				// ping
				if (!isSelectedPlayer && isHuman)
				{
					if (ingame.PingTimes[playerWidget.player] < PING_LIMIT)
					{
						ssprintf(lastString, "%d", ingame.PingTimes[playerWidget.player]);
					}
					else
					{
						sstrcpy(lastString, "∞");
					}
				}
			}
			else
			{
				// Structures
				if (isAlly || gInputManager.debugManager().debugMappingsAllowed())
				{
					// NOTE, This tallys up *all* the structures you have. Test out via 'start with no base'.
					int num = apsStructLists[playerWidget.player].size();
					ssprintf(lastString, "%d", num);
				}
			}

			playerWidget.score->setString(scoreString);
			playerWidget.kills->setString(killsString);
			playerWidget.units->setString(unitsString);
			playerWidget.lastColumn->setString(lastString);

			auto lockedScreen = screenPointer.lock();

			if (isHuman || (game.type == LEVEL_TYPE::SKIRMISH && playerWidget.player < game.maxPlayers))
			{
				// alliance
				// manage buttons by showing or hiding them. gifts only in campaign,
				if (alliancesCanGiveAnything(game.alliance))
				{
					if (isAlly && !isSelectedPlayer && !giftsUp[playerWidget.player])
					{
						if (alliancesCanGiveResearchAndRadar(game.alliance))
						{
							widgReveal(lockedScreen, MULTIMENU_GIFT_RAD + playerWidget.player);
							widgReveal(lockedScreen, MULTIMENU_GIFT_RES + playerWidget.player);
						}
						widgReveal(lockedScreen, MULTIMENU_GIFT_DRO + playerWidget.player);
						widgReveal(lockedScreen, MULTIMENU_GIFT_POW + playerWidget.player);
						giftsUp[playerWidget.player] = true;
					}
					else if (!isAlly && !isSelectedPlayer && giftsUp[playerWidget.player])
					{
						if (alliancesCanGiveResearchAndRadar(game.alliance))
						{
							widgHide(lockedScreen, MULTIMENU_GIFT_RAD + playerWidget.player);
							widgHide(lockedScreen, MULTIMENU_GIFT_RES + playerWidget.player);
						}
						widgHide(lockedScreen, MULTIMENU_GIFT_DRO + playerWidget.player);
						widgHide(lockedScreen, MULTIMENU_GIFT_POW + playerWidget.player);
						giftsUp[playerWidget.player] = false;
					}
				}
			}

			// clean up widgets if player leaves while menu is up.
			if (!isHuman && !(game.type == LEVEL_TYPE::SKIRMISH && playerWidget.player < game.maxPlayers))
			{
				if (widgGetFromID(lockedScreen, MULTIMENU_CHANNEL + playerWidget.player) != nullptr)
				{
					widgDelete(lockedScreen, MULTIMENU_CHANNEL + playerWidget.player);
				}

				if (widgGetFromID(lockedScreen, MULTIMENU_ALLIANCE_BASE + playerWidget.player) != nullptr)
				{
					widgDelete(lockedScreen, MULTIMENU_ALLIANCE_BASE + playerWidget.player);
					widgDelete(lockedScreen, MULTIMENU_GIFT_RAD + playerWidget.player);
					widgDelete(lockedScreen, MULTIMENU_GIFT_RES + playerWidget.player);
					widgDelete(lockedScreen, MULTIMENU_GIFT_DRO + playerWidget.player);
					widgDelete(lockedScreen, MULTIMENU_GIFT_POW + playerWidget.player);
					giftsUp[playerWidget.player] = false;
				}
			}
		}
	}

public:
	void display(int xOffset, int yOffset) override
	{
		auto x0 = xOffset + x();
		auto y0 = yOffset + y();

		auto columns = getColumnOffsets();
		auto rows = getRowOffsets();
		const std::vector<glm::ivec4> lines = {
			glm::ivec4(x0, y0 + rows[1], x0 + width(), y0 + rows[1]),
			glm::ivec4(x0 + columns[1], y0, x0 + columns[1], y0 + height()),
			glm::ivec4(x0 + columns[2], y0, x0 + columns[2], y0 + height()),
			glm::ivec4(x0 + columns[3], y0, x0 + columns[3], y0 + height()),
			glm::ivec4(x0 + columns[4], y0, x0 + columns[4], y0 + height()),
			glm::ivec4(x0 + columns[5], y0, x0 + columns[5], y0 + height()),
			glm::ivec4(x0 + columns[6], y0, x0 + columns[6], y0 + height()),
		};

		iV_Lines(lines, WZCOL_BLACK);

#ifdef DEBUG
		char str[128];

		for (unsigned q = 0; q < 2; ++q)
		{
			unsigned xPos = 0;
			unsigned yPos = q * 12;
			bool isTotal = q != 0;

			char const *srText[2] = {_("Sent/Received per sec —"), _("Total Sent/Received —")};
			ssprintf(str, "%s", srText[q]);
			iV_DrawText(str, x0 + xPos, y0 + height() + yPos, font_small);
			xPos += iV_GetTextWidth(str, font_small) + 20;

			ssprintf(str, _("Traf: %lu/%lu"), NETgetStatistic(NetStatisticRawBytes, true, isTotal), NETgetStatistic(NetStatisticRawBytes, false, isTotal));
			iV_DrawText(str, x0 + xPos, y0 + height() + yPos, font_small);
			xPos += iV_GetTextWidth(str, font_small) + 20;

			ssprintf(str, _("Uncompressed: %lu/%lu"), NETgetStatistic(NetStatisticUncompressedBytes, true, isTotal), NETgetStatistic(NetStatisticUncompressedBytes, false, isTotal));
			iV_DrawText(str, x0 + xPos, y0 + height() + yPos, font_small);
			xPos += iV_GetTextWidth(str, font_small) + 20;

			ssprintf(str, _("Pack: %lu/%lu"), NETgetStatistic(NetStatisticPackets, true, isTotal), NETgetStatistic(NetStatisticPackets, false, isTotal));
			iV_DrawText(str, x0 + xPos, y0 + height() + yPos, font_small);
	}
#endif
	}

	void run(W_CONTEXT *context) override
	{
		char timerStr[128];
		getAsciiTime(timerStr, gameTime);
		timerLabel->setString(timerStr);
		updatePlayersWidgets();

		powerLabel->hide();
		pingLabel->hide();
		structsLabel->hide();
		if (gInputManager.debugManager().debugMappingsAllowed())
		{
			powerLabel->show();
		}
		else if (runningMultiplayer())
		{
			pingLabel->show();
		}
		else
		{
			structsLabel->show();
		}
	}

private:
	std::shared_ptr<W_LABEL> timerLabel;
	std::shared_ptr<W_LABEL> powerLabel;
	std::shared_ptr<W_LABEL> pingLabel;
	std::shared_ptr<W_LABEL> structsLabel;
	std::vector<PlayerWidgets> playersWidgets;
};

class WzMultiMenuTabs : public MultichoiceWidget
{
public:
	WzMultiMenuTabs(int value = -1) : MultichoiceWidget(value) { }
	virtual void display(int xOffset, int yOffset) override { }
};

constexpr int MULTIMENUFORM_INTERNAL_PADDING = 10;
constexpr int MULTIMENUFORM_PANEL_TABS_HEIGHT = 20;

class WzMultiWidget : public IntFormAnimated
{
public:
	WzMultiWidget(bool openAnimate = true)
	: IntFormAnimated(openAnimate)
	{ }
public:
	static std::shared_ptr<WzMultiWidget> make(bool openAnimate, std::function<void ()> closeButtonHandler);
protected:
	virtual void geometryChanged() override;
	virtual int32_t idealWidth() override;
	virtual int32_t idealHeight() override;
private:
	void initialize(std::function<void ()> closeButtonHandler);
	void switchAttachedPanel(const std::shared_ptr<WIDGET> newPanel);
private:
	std::shared_ptr<W_BUTTON> closeButton;
	std::shared_ptr<WzMultiMenuTabs> panelSwitcher;
	std::shared_ptr<MultiMenuGrid> multiMenuGrid;
	std::shared_ptr<WIDGET> teamStrategyViewWrapped;
	std::vector<std::shared_ptr<WIDGET>> panels;
	std::shared_ptr<WIDGET> currentlyAttachedPanel;
};

std::shared_ptr<WzMultiWidget> WzMultiWidget::make(bool openAnimate, std::function<void ()> closeButtonHandler)
{
	auto result = std::make_shared<WzMultiWidget>(openAnimate);
	result->initialize(closeButtonHandler);
	return result;
}

void WzMultiWidget::geometryChanged()
{
	if (closeButton)
	{
		closeButton->callCalcLayout();
	}
	if (panelSwitcher)
	{
		panelSwitcher->callCalcLayout();
	}
	for (auto& panel : panels)
	{
		int panelY0 = 0;
		if (panelSwitcher)
		{
			panelY0 += panelSwitcher->y() + panelSwitcher->height() + MULTIMENUFORM_INTERNAL_PADDING;
		}
		int panelHeight = std::min(panel->idealHeight(), height() - panelY0);
		panel->setGeometry(0, panelY0, width(), panelHeight);
	}
}

int32_t WzMultiWidget::idealWidth()
{
	int32_t maxPanelIdealWidth = 0;
	for (auto& panel : panels)
	{
		maxPanelIdealWidth = std::max(maxPanelIdealWidth, panel->idealWidth());
	}
	return maxPanelIdealWidth;
}

int32_t WzMultiWidget::idealHeight()
{
	int32_t result = 0;
	if (panelSwitcher)
	{
		result += MULTIMENUFORM_INTERNAL_PADDING + panelSwitcher->idealHeight() + MULTIMENUFORM_INTERNAL_PADDING;
	}
	int32_t maxPanelIdealHeight = 0;
	for (auto& panel : panels)
	{
		maxPanelIdealHeight = std::max(maxPanelIdealHeight, panel->idealHeight());
	}
	result += maxPanelIdealHeight + MULTIMENUFORM_INTERNAL_PADDING;
	return result;
}

void WzMultiWidget::switchAttachedPanel(const std::shared_ptr<WIDGET> newPanel)
{
	if (currentlyAttachedPanel)
	{
		detach(currentlyAttachedPanel);
	}
	currentlyAttachedPanel = newPanel;
	attach(newPanel, ChildZPos::Back);
}

void WzMultiWidget::initialize(std::function<void ()> closeButtonHandler)
{
	if (closeButtonHandler)
	{
		// Add the close button.
		W_BUTINIT sButInit;
		sButInit.id = MULTIMENU_CLOSE;
		sButInit.pTip = _("Close");
		sButInit.pDisplay = intDisplayImageHilight;
		sButInit.UserData = PACKDWORD_TRI(0, IMAGE_CLOSEHILIGHT , IMAGE_CLOSE);
		closeButton = std::make_shared<W_BUTTON>(&sButInit);
		closeButton->addOnClickHandler([closeButtonHandler](W_BUTTON&) {
			closeButtonHandler();
		});
		attach(closeButton);
		closeButton->setCalcLayout([](WIDGET *psButton) {
			auto psParent = psButton->parent();
			ASSERT_OR_RETURN(, psParent != nullptr, "No parent");
			psButton->setGeometry(psParent->width() - CLOSE_WIDTH, 0, CLOSE_WIDTH, CLOSE_HEIGHT);
		});
	}

	multiMenuGrid = MultiMenuGrid::make();
	panels.push_back(multiMenuGrid);

	bool allowAIsForTeamStrategy = !NetPlay.bComms;
	if (gameHasTeamStrategyView(allowAIsForTeamStrategy))
	{
		auto teamStrategyView = createTeamStrategyView(allowAIsForTeamStrategy);
		if (teamStrategyView)
		{
			transformTeamStrategyViewMode(teamStrategyView, true);
			teamStrategyViewSetBackgroundColor(teamStrategyView, pal_RGBA(25, 0, 110, 180));
			teamStrategyViewWrapped = Margin(0, 10).wrap(teamStrategyView);
			panels.push_back(teamStrategyViewWrapped);
		}
	}

	if (teamStrategyViewWrapped)
	{
		// add tabs
		panelSwitcher = std::make_shared<WzMultiMenuTabs>(0);
		attach(panelSwitcher);
		panelSwitcher->setButtonAlignment(MultibuttonWidget::ButtonAlignment::CENTER_ALIGN);
		panelSwitcher->addButton(0, WzPanelTabButton::make(_("Players View")));
		panelSwitcher->addButton(1, WzPanelTabButton::make(_("Team Strategy")));
		panelSwitcher->choose(0);
		panelSwitcher->addOnChooseHandler([](MultibuttonWidget& widget, int newValue){
			// Switch actively-displayed panel
			auto weakParent = std::weak_ptr<WIDGET>(widget.parent());
			widgScheduleTask([newValue, weakParent](){
				auto strongParent = std::dynamic_pointer_cast<WzMultiWidget>(weakParent.lock());
				ASSERT_OR_RETURN(, strongParent != nullptr, "Parent doesn't exist?");
				strongParent->switchAttachedPanel(strongParent->panels[newValue]);
			});
		});
		panelSwitcher->setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
			auto psParent = std::dynamic_pointer_cast<WzMultiWidget>(psWidget->parent());
			ASSERT_OR_RETURN(, psParent != nullptr, "No parent");
			int panelSwitcherWidth = psWidget->idealWidth();
			int panelSwitcherX0 = (psParent->width() - panelSwitcherWidth) / 2;
			psWidget->setGeometry(panelSwitcherX0, MULTIMENUFORM_INTERNAL_PADDING, panelSwitcherWidth, MULTIMENUFORM_PANEL_TABS_HEIGHT);
		}));
	}

	switchAttachedPanel(multiMenuGrid);
}

std::shared_ptr<IntFormAnimated> intCreateMultiMenuForm(std::function<void ()> closeButtonHandler)
{
	auto form = WzMultiWidget::make(true, closeButtonHandler);
	return form;
}

bool intAddMultiMenu()
{
	//check for already open.
	if (widgGetFromID(psWScreen, MULTIMENU_FORM))
	{
		intCloseMultiMenu();
		return true;
	}

	if (intMode != INT_INTELMAP)
	{
		intResetScreen(false);
	}

	auto form = intCreateMultiMenuForm([]() {
		widgScheduleTask([]() {
			intCloseMultiMenu();
		});
	});
	form->id = MULTIMENU_FORM;
	form->setCalcLayout([](WIDGET *form) {
		auto width = std::min((int32_t)screenWidth - 20, form->idealWidth());
		auto height = form->idealHeight();
		form->setGeometry((screenWidth - width) / 2, MULTIMENU_FORM_Y, width, height);
	});

	psWScreen->psForm->attach(form);

	intShowPowerBar();						// add power bar

	if (intMode != INT_INTELMAP)
	{
		intMode		= INT_MULTIMENU;			// change interface mode.
	}
	MultiMenuUp = true;
	return true;
}

// ////////////////////////////////////////////////////////////////////////////
void intCloseMultiMenuNoAnim()
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
bool intCloseMultiMenu()
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
