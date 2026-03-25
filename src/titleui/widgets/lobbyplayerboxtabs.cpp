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

#include "lib/widget/label.h"
#include "lib/widget/button.h"
#include "lib/widget/popovermenu.h"
#include "advcheckbox.h"

#include "lib/framework/input.h"
#include "lib/framework/wzapp.h"

#include "lib/netplay/netplay.h"

#include "src/titleui/multiplayer.h"

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
	if (currentPopoverMenu)
	{
		currentPopoverMenu->closeMenu();
	}
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
	const auto& locked = getLockedOptions();
	addOptionsCheckbox(_("Lock Teams"), locked.teams, false, [](WzAdvCheckbox& button){
		auto newLocked = getLockedOptions();
		newLocked.teams = button.isChecked();
		updateLockedOptionsOnHost(newLocked);
	});
	addOptionsCheckbox(_("Lock Position"), locked.position, false, [](WzAdvCheckbox& button){
		auto newLocked = getLockedOptions();
		newLocked.position = button.isChecked();
		updateLockedOptionsOnHost(newLocked);
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

