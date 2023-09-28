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
	static std::shared_ptr<WzInGameChatScreen> make(const OnCloseFunc& onCloseFunction, WzChatMode initialChatMode);

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
	void initialize(WzChatMode initialChatMode);

public:
	static std::shared_ptr<WzInGameChatBoxForm> make(WzChatMode initialChatMode)
	{
		class make_shared_enabler: public WzInGameChatBoxForm {};
		auto widget = std::make_shared<make_shared_enabler>();
		widget->initialize(initialChatMode);
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
}

void WzInGameChatBoxForm::endChatBoxEditing()
{
	chatBox->focusLost();
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

void WzInGameChatBoxForm::initialize(WzChatMode initialChatMode)
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

public:
	PIELIGHT backgroundColor = pal_RGBA(0, 0, 0, 80);
	std::function<void ()> onClickedFunc;
	std::function<void ()> onCancelPressed;
private:
	std::shared_ptr<WzInGameChatBoxForm> consoleBox;
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

void WzInGameChatScreen_CLICKFORM::initialize(WzChatMode initialChatMode)
{
	consoleBox = WzInGameChatBoxForm::make(initialChatMode);
	attach(consoleBox);
	consoleBox->id = CHAT_CONSOLEBOX;
	consoleBox->setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
		psWidget->setGeometry(CHAT_CONSOLEBOXX, CHAT_CONSOLEBOXY, CHAT_CONSOLEBOXW, CHAT_CONSOLEBOXH);
	}));
	consoleBox->startChatBoxEditing();

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

void WzInGameChatScreen_CLICKFORM::clicked(W_CONTEXT *psContext, WIDGET_KEY key)
{
	clearFocusAndEditing();
	if (onClickedFunc)
	{
		onClickedFunc();
	}
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

	// while this is displayed, clear the input buffer
	// (ensuring that subsequent screens / the main screen do not get the input to handle)
	inputLoseFocus();
}

// MARK: - WzGameStartOverlayScreen

std::shared_ptr<WzInGameChatScreen> WzInGameChatScreen::make(const OnCloseFunc& _onCloseFunc, WzChatMode initialChatMode)
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

std::shared_ptr<W_SCREEN> createChatScreen(std::function<void ()> onCloseFunc, WzChatMode initialChatMode)
{
	auto screen = WzInGameChatScreen::make(onCloseFunc, initialChatMode);
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
