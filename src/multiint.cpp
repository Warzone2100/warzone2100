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
 * MultiInt.c
 *
 * Alex Lee, 98. Pumpkin Studios, Bath.
 * Functions to display and handle the multiplayer interface screens,
 * along with connection and game options.
 */

#include "lib/framework/wzapp.h"
#include "lib/framework/wzconfig.h"
#include "lib/framework/wzpaths.h"

#include <time.h>

#include "lib/framework/frameresource.h"
#include "lib/framework/file.h"
#include "lib/framework/stdio_ext.h"
#include "lib/framework/physfs_ext.h"

#include <wzmaplib/map_preview.h>

/* Includes direct access to render library */
#include "lib/ivis_opengl/bitimage.h"
#include "lib/ivis_opengl/pieblitfunc.h"
#include "lib/ivis_opengl/pietypes.h"
#include "lib/ivis_opengl/piestate.h"
#include "lib/ivis_opengl/pieclip.h"
#include "lib/ivis_opengl/piemode.h"
#include "lib/ivis_opengl/piepalette.h"
#include "lib/ivis_opengl/screen.h"

#include "lib/sound/audio.h"
#include "lib/sound/audio_id.h"

#include "lib/gamelib/gtime.h"
#include "lib/netplay/netplay.h"
#include "lib/widget/editbox.h"
#include "lib/widget/button.h"
#include "lib/widget/scrollablelist.h"
#include "lib/widget/widget.h"
#include "lib/widget/widgint.h"
#include "lib/widget/label.h"
#include "lib/widget/paragraph.h"
#include "lib/widget/multibutform.h"

#include "challenge.h"
#include "main.h"
#include "levels.h"
#include "objects.h"
#include "display.h"// pal stuff
#include "display3d.h"
#include "objmem.h"
#include "gateway.h"
#include "clparse.h"
#include "configuration.h"
#include "intdisplay.h"
#include "design.h"
#include "hci.h"
#include "power.h"
#include "loadsave.h"			// for blueboxes.
#include "component.h"
#include "map.h"
#include "console.h"			// chat box stuff
#include "frend.h"
#include "advvis.h"
#include "frontend.h"
#include "data.h"
#include "game.h"
#include "warzoneconfig.h"
#include "modding.h"
#include "qtscript.h"
#include "random.h"
#include "notifications.h"
#include "radar.h"
#include "lib/framework/wztime.h"

#include "multiplay.h"
#include "multiint.h"
#include "multijoin.h"
#include "multistat.h"
#include "multirecv.h"
#include "multimenu.h"
#include "multilimit.h"
#include "multigifts.h"

#include "titleui/titleui.h"

#include "warzoneconfig.h"

#include "init.h"
#include "levels.h"
#include "wrappers.h"
#include "faction.h"

#include "activity.h"
#include <algorithm>

#define MAP_PREVIEW_DISPLAY_TIME 2500	// number of milliseconds to show map in preview
#define VOTE_TAG                 "voting"
#define KICK_REASON_TAG          "kickReason"

// ////////////////////////////////////////////////////////////////////////////
// vars
extern char	MultiCustomMapsPath[PATH_MAX];
extern char	MultiPlayersPath[PATH_MAX];
extern bool bSendingMap;			// used to indicate we are sending a map

enum RoomMessageType {
	RoomMessagePlayer,
	RoomMessageSystem,
	RoomMessageNotify
};

struct RoomMessage
{
	std::shared_ptr<PlayerReference> sender = nullptr;
	std::string text;
	std::time_t time = std::time(nullptr);
	RoomMessageType type;

	static RoomMessage player(uint32_t messageSender, std::string messageText)
	{
		auto message = RoomMessage(RoomMessagePlayer, messageText);
		message.sender = NetPlay.playerReferences[messageSender];
		return message;
	}

	static RoomMessage system(std::string messageText)
	{
		return RoomMessage(RoomMessageSystem, messageText);
	}

	static RoomMessage notify(std::string messageText)
	{
		return RoomMessage(RoomMessageNotify, messageText);
	}

private:
	RoomMessage(RoomMessageType messageType, std::string messageText)
	{
		type = messageType;
		text = messageText;
	}
};

char sPlayer[128] = {'\0'}; // player name (to be used)
bool multiintDisableLobbyRefresh = false; // if we allow lobby to be refreshed or not.

static UDWORD hideTime = 0;
static uint8_t playerVotes[MAX_PLAYERS];
LOBBY_ERROR_TYPES LobbyError = ERROR_NOERROR;
static bool bInActualHostedLobby = false;
/// end of globals.
// ////////////////////////////////////////////////////////////////////////////
// Function protos

// widget functions
static W_EDITBOX* addMultiEditBox(UDWORD formid, UDWORD id, UDWORD x, UDWORD y, char const *tip, char const *tipres, UDWORD icon, UDWORD iconhi, UDWORD iconid);
static W_FORM * addBlueForm(UDWORD parent, UDWORD id, UDWORD x, UDWORD y, UDWORD w, UDWORD h);
static void drawReadyButton(UDWORD player);

// Drawing Functions
static void displayChatEdit(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset);
static void displayPlayer(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset);
static void displayPosition(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset);
static void displayColour(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset);
static void displayFaction(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset);
static void displayTeamChooser(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset);
static void displayAi(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset);
static void displayDifficulty(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset);
static void displayMultiEditBox(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset);

// pUserData structures used by drawing functions
struct DisplayPlayerCache {
	std::string	fullMainText;	// the “full” main text (used for storing the full player name when displaying a player)
	WzText		wzMainText;		// the main text

	WzText		wzSubText;		// the sub text (used for players)
	WzText		wzEloText;      // the elo text (used for players)
};
struct DisplayPositionCache {
	WzText wzPositionText;
};
struct DisplayAICache {
	WzText wzText;
};
struct DisplayDifficultyCache {
	WzText wzDifficultyText;
};

// Game option functions
static	void	addGameOptions();
static void addChatBox(bool preserveOldChat = false);
static	void	disableMultiButs();
static	void	SendFireUp();

static	void	decideWRF();

static bool		SendColourRequest(UBYTE player, UBYTE col);
static bool		SendFactionRequest(UBYTE player, UBYTE faction);
static bool		SendPositionRequest(UBYTE player, UBYTE chosenPlayer);
bool changeReadyStatus(UBYTE player, bool bReady);
static void stopJoining(std::shared_ptr<WzTitleUI> parent);
static int difficultyIcon(int difficulty);

static void sendRoomChatMessage(char const *text);

static int factionIcon(FactionID faction);

static bool multiplayPlayersReady();
static bool multiplayIsStartingGame();
// ////////////////////////////////////////////////////////////////////////////
// map previews..

static const char *difficultyList[] = { N_("Easy"), N_("Medium"), N_("Hard"), N_("Insane") };
static const AIDifficulty difficultyValue[] = { AIDifficulty::EASY, AIDifficulty::MEDIUM, AIDifficulty::HARD, AIDifficulty::INSANE };
static struct
{
	bool scavengers;
	bool alliances;
	bool teams;
	bool power;
	bool difficulty;
	bool ai;
	bool position;
	bool bases;
} locked;

struct AIDATA
{
	AIDATA() : assigned(0) {}
	char name[MAX_LEN_AI_NAME];
	char js[MAX_LEN_AI_NAME];
	char tip[255 + 128];            ///< may contain optional AI tournament data
	char difficultyTips[4][255];    ///< optional difficulty level info
	int assigned;                   ///< How many AIs have we assigned of this type
};
static std::vector<AIDATA> aidata;

struct WzMultiButton : public W_BUTTON
{
	WzMultiButton() : W_BUTTON() {}

	void display(int xOffset, int yOffset) override;

	Image imNormal;
	Image imDown;
	unsigned doHighlight;
	unsigned tc;
};

class ChatBoxWidget : public IntFormAnimated
{
protected:
	ChatBoxWidget(): IntFormAnimated(true) {}
	virtual void initialize();

public:
	static std::shared_ptr<ChatBoxWidget> make()
	{
		class make_shared_enabler: public ChatBoxWidget {};
		auto widget = std::make_shared<make_shared_enabler>();
		widget->initialize();
		return widget;
	}

	virtual ~ChatBoxWidget();
	void addMessage(RoomMessage const &message);
	void initializeMessages(bool preserveOldChat);

protected:
	void geometryChanged() override;

private:
	std::shared_ptr<ScrollableListWidget> messages;
	std::shared_ptr<W_EDITBOX> editBox;
	std::shared_ptr<CONSOLE_MESSAGE_LISTENER> handleConsoleMessage;
	void displayMessage(RoomMessage const &message);

	static std::vector<RoomMessage> persistentMessageLocalStorage;
};

std::vector<RoomMessage> ChatBoxWidget::persistentMessageLocalStorage;

void displayRoomMessage(RoomMessage const &message)
{
	if (auto chatBox = (ChatBoxWidget *)widgGetFromID(psWScreen, MULTIOP_CHATBOX)) {
		chatBox->addMessage(message);
	}
}

void displayRoomSystemMessage(char const *text)
{
	displayRoomMessage(RoomMessage::system(text));
}

void displayRoomNotifyMessage(char const *text)
{
	displayRoomMessage(RoomMessage::notify(text));
}

const std::vector<WzString> getAINames()
{
	std::vector<WzString> l;
	for (const auto &i : aidata)
	{
		if (i.js[0] != '\0')
		{
			l.push_back(WzString::fromUtf8(i.js));
		}
	}
	return l;
}

const char *getAIName(int player)
{
	if (NetPlay.players[player].ai >= 0 && NetPlay.players[player].ai != AI_CUSTOM)
	{
		return aidata[NetPlay.players[player].ai].name;
	}
	else
	{
		return _("Commander");
	}
}

void loadMultiScripts()
{
	bool defaultRules = true;
	char aFileName[256];
	char aPathName[256];
	LEVEL_DATASET *psLevel = levFindDataSet(game.map, &game.hash);
	ASSERT_OR_RETURN(, psLevel, "No level found for %s", game.map);
	sstrcpy(aFileName, psLevel->apDataFiles[psLevel->game]);
	aFileName[strlen(aFileName) - 4] = '\0';
	sstrcpy(aPathName, aFileName);
	sstrcat(aFileName, ".json");
	sstrcat(aPathName, "/");
	WzString ininame;
	WzString path;
	bool loadExtra = false;

	if (challengeFileName.length() > 0)
	{
		ininame = challengeFileName;
		path = "challenges/";
		loadExtra = true;
	}

	if (getHostLaunch() == HostLaunch::Skirmish)
	{
		ininame = "tests/" + WzString::fromUtf8(wz_skirmish_test());
		path = "tests/";
		loadExtra = true;
	}

	if (getHostLaunch() == HostLaunch::Autohost)
	{
		ininame = "autohost/" + WzString::fromUtf8(wz_skirmish_test());
		path = "autohost/";
		loadExtra = true;
	}

	// Reset assigned counter
	for (auto it = aidata.begin(); it < aidata.end(); ++it)
	{
		(*it).assigned = 0;
	}

	// Load map scripts
	if (loadExtra && PHYSFS_exists(ininame.toUtf8().c_str()))
	{
		WzConfig ini(ininame, WzConfig::ReadOnly);
		debug(LOG_SAVE, "Loading map scripts");
		if (ini.beginGroup("scripts"))
		{
			if (ini.contains("extra"))
			{
				loadGlobalScript(path + ini.value("extra").toWzString());
			}
			if (ini.contains("rules"))
			{
				loadGlobalScript(path + ini.value("rules").toWzString());
				defaultRules = false;
			}
		}
		ini.endGroup();
	}

	// Load multiplayer rules
	resForceBaseDir("messages/");
	resLoadFile("SMSG", "multiplay.txt");
	if (defaultRules)
	{
		debug(LOG_SAVE, "Loading default rules");
		loadGlobalScript("multiplay/script/rules/init.js");
	}

	// Backup data hashes, since AI and scavenger scripts aren't run on all clients.
	uint32_t oldHash1 = DataHash[DATA_SCRIPT];
	uint32_t oldHash2 = DataHash[DATA_SCRIPTVAL];

	// Load AI players for skirmish games
	resForceBaseDir("multiplay/skirmish/");
	if (bMultiPlayer && game.type == LEVEL_TYPE::SKIRMISH)
	{
		for (unsigned i = 0; i < game.maxPlayers; i++)
		{
			// Skip human players. Do not skip local player for autogames
			const bool bIsLocalPlayer = i == selectedPlayer;
			const bool bShouldSkipThisPlayer = !bIsLocalPlayer || !autogame_enabled();
			if (NetPlay.players[i].allocated && bShouldSkipThisPlayer)
			{
				continue;
			}

			// Make sure local player has an AI in autogames
			if (/*NetPlay.players[i].ai < 0 &&*/ bIsLocalPlayer && autogame_enabled())
			{
				//NetPlay.players[i].ai = 0;
				// levLoadData handles making sure the local player has an AI in autogames
				// TODO: clean this mess up, and pick one place to handle initializing the AI in autogames
				continue;
			}

			if (NetPlay.players[i].ai >= 0 && myResponsibility(i))
			{
				if (aidata[NetPlay.players[i].ai].js[0] != '\0')
				{
					debug(LOG_SAVE, "Loading javascript AI for player %d", i);
					loadPlayerScript(WzString("multiplay/skirmish/") + aidata[NetPlay.players[i].ai].js, i, NetPlay.players[i].difficulty);
				}
			}
		}
	}

	// Load scavengers
	if (game.scavengers && myResponsibility(scavengerPlayer()))
	{
		debug(LOG_SAVE, "Loading scavenger AI for player %d", scavengerPlayer());
		loadPlayerScript("multiplay/script/scavengers/init.js", scavengerPlayer(), AIDifficulty::EASY);
	}

	// Restore data hashes, since AI and scavenger scripts aren't run on all clients.
	DataHash[DATA_SCRIPT]    = oldHash1;  // Not all players load the same AI scripts.
	DataHash[DATA_SCRIPTVAL] = oldHash2;

	// Reset resource path, otherwise things break down the line
	resForceBaseDir("");
}

static MAP_TILESET_TYPE guessMapTilesetType(LEVEL_DATASET *psLevel)
{
	unsigned t = 0, c = 0;

	if (psLevel->psBaseData && psLevel->psBaseData->pName)
	{
		if (sscanf(psLevel->psBaseData->pName, "MULTI_CAM_%u", &c) != 1)
		{
			sscanf(psLevel->psBaseData->pName, "MULTI_T%u_C%u", &t, &c);
		}
	}

	switch (c)
	{
	case 1:
		return TILESET_ARIZONA;
		break;
	case 2:
		return TILESET_URBAN;
		break;
	case 3:
		return TILESET_ROCKIES;
		break;
	}

	debug(LOG_MAP, "Could not guess map tileset, using ARIZONA.");
	return TILESET_ARIZONA;
}

static void loadEmptyMapPreview()
{
	// No map is available to preview, so improvise.
	char *imageData = (char *)malloc(BACKDROP_HACK_WIDTH * BACKDROP_HACK_HEIGHT * 3);
	memset(imageData, 0, BACKDROP_HACK_WIDTH * BACKDROP_HACK_HEIGHT * 3); //dunno about background color
	int const ex = 100, ey = 100, bx = 5, by = 8;
	for (unsigned n = 0; n < 125; ++n)
	{
		int sx = rand() % (ex - bx), sy = rand() % (ey - by);
		char col[3] = {char(rand() % 256), char(rand() % 256), char(rand() % 256)};
		for (unsigned y = 0; y < by; ++y)
			for (unsigned x = 0; x < bx; ++x)
				if (("\2\1\261\11\6"[x] >> y & 1) == 1) // ?
				{
					memcpy(imageData + 3 * (sx + x + BACKDROP_HACK_WIDTH * (sy + y)), col, 3);
				}
	}

	// Slight hack to init array with a special value used to determine how many players on map
	Vector2i playerpos[MAX_PLAYERS];
	for (size_t i = 0; i < MAX_PLAYERS; ++i)
	{
		playerpos[i] = Vector2i(0x77777777, 0x77777777);
	}

	screen_enableMapPreview(ex, ey, playerpos);

	screen_Upload(imageData);
	free(imageData);
}

static inline WzMap::MapPreviewColor PIELIGHT_to_MapPreviewColor(PIELIGHT color)
{
	WzMap::MapPreviewColor previewColor;
	previewColor.r = color.byte.r;
	previewColor.g = color.byte.g;
	previewColor.b = color.byte.b;
	previewColor.a = color.byte.a;
	return previewColor;
}

class WzLobbyPreviewPlayerColorProvider : public WzMap::MapPlayerColorProvider
{
public:
	~WzLobbyPreviewPlayerColorProvider() { }

	// -1 = scavs
	virtual WzMap::MapPreviewColor getPlayerColor(int8_t mapPlayer) override
	{
		unsigned player = mapPlayer == -1? scavengerSlot() : mapPlayer;
		if (player >= MAX_PLAYERS)
		{
			debug(LOG_ERROR, "Bad player");
			return MapPlayerColorProvider::getPlayerColor(mapPlayer);
		}
		unsigned playerid = getPlayerColour(RemapPlayerNumber(player));
		// kludge to fix black, so you can see it on some maps.
		PIELIGHT color = playerid == 3? WZCOL_GREY : clanColours[playerid];
		return PIELIGHT_to_MapPreviewColor(color);
	}
private:
	// -----------------------------------------------------------------------------------------
	// Remaps old player number based on position on map to new owner
	UDWORD RemapPlayerNumber(UDWORD OldNumber)
	{
		int i;

		if (game.type == LEVEL_TYPE::CAMPAIGN)		// don't remap for SP games
		{
			return OldNumber;
		}

		for (i = 0; i < MAX_PLAYERS; i++)
		{
			if (OldNumber == NetPlay.players[i].position)
			{
				return i;
			}
		}
		ASSERT(false, "Found no player position for player %d", (int)OldNumber);
		return 0;
	}
};

/// Loads the entire map just to show a picture of it
void loadMapPreview(bool hideInterface)
{
	std::string		aFileName;
	Vector2i playerpos[MAX_PLAYERS];	// Will hold player positions

	// absurd hack, since there is a problem with updating this crap piece of info, we're setting it to
	// true by default for now, like it used to be
	// TODO: Is this still needed at all?
	game.mapHasScavengers = true; // this is really the wrong place for it, but this is where it has to be

	// load the terrain types
	LEVEL_DATASET *psLevel = levFindDataSet(game.map, &game.hash);
	if (psLevel == nullptr)
	{
		debug(LOG_INFO, "Could not find level dataset \"%s\" %s. We %s waiting for a download.", game.map, game.hash.toString().c_str(), !NetPlay.wzFiles.empty() ? "are" : "aren't");
		loadEmptyMapPreview();
		return;
	}
	if (psLevel->realFileName == nullptr)
	{
		builtInMap = true;
		debug(LOG_WZ, "Loading map preview: \"%s\" builtin t%d", psLevel->pName, psLevel->dataDir);
	}
	else
	{
		builtInMap = false;
		debug(LOG_WZ, "Loading map preview: \"%s\" in (%s)\"%s\"  %s t%d", psLevel->pName, WZ_PHYSFS_getRealDir_String(psLevel->realFileName).c_str(), psLevel->realFileName, psLevel->realFileHash.toString().c_str(), psLevel->dataDir);
	}
	rebuildSearchPath(psLevel->dataDir, false, psLevel->realFileName);
	const char* pGamPath = psLevel->apDataFiles[psLevel->game];
	if (!pGamPath)
	{
		debug(LOG_ERROR, "No path for level \"%s\"? (%s)", psLevel->pName, (psLevel->realFileName) ? psLevel->realFileName : "null");
		loadEmptyMapPreview();
		return;
	}
	aFileName = pGamPath;
	// Remove the file extension (ex. ".gam")
	auto lastPeriodPos = aFileName.rfind('.');
	if (std::string::npos != lastPeriodPos)
	{
		aFileName = aFileName.substr(0, std::max<size_t>(lastPeriodPos, (size_t)1));
	}

	// load the map data
	aFileName += "/";
	auto data = WzMap::Map::loadFromPath(aFileName, WzMap::MapType::SKIRMISH, psLevel->players, rand(), true, std::unique_ptr<WzMap::LoggingProtocol>(new WzMapDebugLogger()), std::unique_ptr<WzMapPhysFSIO>(new WzMapPhysFSIO()));
	if (!data)
	{
		debug(LOG_ERROR, "Failed to load map from path: %s", aFileName.c_str());
		loadEmptyMapPreview();
		return;
	}

	WzMap::MapPreviewColorScheme previewColorScheme;
	previewColorScheme.hqColor = PIELIGHT_to_MapPreviewColor(WZCOL_MAP_PREVIEW_HQ);
	previewColorScheme.oilResourceColor = PIELIGHT_to_MapPreviewColor(WZCOL_MAP_PREVIEW_OIL);
	previewColorScheme.oilBarrelColor = PIELIGHT_to_MapPreviewColor(WZCOL_MAP_PREVIEW_BARREL);
	previewColorScheme.playerColorProvider = std::unique_ptr<WzMap::MapPlayerColorProvider>(new WzLobbyPreviewPlayerColorProvider());
	switch (guessMapTilesetType(psLevel))
	{
	case TILESET_ARIZONA:
		previewColorScheme.tilesetColors = WzMap::TilesetColorScheme::TilesetArizona();
		break;
	case TILESET_URBAN:
		previewColorScheme.tilesetColors = WzMap::TilesetColorScheme::TilesetUrban();
		break;
	case TILESET_ROCKIES:
		previewColorScheme.tilesetColors = WzMap::TilesetColorScheme::TilesetRockies();
		break;
	default:
		debug(LOG_FATAL, "Invalid tileset type");
		// silence warnings
		abort();
		return;
	}

	// Slight hack to init array with a special value used to determine how many players on map
	for (size_t i = 0; i < MAX_PLAYERS; ++i)
	{
		playerpos[i] = Vector2i(0x77777777, 0x77777777);
	}

	std::unique_ptr<WzMap::LoggingProtocol> generatePreviewLogger(new WzMapDebugLogger());
	auto mapPreviewResult = WzMap::generate2DMapPreview(*data, previewColorScheme, generatePreviewLogger.get());
	if (!mapPreviewResult)
	{
		// Failed to generate map preview
		debug(LOG_ERROR, "Failed to generate map preview for: %s", psLevel->pName);
		loadEmptyMapPreview();
		return;
	}

	// for the backdrop, we currently need to copy this to the top-left of an image that's BACKDROP_HACK_WIDTH x BACKDROP_HACK_HEIGHT
	size_t backdropSize = sizeof(char) * BACKDROP_HACK_WIDTH * BACKDROP_HACK_HEIGHT;
	char *backdropData = (char *)malloc(backdropSize * 3);		// used for the texture
	if (!backdropData)
	{
		debug(LOG_FATAL, "Out of memory for texture!");
		abort();	// should be a fatal error ?
		return;
	}
	ASSERT(mapPreviewResult->width <= BACKDROP_HACK_WIDTH, "mapData width somehow exceeds backdrop width?");
	memset(backdropData, 0, sizeof(char) * BACKDROP_HACK_WIDTH * BACKDROP_HACK_HEIGHT * 3); //dunno about background color
	char *imageData = reinterpret_cast<char*>(mapPreviewResult->imageData.data());
	for (int y = 0; y < mapPreviewResult->height; ++y)
	{
		char *pSrc = imageData + (3 * (y * mapPreviewResult->width));
		char *pDst = backdropData + (3 * (y * BACKDROP_HACK_WIDTH));
		memcpy(pDst, pSrc, std::min<size_t>(mapPreviewResult->width, BACKDROP_HACK_WIDTH) * 3);
	}

	for (auto kv : mapPreviewResult->playerHQPosition)
	{
		int8_t mapPlayer = kv.first;
		unsigned player = mapPlayer == -1? scavengerSlot() : mapPlayer;
		if (player >= MAX_PLAYERS)
		{
			debug(LOG_ERROR, "Bad player");
			continue;
		}
		playerpos[player] = Vector2i(kv.second.first, kv.second.second);
	}

	screen_enableMapPreview(mapPreviewResult->width, mapPreviewResult->height, playerpos);

	screen_Upload(backdropData);

	free(backdropData);

	if (hideInterface)
	{
		hideTime = gameTime;
	}
}

// ////////////////////////////////////////////////////////////////////////////
// helper func

int matchAIbyName(const char *name)
{
	int i = 0;

	if (name[0] == '\0')
	{
		return AI_CLOSED;
	}
	for (auto it = aidata.cbegin(); it < aidata.cend(); ++it, i++)
	{
		if (strncasecmp(name, (*it).name, MAX_LEN_AI_NAME) == 0)
		{
			return i;
		}
	}
	return AI_NOT_FOUND;
}

void readAIs()
{
	char basepath[PATH_MAX];
	const char *sSearchPath = "multiplay/skirmish/";

	aidata.clear();

	sstrcpy(basepath, sSearchPath);
	WZ_PHYSFS_enumerateFiles(basepath, [&](const char *file) -> bool {
		char path[PATH_MAX];
		// See if this filename contains the extension we're looking for
		if (!strstr(file, ".json"))
		{
			return true; // continue;
		}
		sstrcpy(path, basepath);
		sstrcat(path, file);
		WzConfig aiconf(path, WzConfig::ReadOnly);
		AIDATA ai;
		aiconf.beginGroup("AI");

		if (aiconf.contains("name"))
		{
			sstrcpy(ai.name, _(aiconf.value("name", "").toWzString().toUtf8().c_str()));
		}
		else
		{
			sstrcpy(ai.name, _("MISSING AI NAME"));
		}

		sstrcpy(ai.js, aiconf.value("js", "").toWzString().toUtf8().c_str());

		const char *difficultyKeys[] = { "easy_tip", "medium_tip", "hard_tip", "insane_tip" };
		for (int i = 0; i < ARRAY_SIZE(difficultyKeys); i++)
		{
			if (aiconf.contains(difficultyKeys[i]))
			{
				sstrcpy(ai.difficultyTips[i], _(aiconf.value(difficultyKeys[i], "").toWzString().toUtf8().c_str()));
			}
			else
			{
				// note that the empty string "" must never be translated
				sstrcpy(ai.difficultyTips[i], "");
			}
		}

		if (aiconf.contains("tip"))
		{
			sstrcpy(ai.tip, _(aiconf.value("tip", "").toWzString().toUtf8().c_str()));
		}
		else
		{
			sstrcpy(ai.tip, _("MISSING AI DESCRIPTION"));
		}

		int wins = aiconf.value("wins", 0).toInt();
		int losses = aiconf.value("losses", 0).toInt();
		int draws = aiconf.value("draws", 0).toInt();
		int total = wins + losses + draws;
		if (total)
		{
			float win_percentage = static_cast<float>(wins) / total * 100;
			float loss_percentage = static_cast<float>(losses) / total * 100;
			float draw_percentage = static_cast<float>(draws) / total * 100;
			sstrcat(ai.tip, "\n");
			char statistics[127];
			ssprintf(statistics, _("AI tournament: %3.1f%% wins, %3.1f%% losses, %3.1f%% draws"), win_percentage, loss_percentage, draw_percentage);
			sstrcat(ai.tip, statistics);
		}

		if (strcmp(file, "nb_generic.json") == 0)
		{
			aidata.insert(aidata.begin(), ai);
		}
		else
		{
			aidata.push_back(ai);
		}
		aiconf.endGroup();
		return true; // continue
	});
}

//sets sWRFILE form game.map
static void decideWRF()
{
	// try and load it from the maps directory first,
	sstrcpy(aLevelName, MultiCustomMapsPath);
	sstrcat(aLevelName, game.map);
	sstrcat(aLevelName, ".wrf");
	debug(LOG_WZ, "decideWRF: %s", aLevelName);
	//if the file exists in the downloaded maps dir then use that one instead.
	// FIXME: Try to incorporate this into physfs setup somehow for sane paths
	if (!PHYSFS_exists(aLevelName))
	{
		sstrcpy(aLevelName, game.map);		// doesn't exist, must be a predefined one.
	}
}

// ////////////////////////////////////////////////////////////////////////
// Lobby error reading
LOBBY_ERROR_TYPES getLobbyError()
{
	return LobbyError;
}

void setLobbyError(LOBBY_ERROR_TYPES error_type)
{
	LobbyError = error_type;
	if (LobbyError <= ERROR_FULL)
	{
		multiintDisableLobbyRefresh = false;
	}
	else
	{
		multiintDisableLobbyRefresh = true;
	}
}

// NOTE: Must call NETinit(true); before this will actually work
std::vector<JoinConnectionDescription> findLobbyGame(const std::string& lobbyAddress, unsigned int lobbyPort, uint32_t lobbyGameId)
{
	WzString originalLobbyServerName = WzString::fromUtf8(NETgetMasterserverName());
	unsigned int originalLobbyServerPort = NETgetMasterserverPort();

	if (!lobbyAddress.empty())
	{
		if (lobbyPort == 0)
		{
			debug(LOG_ERROR, "Invalid lobby port #");
			return {};
		}
		NETsetMasterserverName(lobbyAddress.c_str());
		NETsetMasterserverPort(lobbyPort);
	}

	auto cleanup = [&]() {
		NETsetMasterserverName(originalLobbyServerName.toUtf8().c_str());
		NETsetMasterserverPort(originalLobbyServerPort);
	};

	if (getLobbyError() != ERROR_INVALID)
	{
		setLobbyError(ERROR_NOERROR);
	}

	GAMESTRUCT lobbyGame;
	memset(&lobbyGame, 0x00, sizeof(lobbyGame));
	if (!NETfindGame(lobbyGameId, lobbyGame))
	{
		// failed to get list of games from lobby server
		debug(LOG_ERROR, "Failed to find gameId in lobby server");
		cleanup();
		return {};
	}

	if (getLobbyError())
	{
		debug(LOG_ERROR, "Lobby error: %d", (int)getLobbyError());
		cleanup();
		return {};
	}

	if (lobbyGame.desc.dwSize == 0)
	{
		debug(LOG_ERROR, "Invalid game struct");
		cleanup();
		return {};
	}

	if (lobbyGame.gameId != lobbyGameId)
	{
		ASSERT(lobbyGame.gameId == lobbyGameId, "NETfindGame returned a non-matching game"); // logic error
		cleanup();
		return {};
	}

	// found the game id, but is it compatible?

	if (!NETisCorrectVersion(lobbyGame.game_version_major, lobbyGame.game_version_minor))
	{
		// incompatible version
		debug(LOG_ERROR, "Failed to find a matching + compatible game in the lobby server");
		cleanup();
		return {};
	}

	// found the game
	if (strlen(lobbyGame.desc.host) == 0)
	{
		debug(LOG_ERROR, "Found the game, but no host details available");
		cleanup();
		return {};
	}
	std::string host = lobbyGame.desc.host;
	return {JoinConnectionDescription(host, lobbyGame.hostPort)};
}

static JoinGameResult joinGameInternal(std::vector<JoinConnectionDescription> connection_list, std::shared_ptr<WzTitleUI> oldUI);
static JoinGameResult joinGameInternalConnect(const char *host, uint32_t port, std::shared_ptr<WzTitleUI> oldUI);

JoinGameResult joinGame(const char *connectionString)
{
	if (strchr(connectionString, '[') == NULL || strchr(connectionString, ']') == NULL) // it is not IPv6. For more see rfc3986 section-3.2.2
	{
		const char* ddch = strchr(connectionString, ':');
		if(ddch != NULL)
		{
			uint32_t serverPort = atoi(ddch+1);
			std::string serverIP = "";
			serverIP.assign(connectionString, ddch - connectionString);
			debug(LOG_INFO, "Connecting to ip [%s] port %d", serverIP.c_str(), serverPort);
			return joinGame(serverIP.c_str(), serverPort);
		}
	}
	return joinGame(connectionString, 0);
}

JoinGameResult joinGame(const char *host, uint32_t port)
{
	std::string hostStr = (host != nullptr) ? std::string(host) : std::string();
	return joinGame(std::vector<JoinConnectionDescription>({JoinConnectionDescription(hostStr, port)}));
}

JoinGameResult joinGame(const std::vector<JoinConnectionDescription>& connection_list) {
	return joinGameInternal(connection_list, wzTitleUICurrent);
}

static JoinGameResult joinGameInternal(std::vector<JoinConnectionDescription> connection_list, std::shared_ptr<WzTitleUI> oldUI){

	if (connection_list.size() > 1)
	{
		// sort the list, based on NETgetJoinPreferenceIPv6
		// preserve the original relative order amongst each class of IPv4/IPv6 addresses
		bool bSortIPv6First = NETgetJoinPreferenceIPv6();
		std::stable_sort(connection_list.begin(), connection_list.end(), [bSortIPv6First](const JoinConnectionDescription& a, const JoinConnectionDescription& b) -> bool {
			bool a_isIPv6 = a.host.find(":") != std::string::npos; // this is a very simplistic test - if the host contains ":" we treat it as IPv6
			bool b_isIPv6 = b.host.find(":") != std::string::npos;
			return (bSortIPv6First) ? (a_isIPv6 && !b_isIPv6) : (!a_isIPv6 && b_isIPv6);
		});
	}

	for (const auto& connDesc : connection_list)
	{
		JoinGameResult result = joinGameInternalConnect(connDesc.host.c_str(), connDesc.port, oldUI);
		switch (result)
		{
			case JoinGameResult::FAILED:
				continue;
			case JoinGameResult::PENDING_PASSWORD:
				return result;
			case JoinGameResult::JOINED:
				ActivityManager::instance().joinGameSucceeded(connDesc.host.c_str(), connDesc.port);
				return result;
		}
	}

	// Failed to connect to all IPs / options in list
	// Change to an error display.
	changeTitleUI(std::make_shared<WzMsgBoxTitleUI>(WzString(_("Error while joining.")), wzTitleUICurrent));
	ActivityManager::instance().joinGameFailed(connection_list);
	return JoinGameResult::FAILED;
}

/**
 * Try connecting to the given host, show a password screen, the multiplayer screen or an error.
 * The reason for this having a third parameter is so that the password dialog
 *  doesn't turn into the parent of the next connection attempt.
 * Any other barriers/auth methods/whatever would presumably benefit in the same way.
 */
static JoinGameResult joinGameInternalConnect(const char *host, uint32_t port, std::shared_ptr<WzTitleUI> oldUI)
{
	// oldUI may get captured for use in the password dialog, among other things.
	PLAYERSTATS	playerStats;

	if (ingame.localJoiningInProgress)
	{
		return JoinGameResult::FAILED;
	}

	if (!NETjoinGame(host, port, (char *)sPlayer))	// join
	{
		switch (getLobbyError())
		{
		case ERROR_HOSTDROPPED:
			setLobbyError(ERROR_NOERROR);
			break;
		case ERROR_WRONGPASSWORD:
			{
				std::string capturedHost = host;
				changeTitleUI(std::make_shared<WzPassBoxTitleUI>([=](const char * pass) {
					if (!pass) {
						changeTitleUI(oldUI);
					} else {
						NETsetGamePassword(pass);
						JoinConnectionDescription conn(capturedHost, port);
						joinGameInternal({conn}, oldUI);
					}
				}));
				return JoinGameResult::PENDING_PASSWORD;
			}
		default:
			break;
		}

		return JoinGameResult::FAILED;
	}
	ingame.localJoiningInProgress	= true;

	loadMultiStats(sPlayer, &playerStats);
	setMultiStats(selectedPlayer, playerStats, false);
	setMultiStats(selectedPlayer, playerStats, true);

	changeTitleUI(std::make_shared<WzMultiplayerOptionsTitleUI>(oldUI));

	if (war_getMPcolour() >= 0)
	{
		SendColourRequest(selectedPlayer, war_getMPcolour());
	}

	return JoinGameResult::JOINED;
}

// ////////////////////////////////////////////////////////////////////////////
// Game Options Screen.

// ////////////////////////////////////////////////////////////////////////////

static void addInlineChooserBlueForm(const std::shared_ptr<W_SCREEN> &psScreen, W_FORM *psParent, UDWORD id, WzString txt, UDWORD x, UDWORD y, UDWORD w, UDWORD h)
{
	W_FORMINIT sFormInit;                  // draw options box.
	sFormInit.formID = MULTIOP_INLINE_OVERLAY_ROOT_FRM;
	sFormInit.id	= id;
	sFormInit.x		= (UWORD) x;
	sFormInit.y		= (UWORD) y;
	sFormInit.style = WFORM_PLAIN;
	sFormInit.width = (UWORD)w;//190;
	sFormInit.height = (UWORD)h; //27;
	sFormInit.pDisplay =  intDisplayFeBox;

	std::weak_ptr<WIDGET> psWeakParent(psParent->shared_from_this());
	sFormInit.calcLayout = [x, y, psWeakParent](WIDGET *psWidget, unsigned int, unsigned int, unsigned int, unsigned int){
		if (auto psParent = psWeakParent.lock())
		{
			psWidget->move(psParent->screenPosX() + x, psParent->screenPosY() + y);
		}
	};
	widgAddForm(psScreen, &sFormInit);
}

static W_FORM * addBlueForm(UDWORD parent, UDWORD id, UDWORD x, UDWORD y, UDWORD w, UDWORD h)
{
	W_FORMINIT sFormInit;                  // draw options box.
	sFormInit.formID = parent;
	sFormInit.id	= id;
	sFormInit.x		= (UWORD) x;
	sFormInit.y		= (UWORD) y;
	sFormInit.style = WFORM_PLAIN;
	sFormInit.width = (UWORD)w;//190;
	sFormInit.height = (UWORD)h; //27;
	sFormInit.pDisplay =  intDisplayFeBox;
	return widgAddForm(psWScreen, &sFormInit);
}


struct LimitIcon
{
	char const *stat;
	char const *desc;
	int         icon;
};
static const LimitIcon limitIcons[] =
{
	{"A0LightFactory",  N_("Tanks disabled!!"),  IMAGE_NO_TANK},
	{"A0CyborgFactory", N_("Cyborgs disabled."), IMAGE_NO_CYBORG},
	{"A0VTolFactory1",  N_("VTOLs disabled."),   IMAGE_NO_VTOL},
	{"A0Sat-linkCentre", N_("Satellite Uplink disabled."), IMAGE_NO_UPLINK},
	{"A0LasSatCommand",  N_("Laser Satellite disabled."),  IMAGE_NO_LASSAT},
	{nullptr,  N_("Structure Limits Enforced."),  IMAGE_DARK_LOCKED},
};

void updateStructureDisabledFlags()
{
	// The host works out the flags.
	if (ingame.side != InGameSide::HOST_OR_SINGLEPLAYER)
	{
		return;
	}

	unsigned flags = ingame.flags & MPFLAGS_FORCELIMITS;

	assert(MPFLAGS_FORCELIMITS == (1 << (ARRAY_SIZE(limitIcons) - 1)));
	for (unsigned i = 0; i < ARRAY_SIZE(limitIcons) - 1; ++i)	// skip last item, MPFLAGS_FORCELIMITS
	{
		int stat = getStructStatFromName(limitIcons[i].stat);
		bool disabled = stat >= 0 && asStructureStats[stat].upgrade[0].limit == 0;
		flags |= disabled << i;
	}

	ingame.flags = flags;
}

static void updateLimitIcons()
{
	widgDelete(psWScreen, MULTIOP_NO_SOMETHING);
	int y = 2;
	bool formBackgroundAdded = false;
	for (int i = 0; i < ARRAY_SIZE(limitIcons); ++i)
	{
		if ((ingame.flags & 1 << i) != 0)
		{
			// only add the background once. Must be added *before* the "icons" as the form acts as their parent
			if (!formBackgroundAdded)
			{
				addBlueForm(MULTIOP_OPTIONS, MULTIOP_NO_SOMETHING, MULTIOP_HOSTX, MULTIOP_NO_SOMETHINGY, MULTIOP_ICON_LIMITS_X2, MULTIOP_ICON_LIMITS_Y2);
				formBackgroundAdded = true;
			}

			addMultiBut(psWScreen, MULTIOP_NO_SOMETHING, MULTIOP_NO_SOMETHINGY + i, MULTIOP_NO_SOMETHINGX, y,
			            35, 28, _(limitIcons[i].desc),
			            limitIcons[i].icon, limitIcons[i].icon, limitIcons[i].icon);
			y += 28 + 3;
		}
	}
}

WzString formatGameName(WzString name)
{
	WzString withoutTechlevel = WzString::fromUtf8(mapNameWithoutTechlevel(name.toUtf8().c_str()));
	return withoutTechlevel + " (T" + WzString::number(game.techLevel) + " " + WzString::number(game.maxPlayers) + "P)";
}

void resetVoteData()
{
	for (unsigned int i = 0; i < MAX_PLAYERS; ++i)
	{
		playerVotes[i] = 0;
	}
}

static void sendVoteData(uint8_t currentVote)
{
	NETbeginEncode(NETbroadcastQueue(), NET_VOTE);
	NETuint32_t(&selectedPlayer);
	NETuint8_t(&currentVote);
	NETend();
}

static uint8_t getVoteTotal()
{
	ASSERT_HOST_ONLY(return true);

	uint8_t total = 0;

	for (unsigned i = 0; i < MAX_PLAYERS; ++i)
	{
		if (isHumanPlayer(i))
		{
			if (selectedPlayer == i)
			{
				// always count the host as a "yes" vote.
				playerVotes[i] = 1;
			}
			total += playerVotes[i];
		}
		else
		{
			playerVotes[i] = 0;
		}
	}

	return total;
}

static bool recvVote(NETQUEUE queue)
{
	ASSERT_HOST_ONLY(return true);

	uint8_t newVote;
	uint32_t player;

	NETbeginDecode(queue, NET_VOTE);
	NETuint32_t(&player); // TODO: check that NETQUEUE belongs to that player :wink:
	NETuint8_t(&newVote);
	NETend();

	if (player >= MAX_PLAYERS)
	{
		debug(LOG_ERROR, "Invalid NET_VOTE from player %d: player id = %d", queue.index, static_cast<int>(player));
		return false;
	}

	playerVotes[player] = (newVote == 1) ? 1 : 0;

	debug(LOG_NET, "total votes: %d/%d", static_cast<int>(getVoteTotal()), static_cast<int>(NET_numHumanPlayers()));

	// there is no "votes" that disallows map change so assume they are all allowing
	if(newVote == 1) {
		char msg[128] = {0};
		ssprintf(msg, _("%s (%d) allowed map change. Total: %d/%d"), NetPlay.players[player].name, player, static_cast<int>(getVoteTotal()), static_cast<int>(NET_numHumanPlayers()));
		sendRoomSystemMessage(msg);
	}

	return true;
}

// Show a vote popup to allow changing maps or using the randomization feature.
static void setupVoteChoice()
{
	//This shouldn't happen...
	if (NetPlay.isHost)
	{
		ASSERT(false, "Host tried to send vote data to themself");
		return;
	}

	if (!hasNotificationsWithTag(VOTE_TAG))
	{
		WZ_Notification notification;
		notification.duration = 0;
		notification.contentTitle = _("Vote");
		notification.contentText = _("Allow host to change map or randomize?");
		notification.action = WZ_Notification_Action("Allow", [](const WZ_Notification&) {
			uint8_t vote = 1;
			sendVoteData(vote);
		});
		notification.tag = VOTE_TAG;

		addNotification(notification, WZ_Notification_Trigger(GAME_TICKS_PER_SEC * 1));
	}
}

static bool canChangeMapOrRandomize()
{
	ASSERT_HOST_ONLY(return true);

	uint8_t numHumans = NET_numHumanPlayers();
	bool allowed = (static_cast<float>(getVoteTotal()) / static_cast<float>(numHumans)) > 0.5f;

	resetVoteData(); //So the host can only do one change every vote session

	if (numHumans == 1)
	{
		return true;
	}

	if (!allowed)
	{
		//setup a vote popup for the clients
		NETbeginEncode(NETbroadcastQueue(), NET_VOTE_REQUEST);
		NETend();

		displayRoomSystemMessage(_("Not enough votes to randomize or change the map."));
	}

	return allowed;
}

static void addMultiButton(std::shared_ptr<MultibuttonWidget> mbw, int value, Image image, Image imageDown, char const *tip)
{
	auto button = std::make_shared<W_BUTTON>();
	button->setImages(image, imageDown, mpwidgetGetFrontHighlightImage(image));
	button->setTip(tip);

	mbw->addButton(value, button);
}

// need to check for side effects.
static void addGameOptions()
{
	widgDelete(psWScreen, MULTIOP_OPTIONS);  				// clear options list
	widgDelete(psWScreen, FRONTEND_SIDETEXT3);				// del text..

	WIDGET *parent = widgGetFromID(psWScreen, FRONTEND_BACKDROP);

	// draw options box.
	auto optionsForm = std::make_shared<IntFormAnimated>(false);
	parent->attach(optionsForm);
	optionsForm->id = MULTIOP_OPTIONS;
	optionsForm->setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
		psWidget->setGeometry(MULTIOP_OPTIONSX, MULTIOP_OPTIONSY, MULTIOP_OPTIONSW, MULTIOP_OPTIONSH);
	}));

	addSideText(FRONTEND_SIDETEXT3, MULTIOP_OPTIONSX - 3 , MULTIOP_OPTIONSY, _("OPTIONS"));

	// game name box
	if (NetPlay.bComms)
	{
		addMultiEditBox(MULTIOP_OPTIONS, MULTIOP_GNAME, MCOL0, MROW2, _("Select Game Name"), game.name, IMAGE_EDIT_GAME, IMAGE_EDIT_GAME_HI, MULTIOP_GNAME_ICON);
	}
	else
	{
		addMultiEditBox(MULTIOP_OPTIONS, MULTIOP_GNAME, MCOL0, MROW2, _("Game Name"),
		                challengeActive ? game.name : _("One-Player Skirmish"), IMAGE_EDIT_GAME,
		                IMAGE_EDIT_GAME_HI, MULTIOP_GNAME_ICON);
		// disable for one-player skirmish
		widgSetButtonState(psWScreen, MULTIOP_GNAME, WEDBS_DISABLE);
	}
	widgSetButtonState(psWScreen, MULTIOP_GNAME_ICON, WBUT_DISABLE);

	// map chooser

	// This is a bit complicated, but basically, see addMultiEditBox,
	//  and then consider that the two buttons are relative to MCOL0, MROW3.
	// MCOL for N >= 1 is basically useless because that's not the actual rule followed by addMultiEditBox.
	// And that's what this panel is meant to align to.
	addBlueForm(MULTIOP_OPTIONS, MULTIOP_MAP, MCOL0, MROW3, MULTIOP_EDITBOXW + MULTIOP_EDITBOXH, MULTIOP_EDITBOXH);
	W_LABINIT sLabInit;
	sLabInit.formID = MULTIOP_MAP;
	sLabInit.id		= MULTIOP_MAP + 1;
	sLabInit.x		= 3;
	sLabInit.y		= 4;
	sLabInit.width	= MULTIOP_EDITBOXW - 24 - 5;
	sLabInit.height = 20;
	sLabInit.pText	= formatGameName(game.map);
	widgAddLabel(psWScreen, &sLabInit);
	addMultiBut(psWScreen, MULTIOP_MAP, MULTIOP_MAP_ICON, MULTIOP_EDITBOXW + 2, 2, MULTIOP_EDITBOXH, MULTIOP_EDITBOXH, _("Select Map\nCan be blocked by players' votes"), IMAGE_EDIT_MAP, IMAGE_EDIT_MAP_HI, true);
	addMultiBut(psWScreen, MULTIOP_MAP, MULTIOP_MAP_MOD, MULTIOP_EDITBOXW - 14, 1, 12, 12, _("Map-Mod!"), IMAGE_LAMP_RED, IMAGE_LAMP_AMBER, false);
	addMultiBut(psWScreen, MULTIOP_MAP, MULTIOP_MAP_RANDOM, MULTIOP_EDITBOXW - 24, 15, 12, 12, _("Random map!"), IMAGE_WEE_DIE, IMAGE_WEE_DIE, false);
	if (!game.isMapMod)
	{
		widgHide(psWScreen, MULTIOP_MAP_MOD);
	}
	if (!game.isRandom)
	{
		widgHide(psWScreen, MULTIOP_MAP_RANDOM);
	}
	// disable for challenges
	if (challengeActive)
	{
		widgSetButtonState(psWScreen, MULTIOP_MAP_ICON, WBUT_DISABLE);
	}
	// password box
	if (NetPlay.bComms && ingame.side == InGameSide::HOST_OR_SINGLEPLAYER)
	{
		auto editBox = addMultiEditBox(MULTIOP_OPTIONS, MULTIOP_PASSWORD_EDIT, MCOL0, MROW4, _("Click to set Password"), NetPlay.gamePassword, IMAGE_UNLOCK_BLUE, IMAGE_LOCK_BLUE, MULTIOP_PASSWORD_BUT);
		editBox->setPlaceholder(_("Enter password here"));
		auto *pPasswordButton = dynamic_cast<WzMultiButton*>(widgGetFromID(psWScreen, MULTIOP_PASSWORD_BUT));
		if (pPasswordButton)
		{
			pPasswordButton->minClickInterval = GAME_TICKS_PER_SEC / 2;
		}
		if (NetPlay.GamePassworded)
		{
			widgSetButtonState(psWScreen, MULTIOP_PASSWORD_BUT, WBUT_CLICKLOCK);
			widgSetButtonState(psWScreen, MULTIOP_PASSWORD_EDIT, WEDBS_DISABLE);
		}
	}

	//just display the game options.
	addMultiEditBox(MULTIOP_OPTIONS, MULTIOP_PNAME, MCOL0, MROW1, _("Select Player Name"), (char *) sPlayer, IMAGE_EDIT_PLAYER, IMAGE_EDIT_PLAYER_HI, MULTIOP_PNAME_ICON);

	auto optionsList = std::make_shared<ListWidget>();
	optionsForm->attach(optionsList);
	optionsList->setChildSize(MULTIOP_BLUEFORMW, 29);
	optionsList->setChildSpacing(2, 2);
	optionsList->setGeometry(MCOL0, MROW5, MULTIOP_BLUEFORMW, optionsForm->height() - MROW5);

	auto scavengerChoice = std::make_shared<MultichoiceWidget>(game.scavengers);
	optionsList->attach(scavengerChoice);
	scavengerChoice->id = MULTIOP_GAMETYPE;
	scavengerChoice->setLabel(_("Scavengers"));
	if (game.mapHasScavengers)
	{
		addMultiButton(scavengerChoice, true, Image(FrontImages, IMAGE_SCAVENGERS_ON), Image(FrontImages, IMAGE_SCAVENGERS_ON_HI), _("Scavengers"));
	}
	addMultiButton(scavengerChoice, false, Image(FrontImages, IMAGE_SCAVENGERS_OFF), Image(FrontImages, IMAGE_SCAVENGERS_OFF_HI), _("No Scavengers"));
	scavengerChoice->enable(!locked.scavengers);
	optionsList->addWidgetToLayout(scavengerChoice);

	auto allianceChoice = std::make_shared<MultichoiceWidget>(game.alliance);
	optionsList->attach(allianceChoice);
	allianceChoice->id = MULTIOP_ALLIANCES;
	allianceChoice->setLabel(_("Alliances"));
	addMultiButton(allianceChoice, NO_ALLIANCES, Image(FrontImages, IMAGE_NOALLI), Image(FrontImages, IMAGE_NOALLI_HI), _("No Alliances"));
	addMultiButton(allianceChoice, ALLIANCES, Image(FrontImages, IMAGE_ALLI), Image(FrontImages, IMAGE_ALLI_HI), _("Allow Alliances"));
	addMultiButton(allianceChoice, ALLIANCES_UNSHARED, Image(FrontImages, IMAGE_ALLI_UNSHARED), Image(FrontImages, IMAGE_ALLI_UNSHARED_HI), _("Locked Teams, No Shared Research"));
	addMultiButton(allianceChoice, ALLIANCES_TEAMS, Image(FrontImages, IMAGE_ALLI_TEAMS), Image(FrontImages, IMAGE_ALLI_TEAMS_HI), _("Locked Teams"));
	allianceChoice->enable(!locked.alliances);
	optionsList->addWidgetToLayout(allianceChoice);

	auto powerChoice = std::make_shared<MultichoiceWidget>(game.power);
	optionsList->attach(powerChoice);
	powerChoice->id = MULTIOP_POWER;
	powerChoice->setLabel(_("Power"));
	addMultiButton(powerChoice, LEV_LOW, Image(FrontImages, IMAGE_POWLO), Image(FrontImages, IMAGE_POWLO_HI), _("Low Power Levels"));
	addMultiButton(powerChoice, LEV_MED, Image(FrontImages, IMAGE_POWMED), Image(FrontImages, IMAGE_POWMED_HI), _("Medium Power Levels"));
	addMultiButton(powerChoice, LEV_HI, Image(FrontImages, IMAGE_POWHI), Image(FrontImages, IMAGE_POWHI_HI), _("High Power Levels"));
	powerChoice->enable(!locked.power);
	optionsList->addWidgetToLayout(powerChoice);

	auto baseTypeChoice = std::make_shared<MultichoiceWidget>(game.base);
	optionsList->attach(baseTypeChoice);
	baseTypeChoice->id = MULTIOP_BASETYPE;
	baseTypeChoice->setLabel(_("Base"));
	addMultiButton(baseTypeChoice, CAMP_CLEAN, Image(FrontImages, IMAGE_NOBASE), Image(FrontImages, IMAGE_NOBASE_HI), _("Start with No Bases"));
	addMultiButton(baseTypeChoice, CAMP_BASE, Image(FrontImages, IMAGE_SBASE), Image(FrontImages, IMAGE_SBASE_HI), _("Start with Bases"));
	addMultiButton(baseTypeChoice, CAMP_WALLS, Image(FrontImages, IMAGE_LBASE), Image(FrontImages, IMAGE_LBASE_HI), _("Start with Advanced Bases"));
	baseTypeChoice->enable(!locked.bases);
	optionsList->addWidgetToLayout(baseTypeChoice);

	auto mapPreviewButton = std::make_shared<MultibuttonWidget>();
	optionsList->attach(mapPreviewButton);
	mapPreviewButton->id = MULTIOP_MAP_PREVIEW;
	mapPreviewButton->setLabel(_("Map Preview"));
	addMultiButton(mapPreviewButton, 0, Image(FrontImages, IMAGE_FOG_OFF), Image(FrontImages, IMAGE_FOG_OFF_HI), _("Click to see Map"));
	optionsList->addWidgetToLayout(mapPreviewButton);

	/* Add additional controls if we are (or going to be) hosting the game */
	if (ingame.side == InGameSide::HOST_OR_SINGLEPLAYER)
	{
		auto structureLimitsLabel = challengeActive ? _("Show Structure Limits") : _("Set Structure Limits");
		auto structLimitsButton = std::make_shared<MultibuttonWidget>();
		optionsList->attach(structLimitsButton);
		structLimitsButton->id = MULTIOP_STRUCTLIMITS;
		structLimitsButton->setLabel(structureLimitsLabel);
		addMultiButton(structLimitsButton, 0, Image(FrontImages, IMAGE_SLIM), Image(FrontImages, IMAGE_SLIM_HI), structureLimitsLabel);
		optionsList->addWidgetToLayout(structLimitsButton);

		/* ...and even more controls if we are not starting a challenge */
		if (!challengeActive)
		{
			auto randomButton = std::make_shared<MultibuttonWidget>();
			optionsList->attach(randomButton);
			randomButton->id = MULTIOP_RANDOM;
			randomButton->setLabel(_("Random Game Options"));
			addMultiButton(randomButton, 0, Image(FrontImages, IMAGE_RELOAD), Image(FrontImages, IMAGE_RELOAD), _("Random Game Options\nCan be blocked by players' votes"));
			randomButton->setButtonMinClickInterval(GAME_TICKS_PER_SEC / 2);
			optionsList->addWidgetToLayout(randomButton);

			/* Add the tech level choice if we have already started hosting. The only real reason this is displayed only after
			   starting the host is due to the fact that there is not enough room before the "Host Game" button is hidden.		*/
			if (NetPlay.isHost)
			{
				auto TechnologyChoice = std::make_shared<MultichoiceWidget>(game.techLevel);
				optionsList->attach(TechnologyChoice);
				TechnologyChoice->id = MULTIOP_TECHLEVEL;
				TechnologyChoice->setLabel(_("Tech"));
				addMultiButton(TechnologyChoice, TECH_1, Image(FrontImages, IMAGE_TECHLO), Image(FrontImages, IMAGE_TECHLO_HI), _("Technology Level 1"));
				addMultiButton(TechnologyChoice, TECH_2, Image(FrontImages, IMAGE_TECHMED), Image(FrontImages, IMAGE_TECHMED_HI), _("Technology Level 2"));
				addMultiButton(TechnologyChoice, TECH_3, Image(FrontImages, IMAGE_TECHHI), Image(FrontImages, IMAGE_TECHHI_HI), _("Technology Level 3"));
				addMultiButton(TechnologyChoice, TECH_4, Image(FrontImages, IMAGE_COMPUTER_Y), Image(FrontImages, IMAGE_COMPUTER_Y_HI), _("Technology Level 4"));
				optionsList->addWidgetToLayout(TechnologyChoice);
			}
			/* If not hosting (yet), add the button for starting the host. */
			else
			{
				auto hostButton = std::make_shared<MultibuttonWidget>();
				optionsList->attach(hostButton);
				hostButton->id = MULTIOP_HOST;
				hostButton->setLabel(_("Start Hosting Game"));
				addMultiButton(hostButton, 0, Image(FrontImages, IMAGE_HOST), Image(FrontImages, IMAGE_HOST_HI), _("Start Hosting Game"));
				optionsList->addWidgetToLayout(hostButton);
			}
		}
	}

	// cancel
	addMultiBut(psWScreen, MULTIOP_OPTIONS, CON_CANCEL,
	            MULTIOP_CANCELX, MULTIOP_CANCELY,
	            iV_GetImageWidth(FrontImages, IMAGE_RETURN),
	            iV_GetImageHeight(FrontImages, IMAGE_RETURN),
	            _("Return To Previous Screen"), IMAGE_RETURN, IMAGE_RETURN_HI, IMAGE_RETURN_HI);

	// Add any relevant factory disabled icons.
	updateStructureDisabledFlags();
	updateLimitIcons();
}

// ////////////////////////////////////////////////////////////////////////////
// Colour functions

static bool safeToUseColour(unsigned player, unsigned otherPlayer)
{
	// Player wants to take the colour from otherPlayer. May not take from a human otherPlayer, unless we're the host.
	return player == otherPlayer || NetPlay.isHost || !isHumanPlayer(otherPlayer);
}

static int getPlayerTeam(int i)
{
	return alliancesSetTeamsBeforeGame(game.alliance) ? NetPlay.players[i].team : i;
}

/**
 * Checks if all players are on the same team. If so, return that team; if not, return -1;
 * if there are no players, return team MAX_PLAYERS.
 */
static int allPlayersOnSameTeam(int except)
{
	int minTeam = MAX_PLAYERS, maxTeam = 0, numPlayers = 0;
	for (unsigned i = 0; i < game.maxPlayers; ++i)
	{
		if (i != except && (NetPlay.players[i].allocated || NetPlay.players[i].ai >= 0))
		{
			int team = getPlayerTeam(i);
			minTeam = std::min(minTeam, team);
			maxTeam = std::max(maxTeam, team);
			++numPlayers;
		}
	}
	if (minTeam == MAX_PLAYERS || minTeam == maxTeam)
	{
		return minTeam;  // Players all on same team.
	}
	return -1;  // Players not all on same team.
}

static int playerBoxHeight(int player)
{
	int gap = MULTIOP_PLAYERSH - MULTIOP_TEAMSHEIGHT * game.maxPlayers;
	int gapDiv = game.maxPlayers - 1;
	gap = std::min(gap, 5 * gapDiv);
	STATIC_ASSERT(MULTIOP_TEAMSHEIGHT == MULTIOP_PLAYERHEIGHT);  // Why are these different defines?
	return (MULTIOP_TEAMSHEIGHT * gapDiv + gap) * NetPlay.players[player].position / gapDiv;
}

void WzMultiplayerOptionsTitleUI::closeAllChoosers()
{
	closeColourChooser();
	closeTeamChooser();
	closeFactionChooser();
	closePositionChooser();

	// AiChooser and DifficultyChooser currently use the same form, so to avoid a double-delete-later, do it once explicitly here
	widgDeleteLater(psInlineChooserOverlayScreen, MULTIOP_AI_FORM);
	widgDeleteLater(psInlineChooserOverlayScreen, FRONTEND_SIDETEXT2);
	aiChooserUp = -1;
	difficultyChooserUp = -1;
	widgRemoveOverlayScreen(psInlineChooserOverlayScreen);
}

void WzMultiplayerOptionsTitleUI::initInlineChooser(uint32_t player)
{
	// delete everything on that player's row,
	widgDelete(psWScreen, MULTIOP_PLAYER_START + player);
	widgDelete(psWScreen, MULTIOP_TEAMS_START + player);
	widgDelete(psWScreen, MULTIOP_READY_FORM_ID + player);
	widgDelete(psWScreen, MULTIOP_COLOUR_START + player);
	widgDelete(psWScreen, MULTIOP_FACTION_START + player);

	// remove any choosers already up
	closeAllChoosers();

	widgRegisterOverlayScreen(psInlineChooserOverlayScreen, 1);
}

IntFormAnimated* WzMultiplayerOptionsTitleUI::initRightSideChooser(const char* sideText)
{
	// remove any choosers already up
	closeAllChoosers();

	// delete everything on that player's row,
	widgDelete(psWScreen, MULTIOP_PLAYERS);
	widgDelete(psWScreen, FRONTEND_SIDETEXT2);

	widgRegisterOverlayScreen(psInlineChooserOverlayScreen, 1);

	WIDGET* psParent = widgGetFromID(psWScreen, FRONTEND_BACKDROP);
	if (psParent == nullptr)
	{
		return nullptr;
	}
	std::weak_ptr<WIDGET> psWeakParent(psParent->shared_from_this());

	WIDGET *chooserParent = widgGetFromID(psInlineChooserOverlayScreen, MULTIOP_INLINE_OVERLAY_ROOT_FRM);

	auto aiForm = std::make_shared<IntFormAnimated>(false);
	chooserParent->attach(aiForm);
	aiForm->id = MULTIOP_AI_FORM;
	aiForm->setCalcLayout([psWeakParent](WIDGET *psWidget, unsigned int, unsigned int, unsigned int, unsigned int){
		if (auto psParent = psWeakParent.lock())
		{
			psWidget->setGeometry(psParent->screenPosX() + MULTIOP_PLAYERSX, psParent->screenPosY() + MULTIOP_PLAYERSY, MULTIOP_PLAYERSW, MULTIOP_PLAYERSH);
		}
	});

	W_LABEL *psSideTextLabel = addSideText(psInlineChooserOverlayScreen, MULTIOP_INLINE_OVERLAY_ROOT_FRM, FRONTEND_SIDETEXT2, MULTIOP_PLAYERSX - 3, MULTIOP_PLAYERSY, sideText);
	if (psSideTextLabel)
	{
		psSideTextLabel->setCalcLayout([psWeakParent](WIDGET *psWidget, unsigned int, unsigned int, unsigned int, unsigned int){
			if (auto psParent = psWeakParent.lock())
			{
				psWidget->setGeometry(psParent->screenPosX() + MULTIOP_PLAYERSX - 3, psParent->screenPosY() + MULTIOP_PLAYERSY, MULTIOP_PLAYERSW, MULTIOP_PLAYERSH);
			}
		});
	}

	return aiForm.get();
}

static bool addMultiButWithClickHandler(const std::shared_ptr<W_SCREEN> &screen, UDWORD formid, UDWORD id, UDWORD x, UDWORD y, UDWORD width, UDWORD height, const char *tipres, UDWORD norm, UDWORD down, UDWORD hi, const W_BUTTON::W_BUTTON_ONCLICK_FUNC& clickHandler, unsigned tc = MAX_PLAYERS)
{
	if (!addMultiBut(screen, formid, id, x, y, width, height, tipres, norm, down, hi, tc))
	{
		return false;
	}
	WzMultiButton *psButton = static_cast<WzMultiButton*>(widgGetFromID(screen, id));
	if (!psButton)
	{
		return false;
	}
	psButton->addOnClickHandler(clickHandler);
	return true;
}

void WzMultiplayerOptionsTitleUI::openDifficultyChooser(uint32_t player)
{
	IntFormAnimated *aiForm = initRightSideChooser(_("DIFFICULTY"));
	if (!aiForm)
	{
		debug(LOG_ERROR, "Failed to initialize right-side chooser?");
		return;
	}

	auto psWeakTitleUI = std::weak_ptr<WzMultiplayerOptionsTitleUI>(std::dynamic_pointer_cast<WzMultiplayerOptionsTitleUI>(shared_from_this()));

	for (int difficultyIdx = 0; difficultyIdx < 4; difficultyIdx++)
	{
		auto onClickHandler = [psWeakTitleUI, difficultyIdx, player](W_BUTTON& clickedButton) {
			auto pStrongPtr = psWeakTitleUI.lock();
			ASSERT_OR_RETURN(, pStrongPtr.operator bool(), "WzMultiplayerOptionsTitleUI no longer exists");
			NetPlay.players[player].difficulty = difficultyValue[difficultyIdx];
			NETBroadcastPlayerInfo(player);
			pStrongPtr->closeDifficultyChooser();
			pStrongPtr->addPlayerBox(true);
			resetReadyStatus(false);
		};

		W_BUTINIT sButInit;
		sButInit.formID = MULTIOP_AI_FORM;
		sButInit.id = MULTIOP_DIFFICULTY_CHOOSE_START + difficultyIdx;
		sButInit.x = 7;
		sButInit.y = (MULTIOP_PLAYERHEIGHT + 5) * difficultyIdx + 4;
		sButInit.width = MULTIOP_PLAYERWIDTH + 1;
		sButInit.height = MULTIOP_PLAYERHEIGHT;
		switch (difficultyIdx)
		{
		case 0: sButInit.pTip = _("Starts disadvantaged"); break;
		case 1: sButInit.pTip = _("Plays nice"); break;
		case 2: sButInit.pTip = _("No holds barred"); break;
		case 3: sButInit.pTip = _("Starts with advantages"); break;
		}
		const char *difficultyTip = aidata[NetPlay.players[player].ai].difficultyTips[difficultyIdx];
		if (strcmp(difficultyTip, "") != 0)
		{
			sButInit.pTip += "\n";
			sButInit.pTip += difficultyTip;
		}
		sButInit.pDisplay = displayDifficulty;
		sButInit.UserData = difficultyIdx;
		sButInit.pUserData = new DisplayDifficultyCache();
		sButInit.onDelete = [](WIDGET *psWidget) {
			assert(psWidget->pUserData != nullptr);
			delete static_cast<DisplayDifficultyCache *>(psWidget->pUserData);
			psWidget->pUserData = nullptr;
		};
		auto psButton = widgAddButton(psInlineChooserOverlayScreen, &sButInit);
		if (psButton)
		{
			psButton->addOnClickHandler(onClickHandler);
		}
	}

	difficultyChooserUp = player;
}

void WzMultiplayerOptionsTitleUI::openAiChooser(uint32_t player)
{
	IntFormAnimated *aiForm = initRightSideChooser(_("CHOOSE AI"));
	if (!aiForm)
	{
		debug(LOG_ERROR, "Failed to initialize right-side chooser?");
		return;
	}

	auto psWeakTitleUI = std::weak_ptr<WzMultiplayerOptionsTitleUI>(std::dynamic_pointer_cast<WzMultiplayerOptionsTitleUI>(shared_from_this()));

	W_BUTINIT sButInit;
	sButInit.formID = MULTIOP_AI_FORM;
	sButInit.x = 7;
	sButInit.width = MULTIOP_PLAYERWIDTH + 1;
	sButInit.height = MULTIOP_PLAYERHEIGHT;
	sButInit.pDisplay = displayAi;
	sButInit.initPUserDataFunc = []() -> void * { return new DisplayAICache(); };
	sButInit.onDelete = [](WIDGET *psWidget) {
		assert(psWidget->pUserData != nullptr);
		delete static_cast<DisplayAICache *>(psWidget->pUserData);
		psWidget->pUserData = nullptr;
	};

	auto openCloseOnClickHandler = [psWeakTitleUI, player](W_BUTTON& clickedButton) {
		auto pStrongPtr = psWeakTitleUI.lock();
		ASSERT_OR_RETURN(, pStrongPtr.operator bool(), "WzMultiplayerOptionsTitleUI no longer exists");

		switch (clickedButton.id)
		{
		case MULTIOP_AI_CLOSED:
			NetPlay.players[player].ai = AI_CLOSED;
			break;
		case MULTIOP_AI_OPEN:
			NetPlay.players[player].ai = AI_OPEN;
			break;
		default:
			debug(LOG_ERROR, "Unexpected button id");
			return;
			break;
		}

		// common code
		NetPlay.players[player].difficulty = AIDifficulty::DISABLED; // disable AI for this slot
		NETBroadcastPlayerInfo(player);
		pStrongPtr->closeAiChooser();
		pStrongPtr->addPlayerBox(true);
		resetReadyStatus(false);
		ActivityManager::instance().updateMultiplayGameData(game, ingame, NETGameIsLocked());
	};

	// Open button
	if (NetPlay.bComms)
	{
		sButInit.id = MULTIOP_AI_OPEN;
		sButInit.pTip = _("Allow human players to join in this slot");
		sButInit.UserData = (UDWORD)AI_OPEN;
		sButInit.y = 3;	//Top most position
		auto psButton = widgAddButton(psInlineChooserOverlayScreen, &sButInit);
		if (psButton)
		{
			psButton->addOnClickHandler(openCloseOnClickHandler);
		}
	}

	// Closed button
	sButInit.pTip = _("Leave this slot unused");
	sButInit.id = MULTIOP_AI_CLOSED;
	sButInit.UserData = (UDWORD)AI_CLOSED;
	if (NetPlay.bComms)
	{
		sButInit.y = sButInit.y + sButInit.height;
	}
	else
	{
		sButInit.y = 3; //since we don't have the lone mpbutton, we can start at position 0
	}
	auto psCloseButton = widgAddButton(psInlineChooserOverlayScreen, &sButInit);
	if (psCloseButton)
	{
		psCloseButton->addOnClickHandler(openCloseOnClickHandler);
	}

	auto pAIScrollableList = ScrollableListWidget::make();
	aiForm->attach(pAIScrollableList);
	pAIScrollableList->setBackgroundColor(WZCOL_TRANSPARENT_BOX);
	int aiListStartXPos = sButInit.x;
	int aiListStartYPos = (sButInit.height + sButInit.y) + 10;
	int aiListEntryHeight = sButInit.height;
	int aiListEntryWidth = sButInit.width;
	int aiListHeight = aiListEntryHeight * (NetPlay.bComms ? 7 : 8);
	pAIScrollableList->setCalcLayout([aiListStartXPos, aiListStartYPos, aiListEntryWidth, aiListHeight](WIDGET *psWidget, unsigned int, unsigned int, unsigned int, unsigned int){
		psWidget->setGeometry(aiListStartXPos, aiListStartYPos, aiListEntryWidth, aiListHeight);
	});

	W_BUTINIT emptyInit;
	for (size_t aiIdx = 0; aiIdx < aidata.size(); aiIdx++)
	{
		auto pAIRow = std::make_shared<W_BUTTON>(&emptyInit);
		pAIRow->setTip(aidata[aiIdx].tip);
		pAIRow->id = MULTIOP_AI_START + aiIdx;
		pAIRow->UserData = aiIdx;
		pAIRow->setGeometry(0, 0, sButInit.width, sButInit.height);
		pAIRow->displayFunction = displayAi;
		pAIRow->pUserData = new DisplayAICache();
		pAIRow->setOnDelete([](WIDGET *psWidget) {
			assert(psWidget->pUserData != nullptr);
			delete static_cast<DisplayAICache *>(psWidget->pUserData);
			psWidget->pUserData = nullptr;
		});
		pAIRow->addOnClickHandler([psWeakTitleUI, aiIdx, player](W_BUTTON& clickedButton) {
			auto pStrongPtr = psWeakTitleUI.lock();
			ASSERT_OR_RETURN(, pStrongPtr.operator bool(), "WzMultiplayerOptionsTitleUI no longer exists");
			NetPlay.players[player].ai = aiIdx;
			sstrcpy(NetPlay.players[player].name, getAIName(player));
			NetPlay.players[player].difficulty = AIDifficulty::MEDIUM;
			NETBroadcastPlayerInfo(player);
			pStrongPtr->closeAiChooser();
			pStrongPtr->addPlayerBox(true);
			resetReadyStatus(false);
			ActivityManager::instance().updateMultiplayGameData(game, ingame, NETGameIsLocked());
		});
		pAIScrollableList->addItem(pAIRow);
	}

	aiChooserUp = player;
}

void WzMultiplayerOptionsTitleUI::openPositionChooser(uint32_t player)
{
	closeAllChoosers();

	positionChooserUp = player;
	addPlayerBox(true);
}

static bool SendTeamRequest(UBYTE player, UBYTE chosenTeam); // forward-declare

void WzMultiplayerOptionsTitleUI::openTeamChooser(uint32_t player)
{
	UDWORD i;
	int disallow = allPlayersOnSameTeam(player);

	bool canChangeTeams = !locked.teams;
	bool canKickPlayer = player != selectedPlayer && NetPlay.bComms && NetPlay.isHost && NetPlay.players[player].allocated;
	if (!canChangeTeams && !canKickPlayer)
	{
		return;
	}

	debug(LOG_NET, "Opened team chooser for %d, current team: %d", player, NetPlay.players[player].team);

	initInlineChooser(player);

	// add form.
	auto psParentForm = (W_FORM *)widgGetFromID(psWScreen, MULTIOP_PLAYERS);
	addInlineChooserBlueForm(psInlineChooserOverlayScreen, psParentForm, MULTIOP_TEAMCHOOSER_FORM, "", 8, playerBoxHeight(player), MULTIOP_ROW_WIDTH, MULTIOP_TEAMSHEIGHT);

	auto psWeakTitleUI = std::weak_ptr<WzMultiplayerOptionsTitleUI>(std::dynamic_pointer_cast<WzMultiplayerOptionsTitleUI>(shared_from_this()));

	if (canChangeTeams)
	{
		int teamW = iV_GetImageWidth(FrontImages, IMAGE_TEAM0);
		int teamH = iV_GetImageHeight(FrontImages, IMAGE_TEAM0);
		int space = MULTIOP_ROW_WIDTH - 4 - teamW * (game.maxPlayers + 1);
		int spaceDiv = game.maxPlayers;
		space = std::min(space, 3 * spaceDiv);

		auto onClickHandler = [player, psWeakTitleUI](W_BUTTON &button) {
			UDWORD id = button.id;
			auto pStrongPtr = psWeakTitleUI.lock();
			ASSERT_OR_RETURN(, pStrongPtr.operator bool(), "WzMultiplayerOptionsTitleUI no longer exists");

			//clicked on a team
			STATIC_ASSERT(MULTIOP_TEAMCHOOSER + MAX_PLAYERS - 1 <= MULTIOP_TEAMCHOOSER_END);
			if (id >= MULTIOP_TEAMCHOOSER && id <= MULTIOP_TEAMCHOOSER + MAX_PLAYERS - 1)
			{
				ASSERT(id >= MULTIOP_TEAMCHOOSER
					   && (id - MULTIOP_TEAMCHOOSER) < MAX_PLAYERS, "processMultiopWidgets: wrong id - MULTIOP_TEAMCHOOSER value (%d)", id - MULTIOP_TEAMCHOOSER);

				resetReadyStatus(false);		// will reset only locally if not a host

				SendTeamRequest(player, (UBYTE)id - MULTIOP_TEAMCHOOSER);

				debug(LOG_WZ, "Changed team for player %d to %d", player, NetPlay.players[player].team);

				pStrongPtr->closeTeamChooser();

				// restore player list
				pStrongPtr->addPlayerBox(true);
			}
		};

		// add the teams, skipping the one we CAN'T be on (if applicable)
		for (i = 0; i < game.maxPlayers; i++)
		{
			if (i != disallow)
			{
				addMultiButWithClickHandler(psInlineChooserOverlayScreen, MULTIOP_TEAMCHOOSER_FORM, MULTIOP_TEAMCHOOSER + i, i * (teamW * spaceDiv + space) / spaceDiv + 3, 6, teamW, teamH, _("Team"), IMAGE_TEAM0 + i, IMAGE_TEAM0_HI + i, IMAGE_TEAM0_HI + i, onClickHandler);
			}
			// may want to add some kind of 'can't do' icon instead of being blank?
		}
	}

	// add a kick button
	if (canKickPlayer)
	{
		auto onClickHandler = [player, psWeakTitleUI](W_BUTTON &button) {
			auto pStrongPtr = psWeakTitleUI.lock();
			ASSERT_OR_RETURN(, pStrongPtr.operator bool(), "WzMultiplayerOptionsTitleUI no longer exists");

			std::string msg = astringf(_("The host has kicked %s from the game!"), getPlayerName(player));
			kickPlayer(player, _("The host has kicked you from the game."), ERROR_KICKED);
			sendRoomSystemMessage(msg.c_str());
			resetReadyStatus(true);		//reset and send notification to all clients
			pStrongPtr->closeTeamChooser();
		};

		const int imgwidth = iV_GetImageWidth(FrontImages, IMAGE_NOJOIN);
		const int imgheight = iV_GetImageHeight(FrontImages, IMAGE_NOJOIN);
		addMultiButWithClickHandler(psInlineChooserOverlayScreen, MULTIOP_TEAMCHOOSER_FORM, MULTIOP_TEAMCHOOSER_KICK, MULTIOP_ROW_WIDTH - imgwidth - 4, 8, imgwidth, imgheight,
			("Kick player"), IMAGE_NOJOIN, IMAGE_NOJOIN, IMAGE_NOJOIN, onClickHandler);
	}

	inlineChooserUp = player;
}

void WzMultiplayerOptionsTitleUI::openColourChooser(uint32_t player)
{
	ASSERT_OR_RETURN(, player < MAX_PLAYERS, "Invalid player number");
	initInlineChooser(player);

	// add form.
	auto psParentForm = (W_FORM *)widgGetFromID(psWScreen, MULTIOP_PLAYERS);
	addInlineChooserBlueForm(psInlineChooserOverlayScreen, psParentForm, MULTIOP_COLCHOOSER_FORM, "",
		7,
		playerBoxHeight(player),
		MULTIOP_ROW_WIDTH, MULTIOP_PLAYERHEIGHT);

	// add the flags
	int flagW = iV_GetImageWidth(FrontImages, IMAGE_PLAYERN);
	int flagH = iV_GetImageHeight(FrontImages, IMAGE_PLAYERN);
	int space = MULTIOP_ROW_WIDTH - 0 - flagW * MAX_PLAYERS_IN_GUI;
	int spaceDiv = MAX_PLAYERS_IN_GUI;
	space = std::min(space, 5 * spaceDiv);

	auto psWeakTitleUI = std::weak_ptr<WzMultiplayerOptionsTitleUI>(std::dynamic_pointer_cast<WzMultiplayerOptionsTitleUI>(shared_from_this()));

	for (unsigned i = 0; i < MAX_PLAYERS_IN_GUI; i++)
	{
		auto onClickHandler = [player, psWeakTitleUI](W_BUTTON &button) {
			UDWORD id = button.id;
			auto pStrongPtr = psWeakTitleUI.lock();
			ASSERT_OR_RETURN(, pStrongPtr.operator bool(), "WzMultiplayerOptionsTitleUI no longer exists");

			STATIC_ASSERT(MULTIOP_COLCHOOSER + MAX_PLAYERS - 1 <= MULTIOP_COLCHOOSER_END);
			if (id >= MULTIOP_COLCHOOSER && id < MULTIOP_COLCHOOSER + MAX_PLAYERS - 1)  // chose a new colour.
			{
				resetReadyStatus(false, true);		// will reset only locally if not a host
				SendColourRequest(player, id - MULTIOP_COLCHOOSER);
				pStrongPtr->closeColourChooser();
				pStrongPtr->addPlayerBox(true);
			}
		};

		addMultiButWithClickHandler(psInlineChooserOverlayScreen, MULTIOP_COLCHOOSER_FORM, MULTIOP_COLCHOOSER + getPlayerColour(i),
			i * (flagW * spaceDiv + space) / spaceDiv + 4, 4, // x, y
			flagW, flagH,  // w, h
			getPlayerColourName(i), IMAGE_PLAYERN, IMAGE_PLAYERN_HI, IMAGE_PLAYERN_HI, onClickHandler, getPlayerColour(i)
		);

		if (!safeToUseColour(selectedPlayer, i))
		{
			widgSetButtonState(psInlineChooserOverlayScreen, MULTIOP_COLCHOOSER + getPlayerColour(i), WBUT_DISABLE);
		}
	}

	inlineChooserUp = player;
}

void WzMultiplayerOptionsTitleUI::closeColourChooser()
{
	inlineChooserUp = -1;
	widgDeleteLater(psInlineChooserOverlayScreen, MULTIOP_COLCHOOSER_FORM);
	widgRemoveOverlayScreen(psInlineChooserOverlayScreen);
}

void WzMultiplayerOptionsTitleUI::closeTeamChooser()
{
	inlineChooserUp = -1;
	widgDeleteLater(psInlineChooserOverlayScreen, MULTIOP_TEAMCHOOSER_FORM);
	widgRemoveOverlayScreen(psInlineChooserOverlayScreen);
}

void WzMultiplayerOptionsTitleUI::closeFactionChooser()
{
	inlineChooserUp = -1;
	widgDeleteLater(psInlineChooserOverlayScreen, MULTIOP_FACCHOOSER_FORM);
	widgRemoveOverlayScreen(psInlineChooserOverlayScreen);
}

void WzMultiplayerOptionsTitleUI::closeAiChooser()
{
	// AiChooser and DifficultyChooser currently use the same formID
	// Just call closeAllChoosers() for now
	closeAllChoosers();
}

void WzMultiplayerOptionsTitleUI::closeDifficultyChooser()
{
	// AiChooser and DifficultyChooser currently use the same formID
	// Just call closeAllChoosers() for now
	closeAllChoosers();
}

void WzMultiplayerOptionsTitleUI::closePositionChooser()
{
	positionChooserUp = -1;
}

void WzMultiplayerOptionsTitleUI::openFactionChooser(uint32_t player)
{
	ASSERT_OR_RETURN(, player < MAX_PLAYERS, "Invalid player number");
	initInlineChooser(player);

	// add form.
	auto psParentForm = (W_FORM *)widgGetFromID(psWScreen, MULTIOP_PLAYERS);
	addInlineChooserBlueForm(psInlineChooserOverlayScreen, psParentForm, MULTIOP_FACCHOOSER_FORM, "",
		7,
		playerBoxHeight(player),
		MULTIOP_ROW_WIDTH, MULTIOP_PLAYERHEIGHT);

	// add the flags
	int flagW = iV_GetImageWidth(FrontImages, IMAGE_FACTION_NORMAL) + 4;
	int flagH = iV_GetImageHeight(FrontImages, IMAGE_PLAYERN);
	int space = MULTIOP_ROW_WIDTH - 0 - flagW * NUM_FACTIONS;
	int spaceDiv = NUM_FACTIONS;
	space = std::min(space, 5 * spaceDiv);

	auto psWeakTitleUI = std::weak_ptr<WzMultiplayerOptionsTitleUI>(std::dynamic_pointer_cast<WzMultiplayerOptionsTitleUI>(shared_from_this()));

	for (unsigned int i = 0; i < NUM_FACTIONS; i++)
	{
		auto onClickHandler = [player, psWeakTitleUI](W_BUTTON &button) {
			UDWORD id = button.id;
			auto pStrongPtr = psWeakTitleUI.lock();
			ASSERT_OR_RETURN(, pStrongPtr.operator bool(), "WzMultiplayerOptionsTitleUI no longer exists");

			STATIC_ASSERT(MULTIOP_FACCHOOSER + NUM_FACTIONS - 1 <= MULTIOP_FACCHOOSER_END);
			if (id >= MULTIOP_FACCHOOSER && id <= MULTIOP_FACCHOOSER + NUM_FACTIONS -1)
			{
				resetReadyStatus(false, true);
				uint8_t idx = id - MULTIOP_FACCHOOSER;
				SendFactionRequest(player, idx);
				pStrongPtr->closeFactionChooser();
				pStrongPtr->addPlayerBox(true);
			}
		};

		addMultiButWithClickHandler(psInlineChooserOverlayScreen, MULTIOP_FACCHOOSER_FORM, MULTIOP_FACCHOOSER + i,
			i * (flagW * spaceDiv + space) / spaceDiv + 7,  4, // x, y
			flagW, flagH,  // w, h
			to_localized_string(static_cast<FactionID>(i)),
			IMAGE_FACTION_NORMAL+i, IMAGE_FACTION_NORMAL_HI+i, IMAGE_FACTION_NORMAL_HI+i, onClickHandler
		);
	}

	inlineChooserUp = player;
}

static void changeTeam(UBYTE player, UBYTE team)
{
	NetPlay.players[player].team = team;
	debug(LOG_WZ, "set %d as new team for player %d", team, player);
	NETBroadcastPlayerInfo(player);
	netPlayersUpdated = true;
}

static bool SendTeamRequest(UBYTE player, UBYTE chosenTeam)
{
	if (NetPlay.isHost)			// do or request the change.
	{
		changeTeam(player, chosenTeam);	// do the change, remember only the host can do this to avoid confusion.
	}
	else
	{
		NETbeginEncode(NETnetQueue(NET_HOST_ONLY), NET_TEAMREQUEST);

		NETuint8_t(&player);
		NETuint8_t(&chosenTeam);

		NETend();

	}
	return true;
}

bool recvTeamRequest(NETQUEUE queue)
{
	ASSERT_HOST_ONLY(return true);

	NETbeginDecode(queue, NET_TEAMREQUEST);

	UBYTE player, team;
	NETuint8_t(&player);
	NETuint8_t(&team);
	NETend();

	if (player >= MAX_PLAYERS || team >= MAX_PLAYERS)
	{
		debug(LOG_NET, "NET_TEAMREQUEST invalid, player %d team, %d", (int) player, (int) team);
		debug(LOG_ERROR, "Invalid NET_TEAMREQUEST from player %d: Tried to change player %d (team %d)",
		      queue.index, (int)player, (int)team);
		return false;
	}

	if (whosResponsible(player) != queue.index)
	{
		HandleBadParam("NET_TEAMREQUEST given incorrect params.", player, queue.index);
		return false;
	}

	if (locked.teams)
	{
		return false;
	}

	if (NetPlay.players[player].team != team)
	{
		resetReadyStatus(false);
	}
	debug(LOG_NET, "%s is now part of team: %d", NetPlay.players[player].name, (int) team);
	changeTeam(player, team); // we do this regardless, in case of sync issues

	return true;
}

static bool SendReadyRequest(UBYTE player, bool bReady)
{
	if (NetPlay.isHost)			// do or request the change.
	{
		return changeReadyStatus(player, bReady);
	}
	else
	{
		NETbeginEncode(NETnetQueue(NET_HOST_ONLY), NET_READY_REQUEST);
		NETuint8_t(&player);
		NETbool(&bReady);
		NETend();
	}
	return true;
}

bool recvReadyRequest(NETQUEUE queue)
{
	ASSERT_HOST_ONLY(return true);

	NETbeginDecode(queue, NET_READY_REQUEST);

	UBYTE player;
	bool bReady = false;
	NETuint8_t(&player);
	NETbool(&bReady);
	NETend();

	if (player >= MAX_PLAYERS)
	{
		debug(LOG_ERROR, "Invalid NET_READY_REQUEST from player %d: player id = %d",
		      queue.index, (int)player);
		return false;
	}

	if (whosResponsible(player) != queue.index)
	{
		HandleBadParam("NET_READY_REQUEST given incorrect params.", player, queue.index);
		return false;
	}

	// do not allow players to select 'ready' if we are sending a map too them!
	// TODO: make a new icon to show this state?
	if (!NetPlay.players[player].wzFiles.empty())
	{
		return false;
	}

	return changeReadyStatus((UBYTE)player, bReady);
}

bool changeReadyStatus(UBYTE player, bool bReady)
{
	NetPlay.players[player].ready = bReady;
	NETBroadcastPlayerInfo(player);
	netPlayersUpdated = true;

	return true;
}

static bool changePosition(UBYTE player, UBYTE position)
{
	int i;

	for (i = 0; i < MAX_PLAYERS; i++)
	{
		if (NetPlay.players[i].position == position)
		{
			debug(LOG_NET, "Swapping positions between players %d(%d) and %d(%d)",
			      player, NetPlay.players[player].position, i, NetPlay.players[i].position);
			std::swap(NetPlay.players[i].position, NetPlay.players[player].position);
			std::swap(NetPlay.players[i].team, NetPlay.players[player].team);
			NETBroadcastTwoPlayerInfo(player, i);
			netPlayersUpdated = true;
			return true;
		}
	}
	debug(LOG_ERROR, "Failed to swap positions for player %d, position %d", (int)player, (int)position);
	if (player < game.maxPlayers && position < game.maxPlayers)
	{
		debug(LOG_NET, "corrupted positions: player (%u) new position (%u) old position (%d)", player, position, NetPlay.players[player].position);
		// Positions were corrupted. Attempt to fix.
		NetPlay.players[player].position = position;
		NETBroadcastPlayerInfo(player);
		netPlayersUpdated = true;
		return true;
	}
	return false;
}

bool changeColour(unsigned player, int col, bool isHost)
{
	if (col < 0 || col >= MAX_PLAYERS_IN_GUI)
	{
		return true;
	}

	if (getPlayerColour(player) == col)
	{
		return true;  // Nothing to do.
	}

	for (unsigned i = 0; i < MAX_PLAYERS; ++i)
	{
		if (getPlayerColour(i) == col)
		{
			if (!isHost && NetPlay.players[i].allocated)
			{
				return true;  // May not swap.
			}

			debug(LOG_NET, "Swapping colours between players %d(%d) and %d(%d)",
			      player, getPlayerColour(player), i, getPlayerColour(i));
			setPlayerColour(i, getPlayerColour(player));
			setPlayerColour(player, col);
			NETBroadcastTwoPlayerInfo(player, i);
			netPlayersUpdated = true;
			return true;
		}
	}
	debug(LOG_ERROR, "Failed to swap colours for player %d, colour %d", (int)player, (int)col);
	if (player < game.maxPlayers && col < MAX_PLAYERS)
	{
		// Colours were corrupted. Attempt to fix.
		debug(LOG_NET, "corrupted colours: player (%u) new colour (%u) old colour (%d)", player, col, NetPlay.players[player].colour);
		setPlayerColour(player, col);
		NETBroadcastPlayerInfo(player);
		netPlayersUpdated = true;
		return true;
	}
	return false;
}

static bool SendColourRequest(UBYTE player, UBYTE col)
{
	if (NetPlay.isHost)			// do or request the change
	{
		return changeColour(player, col, true);
	}
	else
	{
		// clients tell the host which color they want
		NETbeginEncode(NETnetQueue(NET_HOST_ONLY), NET_COLOURREQUEST);
		NETuint8_t(&player);
		NETuint8_t(&col);
		NETend();
	}
	return true;
}

static bool SendFactionRequest(UBYTE player, UBYTE faction)
{
	// TODO: needs to be rewritten from scratch
	ASSERT_OR_RETURN(false, faction <= static_cast<UBYTE>(MAX_FACTION_ID), "Invalid faction: %u", (unsigned int)faction);
	if (NetPlay.isHost)			// do or request the change
	{
		NetPlay.players[player].faction = static_cast<FactionID>(faction);
		NETBroadcastPlayerInfo(player);
		return true;
	}
	else
	{
		// clients tell the host which color they want
		NETbeginEncode(NETnetQueue(NET_HOST_ONLY), NET_FACTIONREQUEST);
		NETuint8_t(&player);
		NETuint8_t(&faction);
		NETend();
	}
	return true;
}

static bool SendPositionRequest(UBYTE player, UBYTE position)
{
	if (NetPlay.isHost)			// do or request the change
	{
		return changePosition(player, position);
	}
	else
	{
		debug(LOG_NET, "Requesting the host to change our position. From %d to %d", player, position);
		// clients tell the host which position they want
		NETbeginEncode(NETnetQueue(NET_HOST_ONLY), NET_POSITIONREQUEST);
		NETuint8_t(&player);
		NETuint8_t(&position);
		NETend();
	}
	return true;
}

bool recvFactionRequest(NETQUEUE queue)
{
	ASSERT_HOST_ONLY(return true);

	NETbeginDecode(queue, NET_FACTIONREQUEST);

	UBYTE player, faction;
	NETuint8_t(&player);
	NETuint8_t(&faction);
	NETend();

	if (player >= MAX_PLAYERS)
	{
		debug(LOG_ERROR, "Invalid NET_FACTIONREQUEST from player %d: Tried to change player %d to faction %d",
		      queue.index, (int)player, (int)faction);
		return false;
	}

	if (whosResponsible(player) != queue.index)
	{
		HandleBadParam("NET_FACTIONREQUEST given incorrect params.", player, queue.index);
		return false;
	}

	auto newFactionId = uintToFactionID(faction);
	if (!newFactionId.has_value())
	{
		HandleBadParam("NET_FACTIONREQUEST given incorrect params.", player, queue.index);
		return false;
	}

	resetReadyStatus(false, true);

	NetPlay.players[player].faction = newFactionId.value();
	NETBroadcastPlayerInfo(player);
	return true;
}

bool recvColourRequest(NETQUEUE queue)
{
	ASSERT_HOST_ONLY(return true);

	NETbeginDecode(queue, NET_COLOURREQUEST);

	UBYTE player, col;
	NETuint8_t(&player);
	NETuint8_t(&col);
	NETend();

	if (player >= MAX_PLAYERS)
	{
		debug(LOG_ERROR, "Invalid NET_COLOURREQUEST from player %d: Tried to change player %d to colour %d",
		      queue.index, (int)player, (int)col);
		return false;
	}

	if (whosResponsible(player) != queue.index)
	{
		HandleBadParam("NET_COLOURREQUEST given incorrect params.", player, queue.index);
		return false;
	}

	resetReadyStatus(false, true);

	return changeColour(player, col, false);
}

bool recvPositionRequest(NETQUEUE queue)
{
	ASSERT_HOST_ONLY(return true);

	NETbeginDecode(queue, NET_POSITIONREQUEST);

	UBYTE	player, position;
	NETuint8_t(&player);
	NETuint8_t(&position);
	NETend();
	debug(LOG_NET, "Host received position request from player %d to %d", player, position);

	if (player >= MAX_PLAYERS || position >= MAX_PLAYERS)
	{
		debug(LOG_ERROR, "Invalid NET_POSITIONREQUEST from player %d: Tried to change player %d to %d",
		      queue.index, (int)player, (int)position);
		return false;
	}

	if (whosResponsible(player) != queue.index)
	{
		HandleBadParam("NET_POSITIONREQUEST given incorrect params.", player, queue.index);
		return false;
	}

	if (locked.position)
	{
		return false;
	}

	resetReadyStatus(false);

	return changePosition(player, position);
}


static void drawReadyButton(UDWORD player)
{
	int disallow = allPlayersOnSameTeam(-1);

	// delete 'ready' botton form
	WIDGET *parent = widgGetFromID(psWScreen, MULTIOP_READY_FORM_ID + player);

	if (!parent)
	{
		// add form to hold 'ready' botton
		parent = addBlueForm(MULTIOP_PLAYERS, MULTIOP_READY_FORM_ID + player,
					7 + MULTIOP_PLAYERWIDTH - MULTIOP_READY_WIDTH,
					playerBoxHeight(player),
					MULTIOP_READY_WIDTH, MULTIOP_READY_HEIGHT);
	}


	auto deleteExistingReadyButton = [player]() {
		widgDelete(widgGetFromID(psWScreen, MULTIOP_READY_START + player));
		widgDelete(widgGetFromID(psWScreen, MULTIOP_READY_START + MAX_PLAYERS + player)); // "Ready?" text label
	};
	auto deleteExistingDifficultyButton = [player]() {
		widgDelete(widgGetFromID(psWScreen, MULTIOP_DIFFICULTY_INIT_START + player));
	};

	if (!NetPlay.players[player].allocated && NetPlay.players[player].ai >= 0)
	{
		deleteExistingReadyButton();
		int playerDifficulty = static_cast<int8_t>(NetPlay.players[player].difficulty);
		int icon = difficultyIcon(playerDifficulty);
		char tooltip[128 + 255];
		sstrcpy(tooltip, _(difficultyList[playerDifficulty]));
		const char *difficultyTip = aidata[NetPlay.players[player].ai].difficultyTips[playerDifficulty];
		if (strcmp(difficultyTip, "") != 0)
		{
			sstrcat(tooltip, "\n");
			sstrcat(tooltip, difficultyTip);
		}
		addMultiBut(psWScreen, MULTIOP_READY_FORM_ID + player, MULTIOP_DIFFICULTY_INIT_START + player, 6, 4, MULTIOP_READY_WIDTH, MULTIOP_READY_HEIGHT,
		            (NetPlay.isHost && !locked.difficulty) ? _("Click to change difficulty") : tooltip, icon, icon, icon);
		return;
	}
	else if (!NetPlay.players[player].allocated)
	{
		// closed or open - remove ready / difficulty button
		deleteExistingReadyButton();
		deleteExistingDifficultyButton();
		return;
	}

	if (disallow != -1)
	{
		// remove ready / difficulty button
		deleteExistingReadyButton();
		deleteExistingDifficultyButton();
		return;
	}

	bool isMe = player == selectedPlayer;
	int isReady = NETgetDownloadProgress(player) != 100 ? 2 : NetPlay.players[player].ready ? 1 : 0;
	char const *const toolTips[2][3] = {{_("Waiting for player"), _("Player is ready"), _("Player is downloading")}, {_("Click when ready"), _("Waiting for other players"), _("Waiting for download")}};
	unsigned images[2][3] = {{IMAGE_CHECK_OFF, IMAGE_CHECK_ON, IMAGE_CHECK_DOWNLOAD}, {IMAGE_CHECK_OFF_HI, IMAGE_CHECK_ON_HI, IMAGE_CHECK_DOWNLOAD_HI}};

	// draw 'ready' button
	auto pReadyBut = addMultiBut(psWScreen, MULTIOP_READY_FORM_ID + player, MULTIOP_READY_START + player, 3, 10, MULTIOP_READY_WIDTH, MULTIOP_READY_HEIGHT,
	            toolTips[isMe][isReady], images[0][isReady], images[0][isReady], images[isMe][isReady]);
	ASSERT_OR_RETURN(, pReadyBut != nullptr, "Failed to create ready button");
	pReadyBut->minClickInterval = GAME_TICKS_PER_SEC;
	pReadyBut->unlock();

	std::shared_ptr<W_LABEL> label;
	auto existingLabel = widgFormGetFromID(parent->shared_from_this(), MULTIOP_READY_START + MAX_PLAYERS + player);
	if (existingLabel)
	{
		label = std::dynamic_pointer_cast<W_LABEL>(existingLabel);
	}
	if (label == nullptr)
	{
		label = std::make_shared<W_LABEL>();
		parent->attach(label);
		label->id = MULTIOP_READY_START + MAX_PLAYERS + player;
	}
	label->setGeometry(0, 0, MULTIOP_READY_WIDTH, 17);
	label->setTextAlignment(WLAB_ALIGNBOTTOM);
	label->setFont(font_small, WZCOL_TEXT_BRIGHT);
	label->setString(_("READY?"));
}

static bool canChooseTeamFor(int i)
{
	return (i == selectedPlayer || NetPlay.isHost);
}

// ////////////////////////////////////////////////////////////////////////////
// box for players.

void WzMultiplayerOptionsTitleUI::addPlayerBox(bool players)
{
	// if background isn't there, then return since were not ready to draw the box yet!
	if (widgGetFromID(psWScreen, FRONTEND_BACKDROP) == nullptr)
	{
		return;
	}

	widgDelete(psWScreen, MULTIOP_PLAYERS);		// del player window
	widgDelete(psWScreen, FRONTEND_SIDETEXT2);	// del text too,

	if (aiChooserUp >= 0)
	{
		return;
	}
	else if (difficultyChooserUp >= 0)
	{
		return;
	}

	// draw player window
	WIDGET *widgetParent = widgGetFromID(psWScreen, FRONTEND_BACKDROP);

	auto playersForm = std::make_shared<IntFormAnimated>(false);
	widgetParent->attach(playersForm);
	playersForm->id = MULTIOP_PLAYERS;
	playersForm->setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
		psWidget->setGeometry(MULTIOP_PLAYERSX, MULTIOP_PLAYERSY, MULTIOP_PLAYERSW, MULTIOP_PLAYERSH);
	}));

	W_LABEL* pPlayersLabel = addSideText(FRONTEND_SIDETEXT2, MULTIOP_PLAYERSX - 3, MULTIOP_PLAYERSY, _("PLAYERS"));
	pPlayersLabel->hide(); // hide for now

	if (players)
	{
		int  team = -1;
		bool allOnSameTeam = true;

		for (int i = 0; i < game.maxPlayers; i++)
		{
			if (NetPlay.players[i].difficulty != AIDifficulty::DISABLED || isHumanPlayer(i))
			{
				int myTeam = getPlayerTeam(i);
				if (team == -1)
				{
					team = myTeam;
				}
				else if (myTeam != team)
				{
					allOnSameTeam = false;
					break;  // We just need to know if we have enough to start a game
				}
			}
		}

		for (int i = 0; i < game.maxPlayers; i++)
		{
			if (positionChooserUp >= 0 && positionChooserUp != i && (NetPlay.isHost || !isHumanPlayer(i)))
			{
				W_BUTINIT sButInit;
				sButInit.formID = MULTIOP_PLAYERS;
				sButInit.id = MULTIOP_PLAYER_START + i;
				sButInit.x = 7;
				sButInit.y = playerBoxHeight(i);
				sButInit.width = MULTIOP_PLAYERWIDTH + 1;
				sButInit.height = MULTIOP_PLAYERHEIGHT;
				sButInit.pTip = _("Click to change to this slot");
				sButInit.pDisplay = displayPosition;
				sButInit.UserData = i;
				sButInit.pUserData = new DisplayPositionCache();
				sButInit.onDelete = [](WIDGET *psWidget) {
					assert(psWidget->pUserData != nullptr);
					delete static_cast<DisplayPositionCache *>(psWidget->pUserData);
					psWidget->pUserData = nullptr;
				};
				widgAddButton(psWScreen, &sButInit);
				continue;
			}
			else if (i == inlineChooserUp)
			{
				// skip adding player info box, since inline chooser is up for this player
				continue;
			}
			else if (ingame.localOptionsReceived)
			{
				//add team chooser
				W_BUTINIT sButInit;
				sButInit.formID = MULTIOP_PLAYERS;
				sButInit.id = MULTIOP_TEAMS_START + i;
				sButInit.x = 7;
				sButInit.y = playerBoxHeight(i);
				sButInit.width = MULTIOP_TEAMSWIDTH;
				sButInit.height = MULTIOP_TEAMSHEIGHT;
				if (canChooseTeamFor(i) && !locked.teams)
				{
					sButInit.pTip = _("Choose Team");
				}
				else if (locked.teams)
				{
					sButInit.pTip = _("Teams locked");
				}
				sButInit.pDisplay = displayTeamChooser;
				sButInit.UserData = i;

				if (alliancesSetTeamsBeforeGame(game.alliance))
				{
					// only if not disabled and in locked teams mode
					widgAddButton(psWScreen, &sButInit);
				}
			}

			// draw player colour
			W_BUTINIT sColInit;
			sColInit.formID = MULTIOP_PLAYERS;
			sColInit.id = MULTIOP_COLOUR_START + i;
			sColInit.x = 7 + MULTIOP_TEAMSWIDTH;
			sColInit.y = playerBoxHeight(i);
			sColInit.width = MULTIOP_COLOUR_WIDTH;
			sColInit.height = MULTIOP_PLAYERHEIGHT;
			if (selectedPlayer == i || NetPlay.isHost)
			{
				sColInit.pTip = _("Click to change player colour");
			}
			sColInit.pDisplay = displayColour;
			sColInit.UserData = i;
			widgAddButton(psWScreen, &sColInit);

			// draw player faction
			W_BUTINIT sFacInit;
			sFacInit.formID = MULTIOP_PLAYERS;
			sFacInit.id = MULTIOP_FACTION_START+i;
			sFacInit.x = 7 + MULTIOP_TEAMSWIDTH+MULTIOP_COLOUR_WIDTH;
			sFacInit.y = playerBoxHeight(i);
			sFacInit.width = MULTIOP_FACTION_WIDTH;
			sFacInit.height = MULTIOP_PLAYERHEIGHT;
			if (selectedPlayer == i || NetPlay.isHost)
			{
				sFacInit.pTip = _("Click to change player faction");
			}
			sFacInit.pDisplay = displayFaction;
			sFacInit.UserData = i;
			widgAddButton(psWScreen, &sFacInit);

			if (ingame.localOptionsReceived)
			{
				// do not draw "Ready" button if all players are on the same team,
				// but always draw the difficulty buttons for AI players
				if (!allOnSameTeam || (!NetPlay.players[i].allocated && NetPlay.players[i].ai >= 0))
				{
					drawReadyButton(i);
				}

				// draw player info box
				W_BUTINIT sButInit;
				sButInit.formID = MULTIOP_PLAYERS;
				sButInit.id = MULTIOP_PLAYER_START + i;
				sButInit.x = 7 + MULTIOP_TEAMSWIDTH + MULTIOP_COLOUR_WIDTH + MULTIOP_FACTION_WIDTH;
				sButInit.y = playerBoxHeight(i);
				sButInit.width = MULTIOP_PLAYERWIDTH - MULTIOP_TEAMSWIDTH - MULTIOP_READY_WIDTH - MULTIOP_COLOUR_WIDTH - MULTIOP_FACTION_WIDTH;
				sButInit.height = MULTIOP_PLAYERHEIGHT;
				if ((selectedPlayer == i || NetPlay.isHost) && NetPlay.players[i].allocated && !locked.position)
				{
					sButInit.pTip = _("Click to change player position");
				}
				else if (!NetPlay.players[i].allocated)
				{
					if (NetPlay.isHost && !locked.ai)
					{
						sButInit.style |= WBUT_SECONDARY;
						sButInit.pTip = _("Click to change AI, right click to distribute choice");
					}
					else if (NetPlay.players[i].ai >= 0)
					{
						// show AI description. Useful for challenges.
						sButInit.pTip = aidata[NetPlay.players[i].ai].tip;
					}
				}
				if (NetPlay.players[i].allocated && !getMultiStats(i).identity.empty())
				{
					if (!sButInit.pTip.empty())
					{
						sButInit.pTip += "\n";
					}
					std::string hash = getMultiStats(i).identity.publicHashString(20);
					sButInit.pTip += _("Player ID: ");
					sButInit.pTip += hash.empty()? _("(none)") : hash;
				}
				sButInit.pDisplay = displayPlayer;
				sButInit.UserData = i;
				sButInit.pUserData = new DisplayPlayerCache();
				sButInit.onDelete = [](WIDGET *psWidget) {
					assert(psWidget->pUserData != nullptr);
					delete static_cast<DisplayPlayerCache *>(psWidget->pUserData);
					psWidget->pUserData = nullptr;
				};
				widgAddButton(psWScreen, &sButInit);
			}
		}
	}
}

/*
 * Notify all players of host launching the game
 */
static void SendFireUp()
{
	uint32_t randomSeed = rand();  // Pick a random random seed for the synchronised random number generator.

	NETbeginEncode(NETbroadcastQueue(), NET_FIREUP);
	NETuint32_t(&randomSeed);
	NETend();
	printSearchPath();
	gameSRand(randomSeed);  // Set the seed for the synchronised random number generator. The clients will use the same seed.
}

// host kicks a player from a game.
void kickPlayer(uint32_t player_id, const char *reason, LOBBY_ERROR_TYPES type)
{
	// send a kick msg
	NETbeginEncode(NETbroadcastQueue(), NET_KICK);
	NETuint32_t(&player_id);
	NETstring(reason, MAX_KICK_REASON);
	NETenum(&type);
	NETend();
	NETflush();
	wzDelay(300);
	debug(LOG_NET, "Kicking player %u (%s). Reason: %s", (unsigned int)player_id, getPlayerName(player_id), reason);

	ActivityManager::instance().hostKickPlayer(NetPlay.players[player_id], type, reason);

	NETplayerKicked(player_id);
}

void displayKickReasonPopup(const std::string &reason)
{
	WZ_Notification notification;
	notification.duration = GAME_TICKS_PER_SEC * 10;
	notification.contentTitle = _("Kicked from game");
	notification.contentText = reason;
	notification.tag = KICK_REASON_TAG;

	addNotification(notification, WZ_Notification_Trigger(GAME_TICKS_PER_SEC * 1));
}

RoomMessage buildMessage(int32_t sender, const char *text)
{
	switch (sender)
	{
	case SYSTEM_MESSAGE:
		return RoomMessage::system(text);
	case NOTIFY_MESSAGE:
		return RoomMessage::notify(text);
	default:
		if (sender >= 0 && sender < MAX_PLAYERS_IN_GUI)
		{
			return RoomMessage::player(sender, text);
		}

		debug(LOG_ERROR, "Invalid message sender %d.", sender);
		return RoomMessage::system(text);
	}
}

void ChatBoxWidget::initialize()
{
	id = MULTIOP_CHATBOX;

	attach(messages = ScrollableListWidget::make());
	messages->setSnapOffset(true);
	messages->setStickToBottom(true);
	messages->setPadding({3, 4, 3, 4});
	messages->setItemSpacing(1);

	handleConsoleMessage = std::make_shared<CONSOLE_MESSAGE_LISTENER>([&](ConsoleMessage const &message) -> void
	{
		addMessage(buildMessage(message.sender, message.text));
	});

	W_EDBINIT sEdInit;
	sEdInit.formID = MULTIOP_CHATBOX;
	sEdInit.id = MULTIOP_CHATEDIT;
	sEdInit.pUserData = nullptr;
	sEdInit.pBoxDisplay = displayChatEdit;
	editBox = std::make_shared<W_EDITBOX>(&sEdInit);
	attach(editBox);

	consoleAddMessageListener(handleConsoleMessage);
}

ChatBoxWidget::~ChatBoxWidget()
{
	consoleRemoveMessageListener(handleConsoleMessage);
}


class ChatBoxPlayerNameWidget: public WIDGET
{
public:
	ChatBoxPlayerNameWidget(std::shared_ptr<PlayerReference> const &player):
		WIDGET(WIDG_UNSPECIFIED_TYPE),
		player(player),
		font(font_regular),
		cachedText("", font)
	{
		updateLayout();
	}

	void display(int xOffset, int yOffset) override
	{
		auto left = xOffset + x();
		auto top = yOffset + y();
		auto marginLeft = left + leftMargin;
		auto textX = marginLeft + horizontalPadding;
		auto textY = top - cachedText->aboveBase();
		pie_UniTransBoxFill(marginLeft, top, left + width(), top + height(), pal_GetTeamColour((*player)->colour));
		cachedText->renderOutlined(textX, textY, WZCOL_WHITE, {0, 0, 0, 128});
	}

	void run(W_CONTEXT *) override
	{
		if (layoutName != (*player)->name)
		{
			updateLayout();
		}
		cachedText.tick();
	}

private:
	std::shared_ptr<PlayerReference> player;
	iV_fonts font;
	std::string layoutName;
	WzCachedText cachedText;
	int32_t horizontalPadding = 3;
	int32_t leftMargin = 3;

	void updateLayout()
	{
		layoutName = (*player)->name;
		cachedText = WzCachedText(layoutName, font);
		setGeometry(x(), y(), cachedText->width() + leftMargin + 2 * horizontalPadding, cachedText->lineSize());
	}
};

void ChatBoxWidget::addMessage(RoomMessage const &message)
{
	ChatBoxWidget::persistentMessageLocalStorage.push_back(message);
	displayMessage(message);
}

void ChatBoxWidget::geometryChanged()
{
	auto messagesHeight = height() - MULTIOP_CHATEDITH - 1;
	messages->setGeometry(1, 1, width() - 2, messagesHeight);
	editBox->setGeometry(MULTIOP_CHATEDITX, messages->y() + messagesHeight, MULTIOP_CHATEDITW, MULTIOP_CHATEDITH);
}

void ChatBoxWidget::initializeMessages(bool preserveOldChat)
{
	if (preserveOldChat)
	{
		for (auto message: ChatBoxWidget::persistentMessageLocalStorage)
		{
			displayMessage(message);
		}
	} else {
		ChatBoxWidget::persistentMessageLocalStorage.clear();
	}
}

void ChatBoxWidget::displayMessage(RoomMessage const &message)
{
	auto paragraph = std::make_shared<Paragraph>();
	paragraph->setGeometry(0, 0, messages->calculateListViewWidth(), 0);

	switch (message.type)
	{
	case RoomMessageSystem:
		paragraph->setFontColour(WZCOL_CONS_TEXT_SYSTEM);
		paragraph->addText(message.text);
		break;

	case RoomMessagePlayer:
		paragraph->setFont(font_small);
		paragraph->setFontColour({0xc0, 0xc0, 0xc0, 0xff});
		paragraph->addText(formatLocalDateTime("%H:%M", message.time));

		paragraph->addWidget(std::make_shared<ChatBoxPlayerNameWidget>(message.sender), iV_GetTextAboveBase(font_regular));

		paragraph->setFont(font_regular);
		paragraph->setShadeColour({0, 0, 0, 0});
		paragraph->setFontColour(WZCOL_WHITE);
		paragraph->addText(astringf(" %s", message.text.c_str()));

		break;

	case RoomMessageNotify:
	default:
		paragraph->setFontColour(WZCOL_YELLOW);
		paragraph->addText(message.text);
		break;
	}

	messages->addItem(paragraph);
}

static void addChatBox(bool preserveOldChat)
{
	if (widgGetFromID(psWScreen, FRONTEND_TOPFORM))
	{
		widgDelete(psWScreen, FRONTEND_TOPFORM);
	}

	if (widgGetFromID(psWScreen, MULTIOP_CHATBOX))
	{
		return;
	}

	WIDGET *parent = widgGetFromID(psWScreen, FRONTEND_BACKDROP);
	auto chatBox = ChatBoxWidget::make();
	parent->attach(chatBox);
	chatBox->setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
		if (auto parent = psWidget->parent())
		{
			psWidget->setGeometry(MULTIOP_CHATBOXX, MULTIOP_CHATBOXY, MULTIOP_CHATBOXW, parent->height() - MULTIOP_CHATBOXY);
		}
	}));
	chatBox->initializeMessages(preserveOldChat);

	addSideText(FRONTEND_SIDETEXT4, MULTIOP_CHATBOXX - 3, MULTIOP_CHATBOXY, _("CHAT"));

	if (!getModList().empty())
	{
		WzString modListMessage = _("Mod: ");
		modListMessage += getModList().c_str();
		displayRoomSystemMessage(modListMessage.toUtf8().c_str());
	}
}

// ////////////////////////////////////////////////////////////////////////////
static void disableMultiButs()
{
	if (!NetPlay.isHost)
	{
		// edit box icons.
		widgSetButtonState(psWScreen, MULTIOP_GNAME_ICON, WBUT_DISABLE);
		widgSetButtonState(psWScreen, MULTIOP_MAP_ICON, WBUT_DISABLE);

		// edit boxes
		widgSetButtonState(psWScreen, MULTIOP_GNAME, WEDBS_DISABLE);

		((MultichoiceWidget *)widgGetFromID(psWScreen, MULTIOP_GAMETYPE))->disable();  // Scavengers.
		((MultichoiceWidget *)widgGetFromID(psWScreen, MULTIOP_BASETYPE))->disable();  // camapign subtype.
		((MultichoiceWidget *)widgGetFromID(psWScreen, MULTIOP_POWER))->disable();  // pow levels
		((MultichoiceWidget *)widgGetFromID(psWScreen, MULTIOP_ALLIANCES))->disable();
	}
}

////////////////////////////////////////////////////////////////////////////
static void stopJoining(std::shared_ptr<WzTitleUI> parent)
{
	bInActualHostedLobby = false;

	reloadMPConfig(); // reload own settings
	cancelOrDismissNotificationsWithTag(VOTE_TAG);

	debug(LOG_NET, "player %u (Host is %s) stopping.", selectedPlayer, NetPlay.isHost ? "true" : "false");

	pie_LoadBackDrop(SCREEN_RANDOMBDROP);

	if (NetPlay.isHost) // cancel a hosted game.
	{
		// annouce we are leaving...
		debug(LOG_NET, "Host is quitting game...");
		NETbeginEncode(NETbroadcastQueue(), NET_HOST_DROPPED);
		NETend();
		sendLeavingMsg();								// say goodbye
		NETclose();										// quit running game.
		ActivityManager::instance().hostLobbyQuit();
		changeTitleUI(wzTitleUICurrent);				// refresh options screen.
		ingame.localJoiningInProgress = false;
		return;
	}
	else if (ingame.localJoiningInProgress)				// cancel a joined game.
	{
		debug(LOG_NET, "Canceling game...");
		sendLeavingMsg();								// say goodbye
		NETclose();										// quit running game.

		// if we were in a midle of transferring a file, then close the file handle
		for (auto const &file : NetPlay.wzFiles)
		{
			debug(LOG_NET, "closing aborted file");		// no need to delete it, we do size check on (map) file
			PHYSFS_close(file.handle);
		}
		NetPlay.wzFiles.clear();
		ingame.localJoiningInProgress = false;			// reset local flags
		ingame.localOptionsReceived = false;

		// joining and host was transferred.
		if (ingame.side == InGameSide::MULTIPLAYER_CLIENT && NetPlay.isHost)
		{
			NetPlay.isHost = false;
		}

		ActivityManager::instance().joinedLobbyQuit();
		changeTitleMode(MULTI);

		selectedPlayer = 0;
		realSelectedPlayer = 0;
		return;
	}
	debug(LOG_NET, "We have stopped joining.");
	ActivityManager::instance().joinedLobbyQuit();
	changeTitleUI(parent);
	selectedPlayer = 0;
	realSelectedPlayer = 0;
	for (auto& player : NetPlay.players)
	{
		player.resetAll();
	}
}

static void resetPlayerPositions()
{
	// Reset players' positions or bad things could happen to scavenger slot

	for (unsigned int i = 0; i < MAX_PLAYERS; ++i)
	{
		NetPlay.players[i].position = i;
		NetPlay.players[i].team = i;
		NETBroadcastPlayerInfo(i);
	}
}

static unsigned int repositionHumanSlots()
{
	unsigned int pos = 0;

	// First, put human players at the top
	for (unsigned int i = 0; i < MAX_PLAYERS; ++i)
	{
		if (isHumanPlayer(i))
		{
			// Skip the scavenger slot
			if (game.mapHasScavengers && pos == scavengerSlot())
			{
				++pos;
			}
			NetPlay.players[i].position = pos;
			NETBroadcastPlayerInfo(i);
			++pos;
		}
	}

	return pos;
}

static void updateMapWidgets(LEVEL_DATASET *mapData)
{
	sstrcpy(game.map, mapData->pName);
	game.hash = levGetFileHash(mapData);
	game.maxPlayers = mapData->players;
	game.isMapMod = CheckForMod(mapData->realFileName);
	game.isRandom = CheckForRandom(mapData->realFileName, mapData->pName);
	if (game.isMapMod)
	{
		widgReveal(psWScreen, MULTIOP_MAP_MOD);
	}
	else
	{
		widgHide(psWScreen, MULTIOP_MAP_MOD);
	}
	(game.isRandom? widgReveal : widgHide)(psWScreen, MULTIOP_MAP_RANDOM);

	WzString name = formatGameName(game.map);
	widgSetString(psWScreen, MULTIOP_MAP + 1, name.toUtf8().c_str()); //What a horrible, horrible way to do this! FIX ME! (See addBlueForm)
}

static void loadMapChallengeSettings(WzConfig& ini)
{
	ini.beginGroup("locked"); // GUI lockdown
	{
		locked.power = ini.value("power", challengeActive).toBool();
		locked.alliances = ini.value("alliances", challengeActive).toBool();
		locked.teams = ini.value("teams", challengeActive).toBool();
		locked.difficulty = ini.value("difficulty", challengeActive).toBool();
		locked.ai = ini.value("ai", challengeActive).toBool();
		locked.scavengers = ini.value("scavengers", challengeActive).toBool();
		locked.position = ini.value("position", challengeActive).toBool();
		locked.bases = ini.value("bases", challengeActive).toBool();
	}
	ini.endGroup();

	const bool bIsAutoHostOrAutoGame = getHostLaunch() == HostLaunch::Skirmish || getHostLaunch() == HostLaunch::Autohost;
	if (challengeActive || bIsAutoHostOrAutoGame)
	{
		ini.beginGroup("challenge");
		{
			sstrcpy(game.map, ini.value("map", game.map).toWzString().toUtf8().c_str());
			game.hash = levGetMapNameHash(game.map);

			LEVEL_DATASET* mapData = levFindDataSet(game.map, &game.hash);
			if (!mapData)
			{
				code_part log_level = (bIsAutoHostOrAutoGame) ? LOG_ERROR : LOG_FATAL;
				debug(log_level, "Map %s not found!", game.map);
				if (bIsAutoHostOrAutoGame)
				{
					exit(1);
				}
			}
			game.maxPlayers = mapData->players;

			uint8_t configuredMaxPlayers = ini.value("maxPlayers", game.maxPlayers).toUInt();
			if (getHostLaunch() == HostLaunch::Autohost)
			{
				// always use the autohost config - if it specifies an invalid number of players, this is a bug in the config
				game.maxPlayers = std::max((uint8_t)1u, configuredMaxPlayers);
			}
			else
			{
				game.maxPlayers = std::min(std::max((uint8_t)1u, configuredMaxPlayers), game.maxPlayers);
			}
			game.scavengers = ini.value("scavengers", game.scavengers).toBool();
			game.alliance = ini.value("alliances", ALLIANCES_TEAMS).toInt();
			game.power = ini.value("powerLevel", game.power).toInt();
			game.base = ini.value("bases", game.base + 1).toInt() - 1;		// count from 1 like the humans do
			sstrcpy(game.name, ini.value("name").toWzString().toUtf8().c_str());
			game.techLevel = ini.value("techLevel", game.techLevel).toInt();

			// DEPRECATED: This seems to have been odd workaround for not having the locked group handled.
			//             Keeping it around in case mods use it.
			locked.position = !ini.value("allowPositionChange", !locked.position).toBool();
		}
		ini.endGroup();
	}
	else
	{
		ini.beginGroup("defaults");
		{
			game.scavengers = ini.value("scavengers", game.scavengers).toBool();
			game.base = ini.value("bases", game.base).toInt();
			game.alliance = ini.value("alliances", game.alliance).toInt();
			game.power = ini.value("powerLevel", game.power).toInt();
		}
		ini.endGroup();
	}
}


static void resolveAIForPlayer(int player, WzString& aiValue)
{
	if (aiValue.compare("null") == 0)
	{
		return;
	}

	// strip given path down to filename
	WzString filename = WzString::fromUtf8(WzPathInfo::fromPlatformIndependentPath(aiValue.toUtf8()).fileName().c_str());

	// look up AI value in vector of known skirmish AIs
	for (unsigned ai = 0; ai < aidata.size(); ++ai)
	{
		if (filename == aidata[ai].js)
		{
			NetPlay.players[player].ai = ai;
			return;
		}
	}

	// did not find from known skirmish AIs, assume custom AI
	NetPlay.players[player].ai = AI_CUSTOM;
}

static void loadMapPlayerSettings(WzConfig& ini)
{
	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		ini.beginGroup("player_" + WzString::number(i));
		if (ini.contains("team"))
		{
			NetPlay.players[i].team = ini.value("team").toInt();
		}
		else if (challengeActive) // team is a required key for challenges
		{
			NetPlay.players[i].ai = AI_CLOSED;
		}

		/* Load pre-configured AIs */
		if (ini.contains("ai"))
		{
			WzString val = ini.value("ai").toWzString();
			resolveAIForPlayer(i, val);
		}

		/* Try finding a name field, if not found use AI names for AI players if in SP skirmish */
		if (ini.contains("name"))
		{
			sstrcpy(NetPlay.players[i].name, ini.value("name").toWzString().toUtf8().c_str());
		}
		else if (!NetPlay.bComms && i != selectedPlayer)
		{
			sstrcpy(NetPlay.players[i].name, getAIName(i));
		}

		NetPlay.players[i].position = MAX_PLAYERS;  // Invalid value, fix later.
		if (ini.contains("position"))
		{
			NetPlay.players[i].position = std::min(std::max(ini.value("position").toInt(), 0), MAX_PLAYERS);
		}
		if (ini.contains("difficulty"))
		{
			/* If difficulty is set, but we have no AI, use default */
			if (!ini.contains("ai"))
			{
				NetPlay.players[i].ai = 0;
			}

			WzString value = ini.value("difficulty", "Medium").toWzString();
			for (unsigned j = 0; j < ARRAY_SIZE(difficultyList); ++j)
			{
				if (strcasecmp(difficultyList[j], value.toUtf8().c_str()) == 0)
				{
					NetPlay.players[i].difficulty = difficultyValue[j];
				}
			}
		}
		if (ini.contains("faction"))
		{
			WzString value = ini.value("faction", "Normal").toWzString();
			for (uint8_t f_id = 0; f_id < NUM_FACTIONS; ++f_id)
			{
				const FACTION* faction = getFactionByID(static_cast<FactionID>(f_id));
				if (faction->name == value)
				{
					NetPlay.players[i].faction = static_cast<FactionID>(f_id);
				}
			}
		}
		ini.endGroup();
	}

	// Fix duplicate or unset player positions.
	PlayerMask havePosition = 0;
	for (int i = 0; i < MAX_PLAYERS; ++i)
	{
		if (NetPlay.players[i].position < MAX_PLAYERS)
		{
			PlayerMask old = havePosition;
			havePosition |= PlayerMask(1) << NetPlay.players[i].position;
			if (havePosition == old)
			{
				ASSERT(false, "Duplicate position %d", NetPlay.players[i].position);
				NetPlay.players[i].position = MAX_PLAYERS;
			}
		}
	}
	int pos = 0;
	for (int i = 0; i < MAX_PLAYERS; ++i)
	{
		if (NetPlay.players[i].position >= MAX_PLAYERS)
		{
			while ((havePosition & (PlayerMask(1) << pos)) != 0)
			{
				++pos;
			}
			NetPlay.players[i].position = pos;
			++pos;
		}
	}
}

static int playersPerTeam()
{
	for (unsigned numTeams = game.maxPlayers - 1; numTeams > 1; --numTeams)
	{
		if (game.maxPlayers % numTeams == 0)
		{
			return numTeams;
		}
	}
	return 1;
}

static void swapPlayerColours(uint32_t player1, uint32_t player2)
{
	auto player1Colour = getPlayerColour(player1);
	setPlayerColour(player1, getPlayerColour(player2));
	setPlayerColour(player2, player1Colour);
}

/**
 * Resets all player difficulties, positions, teams and colors etc.
 */
static void resetPlayerConfiguration(const bool bShouldResetLocal = false)
{
	auto selectedPlayerPosition = bShouldResetLocal? 0: NetPlay.players[selectedPlayer].position;
	for (unsigned playerIndex = 0; playerIndex < MAX_PLAYERS_IN_GUI; playerIndex++)
	{
		setPlayerColour(playerIndex, playerIndex);
		swapPlayerColours(playerIndex, rand() % (playerIndex + 1));
		NetPlay.players[playerIndex].position = playerIndex;

		if (!bShouldResetLocal && playerIndex == selectedPlayer)
		{
			continue;
		}

		NetPlay.players[playerIndex].team = playerIndex / playersPerTeam();

		if (NetPlay.bComms)
		{
			NetPlay.players[playerIndex].difficulty =  AIDifficulty::DISABLED;
			NetPlay.players[playerIndex].ai = AI_OPEN;
			NetPlay.players[playerIndex].name[0] = '\0';
		}
		else
		{
			NetPlay.players[playerIndex].difficulty = AIDifficulty::DEFAULT;
			NetPlay.players[playerIndex].ai = 0;

			/* ensure all players have a name in One Player Skirmish games */
			sstrcpy(NetPlay.players[playerIndex].name, getAIName(playerIndex));
		}
	}

	if (!bShouldResetLocal && selectedPlayerPosition < game.maxPlayers && selectedPlayer != selectedPlayerPosition) {
		std::swap(NetPlay.players[selectedPlayer].position, NetPlay.players[selectedPlayerPosition].position);
	}

	sstrcpy(NetPlay.players[selectedPlayer].name, sPlayer);

	for (unsigned playerIndex = 0; playerIndex < MAX_PLAYERS; playerIndex++)
	{
		if (getPlayerColour(playerIndex) == war_getMPcolour())
		{
			swapPlayerColours(selectedPlayer, playerIndex);
			break;
		}
	}
}

/**
 * Loads challenge and player configurations from level/autohost/test .json-files.
 */
static void loadMapChallengeAndPlayerSettings(bool forceLoadPlayers = false)
{
	char aFileName[256];
	LEVEL_DATASET* psLevel = levFindDataSet(game.map, &game.hash);

	ASSERT_OR_RETURN(, psLevel, "No level found for %s", game.map);
	sstrcpy(aFileName, psLevel->apDataFiles[psLevel->game]);
	aFileName[std::max<size_t>(strlen(aFileName), 4) - 4] = '\0';
	sstrcat(aFileName, ".json");

	WzString ininame = challengeActive ? sRequestResult : aFileName;
	bool warnIfMissing = false;
	if (getHostLaunch() == HostLaunch::Skirmish)
	{
		ininame = "tests/" + WzString::fromUtf8(wz_skirmish_test());
		warnIfMissing = true;
	}
	if (getHostLaunch() == HostLaunch::Autohost)
	{
		ininame = "autohost/" + WzString::fromUtf8(wz_skirmish_test());
		warnIfMissing = true;
	}

	const bool bIsOnline = NetPlay.bComms && NetPlay.isHost;
	if (!PHYSFS_exists(ininame.toUtf8().c_str()))
	{
		if (warnIfMissing)
		{
			debug(LOG_ERROR, "Missing specified file: %s", ininame.toUtf8().c_str());
		}

		/* Just reset the players if config is not found and host is not started yet */
		if (!bIsOnline) {
			resetPlayerConfiguration();
		}

		return;
	}
	WzConfig ini(ininame, WzConfig::ReadOnly);

	loadMapChallengeSettings(ini);

	/* Do not load player settings if we are already hosting an online match */
	if (!bIsOnline || forceLoadPlayers)
	{
		loadMapPlayerSettings(ini);
	}
}

static void randomizeLimit(const char *name)
{
	int stat = getStructStatFromName(name);
	if (rand() % 2 == 0)
	{
		asStructureStats[stat].upgrade[0].limit = asStructureStats[stat].base.limit;
	}
	else
	{
		asStructureStats[stat].upgrade[0].limit = 0;
	}
}

/* Generate random options */
static void randomizeOptions()
{
	RUN_ONLY_ON_SIDE(InGameSide::HOST_OR_SINGLEPLAYER)

	if (NetPlay.bComms && NetPlay.isHost && !canChangeMapOrRandomize())
	{
		return;
	}

	resetPlayerPositions();

	// Don't randomize the map once hosting for true multiplayer has started
	if (!NetPlay.isHost || !bMultiPlayer || !NetPlay.bComms)
	{
		// Pick a map for a number of players and tech level
		game.techLevel = (rand() % 4) + 1;
		LEVEL_LIST levels;
		do
		{
			// don't kick out already joined players because of randomize
			int players = NET_numHumanPlayers();
			int minimumPlayers = std::max(players, 2);
			current_numplayers = minimumPlayers;
			if (minimumPlayers < MAX_PLAYERS_IN_GUI)
			{
				current_numplayers += (rand() % (MAX_PLAYERS_IN_GUI - minimumPlayers));
			}
			levels = enumerateMultiMaps(game.techLevel, current_numplayers);
		}
		while (levels.empty()); // restart when there are no maps for a random number of players

		int pickedLevel = rand() % levels.size();
		LEVEL_DATASET *mapData = levels[pickedLevel];

		updateMapWidgets(mapData);
		loadMapPreview(false);
		loadMapChallengeAndPlayerSettings();
	}

	// Reset and randomize player positions, also to guard
	// against case where in the previous map some players
	// took slots, which aren't available anymore

	unsigned pos = repositionHumanSlots();

	// Fill with AIs if places remain
	for (int i = 0; i < current_numplayers; i++)
	{
		if (!isHumanPlayer(i) && !(game.mapHasScavengers && NetPlay.players[i].position == scavengerSlot()))
		{
			// Skip the scavenger slot
			if (game.mapHasScavengers && pos == scavengerSlot())
			{
				pos++;
			}
			NetPlay.players[i].position = pos;
			NETBroadcastPlayerInfo(i);
			pos++;
		}
	}

	// Randomize positions
	for (int i = 0; i < current_numplayers; i++)
	{
		SendPositionRequest(i, NetPlay.players[rand() % current_numplayers].position);
	}

	// Structure limits are simply 0 or max, only for NO_TANK, NO_CYBORG, NO_VTOL, NO_UPLINK, NO_LASSAT.
	if (!bLimiterLoaded || !asStructureStats)
	{
		initLoadingScreen(true);
		if (resLoad("wrf/limiter_data.wrf", 503))
		{
			bLimiterLoaded = true;
		}
		closeLoadingScreen();
	}
	resetLimits();
	for (int i = 0; i < ARRAY_SIZE(limitIcons) - 1; ++i)	// skip last item, MPFLAGS_FORCELIMITS
	{
		randomizeLimit(limitIcons[i].stat);
	}
	if (rand() % 2 == 0)
	{
		ingame.flags |= MPFLAGS_FORCELIMITS;
	}
	else
	{
		ingame.flags &= ~MPFLAGS_FORCELIMITS;
	}
	createLimitSet();
	applyLimitSet();
	updateStructureDisabledFlags();
	updateLimitIcons();

	// Game options
	if (!locked.scavengers && game.mapHasScavengers)
	{
		game.scavengers = rand() % 2;
		((MultichoiceWidget *)widgGetFromID(psWScreen, MULTIOP_GAMETYPE))->choose(game.scavengers);
	}

	if (!locked.alliances)
	{
		game.alliance = rand() % 4;
		((MultichoiceWidget *)widgGetFromID(psWScreen, MULTIOP_ALLIANCES))->choose(game.alliance);
	}
	if (!locked.power)
	{
		game.power = rand() % 3;
		((MultichoiceWidget *)widgGetFromID(psWScreen, MULTIOP_POWER))->choose(game.power);
	}
	if (!locked.bases)
	{
		game.base = rand() % 3;
		((MultichoiceWidget *)widgGetFromID(psWScreen, MULTIOP_BASETYPE))->choose(game.base);
	}
	if (NetPlay.isHost)
	{
		game.techLevel = rand() % 4;
		((MultichoiceWidget *)widgGetFromID(psWScreen, MULTIOP_TECHLEVEL))->choose(game.techLevel);

		resetReadyStatus(true);
	}
}

bool WzMultiplayerOptionsTitleUI::startHost()
{
	resetReadyStatus(false);
	removeWildcards((char*)sPlayer);

	const bool bIsAutoHostOrAutoGame = getHostLaunch() == HostLaunch::Skirmish || getHostLaunch() == HostLaunch::Autohost;
	if (!hostCampaign((char*)game.name, (char*)sPlayer, bIsAutoHostOrAutoGame))
	{
		displayRoomSystemMessage(_("Sorry! Failed to host the game."));
		return false;
	}

	bInActualHostedLobby = true;

	widgDelete(psWScreen, MULTIOP_REFRESH);
	widgDelete(psWScreen, MULTIOP_HOST);
	widgDelete(psWScreen, MULTIOP_FILTER_TOGGLE);

	ingame.localOptionsReceived = true;

	addGameOptions(); // update game options box.
	addChatBox();

	disableMultiButs();
	addPlayerBox(true);

	return true;
}

/*
 * Process click events on the multiplayer/skirmish options screen
 * 'id' is id of the button that was pressed
 */
void WzMultiplayerOptionsTitleUI::processMultiopWidgets(UDWORD id)
{
	PLAYERSTATS playerStats;

	// host, who is setting up the game
	if (ingame.side == InGameSide::HOST_OR_SINGLEPLAYER)
	{
		switch (id)												// Options buttons
		{
		case MULTIOP_GNAME:										// we get this when nec.
			sstrcpy(game.name, widgGetString(psWScreen, MULTIOP_GNAME));
			removeWildcards(game.name);
			widgSetString(psWScreen, MULTIOP_GNAME, game.name);

			if (NetPlay.isHost && NetPlay.bComms)
			{
				NETsetLobbyOptField(game.name, NET_LOBBY_OPT_FIELD::GNAME);
				sendOptions();
				NETregisterServer(WZ_SERVER_UPDATE);

				displayRoomSystemMessage(_("Game Name Updated."));
			}
			break;

		case MULTIOP_GNAME_ICON:
			break;

		case MULTIOP_MAP:
			widgDelete(psWScreen, MULTIOP_PLAYERS);
			widgDelete(psWScreen, FRONTEND_SIDETEXT2);  // del text too,

			debug(LOG_WZ, "processMultiopWidgets[MULTIOP_MAP_ICON]: %s.wrf", MultiCustomMapsPath);
			addMultiRequest(MultiCustomMapsPath, ".wrf", MULTIOP_MAP, 0, widgGetString(psWScreen, MULTIOP_MAP));

			widgSetString(psWScreen, MULTIOP_MAP + 1 , game.map); //What a horrible hack! FIX ME! (See addBlueForm())
			widgReveal(psWScreen, MULTIOP_MAP_MOD);
			widgReveal(psWScreen, MULTIOP_MAP_RANDOM);
			break;

		case MULTIOP_MAP_ICON:
			widgDelete(psWScreen, MULTIOP_PLAYERS);
			widgDelete(psWScreen, FRONTEND_SIDETEXT2);					// del text too,

			debug(LOG_WZ, "processMultiopWidgets[MULTIOP_MAP_ICON]: %s.wrf", MultiCustomMapsPath);
			addMultiRequest(MultiCustomMapsPath, ".wrf", MULTIOP_MAP, current_numplayers);

			if (NetPlay.isHost && NetPlay.bComms)
			{
				sendOptions();

				NETsetLobbyOptField(game.map, NET_LOBBY_OPT_FIELD::MAPNAME);
				NETregisterServer(WZ_SERVER_UPDATE);
			}
			break;

		case MULTIOP_MAP_PREVIEW:
			loadMapPreview(true);
			break;
		}
	}

	// host who is setting up or has hosted
	if (ingame.side == InGameSide::HOST_OR_SINGLEPLAYER)
	{
		switch (id)
		{
		case MULTIOP_GAMETYPE:
			game.scavengers = ((MultichoiceWidget *)widgGetFromID(psWScreen, MULTIOP_GAMETYPE))->currentValue();
			resetReadyStatus(false);
			if (NetPlay.isHost)
			{
				sendOptions();
			}
			break;

		case MULTIOP_BASETYPE:
			game.base = ((MultichoiceWidget *)widgGetFromID(psWScreen, MULTIOP_BASETYPE))->currentValue();
			addGameOptions();

			resetReadyStatus(false);

			if (NetPlay.isHost)
			{
				sendOptions();
			}
			break;

		case MULTIOP_ALLIANCES:
			game.alliance = ((MultichoiceWidget *)widgGetFromID(psWScreen, MULTIOP_ALLIANCES))->currentValue();

			resetReadyStatus(false);
			netPlayersUpdated = true;

			if (NetPlay.isHost)
			{
				sendOptions();
			}
			break;

		case MULTIOP_POWER:  // set power level
			game.power = ((MultichoiceWidget *)widgGetFromID(psWScreen, MULTIOP_POWER))->currentValue();

			resetReadyStatus(false);

			if (NetPlay.isHost)
			{
				sendOptions();
			}
			break;

		case MULTIOP_TECHLEVEL:
			game.techLevel = ((MultichoiceWidget *)widgGetFromID(psWScreen, MULTIOP_TECHLEVEL))->currentValue();
			addGameOptions(); //refresh to see the proper tech level in the map name

			resetReadyStatus(false);

			if (NetPlay.isHost)
			{
				sendOptions();
			}
			break;

		case MULTIOP_PASSWORD_EDIT:
			{
				unsigned result = widgGetButtonState(psWScreen, MULTIOP_PASSWORD_BUT);
				if (result != 0)
				{
					break;
				}
			}
			// fallthrough
		case MULTIOP_PASSWORD_BUT:
			{
				char buf[255];

				UDWORD currentButState = widgGetButtonState(psWScreen, MULTIOP_PASSWORD_BUT);
				bool willSet = (currentButState & WBUT_CLICKLOCK) == 0;
				char game_password[password_string_size] = {0};
				sstrcpy(game_password, widgGetString(psWScreen, MULTIOP_PASSWORD_EDIT));
				const size_t passLength = strlen(game_password) > 0;
				willSet &= (passLength > 0);
				debug(LOG_NET, "Password button hit, %d", (int)willSet);
				widgSetButtonState(psWScreen, MULTIOP_PASSWORD_BUT,  willSet ? WBUT_CLICKLOCK : 0);
				widgSetButtonState(psWScreen, MULTIOP_PASSWORD_EDIT, willSet ? WEDBS_DISABLE  : 0);
				if (willSet)
				{
					NETsetGamePassword(game_password);
					// say password is now required to join games?
					ssprintf(buf, _("*** password [%s] is now required! ***"), NetPlay.gamePassword);
					displayRoomNotifyMessage(buf);
				}
				else
				{
					NETresetGamePassword();
					ssprintf(buf, "%s", _("*** password is NOT required! ***"));
					displayRoomNotifyMessage(buf);
				}
			}
			break;
		}
	}

	// these work all the time.
	switch (id)
	{
	case MULTIOP_MAP_MOD:
		char buf[256];
		ssprintf(buf, "%s", _("This is a map-mod, it can change your playing experience!"));
		displayRoomSystemMessage(buf);
		break;

	case MULTIOP_MAP_RANDOM:
		ssprintf(buf, "%s", _("This is a random map, it can vary your playing experience!"));
		displayRoomSystemMessage(buf);
		break;

	case MULTIOP_STRUCTLIMITS:
		changeTitleUI(std::make_shared<WzMultiLimitTitleUI>(std::dynamic_pointer_cast<WzMultiplayerOptionsTitleUI>(wzTitleUICurrent)));
		break;

	case MULTIOP_PNAME:
		sstrcpy(sPlayer, widgGetString(psWScreen, MULTIOP_PNAME));

		// chop to 15 chars..
		while (strlen(sPlayer) > 15)	// clip name.
		{
			sPlayer[strlen(sPlayer) - 1] = '\0';
		}
		removeWildcards(sPlayer);
		// update string.
		widgSetString(psWScreen, MULTIOP_PNAME, sPlayer);
		printConsoleNameChange(NetPlay.players[selectedPlayer].name, sPlayer);

		NETchangePlayerName(selectedPlayer, (char *)sPlayer);			// update if joined.
		loadMultiStats((char *)sPlayer, &playerStats);
		setMultiStats(selectedPlayer, playerStats, false);
		setMultiStats(selectedPlayer, playerStats, true);
		lookupRatingAsync(selectedPlayer);
		netPlayersUpdated = true;

		if (NetPlay.isHost && NetPlay.bComms)
		{
			sendOptions();
			NETsetLobbyOptField(NetPlay.players[selectedPlayer].name, NET_LOBBY_OPT_FIELD::HOSTNAME);
			NETregisterServer(WZ_SERVER_UPDATE);
		}

		break;

	case MULTIOP_PNAME_ICON:
		widgDelete(psWScreen, MULTIOP_PLAYERS);
		widgDelete(psWScreen, FRONTEND_SIDETEXT2);					// del text too,

		addMultiRequest(MultiPlayersPath, ".sta2", MULTIOP_PNAME, 0);
		break;

	case MULTIOP_HOST:
		debug(LOG_NET, "MULTIOP_HOST enabled");
		sstrcpy(game.name, widgGetString(psWScreen, MULTIOP_GNAME));
		sstrcpy(sPlayer, widgGetString(psWScreen, MULTIOP_PNAME));

		resetVoteData();
		resetDataHash();

		startHost();
		break;
	case MULTIOP_RANDOM:
		randomizeOptions();
		break;

	case MULTIOP_CHATEDIT:

		// don't send empty lines to other players in the lobby
		if (!strcmp(widgGetString(psWScreen, MULTIOP_CHATEDIT), ""))
		{
			break;
		}

		sendRoomChatMessage(widgGetString(psWScreen, MULTIOP_CHATEDIT));
		widgSetString(psWScreen, MULTIOP_CHATEDIT, "");
		break;

	case CON_CANCEL:
		pie_LoadBackDrop(SCREEN_RANDOMBDROP);
		setHostLaunch(HostLaunch::Normal); // Dont load the autohost file on subsequent hosts
		performedFirstStart = false; // Reset everything
		if (!challengeActive)
		{
			if (NetPlay.bComms && ingame.side == InGameSide::MULTIPLAYER_CLIENT && !NetPlay.isHost)
			{
				// remove a potential "allow" vote if we gracefully leave
				sendVoteData(0);
			}
			NETGameLocked(false);		// reset status on a cancel
			stopJoining(parent);
		}
		else
		{
			NETclose();
			ingame.localJoiningInProgress = false;
			changeTitleMode(SINGLE);
			addChallenges();
		}
		break;
	case MULTIOP_MAP_PREVIEW:
		loadMapPreview(true);
		break;
	default:
		break;
	}

	STATIC_ASSERT(MULTIOP_TEAMS_START + MAX_PLAYERS - 1 <= MULTIOP_TEAMS_END);
	if (id >= MULTIOP_TEAMS_START && id <= MULTIOP_TEAMS_START + MAX_PLAYERS - 1 && !locked.teams)  // Clicked on a team chooser
	{
		int clickedMenuID = id - MULTIOP_TEAMS_START;

		//make sure team chooser is not up before adding new one for another player
		if (canChooseTeamFor(clickedMenuID) && positionChooserUp < 0)
		{
			openTeamChooser(clickedMenuID);
		}
	}

	// 'ready' button
	if (id >= MULTIOP_READY_START && id <= MULTIOP_READY_END) // clicked on a player
	{
		UBYTE player = (UBYTE)(id - MULTIOP_READY_START);

		if (player == selectedPlayer && positionChooserUp < 0)
		{
			// Lock the "ready" button (until the request is processed)
			widgSetButtonState(psWScreen, id, WBUT_LOCK);

			SendReadyRequest(selectedPlayer, !NetPlay.players[player].ready);

			// if hosting try to start the game if everyone is ready
			if (NetPlay.isHost && multiplayPlayersReady())
			{
				startMultiplayerGame();
				// reset flag in case people dropped/quit on join screen
				NETsetPlayerConnectionStatus(CONNECTIONSTATUS_NORMAL, NET_ALL_PLAYERS);
			}
		}

		if (NetPlay.isHost && !alliancesSetTeamsBeforeGame(game.alliance))
		{
			if (mouseDown(MOUSE_RMB) && player != NetPlay.hostPlayer) // both buttons....
			{
				std::string msg = astringf(_("The host has kicked %s from the game!"), getPlayerName(player));
				sendRoomSystemMessage(msg.c_str());
				kickPlayer(player, _("The host has kicked you from the game."), ERROR_KICKED);
				resetReadyStatus(true);		//reset and send notification to all clients
			}
		}
	}

	if (id >= MULTIOP_COLOUR_START && id <= MULTIOP_COLOUR_END && (id - MULTIOP_COLOUR_START == selectedPlayer || NetPlay.isHost))
	{
		if (positionChooserUp < 0)		// not choosing something else already
		{
			openColourChooser(id - MULTIOP_COLOUR_START);
		}
	}

	// clicked on a player
	STATIC_ASSERT(MULTIOP_PLAYER_START + MAX_PLAYERS - 1 <= MULTIOP_PLAYER_END);
	if (id >= MULTIOP_PLAYER_START && id <= MULTIOP_PLAYER_START + MAX_PLAYERS - 1
	    && (id - MULTIOP_PLAYER_START == selectedPlayer || NetPlay.isHost
	        || (positionChooserUp >= 0 && !isHumanPlayer(id - MULTIOP_PLAYER_START))))
	{
		int player = id - MULTIOP_PLAYER_START;
		if ((player == selectedPlayer || (NetPlay.players[player].allocated && NetPlay.isHost))
			&& !locked.position
		    && positionChooserUp < 0)
		{
			openPositionChooser(player);
		}
		else if (positionChooserUp == player)
		{
			closePositionChooser();	// changed his mind
			addPlayerBox(true);
		}
		else if (positionChooserUp >= 0)
		{
			// Switch player
			resetReadyStatus(false);		// will reset only locally if not a host
			SendPositionRequest(positionChooserUp, NetPlay.players[player].position);
			closePositionChooser();
			addPlayerBox(true);
		}
		else if (!NetPlay.players[player].allocated && !locked.ai && NetPlay.isHost
		         && positionChooserUp < 0)
		{
			if (widgGetButtonKey_DEPRECATED(psWScreen) == WKEY_SECONDARY)
			{
				// Right clicking distributes selected AI's type and difficulty to all other AIs
				for (int i = 0; i < MAX_PLAYERS; ++i)
				{
					// Don't change open/closed slots or humans
					if (NetPlay.players[i].ai >= 0 && i != player && !isHumanPlayer(i))
					{
						NetPlay.players[i].ai = NetPlay.players[player].ai;
						NetPlay.players[i].difficulty = NetPlay.players[player].difficulty;
						sstrcpy(NetPlay.players[i].name, getAIName(player));
						NETBroadcastPlayerInfo(i);
					}
				}
				addPlayerBox(true);
				resetReadyStatus(false);
			}
			else
			{
				openAiChooser(player);
			}
		}
	}

	if (id >= MULTIOP_DIFFICULTY_INIT_START && id <= MULTIOP_DIFFICULTY_INIT_END
	    && !locked.difficulty && NetPlay.isHost && positionChooserUp < 0)
	{
		openDifficultyChooser(id - MULTIOP_DIFFICULTY_INIT_START);
		addPlayerBox(true);
	}

	// clicked on faction chooser button
	if (id >= MULTIOP_FACTION_START && id <= MULTIOP_FACTION_END && (id - MULTIOP_FACTION_START == selectedPlayer || NetPlay.isHost))
	{
		if (positionChooserUp < 0)		// not choosing something else already
		{
			openFactionChooser(id - MULTIOP_FACTION_START);
		}
	}
}

/* Start a multiplayer or skirmish game */
void startMultiplayerGame()
{
	ASSERT_HOST_ONLY(return);

	decideWRF();										// set up swrf & game.map
	bMultiPlayer = true;
	bMultiMessages = true;
	NETsetPlayerConnectionStatus(CONNECTIONSTATUS_NORMAL, NET_ALL_PLAYERS);  // reset disconnect conditions
	initLoadingScreen(true);
	if (NetPlay.isHost)
	{
		// This sets the limits to whatever the defaults are for the limiter screen
		// If host sets limits, then we do not need to do the following routine.
		if (!bLimiterLoaded)
		{
			debug(LOG_NET, "limiter was NOT activated, setting defaults");

			if (!resLoad("wrf/limiter_data.wrf", 503))
			{
				debug(LOG_INFO, "Unable to load limiter_data.");
			}
		}
		else
		{
			debug(LOG_NET, "limiter was activated");
		}

		resetDataHash();	// need to reset it, since host's data has changed.
		createLimitSet();
		debug(LOG_NET, "sending our options to all clients");
		sendOptions();
		NEThaltJoining();							// stop new players entering.
		ingame.TimeEveryoneIsInGame = 0;
		ingame.isAllPlayersDataOK = false;
		memset(&ingame.DataIntegrity, 0x0, sizeof(ingame.DataIntegrity));	//clear all player's array
		SendFireUp();								//bcast a fireup message
	}

	debug(LOG_NET, "title mode STARTGAME is set--Starting Game!");
	changeTitleMode(STARTGAME);

	if (NetPlay.isHost)
	{
		sendRoomSystemMessage(_("Host is Starting Game"));
	}
}

// ////////////////////////////////////////////////////////////////////////////
// Net message handling

void WzMultiplayerOptionsTitleUI::frontendMultiMessages(bool running)
{
	NETQUEUE queue;
	uint8_t type;
	bool ignoredMessage = false;

	while (NETrecvNet(&queue, &type))
	{
		// Copy the message to the global one used by the new NET API
		switch (type)
		{
		case NET_FILE_REQUESTED:
			recvMapFileRequested(queue);
			break;

		case NET_FILE_PAYLOAD:
			{
				bool done = recvMapFileData(queue);
				if (running)
				{
					((MultibuttonWidget *)widgGetFromID(psWScreen, MULTIOP_MAP_PREVIEW))->enable(done);  // turn preview button on or off
				}
				break;
			}

		case NET_FILE_CANCELLED:
			{
				ASSERT_HOST_ONLY(break);

				Sha256 hash;
				hash.setZero();

				NETbeginDecode(queue, NET_FILE_CANCELLED);
				NETbin(hash.bytes, hash.Bytes);
				NETend();

				debug(LOG_WARNING, "Received file cancel request from player %u, they weren't expecting the file.", queue.index);
				auto &wzFiles = NetPlay.players[queue.index].wzFiles;
				wzFiles.erase(std::remove_if(wzFiles.begin(), wzFiles.end(), [&](WZFile const &file) { return file.hash == hash; }), wzFiles.end());
			}
			break;

		case NET_OPTIONS:					// incoming options file.
			recvOptions(queue);
			bInActualHostedLobby = true;
			ingame.localOptionsReceived = true;

			if (std::dynamic_pointer_cast<WzMultiplayerOptionsTitleUI>(wzTitleUICurrent))
			{
				addGameOptions();
				disableMultiButs();
				addChatBox();
			}
			break;

		case GAME_ALLIANCE:
			recvAlliance(queue, false);
			break;

		case NET_COLOURREQUEST:
			if (multiplayIsStartingGame())
			{
				ignoredMessage = true;
				break;
			}
			recvColourRequest(queue);
			break;

		case NET_FACTIONREQUEST:
			if (multiplayIsStartingGame())
			{
				ignoredMessage = true;
				break;
			}
			recvFactionRequest(queue);
			break;

		case NET_POSITIONREQUEST:
			if (multiplayIsStartingGame())
			{
				ignoredMessage = true;
				break;
			}
			recvPositionRequest(queue);
			break;

		case NET_TEAMREQUEST:
			if (multiplayIsStartingGame())
			{
				ignoredMessage = true;
				break;
			}
			recvTeamRequest(queue);
			break;

		case NET_READY_REQUEST:
			if (multiplayIsStartingGame())
			{
				ignoredMessage = true;
				break;
			}
			recvReadyRequest(queue);

			// If hosting and game not yet started, try to start the game if everyone is ready.
			if (NetPlay.isHost && multiplayPlayersReady())
			{
				startMultiplayerGame();
			}
			break;

		case NET_PING:						// diagnostic ping msg.
			recvPing(queue);
			break;

		case NET_PLAYER_DROPPED:		// remote player got disconnected
			{
				uint32_t player_id = MAX_PLAYERS;

				resetReadyStatus(false);

				NETbeginDecode(queue, NET_PLAYER_DROPPED);
				{
					NETuint32_t(&player_id);
				}
				NETend();

				if (player_id >= MAX_PLAYERS)
				{
					debug(LOG_INFO, "** player %u has dropped - huh?", player_id);
					break;
				}

				if (whosResponsible(player_id) != queue.index && queue.index != NET_HOST_ONLY)
				{
					HandleBadParam("NET_PLAYER_DROPPED given incorrect params.", player_id, queue.index);
					break;
				}

				debug(LOG_INFO, "** player %u has dropped!", player_id);

				MultiPlayerLeave(player_id);		// get rid of their stuff
				NET_InitPlayer(player_id, false);           // sets index player's array to false
				NETsetPlayerConnectionStatus(CONNECTIONSTATUS_PLAYER_DROPPED, player_id);
				playerVotes[player_id] = 0;
				ActivityManager::instance().updateMultiplayGameData(game, ingame, NETGameIsLocked());
				if (player_id == NetPlay.hostPlayer || player_id == selectedPlayer)	// if host quits or we quit, abort out
				{
					stopJoining(parent);
				}
				break;
			}
		case NET_PLAYERRESPONDING:			// remote player is now playing.
			{
				uint32_t player_id;

				resetReadyStatus(false);

				NETbeginDecode(queue, NET_PLAYERRESPONDING);
				// the player that has just responded
				NETuint32_t(&player_id);
				NETend();

				ingame.JoiningInProgress[player_id] = false;
				ingame.DataIntegrity[player_id] = false;
				break;
			}
		case NET_FIREUP:					// campaign game started.. can fire the whole shebang up...
			cancelOrDismissNotificationsWithTag(VOTE_TAG); // don't need vote notifications anymore
			if (NET_HOST_ONLY != queue.index)
			{
				HandleBadParam("NET_FIREUP given incorrect params.", 255, queue.index);
				break;
			}
			debug(LOG_NET, "NET_FIREUP was received ...");
			if (ingame.localOptionsReceived)
			{
				uint32_t randomSeed = 0;
				NETbeginDecode(queue, NET_FIREUP);
				NETuint32_t(&randomSeed);
				NETend();

				gameSRand(randomSeed);  // Set the seed for the synchronised random number generator, using the seed given by the host.

				debug(LOG_NET, "& local Options Received (MP game)");
				ingame.TimeEveryoneIsInGame = 0;			// reset time
				resetDataHash();
				decideWRF();

				bMultiPlayer = true;
				bMultiMessages = true;
				changeTitleMode(STARTGAME);

				// Start the game before processing more messages.
				NETpop(queue);
				return;
			}
			ASSERT(false, "NET_FIREUP was received, but !ingame.localOptionsReceived.");
			break;

		case NET_KICK:						// player is forcing someone to leave
			{
				uint32_t player_id;
				char reason[MAX_KICK_REASON];
				LOBBY_ERROR_TYPES KICK_TYPE = ERROR_NOERROR;

				NETbeginDecode(queue, NET_KICK);
				NETuint32_t(&player_id);
				NETstring(reason, MAX_KICK_REASON);
				NETenum(&KICK_TYPE);
				NETend();

				if (player_id >= MAX_PLAYERS)
				{
					debug(LOG_ERROR, "NET_KICK message with invalid player_id: (%" PRIu32")", player_id);
					break;
				}

				playerVotes[player_id] = 0;

				if (player_id == NET_HOST_ONLY)
				{
					char buf[250] = {'\0'};

					ssprintf(buf, "*Player %d (%s : %s) tried to kick %u", (int) queue.index, NetPlay.players[queue.index].name, NetPlay.players[queue.index].IPtextAddress, player_id);
					NETlogEntry(buf, SYNC_FLAG, 0);
					debug(LOG_ERROR, "%s", buf);
					if (NetPlay.isHost)
					{
						NETplayerKicked((unsigned int) queue.index);
					}
					break;
				}

				if (selectedPlayer == player_id)	// we've been told to leave.
				{
					setLobbyError(KICK_TYPE);
					stopJoining(std::make_shared<WzMsgBoxTitleUI>(WzString(_("You have been kicked: ")) + reason, parent));
					debug(LOG_INFO, "You have been kicked, because %s ", reason);
					displayKickReasonPopup(reason);
					ActivityManager::instance().wasKickedByPlayer(NetPlay.players[queue.index], KICK_TYPE, reason);
				}
				else
				{
					NETplayerKicked(player_id);
				}
				break;
			}
		case NET_HOST_DROPPED:
			NETbeginDecode(queue, NET_HOST_DROPPED);
			NETend();
			stopJoining(std::make_shared<WzMsgBoxTitleUI>(WzString(_("No connection to host.")), parent));
			debug(LOG_NET, "The host has quit!");
			setLobbyError(ERROR_HOSTDROPPED);
			break;

		case NET_TEXTMSG:					// Chat message
			if (ingame.localOptionsReceived)
			{
				NetworkTextMessage message;
				if (message.receive(queue)) {
					displayRoomMessage(buildMessage(message.sender, message.text));
					audio_PlayTrack(FE_AUDIO_MESSAGEEND);
				}
			}
			break;

		case NET_VOTE:
			if (NetPlay.isHost && ingame.localOptionsReceived)
			{
				recvVote(queue);
			}
			break;

		case NET_VOTE_REQUEST:
			if (!NetPlay.isHost)
			{
				setupVoteChoice();
			}
			break;

		default:
			ignoredMessage = true;
			break;
		}

		if (ignoredMessage)
		{
			debug(LOG_ERROR, "Didn't handle %s message!", messageTypeToString(type));
		}

		NETpop(queue);
	}
}

TITLECODE WzMultiplayerOptionsTitleUI::run()
{
	static UDWORD	lastrefresh = 0;
	PLAYERSTATS		playerStats;

	frontendMultiMessages(true);
	if (NetPlay.isHost)
	{
		// send it for each player that needs it
		sendMap();
	}

	// update boxes?
	if (netPlayersUpdated || (NetPlay.isHost && mouseDown(MOUSE_LMB) && gameTime - lastrefresh > 500))
	{
		netPlayersUpdated = false;
		lastrefresh = gameTime;
		if (!multiRequestUp && (NetPlay.isHost || ingame.localJoiningInProgress))
		{
			addPlayerBox(true);				// update the player box.
			loadMapPreview(false);
		}
	}


	// update scores and pings if far enough into the game
	if (ingame.localOptionsReceived && ingame.localJoiningInProgress)
	{
		sendScoreCheck();
		sendPing();
	}

	// if we don't have the focus, then autoclick in the chatbox.
	if (psWScreen->psFocus.expired())
	{
		W_CONTEXT context = W_CONTEXT::ZeroContext();
		context.mx			= mouseX();
		context.my			= mouseY();

		W_EDITBOX* pChatEdit = dynamic_cast<W_EDITBOX*>(widgGetFromID(psWScreen, MULTIOP_CHATEDIT));
		if (pChatEdit)
		{
			pChatEdit->simulateClick(&context, true);
		}
	}

	// chat box handling
	if (widgGetFromID(psWScreen, MULTIOP_CHATBOX))
	{
		while (getNumberConsoleMessages() > getConsoleLineInfo())
		{
			removeTopConsoleMessage();
		}
		updateConsoleMessages();								// run the chatbox
	}

	// widget handling

	/* Map or player selection is open */
	if (multiRequestUp)
	{
		WidgetTriggers const &triggers = widgRunScreen(psRScreen);
		unsigned id = triggers.empty() ? 0 : triggers.front().widget->id; // Just use first click here, since the next click could be on another menu.

		LEVEL_DATASET *mapData = nullptr;
		bool isHoverPreview = false;
		WzString sTemp;
		if (runMultiRequester(id, &id, &sTemp, &mapData, &isHoverPreview))
		{
			switch (id)
			{
			case MULTIOP_PNAME:
				sstrcpy(sPlayer, sTemp.toUtf8().c_str());
				widgSetString(psWScreen, MULTIOP_PNAME, sTemp.toUtf8().c_str());

				removeWildcards((char *)sPlayer);

				printConsoleNameChange(NetPlay.players[selectedPlayer].name, sPlayer);

				NETchangePlayerName(selectedPlayer, (char *)sPlayer);
				loadMultiStats((char *)sPlayer, &playerStats);
				setMultiStats(selectedPlayer, playerStats, false);
				setMultiStats(selectedPlayer, playerStats, true);
				lookupRatingAsync(selectedPlayer);
				netPlayersUpdated = true;
				if (NetPlay.isHost && NetPlay.bComms)
				{
					sendOptions();
					NETsetLobbyOptField(NetPlay.players[selectedPlayer].name, NET_LOBBY_OPT_FIELD::HOSTNAME);
					NETregisterServer(WZ_SERVER_UPDATE);
				}
				break;
			case MULTIOP_MAP:
				{
					if (isHoverPreview)
					{
						char oldGameMap[128];

						sstrcpy(oldGameMap, game.map);
						Sha256 oldGameHash = game.hash;
						uint8_t oldMaxPlayers = game.maxPlayers;
						bool oldGameIsMapMod = game.isMapMod;
						bool oldGameIsRandom = game.isRandom;

						sstrcpy(game.map, mapData->pName);
						game.hash = levGetFileHash(mapData);
						game.maxPlayers = mapData->players;
						game.isMapMod = CheckForMod(mapData->realFileName);
						game.isRandom = CheckForRandom(mapData->realFileName, mapData->apDataFiles[0]);
						loadMapPreview(false);

						/* Change game info to match the previous selection if hover preview was displayed */
						sstrcpy(game.map, oldGameMap);
						game.hash = oldGameHash;
						game.maxPlayers = oldMaxPlayers;
						game.isMapMod = oldGameIsMapMod;
						game.isRandom = oldGameIsRandom;
						break;
					}

					if (NetPlay.bComms && NetPlay.isHost)
					{
						uint8_t numHumans = NET_numHumanPlayers();
						if (numHumans > mapData->players)
						{
							displayRoomSystemMessage(_("Cannot change to a map with too few slots for all players."));
							break;
						}
						if (mapData->players < game.maxPlayers)
						{
							displayRoomSystemMessage(_("Cannot change to a map with fewer slots."));
							break;
						}
						if (!canChangeMapOrRandomize())
						{
							break;
						}
					}

					uint8_t oldMaxPlayers = game.maxPlayers;

					sstrcpy(game.map, mapData->pName);
					game.hash = levGetFileHash(mapData);
					game.maxPlayers = mapData->players;
					game.isMapMod = CheckForMod(mapData->realFileName);
					game.isRandom = CheckForRandom(mapData->realFileName, mapData->apDataFiles[0]);
					loadMapPreview(true);
					loadMapChallengeAndPlayerSettings();

					WzString name = formatGameName(game.map);
					widgSetString(psWScreen, MULTIOP_MAP + 1, name.toUtf8().c_str()); //What a horrible, horrible way to do this! FIX ME! (See addBlueForm)

					//Reset player slots if it's a smaller map.
					if (NetPlay.isHost && NetPlay.bComms && oldMaxPlayers > game.maxPlayers)
					{
						resetPlayerPositions();
						repositionHumanSlots();

						const std::vector<uint8_t>& humans = NET_getHumanPlayers();
						size_t playerInc = 0;

						for (uint8_t slotInc = 0; slotInc < game.maxPlayers && playerInc < humans.size(); ++slotInc)
						{
							changePosition(humans[playerInc], slotInc);
							++playerInc;
						}
					}

					addGameOptions();

					if (NetPlay.isHost && NetPlay.bComms)
					{
						sendOptions();
						NETsetLobbyOptField(game.map, NET_LOBBY_OPT_FIELD::MAPNAME);
						NETregisterServer(WZ_SERVER_UPDATE);
					}
				}
				break;
			default:
				loadMapPreview(false);  // Restore the preview of the old map.
				break;
			}
			if (!isHoverPreview)
			{
				addPlayerBox(ingame.side != InGameSide::HOST_OR_SINGLEPLAYER);
			}
		}
	}
	/* Map/Player selection (multi-requester) is closed */
	else
	{
		if (hideTime != 0)
		{
			// we abort the 'hidetime' on press of a mouse button.
			if (gameTime - hideTime < MAP_PREVIEW_DISPLAY_TIME && !mousePressed(MOUSE_LMB) && !mousePressed(MOUSE_RMB))
			{
				return TITLECODE_CONTINUE;
			}
			inputLoseFocus();	// remove the mousepress from the input stream.
			hideTime = 0;
		}
		else
		{
			WidgetTriggers const &triggers = widgRunScreen(psWScreen);
			unsigned id = triggers.empty() ? 0 : triggers.front().widget->id; // Just use first click here, since the next click could be on another menu.

			if (!triggers.empty() && (!multiplayIsStartingGame() || id == CON_CANCEL))
			{
				processMultiopWidgets(id);
			}
		}
	}

	widgDisplayScreen(psWScreen);									// show the widgets currently running

	if (multiRequestUp)
	{
		widgDisplayScreen(psRScreen);								// show the Requester running
	}

	if (CancelPressed())
	{
		processMultiopWidgets(CON_CANCEL);  // "Press" the cancel button to clean up net connections and stuff.
	}
	if (!NetPlay.isHostAlive && ingame.side == InGameSide::MULTIPLAYER_CLIENT)
	{
		cancelOrDismissNotificationsWithTag(VOTE_TAG);
		changeTitleUI(std::make_shared<WzMsgBoxTitleUI>(WzString(_("The host has quit.")), parent));
		pie_LoadBackDrop(SCREEN_RANDOMBDROP);
	}

	return TITLECODE_CONTINUE;
}

WzMultiplayerOptionsTitleUI::WzMultiplayerOptionsTitleUI(std::shared_ptr<WzTitleUI> parent)
	: parent(parent)
	, inlineChooserUp(-1)
	, aiChooserUp(-1)
	, difficultyChooserUp(-1)
	, positionChooserUp(-1)
{
}

WzMultiplayerOptionsTitleUI::~WzMultiplayerOptionsTitleUI()
{
	widgRemoveOverlayScreen(psInlineChooserOverlayScreen);
	bInActualHostedLobby = false;
}

void WzMultiplayerOptionsTitleUI::screenSizeDidChange(unsigned int oldWidth, unsigned int oldHeight, unsigned int newWidth, unsigned int newHeight)
{
	// NOTE: To properly support resizing the inline overlay screen based on underlying screen layer recalculations
	// frontendScreenSizeDidChange() should be called after intScreenSizeDidChange() in gameScreenSizeDidChange()
	if (psInlineChooserOverlayScreen == nullptr) return;
	psInlineChooserOverlayScreen->screenSizeDidChange(oldWidth, oldHeight, newWidth, newHeight);
}

static void printHostHelpMessagesToConsole()
{
	char buf[512] = { '\0' };
	if (NetPlay.bComms)
	{
		if (NetPlay.isUPNP)
		{
			if (NetPlay.isUPNP_CONFIGURED)
			{
				ssprintf(buf, "%s", _("UPnP has been enabled."));
			}
			else
			{
				if (NetPlay.isUPNP_ERROR)
				{
					ssprintf(buf, "%s", _("UPnP detection failed. You must manually configure router yourself."));
				}
				else
				{
					ssprintf(buf, "%s", _("UPnP detection is in progress..."));
				}
			}
			displayRoomNotifyMessage(buf);
		}
		else
		{
			ssprintf(buf, "%s", _("UPnP detection disabled by user. Autoconfig of port 2100 will not happen."));
			displayRoomNotifyMessage(buf);
		}
	}
	if (challengeActive)
	{
		ssprintf(buf, "%s", _("Hit the ready box to begin your challenge!"));
	}
	else if (!NetPlay.isHost)
	{
		ssprintf(buf, "%s", _("Press the start hosting button to begin hosting a game."));
	}
	displayRoomNotifyMessage(buf);
}

void calcBackdropLayoutForMultiplayerOptionsTitleUI(WIDGET *psWidget, unsigned int, unsigned int, unsigned int newScreenWidth, unsigned int newScreenHeight)
{
	auto height = newScreenHeight - 80;
	CLIP(height, HIDDEN_FRONTEND_HEIGHT, HIDDEN_FRONTEND_WIDTH);

	psWidget->setGeometry(
		((newScreenWidth - HIDDEN_FRONTEND_WIDTH) / 2),
		((newScreenHeight - height) / 2),
		HIDDEN_FRONTEND_WIDTH - 1,
		height
	);
}

void WzMultiplayerOptionsTitleUI::start()
{
	const bool bReenter = performedFirstStart;
	performedFirstStart = true;
	netPlayersUpdated = true;

	addBackdrop()->setCalcLayout(calcBackdropLayoutForMultiplayerOptionsTitleUI);
	addTopForm(true);

	if (getLobbyError() != ERROR_INVALID)
	{
		setLobbyError(ERROR_NOERROR);
	}

	/* Entering the first time */
	if (!bReenter)
	{
		initKnownPlayers();
		resetPlayerConfiguration(true);
		memset(&locked, 0, sizeof(locked));
		loadMapChallengeAndPlayerSettings(true);
		game.isMapMod = false;
		game.isRandom = false;
		game.mapHasScavengers = true; // FIXME, should default to false

		inlineChooserUp = -1;
		aiChooserUp = -1;
		difficultyChooserUp = -1;
		positionChooserUp = -1;

		// Initialize the inline chooser overlay screen
		psInlineChooserOverlayScreen = W_SCREEN::make();
		auto newRootFrm = W_FULLSCREENOVERLAY_CLICKFORM::make(MULTIOP_INLINE_OVERLAY_ROOT_FRM);
		std::weak_ptr<W_SCREEN> psWeakInlineOverlayScreen(psInlineChooserOverlayScreen);
		WzMultiplayerOptionsTitleUI *psTitleUI = this;
		newRootFrm->onClickedFunc = [psWeakInlineOverlayScreen, psTitleUI]() {
			if (auto psOverlayScreen = psWeakInlineOverlayScreen.lock())
			{
				widgRemoveOverlayScreen(psOverlayScreen);
			}
			psTitleUI->closeAllChoosers();
			psTitleUI->addPlayerBox(true);
		};
		newRootFrm->onCancelPressed = newRootFrm->onClickedFunc;
		psInlineChooserOverlayScreen->psForm->attach(newRootFrm);

		ingame.localOptionsReceived = false;

		PLAYERSTATS	nullStats;
		loadMultiStats((char*)sPlayer, &nullStats);
		lookupRatingAsync(selectedPlayer);

		/* Entering the first time with challenge, immediately start the host */
		if (challengeActive && !startHost())
		{
			debug(LOG_ERROR, "Failed to host the challenge.");
			return;
		}
	}

	loadMapPreview(false);

	/* Re-entering or entering without a challenge */
	if (bReenter || !challengeActive)
	{
		addPlayerBox(false);
		addGameOptions();
		addChatBox(bReenter);

		if (ingame.side == InGameSide::HOST_OR_SINGLEPLAYER)
		{
			printHostHelpMessagesToConsole();
		}
	}

	/* Reset structure limits if we are entering the first time or if we have a challenge */
	if (!bReenter || challengeActive)
	{
		resetLimits();
		updateStructureDisabledFlags();
		updateLimitIcons();
	}

	if (autogame_enabled() || getHostLaunch() == HostLaunch::Autohost)
	{
		if (!ingame.localJoiningInProgress)
		{
			processMultiopWidgets(MULTIOP_HOST);
		}
		SendReadyRequest(selectedPlayer, true);
		if (getHostLaunch() == HostLaunch::Skirmish)
		{
			startMultiplayerGame();
			// reset flag in case people dropped/quit on join screen
			NETsetPlayerConnectionStatus(CONNECTIONSTATUS_NORMAL, NET_ALL_PLAYERS);
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
// Drawing functions

void displayChatEdit(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset)
{
	int x = xOffset + psWidget->x();
	int y = yOffset + psWidget->y();

	// draws the line at the bottom of the multiplayer join dialog separating the chat
	// box from the input box
	iV_Line(x, y, x + psWidget->width(), y, WZCOL_MENU_SEPARATOR);
}

// ////////////////////////////////////////////////////////////////////////////

void displayTeamChooser(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset)
{
	int x = xOffset + psWidget->x();
	int y = yOffset + psWidget->y();
	UDWORD		i = psWidget->UserData;

	ASSERT_OR_RETURN(, i < MAX_PLAYERS && NetPlay.players[i].team >= 0 && NetPlay.players[i].team < MAX_PLAYERS, "Team index out of bounds");

	drawBlueBox(x, y, psWidget->width(), psWidget->height());

	if (NetPlay.players[i].difficulty != AIDifficulty::DISABLED)
	{
		iV_DrawImage(FrontImages, IMAGE_TEAM0 + NetPlay.players[i].team, x + 2, y + 8);
	}
}

void displayPosition(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset)
{
	// Any widget using displayPosition must have its pUserData initialized to a (DisplayPositionCache*)
	assert(psWidget->pUserData != nullptr);
	DisplayPositionCache& cache = *static_cast<DisplayPositionCache *>(psWidget->pUserData);

	const int x = xOffset + psWidget->x();
	const int y = yOffset + psWidget->y();
	const int i = psWidget->UserData;
	char text[80];

	drawBlueBox(x, y, psWidget->width(), psWidget->height());
	ssprintf(text, _("Click to take player slot %d"), NetPlay.players[i].position);
	cache.wzPositionText.setText(text, font_regular);
	cache.wzPositionText.render(x + 10, y + 22, WZCOL_FORM_TEXT);
}

static void displayAi(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset)
{
	// Any widget using displayAi must have its pUserData initialized to a (DisplayAICache*)
	assert(psWidget->pUserData != nullptr);
	DisplayAICache& cache = *static_cast<DisplayAICache *>(psWidget->pUserData);

	const int x = xOffset + psWidget->x();
	const int y = yOffset + psWidget->y();
	const int j = psWidget->UserData;
	const char *commsText[] = { N_("Open"), N_("Closed") };

	drawBlueBox(x, y, psWidget->width(), psWidget->height());
	cache.wzText.setText((j >= 0) ? aidata[j].name : gettext(commsText[j + 2]), font_regular);
	cache.wzText.render(x + 10, y + 22, WZCOL_FORM_TEXT);
}

static int difficultyIcon(int difficulty)
{
	switch (difficulty)
	{
	case 0: return IMAGE_EASY;
	case 1: return IMAGE_MEDIUM;
	case 2: return IMAGE_HARD;
	case 3: return IMAGE_INSANE;
	default: return IMAGE_NO;	/// what??
	}
}

static int factionIcon(FactionID faction)
{
	switch (faction)
	{
	case 0: return IMAGE_FACTION_NORMAL;
	case 1: return IMAGE_FACTION_NEXUS;
	case 2: return IMAGE_FACTION_COLLECTIVE;
	default: return IMAGE_NO;	/// what??
	}
}

static void displayDifficulty(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset)
{
	// Any widget using displayDifficulty must have its pUserData initialized to a (DisplayDifficultyCache*)
	assert(psWidget->pUserData != nullptr);
	DisplayDifficultyCache& cache = *static_cast<DisplayDifficultyCache *>(psWidget->pUserData);

	const int x = xOffset + psWidget->x();
	const int y = yOffset + psWidget->y();
	const int j = psWidget->UserData;

	ASSERT_OR_RETURN(, j < ARRAY_SIZE(difficultyList), "Bad difficulty found: %d", j);
	drawBlueBox(x, y, psWidget->width(), psWidget->height());
	iV_DrawImage(FrontImages, difficultyIcon(j), x + 5, y + 5);
	cache.wzDifficultyText.setText(gettext(difficultyList[j]), font_regular);
	cache.wzDifficultyText.render(x + 42, y + 22, WZCOL_FORM_TEXT);
}

static bool isKnownPlayer(std::map<std::string, EcKey::Key> const &knownPlayers, std::string const &name, EcKey const &key)
{
	if (key.empty())
	{
		return false;
	}
	std::map<std::string, EcKey::Key>::const_iterator i = knownPlayers.find(name);
	return i != knownPlayers.end() && key.toBytes(EcKey::Public) == i->second;
}

// ////////////////////////////////////////////////////////////////////////////
void displayPlayer(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset)
{
	// Any widget using displayPlayer must have its pUserData initialized to a (DisplayPlayerCache*)
	assert(psWidget->pUserData != nullptr);
	DisplayPlayerCache& cache = *static_cast<DisplayPlayerCache *>(psWidget->pUserData);

	int const x = xOffset + psWidget->x();
	int const y = yOffset + psWidget->y();
	unsigned const j = psWidget->UserData;

	const int nameX = 32;

	unsigned downloadProgress = NETgetDownloadProgress(j);

	drawBlueBox(x, y, psWidget->width(), psWidget->height());
	if (downloadProgress != 100)
	{
		char progressString[MAX_STR_LENGTH];
		ssprintf(progressString, j != selectedPlayer ? _("Sending Map: %u%% ") : _("Map: %u%% downloaded"), downloadProgress);
		cache.wzMainText.setText(progressString, font_regular);
		cache.wzMainText.render(x + 5, y + 22, WZCOL_FORM_TEXT);
		return;
	}
	else if (ingame.localOptionsReceived && NetPlay.players[j].allocated)					// only draw if real player!
	{
		std::string name = NetPlay.players[j].name;

		drawBlueBox(x, y, psWidget->width(), psWidget->height());

		std::map<std::string, EcKey::Key> serverPlayers;  // TODO Fill this with players known to the server (needs implementing on the server, too). Currently useless.

		PIELIGHT colour;
		if (ingame.PingTimes[j] >= PING_LIMIT)
		{
			colour = WZCOL_FORM_PLAYER_NOPING;
		}
		else if (isKnownPlayer(serverPlayers, name, getMultiStats(j).identity))
		{
			colour = WZCOL_FORM_PLAYER_KNOWN_BY_SERVER;
		}
		else if (isLocallyKnownPlayer(name, getMultiStats(j).identity))
		{
			colour = WZCOL_FORM_PLAYER_KNOWN;
		}
		else
		{
			colour = WZCOL_FORM_PLAYER_UNKNOWN;
		}

		// name
		if (cache.fullMainText != name)
		{
			if ((int)iV_GetTextWidth(name.c_str(), font_regular) > psWidget->width() - nameX)
			{
				while (!name.empty() && (int)iV_GetTextWidth((name + "...").c_str(), font_regular) > psWidget->width() - nameX)
				{
					name.resize(name.size() - 1);  // Clip name.
				}
				name += "...";
			}
			cache.wzMainText.setText(name, font_regular);
			cache.fullMainText = name;
		}
		std::string subText;
		if (j == NET_HOST_ONLY && NetPlay.bComms)
		{
			subText += _("HOST");
		}
		if (NetPlay.bComms && j != selectedPlayer)
		{
			char buf[250] = {'\0'};

			// show "actual" ping time
			ssprintf(buf, "%s%s: ", subText.empty() ? "" : ", ", _("Ping"));
			subText += buf;
			if (ingame.PingTimes[j] < PING_LIMIT)
			{
				ssprintf(buf, "%03d", ingame.PingTimes[j]);
			}
			else
			{
				ssprintf(buf, "%s", "∞");  // Player has ping of somewhat questionable quality.
			}
			subText += buf;
		}

		PLAYERSTATS stat = getMultiStats(j);
		auto ar = stat.autorating;
		if (!ar.valid)
		{
			ar.dummy = stat.played < 5;
			// star 1 total droid kills
			ar.star[0] = stat.totalKills > 600? 1 : stat.totalKills > 300? 2 : stat.totalKills > 150? 3 : 0;

			// star 2 games played
			ar.star[1] = stat.played > 200? 1 : stat.played > 100? 2 : stat.played > 50? 3 : 0;

			// star 3 games won.
			ar.star[2] = stat.wins > 80? 1 : stat.wins > 40? 2 : stat.wins > 10? 3 : 0;

			// medals.
			ar.medal = stat.wins >= 24 && stat.wins > 8 * stat.losses? 1 : stat.wins >= 12 && stat.wins > 4 * stat.losses? 2 : stat.wins >= 6 && stat.wins > 2 * stat.losses? 3 : 0;

			ar.level = 0;
			ar.autohoster = false;
			ar.elo.clear();
		}

		int H = 5;
		cache.wzMainText.render(x + nameX, y + 22 - H*!subText.empty() - H*(ar.valid && !ar.elo.empty()), colour);
		if (!subText.empty())
		{
			cache.wzSubText.setText(subText, font_small);
			cache.wzSubText.render(x + nameX, y + 28 - H*!ar.elo.empty(), WZCOL_TEXT_MEDIUM);
		}

		if (ar.autohoster)
		{
			iV_DrawImage(FrontImages, IMAGE_PLAYER_PC, x, y + 11);
		}
		else if (ar.dummy)
		{
			iV_DrawImage(FrontImages, IMAGE_MEDAL_DUMMY, x + 4, y + 13);
		}
		else
		{
			constexpr int starImgs[4] = {0, IMAGE_MULTIRANK1, IMAGE_MULTIRANK2, IMAGE_MULTIRANK3};
			if (1 <= ar.star[0] && ar.star[0] < ARRAY_SIZE(starImgs))
			{
				iV_DrawImage(FrontImages, starImgs[ar.star[0]], x + 4, y + 3);
			}
			if (1 <= ar.star[1] && ar.star[1] < ARRAY_SIZE(starImgs))
			{
				iV_DrawImage(FrontImages, starImgs[ar.star[1]], x + 4, y + 13);
			}
			if (1 <= ar.star[2] && ar.star[2] < ARRAY_SIZE(starImgs))
			{
				iV_DrawImage(FrontImages, starImgs[ar.star[2]], x + 4, y + 23);
			}
			constexpr int medalImgs[4] = {0, IMAGE_MEDAL_GOLD, IMAGE_MEDAL_SILVER, IMAGE_MEDAL_BRONZE};
			if (1 <= ar.medal && ar.medal < ARRAY_SIZE(medalImgs))
			{
				iV_DrawImage(FrontImages, medalImgs[ar.medal], x + 16 - 2*(ar.level != 0), y + 11);
			}
		}
		constexpr int levelImgs[9] = {0, IMAGE_LEV_0, IMAGE_LEV_1, IMAGE_LEV_2, IMAGE_LEV_3, IMAGE_LEV_4, IMAGE_LEV_5, IMAGE_LEV_6, IMAGE_LEV_7};
		if (1 <= ar.level && ar.level < ARRAY_SIZE(levelImgs))
		{
			iV_DrawImage(IntImages, levelImgs[ar.star[2]], x + 24, y + 15);
		}

		if (!ar.elo.empty())
		{
			cache.wzEloText.setText(ar.elo, font_small);
			cache.wzEloText.render(x + nameX, y + 28 + H*!subText.empty(), WZCOL_TEXT_BRIGHT);
		}

		NetPlay.players[j].difficulty = AIDifficulty::HUMAN;
	}
	else	// AI
	{
		char aitext[80];

		if (NetPlay.players[j].ai >= 0)
		{
			iV_DrawImage(FrontImages, IMAGE_PLAYER_PC, x, y + 11);
		}

		// challenges may use custom AIs that are not in aidata and set to 127
		if (!challengeActive)
		{
		    ASSERT_OR_RETURN(, NetPlay.players[j].ai < (int)aidata.size(), "Uh-oh, AI index out of bounds");
		}

		switch (NetPlay.players[j].ai)
		{
		case AI_OPEN: sstrcpy(aitext, _("Open")); break;
		case AI_CLOSED: sstrcpy(aitext, _("Closed")); break;
		default: sstrcpy(aitext, NetPlay.players[j].name ); break;
		}
		cache.wzMainText.setText(aitext, font_regular);
		cache.wzMainText.render(x + nameX, y + 22, WZCOL_FORM_TEXT);
	}
}

void displayColour(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset)
{
	const int x = xOffset + psWidget->x();
	const int y = yOffset + psWidget->y();
	const int j = psWidget->UserData;

	drawBlueBox(x, y, psWidget->width(), psWidget->height());
	if (NetPlay.players[j].wzFiles.empty() && NetPlay.players[j].difficulty != AIDifficulty::DISABLED)
	{
		int player = getPlayerColour(j);
		STATIC_ASSERT(MAX_PLAYERS <= 16);
		iV_DrawImageTc(FrontImages, IMAGE_PLAYERN, IMAGE_PLAYERN_TC, x + 7, y + 9, pal_GetTeamColour(player));
	}
}

void displayFaction(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset)
{
	const int x = xOffset + psWidget->x();
	const int y = yOffset + psWidget->y();
	const int j = psWidget->UserData;

	drawBlueBox(x, y, psWidget->width(), psWidget->height());
	if (NetPlay.players[j].wzFiles.empty() && NetPlay.players[j].difficulty != AIDifficulty::DISABLED)
	{
		FactionID faction = NetPlay.players[j].faction;
		iV_DrawImage(FrontImages, factionIcon(faction), x + 5, y + 8);
	}
}

// ////////////////////////////////////////////////////////////////////////////
// Display blue box
void intDisplayFeBox(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset)
{
	int x = xOffset + psWidget->x();
	int y = yOffset + psWidget->y();
	int w = psWidget->width();
	int h = psWidget->height();

	drawBlueBox(x, y, w, h);
}

// ////////////////////////////////////////////////////////////////////////////
// Display edit box
void displayMultiEditBox(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset)
{
	int x = xOffset + psWidget->x();
	int y = yOffset + psWidget->y();

	drawBlueBox(x, y, psWidget->width(), psWidget->height());

	if (((W_EDITBOX *)psWidget)->state & WEDBS_DISABLE)					// disabled
	{
		PIELIGHT colour;

		colour.byte.r = FILLRED;
		colour.byte.b = FILLBLUE;
		colour.byte.g = FILLGREEN;
		colour.byte.a = FILLTRANS;
		pie_UniTransBoxFill(x, y, x + psWidget->width() + psWidget->height(), y + psWidget->height(), colour);
	}
}

Image mpwidgetGetFrontHighlightImage(Image image)
{
	if (image.isNull())
	{
		return Image();
	}
	switch (image.width())
	{
	case 30: return Image(FrontImages, IMAGE_HI34);
	case 60: return Image(FrontImages, IMAGE_HI64);
	case 19: return Image(FrontImages, IMAGE_HI23);
	case 27: return Image(FrontImages, IMAGE_HI31);
	case 35: return Image(FrontImages, IMAGE_HI39);
	case 37: return Image(FrontImages, IMAGE_HI41);
	case 56: return Image(FrontImages, IMAGE_HI56);
	}
	return Image();
}

void WzMultiButton::display(int xOffset, int yOffset)
{
	int x0 = xOffset + x();
	int y0 = yOffset + y();
	Image hiToUse(nullptr, 0);

	// FIXME: This seems to be a way to conserve space, so you can use a
	// transparent icon with these edit boxes.
	// hack for multieditbox
	if (imNormal.id == IMAGE_EDIT_MAP || imNormal.id == IMAGE_EDIT_GAME || imNormal.id == IMAGE_EDIT_PLAYER
	    || imNormal.id == IMAGE_LOCK_BLUE || imNormal.id == IMAGE_UNLOCK_BLUE)
	{
		drawBlueBox(x0 - 2, y0 - 2, height(), height());  // box on end.
	}

	// evaluate auto-frame
	bool highlight = (getState() & WBUT_HIGHLIGHT) != 0;

	// evaluate auto-frame
	if (doHighlight == 1 && highlight)
	{
		hiToUse = mpwidgetGetFrontHighlightImage(imNormal);
	}

	bool down = (getState() & (WBUT_DOWN | WBUT_LOCK | WBUT_CLICKLOCK)) != 0;
	bool grey = (getState() & WBUT_DISABLE) != 0;

	Image toDraw[3];
	int numToDraw = 0;

	// now display
	toDraw[numToDraw++] = imNormal;

	// hilights etc..
	if (down)
	{
		toDraw[numToDraw++] = imDown;
	}
	if (highlight && !grey && hiToUse.images != nullptr)
	{
		toDraw[numToDraw++] = hiToUse;
	}

	for (int n = 0; n < numToDraw; ++n)
	{
		Image tcImage(toDraw[n].images, toDraw[n].id + 1);
		if (tc == MAX_PLAYERS)
		{
			iV_DrawImage(toDraw[n], x0, y0);
		}
		else if (tc == MAX_PLAYERS + 1)
		{
			const int scale = 4000;
			int f = realTime % scale;
			PIELIGHT mix;
			mix.byte.r = 128 + iSinR(65536 * f / scale + 65536 * 0 / 3, 127);
			mix.byte.g = 128 + iSinR(65536 * f / scale + 65536 * 1 / 3, 127);
			mix.byte.b = 128 + iSinR(65536 * f / scale + 65536 * 2 / 3, 127);
			mix.byte.a = 255;
			iV_DrawImageTc(toDraw[n], tcImage, x0, y0, mix);
		}
		else
		{
			iV_DrawImageTc(toDraw[n], tcImage, x0, y0, pal_GetTeamColour(tc));
		}
	}

	if (grey)
	{
		// disabled, render something over it!
		iV_TransBoxFill(x0, y0, x0 + width(), y0 + height());
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
// common widgets

static W_EDITBOX* addMultiEditBox(UDWORD formid, UDWORD id, UDWORD x, UDWORD y, char const *tip, char const *tipres, UDWORD icon, UDWORD iconhi, UDWORD iconid)
{
	W_EDBINIT sEdInit;                           // editbox
	sEdInit.formID = formid;
	sEdInit.id = id;
	sEdInit.x = (short)x;
	sEdInit.y = (short)y;
	sEdInit.width = MULTIOP_EDITBOXW;
	sEdInit.height = MULTIOP_EDITBOXH;
	sEdInit.pText = tipres;
	sEdInit.pBoxDisplay = displayMultiEditBox;
	auto editBox = widgAddEditBox(psWScreen, &sEdInit);
	if (!editBox)
	{
		return nullptr;
	}

	addMultiBut(psWScreen, MULTIOP_OPTIONS, iconid, x + MULTIOP_EDITBOXW + 2, y + 2, MULTIOP_EDITBOXH, MULTIOP_EDITBOXH, tip, icon, iconhi, iconhi);
	return editBox;
}

/////////////////////////////////////////////////////////////////////////////////////////
std::shared_ptr<W_BUTTON> addMultiBut(WIDGET &parent, UDWORD id, UDWORD x, UDWORD y, UDWORD width, UDWORD height, const char *tipres, UDWORD norm, UDWORD down, UDWORD hi, unsigned tc)
{
	std::shared_ptr<WzMultiButton> button;
	auto existingWidget = widgFormGetFromID(parent.shared_from_this(), id);
	if (existingWidget)
	{
		button = std::dynamic_pointer_cast<WzMultiButton>(existingWidget);
	}
	if (button == nullptr)
	{
		button = std::make_shared<WzMultiButton>();
		parent.attach(button);
		button->id = id;
	}
	button->setGeometry(x, y, width, height);
	button->setTip((tipres != nullptr) ? std::string(tipres) : std::string());
	button->imNormal = Image(FrontImages, norm);
	button->imDown = Image(FrontImages, down);
	button->doHighlight = hi;
	button->tc = tc;
	return button;
}

std::shared_ptr<W_BUTTON> addMultiBut(const std::shared_ptr<W_SCREEN> &screen, UDWORD formid, UDWORD id, UDWORD x, UDWORD y, UDWORD width, UDWORD height, const char *tipres, UDWORD norm, UDWORD down, UDWORD hi, unsigned tc)
{
	return addMultiBut(*widgGetFromID(screen, formid), id, x, y, width, height, tipres, norm, down, hi, tc);
}

/* Returns true if the multiplayer game can start (i.e. all players are ready) */
static bool multiplayPlayersReady()
{
	bool bReady = true;
	size_t numReadyPlayers = 0;

	for (unsigned int player = 0; player < game.maxPlayers; player++)
	{
		// check if this human player is ready, ignore AIs
		if (NetPlay.players[player].allocated)
		{
			if ((!NetPlay.players[player].ready || ingame.PingTimes[player] >= PING_LIMIT))
			{
				bReady = false;
			}
			else
			{
				numReadyPlayers++;
			}
		}
		else if (NetPlay.players[player].ai >= 0)
		{
			numReadyPlayers++;
		}
	}

	return bReady && numReadyPlayers > 0;
}

static bool multiplayIsStartingGame()
{
	return bInActualHostedLobby && multiplayPlayersReady();
}

void sendRoomSystemMessage(char const *text)
{
	NetworkTextMessage message(SYSTEM_MESSAGE, text);
	displayRoomSystemMessage(text);
	message.enqueue(NETbroadcastQueue());
}

static void sendRoomChatMessage(char const *text)
{
	NetworkTextMessage message(selectedPlayer, text);
	displayRoomMessage(RoomMessage::player(selectedPlayer, text));
	message.enqueue(NETbroadcastQueue());
}
