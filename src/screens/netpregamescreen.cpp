/*
	This file is part of Warzone 2100.
	Copyright (C) 2023  Warzone 2100 Project

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

#include <string.h>

#include "lib/framework/frame.h"
#include "lib/framework/math_ext.h"
#include "netpregamescreen.h"
#include "lib/widget/widgint.h"
#include "lib/widget/label.h"
#include "lib/widget/scrollablelist.h"
#include "lib/widget/button.h"
#include "lib/ivis_opengl/pieblitfunc.h"
#include "lib/ivis_opengl/piepalette.h"
#include "lib/netplay/netplay.h"

#include "../intimage.h"
#include "../display.h"
#include "../hci.h"
#include "../multiplay.h"
#include "../frontend.h"
#include "../multiint.h"
#include "../multivote.h"

#include "../hci/teamstrategy.h"

#include <array>
#include <chrono>
#include <deque>

enum class PregameWaitingState
{
	Uninitialized,
	WaitingForPlayersToJoin,
	WaitingForTeamPlanning,
	WaitingForFinalCountdown
};

class WzGameStartOverlayScreen_CLICKFORM;
struct WzGameStartOverlayScreen;

// MARK: - Globals

static std::weak_ptr<WzGameStartOverlayScreen> psCurrentGameStartScreen;

// MARK: - WzGameStartOverlayScreen definition

struct WzGameStartOverlayScreen: public W_SCREEN
{
protected:
	WzGameStartOverlayScreen(): W_SCREEN() {}

public:
	typedef std::function<void ()> OnCloseFunc;
	static std::shared_ptr<WzGameStartOverlayScreen> make(const OnCloseFunc& onCloseFunction);

protected:
	friend class WzGameStartOverlayScreen_CLICKFORM;
	void closeScreen();

private:
	OnCloseFunc onCloseFunc;

private:
	WzGameStartOverlayScreen(WzGameStartOverlayScreen const &) = delete;
	WzGameStartOverlayScreen &operator =(WzGameStartOverlayScreen const &) = delete;
};

// MARK: - WzLoadingPlayersStatusForm

class WzLoadingPlayersStatusForm : public W_FORM
{
public:
	WzLoadingPlayersStatusForm() : W_FORM() {}
	~WzLoadingPlayersStatusForm() { }

public:
	static std::shared_ptr<WzLoadingPlayersStatusForm> make();

public:
	virtual void display(int xOffset, int yOffset) override;
	virtual void geometryChanged() override;
	virtual void screenSizeDidChange(int oldWidth, int oldHeight, int newWidth, int newHeight) override;
	virtual int32_t idealWidth() override;
	virtual int32_t idealHeight() override;
public:
	std::shared_ptr<W_LABEL>					titleLabel;
	std::shared_ptr<ScrollableListWidget>		playersList;
	std::shared_ptr<ScrollableListWidget>		spectatorsList;
	UDWORD										lastUpdateTime = 0;
	UDWORD										autoStateRefreshInterval = GAME_TICKS_PER_SEC * 3;
	static constexpr int PLAYER_LOADING_STATUS_ITEM_SPACING = 10;
};

class WzPlayerStatusCheckboxButton : public W_BUTTON
{
	#define IndicatorSize 10
	#define IndicatorRightPadding 5
public:
	WzPlayerStatusCheckboxButton(uint32_t playerIdx, std::shared_ptr<WzText> checkmarkText)
	: W_BUTTON()
	, player(playerIdx)
	, checkmarkText(checkmarkText)
	{ }

	~WzPlayerStatusCheckboxButton()
	{
		closeHostOptionsOverlay();
	}

	void run(W_CONTEXT *) override
	{
		LoadingStatus newStatus = LoadingStatus::Loading;
		if (isHumanPlayer(player))
		{
			newStatus = ingame.JoiningInProgress[player] ? LoadingStatus::Loading : LoadingStatus::Ready; // Human player, still joining or joined.
			if (ingame.PendingDisconnect[player])
			{
				newStatus = LoadingStatus::Disconnected;
			}
		}
		else if (NetPlay.players[player].ai >= 0)
		{
			newStatus = LoadingStatus::Ready;  // AI player (automatically joined).
		}
		else
		{
			newStatus = LoadingStatus::Disconnected;
		}
		if (newStatus != status)
		{
			switch (newStatus)
			{
				case LoadingStatus::Loading:
					enableProgressIndicator();
					break;
				case LoadingStatus::Ready:
					setProgressBorder(nullopt);
					closeHostOptionsOverlay();
					break;
				case LoadingStatus::Disconnected:
					setProgressBorder(nullopt);
					closeHostOptionsOverlay();
					break;
			}
			status = newStatus;
		}
	}

	virtual void geometryChanged() override
	{
		if (status == LoadingStatus::Loading)
		{
			enableProgressIndicator();
		}
	}

	void setString(WzString string) override
	{
		W_BUTTON::setString(string);
		wzText.setText(string, FontID);
	}

	void display(int xOffset, int yOffset) override
	{
		int x0 = x() + xOffset;
		int y0 = y() + yOffset;

		bool isClickLock = (state & WBUT_CLICKLOCK) != 0;
		bool isDown = (state & (WBUT_DOWN | WBUT_LOCK | WBUT_CLICKLOCK)) != 0;
		bool isHighlight = (state & WBUT_HIGHLIGHT) != 0;

		if (NetPlay.isHost)
		{
			// hosts can click on loading players to get options - show hover state
			if (status == LoadingStatus::Loading && (isHighlight || isDown))
			{
				int boxX0 = std::max<int>(x0 - 1, 0);
				int boxX1 = boxX0 + width() + 2;
				int boxY0 = y0;
				pie_UniTransBoxFill(boxX0, boxY0, boxX1, boxY0 + height(), WZCOL_MENU_BACKGROUND);
				if (isDown)
				{
					pie_UniTransBoxFill(boxX0, boxY0, boxX1, boxY0 + height(), pal_RGBA(0,0,0,100));
				}

				PIELIGHT borderColor = (isHighlight || isClickLock) ? pal_RGBA(255, 255, 255, 180) : pal_RGBA(255, 255, 255, 75);
				iV_Box(boxX0 - 1, boxY0 - 1, boxX1 + 1, boxY0 + height() + 1, borderColor);
			}
		}

		wzText.setText(pText, FontID);

		int indicator_x1 = IndicatorSize + IndicatorRightPadding;

		int fx = x0 + indicator_x1;
		int fy = y0 + (height() - wzText.lineSize()) / 2 - wzText.aboveBase();

		PIELIGHT textColor = WZCOL_TEXT_MEDIUM;
		switch (status)
		{
			case LoadingStatus::Loading:
				textColor = WZCOL_TEXT_DARK;
				break;
			case LoadingStatus::Ready:
				textColor = WZCOL_TEXT_BRIGHT;
				break;
			case LoadingStatus::Disconnected:
				textColor = WZCOL_GREY;
				break;
		}

		if (status == LoadingStatus::Ready)
		{
			int checkmarkX0 = x0;
			int checkmarkY0 = y0 + (height() - checkmarkText->lineSize()) / 2 - checkmarkText->aboveBase();
			checkmarkText->render(checkmarkX0, checkmarkY0, pal_RGBA(0,255,0,255));
		}

		wzText.render(fx, fy, textColor);
	}

	int32_t idealWidth() override
	{
		return wzText.width() + IndicatorSize + IndicatorRightPadding;
	}

	int32_t idealHeight() override
	{
		return std::max<int>(iV_GetTextLineSize(FontID), IndicatorSize);
	}

protected:
	// override default button click handling
	void released(W_CONTEXT *context, WIDGET_KEY mouseButton) override
	{
		bool clickAndReleaseOnThisButton = ((state & WBUT_DOWN) != 0); // relies on W_BUTTON handling to properly set WBUT_DOWN

		W_BUTTON::released(context, mouseButton);

		if (!clickAndReleaseOnThisButton)
		{
			return; // do nothing
		}

		if (mouseButton == WKEY_PRIMARY)
		{
			if (NetPlay.isHost && status == LoadingStatus::Loading)
			{
				ASSERT(player != NetPlay.hostPlayer, "Shouldn't be possible for host player?");
				// Display a pop-up for managing loading players
				displayHostOptionsOverlay(shared_from_this());
				setState(WBUT_CLICKLOCK);
			}
		}
	}

private:
	void enableProgressIndicator()
	{
		int yPadding = (height() > IndicatorSize) ? height() - IndicatorSize : 0;
		setProgressBorder(W_BUTTON::ProgressBorder::indeterminate(W_BUTTON::ProgressBorder::BorderInset(0, yPadding / 2, width() - IndicatorSize, yPadding / 2)), WZCOL_TEXT_MEDIUM);
	}

	std::shared_ptr<WIDGET> createHostOptionsPopoverForm()
	{
		// create all the buttons / option rows
		std::weak_ptr<WzPlayerStatusCheckboxButton> weakPlayerStatusButton = std::dynamic_pointer_cast<WzPlayerStatusCheckboxButton>(shared_from_this());
		auto createOptionsButton = [weakPlayerStatusButton](const WzString& text, const std::function<void (W_BUTTON& button)>& onClickFunc) -> std::shared_ptr<W_BUTTON> {
			auto button = std::make_shared<W_BUTTON>();
			button->setString(text);
			button->FontID = font_regular;
			button->displayFunction = PopoverMenuButtonDisplayFunc;
			button->pUserData = new PopoverMenuButtonDisplayCache();
			button->setOnDelete([](WIDGET *psWidget) {
				assert(psWidget->pUserData != nullptr);
				delete static_cast<PopoverMenuButtonDisplayCache *>(psWidget->pUserData);
				psWidget->pUserData = nullptr;
			});
			int minButtonWidthForText = iV_GetTextWidth(text, button->FontID);
			button->setGeometry(0, 0, minButtonWidthForText + 10, iV_GetTextLineSize(button->FontID) + 2);

			// On click, perform the designated onClickHandler and close the popover form / overlay
			button->addOnClickHandler([weakPlayerStatusButton, onClickFunc](W_BUTTON& button) {
				if (auto strongParent = weakPlayerStatusButton.lock())
				{
					widgRemoveOverlayScreen(strongParent->optionsOverlayScreen);
					onClickFunc(button);
					strongParent->optionsOverlayScreen.reset();
				}
			});
			return button;
		};

		std::vector<std::shared_ptr<WIDGET>> buttons;
		WzString kickButtonStr = (NetPlay.players[player].isSpectator ? _("Kick Spectator") : _("Kick Player"));
		buttons.push_back(createOptionsButton(kickButtonStr, [weakPlayerStatusButton](W_BUTTON& button){
			if (auto strongParent = weakPlayerStatusButton.lock())
			{
				auto targetPlayer = strongParent->player;
				if (isHumanPlayer(targetPlayer) && targetPlayer != NetPlay.hostPlayer && strongParent->status == LoadingStatus::Loading)
				{
					uint32_t numHumanPlayers = 0;
					for (uint32_t i = 0; i < MAX_PLAYERS; i++)
					{
						if (isHumanPlayer(i))
						{
							++numHumanPlayers;
						}
					}
					if (NetPlay.players[targetPlayer].isSpectator || (numHumanPlayers <= 2 && !NetPlay.players[NetPlay.hostPlayer].isSpectator))
					{
						kickPlayer(targetPlayer, _("The host has kicked you from the game."), ERROR_KICKED, false);
					}
					else
					{
						startKickVote(targetPlayer);
					}
				}
			}
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
		itemsList->setBorderColor(pal_RGBA(152, 151, 154, 255));
		Padding padding = {3, 4, 3, 4};
		itemsList->setPadding(padding);
		const int itemSpacing = 4;
		itemsList->setItemSpacing(itemSpacing);
		itemsList->setGeometry(itemsList->x(), itemsList->y(), maxButtonWidth + padding.left + padding.right, totalButtonHeight + ((static_cast<int>(buttons.size()) - 1) * itemSpacing) + padding.top + padding.bottom);
		for (auto& button : buttons)
		{
			itemsList->addItem(button);
		}

		return itemsList;
	}

	void displayHostOptionsOverlay(const std::shared_ptr<WIDGET>& psParent)
	{
		auto lockedScreen = screenPointer.lock();
		ASSERT(lockedScreen != nullptr, "No associated screen pointer?");

		// Initialize the options overlay screen
		optionsOverlayScreen = W_SCREEN::make();
		auto newRootFrm = W_FULLSCREENOVERLAY_CLICKFORM::make();
		newRootFrm->displayFunction = displayChildDropShadows;
		std::weak_ptr<W_SCREEN> psWeakOptionsOverlayScreen(optionsOverlayScreen);
		std::weak_ptr<WzPlayerStatusCheckboxButton> psWeakPlayerStatusButton = std::dynamic_pointer_cast<WzPlayerStatusCheckboxButton>(shared_from_this());
		newRootFrm->onClickedFunc = [psWeakOptionsOverlayScreen, psWeakPlayerStatusButton]() {
			if (auto psOverlayScreen = psWeakOptionsOverlayScreen.lock())
			{
				widgRemoveOverlayScreen(psOverlayScreen);
			}
			// Destroy Options overlay / overlay screen, reset button state
			if (auto strongParent = psWeakPlayerStatusButton.lock())
			{
				strongParent->optionsOverlayScreen.reset();
				strongParent->setState(0);
			}
		};
		newRootFrm->onCancelPressed = newRootFrm->onClickedFunc;
		optionsOverlayScreen->psForm->attach(newRootFrm);

		// Create the pop-over form
		auto optionsPopOver = createHostOptionsPopoverForm();
		newRootFrm->attach(optionsPopOver);

		// Position the pop-over form
		std::weak_ptr<WIDGET> weakParent = psParent;
		optionsPopOver->setCalcLayout([weakParent](WIDGET *psWidget) {
			auto psParent = weakParent.lock();
			ASSERT_OR_RETURN(, psParent != nullptr, "parent is null");
			// (Ideally, with its left aligned with the right side of the "parent" widget, but ensure full visibility on-screen)
			int popOverX0 = psParent->screenPosX() + psParent->width() + 2;
			if (popOverX0 + psWidget->width() > screenWidth)
			{
				popOverX0 = screenWidth - psWidget->width();
			}
			// (Ideally, with its top aligned with the top of the "parent" widget, but ensure full visibility on-screen)
			int popOverY0 = psParent->screenPosY() - 4;
			if (popOverY0 + psWidget->height() > screenHeight)
			{
				popOverY0 = screenHeight - psWidget->height();
			}
			psWidget->move(popOverX0, popOverY0);
		});

		widgRegisterOverlayScreenOnTopOfScreen(optionsOverlayScreen, lockedScreen);
	}

	void closeHostOptionsOverlay()
	{
		if (optionsOverlayScreen)
		{
			widgRemoveOverlayScreen(optionsOverlayScreen);
			optionsOverlayScreen.reset();
			state &= ~WBUT_CLICKLOCK;
		}
	}

private:
	enum class LoadingStatus
	{
		Loading,
		Ready,
		Disconnected
	};
	LoadingStatus status = LoadingStatus::Loading;
	uint32_t player;
	WzText wzText;
	std::shared_ptr<WzText> checkmarkText;
	std::shared_ptr<W_SCREEN> optionsOverlayScreen;
};

std::shared_ptr<WzLoadingPlayersStatusForm> WzLoadingPlayersStatusForm::make()
{
	auto result = std::make_shared<WzLoadingPlayersStatusForm>();

	// Add title text
	result->titleLabel = std::make_shared<W_LABEL>();
	result->titleLabel->setFont(font_regular_bold, WZCOL_FORM_TEXT);
	result->titleLabel->setString(WzString::fromUtf8(_("Loading Status:")));
	result->titleLabel->setGeometry(0, 0, result->titleLabel->getMaxLineWidth(), iV_GetTextLineSize(font_regular_bold));
	result->titleLabel->setCacheNeverExpires(true);
	result->attach(result->titleLabel);
	result->titleLabel->setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
		auto psParent = std::dynamic_pointer_cast<WzLoadingPlayersStatusForm>(psWidget->parent());
		ASSERT_OR_RETURN(, psParent != nullptr, "No parent");
		psWidget->setGeometry(0, 0, psWidget->width(), psWidget->height());
	}));

	// add table for players
	result->playersList = ScrollableListWidget::make();
	result->attach(result->playersList);
	result->playersList->setBackgroundColor(pal_RGBA(0, 0, 0, 25));
	result->playersList->setItemSpacing(1);

	auto checkmarkText = std::make_shared<WzText>();
	checkmarkText->setText("✓", font_regular);

	// Add rows for all players
	std::vector<uint32_t> playerIndexes;
	for (uint32_t player = 0; player < std::min<uint32_t>(game.maxPlayers, MAX_PLAYERS); ++player)
	{
		if (isHumanPlayer(player) // is an active (connected) human player
					|| NetPlay.players[player].difficulty == AIDifficulty::HUMAN // was a human player (probably disconnected)
					|| NetPlay.players[player].ai >= 0) // is an AI bot
		{
			playerIndexes.push_back(player);
		}
	}
	// sort by player position
	std::sort(playerIndexes.begin(), playerIndexes.end(), [](uint32_t playerA, uint32_t playerB) -> bool {
		return NetPlay.players[playerA].position < NetPlay.players[playerB].position;
	});
	for (auto player : playerIndexes)
	{
		auto pPlayerCheckbox = std::make_shared<WzPlayerStatusCheckboxButton>(player, checkmarkText);
		pPlayerCheckbox->FontID = font_regular;
		pPlayerCheckbox->setString(getPlayerName(player));
		pPlayerCheckbox->setGeometry(0, 0, pPlayerCheckbox->idealWidth(), pPlayerCheckbox->idealHeight());
		result->playersList->addItem(pPlayerCheckbox);
	}

	// add table for spectators
	result->spectatorsList = ScrollableListWidget::make();
	result->spectatorsList->setBackgroundColor(pal_RGBA(0, 0, 0, 25));
	result->spectatorsList->setItemSpacing(1);

	// Add rows for all spectators
	uint32_t numSpectators = 0;
	for (uint32_t player = MAX_PLAYERS; player < MAX_CONNECTED_PLAYERS; ++player)
	{
		if (isHumanPlayer(player) // is an active (connected) human player
					|| NetPlay.players[player].difficulty == AIDifficulty::HUMAN // was a human player (probably disconnected)
			)
		{
			auto pPlayerCheckbox = std::make_shared<WzPlayerStatusCheckboxButton>(player, checkmarkText);
			pPlayerCheckbox->FontID = font_regular;
			pPlayerCheckbox->setString(getPlayerName(player));
			pPlayerCheckbox->setGeometry(0, 0, pPlayerCheckbox->idealWidth(), pPlayerCheckbox->idealHeight());
			result->spectatorsList->addItem(pPlayerCheckbox);
			numSpectators++;
		}
	}

	if (numSpectators)
	{
		result->attach(result->spectatorsList);
	}
	else
	{
		result->spectatorsList.reset();
	}

	result->playersList->setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
		auto psParent = std::dynamic_pointer_cast<WzLoadingPlayersStatusForm>(psWidget->parent());
		ASSERT_OR_RETURN(, psParent != nullptr, "No parent");
		int x0 = 0;
		int y0 = psParent->titleLabel->y() + psParent->titleLabel->height() + PLAYER_LOADING_STATUS_ITEM_SPACING;
		int height = psParent->height() - y0;
		psWidget->setGeometry(x0, y0, psWidget->idealWidth(), height);
	}));

	if (result->spectatorsList)
	{
		result->spectatorsList->setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
			auto psParent = std::dynamic_pointer_cast<WzLoadingPlayersStatusForm>(psWidget->parent());
			ASSERT_OR_RETURN(, psParent != nullptr, "No parent");
			int x0 = psParent->playersList->x() + psParent->playersList->width() + PLAYER_LOADING_STATUS_ITEM_SPACING;
			int y0 = psParent->titleLabel->y() + psParent->titleLabel->height() + PLAYER_LOADING_STATUS_ITEM_SPACING;
			int height = psParent->height() - y0;
			psWidget->setGeometry(x0, y0, psWidget->idealWidth(), height);
		}));
	}

	return result;
}

void WzLoadingPlayersStatusForm::display(int xOffset, int yOffset)
{
	// currently, no-op
}

void WzLoadingPlayersStatusForm::geometryChanged()
{
	if (titleLabel)
	{
		titleLabel->callCalcLayout();
	}
	if (playersList)
	{
		playersList->callCalcLayout();
	}
	if (spectatorsList)
	{
		spectatorsList->callCalcLayout();
	}
}

int32_t WzLoadingPlayersStatusForm::idealWidth()
{
	int32_t listsWidth = playersList->idealWidth();
	if (spectatorsList)
	{
		listsWidth += PLAYER_LOADING_STATUS_ITEM_SPACING + spectatorsList->idealWidth();
	}
	return std::max(titleLabel->idealWidth(), listsWidth);
}

int32_t WzLoadingPlayersStatusForm::idealHeight()
{
	return titleLabel->y() + titleLabel->height() + PLAYER_LOADING_STATUS_ITEM_SPACING + std::max(playersList->idealHeight(), (spectatorsList) ? spectatorsList->idealHeight() : 0);
}

void WzLoadingPlayersStatusForm::screenSizeDidChange(int oldWidth, int oldHeight, int newWidth, int newHeight)
{
	// call default implementation (which ultimately propagates to all children
	W_FORM::screenSizeDidChange(oldWidth, oldHeight, newWidth, newHeight);
}

// MARK: - WzCountdownLabel

class WzCountdownLabel : public WIDGET
{
public:
	virtual void display(int xOffset, int yOffset) override;
	virtual int32_t idealWidth() override;
	virtual int32_t idealHeight() override;
public:
	void setCountdown(const std::chrono::steady_clock::time_point startTime, std::chrono::seconds::rep countdownSeconds);
	void clearCountdown() { startTime = nullopt; }
	void setTextAlignment(WzTextAlignment align);
public:
	optional<std::chrono::steady_clock::time_point> startTime;
	std::chrono::seconds::rep countdownSeconds = 0;
	int32_t maxWidth = 0;
	iV_fonts FontID = font_large;
	PIELIGHT fontColour = pal_RGBA(255, 255, 255, 255);
	WzText displayedNumber;
};


void WzCountdownLabel::setCountdown(const std::chrono::steady_clock::time_point _startTime, std::chrono::seconds::rep _countdownSeconds)
{
	startTime = _startTime;
	countdownSeconds = _countdownSeconds;
	maxWidth = iV_GetTextWidth(WzString::number(countdownSeconds), font_large); // not accurate for all numbers, but hopefully in the ballpark...
	dirty = true;
}

void WzCountdownLabel::setTextAlignment(WzTextAlignment align)
{
	style &= ~(WLAB_ALIGNLEFT | WLAB_ALIGNCENTRE | WLAB_ALIGNRIGHT);
	style |= align;
	dirty = true;
}

void WzCountdownLabel::display(int xOffset, int yOffset)
{
	if (!startTime.has_value())
	{
		return;
	}
	auto currentTime = std::chrono::steady_clock::now();
	auto secondsLeft = countdownSeconds - std::chrono::duration_cast<std::chrono::seconds>(currentTime - startTime.value()).count();

	if (secondsLeft == 0)
	{
		return;
	}

	displayedNumber.setText(WzString::number(secondsLeft), FontID);

	int textTotalHeight = displayedNumber.lineSize();

	Vector2i textBoundingBoxOffset(0, 0);
	if ((style & WLAB_ALIGNTOPLEFT) != 0)  // Align top
	{
		textBoundingBoxOffset.y = yOffset + y();
	}
	else if ((style & WLAB_ALIGNBOTTOMLEFT) != 0)  // Align bottom
	{
		textBoundingBoxOffset.y = yOffset + y() + (height() - textTotalHeight);
	}
	else
	{
		textBoundingBoxOffset.y = yOffset + y() + (height() - textTotalHeight) / 2;
	}

	int fx = 0;
	if (style & WLAB_ALIGNCENTRE)
	{
		int fw = displayedNumber.width();
		fx = xOffset + x() + (width() - fw) / 2;
	}
	else if (style & WLAB_ALIGNRIGHT)
	{
		int fw = displayedNumber.width();
		fx = xOffset + x() + width() - fw;
	}
	else
	{
		fx = xOffset + x();
	}

	float fy = float(textBoundingBoxOffset.y) - float(displayedNumber.aboveBase());

	PIELIGHT textColour = fontColour;
	if (secondsLeft <= 3)
	{
		auto fractionOfWholeSecond = (std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - startTime.value()).count() % 1000);
		if (fractionOfWholeSecond != 0)
		{
			textColour.byte.a -= static_cast<uint8_t>(static_cast<float>(textColour.byte.a) * std::min<float>((fractionOfWholeSecond / 1000.f), 1.0f));
		}
	}

	displayedNumber.render(textBoundingBoxOffset.x + fx, fy, textColour);
}

int32_t WzCountdownLabel::idealWidth()
{
	return maxWidth;
}

int32_t WzCountdownLabel::idealHeight()
{
	return iV_GetTextLineSize(FontID);
}

// MARK: - WzGameStartOverlayScreen_CLICKFORM

#define WZ_TEAMPLANNING_COUNTDOWN_MS 20000
#define WZ_FINAL_COUNTDOWN_MS 3000

class WzGameStartOverlayScreen_CLICKFORM : public W_CLICKFORM
{
protected:
	static constexpr int32_t INTERNAL_PADDING = 15;
protected:
	WzGameStartOverlayScreen_CLICKFORM(W_FORMINIT const *init);
	WzGameStartOverlayScreen_CLICKFORM();
	~WzGameStartOverlayScreen_CLICKFORM();
	void initialize();
	void recalcLayout();
public:
	static std::shared_ptr<WzGameStartOverlayScreen_CLICKFORM> make(UDWORD formID = 0);
	void clicked(W_CONTEXT *psContext, WIDGET_KEY key) override;
	void display(int xOffset, int yOffset) override;
	void run(W_CONTEXT *psContext) override;
	void geometryChanged() override
	{
		recalcLayout();
	}

private:
	void updateData();
	void transitionToNextState();

public:
	PIELIGHT backgroundColor = pal_RGBA(0, 0, 0, 200);
	std::function<void ()> onClickedFunc;
	std::function<void ()> onCancelPressed;
private:
	std::shared_ptr<W_LABEL> topLabel;
	std::shared_ptr<WzCountdownLabel> countdownLabel;
	std::shared_ptr<CONSOLE_MESSAGE_LISTENER> handleConsoleMessage;
	std::shared_ptr<WIDGET> teamStrategyView;
	std::shared_ptr<W_LABEL> strategySideTextLabel;
	std::shared_ptr<WzLoadingPlayersStatusForm> playersLoadingStatus;
	optional<std::chrono::steady_clock::time_point> currentCountdownStart;
	bool bHasTeamPlanningStage = false;
	PregameWaitingState currentState = PregameWaitingState::Uninitialized;
private:
	struct CachedConsoleMessage
	{
		WzText display;
		std::chrono::steady_clock::time_point timeAdded;
		std::chrono::milliseconds duration;
		CONSOLE_TEXT_JUSTIFICATION JustifyType;	// text justification
		int		player;			// Player who sent this message or SYSTEM_MESSAGE
		CachedConsoleMessage(const std::string &text, iV_fonts fontID, std::chrono::steady_clock::time_point time, std::chrono::milliseconds duration, CONSOLE_TEXT_JUSTIFICATION justify, int plr)
						: display(WzString::fromUtf8(text), fontID), timeAdded(time), duration(duration), JustifyType(justify), player(plr) {}

		CachedConsoleMessage& operator =(CachedConsoleMessage&& input) noexcept
		{
			display = std::move(input.display);
			timeAdded = input.timeAdded;
			duration = input.duration;
			JustifyType = input.JustifyType;
			player = input.player;
			return *this;
		}
	};
	std::deque<CachedConsoleMessage> currentConsoleMessages;
};

WzGameStartOverlayScreen_CLICKFORM::WzGameStartOverlayScreen_CLICKFORM(W_FORMINIT const *init) : W_CLICKFORM(init) {}
WzGameStartOverlayScreen_CLICKFORM::WzGameStartOverlayScreen_CLICKFORM() : W_CLICKFORM() {}
WzGameStartOverlayScreen_CLICKFORM::~WzGameStartOverlayScreen_CLICKFORM()
{
	consoleRemoveMessageListener(handleConsoleMessage);
}

std::shared_ptr<WzGameStartOverlayScreen_CLICKFORM> WzGameStartOverlayScreen_CLICKFORM::make(UDWORD formID)
{
	W_FORMINIT sInit;
	sInit.id = formID;
	sInit.style = WFORM_PLAIN | WFORM_CLICKABLE;
	sInit.x = 0;
	sInit.y = 0;
	sInit.width = screenWidth - 1;
	sInit.height = screenHeight - 1;
	sInit.calcLayout = LAMBDA_CALCLAYOUT_SIMPLE({
		psWidget->setGeometry(0, 0, screenWidth, screenHeight);
	});

	class make_shared_enabler: public WzGameStartOverlayScreen_CLICKFORM
	{
	public:
		make_shared_enabler(W_FORMINIT const *init): WzGameStartOverlayScreen_CLICKFORM(init) {}
	};
	auto widget = std::make_shared<make_shared_enabler>(&sInit);
	widget->initialize();
	return widget;
}

void WzGameStartOverlayScreen_CLICKFORM::initialize()
{
	// Create topLabel (bold)
	topLabel = std::make_shared<W_LABEL>();
	topLabel->setFont(font_medium_bold, WZCOL_TEXT_BRIGHT);
	topLabel->setTextAlignment(WLAB_ALIGNCENTRE);
	attach(topLabel);

	// Create countdownLabel (bold)
	countdownLabel = std::make_shared<WzCountdownLabel>();
	countdownLabel->setTextAlignment(WLAB_ALIGNRIGHT);
	countdownLabel->clearCountdown();
	attach(countdownLabel);

	// Create a console message listener
	handleConsoleMessage = std::make_shared<CONSOLE_MESSAGE_LISTENER>([&](ConsoleMessage const &message) -> void
	{
		std::chrono::steady_clock::time_point addedTime = std::chrono::steady_clock::now();
		uint32_t messageDuration = std::min<uint32_t>((message.duration > 0) ? message.duration : 5000, 5000);
		currentConsoleMessages.emplace_back(message.text, font_regular, addedTime, std::chrono::milliseconds(messageDuration), message.justification, message.sender);
		if (currentConsoleMessages.size() > 5)
		{
			currentConsoleMessages.pop_front();
		}
	});

	// Determine if any players will have a team to share strategy with
	bHasTeamPlanningStage = gameHasTeamStrategyView();

	// Create a team strategy view (if player is on a team) - may return null if there's no reason to display it
	teamStrategyView = createTeamStrategyView();
	if (teamStrategyView)
	{
		// Add side label
		strategySideTextLabel = addSideText(this, 0, 0, 0, _("TEAM STRATEGY"));

		attach(teamStrategyView);
	}

	// Create a PlayerConnectionStatusForm
	playersLoadingStatus = WzLoadingPlayersStatusForm::make();
	attach(playersLoadingStatus);

	recalcLayout();

	consoleAddMessageListener(handleConsoleMessage);
}

void WzGameStartOverlayScreen_CLICKFORM::recalcLayout()
{
	int maxChildWidth = width() - (INTERNAL_PADDING * 2);

	auto topLabelWidth = maxChildWidth;
	topLabel->setGeometry(INTERNAL_PADDING, INTERNAL_PADDING + 8, topLabelWidth, topLabel->requiredHeight());

	int countdownLabelWidth = 30;
	countdownLabel->setGeometry(width() - (INTERNAL_PADDING + countdownLabelWidth), INTERNAL_PADDING, countdownLabelWidth, countdownLabel->idealHeight());

	int bottomOfLastElement = topLabel->y() + topLabel->height();

	// Bottom-aligned widget (player loading status)
	std::shared_ptr<WIDGET> bottomWidget = playersLoadingStatus;
	int bottomWidgetWidth = bottomWidget->idealWidth();
	int bottomWidgetHeight = bottomWidget->idealHeight();
	bottomWidget->setGeometry((width() - bottomWidgetWidth) / 2, height() - (bottomWidgetHeight + INTERNAL_PADDING), bottomWidgetWidth, bottomWidgetHeight);

	if (teamStrategyView)
	{
		// place the team strategy information in the middle, below the topLabel
		int teamStrategyTopPadding = INTERNAL_PADDING * 2;
		int teamStrategyViewWidth = std::min(maxChildWidth - strategySideTextLabel->requiredHeight(), teamStrategyView->idealWidth());
		int maxAvailableHeightForTeamStrategyView = height() - (bottomWidget->visible() ? bottomWidgetHeight : 0) - (teamStrategyTopPadding * 2);
		int teamStrategyViewHeight = std::min(maxAvailableHeightForTeamStrategyView, teamStrategyView->idealHeight());
		int teamStrategyX0 = (width() - teamStrategyViewWidth) / 2;
		int teamStrategyY0 = bottomOfLastElement + teamStrategyTopPadding;
		teamStrategyView->setGeometry(teamStrategyX0, teamStrategyY0, teamStrategyViewWidth, teamStrategyViewHeight);

		// place the side text... to the side
		int strategySideTextX0 = std::max(teamStrategyX0 - strategySideTextLabel->requiredHeight(), 0);
		if (strategySideTextX0 >= strategySideTextLabel->requiredHeight())
		{
			strategySideTextLabel->setGeometry(strategySideTextX0, teamStrategyY0, strategySideTextLabel->width(), strategySideTextLabel->height());
			strategySideTextLabel->show();
		}
		else
		{
			strategySideTextLabel->hide();
		}
	}
}

void WzGameStartOverlayScreen_CLICKFORM::clicked(W_CONTEXT *psContext, WIDGET_KEY key)
{
	if (onClickedFunc)
	{
		onClickedFunc();
	}
}

static void pregamscreen_draw_console_text(WzText &display, PIELIGHT colour, int x, int y, CONSOLE_TEXT_JUSTIFICATION justify, int width)
{
	switch (justify)
	{
	case LEFT_JUSTIFY:
		break; // do nothing
	case RIGHT_JUSTIFY:
		x = x + width - display.width();
		break;
	case CENTRE_JUSTIFY:
		x = x + (width - display.width()) / 2;
		break;
	}
	display.render(x, y, colour);
}

void WzGameStartOverlayScreen_CLICKFORM::display(int xOffset, int yOffset)
{
	if (!visible())
	{
		return; // skip if hidden
	}

	if (backgroundColor.isTransparent())
	{
		return;
	}

	int x0 = x() + xOffset;
	int y0 = y() + yOffset;

	// draw background over everything
	pie_UniTransBoxFill(x0, y0, x0 + width(), y0 + height(), backgroundColor);

	// draw console messages, if present
	int TextX0 = x0 + INTERNAL_PADDING;
	int TextYpos = y0 + topLabel->y() + topLabel->height() + INTERNAL_PADDING;
	int TextMaxWidth = width() - (INTERNAL_PADDING * 2);
	for (auto& ActiveMessage : currentConsoleMessages)
	{
		PIELIGHT textColor = getConsoleTextColor(ActiveMessage.player);
		textColor.byte.a = textColor.byte.a / 2;
		pregamscreen_draw_console_text(ActiveMessage.display, textColor, TextX0,
						 TextYpos, ActiveMessage.JustifyType, TextMaxWidth);
		TextYpos += ActiveMessage.display.lineSize();
	}
}

void WzGameStartOverlayScreen_CLICKFORM::run(W_CONTEXT *psContext)
{
	if (keyPressed(KEY_ESC))
	{
		if (onCancelPressed)
		{
			onCancelPressed();
		}
	}
	inputLoseFocus();	// clear the input buffer.

	updateData();
}

void WzGameStartOverlayScreen_CLICKFORM::transitionToNextState()
{
	auto transitionToFinalCountdown = [&]() {
		currentState = PregameWaitingState::WaitingForFinalCountdown;
		topLabel->setString(_("Game will start in ..."));
		currentCountdownStart = std::chrono::steady_clock::now();
		countdownLabel->setCountdown(currentCountdownStart.value(), WZ_FINAL_COUNTDOWN_MS / 1000);
		recalcLayout();
	};

	switch (currentState)
	{
		case PregameWaitingState::Uninitialized:
			currentState = PregameWaitingState::WaitingForPlayersToJoin;
			topLabel->setString(_("Waiting for other players ..."));
			recalcLayout();
			break;
		case PregameWaitingState::WaitingForPlayersToJoin:
			if (bHasTeamPlanningStage)
			{
				currentState = PregameWaitingState::WaitingForTeamPlanning;
				topLabel->setString(_("Team Planning"));
				currentCountdownStart = std::chrono::steady_clock::now();
				countdownLabel->setCountdown(currentCountdownStart.value(), WZ_TEAMPLANNING_COUNTDOWN_MS / 1000);
				if (teamStrategyView)
				{
					transformTeamStrategyViewMode(teamStrategyView, true);

					// if needed, hide playersLoadingStatus so the team planning can use the full height
					if (true)
					{
						playersLoadingStatus->hide();
					}
				}

				recalcLayout();
			}
			else
			{
				transitionToFinalCountdown();
			}
			break;
		case PregameWaitingState::WaitingForTeamPlanning:
			transitionToFinalCountdown();
			break;
		case PregameWaitingState::WaitingForFinalCountdown:
			break;
	}
}

void WzGameStartOverlayScreen_CLICKFORM::updateData()
{
	// Prune expired console messages
	auto now = std::chrono::steady_clock::now();
	for (auto i = currentConsoleMessages.begin(); i != currentConsoleMessages.end();)
	{
		if ((std::chrono::duration_cast<std::chrono::milliseconds>(now - i->timeAdded) > i->duration))
		{
			i = currentConsoleMessages.erase(i);
		}
		else
		{
			++i;
		}
	}

	// Handle game start state
	if (currentState == PregameWaitingState::Uninitialized)
	{
		transitionToNextState();
	}
	if (currentState == PregameWaitingState::WaitingForPlayersToJoin)
	{
		size_t playersWaitingToJoin = 0;
		for (int i = 0; i < MAX_CONNECTED_PLAYERS; i++)
		{
			if (isHumanPlayer(i))
			{
				if (ingame.JoiningInProgress[i])
				{
					playersWaitingToJoin++;
				}
			}
		}
		// if all players have joined, transition to the next state
		if (playersWaitingToJoin == 0)
		{
			transitionToNextState();
		}
	}
	if (currentState == PregameWaitingState::WaitingForTeamPlanning)
	{
		if (currentCountdownStart.has_value())
		{
			auto currentTime = std::chrono::steady_clock::now();
			if (std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - currentCountdownStart.value()).count() >= WZ_TEAMPLANNING_COUNTDOWN_MS)
			{
				transitionToNextState();
			}
		}
		else
		{
			currentCountdownStart = std::chrono::steady_clock::now();
		}
	}
	if (currentState == PregameWaitingState::WaitingForFinalCountdown)
	{
		if (currentCountdownStart.has_value())
		{
			auto currentTime = std::chrono::steady_clock::now();
			if (std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - currentCountdownStart.value()).count() >= WZ_FINAL_COUNTDOWN_MS)
			{
				if (auto strongScreen = screenPointer.lock())
				{
					auto gameStartOverlayScreen = std::dynamic_pointer_cast<WzGameStartOverlayScreen>(strongScreen);
					ASSERT(gameStartOverlayScreen != nullptr, "Invalid gameStartOverlayScreen");
					gameStartOverlayScreen->closeScreen();
				}
				else
				{
					ASSERT(false, "No screen is currently associated with this root form?");
				}
			}
		}
		else
		{
			currentCountdownStart = std::chrono::steady_clock::now();
		}
	}
}

// MARK: - WzGameStartOverlayScreen

std::shared_ptr<WzGameStartOverlayScreen> WzGameStartOverlayScreen::make(const OnCloseFunc& _onCloseFunc)
{
	class make_shared_enabler: public WzGameStartOverlayScreen {};
	auto newRootFrm = WzGameStartOverlayScreen_CLICKFORM::make();
	auto screen = std::make_shared<make_shared_enabler>();
	screen->initialize(newRootFrm);
	screen->onCloseFunc = _onCloseFunc;

	return screen;
}

void WzGameStartOverlayScreen::closeScreen()
{
	shutdownGameStartScreen();
	onCloseFunc();
}

std::shared_ptr<W_SCREEN> createGameStartScreen(std::function<void ()> onCloseFunc)
{
	auto screen = WzGameStartOverlayScreen::make(onCloseFunc);
	widgRegisterOverlayScreenOnTopOfScreen(screen, psWScreen);
	psCurrentGameStartScreen = screen;
	return screen;
}

void shutdownGameStartScreen()
{
	if (auto strongGameStartScreen = psCurrentGameStartScreen.lock())
	{
		widgRemoveOverlayScreen(strongGameStartScreen);
		psCurrentGameStartScreen.reset();
	}
}
