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
#include "chatoptions.h"
#include "lib/widget/widgint.h"
#include "lib/widget/label.h"
#include "lib/widget/margin.h"
#include "lib/widget/scrollablelist.h"
#include "lib/widget/button.h"
#include "lib/widget/table.h"
#include "lib/ivis_opengl/pieblitfunc.h"
#include "lib/ivis_opengl/piepalette.h"
#include "lib/netplay/netplay.h"

#include "../intimage.h"
#include "../hci.h"
#include "../multiplay.h"
#include "../main.h"

#include <array>
#include <chrono>

// MARK: - Constants

const PIELIGHT WZCOL_CHATOPTIONS_BUTTON_FILL_COLOR = pal_RGBA(25, 0, 110, 220);
const PIELIGHT WZCOL_CHATOPTIONS_BUTTON_FILL_COLOR_DARK = pal_RGBA(10, 0, 70, 250);
const PIELIGHT WZCOL_CHATOPTIONS_BUTTON_BORDER_LIGHT = pal_RGBA(100, 100, 255, 255);
const PIELIGHT WZCOL_CHATOPTIONS_BUTTON_BORDER_HIGHLIGHT = pal_RGBA(255, 255, 255, 225);

const PIELIGHT WZCOL_CHATOPTIONS_HOST_GLOBAL_SET_COLOR = pal_RGBA(100, 100, 255, 255);

constexpr int ChatOptionsButtonHorizontalPadding = 5;
constexpr int CHATOPT_FORM_EXTERNAL_PADDING = 20;
constexpr int CHATOPT_FORM_INTERNAL_PADDING = 10;
constexpr int CHATOPT_PLAYER_BUTTON_EXTERNAL_PADDING_X = 5;
constexpr int CHATOPT_PLAYER_BUTTON_INTERNAL_PADDING_X = 6;
constexpr int CHATOPT_PLAYER_BUTTON_HEIGHT = 22;

constexpr int CHATOPT_COLUMN_HEADER_PADDING_X = 5;
constexpr int CHATOPT_COLUMN_HEADER_PADDING_Y = 5;
constexpr int CHATOPT_COLUMN_PADDING_X = 2;

// MARK: - Classes

class WzChatOptionsButton;
class WzHostFreeChatPlayerMuteButton;

class WzChatOptionsForm : public W_CLICKFORM
{
protected:
	void initialize(bool showHostOptions);
public:
	WzChatOptionsForm(): W_CLICKFORM() {}
	static std::shared_ptr<WzChatOptionsForm> make(bool showHostOptions, const std::function<void ()>& onSettingsChange);
public:
	void display(int xOffset, int yOffset) override;
	virtual void run(W_CONTEXT *psContext) override;
	void geometryChanged() override;
	int32_t idealWidth() override;
	int32_t idealHeight() override;
public:
	void callOnSettingsChanged();
private:
	void refreshData();
	int32_t requiredHeightForTextAndButtons();
	void changeFreeChatForAll(bool enabled);
	std::shared_ptr<TableRow> newPlayerMuteRow(uint32_t playerIdx, bool allowGlobalConfig);
	std::pair<std::vector<size_t>, size_t> getMaxTableColumnDataWidths();
private:
	std::shared_ptr<WzChatOptionsButton> updateButton;

	bool showingHostingOptions = false;
	std::shared_ptr<W_LABEL> hostChatOptionsLabel;
	std::shared_ptr<W_LABEL> globalOptionsLabel;
	std::shared_ptr<WzChatOptionsButton> freechatEnableAllButton;
	std::shared_ptr<WzChatOptionsButton> freechatDisableAllButton;
	std::shared_ptr<WzHostFreeChatPlayerMuteButton> joiningClientsButton;

	std::shared_ptr<W_LABEL> chatOptionsLabel;
	std::shared_ptr<W_LABEL> chatOptionsDescriptionLabel;
	std::shared_ptr<W_LABEL> chatOptionsHostDisabledMsgLabel;

	std::shared_ptr<ScrollableTableWidget> playersTable;

	std::function<void ()> onSettingsChange;
};

class WzChatOptionsButton : public W_BUTTON
{
public:
	static std::shared_ptr<WzChatOptionsButton> make(const WzString& messageText, bool disabled = false)
	{
		auto result = std::make_shared<WzChatOptionsButton>();
		result->pText = messageText;
		return result;
	}
public:
	int32_t getButtonTextWidth()
	{
		return iV_GetTextWidth(pText, FontID);
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

		bool isDown = (getState() & (WBUT_DOWN | WBUT_LOCK | WBUT_CLICKLOCK)) != 0;
		bool isDisabled = (state & WBUT_DISABLE) != 0;
		bool isHighlight = (state & WBUT_HIGHLIGHT) != 0;

		// Draw background
		auto light_border = WZCOL_CHATOPTIONS_BUTTON_BORDER_HIGHLIGHT;
		auto regular_border = (!isDisabled) ? WZCOL_CHATOPTIONS_BUTTON_BORDER_LIGHT : WZCOL_FORM_DARK;
		auto fill_color = isDown || isDisabled ? WZCOL_CHATOPTIONS_BUTTON_FILL_COLOR_DARK : WZCOL_CHATOPTIONS_BUTTON_FILL_COLOR;
		iV_ShadowBox(xPos, yPos, xPos + w, yPos + h, 0, isDown || isHighlight ? light_border : regular_border, isDown || isHighlight ? light_border : regular_border, fill_color);

		// Select label text and color
		PIELIGHT msgColour = (isDown) ? WZCOL_TEXT_BRIGHT : WZCOL_FORM_TEXT;
		if (isDisabled)
		{
			msgColour = WZCOL_FORM_DISABLE;
		}

		iV_fonts fontID = wzMessageText.getFontID();
		if (fontID == font_count || lastWidgetWidth != width() || wzMessageText.getText() != pText)
		{
			fontID = FontID;
		}
		wzMessageText.setText(pText, fontID);

		int availableButtonTextWidth = w - (ChatOptionsButtonHorizontalPadding * 2);
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

		int textX0 = std::max(xPos + ChatOptionsButtonHorizontalPadding, xPos + ((w - wzMessageText.width()) / 2));
		int textY0 = static_cast<int>(yPos + (h - wzMessageText.lineSize()) / 2 - float(wzMessageText.aboveBase()));

		int maxTextDisplayableWidth = w - (ChatOptionsButtonHorizontalPadding * 2);
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

class WzPlayerChatMuteButtonBase : public W_BUTTON
{
public:
	virtual void updateData() = 0;
	virtual bool getMuteStatus() const = 0;
	virtual void setMuteStatus(bool muted) = 0;

public:
	// special click-handling functions - subclasses can override
	virtual void clickPrimary() { toggle(); }
	virtual bool clickSecondary(bool synthesizedFromHold) { return false; }

public:
	void display(int xOffset, int yOffset) override
	{
		int x0 = x() + xOffset;
		int y0 = y() + yOffset;
		int x1 = x0 + width();
		int y1 = y0 + height();

		bool isDown = !isSelectedPlayer && ((getState() & (WBUT_DOWN | WBUT_LOCK | WBUT_CLICKLOCK)) != 0);
		bool isDisabled = (getState() & WBUT_DISABLE) != 0;
		bool isHighlight = !isSelectedPlayer && !isDisabled && ((getState() & WBUT_HIGHLIGHT) != 0);

		// Display the button.
		auto light_border = WZCOL_CHATOPTIONS_BUTTON_BORDER_HIGHLIGHT;
		auto regular_border = (!isDisabled) ? WZCOL_CHATOPTIONS_BUTTON_BORDER_LIGHT : WZCOL_FORM_DARK;
		if (isSelectedPlayer)
		{
			regular_border = WZCOL_CHATOPTIONS_BUTTON_FILL_COLOR;
		}
		auto fill_color = isDown || isDisabled ? WZCOL_CHATOPTIONS_BUTTON_FILL_COLOR_DARK : WZCOL_CHATOPTIONS_BUTTON_FILL_COLOR;
		iV_ShadowBox(x0, y0, x1, y1, 0, isDown || isHighlight ? light_border : regular_border, isDown || isHighlight ? light_border : regular_border, fill_color);

		if (pText.isEmpty())
		{
			return;
		}

		bool muteStatus = getMuteStatus();

		wzText.setText(pText, FontID);

		int fx = CHATOPT_PLAYER_BUTTON_EXTERNAL_PADDING_X + 16 + CHATOPT_PLAYER_BUTTON_INTERNAL_PADDING_X;
		int fy = (height() - wzText.lineSize()) / 2 - wzText.aboveBase();
		int maxTextDisplayableWidth = width() - fx;
		bool isTruncated = maxTextDisplayableWidth < wzText.width();
		if (isTruncated)
		{
			maxTextDisplayableWidth -= (iV_GetEllipsisWidth(wzText.getFontID()) + 2);
		}
		int textX0 = x0 + fx;
		int textY0 = y0 + fy;

		PIELIGHT textColor = (isDown) ? WZCOL_TEXT_BRIGHT : WZCOL_FORM_TEXT;
		if (isDisabled)
		{
			textColor.byte.a = (textColor.byte.a / 2);
		}
		else if (muteStatus)
		{
			textColor = WZCOL_TEXT_MEDIUM;
		}
		wzText.render(textX0, textY0, textColor, 0.0f, maxTextDisplayableWidth);
		if (isTruncated)
		{
			// Render ellipsis
			iV_DrawEllipsis(wzText.getFontID(), Vector2f(textX0 + maxTextDisplayableWidth + 2, textY0), textColor);
		}

		UWORD ImageID = IMAGE_INTFAC_VOLUME_UP;
		uint8_t imageAlpha = (!isDisabled) ? 255 : 125;
		bool skipVolumeIconDraw = false;
		if (!muteStatus && !isDisabled)
		{
			ImageID = IMAGE_INTFAC_VOLUME_UP;
			if (isSelectedPlayer)
			{
				skipVolumeIconDraw = true;
			}
		}
		else
		{
			ImageID = IMAGE_INTFAC_VOLUME_MUTE;
		}
		if (!skipVolumeIconDraw)
		{
			if (!muteIconColorOverride.has_value())
			{
				iV_DrawImageFileAnisotropic(IntImages, ImageID, x0 + CHATOPT_PLAYER_BUTTON_EXTERNAL_PADDING_X, y0 + 2, Vector2f{16,16}, defaultProjectionMatrix(), imageAlpha);
			}
			else
			{
				iV_DrawImageTint(IntImages, ImageID, x0 + CHATOPT_PLAYER_BUTTON_EXTERNAL_PADDING_X, y0 + 2, muteIconColorOverride.value(), Vector2f{16,16});
			}
		}
	}

	int32_t idealWidth() override
	{
		return iV_GetTextWidth(pText, FontID) + (CHATOPT_PLAYER_BUTTON_EXTERNAL_PADDING_X * 2) + CHATOPT_PLAYER_BUTTON_INTERNAL_PADDING_X + 16;
	}

	void toggle()
	{
		setMuteStatus(!getMuteStatus());
	}

	void updateStringFromPlayerName(uint32_t playerIdx)
	{
		// keep player name updated
		if (!isHumanPlayer(playerIdx))
		{
			setString("");
			isSelectedPlayer = false;
			return;
		}
		setString(getPlayerName(playerIdx));
		isSelectedPlayer = (playerIdx == selectedPlayer);
	}

	void setMuteIconColorOverride(optional<PIELIGHT> _muteIconColorOverride)
	{
		muteIconColorOverride = _muteIconColorOverride;
	}

	optional<uint32_t> getPlayerIdxFromPosition(uint32_t playerPosition)
	{
		for (size_t idx = 0; idx < NetPlay.players.size(); idx++)
		{
			if (NetPlay.players[idx].position == playerPosition)
			{
				return idx;
			}
		}
		return nullopt;
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
			clickPrimary();
		}
		else if (mouseButton == WKEY_SECONDARY)
		{
			clickSecondary(false);
		}
	}
protected:
	WzText wzText;
	optional<PIELIGHT> muteIconColorOverride;
	bool isSelectedPlayer = false;
};

class WzHostFreeChatPlayerMuteButton : public WzPlayerChatMuteButtonBase
{
public:
	static std::shared_ptr<WzHostFreeChatPlayerMuteButton> make(optional<uint32_t> playerIdx, std::shared_ptr<WzChatOptionsForm> parentForm)
	{
		ASSERT_OR_RETURN(nullptr, playerIdx.value_or(0) < MAX_CONNECTED_PLAYERS, "Invalid playerPosition");
		auto button = std::make_shared<WzHostFreeChatPlayerMuteButton>();
		button->playerIdx = playerIdx;
		button->parentForm = parentForm;
		button->FontID = font_regular;
		button->updateData();
		button->setGeometry(0, 0, button->idealWidth(), CHATOPT_PLAYER_BUTTON_HEIGHT);
		return button;
	}

public:
	virtual void run(W_CONTEXT *psContext) override
	{
		if (playerIdx.has_value())
		{
			// keep player name updated
			updateStringFromPlayerName(playerIdx.value());
			bool isDisabled = (getState() & WBUT_DISABLE) != 0;
			bool isHuman = isHumanPlayer(playerIdx.value());
			if (isHuman == isDisabled)
			{
				setState((isHuman) ? 0 : WBUT_DISABLE);
			}
		}
	}

public:

	virtual void updateData() override
	{
		if (playerIdx.has_value())
		{
			updateStringFromPlayerName(playerIdx.value());
		}
		else
		{
			setString(_("New Joins"));
		}
	}

	virtual bool getMuteStatus() const override
	{
		if (playerIdx.has_value())
		{
			return !ingame.hostChatPermissions[playerIdx.value()];
		}
		else
		{
			return !NETgetDefaultMPHostFreeChatPreference();
		}
	}

	virtual void setMuteStatus(bool muted) override
	{
		if (!playerIdx.has_value())
		{
			NETsetDefaultMPHostFreeChatPreference(!muted);
			// do not update any existing players - this just sets the default for new joins
			return;
		}
		if (!isHumanPlayer(playerIdx.value()))
		{
			return;
		}
		ingame.hostChatPermissions[playerIdx.value()] = !muted;
		if (NetPlay.isHost && NetPlay.bComms)
		{
			sendHostConfig();
		}
		if (auto strongParent = parentForm.lock())
		{
			strongParent->callOnSettingsChanged();
		}
	}
private:
	optional<uint32_t> playerIdx = nullopt;
	std::weak_ptr<WzChatOptionsForm> parentForm;
};

class WzPlayerMuteStatusLabel : public WIDGET
{
public:
	static std::shared_ptr<WzPlayerMuteStatusLabel> make(uint32_t playerIdx)
	{
		ASSERT_OR_RETURN(nullptr, playerIdx < MAX_CONNECTED_PLAYERS, "Invalid playerIdx");
		auto label = std::make_shared<WzPlayerMuteStatusLabel>();
		label->playerIdx = playerIdx;
		label->FontID = font_regular;
		label->updateData();
		label->setGeometry(0, 0, label->idealWidth(), CHATOPT_PLAYER_BUTTON_HEIGHT);
		return label;
	}

public:
	virtual void run(W_CONTEXT *psContext) override
	{
		// keep player name updated
		updateStringFromPlayerName();

		bool isHuman = isHumanPlayer(playerIdx);
		if (isHuman == isDisabled)
		{
			if (!isHuman)
			{
				// no human player for this slot anymore - just disable it
				isDisabled = true;
			}
			else
			{
				// enable it
				isDisabled = false;
			}
		}
	}

	void display(int xOffset, int yOffset) override
	{
		int x0 = x() + xOffset;
		int y0 = y() + yOffset;

		if (pText.isEmpty())
		{
			return;
		}

		bool muteStatus = getMuteStatus();

		wzText.setText(pText, FontID);

		int fx = CHATOPT_PLAYER_BUTTON_EXTERNAL_PADDING_X + 16 + CHATOPT_PLAYER_BUTTON_INTERNAL_PADDING_X;
		int fy = (height() - wzText.lineSize()) / 2 - wzText.aboveBase();
		int maxTextDisplayableWidth = width() - fx;
		bool isTruncated = maxTextDisplayableWidth < wzText.width();
		if (isTruncated)
		{
			maxTextDisplayableWidth -= (iV_GetEllipsisWidth(wzText.getFontID()) + 2);
		}
		int textX0 = x0 + fx;
		int textY0 = y0 + fy;

		PIELIGHT textColor = WZCOL_FORM_TEXT;
		if (isDisabled)
		{
			textColor.byte.a = (textColor.byte.a / 2);
		}
		else if (muteStatus)
		{
			textColor = WZCOL_TEXT_MEDIUM;
		}
		wzText.render(textX0, textY0, textColor, 0.0f, maxTextDisplayableWidth);
		if (isTruncated)
		{
			// Render ellipsis
			iV_DrawEllipsis(wzText.getFontID(), Vector2f(textX0 + maxTextDisplayableWidth + 2, textY0), textColor);
		}

		UWORD ImageID = IMAGE_INTFAC_VOLUME_UP;
		uint8_t imageAlpha = (!isDisabled) ? 255 : 125;
		bool skipVolumeIconDraw = false;
		if (!muteStatus && !isDisabled)
		{
			ImageID = IMAGE_INTFAC_VOLUME_UP;
			if (isSelectedPlayer)
			{
				skipVolumeIconDraw = true;
			}
		}
		else
		{
			ImageID = IMAGE_INTFAC_VOLUME_MUTE;
		}
		if (!skipVolumeIconDraw)
		{
			const int iconSize = 16;
			int iconY0 = y0 + (height() - iconSize) / 2;
			if (!muteIconColorOverride.has_value())
			{
				iV_DrawImageFileAnisotropic(IntImages, ImageID, x0 + CHATOPT_PLAYER_BUTTON_EXTERNAL_PADDING_X, iconY0, Vector2f{iconSize,iconSize}, defaultProjectionMatrix(), imageAlpha);
			}
			else
			{
				iV_DrawImageTint(IntImages, ImageID, x0 + CHATOPT_PLAYER_BUTTON_EXTERNAL_PADDING_X, iconY0, muteIconColorOverride.value(), Vector2f{iconSize,iconSize});
			}
		}
	}

	virtual int32_t idealWidth() override
	{
		return iV_GetTextWidth(pText, FontID) + (CHATOPT_PLAYER_BUTTON_EXTERNAL_PADDING_X * 2) + CHATOPT_PLAYER_BUTTON_INTERNAL_PADDING_X + 16;
	}

public:

	int32_t requiredHeight() const
	{
		if (pText.isEmpty())
		{
			return 0;
		}

		return iV_GetTextLineSize(FontID);
	}

	void updateData()
	{
		updateStringFromPlayerName();
	}

	bool getMuteStatus() const
	{
		// display *applied* mute setting (including global and local settings)
		return isPlayerMuted(playerIdx);
	}

private:

	void updateStringFromPlayerName()
	{
		// keep player name updated
		if (!isHumanPlayer(playerIdx))
		{
			pText.clear();
			isSelectedPlayer = false;
			return;
		}
		pText = getPlayerName(playerIdx);
		isSelectedPlayer = (playerIdx == selectedPlayer);
	}

private:
	uint32_t playerIdx = 0;
	iV_fonts FontID = font_regular;
	WzString pText;
	WzText wzText;
	optional<PIELIGHT> muteIconColorOverride;
	bool isSelectedPlayer = false;
	bool isDisabled = false;
};

class WzChatMuteColButtonBase : public W_BUTTON
{
public:
	virtual bool getMuteStatus() const = 0;
	virtual void setMuteStatus(bool muted) = 0;

public:
	// special click-handling functions - subclasses can override
	virtual void clickPrimary() { toggle(); }
	virtual bool clickSecondary(bool synthesizedFromHold) { return false; }

public:
	void display(int xOffset, int yOffset) override
	{
		int x0 = x() + xOffset;
		int y0 = y() + yOffset;

		bool isDown = ((getState() & (WBUT_DOWN | WBUT_LOCK | WBUT_CLICKLOCK)) != 0);
		bool isDisabled = (getState() & WBUT_DISABLE) != 0;
		bool isHighlight = !isDisabled && ((getState() & WBUT_HIGHLIGHT) != 0);

		if (!isDisabled)
		{
			// Display the background and border
			int displayedWidth = std::min((CHATOPT_PLAYER_BUTTON_EXTERNAL_PADDING_X * 2) + 16, width());
			int boxX0 = x0 + ((width() - displayedWidth) / 2);
			int boxX1 = boxX0 + displayedWidth;
			int y1 = y0 + height();
			auto light_border = WZCOL_CHATOPTIONS_BUTTON_BORDER_HIGHLIGHT;
			auto regular_border = (!isDisabled) ? WZCOL_CHATOPTIONS_BUTTON_BORDER_LIGHT : WZCOL_FORM_DARK;
			auto fill_color = isDown || isDisabled ? WZCOL_CHATOPTIONS_BUTTON_FILL_COLOR_DARK : WZCOL_CHATOPTIONS_BUTTON_FILL_COLOR;
			iV_ShadowBox(boxX0, y0, boxX1, y1, 0, isDown || isHighlight ? light_border : regular_border, isDown || isHighlight ? light_border : regular_border, fill_color);
		}

		bool muteStatus = getMuteStatus();

		UWORD ImageID = IMAGE_INTFAC_VOLUME_UP;
		uint8_t imageAlpha = (!isDisabled) ? 255 : 125;
		bool skipVolumeIconDraw = false;
		if (!muteStatus)
		{
			ImageID = IMAGE_INTFAC_VOLUME_UP;
			if (hideAllowedIcon)
			{
				skipVolumeIconDraw = true;
			}
		}
		else
		{
			ImageID = IMAGE_INTFAC_VOLUME_MUTE;
		}
		if (!skipVolumeIconDraw)
		{
			const int iconSize = 16;
			int iconX0 = x0 + (width() - iconSize) / 2;
			int iconY0 = y0 + (height() - iconSize) / 2;
			if (!muteIconColorOverride.has_value())
			{
				iV_DrawImageFileAnisotropic(IntImages, ImageID, iconX0, iconY0, Vector2f{iconSize,iconSize}, defaultProjectionMatrix(), imageAlpha);
			}
			else
			{
				iV_DrawImageTint(IntImages, ImageID, iconX0, iconY0, muteIconColorOverride.value(), Vector2f{iconSize,iconSize});
			}
		}
	}

	int32_t idealWidth() override
	{
		return getDefaultIdealWidth();
	}

	static int32_t getDefaultIdealWidth()
	{
		return (CHATOPT_PLAYER_BUTTON_EXTERNAL_PADDING_X * 2) + 16;
	}

	void toggle()
	{
		setMuteStatus(!getMuteStatus());
	}

	void setHideAllowedIcon(bool hide)
	{
		hideAllowedIcon = hide;
	}

	void setMuteIconColorOverride(optional<PIELIGHT> _muteIconColorOverride)
	{
		muteIconColorOverride = _muteIconColorOverride;
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
			clickPrimary();
		}
		else if (mouseButton == WKEY_SECONDARY)
		{
			clickSecondary(false);
		}
	}
protected:
	WzText wzText;
	optional<PIELIGHT> muteIconColorOverride;
	bool hideAllowedIcon = false;
};

class WzLocalChatMuteColButton : public WzChatMuteColButtonBase
{
public:
	static std::shared_ptr<WzLocalChatMuteColButton> make(uint32_t playerIdx)
	{
		ASSERT_OR_RETURN(nullptr, playerIdx < MAX_CONNECTED_PLAYERS, "Invalid playerPosition");
		auto button = std::make_shared<WzLocalChatMuteColButton>();
		button->playerIdx = playerIdx;
		if (playerIdx == selectedPlayer)
		{
			button->setHideAllowedIcon(true);
		}
		button->setGeometry(0, 0, button->idealWidth(), CHATOPT_PLAYER_BUTTON_HEIGHT);
		return button;
	}

public:
	virtual void run(W_CONTEXT *psContext) override
	{
		bool isDisabled = (getState() & WBUT_DISABLE) != 0;
		bool isHuman = isHumanPlayer(playerIdx);
		bool shouldBeDisabled = (playerIdx == selectedPlayer) || !isHuman;
		if (shouldBeDisabled != isDisabled)
		{
			setState((shouldBeDisabled) ? WBUT_DISABLE : 0);
		}
	}

public:

	virtual bool getMuteStatus() const override
	{
		return ingame.muteChat[playerIdx];
	}

	virtual void setMuteStatus(bool muted) override
	{
		if (!isHumanPlayer(playerIdx))
		{
			return;
		}
		if (playerIdx == selectedPlayer)
		{
			return;
		}
		setPlayerMuted(playerIdx, muted);
	}

private:
	uint32_t playerIdx = 0;
};

class WzGlobalChatMuteColButton : public WzChatMuteColButtonBase
{
public:
	static std::shared_ptr<WzGlobalChatMuteColButton> make(optional<uint32_t> playerIdx, std::shared_ptr<WzChatOptionsForm> parentForm, bool readOnly)
	{
		ASSERT_OR_RETURN(nullptr, playerIdx.value_or(0) < MAX_CONNECTED_PLAYERS, "Invalid playerPosition");
		auto button = std::make_shared<WzGlobalChatMuteColButton>();
		button->playerIdx = playerIdx;
		button->parentForm = parentForm;
		button->readOnly = readOnly;
		button->setHideAllowedIcon(true);
		button->updateButtonStatus();
		button->setGeometry(0, 0, button->idealWidth(), CHATOPT_PLAYER_BUTTON_HEIGHT);
		return button;
	}

public:
	virtual void run(W_CONTEXT *psContext) override
	{
		updateButtonStatus();
	}

public:

	virtual bool getMuteStatus() const override
	{
		if (playerIdx.has_value())
		{
			return !ingame.hostChatPermissions[playerIdx.value()];
		}
		else
		{
			return !NETgetDefaultMPHostFreeChatPreference();
		}
	}

	virtual void setMuteStatus(bool muted) override
	{
		if (readOnly)
		{
			return;
		}
		if (!playerIdx.has_value())
		{
			NETsetDefaultMPHostFreeChatPreference(!muted);
			// do not update any existing players - this just sets the default for new joins
			return;
		}
		if (!isHumanPlayer(playerIdx.value()))
		{
			return;
		}
		ingame.hostChatPermissions[playerIdx.value()] = !muted;
		if (NetPlay.isHost && NetPlay.bComms)
		{
			sendHostConfig();
		}
		if (auto strongParent = parentForm.lock())
		{
			strongParent->callOnSettingsChanged();
		}
	}

private:
	void updateButtonStatus()
	{
		if (playerIdx.has_value())
		{
			bool isDisabled = (getState() & WBUT_DISABLE) != 0;
			bool isHuman = isHumanPlayer(playerIdx.value());
			bool shouldBeDisabled = readOnly || !isHuman;
			if (shouldBeDisabled != isDisabled)
			{
				setState((shouldBeDisabled) ? WBUT_DISABLE : 0);
			}
		}
	}

private:
	optional<uint32_t> playerIdx = nullopt;
	std::weak_ptr<WzChatOptionsForm> parentForm;
	bool readOnly = false;
};

// MARK: - WzChatOptionsform

std::shared_ptr<WzChatOptionsForm> WzChatOptionsForm::make(bool showHostOptions, const std::function<void ()>& onSettingsChange)
{
	auto result = std::make_shared<WzChatOptionsForm>();
	result->onSettingsChange = onSettingsChange;
	result->initialize(showHostOptions);
	return result;
}

void WzChatOptionsForm::display(int xOffset, int yOffset)
{
	int x0 = x() + xOffset;
	int y0 = y() + yOffset;
	int x1 = x0 + width();
	int y1 = y0 + height();

	bool showHighlighted = true;
	PIELIGHT borderColor = (!showHighlighted) ? pal_RGBA(0, 0, 0, 255) : pal_RGBA(255, 255, 255, 100);

	// draw background + border
	pie_UniTransBoxFill((float)x0, (float)y0, (float)x1, (float)y1, pal_RGBA(0,0,0,150));
	iV_TransBoxFill((float)x0, (float)y0, (float)x1, (float)y1);
	iV_Box(x0, y0, x1, y1,borderColor);
}

int32_t WzChatOptionsForm::requiredHeightForTextAndButtons()
{
	int32_t result = 0;
	if (showingHostingOptions)
	{
		result += CHATOPT_FORM_EXTERNAL_PADDING + hostChatOptionsLabel->requiredHeight();
		result += CHATOPT_FORM_INTERNAL_PADDING + globalOptionsLabel->requiredHeight();
		result += CHATOPT_FORM_INTERNAL_PADDING + std::max({freechatEnableAllButton->height(), freechatDisableAllButton->height(), joiningClientsButton->height()});
		result += CHATOPT_FORM_INTERNAL_PADDING;
	}
	else
	{
		result += (CHATOPT_FORM_EXTERNAL_PADDING - CHATOPT_FORM_INTERNAL_PADDING);
	}

	result += CHATOPT_FORM_INTERNAL_PADDING + chatOptionsLabel->requiredHeight();
	result += CHATOPT_FORM_INTERNAL_PADDING + chatOptionsDescriptionLabel->requiredHeight();
	result += CHATOPT_FORM_INTERNAL_PADDING + chatOptionsHostDisabledMsgLabel->requiredHeight();
	// do not include players table
	result += CHATOPT_FORM_EXTERNAL_PADDING;
	return result;
}

int32_t WzChatOptionsForm::idealHeight()
{
	int result = requiredHeightForTextAndButtons();
	result += CHATOPT_FORM_INTERNAL_PADDING + playersTable->idealHeight();
	return result;
}

int32_t WzChatOptionsForm::idealWidth()
{
	int32_t result = 0;

	if (showingHostingOptions)
	{
		result = std::max(result, (CHATOPT_FORM_EXTERNAL_PADDING * 2) + globalOptionsLabel->idealWidth());
		if (joiningClientsButton && freechatEnableAllButton && freechatDisableAllButton)
		{
			result = std::max(result, (CHATOPT_FORM_EXTERNAL_PADDING + joiningClientsButton->width() + CHATOPT_FORM_INTERNAL_PADDING + freechatEnableAllButton->width() + CHATOPT_FORM_INTERNAL_PADDING + freechatDisableAllButton->width() + CHATOPT_FORM_EXTERNAL_PADDING));
		}
	}
	result = std::max(result, (CHATOPT_FORM_EXTERNAL_PADDING * 2) + chatOptionsLabel->idealWidth());
	result = std::max(result, (CHATOPT_FORM_EXTERNAL_PADDING * 2) + chatOptionsDescriptionLabel->getMaxLineWidth());
	result = std::max(result, (CHATOPT_FORM_EXTERNAL_PADDING * 2) + chatOptionsHostDisabledMsgLabel->getMaxLineWidth());

	auto maxColumnDataWidthsResult = getMaxTableColumnDataWidths();
	int idealTableWidth = playersTable->getTableWidthNeededForTotalColumnWidth(playersTable->getNumColumns(), maxColumnDataWidthsResult.second);
	result = std::max(result, (CHATOPT_FORM_EXTERNAL_PADDING * 2) + idealTableWidth);

	return result;
}

static bool hostHasMutedAnyPlayers()
{
	for (uint32_t i = 0; i < MAX_CONNECTED_PLAYERS; ++i)
	{
		if (!isHumanPlayer(i))
		{
			continue;
		}
		if (!ingame.hostChatPermissions[i])
		{
			return true;
		}
	}
	return false;
}

void WzChatOptionsForm::run(W_CONTEXT *psContext)
{
	if (hostHasMutedAnyPlayers())
	{
		chatOptionsHostDisabledMsgLabel->show();
	}
	else
	{
		chatOptionsHostDisabledMsgLabel->hide();
	}
}

void WzChatOptionsForm::geometryChanged()
{
	int w = width();
	int h = height();

	if (w == 0 || h == 0)
	{
		return;
	}

	int maxPlayersTableWidth = std::max((w - (CHATOPT_FORM_EXTERNAL_PADDING * 2)), 0);
	int totalAvailableHeightForPlayersTable = h - requiredHeightForTextAndButtons() - CHATOPT_FORM_INTERNAL_PADDING;

	if (totalAvailableHeightForPlayersTable < 0)
	{
		// not enough height to display the lists at all?
		return;
	}

	int currentY0 = 0;

	updateButton->setGeometry(w - (CHATOPT_FORM_EXTERNAL_PADDING + updateButton->width()), CHATOPT_FORM_EXTERNAL_PADDING, updateButton->width(), updateButton->height());

	if (showingHostingOptions)
	{
		currentY0 = globalOptionsLabel->y() + globalOptionsLabel->height();

		int lastButtonX1 = 0;
		if (GetGameMode() != GS_NORMAL) // if not yet in a game
		{
			joiningClientsButton->setGeometry(CHATOPT_FORM_EXTERNAL_PADDING, currentY0 + CHATOPT_FORM_INTERNAL_PADDING, joiningClientsButton->width(), joiningClientsButton->height());
			lastButtonX1 = joiningClientsButton->x() + joiningClientsButton->width();
		}
		else
		{
			// in game, hide this button
			joiningClientsButton->hide();
			lastButtonX1 = (CHATOPT_FORM_EXTERNAL_PADDING - CHATOPT_FORM_INTERNAL_PADDING);
		}

		freechatEnableAllButton->setGeometry(lastButtonX1 + CHATOPT_FORM_INTERNAL_PADDING, currentY0 + CHATOPT_FORM_INTERNAL_PADDING, freechatEnableAllButton->width(), freechatEnableAllButton->height());
		lastButtonX1 = freechatEnableAllButton->x() + freechatEnableAllButton->width();

		freechatDisableAllButton->setGeometry(lastButtonX1 + CHATOPT_FORM_INTERNAL_PADDING, currentY0 + CHATOPT_FORM_INTERNAL_PADDING, freechatDisableAllButton->width(), freechatDisableAllButton->height());

		currentY0 = std::max(globalOptionsLabel->y() + globalOptionsLabel->height(), freechatDisableAllButton->y() + freechatDisableAllButton->height()) + CHATOPT_FORM_INTERNAL_PADDING;
	}
	else
	{
		currentY0 = (CHATOPT_FORM_EXTERNAL_PADDING - CHATOPT_FORM_INTERNAL_PADDING);
	}

	chatOptionsLabel->setGeometry(CHATOPT_FORM_EXTERNAL_PADDING, currentY0 + CHATOPT_FORM_INTERNAL_PADDING, chatOptionsLabel->getMaxLineWidth(), chatOptionsLabel->requiredHeight());

	currentY0 = chatOptionsLabel->y() + chatOptionsLabel->height();

	chatOptionsDescriptionLabel->setGeometry(CHATOPT_FORM_EXTERNAL_PADDING, currentY0 + CHATOPT_FORM_INTERNAL_PADDING, chatOptionsDescriptionLabel->getMaxLineWidth(), chatOptionsDescriptionLabel->requiredHeight());
	currentY0 = chatOptionsDescriptionLabel->y() + chatOptionsDescriptionLabel->height();

	auto maxColumnDataWidthsResult = getMaxTableColumnDataWidths();
	int idealTableWidth = playersTable->getTableWidthNeededForTotalColumnWidth(playersTable->getNumColumns(), maxColumnDataWidthsResult.second);

	int playersTableWidth = std::min(maxPlayersTableWidth, idealTableWidth);
	playersTable->setGeometry(CHATOPT_FORM_EXTERNAL_PADDING, currentY0 + CHATOPT_FORM_INTERNAL_PADDING, playersTableWidth, totalAvailableHeightForPlayersTable);

	currentY0 = playersTable->y() + playersTable->height();

	chatOptionsHostDisabledMsgLabel->setGeometry(CHATOPT_FORM_EXTERNAL_PADDING, currentY0 + CHATOPT_FORM_INTERNAL_PADDING, chatOptionsHostDisabledMsgLabel->getMaxLineWidth(), chatOptionsHostDisabledMsgLabel->requiredHeight());
}

constexpr size_t playerNameColIdx = 0;
constexpr int32_t MAX_PLAYER_NAME_COLUMN_WIDTH = 200;

std::pair<std::vector<size_t>, size_t> WzChatOptionsForm::getMaxTableColumnDataWidths()
{
	size_t totalNeededColumnWidth = 0;
	std::vector<size_t> maxColumnDataWidths;
	auto& minimumColumnWidths = playersTable->getMinimumColumnWidths();
	ASSERT(minimumColumnWidths.size() == playersTable->getNumColumns(), "Number of minimum column widths does not match number of columns!");
	for (size_t colIdx = 0; colIdx < playersTable->getNumColumns(); ++colIdx)
	{
		int32_t maxIdealContentWidth = playersTable->getColumnMaxContentIdealWidth(colIdx);
		maxIdealContentWidth = std::max<int32_t>({maxIdealContentWidth, 0, static_cast<int32_t>(minimumColumnWidths.at(colIdx))});
		if (colIdx == playerNameColIdx)
		{
			// restrict player name column to a fixed maximum width
			maxIdealContentWidth = std::min<int32_t>(maxIdealContentWidth, MAX_PLAYER_NAME_COLUMN_WIDTH);
		}
		size_t maxIdealContentWidth_sizet = static_cast<size_t>(maxIdealContentWidth);
		maxColumnDataWidths.push_back(maxIdealContentWidth_sizet);
		totalNeededColumnWidth += maxIdealContentWidth_sizet;
	}
	return {maxColumnDataWidths, totalNeededColumnWidth};
}

std::shared_ptr<TableRow> WzChatOptionsForm::newPlayerMuteRow(uint32_t playerIdx, bool allowGlobalConfig)
{
	ASSERT_OR_RETURN(nullptr, playerIdx < MAX_CONNECTED_PLAYERS, "Invalid playerIdx");

	std::vector<std::shared_ptr<WIDGET>> columnWidgets;

	std::shared_ptr<WzChatOptionsForm> sharedSelf = std::dynamic_pointer_cast<WzChatOptionsForm>(shared_from_this());

	// Player Name widget
	auto playerNameLabel = WzPlayerMuteStatusLabel::make(playerIdx);
	ASSERT_OR_RETURN(nullptr, playerNameLabel != nullptr, "Failed to make widget");
	playerNameLabel->setGeometry(0, 0, playerNameLabel->idealWidth(), playerNameLabel->requiredHeight());
	playerNameLabel->setTransparentToMouse(true);
	columnWidgets.push_back(playerNameLabel);

	// Global Mute column
	auto globalMuteButton = WzGlobalChatMuteColButton::make(playerIdx, sharedSelf, !allowGlobalConfig);
	ASSERT_OR_RETURN(nullptr, globalMuteButton != nullptr, "Failed to make widget");
	columnWidgets.push_back(globalMuteButton);

	// Local Mute column
	auto localMuteButton = WzLocalChatMuteColButton::make(playerIdx);
	ASSERT_OR_RETURN(nullptr, localMuteButton != nullptr, "Failed to make widget");
	if (playerIdx == selectedPlayer)
	{
		localMuteButton->setState(WBUT_DISABLE);
	}
	columnWidgets.push_back(localMuteButton);

	int rowHeight = 0;
	for (auto& col : columnWidgets)
	{
		rowHeight = std::max(rowHeight, col->idealHeight());
	}

	playerNameLabel->setGeometry(0, 0, playerNameLabel->idealWidth(), rowHeight);

	auto row = TableRow::make(columnWidgets, rowHeight);
	row->setDisabledColor(WZCOL_FORM_DARK);
	row->setHighlightsOnMouseOver(true);

	return row;
}

void WzChatOptionsForm::refreshData()
{
	// Add rows for all players
	std::vector<uint32_t> playerIndexes;
	for (uint32_t player = 0; player < std::min<uint32_t>(game.maxPlayers, MAX_PLAYERS); ++player)
	{
		if (isHumanPlayer(player)) // is an active (connected) human
		{
			playerIndexes.push_back(player);
		}
	}
	// sort by player position
	std::sort(playerIndexes.begin(), playerIndexes.end(), [](uint32_t playerA, uint32_t playerB) -> bool {
		return NetPlay.players[playerA].position < NetPlay.players[playerB].position;
	});

	// Rows for all spectators
	std::vector<uint32_t> spectatorIndexes;
	for (uint32_t player = MAX_PLAYERS; player < MAX_CONNECTED_PLAYERS; ++player)
	{
		if (isHumanPlayer(player) // is an active (connected) human
					&& NetPlay.players[player].isSpectator) // is a spectator
		{
			spectatorIndexes.push_back(player);
		}
	}

	playersTable->clearRows();
	// Add players to playersTable
	for (auto playerIdx : playerIndexes)
	{
		playersTable->addRow(newPlayerMuteRow(playerIdx, showingHostingOptions));
	}
	// Add spectators to playersTable
	for (auto playerIdx : spectatorIndexes)
	{
		playersTable->addRow(newPlayerMuteRow(playerIdx, showingHostingOptions));
	}

	geometryChanged();
}

static std::shared_ptr<WIDGET> createColHeaderLabel(const char* text)//, int minLeftPadding)
{
	auto label = std::make_shared<W_LABEL>();
	label->setTextAlignment(WLAB_ALIGNCENTRE);
	label->setString(text);
	label->setGeometry(0, 0, label->getMaxLineWidth(), 0);
	label->setCacheNeverExpires(true);
	label->setCanTruncate(true);
	return Margin(CHATOPT_COLUMN_HEADER_PADDING_Y, CHATOPT_COLUMN_HEADER_PADDING_X, CHATOPT_COLUMN_HEADER_PADDING_Y, CHATOPT_COLUMN_HEADER_PADDING_X).wrap(label);
}

void WzChatOptionsForm::initialize(bool showHostOptions)
{
	showingHostingOptions = showHostOptions;

	int currentY0 = 0;

	// top-right controls
	updateButton = WzChatOptionsButton::make("\u21BB", false); // "↻"
	updateButton->setGeometry(updateButton->x(), updateButton->y(), std::max(updateButton->getButtonTextWidth() + CHATOPT_FORM_INTERNAL_PADDING, CHATOPT_PLAYER_BUTTON_HEIGHT), CHATOPT_PLAYER_BUTTON_HEIGHT);
	updateButton->setTip(_("Refresh"));
	attach(updateButton);
	updateButton->addOnClickHandler([](W_BUTTON& button) {
		auto psParent = std::dynamic_pointer_cast<WzChatOptionsForm>(button.parent());
		ASSERT_OR_RETURN(, psParent != nullptr, "No parent");
		widgScheduleTask([psParent](){
			psParent->refreshData();
			psParent->callCalcLayout();
		});
	});

	auto sharedSelf = std::dynamic_pointer_cast<WzChatOptionsForm>(shared_from_this());
	std::weak_ptr<WzChatOptionsForm> weakSelf(sharedSelf);

	if (showHostOptions)
	{
		hostChatOptionsLabel = std::make_shared<W_LABEL>();
		hostChatOptionsLabel->setFont(font_medium_bold, WZCOL_TEXT_BRIGHT);
		hostChatOptionsLabel->setString(_("Host Chat Options"));
		attach(hostChatOptionsLabel);
		hostChatOptionsLabel->setGeometry(CHATOPT_FORM_EXTERNAL_PADDING, currentY0 + CHATOPT_FORM_EXTERNAL_PADDING, hostChatOptionsLabel->getMaxLineWidth(), hostChatOptionsLabel->requiredHeight());
		hostChatOptionsLabel->setTransparentToMouse(true);

		currentY0 = hostChatOptionsLabel->y() + hostChatOptionsLabel->height();

		globalOptionsLabel = std::make_shared<W_LABEL>();
		globalOptionsLabel->setFont(font_regular_bold, WZCOL_TEXT_MEDIUM);
		globalOptionsLabel->setString(_("Global Options:"));
		attach(globalOptionsLabel);
		globalOptionsLabel->setGeometry(CHATOPT_FORM_EXTERNAL_PADDING, currentY0 + CHATOPT_FORM_INTERNAL_PADDING, globalOptionsLabel->getMaxLineWidth(), globalOptionsLabel->requiredHeight());
		globalOptionsLabel->setTransparentToMouse(true);

		// Add "Enable All" and "Disable All" buttons
		freechatEnableAllButton = WzChatOptionsButton::make(_("Enable All"), false);
		attach(freechatEnableAllButton);
		freechatEnableAllButton->addOnClickHandler([weakSelf](W_BUTTON& widg) {
			auto strongSelf = weakSelf.lock();
			ASSERT_OR_RETURN(, strongSelf != nullptr, "Null form?");
			strongSelf->changeFreeChatForAll(true);
		});
		freechatEnableAllButton->setGeometry(CHATOPT_FORM_EXTERNAL_PADDING, currentY0 + CHATOPT_FORM_INTERNAL_PADDING, freechatEnableAllButton->getButtonTextWidth() + CHATOPT_FORM_INTERNAL_PADDING, CHATOPT_PLAYER_BUTTON_HEIGHT);
		freechatDisableAllButton = WzChatOptionsButton::make(_("Disable All"), false);
		attach(freechatDisableAllButton);
		freechatDisableAllButton->addOnClickHandler([weakSelf](W_BUTTON& widg) {
			auto strongSelf = weakSelf.lock();
			ASSERT_OR_RETURN(, strongSelf != nullptr, "Null form?");
			strongSelf->changeFreeChatForAll(false);
		});
		freechatDisableAllButton->setGeometry(CHATOPT_FORM_EXTERNAL_PADDING, currentY0 + CHATOPT_FORM_INTERNAL_PADDING, freechatDisableAllButton->getButtonTextWidth() + CHATOPT_FORM_INTERNAL_PADDING, CHATOPT_PLAYER_BUTTON_HEIGHT);

		// Add "New Joins" button
		joiningClientsButton = WzHostFreeChatPlayerMuteButton::make(nullopt, sharedSelf);
		attach(joiningClientsButton);
		joiningClientsButton->setGeometry(CHATOPT_FORM_EXTERNAL_PADDING, currentY0 + CHATOPT_FORM_INTERNAL_PADDING, joiningClientsButton->width(), joiningClientsButton->height());

		currentY0 = std::max(globalOptionsLabel->y() + globalOptionsLabel->height(), freechatDisableAllButton->y() + freechatDisableAllButton->height()) + CHATOPT_FORM_INTERNAL_PADDING;
	}

	chatOptionsLabel = std::make_shared<W_LABEL>();
	chatOptionsLabel->setFont(font_medium_bold, WZCOL_TEXT_BRIGHT);
	chatOptionsLabel->setString(_("Chat Options"));
	attach(chatOptionsLabel);
	chatOptionsLabel->setGeometry(CHATOPT_FORM_EXTERNAL_PADDING, currentY0 + CHATOPT_FORM_INTERNAL_PADDING, chatOptionsLabel->getMaxLineWidth(), chatOptionsLabel->requiredHeight());
	chatOptionsLabel->setTransparentToMouse(true);
	currentY0 = chatOptionsLabel->y() + chatOptionsLabel->height();

	chatOptionsDescriptionLabel = std::make_shared<W_LABEL>();
	chatOptionsDescriptionLabel->setFont(font_regular, WZCOL_TEXT_MEDIUM);
	chatOptionsDescriptionLabel->setString(_("Mute or configure player free chat."));
	chatOptionsDescriptionLabel->setCanTruncate(true);
	attach(chatOptionsDescriptionLabel);
	chatOptionsDescriptionLabel->setGeometry(CHATOPT_FORM_EXTERNAL_PADDING, currentY0 + CHATOPT_FORM_INTERNAL_PADDING, chatOptionsDescriptionLabel->getMaxLineWidth(), chatOptionsDescriptionLabel->requiredHeight());
	chatOptionsDescriptionLabel->setTransparentToMouse(true);
	currentY0 = chatOptionsDescriptionLabel->y() + chatOptionsDescriptionLabel->height();

	chatOptionsHostDisabledMsgLabel = std::make_shared<W_LABEL>();
	chatOptionsHostDisabledMsgLabel->setFont(font_regular, WZCOL_CHATOPTIONS_HOST_GLOBAL_SET_COLOR);
	chatOptionsHostDisabledMsgLabel->setString(_("The host has globally muted 1+ players."));
	chatOptionsHostDisabledMsgLabel->setCanTruncate(true);
	attach(chatOptionsHostDisabledMsgLabel);
	chatOptionsHostDisabledMsgLabel->setGeometry(CHATOPT_FORM_EXTERNAL_PADDING, currentY0 + CHATOPT_FORM_INTERNAL_PADDING, chatOptionsHostDisabledMsgLabel->getMaxLineWidth(), chatOptionsHostDisabledMsgLabel->requiredHeight());
	chatOptionsHostDisabledMsgLabel->setTransparentToMouse(true);
	chatOptionsHostDisabledMsgLabel->hide();

	// Table of players

	// Create column headers for players table
	std::vector<TableColumn> columns;
	columns.push_back({createColHeaderLabel(" "), TableColumn::ResizeBehavior::FIXED_WIDTH});
	columns.push_back({createColHeaderLabel(_("Global")), TableColumn::ResizeBehavior::FIXED_WIDTH});
	columns.push_back({createColHeaderLabel(_("Local")), TableColumn::ResizeBehavior::FIXED_WIDTH});

	int maxConfigOptColumnWidth = WzChatMuteColButtonBase::getDefaultIdealWidth();
	for (size_t i = 1; i < columns.size(); ++i)
	{
		const auto& column = columns[i];
		maxConfigOptColumnWidth = std::max(maxConfigOptColumnWidth, column.columnWidget->idealWidth());
	}

	int maxIdealColumnHeight = 0;
	std::vector<size_t> minimumColumnWidths;
	minimumColumnWidths.push_back(200);	// fixed minimum width for player names
	for (size_t i = 1; i < columns.size(); ++i)
	{
		const auto& column = columns[i];
		minimumColumnWidths.push_back(static_cast<size_t>(std::max<int>({column.columnWidget->idealWidth(), 0, maxConfigOptColumnWidth})));
		maxIdealColumnHeight = std::max(maxIdealColumnHeight, column.columnWidget->idealHeight());
	}

	int playersTableColumnHeaderHeight = maxIdealColumnHeight;
	playersTable = ScrollableTableWidget::make(columns, playersTableColumnHeaderHeight);
	playersTable->setColumnPadding(Vector2i(CHATOPT_COLUMN_PADDING_X, 0));
	attach(playersTable);
	playersTable->setBackgroundColor(pal_RGBA(0, 0, 0, 0));
	playersTable->setMinimumColumnWidths(minimumColumnWidths);
	playersTable->setDrawColumnLines(true);
	playersTable->setItemSpacing(2);

	refreshData();
}

void WzChatOptionsForm::callOnSettingsChanged()
{
	if (onSettingsChange)
	{
		onSettingsChange();
	}
}

void WzChatOptionsForm::changeFreeChatForAll(bool enabled)
{
	NETsetDefaultMPHostFreeChatPreference(enabled);
	if (NetPlay.isHost && NetPlay.bComms)
	{
		// update all existing slots
		for (uint32_t player = 0; player < MAX_CONNECTED_PLAYERS; ++player)
		{
			ingame.hostChatPermissions[player] = NETgetDefaultMPHostFreeChatPreference();
		}
		sendHostConfig();
		callOnSettingsChanged();
	}
}

// MARK: - Public functions

std::shared_ptr<W_FORM> createChatOptionsForm(bool showHostOptions, const std::function<void ()>& onSettingsChange)
{
	auto result = WzChatOptionsForm::make(showHostOptions, onSettingsChange);
	return result;
}
