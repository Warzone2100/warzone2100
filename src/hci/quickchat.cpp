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
#include "lib/framework/wztime.h"
#include "lib/sound/audio.h"
#include "lib/sound/audio_id.h"
#include "quickchat.h"
#include "teamstrategy.h"
#include "lib/widget/widgint.h"
#include "lib/widget/label.h"
#include "lib/widget/margin.h"
#include "lib/widget/scrollablelist.h"
#include "lib/widget/button.h"
#include "lib/widget/multibutform.h"
#include "lib/widget/paneltabbutton.h"
#include "lib/ivis_opengl/pieblitfunc.h"
#include "lib/ivis_opengl/piepalette.h"
#include "lib/netplay/netplay.h"

#include "../ai.h"
#include "../intimage.h"
#include "../display.h"
#include "../hci.h"
#include "../multiplay.h"
#include "../notifications.h"
#include "../stats.h"
#include "../qtscript.h"
#include "../main.h"
#include "../faction.h"

#include <array>
#include <chrono>
#include <deque>


// MARK: - Globals

static WzQuickChatTargeting defaultTeamChatTargeting;
static WzQuickChatTargeting lastTeamChatTargeting;
static std::array<std::deque<std::chrono::steady_clock::time_point>, MAX_CONNECTED_PLAYERS> lastQuickChatMessageTimes;

// MARK: - Message throttling

constexpr size_t DEFAULT_MAX_MESSAGES_IN_INTERVAL = 8;
constexpr size_t MAX_RECORDED_MESSAGE_TIMES = 200;
constexpr std::chrono::milliseconds MESSAGE_INTERVAL(5000);
constexpr std::chrono::milliseconds MESSAGE_THROTTLE_TIMEOUT_INTERVAL(5000);

optional<std::chrono::steady_clock::time_point> playerSpamMutedUntil(uint32_t playerIdx)
{
	if (playerIdx >= lastQuickChatMessageTimes.size())
	{
		return nullopt;
	}

	if (lastQuickChatMessageTimes[playerIdx].empty())
	{
		return nullopt;
	}

	size_t maxMessageInLastInterval = DEFAULT_MAX_MESSAGES_IN_INTERVAL;
	std::chrono::steady_clock::time_point currentTime = std::chrono::steady_clock::now();
	std::chrono::steady_clock::time_point newestRecordedMessageTime = lastQuickChatMessageTimes[playerIdx].back();
	size_t numMessagesInLastInterval = 0;
	for (auto it = lastQuickChatMessageTimes[playerIdx].rbegin(); it != lastQuickChatMessageTimes[playerIdx].rend(); ++it)
	{
		if (std::chrono::duration_cast<std::chrono::seconds>(newestRecordedMessageTime - *it) <= MESSAGE_INTERVAL)
		{
			++numMessagesInLastInterval;
		}
		else
		{
			break;
		}
	}

	if (numMessagesInLastInterval >= maxMessageInLastInterval
		&& std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - newestRecordedMessageTime) < MESSAGE_THROTTLE_TIMEOUT_INTERVAL)
	{
		// throttle incoming messages
		return newestRecordedMessageTime + MESSAGE_THROTTLE_TIMEOUT_INTERVAL;
	}

	return nullopt;
}

void playerSpamMuteNotifyIndexSwap(uint32_t playerIndexA, uint32_t playerIndexB)
{
	ASSERT_OR_RETURN(, playerIndexA < lastQuickChatMessageTimes.size(), "playerIndexA invalid: %" PRIu32, playerIndexA);
	ASSERT_OR_RETURN(, playerIndexB < lastQuickChatMessageTimes.size(), "playerIndexB invalid: %" PRIu32, playerIndexB);
	std::swap(lastQuickChatMessageTimes[playerIndexA], lastQuickChatMessageTimes[playerIndexB]);
}

void playerSpamMuteReset(uint32_t playerIndex)
{
	ASSERT_OR_RETURN(, playerIndex < lastQuickChatMessageTimes.size(), "playerIndex invalid: %" PRIu32, playerIndex);
	lastQuickChatMessageTimes[playerIndex].clear();
}

void recordPlayerMessageSent(uint32_t playerIdx)
{
	if (playerSpamMutedUntil(playerIdx).has_value())
	{
		// ignore incoming message
		return;
	}
	lastQuickChatMessageTimes[playerIdx].push_back(std::chrono::steady_clock::now());
	while (lastQuickChatMessageTimes[playerIdx].size() > MAX_RECORDED_MESSAGE_TIMES)
	{
		lastQuickChatMessageTimes[playerIdx].pop_front();
	}
}

static bool isInternalMessage(WzQuickChatMessage msg)
{
	return static_cast<uint32_t>(msg) >= WzQuickChatMessage_FIRST_INTERNAL_MSG_VALUE;
}

// MARK: - Helper functions

std::string to_output_string(WzQuickChatMessage msg, const optional<WzQuickChatMessageData>& messageData);

static optional<size_t> numberButtonPressed()
{
	optional<size_t> keyPressForNumber;
	if (keyPressed(KEY_1) || keyPressed(KEY_KP_1))
	{
		keyPressForNumber = 0;
	}
	else if (keyPressed(KEY_2) || keyPressed(KEY_KP_2))
	{
		keyPressForNumber = 1;
	}
	else if (keyPressed(KEY_3) || keyPressed(KEY_KP_3))
	{
		keyPressForNumber = 2;
	}
	else if (keyPressed(KEY_4) || keyPressed(KEY_KP_4))
	{
		keyPressForNumber = 3;
	}
	else if (keyPressed(KEY_5) || keyPressed(KEY_KP_5))
	{
		keyPressForNumber = 4;
	}
	else if (keyPressed(KEY_6) || keyPressed(KEY_KP_6))
	{
		keyPressForNumber = 5;
	}
	else if (keyPressed(KEY_7) || keyPressed(KEY_KP_7))
	{
		keyPressForNumber = 6;
	}
	else if (keyPressed(KEY_8) || keyPressed(KEY_KP_8))
	{
		keyPressForNumber = 7;
	}
	else if (keyPressed(KEY_9) || keyPressed(KEY_KP_9))
	{
		keyPressForNumber = 8;
	}
	else if (keyPressed(KEY_0) || keyPressed(KEY_KP_0))
	{
		keyPressForNumber = 9;
	}
	return keyPressForNumber;
}

static std::vector<uint32_t> getPlayerTeammates(uint32_t playerIdx, bool includeAIs = false, bool includeAlliances = false)
{
	auto inputPlayerTeam = checkedGetPlayerTeam(playerIdx);
	std::vector<uint32_t> playersOnSameTeamAsInputPlayer;
	for (int32_t player = 0; player < std::min<int32_t>(game.maxPlayers, MAX_PLAYERS); ++player)
	{
		if (player == playerIdx)
		{
			continue;
		}
		if ((checkedGetPlayerTeam(player) != inputPlayerTeam) && (!includeAlliances || !aiCheckAlliances(playerIdx, player)))
		{
			continue;
		}
		if (isHumanPlayer(player) // is an active (connected) human player
			|| NetPlay.players[player].difficulty == AIDifficulty::HUMAN // was a human player (probably disconnected)
			|| (includeAIs && NetPlay.players[player].ai >= 0 && NetPlay.players[player].difficulty != AIDifficulty::DISABLED) // an AI
			)
		{
			playersOnSameTeamAsInputPlayer.push_back(static_cast<uint32_t>(player));
		}
	}

	return playersOnSameTeamAsInputPlayer;
}

// MARK: - Classes

const PIELIGHT WZCOL_QUICKCHAT_TABS_FILL_COLOR = pal_RGBA(25, 0, 110, 220);
const PIELIGHT WZCOL_QUICKCHAT_TABS_FILL_COLOR_DARK = pal_RGBA(10, 0, 70, 250);
const PIELIGHT WZCOL_QUICKCHAT_TABS_BORDER_LIGHT = pal_RGBA(255, 255, 255, 150);
const PIELIGHT WZCOL_QUICKCHAT_BUTTON_FILL_COLOR = pal_RGBA(0, 0, 67, 255);

constexpr int QuickChatButtonVerticalPadding = 5;
constexpr int QuickChatButtonHorizontalPadding = 5;

class WzQuickChatButton : public W_BUTTON
{
public:
	static std::shared_ptr<WzQuickChatButton> make(const WzString& messageText, bool disabled = false)
	{
		auto result = std::make_shared<WzQuickChatButton>();
		result->pText = messageText;
		return result;
	}
public:
	static int32_t getIdealBaseButtonHeight()
	{
		return iV_GetTextLineSize(font_regular) + (QuickChatButtonVerticalPadding * 2);
	}
public:
	// Display the text, shrunk to fit, with a faint border around it (that gets bright when highlighted)
	virtual void display(int xOffset, int yOffset) override
	{
		const std::shared_ptr<WIDGET> pParent = this->parent();
		ASSERT_OR_RETURN(, pParent != nullptr, "Keymap buttons should have a parent container!");

		int xPos = xOffset + x();
		int yPos = yOffset + y();
		int h = height();
		int w = width();

		bool isDisabled = (state & WBUT_DISABLE) != 0;
		bool isHighlight = (state & WBUT_HIGHLIGHT) != 0;

		// Draw background
		pie_BoxFill(xPos, yPos, xPos + w, yPos + h, WZCOL_QUICKCHAT_BUTTON_FILL_COLOR);
		PIELIGHT borderColor = (isHighlight && !isDisabled) ? pal_RGBA(255, 255, 255, 200) : pal_RGBA(255, 255, 255, 75);
		iV_Box(xPos + 1, yPos + 1, xPos + w, yPos + h, borderColor);

		if (isHighlight && !isDisabled)
		{
			pie_UniTransBoxFill(xPos, yPos, xPos + w, yPos + h, pal_RGBA(0, 0, 255, 70));
		}

		// Select label text and color
		PIELIGHT msgColour = (!isDisabled) ? WZCOL_FORM_TEXT : WZCOL_FORM_DISABLE;

		iV_fonts fontID = wzMessageText.getFontID();
		if (fontID == font_count || lastWidgetWidth != width() || wzMessageText.getText() != pText)
		{
			fontID = FontID;
		}
		wzMessageText.setText(pText, fontID);

		int availableButtonTextWidth = w - (QuickChatButtonHorizontalPadding * 2);
		if (wzMessageText.width() > availableButtonTextWidth)
		{
			// text would exceed the bounds of the button area
			// try to shrink font so it fits
			do {
				if (fontID == font_small || fontID == font_bar)
				{
					break;
				}
				auto result = iV_ShrinkFont(fontID);
				if (!result.has_value())
				{
					break;
				}
				fontID = result.value();
				wzMessageText.setText(pText, fontID);
			} while (wzMessageText.width() > availableButtonTextWidth);
		}
		lastWidgetWidth = width();

		int textX0 = xPos + QuickChatButtonHorizontalPadding;
		int textY0 = static_cast<int>(yPos + (h - wzMessageText.lineSize()) / 2 - float(wzMessageText.aboveBase()));

		int maxTextDisplayableWidth = w - (QuickChatButtonHorizontalPadding * 2);
		isTruncated = maxTextDisplayableWidth < wzMessageText.width();
		if (isTruncated)
		{
			maxTextDisplayableWidth -= (iV_GetEllipsisWidth(wzMessageText.getFontID()) + 2);
		}
		wzMessageText.render(textX0, textY0, msgColour, 0.0f, maxTextDisplayableWidth);

		if (isTruncated)
		{
			// Render ellipsis
			iV_DrawEllipsis(wzMessageText.getFontID(), Vector2f(textX0 + maxTextDisplayableWidth + 2, textY0), msgColour);
		}
	}

	std::string getTip() override
	{
		if (pTip.empty() && !isTruncated) {
			return "";
		}

		if (!isTruncated) {
			return pTip;
		}

		return pText.toUtf8();
	}

public:
	void simulateClick()
	{
		W_CONTEXT context = W_CONTEXT::ZeroContext();
		context.mx			= screenPosX();
		context.my			= screenPosY();

		clicked(&context, WKEY_PRIMARY);
		released(&context, WKEY_PRIMARY);
	}
private:
	WzText wzMessageText;
	int lastWidgetWidth = 0;
	bool isTruncated = false;
};

class WzQuickChatSection : public W_CLICKFORM
{
public:
	typedef std::function<bool (WzQuickChatMessage)> ShouldEnableOptionFunc;
private:
	void initialize(const WzString& sectionTitle);
public:
	static std::shared_ptr<WzQuickChatSection> make(const WzString& sectionTitle)
	{
		auto result = std::make_shared<WzQuickChatSection>();
		result->initialize(sectionTitle);
		return result;
	}
public:
	void clickPrimary();
	bool clickSecondary(bool synthesizedFromHold) { return false; }
public:
	void display(int xOffset, int yOffset) override;
	void displayRecursive(WidgetGraphicsContext const &context) override;
	void run(W_CONTEXT *psContext) override;
	int32_t idealWidth() override;
	int32_t idealHeight() override;
	void geometryChanged() override;
	void released(W_CONTEXT *context, WIDGET_KEY mouseButton) override;
	void focusLost() override;
	void highlight(W_CONTEXT *) override;
	void highlightLost() override;
public:
	void addChatOption(WzQuickChatMessage message, ShouldEnableOptionFunc shouldEnableOption = nullptr);
	void addCheatOption(const char* cheatCommand, bool enabledOption = true);
private:
	std::vector<WzQuickChatMessage> messages;
	std::shared_ptr<IntListTabWidget> messagesList;
	bool hasFocus = false;
	bool hasMouseOver = false;
};

constexpr int QC_PLAYER_BUTTON_PADDING = 20;
constexpr int QC_PLAYER_BUTTON_HEIGHT = 20;

class WzQuickChatToggleMixedCheckButton : public W_BUTTON
{
public:
	enum class MixedCheckState
	{
		Unchecked,
		Mixed,
		Checked
	};
public:
	static std::shared_ptr<WzQuickChatToggleMixedCheckButton> make(const WzString& text)
	{
		auto button = std::make_shared<WzQuickChatToggleMixedCheckButton>();
		button->setString(text);
		button->FontID = font_regular;
		int minButtonWidthForText = iV_GetTextWidth(text, button->FontID);
		button->setGeometry(0, 0, minButtonWidthForText + QC_PLAYER_BUTTON_PADDING, QC_PLAYER_BUTTON_HEIGHT);
		return button;
	}
public:
	void display(int xOffset, int yOffset) override
	{
		int x0 = x() + xOffset;
		int y0 = y() + yOffset;
		int x1 = x0 + width();
		int y1 = y0 + height();

		bool isDown = (getState() & (WBUT_DOWN | WBUT_LOCK | WBUT_CLICKLOCK)) != 0;
		bool isDisabled = (getState() & WBUT_DISABLE) != 0;
		bool isHighlight = !isDisabled && ((getState() & WBUT_HIGHLIGHT) != 0);
		bool isChecked = currentChecked == MixedCheckState::Checked;

		// Display the button.
		auto light_border = WZCOL_QUICKCHAT_TABS_BORDER_LIGHT;
		auto fill_color = isDown || isDisabled ? WZCOL_QUICKCHAT_TABS_FILL_COLOR_DARK : WZCOL_QUICKCHAT_TABS_FILL_COLOR;
		iV_ShadowBox(x0, y0, x1, y1, 0, isDown || isHighlight || isChecked ? light_border : WZCOL_FORM_DARK, isDown || isHighlight || isChecked ? light_border : WZCOL_FORM_DARK, fill_color);

		wzText.setText(pText, FontID);
		switch (currentChecked)
		{
			case MixedCheckState::Unchecked:
				wzStatusText.setText("", font_regular);
				break;
			case MixedCheckState::Mixed:
				wzStatusText.setText("-", font_regular);
				break;
			case MixedCheckState::Checked:
				wzStatusText.setText("✓", font_regular);
				break;
		}

		int fw = wzText.width();
		int fx = x0 + (width() - fw) / 2;
		int fy = y0 + (height() - wzText.lineSize()) / 2 - wzText.aboveBase();
		PIELIGHT textColor = (isDown || isChecked) ? WZCOL_TEXT_BRIGHT : WZCOL_FORM_TEXT;
		if (isDisabled)
		{
			textColor.byte.a = (textColor.byte.a / 2);
		}
		wzText.render(fx, fy, textColor);

		wzStatusText.render(x0 + 5, fy, textColor);
	}

	void setCheckedState(MixedCheckState newState)
	{
		currentChecked = newState;
	}

	MixedCheckState getCheckedState() const
	{
		return currentChecked;
	}

	void toggle()
	{
		switch (currentChecked)
		{
			case MixedCheckState::Unchecked:
				currentChecked = MixedCheckState::Checked;
				break;
			case MixedCheckState::Mixed:
				currentChecked = MixedCheckState::Checked;
				break;
			case MixedCheckState::Checked:
				currentChecked = MixedCheckState::Unchecked;
				break;
		}
	}

protected:
	void clicked(W_CONTEXT *context, WIDGET_KEY key) override
	{
		W_BUTTON::clicked(context, key);

		if ((state & WBUT_DOWN) != 0)
		{
			toggle();
		}
	}
private:
	WzText wzText;
	WzText wzStatusText;
	MixedCheckState currentChecked;
};

class WzQuickChatPlayerButton : public W_BUTTON
{
public:
	static std::shared_ptr<WzQuickChatPlayerButton> make(uint32_t playerIdx)
	{
		auto button = std::make_shared<WzQuickChatPlayerButton>();
		button->playerIdx = playerIdx;
		button->playerPos = NetPlay.players[playerIdx].position;
		WzString playerButtonText;
		if (button->playerPos <= 10)
		{
			playerButtonText = WzString::number(button->playerPos) + ": ";
		}
		playerButtonText += getPlayerName(playerIdx);
		button->setString(playerButtonText);
		button->FontID = font_regular;
		bool disabledOption = (NetPlay.players[playerIdx].isSpectator != NetPlay.players[realSelectedPlayer].isSpectator);
		if (disabledOption)
		{
			button->setState(WBUT_DISABLE);
		}
		int minButtonWidthForText = iV_GetTextWidth(playerButtonText, button->FontID);
		button->setGeometry(0, 0, minButtonWidthForText + QC_PLAYER_BUTTON_PADDING, QC_PLAYER_BUTTON_HEIGHT);
		return button;
	}
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
		auto light_border = WZCOL_QUICKCHAT_TABS_BORDER_LIGHT;
		auto fill_color = isDown || isDisabled ? WZCOL_QUICKCHAT_TABS_FILL_COLOR_DARK : WZCOL_QUICKCHAT_TABS_FILL_COLOR;
		iV_ShadowBox(x0, y0, x1, y1, 0, isDown || isHighlight ? light_border : WZCOL_FORM_DARK, isDown || isHighlight ? light_border : WZCOL_FORM_DARK, fill_color);

		if (haveText)
		{
			wzText.setText(pText, FontID);
			int fw = wzText.width();
			int fx = x0 + (width() - fw) / 2;
			int fy = y0 + (height() - wzText.lineSize()) / 2 - wzText.aboveBase();
			PIELIGHT textColor = (isDown) ? WZCOL_TEXT_BRIGHT : WZCOL_FORM_TEXT;
			if (isDisabled)
			{
				textColor.byte.a = (textColor.byte.a / 2);
			}
			wzText.render(fx, fy, textColor);
		}
	}

	void setIsChosen(bool chosen)
	{
		if (chosen)
		{
			state |= WBUT_CLICKLOCK;
		}
		else
		{
			state &= ~WBUT_CLICKLOCK;
		}
	}

	void toggle()
	{
		if ((state & WBUT_CLICKLOCK) == 0)
		{
			state |= WBUT_CLICKLOCK;
		}
		else
		{
			state &= ~WBUT_CLICKLOCK;
		}
	}

	bool isChosen() const
	{
		return (state & (WBUT_LOCK | WBUT_CLICKLOCK)) != 0;
	}

	uint32_t playerIndex() const
	{
		return playerIdx;
	}

	uint32_t playerPosNumber() const
	{
		return playerPos;
	}

protected:
	void clicked(W_CONTEXT *context, WIDGET_KEY key) override
	{
		W_BUTTON::clicked(context, key);

		if ((state & WBUT_DOWN) != 0)
		{
			toggle();
		}
	}
private:
	WzText wzText;
	uint32_t playerIdx;
	uint32_t playerPos;
};

class WzQuickChatSendToSelector;

class WzQuickChatSendToPlayersSelector : public WIDGET
{
public:
	static std::shared_ptr<WzQuickChatSendToPlayersSelector> make(const std::vector<uint32_t>& playersToShow, bool showHumanTeammatesButton, bool showAITeammatesButton, const std::shared_ptr<WzQuickChatSendToSelector>& selectorParent)
	{
		auto result = std::make_shared<WzQuickChatSendToPlayersSelector>();
		result->initialize(playersToShow, showHumanTeammatesButton, showAITeammatesButton, selectorParent);
		return result;
	}
public:
	void updateDisplay(const std::shared_ptr<WzQuickChatSendToSelector>& selectorParent);
private:
	void initialize(const std::vector<uint32_t>& playersToShow, bool showHumanTeammatesButton, bool showAITeammatesButton, const std::shared_ptr<WzQuickChatSendToSelector>& selectorParent);
public:
	void display(int xOffset, int yOffset) override;
	int32_t idealWidth() override;
	int32_t idealHeight() override;
	void geometryChanged() override;
private:
	std::shared_ptr<WzQuickChatToggleMixedCheckButton> humanTeammates;
	std::shared_ptr<WzQuickChatToggleMixedCheckButton> aiTeammates;
	std::vector<std::shared_ptr<WzQuickChatPlayerButton>> playerButtons;
	std::shared_ptr<ScrollableListWidget> playersList;
};

class WzQuickChatSendToSelector : public WIDGET
{
public:
	static std::shared_ptr<WzQuickChatSendToSelector> make(bool teamOnly, const WzQuickChatTargeting& initialTargeting);
protected:
	void display(int xOffset, int yOffset) override;
	void run(W_CONTEXT *psContext) override;
public:
	int32_t idealWidth() override;
	int32_t idealHeight() override;
	void clicked(W_CONTEXT *psContext, WIDGET_KEY key) override;
	void highlight(W_CONTEXT *) override;
	void highlightLost() override;
public:
	std::unordered_set<uint32_t> currentlySelectedPlayers() const;
	const WzQuickChatTargeting& currentTargeting() const;
	const std::vector<uint32_t>& getPlayerHumanTeammates() const { return playerHumanTeammates; }
	const std::vector<uint32_t>& getPlayerAITeammates() const { return playerAITeammates; }
protected:
	friend class WzQuickChatSendToPlayersSelector;
	void updatePlayerSelectionState(uint32_t playerIdx, bool selected);
	void updateTargetingByCategory(optional<bool> all = nullopt, optional<bool> humanTeammates = nullopt, optional<bool> aiTeammates = nullopt);
private:
	void initialize(bool teamOnly, const WzQuickChatTargeting& initialTargeting);
	void setPlayerSelectionStateFromTargeting(const WzQuickChatTargeting& targeting);
	void openChooser();
	void closeChooser();
	int calculateDropdownListScreenPosY() const;
	void updateSelectedPlayersString();
	void updateTargeting();
private:
	std::vector<uint32_t> playersToShow;
	std::vector<uint32_t> playerTeammates;
	std::vector<uint32_t> playerHumanTeammates;
	std::vector<uint32_t> playerAITeammates;
	enum class PlayerSelectionState
	{
		NotAvailable,
		Unselected,
		Selected
	};
	std::array<PlayerSelectionState, MAX_CONNECTED_PLAYERS> playerSelections;
	WzQuickChatTargeting targeting;
	std::shared_ptr<WzQuickChatSendToPlayersSelector> playersSelectorWidget;
	std::shared_ptr<W_SCREEN> overlayScreen;
	WzText sendToText;
	WzText cachedPlayerText;
	bool teamOnly = false;
	bool hasHighlight = false;
};

class WzQuickChatPanel : public WIDGET
{
public:
	enum class WzQuickChatSendToMode
	{
		GlobalSendOptions,
		TeamSendOptions,
	};
public:
	static std::shared_ptr<WzQuickChatPanel> make(const WzString& panelName, optional<WzQuickChatSendToMode> sendToMode);
public:
	void showSendTo();
	void addSection(const std::shared_ptr<WzQuickChatSection>& section, size_t row = 0);
	void setMinSectionsPerRowForLayout(size_t minSectionsPerRow);
	size_t numSections() const { return sections.size(); }
	WzQuickChatTargeting getTargeting() const;
public:
	bool handleTabPress();
private:
	void sendQuickChat(WzQuickChatMessage message, uint32_t fromPlayer);
public:
	void clicked(W_CONTEXT *psContext, WIDGET_KEY key = WKEY_PRIMARY) override;
	void focusLost() override;
	int32_t idealWidth() override;
	int32_t idealHeight() override;
	void geometryChanged() override;
	void displayRecursive(WidgetGraphicsContext const &context) override;
	void run(W_CONTEXT *psContext) override;
private:
	void recalcLayout();
protected:
	friend class WzQuickChatForm;
	friend class WzQuickChatSection;
	friend class WzQuickChatSendToSelector;
	void takeFocus();
	void handleSelfOrChildFocusChange();
private:
	WzString panelName;
	typedef std::vector<std::shared_ptr<WzQuickChatSection>> Row;
	std::vector<Row> sectionRows;
	std::vector<std::shared_ptr<WzQuickChatSection>> sections;
	std::shared_ptr<WzQuickChatSendToSelector> sendToWidget;
	optional<WzQuickChatSendToMode> sendToMode;
	size_t layoutMinSectionsPerRow = 1;
	bool hasFocus = false;
};

class WzQuickChatPanelTabs : public MultichoiceWidget
{
public:
	WzQuickChatPanelTabs(int value = -1) : MultichoiceWidget(value) { }
	virtual void display(int xOffset, int yOffset) override { }
};

constexpr int QCFORM_INTERNAL_PADDING = 10;
constexpr int QCFORM_PANEL_TABS_HEIGHT = 20;

class WzQuickChatForm : public W_CLICKFORM
{
protected:
	void initialize(WzQuickChatContext context, optional<WzQuickChatMode> startingPanel = nullopt);
public:
	static std::shared_ptr<WzQuickChatForm> make(WzQuickChatContext context, optional<WzQuickChatMode> startingPanel = nullopt);
public:
	void display(int xOffset, int yOffset) override;
	void geometryChanged() override;
	int32_t idealHeight() override;
	void released(W_CONTEXT *context, WIDGET_KEY mouseButton) override;
public:
	void panelTakeFocus();
protected:
	friend class WzQuickChatPanel;
	void setIsHighlightedMode(bool childIsFocused);
private:
	void initializeLobby();
	void initializeInGame(optional<WzQuickChatMode> startingMode);
	void recalcLayout();
	void switchAttachedPanel(const std::shared_ptr<WzQuickChatPanel> newPanel);
private:
	std::shared_ptr<WzQuickChatPanel> createInGameGlobalPanel();
	std::shared_ptr<WzQuickChatPanel> createInGameTeamPanel(const std::vector<uint32_t>& playerTeammates);
	std::shared_ptr<WzQuickChatPanel> createInGameCheatPanel();
	std::shared_ptr<WzQuickChatPanel> createInGameEndGamePanel();
public:
	std::function<void ()> onQuickChatSent;
private:
	std::shared_ptr<W_LABEL> quickChatLabel;
	std::shared_ptr<WzQuickChatPanelTabs> panelSwitcher;
	std::vector<std::shared_ptr<WzQuickChatPanel>> panels;
	std::shared_ptr<WzQuickChatPanel> currentlyAttachedPanel;
	bool showHighlighted = false;
};

// MARK: - WzQuickChatTargeting

void WzQuickChatTargeting::reset()
{
	all = false;
	humanTeammates = false;
	aiTeammates = false;
	specificPlayers.clear();
}

bool WzQuickChatTargeting::noTargets() const
{
	return !all && !humanTeammates && !aiTeammates && specificPlayers.empty();
}

WzQuickChatTargeting WzQuickChatTargeting::targetAll()
{
	WzQuickChatTargeting targeting;
	targeting.all = true;
	return targeting;
}

// MARK: - WzQuickChatSendToSelector

std::shared_ptr<WzQuickChatSendToSelector> WzQuickChatSendToSelector::make(bool teamOnly, const WzQuickChatTargeting& initialTargeting)
{
	auto result = std::make_shared<WzQuickChatSendToSelector>();
	result->initialize(teamOnly, initialTargeting);
	return result;
}

std::unordered_set<uint32_t> WzQuickChatSendToSelector::currentlySelectedPlayers() const
{
	std::unordered_set<uint32_t> result;
	for (size_t playerIdx = 0; playerIdx < playerSelections.size(); ++playerIdx)
	{
		if (playerSelections[playerIdx] == PlayerSelectionState::Selected)
		{
			result.insert(playerIdx);
		}
	}
	return result;
}

void WzQuickChatSendToSelector::updateTargeting()
{
	auto specificPlayers = currentlySelectedPlayers();

	targeting.reset();

	targeting.humanTeammates = !playerHumanTeammates.empty() && std::all_of(playerHumanTeammates.begin(), playerHumanTeammates.end(), [&specificPlayers](uint32_t playerIdx) -> bool {
		return specificPlayers.count(playerIdx) > 0;
	});
	targeting.aiTeammates = !playerAITeammates.empty() && std::all_of(playerAITeammates.begin(), playerAITeammates.end(), [&specificPlayers](uint32_t playerIdx) -> bool {
		return specificPlayers.count(playerIdx) > 0;
	});

	if (teamOnly)
	{
		targeting.all = false;
	}
	else
	{
		// "global" targeting mode
		targeting.all = !playersToShow.empty() && std::all_of(playersToShow.begin(), playersToShow.end(), [&specificPlayers](uint32_t playerIdx) -> bool {
			return specificPlayers.count(playerIdx) > 0;
		});
	}

	if (targeting.all)
	{
		specificPlayers.clear();
	}
	if (targeting.humanTeammates)
	{
		for (auto playerIdx : playerHumanTeammates)
		{
			specificPlayers.erase(playerIdx);
		}
	}
	if (targeting.aiTeammates)
	{
		for (auto playerIdx : playerAITeammates)
		{
			specificPlayers.erase(playerIdx);
		}
	}
	targeting.specificPlayers = specificPlayers;
}

const WzQuickChatTargeting& WzQuickChatSendToSelector::currentTargeting() const
{
	return targeting;
}

void WzQuickChatSendToSelector::initialize(bool _teamOnly, const WzQuickChatTargeting& initialTargeting)
{
	teamOnly = _teamOnly;

	playerTeammates = getPlayerTeammates(selectedPlayer, true, true);

	// filter playerTeammates for humans and AIs
	std::copy_if(playerTeammates.begin(), playerTeammates.end(), std::back_inserter(playerHumanTeammates), [](uint32_t playerIdx) -> bool {
		return NetPlay.players[playerIdx].allocated && !NetPlay.players[playerIdx].isSpectator;
	});
	std::copy_if(playerTeammates.begin(), playerTeammates.end(), std::back_inserter(playerAITeammates), [](uint32_t playerIdx) -> bool {
		return NetPlay.players[playerIdx].ai >= 0 && !NetPlay.players[playerIdx].isSpectator;
	});

	if (!teamOnly)
	{
		for (uint32_t playerIdx = 0; playerIdx < MAX_PLAYERS; ++playerIdx)
		{
			if (playerIdx == selectedPlayer)
			{
				continue;
			}

			if (isHumanPlayer(playerIdx) // is an active (connected) human player
				|| (NetPlay.players[playerIdx].ai >= 0) // an AI
				)
			{
				playersToShow.push_back(playerIdx);
			}
		}
	}
	else
	{
		playersToShow = playerTeammates;
	}

	// Sort playersToShow by position
	std::sort(playersToShow.begin(), playersToShow.end(), [](uint32_t playerIdxA, uint32_t playerIdxB) {
		return NetPlay.players[playerIdxA].position < NetPlay.players[playerIdxB].position;
	});

	for (size_t i = 0; i < playerSelections.size(); ++i)
	{
		playerSelections[i] = PlayerSelectionState::NotAvailable;
	}

	for (auto playerIdx : playersToShow)
	{
		playerSelections[playerIdx] = PlayerSelectionState::Unselected;
	}

	targeting = initialTargeting;
	setPlayerSelectionStateFromTargeting(targeting);

	playersSelectorWidget = WzQuickChatSendToPlayersSelector::make(playersToShow, teamOnly && !playerHumanTeammates.empty(), teamOnly && !playerAITeammates.empty(), std::dynamic_pointer_cast<WzQuickChatSendToSelector>(shared_from_this()));

//	updateTargeting();
	updateSelectedPlayersString();

	sendToText.setText(_("To:"), font_regular);
}

void WzQuickChatSendToSelector::updatePlayerSelectionState(uint32_t playerIdx, bool selected)
{
	if (playerSelections[playerIdx] == PlayerSelectionState::NotAvailable)
	{
		return;
	}
	playerSelections[playerIdx] = (selected) ? PlayerSelectionState::Selected : PlayerSelectionState::Unselected;
	updateTargeting();
	updateSelectedPlayersString();
	playersSelectorWidget->updateDisplay(std::dynamic_pointer_cast<WzQuickChatSendToSelector>(shared_from_this()));
}

void WzQuickChatSendToSelector::updateTargetingByCategory(optional<bool> all, optional<bool> humanTeammates, optional<bool> aiTeammates)
{
	if (all.has_value())
	{
		targeting.all = all.value();
		targeting.specificPlayers.clear();
	}
	if (humanTeammates.has_value())
	{
		targeting.humanTeammates = humanTeammates.value();
		targeting.specificPlayers.clear();
	}
	if (aiTeammates.has_value())
	{
		targeting.aiTeammates = aiTeammates.value();
		targeting.specificPlayers.clear();
	}
	setPlayerSelectionStateFromTargeting(targeting);
	updateSelectedPlayersString();
	playersSelectorWidget->updateDisplay(std::dynamic_pointer_cast<WzQuickChatSendToSelector>(shared_from_this()));
}

void WzQuickChatSendToSelector::setPlayerSelectionStateFromTargeting(const WzQuickChatTargeting& newTargeting)
{
	for (auto& selection : playerSelections)
	{
		if (selection == PlayerSelectionState::NotAvailable)
		{
			continue;
		}
		selection = PlayerSelectionState::Unselected;
	}

	if (newTargeting.all)
	{
		for (auto& selection : playerSelections)
		{
			if (selection == PlayerSelectionState::NotAvailable)
			{
				continue;
			}
			selection = PlayerSelectionState::Selected;
		}
		return;
	}

	if (newTargeting.humanTeammates)
	{
		for (auto playerIdx : playerHumanTeammates)
		{
			if (playerSelections[playerIdx] == PlayerSelectionState::NotAvailable)
			{
				continue;
			}
			playerSelections[playerIdx] = PlayerSelectionState::Selected;
		}
	}

	if (newTargeting.aiTeammates)
	{
		for (auto playerIdx : playerAITeammates)
		{
			if (playerSelections[playerIdx] == PlayerSelectionState::NotAvailable)
			{
				continue;
			}
			playerSelections[playerIdx] = PlayerSelectionState::Selected;
		}
	}

	for (auto playerIdx : newTargeting.specificPlayers)
	{
		if (playerSelections[playerIdx] == PlayerSelectionState::NotAvailable)
		{
			continue;
		}
		playerSelections[playerIdx] = PlayerSelectionState::Selected;
	}
}

void WzQuickChatSendToSelector::updateSelectedPlayersString()
{
	if (targeting.all)
	{
		cachedPlayerText.setText(_("All"), font_regular);
		return;
	}

	WzString selectedPlayersString;
	auto currentPlayersSet = currentlySelectedPlayers();

	if (targeting.humanTeammates && targeting.aiTeammates)
	{
		selectedPlayersString += _("Team");
		for (auto playerIdx : playerTeammates)
		{
			currentPlayersSet.erase(playerIdx);
		}
	}
	else if (targeting.humanTeammates)
	{
		if (playerAITeammates.empty())
		{
			selectedPlayersString += _("Team");
		}
		else
		{
			selectedPlayersString += _("Human Teammates");
		}
		for (auto playerIdx : playerHumanTeammates)
		{
			currentPlayersSet.erase(playerIdx);
		}
	}
	else if (targeting.aiTeammates)
	{
		if (playerHumanTeammates.empty())
		{
			selectedPlayersString += _("Team");
		}
		else
		{
			selectedPlayersString += _("Bot Teammates");
		}
		for (auto playerIdx : playerAITeammates)
		{
			currentPlayersSet.erase(playerIdx);
		}
	}

	std::vector<uint32_t> additionalPlayers;
	additionalPlayers.insert(additionalPlayers.end(), currentPlayersSet.begin(), currentPlayersSet.end());
	std::sort(additionalPlayers.begin(), additionalPlayers.end(), [](uint32_t playerIdxA, uint32_t playerIdxB) {
		return NetPlay.players[playerIdxA].position < NetPlay.players[playerIdxB].position;
	});

	if (selectedPlayersString.isEmpty() && additionalPlayers.size() == 1)
	{
		selectedPlayersString += getPlayerName(additionalPlayers.front());
	}
	else if (!additionalPlayers.empty())
	{
		if (!selectedPlayersString.isEmpty())
		{
			selectedPlayersString += " + ";
		}
		selectedPlayersString += WzString::fromUtf8(astringf(_("%u players"), static_cast<unsigned>(additionalPlayers.size())));
	}

	if (selectedPlayersString.isEmpty())
	{
		selectedPlayersString = _("Choose a recipient");
	}

	cachedPlayerText.setText(selectedPlayersString, font_regular);
}

void WzQuickChatSendToSelector::display(int xOffset, int yOffset)
{
	int x0 = x() + xOffset;
	int y0 = y() + yOffset;

	if (overlayScreen)
	{
		pie_UniTransBoxFill(x0, y0, x0 + width(), y0 + height(), WZCOL_QUICKCHAT_TABS_FILL_COLOR_DARK);
	}

	int toTextX0 = x0;
	int toTextY0 = y0 + (height() - sendToText.lineSize()) / 2 - sendToText.aboveBase();
	sendToText.render(toTextX0, toTextY0, WZCOL_TEXT_BRIGHT);

	int fx = toTextX0 + sendToText.width() + 4;
	int fy = y0 + (height() - cachedPlayerText.lineSize()) / 2 - cachedPlayerText.aboveBase();
	PIELIGHT textColor = (targeting.noTargets()) ? WZCOL_RED : WZCOL_TEXT_MEDIUM;
	cachedPlayerText.render(fx, fy, textColor);

	if (hasHighlight)
	{
		iV_Box(x0, y0, x0 + width(), y0 + height(), WZCOL_QUICKCHAT_TABS_BORDER_LIGHT);
	}
}

int32_t WzQuickChatSendToSelector::idealWidth()
{
	return 200; // TODO: Calculate
}

int32_t WzQuickChatSendToSelector::idealHeight()
{
	return cachedPlayerText.lineSize();
}

void WzQuickChatSendToSelector::highlight(W_CONTEXT *)
{
	hasHighlight = true;
}

void WzQuickChatSendToSelector::highlightLost()
{
	hasHighlight = false;
}

void WzQuickChatSendToSelector::clicked(W_CONTEXT *psContext, WIDGET_KEY key)
{
	auto weakThis = std::weak_ptr<WzQuickChatSendToSelector>(std::dynamic_pointer_cast<WzQuickChatSendToSelector>(shared_from_this()));
	widgScheduleTask([weakThis](){
		auto strongSendToSelector = weakThis.lock();
		ASSERT_OR_RETURN(, strongSendToSelector != nullptr, "Widget no longer exists?");
		auto strongParent = std::dynamic_pointer_cast<WzQuickChatPanel>(strongSendToSelector->parent());
		ASSERT_OR_RETURN(, strongParent != nullptr, "Parent doesn't exist?");
		// give the current panel focus
		strongParent->takeFocus();

		// open chooser
		strongSendToSelector->openChooser();
	});
}

class ChooserOverlay: public WIDGET
{
public:
	ChooserOverlay(std::function<void ()> onClickedFunc): onClickedFunc(onClickedFunc)
	{
		setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
			psWidget->setGeometry(0, 0, screenWidth - 1, screenHeight - 1);
		}));
	}

	void clicked(W_CONTEXT *, WIDGET_KEY = WKEY_PRIMARY) override
	{
		onClickedFunc();
	}

	std::function<void ()> onClickedFunc;
};

int WzQuickChatSendToSelector::calculateDropdownListScreenPosY() const
{
	int dropDownOverlayPosY = screenPosY() + height() + 1;
	if (dropDownOverlayPosY + playersSelectorWidget->idealHeight() > screenHeight)
	{
		// Positioning the dropdown below would cause it to appear partially or fully offscreen
		// So, instead, position it above
		dropDownOverlayPosY = screenPosY() - (playersSelectorWidget->idealHeight() + 1);
		if (dropDownOverlayPosY < 0)
		{
			// Well, this would be off-screen too...
			// For now: Just position it at the top
			dropDownOverlayPosY = 0;
		}
	}
	return dropDownOverlayPosY;
}

void WzQuickChatSendToSelector::openChooser()
{
	if (overlayScreen != nullptr) { return; }
	std::weak_ptr<WzQuickChatSendToSelector> pWeakThis(std::dynamic_pointer_cast<WzQuickChatSendToSelector>(shared_from_this()));
	widgScheduleTask([pWeakThis]() {
		if (auto playerSelectorWidget = pWeakThis.lock())
		{
			playerSelectorWidget->overlayScreen = W_SCREEN::make();
			widgRegisterOverlayScreenOnTopOfScreen(playerSelectorWidget->overlayScreen, playerSelectorWidget->screenPointer.lock());
			playerSelectorWidget->playersSelectorWidget->setGeometry(playerSelectorWidget->screenPosX(), playerSelectorWidget->calculateDropdownListScreenPosY(), playerSelectorWidget->idealWidth(), playerSelectorWidget->playersSelectorWidget->idealHeight());
			playerSelectorWidget->overlayScreen->psForm->attach(playerSelectorWidget->playersSelectorWidget);
			playerSelectorWidget->overlayScreen->psForm->attach(std::make_shared<ChooserOverlay>([pWeakThis]() { if (auto playerSelectorWidget = pWeakThis.lock()) { playerSelectorWidget->closeChooser(); } }));
		}
	});
}

void WzQuickChatSendToSelector::closeChooser()
{
	std::weak_ptr<WzQuickChatSendToSelector> pWeakThis(std::dynamic_pointer_cast<WzQuickChatSendToSelector>(shared_from_this()));
	widgScheduleTask([pWeakThis]() {
		if (auto playerSelectorWidget = pWeakThis.lock())
		{
			if (playerSelectorWidget->overlayScreen != nullptr)
			{
				widgRemoveOverlayScreen(playerSelectorWidget->overlayScreen);
				if (playerSelectorWidget->overlayScreen->psForm)
				{
					playerSelectorWidget->overlayScreen->psForm->detach(playerSelectorWidget->playersSelectorWidget);
				}
				playerSelectorWidget->overlayScreen = nullptr;
			}
		}
	});
}

void WzQuickChatSendToSelector::run(W_CONTEXT *psContext)
{
	if (overlayScreen)
	{
		playersSelectorWidget->setGeometry(screenPosX(), calculateDropdownListScreenPosY(), width(), playersSelectorWidget->height());

		if (keyPressed(KEY_ESC))
		{
			inputLoseFocus();	// clear the input buffer.
			closeChooser();
		}
	}
}


// MARK: - WzQuickChatSendToPlayersSelector

constexpr int QC_SENDTO_SELECTOR_INTERNAL_SPACING = 4;

void WzQuickChatSendToPlayersSelector::initialize(const std::vector<uint32_t>& playersToShow, bool showHumanTeammatesButton, bool showAITeammatesButton, const std::shared_ptr<WzQuickChatSendToSelector>& selectorParent)
{
	auto psWeakParent = std::weak_ptr<WzQuickChatSendToSelector>(selectorParent);

	if (showHumanTeammatesButton)
	{
		humanTeammates = WzQuickChatToggleMixedCheckButton::make(_("Human Teammates"));
		humanTeammates->addOnClickHandler([psWeakParent](W_BUTTON& widg){
			auto strongParent = psWeakParent.lock();
			ASSERT_OR_RETURN(, strongParent != nullptr, "No parent?");
			auto psStrongToggleMixedButton = std::dynamic_pointer_cast<WzQuickChatToggleMixedCheckButton>(widg.shared_from_this());
			ASSERT_OR_RETURN(, psStrongToggleMixedButton != nullptr, "Unable to get proper access to self?");
			WzQuickChatTargeting modifiedTargeting = strongParent->currentTargeting();
			bool humanTeammatesValue = false;
			switch (psStrongToggleMixedButton->getCheckedState())
			{
				case WzQuickChatToggleMixedCheckButton::MixedCheckState::Mixed:
					// do nothing
					return;
				case WzQuickChatToggleMixedCheckButton::MixedCheckState::Unchecked:
					humanTeammatesValue = false;
					break;
				case WzQuickChatToggleMixedCheckButton::MixedCheckState::Checked:
					humanTeammatesValue = true;
					break;
			}
			strongParent->updateTargetingByCategory(nullopt, humanTeammatesValue, nullopt);
		});
		attach(humanTeammates);
		humanTeammates->setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
			auto psParent = std::dynamic_pointer_cast<WzQuickChatSendToPlayersSelector>(psWidget->parent());
			ASSERT_OR_RETURN(, psParent != nullptr, "Parent is null?");
			psWidget->setGeometry(0, 0, psParent->width(), psWidget->idealHeight());
		}));
	}

	if (showAITeammatesButton)
	{
		aiTeammates = WzQuickChatToggleMixedCheckButton::make(_("Bot Teammates"));
		aiTeammates->addOnClickHandler([psWeakParent](W_BUTTON& widg){
			auto strongParent = psWeakParent.lock();
			ASSERT_OR_RETURN(, strongParent != nullptr, "No parent?");
			auto psStrongToggleMixedButton = std::dynamic_pointer_cast<WzQuickChatToggleMixedCheckButton>(widg.shared_from_this());
			ASSERT_OR_RETURN(, psStrongToggleMixedButton != nullptr, "Unable to get proper access to self?");
			bool aiTeammatesValue = false;
			switch (psStrongToggleMixedButton->getCheckedState())
			{
				case WzQuickChatToggleMixedCheckButton::MixedCheckState::Mixed:
					// do nothing
					return;
				case WzQuickChatToggleMixedCheckButton::MixedCheckState::Unchecked:
					aiTeammatesValue = false;
					break;
				case WzQuickChatToggleMixedCheckButton::MixedCheckState::Checked:
					aiTeammatesValue = true;
					break;
			}
			strongParent->updateTargetingByCategory(nullopt, nullopt, aiTeammatesValue);
		});
		attach(aiTeammates);
		aiTeammates->setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
			auto psParent = std::dynamic_pointer_cast<WzQuickChatSendToPlayersSelector>(psWidget->parent());
			ASSERT_OR_RETURN(, psParent != nullptr, "Parent is null?");
			int y0 = (psParent->humanTeammates) ? psParent->humanTeammates->y() + psParent->humanTeammates->height() + QC_SENDTO_SELECTOR_INTERNAL_SPACING : 0;
			psWidget->setGeometry(0, y0, psParent->width(), psWidget->idealHeight());
		}));
	}

	playersList = ScrollableListWidget::make();
	for (auto playerIdx : playersToShow)
	{
		auto playerButton = WzQuickChatPlayerButton::make(playerIdx);
		playerButton->addOnClickHandler([psWeakParent, playerIdx](W_BUTTON& widg){
			auto strongParent = psWeakParent.lock();
			ASSERT_OR_RETURN(, strongParent != nullptr, "No parent?");
			bool isChosen = (widg.getState() & WBUT_CLICKLOCK) != 0;
			strongParent->updatePlayerSelectionState(playerIdx, isChosen);
		});
		playersList->addItem(playerButton);
		playerButtons.push_back(playerButton);
	}
	attach(playersList);

	// set playersList max height
	int maxPlayersListHeight = std::min(100, static_cast<int>(playersToShow.size()) * QC_PLAYER_BUTTON_HEIGHT);
	playersList->setCalcLayout([maxPlayersListHeight](WIDGET *psWidget) {
		auto psParent = std::dynamic_pointer_cast<WzQuickChatSendToPlayersSelector>(psWidget->parent());
		ASSERT_OR_RETURN(, psParent != nullptr, "Parent is null?");
		int y0 = 0;
		if (psParent->aiTeammates)
		{
			y0 = psParent->aiTeammates->y() + psParent->aiTeammates->height() + QC_SENDTO_SELECTOR_INTERNAL_SPACING;
		}
		else if (psParent-> humanTeammates)
		{
			y0 = psParent->humanTeammates->y() + psParent->humanTeammates->height() + QC_SENDTO_SELECTOR_INTERNAL_SPACING;
		}
		psWidget->setGeometry(0, y0, psParent->width(), maxPlayersListHeight);
	});

	updateDisplay(selectorParent);
}

void WzQuickChatSendToPlayersSelector::display(int xOffset, int yOffset)
{
	int x0 = x() + xOffset;
	int y0 = y() + yOffset;
	int x1 = x0 + width();
	int y1 = y0 + height();

	PIELIGHT borderColor = pal_RGBA(0, 0, 0, 255);

	// draw background + border
	iV_TransBoxFill((float)x0, (float)y0, (float)x1, (float)y1);
	iV_Box(x0, y0, x1, y1,borderColor);
}

void WzQuickChatSendToPlayersSelector::updateDisplay(const std::shared_ptr<WzQuickChatSendToSelector>& selectorParent)
{
	auto targeting = selectorParent->currentTargeting();
	auto selectedPlayers = selectorParent->currentlySelectedPlayers();
	if (humanTeammates)
	{
		if (targeting.humanTeammates)
		{
			humanTeammates->setCheckedState(WzQuickChatToggleMixedCheckButton::MixedCheckState::Checked);
		}
		else
		{
			// distinguish between mixed or unchecked states
			auto possible = selectorParent->getPlayerHumanTeammates();
			if (std::any_of(possible.begin(), possible.end(), [&selectedPlayers](uint32_t playerIdx) -> bool {
				return selectedPlayers.count(playerIdx) > 0;
			}))
			{
				humanTeammates->setCheckedState(WzQuickChatToggleMixedCheckButton::MixedCheckState::Mixed);
			}
			else
			{
				humanTeammates->setCheckedState(WzQuickChatToggleMixedCheckButton::MixedCheckState::Unchecked);
			}
		}
	}
	if (aiTeammates)
	{
		if (targeting.aiTeammates)
		{
			aiTeammates->setCheckedState(WzQuickChatToggleMixedCheckButton::MixedCheckState::Checked);
		}
		else
		{
			// distinguish between mixed or unchecked states
			auto possible = selectorParent->getPlayerAITeammates();
			if (std::any_of(possible.begin(), possible.end(), [&selectedPlayers](uint32_t playerIdx) -> bool {
				return selectedPlayers.count(playerIdx) > 0;
			}))
			{
				aiTeammates->setCheckedState(WzQuickChatToggleMixedCheckButton::MixedCheckState::Mixed);
			}
			else
			{
				aiTeammates->setCheckedState(WzQuickChatToggleMixedCheckButton::MixedCheckState::Unchecked);
			}
		}
	}

	for (auto& playerButton : playerButtons)
	{
		if (selectedPlayers.count(playerButton->playerIndex()) > 0)
		{
			playerButton->setIsChosen(true);
		}
		else
		{
			playerButton->setIsChosen(false);
		}
	}
}

void WzQuickChatSendToPlayersSelector::geometryChanged()
{
	if (humanTeammates)
	{
		humanTeammates->callCalcLayout();
	}
	if (aiTeammates)
	{
		aiTeammates->callCalcLayout();
	}
	playersList->callCalcLayout();
}

int32_t WzQuickChatSendToPlayersSelector::idealWidth()
{
	return 200; // TODO: Calculate
}

int32_t WzQuickChatSendToPlayersSelector::idealHeight()
{
	return playersList->height() + ((humanTeammates) ? humanTeammates->idealHeight() : 0) + ((aiTeammates) ? aiTeammates->idealHeight() : 0);
}


// MARK: - WzQuickChatSection

void WzQuickChatSection::displayRecursive(WidgetGraphicsContext const &context)
{
	// call parent displayRecursive
	WIDGET::displayRecursive(context);

	if (hasFocus)
	{
		// "over-draw" with keyboard shortcut identifiers for all of the messages (and a border to make it clear it's selected)

		int x0 = context.getXOffset() + x();
		int y0 = context.getYOffset() + y();

		iV_Box(x0, y0, x0 + width(), y0 + height(), pal_RGBA(255, 255, 255, 150));

		int lineSize = iV_GetTextLineSize(font_bar);
		int aboveBase = iV_GetTextAboveBase(font_bar);

		size_t widgetDisplayIndex = 0;
		for (size_t i = messagesList->listWidget()->firstWidgetShownIndex(); i <= messagesList->listWidget()->lastWidgetShownIndex(); ++i)
		{
			auto widget = messagesList->listWidget()->getWidgetAtIndex(i);
			int widgetX0 = widget->screenPosX();
			int widgetY0 = widget->screenPosY();
			int widgetX1 = widgetX0 + widget->width();

			int overlayKeyX0 = widgetX1 - lineSize;
			int overlayKeyY0 = widgetY0;
			pie_UniTransBoxFill(overlayKeyX0, overlayKeyY0, overlayKeyX0 + lineSize, overlayKeyY0 + lineSize, pal_RGBA(0, 0, 255, 150));
			auto numberStr = WzString::number((widgetDisplayIndex < 9) ? widgetDisplayIndex + 1 : 0);
			int textX0 = overlayKeyX0 + (lineSize - iV_GetTextWidth(numberStr.toUtf8().c_str(), font_bar)) / 2;
			int textY0 = overlayKeyY0 - aboveBase;
			iV_DrawText(numberStr.toUtf8().c_str(), textX0, textY0, font_bar);

			++widgetDisplayIndex;
		}
	}
}

void WzQuickChatSection::run(W_CONTEXT *psContext)
{
	if (!hasFocus)
	{
		// the only thing we handle if we don't have focus is mousewheel events *if* mouse is over the widget (or a widget child)
		bool mouseIsOverWidgetOrChildren = hasMouseOver || (hitTest(psContext->mx, psContext->my) && isMouseOverScreen(screenPointer.lock()));
		if (mouseIsOverWidgetOrChildren)
		{
			if (mousePressed(MOUSE_WUP))
			{
				if ((messagesList->currentPage() == 0) || !messagesList->setCurrentPage(messagesList->currentPage() - 1))
				{
					// could add a sound effect here
				}
				return;
			}
			if (mousePressed(MOUSE_WDN))
			{
				if (!messagesList->setCurrentPage(messagesList->currentPage() + 1))
				{
					// could add sound effect here
				}
				return;
			}
		}
		return; // nothing else to do
	}

	// handle keypresses when has focus
	if (keyPressed(KEY_ESC))
	{
		// immediately return - parent should handle?
		return;
	}

	if (keyPressed(KEY_TAB))
	{
		// parent should handle switching
		auto psParent = std::dynamic_pointer_cast<WzQuickChatPanel>(parent());
		if (psParent)
		{
			psParent->handleTabPress();
		}
		inputLoseFocus();
		return;
	}

	if (keyPressed(KEY_MINUS) || keyPressed(KEY_KP_MINUS) || mousePressed(MOUSE_WUP))
	{
		if ((messagesList->currentPage() == 0) || !messagesList->setCurrentPage(messagesList->currentPage() - 1))
		{
			// could add a sound effect here
		}
		inputLoseFocus();
		return;
	}
	if (keyPressed(KEY_EQUALS) || keyPressed(KEY_KP_PLUS) || mousePressed(MOUSE_WDN))
	{
		if (!messagesList->setCurrentPage(messagesList->currentPage() + 1))
		{
			// could add sound effect here
		}
		inputLoseFocus();
		return;
	}

	optional<size_t> keyPressForMessageButton = numberButtonPressed();

	inputLoseFocus();

	if (keyPressForMessageButton.has_value())
	{
		size_t displayedMessagesFirstIndex = messagesList->listWidget()->firstWidgetShownIndex();
		size_t targetedMessageIndex = displayedMessagesFirstIndex + keyPressForMessageButton.value();
		if (targetedMessageIndex <= messagesList->listWidget()->lastWidgetShownIndex())
		{
			auto widget = std::dynamic_pointer_cast<WzQuickChatButton>(messagesList->listWidget()->getWidgetAtIndex(targetedMessageIndex));
			if (widget)
			{
				widget->simulateClick();
			}
		}
	}
}

void WzQuickChatSection::highlight(W_CONTEXT *)
{
	hasMouseOver = true;
}

void WzQuickChatSection::highlightLost()
{
	hasMouseOver = false;
}

void WzQuickChatSection::released(W_CONTEXT *context, WIDGET_KEY mouseButton)
{
	bool clickAndReleaseOnThisButton = ((state & WBUT_DOWN) != 0); // relies on W_CLICKFORM handling to properly set WBUT_DOWN

	W_CLICKFORM::released(context, mouseButton);

	if (!clickAndReleaseOnThisButton)
	{
		return; // do nothing
	}

	if (mouseButton == WKEY_PRIMARY)
	{
		clickPrimary();
	}
	else if (mouseButton == WKEY_SECONDARY)
	{
		clickSecondary(false);
	}
}

void WzQuickChatSection::clickPrimary()
{
	/* Tell the form that the panel has focus */
	if (auto lockedScreen = screenPointer.lock())
	{
		hasFocus = true;
		lockedScreen->setFocus(shared_from_this());
		auto psParent = std::dynamic_pointer_cast<WzQuickChatPanel>(parent());
		if (psParent)
		{
			psParent->handleSelfOrChildFocusChange();
		}
	}
}

void WzQuickChatSection::focusLost()
{
	hasFocus = false;
	auto psParent = std::dynamic_pointer_cast<WzQuickChatPanel>(parent());
	if (psParent)
	{
		psParent->handleSelfOrChildFocusChange();
	}
}

int32_t WzQuickChatSection::idealWidth()
{
	return messagesList->idealWidth();
}

int32_t WzQuickChatSection::idealHeight()
{
	int32_t displayedChildren = std::min(10, static_cast<int32_t>(messagesList->childrenSize()));
	return messagesList->heightOfTabsLabel() + (WzQuickChatButton::getIdealBaseButtonHeight() * displayedChildren) + (OBJ_GAP * (displayedChildren - 1));
}

void WzQuickChatSection::geometryChanged()
{
	if (messagesList)
	{
		messagesList->callCalcLayout();
	}
}

void WzQuickChatSection::display(int xOffset, int yOffset)
{
	// currently, no-op
}

void WzQuickChatSection::initialize(const WzString& sectionTitle)
{
	attach(messagesList = IntListTabWidget::make(TabAlignment::RightAligned));
	messagesList->setChildSpacing(OBJ_GAP, OBJ_GAP);
	messagesList->setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
		auto psParent = psWidget->parent();
		ASSERT_OR_RETURN(, psParent != nullptr, "No parent");
		auto psMessagesList = std::dynamic_pointer_cast<IntListTabWidget>(psWidget->shared_from_this());
		ASSERT_OR_RETURN(, psMessagesList != nullptr, "Not expected type?");
		psMessagesList->setGeometry(0, 0, psParent->width(), psParent->height());
		int desiredChildWidth = psParent->width();
		psMessagesList->setChildSize(desiredChildWidth, WzQuickChatButton::getIdealBaseButtonHeight());
	}));
	WzString titleStr = sectionTitle;
	titleStr += " ";
	messagesList->setTitle(titleStr);
}

void WzQuickChatSection::addChatOption(WzQuickChatMessage message, ShouldEnableOptionFunc shouldEnableOption /*= nullptr*/)
{
	bool enabledOption = true;
	if (shouldEnableOption)
	{
		enabledOption = shouldEnableOption(message);
	}
	auto messageButton = WzQuickChatButton::make(to_display_string(message), !enabledOption);
	std::weak_ptr<WzQuickChatSection> psWeakSection = std::dynamic_pointer_cast<WzQuickChatSection>(shared_from_this());
	messageButton->addOnClickHandler([message, psWeakSection](W_BUTTON& widg) {
		auto psStrongSection = psWeakSection.lock();
		ASSERT_OR_RETURN(, psStrongSection != nullptr, "No valid section?");
		auto psParent = std::dynamic_pointer_cast<WzQuickChatPanel>(psStrongSection->parent());
		ASSERT_OR_RETURN(, psParent != nullptr, "No valid parent?");
		psParent->sendQuickChat(message, selectedPlayer);
	});
	messagesList->addWidgetToLayout(messageButton);
}

void WzQuickChatSection::addCheatOption(const char* cheatCommand, bool enabledOption /* = true */)
{
	auto messageButton = WzQuickChatButton::make(cheatCommand, !enabledOption);
	messagesList->addWidgetToLayout(messageButton);
}

// MARK: - WzQuickChatPanel

std::shared_ptr<WzQuickChatPanel> WzQuickChatPanel::make(const WzString& panelName, optional<WzQuickChatSendToMode> sendToMode)
{
	auto result = std::make_shared<WzQuickChatPanel>();
	result->panelName = panelName;
	result->sendToMode = sendToMode;
	return result;
}

void WzQuickChatPanel::addSection(const std::shared_ptr<WzQuickChatSection>& section, size_t row /*= 0*/)
{
	if (row >= sectionRows.size())
	{
		sectionRows.resize(row + 1);
	}
	sectionRows[row].push_back(section);
	sections.push_back(section);
	attach(section);
	recalcLayout();
}

void WzQuickChatPanel::showSendTo()
{
	if (!sendToMode.has_value())
	{
		if (sendToWidget)
		{
			detach(sendToWidget);
			sendToWidget.reset();
		}
		return;
	}
	WzQuickChatTargeting initialTargeting;
	switch (sendToMode.value())
	{
		case WzQuickChatSendToMode::GlobalSendOptions:
			initialTargeting.all = true;
			break;
		case WzQuickChatSendToMode::TeamSendOptions:
			initialTargeting = lastTeamChatTargeting;
			break;
	}

	sendToWidget = WzQuickChatSendToSelector::make((sendToMode.value() == WzQuickChatSendToMode::TeamSendOptions), initialTargeting);
	attach(sendToWidget);
}

WzQuickChatTargeting WzQuickChatPanel::getTargeting() const
{
	if (sendToWidget)
	{
		return sendToWidget->currentTargeting();
	}
	else
	{
		WzQuickChatTargeting initialTargeting;
		if (sendToMode.has_value())
		{
			switch (sendToMode.value())
			{
				case WzQuickChatSendToMode::GlobalSendOptions:
					initialTargeting.all = true;
					break;
				case WzQuickChatSendToMode::TeamSendOptions:
					initialTargeting = defaultTeamChatTargeting;
					break;
			}
		}
		return initialTargeting;
	}
}

void WzQuickChatPanel::setMinSectionsPerRowForLayout(size_t minSectionsPerRow)
{
	if (minSectionsPerRow == layoutMinSectionsPerRow)
	{
		return;
	}
	layoutMinSectionsPerRow = std::max<size_t>(minSectionsPerRow, 1);
	recalcLayout();
}

void WzQuickChatPanel::recalcLayout()
{
	// Send-To widget is always on top, if set
	int sectionsTopY0 = 0;
	if (sendToWidget)
	{
		sendToWidget->setGeometry(10, sectionsTopY0, sendToWidget->idealWidth(), sendToWidget->idealHeight());
		sectionsTopY0 += sendToWidget->y() + sendToWidget->height() + 10;
	}

	if (sectionRows.empty())
	{
		return;
	}

	// Sections row contents are laid out left-to-right
	for (const auto& row : sectionRows)
	{
		int layoutSectionsPerRow = std::max(layoutMinSectionsPerRow, row.size());
		int sectionWidth = static_cast<int>((width() - (static_cast<int>(layoutSectionsPerRow + 1) * 10)) / static_cast<int>(layoutSectionsPerRow));
		int lastSectionX1 = 0;
		int32_t maxSectionIdealHeight = 0;
		for (auto& section : row)
		{
			int sectionX0 = lastSectionX1 + 10;
			section->setGeometry(sectionX0, sectionsTopY0, sectionWidth, section->idealHeight());
			lastSectionX1 = sectionX0 + sectionWidth;
			maxSectionIdealHeight = std::max(maxSectionIdealHeight, section->idealHeight());
		}

		sectionsTopY0 += maxSectionIdealHeight + 10;
	}
}

void WzQuickChatPanel::takeFocus()
{
	/* Tell the form that the panel has focus */
	if (auto lockedScreen = screenPointer.lock())
	{
		lockedScreen->setFocus(shared_from_this());
		hasFocus = true;
		auto psParent = std::dynamic_pointer_cast<WzQuickChatForm>(parent());
		if (psParent)
		{
			psParent->setIsHighlightedMode(true);
		}
	}
}

void WzQuickChatPanel::clicked(W_CONTEXT *psContext, WIDGET_KEY)
{
	takeFocus();
}

void WzQuickChatPanel::handleSelfOrChildFocusChange()
{
	bool childHasFocus = false;
	auto psParent = std::dynamic_pointer_cast<WzQuickChatForm>(parent());
	auto lockedScreen = screenPointer.lock();
	if (psParent && lockedScreen)
	{
		auto newFocusWidget = lockedScreen->getWidgetWithFocus();
		if (newFocusWidget)
		{
			childHasFocus = newFocusWidget->hasAncestor(this);
		}
		psParent->setIsHighlightedMode(childHasFocus);
	}
}

void WzQuickChatPanel::focusLost()
{
	hasFocus = false;
	handleSelfOrChildFocusChange();
}

int32_t WzQuickChatPanel::idealWidth()
{
	int maxIdealWidth = 0;
	if (sendToWidget)
	{
		maxIdealWidth = std::max(maxIdealWidth, sendToWidget->idealWidth());
	}
	for (const auto& row : sectionRows)
	{
		int maxSectionIdealWidthTotal = 10;
		for (size_t i = 0; i < row.size(); ++i)
		{
			maxSectionIdealWidthTotal += row[i]->idealWidth() + 10;
		}
		maxIdealWidth = std::max(maxIdealWidth, maxSectionIdealWidthTotal);
	}
	return maxIdealWidth;
}

int32_t WzQuickChatPanel::idealHeight()
{
	int result = 10;
	if (sendToWidget)
	{
		result += sendToWidget->idealHeight() + 10;
	}
	for (const auto& row : sectionRows)
	{
		int32_t maxSectionIdealHeight = 0;
		for (size_t i = 0; i < row.size(); ++i)
		{
			maxSectionIdealHeight = std::max(maxSectionIdealHeight, row[i]->idealHeight());
		}
		result += maxSectionIdealHeight + 10;
	}
	return result;
}

void WzQuickChatPanel::geometryChanged()
{
	recalcLayout();
}

void WzQuickChatPanel::displayRecursive(WidgetGraphicsContext const &context)
{
	// call parent displayRecursive
	WIDGET::displayRecursive(context);

	if (hasFocus)
	{
		// "over-draw" with keyboard shortcut identifiers for all of the sections

		int x0 = context.getXOffset() + x();
		int y0 = context.getYOffset() + y();

		int lineSize = iV_GetTextLineSize(font_bar);
		int aboveBase = iV_GetTextAboveBase(font_bar);

		for (size_t i = 0; i < sections.size(); ++i)
		{
			auto& section = sections[i];
			int sectionX0 = section->x() + x0;
			int sectionY0 = section->y() + y0;

			int overlayKeyX0 = sectionX0 + (section->width() - lineSize) / 2;
			int overlayKeyX1 = sectionY0;
			pie_UniTransBoxFill(overlayKeyX0, overlayKeyX1, overlayKeyX0 + lineSize, overlayKeyX1 + lineSize, pal_RGBA(0, 0, 255, 150));
			auto numberStr = WzString::number(i + 1);
			int textX0 = overlayKeyX0 + (lineSize - iV_GetTextWidth(numberStr.toUtf8().c_str(), font_bar)) / 2;
			int textY0 = overlayKeyX1 - aboveBase;
			iV_DrawText(numberStr.toUtf8().c_str(), textX0, textY0, font_bar);
		}
	}
}

bool WzQuickChatPanel::handleTabPress()
{
	// find current section with focus
	if (auto lockedScreen = screenPointer.lock())
	{
		auto widgetWithFocus = lockedScreen->getWidgetWithFocus();
		if (widgetWithFocus == nullptr)
		{
			return false;
		}
		auto it = std::find(sections.begin(), sections.end(), widgetWithFocus);
		if (it == sections.end())
		{
			return false;
		}
//		if (keyDown(KEY_LSHIFT) || keyDown(KEY_RSHIFT))
//		{
//			if (it != sections.begin())
//			{
//				--it;
//			}
//			else
//			{
//				it = sections.end() - 1;
//			}
//		}
//		else
//		{
			++it;
			if (it == sections.end())
			{
				it = sections.begin();
			}
//		}
		inputLoseFocus();
		auto targetWidgetToTakeFocus = *it;
		widgScheduleTask([targetWidgetToTakeFocus](){
			targetWidgetToTakeFocus->clickPrimary();
		});
		return true;
	}
	return false;
}

void WzQuickChatPanel::sendQuickChat(WzQuickChatMessage message, uint32_t fromPlayer)
{
	::sendQuickChat(message, selectedPlayer, getTargeting());
	auto psParent = std::dynamic_pointer_cast<WzQuickChatForm>(parent());
	ASSERT_OR_RETURN(, psParent != nullptr, "No parent?");
	if (psParent->onQuickChatSent)
	{
		psParent->onQuickChatSent();
	}
}

void WzQuickChatPanel::run(W_CONTEXT *psContext)
{
	if (keyPressed(KEY_ESC))
	{
		if (auto lockedScreen = screenPointer.lock())
		{
			auto widgetWithFocus = lockedScreen->getWidgetWithFocus();
			if (widgetWithFocus != nullptr && widgetWithFocus != shared_from_this())
			{
				if (widgetWithFocus->hasAncestor(this))
				{
					// the focused widget is a child widget - take focus
					takeFocus();
					inputLoseFocus(); // clear input buffer
					return;
				}
			}
		}
		// else, immediately return - parent should handle?
		return;
	}

	if (!hasFocus)
	{
		return; // nothing to do
	}

	// handle keypresses when has focus

	if (keyPressed(KEY_TAB))
	{
		if (!handleTabPress())
		{
			// let the parent handle it
		}
		return;
	}

	optional<size_t> keyPressForSection = numberButtonPressed();

	inputLoseFocus();

	if (keyPressForSection.has_value() && keyPressForSection.value() < sections.size())
	{
		sections[keyPressForSection.value()]->clickPrimary();
	}
}

// MARK: - WzQuickChatForm

void WzQuickChatForm::released(W_CONTEXT *context, WIDGET_KEY mouseButton)
{
	bool clickAndReleaseOnThisForm = ((state & WBUT_DOWN) != 0); // relies on W_CLICKFORM handling to properly set WBUT_DOWN

	W_CLICKFORM::released(context, mouseButton);

	if (!clickAndReleaseOnThisForm)
	{
		return; // do nothing
	}

	if (mouseButton == WKEY_PRIMARY)
	{
		// give the current panel focus
		panelTakeFocus();
	}
}

void WzQuickChatForm::panelTakeFocus()
{
	// give the current panel focus
	if (currentlyAttachedPanel)
	{
		currentlyAttachedPanel->takeFocus();
	}
}

void WzQuickChatForm::display(int xOffset, int yOffset)
{
	int x0 = x() + xOffset;
	int y0 = y() + yOffset;
	int x1 = x0 + width();
	int y1 = y0 + height();

	PIELIGHT borderColor = (!showHighlighted) ? pal_RGBA(0, 0, 0, 255) : pal_RGBA(255, 255, 255, 100);

	// draw background + border
	iV_TransBoxFill((float)x0, (float)y0, (float)x1, (float)y1);
	iV_Box(x0, y0, x1, y1,borderColor);
}

int32_t WzQuickChatForm::idealHeight()
{
	int suggestedHeight = QCFORM_INTERNAL_PADDING;
	if (panelSwitcher)
	{
		suggestedHeight += std::max(panelSwitcher->idealHeight(), quickChatLabel->requiredHeight()) + QCFORM_INTERNAL_PADDING;
	}
	else
	{
		suggestedHeight += quickChatLabel->requiredHeight() + QCFORM_INTERNAL_PADDING;
	}
	int maxPanelHeight = 0;
	for (auto& panel : panels)
	{
		maxPanelHeight = std::max(maxPanelHeight, panel->idealHeight());
	}
	suggestedHeight += maxPanelHeight;
	return suggestedHeight;
}

void WzQuickChatForm::geometryChanged()
{
	recalcLayout();
}

void WzQuickChatForm::recalcLayout()
{
	quickChatLabel->callCalcLayout();
	int panelsTopY0 = QCFORM_INTERNAL_PADDING + quickChatLabel->requiredHeight() + QCFORM_INTERNAL_PADDING;
	if (panelSwitcher)
	{
		panelSwitcher->callCalcLayout();
		panelsTopY0 = std::max(panelsTopY0, panelSwitcher->y() + panelSwitcher->height() + QCFORM_INTERNAL_PADDING);
	}
	for (auto& panel : panels)
	{
		panel->setGeometry(0, panelsTopY0, width(), height() - panelsTopY0);
	}
}

std::shared_ptr<WzQuickChatForm> WzQuickChatForm::make(WzQuickChatContext context, optional<WzQuickChatMode> startingPanel /*= nullopt*/)
{
	auto result = std::make_shared<WzQuickChatForm>();
	result->initialize(context, startingPanel);
	return result;
}

void WzQuickChatForm::initialize(WzQuickChatContext context, optional<WzQuickChatMode> startingMode /*= nullopt*/)
{
	quickChatLabel = std::make_shared<W_LABEL>();
	quickChatLabel->setFont(font_regular, WZCOL_TEXT_MEDIUM);
	quickChatLabel->setString(_("Quick Chat:"));
	attach(quickChatLabel);
	quickChatLabel->setGeometry(QCFORM_INTERNAL_PADDING, QCFORM_INTERNAL_PADDING, quickChatLabel->getMaxLineWidth(), quickChatLabel->requiredHeight());
	quickChatLabel->setTransparentToMouse(true);

	switch (context)
	{
		case WzQuickChatContext::Lobby:
			initializeLobby();
			break;
		case WzQuickChatContext::InGame:
			initializeInGame(startingMode);
			break;
	}
	if (panels.size() > 1)
	{
		// Display a panel switcher
		panelSwitcher = std::make_shared<WzQuickChatPanelTabs>(0);
		attach(panelSwitcher);
		panelSwitcher->setButtonAlignment(MultibuttonWidget::ButtonAlignment::CENTER_ALIGN);
		size_t idxSelectedPanel = 0;
		for (size_t i = 0; i < panels.size(); ++i)
		{
			panelSwitcher->addButton(i, WzPanelTabButton::make(panels[i]->panelName));
			if (currentlyAttachedPanel == panels[i])
			{
				idxSelectedPanel = i;
			}
		}
		panelSwitcher->choose(idxSelectedPanel);
		panelSwitcher->addOnChooseHandler([](MultibuttonWidget& widget, int newValue){
			// Switch actively-displayed panel
			auto weakParent = std::weak_ptr<WIDGET>(widget.parent());
			widgScheduleTask([newValue, weakParent](){
				auto strongParent = std::dynamic_pointer_cast<WzQuickChatForm>(weakParent.lock());
				ASSERT_OR_RETURN(, strongParent != nullptr, "Parent doesn't exist?");
				strongParent->switchAttachedPanel(strongParent->panels[newValue]);
				// give the new panel focus
				if (strongParent->currentlyAttachedPanel)
				{
					strongParent->currentlyAttachedPanel->takeFocus();
				}
			});
		});
		panelSwitcher->setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
			auto psParent = std::dynamic_pointer_cast<WzQuickChatForm>(psWidget->parent());
			ASSERT_OR_RETURN(, psParent != nullptr, "No parent");
			int quickChatLabelWidth = psParent->quickChatLabel->width();
			int maxPanelSwitcherWidth = psParent->width() - ((QCFORM_INTERNAL_PADDING + quickChatLabelWidth) * 2);
			int panelSwitcherWidth = std::min(maxPanelSwitcherWidth, psWidget->idealWidth());
			int panelSwitcherX0 = (psParent->width() - panelSwitcherWidth) / 2;
			psWidget->setGeometry(panelSwitcherX0, QCFORM_INTERNAL_PADDING, panelSwitcherWidth, QCFORM_PANEL_TABS_HEIGHT);
		}));
	}

	recalcLayout();
}

void WzQuickChatForm::setIsHighlightedMode(bool childIsFocused)
{
	showHighlighted = childIsFocused;
	quickChatLabel->setFontColour((childIsFocused) ? WZCOL_TEXT_BRIGHT : WZCOL_TEXT_MEDIUM);
}

void WzQuickChatForm::initializeLobby()
{
	auto panel = WzQuickChatPanel::make(_("Lobby Chat"), WzQuickChatPanel::WzQuickChatSendToMode::GlobalSendOptions);

	auto requests = WzQuickChatSection::make(_("Requests"));
	requests->addChatOption(WzQuickChatMessage::LOBBY_DESIRE_TO_PLAY);					// I want to play
	requests->addChatOption(WzQuickChatMessage::LOBBY_DESIRE_TO_SPECTATE);				// I want to spectate
	requests->addChatOption(WzQuickChatMessage::LOBBY_DESIRE_TO_SWITCH_TEAMS);			// I want to switch teams
	requests->addChatOption(WzQuickChatMessage::LOBBY_REQUEST_CHECK_READY);				// Please check ready so we can start
	requests->addChatOption(WzQuickChatMessage::REQUEST_PLEASE_WAIT);					// Please wait
	requests->addChatOption(WzQuickChatMessage::REQUEST_LETS_GO);						// Let's go!
	panel->addSection(requests);

	// Notices
	auto notices = WzQuickChatSection::make(_("Notices"));
	notices->addChatOption(WzQuickChatMessage::NOTICE_ALMOST_READY);					// Almost ready
	notices->addChatOption(WzQuickChatMessage::NOTICE_READY);							// Ready
	notices->addChatOption(WzQuickChatMessage::NOTICE_BRB);								// Be right back
	notices->addChatOption(WzQuickChatMessage::NOTICE_I_AM_BACK);						// I'm back
	panel->addSection(notices);

	// Reactions
	auto reactions = WzQuickChatSection::make(_("Reactions"));
	reactions->addChatOption(WzQuickChatMessage::REACTIONS_YES);						// Yes
	reactions->addChatOption(WzQuickChatMessage::REACTIONS_NO);							// No
	reactions->addChatOption(WzQuickChatMessage::REACTIONS_MAYBE);						// Maybe
	reactions->addChatOption(WzQuickChatMessage::REACTIONS_NOT_YET);					// Not Yet
	reactions->addChatOption(WzQuickChatMessage::REACTIONS_SOON);						// Soon
	reactions->addChatOption(WzQuickChatMessage::REACTIONS_THANK_YOU);					// Thank you
	reactions->addChatOption(WzQuickChatMessage::REACTIONS_NO_PROBLEM);					// No problem
	reactions->addChatOption(WzQuickChatMessage::LOBBY_GREETING_WAVE);					// Hey everyone!
	reactions->addChatOption(WzQuickChatMessage::LOBBY_GREETING_WELCOME);				// Welcome!
	reactions->addChatOption(WzQuickChatMessage::REACTIONS_SORRY_DO_NOT_UNDERSTAND_PLEASE_USE_QUICK_CHAT);	// Sorry, I don't understand. Please use Quick Chat?
	panel->addSection(reactions);

	panels.push_back(panel);

	if (!currentlyAttachedPanel)
	{
		// default to global, which is always available
		switchAttachedPanel(panel);
	}
}

void WzQuickChatForm::switchAttachedPanel(const std::shared_ptr<WzQuickChatPanel> newPanel)
{
	if (currentlyAttachedPanel)
	{
		detach(currentlyAttachedPanel);
	}
	currentlyAttachedPanel = newPanel;
	attach(newPanel);
}

void WzQuickChatForm::initializeInGame(optional<WzQuickChatMode> startingMode)
{
	WzQuickChatMode firstVisiblePanel = startingMode.value_or(WzQuickChatMode::Global);

	// Initialize 1-3 panels
	auto playerTeammates = getPlayerTeammates(selectedPlayer, true, true);
	bool hasTeammates = !playerTeammates.empty();

	// Global Panel
	auto globalPanel = createInGameGlobalPanel();
	panels.push_back(globalPanel);
	if (firstVisiblePanel == WzQuickChatMode::Global)
	{
		switchAttachedPanel(globalPanel);
	}

	// Team Panel (only if we have teammates to send anything to!)
	if (hasTeammates)
	{
		auto teamPanel = createInGameTeamPanel(playerTeammates);
		panels.push_back(teamPanel);
		if (firstVisiblePanel == WzQuickChatMode::Team)
		{
			switchAttachedPanel(teamPanel);
		}
	}

//	// Cheats Panel (only if debug mode is enabled)
//	const DebugInputManager& dbgInputManager = gInputManager.debugManager();
//	if (dbgInputManager.debugMappingsAllowed())
//	{
//		auto cheatsPanel = createInGameCheatPanel();
//		panels.push_back(cheatsPanel);
//		if (firstVisiblePanel == WzQuickChatMode::Cheats)
//		{
//			switchAttachedPanel(cheatsPanel);
//		}
//	}

	// End-Game reactions panel
	auto endGamePanel = createInGameEndGamePanel();
	panels.push_back(endGamePanel);
	if (firstVisiblePanel == WzQuickChatMode::EndGame)
	{
		switchAttachedPanel(endGamePanel);
	}

	if (!currentlyAttachedPanel)
	{
		// default to global, which is always available
		switchAttachedPanel(globalPanel);
	}
}

std::shared_ptr<WzQuickChatPanel> WzQuickChatForm::createInGameGlobalPanel()
{
	auto panel = WzQuickChatPanel::make(_("Global"), WzQuickChatPanel::WzQuickChatSendToMode::GlobalSendOptions);

	auto requests = WzQuickChatSection::make(_("Requests"));
	requests->addChatOption(WzQuickChatMessage::REQUEST_PLEASE_WAIT);					// Please wait
	requests->addChatOption(WzQuickChatMessage::REQUEST_LETS_GO);						// Let's go!
	panel->addSection(requests);

	// Notices
	auto notices = WzQuickChatSection::make(_("Notices"));
	notices->addChatOption(WzQuickChatMessage::NOTICE_ALMOST_READY);					// Almost ready
	notices->addChatOption(WzQuickChatMessage::NOTICE_READY);							// Ready
	notices->addChatOption(WzQuickChatMessage::NOTICE_BRB);								// Be right back
	notices->addChatOption(WzQuickChatMessage::NOTICE_I_AM_BACK);						// I'm back
	panel->addSection(notices);

	// Taunts
	auto taunts = WzQuickChatSection::make(_("Taunts"));
	taunts->addChatOption(WzQuickChatMessage::REACTIONS_TAUNT_GET_READY);					// Get ready...
	taunts->addChatOption(WzQuickChatMessage::REACTIONS_TAUNT_YOURE_GOING_TO_REGRET_THAT);	// You're going to regret that
	taunts->addChatOption(WzQuickChatMessage::REACTIONS_TAUNT_BARELY_A_SCRATCH);			// Barely a scratch!
	panel->addSection(taunts);

	// Reactions
	auto reactions = WzQuickChatSection::make(_("Reactions"));
	reactions->addChatOption(WzQuickChatMessage::REACTIONS_YES);						// Yes
	reactions->addChatOption(WzQuickChatMessage::REACTIONS_NO);							// No
	reactions->addChatOption(WzQuickChatMessage::REACTIONS_MAYBE);						// Maybe
	reactions->addChatOption(WzQuickChatMessage::REACTIONS_NOT_YET);					// Not Yet
	reactions->addChatOption(WzQuickChatMessage::REACTIONS_SOON);						// Soon
	reactions->addChatOption(WzQuickChatMessage::REACTIONS_THANK_YOU);					// Thank you
	reactions->addChatOption(WzQuickChatMessage::REACTIONS_NO_PROBLEM);					// No problem
	reactions->addChatOption(WzQuickChatMessage::REACTIONS_WELL_PLAYED);				// Well-played
	reactions->addChatOption(WzQuickChatMessage::REACTIONS_CENSORED);					// @#%*!
	reactions->addChatOption(WzQuickChatMessage::REACTIONS_SORRY_DO_NOT_UNDERSTAND_PLEASE_USE_QUICK_CHAT);	// Sorry, I don't understand. Please use Quick Chat?
	panel->addSection(reactions);

	return panel;
}

std::shared_ptr<WzQuickChatPanel> WzQuickChatForm::createInGameTeamPanel(const std::vector<uint32_t>& playerTeammates)
{
	auto panel = WzQuickChatPanel::make(_("Team"), WzQuickChatPanel::WzQuickChatSendToMode::TeamSendOptions);

	// Coordination
	auto requests = WzQuickChatSection::make(_("Coordination"));
	requests->addChatOption(WzQuickChatMessage::TEAM_REQUEST_ATTACK_NOW);				// Attack now?
	requests->addChatOption(WzQuickChatMessage::TEAM_REQUEST_GROUP_UP);					// Group up
	requests->addChatOption(WzQuickChatMessage::TEAM_REQUEST_SPLIT_UP);					// Split up
	requests->addChatOption(WzQuickChatMessage::TEAM_REQUEST_FOCUS_ATTACKS_WHERE_MARKED);// Focus attacks where I've marked
	requests->addChatOption(WzQuickChatMessage::TEAM_REQUEST_RETREAT);					// Retreat!
	requests->addChatOption(WzQuickChatMessage::TEAM_REQUEST_NEED_HELP);					// I need help!
	requests->addChatOption(WzQuickChatMessage::REQUEST_PLEASE_WAIT);					// Please wait
	requests->addChatOption(WzQuickChatMessage::REQUEST_LETS_GO);						// Let's go!
	requests->addChatOption(WzQuickChatMessage::TEAM_REQUEST_LEFT_SIDE);					// Left side
	requests->addChatOption(WzQuickChatMessage::TEAM_REQUEST_RIGHT_SIDE);				// Right side
	requests->addChatOption(WzQuickChatMessage::TEAM_REQUEST_TRANSFER_UNITS);//**COND**	// Can someone please transfer me some units?
	requests->addChatOption(WzQuickChatMessage::TEAM_REQUEST_TRANSFER_REBUILD);//**COND**// Can someone please transfer me trucks so I can rebuild?
	panel->addSection(requests);

	// Notices
	auto notices = WzQuickChatSection::make(_("Notices"));
	notices->addChatOption(WzQuickChatMessage::NOTICE_ALMOST_READY);					// Almost ready
	notices->addChatOption(WzQuickChatMessage::NOTICE_READY);							// Ready
	notices->addChatOption(WzQuickChatMessage::NOTICE_BRB);								// Be right back
	notices->addChatOption(WzQuickChatMessage::NOTICE_I_AM_BACK);						// I'm back
	notices->addChatOption(WzQuickChatMessage::TEAM_NOTICE_ATTACKING_NOW);				// Attacking now
	notices->addChatOption(WzQuickChatMessage::TEAM_NOTICE_ON_MY_WAY);					// On my way!
	notices->addChatOption(WzQuickChatMessage::TEAM_NOTICE_OPPONENTS_COMING);			// They're coming!
	notices->addChatOption(WzQuickChatMessage::TEAM_NOTICE_BEING_ATTACKED);				// I'm being attacked!
	notices->addChatOption(WzQuickChatMessage::TEAM_NOTICE_RUSHING_OILS);				// I'm rushing oils
	notices->addChatOption(WzQuickChatMessage::TEAM_NOTICE_OPPONENTS_RUSHING_OILS);		// They're rushing oils!
	notices->addChatOption(WzQuickChatMessage::TEAM_NOTICE_DONT_HAVE_POWER);//**COND**	// I don't have enough power
	panel->addSection(notices);

	// Suggestions
	auto suggestions = WzQuickChatSection::make(_("Suggestions"));
	suggestions->addChatOption(WzQuickChatMessage::TEAM_SUGGESTION_BUILD_MORE_UNITS);		// I suggest building more units
	suggestions->addChatOption(WzQuickChatMessage::TEAM_SUGGESTION_BUILD_DIFFERENT_UNITS);	// I suggest building different units
	suggestions->addChatOption(WzQuickChatMessage::TEAM_SUGGESTION_CHECK_TEAM_STRATEGY);		// I suggest checking the team strategy table
	suggestions->addChatOption(WzQuickChatMessage::TEAM_SUGGESTION_RESEARCH_DIFFERENT_TECH);	// I suggest researching different tech
	suggestions->addChatOption(WzQuickChatMessage::TEAM_SUGGESTION_KEEP_RESEARCH_BUSY);		// Be sure to keep research centers busy
	suggestions->addChatOption(WzQuickChatMessage::TEAM_SUGGESTION_BUILD_ANTI_AIR);			// I suggest building anti-air
	suggestions->addChatOption(WzQuickChatMessage::TEAM_SUGGESTION_REPAIR_UNITS);			// I suggest repairing your units
	suggestions->addChatOption(WzQuickChatMessage::TEAM_SUGGESTION_BUILD_REPAIR_FACILITIES);	// I suggest building repair facilities
	suggestions->addChatOption(WzQuickChatMessage::TEAM_SUGGESTION_BUILD_POWER_GEN);			// I suggest building more power generators
	suggestions->addChatOption(WzQuickChatMessage::TEAM_SUGGESTION_BUILD_CAPTURE_OILS);		// I suggest capturing oil resources
	panel->addSection(suggestions);

	// Reactions
	auto reactions = WzQuickChatSection::make(_("Reactions"));
	reactions->addChatOption(WzQuickChatMessage::REACTIONS_YES);							// Yes
	reactions->addChatOption(WzQuickChatMessage::REACTIONS_NO);							// No
	reactions->addChatOption(WzQuickChatMessage::REACTIONS_MAYBE);						// Maybe
	reactions->addChatOption(WzQuickChatMessage::REACTIONS_NOT_YET);						// Not Yet
	reactions->addChatOption(WzQuickChatMessage::REACTIONS_SOON);						// Soon
	reactions->addChatOption(WzQuickChatMessage::REACTIONS_THANK_YOU);					// Thank you
	reactions->addChatOption(WzQuickChatMessage::REACTIONS_NO_PROBLEM);					// No problem
	reactions->addChatOption(WzQuickChatMessage::REACTIONS_WELL_PLAYED);					// Well-played
	reactions->addChatOption(WzQuickChatMessage::REACTIONS_CENSORED);					// @#%*!
	reactions->addChatOption(WzQuickChatMessage::REACTIONS_SORRY_DO_NOT_UNDERSTAND_PLEASE_USE_QUICK_CHAT);	// Sorry, I don't understand. Please use Quick Chat?
	reactions->addChatOption(WzQuickChatMessage::REACTIONS_TEAM_THAT_DIDNT_GO_WELL);		// That didn't go well...
	reactions->addChatOption(WzQuickChatMessage::REACTIONS_TEAM_I_HAVE_ANOTHER_PLAN);	// I have another plan
	panel->addSection(reactions);

	if (playerTeammates.size() > 1)
	{
		// Show Send-To options
		panel->showSendTo();
	}

	return panel;
}

std::shared_ptr<WzQuickChatPanel> WzQuickChatForm::createInGameCheatPanel()
{
	auto panel = WzQuickChatPanel::make(_("Cheats"), nullopt);

	// TODO: Implement

	return panel;
}

std::shared_ptr<WzQuickChatPanel> WzQuickChatForm::createInGameEndGamePanel()
{
	auto panel = WzQuickChatPanel::make(_("End-Game"), WzQuickChatPanel::WzQuickChatSendToMode::GlobalSendOptions);

	// End-Game Reactions
	auto endReactions = WzQuickChatSection::make(_("Reactions"));
	endReactions->addChatOption(WzQuickChatMessage::REACTIONS_ENDGAME_GOOD_GAME);			// Good game
	endReactions->addChatOption(WzQuickChatMessage::REACTIONS_ENDGAME_I_GIVE_UP);			// I give up
	endReactions->addChatOption(WzQuickChatMessage::REACTIONS_ENDGAME_SORRY_HAVE_TO_LEAVE);	// Sorry, I have to leave
	panel->addSection(endReactions);

	panel->setMinSectionsPerRowForLayout(4);
	return panel;
}

// MARK: -

namespace WzQuickChatDataContexts {

// - INTERNAL_ADMIN_ACTION_NOTICE
namespace INTERNAL_ADMIN_ACTION_NOTICE {
	WzQuickChatMessageData constructMessageData(Context ctx, uint32_t responsiblePlayerIdx, uint32_t targetPlayerIdx)
	{
		return WzQuickChatMessageData { static_cast<uint32_t>(ctx), responsiblePlayerIdx, targetPlayerIdx };
	}

	std::string to_output_string(WzQuickChatMessageData messageData)
	{
		uint32_t responsiblePlayerIdx = messageData.dataA;
		uint32_t targetPlayerIdx = messageData.dataB;

		if (responsiblePlayerIdx >= MAX_CONNECTED_PLAYERS)
		{
			return std::string();
		}
		if (targetPlayerIdx >= MAX_CONNECTED_PLAYERS)
		{
			return std::string();
		}

		const char* responsiblePlayerName = getPlayerName(responsiblePlayerIdx);
		const char* targetPlayerName = getPlayerName(targetPlayerIdx);

		const char* responsiblePlayerType = _("Player");
		if (responsiblePlayerIdx == NetPlay.hostPlayer)
		{
			responsiblePlayerType = _("Host");
		}
		else if (NetPlay.players[responsiblePlayerIdx].isAdmin)
		{
			responsiblePlayerType = _("Admin");
		}

		switch (messageData.dataContext)
		{
			case static_cast<uint32_t>(Context::Invalid):
				return "";
			case static_cast<uint32_t>(Context::Team):
				return astringf(_("%s (%s) changed team of player (%s) to: %d"), responsiblePlayerType, responsiblePlayerName, targetPlayerName, NetPlay.players[targetPlayerIdx].team);
			case static_cast<uint32_t>(Context::Position):
				return astringf(_("%s (%s) changed position of player (%s) to: %d"), responsiblePlayerType, responsiblePlayerName, targetPlayerName, NetPlay.players[targetPlayerIdx].position);
			case static_cast<uint32_t>(Context::Color):
				return astringf(_("%s (%s) changed color of player (%s) to: %s"), responsiblePlayerType, responsiblePlayerName, targetPlayerName, getPlayerColourName(targetPlayerIdx));
			case static_cast<uint32_t>(Context::Faction):
				return astringf(_("%s (%s) changed faction of player (%s) to: %s"), responsiblePlayerType, responsiblePlayerName, targetPlayerName, to_localized_string(static_cast<FactionID>(NetPlay.players[targetPlayerIdx].faction)));
		}

		return ""; // Silence compiler warning
	}
} // namespace INTERNAL_ADMIN_ACTION_NOTICE

// - INTERNAL_LOCALIZED_LOBBY_NOTICE
namespace INTERNAL_LOCALIZED_LOBBY_NOTICE {
	WzQuickChatMessageData constructMessageData(Context ctx, uint32_t targetPlayerIdx, uint32_t additionalData)
	{
		return WzQuickChatMessageData { static_cast<uint32_t>(ctx), additionalData, targetPlayerIdx };
	}

	std::string to_output_string(WzQuickChatMessageData messageData)
	{
		uint32_t additionalData = messageData.dataA;
		uint32_t targetPlayerIdx = messageData.dataB;

		if (targetPlayerIdx >= MAX_CONNECTED_PLAYERS)
		{
			return std::string();
		}

		const char* targetPlayerName = getPlayerName(targetPlayerIdx);

		switch (messageData.dataContext)
		{
			case static_cast<uint32_t>(Context::Invalid):
				return "";
			case static_cast<uint32_t>(Context::NotReadyKickWarning):
				if (targetPlayerIdx == selectedPlayer)
				{
					audio_PlayTrack(ID_SOUND_ZOOM_ON_RADAR);
					return _("NOTICE: If you don't check Ready soon, you will be kicked from the room");
				}
				else
				{
					// Not intended for this player, but show a single message when 5 seconds remaining
					// so other players know a kick is coming
					if (additionalData == 5 && !isBlindSimpleLobby(game.blindMode))
					{
						return astringf(_("Player will be kicked if they don't check Ready soon: %s"), targetPlayerName);
					}
					return "";
				}
			case static_cast<uint32_t>(Context::NotReadyKicked):
				return astringf(_("Auto-kicking player (%s) because they waited too long to check Ready"), targetPlayerName);
			case static_cast<uint32_t>(Context::PlayerShouldCheckReadyNotice):
			{
				audio_PlayTrack(ID_SOUND_ZOOM_ON_RADAR);
				std::string result;
				if (isBlindSimpleLobby(game.blindMode))
				{
					result = _("NOTICE: Please check Ready");
				}
				else
				{
					result = _("NOTICE: Please check Ready so the game can begin");
				}
				if (additionalData > 0)
				{
					// auto-not-ready-kick is enabled
					result += "\n";
					result += _("Players who don't check Ready in time will be kicked.");
				}
				return result;
			}
		}

		return ""; // Silence compiler warning
	}
} // namespace INTERNAL_LOCALIZED_LOBBY_NOTICE

} // namespace WzQuickChatDataContexts

// MARK: - Public functions

bool quickChatMessageExpectsExtraData(WzQuickChatMessage msg)
{
	switch (msg)
	{
		// WZ-generated internal messages which require extra data
		case WzQuickChatMessage::INTERNAL_ADMIN_ACTION_NOTICE:
		case WzQuickChatMessage::INTERNAL_LOCALIZED_LOBBY_NOTICE:
			return true;

		default:
			return false;
	}
}

int32_t to_output_sender(WzQuickChatMessage msg, uint32_t sender)
{
	// Certain internal messages override the sender for display purposes
	switch (msg)
	{
		// WZ-generated internal messages - not for users to deliberately send
		case WzQuickChatMessage::INTERNAL_ADMIN_ACTION_NOTICE:
			// override the "sender" to SYSTEM_MESSAGE type
			return SYSTEM_MESSAGE;
		case WzQuickChatMessage::INTERNAL_LOCALIZED_LOBBY_NOTICE:
			return NOTIFY_MESSAGE;

		default:
			return sender;
	}
}

std::string to_output_string(WzQuickChatMessage msg, const optional<WzQuickChatMessageData>& messageData)
{
	switch (msg)
	{
		// These messages have a long form
		case WzQuickChatMessage::TEAM_REQUEST_TRANSFER_UNITS:
			return _("Can someone please transfer me some units?");
		case WzQuickChatMessage::TEAM_REQUEST_TRANSFER_REBUILD:
			return _("Can someone please transfer me trucks so I can rebuild?");
		case WzQuickChatMessage::REACTIONS_SORRY_DO_NOT_UNDERSTAND_PLEASE_USE_QUICK_CHAT:
			return _("Sorry, I don't understand. (Please use Quick Chat?)");

		// All suggestions use the "I suggest: " prefix
		case WzQuickChatMessage::TEAM_SUGGESTION_BUILD_MORE_UNITS:
			// TRANSLATORS: A suggestion to other player(s)
			return _("I suggest: Building more units");
		case WzQuickChatMessage::TEAM_SUGGESTION_BUILD_DIFFERENT_UNITS:
			// TRANSLATORS: A suggestion to other player(s)
			return _("I suggest: Building different units");
		case WzQuickChatMessage::TEAM_SUGGESTION_CHECK_TEAM_STRATEGY:
			// TRANSLATORS: A suggestion to other player(s)
			return _("I suggest: Checking the team strategy view");
		case WzQuickChatMessage::TEAM_SUGGESTION_RESEARCH_DIFFERENT_TECH:
			// TRANSLATORS: A suggestion to other player(s)
			return _("I suggest: Researching different tech");
		case WzQuickChatMessage::TEAM_SUGGESTION_KEEP_RESEARCH_BUSY:
			// TRANSLATORS: A suggestion to other player(s)
			return _("I suggest: Keeping research centers busy");
		case WzQuickChatMessage::TEAM_SUGGESTION_BUILD_ANTI_AIR:
			// TRANSLATORS: A suggestion to other player(s)
			return _("I suggest: Building anti-air");
		case WzQuickChatMessage::TEAM_SUGGESTION_REPAIR_UNITS:
			// TRANSLATORS: A suggestion to other player(s)
			return _("I suggest: Repairing your units");
		case WzQuickChatMessage::TEAM_SUGGESTION_BUILD_REPAIR_FACILITIES:
			// TRANSLATORS: A suggestion to other player(s)
			return _("I suggest: Building repair facilities");
		case WzQuickChatMessage::TEAM_SUGGESTION_BUILD_POWER_GEN:
			// TRANSLATORS: A suggestion to other player(s)
			return _("I suggest: Building power generators");
		case WzQuickChatMessage::TEAM_SUGGESTION_BUILD_CAPTURE_OILS:
			// TRANSLATORS: A suggestion to other player(s)
			return _("I suggest: Capturing oil resources");

		// WZ-generated internal messages - not for users to deliberately send
		case WzQuickChatMessage::INTERNAL_ADMIN_ACTION_NOTICE:
			return WzQuickChatDataContexts::INTERNAL_ADMIN_ACTION_NOTICE::to_output_string(messageData.value());
		case WzQuickChatMessage::INTERNAL_LOCALIZED_LOBBY_NOTICE:
			return WzQuickChatDataContexts::INTERNAL_LOCALIZED_LOBBY_NOTICE::to_output_string(messageData.value());

		default:
			return to_display_string(msg);
	}
	return ""; // Silence compiler warning
}

const char* to_display_string(WzQuickChatMessage msg)
{
	switch (msg)
	{
		// LOBBY ONLY
		case WzQuickChatMessage::LOBBY_GREETING_WAVE:
			return _("Hey everyone!");
		case WzQuickChatMessage::LOBBY_GREETING_WELCOME:
			return _("Welcome!");
		case WzQuickChatMessage::LOBBY_DESIRE_TO_SPECTATE:
			return _("I want to spectate");
		case WzQuickChatMessage::LOBBY_DESIRE_TO_PLAY:
			return _("I want to play");
		case WzQuickChatMessage::LOBBY_DESIRE_TO_SWITCH_TEAMS:
			return _("I want to switch teams");
		case WzQuickChatMessage::LOBBY_REQUEST_CHECK_READY:
			return _("Please check Ready so we can start");

		// ALL (lobby + in-game global *and* team)

		case WzQuickChatMessage::REQUEST_PLEASE_WAIT:
			return _("Please wait");
		case WzQuickChatMessage::REQUEST_LETS_GO:
			return _("Let's go!");

		case WzQuickChatMessage::NOTICE_ALMOST_READY:
			// TRANSLATORS: As in "I am almost ready" (but shorter, more informal)
			return _("Almost ready");
		case WzQuickChatMessage::NOTICE_READY:
			// TRANSLATORS: As in "I am ready" (but shorter, more informal)
			return _("Ready");
		case WzQuickChatMessage::NOTICE_BRB:
			return _("Be right back");
		case WzQuickChatMessage::NOTICE_I_AM_BACK:
			return _("I'm back");

		case WzQuickChatMessage::REACTIONS_YES:
			return _("Yes");
		case WzQuickChatMessage::REACTIONS_NO:
			return _("No");
		case WzQuickChatMessage::REACTIONS_MAYBE:
			return _("Maybe");
		case WzQuickChatMessage::REACTIONS_NOT_YET:
			return _("Not yet");
		case WzQuickChatMessage::REACTIONS_SOON:
			return _("Soon");
		case WzQuickChatMessage::REACTIONS_THANK_YOU:
			return _("Thank you");
		case WzQuickChatMessage::REACTIONS_NO_PROBLEM:
			return _("No problem");

		// In-game reactions (global + team)
		case WzQuickChatMessage::REACTIONS_WELL_PLAYED:
			return _("Well-played");
		case WzQuickChatMessage::REACTIONS_CENSORED:
			// TRANSLATORS: Should probably be left as-is, unless there is a better way of denoting a censored outburst in your language
			return _("@#%*!");
		case WzQuickChatMessage::REACTIONS_SORRY_DO_NOT_UNDERSTAND_PLEASE_USE_QUICK_CHAT:
			return _("Sorry, I don't understand. (Use Quick Chat?)");

		// Global reactions (taunts)
		case WzQuickChatMessage::REACTIONS_TAUNT_GET_READY:
			return _("Get ready...");
		case WzQuickChatMessage::REACTIONS_TAUNT_YOURE_GOING_TO_REGRET_THAT:
			return _("You're going to regret that");
		case WzQuickChatMessage::REACTIONS_TAUNT_BARELY_A_SCRATCH:
			return _("Barely a scratch!");

		// Team communication
		case WzQuickChatMessage::TEAM_REQUEST_ATTACK_NOW:
			return _("Attack now?");
		case WzQuickChatMessage::TEAM_REQUEST_GROUP_UP:
			return _("Group up");
		case WzQuickChatMessage::TEAM_REQUEST_SPLIT_UP:
			return _("Split up");
		case WzQuickChatMessage::TEAM_REQUEST_FOCUS_ATTACKS_WHERE_MARKED:
			return _("Focus attacks where marked");
		case WzQuickChatMessage::TEAM_REQUEST_RETREAT:
			return _("Retreat!");
		case WzQuickChatMessage::TEAM_REQUEST_NEED_HELP:
			return _("I need help!");
		case WzQuickChatMessage::TEAM_REQUEST_LEFT_SIDE:
			return _("Left side");
		case WzQuickChatMessage::TEAM_REQUEST_RIGHT_SIDE:
			return _("Right side");
		case WzQuickChatMessage::TEAM_REQUEST_TRANSFER_UNITS:
			return _("Transfer Request: Units");
		case WzQuickChatMessage::TEAM_REQUEST_TRANSFER_REBUILD:
			return _("Transfer Request: Trucks");

		case WzQuickChatMessage::TEAM_NOTICE_ATTACKING_NOW:
			// TRANSLATORS: As in "I am attacking now!" (but shorter, more informal)
			return _("Attacking now!");
		case WzQuickChatMessage::TEAM_NOTICE_ON_MY_WAY:
			// TRANSLATORS: As in "I am on my way!" (but shorter, more informal)
			return _("On my way!");
		case WzQuickChatMessage::TEAM_NOTICE_OPPONENTS_COMING:
			return _("They're coming!");
		case WzQuickChatMessage::TEAM_NOTICE_BEING_ATTACKED:
			return _("I'm being attacked!");
		case WzQuickChatMessage::TEAM_NOTICE_RUSHING_OILS:
			return _("I'm rushing oils");
		case WzQuickChatMessage::TEAM_NOTICE_OPPONENTS_RUSHING_OILS:
			return _("They're rushing oils!");
		case WzQuickChatMessage::TEAM_NOTICE_DONT_HAVE_POWER:
			return _("I don't have enough power");

		case WzQuickChatMessage::TEAM_SUGGESTION_BUILD_MORE_UNITS:
			// TRANSLATORS: A suggestion to other player(s), as in "Suggestion: %s"
			return _("Build more units");
		case WzQuickChatMessage::TEAM_SUGGESTION_BUILD_DIFFERENT_UNITS:
			// TRANSLATORS: A suggestion to other player(s), as in "Suggestion: %s"
			return _("Build different units");
		case WzQuickChatMessage::TEAM_SUGGESTION_CHECK_TEAM_STRATEGY:
			// TRANSLATORS: A suggestion to other player(s), as in "Suggestion: %s"
			return _("Check team strategy view");
		case WzQuickChatMessage::TEAM_SUGGESTION_RESEARCH_DIFFERENT_TECH:
			// TRANSLATORS: A suggestion to other player(s), as in "Suggestion: %s"
			return _("Research different tech");
		case WzQuickChatMessage::TEAM_SUGGESTION_KEEP_RESEARCH_BUSY:
			// TRANSLATORS: A suggestion to other player(s), as in "Suggestion: %s"
			return _("Keep research centers busy");
		case WzQuickChatMessage::TEAM_SUGGESTION_BUILD_ANTI_AIR:
			// TRANSLATORS: A suggestion to other player(s), as in "Suggestion: %s"
			return _("Build anti-air");
		case WzQuickChatMessage::TEAM_SUGGESTION_REPAIR_UNITS:
			// TRANSLATORS: A suggestion to other player(s), as in "Suggestion: %s"
			return _("Repair your units");
		case WzQuickChatMessage::TEAM_SUGGESTION_BUILD_REPAIR_FACILITIES:
			// TRANSLATORS: A suggestion to other player(s), as in "Suggestion: %s"
			return _("Build repair facilities");
		case WzQuickChatMessage::TEAM_SUGGESTION_BUILD_POWER_GEN:
			// TRANSLATORS: A suggestion to other player(s), as in "Suggestion: %s"
			return _("Build power generators");
		case WzQuickChatMessage::TEAM_SUGGESTION_BUILD_CAPTURE_OILS:
			// TRANSLATORS: A suggestion to other player(s), as in "Suggestion: %s"
			return _("Capture oil resources");

		// Team reactions
		case WzQuickChatMessage::REACTIONS_TEAM_THAT_DIDNT_GO_WELL:
			return _("That didn't go well...");
		case WzQuickChatMessage::REACTIONS_TEAM_I_HAVE_ANOTHER_PLAN:
			return _("I have another plan");

		// End game reactions
		case WzQuickChatMessage::REACTIONS_ENDGAME_GOOD_GAME:
			return _("Good game");
		case WzQuickChatMessage::REACTIONS_ENDGAME_I_GIVE_UP:
			return _("I give up");
		case WzQuickChatMessage::REACTIONS_ENDGAME_SORRY_HAVE_TO_LEAVE:
			return _("Sorry, I have to leave");

		// WZ-generated internal messages - not for users to deliberately send
		case WzQuickChatMessage::INTERNAL_MSG_DELIVERY_FAILURE_TRY_AGAIN:
			return _("Message delivery failure - try again");
		case WzQuickChatMessage::INTERNAL_LOBBY_NOTICE_MAP_DOWNLOADED:
			return _("Map Downloaded");
		case WzQuickChatMessage::INTERNAL_ADMIN_ACTION_NOTICE:
			return _("Admin modified a setting");
		case WzQuickChatMessage::INTERNAL_LOCALIZED_LOBBY_NOTICE:
			return "";

		// not a valid message
		case WzQuickChatMessage::MESSAGE_COUNT:
			ASSERT(false, "Invalid message");
			return "";
	}

	return "";
}

std::shared_ptr<W_FORM> createQuickChatForm(WzQuickChatContext context, const std::function<void ()>& onQuickChatSent, optional<WzQuickChatMode> startingPanel /*= nullopt*/)
{
	auto result = WzQuickChatForm::make(context, startingPanel);
	result->onQuickChatSent = onQuickChatSent;
	return result;
}

bool to_WzQuickChatMessage(uint32_t value, WzQuickChatMessage& output)
{
	if (value < static_cast<uint32_t>(WzQuickChatMessage::MESSAGE_COUNT))
	{
		output = static_cast<WzQuickChatMessage>(value);
		return true;
	}
	return false;
}

std::unordered_map<std::string, uint32_t> getWzQuickChatMessageValueMap()
{
	std::unordered_map<std::string, uint32_t> result;
	#define QC_ENUM_ADD_TO_MAP(ENUM) result[#ENUM] = static_cast<uint32_t>(WzQuickChatMessage::ENUM);
	FOREACH_QUICKCHATMSG(QC_ENUM_ADD_TO_MAP)

	// filter out anything starting with "INTERNAL_"
	for (auto it = result.begin(); it != result.end(); /* no increment */)
	{
		if (it->first.rfind("INTERNAL_", 0) == 0)
		{
			it = result.erase(it);
		}
		else
		{
			it++;
		}
	}

	return result;
}

void quickChatInitInGame()
{
	if (NetPlay.bComms)
	{
		// for true multiplayer games, always default team targeting to human allies
		defaultTeamChatTargeting = WzQuickChatTargeting();
		defaultTeamChatTargeting.humanTeammates = true;
	}
	else
	{
		// for local skirmish, always default team targeting to computer allies
		defaultTeamChatTargeting = WzQuickChatTargeting();
		defaultTeamChatTargeting.aiTeammates = true;
	}

	lastTeamChatTargeting = defaultTeamChatTargeting;

	for (auto& playerMessageTimes : lastQuickChatMessageTimes)
	{
		playerMessageTimes.clear();
	}
}

static bool playerHasHumanAndAITeammates(uint32_t playerIdx)
{
	ASSERT_OR_RETURN(false, playerIdx < MAX_CONNECTED_PLAYERS, "Invalid player: %" PRIu32, playerIdx);
	auto playerTeammates = getPlayerTeammates(playerIdx, true, true);
	if (playerTeammates.empty())
	{
		return false;
	}
	bool firstIsOrWasHuman = isHumanPlayer(playerTeammates.front()) // is an active (connected) human player
							|| NetPlay.players[playerTeammates.front()].difficulty == AIDifficulty::HUMAN; // was a human player (probably disconnected);
	return std::any_of(std::next(playerTeammates.begin()), playerTeammates.end(), [firstIsOrWasHuman](uint32_t playerIdx) -> bool {
		bool thisIsOrWasHuman = isHumanPlayer(playerIdx) // is an active (connected) human player
								|| NetPlay.players[playerIdx].difficulty == AIDifficulty::HUMAN; // was a human player (probably disconnected);
		return thisIsOrWasHuman != firstIsOrWasHuman;
	});
}

static std::string formatReceivers(uint32_t senderIdx, const WzQuickChatTargeting& targeting)
{
	if (targeting.all)
	{
		return _("Global");
	}

	bool toAllies = (targeting.aiTeammates || targeting.humanTeammates);

	auto alliesStr = [&]() -> const char *
	{
		if (targeting.aiTeammates && targeting.humanTeammates)
		{
			return _("Allies");
		}
		if (targeting.aiTeammates)
		{
			// only need to distinguish if the sending player has both human and AI teammates
			if (playerHasHumanAndAITeammates(senderIdx))
			{
				return _("Bot Allies");
			}
			else
			{
				return _("Allies");
			}
		}
		if (targeting.humanTeammates)
		{
			// only need to distinguish if the sending player has both human and AI teammates
			if (playerHasHumanAndAITeammates(senderIdx))
			{
				return _("Human Allies");
			}
			else
			{
				return _("Allies");
			}
		}
		return nullptr;
	};

	if (toAllies && targeting.specificPlayers.empty()) {
		auto pRetString = alliesStr();
		return (pRetString) ? pRetString : "";
	}

	if (targeting.specificPlayers.empty())
	{
		return "";
	}

	auto directs = targeting.specificPlayers.begin();
	std::stringstream ss;
	if (targeting.aiTeammates || targeting.humanTeammates) {
		auto pRetString = alliesStr();
		ss << pRetString;
		ss << " & ";
	} else {
		ss << _("private to ");
		ss << getPlayerName(*directs++);
	}

	while (directs != targeting.specificPlayers.end())
	{
		auto nextName = getPlayerName(*directs++);
		if (!nextName)
		{
			continue;
		}
		ss << (directs == targeting.specificPlayers.end() ? _(" and ") : ", ");
		ss << nextName;
	}

	return ss.str();
}

void addQuickChatMessageToConsole(WzQuickChatMessage message, uint32_t sender, const WzQuickChatTargeting& targeting, const optional<WzQuickChatMessageData>& messageData)
{
	char formatted[MAX_CONSOLE_STRING_LENGTH];
	ssprintf(formatted, "[%s] %s (%s): %s", formatLocalDateTime("%H:%M").c_str(), getPlayerName(sender), formatReceivers(sender, targeting).c_str(), to_output_string(message, messageData).c_str());
	bool teamSpecific = !targeting.all && (targeting.humanTeammates || targeting.aiTeammates);
	addConsoleMessage(formatted, DEFAULT_JUSTIFY, to_output_sender(message, sender), teamSpecific);
}

void addLobbyQuickChatMessageToConsole(WzQuickChatMessage message, uint32_t sender, const WzQuickChatTargeting& targeting, const optional<WzQuickChatMessageData>& messageData)
{
	bool teamSpecific = !targeting.all && (targeting.humanTeammates || targeting.aiTeammates);
	auto outputMsg = to_output_string(message, messageData);
	if (outputMsg.empty())
	{
		return;
	}
	addConsoleMessage(outputMsg.c_str(), DEFAULT_JUSTIFY, to_output_sender(message, sender), teamSpecific);
}

bool shouldHideQuickChatMessageFromLocalDisplay(WzQuickChatMessage message)
{
	if (!isInternalMessage(message))
	{
		return false;
	}
	// special cases for internal messages that should be displayed locally
	switch (message)
	{
		case WzQuickChatMessage::INTERNAL_ADMIN_ACTION_NOTICE:
			return false;
		default:
			break;
	}

	// hide local display of this message
	return true;
}

void sendQuickChat(WzQuickChatMessage message, uint32_t fromPlayer, WzQuickChatTargeting targeting, optional<WzQuickChatMessageData> messageData /*= nullopt*/)
{
	ASSERT_OR_RETURN(, myResponsibility(fromPlayer), "We are not responsible for player: %" PRIu32, fromPlayer);
	ASSERT_OR_RETURN(, quickChatMessageExpectsExtraData(message) == messageData.has_value(), "Message [%d] unexpectedly %s", static_cast<int>(message), messageData.has_value() ? "has message data" : "lacks message data");

	bool internalMessage = isInternalMessage(message);

	auto mutedUntil = playerSpamMutedUntil(fromPlayer);
	if (mutedUntil.has_value() && !internalMessage)
	{
		auto currentTime = std::chrono::steady_clock::now();
		auto duration_until_send_allowed = std::chrono::duration_cast<std::chrono::seconds>(mutedUntil.value() - currentTime).count();
		auto duration_timeout_message = astringf(_("You have sent too many messages in the last few seconds. Please wait and try again."), static_cast<unsigned>(duration_until_send_allowed));
		addConsoleMessage(duration_timeout_message.c_str(), DEFAULT_JUSTIFY, INFO_MESSAGE, false, static_cast<UDWORD>(std::chrono::duration_cast<std::chrono::milliseconds>(mutedUntil.value() - currentTime).count()));
		return;
	}

	bool isInGame = (GetGameMode() == GS_NORMAL);
	auto senderTeam = checkedGetPlayerTeam(fromPlayer);
	bool senderIsSpectator = NetPlay.players[fromPlayer].isSpectator;
	bool senderCanUseSecuredMessages = realSelectedPlayer == NetPlay.hostPlayer || realSelectedPlayer < MAX_PLAYERS; // non-host spectator slots don't currently support / send secured messages

	auto isPotentiallyValidTarget = [&](uint32_t playerIdx) -> bool {
		if (playerIdx == fromPlayer)
		{
			return false;
		}

		if (isInGame && senderIsSpectator && !NetPlay.players[playerIdx].isSpectator)
		{
			return false;
		}

		if (NetPlay.players[playerIdx].allocated)
		{
			// human player that's connected
			return true;
		}
		else if (isInGame
				 && playerIdx < game.maxPlayers
				 && NetPlay.players[playerIdx].ai >= 0
				 && NetPlay.players[playerIdx].difficulty != AIDifficulty::DISABLED
				 && !NetPlay.players[playerIdx].isSpectator)
		{
			// AI player that's still in the game
			return true;
		}
		else
		{
			// not a valid target player
			return false;
		}
	};

	// build a list of players to whom this message should be sent
	std::unordered_set<uint32_t> recipients;
	for (auto playerIdx : targeting.specificPlayers)
	{
		if (isPotentiallyValidTarget(playerIdx))
		{
			recipients.insert(playerIdx);
		}
	}

	for (uint32_t playerIdx = 0; playerIdx < MAX_CONNECTED_PLAYERS; ++playerIdx)
	{
		if (!isPotentiallyValidTarget(playerIdx))
		{
			continue;
		}

		if (NetPlay.players[playerIdx].allocated)
		{
			// human player that's connected
			if (targeting.all
				|| (targeting.humanTeammates && checkedGetPlayerTeam(playerIdx) == senderTeam)
				)
			{
				recipients.insert(playerIdx);
				continue;
			}
		}
		else if (playerIdx < game.maxPlayers && NetPlay.players[playerIdx].ai >= 0 && NetPlay.players[playerIdx].difficulty != AIDifficulty::DISABLED && !NetPlay.players[playerIdx].isSpectator)
		{
			// AI player that's still in the game
			if (targeting.all
				|| (targeting.aiTeammates && checkedGetPlayerTeam(playerIdx) == senderTeam)
				)
			{
				recipients.insert(playerIdx);
				continue;
			}
		}
	}

	uint32_t messageValue = static_cast<uint32_t>(message);
	bool teamSpecific = !targeting.all && (targeting.humanTeammates || targeting.aiTeammates);

	for (auto recipient : recipients)
	{
		if (myResponsibility(recipient))
		{
			// don't bother sending a net message
			if (recipient == selectedPlayer)
			{
				// Targetted at us - Output to console
				addQuickChatMessageToConsole(message, fromPlayer, targeting, messageData);
			}
			else
			{
				// this is presumably a host sending a message to an AI
				if (!(NetPlay.players[recipient].ai >= 0))
				{
					ASSERT(false, "Not an AI??");
					continue;
				}
				ASSERT(NetPlay.isHost, "Not the host?");
				triggerEventQuickChatMessage(fromPlayer, recipient, message, teamSpecific);
			}
			continue;
		}

		NETQUEUE queue = NETnetQueue((recipient < MAX_PLAYERS) ? whosResponsible(recipient) : recipient);
		bool sendSecured = isInGame && (queue.index == NetPlay.hostPlayer || queue.index < MAX_PLAYERS) && senderCanUseSecuredMessages;
		optional<MessageWriter> w;
		if (sendSecured)
		{
			w = NETbeginEncodeSecured(queue, NET_QUICK_CHAT_MSG);
			if (!w)
			{
				debug(LOG_NET, "Failed to encode secured message for queue.index: %" PRIu32, static_cast<uint32_t>(queue.index));
				continue;
			}
		}
		else
		{
			w = NETbeginEncode(queue, NET_QUICK_CHAT_MSG);
		}
		auto& wref = *w;

		NETuint32_t(wref, fromPlayer);
		NETuint32_t(wref, recipient);
		NETuint32_t(wref, messageValue);
		// send targeting structure
		NETbool(wref, targeting.all);
		NETbool(wref, targeting.humanTeammates);
		NETbool(wref, targeting.aiTeammates);
		uint32_t numSpecificRecipients = static_cast<uint32_t>(targeting.specificPlayers.size());
		NETuint32_t(wref, numSpecificRecipients);
		for (auto playerIdx : targeting.specificPlayers)
		{
			NETuint32_t(wref, playerIdx);
		}
		if (quickChatMessageExpectsExtraData(message))
		{
			if (messageData.has_value())
			{
				NETuint32_t(wref, messageData.value().dataContext);
				NETuint32_t(wref, messageData.value().dataA);
				NETuint32_t(wref, messageData.value().dataB);
			}
		}
		NETend(wref);
	}

	if (fromPlayer == selectedPlayer && (!recipients.empty() || !isInGame) && !shouldHideQuickChatMessageFromLocalDisplay(message))
	{
		if (isInGame)
		{
			addQuickChatMessageToConsole(message, fromPlayer, targeting, messageData);
		}
		else
		{
			addLobbyQuickChatMessageToConsole(message, fromPlayer, targeting, messageData);
		}
	}

	if (!recipients.empty() && !internalMessage)
	{
		recordPlayerMessageSent(fromPlayer);
	}
}

bool shouldProcessQuickChatMessage(const NETQUEUE& queue, bool isInGame, WzQuickChatMessage message, uint32_t sender, uint32_t recipient, const WzQuickChatTargeting& targeting, const optional<WzQuickChatMessageData>& messageData)
{
	bool internalMessage = isInternalMessage(message);

	if (sender >= MAX_CONNECTED_PLAYERS || recipient >= MAX_CONNECTED_PLAYERS)
	{
		return false;
	}

	if (whosResponsible(sender) != queue.index)
	{
		return false;
	}

	if (!myResponsibility(recipient))
	{
		return false;
	}

	bool senderIsSpectator = NetPlay.players[sender].isSpectator;
	if (isInGame && senderIsSpectator && !NetPlay.players[recipient].isSpectator)
	{
		// spectators can't talk to players in-game
		return false;
	}

	auto senderSpamMute = playerSpamMutedUntil(sender);
	if (senderSpamMute.has_value() && !internalMessage)
	{
		// ignore message sent while player send was throttled
		return false;
	}

	// begin: message-specific checks
	switch (message)
	{
		case WzQuickChatMessage::INTERNAL_ADMIN_ACTION_NOTICE:
		case WzQuickChatMessage::INTERNAL_LOCALIZED_LOBBY_NOTICE:
			// should only ever be sent by the host
			if (queue.index != NetPlay.hostPlayer)
			{
				return false;
			}
			break;
		default:
			// no special handling
			break;
	}
	// end: message-specific checks

	// passes checks
	return true;
}

bool recvQuickChat(NETQUEUE queue)
{
	bool isInGame = (GetGameMode() == GS_NORMAL);
	bool senderCanUseSecuredMessages = queue.index == NetPlay.hostPlayer || queue.index < MAX_PLAYERS;
	bool expectingSecuredMessage = isInGame && (realSelectedPlayer == NetPlay.hostPlayer || realSelectedPlayer < MAX_PLAYERS) && senderCanUseSecuredMessages; // spectator slots do not expect secured messages

	uint32_t sender = MAX_CONNECTED_PLAYERS;
	uint32_t recipient = MAX_CONNECTED_PLAYERS;
	uint32_t messageValue = static_cast<uint32_t>(WzQuickChatMessage::MESSAGE_COUNT);
	WzQuickChatTargeting targeting;
	optional<WzQuickChatMessageData> messageData;

	WzQuickChatMessage msgEnumVal;

	optional<MessageReader> r;
	if (expectingSecuredMessage)
	{
		r = NETbeginDecodeSecured(queue, NET_QUICK_CHAT_MSG);
		if (!r)
		{
			return false;
		}
	}
	else
	{
		r = NETbeginDecode(queue, NET_QUICK_CHAT_MSG);
	}
	auto& rref = *r;
	NETuint32_t(rref, sender);
	NETuint32_t(rref, recipient);
	NETuint32_t(rref, messageValue);
	// receive targeting structure
	NETbool(rref, targeting.all);
	NETbool(rref, targeting.humanTeammates);
	NETbool(rref, targeting.aiTeammates);
	uint32_t numSpecificRecipients = 0;
	NETuint32_t(rref, numSpecificRecipients);
	for (uint32_t i = 0; i < numSpecificRecipients; ++i)
	{
		uint32_t tmp_playerIdx = std::numeric_limits<uint32_t>::max();
		NETuint32_t(rref, tmp_playerIdx);
		if (tmp_playerIdx < MAX_CONNECTED_PLAYERS)
		{
			targeting.specificPlayers.insert(tmp_playerIdx);
		}
	}
	bool validMessageEnumValue = to_WzQuickChatMessage(messageValue, msgEnumVal);
	if (validMessageEnumValue && quickChatMessageExpectsExtraData(msgEnumVal))
	{
		messageData = WzQuickChatMessageData();
		NETuint32_t(rref, messageData.value().dataContext);
		NETuint32_t(rref, messageData.value().dataA);
		NETuint32_t(rref, messageData.value().dataB);
	}
	NETend(rref);

	if (!validMessageEnumValue)
	{
		return false;
	}

	if (!shouldProcessQuickChatMessage(queue, isInGame, msgEnumVal, sender, recipient, targeting, messageData))
	{
		return false;
	}

	bool internalMessage = isInternalMessage(msgEnumVal);
	if (!internalMessage && recipient == selectedPlayer)
	{
		recordPlayerMessageSent(sender);
	}

	if (recipient == selectedPlayer || recipient == realSelectedPlayer)
	{
		// Output to the console
		if (isInGame)
		{
			addQuickChatMessageToConsole(msgEnumVal, sender, targeting, messageData);
			audio_PlayTrack(ID_SOUND_MESSAGEEND);
		}
		else
		{
			addLobbyQuickChatMessageToConsole(msgEnumVal, sender, targeting, messageData);
			audio_PlayTrack(FE_AUDIO_MESSAGEEND);
		}
	}
	else
	{
		if (!isInGame)
		{
			// silently discard messages to AIs when not in the game
			return true;
		}

		ASSERT_OR_RETURN(false, NetPlay.players[recipient].ai >= 0, "Recipient is not an AI?: %" PRIu32, recipient);
		bool teamSpecific = !targeting.all && (targeting.humanTeammates || targeting.aiTeammates);
		triggerEventQuickChatMessage(sender, recipient, msgEnumVal, teamSpecific);
	}

	return true;
}
