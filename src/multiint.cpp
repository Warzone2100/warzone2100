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
#include "lib/netplay/netpermissions.h"
#include "lib/netplay/port_mapping_manager.h"
#include "lib/widget/editbox.h"
#include "lib/widget/button.h"
#include "lib/widget/scrollablelist.h"
#include "lib/widget/widget.h"
#include "lib/widget/widgint.h"
#include "lib/widget/label.h"
#include "lib/widget/paragraph.h"
#include "lib/widget/multibutform.h"
#include "lib/widget/checkbox.h"
#include "lib/widget/margin.h"

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
#include "urlhelpers.h"
#include "hci/quickchat.h"
#include "hci/teamstrategy.h"
#include "hci/chatoptions.h"
#include "multivote.h"
#include "campaigninfo.h"
#include "screens/joiningscreen.h"
#include "titleui/widgets/lobbyplayerrow.h"
#include "titleui/widgets/blindwaitingroom.h"
#include "titleui/widgets/multilobbyoptions.h"
#include "titleui/widgets/starthostingbutton.h"
#include "lib/widget/popovermenu.h"
#include "titleui/widgets/advcheckbox.h"

#include "activity.h"
#include <algorithm>
#include <set>
#include "3rdparty/gsl_finally.h"

#define MAP_PREVIEW_DISPLAY_TIME 2500	// number of milliseconds to show map in preview
#define LOBBY_DISABLED_TAG       "lobbyDisabled"
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
std::string defaultSkirmishAI = "";

static UDWORD hideTime = 0;
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
static W_EDITBOX* addMultiEditBox(UDWORD formid, UDWORD id, UDWORD x, UDWORD y, char const *tip, char const *tipres, UDWORD icon, UDWORD iconhi, UDWORD iconid, bool disabled = false);
static int numSlotsToBeDisplayed();
static inline bool spectatorSlotsSupported();

// Drawing Functions
static void displayChatEdit(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset);
static void displaySpectatorAddButton(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset);
static void displayAi(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset);
static void displayDifficulty(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset);
static void displayMultiEditBox(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset);
static void drawBlueBox_Spectator(UDWORD x, UDWORD y, UDWORD w, UDWORD h);
static void drawBlueBox_SpectatorOnly(UDWORD x, UDWORD y, UDWORD w, UDWORD h);
void intDisplayFeBox_SpectatorOnly(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset);

// pUserData structures used by drawing functions
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
static void addChatBox(bool preserveOldChat = false);
static	void	SendFireUp();

static	void	decideWRF();

static bool		SendFactionRequest(UBYTE player, UBYTE faction);
static bool		SendPositionRequest(UBYTE player, UBYTE chosenPlayer);
static bool		SendPlayerSlotTypeRequest(uint32_t player, bool isSpectator);
bool changeReadyStatus(UBYTE player, bool bReady);
static void stopJoining(std::shared_ptr<WzTitleUI> parent, LOBBY_ERROR_TYPES errorResult);

static void sendRoomChatMessage(char const *text, bool skipLocalDisplay = false);

static bool multiplayLacksEnoughPlayersToAutostart();

// ////////////////////////////////////////////////////////////////////////////
// map previews..

static std::array<const char *, 5> difficultyList = { N_("Super Easy"), N_("Easy"), N_("Medium"), N_("Hard"), N_("Insane") };
static const AIDifficulty difficultyValue[] = { AIDifficulty::SUPEREASY, AIDifficulty::EASY, AIDifficulty::MEDIUM, AIDifficulty::HARD, AIDifficulty::INSANE };
static MultiplayOptionsLocked locked;
static bool spectatorHost = false;
static uint16_t defaultOpenSpectatorSlots = 0;
static std::vector<AIDATA> aidata;

const std::vector<AIDATA>& getAIData() { return aidata; }
const MultiplayOptionsLocked& getLockedOptions() { return locked; }

bool updateLockedOptionsFromHost(const MultiplayOptionsLocked& newOpts)
{
	ASSERT_OR_RETURN(false, !NetPlay.isHost, "Should not be called for the host");
	if (newOpts == locked)
	{
		return false;
	}
	locked = newOpts;
	return true;
}

const char* getDifficultyListStr(size_t idx)
{
	ASSERT_OR_RETURN("", idx < difficultyList.size(), "Invalid idx: %zu", idx);
	return difficultyList[idx];
}
size_t getDifficultyListCount()
{
	return difficultyList.size();
}

class ChatBoxButton : public W_BUTTON
{
public:
	void display(int xOffset, int yOffset) override
	{
		int x0 = x() + xOffset;
		int y0 = y() + yOffset;
		int x1 = x0 + width();
		int y1 = y0 + height();

		bool haveText = !pText.isEmpty();

		bool isDown = (getState() & (WBUT_DOWN | WBUT_LOCK | WBUT_CLICKLOCK)) != 0;
		bool isDisabled = (getState() & WBUT_DISABLE) != 0;
		bool isHighlight = !isDisabled && ((getState() & WBUT_HIGHLIGHT) != 0);

		// Display the button.
		auto border_color = !isDisabled ? pal_RGBA(255, 255, 255, 120) : WZCOL_FORM_DARK;
		auto fill_color = isDown || isDisabled ? pal_RGBA(10, 0, 70, 250) : pal_RGBA(25, 0, 110, 220);
		iV_ShadowBox(x0, y0, x1, y1, 0, border_color, border_color, fill_color);
		if (isHighlight)
		{
			iV_Box(x0 + 2, y0 + 2, x1 - 2, y1 - 2, WZCOL_FORM_HILITE);
		}

		if (haveText)
		{
			text.setText(pText, FontID);
			int fw = text.width();
			int fx = x0 + (width() - fw) / 2;
			int fy = y0 + (height() - text.lineSize()) / 2 - text.aboveBase();
			PIELIGHT textColor = WZCOL_FORM_TEXT;
			if (isDisabled)
			{
				textColor.byte.a = (textColor.byte.a / 2);
			}
			text.render(fx, fy, textColor);
		}
	}
private:
	WzText text;
};

class ChatBoxWidget : public IntFormAnimated
{
public:
	enum class ChatBoxSendMode
	{
		DISABLED,
		HOSTMSG_ONLY,
		ENABLED
	};
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

	void setSendMode(ChatBoxSendMode mode);

	void takeFocus();

	void handleUpdateInHostedLobby();

protected:
	void geometryChanged() override;
	void display(int xOffset, int yOffset) override;
	void run(W_CONTEXT *psContext) override;
	void focusLost() override;

private:
	void openQuickChatOverlay();
	void closeQuickChatOverlay();
	int32_t calculateQuickChatScreenPosY();

	void setQuickChatButtonDisplay();
	void setEditBoxDisplay();

	void displayOptionsOverlay(const std::shared_ptr<WIDGET>& psParent);

private:
	std::shared_ptr<ScrollableListWidget> messages;
	std::shared_ptr<ChatBoxButton> quickChatButton;
	std::shared_ptr<ChatBoxButton> optionsButton;
	std::shared_ptr<W_EDITBOX> editBox;
	std::shared_ptr<CONSOLE_MESSAGE_LISTENER> handleConsoleMessage;
	std::shared_ptr<W_SCREEN> quickChatOverlayScreen;
	std::shared_ptr<W_FORM> quickChatForm;
	std::shared_ptr<W_SCREEN> optionsOverlayScreen;

	ChatBoxSendMode currentMode = ChatBoxSendMode::ENABLED;
	bool hasFocus = false;

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

void frontendCycleAIs()
{
	std::string currAI = getDefaultSkirmishAI();

	for (size_t i = 0; i < aidata.size(); ++i)
	{
		if (aidata[i].js == currAI)
		{
			size_t idx = ((i + 1) >= aidata.size()) ? 0 : (i + 1);
			setDefaultSkirmishAI(aidata[idx].js);
			break;
		}
	}
}

void setDefaultSkirmishAI(const std::string& name)
{
	defaultSkirmishAI = name;
}

std::string getDefaultSkirmishAI(const bool& displayNameOnly/*=false*/)
{
	const std::string fallbackAI = DEFAULT_SKIRMISH_AI_SCRIPT_NAME;
	size_t fallbackIdx = 0;
	std::string name = "";

	for (size_t i = 0; i < aidata.size(); ++i)
	{
		if (aidata[i].js == fallbackAI)
		{
			fallbackIdx = i;
		}
		if (aidata[i].js == defaultSkirmishAI)
		{
			name = (displayNameOnly) ? aidata[i].name : aidata[i].js;
		}
	}

	// Something weird happened, config messed with, or mod AI missing?
	if (name.empty() && aidata.size() > 0)
	{
		setDefaultSkirmishAI(aidata[fallbackIdx].js);
		name = (displayNameOnly) ? aidata[fallbackIdx].name : aidata[fallbackIdx].js;
	}

	return name;
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
	sstrcpy(aFileName, psLevel->apDataFiles[psLevel->game].c_str());
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
					if (!loadPlayerScript(WzString("multiplay/skirmish/") + aidata[NetPlay.players[i].ai].js, i, NetPlay.players[i].difficulty))
					{
						debug(LOG_ERROR, "Failed to load AI!: %s", aidata[NetPlay.players[i].ai].js);
					}
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

	if (psLevel->psBaseData && !psLevel->psBaseData->pName.empty())
	{
		if (sscanf(psLevel->psBaseData->pName.c_str(), "MULTI_CAM_%u", &c) != 1)
		{
			sscanf(psLevel->psBaseData->pName.c_str(), "MULTI_T%u_C%u", &t, &c);
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

static void loadEmptyMapPreview(bool withQuestionMarks = true)
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
	int const ex = 100, ey = 100;
	if (withQuestionMarks)
	{
		int const bx = 5, by = 8;
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

	if (isBlindSimpleLobby(game.blindMode) && !NetPlay.isHost)
	{
		loadEmptyMapPreview(false);
		return;
	}

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
		useTerrainOverrides = shouldLoadTerrainTypeOverrides(psLevel->pName);
		debug(LOG_WZ, "Loading map preview: \"%s\" builtin t%d override %d", psLevel->pName.c_str(), psLevel->dataDir, (int)useTerrainOverrides);
	}
	else
	{
		builtInMap = false;
		useTerrainOverrides = false;
		debug(LOG_WZ, "Loading map preview: \"%s\" in (%s)\"%s\"  %s t%d", psLevel->pName.c_str(), WZ_PHYSFS_getRealDir_String(psLevel->realFileName).c_str(), psLevel->realFileName, psLevel->realFileHash.toString().c_str(), psLevel->dataDir);
	}
	rebuildSearchPath(psLevel->dataDir, false);
	if (psLevel->apDataFiles[psLevel->game].empty())
	{
		debug(LOG_ERROR, "No path for level \"%s\"? (%s)", psLevel->pName.c_str(), (psLevel->realFileName) ? psLevel->realFileName : "null");
		loadEmptyMapPreview();
		return;
	}
	aFileName = psLevel->apDataFiles[psLevel->game];

	auto gameLoadDetails = (psLevel->realFileName) ? GameLoadDetails::makeMapPackageLoad(psLevel->realFileName) : GameLoadDetails::makeLevelFileLoad(aFileName);

	// load the map data
	auto data = gameLoadDetails.getMap(rand());
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
	previewColorScheme.playerColorProvider = std::make_unique<WzLobbyPreviewPlayerColorProvider>();
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
	auto mapPreviewResult = WzMap::generate2DMapPreview(*data, previewColorScheme, WzMap::MapStatsConfiguration(WzMap::MapType::SKIRMISH), generatePreviewLogger.get());
	if (!mapPreviewResult || mapPreviewResult->width == 0 || mapPreviewResult->height == 0)
	{
		// Failed to generate map preview
		debug(LOG_ERROR, "Failed to generate map preview for: %s", psLevel->pName.c_str());
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

void refreshMultiplayerOptionsTitleUIIfActive()
{
	auto psCurr = std::dynamic_pointer_cast<WzMultiplayerOptionsTitleUI>(wzTitleUICurrent);
	if (psCurr)
	{
		psCurr->updateGameOptions();
	}
}

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

		if (strcmp(ai.js, defaultSkirmishAI.c_str()) == 0)
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

std::string JoinConnectionDescription::connectiontype_to_string(JoinConnectionDescription::JoinConnectionType type)
{
	switch (type)
	{
	case JoinConnectionDescription::JoinConnectionType::TCP_DIRECT: return "tcp";
#ifdef WZ_GNS_NETWORK_BACKEND_ENABLED
	case JoinConnectionDescription::JoinConnectionType::GNS_DIRECT: return "gns";
#endif
	}
	return {}; // silence compiler warning
}

optional<JoinConnectionDescription::JoinConnectionType> JoinConnectionDescription::connectiontype_from_string(const std::string& str)
{
	if (str == "tcp")
	{
		return JoinConnectionDescription::JoinConnectionType::TCP_DIRECT;
	}
#ifdef WZ_GNS_NETWORK_BACKEND_ENABLED
	if (str == "gns")
	{
		return JoinConnectionDescription::JoinConnectionType::GNS_DIRECT;
	}
#endif
	return nullopt;
}

void to_json(nlohmann::json& j, const JoinConnectionDescription::JoinConnectionType& v)
{
	j = JoinConnectionDescription::connectiontype_to_string(v);
}

void from_json(const nlohmann::json& j, JoinConnectionDescription::JoinConnectionType& v)
{
	auto str = j.get<std::string>();
	auto result = JoinConnectionDescription::connectiontype_from_string(str);
	if (result.has_value())
	{
		v = result.value();
	}
	else
	{
		throw nlohmann::json::type_error::create(302, "JoinConnectionType value is unknown: \"" + str + "\"", &j);
	}
}

void to_json(nlohmann::json& j, const JoinConnectionDescription& v)
{
	j = nlohmann::json::object();
	j["h"] = v.host;
	j["p"] = v.port;
	j["t"] = v.type;
}

void from_json(const nlohmann::json& j, JoinConnectionDescription& v)
{
	v.host = j.at("h").get<std::string>();
	v.port = j.at("p").get<int32_t>();
	v.type = j.at("t").get<JoinConnectionDescription::JoinConnectionType>();
}

// NOTE: Must call NETinit(); before this will actually work
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

	GAMESTRUCT lobbyGame;
	memset(&lobbyGame, 0x00, sizeof(lobbyGame));
	if (!NETfindGame(lobbyGameId, lobbyGame))
	{
		// failed to get list of games from lobby server
		debug(LOG_ERROR, "Failed to find gameId in lobby server");
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
	std::vector<JoinConnectionDescription> connList;
	connList.emplace_back(JoinConnectionDescription::JoinConnectionType::TCP_DIRECT, host, lobbyGame.hostPort);
	return connList;
}

void joinGame(const char *connectionString, bool asSpectator /*= false*/)
{
	const char* a = strchr(connectionString, '[');
	const char* b = (a != nullptr) ? strchr(a, ']') : nullptr;
	if (a != nullptr && b != nullptr)
	{
		// Bracketed host (example, bracketed IPv6 address) - may have a port after
		std::string serverIP = "";
		const char* ipv6AddressStart = a+1;
		serverIP.assign(ipv6AddressStart, b-ipv6AddressStart);
		uint32_t serverPort = 0; // default port
		if (*(b+1) == ':')
		{
			// extract the port
			serverPort = atoi(b+2);
		}
		debug(LOG_INFO, "Connecting to host [%s] port %d", serverIP.c_str(), serverPort);
		joinGame(serverIP.c_str(), serverPort, asSpectator);
		return;
	}
	else
	{
		const char* ddch = strrchr(connectionString, ':');
		const char* firstCh = (ddch != nullptr) ? strchr(connectionString, ':') : nullptr;
		if (ddch != nullptr && firstCh == ddch)
		{
			// Presumably a host:port string (has a single colon), where host is either an IPv4 address or a hostname
			uint32_t serverPort = atoi(ddch+1);
			std::string serverIP = "";
			serverIP.assign(connectionString, ddch - connectionString);
			debug(LOG_INFO, "Connecting to host [%s] port %d", serverIP.c_str(), serverPort);
			joinGame(serverIP.c_str(), serverPort, asSpectator);
			return;
		}
		// otherwise fall-back to just treating the entire connectionString as the host (IPv4/6 address, hostname, etc)
	}
	joinGame(connectionString, 0, asSpectator);
}

void joinGame(const char *host, uint32_t port, bool asSpectator /*= false*/)
{
	std::string hostStr = (host != nullptr) ? std::string(host) : std::string();
	std::vector<JoinConnectionDescription> connList;
	connList.emplace_back(JoinConnectionDescription::JoinConnectionType::TCP_DIRECT, hostStr, port);
#ifdef WZ_GNS_NETWORK_BACKEND_ENABLED
	connList.emplace_back(JoinConnectionDescription::JoinConnectionType::GNS_DIRECT, hostStr, port);
#endif
	joinGame(connList, asSpectator);
}

void joinGame(const std::vector<JoinConnectionDescription>& connection_list, bool asSpectator /*= false*/) {
	startJoiningAttempt(sPlayer, connection_list, asSpectator);
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

void updateStructureDisabledFlags()
{
	// The host works out the flags.
	if (ingame.side != InGameSide::HOST_OR_SINGLEPLAYER)
	{
		return;
	}

	unsigned flags = ingame.flags & MPFLAGS_FORCELIMITS;

	static_assert(MPFLAGS_FORCELIMITS == (1 << (static_cast<unsigned>(limitIcons.size()) - 1)), "");
	for (unsigned i = 0; i < static_cast<unsigned>(limitIcons.size()) - 1; ++i)	// skip last item, MPFLAGS_FORCELIMITS
	{
		int stat = getStructStatFromName(limitIcons[i].stat);
		bool disabled = stat >= 0 && asStructureStats[stat].upgrade[0].limit == 0;
		flags |= disabled << i;
	}

	ingame.flags = flags;
}

WzString formatGameName(WzString name)
{
	WzString withoutTechlevel = WzString::fromUtf8(mapNameWithoutTechlevel(name.toUtf8().c_str()));
	return withoutTechlevel + " (T" + WzString::number(game.techLevel) + " " + WzString::number(game.maxPlayers) + "P)";
}

static bool canChangeMapOrRandomize()
{
	ASSERT_HOST_ONLY(return false);

	if (!ingame.localJoiningInProgress)  // Only if game hasn't actually started yet.
	{
		debug(LOG_INFO, "Cannot randomize match options after game has started");
		return false;
	}

	uint8_t numHumans = NET_numHumanPlayers();
	bool allowed = (static_cast<float>(getLobbyChangeVoteTotal()) / static_cast<float>(numHumans)) > 0.5f;

	resetLobbyChangeVoteData(); //So the host can only do one change every vote session

	if (numHumans == 1)
	{
		return true;
	}

	if (!allowed)
	{
		startLobbyChangeVote();
		displayRoomSystemMessage(_("Not enough votes to randomize or change the map."));
	}

	return allowed;
}

static bool updatePlayerNameBox()
{
	auto pNameEditBox = widgGetFromID(psWScreen, MULTIOP_PNAME);
	auto pNameMultiBut = widgGetFromID(psWScreen, MULTIOP_PNAME_ICON);

	ASSERT_OR_RETURN(false, pNameEditBox != nullptr, "No player name edit box?");
	ASSERT_OR_RETURN(false, pNameMultiBut != nullptr, "No player name edit button?");

	const bool isInBlindMode = (game.blindMode != BLIND_MODE::NONE);
	const bool changesAllowed = ingame.hostChatPermissions[selectedPlayer] && !isInBlindMode && !locked.name;

	WzString playerNameTip;
	if (!isInBlindMode)
	{
		if (changesAllowed)
		{
			playerNameTip = _("Select Player Name");
		}
		else if (!ingame.hostChatPermissions[selectedPlayer])
		{
			playerNameTip = _("Player Name changes are not allowed when you are muted");
		}
		else
		{
			playerNameTip = _("Player Name changes are not allowed by the host");
		}
	}
	else
	{
		if (game.blindMode >= BLIND_MODE::BLIND_GAME)
		{
			playerNameTip = _("Blind Game:");
			playerNameTip += "\n";
			playerNameTip += _("- Player names will not be visible to other players (until the game is over)");
		}
		else
		{
			playerNameTip = _("Blind Lobby:");
			playerNameTip += "\n";
			playerNameTip += _("- Player names will not be visible to other players (until the game has started)");
		}
	}

	pNameEditBox->setTip(playerNameTip.toUtf8());

	pNameEditBox->setState((changesAllowed) ? 0 : WEDBS_DISABLE);
	pNameMultiBut->setState((changesAllowed) ? 0 : WBUT_DISABLE);

	return true;
}

void multiLobbyHandleHostOptionsChanges(const std::array<bool, MAX_CONNECTED_PLAYERS>& priorHostChatPermissions)
{
	updatePlayerNameBox();
	// Possible future TODO: Update other UI if locked options changed?
}

// need to check for side effects.
void WzMultiplayerOptionsTitleUI::addGameOptions()
{
	auto psExistingOptions = widgGetFromID(psWScreen, MULTIOP_OPTIONS);
	if (psExistingOptions)
	{
		// options is already up - refresh data
		updateGameOptions();
		return;
	}

	widgDelete(psWScreen, MULTIOP_OPTIONS);  				// clear options list
	widgDelete(psWScreen, FRONTEND_SIDETEXT3);				// del text..

	WIDGET *psBackdrop = widgGetFromID(psWScreen, FRONTEND_BACKDROP);

	// draw options box.
	auto optionsForm = std::make_shared<IntFormAnimated>(false);
	psBackdrop->attach(optionsForm);
	optionsForm->id = MULTIOP_OPTIONS;
	optionsForm->setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
		psWidget->setGeometry(MULTIOP_OPTIONSX, MULTIOP_OPTIONSY, MULTIOP_OPTIONSW, MULTIOP_OPTIONSH);
	}));

	addSideText(FRONTEND_SIDETEXT3, MULTIOP_OPTIONSX - 6 , MULTIOP_OPTIONSY, _("OPTIONS"));

	bool isInBlindMode = (game.blindMode != BLIND_MODE::NONE);
	auto pNameEditBox = addMultiEditBox(MULTIOP_OPTIONS, MULTIOP_PNAME, MCOL0, MROW1, "", (char *) sPlayer, IMAGE_EDIT_PLAYER, IMAGE_EDIT_PLAYER_HI, MULTIOP_PNAME_ICON, isInBlindMode);
	if (pNameEditBox)
	{
		updatePlayerNameBox();
	}

	// Create the Start Hosting button
	auto psWeakTitleUI = std::weak_ptr<WzMultiplayerOptionsTitleUI>(std::dynamic_pointer_cast<WzMultiplayerOptionsTitleUI>(shared_from_this()));
	startHostingButton = WzStartHostingButton::make(std::dynamic_pointer_cast<WzMultiplayerOptionsTitleUI>(shared_from_this()));
	startHostingButton->addOnClickHandler([psWeakTitleUI](W_BUTTON&) {
		if (auto strongTitleUI = psWeakTitleUI.lock())
		{
			strongTitleUI->startHost();
		}
	});
	optionsForm->attach(startHostingButton);
	int hostButtonHeight = startHostingButton->idealHeight();
	startHostingButton->setGeometry(1, optionsForm->height() - hostButtonHeight - 1, optionsForm->width() - 2, hostButtonHeight);

	bool showHostButton = (ingame.side == InGameSide::HOST_OR_SINGLEPLAYER) && (!challengeActive) && !(NetPlay.isHost);
	if (showHostButton)
	{
		startHostingButton->show();
	}
	else
	{
		startHostingButton->hide();
	}

	// Add any relevant factory disabled icons / flags
	updateStructureDisabledFlags();

	// Add game options list
	multiLobbyOptionsForm = makeWzMultiLobbyOptionsForm(challengeActive, std::dynamic_pointer_cast<WzMultiplayerOptionsTitleUI>(shared_from_this()));
	optionsForm->attach(multiLobbyOptionsForm);
	multiLobbyOptionsForm->setCalcLayout([psWeakTitleUI](WIDGET *psWidget){
		auto psParent = psWidget->parent();
		ASSERT_OR_RETURN(, psParent != nullptr, "No parent?");
		auto strongTitleUI = psWeakTitleUI.lock();
		ASSERT_OR_RETURN(, strongTitleUI != nullptr, "No titleUI?");
		int x0 = 1;
		int y0 = 42;
		int width = psParent->width() - 2;
		bool showHostButton = (strongTitleUI->startHostingButton) ? strongTitleUI->startHostingButton->visible() : false;
		int parentHeight = psParent->height();
		int hostButtonVerticalSpace = (showHostButton) ? (parentHeight - strongTitleUI->startHostingButton->y()) : 0;
		int maxOptionsListHeight = parentHeight - y0 - hostButtonVerticalSpace;
		psWidget->setGeometry(x0, y0, width, std::min<int>(maxOptionsListHeight, psWidget->idealHeight()));
	});

	// cancel
	addMultiBut(psWScreen, MULTIOP_OPTIONS, CON_CANCEL,
	            MULTIOP_CANCELX, MULTIOP_CANCELY,
	            iV_GetImageWidth(FrontImages, IMAGE_RETURN),
	            iV_GetImageHeight(FrontImages, IMAGE_RETURN),
	            _("Return To Previous Screen"), IMAGE_RETURN, IMAGE_RETURN_HI, IMAGE_RETURN_HI);
}

bool isHostOrAdmin()
{
	return NetPlay.isHost || NetPlay.players[selectedPlayer].isAdmin;
}

bool isPlayerHostOrAdmin(uint32_t playerIdx)
{
	ASSERT_OR_RETURN(false, playerIdx < MAX_CONNECTED_PLAYERS, "Invalid player idx: %" PRIu32, playerIdx);
	return (playerIdx == NetPlay.hostPlayer) || NetPlay.players[playerIdx].isAdmin;
}

static bool shouldInformOfAdminAction(uint32_t targetPlayerIdx, uint32_t responsibleIdx)
{
	ASSERT_OR_RETURN(false, targetPlayerIdx < MAX_CONNECTED_PLAYERS, "Invalid targetPlayerIdx: %" PRIu32, targetPlayerIdx);
	ASSERT_OR_RETURN(false, responsibleIdx < MAX_CONNECTED_PLAYERS, "Invalid responsibleIdx: %" PRIu32, responsibleIdx);

	if (responsibleIdx == targetPlayerIdx)
	{
		return false; // do not inform about self-action
	}
	if (!isPlayerHostOrAdmin(responsibleIdx))
	{
		return false;
	}
	if (!NetPlay.players[targetPlayerIdx].allocated)
	{
		return false; // do not inform if target isn't a human player
	}

	return true;
}


// ////////////////////////////////////////////////////////////////////////////
// Colour functions

static bool safeToUseColour(unsigned player, unsigned otherPlayer)
{
	// Player wants to take the colour from otherPlayer. May not take from a human otherPlayer, unless we're the host/admin.
	return player == otherPlayer || isHostOrAdmin() || !isHumanPlayer(otherPlayer);
}

static int getPlayerTeam(int i)
{
	return alliancesSetTeamsBeforeGame(game.alliance) ? NetPlay.players[i].team : i;
}

/**
 * Checks if all players are on the same team. If so, return that team; if not, return -1;
 * if there are no players, return team MAX_PLAYERS.
 */
int allPlayersOnSameTeam(int except)
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
	if (isBlindSimpleLobby(game.blindMode) && !NetPlay.isHost)
	{
		// Get Y position of virtual player row in WzPlayerBlindWaitingRoom widget
		auto pBlindWaitingRoom = widgFormGetFromID(psWScreen->psForm, MULTIOP_BLIND_WAITING_ROOM);
		if (!pBlindWaitingRoom)
		{
			return -1;
		}
		return getWzPlayerBlindWaitingRoomPlayerRowY0(pBlindWaitingRoom) + pBlindWaitingRoom->y();
	}
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
	ASSERT_HOST_ONLY(return);

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
			resetReadyStatus(false, isBlindSimpleLobby(game.blindMode));
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

static void changeAI(uint32_t player, int8_t ai)
{
	ASSERT_OR_RETURN(, player < MAX_CONNECTED_PLAYERS, "Invalid player: %" PRIu32, player);

	bool previousPlayersShouldCheckReadyValue = multiplayPlayersShouldCheckReady();

	NetPlay.players[player].ai = ai;

	if (ingame.localJoiningInProgress)  // Only if game hasn't actually started yet.
	{
		handlePossiblePlayersShouldCheckReadyChange(previousPlayersShouldCheckReadyValue);
	}
}

void WzMultiplayerOptionsTitleUI::openAiChooser(uint32_t player)
{
	ASSERT_HOST_ONLY(return);

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

		if (!NetPlay.players[player].allocated)
		{
			switch (clickedButton.id)
			{
			case MULTIOP_AI_CLOSED:
				changeAI(player, AI_CLOSED);
				NetPlay.players[player].isSpectator = false;
				break;
			case MULTIOP_AI_OPEN:
				changeAI(player, AI_OPEN);
				NetPlay.players[player].isSpectator = false;
				break;
			case MULTIOP_AI_SPECTATOR:
				// set slot to open
				changeAI(player, AI_OPEN);
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
			resetReadyStatus(false, isBlindSimpleLobby(game.blindMode));
		}
		else
		{
			debug(LOG_INFO, "Player joined slot %d while chooser was open", player);
		}
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
				if (!NetPlay.players[player].allocated)
				{
					NetPlay.players[player].isSpectator = false;
					changeAI(player, aiIdx);
					setPlayerName(player, getAIName(player));
					// Difficulty is preserved when switching open AI slots.
					if (NetPlay.players[player].difficulty == AIDifficulty::DISABLED)
					{
						NetPlay.players[player].difficulty = AIDifficulty::DEFAULT;
					}
					NETBroadcastPlayerInfo(player);
					resetReadyStatus(false);
				}
				else
				{
					debug(LOG_INFO, "Player joined slot %d while chooser was open", player);
				}
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

				std::string playerName = getPlayerName(switcherPlayerIdx, true);

				NETmoveSpectatorToPlayerSlot(switcherPlayerIdx, selectPositionRow->targetPlayerIdx, true);
				resetReadyStatus(true, isBlindSimpleLobby(game.blindMode));		//reset and send notification to all clients
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

std::set<uint32_t> validPlayerIdxTargetsForPlayerPositionMove(uint32_t player)
{
	std::set<uint32_t> validTargetPlayerIdx;
	for (uint32_t i = 0; i < game.maxPlayers; i++)
	{
		if (player != i
			&& (isHostOrAdmin() || !isHumanPlayer(i)) // host/admin can move a player to any slot, player can only move to empty slots
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

void WzMultiplayerOptionsTitleUI::updateGameOptions()
{
	updatePlayerNameBox();

	auto psMultiLobbyOptionsForm = std::dynamic_pointer_cast<WzMultiLobbyOptionsWidgetBase>(multiLobbyOptionsForm);
	if (psMultiLobbyOptionsForm)
	{
		psMultiLobbyOptionsForm->refreshData();
	}

	if (startHostingButton)
	{
		bool showHostButton = (ingame.side == InGameSide::HOST_OR_SINGLEPLAYER) && (!challengeActive) && !(NetPlay.isHost);
		if (showHostButton)
		{
			startHostingButton->show();
		}
		else
		{
			startHostingButton->hide();
		}
	}

	if (multiLobbyOptionsForm)
	{
		multiLobbyOptionsForm->callCalcLayout();
	}
}

static bool SendTeamRequest(UBYTE player, UBYTE chosenTeam); // forward-declare

void WzMultiplayerOptionsTitleUI::openTeamChooser(uint32_t player)
{
	ASSERT_OR_RETURN(, player < MAX_CONNECTED_PLAYERS, "Invalid player: %" PRIu32 "", player);

	const bool bIsTrueMultiplayerGame = bMultiPlayer && NetPlay.bComms;

	UDWORD i;
	int disallow = allPlayersOnSameTeam(player);
	if (bIsTrueMultiplayerGame && isHostOrAdmin())
	{
		// allow configuration of all teams in true multiplayer mode (by host/admin), even if they would block the game starting
		// (i.e. even if all players would be configured to be on the same team)
		disallow = -1;
	}
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

				resetReadyStatus(false, isBlindSimpleLobby(game.blindMode));		// will reset only locally if not a host

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

			std::string msg = astringf(_("The host has kicked %s from the game!"), getPlayerName(player, true));
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

			std::string msg = astringf(_("The host has banned %s from the game!"), getPlayerName(player, true));
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
			("Ban player"), IMAGE_NOJOIN_FULL, IMAGE_NOJOIN_FULL, IMAGE_NOJOIN_FULL, banOnClickHandler);
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

					std::string playerName = getPlayerName(player, true);

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

void WzMultiplayerOptionsTitleUI::openMapChooser()
{
	widgDelete(psWScreen, MULTIOP_PLAYERS);
	widgDelete(psWScreen, FRONTEND_SIDETEXT2);					// del text too,

	debug(LOG_WZ, "openMapChooser: %s.wrf", MultiCustomMapsPath);
	addMultiRequest(MultiCustomMapsPath, ".wrf", MULTIOP_MAP, current_numplayers);

	if (NetPlay.isHost && NetPlay.bComms)
	{
		sendOptions();

		NETsetLobbyOptField(game.map, NET_LOBBY_OPT_FIELD::MAPNAME);
		NETregisterServer(WZ_SERVER_UPDATE);
	}
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

static void informIfAdminChangedOtherTeam(uint32_t targetPlayerIdx, uint32_t responsibleIdx)
{
	if (!shouldInformOfAdminAction(targetPlayerIdx, responsibleIdx))
	{
		return;
	}

	sendQuickChat(WzQuickChatMessage::INTERNAL_ADMIN_ACTION_NOTICE, realSelectedPlayer, WzQuickChatTargeting::targetAll(), WzQuickChatDataContexts::INTERNAL_ADMIN_ACTION_NOTICE::constructMessageData(WzQuickChatDataContexts::INTERNAL_ADMIN_ACTION_NOTICE::Context::Team, responsibleIdx, targetPlayerIdx));

	std::string senderPublicKeyB64 = base64Encode(getOutputPlayerIdentity(responsibleIdx).toBytes(EcKey::Public));
	debug(LOG_INFO, "Admin %s (%s) changed team of player ([%u] %s) to: %d", getPlayerName(responsibleIdx), senderPublicKeyB64.c_str(), NetPlay.players[targetPlayerIdx].position, getPlayerName(targetPlayerIdx), NetPlay.players[targetPlayerIdx].team);
}

void handlePossiblePlayersShouldCheckReadyChange(bool previousPlayersShouldCheckReadyValue)
{
	bool currentPlayersShouldCheckReadyValue = multiplayPlayersShouldCheckReady();
	if (previousPlayersShouldCheckReadyValue != currentPlayersShouldCheckReadyValue)
	{
		if (currentPlayersShouldCheckReadyValue)
		{
			debug(LOG_NET, "Starting to track not-ready time");

			// Player should now check ready
			// Update ingame.lastNotReadyTimes for all not-ready players to "now"
			auto now = std::chrono::steady_clock::now();
			for (size_t i = 0; i < NetPlay.players.size(); i++)
			{
				if (!isHumanPlayer(i)) { continue; }
				if (!NetPlay.players[i].ready)
				{
					ingame.lastNotReadyTimes[i] = now;
				}
			}

			if (NetPlay.isHost && bMultiPlayer && NetPlay.bComms)
			{
				auto autoNotReadyKickSeconds = war_getAutoNotReadyKickSeconds();
				sendQuickChat(WzQuickChatMessage::INTERNAL_LOCALIZED_LOBBY_NOTICE, realSelectedPlayer, WzQuickChatTargeting::targetAll(), WzQuickChatDataContexts::INTERNAL_LOCALIZED_LOBBY_NOTICE::constructMessageData(WzQuickChatDataContexts::INTERNAL_LOCALIZED_LOBBY_NOTICE::Context::PlayerShouldCheckReadyNotice, NetPlay.hostPlayer, (autoNotReadyKickSeconds > 0) ? static_cast<uint32_t>(autoNotReadyKickSeconds) : 0));
			}
		}
		else
		{
			debug(LOG_NET, "Stopping not-ready time tracking");
			bool shouldAccumulateNotReadyTime = isBlindSimpleLobby(game.blindMode);

			// Players are no longer required to check ready
			auto now = std::chrono::steady_clock::now();
			for (size_t i = 0; i < NetPlay.players.size(); i++)
			{
				if (!isHumanPlayer(i)) { continue; }
				if (shouldAccumulateNotReadyTime)
				{
					// Update ingame.secondsNotReady for all not-ready players
					if (!NetPlay.players[i].ready)
					{
						// accumulate time spent not ready while they _could_ check ready
						if (ingame.lastNotReadyTimes[i].has_value())
						{
							ingame.secondsNotReady[i] += std::chrono::duration_cast<std::chrono::seconds>(now - ingame.lastNotReadyTimes[i].value()).count();
						}
					}
				}
				else
				{
					// Reset the not-ready time for all players
					// (time will start accumulating again when multiplayPlayersShouldCheckReady becomes true)
					ingame.secondsNotReady[i] = 0;
				}
				// Reset ingame.lastNotReadyTimes to nullopt
				ingame.lastNotReadyTimes[i].reset();
			}
		}
		wz_command_interface_output_room_status_json(true);
	}
}

static bool changeTeam(UBYTE player, UBYTE team, uint32_t responsibleIdx)
{
	if (team >= MAX_CONNECTED_PLAYERS)
	{
		return false;
	}

	if (player >= MAX_CONNECTED_PLAYERS)
	{
		return false;
	}

	if (NetPlay.players[player].team == team)
	{
		NETBroadcastPlayerInfo(player); // we do this regardless, in case of sync issues // FUTURE TODO: Doublecheck if this is still needed?
		return false;  // Nothing to do.
	}

	bool previousPlayersShouldCheckReadyValue = multiplayPlayersShouldCheckReady();

	NetPlay.players[player].team = team;
	debug(LOG_WZ, "set %d as new team for player %d", team, player);
	NETBroadcastPlayerInfo(player);
	informIfAdminChangedOtherTeam(player, responsibleIdx);

	handlePossiblePlayersShouldCheckReadyChange(previousPlayersShouldCheckReadyValue);

	netPlayersUpdated = true;
	return true;
}

static bool SendTeamRequest(UBYTE player, UBYTE chosenTeam)
{
	if (NetPlay.isHost)			// do or request the change.
	{
		changeTeam(player, chosenTeam, realSelectedPlayer);	// do the change, remember only the host can do this to avoid confusion.
	}
	else
	{
		auto w = NETbeginEncode(NETnetQueue(NetPlay.hostPlayer), NET_TEAMREQUEST);
		NETuint8_t(w, player);
		NETuint8_t(w, chosenTeam);
		NETend(w);

	}
	return true;
}

bool recvTeamRequest(NETQUEUE queue)
{
	ASSERT_HOST_ONLY(return true);

	auto r = NETbeginDecode(queue, NET_TEAMREQUEST);

	UBYTE player, team;
	NETuint8_t(r, player);
	NETuint8_t(r, team);
	NETend(r);

	if (player >= MAX_PLAYERS || team >= MAX_PLAYERS)
	{
		debug(LOG_NET, "NET_TEAMREQUEST invalid, player %d team, %d", (int) player, (int) team);
		debug(LOG_ERROR, "Invalid NET_TEAMREQUEST from player %d: Tried to change player %d (team %d)",
		      queue.index, (int)player, (int)team);
		return false;
	}

	if (whosResponsible(player) != queue.index && !NetPlay.players[queue.index].isAdmin)
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
	debug(LOG_NET, "%s is now part of team: %d", getPlayerName(player), (int) team);
	return changeTeam(player, team, queue.index); // we do this regardless, in case of sync issues
}

bool sendReadyRequest(UBYTE player, bool bReady)
{
	if (NetPlay.isHost)			// do or request the change.
	{
		bool changedValue = changeReadyStatus(player, bReady);
		if (changedValue && wz_command_interface_enabled())
		{
			const auto& identity = getOutputPlayerIdentity(player);
			std::string playerPublicKeyB64 = base64Encode(identity.toBytes(EcKey::Public));
			std::string playerIdentityHash = identity.publicHashString();
			std::string playerVerifiedStatus = (ingame.VerifiedIdentity[player]) ? "V" : "?";
			std::string playerName = getPlayerName(player);
			std::string playerNameB64 = base64Encode(std::vector<unsigned char>(playerName.begin(), playerName.end()));
			wz_command_interface_output("WZEVENT: readyStatus=%d: %" PRIu32 " %s %s %s %s %s\n", bReady ? 1 : 0, player, playerPublicKeyB64.c_str(), playerIdentityHash.c_str(), playerVerifiedStatus.c_str(), playerNameB64.c_str(), NetPlay.players[player].IPtextAddress);

			wz_command_interface_output_room_status_json();
		}
		return changedValue;
	}
	else
	{
		auto w = NETbeginEncode(NETnetQueue(NetPlay.hostPlayer), NET_READY_REQUEST);
		NETuint8_t(w, player);
		NETbool(w, bReady);
		NETend(w);
	}
	return true;
}

bool recvReadyRequest(NETQUEUE queue)
{
	ASSERT_HOST_ONLY(return true);

	auto r = NETbeginDecode(queue, NET_READY_REQUEST);

	UBYTE player;
	bool bReady = false;
	NETuint8_t(r, player);
	NETbool(r, bReady);
	NETend(r);

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
		debug(LOG_INFO, "Player has empty identity: (player: %u, name: \"%s\", IP: %s)", (unsigned)player, getPlayerName(player), NetPlay.players[player].IPtextAddress);

		// kick the player
		HandleBadParam("NET_READY_REQUEST failed due to player empty identity.", player, queue.index);
		return false;
	}

	if (!NetPlay.players[player].isSpectator && !multiplayPlayersCanCheckReady())
	{
		// silently ignore players trying to check ready when they shouldn't - refresh player info with current state
		NETBroadcastPlayerInfo(player);
		return false;
	}

	bool changedValue = changeReadyStatus((UBYTE)player, bReady);
	if (changedValue && wz_command_interface_enabled())
	{
		const auto& identity = getOutputPlayerIdentity(player);
		std::string playerPublicKeyB64 = base64Encode(identity.toBytes(EcKey::Public));
		std::string playerIdentityHash = identity.publicHashString();
		std::string playerVerifiedStatus = (ingame.VerifiedIdentity[player]) ? "V" : "?";
		std::string playerName = getPlayerName(player);
		std::string playerNameB64 = base64Encode(std::vector<unsigned char>(playerName.begin(), playerName.end()));
		wz_command_interface_output("WZEVENT: readyStatus=%d: %" PRIu32 " %s %s %s %s %s\n", bReady ? 1 : 0, player, playerPublicKeyB64.c_str(), playerIdentityHash.c_str(), playerVerifiedStatus.c_str(), playerNameB64.c_str(), NetPlay.players[player].IPtextAddress);

		wz_command_interface_output_room_status_json();
	}
	return changedValue;
}

static bool changeReadyStatusImpl(UBYTE player, bool bReady, bool skipBroadcastUpdate)
{
	bool changedValue = NetPlay.players[player].ready != bReady;
	NetPlay.players[player].ready = bReady;
	if (changedValue)
	{
		if (bReady)
		{
			auto now = std::chrono::steady_clock::now();
			if (ingame.lastNotReadyTimes[player].has_value())
			{
				// accumulate time spent since the player last went "not ready"
				ingame.secondsNotReady[player] += std::chrono::duration_cast<std::chrono::seconds>(now - ingame.lastNotReadyTimes[player].value()).count();
				ingame.lastNotReadyTimes[player].reset();
			}
			ingame.lastReadyTimes[player] = now;
		}
		else
		{
			if (multiplayPlayersShouldCheckReady())
			{
				ingame.lastNotReadyTimes[player] = std::chrono::steady_clock::now();
			}
		}
	}
	if (!skipBroadcastUpdate)
	{
		NETBroadcastPlayerInfo(player);
	}
	netPlayersUpdated = true;
	// Player is fast! Clicked the "Ready" button before we had a chance to ping him/her
	// change PingTime to some value less than PING_LIMIT, so that multiplayPlayersReady
	// doesnt block
	ingame.PingTimes[player] = ingame.PingTimes[player] == PING_LIMIT ? 1 : ingame.PingTimes[player];
	return changedValue;
}

bool changeReadyStatus(UBYTE player, bool bReady)
{
	return changeReadyStatusImpl(player, bReady, false);
}

static void informIfAdminChangedOtherPosition(uint32_t targetPlayerIdx, uint32_t responsibleIdx)
{
	if (!shouldInformOfAdminAction(targetPlayerIdx, responsibleIdx))
	{
		return;
	}

	sendQuickChat(WzQuickChatMessage::INTERNAL_ADMIN_ACTION_NOTICE, realSelectedPlayer, WzQuickChatTargeting::targetAll(), WzQuickChatDataContexts::INTERNAL_ADMIN_ACTION_NOTICE::constructMessageData(WzQuickChatDataContexts::INTERNAL_ADMIN_ACTION_NOTICE::Context::Position, responsibleIdx, targetPlayerIdx));

	std::string senderPublicKeyB64 = base64Encode(getOutputPlayerIdentity(responsibleIdx).toBytes(EcKey::Public));
	debug(LOG_INFO, "Admin %s (%s) changed position of player (%s) to: %d", getPlayerName(responsibleIdx), senderPublicKeyB64.c_str(), getPlayerName(targetPlayerIdx), NetPlay.players[targetPlayerIdx].position);
}

static bool changePosition(UBYTE player, UBYTE position, uint32_t responsibleIdx)
{
	ASSERT_HOST_ONLY(return false);
	ASSERT_OR_RETURN(false, player < MAX_PLAYERS, "Invalid player idx: %" PRIu8, player);
	int i;

	if (NetPlay.players[player].position == position)
	{
		// nothing to do
		return false;
	}

	for (i = 0; i < MAX_PLAYERS; i++)
	{
		if (NetPlay.players[i].position == position)
		{
			if (!isHumanPlayer(i) || isPlayerHostOrAdmin(responsibleIdx))
			{
				debug(LOG_NET, "Swapping positions between players %d(%d) and %d(%d)",
					  player, NetPlay.players[player].position, i, NetPlay.players[i].position);
				std::swap(NetPlay.players[i].position, NetPlay.players[player].position);
				std::swap(NetPlay.players[i].team, NetPlay.players[player].team);
				if (NetPlay.players[player].ready && isHumanPlayer(player) && ingame.JoiningInProgress[player])
				{
					changeReadyStatusImpl(player, false, true);
				}
				if (NetPlay.players[i].ready && isHumanPlayer(i) && ingame.JoiningInProgress[i])
				{
					changeReadyStatusImpl(i, false, true);
				}
				NETBroadcastTwoPlayerInfo(player, i);
				informIfAdminChangedOtherPosition(player, responsibleIdx);
				netPlayersUpdated = true;
				return true;
			}
			else
			{
				debug(LOG_INFO, "Unable to swap positions between players %d(%d) and %d(%d) - lack of privileges",
					  player, NetPlay.players[player].position, i, NetPlay.players[i].position);
				return false;
			}
		}
	}
	debug(LOG_ERROR, "Failed to swap positions for player %d, position %d", (int)player, (int)position);
	if (player < game.maxPlayers && position < game.maxPlayers)
	{
		debug(LOG_NET, "corrupted positions: player (%u) new position (%u) old position (%d)", player, position, NetPlay.players[player].position);
		// Positions were corrupted. Attempt to fix.
		NetPlay.players[player].position = position;
		if (NetPlay.players[player].ready && isHumanPlayer(player) && ingame.JoiningInProgress[player])
		{
			changeReadyStatusImpl(player, false, true);
		}
		NETBroadcastPlayerInfo(player);
		informIfAdminChangedOtherPosition(player, responsibleIdx);
		netPlayersUpdated = true;
		return true;
	}
	return false;
}

static void informIfAdminChangedOtherColor(uint32_t targetPlayerIdx, uint32_t responsibleIdx)
{
	if (!shouldInformOfAdminAction(targetPlayerIdx, responsibleIdx))
	{
		return;
	}

	sendQuickChat(WzQuickChatMessage::INTERNAL_ADMIN_ACTION_NOTICE, realSelectedPlayer, WzQuickChatTargeting::targetAll(), WzQuickChatDataContexts::INTERNAL_ADMIN_ACTION_NOTICE::constructMessageData(WzQuickChatDataContexts::INTERNAL_ADMIN_ACTION_NOTICE::Context::Color, responsibleIdx, targetPlayerIdx));

	std::string senderPublicKeyB64 = base64Encode(getOutputPlayerIdentity(responsibleIdx).toBytes(EcKey::Public));
	debug(LOG_INFO, "Admin %s (%s) changed color of player ([%u] %s) to: [%d] %s", getPlayerName(responsibleIdx), senderPublicKeyB64.c_str(), NetPlay.players[targetPlayerIdx].position, getPlayerName(targetPlayerIdx), NetPlay.players[targetPlayerIdx].colour, getPlayerColourName(targetPlayerIdx));
}

bool changeColour(unsigned player, int col, uint32_t responsibleIdx)
{
	if (col < 0 || col >= MAX_PLAYERS_IN_GUI)
	{
		return false;
	}

	if (player >= MAX_PLAYERS)
	{
		return false;
	}

	if (getPlayerColour(player) == col)
	{
		return false;  // Nothing to do.
	}

	for (unsigned i = 0; i < MAX_PLAYERS; ++i)
	{
		if (getPlayerColour(i) == col)
		{
			if (!isPlayerHostOrAdmin(responsibleIdx) && NetPlay.players[i].allocated)
			{
				return false;  // May not swap.
			}

			debug(LOG_NET, "Swapping colours between players %d(%d) and %d(%d)",
			      player, getPlayerColour(player), i, getPlayerColour(i));
			setPlayerColour(i, getPlayerColour(player));
			setPlayerColour(player, col);
			NETBroadcastTwoPlayerInfo(player, i);
			informIfAdminChangedOtherColor(player, responsibleIdx);
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
		informIfAdminChangedOtherColor(player, responsibleIdx);
		netPlayersUpdated = true;
		return true;
	}
	return false;
}

bool SendColourRequest(UBYTE player, UBYTE col)
{
	if (NetPlay.isHost)			// do or request the change
	{
		return changeColour(player, col, realSelectedPlayer);
	}
	else
	{
		// clients tell the host which color they want
		auto w = NETbeginEncode(NETnetQueue(NetPlay.hostPlayer), NET_COLOURREQUEST);
		NETuint8_t(w, player);
		NETuint8_t(w, col);
		NETend(w);
	}
	return true;
}

static void informIfAdminChangedOtherFaction(uint32_t targetPlayerIdx, uint32_t responsibleIdx)
{
	if (!shouldInformOfAdminAction(targetPlayerIdx, responsibleIdx))
	{
		return;
	}

	sendQuickChat(WzQuickChatMessage::INTERNAL_ADMIN_ACTION_NOTICE, realSelectedPlayer, WzQuickChatTargeting::targetAll(), WzQuickChatDataContexts::INTERNAL_ADMIN_ACTION_NOTICE::constructMessageData(WzQuickChatDataContexts::INTERNAL_ADMIN_ACTION_NOTICE::Context::Faction, responsibleIdx, targetPlayerIdx));

	std::string senderPublicKeyB64 = base64Encode(getOutputPlayerIdentity(responsibleIdx).toBytes(EcKey::Public));
	debug(LOG_INFO, "Admin %s (%s) changed faction of player ([%u] %s) to: %s", getPlayerName(responsibleIdx), senderPublicKeyB64.c_str(), NetPlay.players[targetPlayerIdx].position, getPlayerName(targetPlayerIdx), to_localized_string(static_cast<FactionID>(NetPlay.players[targetPlayerIdx].faction)));
}

bool changeFaction(unsigned player, FactionID faction, uint32_t responsibleIdx)
{
	if (player >= MAX_PLAYERS)
	{
		return false;
	}

	if (NetPlay.players[player].faction == faction)
	{
		return false;  // Nothing to do.
	}

	NetPlay.players[player].faction = static_cast<FactionID>(faction);
	NETBroadcastPlayerInfo(player);

	informIfAdminChangedOtherFaction(player, responsibleIdx);

	return true;
}

static bool SendFactionRequest(UBYTE player, UBYTE faction)
{
	// TODO: needs to be rewritten from scratch
	ASSERT_OR_RETURN(false, faction <= static_cast<UBYTE>(MAX_FACTION_ID), "Invalid faction: %u", (unsigned int)faction);
	if (NetPlay.isHost)			// do or request the change
	{
		changeFaction(player, static_cast<FactionID>(faction), realSelectedPlayer);
		return true;
	}
	else
	{
		// clients tell the host which color they want
		auto w = NETbeginEncode(NETnetQueue(NetPlay.hostPlayer), NET_FACTIONREQUEST);
		NETuint8_t(w, player);
		NETuint8_t(w, faction);
		NETend(w);
	}
	return true;
}

static bool SendPositionRequest(UBYTE player, UBYTE position)
{
	if (NetPlay.isHost)			// do or request the change
	{
		return changePosition(player, position, realSelectedPlayer);
	}
	else
	{
		debug(LOG_NET, "Requesting the host to change our position. From %d to %d", player, position);
		// clients tell the host which position they want
		auto w = NETbeginEncode(NETnetQueue(NetPlay.hostPlayer), NET_POSITIONREQUEST);
		NETuint8_t(w, player);
		NETuint8_t(w, position);
		NETend(w);
	}
	return true;
}

bool recvFactionRequest(NETQUEUE queue)
{
	ASSERT_HOST_ONLY(return true);

	auto r = NETbeginDecode(queue, NET_FACTIONREQUEST);

	UBYTE player, faction;
	NETuint8_t(r, player);
	NETuint8_t(r, faction);
	NETend(r);

	if (player >= MAX_PLAYERS)
	{
		debug(LOG_ERROR, "Invalid NET_FACTIONREQUEST from player %d: Tried to change player %d to faction %d",
		      queue.index, (int)player, (int)faction);
		return false;
	}

	if (whosResponsible(player) != queue.index && !NetPlay.players[queue.index].isAdmin)
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

	return changeFaction(player, newFactionId.value(), queue.index);
}

bool recvColourRequest(NETQUEUE queue)
{
	ASSERT_HOST_ONLY(return true);

	auto r = NETbeginDecode(queue, NET_COLOURREQUEST);

	UBYTE player, col;
	NETuint8_t(r, player);
	NETuint8_t(r, col);
	NETend(r);

	if (player >= MAX_PLAYERS)
	{
		debug(LOG_ERROR, "Invalid NET_COLOURREQUEST from player %d: Tried to change player %d to colour %d",
		      queue.index, (int)player, (int)col);
		return false;
	}

	if (whosResponsible(player) != queue.index && !NetPlay.players[queue.index].isAdmin)
	{
		HandleBadParam("NET_COLOURREQUEST given incorrect params.", player, queue.index);
		return false;
	}

	return changeColour(player, col, queue.index);
}

bool recvPositionRequest(NETQUEUE queue)
{
	ASSERT_HOST_ONLY(return true);

	auto r = NETbeginDecode(queue, NET_POSITIONREQUEST);

	UBYTE	player, position;
	NETuint8_t(r, player);
	NETuint8_t(r, position);
	NETend(r);
	debug(LOG_NET, "Host received position request from player %d to %d", player, position);

	if (player >= MAX_PLAYERS || position >= MAX_PLAYERS)
	{
		debug(LOG_ERROR, "Invalid NET_POSITIONREQUEST from player %d: Tried to change player %d to %d",
		      queue.index, (int)player, (int)position);
		return false;
	}

	if (whosResponsible(player) != queue.index && !NetPlay.players[queue.index].isAdmin)
	{
		HandleBadParam("NET_POSITIONREQUEST given incorrect params.", player, queue.index);
		return false;
	}

	if (locked.position)
	{
		return false;
	}

	if (!changePosition(player, position, queue.index))
	{
		return false;
	}

	wz_command_interface_output_room_status_json(true);
	return true;
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
	auto w = NETbeginEncode(NETnetQueue((!NetPlay.isHost) ? NetPlay.hostPlayer : player), NET_PLAYER_SLOTTYPE_REQUEST);
	NETuint32_t(w, player);
	NETbool(w, isSpectator);
	NETend(w);
	return true;
}

static bool recvPlayerSlotTypeRequestAndPop(WzMultiplayerOptionsTitleUI& titleUI, NETQUEUE queue)
{
	// A player is requesting a slot type change
	// ex. player -> spectator

	uint32_t playerIndex;
	bool desiredIsSpectator = false;
	auto r = NETbeginDecode(queue, NET_PLAYER_SLOTTYPE_REQUEST);
	NETuint32_t(r, playerIndex);
	NETbool(r, desiredIsSpectator);
	NETend(r);

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

	if (locked.spectators)
	{
		return false;
	}

	const char *pPlayerName = getPlayerName(playerIndex, true);
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
				std::string contentText = astringf(_("Spectator \"%s\" would like to become a player."), getPlayerName(playerIndex)); // NOTE: Do not pass true as second parameter, as this only gets shown on the host
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

	auto r = NETbeginDecode(queue, NET_PLAYER_SWAP_INDEX);
	NETuint32_t(r, playerIndexA);
	NETuint32_t(r, playerIndexB);
	NETend(r);

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
			resetLobbyChangePlayerVote(playerIndex);
		}
	}
	std::swap(ingame.hostChatPermissions[playerIndexA], ingame.hostChatPermissions[playerIndexB]);
	std::swap(ingame.muteChat[playerIndexA], ingame.muteChat[playerIndexB]);
	playerSpamMuteNotifyIndexSwap(playerIndexA, playerIndexB);
	multiSyncPlayerSwap(playerIndexA, playerIndexB);
	multiOptionPrefValuesSwap(playerIndexA, playerIndexB);

	if (playerIndexA == selectedPlayer || playerIndexB == selectedPlayer)
	{
		uint32_t oldPlayerIndex = selectedPlayer;
		uint32_t newPlayerIndex = (playerIndexA == selectedPlayer) ? playerIndexB : playerIndexA;
		bool selectedPlayerWasSpectator = wasSpectator[(playerIndexA == selectedPlayer) ? 0 : 1];

		// Send an acknowledgement that we received and are processing the player index swap for us
		auto w = NETbeginEncode(NETnetQueue(NetPlay.hostPlayer), NET_PLAYER_SWAP_INDEX_ACK);
		NETuint32_t(w, oldPlayerIndex);
		NETuint32_t(w, newPlayerIndex);
		NETend(w);

		// 1.) Basically what happens in NETjoinGame after receiving NET_ACCEPTED

		selectedPlayer = newPlayerIndex;
		realSelectedPlayer = selectedPlayer;
		debug(LOG_NET, "NET_PLAYER_SWAP_INDEX received for me. Switching from player %" PRIu32 " to player %" PRIu32 "", oldPlayerIndex,newPlayerIndex);

		// Must be called *after* we swap the selectedPlayer / realSelectedPlayer
		swapPlayerMultiStatsLocal(playerIndexA, playerIndexB);

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

		if ((game.blindMode != BLIND_MODE::NONE) && (selectedPlayer < MAX_PLAYERS))
		{
			std::string blindLobbyMessage = _("BLIND LOBBY NOTICE:");
			blindLobbyMessage += "\n- ";
			blindLobbyMessage += astringf(_("You have been assigned the codename: \"%s\""), getPlayerGenericName(selectedPlayer));
			displayRoomNotifyMessage(blindLobbyMessage.c_str());
		}
	}
	else
	{
		swapPlayerMultiStatsLocal(playerIndexA, playerIndexB);

		// TODO: display a textual description of the move
		// just wait for the new player info to be broadcast by the host
	}

	ActivityManager::instance().updateMultiplayGameData(game, ingame, NETGameIsLocked());
	netPlayersUpdated = true;	// update the player box.

	return SwapPlayerIndexesResult::SUCCESS;
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
		if (highlight || down)
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
		if (currentPopoverMenu)
		{
			currentPopoverMenu->closeMenu();
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
	std::shared_ptr<PopoverMenuWidget> createOptionsPopoverForm();

private:
	std::weak_ptr<WzMultiplayerOptionsTitleUI> weakTitleUI;
	std::vector<std::shared_ptr<WzPlayerBoxTabButton>> tabButtons;
	std::shared_ptr<WzPlayerBoxOptionsButton> optionsButton;
	std::shared_ptr<PopoverMenuWidget> currentPopoverMenu;
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

std::shared_ptr<PopoverMenuWidget> WzPlayerBoxTabs::createOptionsPopoverForm()
{
	auto popoverMenu = PopoverMenuWidget::make();

	// create all the buttons / option rows
	auto addOptionsSpacer = [&popoverMenu]() -> std::shared_ptr<WIDGET> {
		auto spacerWidget = std::make_shared<WIDGET>();
		spacerWidget->setGeometry(0, 0, 1, 5);
		popoverMenu->addMenuItem(spacerWidget, false);
		return spacerWidget;
	};
	auto addOptionsCheckbox = [&popoverMenu](const WzString& text, bool isChecked, bool isDisabled, const std::function<void (WzAdvCheckbox& button)>& onClickFunc) -> std::shared_ptr<WzAdvCheckbox> {
		auto pCheckbox = WzAdvCheckbox::make(text, "");
		pCheckbox->FontID = font_regular;
		pCheckbox->setOuterVerticalPadding(4);
		pCheckbox->setInnerHorizontalPadding(5);
		pCheckbox->setIsChecked(isChecked);
		if (isDisabled)
		{
			pCheckbox->setState(WBUT_DISABLE);
		}
		pCheckbox->setGeometry(0, 0, pCheckbox->idealWidth(), pCheckbox->idealHeight());
		if (onClickFunc)
		{
			pCheckbox->addOnClickHandler([onClickFunc](W_BUTTON& button){
				auto checkBoxButton = std::dynamic_pointer_cast<WzAdvCheckbox>(button.shared_from_this());
				ASSERT_OR_RETURN(, checkBoxButton != nullptr, "checkBoxButton is null");
				onClickFunc(*checkBoxButton);
			});
		}
		popoverMenu->addMenuItem(pCheckbox, false);
		return pCheckbox;
	};

	bool hasOpenSpectatorSlots = hasOpenSpectatorOnlySlots();
	std::weak_ptr<WzMultiplayerOptionsTitleUI> weakTitleUICopy = weakTitleUI;
	addOptionsCheckbox(_("Enable Spectator Join"), hasOpenSpectatorSlots, !hasOpenSpectatorSlots && !canAddSpectatorOnlySlots(), [weakTitleUICopy](WzAdvCheckbox& button){
		bool enableValue = button.isChecked();
		widgScheduleTask([enableValue, weakTitleUICopy]{
			auto strongTitleUI = weakTitleUICopy.lock();
			ASSERT_OR_RETURN(, strongTitleUI != nullptr, "No Title UI?");
			enableSpectatorJoin(enableValue);
			strongTitleUI->updatePlayers();
		});
	});
	addOptionsSpacer();
	addOptionsCheckbox(_("Lock Teams"), locked.teams, false, [](WzAdvCheckbox& button){
		locked.teams = button.isChecked();
		sendHostConfig();
	});
	addOptionsCheckbox(_("Lock Position"), locked.position, false, [](WzAdvCheckbox& button){
		locked.position = button.isChecked();
		sendHostConfig();
	});

	int32_t idealMenuHeight = popoverMenu->idealHeight();
	int32_t menuHeight = idealMenuHeight;
	if (menuHeight > screenHeight)
	{
		menuHeight = screenHeight;
	}
	popoverMenu->setGeometry(popoverMenu->x(), popoverMenu->y(), popoverMenu->idealWidth(), menuHeight);

	return popoverMenu;
}

void WzPlayerBoxTabs::displayOptionsOverlay(const std::shared_ptr<WIDGET>& psParent)
{
	if (currentPopoverMenu)
	{
		currentPopoverMenu->closeMenu();
	}

	optionsButton->setState(WBUT_CLICKLOCK);
	currentPopoverMenu = createOptionsPopoverForm();
	auto psWeakSelf = std::weak_ptr<WzPlayerBoxTabs>(std::static_pointer_cast<WzPlayerBoxTabs>(shared_from_this()));
	currentPopoverMenu->openMenu(psParent, PopoverWidget::Alignment::LeftOfParent, Vector2i(0, 1), [psWeakSelf](){
		if (auto strongSelf = psWeakSelf.lock())
		{
			strongSelf->optionsButton->setState(0);
		}
	});
}

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
	auto titleUI = std::static_pointer_cast<WzMultiplayerOptionsTitleUI>(shared_from_this());

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
	playerRow->setTransparentToMouse(true); // ensure clicks on this display-only row have no effect
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

	if (!players)
	{
		return;
	}

	auto titleUI = std::static_pointer_cast<WzMultiplayerOptionsTitleUI>(shared_from_this());

	if (isBlindSimpleLobby(game.blindMode) && !NetPlay.isHost)
	{
		// Display custom "waiting room" that completely blinds the current player

		auto blindWaitingRoom = makeWzPlayerBlindWaitingRoom(titleUI);
		blindWaitingRoom->id = MULTIOP_BLIND_WAITING_ROOM;
		playersForm->attach(blindWaitingRoom);
		blindWaitingRoom->setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
			auto psParent = psWidget->parent();
			ASSERT_OR_RETURN(, psParent != nullptr, "No parent?");
			int x0 = PLAYERBOX_X0 - 1;
			int y0 = 1;
			int width = MULTIOP_PLAYERSW - (PLAYERBOX_X0 * 2);
			int height = psParent->height() - y0 - 1;
			psWidget->setGeometry(x0, y0, width, height);
		}));
	}
	else
	{
		// Normal case - display players / spectators lists

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

	auto w = NETbeginEncode(NETbroadcastQueue(), NET_FIREUP);
	NETuint32_t(w, randomSeed);
	NETend(w);
	printSearchPath();
	gameSRand(randomSeed);  // Set the seed for the synchronised random number generator. The clients will use the same seed.
}

void to_json(nlohmann::json& j, const KickRedirectInfo& v)
{
	j = nlohmann::json::object();
	j["c"] = v.connList;
	if (v.gamePassword.has_value())
	{
		j["p"] = v.gamePassword.value();
	}
	j["s"] = (v.asSpectator) ? 1 : 0;
}

void from_json(const nlohmann::json& j, KickRedirectInfo& v)
{
	v.connList = j.at("c").get<std::vector<JoinConnectionDescription>>();
	v.gamePassword = nullopt;
	auto it = j.find("p");
	if (it != j.end())
	{
		v.gamePassword = it.value().get<std::string>();
	}
	v.asSpectator = false;
	it = j.find("s");
	if (it != j.end())
	{
		v.asSpectator = it.value().get<uint32_t>() != 0;
	}
}

// host kicks a player from a game.
void kickPlayer(uint32_t player_id, const char *reason, LOBBY_ERROR_TYPES type, bool banPlayer)
{
	ASSERT_HOST_ONLY(return);

	debug(LOG_INFO, "Kicking player %u (%s). Reason: %s", (unsigned int)player_id, getPlayerName(player_id), reason);

	// send a kick msg
	auto w = NETbeginEncode(NETbroadcastQueue(), NET_KICK);
	NETuint32_t(w, player_id);
	NETstring(w, reason, MAX_KICK_REASON);
	NETenum(w, type);
	NETend(w);
	NETflush();
	wzDelay(300);

	ActivityManager::instance().hostKickPlayer(NetPlay.players[player_id], type, reason);

	if (banPlayer && NetPlay.players[player_id].allocated)
	{
		addIPToBanList(NetPlay.players[player_id].IPtextAddress, NetPlay.players[player_id].name);
	}

	NETplayerKicked(player_id);
}

bool kickRedirectPlayer(uint32_t player_id, const KickRedirectInfo& redirectInfo)
{
	ASSERT_HOST_ONLY(return false);
	ASSERT_OR_RETURN(false, player_id < NetPlay.players.size(), "Invalid player_id: %" PRIu32, player_id);
	ASSERT_OR_RETURN(false, ingame.localJoiningInProgress, "Only if the game hasn't started yet");

	std::string redirectStr;
	try {
		nlohmann::json obj = redirectInfo;
		redirectStr = obj.dump(-1, ' ', false, nlohmann::json::error_handler_t::replace);
	}
	catch (const std::exception& e) {
		debug(LOG_ERROR, "Failed to encode redirect string with error: %s", e.what());
		return false;
	}

	if (redirectStr.size() > MAX_KICK_REASON)
	{
		debug(LOG_ERROR, "Encoded redirect string length (%zu) exceeds maximum supported - redirect failed", redirectStr.size());
		return false;
	}

	// send a kick msg, with redirect info, to the player
	LOBBY_ERROR_TYPES type = ERROR_REDIRECT;
	auto w = NETbeginEncode(NETnetQueue(player_id), NET_KICK);
	NETuint32_t(w, player_id);
	NETstring(w, redirectStr.c_str(), MAX_KICK_REASON);
	NETenum(w, type);
	NETend(w);

	// send a kick about this player to everyone else (without the redirect string)
	w = NETbeginEncode(NETbroadcastQueue(player_id), NET_KICK);
	NETuint32_t(w, player_id);
	char emptyReason[1] = {};
	NETstring(w, emptyReason, 0);
	NETenum(w, type);
	NETend(w);

	// flush
	NETflush();

	ActivityManager::instance().hostKickPlayer(NetPlay.players[player_id], type, redirectStr);
	NETplayerKicked(player_id);
	return true;
}

bool kickRedirectPlayer(uint32_t player_id, JoinConnectionDescription::JoinConnectionType connectionType, uint16_t newPort, bool asSpectator, optional<std::string> gamePassword)
{
	ASSERT_HOST_ONLY(return false);

	KickRedirectInfo redirectInfo;
	redirectInfo.connList.push_back(JoinConnectionDescription(connectionType, "=", newPort)); // special "=" value is treated as "same address" on the client side
	redirectInfo.gamePassword = gamePassword;
	redirectInfo.asSpectator = asSpectator;

	return kickRedirectPlayer(player_id, redirectInfo);
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
	std::weak_ptr<ChatBoxWidget> weakSelf = std::weak_ptr<ChatBoxWidget>(std::dynamic_pointer_cast<ChatBoxWidget>(shared_from_this()));

	attach(messages = ScrollableListWidget::make());
	messages->setSnapOffset(true);
	messages->setStickToBottom(true);
	messages->setPadding({3, 4, 3, 4});
	messages->setItemSpacing(1);

	handleConsoleMessage = std::make_shared<CONSOLE_MESSAGE_LISTENER>([&](ConsoleMessage const &message) -> void
	{
		addMessage(buildMessage(message.sender, message.text));
	});

	attach(quickChatButton = std::make_shared<ChatBoxButton>());
	setQuickChatButtonDisplay();
	quickChatButton->addOnClickHandler([weakSelf](W_BUTTON& widg) {
		widgScheduleTask([weakSelf]() {
			auto strongParentForm = weakSelf.lock();
			ASSERT_OR_RETURN(, strongParentForm != nullptr, "No parent form");
			strongParentForm->openQuickChatOverlay();
		});
	});

	if (NetPlay.bComms)
	{
		attach(optionsButton = std::make_shared<ChatBoxButton>());
		optionsButton->setString("\u2699"); // "⚙"
		optionsButton->setTip(_("Chat Options"));
		optionsButton->FontID = font_regular;
		int minButtonWidthForText = iV_GetTextWidth(optionsButton->pText, optionsButton->FontID);
		optionsButton->setGeometry(0, 0, minButtonWidthForText + 10, iV_GetTextLineSize(optionsButton->FontID));
		optionsButton->addOnClickHandler([weakSelf](W_BUTTON& widg) {
			widgScheduleTask([weakSelf]() {
				auto strongParentForm = weakSelf.lock();
				ASSERT_OR_RETURN(, strongParentForm != nullptr, "No parent form");
				strongParentForm->displayOptionsOverlay(strongParentForm->optionsButton);
			});
		});
	}

	W_EDBINIT sEdInit;
	sEdInit.formID = MULTIOP_CHATBOX;
	sEdInit.id = MULTIOP_CHATEDIT;
	sEdInit.pUserData = nullptr;
	sEdInit.pBoxDisplay = displayChatEdit;
	editBox = std::make_shared<W_EDITBOX>(&sEdInit);
	attach(editBox);
	setEditBoxDisplay();
	editBox->setOnReturnHandler([weakSelf](W_EDITBOX& widg) {
		// don't send empty lines to other players in the lobby
		WzString str = widg.getString();
		if (str.isEmpty())
		{
			return;
		}

		auto strongParentForm = weakSelf.lock();
		ASSERT_OR_RETURN(, strongParentForm != nullptr, "No parent form");

		switch (strongParentForm->currentMode)
		{
			case ChatBoxWidget::ChatBoxSendMode::DISABLED:
				displayRoomSystemMessage(_("The host has disabled free chat. Please use Quick Chat."));
				return;
			case ChatBoxWidget::ChatBoxSendMode::HOSTMSG_ONLY:
				// Always ensure sent message starts with /hostmsg
				if (!str.startsWith(LOBBY_COMMAND_PREFIX "hostmsg"))
				{
					str = LOBBY_COMMAND_PREFIX "hostmsg " + str;
				}
				break;
			case ChatBoxWidget::ChatBoxSendMode::ENABLED:
				// just send message as-is
				break;
		}

		sendRoomChatMessage(str.toUtf8().c_str(), (strongParentForm->currentMode != ChatBoxWidget::ChatBoxSendMode::ENABLED));
		widg.setString("");
	});
	editBox->setOnTabHandler([weakSelf](W_EDITBOX& widg) -> bool {
		// on tab handler
		widgScheduleTask([weakSelf]() {
			auto strongParentForm = weakSelf.lock();
			ASSERT_OR_RETURN(, strongParentForm != nullptr, "No parent form");
			strongParentForm->openQuickChatOverlay();
		});
		return true;
	});

	consoleAddMessageListener(handleConsoleMessage);

	// Initialize the overlay screen
	quickChatOverlayScreen = W_SCREEN::make();
	auto newRootFrm = W_FULLSCREENOVERLAY_CLICKFORM::make();
	std::weak_ptr<ChatBoxWidget> psWeakChatBoxWidget = std::dynamic_pointer_cast<ChatBoxWidget>(shared_from_this());
	newRootFrm->onClickedFunc = [psWeakChatBoxWidget]() {
		widgScheduleTask([psWeakChatBoxWidget]{
			auto psChatBoxWidget = psWeakChatBoxWidget.lock();
			ASSERT_OR_RETURN(, psChatBoxWidget != nullptr, "ChatBoxWidget is null");
			psChatBoxWidget->closeQuickChatOverlay();
		});
	};
	newRootFrm->onCancelPressed = newRootFrm->onClickedFunc;
	quickChatOverlayScreen->psForm->attach(newRootFrm);

	// Add the Quick Chat form to the overlay screen form
	quickChatForm = createQuickChatForm(WzQuickChatContext::Lobby, [psWeakChatBoxWidget]() {
		// on quick chat sent: close the overlay
		widgScheduleTask([psWeakChatBoxWidget]{
			auto psChatBoxWidget = psWeakChatBoxWidget.lock();
			ASSERT_OR_RETURN(, psChatBoxWidget != nullptr, "ChatBoxWidget is null");
			psChatBoxWidget->closeQuickChatOverlay();
		});
	});
	newRootFrm->attach(quickChatForm);
	quickChatForm->setCalcLayout([psWeakChatBoxWidget](WIDGET* psWidget){
		auto psChatBoxWidget = psWeakChatBoxWidget.lock();
		ASSERT_OR_RETURN(, psChatBoxWidget != nullptr, "ChatBoxWidget is null");
		int quickChatWidth = psChatBoxWidget->width();
		int x0 = psChatBoxWidget->screenPosX();
		int y0 = psChatBoxWidget->calculateQuickChatScreenPosY();
		psWidget->setGeometry(x0, y0, quickChatWidth, psWidget->idealHeight());
	});

	handleUpdateInHostedLobby();
}

void ChatBoxWidget::handleUpdateInHostedLobby()
{
	if (optionsButton)
	{
		if (bInActualHostedLobby)
		{
			optionsButton->setState(0);
		}
		else
		{
			optionsButton->setState(WBUT_DISABLE);
		}
	}
}

void ChatBoxWidget::setQuickChatButtonDisplay()
{
	ASSERT_OR_RETURN(, quickChatButton != nullptr, "No quick chat button?");
	bool compactButtonForm = (currentMode == ChatBoxSendMode::ENABLED);
	if (compactButtonForm)
	{
		quickChatButton->setString("+");
	}
	else
	{
		WzString str = WzString("+ ") + _("Quick Chat");
		quickChatButton->setString(str);
	}
	quickChatButton->setTip(_("Quick Chat"));
	quickChatButton->FontID = font_regular_bold;
	int minButtonWidthForText = iV_GetTextWidth(quickChatButton->pText, quickChatButton->FontID);
	quickChatButton->setGeometry(quickChatButton->x(), quickChatButton->y(), minButtonWidthForText + 20, iV_GetTextLineSize(quickChatButton->FontID));
}

void ChatBoxWidget::setEditBoxDisplay()
{
	switch (currentMode)
	{
		case ChatBoxSendMode::DISABLED:
			editBox->setPlaceholder(_("Press the Tab key to open Quick Chat."));
			editBox->setPlaceholderTextColor(WZCOL_TEXT_MEDIUM);
			editBox->setTip(_("The host has disabled free chat. Please use Quick Chat."));
			editBox->setState(WEDBS_DISABLE);
			break;
		case ChatBoxSendMode::HOSTMSG_ONLY:
			editBox->setPlaceholder(_("Press the Tab key to open Quick Chat."));
			editBox->setPlaceholderTextColor(WZCOL_TEXT_MEDIUM);
			editBox->setTip(_("The host has disabled free chat. Please use Quick Chat or /hostmsg commands."));
			editBox->setState(0);
			break;
		case ChatBoxSendMode::ENABLED:
			editBox->setPlaceholder("");
			editBox->setPlaceholderTextColor(nullopt);
			editBox->setTip("");
			editBox->setState(0);
			break;
	}
}

void ChatBoxWidget::setSendMode(ChatBoxSendMode mode)
{
	if (mode == currentMode)
	{
		return;
	}
	currentMode = mode;
	// Update UI
	setQuickChatButtonDisplay();
	setEditBoxDisplay();
	geometryChanged();
	// message the user
	switch (mode)
	{
		case ChatBoxSendMode::DISABLED:
			displayRoomSystemMessage(_("The host has disabled free chat. Please use Quick Chat."));
			break;
		case ChatBoxSendMode::HOSTMSG_ONLY:
			displayRoomSystemMessage(_("The host has disabled free chat. Please use Quick Chat or /hostmsg commands."));
			break;
		case ChatBoxSendMode::ENABLED:
			displayRoomSystemMessage(_("The host has enabled free chat for you."));
			break;
	}
}

int32_t ChatBoxWidget::calculateQuickChatScreenPosY()
{
	int32_t idealQuickChatHeight = quickChatForm->idealHeight();
	// Try to position it above the chat widget
	int32_t screenY0 = screenPosY() - idealQuickChatHeight;
	return std::max(screenY0, 0);
}

void ChatBoxWidget::takeFocus()
{
	if (currentMode != ChatBoxSendMode::DISABLED)
	{
		// give the edit box the focus
		W_CONTEXT context = W_CONTEXT::ZeroContext();
		context.mx			= mouseX();
		context.my			= mouseY();
		editBox->simulateClick(&context, true);
	}
	else
	{
		// when the edit box is disabled, keyboard events (tab, enter) should open the quick chat
		// to handle this, tell the form that the ChatBoxWidget has focus
		if (auto lockedScreen = screenPointer.lock())
		{
			lockedScreen->setFocus(shared_from_this());
			hasFocus = true;
		}
		else
		{
			// If the ChatBoxWidget isn't currently attached to a screen when this is triggered, focus issues may occur
			ASSERT(false, "ChatBoxWidget is not attached to any screen?");
		}
	}
}

void ChatBoxWidget::run(W_CONTEXT *psContext)
{
	if (!hasFocus) { return; }

	if (keyPressed(KEY_TAB))
	{
		std::weak_ptr<ChatBoxWidget> weakSelf = std::weak_ptr<ChatBoxWidget>(std::dynamic_pointer_cast<ChatBoxWidget>(shared_from_this()));
		widgScheduleTask([weakSelf]() {
			auto strongParentForm = weakSelf.lock();
			ASSERT_OR_RETURN(, strongParentForm != nullptr, "No parent form");
			strongParentForm->openQuickChatOverlay();
		});
		inputLoseFocus();
	}
	if (keyPressed(KEY_ESC))
	{
		// "eat" the ESC keypress, just as if the editbox had focus
		inputLoseFocus();
	}
}

void ChatBoxWidget::focusLost()
{
	hasFocus = false;
}

void ChatBoxWidget::openQuickChatOverlay()
{
	quickChatOverlayScreen->screenSizeDidChange(screenWidth, screenHeight, screenWidth, screenHeight); // trigger a screenSizeDidChange so it can relayout (if screen size changed since last time it was registered as an overlay screen)
	widgRegisterOverlayScreen(quickChatOverlayScreen, 1);
	editBox->stopEditing();

	// Set the focus to the quickChatForm by simulating a click on it
	W_CONTEXT context = W_CONTEXT::ZeroContext();
	context.mx			= quickChatForm->screenPosX();
	context.my			= quickChatForm->screenPosY();
	quickChatForm->clicked(&context, WKEY_PRIMARY);
	quickChatForm->released(&context, WKEY_PRIMARY);
}

void ChatBoxWidget::closeQuickChatOverlay()
{
	widgRemoveOverlayScreen(quickChatOverlayScreen);
	// Auto-click edit box
	W_CONTEXT sContext = W_CONTEXT::ZeroContext();
	editBox->clicked(&sContext);
}

ChatBoxWidget::~ChatBoxWidget()
{
	consoleRemoveMessageListener(handleConsoleMessage);
	widgRemoveOverlayScreen(quickChatOverlayScreen);
	if (optionsOverlayScreen)
	{
		widgRemoveOverlayScreen(optionsOverlayScreen);
	}
}

void ChatBoxWidget::displayOptionsOverlay(const std::shared_ptr<WIDGET>& psParent)
{
	auto lockedScreen = screenPointer.lock();
	ASSERT(lockedScreen != nullptr, "The WzPlayerBoxTabs does not have an associated screen pointer?");

	// Initialize the options overlay screen
	optionsOverlayScreen = W_SCREEN::make();
	auto newRootFrm = W_FULLSCREENOVERLAY_CLICKFORM::make();
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

	// Add the Chat Options form to the overlay screen form
	auto chatOptionsForm = createChatOptionsForm(NetPlay.isHost, []() {
		// on settings change, refresh the chat box to reflect permission changes
		addChatBox();
	});
	newRootFrm->attach(chatOptionsForm);
	chatOptionsForm->setCalcLayout([psWeakChatBoxWidget](WIDGET* psWidget){
		auto psChatBoxWidget = psWeakChatBoxWidget.lock();
		ASSERT_OR_RETURN(, psChatBoxWidget != nullptr, "ChatBoxWidget is null");
		int chatBoxScreenPosX = psChatBoxWidget->screenPosX();
		int chatBoxScreenWidth = psChatBoxWidget->width();
		int idealChatOptionsWidth = psWidget->idealWidth();
		int maxWidth = screenWidth - 40;
		int w = std::min(idealChatOptionsWidth, maxWidth);
		int x0 = chatBoxScreenPosX + (chatBoxScreenWidth - w) / 2;
		// Try to position it above the chat widget
		int idealChatOptionsHeight = psWidget->idealHeight();
		int y0 = std::max(20, psChatBoxWidget->screenPosY() - idealChatOptionsHeight);
		int maxHeight = screenHeight - y0 - 20;
		psWidget->setGeometry(x0, y0, w, std::min(maxHeight, idealChatOptionsHeight));
		// idealHeight may have changed due to the width change, so check and set again...
		int lastIdealChatOptionsHeight = idealChatOptionsHeight;
		idealChatOptionsHeight = psWidget->idealHeight();
		if (lastIdealChatOptionsHeight != idealChatOptionsHeight)
		{
			y0 = std::max(20, psChatBoxWidget->screenPosY() - idealChatOptionsHeight);
			psWidget->setGeometry(x0, y0, w, std::min(maxHeight, idealChatOptionsHeight));
		}
	});

	widgRegisterOverlayScreenOnTopOfScreen(optionsOverlayScreen, lockedScreen);
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

constexpr int MULTIOP_CHATEDITAREA_INSET = 5;
constexpr int MULTIOP_CHATEDITAREA_INTERNAL_PADDING = 6;
constexpr int MULTIOP_CHATEDITAREA_HEIGHT = MULTIOP_CHATEDITH + (MULTIOP_CHATEDITAREA_INSET * 2);

void ChatBoxWidget::geometryChanged()
{
	auto messagesHeight = height() - MULTIOP_CHATEDITAREA_HEIGHT - 1;
	messages->setGeometry(1, 1, width() - 2, messagesHeight);

	int messageEditAreaY0 = messages->y() + messagesHeight + MULTIOP_CHATEDITAREA_INSET;

	quickChatButton->setGeometry(MULTIOP_CHATEDITAREA_INSET, messageEditAreaY0, quickChatButton->width(), MULTIOP_CHATEDITH);
	if (optionsButton)
	{
		optionsButton->setGeometry(width() - MULTIOP_CHATEDITAREA_INSET - optionsButton->width(), messageEditAreaY0, optionsButton->width(), MULTIOP_CHATEDITH);
	}
	int buttonsWidth = quickChatButton->width() + ((optionsButton) ? optionsButton->width() : 0);

	int chatEditBoxWidth = width() - (MULTIOP_CHATEDITAREA_INSET * 2) - buttonsWidth - (MULTIOP_CHATEDITAREA_INTERNAL_PADDING * 2);
	editBox->setGeometry(quickChatButton->x() + quickChatButton->width() + MULTIOP_CHATEDITAREA_INTERNAL_PADDING, messageEditAreaY0, chatEditBoxWidth, MULTIOP_CHATEDITH);
}

void ChatBoxWidget::display(int xOffset, int yOffset)
{
	IntFormAnimated::display(xOffset, yOffset);

	int x0 = xOffset + x();
	int y0 = yOffset + y();

	// draws the line at the bottom of the multiplayer join dialog separating the chat
	// box from the input area
	int messageEditAreaY0 = y0 + messages->y() + messages->height();
	int messageEditAreaX0 = x0;

	pie_UniTransBoxFill(messageEditAreaX0 + 1, messageEditAreaY0, messageEditAreaX0 + width() - 1, y0 + height() - 1, pal_RGBA(0,0,100,80));

	iV_Line(messageEditAreaX0 + 1, messageEditAreaY0, messageEditAreaX0 + width() - 1, messageEditAreaY0, WZCOL_FORM_DARK);
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
		paragraph->addText(WzString::fromUtf8(message.text));
		break;

	case RoomMessagePlayer:
	{
		ASSERT(message.sender.get() != nullptr, "Null message sender?");
		paragraph->setFont(font_small);
		paragraph->setFontColour({0xc0, 0xc0, 0xc0, 0xff});
		paragraph->addText(WzString::fromUtf8(formatLocalDateTime("%H:%M", message.time)));

		auto playerNameWidget = std::make_shared<ChatBoxPlayerNameWidget>(message.sender);
		paragraph->addWidget(playerNameWidget, playerNameWidget->aboveBase());

		paragraph->setFont(font_regular);
		paragraph->setShadeColour({0, 0, 0, 0});
		bool specSender = (*message.sender)->isSpectator && !message.sender->isHost();
		paragraph->setFontColour((!specSender) ? WZCOL_WHITE : WZCOL_TEXT_MEDIUM);
		paragraph->addText(WzString::fromUtf8(astringf(" %s", message.text.c_str())));

		break;
	}

	case RoomMessageNotify:
	default:
		paragraph->setFontColour(WZCOL_YELLOW);
		paragraph->addText(WzString::fromUtf8(message.text));
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

	auto desiredSendMode = (selectedPlayer < MAX_CONNECTED_PLAYERS && ingame.hostChatPermissions[selectedPlayer]) ? ChatBoxWidget::ChatBoxSendMode::ENABLED : ChatBoxWidget::ChatBoxSendMode::HOSTMSG_ONLY;

	auto psExistingChatBoxWidget = widgGetFromID(psWScreen, MULTIOP_CHATBOX);
	if (psExistingChatBoxWidget)
	{
		// chat is already up
		// update send mode if needed
		if (auto chatBox = dynamic_cast<ChatBoxWidget *>(psExistingChatBoxWidget))
		{
			chatBox->setSendMode(desiredSendMode);
		}
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
	chatBox->setSendMode(desiredSendMode);
	chatBox->initializeMessages(preserveOldChat);

	addSideText(FRONTEND_SIDETEXT4, MULTIOP_CHATBOXX - 6, MULTIOP_CHATBOXY, _("CHAT"));

	if (!getModList().empty())
	{
		WzString modListMessage = _("Mod: ");
		modListMessage += getModList().c_str();
		displayRoomSystemMessage(modListMessage.toUtf8().c_str());
	}
}

// ////////////////////////////////////////////////////////////////////////////
static void updateInActualHostedLobby(bool value)
{
	bInActualHostedLobby = value;
	auto pChatBox = dynamic_cast<ChatBoxWidget *>(widgGetFromID(psWScreen, MULTIOP_CHATBOX));
	if (pChatBox)
	{
		pChatBox->handleUpdateInHostedLobby();
	}
}

////////////////////////////////////////////////////////////////////////////
static void stopJoining(std::shared_ptr<WzTitleUI> parent, LOBBY_ERROR_TYPES errorResult)
{
	updateInActualHostedLobby(false);

	reloadMPConfig(); // reload own settings
	cancelOrDismissVoteNotifications();
	cancelOrDismissNotificationIfTag([](const std::string& tag) {
		return (tag.rfind(SLOTTYPE_TAG_PREFIX, 0) == 0);
	});

	debug(LOG_NET, "player %u (Host is %s) stopping.", selectedPlayer, NetPlay.isHost ? "true" : "false");

	pie_LoadBackDrop(SCREEN_RANDOMBDROP);

	if (NetPlay.isHost) // cancel a hosted game.
	{
		// annouce we are leaving...
		debug(LOG_NET, "Host is quitting game...");
		auto w = NETbeginEncode(NETbroadcastQueue(), NET_HOST_DROPPED);
		NETend(w);
		sendLeavingMsg();								// say goodbye
		NETclose();										// quit running game.
		ActivityManager::instance().hostLobbyQuit(errorResult);
		changeTitleUI(wzTitleUICurrent);				// refresh options screen.
		ingame.localJoiningInProgress = false;
		return;
	}
	else if (ingame.localJoiningInProgress)				// cancel a joined game.
	{
		auto origInGameSide = ingame.side;

		debug(LOG_NET, "Canceling game...");
		sendLeavingMsg();								// say goodbye
		NETclose();										// quit running game.

		NET_clearDownloadingWZFiles();
		ingame.localJoiningInProgress = false;			// reset local flags
		ingame.localOptionsReceived = false;

		if (origInGameSide == InGameSide::MULTIPLAYER_CLIENT)
		{
			saveMultiOptionPrefValues(sPlayer, selectedPlayer); // persist any changes to multioption preferences

			ingame.side = InGameSide::HOST_OR_SINGLEPLAYER; // must reset to default so that rebuildSearchPath properly rebuilds local mods paths
			bool needsLevReload = !game.modHashes.empty();
			game.modHashes.clear(); // must clear this so that when search paths are reloaded we don't reload mods downloaded for the last game
			rebuildSearchPath(mod_multiplay, true);
			if (needsLevReload)
			{
				// Clear & reload level list
				levShutDown();
				levInitialise();
				pal_Init(); // Update palettes.
				if (!buildMapList(false))
				{
					debug(LOG_FATAL, "Failed to rebuild map / level list?");
				}
			}
		}

		// joining and host was transferred.
		if (origInGameSide == InGameSide::MULTIPLAYER_CLIENT && NetPlay.isHost)
		{
			NetPlay.isHost = false;
		}

		ActivityManager::instance().joinedLobbyQuit(errorResult);
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
	ActivityManager::instance().joinedLobbyQuit(errorResult);
	NETremRedirects();
	changeTitleUI(parent);
	selectedPlayer = 0;
	realSelectedPlayer = 0;
	for (auto& player : NetPlay.players)
	{
		player.resetAll();
	}
	NetPlay.players.resize(MAX_CONNECTED_PLAYERS);
	for (size_t i = 0; i < NetPlay.scriptSetPlayerDataStrings.size(); i++)
	{
		NetPlay.scriptSetPlayerDataStrings[i].clear();
	}
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

static void updateMapSettings(LEVEL_DATASET *mapData)
{
	ASSERT_OR_RETURN(, mapData != nullptr, "Invalid mapData?");
	sstrcpy(game.map, mapData->pName.c_str());
	game.hash = levGetFileHash(mapData);
	game.maxPlayers = mapData->players;
	game.isMapMod = CheckForMod(mapData->realFileName);
	game.isRandom = CheckForRandom(mapData->realFileName, mapData->apDataFiles[0].c_str());
}

bool blindModeFromStr(const WzString& str, BLIND_MODE& mode_output)
{
	const std::unordered_map<WzString, BLIND_MODE> mappings = {
		{"none", BLIND_MODE::NONE},
		{"blind_lobby", BLIND_MODE::BLIND_LOBBY},
		{"blind_lobby_simple_lobby", BLIND_MODE::BLIND_LOBBY_SIMPLE_LOBBY},
		{"blind_game", BLIND_MODE::BLIND_GAME},
		{"blind_game_simple_lobby", BLIND_MODE::BLIND_GAME_SIMPLE_LOBBY},
	};

	auto it = mappings.find(str);
	if (it != mappings.end())
	{
		mode_output = it->second;
		return true;
	}

	mode_output = BLIND_MODE::NONE;
	return false;
}

static bool loadMapChallengeSettings(WzConfig& ini)
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
		locked.spectators = ini.value("spectators", challengeActive).toBool();
		locked.name = ini.value("name", false).toBool();
		locked.readybeforefull = ini.value("readybeforefull", (getHostLaunch() == HostLaunch::Autohost)).toBool();
	}
	ini.endGroup();

	const bool bIsAutoHostOrAutoGame = getHostLaunch() == HostLaunch::Skirmish || getHostLaunch() == HostLaunch::Autohost;
	if (challengeActive || bIsAutoHostOrAutoGame)
	{
		ini.beginGroup("challenge");
		{
			char mapName[256] = {0};
			sstrcpy(mapName, ini.value("map", game.map).toWzString().toUtf8().c_str());
			Sha256 mapHash = levGetMapNameHash(mapName);

			LEVEL_DATASET* mapData = levFindDataSet(mapName, &mapHash);
			if (!mapData)
			{
				code_part log_level = (bIsAutoHostOrAutoGame) ? LOG_ERROR : LOG_FATAL;
				debug(log_level, "Map %s not found!", mapName);
				if (wz_command_interface_enabled())
				{
					std::string mapNameB64 = base64Encode(std::vector<unsigned char>(mapName, mapName + strlen(mapName)));
					wz_command_interface_output("WZEVENT: mapNotFound: %s %s\n", mapNameB64.c_str(), mapHash.toString().c_str());
				}
				if (bIsAutoHostOrAutoGame && headlessGameMode())
				{
					wzQuit(1);
				}
				return false;
			}
			sstrcpy(game.map, mapName);
			game.hash = mapHash;
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

			// Allow setting "blind mode"
			WzString blindMode_str = ini.value("blindMode", "none").toWzString();
			if (!blindModeFromStr(blindMode_str, game.blindMode))
			{
				debug(LOG_ERROR, "Invalid blindMode (%s) specified in config .json - ignoring", blindMode_str.toUtf8().c_str());
			}

			// Allow setting gamePassword
			WzString gamePass_str = ini.value("gamePassword", "").toWzString();
			if (!gamePass_str.isEmpty())
			{
				NETsetGamePassword(gamePass_str.toUtf8().c_str());
			}

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

	return true;
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
			for (unsigned j = 0; j < difficultyList.size(); ++j)
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

		if (ini.contains("closed"))
		{
			if (ini.value("closed").toBool())
			{
				NetPlay.players[i].ai = AI_CLOSED;
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
static bool loadMapChallengeAndPlayerSettings(bool forceLoadPlayers = false)
{
	char aFileName[256];
	LEVEL_DATASET* psLevel = levFindDataSet(game.map, &game.hash);

	ASSERT_OR_RETURN(false, psLevel, "No level found for %s", game.map);
	ASSERT_OR_RETURN(false, psLevel->game >= 0 && psLevel->game < LEVEL_MAXFILES, "Invalid psLevel->game: %" PRIi16 " - may be a corrupt level load (%s; hash: %s)", psLevel->game, game.map, game.hash.toString().c_str());
	sstrcpy(aFileName, psLevel->apDataFiles[psLevel->game].c_str());
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

		return !warnIfMissing; // not treated as fatal failure unless warnIfMissing
	}
	WzConfig ini(ininame, WzConfig::ReadOnly);

	bool retVal = loadMapChallengeSettings(ini);

	/* Do not load player settings if we are already hosting an online match */
	if (!bIsOnline || forceLoadPlayers)
	{
		loadMapPlayerSettings(ini);
	}

	return retVal;
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
void multiLobbyRandomizeOptions()
{
	RUN_ONLY_ON_SIDE(InGameSide::HOST_OR_SINGLEPLAYER)

	if (NetPlay.bComms && NetPlay.isHost && !canChangeMapOrRandomize())
	{
		return;
	}

	resetPlayerPositions();

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
	for (int i = 0; i < static_cast<unsigned>(limitIcons.size()) - 1; ++i)	// skip last item, MPFLAGS_FORCELIMITS
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
	updateStructureDisabledFlags();

	// Game options
	if (!locked.scavengers && game.mapHasScavengers)
	{
		game.scavengers = static_cast<uint8_t>(static_cast<uint32_t>(rand()) % (static_cast<uint32_t>(SCAV_TYPE_MAX) + 1));
	}

	if (!locked.alliances)
	{
		game.alliance = static_cast<uint8_t>(static_cast<uint32_t>(rand()) % (static_cast<uint32_t>(ALLIANCE_TYPE_MAX) + 1));
	}
	if (!locked.power)
	{
		game.power = static_cast<uint32_t>(rand()) % (static_cast<uint32_t>(POWER_SETTING_MAX) + 1);
	}
	if (!locked.bases)
	{
		game.base = static_cast<uint8_t>(static_cast<uint32_t>(rand()) % (static_cast<uint32_t>(CAMP_TYPE_MAX) + 1));
	}

	game.techLevel = (static_cast<uint32_t>(rand()) % static_cast<uint32_t>(TECH_LEVEL_MAX)) + TECH_LEVEL_MIN;

	if (NetPlay.isHost)
	{
		resetReadyStatus(true);
	}

	refreshMultiplayerOptionsTitleUIIfActive();

	NETsetLobbyConfigFlagsFields(game.alliance, game.techLevel, game.power, game.base);
	NETregisterServer(WZ_SERVER_UPDATE);
}

void displayLobbyDisabledNotification()
{
	if (!hasNotificationsWithTag(LOBBY_DISABLED_TAG))
	{
		WZ_Notification notification;
		notification.duration = 0;
		notification.contentTitle = _("Multiplayer Lobby Support Unavailable");

		notification.contentText = _("Your client cannot connect to the mutiplayer lobby.");
		notification.contentText += "\n\n";
		notification.contentText += _("Please click the button below for more information on how to fix it.");

		std::string infoLink = NET_getLobbyDisabledInfoLinkURL();
		notification.action = WZ_Notification_Action(_("More Information"), [infoLink](const WZ_Notification&) {
			// Open the infoLink url
			wzAsyncExecOnMainThread([infoLink]{
				if (!openURLInBrowser(infoLink.c_str()))
				{
					debug(LOG_ERROR, "Failed to open url in browser: \"%s\"", infoLink.c_str());
				}
			});
		});
		notification.tag = LOBBY_DISABLED_TAG;

		addNotification(notification, WZ_Notification_Trigger::Immediate());
	}
}

bool WzMultiplayerOptionsTitleUI::startHost()
{
	debug(LOG_NET, "MULTIOP_HOST enabled");
	resetLobbyChangeVoteData();
	resetDataHash();
	resetReadyStatus(false);
	removeWildcards((char*)sPlayer);
	for (size_t i = 0; i < MAX_CONNECTED_PLAYERS; i++)
	{
		ingame.PingTimes[i] = 0;
		ingame.VerifiedIdentity[i] = false;
		ingame.LagCounter[i] = 0;
		ingame.DesyncCounter[i] = 0;
		ingame.lastSentPlayerDataCheck2[i].reset();
		ingame.muteChat[i] = false;
	}
	resetAllMultiOptionPrefValues();
	multiSyncResetAllChallenges();

	if (game.blindMode != BLIND_MODE::NONE)
	{
		generateBlindIdentity();
	}

	const bool bIsAutoHostOrAutoGame = getHostLaunch() == HostLaunch::Skirmish || getHostLaunch() == HostLaunch::Autohost;
	if (!hostCampaign((char*)game.name, (char*)sPlayer, spectatorHost, bIsAutoHostOrAutoGame))
	{
		displayRoomSystemMessage(_("Sorry! Failed to host the game."));
		wz_command_interface_output("WZEVENT: hostingFailed\n");
		if (bIsAutoHostOrAutoGame && headlessGameMode())
		{
			wzQuit(1);
		}
		return false;
	}

	if (NET_getLobbyDisabled())
	{
		displayLobbyDisabledNotification();
	}

	updateInActualHostedLobby(true);

	widgDelete(psWScreen, MULTIOP_REFRESH);
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

	addGameOptions();
	addChatBox();

	addPlayerBox(true);

	if (game.blindMode != BLIND_MODE::NONE)
	{
		printBlindModeHelpMessagesToConsole();
	}

	return true;
}

/*
 * Process click events on the multiplayer/skirmish options screen
 * 'id' is id of the button that was pressed
 */
void WzMultiplayerOptionsTitleUI::processMultiopWidgets(UDWORD id)
{
	PLAYERSTATS playerStats;

	char sPlayer_new[128] = {'\0'};

	// these work all the time.
	switch (id)
	{

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

	case MULTIOP_CHATEDIT:
		// now handled in setOnReturnHandler
		break;

	case CON_CANCEL:

		resetHostLaunch(); // Dont load the autohost file on subsequent hosts
		performedFirstStart = false; // Reset everything
		if (!challengeActive)
		{
			if (NetPlay.bComms && ingame.side == InGameSide::MULTIPLAYER_CLIENT && !NetPlay.isHost)
			{
				// remove a potential "allow" vote if we gracefully leave
				sendLobbyChangeVoteData(0);
			}
			NETGameLocked(false);		// reset status on a cancel
			stopJoining(parent, ERROR_NOERROR);
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
	default:
		break;
	}
}

bool WzMultiplayerOptionsTitleUI::getOption_SpectatorHost()
{
	return spectatorHost;
}

void WzMultiplayerOptionsTitleUI::setOption_SpectatorHost(bool value)
{
	spectatorHost = value;
	if (!spectatorHost)
	{
		// disable blind mode options (which are only available when spectator host)
		game.blindMode = BLIND_MODE::NONE;
	}
}

/* Start a multiplayer or skirmish game */
void startMultiplayerGame()
{
	ASSERT_HOST_ONLY(return);

	wz_command_interface_output("WZEVENT: startMultiplayerGame\n");
	debug(LOG_INFO, "startMultiplayerGame");

	cancelOrDismissNotificationsWithTag(LOBBY_DISABLED_TAG);
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

	if (GetGameMode() == GS_NORMAL)
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
		sendReadyRequest(selectedPlayer, desiredReadyState);
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
	virtual bool changeTeam(uint32_t player, uint8_t team, uint32_t responsibleIdx) override
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
		::changeTeam(player, team, responsibleIdx);
		resetReadyStatus(false, isBlindSimpleLobby(game.blindMode));
		return true;
	}
	virtual bool changePosition(uint32_t player, uint8_t position, uint32_t responsibleIdx) override
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
		if (!::changePosition(player, position, responsibleIdx))
		{
			return false;
		}
		wz_command_interface_output_room_status_json(true);
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
		refreshMultiplayerOptionsTitleUIIfActive(); //refresh to see the proper tech level in the map name
		NETsetLobbyConfigFlagsFields(game.alliance, game.techLevel, game.power, game.base);
		NETregisterServer(WZ_SERVER_UPDATE);
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
		refreshMultiplayerOptionsTitleUIIfActive(); //refresh to see the proper tech level in the map name
		NETsetLobbyConfigFlagsFields(game.alliance, game.techLevel, game.power, game.base);
		NETregisterServer(WZ_SERVER_UPDATE);
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
		refreshMultiplayerOptionsTitleUIIfActive(); //refresh to see the proper tech level in the map name
		return true;
	}
	virtual bool kickPlayer(uint32_t player, const char *reason, bool ban, uint32_t requester) override
	{
		ASSERT_HOST_ONLY(return false);
		ASSERT_OR_RETURN(false, player != NetPlay.hostPlayer, "Unable to kick the host");
		ASSERT_OR_RETURN(false, player < MAX_CONNECTED_PLAYERS, "Invalid player id: %" PRIu32, player);
		if (!NetPlay.players[player].allocated)
		{
			debug(LOG_INFO, "Unable to kick player: %" PRIu32 " - not a connected human player", player);
			return false;
		}
		std::string slotType = (NetPlay.players[player].isSpectator) ? "spectator" : "player";
		sendRoomSystemMessage((std::string("Kicking ")+slotType+": "+std::string(getPlayerName(player, true))).c_str());
		::kickPlayer(player, reason, ERROR_KICKED, ban);
		resetReadyStatus(false, shouldSkipReadyResetOnPlayerJoinLeaveEvent());
		return true;
	}
	virtual bool changeHostChatPermissions(uint32_t player, bool freeChatEnabled) override
	{
		ASSERT_HOST_ONLY(return false);
		ASSERT_OR_RETURN(false, player != NetPlay.hostPlayer, "Unable to mute the host");
		ASSERT_OR_RETURN(false, player < MAX_CONNECTED_PLAYERS, "Invalid player id: %" PRIu32, player);
		if (!NetPlay.players[player].allocated)
		{
			debug(LOG_INFO, "Unable to mute / unmute player: %" PRIu32 " - not a connected human player", player);
			return false;
		}
		if (ingame.hostChatPermissions[player] == freeChatEnabled)
		{
			// no change - nothing to do
			return true;
		}
		ingame.hostChatPermissions[player] = freeChatEnabled;
		sendHostConfig();

		// other clients will automatically display a notice of the change, but display one locally for the host as well
		const char *pPlayerName = getPlayerName(player);
		std::string playerNameStr = (pPlayerName) ? pPlayerName : (std::string("[p") + std::to_string(player) + "]");
		std::string msg;
		if (freeChatEnabled)
		{
			msg = astringf(_("Host: Free chat enabled for: %s"), playerNameStr.c_str());
		}
		else
		{
			msg = astringf(_("Host: Free chat muted for: %s"), playerNameStr.c_str());
		}
		displayRoomSystemMessage(msg.c_str());

		if (wz_command_interface_enabled())
		{
			const auto& identity = getOutputPlayerIdentity(player);
			std::string playerPublicKeyB64 = base64Encode(identity.toBytes(EcKey::Public));
			std::string playerIdentityHash = identity.publicHashString();
			std::string playerVerifiedStatus = (ingame.VerifiedIdentity[player]) ? "V" : "?";
			std::string playerName = getPlayerName(player);
			std::string playerNameB64 = base64Encode(std::vector<unsigned char>(playerName.begin(), playerName.end()));
			wz_command_interface_output("WZEVENT: hostChatPermissions=%s: %" PRIu32 " %" PRIu32 " %s %s %s %s %s\n", (freeChatEnabled) ? "Y" : "N", player, gameTime, playerPublicKeyB64.c_str(), playerIdentityHash.c_str(), playerVerifiedStatus.c_str(), playerNameB64.c_str(), NetPlay.players[player].IPtextAddress);
		}

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
		const char *pPlayerName = getPlayerName(player, true);
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

		const char *pPlayerName = getPlayerName(player, true);
		std::string playerNameStr = (pPlayerName) ? pPlayerName : (std::string("[p") + std::to_string(player) + "]");
		// Ask the spectator if they are okay with a move from spectator -> player?
		SendPlayerSlotTypeRequest(player, false);

		std::string msg = astringf(_("Asking %s to move to Players..."), playerNameStr.c_str());
		sendRoomSystemMessage(msg.c_str());
		return true;
	}
	virtual void quitGame(int exitCode) override
	{
		ASSERT_HOST_ONLY(return);
		auto psStrongMultiOptionsTitleUI = currentMultiOptionsTitleUI.lock();
		if (psStrongMultiOptionsTitleUI)
		{
			stopJoining(psStrongMultiOptionsTitleUI->getParentTitleUI(), ERROR_NOERROR);
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

optional<KickRedirectInfo> parseKickRedirectInfo(const std::string& redirectString, const std::string& currHostAddress)
{
	// Parse the redirect string (should be a valid json object containing an array of connection descriptions and other connection options)
	KickRedirectInfo redirectInfo;
	try {
		auto obj = nlohmann::json::parse(redirectString);
		redirectInfo = obj.get<KickRedirectInfo>();
	}
	catch (const std::exception&) {
		debug(LOG_ERROR, "Invalid redirect string - could not be processed: %s", redirectString.c_str());
		return nullopt;
	}

	// Process & verify the connection descriptions
	for (auto& conn : redirectInfo.connList)
	{
		if (conn.host.empty() || conn.host == "=")
		{
			// replace with the same host's address
			conn.host = currHostAddress;
		}
	}
	redirectInfo.connList.erase(std::remove_if(redirectInfo.connList.begin(), redirectInfo.connList.end(), [&currHostAddress](const JoinConnectionDescription& desc) {
		return desc.host != currHostAddress || desc.port <= 1024;
	}), redirectInfo.connList.end());

	return redirectInfo;
}

void WzMultiplayerOptionsTitleUI::handleKickRedirect(uint8_t kickerPlayerIdx, const std::string& redirectString)
{
	ASSERT_OR_RETURN(, !NetPlay.isHost, "Host shouldn't be processing kick redirect");
	ASSERT_OR_RETURN(, (GetGameMode() != GS_NORMAL), "Game must not be started");

	if (kickerPlayerIdx < NetPlay.players.size())
	{
		ActivityManager::instance().wasKickedByPlayer(NetPlay.players[kickerPlayerIdx], ERROR_REDIRECT, redirectString);
	}

	// Get the info for the host we're currently connected to
	auto optCurrHostAddress = NET_getCurrentHostTextAddress();
	EcKey::Key hostPublicKey = getVerifiedJoinIdentity(NetPlay.hostPlayer).toBytes(EcKey::Public);

	if (!optCurrHostAddress.has_value())
	{
		debug(LOG_ERROR, "Unable to get current host's address?");
		stopJoining(std::make_shared<WzMsgBoxTitleUI>(WzString(_("Disconnected from host:")), WzString(_("Unable to handle host redirect")), parent), ERROR_REDIRECT);
		return;
	}

	// Parse the redirect string (should be a valid json object containing an array of connection descriptions and other connection options)
	auto redirectInfo = parseKickRedirectInfo(redirectString, optCurrHostAddress.value());
	if (!redirectInfo.has_value())
	{
		debug(LOG_ERROR, "Unable to parse kick redirect info");
		stopJoining(std::make_shared<WzMsgBoxTitleUI>(WzString(_("Disconnected from host:")), WzString(_("Unable to process redirect")), parent), ERROR_REDIRECT);
		return;
	}
	if (redirectInfo->connList.empty())
	{
		// No valid connection descriptions!
		debug(LOG_ERROR, "No valid connection descriptions in redirect: %s", redirectString.c_str());
		stopJoining(std::make_shared<WzMsgBoxTitleUI>(WzString(_("Disconnected from host:")), WzString(_("Unable to process redirect")), parent), ERROR_REDIRECT);
		return;
	}

	// Stop current joining
	stopJoining(parent, ERROR_REDIRECT);

	// Attempt a redirect join
	ExpectedHostProperties expectedHostProps;
	expectedHostProps.hostPublicKey = hostPublicKey;
	expectedHostProps.gamePassword = redirectInfo->gamePassword;
	if (!startJoinRedirectAttempt(sPlayer, redirectInfo->connList, redirectInfo->asSpectator, expectedHostProps))
	{
		// Display a message box about being kicked, and unable to rejoin
		debug(LOG_INFO, "startJoinRedirectAttempt failed");
		changeTitleUI(std::make_shared<WzMsgBoxTitleUI>(WzString(_("Disconnected from host:")), WzString(_("Unable to process repeated or invalid redirects")), parent));
	}
}

WzMultiplayerOptionsTitleUI::MultiMessagesResult WzMultiplayerOptionsTitleUI::frontendMultiMessages(bool running)
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
				if (done)
				{
					refreshMultiplayerOptionsTitleUIIfActive(); //refresh to see the proper tech level in the map name
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

				auto r = NETbeginDecode(queue, NET_FILE_CANCELLED);
				NETbin(r, hash.bytes, hash.Bytes);
				NETend(r);

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
				stopJoining(std::make_shared<WzMsgBoxTitleUI>(WzString(_("Disconnected from host:")), WzString(_("Host supplied invalid options")), parent), ERROR_INVALID);
				return MultiMessagesResult::StoppedJoining;
			}
			updateInActualHostedLobby(true);
			ingame.localOptionsReceived = true;

			handleAutoReadyRequest();

			if (std::dynamic_pointer_cast<WzMultiplayerOptionsTitleUI>(wzTitleUICurrent))
			{
				addGameOptions();
				addChatBox();
			}
			break;
		}

		case NET_HOST_CONFIG:
		{
			if (NetPlay.hostPlayer != queue.index)
			{
				HandleBadParam("NET_HOST_CONFIG should be sent by host", 255, queue.index);
				ignoredMessage = true;
				break;
			}
			if (!recvHostConfig(queue))
			{
				// supplied NET_HOST_CONFIG is not valid
				stopJoining(std::make_shared<WzMsgBoxTitleUI>(WzString(_("Disconnected from host:")), WzString(_("Host supplied invalid host config")), parent), ERROR_INVALID);
				break;
			}
			if (std::dynamic_pointer_cast<WzMultiplayerOptionsTitleUI>(wzTitleUICurrent))
			{
				addChatBox(); // refresh chat box options
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
				if (!multiplayLacksEnoughPlayersToAutostart())
				{
					startMultiplayerGame();
				}
				else
				{
					int minAutoStartPlayerCount = getBoundedMinAutostartPlayerCount();
					if (minAutoStartPlayerCount > 0)
					{
						std::string msg = astringf("Game will not start until there are %d players.", minAutoStartPlayerCount);
						sendRoomSystemMessage(msg.c_str());
					}
				}
			}
			break;

		case NET_PING:						// diagnostic ping msg.
			recvPing(queue);
			break;

		case NET_PLAYER_DROPPED:		// remote player got disconnected
			{
				uint32_t player_id = MAX_CONNECTED_PLAYERS;

				resetReadyStatus(false, shouldSkipReadyResetOnPlayerJoinLeaveEvent());

				auto r = NETbeginDecode(queue, NET_PLAYER_DROPPED);
				{
					NETuint32_t(r, player_id);
				}
				NETend(r);

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
					resetLobbyChangePlayerVote(player_id);
				}
				resetMultiOptionPrefValues(player_id);
				ActivityManager::instance().updateMultiplayGameData(game, ingame, NETGameIsLocked());
				if (player_id == NetPlay.hostPlayer || player_id == selectedPlayer)	// if host quits or we quit, abort out
				{
					stopJoining(parent, ERROR_NOERROR);
				}
				updateGameOptions();
				break;
			}
		case NET_PLAYERRESPONDING:			// remote player is now playing.
			{
				uint32_t player_id;

				auto r = NETbeginDecode(queue, NET_PLAYERRESPONDING);
				// the player that has just responded
				NETuint32_t(r, player_id);
				NETend(r);

				if (player_id >= MAX_CONNECTED_PLAYERS)
				{
					debug(LOG_ERROR, "Bad NET_PLAYERRESPONDING received, ID is %d", (int)player_id);
					break;
				}

				if (whosResponsible(player_id) != queue.index && queue.index != NetPlay.hostPlayer)
				{
					HandleBadParam("NET_PLAYERRESPONDING given incorrect params.", player_id, queue.index);
					break;
				}

				ingame.JoiningInProgress[player_id] = false;
				ingame.DataIntegrity[player_id] = false;
				break;
			}
		case NET_FIREUP:					// campaign game started.. can fire the whole shebang up...
			cancelOrDismissVoteNotifications(); // don't need vote notifications anymore
			cancelOrDismissNotificationsWithTag(LOBBY_DISABLED_TAG);
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
				auto r = NETbeginDecode(queue, NET_FIREUP);
				NETuint32_t(r, randomSeed);
				NETend(r);

				saveMultiOptionPrefValues(sPlayer, selectedPlayer); // persist any changes to multioption preferences

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
				return MultiMessagesResult::StoppedJoining;
			}
			ASSERT(false, "NET_FIREUP was received, but !ingame.localOptionsReceived.");
			break;

		case NET_KICK:						// player is forcing someone to leave
			{
				uint32_t player_id = MAX_CONNECTED_PLAYERS;
				char reason[MAX_KICK_REASON];
				LOBBY_ERROR_TYPES KICK_TYPE = ERROR_NOERROR;

				auto r = NETbeginDecode(queue, NET_KICK);
				NETuint32_t(r, player_id);
				NETstring(r, reason, MAX_KICK_REASON);
				NETenum(r, KICK_TYPE);
				NETend(r);

				if (player_id >= MAX_CONNECTED_PLAYERS)
				{
					debug(LOG_ERROR, "NET_KICK message with invalid player_id: (%" PRIu32")", player_id);
					break;
				}

				if (player_id < MAX_PLAYERS)
				{
					resetLobbyChangePlayerVote(player_id);
				}

				if (player_id == NetPlay.hostPlayer)
				{
					char buf[250] = {'\0'};

					ssprintf(buf, "*Player %d (%s : %s) tried to kick %u", (int) queue.index, getPlayerName(queue.index, true), NetPlay.players[queue.index].IPtextAddress, player_id);
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
					if (KICK_TYPE != ERROR_REDIRECT)
					{
						// normal kick
						size_t maxLinePos = nthOccurrenceOfChar(kickReasonStr, '\n', 10);
						if (maxLinePos != std::string::npos)
						{
							kickReasonStr = kickReasonStr.substr(0, maxLinePos);
						}
						stopJoining(std::make_shared<WzMsgBoxTitleUI>(WzString(_("You have been kicked: ")), WzString::fromUtf8(kickReasonStr), parent), KICK_TYPE);
						debug(LOG_INFO, "You have been kicked, because %s ", kickReasonStr.c_str());
						displayKickReasonPopup(kickReasonStr.c_str());
						ActivityManager::instance().wasKickedByPlayer(NetPlay.players[queue.index], KICK_TYPE, reason);
					}
					else
					{
						// kick_redirect
						// (the kickReasonStr is expected to be a formatted string)
						NETpop(queue);
						handleKickRedirect(queue.index, kickReasonStr);
						return MultiMessagesResult::StoppedJoining;
					}
				}
				else
				{
					NETplayerKicked(player_id, KICK_TYPE == ERROR_REDIRECT);
				}
				break;
			}
		case NET_HOST_DROPPED:
		{
			auto r = NETbeginDecode(queue, NET_HOST_DROPPED);
			NETend(r);
			stopJoining(std::make_shared<WzMsgBoxTitleUI>(WzString(_("Connection lost:")), WzString(_("No connection to host.")), parent), ERROR_HOSTDROPPED);
			debug(LOG_NET, "The host has quit!");
			break;
		}
		case NET_TEXTMSG:					// Chat message
			if (ingame.localOptionsReceived)
			{
				NetworkTextMessage message;
				if (message.receive(queue)) {

					bool displayedMessage = false;
					if (message.sender < 0 || (!isPlayerMuted(message.sender) && !playerSpamMutedUntil(message.sender).has_value()))
					{
						displayRoomMessage(buildMessage(message.sender, message.text));
						audio_PlayTrack(FE_AUDIO_MESSAGEEND);
						displayedMessage = true;

						if (message.sender >= 0)
						{
							recordPlayerMessageSent(message.sender);
						}
					}

					bool isLobbySlashCommand = false;
					if (lobby_slashcommands_enabled())
					{
						isLobbySlashCommand = processChatLobbySlashCommands(message, cmdInterface);
						if (isLobbySlashCommand && !displayedMessage)
						{
							// display it anyway, even though user is muted, because it was a processed slash command
							displayRoomMessage(buildMessage(message.sender, message.text));
							audio_PlayTrack(FE_AUDIO_MESSAGEEND);
							displayedMessage = true;
						}
					}

					if (!isLobbySlashCommand)
					{
						cmdInterfaceLogChatMsg(message, "WZCHATLOB");
					}
				}
			}
			break;

		case NET_QUICK_CHAT_MSG:
			if (ingame.localOptionsReceived)
			{
				recvQuickChat(queue);
			}
			break;

		case NET_VOTE:
			if (NetPlay.isHost && ingame.localOptionsReceived)
			{
				if (recvVote(queue, true))
				{
					refreshMultiplayerOptionsTitleUIIfActive();
				}
			}
			break;

		case NET_VOTE_REQUEST:
			if (!NetPlay.isHost && !NetPlay.players[selectedPlayer].isSpectator)
			{
				recvVoteRequest(queue);
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
						stopJoining(std::make_shared<WzMsgBoxTitleUI>(WzString(_("Disconnected from host:")), WzString(_("The host tried to move us to Players, but we never gave permission.")), parent), ERROR_HOSTDROPPED);
						return MultiMessagesResult::StoppedJoining;
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
			ignoredMessage = false;
		}

		NETpop(queue);
	}

	const std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
	autoLagKickRoutine(now);
	autoLobbyNotReadyKickRoutine(now);

	return MultiMessagesResult::Continue;
}

TITLECODE WzMultiplayerOptionsTitleUI::run()
{
	static UDWORD	lastrefresh = 0;
	PLAYERSTATS		playerStats;

	if (frontendMultiMessages(true) == MultiMessagesResult::StoppedJoining)
	{
		// shortcut any further processing - stopped joining, this title UI is about to go away
		return TITLECODE_CONTINUE;
	}
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
			if (aiChooserUp >= 0 && NetPlay.players[aiChooserUp].allocated)
			{
				closeAiChooser(); // close ai chooser that's open for a slot which a player just joined into
			}

			addPlayerBox(true);				// update the player box.
			loadMapPreview(false);
			updateGameOptions();
		}
	}


	// update scores and pings if far enough into the game
	if (ingame.localOptionsReceived && ingame.localJoiningInProgress)
	{
		sendScoreCheck();
		sendPing();
	}

	// if we don't have the focus, then autoclick in the chatbox.
	if (psWScreen->psFocus.expired() && !isMouseOverScreenOverlayChild(mouseX(), mouseY()) && !mouseDown(MOUSE_LMB))
	{
		auto pChatBox = dynamic_cast<ChatBoxWidget *>(widgGetFromID(psWScreen, MULTIOP_CHATBOX));
		if (pChatBox)
		{
			if (wzSeemsLikeNonTouchPlatform()) // only grab focus for chat edit box on non-touch platforms (i.e. platforms that ought to have a physical keyboard)
			{
				pChatBox->takeFocus();
			}
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

						sstrcpy(game.map, mapData->pName.c_str());
						game.hash = levGetFileHash(mapData);
						game.maxPlayers = mapData->players;
						game.isMapMod = CheckForMod(mapData->realFileName);
						game.isRandom = CheckForRandom(mapData->realFileName, mapData->apDataFiles[0].c_str());
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
					updateMapSettings(mapData);

					loadMapPreview(false);
					loadMapChallengeAndPlayerSettings();
					debug(LOG_INFO, "Switching map: %s (builtin: %d)", (!mapData->pName.empty()) ? mapData->pName.c_str() : "n/a", (int)builtInMap);

					//Reset player slots if it's a smaller map.
					if (NetPlay.isHost && NetPlay.bComms && oldMaxPlayers > game.maxPlayers)
					{
						resetPlayerPositions();
						repositionHumanSlots();

						const std::vector<uint8_t>& humans = NET_getHumanPlayers();
						size_t playerInc = 0;

						for (uint8_t slotInc = 0; slotInc < game.maxPlayers && playerInc < humans.size(); ++slotInc)
						{
							changePosition(humans[playerInc], slotInc, realSelectedPlayer);
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

					wz_command_interface_output_room_status_json(true);
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
		cancelOrDismissVoteNotifications();
		cancelOrDismissNotificationsWithTag(LOBBY_DISABLED_TAG);
		cancelOrDismissNotificationIfTag([](const std::string& tag) {
			return (tag.rfind(SLOTTYPE_TAG_PREFIX, 0) == 0);
		});
		stopJoining(std::make_shared<WzMsgBoxTitleUI>(WzString(_("Connection lost:")), WzString(_("The host has quit.")), parent), ERROR_HOSTDROPPED);
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
	bInActualHostedLobby = false; // don't bother calling updateInActualHostedLobby here because we're destroying everything
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
		if (NetPlay.isPortMappingEnabled)
		{
			if (!ipv4MappingRequest)
			{
				return;
			}
			switch (PortMappingManager::instance().get_status(ipv4MappingRequest))
			{
			// rely on callbacks / output in NETaddRedirects() for everything except pending
			case PortMappingDiscoveryStatus::IN_PROGRESS:
				ssprintf(buf, "%s", _("Port mapping creation is in progress..."));
				displayRoomNotifyMessage(buf);
				break;
			default:
				break;
			};
		}
		else
		{
			ssprintf(buf, _("Port mapping disabled by user. Autoconfig of port %d will not happen."), NETgetGameserverPort());
			displayRoomNotifyMessage(buf);
		}
	}
}

void printBlindModeHelpMessagesToConsole()
{
	if (game.blindMode == BLIND_MODE::NONE)
	{
		return;
	}

	WzString blindLobbyMessage;
	if (game.blindMode >= BLIND_MODE::BLIND_GAME)
	{
		blindLobbyMessage = _("BLIND GAME NOTICE:");
		blindLobbyMessage += "\n- ";
		blindLobbyMessage += _("Player names will not be visible to other players (until the game is over)");
	}
	else
	{
		blindLobbyMessage = _("BLIND LOBBY NOTICE:");
		blindLobbyMessage += "\n- ";
		blindLobbyMessage += _("Player names will not be visible to other players (until the game has started)");
	}
	if (NetPlay.hostPlayer < MAX_PLAYERS)
	{
		blindLobbyMessage += "\n- ";
		blindLobbyMessage += _("IMPORTANT: The host is a player, and can see and configure other players (!)");
	}
	else
	{
		blindLobbyMessage += "\n- ";
		blindLobbyMessage += _("The host is a spectator, and can see player identities and configure players / teams");
	}
	displayRoomNotifyMessage(blindLobbyMessage.toUtf8().c_str());
	if (selectedPlayer < MAX_PLAYERS)
	{
		std::string blindLobbyCodenameMessage = "- ";
		blindLobbyCodenameMessage += astringf(_("You have been assigned the codename: \"%s\""), getPlayerGenericName(selectedPlayer));
		displayRoomNotifyMessage(blindLobbyCodenameMessage.c_str());
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

#if defined(__GNUC__) && !defined(__INTEL_COMPILER) && !defined(__clang__) && (12 <= __GNUC__)
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wstringop-overflow" // Ignore on GCC 12+`
#endif

void WzMultiplayerOptionsTitleUI::start()
{
	const bool bReenter = performedFirstStart;
	performedFirstStart = true;
	netPlayersUpdated = true;

	addBackdrop()->setCalcLayout(calcBackdropLayoutForMultiplayerOptionsTitleUI);
	addTopForm(true);

	/* Entering the first time */
	if (!bReenter)
	{
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

		currentMultiOptionsTitleUI = std::dynamic_pointer_cast<WzMultiplayerOptionsTitleUI>(shared_from_this());
		playerDisplayView = PlayerDisplayView::Players;
		bRequestedSelfMoveToPlayers = false;
		bHostRequestedMoveToPlayers.resize(MAX_CONNECTED_PLAYERS, false);
		playerRows.clear();
		initKnownPlayers();
		resetPlayerConfiguration(true);
		memset(&locked, 0, sizeof(locked));
		if (ingame.side == InGameSide::HOST_OR_SINGLEPLAYER)
		{
			spectatorHost = false;
			game.blindMode = BLIND_MODE::NONE;
		}
		defaultOpenSpectatorSlots = war_getMPopenSpectatorSlots();
		if (!loadMapChallengeAndPlayerSettings(true))
		{
			if (challengeActive)
			{
				changeTitleUI(std::make_shared<WzMsgBoxTitleUI>(WzString(_("Failed to load challenge:")), WzString(_("Failed to load the challenge's map or config")), parent));
				return;
			}
			if (getHostLaunch() == HostLaunch::Autohost)
			{
				changeTitleUI(std::make_shared<WzMsgBoxTitleUI>(WzString(_("Failed to process autohost config:")), WzString::fromUtf8(astringf(_("Failed to load the autohost map or config from: %s"), wz_skirmish_test().c_str())), parent));
				resetHostLaunch(); // Don't load the autohost file on subsequent hosts
				return;
			}
			// otherwise, treat as non-fatal...
		}
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

		ingame.localOptionsReceived = false;

		PLAYERSTATS	playerStats;

		loadMultiStats((char*)sPlayer, &playerStats);
		setMultiStats(selectedPlayer, playerStats, true); // just set this locally so that the identity is cached, if changing name

		/* Entering the first time with challenge, immediately start the host */
		if (challengeActive)
		{
			if (!startHost())
			{
				debug(LOG_ERROR, "Failed to host the challenge.");
				return;
			}
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
	if (!bReenter && game.blindMode != BLIND_MODE::NONE && ingame.side == InGameSide::MULTIPLAYER_CLIENT)
	{
		printBlindModeHelpMessagesToConsole();
	}

	/* Reset structure limits if we are entering the first time or if we have a challenge */
	if (!bReenter || challengeActive)
	{
		resetLimits();
		ingame.flags &= ~MPFLAGS_FORCELIMITS;
		createLimitSet(); // should effectively just free it
		updateStructureDisabledFlags();
		updateGameOptions();
	}

	if (autogame_enabled() || getHostLaunch() == HostLaunch::Autohost)
	{
		if (!ingame.localJoiningInProgress)
		{
			startHost();
		}
		if (!getHostLaunchStartNotReady())
		{
			sendReadyRequest(selectedPlayer, true);
		}
		if (getHostLaunch() == HostLaunch::Skirmish)
		{
			startMultiplayerGame();
			// reset flag in case people dropped/quit on join screen
			NETsetPlayerConnectionStatus(CONNECTIONSTATUS_NORMAL, NET_ALL_PLAYERS);
		}
	}
}

#if defined(__GNUC__) && !defined(__INTEL_COMPILER) && !defined(__clang__) && (12 <= __GNUC__)
# pragma GCC diagnostic pop
#endif

std::shared_ptr<WzTitleUI> WzMultiplayerOptionsTitleUI::getParentTitleUI()
{
	return parent;
}

/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
// Drawing functions

void displayChatEdit(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset)
{
	auto psSelf = static_cast<W_EDITBOX*>(psWidget);

	int x = xOffset + psWidget->x();
	int y = yOffset + psWidget->y();

	PIELIGHT borderColor = WZCOL_MENU_SEPARATOR;
	if (!psSelf->isEditing())
	{
		borderColor.byte.a = borderColor.byte.a / 2;
	}
	iV_Box(x, y, x + psWidget->width(), y + psWidget->height(), borderColor);
}

// ////////////////////////////////////////////////////////////////////////////

void displaySpectatorAddButton(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset)
{
	int x = xOffset + psWidget->x();
	int y = yOffset + psWidget->y();

	drawBlueBox_SpectatorOnly(x, y, psWidget->width(), psWidget->height());
	iV_DrawImage(FrontImages, IMAGE_SPECTATOR, x + 2, y + 1);
}

void displayAi(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset)
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

int difficultyIcon(int difficulty)
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

	ASSERT_OR_RETURN(, j < difficultyList.size(), "Bad difficulty found: %d", j);
	drawBlueBox(x, y, psWidget->width(), psWidget->height());
	iV_DrawImage(FrontImages, difficultyIcon(j), x + 5, y + 5);
	cache.wzDifficultyText.setText(gettext(difficultyList[j]), font_regular);
	cache.wzDifficultyText.render(x + 42, y + 22, WZCOL_FORM_TEXT);
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
		pie_UniTransBoxFill(x, y, x + psWidget->width() + psWidget->height() - 1, y + psWidget->height(), colour);
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

static W_EDITBOX* addMultiEditBox(UDWORD formid, UDWORD id, UDWORD x, UDWORD y, char const *tip, char const *tipres, UDWORD icon, UDWORD iconhi, UDWORD iconid, bool disabled)
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

	auto multiBut = addMultiBut(psWScreen, MULTIOP_OPTIONS, iconid, x + MULTIOP_EDITBOXW + 2, y + 2, MULTIOP_EDITBOXH, MULTIOP_EDITBOXH, tip, icon, iconhi, iconhi);

	if (disabled)
	{
		editBox->setState(WEDBS_DISABLE);
		multiBut->setState(WBUT_DISABLE);
	}

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

static bool multiplayHasOpenPlayerSlot()
{
	for (int j = 0; j < MAX_PLAYERS; j++)
	{
		if (NetPlay.players[j].ai == AI_OPEN && !NetPlay.players[j].allocated && NetPlay.players[j].position < game.maxPlayers)
		{
			return true;
		}
	}
	return false;
}

bool multiplayPlayersCanCheckReady()
{
	if (isBlindSimpleLobby(game.blindMode))
	{
		// if in a blind simple lobby, players can always check ready
		return true;
	}

	if (locked.readybeforefull && multiplayHasOpenPlayerSlot())
	{
		return false;
	}

	return (allPlayersOnSameTeam(-1) == -1);
}

bool multiplayPlayersShouldCheckReady()
{
	if (!multiplayPlayersCanCheckReady())
	{
		return false;
	}
	return isBlindSimpleLobby(game.blindMode) || !multiplayHasOpenPlayerSlot();
}

static bool multiplayLacksEnoughPlayersToAutostart()
{
	if (!NetPlay.isHost)
	{
		// host does not share min autostart player count with clients,
		// so just assume it's disabled (and host will start/autostart when appropriate)
		return false;
	}
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
	return !allowStart;
}

/* Returns true if the multiplayer game can start (i.e. all players are ready) */
bool multiplayPlayersReady()
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

bool multiplayIsStartingGame()
{
	return bInActualHostedLobby && multiplayPlayersReady() && !multiplayLacksEnoughPlayersToAutostart();
}

void sendRoomSystemMessage(char const *text)
{
	ASSERT_HOST_ONLY(return);
	NetworkTextMessage message(SYSTEM_MESSAGE, text);
	displayRoomSystemMessage(text);
	message.enqueue(NETbroadcastQueue());
}

void sendRoomNotifyMessage(char const *text)
{
	ASSERT_HOST_ONLY(return);
	NetworkTextMessage message(NOTIFY_MESSAGE, text);
	displayRoomSystemMessage(text);
	message.enqueue(NETbroadcastQueue());
}

void sendRoomSystemMessageToSingleReceiver(char const *text, uint32_t receiver, bool skipLocalDisplay)
{
	ASSERT_OR_RETURN(, isHumanPlayer(receiver), "Invalid receiver: %" PRIu32 "", receiver);
	NetworkTextMessage message(SYSTEM_MESSAGE, text);
	if (!skipLocalDisplay || receiver == selectedPlayer)
	{
		displayRoomSystemMessage(text);
	}
	message.enqueue(NETnetQueue(receiver));
}

static void sendRoomChatMessage(char const *text, bool skipLocalDisplay)
{
	if (NetPlay.bComms)
	{
		auto mutedUntil = playerSpamMutedUntil(selectedPlayer);
		if (mutedUntil.has_value())
		{
			auto currentTime = std::chrono::steady_clock::now();
			auto duration_until_send_allowed = std::chrono::duration_cast<std::chrono::seconds>(mutedUntil.value() - currentTime).count();
			auto duration_timeout_message = astringf(_("You have sent too many messages in the last few seconds. Please wait and try again."), static_cast<unsigned>(duration_until_send_allowed));
			addConsoleMessage(duration_timeout_message.c_str(), DEFAULT_JUSTIFY, INFO_MESSAGE, false, static_cast<UDWORD>(std::chrono::duration_cast<std::chrono::milliseconds>(mutedUntil.value() - currentTime).count()));
			return;
		}
		recordPlayerMessageSent(selectedPlayer);
	}

	NetworkTextMessage message(selectedPlayer, text);
	if (!skipLocalDisplay)
	{
		displayRoomMessage(RoomMessage::player(selectedPlayer, text));
	}
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
bool isSpectatorOnlySlot(UDWORD playerIdx)
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
	j["gameTimeLimitMinutes"] = p.gameTimeLimitMinutes;
	j["playerLeaveMode"] = p.playerLeaveMode;
	j["blindMode"] = p.blindMode;
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
	if (j.contains("gameTimeLimitMinutes"))
	{
		p.gameTimeLimitMinutes = j.at("gameTimeLimitMinutes").get<uint32_t>();
	}
	else
	{
		// default to the old (pre-4.4.0) default value of 0 minutes (disabled)
		p.gameTimeLimitMinutes = 0;
	}
	if (j.contains("playerLeaveMode"))
	{
		p.playerLeaveMode = j.at("playerLeaveMode").get<PLAYER_LEAVE_MODE>();
	}
	else
	{
		// default to the old (pre-4.4.0) behavior of destroy resources
		p.playerLeaveMode = PLAYER_LEAVE_MODE::DESTROY_RESOURCES;
	}
	if (j.contains("blindMode"))
	{
		p.blindMode = j.at("blindMode").get<BLIND_MODE>();
	}
	else
	{
		// default to the old (pre-4.6.0) behavior (no blind mode)
		p.blindMode = BLIND_MODE::NONE;
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
	j["kick"] = false; // Store fixed false value for now (PLAYER.kick removed in WZ 4.6.2)
	j["team"] = p.team;
	j["ready"] = p.ready;
	j["ai"] = p.ai;
	j["difficulty"] = static_cast<int8_t>(p.difficulty);
	//j["autoGame"] = p.autoGame; // MAYBE NOT?
	// Do not persist wzFiles
	// Do not persist IPtextAddress
	j["faction"] = static_cast<uint8_t>(p.faction);
	j["isSpectator"] = p.isSpectator;
	j["isAdmin"] = p.isAdmin;
}

inline void from_json(const nlohmann::json& j, PLAYER& p) {
	std::string str = j.at("name").get<std::string>();
	sstrcpy(p.name, str.c_str());
	p.position = j.at("position").get<int32_t>();
	p.colour = j.at("colour").get<int32_t>();
	p.allocated = j.at("allocated").get<bool>();
	p.heartattacktime = j.at("heartattacktime").get<uint32_t>();
	p.heartbeat = j.at("heartbeat").get<bool>();
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
	if (j.contains("isAdmin"))
	{
		p.isAdmin = j.at("isAdmin").get<bool>();
	}
	else
	{
		// default to the old (pre-4.5.4) default value of false
		p.isAdmin = false;
	}
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

	// Save campaignName
	object["campaignName"] = getCampaignName();

	// Save tweakOptions
	object["tweakOptions"] = getCamTweakOptions();

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

bool WZGameReplayOptionsHandler::optionsUpdatePlayerInfo(nlohmann::json& object) const
{
	// update player names
	auto& netPlayers = object.at("netplay.players");
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
		PLAYER p = netPlayers.at(i).get<PLAYER>();
		sstrcpy(p.name, NetPlay.players[i].name);
		netPlayers[i] = p;
	}

	// update identities stored in multistats
	if (game.blindMode != BLIND_MODE::NONE)
	{
		auto& multistats = object.at("multistats");
		updateMultiStatsIdentitiesInJSON(multistats, true);
	}

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

	// restore `campaignName`
	if (object.contains("campaignName"))
	{
		auto restoredCampaignName = object.at("campaignName").get<std::string>();
		if (!restoredCampaignName.empty())
		{
			setCampaignName(restoredCampaignName);
		}
		else
		{
			clearCampaignName();
		}
	}
	else
	{
		clearCampaignName();
	}

	// restore `tweakOptions`
	if (object.contains("tweakOptions"))
	{
		auto tweakOptionsJSON = object.at("tweakOptions");
		std::unordered_map<std::string, nlohmann::json> tweakOptionsJSONMap;
		for (auto it : tweakOptionsJSON.items())
		{
			tweakOptionsJSONMap[it.key()] = it.value();
		}
		setCamTweakOptions(tweakOptionsJSONMap);
	}
	else
	{
		clearCamTweakOptions();
	}

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
	// Must restore `useTerrainOverrides` (this matters for re-loading the map!) - see loadMapPreview() in multiint.cpp
	builtInMap = (mapData->realFileName == nullptr);
	useTerrainOverrides = builtInMap && shouldLoadTerrainTypeOverrides(mapData->pName);

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
	game.isRandom = mapData && CheckForRandom(mapData->realFileName, mapData->apDataFiles[0].c_str());

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
