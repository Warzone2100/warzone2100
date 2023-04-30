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
#include "lib/widget/checkbox.h"

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
#include "multilobbycommands.h"
#include "stdinreader.h"

#include "activity.h"
#include <algorithm>
#include "3rdparty/gsl_finally.h"

#define MAP_PREVIEW_DISPLAY_TIME 2500	// number of milliseconds to show map in preview
#define VOTE_TAG                 "voting"
#define KICK_REASON_TAG          "kickReason"
#define SLOTTYPE_TAG_PREFIX      "slotType"
#define SLOTTYPE_REQUEST_TAG     SLOTTYPE_TAG_PREFIX "::request"
#define SLOTTYPE_NOTIFICATION_ID_PREFIX SLOTTYPE_REQUEST_TAG "::"

#define PLAYERBOX_X0 7

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
static bool bRequestedSelfMoveToPlayers = false;
static std::vector<bool> bHostRequestedMoveToPlayers = std::vector<bool>(MAX_CONNECTED_PLAYERS, false);

enum class PlayerDisplayView
{
	Players,
	Spectators
};
static PlayerDisplayView playerDisplayView = PlayerDisplayView::Players;

static std::weak_ptr<WzMultiplayerOptionsTitleUI> currentMultiOptionsTitleUI;

/// end of globals.
// ////////////////////////////////////////////////////////////////////////////
// Function protos

// widget functions
static W_EDITBOX* addMultiEditBox(UDWORD formid, UDWORD id, UDWORD x, UDWORD y, char const *tip, char const *tipres, UDWORD icon, UDWORD iconhi, UDWORD iconid);
static std::shared_ptr<W_FORM> createBlueForm(int x, int y, int w, int h, WIDGET_DISPLAY displayFunc = intDisplayFeBox);
static W_FORM * addBlueForm(UDWORD parent, UDWORD id, UDWORD x, UDWORD y, UDWORD w, UDWORD h, WIDGET_DISPLAY displayFunc = intDisplayFeBox);
static int numSlotsToBeDisplayed();
static inline bool spectatorSlotsSupported();
static inline bool isSpectatorOnlySlot(UDWORD playerIdx);
//static inline bool isSpectatorOnlyPosition(UDWORD playerPosition);

// Drawing Functions
static void displayChatEdit(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset);
static void displayPlayer(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset);
static void displayReadyBoxContainer(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset);
static void displayColour(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset);
static void displayTeamChooser(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset);
static void displaySpectatorAddButton(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset);
static void displayAi(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset);
static void displayDifficulty(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset);
static void displayMultiEditBox(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset);
static void drawBlueBox_Spectator(UDWORD x, UDWORD y, UDWORD w, UDWORD h);
static void drawBlueBox_SpectatorOnly(UDWORD x, UDWORD y, UDWORD w, UDWORD h);
void intDisplayFeBox_SpectatorOnly(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset);
static inline void drawBoxForPlayerInfoSegment(UDWORD playerIdx, UDWORD x, UDWORD y, UDWORD w, UDWORD h);

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
static bool		SendPlayerSlotTypeRequest(uint32_t player, bool isSpectator);
bool changeReadyStatus(UBYTE player, bool bReady);
static void stopJoining(std::shared_ptr<WzTitleUI> parent);
static int difficultyIcon(int difficulty);

void sendRoomSystemMessageToSingleReceiver(char const *text, uint32_t receiver);
static void sendRoomChatMessage(char const *text);

static bool multiplayPlayersReady();
static bool multiplayIsStartingGame();
// ////////////////////////////////////////////////////////////////////////////
// map previews..

static const char *difficultyList[] = { N_("Super Easy"), N_("Easy"), N_("Medium"), N_("Hard"), N_("Insane") };
static const AIDifficulty difficultyValue[] = { AIDifficulty::SUPEREASY, AIDifficulty::EASY, AIDifficulty::MEDIUM, AIDifficulty::HARD, AIDifficulty::INSANE };
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
	bool spectators;
} locked;
static bool spectatorHost = false;
static uint16_t defaultOpenSpectatorSlots = 0;

struct AIDATA
{
	AIDATA() : name{0}, js{0}, tip{0}, difficultyTips{0}, assigned(0) {}
	char name[MAX_LEN_AI_NAME];
	char js[MAX_LEN_AI_NAME];
	char tip[255 + 128];            ///< may contain optional AI tournament data
	char difficultyTips[5][255];    ///< optional difficulty level info
	int assigned;                   ///< How many AIs have we assigned of this type
};
static std::vector<AIDATA> aidata;

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
	std::shared_ptr<W_SCREEN> optionsOverlayScreen;

	void displayParagraphContextualMenu(const std::string& textToCopy, const std::shared_ptr<PlayerReference>& sender);
	std::shared_ptr<WIDGET> createParagraphContextualMenuPopoverForm(std::string textToCopy, std::shared_ptr<PlayerReference> sender);

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
		ASSERT_OR_RETURN(_("Commander"), NetPlay.players[player].ai < aidata.size(), "Invalid AI (index: %" PRIi8 ", num AIs: %zu)", NetPlay.players[player].ai, aidata.size());
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
	ASSERT_OR_RETURN(, psLevel->game >= 0 && psLevel->game < LEVEL_MAXFILES, "Invalid psLevel->game: %" PRIi16 " - may be a corrupt level load (%s; hash: %s)", psLevel->game, game.map, game.hash.toString().c_str());
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
	if (game.scavengers != NO_SCAVENGERS && myResponsibility(scavengerPlayer()))
	{
		debug(LOG_SAVE, "Loading scavenger AI for player %d", scavengerPlayer());
		loadPlayerScript("multiplay/script/scavengers/init.js", scavengerPlayer(), AIDifficulty::EASY);
	}

	// Reset resource path, otherwise things break down the line
	resForceBaseDir("");
}

static MAP_TILESET guessMapTilesetType(LEVEL_DATASET *psLevel)
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
		return MAP_TILESET::ARIZONA;
		break;
	case 2:
		return MAP_TILESET::URBAN;
		break;
	case 3:
		return MAP_TILESET::ROCKIES;
		break;
	}

	debug(LOG_MAP, "Could not guess map tileset, using ARIZONA.");
	return MAP_TILESET::ARIZONA;
}

static void loadEmptyMapPreview()
{
	// No map is available to preview, so improvise.
	iV_Image bckImage;
	if (!bckImage.allocate(BACKDROP_HACK_WIDTH, BACKDROP_HACK_HEIGHT, 3, true))
	{
		debug(LOG_ERROR, "Failed to allocate memory for map preview");
		return;
	}
	unsigned char *imageData = bckImage.bmp_w();
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

	screen_Upload(std::move(bckImage));
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
		debug(LOG_INFO, "Could not find level dataset \"%s\" %s. We %s waiting for a download.", game.map, game.hash.toString().c_str(), !NET_getDownloadingWzFiles().empty() ? "are" : "aren't");
		loadEmptyMapPreview();
		return;
	}
	if (psLevel->game < 0 || psLevel->game >= LEVEL_MAXFILES)
	{
		debug(LOG_ERROR, "apDataFiles index (%" PRIi16 ") is out of bounds for: \"%s\" %s.", psLevel->game, game.map, game.hash.toString().c_str());
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
	rebuildSearchPath(psLevel->dataDir, false, psLevel->realFileName, psLevel->customMountPoint);
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
	auto data = WzMap::Map::loadFromPath(aFileName, WzMap::MapType::SKIRMISH, psLevel->players, rand(), std::make_shared<WzMapDebugLogger>(), std::make_shared<WzMapPhysFSIO>());
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
	case MAP_TILESET::ARIZONA:
		previewColorScheme.tilesetColors = WzMap::TilesetColorScheme::TilesetArizona();
		break;
	case MAP_TILESET::URBAN:
		previewColorScheme.tilesetColors = WzMap::TilesetColorScheme::TilesetUrban();
		break;
	case MAP_TILESET::ROCKIES:
		previewColorScheme.tilesetColors = WzMap::TilesetColorScheme::TilesetRockies();
		break;
//	default:
//		debug(LOG_FATAL, "Invalid tileset type");
//		// silence warnings
//		abort();
//		return;
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
	iV_Image bckImage;
	if (!bckImage.allocate(BACKDROP_HACK_WIDTH, BACKDROP_HACK_HEIGHT, 3, true))
	{
		debug(LOG_FATAL, "Failed to allocate memory for map preview");
		abort();	// should be a fatal error ?
		return;
	}
	unsigned char *backdropData = bckImage.bmp_w();
	if (!backdropData)
	{
		debug(LOG_FATAL, "Out of memory for texture!");
		abort();	// should be a fatal error ?
		return;
	}
	ASSERT(mapPreviewResult->width <= BACKDROP_HACK_WIDTH, "mapData width somehow exceeds backdrop width?");
	memset(backdropData, 0, sizeof(char) * BACKDROP_HACK_WIDTH * BACKDROP_HACK_HEIGHT * 3); //dunno about background color
	unsigned char *imageData = mapPreviewResult->imageData.data();
	for (int y = 0; y < mapPreviewResult->height; ++y)
	{
		unsigned char *pSrc = imageData + (3 * (y * mapPreviewResult->width));
		unsigned char *pDst = backdropData + (3 * (y * BACKDROP_HACK_WIDTH));
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

	screen_Upload(std::move(bckImage));

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

		const char *difficultyKeys[] = { "supereasy_dummy_tip", "easy_tip", "medium_tip", "hard_tip", "insane_tip" };
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

		if (strcmp(file, "nexus.json") == 0)
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
	if (LobbyError != ERROR_INVALID)
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

static JoinGameResult joinGameInternal(std::vector<JoinConnectionDescription> connection_list, std::shared_ptr<WzTitleUI> oldUI, bool asSpectator = false);
static JoinGameResult joinGameInternalConnect(const char *host, uint32_t port, std::shared_ptr<WzTitleUI> oldUI, bool asSpectator = false);

JoinGameResult joinGame(const char *connectionString, bool asSpectator /*= false*/)
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
			return joinGame(serverIP.c_str(), serverPort, asSpectator);
		}
	}
	return joinGame(connectionString, 0, asSpectator);
}

JoinGameResult joinGame(const char *host, uint32_t port, bool asSpectator /*= false*/)
{
	std::string hostStr = (host != nullptr) ? std::string(host) : std::string();
	return joinGame(std::vector<JoinConnectionDescription>({JoinConnectionDescription(hostStr, port)}), asSpectator);
}

JoinGameResult joinGame(const std::vector<JoinConnectionDescription>& connection_list, bool asSpectator /*= false*/) {
	return joinGameInternal(connection_list, wzTitleUICurrent, asSpectator);
}

static JoinGameResult joinGameInternal(std::vector<JoinConnectionDescription> connection_list, std::shared_ptr<WzTitleUI> oldUI, bool asSpectator /*= false*/){

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
		JoinGameResult result = joinGameInternalConnect(connDesc.host.c_str(), connDesc.port, oldUI, asSpectator);
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
	changeTitleUI(std::make_shared<WzMsgBoxTitleUI>(WzString(_("Unable to join:")), WzString(_("Error while joining.")), wzTitleUICurrent));
	ActivityManager::instance().joinGameFailed(connection_list);
	return JoinGameResult::FAILED;
}

/**
 * Try connecting to the given host, show a password screen, the multiplayer screen or an error.
 * The reason for this having a third parameter is so that the password dialog
 *  doesn't turn into the parent of the next connection attempt.
 * Any other barriers/auth methods/whatever would presumably benefit in the same way.
 */
static JoinGameResult joinGameInternalConnect(const char *host, uint32_t port, std::shared_ptr<WzTitleUI> oldUI, bool asSpectator /*= false*/)
{
	// oldUI may get captured for use in the password dialog, among other things.
	PLAYERSTATS	playerStats;

	if (ingame.localJoiningInProgress)
	{
		return JoinGameResult::FAILED;
	}

	if (!NETjoinGame(host, port, (char *)sPlayer, asSpectator))	// join
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
						joinGameInternal({conn}, oldUI, asSpectator);
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

	if (selectedPlayer < MAX_PLAYERS && war_getMPcolour() >= 0)
	{
		SendColourRequest(selectedPlayer, war_getMPcolour());
	}

	return JoinGameResult::JOINED;
}

// ////////////////////////////////////////////////////////////////////////////
// Game Options Screen.

// ////////////////////////////////////////////////////////////////////////////

static std::shared_ptr<W_FORM> addInlineChooserBlueForm(const std::shared_ptr<W_SCREEN> &psScreen, W_FORM *psParent, UDWORD id, WzString txt, UDWORD x, UDWORD y, UDWORD w, UDWORD h)
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
	sFormInit.calcLayout = [x, y, psWeakParent](WIDGET *psWidget) {
		if (auto psParent = psWeakParent.lock())
		{
			psWidget->move(psParent->screenPosX() + x, psParent->screenPosY() + y);
		}
	};
	W_FORM *psForm = widgAddForm(psScreen, &sFormInit);
	ASSERT_OR_RETURN(nullptr, psForm != nullptr, "widgAddForm failed");
	return std::dynamic_pointer_cast<W_FORM>(psForm->shared_from_this());
}

static std::shared_ptr<W_FORM> createBlueForm(int x, int y, int w, int h, WIDGET_DISPLAY displayFunc /* = intDisplayFeBox*/)
{
	ASSERT(displayFunc != nullptr, "Must have a display func!");
	auto form = std::make_shared<W_FORM>();
	form->setGeometry(x, y, w, h);
	form->style = WFORM_PLAIN;
	form->displayFunction = displayFunc;
	return form;
}

static W_FORM * addBlueForm(UDWORD parent, UDWORD id, UDWORD x, UDWORD y, UDWORD w, UDWORD h, WIDGET_DISPLAY displayFunc /* = intDisplayFeBox*/)
{
	ASSERT(displayFunc != nullptr, "Must have a display func!");
	W_FORMINIT sFormInit;                  // draw options box.
	sFormInit.formID = parent;
	sFormInit.id	= id;
	sFormInit.x		= (UWORD) x;
	sFormInit.y		= (UWORD) y;
	sFormInit.style = WFORM_PLAIN;
	sFormInit.width = (UWORD)w;//190;
	sFormInit.height = (UWORD)h; //27;
	sFormInit.pDisplay =  displayFunc;
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

static void addMultiButton(std::shared_ptr<MultibuttonWidget> mbw, int value, AtlasImage image, AtlasImage imageDown, char const *tip)
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
		addMultiButton(scavengerChoice, ULTIMATE_SCAVENGERS, AtlasImage(FrontImages, IMAGE_SCAVENGERS_ULTIMATE_ON), AtlasImage(FrontImages, IMAGE_SCAVENGERS_ULTIMATE_ON_HI), _("Ultimate Scavengers"));
		addMultiButton(scavengerChoice, SCAVENGERS, AtlasImage(FrontImages, IMAGE_SCAVENGERS_ON), AtlasImage(FrontImages, IMAGE_SCAVENGERS_ON_HI), _("Scavengers"));
	}
	addMultiButton(scavengerChoice, NO_SCAVENGERS, AtlasImage(FrontImages, IMAGE_SCAVENGERS_OFF), AtlasImage(FrontImages, IMAGE_SCAVENGERS_OFF_HI), _("No Scavengers"));
	scavengerChoice->enable(!locked.scavengers);
	optionsList->addWidgetToLayout(scavengerChoice);

	auto allianceChoice = std::make_shared<MultichoiceWidget>(game.alliance);
	optionsList->attach(allianceChoice);
	allianceChoice->id = MULTIOP_ALLIANCES;
	allianceChoice->setLabel(_("Alliances"));
	addMultiButton(allianceChoice, NO_ALLIANCES, AtlasImage(FrontImages, IMAGE_NOALLI), AtlasImage(FrontImages, IMAGE_NOALLI_HI), _("No Alliances"));
	addMultiButton(allianceChoice, ALLIANCES, AtlasImage(FrontImages, IMAGE_ALLI), AtlasImage(FrontImages, IMAGE_ALLI_HI), _("Allow Alliances"));
	addMultiButton(allianceChoice, ALLIANCES_UNSHARED, AtlasImage(FrontImages, IMAGE_ALLI_UNSHARED), AtlasImage(FrontImages, IMAGE_ALLI_UNSHARED_HI), _("Locked Teams, No Shared Research"));
	addMultiButton(allianceChoice, ALLIANCES_TEAMS, AtlasImage(FrontImages, IMAGE_ALLI_TEAMS), AtlasImage(FrontImages, IMAGE_ALLI_TEAMS_HI), _("Locked Teams"));
	allianceChoice->enable(!locked.alliances);
	optionsList->addWidgetToLayout(allianceChoice);

	auto powerChoice = std::make_shared<MultichoiceWidget>(game.power);
	optionsList->attach(powerChoice);
	powerChoice->id = MULTIOP_POWER;
	powerChoice->setLabel(_("Power"));
	addMultiButton(powerChoice, LEV_LOW, AtlasImage(FrontImages, IMAGE_POWLO), AtlasImage(FrontImages, IMAGE_POWLO_HI), _("Low Power Levels"));
	addMultiButton(powerChoice, LEV_MED, AtlasImage(FrontImages, IMAGE_POWMED), AtlasImage(FrontImages, IMAGE_POWMED_HI), _("Medium Power Levels"));
	addMultiButton(powerChoice, LEV_HI, AtlasImage(FrontImages, IMAGE_POWHI), AtlasImage(FrontImages, IMAGE_POWHI_HI), _("High Power Levels"));
	powerChoice->enable(!locked.power);
	optionsList->addWidgetToLayout(powerChoice);

	auto baseTypeChoice = std::make_shared<MultichoiceWidget>(game.base);
	optionsList->attach(baseTypeChoice);
	baseTypeChoice->id = MULTIOP_BASETYPE;
	baseTypeChoice->setLabel(_("Base"));
	addMultiButton(baseTypeChoice, CAMP_CLEAN, AtlasImage(FrontImages, IMAGE_NOBASE), AtlasImage(FrontImages, IMAGE_NOBASE_HI), _("Start with No Bases"));
	addMultiButton(baseTypeChoice, CAMP_BASE, AtlasImage(FrontImages, IMAGE_SBASE), AtlasImage(FrontImages, IMAGE_SBASE_HI), _("Start with Bases"));
	addMultiButton(baseTypeChoice, CAMP_WALLS, AtlasImage(FrontImages, IMAGE_LBASE), AtlasImage(FrontImages, IMAGE_LBASE_HI), _("Start with Advanced Bases"));
	baseTypeChoice->enable(!locked.bases);
	optionsList->addWidgetToLayout(baseTypeChoice);

	auto mapPreviewButton = std::make_shared<MultibuttonWidget>();
	optionsList->attach(mapPreviewButton);
	mapPreviewButton->id = MULTIOP_MAP_PREVIEW;
	mapPreviewButton->setLabel(_("Map Preview"));
	addMultiButton(mapPreviewButton, 0, AtlasImage(FrontImages, IMAGE_FOG_OFF), AtlasImage(FrontImages, IMAGE_FOG_OFF_HI), _("Click to see Map"));
	optionsList->addWidgetToLayout(mapPreviewButton);

	/* Add additional controls if we are (or going to be) hosting the game */
	if (ingame.side == InGameSide::HOST_OR_SINGLEPLAYER)
	{
		auto structureLimitsLabel = challengeActive ? _("Show Structure Limits") : _("Set Structure Limits");
		auto structLimitsButton = std::make_shared<MultibuttonWidget>();
		optionsList->attach(structLimitsButton);
		structLimitsButton->id = MULTIOP_STRUCTLIMITS;
		structLimitsButton->setLabel(structureLimitsLabel);
		addMultiButton(structLimitsButton, 0, AtlasImage(FrontImages, IMAGE_SLIM), AtlasImage(FrontImages, IMAGE_SLIM_HI), structureLimitsLabel);
		optionsList->addWidgetToLayout(structLimitsButton);

		/* ...and even more controls if we are not starting a challenge */
		if (!challengeActive)
		{
			auto randomButton = std::make_shared<MultibuttonWidget>();
			optionsList->attach(randomButton);
			randomButton->id = MULTIOP_RANDOM;
			randomButton->setLabel(_("Random Game Options"));
			addMultiButton(randomButton, 0, AtlasImage(FrontImages, IMAGE_RELOAD), AtlasImage(FrontImages, IMAGE_RELOAD), _("Random Game Options\nCan be blocked by players' votes"));
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
				addMultiButton(TechnologyChoice, TECH_1, AtlasImage(FrontImages, IMAGE_TECHLO), AtlasImage(FrontImages, IMAGE_TECHLO_HI), _("Technology Level 1"));
				addMultiButton(TechnologyChoice, TECH_2, AtlasImage(FrontImages, IMAGE_TECHMED), AtlasImage(FrontImages, IMAGE_TECHMED_HI), _("Technology Level 2"));
				addMultiButton(TechnologyChoice, TECH_3, AtlasImage(FrontImages, IMAGE_TECHHI), AtlasImage(FrontImages, IMAGE_TECHHI_HI), _("Technology Level 3"));
				addMultiButton(TechnologyChoice, TECH_4, AtlasImage(FrontImages, IMAGE_COMPUTER_Y), AtlasImage(FrontImages, IMAGE_COMPUTER_Y_HI), _("Technology Level 4"));
				optionsList->addWidgetToLayout(TechnologyChoice);
			}
			/* If not hosting (yet), add the button for starting the host. */
			else
			{
				auto hostButton = std::make_shared<MultibuttonWidget>();
				optionsList->attach(hostButton);
				hostButton->id = MULTIOP_HOST;
				hostButton->setLabel(_("Start Hosting Game"));
				addMultiButton(hostButton, 0, AtlasImage(FrontImages, IMAGE_HOST), AtlasImage(FrontImages, IMAGE_HOST_HI), _("Start Hosting Game"));
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
	int minTeam = MAX_PLAYERS, maxTeam = 0;
	for (unsigned i = 0; i < game.maxPlayers; ++i)
	{
		if (i != except && (!NetPlay.players[i].isSpectator) && (NetPlay.players[i].allocated || NetPlay.players[i].ai >= 0))
		{
			int team = getPlayerTeam(i);
			minTeam = std::min(minTeam, team);
			maxTeam = std::max(maxTeam, team);
		}
	}
	if (minTeam == MAX_PLAYERS || minTeam == maxTeam)
	{
		return minTeam;  // Players all on same team.
	}
	return -1;  // Players not all on same team.
}

int WzMultiplayerOptionsTitleUI::playerRowY0(uint32_t row) const
{
	if (row >= playerRows.size())
	{
		return -1;
	}
	return playerRows[row]->y();
}

static int playerBoxHeight(uint32_t rowPosition)
{
	bool hasPlayersTabs = widgGetFromID(psWScreen, MULTIOP_PLAYERS_TABS) != nullptr;
	int playersTop = (hasPlayersTabs) ? MULTIOP_PLAYERS_TABS_H + 1 : 1;
	int gap = (MULTIOP_PLAYERSH - playersTop) - MULTIOP_TEAMSHEIGHT * numSlotsToBeDisplayed();
	int gapDiv = numSlotsToBeDisplayed() - 1;
	gap = std::min(gap, 5 * gapDiv);
	if (hasPlayersTabs)
	{
		gap = 0;
	}
	STATIC_ASSERT(MULTIOP_TEAMSHEIGHT == MULTIOP_PLAYERHEIGHT);  // Why are these different defines?
	return playersTop + (MULTIOP_TEAMSHEIGHT * gapDiv + gap) * rowPosition / gapDiv;
}

void WzMultiplayerOptionsTitleUI::closeAllChoosers()
{
	closeColourChooser();
	closeTeamChooser();

	// AiChooser, DifficultyChooser, and PositionChooser currently use the same form id, so to avoid a double-delete-later, do it once explicitly here
	widgDelete(psInlineChooserOverlayScreen, MULTIOP_AI_FORM);
	widgDelete(psInlineChooserOverlayScreen, FRONTEND_SIDETEXT2);
	aiChooserUp = -1;
	difficultyChooserUp = -1;
	playerSlotSwapChooserUp = nullopt;
	widgRemoveOverlayScreen(psInlineChooserOverlayScreen);
}

void WzMultiplayerOptionsTitleUI::initInlineChooser(uint32_t player)
{
	// remove any choosers already up
	closeAllChoosers();

	psInlineChooserOverlayScreen->screenSizeDidChange(screenWidth, screenHeight, screenWidth, screenHeight); // trigger a screenSizeDidChange so it can relayout (if screen size changed since last time it was registered as an overlay screen)
	widgRegisterOverlayScreen(psInlineChooserOverlayScreen, 1);
}

std::shared_ptr<IntFormAnimated> WzMultiplayerOptionsTitleUI::initRightSideChooser(const char* sideText)
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
	aiForm->setCalcLayout([psWeakParent](WIDGET *psWidget) {
		if (auto psParent = psWeakParent.lock())
		{
			psWidget->setGeometry(psParent->screenPosX() + MULTIOP_PLAYERSX, psParent->screenPosY() + MULTIOP_PLAYERSY, MULTIOP_PLAYERSW, MULTIOP_PLAYERSH);
		}
	});

	W_LABEL *psSideTextLabel = addSideText(psInlineChooserOverlayScreen, MULTIOP_INLINE_OVERLAY_ROOT_FRM, FRONTEND_SIDETEXT2, MULTIOP_PLAYERSX - 3, MULTIOP_PLAYERSY, sideText);
	if (psSideTextLabel)
	{
		psSideTextLabel->setCalcLayout([psWeakParent](WIDGET *psWidget) {
			if (auto psParent = psWeakParent.lock())
			{
				psWidget->setGeometry(psParent->screenPosX() + MULTIOP_PLAYERSX - 3, psParent->screenPosY() + MULTIOP_PLAYERSY, MULTIOP_PLAYERSW, MULTIOP_PLAYERSH);
			}
		});
	}

	return aiForm;
}

static std::shared_ptr<WzMultiButton> addMultiButWithClickHandler(const std::shared_ptr<WIDGET> &parent, UDWORD id, UDWORD x, UDWORD y, UDWORD width, UDWORD height, const char *tipres, UDWORD norm, UDWORD down, UDWORD hi, const W_BUTTON::W_BUTTON_ONCLICK_FUNC& clickHandler, unsigned tc = MAX_PLAYERS)
{
	ASSERT_OR_RETURN(nullptr, parent != nullptr, "Null parent");
	auto psButton = std::dynamic_pointer_cast<WzMultiButton>(addMultiBut(*(parent.get()), id, x, y, width, height, tipres, norm, down, hi, tc));
	if (!psButton)
	{
		return nullptr;
	}
	psButton->addOnClickHandler(clickHandler);
	return psButton;
}

void WzMultiplayerOptionsTitleUI::openDifficultyChooser(uint32_t player)
{
	std::shared_ptr<IntFormAnimated> aiForm = initRightSideChooser(_("DIFFICULTY"));
	if (!aiForm)
	{
		debug(LOG_ERROR, "Failed to initialize right-side chooser?");
		return;
	}

	auto psWeakTitleUI = std::weak_ptr<WzMultiplayerOptionsTitleUI>(std::dynamic_pointer_cast<WzMultiplayerOptionsTitleUI>(shared_from_this()));

	for (int difficultyIdx = static_cast<int>(AIDifficulty::EASY); difficultyIdx < static_cast<int>(AIDifficulty::INSANE) + 1; ++difficultyIdx)
	{
		auto onClickHandler = [psWeakTitleUI, difficultyIdx, player](W_BUTTON& clickedButton) {
			auto pStrongPtr = psWeakTitleUI.lock();
			ASSERT_OR_RETURN(, pStrongPtr.operator bool(), "WzMultiplayerOptionsTitleUI no longer exists");
			NetPlay.players[player].difficulty = difficultyValue[difficultyIdx];
			NETBroadcastPlayerInfo(player);
			resetReadyStatus(false);
			widgScheduleTask([pStrongPtr] {
				pStrongPtr->closeDifficultyChooser();
				pStrongPtr->addPlayerBox(true);
			});
		};

		W_BUTINIT sButInit;
		auto pDifficultyRow = std::make_shared<W_BUTTON>();
		pDifficultyRow->id = 0; //MULTIOP_DIFFICULTY_CHOOSE_START + difficultyIdx;
		pDifficultyRow->setGeometry(7, (MULTIOP_PLAYERHEIGHT + 5) * (difficultyIdx - 1) + 4, MULTIOP_PLAYERWIDTH + 1, MULTIOP_PLAYERHEIGHT);
		std::string tipStr;
		switch (difficultyIdx)
		{
		case static_cast<int>(AIDifficulty::EASY): tipStr = _("Starts disadvantaged"); break;
		case static_cast<int>(AIDifficulty::MEDIUM): tipStr = _("Plays nice"); break;
		case static_cast<int>(AIDifficulty::HARD): tipStr = _("No holds barred"); break;
		case static_cast<int>(AIDifficulty::INSANE): tipStr = _("Starts with advantages"); break;
		}
		if (NetPlay.players[player].ai < aidata.size())
		{
			const char *difficultyTip = aidata[NetPlay.players[player].ai].difficultyTips[difficultyIdx];
			if (strcmp(difficultyTip, "") != 0)
			{
				tipStr += "\n";
				tipStr += difficultyTip;
			}
		}
		else
		{
			ASSERT(false, "Invalid AI (index: %" PRIi8 ", num AIs: %zu)", NetPlay.players[player].ai, aidata.size());
		}
		pDifficultyRow->setTip(tipStr);
		pDifficultyRow->displayFunction = displayDifficulty;
		pDifficultyRow->UserData = difficultyIdx;
		pDifficultyRow->pUserData = new DisplayDifficultyCache();
		pDifficultyRow->setOnDelete([](WIDGET *psWidget) {
			assert(psWidget->pUserData != nullptr);
			delete static_cast<DisplayDifficultyCache *>(psWidget->pUserData);
			psWidget->pUserData = nullptr;
		});
		aiForm->attach(pDifficultyRow);
		pDifficultyRow->addOnClickHandler(onClickHandler);
	}

	difficultyChooserUp = player;
}

void WzMultiplayerOptionsTitleUI::openAiChooser(uint32_t player)
{
	std::shared_ptr<IntFormAnimated> aiForm = initRightSideChooser(_("CHOOSE AI"));
	if (!aiForm)
	{
		debug(LOG_ERROR, "Failed to initialize right-side chooser?");
		return;
	}

	bool spectatorSlotSupported = spectatorSlotsSupported() && isSpectatorOnlySlot(player);
	bool openSlotSupported = (NetPlay.bComms) && !isSpectatorOnlySlot(player);
	bool aiSlotSupported = !isSpectatorOnlySlot(player);

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
			NetPlay.players[player].isSpectator = false;
			break;
		case MULTIOP_AI_OPEN:
			NetPlay.players[player].ai = AI_OPEN;
			NetPlay.players[player].isSpectator = false;
			break;
		case MULTIOP_AI_SPECTATOR:
			// set slot to open
			NetPlay.players[player].ai = AI_OPEN;
			// but also a spectator
			NetPlay.players[player].isSpectator = true;
			break;
		default:
			debug(LOG_ERROR, "Unexpected button id");
			return;
			break;
		}

		// common code
		NetPlay.players[player].difficulty = AIDifficulty::DISABLED; // disable AI for this slot
		NETBroadcastPlayerInfo(player);
		resetReadyStatus(false);
		widgScheduleTask([pStrongPtr] {
			pStrongPtr->closeAiChooser();
			pStrongPtr->addPlayerBox(true);
		});
		ActivityManager::instance().updateMultiplayGameData(game, ingame, NETGameIsLocked());
	};

	const int topMostYPos = 3;

	// Open button
	if (openSlotSupported)
	{
		sButInit.id = MULTIOP_AI_OPEN;
		sButInit.pTip = _("Allow human players to join in this slot");
		sButInit.UserData = (UDWORD)AI_OPEN;
		sButInit.y = topMostYPos;	//Top most position
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
		sButInit.y = topMostYPos; //since we don't have the lone mpbutton, we can start at position 0
	}
	auto psCloseButton = widgAddButton(psInlineChooserOverlayScreen, &sButInit);
	if (psCloseButton)
	{
		psCloseButton->addOnClickHandler(openCloseOnClickHandler);
	}

	// Spectator button
	if (spectatorSlotSupported)
	{
		sButInit.pTip = _("Allow spectators to join in this slot");
		sButInit.id = MULTIOP_AI_SPECTATOR;
		sButInit.UserData = (UDWORD)AI_OPEN;
		sButInit.y = sButInit.y + sButInit.height;
		auto psSpectatorButton = widgAddButton(psInlineChooserOverlayScreen, &sButInit);
		if (psSpectatorButton)
		{
			psSpectatorButton->addOnClickHandler(openCloseOnClickHandler);
		}
	}

	if (aiSlotSupported)
	{
		auto pAIScrollableList = ScrollableListWidget::make();
		aiForm->attach(pAIScrollableList);
		pAIScrollableList->setBackgroundColor(WZCOL_TRANSPARENT_BOX);
		int aiListStartXPos = sButInit.x;
		int aiListStartYPos = (sButInit.height + sButInit.y) + 10;
		int aiListEntryHeight = sButInit.height;
		int aiListEntryWidth = sButInit.width;
		int aiListHeight = aiListEntryHeight * (NetPlay.bComms ? 7 : 8);
		pAIScrollableList->setCalcLayout([aiListStartXPos, aiListStartYPos, aiListEntryWidth, aiListHeight](WIDGET *psWidget) {
			psWidget->setGeometry(aiListStartXPos, aiListStartYPos, aiListEntryWidth, aiListHeight);
		});

		W_BUTINIT emptyInit;
		for (size_t aiIdx = 0; aiIdx < aidata.size(); aiIdx++)
		{
			auto pAIRow = std::make_shared<W_BUTTON>(&emptyInit);
			pAIRow->setTip(aidata[aiIdx].tip);
			pAIRow->id = 0; //MULTIOP_AI_START + aiIdx;
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
				NetPlay.players[player].isSpectator = false;
				NetPlay.players[player].ai = aiIdx;
				setPlayerName(player, getAIName(player));
				NetPlay.players[player].difficulty = AIDifficulty::MEDIUM;
				NETBroadcastPlayerInfo(player);
				resetReadyStatus(false);
				widgScheduleTask([pStrongPtr] {
					pStrongPtr->closeAiChooser();
					pStrongPtr->addPlayerBox(true);
				});
				ActivityManager::instance().updateMultiplayGameData(game, ingame, NETGameIsLocked());
			});
			pAIScrollableList->addItem(pAIRow);
		}
	}

	aiChooserUp = player;
}

class WzPlayerSelectPositionRowFactory
{
public:
	virtual ~WzPlayerSelectPositionRowFactory() { }
	virtual std::shared_ptr<W_BUTTON> make(uint32_t switcherPlayerIdx, uint32_t targetPlayerIdx, const std::shared_ptr<WzMultiplayerOptionsTitleUI>& parent) const = 0;
};

class WzPlayerSelectionChangePositionRow : public W_BUTTON
{
protected:
	WzPlayerSelectionChangePositionRow(uint32_t targetPlayerIdx)
	: W_BUTTON()
	, targetPlayerIdx(targetPlayerIdx)
	{ }
public:
	static std::shared_ptr<WzPlayerSelectionChangePositionRow> make(uint32_t switcherPlayerIdx, uint32_t targetPlayerIdx, const std::shared_ptr<WzMultiplayerOptionsTitleUI>& parent)
	{
		class make_shared_enabler: public WzPlayerSelectionChangePositionRow {
		public:
			make_shared_enabler(uint32_t targetPlayerIdx) : WzPlayerSelectionChangePositionRow(targetPlayerIdx) { }
		};
		auto widget = std::make_shared<make_shared_enabler>(targetPlayerIdx);
		widget->setTip(_("Click to change to this slot"));

		std::weak_ptr<WzMultiplayerOptionsTitleUI> titleUI(parent);
		widget->addOnClickHandler([switcherPlayerIdx, titleUI](W_BUTTON& button){
			auto selectPositionRow = std::dynamic_pointer_cast<WzPlayerSelectionChangePositionRow>(button.shared_from_this());
			ASSERT_OR_RETURN(, selectPositionRow != nullptr, "Wrong widget type");
			auto strongTitleUI = titleUI.lock();
			ASSERT_OR_RETURN(, strongTitleUI != nullptr, "Title UI is gone?");
			// Switch player
			resetReadyStatus(false);		// will reset only locally if not a host
			SendPositionRequest(switcherPlayerIdx, NetPlay.players[selectPositionRow->targetPlayerIdx].position);
			widgScheduleTask([strongTitleUI] {
				strongTitleUI->closePositionChooser();
				strongTitleUI->updatePlayers();
			});
		});

		return widget;
	}

	void display(int xOffset, int yOffset) override
	{
		const int x0 = xOffset + x();
		const int y0 = yOffset + y();
		char text[80];

		drawBlueBox(x0, y0, width(), height());
		ssprintf(text, _("Click to take player slot %u"), static_cast<unsigned>(NetPlay.players[targetPlayerIdx].position));
		cache.wzPositionText.setText(text, font_regular);
		cache.wzPositionText.render(x0 + 10, y0 + 22, WZCOL_FORM_TEXT);
	}
public:
	uint32_t targetPlayerIdx;
	DisplayPositionCache cache;
};

class WzPlayerSelectionChangePositionRowFactory : public WzPlayerSelectPositionRowFactory
{
public:
	~WzPlayerSelectionChangePositionRowFactory() { }
	virtual std::shared_ptr<W_BUTTON> make(uint32_t switcherPlayerIdx, uint32_t targetPlayerIdx, const std::shared_ptr<WzMultiplayerOptionsTitleUI>& parent) const override
	{
		return WzPlayerSelectionChangePositionRow::make(switcherPlayerIdx, targetPlayerIdx, parent);
	}
};

class WzPlayerIndexSwapPositionRow : public W_BUTTON
{
protected:
	WzPlayerIndexSwapPositionRow(uint32_t targetPlayerIdx)
	: W_BUTTON()
	, targetPlayerIdx(targetPlayerIdx)
	{ }
public:
	static std::shared_ptr<WzPlayerIndexSwapPositionRow> make(uint32_t switcherPlayerIdx, uint32_t targetPlayerIdx, const std::shared_ptr<WzMultiplayerOptionsTitleUI>& parent)
	{
		class make_shared_enabler: public WzPlayerIndexSwapPositionRow {
		public:
			make_shared_enabler(uint32_t targetPlayerIdx) : WzPlayerIndexSwapPositionRow(targetPlayerIdx) { }
		};

		auto widget = std::make_shared<make_shared_enabler>(targetPlayerIdx);
		if (targetPlayerIdx != NetPlay.hostPlayer)
		{
			widget->setTip(_("Click to swap player to this slot"));

			std::weak_ptr<WzMultiplayerOptionsTitleUI> titleUI(parent);
			widget->addOnClickHandler([switcherPlayerIdx, titleUI](W_BUTTON& button){
				auto selectPositionRow = std::dynamic_pointer_cast<WzPlayerIndexSwapPositionRow>(button.shared_from_this());
				ASSERT_OR_RETURN(, selectPositionRow != nullptr, "Wrong widget type");
				auto strongTitleUI = titleUI.lock();
				ASSERT_OR_RETURN(, strongTitleUI != nullptr, "Title UI is gone?");

				std::string playerName = getPlayerName(switcherPlayerIdx);

				NETmoveSpectatorToPlayerSlot(switcherPlayerIdx, selectPositionRow->targetPlayerIdx, true);
				resetReadyStatus(true);		//reset and send notification to all clients
				widgScheduleTask([strongTitleUI] {
					strongTitleUI->closePlayerSlotSwapChooser();
					strongTitleUI->updatePlayers();
				});
				std::string msg = astringf(_("Spectator %s has moved to Players"), playerName.c_str());
				sendRoomSystemMessage(msg.c_str());
			});
		}
		else
		{
			widget->setTip(_("Cannot swap with host"));
			widget->setState(WBUT_DISABLE);
		}

		return widget;
	}

	void display(int xOffset, int yOffset) override
	{
		const int x0 = xOffset + x();
		const int y0 = yOffset + y();

		bool isDisabled = (getState() & WBUT_DISABLE) != 0;

		if (isDisabled)
		{
			pie_UniTransBoxFill(x0, y0, x0 + width(), y0 + height(), pal_RGBA(0, 0, 0, 125));
			return;
		}

		iV_Box(x0, y0, x0 + width(), y0 + height(), WZCOL_MENU_BORDER);
	}
public:
	uint32_t targetPlayerIdx;
};

class WzPlayerIndexSwapPositionRowRactory : public WzPlayerSelectPositionRowFactory
{
public:
	~WzPlayerIndexSwapPositionRowRactory() { }
	virtual std::shared_ptr<W_BUTTON> make(uint32_t switcherPlayerIdx, uint32_t targetPlayerIdx, const std::shared_ptr<WzMultiplayerOptionsTitleUI>& parent) const override
	{
		return WzPlayerIndexSwapPositionRow::make(switcherPlayerIdx, targetPlayerIdx, parent);
	}
};

#include <set>

static std::set<uint32_t> validPlayerIdxTargetsForPlayerPositionMove(uint32_t player)
{
	std::set<uint32_t> validTargetPlayerIdx;
	for (uint32_t i = 0; i < game.maxPlayers; i++)
	{
		if (player != i
			&& (NetPlay.isHost || !isHumanPlayer(i)) // host can move a player to any slot, player can only move to empty slots
			&& !isSpectatorOnlySlot(i)) // target cannot be a spectator only slot (for player position changes)
		{
			validTargetPlayerIdx.insert(i);
		}
	}
	return validTargetPlayerIdx;
}

class WzPositionChooser : public WIDGET
{
protected:
	WzPositionChooser(uint32_t switcherPlayerIdx, const std::shared_ptr<WzMultiplayerOptionsTitleUI> &parentTitleUI)
	: WIDGET()
	, switcherPlayerIdx(switcherPlayerIdx)
	, psWeakTitleUI(parentTitleUI)
	{ }
public:
	static std::shared_ptr<WzPositionChooser> make(uint32_t switcherPlayerIdx, const WzPlayerSelectPositionRowFactory& rowFactory, const std::shared_ptr<WzMultiplayerOptionsTitleUI> &parent)
	{
		class make_shared_enabler: public WzPositionChooser {
		public:
			make_shared_enabler(uint32_t switcherPlayerIdx, const std::shared_ptr<WzMultiplayerOptionsTitleUI> &parentTitleUI) : WzPositionChooser(switcherPlayerIdx, parentTitleUI) { }
		};
		auto widget = std::make_shared<make_shared_enabler>(switcherPlayerIdx, parent);

		// Create WzPlayerSelectPositionRow all player positions
		for (uint32_t i = 0; i < game.maxPlayers; i++)
		{
			auto playerPositionRow = rowFactory.make(switcherPlayerIdx, i, parent);
			widget->attach(playerPositionRow);
			widget->positionSelectionRows.push_back(PlayerButtonRow(playerPositionRow, i));
		}
		widget->positionRows();

		// Position each in the appropriate spot
		return widget;
	}

	void clicked(W_CONTEXT *, WIDGET_KEY key) override
	{
		auto psWeakTitleUICopy = psWeakTitleUI;
		widgScheduleTask([psWeakTitleUICopy]{
			auto psTitleUI = psWeakTitleUICopy.lock();
			ASSERT_OR_RETURN(, psTitleUI != nullptr, "Title UI is null");
			psTitleUI->closeAllChoosers(); // this also removes the overlay screen
			psTitleUI->updatePlayers();
		});
	}

	void geometryChanged() override
	{
		positionRows();
	}

	void run(W_CONTEXT *psContext) override
	{
		positionRows();
	}

	void positionRows()
	{
		auto strongTitleUI = psWeakTitleUI.lock();
		ASSERT_OR_RETURN(, strongTitleUI != nullptr, "No valid TitleUI pointer");
		auto validPlayerIdxRowsToDisplay = validPlayerIdxTargetsForPlayerPositionMove(switcherPlayerIdx);
		for (auto& row : positionSelectionRows)
		{
			if (validPlayerIdxRowsToDisplay.count(row.targetPlayerIdx) == 0)
			{
				row.widget->hide();
				continue;
			}
			int newWidth = width();
			row.widget->setGeometry(0, std::max(strongTitleUI->playerRowY0(row.targetPlayerIdx), 0), newWidth, MULTIOP_PLAYERHEIGHT);
			row.widget->show();
		}
	}
private:
	uint32_t switcherPlayerIdx;
	std::weak_ptr<WzMultiplayerOptionsTitleUI> psWeakTitleUI;
	struct PlayerButtonRow
	{
		PlayerButtonRow(const std::shared_ptr<W_BUTTON>& widget, uint32_t targetPlayerIdx)
		: widget(widget)
		, targetPlayerIdx(targetPlayerIdx)
		{ }
		std::shared_ptr<W_BUTTON> widget;
		uint32_t targetPlayerIdx = 0;
	};
	std::vector<PlayerButtonRow> positionSelectionRows;
};

void WzMultiplayerOptionsTitleUI::openPositionChooser(uint32_t player)
{
	// remove any choosers already up
	closeAllChoosers();

	widgRegisterOverlayScreen(psInlineChooserOverlayScreen, 1);

	WIDGET* psParent = widgGetFromID(psWScreen, FRONTEND_BACKDROP);
	ASSERT_OR_RETURN(, psParent != nullptr, "Could not find root form");
	std::weak_ptr<WIDGET> psWeakParent(psParent->shared_from_this());

	WIDGET *chooserParent = widgGetFromID(psInlineChooserOverlayScreen, MULTIOP_INLINE_OVERLAY_ROOT_FRM);

	WzPlayerSelectionChangePositionRowFactory rowFactory;
	auto positionChooser = WzPositionChooser::make(player, rowFactory, std::dynamic_pointer_cast<WzMultiplayerOptionsTitleUI>(shared_from_this()));
	chooserParent->attach(positionChooser);
	positionChooser->id = MULTIOP_AI_FORM;
	positionChooser->setCalcLayout([psWeakParent](WIDGET *psWidget) {
		if (auto psParent = psWeakParent.lock())
		{
			psWidget->setGeometry(psParent->screenPosX() + MULTIOP_PLAYERSX, psParent->screenPosY() + MULTIOP_PLAYERSY, MULTIOP_PLAYERSW, MULTIOP_PLAYERSH);
		}
	});
}

void WzMultiplayerOptionsTitleUI::updatePlayers()
{
	addPlayerBox(true);
}

static bool SendTeamRequest(UBYTE player, UBYTE chosenTeam); // forward-declare

void WzMultiplayerOptionsTitleUI::openTeamChooser(uint32_t player)
{
	ASSERT_OR_RETURN(, player < MAX_CONNECTED_PLAYERS, "Invalid player: %" PRIu32 "", player);

	UDWORD i;
	int disallow = allPlayersOnSameTeam(player);
	SpectatorInfo currSpectatorInfo = NETGameGetSpectatorInfo();

	bool isSpectator = NetPlay.players[player].isSpectator;
	bool canChangeTeams = !locked.teams && !isSpectator && alliancesSetTeamsBeforeGame(game.alliance);
	bool canKickPlayer = player != selectedPlayer && NetPlay.bComms && NetPlay.isHost && NetPlay.players[player].allocated;
	bool canChangeSpectatorStatus = !locked.spectators && NetPlay.bComms && isHumanPlayer(player) && (player != NetPlay.hostPlayer) && (NetPlay.isHost || player == selectedPlayer);
	bool displayMoveToSpectatorsButton = canChangeSpectatorStatus && !isSpectator && (currSpectatorInfo.availableSpectatorSlots() > 0 || (NetPlay.isHost && NETcanOpenNewSpectatorSlot()));
	bool displayMoveToPlayersButton = canChangeSpectatorStatus && isSpectator;
	canChangeSpectatorStatus = canChangeSpectatorStatus && (displayMoveToSpectatorsButton || displayMoveToPlayersButton);

	if (!canChangeTeams && !canKickPlayer && !canChangeSpectatorStatus)
	{
		// can't change teams / spectator status because player is a spectator and spectators are locked
		return;
	}

	debug(LOG_NET, "Opened team chooser for %d, current team: %d", player, NetPlay.players[player].team);

	initInlineChooser(player);

	// add form.
//	addBlueForm(MULTIOP_PLAYERS, MULTIOP_TEAMCHOOSER_FORM, "", 8, playerBoxHeight(player), MULTIOP_ROW_WIDTH, MULTIOP_TEAMSHEIGHT);
	auto psParentForm = (W_FORM *)widgGetFromID(psWScreen, MULTIOP_PLAYERS);
	auto psInlineChooserForm = addInlineChooserBlueForm(psInlineChooserOverlayScreen, psParentForm, MULTIOP_TEAMCHOOSER_FORM, "", PLAYERBOX_X0 + 1, std::max(playerRowY0(player), 0), MULTIOP_ROW_WIDTH, MULTIOP_TEAMSHEIGHT);
	ASSERT(psInlineChooserForm != nullptr, "Failed to create form");

	auto psWeakTitleUI = std::weak_ptr<WzMultiplayerOptionsTitleUI>(std::dynamic_pointer_cast<WzMultiplayerOptionsTitleUI>(shared_from_this()));

	if (canChangeTeams)
	{
		int teamW = iV_GetImageWidth(FrontImages, IMAGE_TEAM0);
		int teamH = iV_GetImageHeight(FrontImages, IMAGE_TEAM0);
		int space = (MULTIOP_ROW_WIDTH - 52) - 4 - teamW * (game.maxPlayers + 1);
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

				widgScheduleTask([pStrongPtr] {
					pStrongPtr->closeTeamChooser();
					// restore player list
					pStrongPtr->addPlayerBox(true);
				});
			}
		};

		// add the teams, skipping the one we CAN'T be on (if applicable)
		for (i = 0; i < game.maxPlayers; i++)
		{
			if (i != disallow)
			{
				addMultiButWithClickHandler(psInlineChooserForm, MULTIOP_TEAMCHOOSER + i, i * (teamW * spaceDiv + space) / spaceDiv + 3, 6, teamW, teamH, _("Team"), IMAGE_TEAM0 + i, IMAGE_TEAM0_HI + i, IMAGE_TEAM0_HI + i, onClickHandler);
			}
			// may want to add some kind of 'can't do' icon instead of being blank?
		}
	}
	else if (isSpectator)
	{
  		const int imgwidth = iV_GetImageWidth(FrontImages, IMAGE_SPECTATOR);
  		const int imgheight = iV_GetImageHeight(FrontImages, IMAGE_SPECTATOR);
  		addMultiBut(psInlineChooserOverlayScreen, MULTIOP_TEAMCHOOSER_FORM, MULTIOP_TEAMCHOOSER_SPECTATOR, 3, 6, imgwidth, imgheight,
  		_("Spectator"), IMAGE_SPECTATOR, IMAGE_SPECTATOR_HI, IMAGE_SPECTATOR_HI);
  	}

	// add kick & ban buttons
	int kickImageX = MULTIOP_ROW_WIDTH;
	if (canKickPlayer)
	{
		// Add "kick" button
		auto onClickHandler = [player, psWeakTitleUI](W_BUTTON &button) {
			auto pStrongPtr = psWeakTitleUI.lock();
			ASSERT_OR_RETURN(, pStrongPtr.operator bool(), "WzMultiplayerOptionsTitleUI no longer exists");

			std::string msg = astringf(_("The host has kicked %s from the game!"), getPlayerName(player));
			kickPlayer(player, _("The host has kicked you from the game."), ERROR_KICKED, false);
			sendRoomSystemMessage(msg.c_str());
			resetReadyStatus(true);		//reset and send notification to all clients
			widgScheduleTask([pStrongPtr] {
				pStrongPtr->closeTeamChooser();
			});
		};

		const int imgwidth = iV_GetImageWidth(FrontImages, IMAGE_NOJOIN);
		const int imgheight = iV_GetImageHeight(FrontImages, IMAGE_NOJOIN);
		kickImageX = MULTIOP_ROW_WIDTH - imgwidth - 4;
		addMultiButWithClickHandler(psInlineChooserForm, MULTIOP_TEAMCHOOSER_KICK, kickImageX, 8, imgwidth, imgheight,
			("Kick player"), IMAGE_NOJOIN, IMAGE_NOJOIN, IMAGE_NOJOIN, onClickHandler);

		// Add "ban" button
		auto banOnClickHandler = [player, psWeakTitleUI](W_BUTTON &button) {
			auto pStrongPtr = psWeakTitleUI.lock();
			ASSERT_OR_RETURN(, pStrongPtr.operator bool(), "WzMultiplayerOptionsTitleUI no longer exists");

			std::string msg = astringf(_("The host has banned %s from the game!"), getPlayerName(player));
			kickPlayer(player, _("The host has banned you from the game."), ERROR_KICKED, true);
			sendRoomSystemMessage(msg.c_str());
			resetReadyStatus(true);		//reset and send notification to all clients
			widgScheduleTask([pStrongPtr] {
				pStrongPtr->closeTeamChooser();
			});
		};

		const int imgwidth_ban = iV_GetImageWidth(FrontImages, IMAGE_NOJOIN_FULL);
		const int imgheight_ban = iV_GetImageHeight(FrontImages, IMAGE_NOJOIN_FULL);
		kickImageX = kickImageX - imgwidth - 4;
		addMultiButWithClickHandler(psInlineChooserForm, MULTIOP_TEAMCHOOSER_BAN, kickImageX, 8, imgwidth_ban, imgheight_ban,
			("Ban player"), IMAGE_NOJOIN_FULL, IMAGE_NOJOIN_FULL, IMAGE_NOJOIN_FULL, onClickHandler);
	}

	if (canChangeSpectatorStatus)
	{
		// Add a "make spectator" button (if there are available spectator slots, and this is a player)
		if (displayMoveToSpectatorsButton)
		{
			const int imgwidth_spec = iV_GetImageWidth(FrontImages, IMAGE_SPECTATOR);
			const int imgheight_spec = iV_GetImageHeight(FrontImages, IMAGE_SPECTATOR);

			W_BUTTON::W_BUTTON_ONCLICK_FUNC onSpecClickHandler;

			if (NetPlay.isHost)
			{
				onSpecClickHandler = [player, psWeakTitleUI](W_BUTTON &button) {
					auto pStrongPtr = psWeakTitleUI.lock();
					ASSERT_OR_RETURN(, pStrongPtr.operator bool(), "WzMultiplayerOptionsTitleUI no longer exists");

					std::string playerName = getPlayerName(player);

					if (!NETmovePlayerToSpectatorOnlySlot(player, true))
					{
						std::string msg = astringf(_("Failed to move %s to Spectators"), playerName.c_str());
						sendRoomSystemMessageToSingleReceiver(msg.c_str(), selectedPlayer);
						return;
					}

					std::string msg = astringf(_("The host has moved %s to Spectators!"), playerName.c_str());
					sendRoomSystemMessage(msg.c_str());
					resetReadyStatus(true);		//reset and send notification to all clients
					widgScheduleTask([pStrongPtr] {
						pStrongPtr->closeTeamChooser();
					});
				};
			}
			else
			{
				onSpecClickHandler = [player, psWeakTitleUI](W_BUTTON &button) {
					auto pStrongPtr = psWeakTitleUI.lock();
					ASSERT_OR_RETURN(, pStrongPtr.operator bool(), "WzMultiplayerOptionsTitleUI no longer exists");

					// Send net message to host asking to be a spectator, as host is responsible for these changes
					SendPlayerSlotTypeRequest(player, true);
					widgScheduleTask([pStrongPtr] {
						pStrongPtr->closeTeamChooser();
					});
				};
			}

			addMultiButWithClickHandler(psInlineChooserForm, MULTIOP_TEAMCHOOSER_SPECTATOR, kickImageX - imgwidth_spec - 4, 6, imgwidth_spec, imgheight_spec,
			_("Move to Spectators"), IMAGE_SPECTATOR, IMAGE_SPECTATOR_HI, IMAGE_SPECTATOR_HI, onSpecClickHandler);
		}
		else if (displayMoveToPlayersButton)
		{
			const int imgwidth_spec = iV_GetImageWidth(FrontImages, IMAGE_EDIT_PLAYER);
			const int imgheight_spec = iV_GetImageHeight(FrontImages, IMAGE_EDIT_PLAYER);

			W_BUTTON::W_BUTTON_ONCLICK_FUNC onSpecClickHandler;
			std::string toolTip;
			if (NetPlay.isHost)
			{
				onSpecClickHandler = [player, psWeakTitleUI](W_BUTTON &button) {
					auto pStrongPtr = psWeakTitleUI.lock();
					ASSERT_OR_RETURN(, pStrongPtr.operator bool(), "WzMultiplayerOptionsTitleUI no longer exists");

					// Ask the spectator if they are okay with a move from spectator -> player?
					SendPlayerSlotTypeRequest(player, false);
					widgScheduleTask([pStrongPtr] {
						pStrongPtr->closeTeamChooser();
					});
				};
				toolTip = _("Ask Spectator to Play");
			}
			else
			{
				// Ask the host for a move from spectator -> player
				onSpecClickHandler = [player, psWeakTitleUI](W_BUTTON &button) {
					auto pStrongPtr = psWeakTitleUI.lock();
					ASSERT_OR_RETURN(, pStrongPtr.operator bool(), "WzMultiplayerOptionsTitleUI no longer exists");

					// Send net message to host asking to be a player, as host is responsible for these changes
					SendPlayerSlotTypeRequest(player, false);

					widgScheduleTask([pStrongPtr] {
						pStrongPtr->closeTeamChooser();
					});
				};
				toolTip = _("Ask to Play");
			}

			auto psButton = addMultiButWithClickHandler(psInlineChooserForm, MULTIOP_TEAMCHOOSER_SPECTATOR, kickImageX - imgwidth_spec - 4, 5, imgwidth_spec, imgheight_spec,
				toolTip.c_str(), IMAGE_EDIT_PLAYER, IMAGE_EDIT_PLAYER_HI, IMAGE_EDIT_PLAYER_HI, onSpecClickHandler);
			ASSERT(psButton != nullptr, "Null button??");
			psButton->drawBlueBorder = false; // HACK: to disable the other hack in WzMultiButton::display() for IMAGE_EDIT_PLAYER (sigh...)
		}
	}

	inlineChooserUp = player;
}

void WzMultiplayerOptionsTitleUI::openColourChooser(uint32_t player)
{
	ASSERT_OR_RETURN(, player < MAX_PLAYERS, "Invalid player number");
	initInlineChooser(player);

	// add form.
	auto psParentForm = (W_FORM *)widgGetFromID(psWScreen, MULTIOP_PLAYERS);
	auto psInlineChooserForm = addInlineChooserBlueForm(psInlineChooserOverlayScreen, psParentForm, MULTIOP_COLCHOOSER_FORM, "",
		PLAYERBOX_X0, std::max(playerRowY0(player), 0),
		MULTIOP_ROW_WIDTH, MULTIOP_PLAYERHEIGHT*2-8);
	ASSERT(psInlineChooserForm != nullptr, "Failed to create form");

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
				widgScheduleTask([pStrongPtr] {
					pStrongPtr->closeColourChooser();
					pStrongPtr->addPlayerBox(true);
				});
			}
		};

		addMultiButWithClickHandler(psInlineChooserForm, MULTIOP_COLCHOOSER + getPlayerColour(i),
			i * (flagW * spaceDiv + space) / spaceDiv + 4, 4, // x, y
			flagW, flagH,  // w, h
			getPlayerColourName(i), IMAGE_PLAYERN, IMAGE_PLAYERN_HI, IMAGE_PLAYERN_HI, onClickHandler, getPlayerColour(i)
		);

		if (!safeToUseColour(selectedPlayer, i))
		{
			widgSetButtonState(psInlineChooserOverlayScreen, MULTIOP_COLCHOOSER + getPlayerColour(i), WBUT_DISABLE);
		}
	}

	int facW = iV_GetImageWidth(FrontImages, IMAGE_FACTION_NORMAL) + 4;
	int facH = iV_GetImageHeight(FrontImages, IMAGE_PLAYERN)-4;
	space = MULTIOP_ROW_WIDTH - 0 - facW * NUM_FACTIONS;
	spaceDiv = NUM_FACTIONS;
	space = std::min(space, 5 * spaceDiv);

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
				widgScheduleTask([pStrongPtr] {
					pStrongPtr->closeColourChooser();
					pStrongPtr->addPlayerBox(true);
				});
			}
		};

		addMultiButWithClickHandler(psInlineChooserForm, MULTIOP_FACCHOOSER + i,
			i * (facW * spaceDiv + space) / spaceDiv + PLAYERBOX_X0,  MULTIOP_PLAYERHEIGHT, // x, y
			facW, facH,  // w, h
			to_localized_string(static_cast<FactionID>(i)),
			IMAGE_FACTION_NORMAL+i, IMAGE_FACTION_NORMAL_HI+i, IMAGE_FACTION_NORMAL_HI+i, onClickHandler
		);
	}

	inlineChooserUp = player;
}

void WzMultiplayerOptionsTitleUI::closeColourChooser()
{
	inlineChooserUp = -1;
	widgDelete(psInlineChooserOverlayScreen, MULTIOP_COLCHOOSER_FORM);
	widgRemoveOverlayScreen(psInlineChooserOverlayScreen);
}

void WzMultiplayerOptionsTitleUI::closeTeamChooser()
{
	inlineChooserUp = -1;
	widgDelete(psInlineChooserOverlayScreen, MULTIOP_TEAMCHOOSER_FORM);
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
	// AiChooser / DifficultyChooser / PositionChooser currently use the same formID
	// Just call closeAllChoosers() for now
	closeAllChoosers();
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
		NETbeginEncode(NETnetQueue(NetPlay.hostPlayer), NET_TEAMREQUEST);

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

	if (!alliancesSetTeamsBeforeGame(game.alliance))
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
		NETbeginEncode(NETnetQueue(NetPlay.hostPlayer), NET_READY_REQUEST);
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

	if (player >= MAX_CONNECTED_PLAYERS)
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

	// do not allow players to select 'ready' if we are sending a map to them!
	// TODO: make a new icon to show this state?
	if (NetPlay.players[player].fileSendInProgress())
	{
		return false;
	}

	// Sanity-check: players should always have an identity to check "ready"
	const PLAYERSTATS& stats = getMultiStats(player);
	if (stats.identity.empty() && bReady && !NetPlay.players[player].isSpectator)
	{
		// log this!
		debug(LOG_INFO, "Player has empty identity: (player: %u, name: \"%s\", IP: %s)", (unsigned)player, NetPlay.players[player].name, NetPlay.players[player].IPtextAddress);

		// kick the player
		HandleBadParam("NET_READY_REQUEST failed due to player empty identity.", player, queue.index);
		return false;
	}

	return changeReadyStatus((UBYTE)player, bReady);
}

bool changeReadyStatus(UBYTE player, bool bReady)
{
	NetPlay.players[player].ready = bReady;
	NETBroadcastPlayerInfo(player);
	netPlayersUpdated = true;
	// Player is fast! Clicked the "Ready" button before we had a chance to ping him/her
	// change PingTime to some value less than PING_LIMIT, so that multiplayPlayersReady
	// doesnt block
	ingame.PingTimes[player] = ingame.PingTimes[player] == PING_LIMIT ? 1 : ingame.PingTimes[player];
	return true;
}

static bool changePosition(UBYTE player, UBYTE position)
{
	ASSERT(player < MAX_PLAYERS, "Invalid player idx: %" PRIu8, player);
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

	if (player >= MAX_PLAYERS)
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
		NETbeginEncode(NETnetQueue(NetPlay.hostPlayer), NET_COLOURREQUEST);
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
		NETbeginEncode(NETnetQueue(NetPlay.hostPlayer), NET_FACTIONREQUEST);
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
		NETbeginEncode(NETnetQueue(NetPlay.hostPlayer), NET_POSITIONREQUEST);
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

static bool SendPlayerSlotTypeRequest(uint32_t player, bool isSpectator)
{
	const char* originalPlayerSlotType = (NetPlay.players[player].isSpectator) ? "Spectator" : "Player";
	const char* desiredPlayerSlotType = (isSpectator) ? "Spectator" : "Player";

	// If I'm a spectator, and I ask to be a player, record locally that I gave permission for such a change
	if (!NetPlay.isHost && NetPlay.players[player].isSpectator && !isSpectator)
	{
		bRequestedSelfMoveToPlayers = true;
	}
	if (NetPlay.isHost && NetPlay.players[player].isSpectator && !isSpectator)
	{
		// If the host asks the spectator if they want to be a player, record this so we can appropriately flag that later move as "host initiated"
		bHostRequestedMoveToPlayers[player] = true;
	}

	debug(LOG_NET, "Requesting the host to change our slot type. From %s to %s", originalPlayerSlotType, desiredPlayerSlotType);
	// clients tell the host which player slot type they want (but the host may not allow)
	NETbeginEncode(NETnetQueue((!NetPlay.isHost) ? NetPlay.hostPlayer : player), NET_PLAYER_SLOTTYPE_REQUEST);
	NETuint32_t(&player);
	NETbool(&isSpectator);
	NETend();
	return true;
}

static bool recvPlayerSlotTypeRequestAndPop(WzMultiplayerOptionsTitleUI& titleUI, NETQUEUE queue)
{
	// A player is requesting a slot type change
	// ex. player -> spectator

	uint32_t playerIndex;
	bool desiredIsSpectator = false;
	NETbeginDecode(queue, NET_PLAYER_SLOTTYPE_REQUEST);
	NETuint32_t(&playerIndex);
	NETbool(&desiredIsSpectator);
	NETend();

	NETpop(queue); // this function *must* handle popping the message itself since, if a player index switch occurs, the queue may be invalidated

	// Basic parameter validation

	if (!ingame.localJoiningInProgress)  // Only if game hasn't actually started yet.
	{
		debug(LOG_WARNING, "Player indexes cannot be swapped after game has started");
		return false;
	}

	if (playerIndex >= MAX_CONNECTED_PLAYERS)
	{
		debug(LOG_ERROR, "Invalid NET_PLAYER_SLOTTYPE_REQUEST from player %" PRIu32 ": Tried to change player %" PRIu32 "",
			  queue.index, playerIndex);
		return false;
	}

	if (whosResponsible(playerIndex) != queue.index && !(queue.index == NetPlay.hostPlayer && whosResponsible(playerIndex) == selectedPlayer && selectedPlayer != NetPlay.hostPlayer))
	{
		HandleBadParam("NET_PLAYER_SLOTTYPE_REQUEST given incorrect params.", playerIndex, queue.index);
		return false;
	}

	if (desiredIsSpectator == NetPlay.players[playerIndex].isSpectator)
	{
		// no-op change - this may be a response to a request
		std::string playerName = getPlayerName(playerIndex);
		std::string text;
		if (NetPlay.isHost)
		{
			// client sending response
			if (desiredIsSpectator)
			{
				text = astringf(_("Spectator %s wants to remain a Spectator"), playerName.c_str());
			}
			else
			{
				text = astringf(_("Player %s wants to remain a Player"), playerName.c_str());
			}
		}
		else
		{
			// host sending a denial
			if (desiredIsSpectator)
			{
				text = astringf(_("Host has declined to switch you to a Player"), playerName.c_str());
			}
			else
			{
				text = astringf(_("Unable to switch to Spectator"), playerName.c_str());
			}
		}
		displayRoomSystemMessage(text.c_str());
		return false;
	}

	if (!NetPlay.isHost)
	{
		// Client-only message handling
		// The host can ask a client (us) if we want to switch player slot type
		if (queue.index != NetPlay.hostPlayer)
		{
			// Only the host can ask!
			debug(LOG_ERROR, "Invalid NET_PLAYER_SLOTTYPE_REQUEST from player %" PRIu32 "", queue.index);
			return false;
		}

		if (!hasNotificationsWithTag(SLOTTYPE_REQUEST_TAG))
		{
			WZ_Notification notification;
			notification.duration = 0;
			std::string contentTitle;
			std::string contentText;
			std::string proceedButtonText;
			std::string notificationId;
			if (desiredIsSpectator)
			{
				contentTitle = _("Do you want to spectate?");
				contentText = _("The host of this game wants to know if you're willing to spectate?");
				contentText += "\n";
				contentText += _("You are currently a Player.");
				proceedButtonText = _("Yes, I will spectate!");
				notificationId = "spectate";
			}
			else
			{
				contentTitle = _("Do you want to play?");
				contentText = _("The host of this game wants to know if you'd like to play?");
				contentText += "\n";
				contentText += _("You are currently a Spectator.");
				proceedButtonText = _("Yes, I want to play!");
				notificationId = "play";
			}
			notification.contentTitle = contentTitle;
			notification.contentText = contentText;
			notification.action = WZ_Notification_Action(proceedButtonText, [desiredIsSpectator](const WZ_Notification&) {
				SendPlayerSlotTypeRequest(selectedPlayer, desiredIsSpectator);
			});
			notification.onDismissed = [](const WZ_Notification&, WZ_Notification_Dismissal_Reason reason) {
				if (reason != WZ_Notification_Dismissal_Reason::USER_DISMISSED) { return; }
				// Notify the host that the answer is "no" by sending a no-op PlayerSlotTypeRequest
				SendPlayerSlotTypeRequest(selectedPlayer, NetPlay.players[selectedPlayer].isSpectator);
			};
			const std::string notificationIdentifierPrefix = SLOTTYPE_NOTIFICATION_ID_PREFIX;
			const std::string notificationIdentifier = notificationIdentifierPrefix + notificationId;
			notification.displayOptions = WZ_Notification_Display_Options::makeIgnorable(notificationIdentifier, 2);
			notification.onIgnored = [](const WZ_Notification&) {
				// Notify the host that the answer is "no" by sending a no-op PlayerSlotTypeRequest
				SendPlayerSlotTypeRequest(selectedPlayer, NetPlay.players[selectedPlayer].isSpectator);
			};
			notification.tag = SLOTTYPE_REQUEST_TAG;

			addNotification(notification, WZ_Notification_Trigger::Immediate());
		}

		return true;
	}

	// Host-only message handling

	ASSERT_HOST_ONLY(return false);

	const char *pPlayerName = getPlayerName(playerIndex);
	std::string playerName = (pPlayerName) ? pPlayerName : (std::string("[p") + std::to_string(playerIndex) + "]");

	if (desiredIsSpectator)
	{
		if (!NETmovePlayerToSpectatorOnlySlot(playerIndex, false))
		{
			// Notify the player that the answer is "no" / the move failed by sending a no-op PlayerSlotTypeRequest
			SendPlayerSlotTypeRequest(playerIndex, NetPlay.players[playerIndex].isSpectator);
			return false;
		}

		std::string msg = astringf(_("Player %s has moved to Spectators"), playerName.c_str());
		sendRoomSystemMessage(msg.c_str());
		resetReadyStatus(true);		//reset and send notification to all clients
	}
	else
	{
		// Spectator wants to switch to a player slot

		// Try a move to any open player slot
		auto result = NETmoveSpectatorToPlayerSlot(playerIndex, nullopt, bHostRequestedMoveToPlayers.at(playerIndex));
		bHostRequestedMoveToPlayers[playerIndex] = false;
		switch (result)
		{
			case SpectatorToPlayerMoveResult::SUCCESS:
			{
				// Was able to move spectator to an open player slot automatically
				std::string msg = astringf(_("Spectator %s has moved to Players"), playerName.c_str());
				sendRoomSystemMessage(msg.c_str());
				resetReadyStatus(true);		//reset and send notification to all clients
				break;
			}
			case SpectatorToPlayerMoveResult::NEEDS_SLOT_SELECTION:
			{
				if (headlessGameMode())
				{
					// displaying a UI won't work
					// so instead, send a room message about the failure
					std::string msg = astringf(_("Unable to move %s to Players - no available slot"), playerName.c_str());
					sendRoomSystemMessage(msg.c_str());
					return true; // and immediately return
				}

				// Display a notification that a spectator would like to switch to a player
				std::string notificationTag = SLOTTYPE_REQUEST_TAG;
				notificationTag += "::" + std::to_string(playerIndex);
				if (hasNotificationsWithTag(notificationTag))
				{
					// no nagging permitted - there's already a notification queued for this player index
					break;
				}
				auto playerSlotSwapChooserUp = titleUI.playerSlotSwapChooserOpenForPlayer();
				if (playerSlotSwapChooserUp.has_value() && playerSlotSwapChooserUp.value() == playerIndex)
				{
					// no nagging permitted - the host already has the player slot swap chooser up for this player!
					break;
				}
				WZ_Notification notification;
				notification.duration = 0;
				std::string contentTitle = _("Spectator would like to become a Player");
				std::string contentText = astringf(_("Spectator \"%s\" would like to become a player."), playerName.c_str());
				contentText += "\n";
				contentText += _("However, there are currently no open Player slots.");
				contentText += "\n\n";
				contentText += _("Would you like to swap this Spectator with a Player?");
				std::string proceedButtonText = _("Yes, select Player slot");
				notification.contentTitle = contentTitle;
				notification.contentText = contentText;
				std::weak_ptr<WzMultiplayerOptionsTitleUI> weakTitleUI(std::dynamic_pointer_cast<WzMultiplayerOptionsTitleUI>(titleUI.shared_from_this()));
				notification.action = WZ_Notification_Action(proceedButtonText, [playerIndex, weakTitleUI](const WZ_Notification&) {
					auto strongTitleUI = weakTitleUI.lock();
					ASSERT_OR_RETURN(, strongTitleUI != nullptr, "Expected Title UI is gone");
					widgScheduleTask([weakTitleUI, playerIndex] {
						auto strongTitleUI = weakTitleUI.lock();
						ASSERT_OR_RETURN(, strongTitleUI != nullptr, "Expected Title UI is gone");
						strongTitleUI->openPlayerSlotSwapChooser(playerIndex);
					});
				});
				notification.onDismissed = [playerIndex](const WZ_Notification&, WZ_Notification_Dismissal_Reason reason) {
					if (reason != WZ_Notification_Dismissal_Reason::USER_DISMISSED) { return; }
					// Notify the player that the answer is "no" by sending a no-op PlayerSlotTypeRequest
					SendPlayerSlotTypeRequest(playerIndex, NetPlay.players[playerIndex].isSpectator);
				};
				notification.tag = notificationTag;

				addNotification(notification, WZ_Notification_Trigger::Immediate());
				// "handled" - now up to the host to decide what happens, so return true below
				break;
			}
			case SpectatorToPlayerMoveResult::FAILED:
			{
				// Notify the player that the answer is "no" by sending a no-op PlayerSlotTypeRequest
				SendPlayerSlotTypeRequest(playerIndex, NetPlay.players[playerIndex].isSpectator);
				return false;
			}
		}
	}

	return true;
}

enum class SwapPlayerIndexesResult
{
	SUCCESS,
	FAILURE,
	ERROR_HOST_MADE_US_PLAYER_WITHOUT_PERMISSION,
};

static SwapPlayerIndexesResult recvSwapPlayerIndexes(NETQUEUE queue, const std::shared_ptr<WzTitleUI>& parent)
{
	// A special message that can be triggered by the host to queue-up a player index swap (for example, converting a player to a spectator-only slot - really should only be used for converting between player and spectator slots in the lobby)
	// For the player being moved, this simulates a lot of the effects of a join, followed by a (quiet / virtual) "leave" of the old slot
	// For other players, this provides a message and a (quiet / virtual) "leave" of the old slot
	// Everyone then waits for the host to provide updated NET_PLAYER_INFO

	ASSERT_OR_RETURN(SwapPlayerIndexesResult::FAILURE, queue.index == NetPlay.hostPlayer, "Message received from non-host user");

	uint32_t playerIndexA;
	uint32_t playerIndexB;

	NETbeginDecode(queue, NET_PLAYER_SWAP_INDEX);
	NETuint32_t(&playerIndexA);
	NETuint32_t(&playerIndexB);
	NETend();

	if (!ingame.localJoiningInProgress)  // Only if game hasn't actually started yet.
	{
		debug(LOG_ERROR, "Player indexes cannot be swapped after game has started");
		return SwapPlayerIndexesResult::FAILURE;
	}
	if (NetPlay.isHost)
	{
		debug(LOG_ERROR, "Should never be called for the host");
		return SwapPlayerIndexesResult::FAILURE;
	}

	if (playerIndexA >= MAX_CONNECTED_PLAYERS)
	{
		debug(LOG_ERROR, "Bad player number (%" PRIu32 ") received from host!", playerIndexA);
		return SwapPlayerIndexesResult::FAILURE;
	}

	if (playerIndexB >= MAX_CONNECTED_PLAYERS)
	{
		debug(LOG_ERROR, "Bad player number (%" PRIu32 ") received from host!", playerIndexB);
		return SwapPlayerIndexesResult::FAILURE;
	}

	// Make sure neither is the host index, as this is *not* supported for the host
	if (playerIndexA == NetPlay.hostPlayer || playerIndexB == NetPlay.hostPlayer)
	{
		debug(LOG_ERROR, "Cannot swap host slot! (players: %" PRIu32 ", %" PRIu32 ")!", playerIndexA, playerIndexB);
		return SwapPlayerIndexesResult::FAILURE;
	}

	// Backup some data
	std::array<bool, 2> wasSpectator = {NetPlay.players[playerIndexA].isSpectator, NetPlay.players[playerIndexB].isSpectator};

	// 1.) Simulate the old slots "leaving" (quietly)
	// From handling of NET_PLAYER_DROPPED (to cleanup old player indexes - wait for new player info from host)
	std::array<uint32_t, 2> playerIndexes = {playerIndexA, playerIndexB};
	for (auto playerIndex : playerIndexes)
	{
		// From MultiPlayerLeave()
		//  From clearPlayer() - this is needed to disconnect PlayerReferences so, for example, old chat messages are still associated with the proper player
		NetPlay.playerReferences[playerIndex]->disconnect();
		NetPlay.playerReferences[playerIndex] = std::make_shared<PlayerReference>(playerIndex);

//		NetPlay.players[playerIndex].difficulty = AIDifficulty::DISABLED;
		//
		NET_InitPlayer(playerIndex, false);
		NETsetPlayerConnectionStatus(CONNECTIONSTATUS_PLAYER_DROPPED, playerIndex); // needed??
		if (playerIndex < MAX_PLAYERS)
		{
			playerVotes[playerIndex] = 0;
		}
	}
	swapPlayerMultiStatsLocal(playerIndexA, playerIndexB);
	multiSyncPlayerSwap(playerIndexA, playerIndexB);

	if (playerIndexA == selectedPlayer || playerIndexB == selectedPlayer)
	{
		uint32_t oldPlayerIndex = selectedPlayer;
		uint32_t newPlayerIndex = (playerIndexA == selectedPlayer) ? playerIndexB : playerIndexA;
		bool selectedPlayerWasSpectator = wasSpectator[(playerIndexA == selectedPlayer) ? 0 : 1];

		// Send an acknowledgement that we received and are processing the player index swap for us
		NETbeginEncode(NETnetQueue(NetPlay.hostPlayer), NET_PLAYER_SWAP_INDEX_ACK);
		NETuint32_t(&oldPlayerIndex);
		NETuint32_t(&newPlayerIndex);
		NETend();

		// 1.) Basically what happens in NETjoinGame after receiving NET_ACCEPTED

		selectedPlayer = newPlayerIndex;
		realSelectedPlayer = selectedPlayer;
		debug(LOG_NET, "NET_PLAYER_SWAP_INDEX received for me. Switching from player %" PRIu32 " to player %" PRIu32 "", oldPlayerIndex,newPlayerIndex);

		NetPlay.players[selectedPlayer].allocated = true;
		setPlayerName(selectedPlayer, sPlayer);
		NetPlay.players[selectedPlayer].heartbeat = true;

		// Ensure name is set properly (and re-send to host)
		NETchangePlayerName(selectedPlayer, (char *)sPlayer);			// update since we're simulating re-joining

		// 2.) Basically what happens in joinGameInternalConnect after successful NETjoinGame

		// Move my stats to the new player slot, and then broadcast
		PLAYERSTATS	playerStats;
		loadMultiStats(sPlayer, &playerStats);
		setMultiStats(selectedPlayer, playerStats, false);
		setMultiStats(selectedPlayer, playerStats, true);

		if (selectedPlayer < MAX_PLAYERS && war_getMPcolour() >= 0)
		{
			SendColourRequest(selectedPlayer, war_getMPcolour());
		}

		if (selectedPlayerWasSpectator != NetPlay.players[selectedPlayer].isSpectator)
		{
			// Spectator status of player slot changed
			debug(LOG_NET, "Spectator status changed! From %s to %s", (selectedPlayerWasSpectator) ? "true" : "false", (NetPlay.players[selectedPlayer].isSpectator) ? "true" : "false");
			if (!NetPlay.players[selectedPlayer].isSpectator)
			{
				if (!bRequestedSelfMoveToPlayers)
				{
					// Host moved us from spectators to players, but we never asked for the move
					debug(LOG_NET, "The host tried to move us to Players, but we never gave permission.");
					return SwapPlayerIndexesResult::ERROR_HOST_MADE_US_PLAYER_WITHOUT_PERMISSION;
				}
				else
				{
					// We did indeed give permission for the move, so reset the flag
					bRequestedSelfMoveToPlayers = false;
				}
			}
		}
	}
	else
	{
		// TODO: display a textual description of the move
		// just wait for the new player info to be broadcast by the host
	}

	ActivityManager::instance().updateMultiplayGameData(game, ingame, NETGameIsLocked());
	netPlayersUpdated = true;	// update the player box.

	return SwapPlayerIndexesResult::SUCCESS;
}

static bool canChooseTeamFor(int i)
{
	return (i == selectedPlayer || NetPlay.isHost);
}

// ////////////////////////////////////////////////////////////////////////////
// tabs for player box

class WzPlayerBoxTabButton : public W_BUTTON
{
protected:
	WzPlayerBoxTabButton()
	: W_BUTTON()
	{}

public:
	static std::shared_ptr<WzPlayerBoxTabButton> make(const std::string& title)
	{
		class make_shared_enabler: public WzPlayerBoxTabButton {};
		auto widget = std::make_shared<make_shared_enabler>();

		// add the titleLabel
		widget->titleLabel = std::make_shared<W_LABEL>();
		widget->titleLabel->setFont(font_regular, WZCOL_TEXT_BRIGHT);
		widget->titleLabel->setString(WzString::fromUtf8(title));
		widget->titleLabel->setGeometry(0, 0, widget->titleLabel->getMaxLineWidth(), widget->titleLabel->idealHeight());
		widget->titleLabel->setCanTruncate(true);

		// add the slot counts label
		widget->slotsCountsLabel = std::make_shared<W_LABEL>();
		widget->slotsCountsLabel->setFont(font_small, WZCOL_TEXT_BRIGHT);

		// refresh tooltip
		widget->refreshTip();

		return widget;
	}

	void display(int xOffset, int yOffset) override;
	void geometryChanged() override;

public:
	void setSlotCounts(uint8_t numTakenSlots, uint8_t totalAvailableSlots);
	void setSlotReadyStatus(uint8_t numTakenSlotsReady);
	void setSelected(bool selected);

private:
	inline AtlasImage getImageForSlotsReadyStatus() const
	{
		if (totalAvailableSlots == 0)
		{
			return AtlasImage();
		}
		if (numTakenSlots == 0)
		{
			return AtlasImage();
		}
		if (numTakenSlots != numTakenSlotsReady)
		{
			return AtlasImage(FrontImages, IMAGE_LAMP_AMBER);
		}
		return AtlasImage(FrontImages, IMAGE_LAMP_GREEN);
	}
	void recalculateSlotsLabel();
	void recalculateTitleLabel();
	void refreshTip();

private:
	uint8_t numTakenSlots = 0;
	uint8_t numTakenSlotsReady = 0;
	uint8_t totalAvailableSlots = 0;
	int rightPadding = 0;
	const int elementPadding = 5;
	const int borderWidth = 1;
	bool selected = false;
	std::shared_ptr<W_LABEL> titleLabel;
	std::shared_ptr<W_LABEL> slotsCountsLabel;
};

void WzPlayerBoxTabButton::setSlotCounts(uint8_t new_numTakenSlots, uint8_t new_totalAvailableSlots)
{
	if (new_numTakenSlots != numTakenSlots || new_totalAvailableSlots != totalAvailableSlots)
	{
		numTakenSlots = new_numTakenSlots;
		totalAvailableSlots = new_totalAvailableSlots;

		WzString slotCountsStr = WzString::number(numTakenSlots) + " / " + WzString::number(totalAvailableSlots);
		slotsCountsLabel->setString(slotCountsStr);
		recalculateSlotsLabel();
		recalculateTitleLabel();
		refreshTip();
	}
}

void WzPlayerBoxTabButton::refreshTip()
{
	WzString tipStr = titleLabel->getString();
	tipStr += "\n";
	tipStr += _("Joined:");
	tipStr += WzString(" ") + WzString::number(numTakenSlots) + "/" + WzString::number(totalAvailableSlots);
	tipStr += " - ";
	tipStr += _("Ready:");
	tipStr += WzString(" ") + WzString::number(numTakenSlotsReady) + "/" + WzString::number(numTakenSlots);
	setTip(tipStr.toUtf8());
}

void WzPlayerBoxTabButton::recalculateSlotsLabel()
{
	int slotCountsLabelWidth = slotsCountsLabel->getMaxLineWidth();
	int slotCountsLabelHeight = slotsCountsLabel->idealHeight();
	int slotCountsLabelX0 = width() - borderWidth - rightPadding - elementPadding - slotCountsLabelWidth;
	int slotCountsLabelY0 = (height() - slotCountsLabelHeight) / 2;
	slotsCountsLabel->setGeometry(slotCountsLabelX0, slotCountsLabelY0, slotCountsLabelWidth, slotCountsLabelHeight);
}

void WzPlayerBoxTabButton::recalculateTitleLabel()
{
	// size titleLabel to take up available space in the middle
	int slotCountsLabelX0 = slotsCountsLabel->x();
	int titleLabelX0 = borderWidth + elementPadding + (AtlasImage(FrontImages, IMAGE_LAMP_GREEN).width() / 2) + elementPadding;
	int titleLabelY0 = (height() - titleLabel->height()) / 2;
	int titleLabelWidth = slotCountsLabelX0 - elementPadding - titleLabelX0;
	titleLabel->setGeometry(titleLabelX0, titleLabelY0, titleLabelWidth, titleLabel->height());
}

void WzPlayerBoxTabButton::setSlotReadyStatus(uint8_t new_numTakenSlotsReady)
{
	numTakenSlotsReady = new_numTakenSlotsReady;
	refreshTip();
}

void WzPlayerBoxTabButton::setSelected(bool new_selected)
{
	selected = new_selected;
}

void WzPlayerBoxTabButton::geometryChanged()
{
	recalculateSlotsLabel();
	recalculateTitleLabel();
}

void WzPlayerBoxTabButton::display(int xOffset, int yOffset)
{
	int x0 = xOffset + x();
	int y0 = yOffset + y();
	int w = width();
	int h = height();
	bool highlight = (getState() & WBUT_HIGHLIGHT) != 0;
	bool down = (getState() & (WBUT_DOWN | WBUT_LOCK | WBUT_CLICKLOCK)) != 0;

	// draw box
	PIELIGHT boxBorder = (!selected) ? WZCOL_MENU_BACKGROUND : WZCOL_MENU_BORDER;
	if (highlight)
	{
		boxBorder = pal_RGBA(255, 255, 255, 255);
	}
	PIELIGHT boxBackground = (!selected) ? WZCOL_MENU_BACKGROUND : WZCOL_MENU_BORDER;
	pie_BoxFill(x0, y0, x0 + w, y0 + h, boxBorder);
	pie_BoxFill(x0 + 1, y0 + 1, x0 + w - 1, y0 + h - 1, boxBackground);
	if (!selected && (!highlight || down))
	{
		pie_UniTransBoxFill(x0 + 1, y0 + 1, x0 + w - 1, y0 + h - 1, pal_RGBA(0, 0, 0, 80));
	}

	// draw ready status image
	AtlasImage readyStatusImage = getImageForSlotsReadyStatus();
	if (!readyStatusImage.isNull())
	{
		int imageDisplayWidth = readyStatusImage.width() / 2;
		int imageDisplayHeight = readyStatusImage.height() / 2;
		int imageX0 = x0 + borderWidth + elementPadding;
		int imageY0 = y0 + ((h - imageDisplayHeight) / 2);
		iV_DrawImageFileAnisotropic(readyStatusImage.images, readyStatusImage.id, imageX0, imageY0, Vector2f(imageDisplayWidth, imageDisplayHeight), defaultProjectionMatrix(), 255);
	}

	// label drawing is handled by the embedded W_LABEL
	titleLabel->display(x0, y0);

	// slot count drawing is handled by the embedded W_LABEL
	slotsCountsLabel->display(x0, y0);
}

class WzPlayerBoxOptionsButton : public W_BUTTON
{
protected:
	WzPlayerBoxOptionsButton()
	: W_BUTTON()
	{}

public:
	static std::shared_ptr<WzPlayerBoxOptionsButton> make()
	{
		class make_shared_enabler: public WzPlayerBoxOptionsButton {};
		auto widget = std::make_shared<make_shared_enabler>();

		// add the titleLabel
		widget->titleLabel = std::make_shared<W_LABEL>();
		widget->titleLabel->setFont(font_regular, WZCOL_TEXT_BRIGHT);
		widget->titleLabel->setString(WzString::fromUtf8("\u2699")); // "⚙"
		std::weak_ptr<WzPlayerBoxOptionsButton> psWeakParent = widget;
		widget->titleLabel->setCalcLayout([psWeakParent](WIDGET *psWidget){
			auto psParent = psWeakParent.lock();
			ASSERT_OR_RETURN(, psParent != nullptr, "Parent is null");
			psWidget->setGeometry(0, 0, psParent->width(), psParent->height());
		});
		widget->titleLabel->setTextAlignment(WLAB_ALIGNCENTRE);

		return widget;
	}

	void geometryChanged() override
	{
		if (titleLabel)
		{
			titleLabel->callCalcLayout();
		}
	}

	void display(int xOffset, int yOffset) override
	{
		int x0 = xOffset + x();
		int y0 = yOffset + y();
		int w = width();
		int h = height();
		bool highlight = (getState() & WBUT_HIGHLIGHT) != 0;
		bool down = (getState() & (WBUT_DOWN | WBUT_LOCK | WBUT_CLICKLOCK)) != 0;
		bool selected = false;

		// draw box
		PIELIGHT boxBorder = (!selected) ? WZCOL_MENU_BACKGROUND : WZCOL_MENU_BORDER;
		if (highlight)
		{
			boxBorder = pal_RGBA(255, 255, 255, 255);
		}
		PIELIGHT boxBackground = (!selected) ? WZCOL_MENU_BACKGROUND : WZCOL_MENU_BORDER;
		pie_BoxFill(x0, y0, x0 + w, y0 + h, boxBorder);
		pie_BoxFill(x0 + 1, y0 + 1, x0 + w - 1, y0 + h - 1, boxBackground);
		if (!selected && (!highlight || down))
		{
			pie_UniTransBoxFill(x0 + 1, y0 + 1, x0 + w - 1, y0 + h - 1, pal_RGBA(0, 0, 0, 80));
		}

		// label drawing is handled by the embedded W_LABEL
		titleLabel->display(x0, y0);
	}

private:
	std::shared_ptr<W_LABEL> titleLabel;
};

class WzPlayerBoxTabs : public WIDGET
{
protected:
	WzPlayerBoxTabs(const std::shared_ptr<WzMultiplayerOptionsTitleUI>& titleUI)
	: WIDGET()
	, weakTitleUI(titleUI)
	{ }

	~WzPlayerBoxTabs()
	{
		if (optionsOverlayScreen)
		{
			widgRemoveOverlayScreen(optionsOverlayScreen);
		}
	}

public:
	static std::shared_ptr<WzPlayerBoxTabs> make(bool displayHostOptions, const std::shared_ptr<WzMultiplayerOptionsTitleUI>& titleUI)
	{
		class make_shared_enabler: public WzPlayerBoxTabs {
		public:
			make_shared_enabler(const std::shared_ptr<WzMultiplayerOptionsTitleUI>& titleUI) : WzPlayerBoxTabs(titleUI) { }
		};
		auto widget = std::make_shared<make_shared_enabler>(titleUI);

		std::weak_ptr<WzMultiplayerOptionsTitleUI> weakTitleUI(titleUI);

		// add the "players" button
		auto playersButton = WzPlayerBoxTabButton::make(_("Players"));
		playersButton->setSelected(true);
		widget->tabButtons.push_back(playersButton);
		widget->attach(playersButton);
		playersButton->addOnClickHandler([weakTitleUI](W_BUTTON& button){
			widgScheduleTask([weakTitleUI](){
				auto strongTitleUI = weakTitleUI.lock();
				ASSERT_OR_RETURN(, strongTitleUI != nullptr, "No parent title UI");
				playerDisplayView = PlayerDisplayView::Players;
				strongTitleUI->updatePlayers();
			});
		});

		// add the "spectators" button
		auto spectatorsButton = WzPlayerBoxTabButton::make(_("Spectators"));
		widget->tabButtons.push_back(spectatorsButton);
		widget->attach(spectatorsButton);
		spectatorsButton->addOnClickHandler([weakTitleUI](W_BUTTON& button){
			widgScheduleTask([weakTitleUI](){
				auto strongTitleUI = weakTitleUI.lock();
				ASSERT_OR_RETURN(, strongTitleUI != nullptr, "No parent title UI");
				playerDisplayView = PlayerDisplayView::Spectators;
				strongTitleUI->updatePlayers();
			});
		});

		if (displayHostOptions)
		{
			// Add "gear" / "Host Options" button
			widget->optionsButton = WzPlayerBoxOptionsButton::make(); // "⚙"
			widget->optionsButton->setTip(_("Host Options"));
			widget->attach(widget->optionsButton);
			widget->optionsButton->addOnClickHandler([](W_BUTTON& button) {
				auto psParent = std::dynamic_pointer_cast<WzPlayerBoxTabs>(button.parent());
				ASSERT_OR_RETURN(, psParent != nullptr, "No parent");
				// Display a "pop-over" options menu
				psParent->displayOptionsOverlay(button.shared_from_this());
			});
		}

		widget->setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
			auto psPlayerBoxTabs = std::dynamic_pointer_cast<WzPlayerBoxTabs>(psWidget->shared_from_this());
			ASSERT_OR_RETURN(, psPlayerBoxTabs.get() != nullptr, "Wrong type of psWidget??");
			psPlayerBoxTabs->recalculateTabLayout();
		}));

		widget->refreshData();

		return widget;
	}

	void geometryChanged() override
	{
		recalculateTabLayout();
	}

	void run(W_CONTEXT *psContext) override
	{
		refreshData();
	}

private:
	void setSelectedTab(size_t index)
	{
		for (size_t i = 0; i < tabButtons.size(); ++i)
		{
			tabButtons[i]->setSelected(i == index);
		}
	}

	void refreshData()
	{
		// update currently-selected tab
		switch (playerDisplayView)
		{
			case PlayerDisplayView::Players:
				setSelectedTab(0);
				break;
			case PlayerDisplayView::Spectators:
				setSelectedTab(1);
				break;
		}

		// update counts of players & spectators
		uint8_t takenPlayerSlots = 0;
		uint8_t totalPlayerSlots = 0;
		uint8_t readyPlayers = 0;
		uint8_t takenSpectatorOnlySlots = 0;
		uint8_t totalSpectatorOnlySlots = 0;
		uint8_t readySpectatorOnlySlots = 0;
		for (size_t i = 0; i < MAX_CONNECTED_PLAYERS; ++i)
		{
			PLAYER const &p = NetPlay.players[i];
			if (p.ai == AI_CLOSED)
			{
				// closed slot - skip
				continue;
			}
			if (isSpectatorOnlySlot(i)
					 && ((i >= NetPlay.players.size()) || !(NetPlay.players[i].isSpectator && NetPlay.players[i].ai == AI_OPEN)))
			{
				// the only slots displayable beyond game.maxPlayers are spectator slots
				continue;
			}
			if (isSpectatorOnlySlot(i))
			{
				if (p.ai == AI_OPEN)
				{
					++totalSpectatorOnlySlots;
					if (NetPlay.players[i].allocated)
					{
						++takenSpectatorOnlySlots;
						if (NetPlay.players[i].ready)
						{
							++readySpectatorOnlySlots;
						}
					}
				}
				continue;
			}
			++totalPlayerSlots;
			if (p.ai == AI_OPEN)
			{
				if (p.allocated)
				{
					++takenPlayerSlots;
					if (NetPlay.players[i].ready)
					{
						++readyPlayers;
					}
				}
			}
			else
			{
				ASSERT(!p.allocated, "Expecting AI bots to not be flagged as allocated?");
				++takenPlayerSlots;
				++readyPlayers; // AI slots are always "ready"
			}
		}

		// players button
		tabButtons[0]->setSlotCounts(takenPlayerSlots, totalPlayerSlots);
		tabButtons[0]->setSlotReadyStatus(readyPlayers);

		// spectators button
		tabButtons[1]->setSlotCounts(takenSpectatorOnlySlots, totalSpectatorOnlySlots);
		tabButtons[1]->setSlotReadyStatus(readySpectatorOnlySlots);
	}

	void recalculateTabLayout()
	{
		int optionsButtonSize = (optionsButton) ? height() : 0;
		int rightPaddingForAllButtons = optionsButtonSize;
		int availableWidthForButtons = width() - rightPaddingForAllButtons; // width minus internal and external button padding
		int baseWidthPerButton = availableWidthForButtons / static_cast<int>(tabButtons.size());
		int x0 = 0;
		for (auto &button : tabButtons)
		{
			int buttonNewWidth = baseWidthPerButton;
			button->setGeometry(x0, 0, buttonNewWidth, height());
			x0 = button->x() + button->width();
		}

		if (optionsButton)
		{
			int optionsButtonX0 = width() - optionsButtonSize;
			optionsButton->setGeometry(optionsButtonX0, 0, optionsButtonSize, optionsButtonSize);
		}
	}

	void displayOptionsOverlay(const std::shared_ptr<WIDGET>& psParent);
	std::shared_ptr<WIDGET> createOptionsPopoverForm();

private:
	std::weak_ptr<WzMultiplayerOptionsTitleUI> weakTitleUI;
	std::vector<std::shared_ptr<WzPlayerBoxTabButton>> tabButtons;
	std::shared_ptr<WzPlayerBoxOptionsButton> optionsButton;
	std::shared_ptr<W_SCREEN> optionsOverlayScreen;
};

static bool hasOpenSpectatorOnlySlots()
{
	// Look for a spectator slot that's available
	for (int i = 0; i < MAX_CONNECTED_PLAYERS; i++)
	{
		if (!isSpectatorOnlySlot(i))
		{
			continue;
		}
		if (game.mapHasScavengers && NetPlay.players[i].position == scavengerSlot())
		{
			continue; // skip it
		}
		if (i == PLAYER_FEATURE)
		{
			continue; // skip it
		}
		if (NetPlay.players[i].isSpectator && NetPlay.players[i].ai == AI_OPEN && !NetPlay.players[i].allocated)
		{
			// found available spectator-only slot
			return true;
		}
	}
	return false;
}

static bool canAddSpectatorOnlySlots()
{
	for (int i = MAX_PLAYER_SLOTS; i < MAX_CONNECTED_PLAYERS; i++)
	{
		if (!isSpectatorOnlySlot(i))
		{
			continue;
		}
		if (NetPlay.players[i].allocated || NetPlay.players[i].isSpectator)
		{
			continue;
		}
		if (game.mapHasScavengers && NetPlay.players[i].position == scavengerSlot())
		{
			continue; // skip it
		}
		if (i == PLAYER_FEATURE)
		{
			continue; // skip it
		}
		return true;
	}
	return false;
}

static void closeAllOpenSpectatorOnlySlots()
{
	ASSERT_HOST_ONLY(return);
	for (int i = 0; i < MAX_CONNECTED_PLAYERS; i++)
	{
		if (!isSpectatorOnlySlot(i))
		{
			continue;
		}
		if (game.mapHasScavengers && NetPlay.players[i].position == scavengerSlot())
		{
			continue; // skip it
		}
		if (i == PLAYER_FEATURE)
		{
			continue; // skip it
		}
		if (NetPlay.players[i].isSpectator && NetPlay.players[i].ai == AI_OPEN && !NetPlay.players[i].allocated)
		{
			// found available spectator-only slot
			// close it
			NetPlay.players[i].ai = AI_CLOSED;
			NetPlay.players[i].isSpectator = false;
			NETBroadcastPlayerInfo(i);
		}
	}
}

static void enableSpectatorJoin(bool enabled)
{
	if (enabled)
	{
		// add max spectator slots
		bool success = false;
		do {
			success = NETopenNewSpectatorSlot();
		} while (success);
	}
	else
	{
		// disable any open / unoccupied spectator slots
		closeAllOpenSpectatorOnlySlots();
	}
	netPlayersUpdated = true;
}

std::shared_ptr<WIDGET> WzPlayerBoxTabs::createOptionsPopoverForm()
{
	// create all the buttons / option rows
	auto createOptionsSpacer = []() -> std::shared_ptr<WIDGET> {
		auto spacerWidget = std::make_shared<WIDGET>();
		spacerWidget->setGeometry(0, 0, 1, 5);
		return spacerWidget;
	};
	auto createOptionsCheckbox = [](const WzString& text, bool isChecked, bool isDisabled, const std::function<void (WzCheckboxButton& button)>& onClickFunc) -> std::shared_ptr<WzCheckboxButton> {
		auto pCheckbox = std::make_shared<WzCheckboxButton>();
		pCheckbox->pText = text;
		pCheckbox->FontID = font_regular;
		pCheckbox->setIsChecked(isChecked);
		pCheckbox->setTextColor(WZCOL_TEXT_BRIGHT);
		if (isDisabled)
		{
			pCheckbox->setState(WBUT_DISABLE);
		}
		Vector2i minimumDimensions = pCheckbox->calculateDesiredDimensions();
		pCheckbox->setGeometry(0, 0, minimumDimensions.x, minimumDimensions.y);
		if (onClickFunc)
		{
			pCheckbox->addOnClickHandler([onClickFunc](W_BUTTON& button){
				auto checkBoxButton = std::dynamic_pointer_cast<WzCheckboxButton>(button.shared_from_this());
				ASSERT_OR_RETURN(, checkBoxButton != nullptr, "checkBoxButton is null");
				onClickFunc(*checkBoxButton);
			});
		}
		return pCheckbox;
	};

	std::vector<std::shared_ptr<WIDGET>> buttons;
	bool hasOpenSpectatorSlots = hasOpenSpectatorOnlySlots();
	std::weak_ptr<WzMultiplayerOptionsTitleUI> weakTitleUICopy = weakTitleUI;
	buttons.push_back(createOptionsCheckbox(_("Enable Spectator Join"), hasOpenSpectatorSlots, !hasOpenSpectatorSlots && !canAddSpectatorOnlySlots(), [weakTitleUICopy](WzCheckboxButton& button){
		bool enableValue = button.getIsChecked();
		widgScheduleTask([enableValue, weakTitleUICopy]{
			auto strongTitleUI = weakTitleUICopy.lock();
			ASSERT_OR_RETURN(, strongTitleUI != nullptr, "No Title UI?");
			enableSpectatorJoin(enableValue);
			strongTitleUI->updatePlayers();
		});
	}));
	buttons.push_back(createOptionsSpacer());
	buttons.push_back(createOptionsCheckbox(_("Lock Teams"), locked.teams, false, [](WzCheckboxButton& button){
		locked.teams = button.getIsChecked();
	}));

	// determine required height for all buttons
	int totalButtonHeight = std::accumulate(buttons.begin(), buttons.end(), 0, [](int a, const std::shared_ptr<WIDGET>& b) {
		return a + b->height();
	});
	int maxButtonWidth = (*(std::max_element(buttons.begin(), buttons.end(), [](const std::shared_ptr<WIDGET>& a, const std::shared_ptr<WIDGET>& b){
		return a->width() < b->width();
	})))->width();

	auto itemsList = ScrollableListWidget::make();
	itemsList->setBackgroundColor(WZCOL_MENU_BACKGROUND);
	itemsList->setPadding({3, 4, 3, 4});
	const int itemSpacing = 4;
	itemsList->setItemSpacing(itemSpacing);
	itemsList->setGeometry(itemsList->x(), itemsList->y(), maxButtonWidth + 8, totalButtonHeight + (static_cast<int>(buttons.size()) * itemSpacing) + 6);
	for (auto& button : buttons)
	{
		itemsList->addItem(button);
	}

	return itemsList;
}

void WzPlayerBoxTabs::displayOptionsOverlay(const std::shared_ptr<WIDGET>& psParent)
{
	auto lockedScreen = screenPointer.lock();
	ASSERT(lockedScreen != nullptr, "The WzPlayerBoxTabs does not have an associated screen pointer?");

	// Initialize the options overlay screen
	optionsOverlayScreen = W_SCREEN::make();
	auto newRootFrm = W_FULLSCREENOVERLAY_CLICKFORM::make();
	std::weak_ptr<W_SCREEN> psWeakOptionsOverlayScreen(optionsOverlayScreen);
	std::weak_ptr<WzPlayerBoxTabs> psWeakPlayerBoxTabs = std::dynamic_pointer_cast<WzPlayerBoxTabs>(shared_from_this());
	newRootFrm->onClickedFunc = [psWeakOptionsOverlayScreen, psWeakPlayerBoxTabs]() {
		if (auto psOverlayScreen = psWeakOptionsOverlayScreen.lock())
		{
			widgRemoveOverlayScreen(psOverlayScreen);
		}
		// Destroy Options overlay / overlay screen
		if (auto strongPlayerBoxTabs = psWeakPlayerBoxTabs.lock())
		{
			strongPlayerBoxTabs->optionsOverlayScreen.reset();
		}
	};
	newRootFrm->onCancelPressed = newRootFrm->onClickedFunc;
	optionsOverlayScreen->psForm->attach(newRootFrm);

	// Create the pop-over form
	auto optionsPopOver = createOptionsPopoverForm();
	newRootFrm->attach(optionsPopOver);

	// Position the pop-over form
	std::weak_ptr<WIDGET> weakParent = psParent;
	optionsPopOver->setCalcLayout([weakParent](WIDGET *psWidget) {
		auto psParent = weakParent.lock();
		ASSERT_OR_RETURN(, psParent != nullptr, "parent is null");
		// (Ideally, with its left aligned with the left side of the "parent" widget, but ensure full visibility on-screen)
		int popOverX0 = psParent->screenPosX();
		if (popOverX0 + psWidget->width() > screenWidth)
		{
			popOverX0 = screenWidth - psWidget->width();
		}
		// (Ideally, with its top aligned with the bottom of the "parent" widget, but ensure full visibility on-screen)
		int popOverY0 = psParent->screenPosY() + psParent->height();
		if (popOverY0 + psWidget->height() > screenHeight)
		{
			popOverY0 = screenHeight - psWidget->height();
		}
		psWidget->move(popOverX0, popOverY0);
	});

	widgRegisterOverlayScreenOnTopOfScreen(optionsOverlayScreen, lockedScreen);
}

// ////////////////////////////////////////////////////////////////////////////
// player row widgets

class WzPlayerRow : public WIDGET
{
protected:
	WzPlayerRow(uint32_t playerIdx, const std::shared_ptr<WzMultiplayerOptionsTitleUI>& parent)
	: WIDGET()
	, parentTitleUI(parent)
	, playerIdx(playerIdx)
	{ }

public:
	static std::shared_ptr<WzPlayerRow> make(uint32_t playerIdx, const std::shared_ptr<WzMultiplayerOptionsTitleUI>& parent)
	{
		class make_shared_enabler: public WzPlayerRow {
		public:
			make_shared_enabler(uint32_t playerIdx, const std::shared_ptr<WzMultiplayerOptionsTitleUI>& parent) : WzPlayerRow(playerIdx, parent) { }
		};
		auto widget = std::make_shared<make_shared_enabler>(playerIdx, parent);

		std::weak_ptr<WzMultiplayerOptionsTitleUI> titleUI(parent);

		// add team button (also displays the spectator "eye" for spectators)
		widget->teamButton = std::make_shared<W_BUTTON>();
		widget->teamButton->setGeometry(0, 0, MULTIOP_TEAMSWIDTH, MULTIOP_TEAMSHEIGHT);
		widget->teamButton->UserData = playerIdx;
		widget->teamButton->displayFunction = displayTeamChooser;
		widget->attach(widget->teamButton);
		widget->teamButton->addOnClickHandler([playerIdx, titleUI](W_BUTTON& button){
			auto strongTitleUI = titleUI.lock();
			ASSERT_OR_RETURN(, strongTitleUI != nullptr, "Title UI is gone?");
			if ((!locked.teams || !locked.spectators))  // Clicked on a team chooser
			{
				if (canChooseTeamFor(playerIdx))
				{
					widgScheduleTask([strongTitleUI, playerIdx] {
						strongTitleUI->openTeamChooser(playerIdx);
					});
				}
			}
		});

		// add player colour
		widget->colorButton = std::make_shared<W_BUTTON>();
		widget->colorButton->setGeometry(MULTIOP_TEAMSWIDTH, 0, MULTIOP_COLOUR_WIDTH, MULTIOP_PLAYERHEIGHT);
		widget->colorButton->UserData = playerIdx;
		widget->colorButton->displayFunction = displayColour;
		widget->attach(widget->colorButton);
		widget->colorButton->addOnClickHandler([playerIdx, titleUI](W_BUTTON& button){
			auto strongTitleUI = titleUI.lock();
			ASSERT_OR_RETURN(, strongTitleUI != nullptr, "Title UI is gone?");
			if (playerIdx == selectedPlayer || NetPlay.isHost)
			{
				if (!NetPlay.players[playerIdx].isSpectator) // not a spectator
				{
					widgScheduleTask([strongTitleUI, playerIdx] {
						strongTitleUI->openColourChooser(playerIdx);
					});
				}
			}
		});

		// add ready button
		widget->updateReadyButton();

		// add player info box (takes up the rest of the space in the middle)
		widget->playerInfo = std::make_shared<W_BUTTON>();
		widget->playerInfo->UserData = playerIdx;
		widget->playerInfo->displayFunction = displayPlayer;
		widget->playerInfo->pUserData = new DisplayPlayerCache();
		widget->playerInfo->setOnDelete([](WIDGET *psWidget) {
			assert(psWidget->pUserData != nullptr);
			delete static_cast<DisplayPlayerCache *>(psWidget->pUserData);
			psWidget->pUserData = nullptr;
		});
		widget->attach(widget->playerInfo);
		widget->playerInfo->setCalcLayout([](WIDGET *psWidget) {
			auto psParent = std::dynamic_pointer_cast<WzPlayerRow>(psWidget->parent());
			ASSERT_OR_RETURN(, psParent != nullptr, "Null parent");
			int x0 = MULTIOP_TEAMSWIDTH + MULTIOP_COLOUR_WIDTH;
			int width = psParent->readyButtonContainer->x() - x0;
			psWidget->setGeometry(x0, 0, width, psParent->height());
		});
		widget->playerInfo->addOnClickHandler([playerIdx, titleUI](W_BUTTON& button){
			auto strongTitleUI = titleUI.lock();
			ASSERT_OR_RETURN(, strongTitleUI != nullptr, "Title UI is gone?");
			if (playerIdx == selectedPlayer || NetPlay.isHost)
			{
				uint32_t player = playerIdx;
				// host can move any player, clients can request to move themselves
				if ((player == selectedPlayer || (NetPlay.players[player].allocated && NetPlay.isHost))
					&& !locked.position
					&& player < MAX_PLAYERS
					&& !isSpectatorOnlySlot(player))
				{
					widgScheduleTask([strongTitleUI, player] {
						strongTitleUI->openPositionChooser(player);
					});
				}
				else if (!NetPlay.players[player].allocated && !locked.ai && NetPlay.isHost)
				{
					if (button.getOnClickButtonPressed() == WKEY_SECONDARY && player < MAX_PLAYERS)
					{
						// Right clicking distributes selected AI's type and difficulty to all other AIs
						for (int i = 0; i < MAX_PLAYERS; ++i)
						{
							// Don't change open/closed slots or humans or spectator-only slots
							if (NetPlay.players[i].ai >= 0 && i != player && !isHumanPlayer(i) && !isSpectatorOnlySlot(i))
							{
								NetPlay.players[i].ai = NetPlay.players[player].ai;
								NetPlay.players[i].isSpectator = NetPlay.players[player].isSpectator;
								NetPlay.players[i].difficulty = NetPlay.players[player].difficulty;
								setPlayerName(i, getAIName(player));
								NETBroadcastPlayerInfo(i);
							}
						}
						widgScheduleTask([strongTitleUI] {
							strongTitleUI->updatePlayers();
							resetReadyStatus(false);
						});
					}
					else
					{
						widgScheduleTask([strongTitleUI, player] {
							strongTitleUI->openAiChooser(player);
						});
					}
				}
			}
		});

		// update tooltips and such
		widget->updateState();

		return widget;
	}

	void geometryChanged() override
	{
		if (readyButtonContainer)
		{
			readyButtonContainer->callCalcLayout();
		}
		if (playerInfo)
		{
			playerInfo->callCalcLayout();
		}
	}

	void updateState()
	{
		// update team button tooltip
		if (NetPlay.players[playerIdx].isSpectator)
		{
			teamButton->setTip(_("Spectator"));
		}
		else if (canChooseTeamFor(playerIdx) && !locked.teams)
		{
			teamButton->setTip(_("Choose Team"));
		}
		else if (locked.teams)
		{
			teamButton->setTip(_("Teams locked"));
		}
		else
		{
			teamButton->setTip(nullptr);
		}

		// hide team button if needed
		bool trueMultiplayerMode = (bMultiPlayer && NetPlay.bComms) || (!NetPlay.isHost && ingame.localJoiningInProgress);
		if (!alliancesSetTeamsBeforeGame(game.alliance) && !NetPlay.players[playerIdx].isSpectator && !trueMultiplayerMode)
		{
			teamButton->hide();
		}
		else
		{
			teamButton->show();
		}

		// update color tooltip
		if ((selectedPlayer == playerIdx || NetPlay.isHost) && (!NetPlay.players[playerIdx].isSpectator))
		{
			colorButton->setTip(_("Click to change player colour"));
		}
		else
		{
			colorButton->setTip(nullptr);
		}

		// update player info box tooltip
		std::string playerInfoTooltip;
		if ((selectedPlayer == playerIdx || NetPlay.isHost) && NetPlay.players[playerIdx].allocated && !locked.position && !isSpectatorOnlySlot(playerIdx))
		{
			playerInfoTooltip = _("Click to change player position");
		}
		else if (!NetPlay.players[playerIdx].allocated)
		{
			if (NetPlay.isHost && !locked.ai)
			{
				playerInfo->style |= WBUT_SECONDARY;
				if (!isSpectatorOnlySlot(playerIdx))
				{
					playerInfoTooltip = _("Click to change AI, right click to distribute choice");
				}
				else
				{
					playerInfoTooltip = _("Click to close spectator slot");
				}
			}
			else if (NetPlay.players[playerIdx].ai >= 0)
			{
				// show AI description. Useful for challenges.
				playerInfoTooltip = aidata[NetPlay.players[playerIdx].ai].tip;
			}
		}
		if (NetPlay.players[playerIdx].allocated)
		{
			const PLAYERSTATS& stats = getMultiStats(playerIdx);
			if (!stats.identity.empty())
			{
				if (!playerInfoTooltip.empty())
				{
					playerInfoTooltip += "\n";
				}
				std::string hash = stats.identity.publicHashString(20);
				playerInfoTooltip += _("Player ID: ");
				playerInfoTooltip += hash.empty()? _("(none)") : hash;
			}
			if (stats.autorating.valid && !stats.autorating.details.empty()
				&& stats.autoratingFrom == RATING_SOURCE_LOCAL) // do not display host-provided details (for now)
			{
				if (!playerInfoTooltip.empty())
				{
					playerInfoTooltip += "\n\n";
				}
				std::string detailsstr = stats.autorating.details;
				if (detailsstr.size() > 512)
				{
					detailsstr = detailsstr.substr(0, 512);
				}
				size_t maxLinePos = nthOccurrenceOfChar(detailsstr, '\n', 10);
				if (maxLinePos != std::string::npos)
				{
					detailsstr = detailsstr.substr(0, maxLinePos);
				}
				playerInfoTooltip += std::string(_("Player rating:")) + "\n";
				if (stats.autoratingFrom == RATING_SOURCE_HOST)
				{
					playerInfoTooltip += std::string("(") + _("Host provided") + ")";
				}
				else
				{
					playerInfoTooltip += std::string("(") + astringf(_("From: %s"), getAutoratingUrl().c_str()) + ")";
				}
				playerInfoTooltip += "\n" + detailsstr;
			}
		}
		playerInfo->setTip(playerInfoTooltip);

		// update ready button
		updateReadyButton();
	}

public:

	void updateReadyButton()
	{
		int disallow = allPlayersOnSameTeam(-1);

		if (!readyButtonContainer)
		{
			// add form to hold 'ready' botton
			readyButtonContainer = createBlueForm(MULTIOP_PLAYERWIDTH - MULTIOP_READY_WIDTH, 0,
						MULTIOP_READY_WIDTH, MULTIOP_READY_HEIGHT, displayReadyBoxContainer);
			readyButtonContainer->UserData = playerIdx;
			attach(readyButtonContainer);
			readyButtonContainer->setCalcLayout([](WIDGET *psWidget) {
				auto psParent = psWidget->parent();
				ASSERT_OR_RETURN(, psParent != nullptr, "Null parent");
				psWidget->setGeometry(psParent->width() - MULTIOP_READY_WIDTH, 0, psWidget->width(), psWidget->height());
			});
		}

		auto deleteExistingReadyButton = [this]() {
			if (readyButton)
			{
				widgDelete(readyButton.get());
				readyButton = nullptr;
			}
			if (readyTextLabel)
			{
				widgDelete(readyTextLabel.get());
				readyTextLabel = nullptr;
			}
		};
		auto deleteExistingDifficultyButton = [this]() {
			if (difficultyChooserButton)
			{
				widgDelete(difficultyChooserButton.get());
				difficultyChooserButton = nullptr;
			}
		};

		if (!NetPlay.players[playerIdx].allocated && NetPlay.players[playerIdx].ai >= 0)
		{
			// Add AI difficulty chooser in place of normal "ready" button
			deleteExistingReadyButton();
			int playerDifficulty = static_cast<int8_t>(NetPlay.players[playerIdx].difficulty);
			int icon = difficultyIcon(playerDifficulty);
			char tooltip[128 + 255];
			if (playerDifficulty >= static_cast<int>(AIDifficulty::EASY) && playerDifficulty < static_cast<int>(AIDifficulty::INSANE) + 1)
			{
				sstrcpy(tooltip, _(difficultyList[playerDifficulty]));
				if (NetPlay.players[playerIdx].ai < aidata.size())
				{
					const char *difficultyTip = aidata[NetPlay.players[playerIdx].ai].difficultyTips[playerDifficulty];
					if (strcmp(difficultyTip, "") != 0)
					{
						sstrcat(tooltip, "\n");
						sstrcat(tooltip, difficultyTip);
					}
				}
			}
			bool freshDifficultyButton = (difficultyChooserButton == nullptr);
			difficultyChooserButton = addMultiBut(*readyButtonContainer, MULTIOP_DIFFICULTY_INIT_START + playerIdx, 6, 4, MULTIOP_READY_WIDTH, MULTIOP_READY_HEIGHT,
						(NetPlay.isHost && !locked.difficulty) ? _("Click to change difficulty") : tooltip, icon, icon, icon);
			auto player = playerIdx;
			auto weakTitleUi = parentTitleUI;
			if (freshDifficultyButton)
			{
				difficultyChooserButton->addOnClickHandler([player, weakTitleUi](W_BUTTON&){
					auto strongTitleUI = weakTitleUi.lock();
					ASSERT_OR_RETURN(, strongTitleUI != nullptr, "Title UI is gone?");
					if (!locked.difficulty && NetPlay.isHost)
					{
						widgScheduleTask([strongTitleUI, player] {
							strongTitleUI->openDifficultyChooser(player);
						});
					}
				});
			}
			return;
		}
		else if (!NetPlay.players[playerIdx].allocated)
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

		deleteExistingDifficultyButton();

		bool isMe = playerIdx == selectedPlayer;
		int isReady = NETgetDownloadProgress(playerIdx) != 100 ? 2 : NetPlay.players[playerIdx].ready ? 1 : 0;
		char const *const toolTips[2][3] = {{_("Waiting for player"), _("Player is ready"), _("Player is downloading")}, {_("Click when ready"), _("Waiting for other players"), _("Waiting for download")}};
		unsigned images[2][3] = {{IMAGE_CHECK_OFF, IMAGE_CHECK_ON, IMAGE_CHECK_DOWNLOAD}, {IMAGE_CHECK_OFF_HI, IMAGE_CHECK_ON_HI, IMAGE_CHECK_DOWNLOAD_HI}};

		// draw 'ready' button
		bool greyedOutReady = (NetPlay.players[playerIdx].isSpectator && NetPlay.players[playerIdx].ready) || (playerIdx != selectedPlayer);
		bool freshReadyButton = (readyButton == nullptr);
		readyButton = addMultiBut(*readyButtonContainer, MULTIOP_READY_START + playerIdx, 3, 10, MULTIOP_READY_WIDTH, MULTIOP_READY_HEIGHT,
					toolTips[isMe][isReady], images[0][isReady], images[0][isReady], images[isMe][isReady], MAX_PLAYERS, (!greyedOutReady) ? 255 : 125);
		ASSERT_OR_RETURN(, readyButton != nullptr, "Failed to create ready button");
		readyButton->minClickInterval = GAME_TICKS_PER_SEC;
		readyButton->unlock();
		if (greyedOutReady && !NetPlay.isHost)
		{
			std::shared_ptr<WzMultiButton> pReadyBut_MultiButton = std::dynamic_pointer_cast<WzMultiButton>(readyButton);
			if (pReadyBut_MultiButton)
			{
				pReadyBut_MultiButton->downStateMask = WBUT_DOWN | WBUT_CLICKLOCK;
			}
			auto currentState = readyButton->getState();
			readyButton->setState(currentState | WBUT_LOCK);
		}
		if (freshReadyButton)
		{
			// must add onclick handler
			auto player = playerIdx;
			auto weakTitleUi = parentTitleUI;
			readyButton->addOnClickHandler([player, weakTitleUi](W_BUTTON& button){
				auto pButton = button.shared_from_this();
				widgScheduleTask([weakTitleUi, player, pButton] {
					auto strongTitleUI = weakTitleUi.lock();
					ASSERT_OR_RETURN(, strongTitleUI != nullptr, "Title UI is gone?");

					if (player == selectedPlayer
						&& (!NetPlay.players[player].isSpectator || !NetPlay.players[player].ready || NetPlay.isHost)) // spectators can never toggle off "ready"
					{
						// Lock the "ready" button (until the request is processed)
						pButton->setState(WBUT_LOCK);

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
							kickPlayer(player, _("The host has kicked you from the game."), ERROR_KICKED, false);
							resetReadyStatus(true);		//reset and send notification to all clients
						}
					}
				});
			});
		}

		if (!readyTextLabel)
		{
			readyTextLabel = std::make_shared<W_LABEL>();
			readyButtonContainer->attach(readyTextLabel);
			readyTextLabel->id = MULTIOP_READY_START + MAX_CONNECTED_PLAYERS + playerIdx;
		}
		readyTextLabel->setGeometry(0, 0, MULTIOP_READY_WIDTH, 17);
		readyTextLabel->setTextAlignment(WLAB_ALIGNBOTTOM);
		readyTextLabel->setFont(font_small, WZCOL_TEXT_BRIGHT);
		readyTextLabel->setString(_("READY?"));
	}

private:
	std::weak_ptr<WzMultiplayerOptionsTitleUI> parentTitleUI;
	unsigned playerIdx = 0;
	std::shared_ptr<W_BUTTON> teamButton;
	std::shared_ptr<W_BUTTON> colorButton;
	std::shared_ptr<W_BUTTON> playerInfo;
	std::shared_ptr<WIDGET> readyButtonContainer;
	std::shared_ptr<W_BUTTON> difficultyChooserButton;
	std::shared_ptr<W_BUTTON> readyButton;
	std::shared_ptr<W_LABEL> readyTextLabel;
};

// ////////////////////////////////////////////////////////////////////////////

void WzMultiplayerOptionsTitleUI::openPlayerSlotSwapChooser(uint32_t playerIndex)
{
	// remove any choosers already up
	closeAllChoosers();

	// make sure "players" view is visible
	playerDisplayView = PlayerDisplayView::Players;
	updatePlayers();

	widgRegisterOverlayScreen(psInlineChooserOverlayScreen, 1);

	WIDGET* psParent = widgGetFromID(psWScreen, FRONTEND_BACKDROP);
	ASSERT_OR_RETURN(, psParent != nullptr, "Could not find root form");
	std::weak_ptr<WIDGET> psWeakParent(psParent->shared_from_this());

	WIDGET *psInlineChooserOverlayRootForm = widgGetFromID(psInlineChooserOverlayScreen, MULTIOP_INLINE_OVERLAY_ROOT_FRM);
	ASSERT_OR_RETURN(, psInlineChooserOverlayRootForm != nullptr, "Could not find overlay root form");
	std::shared_ptr<W_FULLSCREENOVERLAY_CLICKFORM> chooserParent = std::dynamic_pointer_cast<W_FULLSCREENOVERLAY_CLICKFORM>(psInlineChooserOverlayRootForm->shared_from_this());
	auto psParentPlayersForm = (W_FORM *)widgGetFromID(psWScreen, MULTIOP_PLAYERS);
	ASSERT_OR_RETURN(, psParentPlayersForm != nullptr, "Could not find players form?");
	chooserParent->setCutoutWidget(psParentPlayersForm->shared_from_this()); // should be cleared on close
	auto titleUI = std::dynamic_pointer_cast<WzMultiplayerOptionsTitleUI>(shared_from_this());

	int textHeight = iV_GetTextLineSize(font_regular);
	int swapContextFormMargin = 1;
	int swapContextFormPadding = 4;
	int swapContextFormHeight = swapContextFormPadding + textHeight + 4 + MULTIOP_PLAYERHEIGHT + (swapContextFormPadding * 2);
	int additionalHeightForSwappingRow = swapContextFormMargin + swapContextFormHeight;

	auto aiForm = std::make_shared<WIDGET>();
	chooserParent->attach(aiForm);
	aiForm->id = MULTIOP_AI_FORM;
	aiForm->setCalcLayout([psWeakParent, additionalHeightForSwappingRow](WIDGET *psWidget) {
		if (auto psParent = psWeakParent.lock())
		{
			psWidget->setGeometry(psParent->screenPosX() + MULTIOP_PLAYERSX, psParent->screenPosY() + MULTIOP_PLAYERSY, MULTIOP_PLAYERSW, MULTIOP_PLAYERSH + additionalHeightForSwappingRow);
		}
	});
	aiForm->setTransparentToClicks(true);

	WzPlayerIndexSwapPositionRowRactory rowFactory;
	auto positionChooser = WzPositionChooser::make(playerIndex, rowFactory, titleUI);
	aiForm->attach(positionChooser);
	positionChooser->setCalcLayout([](WIDGET *psWidget) {
		psWidget->setGeometry(0, 0, MULTIOP_PLAYERSW, MULTIOP_PLAYERSH);
	});

	// Create parent form for row being swapped
	std::weak_ptr<WzPositionChooser> psWeakPositionChooser(positionChooser);
	auto swapContextForm = std::make_shared<IntFormAnimated>(false);
	positionChooser->attach(swapContextForm);
	swapContextForm->setCalcLayout([psWeakPositionChooser, swapContextFormHeight, swapContextFormMargin](WIDGET *psWidget) {
		if (auto psStrongPositionChooser = psWeakPositionChooser.lock())
		{
			psWidget->setGeometry(0, psStrongPositionChooser->y() + psStrongPositionChooser->height() + swapContextFormMargin, psStrongPositionChooser->width(), swapContextFormHeight);
		}
	});

	// Now create a dummy row for the row being swapped, and display beneath
	auto playerRow = WzPlayerRow::make(playerIndex, titleUI);
	playerRow->setCustomHitTest([](WIDGET *psWidget, int x, int y) -> bool { return false; }); // ensure clicks on this display-only row have no effect
	swapContextForm->attach(playerRow);
	playerRow->setCalcLayout([swapContextFormPadding, textHeight](WIDGET *psWidget) {
		psWidget->setGeometry(PLAYERBOX_X0, swapContextFormPadding + textHeight + 4, MULTIOP_PLAYERWIDTH, MULTIOP_PLAYERHEIGHT);
	});
	auto playerRowLabel = std::make_shared<W_LABEL>();
	playerRowLabel->setFont(font_regular, WZCOL_TEXT_BRIGHT);
	const char* playerDescription = (NetPlay.players[playerIndex].isSpectator) ? _("For Spectator:") : _("For Player:");
	playerRowLabel->setString(playerDescription);
	swapContextForm->attach(playerRowLabel);
	playerRowLabel->setCalcLayout([swapContextFormPadding, textHeight](WIDGET *psWidget) {
		psWidget->setGeometry(PLAYERBOX_X0, swapContextFormPadding, MULTIOP_PLAYERWIDTH, textHeight);
	});
	auto playerRowSwitchArrow = std::make_shared<W_LABEL>();
	playerRowSwitchArrow->setFont(font_regular, WZCOL_TEXT_MEDIUM);
	playerRowSwitchArrow->setString("\u21C5"); // ⇅
	playerRowSwitchArrow->setGeometry(0, 0, playerRowSwitchArrow->idealWidth(), playerRowSwitchArrow->idealHeight());
	swapContextForm->attach(playerRowSwitchArrow);
	playerRowSwitchArrow->setCalcLayout([swapContextFormPadding, textHeight](WIDGET *psWidget) {
		auto psParent = psWidget->parent();
		ASSERT_OR_RETURN(, psParent != nullptr, "No parent?");
		psWidget->setGeometry(psParent->width() - PLAYERBOX_X0 - psWidget->width(), swapContextFormPadding, psWidget->width(), textHeight);
	});

	// Add side text
	W_LABEL *psSideTextLabel = addSideText(psInlineChooserOverlayScreen, MULTIOP_INLINE_OVERLAY_ROOT_FRM, FRONTEND_SIDETEXT2, MULTIOP_PLAYERSX - 3, MULTIOP_PLAYERSY - 15, _("Choose Player Slot"));
	if (psSideTextLabel)
	{
		psSideTextLabel->setCalcLayout([psWeakParent](WIDGET *psWidget) {
			if (auto psParent = psWeakParent.lock())
			{
				psWidget->setGeometry(psParent->screenPosX() + MULTIOP_PLAYERSX - 3, psParent->screenPosY() + MULTIOP_PLAYERSY, MULTIOP_PLAYERSW, MULTIOP_PLAYERSH);
			}
		});
	}

	playerSlotSwapChooserUp = playerIndex;
}

void WzMultiplayerOptionsTitleUI::closePlayerSlotSwapChooser()
{
	WIDGET *psInlineChooserOverlayRootForm = widgGetFromID(psInlineChooserOverlayScreen, MULTIOP_INLINE_OVERLAY_ROOT_FRM);
	std::shared_ptr<W_FULLSCREENOVERLAY_CLICKFORM> chooserParent = std::dynamic_pointer_cast<W_FULLSCREENOVERLAY_CLICKFORM>(psInlineChooserOverlayRootForm->shared_from_this());
	if (chooserParent)
	{
		chooserParent->setCutoutWidget(nullptr);
	}

	// AiChooser / DifficultyChooser / PositionChooser / PlayerSlotSwapChooser currently use the same formID
	// Just call closeAllChoosers() for now
	closeAllChoosers();

	playerSlotSwapChooserUp = nullopt;
}

optional<uint32_t> WzMultiplayerOptionsTitleUI::playerSlotSwapChooserOpenForPlayer() const
{
	return playerSlotSwapChooserUp;
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

	std::shared_ptr<IntFormAnimated> playersForm = std::dynamic_pointer_cast<IntFormAnimated>(widgFormGetFromID(psWScreen->psForm, MULTIOP_PLAYERS));
	if (!playersForm)
	{
		widgDelete(psWScreen, MULTIOP_PLAYERS);
		playersForm = std::make_shared<IntFormAnimated>(false);
		widgetParent->attach(playersForm);
		playersForm->id = MULTIOP_PLAYERS;
		playersForm->setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
			psWidget->setGeometry(MULTIOP_PLAYERSX, MULTIOP_PLAYERSY, MULTIOP_PLAYERSW, MULTIOP_PLAYERSH);
		}));
	}

	widgDelete(psWScreen, FRONTEND_SIDETEXT2);	// del text
	W_LABEL* pPlayersLabel = addSideText(FRONTEND_SIDETEXT2, MULTIOP_PLAYERSX - 3, MULTIOP_PLAYERSY, _("PLAYERS"));
	pPlayersLabel->hide(); // hide for now

	if (players)
	{
		auto titleUI = std::dynamic_pointer_cast<WzMultiplayerOptionsTitleUI>(shared_from_this());

		// Add "players" / "spectators" tab buttons (if in multiplay mode)
		bool isMultiplayMode = NetPlay.bComms && (NetPlay.isHost || ingame.side == InGameSide::MULTIPLAYER_CLIENT);
		std::shared_ptr<WzPlayerBoxTabs> playersTabButtons = std::dynamic_pointer_cast<WzPlayerBoxTabs>(widgFormGetFromID(psWScreen->psForm, MULTIOP_PLAYERS_TABS));
		if (isMultiplayMode && !playersTabButtons)
		{
			playersTabButtons = WzPlayerBoxTabs::make(NetPlay.isHost, titleUI);
			playersTabButtons->id = MULTIOP_PLAYERS_TABS;
			playersForm->attach(playersTabButtons);
			playersTabButtons->setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
				psWidget->setGeometry(PLAYERBOX_X0 - 1, 0, MULTIOP_PLAYERSW - PLAYERBOX_X0, MULTIOP_PLAYERS_TABS_H);
			}));
		}

		ASSERT(static_cast<size_t>(MAX_CONNECTED_PLAYERS) <= NetPlay.players.size(), "Insufficient array size: %zu versus %zu", NetPlay.players.size(), (size_t)MAX_CONNECTED_PLAYERS);
		uint32_t numSlotsDisplayed = 0;
		if (playerRows.empty())
		{
			for (uint32_t i = 0; i < MAX_CONNECTED_PLAYERS; i++)
			{
				if (!ingame.localOptionsReceived)
				{
					break;
				}

				// Create player row
				auto playerRow = WzPlayerRow::make(i, titleUI);
				playersForm->attach(playerRow);
				playerRows.push_back(playerRow);
			}
		}

		for (uint32_t i = 0; i < MAX_CONNECTED_PLAYERS; i++)
		{
			if (!ingame.localOptionsReceived)
			{
				break;
			}
			if (i >= playerRows.size())
			{
				ASSERT(i < playerRows.size(), "Row widget does not currently exist");
				continue;
			}
			auto playerRow = std::dynamic_pointer_cast<WzPlayerRow>(playerRows[i]);

			switch (playerDisplayView)
			{
				case PlayerDisplayView::Players:
					if (isSpectatorOnlySlot(i))
					{
						// should not be visible
						playerRow->hide();
						continue; // skip
					}
					break;
				case PlayerDisplayView::Spectators:
					if (!isSpectatorOnlySlot(i))
					{
						// should not be visible
						playerRow->hide();
						continue; // skip
					}
					break;
			}

			if (isSpectatorOnlySlot(i)
				&& ((i >= NetPlay.players.size()) || !(NetPlay.players[i].isSpectator && NetPlay.players[i].ai == AI_OPEN)))
			{
				// the only slots displayable beyond game.maxPlayers are spectator slots
				playerRow->hide();
				continue;
			}

			uint32_t playerRowPosition = (playerDisplayView == PlayerDisplayView::Players) ? NetPlay.players[i].position : numSlotsDisplayed;
			playerRow->setGeometry(PLAYERBOX_X0, playerBoxHeight(playerRowPosition), MULTIOP_PLAYERWIDTH, MULTIOP_PLAYERHEIGHT);
			playerRow->show();
			playerRow->updateState();
			// make sure it's attached to the current player form
			if (playerRow->parent() == nullptr)
			{
				playersForm->attach(playerRow);
			}

			numSlotsDisplayed++;
		}

		std::shared_ptr<W_BUTTON> spectatorAddButton = std::dynamic_pointer_cast<W_BUTTON>(widgFormGetFromID(psWScreen->psForm, MULTIOP_ADD_SPECTATOR_SLOTS));
		if (!spectatorAddButton)
		{
			// Create a "+ Spectator Slots" button
			const int imgheight = iV_GetImageHeight(FrontImages, IMAGE_SPECTATOR) + 2;
			W_BUTINIT sAddSpecSlotInit;
			sAddSpecSlotInit.formID = MULTIOP_PLAYERS;
			sAddSpecSlotInit.id = MULTIOP_ADD_SPECTATOR_SLOTS;
			sAddSpecSlotInit.x = 7;
			sAddSpecSlotInit.y = MULTIOP_PLAYERSH - (imgheight + 4) - 4;
			sAddSpecSlotInit.width = MULTIOP_TEAMSWIDTH;
			sAddSpecSlotInit.height = imgheight;
			sAddSpecSlotInit.pTip = _("Add spectator slot");
			sAddSpecSlotInit.pDisplay = displaySpectatorAddButton;
			sAddSpecSlotInit.UserData = 0;
			W_BUTTON *pButton = widgAddButton(psWScreen, &sAddSpecSlotInit);
			if (pButton)
			{
				auto psWeakTitleUI = std::weak_ptr<WzMultiplayerOptionsTitleUI>(std::dynamic_pointer_cast<WzMultiplayerOptionsTitleUI>(shared_from_this()));
				auto onClickHandler = [psWeakTitleUI](W_BUTTON& clickedButton) {
					auto pStrongPtr = psWeakTitleUI.lock();
					ASSERT_OR_RETURN(, pStrongPtr.operator bool(), "WzMultiplayerOptionsTitleUI no longer exists");
					widgScheduleTask([psWeakTitleUI]{
						auto pStrongPtr = psWeakTitleUI.lock();
						ASSERT_OR_RETURN(, pStrongPtr.operator bool(), "WzMultiplayerOptionsTitleUI no longer exists");
						if (NETopenNewSpectatorSlot())
						{
							pStrongPtr->addPlayerBox(true);
//							resetReadyStatus(false);
						}
					});
				};
				pButton->addOnClickHandler(onClickHandler);
			}
			spectatorAddButton = (pButton) ? std::dynamic_pointer_cast<W_BUTTON>(pButton->shared_from_this()) : nullptr;
		}

		ASSERT_OR_RETURN(, spectatorAddButton != nullptr, "Unable to create or find button");
		if ((playerDisplayView == PlayerDisplayView::Spectators) && (numSlotsDisplayed < (MAX_PLAYERS_IN_GUI)) && spectatorSlotsSupported() && NetPlay.isHost)
		{
			// spectator add button should be visible
			spectatorAddButton->show();
		}
		else
		{
			spectatorAddButton->hide();
		}
	}
}

/*
 * Notify all players of host launching the game
 */
static void SendFireUp()
{
	uint32_t randomSeed = rand();  // Pick a random random seed for the synchronised random number generator.

	debug(LOG_INFO, "Sending NET_FIREUP");

	NETbeginEncode(NETbroadcastQueue(), NET_FIREUP);
	NETuint32_t(&randomSeed);
	NETend();
	printSearchPath();
	gameSRand(randomSeed);  // Set the seed for the synchronised random number generator. The clients will use the same seed.
}

// host kicks a player from a game.
void kickPlayer(uint32_t player_id, const char *reason, LOBBY_ERROR_TYPES type, bool banPlayer)
{
	ASSERT_HOST_ONLY(return);

	debug(LOG_INFO, "Kicking player %u (%s). Reason: %s", (unsigned int)player_id, getPlayerName(player_id), reason);

	// send a kick msg
	NETbeginEncode(NETbroadcastQueue(), NET_KICK);
	NETuint32_t(&player_id);
	NETstring(reason, MAX_KICK_REASON);
	NETenum(&type);
	NETend();
	NETflush();
	wzDelay(300);

	ActivityManager::instance().hostKickPlayer(NetPlay.players[player_id], type, reason);

	if (banPlayer && NetPlay.players[player_id].allocated)
	{
		addIPToBanList(NetPlay.players[player_id].IPtextAddress, NetPlay.players[player_id].name);
	}

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
		if (sender >= 0 && sender < MAX_CONNECTED_PLAYERS)
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
	editBox->setOnReturnHandler([](W_EDITBOX& widg) {
		// don't send empty lines to other players in the lobby
		auto str = widg.getString();
		if (str.isEmpty())
		{
			return;
		}

		sendRoomChatMessage(str.toUtf8().c_str());
		widg.setString("");
	});

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
		setTransparentToMouse(true);
	}

	void display(int xOffset, int yOffset) override
	{
		auto left = xOffset + x();
		auto top = yOffset + y();
		auto marginLeft = left + leftMargin;
		auto textX = marginLeft + horizontalPadding;
		auto textY = top - cachedText->aboveBase();
		bool isSpectator = (*player)->isSpectator;
		PIELIGHT bckColor = pal_RGBA(0, 0, 0, 70);
		if (!isSpectator)
		{
			bckColor = pal_GetTeamColour((*player)->colour);
		}
		pie_UniTransBoxFill(marginLeft, top, left + width(), top + height(), bckColor);
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

	int aboveBase()
	{
		return ptsAboveBase;
	}

private:
	std::shared_ptr<PlayerReference> player;
	iV_fonts font;
	WzString layoutName;
	WzCachedText cachedText;
	int32_t horizontalPadding = 3;
	int32_t leftMargin = 3;
	int32_t ptsAboveBase = 0;

	void updateLayout()
	{
		layoutName = (*player)->name;
		cachedText = WzCachedText(layoutName, font);
		ptsAboveBase = cachedText->aboveBase();
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
	{
		ASSERT(message.sender.get() != nullptr, "Null message sender?");
		paragraph->setFont(font_small);
		paragraph->setFontColour({0xc0, 0xc0, 0xc0, 0xff});
		paragraph->addText(formatLocalDateTime("%H:%M", message.time));

		auto playerNameWidget = std::make_shared<ChatBoxPlayerNameWidget>(message.sender);
		paragraph->addWidget(playerNameWidget, playerNameWidget->aboveBase());

		paragraph->setFont(font_regular);
		paragraph->setShadeColour({0, 0, 0, 0});
		bool specSender = (*message.sender)->isSpectator && !message.sender->isHost();
		paragraph->setFontColour((!specSender) ? WZCOL_WHITE : WZCOL_TEXT_MEDIUM);
		paragraph->addText(astringf(" %s", message.text.c_str()));

		break;
	}

	case RoomMessageNotify:
	default:
		paragraph->setFontColour(WZCOL_YELLOW);
		paragraph->addText(message.text);
		break;
	}

	std::string msgText = message.text;
	std::shared_ptr<PlayerReference> senderCopy = (message.type == RoomMessagePlayer) ? message.sender : nullptr;
	paragraph->addOnClickHandler([msgText, senderCopy](Paragraph& paragraph, WIDGET_KEY key) {
		// take advantage of the fact that this widget is the great-grand-child of the ChatBoxWidget
		auto psParent = paragraph.parent();
		ASSERT_OR_RETURN(, psParent != nullptr, "Expected parent?");
		auto psMessages = std::dynamic_pointer_cast<ScrollableListWidget>(psParent->parent());
		ASSERT_OR_RETURN(, psMessages != nullptr, "Expected grand-parent missing?");
		auto psChatBoxWidget = std::dynamic_pointer_cast<ChatBoxWidget>(psMessages->parent());
		ASSERT_OR_RETURN(, psChatBoxWidget != nullptr, "Expected great-grand-parent missing?");
		// display contextual menu
		psChatBoxWidget->displayParagraphContextualMenu(msgText, senderCopy);
	});

	messages->addItem(paragraph);
}

#define MENU_BUTTONS_PADDING 20

std::shared_ptr<WIDGET> ChatBoxWidget::createParagraphContextualMenuPopoverForm(std::string textToCopy, std::shared_ptr<PlayerReference> sender)
{
	// create all the buttons / option rows
	std::weak_ptr<ChatBoxWidget> weakChatBoxWidget = std::dynamic_pointer_cast<ChatBoxWidget>(shared_from_this());
	auto createOptionsButton = [weakChatBoxWidget](const WzString& text, const std::function<void (W_BUTTON& button)>& onClickFunc) -> std::shared_ptr<W_BUTTON> {
		auto button = std::make_shared<W_BUTTON>();
		button->setString(text);
		button->FontID = font_regular;
		button->displayFunction = PopoverMenuButtonDisplayFunc;
		auto cache = new PopoverMenuButtonDisplayCache();
		button->pUserData = cache;
		cache->text.setText(text, button->FontID);
		button->setOnDelete([](WIDGET *psWidget) {
			assert(psWidget->pUserData != nullptr);
			delete static_cast<PopoverMenuButtonDisplayCache *>(psWidget->pUserData);
			psWidget->pUserData = nullptr;
		});
		int minButtonWidthForText = cache->text.width();
		button->setGeometry(0, 0, minButtonWidthForText + MENU_BUTTONS_PADDING, cache->text.lineSize() + 4);

		// On click, perform the designated onClickHandler and close the popover form / overlay
		button->addOnClickHandler([weakChatBoxWidget, onClickFunc](W_BUTTON& button) {
			if (auto chatBoxWidget = weakChatBoxWidget.lock())
			{
				widgRemoveOverlayScreen(chatBoxWidget->optionsOverlayScreen);
				onClickFunc(button);
				chatBoxWidget->optionsOverlayScreen.reset();
			}
		});
		return button;
	};

	std::vector<std::shared_ptr<WIDGET>> buttons;
	buttons.push_back(createOptionsButton(_("Copy Text to Clipboard"), [weakChatBoxWidget, textToCopy](W_BUTTON& button){
		if (auto chatBoxWidget = weakChatBoxWidget.lock())
		{
			wzSetClipboardText(textToCopy.c_str());
		}
	}));
	if (sender && !sender->isDetached())
	{
		auto senderIdx = sender->originalIndex();
		if (senderIdx != selectedPlayer)
		{
			bool currentlyMuted = ingame.muteChat[senderIdx];
			std::string muteString;
			if (!currentlyMuted)
			{
				muteString = astringf(_("Mute Player: %s"), (*sender)->name);
			}
			else
			{
				muteString = astringf(_("Unmute Player: %s"), (*sender)->name);
			}
			buttons.push_back(createOptionsButton(WzString::fromUtf8(muteString), [sender, currentlyMuted](W_BUTTON& button){
				if (!sender->isDetached())
				{
					auto playerIdx = sender->originalIndex();
					setPlayerMuted(playerIdx, !currentlyMuted);
				}
			}));
		}
	}

	// determine required height for all buttons
	int totalButtonHeight = std::accumulate(buttons.begin(), buttons.end(), 0, [](int a, const std::shared_ptr<WIDGET>& b) {
		return a + b->height();
	});
	int maxButtonWidth = (*(std::max_element(buttons.begin(), buttons.end(), [](const std::shared_ptr<WIDGET>& a, const std::shared_ptr<WIDGET>& b){
		return a->width() < b->width();
	})))->width();

	auto itemsList = ScrollableListWidget::make();
	itemsList->setBackgroundColor(WZCOL_MENU_BACKGROUND);
	itemsList->setPadding({3, 4, 3, 4});
	const int itemSpacing = 4;
	itemsList->setItemSpacing(itemSpacing);
	itemsList->setGeometry(itemsList->x(), itemsList->y(), maxButtonWidth + 8, totalButtonHeight + (static_cast<int>(buttons.size()) * itemSpacing) + 6);
	for (auto& button : buttons)
	{
		itemsList->addItem(button);
	}

	return itemsList;
}

void ChatBoxWidget::displayParagraphContextualMenu(const std::string& textToCopy, const std::shared_ptr<PlayerReference>& sender)
{
	auto lockedScreen = screenPointer.lock();
	ASSERT(lockedScreen != nullptr, "The WzPlayerBoxTabs does not have an associated screen pointer?");

	// Initialize the options overlay screen
	optionsOverlayScreen = W_SCREEN::make();
	auto newRootFrm = W_FULLSCREENOVERLAY_CLICKFORM::make();
	newRootFrm->displayFunction = displayChildDropShadows;
	std::weak_ptr<W_SCREEN> psWeakOptionsOverlayScreen(optionsOverlayScreen);
	std::weak_ptr<ChatBoxWidget> psWeakChatBoxWidget = std::dynamic_pointer_cast<ChatBoxWidget>(shared_from_this());
	newRootFrm->onClickedFunc = [psWeakOptionsOverlayScreen, psWeakChatBoxWidget]() {
		if (auto psOverlayScreen = psWeakOptionsOverlayScreen.lock())
		{
			widgRemoveOverlayScreen(psOverlayScreen);
		}
		// Destroy Options overlay / overlay screen
		if (auto strongChatBoxWidget = psWeakChatBoxWidget.lock())
		{
			strongChatBoxWidget->optionsOverlayScreen.reset();
		}
	};
	newRootFrm->onCancelPressed = newRootFrm->onClickedFunc;
	optionsOverlayScreen->psForm->attach(newRootFrm);

	// Create the pop-over form
	auto optionsPopOver = createParagraphContextualMenuPopoverForm(textToCopy, sender);
	newRootFrm->attach(optionsPopOver);

	// Position the pop-over form - use current mouse position
	int popOverX0 = std::min<int>(mouseX(), screenWidth - optionsPopOver->width());
	int popOverY0 = std::min<int>(mouseY(), screenHeight - optionsPopOver->height());
	optionsPopOver->move(popOverX0, popOverY0);

	widgRegisterOverlayScreenOnTopOfScreen(optionsOverlayScreen, lockedScreen);
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
	cancelOrDismissNotificationIfTag([](const std::string& tag) {
		return (tag.rfind(SLOTTYPE_TAG_PREFIX, 0) == 0);
	});

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

		NET_clearDownloadingWZFiles();
		ingame.localJoiningInProgress = false;			// reset local flags
		ingame.localOptionsReceived = false;

		// joining and host was transferred.
		if (ingame.side == InGameSide::MULTIPLAYER_CLIENT && NetPlay.isHost)
		{
			NetPlay.isHost = false;
		}

		ActivityManager::instance().joinedLobbyQuit();
		if (parent)
		{
			changeTitleUI(parent);
		}
		else
		{
			changeTitleMode(MULTI);
		}

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
	NetPlay.players.resize(MAX_CONNECTED_PLAYERS);
}

static void resetPlayerPositions()
{
	// Reset players' positions or bad things could happen to scavenger slot

	for (unsigned int i = 0; i < MAX_CONNECTED_PLAYERS; ++i)
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
	ASSERT_OR_RETURN(, mapData != nullptr, "Invalid mapData?");
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
					wzQuit(1);
				}
			}
			game.maxPlayers = mapData->players;

			uint8_t configuredMaxPlayers = ini.value("maxPlayers", game.maxPlayers).toUInt();
			if (getHostLaunch() == HostLaunch::Autohost)
			{
				// always use the autohost config - if it specifies an invalid number of players, this is a bug in the config
				// however, maxPlayers must still be limited to MAX_PLAYERS
				ASSERT(configuredMaxPlayers < MAX_PLAYERS, "Configured maxPlayers (%" PRIu8 ") exceeds MAX_PLAYERS (%d)", configuredMaxPlayers, (int)MAX_PLAYERS);
				configuredMaxPlayers = std::min<uint8_t>(configuredMaxPlayers, MAX_PLAYERS);
				game.maxPlayers = std::max((uint8_t)1u, configuredMaxPlayers);
			}
			else
			{
				game.maxPlayers = std::min(std::max((uint8_t)1u, configuredMaxPlayers), game.maxPlayers);
			}
			game.scavengers = ini.value("scavengers", game.scavengers).toInt();
			game.alliance = ini.value("alliances", ALLIANCES_TEAMS).toInt();
			game.power = ini.value("powerLevel", game.power).toInt();
			game.base = ini.value("bases", game.base + 1).toInt() - 1;		// count from 1 like the humans do
			sstrcpy(game.name, ini.value("name").toWzString().toUtf8().c_str());
			game.techLevel = ini.value("techLevel", game.techLevel).toInt();

			// Allow making the host a spectator (for MP games)
			spectatorHost = ini.value("spectatorHost", false).toBool();

			// Allow configuring of open spectator slots (for MP games)
			unsigned int openSpectatorSlots_uint = ini.value("openSpectatorSlots", 0).toUInt();
			defaultOpenSpectatorSlots = static_cast<uint16_t>(std::min<unsigned int>(openSpectatorSlots_uint, MAX_SPECTATOR_SLOTS));

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
			game.scavengers = ini.value("scavengers", game.scavengers).toInt();
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
		if (filename == aidata[ai].js || aiValue == aidata[ai].js)
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
			setPlayerName(i, ini.value("name").toWzString().toUtf8().c_str());
		}
		else if (!NetPlay.bComms && i != selectedPlayer)
		{
			setPlayerName(i, getAIName(i));
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
		if (ini.contains("spectator") && !challengeActive)
		{
			NetPlay.players[i].isSpectator = ini.value("spectator", false).toBool();
			if (NetPlay.players[i].isSpectator)
			{
				ASSERT(!ini.contains("ai"), "Player %d is set to be a spectator, but also has the \"ai\" key specified - this is unsupported", i);
				NetPlay.players[i].ai = AI_OPEN;
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
				ASSERT(false, "Duplicate position %d at index: %d", NetPlay.players[i].position, i);
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
	ASSERT_OR_RETURN(, player1 < MAX_PLAYERS && player2 < MAX_PLAYERS, "player1 (%" PRIu32 ") & player2 (%" PRIu32 ") must be < MAX_PLAYERS", player1, player2);
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
			clearPlayerName(playerIndex);
			if (playerIndex == PLAYER_FEATURE)
			{
				NetPlay.players[playerIndex].ai = AI_CLOSED;
			}
		}
		else
		{
			NetPlay.players[playerIndex].difficulty = AIDifficulty::DEFAULT;
			NetPlay.players[playerIndex].ai = 0;

			/* ensure all players have a name in One Player Skirmish games */
			setPlayerName(playerIndex, getAIName(playerIndex));
		}
	}

	if (!bShouldResetLocal && selectedPlayerPosition < game.maxPlayers && selectedPlayer != selectedPlayerPosition) {
		std::swap(NetPlay.players[selectedPlayer].position, NetPlay.players[selectedPlayerPosition].position);
	}

	setPlayerName(selectedPlayer, sPlayer);

	if (selectedPlayer < MAX_PLAYERS)
	{
		for (unsigned playerIndex = 0; playerIndex < MAX_PLAYERS; playerIndex++)
		{
			if (getPlayerColour(playerIndex) == war_getMPcolour())
			{
				swapPlayerColours(selectedPlayer, playerIndex);
				break;
			}
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
	ASSERT_OR_RETURN(, psLevel->game >= 0 && psLevel->game < LEVEL_MAXFILES, "Invalid psLevel->game: %" PRIi16 " - may be a corrupt level load (%s; hash: %s)", psLevel->game, game.map, game.hash.toString().c_str());
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
	ASSERT_OR_RETURN(, stat >= 0, "Invalid struct stat name: %s", (name) ? name : "");
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
		debug(LOG_INFO, "Switching map: %s (builtin: %d)", (mapData->pName) ? mapData->pName : "n/a", (int)builtInMap);
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
		game.scavengers = rand() % 3;
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
	for (size_t i = 0; i < MAX_CONNECTED_PLAYERS; i++)
	{
		ingame.PingTimes[i] = 0;
		ingame.VerifiedIdentity[i] = false;
		ingame.LagCounter[i] = 0;
		ingame.lastSentPlayerDataCheck2[i].reset();
		ingame.muteChat[i] = false;
	}
	multiSyncResetAllChallenges();

	const bool bIsAutoHostOrAutoGame = getHostLaunch() == HostLaunch::Skirmish || getHostLaunch() == HostLaunch::Autohost;
	if (!hostCampaign((char*)game.name, (char*)sPlayer, spectatorHost, bIsAutoHostOrAutoGame))
	{
		displayRoomSystemMessage(_("Sorry! Failed to host the game."));
		return false;
	}

	bInActualHostedLobby = true;

	widgDelete(psWScreen, MULTIOP_REFRESH);
	widgDelete(psWScreen, MULTIOP_HOST);
	widgDelete(psWScreen, MULTIOP_FILTER_TOGGLE);

	ingame.localOptionsReceived = true;

	if (NetPlay.bComms)
	{
		auto currentSpectatorSlotInfo = SpectatorInfo::currentNetPlayState();
		auto desiredDefaultOpenSpectatorSlots = std::min<uint16_t>(defaultOpenSpectatorSlots, MAX_SPECTATOR_SLOTS);
		if (currentSpectatorSlotInfo.availableSpectatorSlots() < desiredDefaultOpenSpectatorSlots)
		{
			for (uint16_t addedSpecSlots = currentSpectatorSlotInfo.availableSpectatorSlots(); addedSpecSlots < desiredDefaultOpenSpectatorSlots; ++addedSpecSlots)
			{
				if (!NETopenNewSpectatorSlot())
				{
					break;
				}
			}
		}
	}

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

	char sPlayer_new[128] = {'\0'};

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
		sstrcpy(sPlayer_new, widgGetString(psWScreen, MULTIOP_PNAME));

		// chop to 15 chars..
		while (strlen(sPlayer_new) > 15)	// clip name.
		{
			sPlayer_new[strlen(sPlayer_new) - 1] = '\0';
		}
		removeWildcards(sPlayer_new);

		playerStats = getMultiStats(selectedPlayer); 		// retrieve current identity + stats to pass-in to loadMultiStats on name change
		if (loadMultiStats((char *)sPlayer_new, &playerStats))
		{
			sstrcpy(sPlayer, sPlayer_new);
			// update string.
			widgSetString(psWScreen, MULTIOP_PNAME, sPlayer);
			printConsoleNameChange(NetPlay.players[selectedPlayer].name, sPlayer);

			NETchangePlayerName(selectedPlayer, (char *)sPlayer);			// update if joined.
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
		}
		else
		{
			debug(LOG_ERROR, "Failed to load or create player profile: %s", sPlayer_new);
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
		// now handled in setOnReturnHandler
		break;

	case CON_CANCEL:

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
			pie_LoadBackDrop(SCREEN_RANDOMBDROP);
			addChallenges();
		}
		break;
	case MULTIOP_MAP_PREVIEW:
		loadMapPreview(true);
		break;
	default:
		break;
	}
}

/* Start a multiplayer or skirmish game */
void startMultiplayerGame()
{
	ASSERT_HOST_ONLY(return);

	wz_command_interface_output("WZEVENT: startMultiplayerGame\n");
	debug(LOG_INFO, "startMultiplayerGame");

	cancelOrDismissNotificationIfTag([](const std::string& tag) {
		return (tag.rfind(SLOTTYPE_TAG_PREFIX, 0) == 0);
	});

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
		ingame.TimeEveryoneIsInGame = nullopt;
		ingame.endTime = nullopt;
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

void handleAutoReadyRequest()
{
	if (NetPlay.isHost)
	{
		return; // no-op for host (currently)
	}

	if (multiplayIsStartingGame() || (GetGameMode() == GS_NORMAL))
	{
		return; // don't bother sending anything - the game is starting / started...
	}

	bool desiredReadyState = NetPlay.players[selectedPlayer].ready;

	// Spectators should automatically become ready as soon as necessary files are downloaded
	// and not-ready when files remain to be downloaded
	if (NetPlay.players[selectedPlayer].isSpectator)
	{
		bool haveData = NET_getDownloadingWzFiles().empty();
		desiredReadyState = haveData;
	}

	if (desiredReadyState != NetPlay.players[selectedPlayer].ready)
	{
		// Automatically set ready status
		SendReadyRequest(selectedPlayer, desiredReadyState);
	}
}

void multiClearHostRequestMoveToPlayer(uint32_t playerIdx)
{
	ASSERT_OR_RETURN(, playerIdx < MAX_CONNECTED_PLAYERS, "Invalid playerIdx: %" PRIu32 "", playerIdx);
	if (!NetPlay.isHost) { return; }
	bHostRequestedMoveToPlayers[playerIdx] = false;
}

class WzHostLobbyOperationsInterface : public HostLobbyOperationsInterface
{
public:
	virtual ~WzHostLobbyOperationsInterface() { }

public:
	virtual bool changeTeam(uint32_t player, uint8_t team) override
	{
		ASSERT_HOST_ONLY(return false);
		ASSERT_OR_RETURN(false, player < MAX_PLAYERS, "Invalid player: %" PRIu32, player);
		ASSERT_OR_RETURN(false, team < MAX_PLAYERS, "Invalid team: %" PRIu8, team);
		if (locked.teams)
		{
			debug(LOG_INFO, "Unable to change team - teams are locked");
			return false;
		}
		if (NetPlay.players[player].team == team)
		{
			// no-op - nothing to do
			return true;
		}
		::changeTeam(player, team);
		resetReadyStatus(false);
		return true;
	}
	virtual bool changePosition(uint32_t player, uint8_t position) override
	{
		ASSERT_HOST_ONLY(return false);
		ASSERT_OR_RETURN(false, player < MAX_PLAYERS, "Invalid player: %" PRIu32, player);
		ASSERT_OR_RETURN(false, position < game.maxPlayers, "Invalid position: %" PRIu8, position);
		if (locked.position)
		{
			debug(LOG_INFO, "Unable to change position - positions are locked");
			return false;
		}
		if (NetPlay.players[player].position == position)
		{
			// no-op request - nothing to do
			return true;
		}
		if (!::changePosition(player, position))
		{
			return false;
		}
		resetReadyStatus(false);
		return true;
	}
	virtual bool changeBase(uint8_t baseValue) override
	{
		ASSERT_HOST_ONLY(return false);
		ASSERT_OR_RETURN(false, baseValue == CAMP_CLEAN || baseValue == CAMP_BASE || baseValue == CAMP_WALLS, "Invalid baseValue: %" PRIu8, baseValue);
		if (locked.bases)
		{
			debug(LOG_INFO, "Unable to change bases to %" PRIu8 " - bases are locked", baseValue);
			return false;
		}
		game.base = baseValue;
		resetReadyStatus(false);
		sendOptions();
		addGameOptions(); //refresh to see the proper tech level in the map name
		return true;
	}
	virtual bool changeAlliances(uint8_t allianceValue) override
	{
		ASSERT_HOST_ONLY(return false);
		ASSERT_OR_RETURN(false, allianceValue == NO_ALLIANCES || allianceValue == ALLIANCES || allianceValue == ALLIANCES_UNSHARED || allianceValue == ALLIANCES_TEAMS, "Invalid allianceValue: %" PRIu8, allianceValue);
		if (locked.alliances)
		{
			debug(LOG_INFO, "Unable to change alliances to %" PRIu8 " - alliances are locked", allianceValue);
			return false;
		}
		game.alliance = static_cast<AllianceType>(allianceValue);
		resetReadyStatus(false);
		netPlayersUpdated = true;
		sendOptions();
		addGameOptions(); //refresh to see the proper tech level in the map name
		return true;
	}
	virtual bool changeScavengers(uint8_t scavsValue) override
	{
		ASSERT_HOST_ONLY(return false);
		ASSERT_OR_RETURN(false, scavsValue == NO_SCAVENGERS || scavsValue == SCAVENGERS || scavsValue == ULTIMATE_SCAVENGERS, "Invalid scavsValue: %" PRIu8, scavsValue);
		if (locked.scavengers)
		{
			debug(LOG_INFO, "Unable to change scavengers to %" PRIu8 " - scavengers are locked", scavsValue);
			return false;
		}
		game.scavengers = scavsValue;
		resetReadyStatus(false);
		netPlayersUpdated = true;
		sendOptions();
		addGameOptions(); //refresh to see the proper tech level in the map name
		return true;
	}
	virtual bool kickPlayer(uint32_t player, const char *reason, bool ban) override
	{
		ASSERT_HOST_ONLY(return false);
		ASSERT_OR_RETURN(false, player != NetPlay.hostPlayer, "Unable to kich the host");
		ASSERT_OR_RETURN(false, player < MAX_CONNECTED_PLAYERS, "Invalid player id: %" PRIu32, player);
		if (!NetPlay.players[player].allocated)
		{
			debug(LOG_INFO, "Unable to kick player: %" PRIu32 " - not a connected human player", player);
			return false;
		}
		std::string slotType = (NetPlay.players[player].isSpectator) ? "spectator" : "player";
		sendRoomSystemMessage((std::string("Kicking ")+slotType+": "+std::string(NetPlay.players[player].name)).c_str());
		::kickPlayer(player, reason, ERROR_KICKED, ban);
		resetReadyStatus(false);
		return true;
	}
	virtual bool movePlayerToSpectators(uint32_t player) override
	{
		ASSERT_HOST_ONLY(return false);
		ASSERT_OR_RETURN(false, player != NetPlay.hostPlayer, "Unable to move the host");
		ASSERT_OR_RETURN(false, player < MAX_PLAYER_SLOTS, "Invalid player id: %" PRIu32, player);
		if (!isHumanPlayer(player))
		{
			debug(LOG_INFO, "Unable to move player: %" PRIu32 " - not a connected human player", player);
			return false;
		}
		const char *pPlayerName = getPlayerName(player);
		std::string playerNameStr = (pPlayerName) ? pPlayerName : (std::string("[p") + std::to_string(player) + "]");
		if (!NETmovePlayerToSpectatorOnlySlot(player, true))
		{
			// failure is already logged by NETmovePlayerToSpectatorOnlySlot
			return false;
		}
		std::string msg = astringf(_("Moving %s to Spectators!"), playerNameStr.c_str());
		sendRoomSystemMessage(msg.c_str());
		resetReadyStatus(true);		//reset and send notification to all clients
		return true;
	}
	virtual bool requestMoveSpectatorToPlayers(uint32_t player) override
	{
		ASSERT_HOST_ONLY(return false);
		ASSERT_OR_RETURN(false, player != NetPlay.hostPlayer, "Unable to move the host");
		ASSERT_OR_RETURN(false, player < MAX_CONNECTED_PLAYERS, "Invalid player id: %" PRIu32, player);
		if (!isHumanPlayer(player))
		{
			debug(LOG_INFO, "Unable to move player: %" PRIu32 " - not a connected human player", player);
			return false;
		}

		if (!NetPlay.players[player].isSpectator)
		{
			debug(LOG_INFO, "Unable to move to players: %" PRIu32 " (not a spectator)", player);
			return false;
		}

		const char *pPlayerName = getPlayerName(player);
		std::string playerNameStr = (pPlayerName) ? pPlayerName : (std::string("[p") + std::to_string(player) + "]");
		// Ask the spectator if they are okay with a move from spectator -> player?
		SendPlayerSlotTypeRequest(player, false);

		std::string msg = astringf(_("Asking %s to move to Players..."), playerNameStr.c_str());
		sendRoomSystemMessage(msg.c_str());
		resetReadyStatus(true);		//reset and send notification to all clients
		return true;
	}
	virtual void quitGame(int exitCode) override
	{
		ASSERT_HOST_ONLY(return);
		auto psStrongMultiOptionsTitleUI = currentMultiOptionsTitleUI.lock();
		if (psStrongMultiOptionsTitleUI)
		{
			stopJoining(psStrongMultiOptionsTitleUI->getParentTitleUI());
		}
		wzQuit(exitCode);
	}
};

static WzHostLobbyOperationsInterface cmdInterface;

static int getBoundedMinAutostartPlayerCount()
{
	int minAutoStartPlayerCount = min_autostart_player_count();
	if (minAutoStartPlayerCount <= 0)
	{
		// there is no minimum configured
		return -1;
	}
	int numAIPlayers = 0;
	int maxPlayerSlots = 0;
	for (int j = 0; j < MAX_PLAYERS; j++)
	{
		if ((NetPlay.players[j].allocated || NetPlay.players[j].ai == AI_OPEN) && NetPlay.players[j].position < game.maxPlayers)
		{
			maxPlayerSlots++;
		}
		else if (!NetPlay.players[j].allocated && NetPlay.players[j].ai >= 0)
		{
			numAIPlayers++;
		}
	}
	if (minAutoStartPlayerCount > maxPlayerSlots)
	{
		debug(LOG_WARNING, "startplayers (%d) exceeds maxPlayerSlots (%d) - using the latter; (game.maxPlayers: %" PRIu8 ", # ai players: %d)", minAutoStartPlayerCount, maxPlayerSlots, game.maxPlayers, numAIPlayers);
		minAutoStartPlayerCount = maxPlayerSlots;
	}

	return minAutoStartPlayerCount;
}

void WzMultiplayerOptionsTitleUI::frontendMultiMessages(bool running)
{
	NETQUEUE queue;
	uint8_t type;
	bool ignoredMessage = false;

	while (NETrecvNet(&queue, &type))
	{
		if (!shouldProcessMessage(queue, type))
		{
			continue;
		}

		// Copy the message to the global one used by the new NET API
		switch (type)
		{
		case NET_FILE_REQUESTED:
			recvMapFileRequested(queue);
			break;

		case NET_FILE_PAYLOAD:
			{
				if (NetPlay.hostPlayer != queue.index)
				{
					HandleBadParam("NET_FILE_PAYLOAD given incorrect params.", 255, queue.index);
					ignoredMessage = true;
					break;
				}

				bool done = recvMapFileData(queue);
				if (running)
				{
					auto psWidget = widgGetFromID(psWScreen, MULTIOP_MAP_PREVIEW);
					if (psWidget)
					{
						((MultibuttonWidget *)psWidget)->enable(done);  // turn preview button on or off
					}
				}
				// spectators should automatically become ready as soon as necessary files are downloaded
				// and not-ready when files remain to be downloaded
				handleAutoReadyRequest();
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
				auto &pWzFiles = NetPlay.players[queue.index].wzFiles;
				if (pWzFiles == nullptr)
				{
					ASSERT(false, "Null wzFiles (player: %" PRIu8 ")", queue.index);
					break;
				}
				auto &wzFiles = *pWzFiles;
				wzFiles.erase(std::remove_if(wzFiles.begin(), wzFiles.end(), [&](WZFile const &file) { return file.hash == hash; }), wzFiles.end());
			}
			break;

		case NET_OPTIONS:					// incoming options file.
		{
			if (NetPlay.hostPlayer != queue.index)
			{
				HandleBadParam("NET_OPTIONS should be sent by host", 255, queue.index);
				ignoredMessage = true;
				break;
			}
			if (!recvOptions(queue))
			{
				// supplied NET_OPTIONS are not valid
				setLobbyError(ERROR_INVALID);
				stopJoining(std::make_shared<WzMsgBoxTitleUI>(WzString(_("Disconnected from host:")), WzString(_("Host supplied invalid options")), parent));
				break;
			}
			bInActualHostedLobby = true;
			ingame.localOptionsReceived = true;

			handleAutoReadyRequest();

			if (std::dynamic_pointer_cast<WzMultiplayerOptionsTitleUI>(wzTitleUICurrent))
			{
				addGameOptions();
				disableMultiButs();
				addChatBox();
			}
			break;
		}

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
				bool allowStart = true;
				int minAutoStartPlayerCount = getBoundedMinAutostartPlayerCount();
				if (minAutoStartPlayerCount > 0)
				{
					int playersPresent = 0;
					for (int j = 0; j < MAX_PLAYERS; j++)
					{
						if (!NetPlay.players[j].allocated)
						{
							continue;
						}
						playersPresent++;
					}
					if (playersPresent < minAutoStartPlayerCount)
					{
						allowStart = false;
					}
				}
				if (allowStart)
				{
					startMultiplayerGame();
				}
				else
				{
					std::string msg = astringf("Game will not start until there are %d players.", minAutoStartPlayerCount);
					sendRoomSystemMessage(msg.c_str());
				}
			}
			break;

		case NET_PING:						// diagnostic ping msg.
			recvPing(queue);
			break;

		case NET_PLAYER_DROPPED:		// remote player got disconnected
			{
				uint32_t player_id = MAX_CONNECTED_PLAYERS;

				resetReadyStatus(false);

				NETbeginDecode(queue, NET_PLAYER_DROPPED);
				{
					NETuint32_t(&player_id);
				}
				NETend();

				if (player_id >= MAX_CONNECTED_PLAYERS)
				{
					debug(LOG_INFO, "** player %u has dropped - huh?", player_id);
					break;
				}

				if (whosResponsible(player_id) != queue.index && queue.index != NetPlay.hostPlayer)
				{
					HandleBadParam("NET_PLAYER_DROPPED given incorrect params.", player_id, queue.index);
					break;
				}

				debug(LOG_INFO, "** player %u has dropped!", player_id);

				MultiPlayerLeave(player_id);		// get rid of their stuff
				NET_InitPlayer(player_id, false);           // sets index player's array to false
				NETsetPlayerConnectionStatus(CONNECTIONSTATUS_PLAYER_DROPPED, player_id);
				if (player_id < MAX_PLAYERS)
				{
					playerVotes[player_id] = 0;
				}
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
			cancelOrDismissNotificationIfTag([](const std::string& tag) {
				return (tag.rfind(SLOTTYPE_TAG_PREFIX, 0) == 0);
			});
			if (NetPlay.hostPlayer != queue.index)
			{
				HandleBadParam("NET_FIREUP given incorrect params.", 255, queue.index);
				break;
			}
			debug(LOG_INFO, "NET_FIREUP was received ...");
			if (ingame.localOptionsReceived)
			{
				uint32_t randomSeed = 0;
				NETbeginDecode(queue, NET_FIREUP);
				NETuint32_t(&randomSeed);
				NETend();

				gameSRand(randomSeed);  // Set the seed for the synchronised random number generator, using the seed given by the host.

				debug(LOG_NET, "& local Options Received (MP game)");
				ingame.TimeEveryoneIsInGame = nullopt;			// reset time
				ingame.endTime = nullopt;
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
				uint32_t player_id = MAX_CONNECTED_PLAYERS;
				char reason[MAX_KICK_REASON];
				LOBBY_ERROR_TYPES KICK_TYPE = ERROR_NOERROR;

				NETbeginDecode(queue, NET_KICK);
				NETuint32_t(&player_id);
				NETstring(reason, MAX_KICK_REASON);
				NETenum(&KICK_TYPE);
				NETend();

				if (player_id >= MAX_CONNECTED_PLAYERS)
				{
					debug(LOG_ERROR, "NET_KICK message with invalid player_id: (%" PRIu32")", player_id);
					break;
				}

				if (player_id < MAX_PLAYERS)
				{
					playerVotes[player_id] = 0;
				}

				if (player_id == NetPlay.hostPlayer)
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
					std::string kickReasonStr = reason;
					size_t maxLinePos = nthOccurrenceOfChar(kickReasonStr, '\n', 10);
					if (maxLinePos != std::string::npos)
					{
						kickReasonStr = kickReasonStr.substr(0, maxLinePos);
					}
					stopJoining(std::make_shared<WzMsgBoxTitleUI>(WzString(_("You have been kicked: ")), WzString::fromUtf8(kickReasonStr), parent));
					debug(LOG_INFO, "You have been kicked, because %s ", kickReasonStr.c_str());
					displayKickReasonPopup(kickReasonStr.c_str());
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
			stopJoining(std::make_shared<WzMsgBoxTitleUI>(WzString(_("Connection lost:")), WzString(_("No connection to host.")), parent));
			debug(LOG_NET, "The host has quit!");
			setLobbyError(ERROR_HOSTDROPPED);
			break;

		case NET_TEXTMSG:					// Chat message
			if (ingame.localOptionsReceived)
			{
				NetworkTextMessage message;
				if (message.receive(queue)) {

					if (message.sender < 0 || !isPlayerMuted(message.sender))
					{
						displayRoomMessage(buildMessage(message.sender, message.text));
						audio_PlayTrack(FE_AUDIO_MESSAGEEND);
					}

					if (lobby_slashcommands_enabled())
					{
						processChatLobbySlashCommands(message, cmdInterface);
					}
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
			if (!NetPlay.isHost && !NetPlay.players[selectedPlayer].isSpectator)
			{
				setupVoteChoice();
			}
			break;

		case NET_PLAYER_SLOTTYPE_REQUEST:
			{
				// Note: Because this may trigger a player index + queue swap,
				// it handles NETpop(queue) itself and we must immediately continue to the next loop iteration
				// i.e. Don't use `queue` after recvPlayerSlotTypeRequestAndPop is called
				recvPlayerSlotTypeRequestAndPop(*this, queue);
				continue; // loop for next queue that has a message
			}
		case NET_PLAYER_SWAP_INDEX:
			{
				auto result = recvSwapPlayerIndexes(queue, parent);
				if (result != SwapPlayerIndexesResult::SUCCESS)
				{
					ignoredMessage = true;
					if (result == SwapPlayerIndexesResult::ERROR_HOST_MADE_US_PLAYER_WITHOUT_PERMISSION)
					{
						// Leave the badly behaved (likely modified) host!
						sendRoomChatMessage(_("The host moved me to Players, but I never gave permission for this change. Bye!"));
						debug(LOG_INFO, "Leaving game because host moved us to Players, but we never gave permission.");
						stopJoining(std::make_shared<WzMsgBoxTitleUI>(WzString(_("Disconnected from host:")), WzString(_("The host tried to move us to Players, but we never gave permission.")), parent));
						setLobbyError(ERROR_HOSTDROPPED);
						return;
					}
				}
				break;
			}

		// Note: NET_PLAYER_SWAP_INDEX_ACK is handled in netplay

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
		char sPlayer_new[128] = {'\0'};
		if (runMultiRequester(id, &id, &sTemp, &mapData, &isHoverPreview))
		{
			switch (id)
			{
			case MULTIOP_PNAME:
				sstrcpy(sPlayer_new, sTemp.toUtf8().c_str());
				removeWildcards((char *)sPlayer_new);

				if (loadMultiStats((char *)sPlayer_new, &playerStats))
				{
					sstrcpy(sPlayer, sPlayer_new);
					widgSetString(psWScreen, MULTIOP_PNAME, sTemp.toUtf8().c_str());
					printConsoleNameChange(NetPlay.players[selectedPlayer].name, sPlayer);
					NETchangePlayerName(selectedPlayer, (char *)sPlayer);
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
				}
				else
				{
					debug(LOG_ERROR, "Failed to load or create player profile: %s", sPlayer_new);
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
					debug(LOG_INFO, "Switching map: %s (builtin: %d)", (mapData->pName) ? mapData->pName : "n/a", (int)builtInMap);

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
		cancelOrDismissNotificationIfTag([](const std::string& tag) {
			return (tag.rfind(SLOTTYPE_TAG_PREFIX, 0) == 0);
		});
		changeTitleUI(std::make_shared<WzMsgBoxTitleUI>(WzString(_("Connection lost:")), WzString(_("The host has quit.")), parent));
		pie_LoadBackDrop(SCREEN_RANDOMBDROP);
	}

	return TITLECODE_CONTINUE;
}

WzMultiplayerOptionsTitleUI::WzMultiplayerOptionsTitleUI(std::shared_ptr<WzTitleUI> parent)
	: parent(parent)
	, inlineChooserUp(-1)
	, aiChooserUp(-1)
	, difficultyChooserUp(-1)
{
}

WzMultiplayerOptionsTitleUI::~WzMultiplayerOptionsTitleUI()
{
	closeMultiRequester();
	widgRemoveOverlayScreen(psInlineChooserOverlayScreen);
	closeAllChoosers();
	bInActualHostedLobby = false;
}

void WzMultiplayerOptionsTitleUI::screenSizeDidChange(unsigned int oldWidth, unsigned int oldHeight, unsigned int newWidth, unsigned int newHeight)
{
	// NOTE: To properly support resizing the inline overlay screen based on underlying screen layer recalculations
	// frontendScreenSizeDidChange() should be called after intScreenSizeDidChange() in gameScreenSizeDidChange()
}

static void printHostHelpMessagesToConsole()
{
	char buf[512] = { '\0' };
	if (challengeActive)
	{
		ssprintf(buf, "%s", _("Hit the ready box to begin your challenge!"));
		displayRoomNotifyMessage(buf);
	}
	else if (!NetPlay.isHost)
	{
		ssprintf(buf, "%s", _("Press the start hosting button to begin hosting a game."));
		displayRoomNotifyMessage(buf);
	}
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
			ssprintf(buf, _("UPnP detection disabled by user. Autoconfig of port %d will not happen."), NETgetGameserverPort());
			displayRoomNotifyMessage(buf);
		}
	}
}

void calcBackdropLayoutForMultiplayerOptionsTitleUI(WIDGET *psWidget)
{
	auto height = screenHeight - 80;
	CLIP(height, HIDDEN_FRONTEND_HEIGHT, HIDDEN_FRONTEND_WIDTH);

	psWidget->setGeometry(
		((screenWidth - HIDDEN_FRONTEND_WIDTH) / 2),
		((screenHeight - height) / 2),
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
		currentMultiOptionsTitleUI = std::dynamic_pointer_cast<WzMultiplayerOptionsTitleUI>(shared_from_this());
		playerDisplayView = PlayerDisplayView::Players;
		bRequestedSelfMoveToPlayers = false;
		bHostRequestedMoveToPlayers.resize(MAX_CONNECTED_PLAYERS, false);
		playerRows.clear();
		initKnownPlayers();
		resetPlayerConfiguration(true);
		memset(&locked, 0, sizeof(locked));
		spectatorHost = false;
		defaultOpenSpectatorSlots = war_getMPopenSpectatorSlots();
		loadMapChallengeAndPlayerSettings(true);
		game.isMapMod = false;
		game.isRandom = false;
		game.mapHasScavengers = true; // FIXME, should default to false

		inlineChooserUp = -1;
		aiChooserUp = -1;
		difficultyChooserUp = -1;
		playerSlotSwapChooserUp = nullopt;

		const std::string notificationIdentifierPrefix = SLOTTYPE_NOTIFICATION_ID_PREFIX;
		removeNotificationPreferencesIf([&notificationIdentifierPrefix](const std::string &uniqueNotificationIdentifier) -> bool {
			bool hasPrefix = (strncmp(uniqueNotificationIdentifier.c_str(), notificationIdentifierPrefix.c_str(), notificationIdentifierPrefix.size()) == 0);
			return hasPrefix;
		});

		// Initialize the inline chooser overlay screen
		psInlineChooserOverlayScreen = W_SCREEN::make();
		auto newRootFrm = W_FULLSCREENOVERLAY_CLICKFORM::make(MULTIOP_INLINE_OVERLAY_ROOT_FRM);
		std::weak_ptr<WzMultiplayerOptionsTitleUI> psWeakTitleUI = std::dynamic_pointer_cast<WzMultiplayerOptionsTitleUI>(shared_from_this());
		newRootFrm->onClickedFunc = [psWeakTitleUI]() {
			widgScheduleTask([psWeakTitleUI]{
				auto psTitleUI = psWeakTitleUI.lock();
				ASSERT_OR_RETURN(, psTitleUI != nullptr, "Title UI is null");
				psTitleUI->closeAllChoosers(); // this also removes the overlay screen
				psTitleUI->updatePlayers();
			});
		};
		newRootFrm->onCancelPressed = newRootFrm->onClickedFunc;
		psInlineChooserOverlayScreen->psForm->attach(newRootFrm);

		ingame.localOptionsReceived = false;

		PLAYERSTATS	playerStats;

		loadMultiStats((char*)sPlayer, &playerStats);
		setMultiStats(selectedPlayer, playerStats, true); // just set this locally so that the identity is cached, if changing name
		lookupRatingAsync(selectedPlayer);

		/* Entering the first time with challenge, immediately start the host */
		if (challengeActive && !startHost())
		{
			debug(LOG_ERROR, "Failed to host the challenge.");
			return;
		}
	}

	loadMapPreview(false);

	const bool hostOrSingle = ingame.side == InGameSide::HOST_OR_SINGLEPLAYER;
	/* Re-entering or entering without a challenge */
	if (bReenter || !challengeActive)
	{
		addPlayerBox(false);
		addGameOptions();
		addChatBox(bReenter);

		if (hostOrSingle)
		{
			printHostHelpMessagesToConsole();
		}
	}
	if (!bReenter && challengeActive && hostOrSingle)
	{
		printHostHelpMessagesToConsole(); // Print the challenge text the first time
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

std::shared_ptr<WzTitleUI> WzMultiplayerOptionsTitleUI::getParentTitleUI()
{
	return parent;
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

	ASSERT_OR_RETURN(, i < MAX_CONNECTED_PLAYERS, "Index (%" PRIu32 ") out of bounds", i);

	drawBoxForPlayerInfoSegment(i, x, y, psWidget->width(), psWidget->height());

	if (!NetPlay.players[i].isSpectator)
	{
		if (alliancesSetTeamsBeforeGame(game.alliance))
		{
			ASSERT_OR_RETURN(, NetPlay.players[i].team >= 0 && NetPlay.players[i].team < MAX_PLAYERS, "Team index out of bounds");
			iV_DrawImage(FrontImages, IMAGE_TEAM0 + NetPlay.players[i].team, x + 1, y + 8);
		}
		else
		{
			// TODO: Maybe display something else here to signify "no team, FFA"
		}
	}
	else
	{
		iV_DrawImage(FrontImages, IMAGE_SPECTATOR, x + 1, y + 8);
	}
}

void displaySpectatorAddButton(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset)
{
	int x = xOffset + psWidget->x();
	int y = yOffset + psWidget->y();

	drawBlueBox_SpectatorOnly(x, y, psWidget->width(), psWidget->height());
	iV_DrawImage(FrontImages, IMAGE_SPECTATOR, x + 2, y + 1);
}

static void displayAi(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset)
{
	// Any widget using displayAi must have its pUserData initialized to a (DisplayAICache*)
	assert(psWidget->pUserData != nullptr);
	DisplayAICache& cache = *static_cast<DisplayAICache *>(psWidget->pUserData);

	const int x = xOffset + psWidget->x();
	const int y = yOffset + psWidget->y();
	const int j = psWidget->UserData;

	drawBlueBox(x, y, psWidget->width(), psWidget->height());
	WzString displayText;
	PIELIGHT textColor = WZCOL_FORM_TEXT;
	if (j >= 0)
	{
		if (j < aidata.size())
		{
			displayText = aidata[j].name;
		}
		else
		{
			debug(LOG_ERROR, "Invalid aidata index: %d; (num AIs: %zu)", j, aidata.size());
		}
	}
	else
	{
		const int id = psWidget->id;
		switch (id)
		{
		case MULTIOP_AI_CLOSED:
			displayText = _("Closed");
			break;
		case MULTIOP_AI_OPEN:
			displayText = _("Open");
			break;
		case MULTIOP_AI_SPECTATOR:
			displayText = _("Spectator");
			break;
		default:
			debug(LOG_ERROR, "Unexpected button id: %d", id);
			break;
		}
	}
	cache.wzText.setText(displayText, font_regular);
	cache.wzText.render(x + 10, y + 22, textColor);
}

static int difficultyIcon(int difficulty)
{
	switch (difficulty)
	{
	case static_cast<int>(AIDifficulty::EASY): return IMAGE_EASY;
	case static_cast<int>(AIDifficulty::MEDIUM): return IMAGE_MEDIUM;
	case static_cast<int>(AIDifficulty::HARD): return IMAGE_HARD;
	case static_cast<int>(AIDifficulty::INSANE): return IMAGE_INSANE;
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

	drawBoxForPlayerInfoSegment(j, x, y, psWidget->width(), psWidget->height());
	if (downloadProgress != 100)
	{
		char progressString[MAX_STR_LENGTH];
		ssprintf(progressString, j != selectedPlayer ? _("Sending Map: %u%% ") : _("Map: %u%% downloaded"), downloadProgress);
		cache.wzMainText.setText(progressString, font_regular);
		cache.wzMainText.render(x + 5, y + 22, WZCOL_FORM_TEXT);
		cache.fullMainText = progressString;
		return;
	}
	else if (ingame.localOptionsReceived && NetPlay.players[j].allocated)					// only draw if real player!
	{
		std::string name = NetPlay.players[j].name;

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
			cache.wzMainText.setText(WzString::fromUtf8(name), font_regular);
			cache.fullMainText = name;
		}
		WzString subText;
		if (j == NetPlay.hostPlayer && NetPlay.bComms)
		{
			subText += _("HOST");
		}
		if (NetPlay.bComms && j != selectedPlayer)
		{
			char buf[250] = {'\0'};

			// show "actual" ping time
			ssprintf(buf, "%s%s: ", subText.isEmpty() ? "" : ", ", _("Ping"));
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

		const PLAYERSTATS& stat = getMultiStats(j);
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
		cache.wzMainText.render(x + nameX, y + 22 - H*!subText.isEmpty() - H*(ar.valid && !ar.elo.empty()), colour);
		if (!subText.isEmpty())
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
		if (ar.level > 0 && ar.level < ARRAY_SIZE(levelImgs))
		{
			iV_DrawImage(IntImages, levelImgs[ar.level], x + 24, y + 15);
		}

		if (!ar.elo.empty())
		{
			cache.wzEloText.setText(WzString::fromUtf8(ar.elo), font_small);
			cache.wzEloText.render(x + nameX, y + 28 + H*!subText.isEmpty(), WZCOL_TEXT_BRIGHT);
		}
	}
	else	// AI
	{
		char aitext[100];

		if (NetPlay.players[j].ai >= 0)
		{
			iV_DrawImage(FrontImages, IMAGE_PLAYER_PC, x, y + 11);
		}

		// challenges may use custom AIs that are not in aidata and set to 127
		if (!challengeActive)
		{
		    ASSERT_OR_RETURN(, NetPlay.players[j].ai < (int)aidata.size(), "Uh-oh, AI index out of bounds");
		}

		PIELIGHT textColor = WZCOL_FORM_TEXT;
		switch (NetPlay.players[j].ai)
		{
		case AI_OPEN:
			if (!NetPlay.players[j].isSpectator)
			{
				sstrcpy(aitext, _("Open"));
			}
			else
			{
				sstrcpy(aitext, _("Spectator"));
				textColor.byte.a = 220;
			}
			break;
		case AI_CLOSED: sstrcpy(aitext, _("Closed")); break;
		default: sstrcpy(aitext, getPlayerName(j)); break;
		}
		cache.wzMainText.setText(aitext, font_regular);
		cache.wzMainText.render(x + nameX, y + 22, textColor);
		cache.fullMainText = aitext;
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

void displayColour(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset)
{
	const int x = xOffset + psWidget->x();
	const int y = yOffset + psWidget->y();
	const int j = psWidget->UserData;

	drawBoxForPlayerInfoSegment(j, x, y, psWidget->width(), psWidget->height());
	if (!NetPlay.players[j].fileSendInProgress() && (isHumanPlayer(j) || NetPlay.players[j].difficulty != AIDifficulty::DISABLED) && !NetPlay.players[j].isSpectator)
	{
		int player = getPlayerColour(j);
		STATIC_ASSERT(MAX_PLAYERS <= 16);
		iV_DrawImageTc(FrontImages, IMAGE_PLAYERN, IMAGE_PLAYERN_TC, x + 3, y + 9, pal_GetTeamColour(player));
		FactionID faction = NetPlay.players[j].faction;
		iV_DrawImageFileAnisotropic(FrontImages, factionIcon(faction), x, y, Vector2f(11, 9));
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

static void drawBlueBox_Spectator(UDWORD x, UDWORD y, UDWORD w, UDWORD h)
{
	// Match drawBlueBox behavior
	x -= 1;
	y -= 1;
	w += 2;
	h += 2;
	PIELIGHT backgroundClr = WZCOL_FORM_DARK;
	backgroundClr.byte.a = 200;
	pie_BoxFill(x, y, x + w, y + h, WZCOL_MENU_BORDER);
	pie_UniTransBoxFill(x + 1, y + 1, x + w - 1, y + h - 1, backgroundClr);
}

static void drawBlueBox_SpectatorOnly(UDWORD x, UDWORD y, UDWORD w, UDWORD h)
{
	// Match drawBlueBox behavior
	x -= 1;
	y -= 1;
	w += 2;
	h += 2;
	PIELIGHT backgroundClr = WZCOL_FORM_DARK;
	backgroundClr.byte.a = 175;
//	PIELIGHT borderClr = pal_RGBA(30, 30, 30, 120);
//	pie_UniTransBoxFill(x, y, x + w, y + h, borderClr);
//	pie_UniTransBoxFill(x + 1, y + 1, x + w - 1, y + h - 1, backgroundClr);
	pie_UniTransBoxFill(x, y, x + w, y + h, backgroundClr);
}

// ////////////////////////////////////////////////////////////////////////////
// Display (not-blue) box for spectators
void intDisplayFeBox_Spectator(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset)
{
	int x = xOffset + psWidget->x();
	int y = yOffset + psWidget->y();
	int w = psWidget->width();
	int h = psWidget->height();

	drawBlueBox_Spectator(x, y, w, h);
}
void intDisplayFeBox_SpectatorOnly(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset)
{
	int x = xOffset + psWidget->x();
	int y = yOffset + psWidget->y();
	int w = psWidget->width();
	int h = psWidget->height();

	drawBlueBox_SpectatorOnly(x, y, w, h);
}

static inline void drawBoxForPlayerInfoSegment(UDWORD playerIdx, UDWORD x, UDWORD y, UDWORD w, UDWORD h)
{
	ASSERT(playerIdx < NetPlay.players.size(), "Invalid playerIdx: %zu", static_cast<size_t>(playerIdx));
	if (playerIdx >= NetPlay.players.size() || !NetPlay.players[playerIdx].isSpectator)
	{
		drawBlueBox(x, y, w, h);
	}
	else
	{
		if (!isSpectatorOnlySlot(playerIdx))
		{
			drawBlueBox_Spectator(x, y, w, h);
		}
		else
		{
			drawBlueBox_SpectatorOnly(x, y, w, h);
		}
	}
}

static void displayReadyBoxContainer(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset)
{
	const int x = xOffset + psWidget->x();
	const int y = yOffset + psWidget->y();
	const int j = psWidget->UserData;

	drawBoxForPlayerInfoSegment(j, x, y, psWidget->width(), psWidget->height());
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

AtlasImage mpwidgetGetFrontHighlightImage(AtlasImage image)
{
	if (image.isNull())
	{
		return AtlasImage();
	}
	switch (image.width())
	{
	case 30: return AtlasImage(FrontImages, IMAGE_HI34);
	case 60: return AtlasImage(FrontImages, IMAGE_HI64);
	case 19: return AtlasImage(FrontImages, IMAGE_HI23);
	case 27: return AtlasImage(FrontImages, IMAGE_HI31);
	case 35: return AtlasImage(FrontImages, IMAGE_HI39);
	case 37: return AtlasImage(FrontImages, IMAGE_HI41);
	case 56: return AtlasImage(FrontImages, IMAGE_HI56);
	}
	return AtlasImage();
}

void WzMultiButton::display(int xOffset, int yOffset)
{
	int x0 = xOffset + x();
	int y0 = yOffset + y();
	AtlasImage hiToUse(nullptr, 0);

	// FIXME: This seems to be a way to conserve space, so you can use a
	// transparent icon with these edit boxes.
	// hack for multieditbox
	if (imNormal.id == IMAGE_EDIT_MAP || imNormal.id == IMAGE_EDIT_GAME || imNormal.id == IMAGE_EDIT_PLAYER
	    || imNormal.id == IMAGE_LOCK_BLUE || imNormal.id == IMAGE_UNLOCK_BLUE || drawBlueBorder.value_or(false))
	{
		if (drawBlueBorder.value_or(true)) // if drawBlueBox is explicitly set to false, don't draw the box
		{
			::drawBlueBox(x0 - 2, y0 - 2, height(), height());  // box on end.
		}
	}

	// evaluate auto-frame
	bool highlight = (getState() & WBUT_HIGHLIGHT) != 0;

	// evaluate auto-frame
	if (doHighlight == 1 && highlight)
	{
		hiToUse = mpwidgetGetFrontHighlightImage(imNormal);
	}

	bool down = (getState() & downStateMask) != 0;
	bool grey = (getState() & greyStateMask) != 0;

	AtlasImage toDraw[3];
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
		AtlasImage tcImage(toDraw[n].images, toDraw[n].id + 1);
		if (tc == MAX_PLAYERS)
		{
			iV_DrawImageImage(toDraw[n], x0, y0, alpha);
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

void updateMultiBut(std::shared_ptr<WzMultiButton> button, UDWORD width, UDWORD height, const char *tipres, UDWORD norm, UDWORD down, UDWORD hi, unsigned tc, uint8_t alpha = 255)
{
	button->setGeometry(button->x(), button->y(), width, height);
	button->setTip((tipres != nullptr) ? std::string(tipres) : std::string());
	button->imNormal = AtlasImage(FrontImages, norm);
	button->imDown = AtlasImage(FrontImages, down);
	button->doHighlight = hi;
	button->tc = tc;
	button->alpha = alpha;
}

std::shared_ptr<WzMultiButton> makeMultiBut(UDWORD id, UDWORD width, UDWORD height, const char *tipres, UDWORD norm, UDWORD down, UDWORD hi, unsigned tc, uint8_t alpha)
{
	auto button = std::make_shared<WzMultiButton>();
	button->id = id;
	updateMultiBut(button, width, height, tipres, norm, down, hi, tc, alpha);
	return button;
}

/////////////////////////////////////////////////////////////////////////////////////////
std::shared_ptr<W_BUTTON> addMultiBut(WIDGET &parent, UDWORD id, UDWORD x, UDWORD y, UDWORD width, UDWORD height, const char *tipres, UDWORD norm, UDWORD down, UDWORD hi, unsigned tc, uint8_t alpha)
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
	button->move(x, y);
	updateMultiBut(button, width, height, tipres, norm, down, hi, tc, alpha);
	return button;
}

std::shared_ptr<W_BUTTON> addMultiBut(const std::shared_ptr<W_SCREEN> &screen, UDWORD formid, UDWORD id, UDWORD x, UDWORD y, UDWORD width, UDWORD height, const char *tipres, UDWORD norm, UDWORD down, UDWORD hi, unsigned tc, uint8_t alpha)
{
	WIDGET *pWidget = widgGetFromID(screen, formid);
	ASSERT(pWidget != nullptr, "Unable to find formID: %" PRIu32, formid);
	return addMultiBut(*pWidget, id, x, y, width, height, tipres, norm, down, hi, tc, alpha);
}

/* Returns true if the multiplayer game can start (i.e. all players are ready) */
static bool multiplayPlayersReady()
{
	bool bReady = true;
	size_t numReadyPlayers = 0;

	for (size_t player = 0; player < NetPlay.players.size(); player++)
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

	return bReady && (numReadyPlayers > 1) && (allPlayersOnSameTeam(-1) == -1);
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

void sendRoomNotifyMessage(char const *text)
{
	NetworkTextMessage message(NOTIFY_MESSAGE, text);
	displayRoomSystemMessage(text);
	message.enqueue(NETbroadcastQueue());
}

void sendRoomSystemMessageToSingleReceiver(char const *text, uint32_t receiver)
{
	ASSERT_OR_RETURN(, isHumanPlayer(receiver), "Invalid receiver: %" PRIu32 "", receiver);
	NetworkTextMessage message(SYSTEM_MESSAGE, text);
	displayRoomSystemMessage(text);
	message.enqueue(NETnetQueue(receiver));
}

static void sendRoomChatMessage(char const *text)
{
	NetworkTextMessage message(selectedPlayer, text);
	displayRoomMessage(RoomMessage::player(selectedPlayer, text));
	if (NetPlay.isHost)
	{
		// Always allow the host to execute lobby slash commands
		if (processChatLobbySlashCommands(message, cmdInterface))
		{
			// consume the message
			return;
		}
	}
	if (strncmp(message.text, LOBBY_COMMAND_PREFIX "hostmsg", LOBBY_COMMAND_PREFIX_LENGTH + 7) == 0)
	{
		// send the message to the host only
		message.enqueue(NETnetQueue(NetPlay.hostPlayer));
	}
	else
	{
		message.enqueue(NETbroadcastQueue());
	}
}

static int numSlotsToBeDisplayed()
{
	ASSERT(static_cast<size_t>(MAX_CONNECTED_PLAYERS) <= NetPlay.players.size(), "Insufficient array size: %zu versus %zu", NetPlay.players.size(), (size_t)MAX_CONNECTED_PLAYERS);
	int numSlotsToBeDisplayed = 0;
	for (int i = 0; i < MAX_CONNECTED_PLAYERS; i++)
	{
		if (isSpectatorOnlySlot(i)
				 && ((i >= NetPlay.players.size()) || !(NetPlay.players[i].isSpectator && NetPlay.players[i].ai == AI_OPEN)))
		{
			// the only slots displayable beyond maxPlayers are spectator-only slots that are enabled
			continue;
		}
		numSlotsToBeDisplayed++;
	}
	return numSlotsToBeDisplayed;
}

static inline bool spectatorSlotsSupported()
{
	return NetPlay.bComms;
}

// NOTE: Pass in the index in the NetPlay.players array
static inline bool isSpectatorOnlySlot(UDWORD playerIdx)
{
	ASSERT_OR_RETURN(false, playerIdx < NetPlay.players.size(), "Invalid playerIdx: %" PRIu32 "", playerIdx);
	return playerIdx >= MAX_PLAYERS || NetPlay.players[playerIdx].position >= game.maxPlayers;
}

//// NOTE: Pass in NetPlay.players[i].position
//static inline bool isSpectatorOnlyPosition(UDWORD playerPosition)
//{
//	return playerPosition >= game.maxPlayers;
//}

// MARK: -

#include "version.h"
#include "lib/framework/file.h"

inline void to_json(nlohmann::json& j, const Sha256& p) {
	j = p.toString();
}

inline void from_json(const nlohmann::json& j, Sha256& p) {
	std::string str = j.get<std::string>();
	p.fromString(str);
}

inline void to_json(nlohmann::json& j, const MULTIPLAYERGAME& p) {
	j = nlohmann::json::object();
	j["type"] = p.type;
	j["scavengers"] = p.scavengers;
	j["map"] = WzString::fromUtf8(p.map).toStdString(); // Wrap this in WzString to handle invalid UTF-8 before adding to json object
	j["maxPlayers"] = p.maxPlayers;
	j["name"] = WzString::fromUtf8(p.name).toStdString(); // Wrap this in WzString to handle invalid UTF-8 before adding to json object
	j["hash"] = p.hash;
	j["modHashes"] = p.modHashes;
	j["power"] = p.power;
	j["base"] = p.base;
	j["alliance"] = p.alliance;
	j["mapHasScavengers"] = p.mapHasScavengers;
	j["isMapMod"] = p.isMapMod;
	j["isRandom"] = p.isRandom;
	j["techLevel"] = p.techLevel;
	j["inactivityMinutes"] = p.inactivityMinutes;
}

inline void from_json(const nlohmann::json& j, MULTIPLAYERGAME& p) {
	p.type = j.at("type").get<LEVEL_TYPE>();
	p.scavengers = j.at("scavengers").get<uint8_t>();
	std::string str = j.at("map").get<std::string>();
	sstrcpy(p.map, str.c_str());
	p.maxPlayers = j.at("maxPlayers").get<uint8_t>();
	str = j.at("name").get<std::string>();
	sstrcpy(p.name, str.c_str());
	p.hash = j.at("hash").get<Sha256>();
	p.modHashes = j.at("modHashes").get<std::vector<Sha256>>();
	p.power = j.at("power").get<uint32_t>();
	p.base = j.at("base").get<uint8_t>();
	p.alliance = j.at("alliance").get<uint8_t>();
	p.mapHasScavengers = j.at("mapHasScavengers").get<bool>();
	p.isMapMod = j.at("isMapMod").get<bool>();
	p.isRandom = j.at("isRandom").get<bool>();
	p.techLevel = j.at("techLevel").get<uint32_t>();
	if (j.contains("inactivityMinutes"))
	{
		p.inactivityMinutes = j.at("inactivityMinutes").get<uint32_t>();
	}
	else
	{
		// default to the old (4.2.0 beta-era) value of 4 minutes
		p.inactivityMinutes = 4;
	}
}

inline void to_json(nlohmann::json& j, const MULTISTRUCTLIMITS& p) {
	j = nlohmann::json::object();
	j["id"] = p.id;
	j["limit"] = p.limit;
}

inline void from_json(const nlohmann::json& j, MULTISTRUCTLIMITS& p) {
	p.id = j.at("id").get<uint32_t>();
	p.limit = j.at("limit").get<uint32_t>();
}

inline void to_json(nlohmann::json& j, const InGameSide& p) {
	j = static_cast<bool>(p);
}

inline void from_json(const nlohmann::json& j, InGameSide& p) {
	auto val = j.get<bool>();
	p = static_cast<InGameSide>(val);
}


inline void to_json(nlohmann::json& j, const MULTIPLAYERINGAME& p) {
	j = nlohmann::json::object();

//	auto joiningInProgress = nlohmann::json::array();
//	for (size_t idx = 0; idx < MAX_CONNECTED_PLAYERS; idx++)
//	{
//		joiningInProgress.push_back(p.JoiningInProgress[idx]);
//	}
//	j["JoiningInProgress"] = joiningInProgress;
	j["side"] = p.side;
	j["structureLimits"] = p.lastAppliedStructureLimits; // See applyLimitSet() for why we save `lastAppliedStructureLimits` as `structureLimits`
	j["flags"] = p.flags;
}

inline void from_json(const nlohmann::json& j, MULTIPLAYERINGAME& p) {
//	auto joiningInProgress = j["JoiningInProgress"];
//	ASSERT(joiningInProgress.is_array(), "joiningInProgress should be an array");
//	for (size_t idx = 0; idx < joiningInProgress.size(); idx++)
//	{
//		if (idx >= MAX_CONNECTED_PLAYERS)
//		{
//			debug(LOG_ERROR, "Unexpected size: %zu", joiningInProgress.size());
//			break;
//		}
//		p.JoiningInProgress[idx] = joiningInProgress[idx].get<bool>();
//	}
	p.side = j.at("side").get<InGameSide>();
	p.structureLimits = j.at("structureLimits").get<std::vector<MULTISTRUCTLIMITS>>();
	p.flags = j.at("flags").get<uint8_t>();
}

inline void to_json(nlohmann::json& j, const PLAYER& p) {

	j = nlohmann::json::object();
	j["name"] = WzString::fromUtf8(p.name).toStdString(); // Wrap this in WzString to handle invalid UTF-8 before adding to json object
	j["position"] = p.position;
	j["colour"] = p.colour;
	j["allocated"] = p.allocated;
	j["heartattacktime"] = p.heartattacktime;
	j["heartbeat"] = p.heartbeat;
	j["kick"] = p.kick;
	j["team"] = p.team;
	j["ready"] = p.ready;
	j["ai"] = p.ai;
	j["difficulty"] = static_cast<int8_t>(p.difficulty);
	//j["autoGame"] = p.autoGame; // MAYBE NOT?
	// Do not persist wzFiles
	// Do not persist IPtextAddress
	j["faction"] = static_cast<uint8_t>(p.faction);
	j["isSpectator"] = p.isSpectator;
}

inline void from_json(const nlohmann::json& j, PLAYER& p) {
	std::string str = j.at("name").get<std::string>();
	sstrcpy(p.name, str.c_str());
	p.position = j.at("position").get<int32_t>();
	p.colour = j.at("colour").get<int32_t>();
	p.allocated = j.at("allocated").get<bool>();
	p.heartattacktime = j.at("heartattacktime").get<uint32_t>();
	p.heartbeat = j.at("heartbeat").get<bool>();
	p.kick = j.at("kick").get<bool>();
	p.team = j.at("team").get<int32_t>();
	p.ready = j.at("ready").get<bool>();
	p.ai = j.at("ai").get<int8_t>();
	auto difficultyInt = j.at("difficulty").get<int8_t>();
	p.difficulty = static_cast<AIDifficulty>(difficultyInt); // TODO CHECK
	// autoGame // MAYBE NOT?
	// Do not persist wzFiles
	if (p.wzFiles)
	{
		p.wzFiles->clear();
	}
	else
	{
		ASSERT(false, "Null wzFiles?");
	}
	// Do not persist IPtextAddress
	auto factionUint = j.at("faction").get<uint8_t>();
	p.faction = static_cast<FactionID>(factionUint); // TODO CHECK
	p.isSpectator = j.at("isSpectator").get<bool>();
}

static nlohmann::json DataHashToJSON()
{
	auto jsonArray = nlohmann::json::array();
	for (size_t i = 0; i < DATA_MAXDATA; ++i)
	{
		jsonArray.push_back(DataHash[i]);
	}
	return jsonArray;
}

//static std::vector<uint32_t> DataHashFromJSON(const nlohmann::json& j)
//{
//	std::vector<uint32_t> result(DATA_MAXDATA, 0);
//
//	if (!j.is_array())
//	{
//		debug(LOG_ERROR, "Expecting an array");
//		return result;
//	}
//
//	size_t readSize = std::min<size_t>(j.size(), DATA_MAXDATA);
//	for (size_t idx = 0; idx < readSize; ++idx)
//	{
//		result[idx] = j.at(idx).get<uint32_t>();
//	}
//
//	return result;
//}

bool WZGameReplayOptionsHandler::saveOptions(nlohmann::json& object) const
{
	// random seed
	object["randSeed"] = gameRand_GetSeed();

	// Save versionString
	object["versionString"] = version_getVersionString();

	// Save DataHash here, so we can provide a warning on load (after all data / the game is loaded) in the future
	object["dataHash"] = DataHashToJSON();

	// Save `game`
	object["game"] = game;

	// Save `ingame` (? all of it? some of it?)
	object["ingame"] = ingame;

	// Save `selectedPlayer` (even though we don't actually use it on restore - for now)
	object["selectedPlayer"] = selectedPlayer;

	// ? Do not need to save alliances (?)

	// Save `NetPlay.players` -- possibly recreate NetPlay.playerReferences on load?
	object["netplay.players"] = NetPlay.players;

	// Save `NetPlay.hostPlayer`
	object["netplay.hostPlayer"] = NetPlay.hostPlayer;

	// Save `NetPlay.bComms`
	object["netplay.bComms"] = NetPlay.bComms;

//	// Save `NetPlay.isHost` (but don't load it)
//	object["netplay.isHost"] = NetPlay.isHost;

	// multistats
	auto multiStatsJson = nlohmann::json::array();
	if (!saveMultiStatsToJSON(multiStatsJson))
	{
		debug(LOG_ERROR, "Failed to save multistats info to JSON");
		return false;
	}
	object["multistats"] = multiStatsJson;

	return true;
}

constexpr PHYSFS_sint64 MAX_REPLAY_EMBEDDED_MAP_SIZE_LIMIT = 150 * 1024;

size_t WZGameReplayOptionsHandler::maximumEmbeddedMapBufferSize() const
{
	return MAX_REPLAY_EMBEDDED_MAP_SIZE_LIMIT;
}

bool WZGameReplayOptionsHandler::saveMap(EmbeddedMapData& embeddedMapData) const
{
	ASSERT_OR_RETURN(false, embeddedMapData.mapBinaryData.empty(), "Non empty output buffer?");

	// Provide "version" for this data, just in case it needs to change in the future
	embeddedMapData.dataVersion = 1;

	if (game.isMapMod)
	{
		// do not store map-mods in the replay as they are (a) deprecated and (b) may be huge
		return true;
	}

	LEVEL_DATASET *mapData = levFindDataSet(game.map, &game.hash);
	ASSERT_OR_RETURN(false, mapData != nullptr, "Unable to find map??");

	if (!mapData->realFileName)
	{
		// built-in maps don't have realFileNames for the map archives, nor do we want to save them
		return true;
	}

	PHYSFS_file *mapFileHandle = PHYSFS_openRead(mapData->realFileName);
	if (mapFileHandle == nullptr)
	{
		debug(LOG_INFO, "Unable to open for reading: %s; error: %s", mapData->realFileName, WZ_PHYSFS_getLastError());
		return false;
	}
	{
		auto free_map_handle_ref = gsl::finally([mapFileHandle] { PHYSFS_close(mapFileHandle); });  // establish exit action

		PHYSFS_sint64 filesize = PHYSFS_fileLength(mapFileHandle);
		ASSERT_OR_RETURN(false, filesize > 0, "Invalid filesize");
		if (filesize > MAX_REPLAY_EMBEDDED_MAP_SIZE_LIMIT)
		{
			// currently, an expected failure
			return true;
		}
		// resize the mapBinaryData to be able to contain the map data
		embeddedMapData.mapBinaryData.resize(static_cast<size_t>(filesize));
		auto readBytes = WZ_PHYSFS_readBytes(mapFileHandle, embeddedMapData.mapBinaryData.data(), static_cast<PHYSFS_uint32>(filesize));
		ASSERT_OR_RETURN(false, readBytes == filesize, "Read failed");
	}

	return true;
}

size_t WZGameReplayOptionsHandler::desiredBufferSize() const
{
	auto currentGameMode = ActivityManager::instance().getCurrentGameMode();
	switch (currentGameMode)
	{
		case ActivitySink::GameMode::MENUS:
			// should not happen
			break;
		case ActivitySink::GameMode::CAMPAIGN:
			// replays not currently supported
			break;
		case ActivitySink::GameMode::CHALLENGE:
		case ActivitySink::GameMode::SKIRMISH:
			return 0; // use default
		case ActivitySink::GameMode::MULTIPLAYER:
			// big games need a big buffer
			return std::numeric_limits<size_t>::max();
		default:
			debug(LOG_INFO, "Unhandled case: %u", (unsigned int)currentGameMode);
	}
	return 0;
}

bool WZGameReplayOptionsHandler::restoreOptions(const nlohmann::json& object, EmbeddedMapData&& embeddedMapData, uint32_t replay_netcodeMajor, uint32_t replay_netcodeMinor)
{
	// random seed
	uint32_t rand_seed = object.at("randSeed").get<uint32_t>();
	gameSRand(rand_seed);

	// compare version_string and throw a pop-up warning if not equal
	auto replayVersionStr = object.at("versionString").get<std::string>();
	auto expectedVersionStr = version_getVersionString();
	bool nonMatchingVersionString = replayVersionStr != expectedVersionStr;
	bool nonMatchingNetcodeVersion = !NETisCorrectVersion(replay_netcodeMajor, replay_netcodeMinor);
	if (nonMatchingVersionString || nonMatchingNetcodeVersion)
	{
		if (nonMatchingVersionString)
		{
			debug(LOG_INFO, "Version string mismatch: (replay: %s) - (current: %s)", replayVersionStr.c_str(), expectedVersionStr);
		}
		std::string mismatchVersionDescription = _("The version of Warzone 2100 used to save this replay file does not match the currently-running version.");
		mismatchVersionDescription += "\n\n";
		mismatchVersionDescription += astringf(_("Replay File Saved With: \"%s\""), replayVersionStr.c_str());
		mismatchVersionDescription += "\n";
		if (nonMatchingVersionString)
		{
			mismatchVersionDescription += astringf(_("Current Warzone 2100 Version: \"%s\""), expectedVersionStr);
		}
		else
		{
			// only a netcode mismatch - weird, but display the netcode version info
			mismatchVersionDescription += astringf("Replay File NetcodeVer: (0x%" PRIx32 ", 0x%" PRIx32 ")", replay_netcodeMajor, replay_netcodeMinor);
			mismatchVersionDescription += "\n";
			mismatchVersionDescription += astringf("Current Warzone 2100 NetcodeVer: (0x%" PRIx32 ", 0x%" PRIx32 ")", NETGetMajorVersion(), NETGetMinorVersion());
		}
		mismatchVersionDescription += "\n\n";
		mismatchVersionDescription += _("Replays should usually be played back with the same version used to save the replay.");
		mismatchVersionDescription += "\n";
		mismatchVersionDescription += _("The replay may not playback successfully, or there may be differences in the simulation.");
		wzDisplayDialog((nonMatchingVersionString) ? Dialog_Warning : Dialog_Error, _("Replay Version Mismatch"), mismatchVersionDescription.c_str());
	}

	// restore `game`
	game = object.at("game").get<MULTIPLAYERGAME>();

	// restore `ingame`
	ingame = object.at("ingame").get<MULTIPLAYERINGAME>();

	// ? Do not need to restore alliances (?)

	// restore NetPlay.players
	auto netPlayers = object.at("netplay.players");
	if (!netPlayers.is_array())
	{
		debug(LOG_ERROR, "Invalid netplay.players (not an array)");
		return false;
	}
	if (netPlayers.size() > NetPlay.players.size())
	{
		debug(LOG_ERROR, "Unsupported netplay.players size (%zu)", netPlayers.size());
		return false;
	}
	for (size_t i = 0; i < netPlayers.size(); ++i)
	{
		from_json(netPlayers.at(i), NetPlay.players[i]);
	}

	// restore NetPlay.hostPlayer
	NetPlay.hostPlayer = object.at("netplay.hostPlayer").get<uint32_t>();

	// restore `NetPlay.bComms` (?)
	NetPlay.bComms = object.at("netplay.bComms").get<bool>();

	// restore multistats
	if (!loadMultiStatsFromJSON(object.at("multistats")))
	{
		debug(LOG_ERROR, "Failed to load multistats info from JSON");
		return false;
	}

	// Now verify the info - especially that we can load the level!

	// clear out the old level list.
	levShutDown();
	levInitialise();
	rebuildSearchPath(mod_multiplay, true);	// MUST rebuild search path for the new maps we just got!
	pal_Init(); //Update palettes.
	if (!buildMapList())
	{
		debug(LOG_ERROR, "Failed to build map list");
		return false;
	}

	if (embeddedMapData.dataVersion == 1)
	{
		if (!embeddedMapData.mapBinaryData.empty())
		{
			if (embeddedMapData.mapBinaryData.size() <= MAX_REPLAY_EMBEDDED_MAP_SIZE_LIMIT)
			{
				setSpecialInMemoryMap(std::move(embeddedMapData.mapBinaryData));
			}
			else
			{
				debug(LOG_ERROR, "Embedded map data size (%zu) exceeds maximum supported size", embeddedMapData.mapBinaryData.size());
			}
		}
	}

	LEVEL_DATASET *mapData = levFindDataSet(game.map, &game.hash);
	// See if we have the map or not
	if (mapData == nullptr)
	{
		// Failed to load map
		debug(LOG_POPUP, "Missing map used for replay: \"%s\" (hash: %s)", game.map, game.hash.toString().c_str());
		return false;
	}
	// Must restore `builtInMap` (this matters for re-loading the map!) - see loadMapPreview() in multiint.cpp
	builtInMap = (mapData->realFileName == nullptr);

	for (Sha256 &hash : game.modHashes)
	{
		// TODO: Actually check the loaded mods??

		char filename[256];
		ssprintf(filename, "mods/downloads/%s", hash.toString().c_str());

		if (!PHYSFS_exists(filename))
		{
			debug(LOG_ERROR, "Downloaded mod does not exist: %s", filename);
			return false;
		}
		else if (findHashOfFile(filename) != hash)
		{
			debug(LOG_ERROR, "Downloaded mod is incomplete or corrupt: %s", filename);
			return false;
		}
		else
		{
			// Have the file!
			debug(LOG_INFO, "Downloaded mod found: %s", filename);
		}
	}

	if (mapData && CheckForMod(mapData->realFileName))
	{
		game.isMapMod = true;
	}
	game.isRandom = mapData && CheckForRandom(mapData->realFileName, mapData->apDataFiles[0]);

	// Set various other initialization things (see NET_FIREUP)
	ingame.TimeEveryoneIsInGame = nullopt;			// reset time
	ingame.endTime = nullopt;
	resetDataHash();
	decideWRF();

	// set selectedPlayer to a *new* spectator slot
	if (NetPlay.players.size() != MAX_CONNECTED_PLAYERS)
	{
		// NetPlay.players.size() should always start at MAX_CONNECTED_PLAYERS before this is called
		debug(LOG_ERROR, "Unexpected NetPlay.players.size(): %zu", NetPlay.players.size());
		return false;
	}
	NetPlay.players.push_back(PLAYER());
	size_t replaySpectatorIndex = NetPlay.players.size() - 1;
	NET_InitPlayer(replaySpectatorIndex, false);  // re-init everything
	NetPlay.players[replaySpectatorIndex].allocated = true;
	sstrcpy(NetPlay.players[replaySpectatorIndex].name, "Replay Viewer"); //setPlayerName() asserts cause this index is now equal to MAX_CONNECTED_PLAYERS
	NetPlay.players[replaySpectatorIndex].isSpectator = true;

	selectedPlayer = replaySpectatorIndex;
	realSelectedPlayer = selectedPlayer;

	// Need to initialize net/game queues here before we try to add messages to them
	for (unsigned i = 0; i < MAX_GAMEQUEUE_SLOTS; ++i)
	{
		NETinitQueue(NETgameQueue(i));
		NETsetNoSendOverNetwork(NETgameQueue(i));
	}

	// simulate "NET_PLAYERRESPONDING" sent by all clients
	// ensure that all humans players are set to "joined"
	for (size_t i = 0; i < MAX_CONNECTED_PLAYERS; ++i)
	{
		if (isHumanPlayer(i) && ingame.JoiningInProgress[i])
		{
			ingame.JoiningInProgress[i] = false;
		}
	}
	NetPlay.isHostAlive = true; // lie and say the host is alive for multiplayer games

	return true;
}
