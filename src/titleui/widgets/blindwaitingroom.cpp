/*
	This file is part of Warzone 2100.
	Copyright (C) 2024-2025  Warzone 2100 Project

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
 *  Blind lobby waiting room widgets
 */

#include "blindwaitingroom.h"
#include "lib/widget/widget.h"
#include "lib/widget/label.h"
#include "lib/widget/button.h"
#include "infobutton.h"
#include "lobbyplayerrow.h"

#include "lib/netplay/netplay.h"

#include "src/titleui/multiplayer.h"
#include "src/multistat.h"
#include "src/multiint.h"
#include "src/multiplay.h"

#include "lib/ivis_opengl/pieblitfunc.h"

#include "src/screens/joiningscreen.h"

#include <chrono>

// ////////////////////////////////////////////////////////////////////////////
// Blind Waiting Room

class WzRoomTitleBanner : public WIDGET
{
public:
	typedef std::function<void ()> InfoClickHandler;

protected:
	WzRoomTitleBanner()
	: WIDGET()
	{}

	void initialize(const std::string& title, const InfoClickHandler& infoClickHandler);

public:
	static std::shared_ptr<WzRoomTitleBanner> make(const std::string& title, const InfoClickHandler& infoClickHandler)
	{
		class make_shared_enabler: public WzRoomTitleBanner {};
		auto widget = std::make_shared<make_shared_enabler>();

		widget->initialize(title, infoClickHandler);
		return widget;
	}

	void display(int xOffset, int yOffset) override;
	int32_t idealHeight() override;
	void geometryChanged() override;

private:
	InfoClickHandler infoClickHandler;
	int topPadding = 10;
	int bottomPadding = 10;
	int internalHorizontalPadding = 10;
	std::shared_ptr<W_LABEL> titleLabel;
	std::shared_ptr<WzInfoButton> infoButton;
};

int32_t WzRoomTitleBanner::idealHeight()
{
	// the height for one row of text
	int32_t titleHeight = std::max<int32_t>(titleLabel->idealHeight(), (infoButton) ? infoButton->idealHeight() : 0);
	return topPadding + bottomPadding + titleHeight;
}

void WzRoomTitleBanner::initialize(const std::string& title, const InfoClickHandler& _onInfoButtonClick)
{
	infoClickHandler = _onInfoButtonClick;

	// add the titleLabel
	titleLabel = std::make_shared<W_LABEL>();
	titleLabel->setFont(font_regular, WZCOL_TEXT_BRIGHT);
	titleLabel->setString(WzString::fromUtf8(title));
	titleLabel->setGeometry(0, 0, titleLabel->getMaxLineWidth(), titleLabel->idealHeight());
	titleLabel->setCanTruncate(true);
	titleLabel->setTransparentToMouse(true);
	attach(titleLabel);
	titleLabel->setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
		auto psParent = std::dynamic_pointer_cast<WzRoomTitleBanner>(psWidget->parent());
		ASSERT_OR_RETURN(, psParent != nullptr, "No parent?");
		int x0 = psParent->internalHorizontalPadding;
		int w = psParent->width() - (psParent->internalHorizontalPadding * 3) - 16;
		int h = psParent->height() - (psParent->topPadding + psParent->bottomPadding);
		psWidget->setGeometry(x0, psParent->topPadding, w, h);
	}));

	if (infoClickHandler)
	{
		infoButton = WzInfoButton::make();
		infoButton->setImageDimensions(16);
		attach(infoButton);
		infoButton->setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
			auto psParent = std::dynamic_pointer_cast<WzRoomTitleBanner>(psWidget->parent());
			ASSERT_OR_RETURN(, psParent != nullptr, "No parent?");
			int w = 16 + psParent->internalHorizontalPadding;
			int x0 = psParent->width() - w;
			int h = psParent->height() - (psParent->topPadding + psParent->bottomPadding);
			psWidget->setGeometry(x0, psParent->topPadding, w, h);
		}));
		auto weakSelf = std::weak_ptr<WzRoomTitleBanner>(std::dynamic_pointer_cast<WzRoomTitleBanner>(shared_from_this()));
		infoButton->addOnClickHandler([weakSelf](W_BUTTON&) {
			auto strongSelf = weakSelf.lock();
			ASSERT_OR_RETURN(, strongSelf != nullptr, "No parent?");
			if (strongSelf->infoClickHandler)
			{
				strongSelf->infoClickHandler();
			}
		});
	}
}

void WzRoomTitleBanner::display(int xOffset, int yOffset)
{
	int x0 = xOffset + x();
	int y0 = yOffset + y();
	int w = width();
	int h = height();
	bool highlight = false;

	// draw box
	PIELIGHT boxBorder = WZCOL_MENU_BORDER;
	if (highlight)
	{
		boxBorder = pal_RGBA(255, 255, 255, 255);
	}
	PIELIGHT boxBackground = WZCOL_MENU_BORDER;
	pie_BoxFill(x0, y0, x0 + w, y0 + h, boxBorder);
	pie_BoxFill(x0 + 1, y0 + 1, x0 + w - 1, y0 + h - 1, boxBackground);
}

void WzRoomTitleBanner::geometryChanged()
{
	titleLabel->callCalcLayout();
	if (infoButton)
	{
		infoButton->callCalcLayout();
	}
}

// MARK: - WzBlindRoomHostInfo

class WzBlindRoomHostInfo : public WIDGET
{
protected:
	WzBlindRoomHostInfo()
	: WIDGET()
	{}

	void initialize();

public:
	static std::shared_ptr<WzBlindRoomHostInfo> make()
	{
		class make_shared_enabler: public WzBlindRoomHostInfo {};
		auto widget = std::make_shared<make_shared_enabler>();
		widget->initialize();
		return widget;
	}

	void run(W_CONTEXT *) override;
	int32_t idealHeight() override;
	void geometryChanged() override;

private:
	int topPadding = 4;
	int bottomPadding = 4;
	int internalHorizontalPadding = 10;
	std::shared_ptr<W_LABEL> hostLabel;
	std::shared_ptr<W_LABEL> hostName;
};

int32_t WzBlindRoomHostInfo::idealHeight()
{
	// the height for one row of text
	int32_t titleHeight = std::max<int32_t>(hostLabel->idealHeight(), hostName->idealHeight());
	return topPadding + bottomPadding + titleHeight;
}

void WzBlindRoomHostInfo::initialize()
{
	hostLabel = std::make_shared<W_LABEL>();
	hostLabel->setFont(font_regular, WZCOL_TEXT_DARK);
	hostLabel->setString(_("Host:"));
	hostLabel->setGeometry(0, 0, hostLabel->getMaxLineWidth(), hostLabel->idealHeight());
	hostLabel->setCanTruncate(false);
	hostLabel->setTransparentToMouse(true);
	attach(hostLabel);

	hostName = std::make_shared<W_LABEL>();
	hostName->setFont(font_regular, WZCOL_TEXT_MEDIUM);
	hostName->setString(NetPlay.players[NetPlay.hostPlayer].name);
	hostName->setGeometry(0, 0, hostName->getMaxLineWidth(), hostName->idealHeight());
	hostName->setCanTruncate(true);
	attach(hostName);

	std::string hostTipStr;
	if (NetPlay.hostPlayer < MAX_PLAYER_SLOTS)
	{
		hostTipStr = _("NOTE: Host is also a player.");
	}
	else
	{
		const auto& identity = getOutputPlayerIdentity(NetPlay.hostPlayer);
		if (!identity.empty())
		{
			if (!hostTipStr.empty())
			{
				hostTipStr += "\n";
			}
			std::string hash = identity.publicHashString(20);
			hostTipStr += _("Player ID: ");
			hostTipStr += hash.empty()? _("(none)") : hash;
		}
	}
	hostName->setTip(hostTipStr);
}

void WzBlindRoomHostInfo::geometryChanged()
{
	int w = width();
	int h = height();
	hostLabel->setGeometry(0, 0, hostLabel->width(), h);

	int hostNameX0 = hostLabel->width() + internalHorizontalPadding;
	int hostNameWidth = w - hostNameX0;
	hostName->setGeometry(hostNameX0, 0, hostNameWidth, h);
}

void WzBlindRoomHostInfo::run(W_CONTEXT *)
{
	hostName->setString(NetPlay.players[NetPlay.hostPlayer].name);
}

// MARK: - WzBlindRoomPlayerInfo

class WzBlindRoomPlayerInfo : public WIDGET
{
protected:
	WzBlindRoomPlayerInfo()
	: WIDGET()
	{}

	void initialize(const std::shared_ptr<WzMultiplayerOptionsTitleUI> &parent);

public:
	static std::shared_ptr<WzBlindRoomPlayerInfo> make(const std::shared_ptr<WzMultiplayerOptionsTitleUI> &parent)
	{
		class make_shared_enabler: public WzBlindRoomPlayerInfo {};
		auto widget = std::make_shared<make_shared_enabler>();
		widget->initialize(parent);
		return widget;
	}

	void run(W_CONTEXT *) override;
	int32_t idealHeight() override;
	void geometryChanged() override;

private:
	int topPadding = 4;
	int bottomPadding = 4;
	int internalVerticalPadding = 10;
	std::shared_ptr<W_LABEL> playerLabel;
	std::shared_ptr<WzPlayerRow> playerRow;
};

int32_t WzBlindRoomPlayerInfo::idealHeight()
{
	int32_t result = topPadding + bottomPadding;
	result += playerLabel->idealHeight();
	result += internalVerticalPadding + MULTIOP_PLAYERHEIGHT;
	return result;
}

void WzBlindRoomPlayerInfo::initialize(const std::shared_ptr<WzMultiplayerOptionsTitleUI> &parent)
{
	playerLabel = std::make_shared<W_LABEL>();
	playerLabel->setFont(font_regular, WZCOL_TEXT_DARK);
	playerLabel->setString(_("You:"));
	playerLabel->setGeometry(0, 0, playerLabel->getMaxLineWidth(), playerLabel->idealHeight());
	playerLabel->setCanTruncate(false);
	playerLabel->setTransparentToMouse(true);
	attach(playerLabel);
	int textBottom = playerLabel->y() + playerLabel->height();

	playerRow = WzPlayerRow::make(selectedPlayer, parent);
	attach(playerRow);
	playerRow->setGeometry(0, textBottom + internalVerticalPadding, MULTIOP_PLAYERWIDTH, MULTIOP_PLAYERHEIGHT);

	geometryChanged();
}

void WzBlindRoomPlayerInfo::geometryChanged()
{
	int w = width();

	playerLabel->setGeometry(0, 0, playerLabel->width(), playerLabel->idealHeight());
	int textBottom = playerLabel->y() + playerLabel->height();

	int playerWidgetWidth = std::min<int>(MULTIOP_PLAYERWIDTH, w);
	if (playerWidgetWidth > 0)
	{
		playerRow->setGeometry(0, textBottom + internalVerticalPadding, playerWidgetWidth, MULTIOP_PLAYERHEIGHT);
	}
}

void WzBlindRoomPlayerInfo::run(W_CONTEXT *psContext)
{
	// currently, no-op
}

// MARK: - WzBlindRoomStatusInfo

class WzBlindRoomStatusInfo : public WIDGET
{
protected:
	WzBlindRoomStatusInfo()
	: WIDGET()
	{}

	void initialize();

public:
	static std::shared_ptr<WzBlindRoomStatusInfo> make()
	{
		class make_shared_enabler: public WzBlindRoomStatusInfo {};
		auto widget = std::make_shared<make_shared_enabler>();
		widget->initialize();
		return widget;
	}

	void run(W_CONTEXT *) override;
	int32_t idealHeight() override;
	void geometryChanged() override;

private:
	int topPadding = 4;
	int bottomPadding = 4;
	int internalVerticalPadding = 5;
	std::shared_ptr<W_LABEL> statusLabel;
	std::shared_ptr<W_LABEL> statusDetails;
	std::shared_ptr<WIDGET> indeterminateIndicator;
	WzString statusStr;
	std::chrono::steady_clock::time_point lastStatusUpdate;
	std::chrono::milliseconds minUpdateFreq = std::chrono::milliseconds(500);
};

int32_t WzBlindRoomStatusInfo::idealHeight()
{
	// the height for one row of text
	int result = topPadding + bottomPadding;
	result += statusLabel->idealHeight();
	result += internalVerticalPadding;
	result += statusDetails->idealHeight();
	return result;
}

void WzBlindRoomStatusInfo::initialize()
{
	statusLabel = std::make_shared<W_LABEL>();
	statusLabel->setFont(font_regular, WZCOL_TEXT_DARK);
	statusLabel->setString(_("Status:"));
	statusLabel->setGeometry(0, 0, statusLabel->getMaxLineWidth(), statusLabel->idealHeight());
	statusLabel->setCanTruncate(false);
	statusLabel->setTransparentToMouse(true);
	attach(statusLabel);

	statusDetails = std::make_shared<W_LABEL>();
	statusDetails->setFont(font_regular, WZCOL_TEXT_MEDIUM);
	statusDetails->setString(NetPlay.players[NetPlay.hostPlayer].name);
	statusDetails->setGeometry(0, 0, statusDetails->getMaxLineWidth(), statusDetails->idealHeight());
	statusDetails->setCanTruncate(true);
	attach(statusDetails);

	indeterminateIndicator = createJoiningIndeterminateProgressWidget(font_regular_bold);
	attach(indeterminateIndicator);
}

void WzBlindRoomStatusInfo::geometryChanged()
{
	int w = width();
	statusLabel->setGeometry(0, 0, statusLabel->width(), statusLabel->idealHeight());
	int textBottom = statusLabel->y() + statusLabel->height();

	int statusDetailsWidth = w - indeterminateIndicator->idealWidth() - 5;
	statusDetails->setGeometry(0, textBottom + internalVerticalPadding, statusDetailsWidth, statusDetails->idealHeight());

	indeterminateIndicator->setGeometry(statusDetailsWidth + 5, textBottom + internalVerticalPadding, indeterminateIndicator->idealWidth(), indeterminateIndicator->idealHeight());
}

void WzBlindRoomStatusInfo::run(W_CONTEXT *)
{
	const std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
	if (std::chrono::duration_cast<std::chrono::milliseconds>(now - lastStatusUpdate) < minUpdateFreq)
	{
		return;
	}

	size_t numNotReadyPlayers = 0;
	size_t numReadyPlayers = 0;
	size_t numNotReadySpectators = 0;
	bool hostIsReady = false;
	bool selfIsReady = NetPlay.players[realSelectedPlayer].ready;
	bool playersAllOnSameTeam = (allPlayersOnSameTeam(-1) != -1);

	for (size_t player = 0; player < NetPlay.players.size(); player++)
	{
		// check if this human player is ready, ignore AIs
		if (NetPlay.players[player].allocated)
		{
			if (NetPlay.players[player].ready)
			{
				if (player < game.maxPlayers)
				{
					numReadyPlayers++;
				}
				if (player == NetPlay.hostPlayer)
				{
					hostIsReady = true;
				}
			}
			else
			{
				if (player < game.maxPlayers)
				{
					numNotReadyPlayers++;
				}
				else
				{
					numNotReadySpectators++;
				}
			}
		}
		else if (NetPlay.players[player].ai >= 0 && player < game.maxPlayers)
		{
			numReadyPlayers++;
		}
	}

	// Determine status string
	if (playersAllOnSameTeam)
	{
		if (numNotReadyPlayers + numReadyPlayers > 1)
		{
			statusStr = _("Waiting for host to configure teams");
		}
		else
		{
			statusStr = _("Waiting for players");
		}
	}
	else if (!selfIsReady)
	{
		if (NETgetDownloadProgress(realSelectedPlayer) != 100)
		{
			statusStr = _("Downloading map / mods");
		}
		else
		{
			statusStr = _("Waiting for you to check ready");
		}
	}
	else if (numReadyPlayers > 0 && numNotReadyPlayers > 0)
	{
		statusStr = _("Waiting for players");
	}
	else if (!hostIsReady && NetPlay.players[NetPlay.hostPlayer].isSpectator)
	{
		statusStr = _("Waiting for host to start game");
	}
	else if (numNotReadySpectators > 0)
	{
		statusStr = _("Waiting for spectators");
	}
	else
	{
		statusStr = _("Waiting for players");
	}

	statusDetails->setString(statusStr);

	lastStatusUpdate = now;
}

// MARK: - WzBlindPlayersReadyInfo

class WzBlindPlayersReadyInfo : public WIDGET
{
protected:
	WzBlindPlayersReadyInfo()
	: WIDGET()
	{}

	void initialize();

public:
	static std::shared_ptr<WzBlindPlayersReadyInfo> make()
	{
		class make_shared_enabler: public WzBlindPlayersReadyInfo {};
		auto widget = std::make_shared<make_shared_enabler>();
		widget->initialize();
		return widget;
	}

	void run(W_CONTEXT *) override;
	int32_t idealHeight() override;
	void geometryChanged() override;
	std::string getTip() override;

private:
	int topPadding = 4;
	int bottomPadding = 4;
	int internalHorizontalPadding = 10;
	std::shared_ptr<W_LABEL> readyCountLabel;
	std::shared_ptr<W_LABEL> readyCountValue;
	std::chrono::steady_clock::time_point lastStatusUpdate;
	std::chrono::milliseconds minUpdateFreq = std::chrono::milliseconds(500);
	std::string tooltip;
};

int32_t WzBlindPlayersReadyInfo::idealHeight()
{
	// the height for one row of text
	int32_t titleHeight = std::max<int32_t>(readyCountLabel->idealHeight(), readyCountValue->idealHeight());
	return topPadding + bottomPadding + titleHeight;
}

void WzBlindPlayersReadyInfo::initialize()
{
	readyCountLabel = std::make_shared<W_LABEL>();
	readyCountLabel->setFont(font_regular, WZCOL_TEXT_DARK);
	readyCountLabel->setString(_("Players ready & waiting for match:"));
	readyCountLabel->setGeometry(0, 0, readyCountLabel->getMaxLineWidth(), readyCountLabel->idealHeight());
	readyCountLabel->setCanTruncate(false);
	readyCountLabel->setTransparentToMouse(true);
	attach(readyCountLabel);

	readyCountValue = std::make_shared<W_LABEL>();
	readyCountValue->setFont(font_regular, WZCOL_TEXT_DARK);
	readyCountValue->setString("0");
	readyCountValue->setGeometry(0, 0, readyCountValue->getMaxLineWidth(), readyCountValue->idealHeight());
	readyCountValue->setCanTruncate(true);
	readyCountValue->setTransparentToMouse(true);
	attach(readyCountValue);
}

void WzBlindPlayersReadyInfo::geometryChanged()
{
	int w = width();
	int h = height();

	// ensure the value is fully visible
	int readyCountValueWidth = readyCountValue->getMaxLineWidth();
	int readyCountValueX0 = w - readyCountValueWidth;
	readyCountValue->setGeometry(readyCountValueX0, 0, readyCountValueWidth, h);

	// then use the rest of the space for the label
	int labelWidth = std::max<int>(w - readyCountValueWidth - internalHorizontalPadding, 0);
	readyCountLabel->setGeometry(0, 0, labelWidth, h);
}

void WzBlindPlayersReadyInfo::run(W_CONTEXT *)
{
	const std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
	if (std::chrono::duration_cast<std::chrono::milliseconds>(now - lastStatusUpdate) < minUpdateFreq)
	{
		return;
	}

	size_t numNotReadyPlayers = 0;
	size_t numReadyPlayers = 0;
	size_t numReadyAIBots = 0;
	size_t numNotReadySpectators = 0;
	size_t numReadySpectators = 0;
	bool hostIsReady = false;

	for (size_t player = 0; player < NetPlay.players.size(); player++)
	{
		// check if this human player is ready, ignore AIs
		if (NetPlay.players[player].allocated)
		{
			if (NetPlay.players[player].ready)
			{
				if (player < game.maxPlayers)
				{
					numReadyPlayers++;
				}
				else
				{
					numReadySpectators++;
				}
				if (player == NetPlay.hostPlayer)
				{
					hostIsReady = true;
				}
			}
			else
			{
				if (player < game.maxPlayers)
				{
					numNotReadyPlayers++;
				}
				else
				{
					numNotReadySpectators++;
				}
			}
		}
		else if (NetPlay.players[player].ai >= 0 && player < game.maxPlayers)
		{
			numReadyAIBots++;
		}
	}

	readyCountValue->setString(WzString::number(numReadyPlayers));

	tooltip = astringf(_("Players: (%zu ready, %zu not ready)"), numReadyPlayers, numNotReadyPlayers);
	if (numReadyAIBots > 0)
	{
		tooltip += "\n";
		tooltip += astringf(_("AI Bots: %zu"), numReadyAIBots);
	}
	if (numReadySpectators > 0 || numNotReadySpectators > 0)
	{
		tooltip += "\n";
		tooltip += astringf(_("Spectators: (%zu ready, %zu not ready)"), numReadySpectators, numNotReadySpectators);
	}
	tooltip += "\n";
	tooltip += astringf(_("Host: %s"), (hostIsReady) ? _("ready") : _("not ready"));
}

std::string WzBlindPlayersReadyInfo::getTip()
{
	return tooltip;
}

// MARK: - WzPlayerBlindWaitingRoom

class WzPlayerBlindWaitingRoom : public WIDGET
{
protected:
	WzPlayerBlindWaitingRoom(const std::shared_ptr<WzMultiplayerOptionsTitleUI>& titleUI)
	: WIDGET()
	, weakTitleUI(titleUI)
	{ }

	~WzPlayerBlindWaitingRoom()
	{
	}

public:
	static std::shared_ptr<WzPlayerBlindWaitingRoom> make(const std::shared_ptr<WzMultiplayerOptionsTitleUI>& titleUI)
	{
		class make_shared_enabler: public WzPlayerBlindWaitingRoom {
		public:
			make_shared_enabler(const std::shared_ptr<WzMultiplayerOptionsTitleUI>& titleUI) : WzPlayerBlindWaitingRoom(titleUI) { }
		};
		auto widget = std::make_shared<make_shared_enabler>(titleUI);
		widget->initialize(titleUI);
		return widget;
	}

	void initialize(const std::shared_ptr<WzMultiplayerOptionsTitleUI>& titleUI)
	{
		// Add the "Waiting Room" info bar at top
		titleBanner = WzRoomTitleBanner::make(_("Waiting Room"), []() {
			widgScheduleTask([]() {
				printBlindModeHelpMessagesToConsole();
			});
		});
		attach(titleBanner);

		// Add host details
		hostInfo = WzBlindRoomHostInfo::make();
		attach(hostInfo);

		// Add the current player details
		playerInfo = WzBlindRoomPlayerInfo::make(titleUI);
		attach(playerInfo);

		// Add Status field)
		statusInfo = WzBlindRoomStatusInfo::make();
		attach(statusInfo);

		// Add "People ready & waiting for match: <number>"
		playersReadyInfo = WzBlindPlayersReadyInfo::make();
		attach(playersReadyInfo);

		setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
			auto psPlayerBlindWaitingRoom = std::dynamic_pointer_cast<WzPlayerBlindWaitingRoom>(psWidget->shared_from_this());
			ASSERT_OR_RETURN(, psPlayerBlindWaitingRoom.get() != nullptr, "Wrong type of psWidget??");
			psPlayerBlindWaitingRoom->recalculateInternalLayout();
		}));
	}

	void geometryChanged() override
	{
		recalculateInternalLayout();
	}

	int getPlayerRowY0()
	{
		ASSERT_OR_RETURN(0, playerInfo != nullptr, "Invalid widget?");
		return playerInfo->y();
	}

protected:
	void display(int xOffset, int yOffset) override;

private:

	void recalculateInternalLayout()
	{
		int w = width();
		int h = height();

		titleBanner->setGeometry(0, 0, w, titleBanner->idealHeight());
		int nextWidgetY0 = titleBanner->y() + titleBanner->height() + titleBannerBottomPadding;

		int widgetX0 = outerWidgetPaddingX;
		int usableWidgetWidth = w - (outerWidgetPaddingX * 2);
		hostInfo->setGeometry(widgetX0, nextWidgetY0, usableWidgetWidth, hostInfo->idealHeight());
		nextWidgetY0 = hostInfo->y() + hostInfo->height() + betweenWidgetPaddingY;

		playerInfo->setGeometry(widgetX0, nextWidgetY0, usableWidgetWidth, playerInfo->idealHeight());
		nextWidgetY0 = playerInfo->y() + playerInfo->height() + betweenWidgetPaddingY;

		statusInfo->setGeometry(widgetX0, nextWidgetY0, usableWidgetWidth, statusInfo->idealHeight());
		nextWidgetY0 = statusInfo->y() + statusInfo->height() + betweenWidgetPaddingY;

		// aligned bottom-up
		int bottomWidgetY0 = h - playersReadyInfo->idealHeight() - bottomPaddingY;
		playersReadyInfo->setGeometry(widgetX0, bottomWidgetY0, usableWidgetWidth, playersReadyInfo->idealHeight());
	}

private:
	std::weak_ptr<WzMultiplayerOptionsTitleUI> weakTitleUI;

	int titleBannerBottomPadding = 10;
	int outerWidgetPaddingX = 15;
	int betweenWidgetPaddingY = 10;
	int bottomPaddingY = 15;

	std::shared_ptr<WzRoomTitleBanner> titleBanner;
	std::shared_ptr<WzBlindRoomHostInfo> hostInfo;
	std::shared_ptr<WzBlindRoomPlayerInfo> playerInfo;
	std::shared_ptr<WzBlindRoomStatusInfo> statusInfo;
	std::shared_ptr<WzBlindPlayersReadyInfo> playersReadyInfo;
};

void WzPlayerBlindWaitingRoom::display(int xOffset, int yOffset)
{
	int x0 = xOffset + x();
	int y0 = yOffset + y();
	int w = width();
	int h = height();

	pie_BoxFill(x0, y0, x0 + w, y0 + h, WZCOL_MENU_BACKGROUND);
	iV_Box(x0, y0, x0 + w, y0 + h, WZCOL_MENU_BORDER);
}


// MARK: - Public methods

std::shared_ptr<WIDGET> makeWzPlayerBlindWaitingRoom(const std::shared_ptr<WzMultiplayerOptionsTitleUI>& titleUI)
{
	return WzPlayerBlindWaitingRoom::make(titleUI);
}

int getWzPlayerBlindWaitingRoomPlayerRowY0(const std::shared_ptr<WIDGET>& blindWaitingRoomWidget)
{
	auto pBlindWaitingRoomWidget = std::dynamic_pointer_cast<WzPlayerBlindWaitingRoom>(blindWaitingRoomWidget);
	ASSERT_OR_RETURN(0, pBlindWaitingRoomWidget != nullptr, "Invalid widget");
	return pBlindWaitingRoomWidget->getPlayerRowY0();
}
