/*
	This file is part of Warzone 2100.
	Copyright (C) 2024-2026  Warzone 2100 Project

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
 *  Lobby Player Box Tabs  Widget (and associated display functions)
 */

#include "lobbyplayerboxtabs.h"
#include "src/frontend.h"
#include "src/frend.h"
#include "src/intimage.h"
#include "src/multiint.h"
#include "src/multiplay.h"
#include "src/warzoneconfig.h"

#include "lib/widget/label.h"
#include "lib/widget/button.h"

#include "lib/framework/input.h"
#include "lib/framework/wzapp.h"

#include "lib/netplay/netplay.h"
#include "lib/netplay/netlobby.h"

#include "src/titleui/multiplayer.h"

#include "frontendimagebutton.h"
#include "optionsform.h"
#include "src/titleui/options/optionsforms.h"

// MARK: - Helper functions

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

// MARK: - LobbyHostOptionsForm

class LobbyHostOptionsForm : public WIDGET
{
protected:
	LobbyHostOptionsForm() {}

	void initialize(const std::weak_ptr<WzMultiplayerOptionsTitleUI>& weakTitleUI, const std::function<void ()>& onClose);
public:
	static std::shared_ptr<LobbyHostOptionsForm> make(const std::weak_ptr<WzMultiplayerOptionsTitleUI>& weakTitleUI, const std::function<void ()>& onCloseFunc)
	{
		class make_shared_enabler: public LobbyHostOptionsForm {};
		auto widget = std::make_shared<make_shared_enabler>();
		widget->initialize(weakTitleUI, onCloseFunc);
		return widget;
	}
	int32_t idealWidth() override;
	int32_t idealHeight() override;
protected:
	virtual void geometryChanged() override;
	virtual void display(int xOffset, int yOffset) override;
private:
	std::shared_ptr<OptionsForm> makeHostOptionsForm();

	bool setSpectatorHost(bool value);
	bool setBlindMode(BLIND_MODE value);

private:
	std::weak_ptr<WzMultiplayerOptionsTitleUI> weakTitleUI;
	std::shared_ptr<W_LABEL> formTitle;
	std::shared_ptr<WzFrontendImageButton> closeButton;
	std::shared_ptr<OptionsForm> hostOptionsForm;
	const int innerVerticalSpacing = 15;
	const int outerPaddingX = 20;
	const int optionsListPaddingX = 0;
	const int outerPaddingY = 20;
	const int betweenButtonPadding = 15;
	const int closeButtonVerticalPadding = 5;
};

void LobbyHostOptionsForm::initialize(const std::weak_ptr<WzMultiplayerOptionsTitleUI>& _weakTitleUI, const std::function<void ()>& onClose)
{
	weakTitleUI = _weakTitleUI;

	formTitle = std::make_shared<W_LABEL>();
	formTitle->setFont(font_medium_bold, WZCOL_TEXT_BRIGHT);
	formTitle->setString(_("Host Lobby Options"));
	formTitle->setCanTruncate(true);
	formTitle->setTransparentToMouse(true);
	formTitle->setGeometry(outerPaddingX, outerPaddingY, formTitle->getMaxLineWidth(), iV_GetTextLineSize(font_regular_bold));
	attach(formTitle);

	// Add "Close" button
	closeButton = WzFrontendImageButton::make(nullopt);
	closeButton->setString(_("Close"));
	closeButton->addOnClickHandler([onClose](W_BUTTON&) {
		widgScheduleTask([onClose]() {
			onClose();
		});
	});
	closeButton->setPadding(7, closeButtonVerticalPadding);
	attach(closeButton);

	hostOptionsForm = makeHostOptionsForm();
	attach(hostOptionsForm);
}

int32_t LobbyHostOptionsForm::idealWidth()
{
	return (outerPaddingX * 2) + optionsListPaddingX + hostOptionsForm->idealWidth();
}

int32_t LobbyHostOptionsForm::idealHeight()
{
	int32_t result = (outerPaddingY * 2) + std::max(formTitle->idealHeight(), closeButton->idealHeight() - closeButtonVerticalPadding);
	result += innerVerticalSpacing;
	result += hostOptionsForm->idealHeight();
	result += innerVerticalSpacing;
	return result;
}

static bool hasGameCurrentlyHostedInLobby()
{
	return !NET_getCurrentHostedLobbyGameId().empty();
}

OptionInfo::AvailabilityResult IsGameHostedInLobby(const OptionInfo&)
{
	OptionInfo::AvailabilityResult result;
	result.available = hasGameCurrentlyHostedInLobby();
	result.localizedUnavailabilityReason = _("Game must be listed in lobby to support this option.");
	return result;
}

OptionInfo::OptionAvailabilityCondition MakeIsLobbyHostJoinOptSupportedCondition(netlobby::HostJoinOptionType optType)
{
	return [optType](const OptionInfo&) -> OptionInfo::AvailabilityResult {
		auto optAvailable = NET_getHostJoinOptionSupportedByLobby(optType);
		OptionInfo::AvailabilityResult result;
		result.available = optAvailable.value_or(false);
		if (optAvailable.has_value())
		{
			result.localizedUnavailabilityReason = _("This option is not supported by the current lobby.");
		}
		else
		{
			result.localizedUnavailabilityReason = _("Awaiting lobby information...");
		}
		return result;
	};
}

static bool lobbyHostJoinOptionEnabled(netlobby::HostJoinOptionType optType)
{
	return hasGameCurrentlyHostedInLobby() && NET_getHostJoinOptionSupportedByLobby(optType).value_or(false);
}

std::shared_ptr<OptionsForm> LobbyHostOptionsForm::makeHostOptionsForm()
{
	std::weak_ptr<WzMultiplayerOptionsTitleUI> weakTitleUICopy = weakTitleUI;
	std::weak_ptr<LobbyHostOptionsForm> psWeakParent = std::dynamic_pointer_cast<LobbyHostOptionsForm>(shared_from_this());
	auto result = OptionsForm::make();

	// Slot Options
	result->addSection(OptionsSection(N_("Slot Options"), ""), true);
	{
		auto optionInfo = OptionInfo("mp.spectatorjoin", N_("Spectator Join"), "");
		auto valueChanger = OptionsDropdown<bool>::make(
			[]() {
				OptionChoices<bool> result;
				result.choices = {
					{ _("Off"), "", false },
					{ _("Allow"), "", true },
				};
				bool hasOpenSpectatorSlots = hasOpenSpectatorOnlySlots();
				result.setCurrentIdxForValue(hasOpenSpectatorSlots);
				return result;
			},
			[weakTitleUICopy](const auto& newValue) -> bool {
				auto strongTitleUI = weakTitleUICopy.lock();
				ASSERT_OR_RETURN(false, strongTitleUI != nullptr, "No Title UI?");
				enableSpectatorJoin(newValue);
				strongTitleUI->updatePlayers();
				return true;
			}, true
		);
		result->addOption(optionInfo, valueChanger, true);
	}
	{
		auto optionInfo = OptionInfo("mp.locked.teams", N_("Teams"), "");
		auto valueChanger = OptionsDropdown<bool>::make(
			[]() {
				OptionChoices<bool> result;
				result.choices = {
					{ _("Locked"), "", true },
					{ _("Allow Change"), "", false },
				};
				result.setCurrentIdxForValue(getLockedOptions().teams);
				return result;
			},
			[weakTitleUICopy](const auto& newValue) -> bool {
				auto newLocked = getLockedOptions();
				newLocked.teams = newValue;
				updateLockedOptionsOnHost(newLocked);
				return true;
			}, false
		);
		result->addOption(optionInfo, valueChanger, true);
	}
	{
		auto optionInfo = OptionInfo("mp.locked.position", N_("Position"), "");
		auto valueChanger = OptionsDropdown<bool>::make(
			[]() {
				OptionChoices<bool> result;
				result.choices = {
					{ _("Locked"), "", true },
					{ _("Allow Change"), "", false },
				};
				result.setCurrentIdxForValue(getLockedOptions().position);
				return result;
			},
			[weakTitleUICopy](const auto& newValue) -> bool {
				auto newLocked = getLockedOptions();
				newLocked.position = newValue;
				updateLockedOptionsOnHost(newLocked);
				return true;
			}, false
		);
		result->addOption(optionInfo, valueChanger, true);
	}

	// Game Options
	result->addSection(OptionsSection(N_("Game Options"), ""), true);
	{
		auto optionInfo = OptionInfo("defaults.hosting.inactivityTimeout", N_("Inactivity Timeout"), "");
		optionInfo.addAvailabilityCondition(IsNotInGame);
		auto valueChanger = OptionsDropdown<uint32_t>::make(
			makeInactivityTimeoutOptionsDropdownPopulateFunc(),
			[](const auto& newValue) -> bool {
				war_setMPInactivityMinutes(newValue);
				game.inactivityMinutes = war_getMPInactivityMinutes();
				sendOptions();
				return true;
			}, false
		);
		result->addOption(optionInfo, valueChanger, true);
	}
	{
		auto optionInfo = OptionInfo("defaults.hosting.lagKick", N_("Lag Kick"), "");
		auto valueChanger = OptionsDropdown<int>::make(
			makeLagKickOptionsDropdownPopulateFunc(),
			[](const auto& newValue) -> bool {
				war_setAutoLagKickSeconds(newValue);
				return true;
			}, false
		);
		result->addOption(optionInfo, valueChanger, true);
	}
	{
		auto optionInfo = OptionInfo("defaults.hosting.gameTimeLimit", N_("Game Time Limit"), "");
		optionInfo.addAvailabilityCondition(IsNotInGame);
		auto valueChanger = OptionsDropdown<uint32_t>::make(
			makeGameTimeLimitOptionsDropdownPopulateFunc(),
			[](const auto& newValue) -> bool {
				war_setMPGameTimeLimitMinutes(newValue);
				game.gameTimeLimitMinutes = war_getMPGameTimeLimitMinutes();
				sendOptions();
				return true;
			}, false
		);
		result->addOption(optionInfo, valueChanger, true);
	}

	// Host Join Options
	result->addSection(OptionsSection(N_("Lobby Join Options"), ""), true);
	{
		auto optionInfo = OptionInfo("mp.hostjoinoptions.botprot", N_("Bot Protection"), N_("Whether automated bots or tools can join or gather host connection info (including your IP address). If you are encountering undesirable joins, spammers, or other abuse, you may wish to enable this option."));
		optionInfo.addAvailabilityCondition(IsGameHostedInLobby);
		optionInfo.addAvailabilityCondition(MakeIsLobbyHostJoinOptSupportedCondition(netlobby::HostJoinOptionType::BotProt));
		auto valueChanger = OptionsDropdown<netlobby::HostJoinOptionValue>::make(
			[]() {
				OptionChoices<netlobby::HostJoinOptionValue> result;
				result.choices = {
					{ _("Block Known Bots"), _("Block known bots or automated tools."), netlobby::HostJoinOptionValue::BlockAll },
					{ _("Disabled"), "", netlobby::HostJoinOptionValue::Allow },
				};
				result.setCurrentIdxForValue(lobbyHostJoinOptionEnabled(netlobby::HostJoinOptionType::BotProt) ? NETgetLobbyHostJoinOptions().botProt : netlobby::HostJoinOptionValue::Allow );
				return result;
			},
			[weakTitleUICopy](const auto& newValue) -> bool {
				if (!lobbyHostJoinOptionEnabled(netlobby::HostJoinOptionType::BotProt)) { return false; }
				auto opts = NETgetLobbyHostJoinOptions();
				opts.botProt = newValue;
				NETsetLobbyHostJoinOptions(opts);
				return true;
			}, false
		);
		result->addOption(optionInfo, valueChanger, true);
	}
	{
		auto optionInfo = OptionInfo("mp.hostjoinoptions.proxyips", N_("Proxy / VPN Connections"), N_("Proxies / VPNs hide the true source of connections. May be used by players trying to evade bans or hide their identity."));
		optionInfo.addAvailabilityCondition(IsGameHostedInLobby);
		optionInfo.addAvailabilityCondition(MakeIsLobbyHostJoinOptSupportedCondition(netlobby::HostJoinOptionType::ProxyIPs));
		auto valueChanger = OptionsDropdown<netlobby::HostJoinOptionValue>::make(
			[]() {
				OptionChoices<netlobby::HostJoinOptionValue> result;
				WzString susHelpDescription = _("Block suspicious connections from proxies / VPNs.");
				susHelpDescription += " ";
				susHelpDescription += _("(Recommended)");
				result.choices = {
					{ _("Block All"), _("Block all connections from proxies / VPNs."), netlobby::HostJoinOptionValue::BlockAll },
					{ _("Block Suspicious"), susHelpDescription, netlobby::HostJoinOptionValue::BlockSuspicious },
					{ _("Allow"), "", netlobby::HostJoinOptionValue::Allow },
				};
				result.setCurrentIdxForValue(lobbyHostJoinOptionEnabled(netlobby::HostJoinOptionType::ProxyIPs) ? NETgetLobbyHostJoinOptions().proxyIPs : netlobby::HostJoinOptionValue::Allow );
				return result;
			},
			[weakTitleUICopy](const auto& newValue) -> bool {
				if (!lobbyHostJoinOptionEnabled(netlobby::HostJoinOptionType::ProxyIPs)) { return false; }
				auto opts = NETgetLobbyHostJoinOptions();
				opts.proxyIPs = newValue;
				NETsetLobbyHostJoinOptions(opts);
				return true;
			}, false
		);
		result->addOption(optionInfo, valueChanger, true);
	}
	{
		auto optionInfo = OptionInfo("mp.hostjoinoptions.hostingips", N_("Hosting Provider IPs"), N_("Server hosting providers. May be used by automated tools, port scanners, or players trying to evade bans."));
		optionInfo.addAvailabilityCondition(IsGameHostedInLobby);
		optionInfo.addAvailabilityCondition(MakeIsLobbyHostJoinOptSupportedCondition(netlobby::HostJoinOptionType::HostingIPs));
		auto valueChanger = OptionsDropdown<netlobby::HostJoinOptionValue>::make(
			[]() {
				OptionChoices<netlobby::HostJoinOptionValue> result;
				result.choices = {
					{ _("Block All"), _("Block all connections from hosting provider IPs."), netlobby::HostJoinOptionValue::BlockAll },
					{ _("Block Suspicious"), _("Block suspicious connections from hosting provider IPs."), netlobby::HostJoinOptionValue::BlockSuspicious },
					{ _("Allow"), "", netlobby::HostJoinOptionValue::Allow },
				};
				result.setCurrentIdxForValue(lobbyHostJoinOptionEnabled(netlobby::HostJoinOptionType::HostingIPs) ? NETgetLobbyHostJoinOptions().hostingIPs : netlobby::HostJoinOptionValue::Allow);
				return result;
			},
			[weakTitleUICopy](const auto& newValue) -> bool {
				if (!lobbyHostJoinOptionEnabled(netlobby::HostJoinOptionType::HostingIPs)) { return false; }
				auto opts = NETgetLobbyHostJoinOptions();
				opts.hostingIPs = newValue;
				NETsetLobbyHostJoinOptions(opts);
				return true;
			}, false
		);
		result->addOption(optionInfo, valueChanger, true);
	}
	{
		auto optionInfo = OptionInfo("mp.hostjoinoptions.lobbyjoinbypass", N_("Lobby Join Bypass"), N_("Allow clients to bypass lobby join via direct port:ip connection. (May enable trolls to bypass lobby anti-abuse protections.)"));
		optionInfo.addAvailabilityCondition(IsGameHostedInLobby);
		auto valueChanger = OptionsDropdown<LobbyHostDirectJoinOption>::make(
			[]() {
				OptionChoices<LobbyHostDirectJoinOption> result;
				WzString allowAllDescription = _("Allow any IP to direct join. Bypasses lobby anti-abuse protections.");
				allowAllDescription += " ";
				allowAllDescription += _("(Not Recommended)");
				result.choices = {
					{ _("Off"), _("Do not allow direct joins. Require joining through the lobby."), LobbyHostDirectJoinOption::None },
					{ _("Local"), _("Allow local / LAN IPs to direct join. Require others to join through the lobby."), LobbyHostDirectJoinOption::Local },
					{ _("Allow All"), allowAllDescription, LobbyHostDirectJoinOption::All },
				};
				result.setCurrentIdxForValue(hasGameCurrentlyHostedInLobby() ? NETgetLobbyHostDirectJoinOption() : LobbyHostDirectJoinOption::All);
				return result;
			},
			[](const auto& newValue) -> bool {
				if (!hasGameCurrentlyHostedInLobby()) { return false; }
				NETsetLobbyHostDirectJoinOption(newValue);
				return true;
			}, false
		);
		result->addOption(optionInfo, valueChanger, true);
	}

	return result;
}

void LobbyHostOptionsForm::geometryChanged()
{
	if (width() <= 1 || height() <= 1)
	{
		return;
	}

	// position top button(s)
	int lastButtonX0 = width() - outerPaddingX;
	int buttonY0 = outerPaddingY - closeButtonVerticalPadding;
	int topButtonHeight = closeButton->idealHeight();
	if (closeButton)
	{
		int buttonX0 = lastButtonX0 - closeButton->idealWidth();
		closeButton->setGeometry(buttonX0, buttonY0, closeButton->idealWidth(), topButtonHeight);
		lastButtonX0 = buttonX0 - betweenButtonPadding;
	}

	int maxAvailableOptionsFormHeight = height() - (outerPaddingY * 2) - std::max<int>(formTitle->height(), closeButton->height() - closeButtonVerticalPadding) - (innerVerticalSpacing * 2);
	int hostOptionsFormY0 = outerPaddingY + std::max(formTitle->idealHeight(), closeButton->idealHeight()) + innerVerticalSpacing;
	hostOptionsForm->setGeometry(outerPaddingX + optionsListPaddingX, hostOptionsFormY0, width() - (outerPaddingX * 2) - optionsListPaddingX, std::min<int32_t>(maxAvailableOptionsFormHeight, hostOptionsForm->idealHeight()));

}

void LobbyHostOptionsForm::display(int xOffset, int yOffset)
{
	int x0 = x() + xOffset;
	int y0 = y() + yOffset;
	int x1 = x0 + width();
	int y1 = y0 + height();

	PIELIGHT backColor = WZCOL_TRANSPARENT_BOX;
	backColor.byte.a = std::max<UBYTE>(220, backColor.byte.a);
	pie_UniTransBoxFill(x0, y0, x1, y1, backColor);

	iV_Box(x0, y0, x1, y1, pal_RGBA(255, 255, 255, 175));
}


// MARK: - WzPlayerBoxTabButton

class WzPlayerBoxTabButton : public W_BUTTON
{
protected:
	WzPlayerBoxTabButton();

public:
	static std::shared_ptr<WzPlayerBoxTabButton> make(const std::string& title);

	void display(int xOffset, int yOffset) override;
	void geometryChanged() override;

public:
	void setSlotCounts(uint8_t numTakenSlots, uint8_t totalAvailableSlots);
	void setSlotReadyStatus(uint8_t numTakenSlotsReady);
	void setSelected(bool selected);

private:
	AtlasImage getImageForSlotsReadyStatus() const;
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

WzPlayerBoxTabButton::WzPlayerBoxTabButton()
: W_BUTTON()
{}

std::shared_ptr<WzPlayerBoxTabButton> WzPlayerBoxTabButton::make(const std::string& title)
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

AtlasImage WzPlayerBoxTabButton::getImageForSlotsReadyStatus() const
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

// MARK: - WzPlayerBoxOptionsButton

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

// MARK: - WzPlayerBoxTabs

WzPlayerBoxTabs::WzPlayerBoxTabs(const std::shared_ptr<WzMultiplayerOptionsTitleUI>& titleUI)
: WIDGET()
, weakTitleUI(titleUI)
{ }

WzPlayerBoxTabs::~WzPlayerBoxTabs()
{
	closeOptionsOverlay();
}

std::shared_ptr<WzPlayerBoxTabs> WzPlayerBoxTabs::make(bool displayHostOptions, const std::shared_ptr<WzMultiplayerOptionsTitleUI>& titleUI)
{
	class make_shared_enabler: public WzPlayerBoxTabs {
	public:
		make_shared_enabler(const std::shared_ptr<WzMultiplayerOptionsTitleUI>& titleUI) : WzPlayerBoxTabs(titleUI) { }
	};
	auto widget = std::make_shared<make_shared_enabler>(titleUI);

	std::weak_ptr<WzMultiplayerOptionsTitleUI> weakTitleUI(titleUI);
	std::weak_ptr<WzPlayerBoxTabs> weakSelf(widget);

	// add the "players" button
	auto playersButton = WzPlayerBoxTabButton::make(_("Players"));
	playersButton->setSelected(true);
	widget->tabButtons.push_back(playersButton);
	widget->attach(playersButton);
	playersButton->addOnClickHandler([weakSelf](W_BUTTON& button){
		widgScheduleTask([weakSelf](){
			auto strongSelf = weakSelf.lock();
			ASSERT_OR_RETURN(, strongSelf != nullptr, "Self is null?");
			strongSelf->setDisplayView(PlayerDisplayView::Players);
		});
	});

	// add the "spectators" button
	auto spectatorsButton = WzPlayerBoxTabButton::make(_("Spectators"));
	widget->tabButtons.push_back(spectatorsButton);
	widget->attach(spectatorsButton);
	spectatorsButton->addOnClickHandler([weakSelf](W_BUTTON& button){
		widgScheduleTask([weakSelf](){
			auto strongSelf = weakSelf.lock();
			ASSERT_OR_RETURN(, strongSelf != nullptr, "Self is null?");
			strongSelf->setDisplayView(PlayerDisplayView::Spectators);
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

void WzPlayerBoxTabs::geometryChanged()
{
	recalculateTabLayout();
}

void WzPlayerBoxTabs::run(W_CONTEXT *psContext)
{
	refreshData();
}

WzPlayerBoxTabs::PlayerDisplayView WzPlayerBoxTabs::getDisplayView() const
{
	return playerDisplayView;
}

void WzPlayerBoxTabs::setDisplayView(WzPlayerBoxTabs::PlayerDisplayView view)
{
	playerDisplayView = view;

	auto strongTitleUI = weakTitleUI.lock();
	ASSERT_OR_RETURN(, strongTitleUI != nullptr, "No parent title UI");
	strongTitleUI->updatePlayers();
}

void WzPlayerBoxTabs::setSelectedTab(size_t index)
{
	for (size_t i = 0; i < tabButtons.size(); ++i)
	{
		tabButtons[i]->setSelected(i == index);
	}
}

void WzPlayerBoxTabs::refreshData()
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

void WzPlayerBoxTabs::recalculateTabLayout()
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

void WzPlayerBoxTabs::closeOptionsOverlay()
{
	if (optionsOverlayScreen)
	{
		widgRemoveOverlayScreen(optionsOverlayScreen);
		optionsOverlayScreen.reset();
	}
}

std::shared_ptr<WIDGET> WzPlayerBoxTabs::createOptionsPopoverForm()
{
	std::weak_ptr<WzPlayerBoxTabs> psWeakSelf(std::dynamic_pointer_cast<WzPlayerBoxTabs>(shared_from_this()));
	auto optionsPopOver = LobbyHostOptionsForm::make(weakTitleUI, [psWeakSelf](){
		if (auto strongSelf = psWeakSelf.lock())
		{
			strongSelf->closeOptionsOverlay();
		}
	});
	optionsPopOver->setGeometry(0, 0, optionsPopOver->idealWidth(), optionsPopOver->idealHeight());
	return optionsPopOver;
}

void WzPlayerBoxTabs::displayOptionsOverlay(const std::shared_ptr<WIDGET> &psParent)
{
	auto lockedScreen = screenPointer.lock();
	ASSERT(lockedScreen != nullptr, "The WzPlayerBoxTabs does not have an associated screen pointer?");

	// Initialize the options overlay screen
	optionsOverlayScreen = W_SCREEN::make();
	auto newRootFrm = W_FULLSCREENOVERLAY_CLICKFORM::make();
	std::weak_ptr<W_SCREEN> psWeakOptionsOverlayScreen(optionsOverlayScreen);
	std::weak_ptr<WzPlayerBoxTabs> psWeakParent = std::dynamic_pointer_cast<WzPlayerBoxTabs>(shared_from_this());
	newRootFrm->onClickedFunc = [psWeakOptionsOverlayScreen, psWeakParent]() {
		if (auto psOverlayScreen = psWeakOptionsOverlayScreen.lock())
		{
			widgRemoveOverlayScreen(psOverlayScreen);
		}
		// Destroy Options overlay / overlay screen
		if (auto strongParent = psWeakParent.lock())
		{
			strongParent->optionsOverlayScreen.reset();
		}
	};
	newRootFrm->onCancelPressed = newRootFrm->onClickedFunc;
	optionsOverlayScreen->psForm->attach(newRootFrm);

	// Create the pop-over form
	auto optionsPopOver = createOptionsPopoverForm();
	newRootFrm->attach(optionsPopOver);

	// Position the pop-over form
	optionsPopOver->setCalcLayout([](WIDGET *psWidget) {
		int popOverWidth = std::min<int>(std::max<int>(psWidget->idealWidth(), 400), screenWidth - 40);
		int popOverHeight = std::min<int>(psWidget->idealHeight(), screenHeight - 40);

		// Centered in screen
		int popOverX0 = (screenWidth - popOverWidth) / 2;
		if (popOverX0 < 0)
		{
			popOverX0 = 0;
		}
		int popOverY0 = (screenHeight - popOverHeight) / 2;
		if (popOverY0 < 0)
		{
			popOverY0 = 0;
		}

		psWidget->setGeometry(popOverX0, popOverY0, popOverWidth, popOverHeight);
	});

	widgRegisterOverlayScreenOnTopOfScreen(optionsOverlayScreen, lockedScreen);
	widgScheduleTask([lockedScreen]() {
		// clear focus from the underlying chatbox, which aggressively takes focus
		lockedScreen->setFocus(nullptr);
	});
}
