// SPDX-License-Identifier: GPL-2.0-or-later

/*
	This file is part of Warzone 2100.
	Copyright (C) 2025  Warzone 2100 Project

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

#include "ingameopscreen.h"

#include "lib/framework/wzapp.h"
#include "lib/framework/input.h"
#include "lib/framework/math_ext.h"
#include "lib/ivis_opengl/pieblitfunc.h"
#include "lib/widget/button.h"
#include "../keybind.h"
#include "../loop.h"
#include "../mission.h"
#include "../intdisplay.h"

struct WzInGameOptionsScreen;

static std::weak_ptr<WzInGameOptionsScreen> previousInGameOptionsScreen;

// MARK: -

class WzInGameOptionsScreen_CLICKFORM : public W_CLICKFORM
{
protected:
	static constexpr int32_t INTERNAL_PADDING = 15;
protected:
	WzInGameOptionsScreen_CLICKFORM(W_FORMINIT const *init);
	WzInGameOptionsScreen_CLICKFORM();
	~WzInGameOptionsScreen_CLICKFORM();
	void initialize(const std::shared_ptr<WIDGET>& displayForm);
public:
	static std::shared_ptr<WzInGameOptionsScreen_CLICKFORM> make(const std::shared_ptr<WIDGET>& displayForm, UDWORD formID = 0);
	void clicked(W_CONTEXT *psContext, WIDGET_KEY key) override;
	void display(int xOffset, int yOffset) override;
	void run(W_CONTEXT *psContext) override;
	void geometryChanged() override
	{
		recalcLayout();
	}
	void recalcLayout();

private:
	void clearFocusAndEditing();
	void handleClickOnForm();
	void handleCloseButtonPressed();

public:
	PIELIGHT backgroundColor = pal_RGBA(0, 0, 0, 80);
	std::function<void ()> onClickedFunc;
	std::function<void ()> onCancelPressed;
private:
	std::shared_ptr<WIDGET> wrapperForm;
	const int edgeMargin = 10;
};

struct WzInGameOptionsScreen: public W_SCREEN
{
protected:
	WzInGameOptionsScreen(): W_SCREEN() {}
	~WzInGameOptionsScreen()
	{
		setFocus(nullptr);
	}

public:
	typedef std::function<void ()> OnCloseFunc;
	static std::shared_ptr<WzInGameOptionsScreen> make(const std::shared_ptr<WIDGET>& displayForm, const OnCloseFunc& onCloseFunction);
	void reopeningScreen(const OnCloseFunc& onCloseFunction);

public:
	void closeScreen(bool force = false);

private:
	void pauseGameIfNeeded();
	void unpauseGameIfNeeded();

private:
	OnCloseFunc onCloseFunc;
	std::chrono::steady_clock::time_point openTime;
	bool didStopGameTime = false;
	bool didSetPauseStates = false;

private:
	WzInGameOptionsScreen(WzInGameOptionsScreen const &) = delete;
	WzInGameOptionsScreen &operator =(WzInGameOptionsScreen const &) = delete;
};

// MARK: - WzInGameOptionsScreen implementation

WzInGameOptionsScreen_CLICKFORM::WzInGameOptionsScreen_CLICKFORM(W_FORMINIT const *init) : W_CLICKFORM(init) {}
WzInGameOptionsScreen_CLICKFORM::WzInGameOptionsScreen_CLICKFORM() : W_CLICKFORM() {}
WzInGameOptionsScreen_CLICKFORM::~WzInGameOptionsScreen_CLICKFORM()
{ }

std::shared_ptr<WzInGameOptionsScreen_CLICKFORM> WzInGameOptionsScreen_CLICKFORM::make(const std::shared_ptr<WIDGET>& displayForm, UDWORD formID)
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

	class make_shared_enabler: public WzInGameOptionsScreen_CLICKFORM
	{
	public:
		make_shared_enabler(W_FORMINIT const *init): WzInGameOptionsScreen_CLICKFORM(init) {}
	};
	auto widget = std::make_shared<make_shared_enabler>(&sInit);
	widget->initialize(displayForm);
	return widget;
}

constexpr int GuideNavBarButtonHorizontalPadding = 5;

class InGameOptionsTitleBarButton : public W_BUTTON
{
public:
	static std::shared_ptr<InGameOptionsTitleBarButton> make(const WzString& buttonText, iV_fonts FontID = font_regular)
	{
		auto result = std::make_shared<InGameOptionsTitleBarButton>();
		result->FontID = FontID;
		result->pText = buttonText;
		return result;
	}
	static std::shared_ptr<InGameOptionsTitleBarButton> make(UWORD intfacImageID, int imageDimensions = 16)
	{
		if (!assertValidImage(IntImages, intfacImageID))
		{
			return nullptr;
		}
		auto result = std::make_shared<InGameOptionsTitleBarButton>();
		result->intfacImageID = intfacImageID;
		result->imageDimensions = imageDimensions;
		return result;
	}
public:
	virtual int32_t idealWidth() override
	{
		if (intfacImageID.has_value())
		{
			return imageDimensions + (GuideNavBarButtonHorizontalPadding * 2);
		}
		return iV_GetTextWidth(pText, FontID) + (GuideNavBarButtonHorizontalPadding * 2);
	}

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

		// Draw background (if needed)
		optional<PIELIGHT> backColor;
		if (!isDisabled)
		{
			if (isDown)
			{
				backColor = WZCOL_MENU_SCORE_BUILT;
			}
			else if (isHighlight)
			{
				backColor = pal_RGBA(255,255,255,50);
			}
			if (backColor.has_value())
			{
				pie_UniTransBoxFill(xPos, yPos, xPos + w, yPos + h, backColor.value());
			}
		}

		// Select label text and color
		PIELIGHT msgColour = (!isDown) ? ((isHighlight) ? WZCOL_TEXT_BRIGHT : WZCOL_TEXT_MEDIUM) : WZCOL_FORM_TEXT;
		if (isDisabled)
		{
			msgColour = WZCOL_TEXT_MEDIUM;
			msgColour.byte.a = msgColour.byte.a / 2;
		}

		if (intfacImageID.has_value())
		{
			// Display the image centered
			int imgPosX0 = xPos + (w - imageDimensions) / 2;
			int imgPosY0 = yPos + (h - imageDimensions) / 2;
			PIELIGHT imgColor = msgColour;
			iV_DrawImageFileAnisotropicTint(IntImages, intfacImageID.value(), imgPosX0, imgPosY0, Vector2f(imageDimensions, imageDimensions), imgColor);
			return; // shortcut the rest
		}

		iV_fonts fontID = wzMessageText.getFontID();
		if (fontID == font_count || lastWidgetWidth != width() || wzMessageText.getText() != pText)
		{
			fontID = FontID;
		}
		wzMessageText.setText(pText, fontID);

		int availableButtonTextWidth = w - (GuideNavBarButtonHorizontalPadding * 2);
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

		int textX0 = std::max(xPos + GuideNavBarButtonHorizontalPadding, xPos + ((w - wzMessageText.width()) / 2));
		int textY0 = static_cast<int>(yPos + (h - wzMessageText.lineSize()) / 2 - float(wzMessageText.aboveBase()));

		int maxTextDisplayableWidth = w - (GuideNavBarButtonHorizontalPadding * 2);
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
		if (!isTruncated) {
			return pTip;
		}

		return pText.toUtf8();
	}
private:
	WzText wzMessageText;
	optional<UWORD> intfacImageID = nullopt;
	int imageDimensions = 16;
	int lastWidgetWidth = 0;
	bool isTruncated = false;
};

class InGameOptionsTitleBar : public WIDGET
{
public:
	typedef std::function<void ()> OnCloseFunc;
protected:
	InGameOptionsTitleBar(): WIDGET() {}
	void initialize(const OnCloseFunc& onCloseHandler);
protected:
	virtual void geometryChanged() override;
	virtual void display(int xOffset, int yOffset) override;
public:
	static std::shared_ptr<InGameOptionsTitleBar> make(const OnCloseFunc& onCloseHandler);

	virtual int32_t idealHeight() override;

private:
	void updateLayout();
	void triggerCloseHandler();

private:
	std::shared_ptr<W_BUTTON> m_closeButton;
	OnCloseFunc m_onCloseHandler;
};

std::shared_ptr<InGameOptionsTitleBar> InGameOptionsTitleBar::make(const OnCloseFunc& onCloseHandler)
{
	class make_shared_enabler: public InGameOptionsTitleBar {};
	auto result = std::make_shared<make_shared_enabler>();
	result->initialize(onCloseHandler);
	return result;
}

void InGameOptionsTitleBar::initialize(const OnCloseFunc& onCloseHandler)
{
	m_onCloseHandler = onCloseHandler;

	std::weak_ptr<InGameOptionsTitleBar> weakSelf(std::dynamic_pointer_cast<InGameOptionsTitleBar>(shared_from_this()));

	// Close button
	m_closeButton = InGameOptionsTitleBarButton::make(WzString::fromUtf8("\u2715"), font_regular); // ✕
	m_closeButton->setTip(_("Close"));
	attach(m_closeButton);
	m_closeButton->addOnClickHandler([weakSelf](W_BUTTON&) {
		auto strongSelf = weakSelf.lock();
		ASSERT_OR_RETURN(, strongSelf != nullptr, "No parent?");
		strongSelf->triggerCloseHandler();
	});
}

void InGameOptionsTitleBar::display(int xOffset, int yOffset)
{
	int x0 = x() + xOffset;
	int y0 = y() + yOffset;
	int h = height();
	int w = width();
	iV_Line(x0, y0 + h, x0 + w, y0 + h, pal_RGBA(0,0,0,150));
}

void InGameOptionsTitleBar::geometryChanged()
{
	updateLayout();
}

void InGameOptionsTitleBar::updateLayout()
{
	int w = width();
	int h = height();

	int widgetHeight = h - (GuideNavBarButtonHorizontalPadding * 2);
	int widgetY0 = (h - widgetHeight) / 2;

	int buttonWidth = m_closeButton->idealWidth();

	// Right-align the close button
	m_closeButton->setGeometry(w - (buttonWidth + GuideNavBarButtonHorizontalPadding), widgetY0, buttonWidth, widgetHeight);
}

void InGameOptionsTitleBar::triggerCloseHandler()
{
	ASSERT_OR_RETURN(, m_onCloseHandler != nullptr, "Missing onCloseHandler");
	m_onCloseHandler();
}

int32_t InGameOptionsTitleBar::idealHeight()
{
	return 30;
}

class InGameOptionsBaseForm : public IntFormAnimated
{
protected:
	InGameOptionsBaseForm()
	: IntFormAnimated(true)
	{ }
	void initialize(const std::shared_ptr<WIDGET>& displayForm_, const std::function<void ()>& onCloseFunc)
	{
		titleBar = InGameOptionsTitleBar::make(onCloseFunc);
		attach(titleBar);

		displayForm = displayForm_;
		attach(displayForm);
	}
public:
	static std::shared_ptr<InGameOptionsBaseForm> make(const std::shared_ptr<WIDGET>& displayForm, const std::function<void ()>& onCloseFunc)
	{
		class make_shared_enabler: public InGameOptionsBaseForm {};
		auto result = std::make_shared<make_shared_enabler>();
		result->initialize(displayForm, onCloseFunc);
		return result;
	}
	void geometryChanged() override
	{
		int w = width();
		int h = height();
		titleBar->setGeometry(internalFormPadding, internalFormPadding, w - (internalFormPadding * 2), titleBar->idealHeight());
		int displayFormY0 = titleBar->y() + titleBar->height();
		int displayFormHeight = h - displayFormY0 - internalFormPadding;
		displayForm->setGeometry(internalFormPadding, displayFormY0, w - (internalFormPadding * 2), displayFormHeight);
	};
	int32_t idealWidth() override
	{
		return displayForm->idealWidth() + (internalFormPadding * 2);
	}
	int32_t idealHeight() override
	{
		return titleBar->idealHeight() + displayForm->idealHeight() + (internalFormPadding * 2);
	}
private:
	std::shared_ptr<InGameOptionsTitleBar> titleBar;
	std::shared_ptr<WIDGET> displayForm;
	int32_t internalFormPadding = 1;
};

void WzInGameOptionsScreen_CLICKFORM::initialize(const std::shared_ptr<WIDGET>& displayForm_)
{
	auto weakSelf = std::weak_ptr<WzInGameOptionsScreen_CLICKFORM>(std::dynamic_pointer_cast<WzInGameOptionsScreen_CLICKFORM>(shared_from_this()));

	wrapperForm = InGameOptionsBaseForm::make(displayForm_, [weakSelf]() {
		widgScheduleTask([weakSelf](){
			auto strongSelf = weakSelf.lock();
			ASSERT_OR_RETURN(, strongSelf != nullptr, "No parent?");
			strongSelf->handleCloseButtonPressed();
		});
	});
	attach(wrapperForm);
	recalcLayout();
}

void WzInGameOptionsScreen_CLICKFORM::recalcLayout()
{
	int w = width();
	int h = height();

	int maxFormWidth = w - (edgeMargin * 2);
	int maxFormHeight = h - (edgeMargin * 2);

	// ensure displayForm is centered
	int displayFormWidth = std::min(maxFormWidth, wrapperForm->idealWidth());
	int displayFormHeight = std::min(maxFormHeight, wrapperForm->idealHeight());

	int x0 = (w - displayFormWidth) / 2;
	int y0 = (h - displayFormHeight) / 2;

	wrapperForm->setGeometry(x0, y0, displayFormWidth, displayFormHeight);
}

void WzInGameOptionsScreen_CLICKFORM::clearFocusAndEditing()
{
	if (auto lockedScreen = screenPointer.lock())
	{
		lockedScreen->setFocus(nullptr);
	}
}

void WzInGameOptionsScreen_CLICKFORM::handleClickOnForm()
{
	clearFocusAndEditing();
	if (onClickedFunc)
	{
		onClickedFunc();
	}
}

void WzInGameOptionsScreen_CLICKFORM::handleCloseButtonPressed()
{
	clearFocusAndEditing();
	if (onCancelPressed)
	{
		onCancelPressed();
	}
}

void WzInGameOptionsScreen_CLICKFORM::clicked(W_CONTEXT *psContext, WIDGET_KEY key)
{
	handleClickOnForm();
}

void WzInGameOptionsScreen_CLICKFORM::display(int xOffset, int yOffset)
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

void WzInGameOptionsScreen_CLICKFORM::run(W_CONTEXT *psContext)
{
	if (keyPressed(KEY_ESC))
	{
		clearFocusAndEditing();
		if (onCancelPressed)
		{
			onCancelPressed();
			inputLoseFocus();
		}
	}

	if (visible())
	{
		// while this is displayed, clear the input buffer
		// (ensuring that subsequent screens / the main screen do not get the input to handle)
		inputLoseFocus();
	}
}

std::shared_ptr<WzInGameOptionsScreen> WzInGameOptionsScreen::make(const std::shared_ptr<WIDGET>& displayForm, const OnCloseFunc& _onCloseFunc)
{
	class make_shared_enabler: public WzInGameOptionsScreen {};
	auto newRootFrm = WzInGameOptionsScreen_CLICKFORM::make(displayForm);
	auto screen = std::make_shared<make_shared_enabler>();
	screen->initialize(newRootFrm);
	screen->onCloseFunc = _onCloseFunc;
	std::weak_ptr<WzInGameOptionsScreen> psWeakOverlayScreen(screen);
	newRootFrm->onClickedFunc = [psWeakOverlayScreen]() {
		auto psOverlayScreen = psWeakOverlayScreen.lock();
		if (psOverlayScreen)
		{
			psOverlayScreen->closeScreen(false);
		}
	};
	newRootFrm->onCancelPressed = [psWeakOverlayScreen]() {
		auto psOverlayScreen = psWeakOverlayScreen.lock();
		if (psOverlayScreen)
		{
			psOverlayScreen->closeScreen(true);
		}
	};

	screen->pauseGameIfNeeded();
	screen->openTime = std::chrono::steady_clock::now();

	return screen;
}

void WzInGameOptionsScreen::reopeningScreen(const OnCloseFunc& onCloseFunction)
{
	onCloseFunc = onCloseFunction;
	pauseGameIfNeeded();
	openTime = std::chrono::steady_clock::now();
}

void WzInGameOptionsScreen::closeScreen(bool force)
{
	if (!force && (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - openTime) < std::chrono::milliseconds(250)))
	{
		// ignore errant clicks that might otherwise close screen right after it opened
		return;
	}
	widgRemoveOverlayScreen(shared_from_this());
	unpauseGameIfNeeded();
	if (onCloseFunc)
	{
		onCloseFunc();
		// "consumes" the onCloseFunc
		onCloseFunc = nullptr;
	}
}

void WzInGameOptionsScreen::pauseGameIfNeeded()
{
	if (!runningMultiplayer() && !gameTimeIsStopped())
	{
		/* Stop the clock / simulation */
		gameTimeStop();
		addConsoleMessage(_("PAUSED"), CENTRE_JUSTIFY, SYSTEM_MESSAGE, false, MAX_CONSOLE_MESSAGE_DURATION);

		psForm->show(); // show the root form, which means it accepts all clicks on the screen and dims the background

		didStopGameTime = true;

		if (!gamePaused())
		{
			setGamePauseStatus(true);
			setConsolePause(true);
			setScriptPause(true);
			setAudioPause(true);
			setScrollPause(true);
			didSetPauseStates = true;

			// If cursor trapping is enabled allow the cursor to leave the window
			wzReleaseMouse();
		}
	}
	else
	{
		psForm->hide(); // hide the root form - it won't accept clicks (but its children will be displayed)
	}
}

void WzInGameOptionsScreen::unpauseGameIfNeeded()
{
	if (didStopGameTime)
	{
		if (!runningMultiplayer() && gameTimeIsStopped())
		{
			clearActiveConsole();
			/* Start the clock again */
			gameTimeStart();

			//put any widgets back on for the missions
			resetMissionWidgets();
		}
		didStopGameTime = false;
	}

	if (didSetPauseStates)
	{
		setGamePauseStatus(false);
		setConsolePause(false);
		setScriptPause(false);
		setAudioPause(false);
		setScrollPause(false);
		didSetPauseStates = false;

		// Re-enable cursor trapping if it is enabled
		if (shouldTrapCursor())
		{
			wzGrabMouse();
		}
	}
}

// MARK: -

void showInGameOptionsScreen(const std::shared_ptr<W_SCREEN>& onTopOfScreen, const std::shared_ptr<WIDGET>& displayForm, const std::function<void ()>& onCloseFunc)
{
	if (auto strongCurrent = previousInGameOptionsScreen.lock())
	{
		strongCurrent->closeScreen(true);
		previousInGameOptionsScreen.reset();
	}
	auto newScreen = WzInGameOptionsScreen::make(displayForm, onCloseFunc);
	widgRegisterOverlayScreenOnTopOfScreen(newScreen, onTopOfScreen);
	previousInGameOptionsScreen = newScreen;
}
