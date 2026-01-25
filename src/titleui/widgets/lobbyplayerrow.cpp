/*
	This file is part of Warzone 2100.
	Copyright (C) 2024  Warzone 2100 Project

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
/** \file
 *  Lobby Player Row Widget (and associated display functions)
 */

#include "lobbyplayerrow.h"
#include "src/frontend.h"
#include "src/frend.h"
#include "src/intimage.h"
#include "src/multiint.h"
#include "src/loadsave.h"
#include "src/multiplay.h"
#include "src/ai.h"
#include "src/component.h"
#include "src/clparse.h"
#include "src/challenge.h"
//#include "lib/widget/widget.h"
//#include "lib/widget/label.h"
//#include "lib/widget/button.h"
//#include "infobutton.h"

#include "lib/framework/input.h"

#include "lib/netplay/netplay.h"

#include "src/titleui/multiplayer.h"
#include "src/multistat.h"

#include "lib/ivis_opengl/pieblitfunc.h"

struct DisplayPlayerCache {
	std::string	fullMainText;	// the “full” main text (used for storing the full player name when displaying a player)
	WzText		wzMainText;		// the main text

	std::string fullAltNameText;
	WzText		wzAltNameText;

	WzText		wzSubText;		// the sub text (used for players)
};

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

class ReadyBoxContainerWidget : public W_FORM
{
protected:
	void display(int xOffset, int yOffset) override;
public:
	void setShowAnimatedHighlight(bool val) { showAnimatedHighlight = val; }
private:
	bool showAnimatedHighlight = false;
};

void ReadyBoxContainerWidget::display(int xOffset, int yOffset)
{
	const int x0 = xOffset + x();
	const int y0 = yOffset + y();
	const auto w = width();
	const auto h = height();
	const auto playerIdx = UserData;

	drawBoxForPlayerInfoSegment(playerIdx, x0, y0, w, h);

	if (showAnimatedHighlight)
	{
		const int scale = 2500;
		int f = realTime % scale;
		PIELIGHT mix = pal_RGBA(3, 15, 252, 255);
		mix.byte.a = 155 + iSinR(65536 * f / scale, 75);

		pie_UniTransBoxFill(x0 + 2, y0 + 1, x0 + w - 1, y0 + h - 1, mix);
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

static void displayColour(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset)
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

void displayTeamChooser(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset)
{
	int x = xOffset + psWidget->x();
	int y = yOffset + psWidget->y();
	UDWORD		i = psWidget->UserData;

	ASSERT_OR_RETURN(, i < MAX_CONNECTED_PLAYERS, "Index (%" PRIu32 ") out of bounds", i);

	drawBoxForPlayerInfoSegment(i, x, y, psWidget->width(), psWidget->height());

	if (!NetPlay.players[i].isSpectator)
	{
		if (!isBlindSimpleLobby(game.blindMode) || NetPlay.isHost)
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

static bool isKnownPlayer(std::map<std::string, EcKey::Key> const &knownPlayers, std::string const &name, EcKey const &key)
{
	if (key.empty())
	{
		return false;
	}
	std::map<std::string, EcKey::Key>::const_iterator i = knownPlayers.find(name);
	return i != knownPlayers.end() && key.toBytes(EcKey::Public) == i->second;
}

static void displayAltNameBox(int x, int y, WIDGET *psWidget, DisplayPlayerCache& cache, bool isHighlight)
{
	int altNameBoxWidth = cache.wzAltNameText.width() + 4;
	int altNameBoxHeight = cache.wzAltNameText.lineSize() + 2;
	int altNameBoxX0 = (x + psWidget->width()) - altNameBoxWidth;
	PIELIGHT altNameBoxColor = WZCOL_MENU_BORDER;
	altNameBoxColor.byte.a = static_cast<uint8_t>(static_cast<float>(altNameBoxColor.byte.a) * (isHighlight ? 0.3f : 0.75f));
	pie_UniTransBoxFill(altNameBoxX0, y, altNameBoxX0 + altNameBoxWidth, y + altNameBoxHeight, altNameBoxColor);

	int altNameTextY0 = y + (altNameBoxHeight - cache.wzAltNameText.lineSize()) / 2 - cache.wzAltNameText.aboveBase();
	PIELIGHT altNameTextColor = WZCOL_TEXT_MEDIUM;
	if (isHighlight)
	{
		altNameTextColor.byte.a = static_cast<uint8_t>(static_cast<float>(altNameTextColor.byte.a) * 0.3f);
	}
	cache.wzAltNameText.render(altNameBoxX0 + 2, altNameTextY0, altNameTextColor);
}

void displayPlayer(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset)
{
	// Any widget using displayPlayer must have its pUserData initialized to a (DisplayPlayerCache*)
	assert(psWidget->pUserData != nullptr);
	DisplayPlayerCache& cache = *static_cast<DisplayPlayerCache *>(psWidget->pUserData);

	const auto& aidata = getAIData();

	int const x = xOffset + psWidget->x();
	int const y = yOffset + psWidget->y();
	unsigned const j = psWidget->UserData;
	bool isHighlight = (psWidget->getState() & WBUT_HIGHLIGHT) != 0;

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
		const PLAYERSTATS& stat = getMultiStats(j);

		std::string name = getPlayerName(j);
		if (game.blindMode != BLIND_MODE::NONE)
		{
			// Special override: Show our own name if in spectators
			if (j == selectedPlayer && j > MAX_PLAYER_SLOTS)
			{
				name = NetPlay.players[j].name;
			}
		}
		const char* pAltName = nullptr;
		if (NetPlay.players[j].isAdmin)
		{
			pAltName = _("Admin");
		}

		std::map<std::string, EcKey::Key> serverPlayers;  // TODO Fill this with players known to the server (needs implementing on the server, too). Currently useless.

		PIELIGHT colour;
		iV_fonts nameFont = font_regular;
		if (j == selectedPlayer)
		{
			colour = WZCOL_TEXT_BRIGHT;
//			nameFont = font_regular_bold;
		}
		else if (ingame.PingTimes[j] >= PING_LIMIT)
		{
			colour = WZCOL_FORM_PLAYER_NOPING;
		}
		else if (isKnownPlayer(serverPlayers, name, getMultiStats(j).identity) || game.blindMode != BLIND_MODE::NONE)
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
			cache.wzMainText.setText(WzString::fromUtf8(name), nameFont);
			cache.fullMainText = name;
		}

		WzString subText;

		if (j == NetPlay.hostPlayer && NetPlay.bComms)
		{
			subText += _("HOST");
		}
		else if ((j < MAX_PLAYERS) && (game.blindMode != BLIND_MODE::NONE) && NetPlay.isHost && (NetPlay.hostPlayer >= MAX_PLAYER_SLOTS))
		{
			subText += "\"";
			subText += getPlayerGenericName(j);
			subText += "\"";
		}

		if (NetPlay.bComms && j != selectedPlayer)
		{
			char buf[250] = {'\0'};

			// show ping time
			ssprintf(buf, "%s%s: ", subText.isEmpty() ? "" : ", ", _("Ping"));
			subText += buf;
			if (ingame.PingTimes[j] < PING_LIMIT)
			{
				if (game.blindMode == BLIND_MODE::NONE
					|| (NetPlay.isHost && (NetPlay.hostPlayer >= MAX_PLAYER_SLOTS))
					|| (j == NetPlay.hostPlayer))
				{
					// show actual ping time
					ssprintf(buf, "%03d", ingame.PingTimes[j]);
				}
				else
				{
					// show non-exact ping in blind mode
					const char* pingQualifierStr = "Ok";
					if (ingame.PingTimes[j] > 350)
					{
						pingQualifierStr = "++";
					}
					else if (ingame.PingTimes[j] > 1000)
					{
						pingQualifierStr = "+++";
					}
					ssprintf(buf, "%s", pingQualifierStr);
				}
			}
			else
			{
				ssprintf(buf, "%s", "∞");  // Player has ping of somewhat questionable quality.
			}
			subText += buf;
		}

		bool dummy = stat.played < 5;
		uint8_t star[3] = {0, 0, 0};
		uint8_t medal = 0;

		// star 1 total droid kills
		star[0] = stat.totalKills > 600? 1 : stat.totalKills > 300? 2 : stat.totalKills > 150? 3 : 0;

		// star 2 games played
		star[1] = stat.played > 200? 1 : stat.played > 100? 2 : stat.played > 50? 3 : 0;

		// star 3 games won.
		star[2] = stat.wins > 80? 1 : stat.wins > 40? 2 : stat.wins > 10? 3 : 0;

		// medals.
		medal = stat.wins >= 24 && stat.wins > 8 * stat.losses? 1 : stat.wins >= 12 && stat.wins > 4 * stat.losses? 2 : stat.wins >= 6 && stat.wins > 2 * stat.losses? 3 : 0;

		if (pAltName != nullptr && cache.fullAltNameText.compare(pAltName) != 0)
		{
			std::string altName = pAltName;
			int maxAltNameWidth = static_cast<int>(static_cast<float>(psWidget->width() - nameX) * 0.65f);
			iV_fonts fontID = font_small;
			cache.wzAltNameText.setText(WzString::fromUtf8(altName), fontID);
			cache.fullAltNameText = altName;
			if (cache.wzAltNameText.width() > maxAltNameWidth)
			{
				while (!altName.empty() && ((int)iV_GetTextWidth(altName.c_str(), cache.wzAltNameText.getFontID()) + iV_GetEllipsisWidth(cache.wzAltNameText.getFontID())) > maxAltNameWidth)
				{
					altName.resize(altName.size() - 1);  // Clip alt name.
				}
				altName += "\u2026";
				cache.wzAltNameText.setText(WzString::fromUtf8(altName), fontID);
			}
		}

		if (pAltName && isHighlight)
		{
			// display first, behind everything
			displayAltNameBox(x, y, psWidget, cache, isHighlight);
		}

		int H = 5;
		cache.wzMainText.render(x + nameX, y + 22 - H*!subText.isEmpty(), colour);
		if (!subText.isEmpty())
		{
			cache.wzSubText.setText(subText, font_small);
			cache.wzSubText.render(x + nameX, y + 28, WZCOL_TEXT_MEDIUM);
		}

		if ((game.blindMode != BLIND_MODE::NONE) && (!NetPlay.isHost || NetPlay.hostPlayer < MAX_PLAYER_SLOTS))
		{
			iV_DrawImage(FrontImages, IMAGE_WEE_GUY, x + 4, y + 13);
		}
		else if (dummy)
		{
			iV_DrawImage(FrontImages, IMAGE_MEDAL_DUMMY, x + 4, y + 13);
		}
		else
		{
			constexpr int starImgs[4] = {0, IMAGE_MULTIRANK1, IMAGE_MULTIRANK2, IMAGE_MULTIRANK3};
			if (1 <= star[0] && star[0] < ARRAY_SIZE(starImgs))
			{
				iV_DrawImage(FrontImages, starImgs[star[0]], x + 4, y + 3);
			}
			if (1 <= star[1] && star[1] < ARRAY_SIZE(starImgs))
			{
				iV_DrawImage(FrontImages, starImgs[star[1]], x + 4, y + 13);
			}
			if (1 <= star[2] && star[2] < ARRAY_SIZE(starImgs))
			{
				iV_DrawImage(FrontImages, starImgs[star[2]], x + 4, y + 23);
			}
			constexpr int medalImgs[4] = {0, IMAGE_MEDAL_GOLD, IMAGE_MEDAL_SILVER, IMAGE_MEDAL_BRONZE};
			if (1 <= medal && medal < ARRAY_SIZE(medalImgs))
			{
				iV_DrawImage(FrontImages, medalImgs[medal], x + 16, y + 11);
			}
		}

		if (pAltName && !isHighlight)
		{
			// display last, over top of everything
			displayAltNameBox(x, y, psWidget, cache, isHighlight);
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

static bool canChooseTeamFor(int i)
{
	if (isBlindSimpleLobby(game.blindMode) && !NetPlay.isHost)
	{
		return false;
	}
	return (i == selectedPlayer || isHostOrAdmin());
}

// ////////////////////////////////////////////////////////////////////////////
// Lobby player row

WzPlayerRow::WzPlayerRow(uint32_t playerIdx, const std::shared_ptr<WzMultiplayerOptionsTitleUI>& parent)
: WIDGET()
, parentTitleUI(parent)
, playerIdx(playerIdx)
{ }

std::shared_ptr<WzPlayerRow> WzPlayerRow::make(uint32_t playerIdx, const std::shared_ptr<WzMultiplayerOptionsTitleUI>& parent)
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
		const auto& locked = getLockedOptions();
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
		if (playerIdx == selectedPlayer || isHostOrAdmin())
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
		const auto& locked = getLockedOptions();
		if (playerIdx == selectedPlayer || isHostOrAdmin())
		{
			uint32_t player = playerIdx;
			// host/admin can move any player, clients can request to move themselves if there are available slots
			if (((player == selectedPlayer && validPlayerIdxTargetsForPlayerPositionMove(player).size() > 0) ||
					(NetPlay.players[player].allocated && isHostOrAdmin()))
				&& !locked.position
				&& player < MAX_PLAYERS
				&& !isSpectatorOnlySlot(player)
				&& (!isBlindSimpleLobby(game.blindMode) || NetPlay.isHost))
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
					bool previousPlayersShouldCheckReadyValue = multiplayPlayersShouldCheckReady();
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
					if (ingame.localJoiningInProgress)  // Only if game hasn't actually started yet.
					{
						handlePossiblePlayersShouldCheckReadyChange(previousPlayersShouldCheckReadyValue);
					}
					widgScheduleTask([strongTitleUI] {
						strongTitleUI->updatePlayers();
						resetReadyStatus(false, isBlindSimpleLobby(game.blindMode));
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

void WzPlayerRow::geometryChanged()
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

void WzPlayerRow::updateState()
{
	const auto& aidata = getAIData();
	const auto& locked = getLockedOptions();

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
	if ((selectedPlayer == playerIdx || NetPlay.isHost)
		&& NetPlay.players[playerIdx].allocated
		&& !locked.position
		&& !isSpectatorOnlySlot(playerIdx)
		&& (!isBlindSimpleLobby(game.blindMode) || NetPlay.isHost))
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
			if (NetPlay.isHost || (!trueMultiplayerMode) || (game.blindMode == BLIND_MODE::NONE) || challengeActive)
			{
				// show AI description. Useful for challenges.
				if (NetPlay.players[playerIdx].ai < aidata.size())
				{
					playerInfoTooltip = aidata[NetPlay.players[playerIdx].ai].tip;
				}
			}
		}
	}
	if (NetPlay.players[playerIdx].allocated && (game.blindMode == BLIND_MODE::NONE || (NetPlay.isHost && NetPlay.hostPlayer >= MAX_PLAYER_SLOTS)))
	{
		const auto& identity = getOutputPlayerIdentity(playerIdx);
		if (!identity.empty())
		{
			if (!playerInfoTooltip.empty())
			{
				playerInfoTooltip += "\n";
			}
			std::string hash = identity.publicHashString(20);
			playerInfoTooltip += _("Player ID: ");
			playerInfoTooltip += hash.empty()? _("(none)") : hash;
		}
	}
	playerInfo->setTip(playerInfoTooltip);

	// update ready button
	updateReadyButton();
}

static std::shared_ptr<W_FORM> createReadyButtonContainerForm(int x, int y, int w, int h)
{
	auto form = std::make_shared<ReadyBoxContainerWidget>();
	form->setGeometry(x, y, w, h);
	form->style = WFORM_PLAIN;
	return form;
}

void WzPlayerRow::updateReadyButton()
{
	const auto& aidata = getAIData();
	const auto& locked = getLockedOptions();

	if (!readyButtonContainer)
	{
		// add form to hold 'ready' botton
		readyButtonContainer = createReadyButtonContainerForm(MULTIOP_PLAYERWIDTH - MULTIOP_READY_WIDTH, 0,
					MULTIOP_READY_WIDTH, MULTIOP_READY_HEIGHT);
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
		if (auto readyButtonContainerForm = std::dynamic_pointer_cast<ReadyBoxContainerWidget>(readyButtonContainer))
		{
			readyButtonContainerForm->setShowAnimatedHighlight(false);
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
			sstrcpy(tooltip, gettext(getDifficultyListStr(playerDifficulty)));
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
					(NetPlay.isHost && !locked.difficulty) ? _("Click to change difficulty") : tooltip, icon, icon, icon, MAX_PLAYERS, (NetPlay.isHost && !locked.difficulty) ? 255 : 125);
		auto player = playerIdx;
		auto weakTitleUi = parentTitleUI;
		if (freshDifficultyButton)
		{
			difficultyChooserButton->addOnClickHandler([player, weakTitleUi](W_BUTTON&){
				auto strongTitleUI = weakTitleUi.lock();
				ASSERT_OR_RETURN(, strongTitleUI != nullptr, "Title UI is gone?");
				const auto& locked = getLockedOptions();
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

	if (!multiplayPlayersCanCheckReady())
	{
		// remove ready / difficulty button
		deleteExistingReadyButton();
		deleteExistingDifficultyButton();
		return;
	}

	deleteExistingDifficultyButton();

	const bool isMe = playerIdx == selectedPlayer;
	const int isReady = NETgetDownloadProgress(playerIdx) != 100 ? 2 : NetPlay.players[playerIdx].ready ? 1 : 0;
	char const *const toolTips[2][3] = {{_("Waiting for player"), _("Player is ready"), _("Player is downloading")}, {_("Click when ready"), _("Waiting for other players"), _("Waiting for download")}};
	unsigned images[2][3] = {{IMAGE_CHECK_OFF, IMAGE_CHECK_ON, IMAGE_CHECK_DOWNLOAD}, {IMAGE_CHECK_OFF_HI, IMAGE_CHECK_ON_HI, IMAGE_CHECK_DOWNLOAD_HI}};

	// draw 'ready' button
	bool greyedOutReady = (NetPlay.players[playerIdx].isSpectator && NetPlay.players[playerIdx].ready) || (playerIdx != selectedPlayer);
	bool freshReadyButton = (readyButton == nullptr);
	readyButton = addMultiBut(*readyButtonContainer, MULTIOP_READY_START + playerIdx, 3, 10, MULTIOP_READY_WIDTH, MULTIOP_READY_HEIGHT,
				toolTips[isMe][isReady], images[0][isReady], images[0][isReady], images[isMe][isReady], MAX_PLAYERS, (!greyedOutReady) ? 255 : 125);
	if (auto readyButtonContainerForm = std::dynamic_pointer_cast<ReadyBoxContainerWidget>(readyButtonContainer))
	{
		readyButtonContainerForm->setShowAnimatedHighlight((isMe && !isReady && !greyedOutReady && multiplayPlayersShouldCheckReady()));
	}
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

					sendReadyRequest(selectedPlayer, !NetPlay.players[player].ready);

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
						std::string msg = astringf(_("The host has kicked %s from the game!"), getPlayerName(player, true));
						sendRoomSystemMessage(msg.c_str());
						kickPlayer(player, _("The host has kicked you from the game."), ERROR_KICKED, false);
						resetReadyStatus(true, shouldSkipReadyResetOnPlayerJoinLeaveEvent());		//reset and send notification to all clients
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
	readyTextLabel->setFont(font_small, (isMe) ? WZCOL_TEXT_BRIGHT : WZCOL_TEXT_MEDIUM);
	readyTextLabel->setString(_("READY?"));
}
