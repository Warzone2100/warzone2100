// SPDX-License-Identifier: GPL-2.0-or-later

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
 *  Start Hosting Button
 */

#include "starthostingbutton.h"
#include "frontendimagebutton.h"
#include "optionsform.h"
#include "src/frend.h"
#include "lib/ivis_opengl/pieblitfunc.h"
#include "lib/widget/form.h"
#include "lib/widget/label.h"
#include "lib/netplay/netplay.h"
#include "src/multiplay.h"
#include "src/warzoneconfig.h"


// MARK: - HostingAdvancedOptionsForm

class HostingAdvancedOptionsForm : public WIDGET
{
protected:
	HostingAdvancedOptionsForm() {}

	void initialize(const std::shared_ptr<WzStartHostingButton>& parent, const std::function<void ()>& onClose);
public:
	static std::shared_ptr<HostingAdvancedOptionsForm> make(const std::shared_ptr<WzStartHostingButton>& parent, const std::function<void ()>& onCloseFunc)
	{
		class make_shared_enabler: public HostingAdvancedOptionsForm {};
		auto widget = std::make_shared<make_shared_enabler>();
		widget->initialize(parent, onCloseFunc);
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
	std::weak_ptr<WzMultiplayerOptionsTitleUI> multiOptionsTitleUI;
	std::weak_ptr<WzStartHostingButton> parentStartHostingButton;
	std::shared_ptr<W_LABEL> formTitle;
	std::shared_ptr<WzFrontendImageButton> closeButton;
	std::shared_ptr<OptionsForm> hostOptionsForm;
	const int innerVerticalSpacing = 15;
	const int outerPaddingX = 20;
	const int optionsListPaddingX = 10;
	const int outerPaddingY = 20;
	const int betweenButtonPadding = 15;
	const int closeButtonVerticalPadding = 5;
};

void HostingAdvancedOptionsForm::initialize(const std::shared_ptr<WzStartHostingButton>& parent, const std::function<void ()>& onClose)
{
	multiOptionsTitleUI = parent->getMultiOptionsTitleUI();

	formTitle = std::make_shared<W_LABEL>();
	formTitle->setFont(font_regular_bold, WZCOL_TEXT_MEDIUM);
	formTitle->setString(_("Advanced Hosting Options:"));
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

int32_t HostingAdvancedOptionsForm::idealWidth()
{
	return (outerPaddingX * 2) + optionsListPaddingX +  hostOptionsForm->idealWidth();
}

int32_t HostingAdvancedOptionsForm::idealHeight()
{
	int32_t result = (outerPaddingY * 2) + std::max(formTitle->idealHeight(), closeButton->idealHeight() - closeButtonVerticalPadding);
	result += innerVerticalSpacing;
	result += hostOptionsForm->idealHeight();
	return result;
}

std::shared_ptr<OptionsForm> HostingAdvancedOptionsForm::makeHostOptionsForm()
{
	std::weak_ptr<HostingAdvancedOptionsForm> psWeakParent = std::dynamic_pointer_cast<HostingAdvancedOptionsForm>(shared_from_this());
	auto weakMultiOptionsTitleUI = multiOptionsTitleUI;
	auto result = OptionsForm::make();
	{
		auto optionInfo = OptionInfo("mp.hostRole", N_("Host Role"), "");
		auto valueChanger = OptionsDropdown<bool>::make(
			[weakMultiOptionsTitleUI]() {
				OptionChoices<bool> result;
				result.choices = {
					{ _("Player Host"), _("Host is a player in the game (the default)."), false },
					{ _("Spectator Host"), _("Host is a spectator in the game."), true }
				};
				bool current_SpectatorHostValue = false;
				auto strongMultiOptionsTitleUI = weakMultiOptionsTitleUI.lock();
				ASSERT(strongMultiOptionsTitleUI != nullptr, "No parent multi options title ui?");
				if (strongMultiOptionsTitleUI)
				{
					current_SpectatorHostValue = strongMultiOptionsTitleUI->getOption_SpectatorHost();
				}
				result.currentIdx = (current_SpectatorHostValue) ? 1 : 0;
				return result;
			},
			[psWeakParent](const auto& newValue) -> bool {
				auto strongParent = psWeakParent.lock();
				ASSERT_OR_RETURN(false, strongParent != nullptr, "No parent?");
				return strongParent->setSpectatorHost(newValue);
			}, false
		);
		result->addOption(optionInfo, valueChanger);
	}
	{
		auto optionInfo = OptionInfo("mp.blindMode", N_("Blind Mode"), N_("Whether and when players are blinded to other players' identities."))
			.addAvailabilityCondition([weakMultiOptionsTitleUI](const OptionInfo&) -> OptionInfo::AvailabilityResult {
			auto strongMultiOptionsTitleUI = weakMultiOptionsTitleUI.lock();
			ASSERT_OR_RETURN({}, strongMultiOptionsTitleUI != nullptr, "No parent multi options title ui?");
			return {strongMultiOptionsTitleUI->getOption_SpectatorHost(), _("Blind modes are only available when host is a Spectator Host.")};
		});
		auto valueChanger = OptionsDropdown<BLIND_MODE>::make(
			[weakMultiOptionsTitleUI]() {
				auto strongMultiOptionsTitleUI = weakMultiOptionsTitleUI.lock();
				ASSERT(strongMultiOptionsTitleUI != nullptr, "No parent multi options title ui?");
				bool current_SpectatorHostValue = strongMultiOptionsTitleUI->getOption_SpectatorHost();
				OptionChoices<BLIND_MODE> result;
				result.choices = {
					{ _("None"), _("Players' identities are visible, in lobby and in game (the default)."), BLIND_MODE::NONE },
					{ _("Blind Lobby"), _("Players' identities are hidden from everyone (except the spectator host) until the game starts."), BLIND_MODE::BLIND_LOBBY, !current_SpectatorHostValue },
					{ _("Blind Game"),  _("Players' identities are hidden from everyone (except the spectator host) until the game ends."), BLIND_MODE::BLIND_GAME, !current_SpectatorHostValue }
				};
				switch (game.blindMode)
				{
					case BLIND_MODE::NONE:
						result.currentIdx = 0;
						break;
					case BLIND_MODE::BLIND_LOBBY:
					case BLIND_MODE::BLIND_LOBBY_SIMPLE_LOBBY:
						result.currentIdx = 1;
						break;
					case BLIND_MODE::BLIND_GAME:
					case BLIND_MODE::BLIND_GAME_SIMPLE_LOBBY:
						result.currentIdx = 2;
						break;
				}
				return result;
			},
			[psWeakParent](const auto& newValue) -> bool {
				auto strongParent = psWeakParent.lock();
				ASSERT_OR_RETURN(false, strongParent != nullptr, "No parent?");
				return strongParent->setBlindMode(newValue);
			}, true
		);
		result->addOption(optionInfo, valueChanger);
	}
	return result;
}

bool HostingAdvancedOptionsForm::setSpectatorHost(bool value)
{
	auto strongMultiOptionsTitleUI = multiOptionsTitleUI.lock();
	ASSERT_OR_RETURN(false, strongMultiOptionsTitleUI != nullptr, "No parent multi options title ui?");
	strongMultiOptionsTitleUI->setOption_SpectatorHost(value);
	return true;
}

bool HostingAdvancedOptionsForm::setBlindMode(BLIND_MODE value)
{
	auto strongMultiOptionsTitleUI = multiOptionsTitleUI.lock();
	ASSERT_OR_RETURN(false, strongMultiOptionsTitleUI != nullptr, "No parent multi options title ui?");
	ASSERT_OR_RETURN(false, value == BLIND_MODE::NONE || strongMultiOptionsTitleUI->getOption_SpectatorHost(), "Blind modes are only available when host is a spectator");
	game.blindMode = value;
	return true;
}

void HostingAdvancedOptionsForm::geometryChanged()
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

	int maxAvailableOptionsFormHeight = height() - (outerPaddingY * 2) - std::max<int>(formTitle->height(), closeButton->height() - closeButtonVerticalPadding) - innerVerticalSpacing;
	int hostOptionsFormY0 = outerPaddingY + std::max(formTitle->idealHeight(), closeButton->idealHeight()) + innerVerticalSpacing;
	hostOptionsForm->setGeometry(outerPaddingX + optionsListPaddingX, hostOptionsFormY0, width() - (outerPaddingX * 2) - optionsListPaddingX, std::min<int32_t>(maxAvailableOptionsFormHeight, hostOptionsForm->idealHeight()));

}

void HostingAdvancedOptionsForm::display(int xOffset, int yOffset)
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


// MARK: - WzStartHostingButton

std::shared_ptr<WzStartHostingButton> WzStartHostingButton::make(const std::shared_ptr<WzMultiplayerOptionsTitleUI>& parentTitleUI)
{
	class make_shared_enabler: public WzStartHostingButton {};
	auto widget = std::make_shared<make_shared_enabler>();
	widget->multiOptionsTitleUI = parentTitleUI;
	widget->initialize();
	return widget;
}

WzStartHostingButton::WzStartHostingButton()
{ }

WzStartHostingButton::~WzStartHostingButton()
{
	closeOptionsOverlay();
}

void WzStartHostingButton::initialize()
{
	setString(_("Start Hosting"));
	FontID = font_medium;
	int minButtonWidthForText = iV_GetTextWidth(pText, FontID);
	setGeometry(0, 0, minButtonWidthForText + 10, iV_GetTextLineSize(FontID) + 16);

	bool showOptionsButton = (NetPlay.bComms && ingame.side == InGameSide::HOST_OR_SINGLEPLAYER);
	if (showOptionsButton)
	{
		std::weak_ptr<WzStartHostingButton> weakSelf = std::dynamic_pointer_cast<WzStartHostingButton>(shared_from_this());
		// Initialize an embedded options button to the right
		auto but = WzFrontendImageButton::make(IMAGE_GEAR);
		but->setBorderDrawMode(WzFrontendImageButton::BorderDrawMode::Highlighted);
		but->setPadding(0, 0);
		but->setImageDimensions(16);
		but->setTip(_("Configure Advanced Hosting Options"));
		optionsButton = but;
		attach(optionsButton);
		optionsButton->addOnClickHandler([weakSelf](W_BUTTON&) {
			widgScheduleTask([weakSelf]() {
				auto strongParentForm = weakSelf.lock();
				ASSERT_OR_RETURN(, strongParentForm != nullptr, "No parent form");
				strongParentForm->displayOptionsOverlay(strongParentForm->optionsButton);
			});
		});
	}
}

std::shared_ptr<WzMultiplayerOptionsTitleUI> WzStartHostingButton::getMultiOptionsTitleUI()
{
	return multiOptionsTitleUI.lock();
}

void WzStartHostingButton::geometryChanged()
{
	if (optionsButton)
	{
		int butWidth = std::max<int>(height(), 16);
		int x0 = width() - butWidth;
		optionsButton->setGeometry(x0, 0, butWidth, height());
	}
}

void WzStartHostingButton::display(int xOffset, int yOffset)
{
	int x0 = x() + xOffset;
	int y0 = y() + yOffset;
	int w = width();
	int h = height();

	bool haveText = !pText.isEmpty();

	bool isDown = (getState() & (WBUT_DOWN | WBUT_LOCK | WBUT_CLICKLOCK)) != 0;
	bool isDisabled = (getState() & WBUT_DISABLE) != 0;
	bool isHighlight = !isDisabled && ((getState() & WBUT_HIGHLIGHT) != 0);

	// Display the button.
	auto border_color = (isHighlight) ? WZCOL_TEXT_BRIGHT : WZCOL_TEXT_MEDIUM;
	auto background_color = isDown || isDisabled ? pal_RGBA(10, 0, 70, 250) : WZCOL_MENU_BACKGROUND;
	if (isDisabled)
	{
		border_color = WZCOL_TEXT_DARK;
		background_color.byte.a = (background_color.byte.a / 2);
	}
	else if (isDown)
	{
		background_color = WZCOL_FORM_DARK;
	}

	int backX0 = x0;
	int backY0 = y0;
	pie_BoxFill(backX0, backY0, backX0 + w, backY0 + h, border_color);
	pie_BoxFill(backX0 + 1, backY0 + 1, backX0 + w - 1, backY0 + h - 1, background_color);

	if (haveText)
	{
		wzText.setText(pText, FontID);
		int fw = wzText.width();
		int fx = x0 + (width() - fw) / 2;
		int fy = y0 + (height() - wzText.lineSize()) / 2 - wzText.aboveBase();
		PIELIGHT textColor = WZCOL_TEXT_BRIGHT;
		if (isDisabled)
		{
			textColor = WZCOL_TEXT_DARK;
			textColor.byte.a = (textColor.byte.a / 2);
		}
		else if (isHighlight)
		{
			textColor = WZCOL_TEXT_BRIGHT;
		}
		wzText.render(fx, fy, textColor);
	}
}

std::shared_ptr<WIDGET> WzStartHostingButton::createOptionsPopoverForm()
{
	std::weak_ptr<WzStartHostingButton> psWeakSelf(std::dynamic_pointer_cast<WzStartHostingButton>(shared_from_this()));
	auto optionsPopOver = HostingAdvancedOptionsForm::make(std::dynamic_pointer_cast<WzStartHostingButton>(shared_from_this()), [psWeakSelf](){
		if (auto strongSelf = psWeakSelf.lock())
		{
			strongSelf->closeOptionsOverlay();
		}
	});
	auto maxPopoverHeight = 350;
	optionsPopOver->setGeometry(0, 0, FRONTEND_BUTWIDTH - 100, maxPopoverHeight); // set once to set width
	optionsPopOver->setGeometry(0, 0, optionsPopOver->width(), std::min<int32_t>(maxPopoverHeight, optionsPopOver->idealHeight())); // set again to set height based on idealHeight for that width (if smaller)
	return optionsPopOver;
}

void WzStartHostingButton::closeOptionsOverlay()
{
	if (optionsOverlayScreen)
	{
		widgRemoveOverlayScreen(optionsOverlayScreen);
		optionsOverlayScreen.reset();
	}
}

void WzStartHostingButton::displayOptionsOverlay(const std::shared_ptr<WIDGET>& psParent)
{
	auto lockedScreen = screenPointer.lock();
	ASSERT(lockedScreen != nullptr, "The WzPlayerBoxTabs does not have an associated screen pointer?");

	// Initialize the options overlay screen
	optionsOverlayScreen = W_SCREEN::make();
	auto newRootFrm = W_FULLSCREENOVERLAY_CLICKFORM::make();
	std::weak_ptr<W_SCREEN> psWeakOptionsOverlayScreen(optionsOverlayScreen);
	std::weak_ptr<WzStartHostingButton> psWeakHostingButton = std::dynamic_pointer_cast<WzStartHostingButton>(shared_from_this());
	newRootFrm->onClickedFunc = [psWeakOptionsOverlayScreen, psWeakHostingButton]() {
		if (auto psOverlayScreen = psWeakOptionsOverlayScreen.lock())
		{
			widgRemoveOverlayScreen(psOverlayScreen);
		}
		// Destroy Options overlay / overlay screen
		if (auto strongHostingButton = psWeakHostingButton.lock())
		{
			strongHostingButton->optionsOverlayScreen.reset();
		}
	};
	newRootFrm->onCancelPressed = newRootFrm->onClickedFunc;
	optionsOverlayScreen->psForm->attach(newRootFrm);

	// Create the pop-over form
	auto optionsPopOver = createOptionsPopoverForm();
	newRootFrm->attach(optionsPopOver);

	// Position the pop-over form
	optionsPopOver->setCalcLayout([](WIDGET *psWidget) {
		// Centered in screen
		int popOverX0 = (screenWidth - psWidget->width()) / 2;
		if (popOverX0 < 0)
		{
			popOverX0 = 0;
		}
		int popOverY0 = (screenHeight - psWidget->height()) / 2;
		if (popOverY0 < 0)
		{
			popOverY0 = 0;
		}
		psWidget->move(popOverX0, popOverY0);
	});

	widgRegisterOverlayScreenOnTopOfScreen(optionsOverlayScreen, lockedScreen);
	widgScheduleTask([lockedScreen]() {
		// clear focus from the underlying chatbox, which aggressively takes focus
		lockedScreen->setFocus(nullptr);
	});
}
