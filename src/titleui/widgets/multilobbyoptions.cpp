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
 *  Multiplayer lobby options
 */

#include "multilobbyoptions.h"
#include "lib/widget/widget.h"
#include "lib/widget/label.h"
#include "lib/widget/button.h"
#include "lib/widget/scrollablelist.h"
#include "lib/widget/multibutform.h"
#include "lib/widget/margin.h"
#include "lib/widget/editbox.h"
#include "lib/widget/popover.h"
#include "lib/widget/popovermenu.h"
#include "infobutton.h"
#include "advcheckbox.h"
#include "optionsform.h"

#include "lib/netplay/netplay.h"

#include "src/titleui/multiplayer.h"
#include "src/multistat.h"
#include "src/multiint.h"
#include "src/multiplay.h"
#include "src/multivote.h"
#include "src/multilimit.h"
#include "src/frend.h"
#include "src/loadsave.h"
#include "src/intimage.h"
#include "src/loadsave.h"

#include "lib/ivis_opengl/pieblitfunc.h"

#include "src/screens/joiningscreen.h"


#include <chrono>
#include <tuple>


// MARK: -

std::array<LimitIcon, 6> limitIcons = {
	LimitIcon{"A0LightFactory",  N_("Tanks disabled!!"),  IMAGE_NO_TANK},
	LimitIcon{"A0CyborgFactory", N_("Cyborgs disabled."), IMAGE_NO_CYBORG},
	LimitIcon{"A0VTolFactory1",  N_("VTOLs disabled."),   IMAGE_NO_VTOL},
	LimitIcon{"A0Sat-linkCentre", N_("Satellite Uplink disabled."), IMAGE_NO_UPLINK},
	LimitIcon{"A0LasSatCommand",  N_("Laser Satellite disabled."),  IMAGE_NO_LASSAT},
	LimitIcon{nullptr,  N_("Structure Limits Enforced."),  IMAGE_DARK_LOCKED}
};

constexpr int MultiOptionsRowHeight = 24;

// MARK: - WzMultiLobbyStatusBar

class WzMultiLobbyStatusBar : public WIDGET
{
protected:
	void initialize(bool isChallenge);
public:
	static std::shared_ptr<WzMultiLobbyStatusBar> make(bool isChallenge);
	virtual void display(int xOffset, int yOffset) override;
	int32_t idealWidth() override;
	int32_t idealHeight() override;
	void geometryChanged() override;

	void refreshData();
private:
	std::shared_ptr<W_LABEL> leftStatus;
	std::shared_ptr<W_LABEL> rightStatus;
	int paddingX = 16;
	int paddingY = 5;
	bool isChallenge = false;
};

std::shared_ptr<WzMultiLobbyStatusBar> WzMultiLobbyStatusBar::make(bool isChallenge)
{
	class make_shared_enabler: public WzMultiLobbyStatusBar {};
	auto widget = std::make_shared<make_shared_enabler>();
	widget->initialize(isChallenge);
	return widget;
}

void WzMultiLobbyStatusBar::initialize(bool isChallengeVal)
{
	isChallenge = isChallengeVal;

	leftStatus = std::make_shared<W_LABEL>();
	leftStatus->setFont(font_small, WZCOL_TEXT_MEDIUM);
	attach(leftStatus);

	rightStatus = std::make_shared<W_LABEL>();
	rightStatus->setFont(font_small, WZCOL_TEXT_MEDIUM);
	rightStatus->setTextAlignment(WLAB_ALIGNRIGHT);
	attach(rightStatus);

	refreshData();
}

void WzMultiLobbyStatusBar::refreshData()
{
	WzString leftStatusString;
	WzString rightStatusString;
	if (ingame.side == InGameSide::HOST_OR_SINGLEPLAYER)
	{
		if (!NetPlay.bComms) // bComms == true when a true multiplayer game (false if skirmish or challenge)
		{
			leftStatusString = (isChallenge) ? _("Challenge:") : _("Skirmish:");
			rightStatusString = _("Local (Offline)");
		}
		else
		{
			leftStatusString = _("Hosting:");
			if (NetPlay.GamePassworded)
			{
				rightStatusString = _("Password-Protected (Private)");
			}
			else
			{
				rightStatusString = _("Open (Public)");
			}
		}
	}
	else
	{
		leftStatusString = _("Connected to Host:");
		rightStatusString = NetPlay.players[NetPlay.hostPlayer].name;
	}

	leftStatus->setString(leftStatusString);
	rightStatus->setString(rightStatusString);
}

void WzMultiLobbyStatusBar::geometryChanged()
{
	int usableWidth = width() - (paddingX * 2);
	int widgetX0 = paddingX;
	int widgetHeight = height();
	int widgetWidth = (usableWidth / 2) - (paddingX / 2);

	leftStatus->setGeometry(widgetX0, 0, widgetWidth, widgetHeight);

	widgetX0 = width() - paddingX - widgetWidth;
	rightStatus->setGeometry(widgetX0, 0, widgetWidth, widgetHeight);
}

int32_t WzMultiLobbyStatusBar::idealWidth()
{
	return paddingX + leftStatus->getMaxLineWidth() + paddingX + rightStatus->getMaxLineWidth() + paddingX;
}

int32_t WzMultiLobbyStatusBar::idealHeight()
{
	return (paddingY * 2) + std::max<int32_t>(leftStatus->requiredHeight(), rightStatus->requiredHeight());
}

void WzMultiLobbyStatusBar::display(int xOffset, int yOffset)
{
	auto x0 = xOffset + x();
	auto y0 = yOffset + y();

	// Draw dark background
	pie_UniTransBoxFill(x0, y0, x0 + width(), y0 + height(), pal_RGBA(0,0,0,255));
}

// MARK: - LobbyMultiOptionsSectionWidget

class LobbyMultiOptionsSectionWidget : public MultibuttonWidget
{
public:
	LobbyMultiOptionsSectionWidget();
public:
	virtual void display(int xOffset, int yOffset) override;
	int32_t idealWidth() override;
	int32_t idealHeight() override;
	virtual void stateChanged() override;
};

LobbyMultiOptionsSectionWidget::LobbyMultiOptionsSectionWidget()
	: MultibuttonWidget(-1)
{ }

void LobbyMultiOptionsSectionWidget::display(int xOffset, int yOffset)
{
	// no-op
}

int32_t LobbyMultiOptionsSectionWidget::idealWidth()
{
	return MultibuttonWidget::idealWidth();
}

int32_t LobbyMultiOptionsSectionWidget::idealHeight()
{
	return MultiOptionsRowHeight;
}

void LobbyMultiOptionsSectionWidget::stateChanged()
{
	// no-op
}

// MARK: - LobbyMultiOptionsChoiceWidget

class WzLobbyMultiOptionButton : public W_BUTTON
{
public:
	enum class MultiOptionButtonMode
	{
		HOST_CONFIGURATION,
		CLIENT
	};

public:
	// Custom drawing-handling
	virtual void display(int xOffset, int yOffset) override;

	void setMode(MultiOptionButtonMode newMode)
	{
		mode = newMode;
	}

	void setVoteTotal(int votes);
	void setShowTotalVotes(bool show);
	void setLocalVoteIndicator(bool thumbsUp);
	void setShowLocalVoteIndicatorOnHighlight(bool val);
	void setShowSelected(bool val);

private:
	MultiOptionButtonMode mode = MultiOptionButtonMode::HOST_CONFIGURATION;
	int voteTotal = 0;
	bool showSelected = false;
	bool showTotalVotes = false;
	bool localVoteIndicator = false;
	bool showLocalVoteIndicatorOnHighlight = false;
	PIELIGHT voteIndicatorColor = pal_RGBA(13, 255, 0, 255);
	int voteIndicatorDimensions = 12;
	WzCachedText voteTotalText;
};

void WzLobbyMultiOptionButton::setVoteTotal(int votes)
{
	voteTotal = votes;
	if (voteTotal > 0)
	{
		voteTotalText.setText(WzString::number(votes), font_small);
	}
	else
	{
		voteTotalText.setText("", font_small);
	}
}

void WzLobbyMultiOptionButton::setShowTotalVotes(bool show)
{
	showTotalVotes = show;
}

void WzLobbyMultiOptionButton::setLocalVoteIndicator(bool thumbsUp)
{
	localVoteIndicator = thumbsUp;
}

void WzLobbyMultiOptionButton::setShowLocalVoteIndicatorOnHighlight(bool val)
{
	showLocalVoteIndicatorOnHighlight = val;
}

void WzLobbyMultiOptionButton::setShowSelected(bool val)
{
	showSelected = val;
}

void WzLobbyMultiOptionButton::display(int xOffset, int yOffset)
{
	int x0 = x() + xOffset;
	int y0 = y() + yOffset;
	int w = width();

	bool isDown = ((state & (WBUT_DOWN | WBUT_LOCK | WBUT_CLICKLOCK)) != 0 || showSelected);
	bool isDisabled = (state & WBUT_DISABLE) != 0 || mode == MultiOptionButtonMode::CLIENT;
	bool isHighlight = (state & WBUT_HIGHLIGHT) != 0;

	// Display the button.
	if (!images.normal.isNull())
	{
		iV_DrawImageImage(images.normal, x0, y0);
		if (isDown && !images.down.isNull())
		{
			iV_DrawImageImage(images.down, x0, y0);
		}
		if (isDisabled && !images.disabled.isNull())
		{
			iV_DrawImageImage(images.disabled, x0, y0);
		}
		if (isHighlight && !images.highlighted.isNull())
		{
			iV_DrawImageImage(images.highlighted, x0, y0);
		}
	}

	if (isDisabled && !images.normal.isNull() && images.disabled.isNull())
	{
		// disabled, render something over it!
		auto overlay_color = WZCOL_TRANSPARENT_BOX;
		overlay_color.byte.a = std::max<uint8_t>(std::min<uint8_t>(overlay_color.byte.a, 120), 70);
		if (showSelected)
		{
			overlay_color.byte.a = (overlay_color.byte.a / 3);
		}
		pie_UniTransBoxFill(x0, y0, x0 + width(), y0 + height(), overlay_color);
	}

	if (showTotalVotes && voteTotal != 0)
	{
		// Draw vote totals on right side
		int textWidth = voteTotalText->width();
		int textX0 = x0 + w - textWidth - 3;
		int textY0 = y0 + (height() - voteTotalText->lineSize()) - voteTotalText->aboveBase() - 1;
		int textX1 = textX0 + textWidth;

		int textBorderX0 = std::max<int>(textX0 - 1, x0);
		int textBorderX1 = std::min<int>(textX1 + 1, x0 + w);
		int textBorderY0 = textY0 + voteTotalText->aboveBase();
		int textBorderY1 = textBorderY0 + voteTotalText->lineSize();
		pie_UniTransBoxFill(textBorderX0, textBorderY0, textBorderX1, textBorderY1, WZCOL_TRANSPARENT_BOX);
		voteTotalText->render(textX0-1, textY0+1, WZCOL_BLACK);
		voteTotalText->render(textX0, textY0, voteIndicatorColor);
	}
	else
	{
		voteTotalText.tick();
	}

	if (localVoteIndicator || (isHighlight && showLocalVoteIndicatorOnHighlight))
	{
		// Draw local vote indicator on left side
		PIELIGHT voteColor = voteIndicatorColor;
		if (!localVoteIndicator)
		{
			voteColor.byte.a = 120;
		}
		int imgPosX0 = x0 + 2;
		int imgPosY0 = y0 + (height() - voteIndicatorDimensions) - 2;
		pie_UniTransBoxFill(imgPosX0, imgPosY0, imgPosX0 + voteIndicatorDimensions, imgPosY0 + voteIndicatorDimensions, WZCOL_TRANSPARENT_BOX);
		iV_DrawImageFileAnisotropicTint(FrontImages, IMAGE_STAR_FILL, imgPosX0-1, imgPosY0+1, Vector2f(voteIndicatorDimensions, voteIndicatorDimensions), WZCOL_BLACK);
		iV_DrawImageFileAnisotropicTint(FrontImages, IMAGE_STAR_FILL, imgPosX0, imgPosY0, Vector2f(voteIndicatorDimensions, voteIndicatorDimensions), voteColor);
	}
}

class LobbyMultiOptionsChoiceWidget : public MultibuttonWidget
{

public:
	LobbyMultiOptionsChoiceWidget(bool editMode);

	virtual void display(int xOffset, int yOffset) override;

	bool isInEditMode() { return editMode; }

protected:
	virtual void stateChanged() override;

private:
	bool editMode;
};

LobbyMultiOptionsChoiceWidget::LobbyMultiOptionsChoiceWidget(bool editMode)
	: MultibuttonWidget(-1)
	, editMode(editMode)
{
	lockCurrent = editMode;
}

void LobbyMultiOptionsChoiceWidget::display(int xOffset, int yOffset)
{
	// no-op
}

void LobbyMultiOptionsChoiceWidget::stateChanged()
{
	for (auto i = buttons.cbegin(); i != buttons.cend(); ++i)
	{
		const bool isSelectedItem = i->second == currentValue_;
		i->first->setState(isSelectedItem && lockCurrent ? WBUT_LOCK : disabled ? WBUT_DISABLE : 0);

		auto psMultiOptionButton = std::dynamic_pointer_cast<WzLobbyMultiOptionButton>(i->first);
		if (psMultiOptionButton)
		{
			psMultiOptionButton->setShowSelected(isSelectedItem);
		}
	}
}

// MARK: - WzLobbyPasswordOptionPopover

class WzLobbyPasswordOptionPopover : public WIDGET
{
protected:
	WzLobbyPasswordOptionPopover() { }
	void initialize(const std::weak_ptr<WzMultiLobbyOptionsWidgetBase>& parentOptions, const WzString& defaultPassword);
public:
	static std::shared_ptr<WzLobbyPasswordOptionPopover> make(const std::weak_ptr<WzMultiLobbyOptionsWidgetBase>& parentOptions, const WzString& defaultPassword = "");
	int32_t idealWidth() override;
	int32_t idealHeight() override;
	void geometryChanged() override;
	void givePasswordFieldFocus();
	WzString getCurrentPasswordFieldValue();
protected:
	void display(int xOffset, int yOffset) override;
private:
	void setGamePassword(const WzString& newPass);
	void resetGamePassword();
private:
	std::weak_ptr<WzMultiLobbyOptionsWidgetBase> weakParentMultiOptions;
	std::shared_ptr<W_EDITBOX> passwordEditBox;
	std::shared_ptr<WzAdvCheckbox> requiredPasswordCheckbox;
	int paddingX = 15;
	int paddingY = 15;
	int innerPaddingY = 10;
	int editBoxHeight = 24;
};

std::shared_ptr<WzLobbyPasswordOptionPopover> WzLobbyPasswordOptionPopover::make(const std::weak_ptr<WzMultiLobbyOptionsWidgetBase>& parentOptions, const WzString& defaultPassword)
{
	class make_shared_enabler: public WzLobbyPasswordOptionPopover {};
	auto widget = std::make_shared<make_shared_enabler>();
	widget->initialize(parentOptions, defaultPassword);
	return widget;
}

static void displayPasswordEditBox(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset)
{
	int x = xOffset + psWidget->x();
	int y = yOffset + psWidget->y();

	drawBlueBox(x, y, psWidget->width(), psWidget->height());

	if (((W_EDITBOX *)psWidget)->state & WEDBS_DISABLE) // disabled
	{
		PIELIGHT colour;

		colour.byte.r = FILLRED;
		colour.byte.b = FILLBLUE;
		colour.byte.g = FILLGREEN;
		colour.byte.a = FILLTRANS;
		pie_UniTransBoxFill(x, y, x + psWidget->width() + psWidget->height() - 1, y + psWidget->height(), colour);
	}
}

void WzLobbyPasswordOptionPopover::initialize(const std::weak_ptr<WzMultiLobbyOptionsWidgetBase>& parentOptions, const WzString& defaultPassword)
{
	weakParentMultiOptions = parentOptions;

	auto weakPopover = std::weak_ptr<WzLobbyPasswordOptionPopover>(std::static_pointer_cast<WzLobbyPasswordOptionPopover>(shared_from_this()));

	passwordEditBox = std::make_shared<W_EDITBOX>();
	passwordEditBox->pBoxDisplay = displayPasswordEditBox;
	passwordEditBox->setPlaceholder(_("Set Password"));
	passwordEditBox->setPlaceholderTextColor(WZCOL_TEXT_DARK);
	passwordEditBox->setString((strlen(NetPlay.gamePassword) > 0) ? NetPlay.gamePassword : defaultPassword);
	passwordEditBox->setMaxStringSize(password_string_size - 1);
	passwordEditBox->setOnReturnHandler([weakPopover](W_EDITBOX& widg) {
		auto strongPopover = weakPopover.lock();
		ASSERT_OR_RETURN(, strongPopover != nullptr, "No parent?");
		ASSERT_OR_RETURN(, (NetPlay.bComms && ingame.side == InGameSide::HOST_OR_SINGLEPLAYER), "Not the host?");
		WzString newPassword = widg.getString();
		bool willSet = (strongPopover->requiredPasswordCheckbox->isChecked()) || ((strongPopover->requiredPasswordCheckbox->getState() & WBUT_DISABLE) != 0);
		willSet &= (!newPassword.isEmpty());
		if (willSet)
		{
			strongPopover->setGamePassword(newPassword);
		}
		else
		{
			strongPopover->resetGamePassword();
		}
	});
	passwordEditBox->setOnEscapeHandler([weakPopover](W_EDITBOX& widg) {
		auto strongPopover = weakPopover.lock();
		ASSERT_OR_RETURN(, strongPopover != nullptr, "No parent?");
		ASSERT_OR_RETURN(, (NetPlay.bComms && ingame.side == InGameSide::HOST_OR_SINGLEPLAYER), "Not the host?");
		widg.setString("");
		if (NETGameIsLocked())
		{
			strongPopover->resetGamePassword();
		}
	});
	attach(passwordEditBox);

	requiredPasswordCheckbox = WzAdvCheckbox::make(_("Require Password to Join"), "");
	requiredPasswordCheckbox->setOuterHorizontalPadding(0);
	requiredPasswordCheckbox->setInnerHorizontalPadding(10);
	requiredPasswordCheckbox->setOuterVerticalPadding(0);
	requiredPasswordCheckbox->setIsChecked(NETGameIsLocked());
	requiredPasswordCheckbox->setState(passwordEditBox->getString().isEmpty() ? WBUT_DISABLE : 0);
	requiredPasswordCheckbox->addOnClickHandler([weakPopover](W_BUTTON&) {
		auto strongPopover = weakPopover.lock();
		ASSERT_OR_RETURN(, strongPopover != nullptr, "No parent?");
		ASSERT_OR_RETURN(, (NetPlay.bComms && ingame.side == InGameSide::HOST_OR_SINGLEPLAYER), "Not the host?");
		if (strongPopover->requiredPasswordCheckbox->isChecked())
		{
			strongPopover->setGamePassword(strongPopover->passwordEditBox->getString());
		}
		else
		{
			strongPopover->resetGamePassword();
		}
	});
	attach(requiredPasswordCheckbox);
}

void WzLobbyPasswordOptionPopover::setGamePassword(const WzString& newPass)
{
	char buf[255];
	NETsetGamePassword(newPass.toUtf8().c_str());
	ssprintf(buf, _("*** password [%s] is now required! ***"), NetPlay.gamePassword);
	displayRoomNotifyMessage(buf);
	requiredPasswordCheckbox->setIsChecked(true);
	requiredPasswordCheckbox->setState(0);
	// Update parent form about state change
	if (auto strongParentMultiOptions = weakParentMultiOptions.lock())
	{
		strongParentMultiOptions->refreshData();
	}
}

void WzLobbyPasswordOptionPopover::resetGamePassword()
{
	char buf[255];
	NETresetGamePassword();
	ssprintf(buf, "%s", _("*** password is NOT required! ***"));
	displayRoomNotifyMessage(buf);
	requiredPasswordCheckbox->setIsChecked(false);
	requiredPasswordCheckbox->setState(passwordEditBox->getString().isEmpty() ? WBUT_DISABLE : 0);
	// Update parent form about state change
	if (auto strongParentMultiOptions = weakParentMultiOptions.lock())
	{
		strongParentMultiOptions->refreshData();
	}
}

void WzLobbyPasswordOptionPopover::givePasswordFieldFocus()
{
	// give the edit box the focus
	W_CONTEXT context = W_CONTEXT::ZeroContext();
	context.mx			= passwordEditBox->screenPosX() + passwordEditBox->width();
	context.my			= passwordEditBox->screenPosY();
	passwordEditBox->simulateClick(&context);
}

WzString WzLobbyPasswordOptionPopover::getCurrentPasswordFieldValue()
{
	return passwordEditBox->getString();
}

int32_t WzLobbyPasswordOptionPopover::idealWidth()
{
	return paddingX + std::max<int32_t>(150, requiredPasswordCheckbox->idealWidth()) + paddingX;
}

int32_t WzLobbyPasswordOptionPopover::idealHeight()
{
	return paddingY + editBoxHeight + innerPaddingY + requiredPasswordCheckbox->idealHeight() + paddingY;
}

void WzLobbyPasswordOptionPopover::geometryChanged()
{
	int widgetX0 = paddingX;
	int widgetY0 = paddingY;
	int usableWidth = width() - (paddingX * 2);
	int usableHeight = height() - (paddingY * 2);

	if (usableWidth <= 0 || usableHeight <= 0)
	{
		return;
	}

	passwordEditBox->setGeometry(widgetX0, widgetY0, usableWidth, editBoxHeight);

	widgetY0 = passwordEditBox->y() + passwordEditBox->height() + innerPaddingY;
	requiredPasswordCheckbox->setGeometry(widgetX0, widgetY0, usableWidth, requiredPasswordCheckbox->idealHeight());
}

void WzLobbyPasswordOptionPopover::display(int xOffset, int yOffset)
{
	auto x0 = xOffset + x();
	auto y0 = yOffset + y();

	drawBlueBox(x0, y0, width(), height());
}

// MARK: - Helpers

WzString formatMapName(const WzString& name)
{
	WzString withoutTechlevel = WzString::fromUtf8(mapNameWithoutTechlevel(name.toUtf8().c_str()));
	return withoutTechlevel + " (" + WzString::number(game.maxPlayers) + "P)";
}

// MARK: - WzMultiLobbyOptionsImpl

class WzMultiLobbyOptionsImpl : public WzMultiLobbyOptionsWidgetBase
{
protected:
	WzMultiLobbyOptionsImpl();
	~WzMultiLobbyOptionsImpl();
	void initialize(bool isChallenge, const std::shared_ptr<WzMultiplayerOptionsTitleUI>& titleUI);
public:
	static std::shared_ptr<WzMultiLobbyOptionsImpl> make(bool isChallenge, const std::shared_ptr<WzMultiplayerOptionsTitleUI>& titleUI);

	int32_t idealWidth() override;
	int32_t idealHeight() override;
	void geometryChanged() override;

	virtual void refreshData() override;

	void forceSetShowTotalOptionVotes(bool show);
	bool getShowTotalOptionVotes() const;

private:
	template<typename T>
	void addMultiOptButtonT(const std::shared_ptr<MultibuttonWidget>& mbw, T pref, AtlasImage image, AtlasImage imageDown, char const *tip)
	{
		auto button = std::make_shared<WzLobbyMultiOptionButton>();
		button->setImages(image, imageDown, mpwidgetGetFrontHighlightImage(image));
		button->setTip(tip);

		bool showLocalPrefValue = (ingame.side == InGameSide::MULTIPLAYER_CLIENT) && !(NetPlay.isHost);

		if (showLocalPrefValue)
		{
			button->setMode(WzLobbyMultiOptionButton::MultiOptionButtonMode::CLIENT);
			button->minClickInterval = GAME_TICKS_PER_SEC;
		}
		button->setShowTotalVotes((ingame.side == InGameSide::HOST_OR_SINGLEPLAYER) && NetPlay.isHost);

		auto weakSelf = std::weak_ptr<WzLobbyMultiOptionButton>(button);
		auto updateDataFunc = [pref, weakSelf]() {
			auto strongSelf = weakSelf.lock();
			ASSERT_OR_RETURN(, strongSelf != nullptr, "No longer valid");
			bool showLocalPrefValue = (ingame.side == InGameSide::MULTIPLAYER_CLIENT) && !(NetPlay.isHost);
			if (showLocalPrefValue)
			{
				bool prefValue = getMultiOptionPrefValue(pref);
				strongSelf->setLocalVoteIndicator(prefValue);
			}
			else
			{
				strongSelf->setLocalVoteIndicator(false);
			}
			bool showTotalPrefValues = (ingame.side == InGameSide::HOST_OR_SINGLEPLAYER) && NetPlay.isHost && NetPlay.bComms;
			if (showTotalPrefValues)
			{
				size_t prefValueTotal = getMultiOptionPrefValueTotal(pref);
				strongSelf->setVoteTotal(prefValueTotal);
			}
			else
			{
				strongSelf->setVoteTotal(false);
			}
		};
		updateDataFunc();

		if (showLocalPrefValue)
		{
			button->addOnClickHandler([pref](W_BUTTON& b) {
				auto psButton = std::dynamic_pointer_cast<WzLobbyMultiOptionButton>(b.shared_from_this());
				ASSERT_OR_RETURN(, psButton != nullptr, "Null button?");
				// Toggle pref value state and update setLocalVoteIndicator
				bool newValue = !getMultiOptionPrefValue(pref);
				setMultiOptionPrefValue(pref, newValue);
				psButton->setLocalVoteIndicator(newValue);
			});
			button->setShowLocalVoteIndicatorOnHighlight(true);
		}

		mbw->addButton(static_cast<int>(pref), button);
		multiOptionButtons.push_back(button);

		lobbyOptionButtonUpdateFuncs.push_back(updateDataFunc);
	}

	void addLimitIcons(std::shared_ptr<MultibuttonWidget>& mbw);
	void setGameOptionStates();

	void setShowTotalOptionVotesImpl(bool show);

	typedef std::function<void ()> OnOverlayCloseHandlerFunc;
	void displayOptionsOverlay(const std::shared_ptr<WIDGET>& psParent, const std::shared_ptr<WIDGET>& optionsPopOver, const OnOverlayCloseHandlerFunc& closeHandler = nullptr);
	void displayPasswordOverlayForm(const std::shared_ptr<WIDGET>& psParent);
	void displayMatchOptionsMenu(const std::shared_ptr<WIDGET>& psParent);
	std::shared_ptr<PopoverMenuWidget> buildMatchOptionsMenu();

private:
	std::shared_ptr<WzMultiLobbyStatusBar> lobbyStatusBar;

	std::shared_ptr<W_EDITBOX> gameNameEditBox;

	std::shared_ptr<W_LABEL> mapNameWidget;

	std::shared_ptr<ScrollableListWidget> optionsList;
	std::shared_ptr<WzFrontendImageButton> matchOptionsConfigureButton;
	std::shared_ptr<LobbyMultiOptionsChoiceWidget> scavengerChoice;
	std::shared_ptr<LobbyMultiOptionsChoiceWidget> allianceChoice;
	std::shared_ptr<LobbyMultiOptionsChoiceWidget> powerChoice;
	std::shared_ptr<LobbyMultiOptionsChoiceWidget> baseTypeChoice;
	std::shared_ptr<LobbyMultiOptionsChoiceWidget> technologyChoice;
	std::vector<std::shared_ptr<WzLobbyMultiOptionButton>> multiOptionButtons;

	std::shared_ptr<MultibuttonWidget> limitIconsWidget;
	std::shared_ptr<WIDGET> limitIconsWidgetWrapper;
	std::shared_ptr<WzFrontendImageButton> roomPasswordOptionsButton;
	std::vector<std::function<void ()>> lobbyOptionButtonUpdateFuncs;

	int paddingXLeft = 13;
	int paddingXRight = 10;
	int paddingY = 3;

	bool isChallenge = false;
	optional<bool> showTotalVotes = nullopt;

	std::shared_ptr<PopoverWidget> optionsPopover;
	WzString cachedPasswordFieldValue;
};

WzMultiLobbyOptionsImpl::WzMultiLobbyOptionsImpl()
{ }

WzMultiLobbyOptionsImpl::~WzMultiLobbyOptionsImpl()
{
	if (optionsPopover)
	{
		optionsPopover->close();
	}
}

std::shared_ptr<WzMultiLobbyOptionsImpl> WzMultiLobbyOptionsImpl::make(bool isChallenge, const std::shared_ptr<WzMultiplayerOptionsTitleUI>& titleUI)
{
	class make_shared_enabler: public WzMultiLobbyOptionsImpl {};
	auto widget = std::make_shared<make_shared_enabler>();
	widget->initialize(isChallenge, titleUI);
	return widget;
}

static void displayWrappedMainRoomItem(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset)
{
	int x0 = xOffset + psWidget->x();
	int y0 = yOffset + psWidget->y();
	int w = psWidget->width();
	int h = psWidget->height();
	pie_BoxFill(x0, y0, x0 + w, y0 + h, WZCOL_MENU_BACKGROUND);
}

static void displayWrappedOptionSectionItem(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset)
{
	int x0 = xOffset + psWidget->x();
	int y0 = yOffset + psWidget->y();
	int w = psWidget->width();
	int h = psWidget->height();
	pie_UniTransBoxFill(x0, y0, x0 + w, y0 + h, pal_RGBA(14,12,55,255));
}

static void displayInlineEditBox(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset)
{
	int x = xOffset + psWidget->x();
	int y = yOffset + psWidget->y();

	if (((W_EDITBOX *)psWidget)->isEditing())
	{
		drawBlueBox(x, y, psWidget->width(), psWidget->height());
	}
	else if (((W_EDITBOX *)psWidget)->state & WEDBS_HILITE)
	{
		iV_Box(x, y, x + psWidget->width(), y + psWidget->height(), WZCOL_FORM_HILITE);
	}
}

std::shared_ptr<WIDGET> makeSectionLabelWidget(const char* sectionName)
{
	auto result = WzFrontendImageButton::make(IMAGE_CARET_DOWN_FILL);
	result->setImageDimensions(12);
	result->setString(sectionName);
	result->setPadding(4, 0);
	result->setImageHorizontalOffset(-10);
	result->setBorderDrawMode(WzFrontendImageButton::BorderDrawMode::Never);
	result->setTransparentToMouse(true);
	result->setCustomTextColors(WZCOL_FORM_TEXT, WZCOL_FORM_TEXT);
	PIELIGHT sectionImgColor = WZCOL_TEXT_MEDIUM;
	sectionImgColor.byte.a = sectionImgColor.byte.a / 2;
	result->setCustomImageColor(sectionImgColor);
	return result;
}

static void gameNameEditHandlerFunc(W_EDITBOX& widg)
{
	WzString str = widg.getString();
	if (str.isEmpty())
	{
		widg.setString(game.name);
		return;
	}

	WzString priorGameName = game.name;
	sstrcpy(game.name, str.toUtf8().c_str());
	removeWildcards(game.name);
	widg.setString(game.name);

	if ((priorGameName.compare(str) != 0) && NetPlay.isHost && NetPlay.bComms)
	{
		NETsetLobbyOptField(game.name, NET_LOBBY_OPT_FIELD::GNAME);
		sendOptions();
		NETregisterServer(WZ_SERVER_UPDATE);

		displayRoomSystemMessage(_("Game Name Updated."));
	}
}

void WzMultiLobbyOptionsImpl::initialize(bool _isChallenge, const std::shared_ptr<WzMultiplayerOptionsTitleUI>& titleUI)
{
	isChallenge = _isChallenge;

	std::weak_ptr<WzMultiLobbyOptionsImpl> psWeakMultiLobbyOptions = std::dynamic_pointer_cast<WzMultiLobbyOptionsImpl>(shared_from_this());
	std::weak_ptr<WzMultiplayerOptionsTitleUI> psWeakTitleUI = titleUI;

	const auto& locked = getLockedOptions();

	lobbyStatusBar = WzMultiLobbyStatusBar::make(isChallenge);
	attach(lobbyStatusBar);

	auto optionIsHost = [](MultibuttonWidget&, int newValue) -> bool {
		return (ingame.side == InGameSide::HOST_OR_SINGLEPLAYER);
	};

	auto wrapItemForList = [&](const std::shared_ptr<WIDGET>& widg, WIDGET_DISPLAY displayFunc = nullptr) {
		auto wrappedWidget = Margin(paddingY, paddingXRight, paddingY, paddingXLeft).wrap(widg);
		wrappedWidget->setGeometry(0, 0, wrappedWidget->idealWidth(), wrappedWidget->idealHeight());
		wrappedWidget->displayFunction = displayFunc;
		return wrappedWidget;
	};

	auto addSectionImageButton = [&](const std::shared_ptr<LobbyMultiOptionsSectionWidget>& widg, UWORD frontendImgId, int value) -> std::shared_ptr<WzFrontendImageButton> {
		auto but = WzFrontendImageButton::make(frontendImgId);
		but->setBorderDrawMode(WzFrontendImageButton::BorderDrawMode::Highlighted);
		but->setPadding(0, 0);
		but->setImageDimensions(16);
		but->setGeometry(0, 0, MultiOptionsRowHeight, MultiOptionsRowHeight);
		widg->addButton(value, but);
		return but;
	};

	optionsList = ScrollableListWidget::make();
	PIELIGHT listBackgroundColor = WZCOL_TRANSPARENT_BOX;
	listBackgroundColor.byte.a = std::max<uint8_t>(listBackgroundColor.byte.a, 200);
	optionsList->setBackgroundColor(listBackgroundColor);
	optionsList->setItemSpacing(1);
	optionsList->setDrawRowLines(true);

	// Add "Room Name" row (only in multiplayer or challenge)
	auto roomNameWidget = std::make_shared<LobbyMultiOptionsSectionWidget>();
	roomNameWidget->setOuterPaddingX(-(roomNameWidget->getGap() + 1), 0);
	gameNameEditBox = std::make_shared<W_EDITBOX>();
	gameNameEditBox->pBoxDisplay = displayInlineEditBox;
	gameNameEditBox->setPlaceholder(_("Set Game Name"));
	gameNameEditBox->setPlaceholderTextColor(WZCOL_RED);
	if (!NetPlay.bComms)
	{
		gameNameEditBox->setString(isChallenge ? game.name : _("One-Player Skirmish"));
		gameNameEditBox->setState(WEDBS_DISABLE); // disable for one-player skirmish
	}
	gameNameEditBox->setMaxStringSize(128);
	gameNameEditBox->setOnReturnHandler(gameNameEditHandlerFunc);
	gameNameEditBox->setOnEscapeHandler([](W_EDITBOX& widg) {
		widg.setString(game.name);
	});
	gameNameEditBox->setOnEditingStoppedHandler(gameNameEditHandlerFunc);
	roomNameWidget->setLabel(gameNameEditBox);
	auto psWeakGameNameEditBox = std::weak_ptr<W_EDITBOX>(gameNameEditBox);
	if (NetPlay.bComms)
	{
		auto roomNameEditButton = addSectionImageButton(roomNameWidget, IMAGE_PENCIL_SQUARE, 1);
		roomNameEditButton->addOnClickHandler([psWeakGameNameEditBox](W_BUTTON&) {
			widgScheduleTask([psWeakGameNameEditBox]{
				auto strongEditBox = psWeakGameNameEditBox.lock();
				ASSERT_OR_RETURN(, strongEditBox != nullptr, "Null edit box");
				// give the edit box the focus
				W_CONTEXT context = W_CONTEXT::ZeroContext();
				context.mx			= strongEditBox->screenPosX() + strongEditBox->width();
				context.my			= strongEditBox->screenPosY();
				strongEditBox->simulateClick(&context, true);
			});
		});
		switch (ingame.side)
		{
			case InGameSide::HOST_OR_SINGLEPLAYER:
				roomNameEditButton->setTip(_("Edit Game Name"));
				break;
			case InGameSide::MULTIPLAYER_CLIENT:
				roomNameEditButton->setTip(_("Game Name"));
				roomNameEditButton->setState(WBUT_DISABLE);
				break;
		}
	}
	if (NetPlay.bComms && ingame.side == InGameSide::HOST_OR_SINGLEPLAYER)
	{
		roomPasswordOptionsButton = addSectionImageButton(roomNameWidget, (NETGameIsLocked()) ? IMAGE_LOCK : IMAGE_UNLOCK, 2);
		roomPasswordOptionsButton->addOnClickHandler([psWeakMultiLobbyOptions](W_BUTTON&) {
			widgScheduleTask([psWeakMultiLobbyOptions]{
				auto strongMultiLobbyOptions = psWeakMultiLobbyOptions.lock();
				ASSERT_OR_RETURN(, strongMultiLobbyOptions != nullptr, "No parent");
				strongMultiLobbyOptions->displayPasswordOverlayForm(strongMultiLobbyOptions->roomPasswordOptionsButton);
			});
		});
		roomNameWidget->setGeometry(0, 0, roomNameWidget->idealWidth(), roomNameWidget->idealHeight());
	}
	optionsList->addItem(wrapItemForList(roomNameWidget, displayWrappedMainRoomItem));

	bool hideMapRow = isBlindSimpleLobby(game.blindMode) && !NetPlay.isHost;
	if (!hideMapRow) {
		// Add "Map" row
		auto mapWidget = std::make_shared<LobbyMultiOptionsSectionWidget>();
		mapNameWidget = std::make_shared<W_LABEL>(); // TODO: Replace with custom widget!!
		mapNameWidget->setString(formatMapName(game.map));
		mapWidget->setLabel(mapNameWidget);
		auto mapViewButton = addSectionImageButton(mapWidget, IMAGE_EYE, 1);
		mapViewButton->setTip(_("Click to see Map"));
		mapViewButton->addOnClickHandler([](W_BUTTON&) {
			widgScheduleTask([]{
				loadMapPreview(true);
			});
		});
		auto mapChangeButton = addSectionImageButton(mapWidget, IMAGE_GLOBE, 2);
		mapChangeButton->setTip(_("Select Map\nCan be blocked by players' votes"));
		if (titleUI)
		{
			mapChangeButton->addOnClickHandler([psWeakTitleUI](W_BUTTON&) {
				auto multiOptionsTitleUI = psWeakTitleUI.lock();
				ASSERT_OR_RETURN(, multiOptionsTitleUI != nullptr, "Null title UI?");
				multiOptionsTitleUI->openMapChooser(); // FUTURE TODO: Replace this with a new map chooser overlay screen
			});
		}
		if (isChallenge || ingame.side == InGameSide::MULTIPLAYER_CLIENT || !titleUI)
		{
			mapChangeButton->setState(WBUT_DISABLE);
		}
		mapWidget->setGeometry(0, 0, mapWidget->idealWidth(), mapWidget->idealHeight());
		optionsList->addItem(wrapItemForList(mapWidget, displayWrappedMainRoomItem));
	}

	// SECTION: "Match Options"
	auto matchOptionsSection = std::make_shared<LobbyMultiOptionsSectionWidget>();
	auto matchOptionsLabel = makeSectionLabelWidget(_("Match Options"));
	matchOptionsSection->setLabel(matchOptionsLabel);
	// Add right-side button(s)
	matchOptionsConfigureButton = addSectionImageButton(matchOptionsSection, IMAGE_GEAR, 1);
	matchOptionsConfigureButton->addOnClickHandler([psWeakMultiLobbyOptions](W_BUTTON&) {
		widgScheduleTask([psWeakMultiLobbyOptions]{
			auto strongMultiLobbyOptions = psWeakMultiLobbyOptions.lock();
			ASSERT_OR_RETURN(, strongMultiLobbyOptions != nullptr, "No parent");
			strongMultiLobbyOptions->displayMatchOptionsMenu(strongMultiLobbyOptions->matchOptionsConfigureButton);
		});
	});
	if (isChallenge)
	{
		matchOptionsConfigureButton->setState(WBUT_DISABLE);
	}
	matchOptionsSection->setGeometry(0, 0, matchOptionsSection->idealWidth(), matchOptionsSection->idealHeight());
	optionsList->addItem(wrapItemForList(matchOptionsSection, displayWrappedOptionSectionItem));

	bool matchOptionsEditMode = (ingame.side == InGameSide::HOST_OR_SINGLEPLAYER);

	// Scavengers
	scavengerChoice = std::make_shared<LobbyMultiOptionsChoiceWidget>(matchOptionsEditMode);
	scavengerChoice->setLabel(_("Scavengers"));
	addMultiOptButtonT<ScavType>(scavengerChoice, ULTIMATE_SCAVENGERS, AtlasImage(FrontImages, IMAGE_SCAVENGERS_ULTIMATE_ON), AtlasImage(FrontImages, IMAGE_SCAVENGERS_ULTIMATE_ON_HI), _("Ultimate Scavengers"));
	addMultiOptButtonT<ScavType>(scavengerChoice, SCAVENGERS, AtlasImage(FrontImages, IMAGE_SCAVENGERS_ON), AtlasImage(FrontImages, IMAGE_SCAVENGERS_ON_HI), _("Scavengers"));
	addMultiOptButtonT<ScavType>(scavengerChoice, NO_SCAVENGERS, AtlasImage(FrontImages, IMAGE_SCAVENGERS_OFF), AtlasImage(FrontImages, IMAGE_SCAVENGERS_OFF_HI), _("No Scavengers"));
	scavengerChoice->enable((ingame.side == InGameSide::MULTIPLAYER_CLIENT) || !locked.scavengers);
	scavengerChoice->setGeometry(0, 0, scavengerChoice->idealWidth(), scavengerChoice->idealHeight());
	scavengerChoice->setCanChooseHandler([](MultibuttonWidget&, int newValue) -> bool {
		return !getLockedOptions().scavengers && (ingame.side == InGameSide::HOST_OR_SINGLEPLAYER);
	});
	scavengerChoice->addOnChooseHandler([](MultibuttonWidget& widget, int newValue){
		if (ingame.side == InGameSide::HOST_OR_SINGLEPLAYER)
		{
			game.scavengers = newValue;
			resetReadyStatus(false);
			if (NetPlay.isHost)
			{
				sendOptions();
			}
		}
	});
	optionsList->addItem(wrapItemForList(scavengerChoice));

	// Alliances
	allianceChoice = std::make_shared<LobbyMultiOptionsChoiceWidget>(matchOptionsEditMode);
	allianceChoice->setLabel(_("Alliances"));
	addMultiOptButtonT<AllianceType>(allianceChoice, NO_ALLIANCES, AtlasImage(FrontImages, IMAGE_NOALLI), AtlasImage(FrontImages, IMAGE_NOALLI_HI), _("No Alliances"));
	addMultiOptButtonT<AllianceType>(allianceChoice, ALLIANCES, AtlasImage(FrontImages, IMAGE_ALLI), AtlasImage(FrontImages, IMAGE_ALLI_HI), _("Allow Alliances"));
	addMultiOptButtonT<AllianceType>(allianceChoice, ALLIANCES_UNSHARED, AtlasImage(FrontImages, IMAGE_ALLI_UNSHARED), AtlasImage(FrontImages, IMAGE_ALLI_UNSHARED_HI), _("Locked Teams, No Shared Research"));
	addMultiOptButtonT<AllianceType>(allianceChoice, ALLIANCES_TEAMS, AtlasImage(FrontImages, IMAGE_ALLI_TEAMS), AtlasImage(FrontImages, IMAGE_ALLI_TEAMS_HI), _("Locked Teams"));
	allianceChoice->enable((ingame.side == InGameSide::MULTIPLAYER_CLIENT) || !locked.alliances);
	allianceChoice->setGeometry(0, 0, allianceChoice->idealWidth(), allianceChoice->idealHeight());
	allianceChoice->setCanChooseHandler([](MultibuttonWidget&, int newValue) -> bool {
		return !getLockedOptions().alliances && (ingame.side == InGameSide::HOST_OR_SINGLEPLAYER);
	});
	allianceChoice->addOnChooseHandler([](MultibuttonWidget& widget, int newValue){
		if (ingame.side == InGameSide::HOST_OR_SINGLEPLAYER)
		{
			game.alliance = newValue;

			resetReadyStatus(false);
			netPlayersUpdated = true;

			if (NetPlay.isHost)
			{
				sendOptions();

				NETsetLobbyConfigFlagsFields(game.alliance, game.techLevel, game.power, game.base);
				NETregisterServer(WZ_SERVER_UPDATE);
			}
		}
	});
	optionsList->addItem(wrapItemForList(allianceChoice));

	// Power
	powerChoice = std::make_shared<LobbyMultiOptionsChoiceWidget>(matchOptionsEditMode);
	powerChoice->setLabel(_("Power"));
	addMultiOptButtonT<PowerSetting>(powerChoice, LEV_LOW, AtlasImage(FrontImages, IMAGE_POWLO), AtlasImage(FrontImages, IMAGE_POWLO_HI), _("Low Power Levels"));
	addMultiOptButtonT<PowerSetting>(powerChoice, LEV_MED, AtlasImage(FrontImages, IMAGE_POWMED), AtlasImage(FrontImages, IMAGE_POWMED_HI), _("Medium Power Levels"));
	addMultiOptButtonT<PowerSetting>(powerChoice, LEV_HI, AtlasImage(FrontImages, IMAGE_POWHI), AtlasImage(FrontImages, IMAGE_POWHI_HI), _("High Power Levels"));
	powerChoice->enable((ingame.side == InGameSide::MULTIPLAYER_CLIENT) || !locked.power);
	powerChoice->setGeometry(0, 0, powerChoice->idealWidth(), powerChoice->idealHeight());
	powerChoice->setCanChooseHandler([](MultibuttonWidget&, int newValue) -> bool {
		return !getLockedOptions().power && (ingame.side == InGameSide::HOST_OR_SINGLEPLAYER);
	});
	powerChoice->addOnChooseHandler([](MultibuttonWidget& widget, int newValue){
		if (ingame.side == InGameSide::HOST_OR_SINGLEPLAYER)
		{
			game.power = newValue;
			resetReadyStatus(false);

			if (NetPlay.isHost)
			{
				sendOptions();

				NETsetLobbyConfigFlagsFields(game.alliance, game.techLevel, game.power, game.base);
				NETregisterServer(WZ_SERVER_UPDATE);
			}
		}
	});
	optionsList->addItem(wrapItemForList(powerChoice));

	// Bases
	baseTypeChoice = std::make_shared<LobbyMultiOptionsChoiceWidget>(matchOptionsEditMode);
	baseTypeChoice->setLabel(_("Base"));
	addMultiOptButtonT<CampType>(baseTypeChoice, CAMP_CLEAN, AtlasImage(FrontImages, IMAGE_NOBASE), AtlasImage(FrontImages, IMAGE_NOBASE_HI), _("Start with No Bases"));
	addMultiOptButtonT<CampType>(baseTypeChoice, CAMP_BASE, AtlasImage(FrontImages, IMAGE_SBASE), AtlasImage(FrontImages, IMAGE_SBASE_HI), _("Start with Bases"));
	addMultiOptButtonT<CampType>(baseTypeChoice, CAMP_WALLS, AtlasImage(FrontImages, IMAGE_LBASE), AtlasImage(FrontImages, IMAGE_LBASE_HI), _("Start with Advanced Bases"));
	baseTypeChoice->enable((ingame.side == InGameSide::MULTIPLAYER_CLIENT) || !locked.bases);
	baseTypeChoice->setGeometry(0, 0, baseTypeChoice->idealWidth(), baseTypeChoice->idealHeight());
	baseTypeChoice->setCanChooseHandler([](MultibuttonWidget&, int newValue) -> bool {
		return !getLockedOptions().bases && (ingame.side == InGameSide::HOST_OR_SINGLEPLAYER);
	});
	baseTypeChoice->addOnChooseHandler([](MultibuttonWidget& widget, int newValue){
		if (ingame.side == InGameSide::HOST_OR_SINGLEPLAYER)
		{
			game.base = newValue;

			resetReadyStatus(false);

			if (NetPlay.isHost)
			{
				sendOptions();

				NETsetLobbyConfigFlagsFields(game.alliance, game.techLevel, game.power, game.base);
				NETregisterServer(WZ_SERVER_UPDATE);
			}
		}
	});
	optionsList->addItem(wrapItemForList(baseTypeChoice));

	if (!isChallenge)
	{
		// Tech Level
		technologyChoice = std::make_shared<LobbyMultiOptionsChoiceWidget>(matchOptionsEditMode);
		technologyChoice->setLabel(_("Tech"));
		addMultiOptButtonT<TechLevel>(technologyChoice, TECH_1, AtlasImage(FrontImages, IMAGE_TECHLO), AtlasImage(FrontImages, IMAGE_TECHLO_HI), _("Technology Level 1"));
		addMultiOptButtonT<TechLevel>(technologyChoice, TECH_2, AtlasImage(FrontImages, IMAGE_TECHMED), AtlasImage(FrontImages, IMAGE_TECHMED_HI), _("Technology Level 2"));
		addMultiOptButtonT<TechLevel>(technologyChoice, TECH_3, AtlasImage(FrontImages, IMAGE_TECHHI), AtlasImage(FrontImages, IMAGE_TECHHI_HI), _("Technology Level 3"));
		addMultiOptButtonT<TechLevel>(technologyChoice, TECH_4, AtlasImage(FrontImages, IMAGE_COMPUTER_Y), AtlasImage(FrontImages, IMAGE_COMPUTER_Y_HI), _("Technology Level 4"));
		technologyChoice->setGeometry(0, 0, technologyChoice->idealWidth(), technologyChoice->idealHeight());
		technologyChoice->setCanChooseHandler(optionIsHost);
		technologyChoice->addOnChooseHandler([](MultibuttonWidget& widget, int newValue){
			if (ingame.side == InGameSide::HOST_OR_SINGLEPLAYER)
			{
				game.techLevel = newValue;

				resetReadyStatus(false);

				if (NetPlay.isHost)
				{
					sendOptions();

					NETsetLobbyConfigFlagsFields(game.alliance, game.techLevel, game.power, game.base);
					NETregisterServer(WZ_SERVER_UPDATE);
				}
			}
		});
		optionsList->addItem(wrapItemForList(technologyChoice));
	}

	// Refresh values for match options
	setGameOptionStates();

	// SECTION: "Limits"
	auto limitsSection = std::make_shared<LobbyMultiOptionsSectionWidget>();
	auto limitsLabel = makeSectionLabelWidget(_("Limits"));
	limitsSection->setLabel(limitsLabel);
	auto limitsInfoBut = addSectionImageButton(limitsSection, IMAGE_INFO_CIRCLE, 1);
	limitsInfoBut->addOnClickHandler([](W_BUTTON&) {
		widgScheduleTask([]() {
			printStructureLimitsInfo(ingame.structureLimits, [](const std::string& line) {
				addConsoleMessage(line.c_str(), DEFAULT_JUSTIFY, SYSTEM_MESSAGE);
			});
		});
	});
	if (ingame.side == InGameSide::HOST_OR_SINGLEPLAYER)
	{
		auto limitsOptionsBut = addSectionImageButton(limitsSection, IMAGE_GEAR, 2);
		limitsOptionsBut->addOnClickHandler([](W_BUTTON&) {
			// POSSIBLE FUTURE TODO: Open context menu?
			widgScheduleTask([]() {
				changeTitleUI(std::make_shared<WzMultiLimitTitleUI>(std::dynamic_pointer_cast<WzMultiplayerOptionsTitleUI>(wzTitleUICurrent)));
			});
		});
	}
	limitsSection->setGeometry(0, 0, limitsSection->idealWidth(), limitsSection->idealHeight());
	optionsList->addItem(wrapItemForList(limitsSection, displayWrappedOptionSectionItem));

	limitIconsWidget = std::make_shared<LobbyMultiOptionsSectionWidget>();
	addLimitIcons(limitIconsWidget);
	limitIconsWidget->setGeometry(0, 0, limitIconsWidget->idealWidth(), std::max<int32_t>(limitIconsWidget->idealHeight(), MultiOptionsRowHeight));
	limitIconsWidgetWrapper = wrapItemForList(limitIconsWidget);
	limitIconsWidgetWrapper->show(limitIconsWidget->numButtons() > 0);
	optionsList->addItem(limitIconsWidgetWrapper);

	attach(optionsList);
}

void WzMultiLobbyOptionsImpl::setGameOptionStates()
{
	const auto& locked = getLockedOptions();

	if (NetPlay.bComms)
	{
		gameNameEditBox->setString(game.name);
		if (ingame.side == InGameSide::MULTIPLAYER_CLIENT)
		{
			gameNameEditBox->setState(WEDBS_DISABLE);
		}
	}
	else
	{
		gameNameEditBox->setString(isChallenge ? game.name : _("One-Player Skirmish"));
		gameNameEditBox->setState(WEDBS_DISABLE); // disable for one-player skirmish
	}

	if (roomPasswordOptionsButton)
	{
		roomPasswordOptionsButton->setImage((NETGameIsLocked()) ? IMAGE_LOCK : IMAGE_UNLOCK);
	}

	if (mapNameWidget)
	{
		// Update map name & details
		mapNameWidget->setString(formatMapName(game.map));
	}

	auto setButtonState = [](const std::shared_ptr<LobbyMultiOptionsChoiceWidget>& mbw, int value, int state)
	{
		auto pButton = mbw->getButtonByValue(value);
		if (pButton)
		{
			pButton->setState(state);
		}
	};

	if (!game.mapHasScavengers)
	{
		setButtonState(scavengerChoice, ULTIMATE_SCAVENGERS, WBUT_DISABLE);
		setButtonState(scavengerChoice, SCAVENGERS, WBUT_DISABLE);
	}
	else if (!scavengerChoice->isDisabled())
	{
		setButtonState(scavengerChoice, ULTIMATE_SCAVENGERS, 0);
		setButtonState(scavengerChoice, SCAVENGERS, 0);
	}

	// Update MultiButtonWidget enabled status
	scavengerChoice->enable((ingame.side == InGameSide::MULTIPLAYER_CLIENT) || !locked.scavengers);
	allianceChoice->enable((ingame.side == InGameSide::MULTIPLAYER_CLIENT) || !locked.alliances);
	powerChoice->enable((ingame.side == InGameSide::MULTIPLAYER_CLIENT) || !locked.power);
	baseTypeChoice->enable((ingame.side == InGameSide::MULTIPLAYER_CLIENT) || !locked.bases);

	scavengerChoice->choose(game.scavengers);
	allianceChoice->choose(game.alliance);
	powerChoice->choose(game.power);
	baseTypeChoice->choose(game.base);
	if (technologyChoice)
	{
		technologyChoice->choose(game.techLevel);
	}
}

bool WzMultiLobbyOptionsImpl::getShowTotalOptionVotes() const
{
	return showTotalVotes.value_or((ingame.side == InGameSide::HOST_OR_SINGLEPLAYER) && NetPlay.isHost);
}

void WzMultiLobbyOptionsImpl::refreshData()
{
	lobbyStatusBar->refreshData();

	setGameOptionStates();

	// Update individual option buttons
	for (const auto& buttonUpdateFunc : lobbyOptionButtonUpdateFuncs)
	{
		buttonUpdateFunc();
	}
	setShowTotalOptionVotesImpl(getShowTotalOptionVotes());

	// update limit icons
	limitIconsWidget->clearButtons();
	addLimitIcons(limitIconsWidget);
	limitIconsWidgetWrapper->show(limitIconsWidget->numButtons() > 0);

	optionsList->setLayoutDirty();
}

void WzMultiLobbyOptionsImpl::forceSetShowTotalOptionVotes(bool show)
{
	showTotalVotes = show;
	setShowTotalOptionVotesImpl(showTotalVotes.value());
}

void WzMultiLobbyOptionsImpl::setShowTotalOptionVotesImpl(bool show)
{
	for (auto& button : multiOptionButtons)
	{
		button->setShowTotalVotes(show);
	}
}

void WzMultiLobbyOptionsImpl::addLimitIcons(std::shared_ptr<MultibuttonWidget>& mbw)
{
	for (int i = 0; i < static_cast<unsigned>(limitIcons.size()); ++i)
	{
		if ((ingame.flags & (1 << i)) != 0)
		{
			auto button = std::make_shared<WzLobbyMultiOptionButton>();
			button->setImages(AtlasImage(FrontImages, limitIcons[i].icon), AtlasImage(FrontImages, limitIcons[i].icon), AtlasImage(FrontImages, limitIcons[i].icon));
			button->setTip(_(limitIcons[i].desc));

			mbw->addButton(i, button);
		}
	}
}

int32_t WzMultiLobbyOptionsImpl::idealWidth()
{
	return std::max<int32_t>(lobbyStatusBar->idealWidth(), optionsList->idealWidth());
}

int32_t WzMultiLobbyOptionsImpl::idealHeight()
{
	int32_t result = lobbyStatusBar->idealHeight();
	result += optionsList->idealHeight();
	result += 1;
	return result;
}

void WzMultiLobbyOptionsImpl::geometryChanged()
{
	int w = width();
	int h = height();

	if (w == 0 || h == 0)
	{
		return;
	}

	lobbyStatusBar->setGeometry(0, 0, w, lobbyStatusBar->idealHeight());

	const int wrappedOptionRowHeight = paddingY + MultiOptionsRowHeight + paddingY;
	int idealListHeight = optionsList->idealHeight();
	int availableListHeight = std::min<int32_t>(h - lobbyStatusBar->height() - 1, idealListHeight);
	if (availableListHeight < idealListHeight)
	{
		availableListHeight = (availableListHeight / (wrappedOptionRowHeight + 1)) * (wrappedOptionRowHeight + 1);
	}
	int listY0 = lobbyStatusBar->height();
	optionsList->setGeometry(0, listY0, w, availableListHeight);
}

void WzMultiLobbyOptionsImpl::displayOptionsOverlay(const std::shared_ptr<WIDGET>& psParent, const std::shared_ptr<WIDGET>& optionsPopOver, const OnOverlayCloseHandlerFunc& closeHandler)
{
	if (optionsPopover)
	{
		optionsPopover->close();
	}
	optionsPopover = PopoverWidget::makePopover(psParent, optionsPopOver, PopoverWidget::Style::Interactive, PopoverWidget::Alignment::LeftOfParent, Vector2i(0, 1), closeHandler);
}

void WzMultiLobbyOptionsImpl::displayPasswordOverlayForm(const std::shared_ptr<WIDGET>& psParent)
{
	std::weak_ptr<WzMultiLobbyOptionsImpl> weakMultiLobbyOptions = std::dynamic_pointer_cast<WzMultiLobbyOptionsImpl>(shared_from_this());
	roomPasswordOptionsButton->setState(WBUT_CLICKLOCK);

	auto passwordPopover = WzLobbyPasswordOptionPopover::make(weakMultiLobbyOptions, cachedPasswordFieldValue);
	passwordPopover->setGeometry(0, 0, passwordPopover->idealWidth(), passwordPopover->idealHeight());

	auto weakPasswordPopover = std::weak_ptr<WzLobbyPasswordOptionPopover>(passwordPopover);
	displayOptionsOverlay(psParent, passwordPopover, [weakPasswordPopover, weakMultiLobbyOptions]() {
		// close handler
		auto strongPasswordPopover = weakPasswordPopover.lock();
		ASSERT_OR_RETURN(, strongPasswordPopover != nullptr, "No widget");
		if (auto strongMultiLobbyOptions = weakMultiLobbyOptions.lock())
		{
			strongMultiLobbyOptions->cachedPasswordFieldValue = strongPasswordPopover->getCurrentPasswordFieldValue();
			strongMultiLobbyOptions->roomPasswordOptionsButton->setState(0); // clear clicklock state
		}
	});

	widgScheduleTask([weakPasswordPopover](){
		auto strongPasswordPopover = weakPasswordPopover.lock();
		ASSERT_OR_RETURN(, strongPasswordPopover != nullptr, "No widget");
		strongPasswordPopover->givePasswordFieldFocus();
	});
}

static std::shared_ptr<WzClickableOptionsChoiceWidget> addClickableMenuItem(const std::shared_ptr<PopoverMenuWidget>& menu, const WzString& text, const std::function<void (WIDGET_KEY)>& onClick, bool disabled = false)
{
	auto result = WzClickableOptionsChoiceWidget::make(font_regular);
	result->setString(text);
	result->setTextAlignment(WLAB_ALIGNLEFT);
	if (onClick)
	{
		result->addOnClickHandler([onClick](WzClickableOptionsChoiceWidget&, WIDGET_KEY key) {
			onClick(key);
		});
	}
	result->setDisabled(disabled);
	result->setGeometry(0, 0, result->idealWidth(), result->idealHeight());
	menu->addMenuItem(result, true);
	return result;
}

std::shared_ptr<PopoverMenuWidget> WzMultiLobbyOptionsImpl::buildMatchOptionsMenu()
{
	std::weak_ptr<WzMultiLobbyOptionsImpl> weakMultiLobbyOptions = std::dynamic_pointer_cast<WzMultiLobbyOptionsImpl>(shared_from_this());

	auto popoverMenu = PopoverMenuWidget::make();

	if (ingame.side == InGameSide::HOST_OR_SINGLEPLAYER && !isChallenge)
	{
		// "Randomize Options"
		addClickableMenuItem(popoverMenu, _("Randomize Options"), [](WIDGET_KEY) {
			multiLobbyRandomizeOptions();
		});
	}

	if ((ingame.side == InGameSide::MULTIPLAYER_CLIENT) && !(NetPlay.isHost))
	{
		// [Match Option Preferences:]
		addClickableMenuItem(popoverMenu, _("Match Option Preferences:"), nullptr, true);
		// Enable All Options
		addClickableMenuItem(popoverMenu, _("Star All"), [](WIDGET_KEY) {
			if ((ingame.side == InGameSide::MULTIPLAYER_CLIENT) && !(NetPlay.isHost))
			{
				setMultiOptionBuiltinPrefValues(true);
			}
		});
		// Clear All Options
		addClickableMenuItem(popoverMenu, _("Clear All"), [](WIDGET_KEY) {
			if ((ingame.side == InGameSide::MULTIPLAYER_CLIENT) && !(NetPlay.isHost))
			{
				resetMultiOptionPrefValues(selectedPlayer);
			}
		});
	}

	if ((ingame.side == InGameSide::HOST_OR_SINGLEPLAYER) && !isChallenge)
	{
		addClickableMenuItem(popoverMenu, _("Match Option Preferences:"), nullptr, true);
		if (!getShowTotalOptionVotes())
		{
			addClickableMenuItem(popoverMenu, _("Show Player Preferences"), [weakMultiLobbyOptions](WIDGET_KEY) {
				auto strongMultiLobbyOptions = weakMultiLobbyOptions.lock();
				ASSERT_OR_RETURN(, strongMultiLobbyOptions != nullptr, "Missing pointer");
				strongMultiLobbyOptions->forceSetShowTotalOptionVotes(true);
			});
		}
		else
		{
			addClickableMenuItem(popoverMenu, _("Hide Player Preferences"), [weakMultiLobbyOptions](WIDGET_KEY) {
				auto strongMultiLobbyOptions = weakMultiLobbyOptions.lock();
				ASSERT_OR_RETURN(, strongMultiLobbyOptions != nullptr, "Missing pointer");
				strongMultiLobbyOptions->forceSetShowTotalOptionVotes(false);
			});
		}
	}

	int32_t idealMenuHeight = popoverMenu->idealHeight();
	int32_t menuHeight = idealMenuHeight;
	if (menuHeight > screenHeight)
	{
		menuHeight = screenHeight;
	}
	popoverMenu->setGeometry(popoverMenu->x(), popoverMenu->y(), popoverMenu->idealWidth(), menuHeight);

	return popoverMenu;
}

void WzMultiLobbyOptionsImpl::displayMatchOptionsMenu(const std::shared_ptr<WIDGET> &psParent)
{
	auto popoverMenu = buildMatchOptionsMenu();

	std::weak_ptr<WzMultiLobbyOptionsImpl> weakMultiLobbyOptions = std::dynamic_pointer_cast<WzMultiLobbyOptionsImpl>(shared_from_this());
	matchOptionsConfigureButton->setState(WBUT_CLICKLOCK);

	if (optionsPopover)
	{
		optionsPopover->close();
	}
	optionsPopover = popoverMenu->openMenu(psParent, PopoverWidget::Alignment::LeftOfParent, Vector2i(0, 1), [weakMultiLobbyOptions]() {
		// close handler
		if (auto strongMultiLobbyOptions = weakMultiLobbyOptions.lock())
		{
			strongMultiLobbyOptions->refreshData();
			strongMultiLobbyOptions->matchOptionsConfigureButton->setState(0); // clear clicklock state
		}
	});
}

std::shared_ptr<WzMultiLobbyOptionsWidgetBase> makeWzMultiLobbyOptionsForm(bool isChallenge, const std::shared_ptr<WzMultiplayerOptionsTitleUI>& titleUI)
{
	return WzMultiLobbyOptionsImpl::make(isChallenge, titleUI);
}
