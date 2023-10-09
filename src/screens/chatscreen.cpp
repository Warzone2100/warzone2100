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

public:
	void setChatMode(WzChatMode newMode);
	void startChatBoxEditing();
	void endChatBoxEditing();

protected:
	void closeParentScreen()
	{
		auto parentScreen = std::dynamic_pointer_cast<WzInGameChatScreen>(screenPointer.lock());
		ASSERT_OR_RETURN(, parentScreen != nullptr, "No screen?");
		parentScreen->closeScreen();
	}

private:
	WzChatMode chatMode;
	std::shared_ptr<W_EDITBOX> chatBox;
	std::shared_ptr<W_LABEL> label;
};

WzInGameChatBoxForm::~WzInGameChatBoxForm()
{
	// currently, no-op
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
		const char* pStr = enteredText.toUtf8().c_str();
		auto message = InGameChatMessage(selectedPlayer, pStr);
		attemptCheatCode(message.text);		// parse the message

		switch (strongParent->chatMode)
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

		inputLoseFocus();

		strongParent->endChatBoxEditing();
		strongParent->closeParentScreen();
	});

	chatBox->setOnEscapeHandler([](W_EDITBOX& widg) {
		auto strongParent = std::dynamic_pointer_cast<WzInGameChatBoxForm>(widg.parent());
		ASSERT_OR_RETURN(, strongParent != nullptr, "No parent?");

		inputLoseFocus();
		strongParent->closeParentScreen();
	});

	chatBox->setOnEditingStoppedHandler([](W_EDITBOX& widg) {
		auto strongParent = std::dynamic_pointer_cast<WzInGameChatBoxForm>(widg.parent());
		ASSERT_OR_RETURN(, strongParent != nullptr, "No parent?");
		strongParent->endChatBoxEditing();
	});

	if (onTabHandler)
	{
		chatBox->setOnTabHandler(onTabHandler);
	}

	label = std::make_shared<W_LABEL>();
	attach(label);
	label->setGeometry(2, 2, 60, 16);
	label->setTextAlignment(WLAB_ALIGNTOPLEFT);

	setChatMode(initialChatMode);
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
		psWidget->setGeometry(CHAT_CONSOLEBOXX, CHAT_CONSOLEBOXY, CHAT_CONSOLEBOXW, CHAT_CONSOLEBOXH);
	}));

	// for now, quick chat is only available in skirmish / multiplayer mode, for players
	if (bMultiPlayer && selectedPlayer < MAX_PLAYERS && !NetPlay.players[selectedPlayer].isSpectator)
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

	if (backgroundColor.rgba == 0)
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
		bool chatBoxEnabled = true;
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
