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
#include "chatscreen.h"
#include "lib/widget/widgint.h"
#include "lib/widget/label.h"
#include "lib/widget/button.h"
#include "lib/widget/editbox.h"
#include "lib/ivis_opengl/pieblitfunc.h"
#include "lib/ivis_opengl/piepalette.h"
#include "lib/netplay/netplay.h"

#include "../intdisplay.h"
#include "../display.h"
#include "../hci.h"
#include "../chat.h"
#include "../cheat.h"
#include "../hci/quickchat.h"
#include "../hci/chatoptions.h"

class WzInGameChatScreen_CLICKFORM;
struct WzInGameChatScreen;

// MARK: - Globals

static std::weak_ptr<WzInGameChatScreen> psCurrentChatScreen;

// MARK: - WzInGameChatScreen definition

struct WzInGameChatScreen: public W_SCREEN
{
protected:
	WzInGameChatScreen(): W_SCREEN() {}
	~WzInGameChatScreen()
	{
		setFocus(nullptr);
	}

public:
	typedef std::function<void ()> OnCloseFunc;
	static std::shared_ptr<WzInGameChatScreen> make(const OnCloseFunc& onCloseFunction, WzChatMode initialChatMode, bool startWithQuickChatFocused);

public:
	void closeScreen();

private:
	OnCloseFunc onCloseFunc;

private:
	WzInGameChatScreen(WzInGameChatScreen const &) = delete;
	WzInGameChatScreen &operator =(WzInGameChatScreen const &) = delete;
};

// MARK: - WzInGameChatBoxForm

class WzInGameChatBoxButton : public W_BUTTON
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
		auto border_color = !isDisabled ? pal_RGBA(100, 100, 255, 120) : WZCOL_FORM_DARK;
		auto fill_color = isDown || isDisabled ? pal_RGBA(0, 0, 45, 250) : pal_RGBA(0, 0, 67, 220);
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

class WzInGameChatBoxForm : public IntFormAnimated
{
protected:
	WzInGameChatBoxForm(): IntFormAnimated(true) {}
	void initialize(WzChatMode initialChatMode, const W_EDITBOX::OnTabHandler& onTabHandler);

public:
	static std::shared_ptr<WzInGameChatBoxForm> make(WzChatMode initialChatMode, const W_EDITBOX::OnTabHandler& onTabHandler)
	{
		class make_shared_enabler: public WzInGameChatBoxForm {};
		auto widget = std::make_shared<make_shared_enabler>();
		widget->initialize(initialChatMode, onTabHandler);
		return widget;
	}

	virtual ~WzInGameChatBoxForm();

	virtual void focusLost() override;
	virtual void geometryChanged() override;

public:
	void setChatMode(WzChatMode newMode);
	void startChatBoxEditing();
	void endChatBoxEditing();
	void setChatBoxEnabledStatus(bool freeChatEnabled, bool quickChatEnabled)
	{
		if (freeChatEnabled)
		{
			chatBox->setPlaceholder("");
			chatBox->setTip("");
			chatBox->setPlaceholderTextColor(nullopt);
		}
		else
		{
			if (quickChatEnabled)
			{
				chatBox->setPlaceholder(_("Use Quick Chat to chat with other players."));
				chatBox->setPlaceholderTextColor(WZCOL_TEXT_MEDIUM);
				chatBox->setTip(_("The host has disabled free chat. Please use Quick Chat."));
			}
			else
			{
				chatBox->setPlaceholder(_("The host has disabled free chat."));
				chatBox->setTip("");
				chatBox->setPlaceholderTextColor(WZCOL_TEXT_MEDIUM);
			}
			// but don't disable the edit box, so cheat / chat commands still work
		}
	}

protected:
	void closeParentScreen()
	{
		auto parentScreen = std::dynamic_pointer_cast<WzInGameChatScreen>(screenPointer.lock());
		ASSERT_OR_RETURN(, parentScreen != nullptr, "No screen?");
		parentScreen->closeScreen();
	}

private:
	void displayOptionsOverlay();

private:
	WzChatMode chatMode;
	std::shared_ptr<W_EDITBOX> chatBox;
	std::shared_ptr<W_LABEL> label;
	std::shared_ptr<WzInGameChatBoxButton> optionsButton;
	std::shared_ptr<W_SCREEN> optionsOverlayScreen;
};

WzInGameChatBoxForm::~WzInGameChatBoxForm()
{
	if (optionsOverlayScreen)
	{
		widgRemoveOverlayScreen(optionsOverlayScreen);
		optionsOverlayScreen.reset();
	}
}

void WzInGameChatBoxForm::focusLost()
{
	debug(LOG_INFO, "WzInGameChatBoxForm::focusLost");
}

void WzInGameChatBoxForm::startChatBoxEditing()
{
	// Auto-click it
	W_CONTEXT sContext = W_CONTEXT::ZeroContext();
	chatBox->clicked(&sContext);
	switch (chatMode)
	{
	case WzChatMode::Glob:
		chatBox->setBoxColours(WZCOL_MENU_BORDER, WZCOL_MENU_BORDER, WZCOL_MENU_BACKGROUND);
		label->setFontColour(WZCOL_TEXT_BRIGHT);
		break;
	case WzChatMode::Team:
		chatBox->setBoxColours(WZCOL_YELLOW, WZCOL_YELLOW, WZCOL_MENU_BACKGROUND);
		label->setFontColour(WZCOL_YELLOW);
		break;
	}
}

void WzInGameChatBoxForm::endChatBoxEditing()
{
	chatBox->stopEditing();
	label->setFontColour(WZCOL_TEXT_MEDIUM);
	chatBox->setBoxColours(WZCOL_MENU_BORDER, WZCOL_MENU_BORDER, WZCOL_MENU_BACKGROUND);
}

void WzInGameChatBoxForm::setChatMode(WzChatMode newMode)
{
	chatMode = newMode;

	switch (chatMode)
	{
		case WzChatMode::Glob:
			chatBox->setBoxColours(WZCOL_MENU_BORDER, WZCOL_MENU_BORDER, WZCOL_MENU_BACKGROUND);
			label->setFontColour(WZCOL_TEXT_BRIGHT);
			label->setString(_("Chat: All"));
			break;
		case WzChatMode::Team:
			chatBox->setBoxColours(WZCOL_YELLOW, WZCOL_YELLOW, WZCOL_MENU_BACKGROUND);
			label->setFontColour(WZCOL_YELLOW);
			label->setString(_("Chat: Team"));
			break;
	}
}

/**
 * Parse what the player types in the chat box.
 *
 * Messages prefixed with "." are interpreted as messages to allies.
 * Messages prefixed with 1 or more digits (0-9) are interpreted as private messages to players.
 *
 * Examples:
 * - ".hi allies" sends "hi allies" to all allies.
 * - "123hi there" sends "hi there" to players 1, 2 and 3.
 * - ".123hi there" sends "hi there" to allies and players 1, 2 and 3.
 **/
static void parseChatMessageModifiers(InGameChatMessage &message)
{
	if (*message.text == '.')
	{
		message.toAllies = true;
		message.text++;
	}

	for (; *message.text >= '0' && *message.text <= '9'; ++message.text)  // for each 0..9 numeric char encountered
	{
		message.addReceiverByPosition(*message.text - '0');
	}
}

void WzInGameChatBoxForm::geometryChanged()
{
	int remainingWidth = width();

	int usableHeight = height() - 4;

	label->setGeometry(2, 2, 60, 16);
	remainingWidth -= (label->x() + label->width());

	if (optionsButton)
	{
		int optionsButtonHeight = std::max({optionsButton->height(), usableHeight, iV_GetTextLineSize(optionsButton->FontID)});
		int optionsButtonWidth = std::max(optionsButton->idealWidth(), optionsButtonHeight);
		int optionsButtonY0 = std::max(0, (height() - optionsButtonHeight) / 2);
		optionsButton->setGeometry(width() - 2 - optionsButtonWidth, optionsButtonY0, optionsButtonWidth, optionsButtonHeight);
		remainingWidth -= (optionsButtonWidth + 2 + 4);
	}

	int chatBoxPaddingX = 18;
	int chatBoxWidth = std::max(1, (remainingWidth - chatBoxPaddingX));
	chatBox->setGeometry(label->x() + label->width() + chatBoxPaddingX, 2, chatBoxWidth, 16);
}

void WzInGameChatBoxForm::initialize(WzChatMode initialChatMode, const W_EDITBOX::OnTabHandler& onTabHandler)
{
	chatBox = std::make_shared<W_EDITBOX>();
	attach(chatBox);
	chatBox->id = CHAT_EDITBOX;
	chatBox->setGeometry(80, 2, 320, 16);

	chatBox->setOnReturnHandler([](W_EDITBOX& widg) {

		auto strongParent = std::dynamic_pointer_cast<WzInGameChatBoxForm>(widg.parent());
		ASSERT_OR_RETURN(, strongParent != nullptr, "No parent?");

		auto enteredText = widg.getString();
		WzChatMode currChatMode = strongParent->chatMode;
		inputLoseFocus();

		auto weakParent = std::weak_ptr<WzInGameChatBoxForm>(strongParent);
		widgScheduleTask([enteredText, currChatMode, weakParent]() {

			// must process the enteredText inside a scheduled task as side effects of attemptCheatCode may modify the overlay screens / widgets

			const char* pStr = enteredText.toUtf8().c_str();
			auto message = InGameChatMessage(selectedPlayer, pStr);
			bool processedCheatCode = attemptCheatCode(message.text);		// parse the message

			bool skipSendingMessage = (bMultiPlayer && (selectedPlayer >= MAX_CONNECTED_PLAYERS || !ingame.hostChatPermissions[selectedPlayer]));

			if (!skipSendingMessage)
			{
				switch (currChatMode)
				{
					case WzChatMode::Glob:
						parseChatMessageModifiers(message);
						break;
					case WzChatMode::Team:
						message.toAllies = true;
						break;
				}

				if (strlen(message.text))
				{
					message.send();
				}
			}
			else
			{
				if (strlen(message.text) && !processedCheatCode && selectedPlayer < MAX_CONNECTED_PLAYERS)
				{
					addConsoleMessage(_("Did not send message - free chat is disabled by host. Please use Quick Chat."), DEFAULT_JUSTIFY, NOTIFY_MESSAGE, false);
				}
			}

			auto parent = weakParent.lock();
			ASSERT_OR_RETURN(, parent != nullptr, "No parent?");
			parent->endChatBoxEditing();
			parent->closeParentScreen();
		});
	});

	chatBox->setOnEscapeHandler([](W_EDITBOX& widg) {
		auto strongParent = std::dynamic_pointer_cast<WzInGameChatBoxForm>(widg.parent());
		ASSERT_OR_RETURN(, strongParent != nullptr, "No parent?");

		inputLoseFocus();

		auto weakParent = std::weak_ptr<WzInGameChatBoxForm>(strongParent);
		widgScheduleTask([weakParent]() {
			auto parent = weakParent.lock();
			ASSERT_OR_RETURN(, parent != nullptr, "No parent?");
			parent->closeParentScreen();
		});
	});

	chatBox->setOnEditingStoppedHandler([](W_EDITBOX& widg) {
		auto strongParent = std::dynamic_pointer_cast<WzInGameChatBoxForm>(widg.parent());
		ASSERT_OR_RETURN(, strongParent != nullptr, "No parent?");

		auto weakParent = std::weak_ptr<WzInGameChatBoxForm>(strongParent);
		widgScheduleTask([weakParent]() {
			auto parent = weakParent.lock();
			ASSERT_OR_RETURN(, parent != nullptr, "No parent?");
			parent->endChatBoxEditing();
		});
	});

	if (onTabHandler)
	{
		chatBox->setOnTabHandler(onTabHandler);
	}

	label = std::make_shared<W_LABEL>();
	attach(label);
	label->setGeometry(2, 2, 60, 16);
	label->setTextAlignment(WLAB_ALIGNTOPLEFT);

	if (NetPlay.bComms)
	{
		optionsButton = std::make_shared<WzInGameChatBoxButton>();
		attach(optionsButton);
		optionsButton->setString("\u2699"); // "⚙"
		optionsButton->setTip(_("Chat Options"));
		optionsButton->FontID = font_regular;
		int minButtonWidthForText = iV_GetTextWidth(optionsButton->pText, optionsButton->FontID);
		optionsButton->setGeometry(0, 0, minButtonWidthForText + 10, iV_GetTextLineSize(optionsButton->FontID));
		std::weak_ptr<WzInGameChatBoxForm> weakSelf = std::dynamic_pointer_cast<WzInGameChatBoxForm>(shared_from_this());
		optionsButton->addOnClickHandler([weakSelf](W_BUTTON& widg) {
			widgScheduleTask([weakSelf]() {
				auto strongParentForm = weakSelf.lock();
				ASSERT_OR_RETURN(, strongParentForm != nullptr, "No parent form");
				// Open the Chat Options overlay
				strongParentForm->displayOptionsOverlay();
			});
		});
	}

	setChatMode(initialChatMode);
}

void WzInGameChatBoxForm::displayOptionsOverlay()
{
	auto lockedScreen = screenPointer.lock();
	ASSERT(lockedScreen != nullptr, "The WzInGameChatBoxForm does not have an associated screen pointer?");

	// Initialize the options overlay screen
	optionsOverlayScreen = W_SCREEN::make();
	auto newRootFrm = W_FULLSCREENOVERLAY_CLICKFORM::make();
	std::weak_ptr<W_SCREEN> psWeakOptionsOverlayScreen(optionsOverlayScreen);
	std::weak_ptr<WzInGameChatBoxForm> psWeakChatBoxWidget = std::dynamic_pointer_cast<WzInGameChatBoxForm>(shared_from_this());
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
		// FUTURE TODO: could update the visible chat screen UI if change impacted selectedPlayer?
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
		// Try to position it below the chat box widget
		int idealChatOptionsHeight = psWidget->idealHeight();
		int y0 = std::max(20, psChatBoxWidget->screenPosY() + psChatBoxWidget->height() + 20);
		int maxHeight = screenHeight - y0 - 20;
		psWidget->setGeometry(x0, y0, w, std::min(maxHeight, idealChatOptionsHeight));
		// idealHeight may have changed due to the width change, so check and set again...
		int lastIdealChatOptionsHeight = idealChatOptionsHeight;
		idealChatOptionsHeight = psWidget->idealHeight();
		if (lastIdealChatOptionsHeight != idealChatOptionsHeight)
		{
			psWidget->setGeometry(x0, y0, w, std::min(maxHeight, idealChatOptionsHeight));
		}
	});

	widgRegisterOverlayScreenOnTopOfScreen(optionsOverlayScreen, lockedScreen);
}

// MARK: - WzInGameChatScreen_CLICKFORM

class WzInGameChatScreen_CLICKFORM : public W_CLICKFORM
{
protected:
	static constexpr int32_t INTERNAL_PADDING = 15;
protected:
	WzInGameChatScreen_CLICKFORM(W_FORMINIT const *init);
	WzInGameChatScreen_CLICKFORM();
	~WzInGameChatScreen_CLICKFORM();
	void initialize(WzChatMode initialChatMode);
	void recalcLayout();
public:
	static std::shared_ptr<WzInGameChatScreen_CLICKFORM> make(WzChatMode initialChatMode, UDWORD formID = 0);
	void clicked(W_CONTEXT *psContext, WIDGET_KEY key) override;
	void display(int xOffset, int yOffset) override;
	void run(W_CONTEXT *psContext) override;
	void geometryChanged() override
	{
		recalcLayout();
	}

private:
	void clearFocusAndEditing();
	void handleClickOnForm();

protected:
	friend struct WzInGameChatScreen;
	void giveQuickChatFocus();
	bool giveChatBoxFocus();

public:
	PIELIGHT backgroundColor = pal_RGBA(0, 0, 0, 80);
	std::function<void ()> onClickedFunc;
	std::function<void ()> onCancelPressed;
private:
	std::shared_ptr<WzInGameChatBoxForm> consoleBox;
	std::shared_ptr<W_FORM> quickChatForm;
};

WzInGameChatScreen_CLICKFORM::WzInGameChatScreen_CLICKFORM(W_FORMINIT const *init) : W_CLICKFORM(init) {}
WzInGameChatScreen_CLICKFORM::WzInGameChatScreen_CLICKFORM() : W_CLICKFORM() {}
WzInGameChatScreen_CLICKFORM::~WzInGameChatScreen_CLICKFORM()
{ }

std::shared_ptr<WzInGameChatScreen_CLICKFORM> WzInGameChatScreen_CLICKFORM::make(WzChatMode initialChatMode, UDWORD formID)
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

	class make_shared_enabler: public WzInGameChatScreen_CLICKFORM
	{
	public:
		make_shared_enabler(W_FORMINIT const *init): WzInGameChatScreen_CLICKFORM(init) {}
	};
	auto widget = std::make_shared<make_shared_enabler>(&sInit);
	widget->initialize(initialChatMode);
	return widget;
}

void WzInGameChatScreen_CLICKFORM::giveQuickChatFocus()
{
	if (!quickChatForm)
	{
		giveChatBoxFocus();
		return;
	}
	if (consoleBox)
	{
		consoleBox->endChatBoxEditing();
	}
	// Set the focus to the quickChatForm by simulating a click on it
	W_CONTEXT context = W_CONTEXT::ZeroContext();
	context.mx			= quickChatForm->screenPosX();
	context.my			= quickChatForm->screenPosY();
	quickChatForm->clicked(&context, WKEY_PRIMARY);
	quickChatForm->released(&context, WKEY_PRIMARY);
}

bool WzInGameChatScreen_CLICKFORM::giveChatBoxFocus()
{
	if (!consoleBox)
	{
		return false;
	}

	consoleBox->startChatBoxEditing();
	return true;
}

void WzInGameChatScreen_CLICKFORM::initialize(WzChatMode initialChatMode)
{
	bool freeChatEnabled = !bMultiPlayer || selectedPlayer >= MAX_CONNECTED_PLAYERS || ingame.hostChatPermissions[selectedPlayer];
	// for now, quick chat is only available in skirmish / multiplayer mode, for players
	bool quickChatEnabled = (bMultiPlayer && selectedPlayer < MAX_PLAYERS && !NetPlay.players[selectedPlayer].isSpectator);

	auto weakSelf = std::weak_ptr<WzInGameChatScreen_CLICKFORM>(std::dynamic_pointer_cast<WzInGameChatScreen_CLICKFORM>(shared_from_this()));
	consoleBox = WzInGameChatBoxForm::make(initialChatMode, [weakSelf](W_EDITBOX&) -> bool {
		// on tab handler
		widgScheduleTask([weakSelf]() {
			auto strongParentForm = weakSelf.lock();
			ASSERT_OR_RETURN(, strongParentForm != nullptr, "No parent form");
			strongParentForm->giveQuickChatFocus();
		});
		return true;
	});
	attach(consoleBox);
	consoleBox->id = CHAT_CONSOLEBOX;
	consoleBox->setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
		int desiredWidth = CHAT_CONSOLEBOXW;
		int x0 = (screenWidth - desiredWidth) / 2;
		psWidget->setGeometry(x0, CHAT_CONSOLEBOXY, desiredWidth, CHAT_CONSOLEBOXH);
	}));
	consoleBox->setChatBoxEnabledStatus(freeChatEnabled, quickChatEnabled);

	// for now, quick chat is only available in skirmish / multiplayer mode, for players
	if (quickChatEnabled)
	{
		quickChatForm = createQuickChatForm(WzQuickChatContext::InGame, [weakSelf]() {
				// on quick-chat sent
				widgScheduleTask([weakSelf]() {
					// click on the form so we ultimately close the chat screen
					auto strongParentForm = weakSelf.lock();
					ASSERT_OR_RETURN(, strongParentForm != nullptr, "No parent form");
					auto parentScreen = std::dynamic_pointer_cast<WzInGameChatScreen>(strongParentForm->screenPointer.lock());
					ASSERT_OR_RETURN(, parentScreen != nullptr, "No screen?");
					parentScreen->closeScreen();
				});
			},
			(initialChatMode == WzChatMode::Team) ? WzQuickChatMode::Team : WzQuickChatMode::Global);
		attach(quickChatForm);
		quickChatForm->setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
			constexpr int MAX_QUICKCHAT_WIDTH = 1200;
			int quickChatWidth = std::min<int32_t>(screenWidth - 40, MAX_QUICKCHAT_WIDTH);
			int x0 = std::max<int32_t>((screenWidth - quickChatWidth) / 2, 20);
			psWidget->setGeometry(x0, CHAT_CONSOLEBOXY + CHAT_CONSOLEBOXH + 100, quickChatWidth, psWidget->idealHeight());
		}));
	}

	recalcLayout();
}

void WzInGameChatScreen_CLICKFORM::recalcLayout()
{
	if (consoleBox)
	{
		consoleBox->callCalcLayout();
	}
}

void WzInGameChatScreen_CLICKFORM::clearFocusAndEditing()
{
	if (auto lockedScreen = screenPointer.lock())
	{
		lockedScreen->setFocus(nullptr);
	}
	if (consoleBox)
	{
		consoleBox->endChatBoxEditing();
	}
}

void WzInGameChatScreen_CLICKFORM::handleClickOnForm()
{
	clearFocusAndEditing();
	if (onClickedFunc)
	{
		onClickedFunc();
	}
}

void WzInGameChatScreen_CLICKFORM::clicked(W_CONTEXT *psContext, WIDGET_KEY key)
{
	handleClickOnForm();
}

void WzInGameChatScreen_CLICKFORM::display(int xOffset, int yOffset)
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
}

void WzInGameChatScreen_CLICKFORM::run(W_CONTEXT *psContext)
{
	if (keyPressed(KEY_ESC))
	{
		clearFocusAndEditing();
		if (onCancelPressed)
		{
			onCancelPressed();
		}
	}

	if (keyPressed(KEY_TAB))
	{
		// no child widget consumed the TAB keypress - swap focus between quick chat and chat box
		bool currFocusOnChatbox = false;
		if (auto lockedScreen = screenPointer.lock())
		{
			if (auto widgetWithFocus = lockedScreen->getWidgetWithFocus())
			{
				if (consoleBox)
				{
					if (widgetWithFocus == consoleBox || widgetWithFocus->hasAncestor(consoleBox.get()))
					{
						currFocusOnChatbox = true;
					}
				}
			}
		}

		if (currFocusOnChatbox)
		{
			giveQuickChatFocus();
		}
		else
		{
			giveChatBoxFocus();
		}
	}

	// while this is displayed, clear the input buffer
	// (ensuring that subsequent screens / the main screen do not get the input to handle)
	inputLoseFocus();
}

// MARK: - WzGameStartOverlayScreen

std::shared_ptr<WzInGameChatScreen> WzInGameChatScreen::make(const OnCloseFunc& _onCloseFunc, WzChatMode initialChatMode, bool startWithQuickChatFocused)
{
	class make_shared_enabler: public WzInGameChatScreen {};
	auto newRootFrm = WzInGameChatScreen_CLICKFORM::make(initialChatMode);
	auto screen = std::make_shared<make_shared_enabler>();
	screen->initialize(newRootFrm);
	screen->onCloseFunc = _onCloseFunc;
	std::weak_ptr<WzInGameChatScreen> psWeakHelpOverlayScreen(screen);
	newRootFrm->onClickedFunc = [psWeakHelpOverlayScreen]() {
		auto psOverlayScreen = psWeakHelpOverlayScreen.lock();
		if (psOverlayScreen)
		{
			psOverlayScreen->closeScreen();
		}
	};
	newRootFrm->onCancelPressed = newRootFrm->onClickedFunc;

	// must select default element focus *after* adding the root form to the screen
	if (startWithQuickChatFocused)
	{
		newRootFrm->giveQuickChatFocus();
	}
	else
	{
		bool chatBoxEnabled = !bMultiPlayer || selectedPlayer >= MAX_CONNECTED_PLAYERS || ingame.hostChatPermissions[selectedPlayer];
		if (chatBoxEnabled)
		{
			newRootFrm->giveChatBoxFocus();
		}
		else
		{
			newRootFrm->giveQuickChatFocus();
		}
	}

	return screen;
}

void WzInGameChatScreen::closeScreen()
{
	shutdownChatScreen();
	if (onCloseFunc)
	{
		onCloseFunc();
	}
}

std::shared_ptr<W_SCREEN> createChatScreen(std::function<void ()> onCloseFunc, WzChatMode initialChatMode, bool startWithQuickChatFocused)
{
	auto screen = WzInGameChatScreen::make(onCloseFunc, initialChatMode, startWithQuickChatFocused);
	widgRegisterOverlayScreenOnTopOfScreen(screen, psWScreen);
	psCurrentChatScreen = screen;
	return screen;
}

void shutdownChatScreen()
{
	if (auto strongGameStartScreen = psCurrentChatScreen.lock())
	{
		widgRemoveOverlayScreen(strongGameStartScreen);
		psCurrentChatScreen.reset();
	}
}
