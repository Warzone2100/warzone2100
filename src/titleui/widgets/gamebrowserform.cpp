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
/** \file
 *  Game Browser Form
 */

#include "gamebrowserform.h"

#include "lib/framework/wzapp.h"
#include "lib/framework/string_ext.h"
#include "lib/widget/label.h"
#include "lib/widget/table.h"
#include "lib/widget/margin.h"
#include "lib/widget/paragraph.h"
#include "lib/widget/popovermenu.h"
#include "lib/widget/editbox.h"
#include "lib/netplay/netplay.h"
#include "lib/ivis_opengl/pieblitfunc.h"
#include "src/multiplay.h"
#include "src/ai.h"
#include "src/frend.h"
#include "src/multiint.h"
#include "src/activity.h"
#include "src/updatemanager.h"
#include "frontendimagebutton.h"
#include "advcheckbox.h"
#include "src/screens/joiningscreen.h"
#include "lib/netplay/netlobby.h"
#include "src/warzoneconfig.h"
#include "optionsform.h"
#include "src/multijoin_helpers.h"

#include <numeric>
#include <vector>
#include <optional>
#include <unordered_set>

static constexpr uint32_t LOB_DEFAULT_BEHAVIOR_CHECKSUM = 2462613339;

// MARK: - Lobby Status Overlay Widget

class LobbyStatusOverlayWidget : public WIDGET
{
public:
	typedef std::function<void()> OnResetLobbyAddressClickFunc;
protected:
	void initialize(const OnResetLobbyAddressClickFunc& onResetLobbyAddressClickFunc);
	virtual void geometryChanged() override;
	virtual void display(int xOffset, int yOffset) override;
public:
	static std::shared_ptr<LobbyStatusOverlayWidget> make(const OnResetLobbyAddressClickFunc& onResetLobbyAddressClickFunc)
	{
		class make_shared_enabler: public LobbyStatusOverlayWidget {};
		auto widget = std::make_shared<make_shared_enabler>();
		widget->initialize(onResetLobbyAddressClickFunc);
		return widget;
	}
	void showIndeterminateIndicator(bool show);
	void showResetLobbyAddressButton(bool show);
	virtual void setString(WzString string) override;
	virtual int32_t idealWidth() override;
	virtual int32_t idealHeight() override;
private:
	void recalcLayout();
private:
	std::shared_ptr<W_LABEL> textLabel;
	std::shared_ptr<WIDGET> indeterminateIndicator;
	std::shared_ptr<WzFrontendImageButton> resetLobbyAddressButton;

	OnResetLobbyAddressClickFunc onResetLobbyAddressClickFunc;

	const int verticalBetweenItemsPadding = 20;
};

void LobbyStatusOverlayWidget::initialize(const OnResetLobbyAddressClickFunc& _onResetLobbyAddressClickFunc)
{
	onResetLobbyAddressClickFunc = _onResetLobbyAddressClickFunc;

	textLabel = std::make_shared<W_LABEL>();
	textLabel->setFont(font_medium_bold, WZCOL_TEXT_MEDIUM);
	textLabel->setTextAlignment(WLAB_ALIGNTOP);
	textLabel->setCanTruncate(true);
	textLabel->setTransparentToClicks(true);
	attach(textLabel);

	indeterminateIndicator = createJoiningIndeterminateProgressWidget(font_medium_bold);
	indeterminateIndicator->show(false);
	attach(indeterminateIndicator);

	resetLobbyAddressButton = WzFrontendImageButton::make(IMAGE_ARROW_UNDO);
	resetLobbyAddressButton->setString(_("Reset Lobby Address to Default"));
	resetLobbyAddressButton->setPadding(6, 5);
	resetLobbyAddressButton->setBackgroundColor(WZCOL_TRANSPARENT_BOX);
	resetLobbyAddressButton->setGeometry(0, 0, resetLobbyAddressButton->idealWidth(), resetLobbyAddressButton->idealHeight());
	auto weakSelf = std::weak_ptr<LobbyStatusOverlayWidget>(std::dynamic_pointer_cast<LobbyStatusOverlayWidget>(shared_from_this()));
	resetLobbyAddressButton->addOnClickHandler([weakSelf](W_BUTTON& but) {
		auto strongSelf = weakSelf.lock();
		ASSERT_OR_RETURN(, strongSelf != nullptr, "No parent?");
		ASSERT_OR_RETURN(, strongSelf->onResetLobbyAddressClickFunc != nullptr, "Func is null?");
		strongSelf->onResetLobbyAddressClickFunc();
	});
	resetLobbyAddressButton->show(false);
	attach(resetLobbyAddressButton);
}

int32_t LobbyStatusOverlayWidget::idealWidth()
{
	return std::max<int32_t>(textLabel->idealWidth(), indeterminateIndicator->idealWidth());
}

int32_t LobbyStatusOverlayWidget::idealHeight()
{
	auto result = 0;
	if (!textLabel->getString().isEmpty())
	{
		result += iV_GetTextLineSize(font_medium_bold);
	}
	if (indeterminateIndicator->visible())
	{
		if (result > 0)
		{
			result += verticalBetweenItemsPadding;
		}
		result += indeterminateIndicator->idealHeight();
	}
	if (resetLobbyAddressButton->visible())
	{
		if (result > 0)
		{
			result += verticalBetweenItemsPadding;
		}
		result += resetLobbyAddressButton->idealHeight();
	}
	return result;
}

void LobbyStatusOverlayWidget::recalcLayout()
{
	int w = width();

	int nextWidgY0 = 0;
	if (!textLabel->getString().isEmpty())
	{
		int labelWidth = std::min(w, textLabel->idealWidth());
		int labelX0 = (w - labelWidth) / 2;
		textLabel->setGeometry(labelX0, nextWidgY0, labelWidth, textLabel->idealHeight());
		nextWidgY0 = textLabel->height() + verticalBetweenItemsPadding;
	}

	if (indeterminateIndicator->visible())
	{
		int indicatorX0 = (w - indeterminateIndicator->idealWidth()) / 2;
		indeterminateIndicator->setGeometry(indicatorX0, nextWidgY0, indeterminateIndicator->idealWidth(), indeterminateIndicator->idealHeight());
		nextWidgY0 = indeterminateIndicator->y() + indeterminateIndicator->height() + verticalBetweenItemsPadding;
	}

	if (resetLobbyAddressButton->visible())
	{
		int resetButtonWidth = std::min<int>(resetLobbyAddressButton->idealWidth(), w);
		int resetButtonX0 = (w - resetButtonWidth) / 2;
		resetLobbyAddressButton->setGeometry(resetButtonX0, nextWidgY0, resetButtonWidth, resetLobbyAddressButton->idealHeight());
	}
}

void LobbyStatusOverlayWidget::geometryChanged()
{
	recalcLayout();
}

void LobbyStatusOverlayWidget::display(int xOffset, int yOffset)
{
	int x0 = x() + xOffset;
	int y0 = y() + yOffset;

	if (indeterminateIndicator->visible())
	{
		int indicatorX0 = x0 + indeterminateIndicator->x();
		int indicatorY0 = y0 + indeterminateIndicator->y();
		int indicatorX1 = indicatorX0 + indeterminateIndicator->width();
		int indicatorY1 = indicatorY0 + indeterminateIndicator->height();

		pie_UniTransBoxFill(std::max<int>(indicatorX0 - 20, 0), std::max<int>(indicatorY0 - 10, 0), std::min<int>(indicatorX1 + 20, screenWidth), std::min<int>(indicatorY1 + 20, screenHeight), WZCOL_TRANSPARENT_BOX);
	}
}

void LobbyStatusOverlayWidget::showIndeterminateIndicator(bool show)
{
	if (indeterminateIndicator->visible() == show)
	{
		return;
	}
	indeterminateIndicator->show(show);
	recalcLayout();
}

void LobbyStatusOverlayWidget::setString(WzString string)
{
	textLabel->setString(string);
	recalcLayout();
}

void LobbyStatusOverlayWidget::showResetLobbyAddressButton(bool show)
{
	resetLobbyAddressButton->show(show);
	recalcLayout();
}

// MARK: - Lobby Game Column Widgets

#define WZLOBBY_WIDG_PADDING_Y 4
#define WZLOBBY_WIDG_PADDING_X 0
#define WZLOBBY_WIDG_LINE_SPACING 2

class LobbyGameIconWidget : public WIDGET
{
protected:
	LobbyGameIconWidget() { }
	void initialize(optional<UWORD> frontendImgId);
	virtual void display(int xOffset, int yOffset) override;
public:
	static std::shared_ptr<LobbyGameIconWidget> make(optional<UWORD> frontendImgId)
	{
		class make_shared_enabler: public LobbyGameIconWidget {};
		auto widget = std::make_shared<make_shared_enabler>();
		widget->initialize(frontendImgId);
		return widget;
	}
	void setImage(optional<UWORD> newFrontendImgID);
	void setCustomImageColor(optional<PIELIGHT> color);
	void setImageSize(WzSize imageDimensions);
	virtual int32_t idealWidth() override;
	virtual int32_t idealHeight() override;
	virtual void setTip(std::string string) override;
	virtual std::string getTip() override;
private:
	optional<UWORD> frontendImgID = nullopt;
	optional<PIELIGHT> customImgColor = nullopt;
	bool missingImage = false;
	WzSize imageDimensions = WzSize(16,16);
	std::string tipStr;
};

void LobbyGameIconWidget::initialize(optional<UWORD> frontendImgId)
{
	setImage(frontendImgId);
	setGeometry(0, 0, idealWidth(), idealHeight());
}

int32_t LobbyGameIconWidget::idealWidth()
{
	return imageDimensions.width() + (WZLOBBY_WIDG_PADDING_X * 2);
}

int32_t LobbyGameIconWidget::idealHeight()
{
	return imageDimensions.height() + (WZLOBBY_WIDG_PADDING_Y * 2);
}

void LobbyGameIconWidget::setImage(optional<UWORD> newFrontendImgID)
{
	missingImage = false;

	// validate newFrontendImgID value
	if (newFrontendImgID.has_value() && newFrontendImgID.value() >= FrontImages->numImages())
	{
		// out of bounds value (likely a mod has an incomplete / older frontend.img file)
		debug(LOG_ERROR, "frontend.img missing expected image id \"%u\" (num loaded: %zu) - remove any mods", static_cast<unsigned>(newFrontendImgID.value()), FrontImages->numImages());
		newFrontendImgID.reset();
		missingImage = true;
	}

	frontendImgID = newFrontendImgID;
}

void LobbyGameIconWidget::setCustomImageColor(optional<PIELIGHT> color)
{
	customImgColor = color;
}

void LobbyGameIconWidget::setImageSize(WzSize newImageDimensions)
{
	imageDimensions = newImageDimensions;
}

void LobbyGameIconWidget::display(int xOffset, int yOffset)
{
	int x0 = x() + xOffset;
	int y0 = y() + yOffset;
	int w = width();

	// Display the image horizontally centered
	int imgPosX0 = x0 + (w - imageDimensions.width()) / 2;
	// Display the image top-aligned
	int imgPosY0 = y0 + WZLOBBY_WIDG_PADDING_Y;

	if (frontendImgID.has_value())
	{
		PIELIGHT imgColor = customImgColor.value_or(WZCOL_TEXT_MEDIUM);
		iV_DrawImageFileAnisotropicTint(FrontImages, frontendImgID.value(), imgPosX0, imgPosY0, Vector2f(imageDimensions.width(), imageDimensions.height()), imgColor);
	}
	else if (missingImage)
	{
		pie_UniTransBoxFill(imgPosX0, imgPosY0, imgPosX0 + imageDimensions.width(), imgPosY0 + imageDimensions.height(), WZCOL_RED);
	}
}

void LobbyGameIconWidget::setTip(std::string string)
{
	tipStr = string;
}

std::string LobbyGameIconWidget::getTip()
{
	return tipStr;
}

class LobbyGamePlayersWidget : public WIDGET
{
protected:
	LobbyGamePlayersWidget() { }
	void initialize(uint8_t currentPlayers, uint8_t maxPlayers);
	virtual void geometryChanged() override;
public:
	static std::shared_ptr<LobbyGamePlayersWidget> make(uint8_t currentPlayers, uint8_t maxPlayers)
	{
		class make_shared_enabler: public LobbyGamePlayersWidget {};
		auto widget = std::make_shared<make_shared_enabler>();
		widget->initialize(currentPlayers, maxPlayers);
		return widget;
	}
	void updateValue(uint8_t currentPlayers, uint8_t maxPlayers);
	virtual int32_t idealWidth() override;
	virtual int32_t idealHeight() override;
private:
	std::shared_ptr<W_LABEL> playersCount;
};

void LobbyGamePlayersWidget::initialize(uint8_t currentPlayers, uint8_t maxPlayers)
{
	playersCount = std::make_shared<W_LABEL>();
	playersCount->setFont(font_regular, WZCOL_FORM_LIGHT);
	playersCount->setTextAlignment(WLAB_ALIGNTOP);
	playersCount->setCanTruncate(true);
	playersCount->setTransparentToClicks(true);
	attach(playersCount);

	updateValue(currentPlayers, maxPlayers);

	setGeometry(0, 0, idealWidth(), idealHeight());
}

void LobbyGamePlayersWidget::geometryChanged()
{
	int w = width();
	int h = height();
	playersCount->setGeometry(WZLOBBY_WIDG_PADDING_X, WZLOBBY_WIDG_PADDING_Y, w - (WZLOBBY_WIDG_PADDING_X * 2), h - (WZLOBBY_WIDG_PADDING_Y * 2));
}

void LobbyGamePlayersWidget::updateValue(uint8_t currentPlayers, uint8_t maxPlayers)
{
	auto str = WzString::number(currentPlayers) + "/" + WzString::number(maxPlayers);
	playersCount->setString(str);
	playersCount->setFontColour((currentPlayers == maxPlayers) ? WZCOL_TEXT_MEDIUM : WZCOL_FORM_LIGHT);
}

int32_t LobbyGamePlayersWidget::idealWidth()
{
	return playersCount->idealWidth() + (WZLOBBY_WIDG_PADDING_X * 2);
}

int32_t LobbyGamePlayersWidget::idealHeight()
{
	int32_t result = (WZLOBBY_WIDG_PADDING_Y * 2);
	result += playersCount->idealHeight();
	return result;
}

class LobbyGameNameWidget : public WIDGET
{
protected:
	LobbyGameNameWidget() { }
	void initialize(const WzString& roomName, const WzString& hostName, const std::string& hostPublicIdentity);
	virtual void geometryChanged() override;
public:
	static std::shared_ptr<LobbyGameNameWidget> make(const WzString& roomName, const WzString& hostName, const std::string& hostPublicIdentity)
	{
		class make_shared_enabler: public LobbyGameNameWidget {};
		auto widget = std::make_shared<make_shared_enabler>();
		widget->initialize(roomName, hostName, hostPublicIdentity);
		return widget;
	}
	void updateValue(const WzString& roomName, const WzString& hostName);
	virtual int32_t idealWidth() override;
	virtual int32_t idealHeight() override;
private:
	std::shared_ptr<W_LABEL> roomNameWidg;
	std::shared_ptr<W_LABEL> hostNameWidg;
};

void LobbyGameNameWidget::initialize(const WzString& roomName, const WzString& hostName, const std::string& hostPublicIdentity)
{
	roomNameWidg = std::make_shared<W_LABEL>();
	roomNameWidg->setFont(font_regular, WZCOL_TEXT_BRIGHT);
	roomNameWidg->setCanTruncate(true);
	roomNameWidg->setTransparentToClicks(true);
	attach(roomNameWidg);

	hostNameWidg = std::make_shared<W_LABEL>();
	hostNameWidg->setFont(font_regular, WZCOL_TEXT_MEDIUM);
	hostNameWidg->setCanTruncate(true);
	hostNameWidg->setTransparentToClicks(true);
	attach(hostNameWidg);
	if (!hostPublicIdentity.empty())
	{
		auto tipStr = astringf(_("Host Name: %s"), hostName.toUtf8().c_str());
		tipStr += "\n";
		tipStr += astringf(_("ID: %s"), hostPublicIdentity.c_str());
		hostNameWidg->setTip(tipStr);
	}

	updateValue(roomName, hostName);
	setGeometry(0, 0, idealWidth(), idealHeight());
}

void LobbyGameNameWidget::geometryChanged()
{
	int w = width();
	roomNameWidg->setGeometry(WZLOBBY_WIDG_PADDING_X, WZLOBBY_WIDG_PADDING_Y, w - (WZLOBBY_WIDG_PADDING_X * 2), roomNameWidg->idealHeight());
	int nextLineY0 = roomNameWidg->y() + roomNameWidg->height() + WZLOBBY_WIDG_LINE_SPACING;
	hostNameWidg->setGeometry(WZLOBBY_WIDG_PADDING_X, nextLineY0, w - (WZLOBBY_WIDG_PADDING_X * 2), hostNameWidg->idealHeight());
}

int32_t LobbyGameNameWidget::idealWidth()
{
	return std::max<int32_t>(roomNameWidg->idealWidth(), hostNameWidg->idealWidth()) + (WZLOBBY_WIDG_PADDING_X * 2);
}

int32_t LobbyGameNameWidget::idealHeight()
{
	int32_t result = (WZLOBBY_WIDG_PADDING_Y * 2);
	result += roomNameWidg->idealHeight() + WZLOBBY_WIDG_LINE_SPACING + hostNameWidg->idealHeight();
	return result;
}

void LobbyGameNameWidget::updateValue(const WzString& roomName, const WzString& hostName)
{
	roomNameWidg->setString(roomName);
	hostNameWidg->setString(hostName);
}

class LobbyGameMapWidget : public WIDGET
{
protected:
	LobbyGameMapWidget() { }
	void initialize(const WzString& mapName, optional<AllianceType> alliancesMode, BLIND_MODE blindMode);
	virtual void geometryChanged() override;
public:
	static std::shared_ptr<LobbyGameMapWidget> make(const WzString& mapName, optional<AllianceType> alliancesMode, BLIND_MODE blindMode)
	{
		class make_shared_enabler: public LobbyGameMapWidget {};
		auto widget = std::make_shared<make_shared_enabler>();
		widget->initialize(mapName, alliancesMode, blindMode);
		return widget;
	}
	void updateValue(const WzString& mapName, optional<AllianceType> alliancesMode, BLIND_MODE blindMode);
	virtual int32_t idealWidth() override;
	virtual int32_t idealHeight() override;
private:
	std::shared_ptr<W_LABEL> mapNameWidg;
	std::shared_ptr<W_LABEL> modeDetailsWidg;
	BLIND_MODE blindMode_ = BLIND_MODE::NONE;
};

void LobbyGameMapWidget::initialize(const WzString& mapName, optional<AllianceType> alliancesMode, BLIND_MODE blindMode)
{
	mapNameWidg = std::make_shared<W_LABEL>();
	mapNameWidg->setFont(font_regular, WZCOL_TEXT_MEDIUM);
	mapNameWidg->setCanTruncate(true);
	mapNameWidg->setTransparentToClicks(true);
	attach(mapNameWidg);

	modeDetailsWidg = std::make_shared<W_LABEL>();
	modeDetailsWidg->setFont(font_small, WZCOL_TEXT_MEDIUM);
	modeDetailsWidg->setCanTruncate(true);
	modeDetailsWidg->setTransparentToClicks(true);
	attach(modeDetailsWidg);

	updateValue(mapName, alliancesMode, blindMode);
	setGeometry(0, 0, idealWidth(), idealHeight());
}

void LobbyGameMapWidget::geometryChanged()
{
	int w = width();
	mapNameWidg->setGeometry(WZLOBBY_WIDG_PADDING_X, WZLOBBY_WIDG_PADDING_Y, w - (WZLOBBY_WIDG_PADDING_X * 2), mapNameWidg->idealHeight());
	int nextLineY0 = mapNameWidg->y() + mapNameWidg->height() + WZLOBBY_WIDG_LINE_SPACING;
	modeDetailsWidg->setGeometry(WZLOBBY_WIDG_PADDING_X, nextLineY0, w - (WZLOBBY_WIDG_PADDING_X * 2), modeDetailsWidg->idealHeight());
}

int32_t LobbyGameMapWidget::idealWidth()
{
	return std::max<int32_t>(mapNameWidg->idealWidth(), modeDetailsWidg->idealWidth()) + (WZLOBBY_WIDG_PADDING_X * 2);
}

int32_t LobbyGameMapWidget::idealHeight()
{
	int32_t result = (WZLOBBY_WIDG_PADDING_Y * 2);
	result += mapNameWidg->idealHeight() + WZLOBBY_WIDG_LINE_SPACING + modeDetailsWidg->idealHeight();
	return result;
}

static const char* allianceTypeToLobbyGameDisplayString(optional<AllianceType> v)
{
	if (!v.has_value())
	{
		return "";
	}
	switch (v.value())
	{
		case AllianceType::NO_ALLIANCES:
			// TRANSLATORS: "Free-for-all" acronym / abbreviation
			return _("FFA");
		case AllianceType::ALLIANCES:
			return _("Alliances Allowed");
		case AllianceType::ALLIANCES_TEAMS:
			return _("Teams");
		case AllianceType::ALLIANCES_UNSHARED:
			return _("Teams (No Shared Res)");
	}
	return "";
}

void LobbyGameMapWidget::updateValue(const WzString& mapName, optional<AllianceType> alliancesMode, BLIND_MODE blindMode)
{
	blindMode_ = blindMode;

	if (isBlindSimpleLobby(blindMode))
	{
		// Display "Matchmaking Room" label instead of the map
		mapNameWidg->setFont(font_regular_bold);
		mapNameWidg->setString(_("Matchmaking"));
	}
	else
	{
		mapNameWidg->setFont(font_regular);
		mapNameWidg->setString(mapName);
	}

	auto modeDetailsStr = WzString(allianceTypeToLobbyGameDisplayString(alliancesMode));
	if (blindMode != BLIND_MODE::NONE && blindMode != BLIND_MODE::BLIND_LOBBY_SIMPLE_LOBBY)
	{
		if (!modeDetailsStr.isEmpty())
		{
			modeDetailsStr += ", ";
		}
		switch (blindMode)
		{
			case BLIND_MODE::BLIND_LOBBY:
				modeDetailsStr += _("Blind Lobby");
				break;
			case BLIND_MODE::BLIND_GAME:
			case BLIND_MODE::BLIND_GAME_SIMPLE_LOBBY:
				modeDetailsStr += _("Blind Game");
				break;
			default:
				break;
		}
	}
	modeDetailsWidg->setString(modeDetailsStr);
}

class LobbyGameModsWidget : public WIDGET
{
protected:
	LobbyGameModsWidget() { }
	void initialize(const std::vector<netlobby::ModDetails>& modsList, bool noVtol, bool noTanks, bool noCyborgs);
	virtual void geometryChanged() override;
public:
	static std::shared_ptr<LobbyGameModsWidget> make(const std::vector<netlobby::ModDetails>& modsList, bool noVtol, bool noTanks, bool noCyborgs)
	{
		class make_shared_enabler: public LobbyGameModsWidget {};
		auto widget = std::make_shared<make_shared_enabler>();
		widget->initialize(modsList, noVtol, noTanks, noCyborgs);
		return widget;
	}
	void updateValue(const std::vector<netlobby::ModDetails>& modsList, bool noVtol, bool noTanks, bool noCyborgs);
	virtual int32_t idealWidth() override;
	virtual int32_t idealHeight() override;
private:
	std::shared_ptr<W_LABEL> modsListWidg;
	std::vector<std::shared_ptr<WIDGET>> limitWidgets;
	std::shared_ptr<W_LABEL> limitsListWidg;
};

void LobbyGameModsWidget::initialize(const std::vector<netlobby::ModDetails>& modsList, bool noVtol, bool noTanks, bool noCyborgs)
{
	modsListWidg = std::make_shared<W_LABEL>();
	modsListWidg->setFont(font_small, pal_RGBA(255,50,50,255));
	modsListWidg->setCanTruncate(true);
	modsListWidg->setTransparentToClicks(true);
	attach(modsListWidg);

	limitsListWidg = std::make_shared<W_LABEL>();
	limitsListWidg->setFont(font_small, WZCOL_TEXT_MEDIUM);
	limitsListWidg->setCanTruncate(true);
	limitsListWidg->setTransparentToClicks(true);
	attach(limitsListWidg);

	updateValue(modsList, noVtol, noTanks, noCyborgs);
	setGeometry(0, 0, idealWidth(), idealHeight());
}

void LobbyGameModsWidget::geometryChanged()
{
	int w = width();
	modsListWidg->setGeometry(WZLOBBY_WIDG_PADDING_X, WZLOBBY_WIDG_PADDING_Y + 2, w - (WZLOBBY_WIDG_PADDING_X * 2), modsListWidg->idealHeight());

	int nextLineY0 = modsListWidg->y() + modsListWidg->height() + WZLOBBY_WIDG_LINE_SPACING + 1;
	limitsListWidg->setGeometry(WZLOBBY_WIDG_PADDING_X, nextLineY0, w - (WZLOBBY_WIDG_PADDING_X * 2), limitsListWidg->idealHeight());

	int limitWidgX0 = 0;
	for (const auto& widg : limitWidgets)
	{
		widg->setGeometry(limitWidgX0, nextLineY0, widg->width(), widg->height());
		limitWidgX0 = widg->x() + widg->width();
	}
}

int32_t LobbyGameModsWidget::idealWidth()
{
	return std::max<int32_t>(modsListWidg->idealWidth(), limitsListWidg->idealWidth()) + (WZLOBBY_WIDG_PADDING_X * 2);
}

int32_t LobbyGameModsWidget::idealHeight()
{
	int32_t result = (WZLOBBY_WIDG_PADDING_Y * 2) + 2;
	result += modsListWidg->idealHeight() + (WZLOBBY_WIDG_LINE_SPACING + 1) + limitsListWidg->idealHeight();
	return result;
}

void LobbyGameModsWidget::updateValue(const std::vector<netlobby::ModDetails>& modsList, bool noVtol, bool noTanks, bool noCyborgs)
{
	WzString modsListStr;
	for (const auto& mod : modsList)
	{
		if (!modsListStr.isEmpty())
		{
			modsListStr.append(", ");
		}
		modsListStr.append(mod.name);
	}
	modsListWidg->setString(modsListStr);

	WzString limitsStr;
	if (noTanks)
	{
		limitsStr.append(_("No Tanks"));
	}
	if (noVtol)
	{
		if (!limitsStr.isEmpty())
		{
			limitsStr.append(" | ");
		}
		limitsStr.append(_("No VTOL"));
	}
	if (noCyborgs)
	{
		if (!limitsStr.isEmpty())
		{
			limitsStr.append(" | ");
		}
		limitsStr.append(_("No Cyborgs"));
	}
	limitsListWidg->setString(limitsStr);

	for (const auto& widg : limitWidgets)
	{
		detach(widg);
	}
	limitWidgets.clear();
}

class LobbyActionWidgets : public WIDGET
{
protected:
	LobbyActionWidgets() { }
	void initialize(const std::function<void()>& onSpectateHandler);
	virtual void geometryChanged() override;
public:
	typedef std::function<void()> OnSpectateHandlerFunc;

	static std::shared_ptr<LobbyActionWidgets> make(const OnSpectateHandlerFunc& onSpectateHandler)
	{
		class make_shared_enabler: public LobbyActionWidgets {};
		auto widget = std::make_shared<make_shared_enabler>();
		widget->initialize(onSpectateHandler);
		return widget;
	}
	virtual int32_t idealWidth() override;
	virtual int32_t idealHeight() override;
private:
	std::shared_ptr<WzFrontendImageButton> watchButton;
};

void LobbyActionWidgets::initialize(const OnSpectateHandlerFunc& onSpectateHandler)
{
	watchButton = WzFrontendImageButton::make(IMAGE_EYE);
	watchButton->setTip(_("Spectate Game"));
	watchButton->setBorderDrawMode(WzFrontendImageButton::BorderDrawMode::Highlighted);
	watchButton->setGeometry(0, 0, watchButton->idealWidth(), watchButton->idealHeight());
	attach(watchButton);
	if (onSpectateHandler)
	{
		watchButton->addOnClickHandler([onSpectateHandler](W_BUTTON&) {
			onSpectateHandler();
		});
	}
	else
	{
		watchButton->hide();
	}
}

void LobbyActionWidgets::geometryChanged()
{
	int w = width();

	int watchButtonX0 = w - watchButton->width();
	int watchButtonY0 = WZLOBBY_WIDG_PADDING_Y;
	watchButton->setGeometry(watchButtonX0, watchButtonY0, watchButton->width(), watchButton->height());
}

int32_t LobbyActionWidgets::idealWidth()
{
	int32_t result = (WZLOBBY_WIDG_PADDING_X * 2);
	result += watchButton->idealWidth();
	return result;
}

int32_t LobbyActionWidgets::idealHeight()
{
	return watchButton->idealHeight() + (WZLOBBY_WIDG_PADDING_Y * 2);
}

// MARK: - ConnectToForm

class ConnectToForm : public W_BUTTON
{
public:
	typedef std::function<void(WzString inputStr, bool asSpectator)> OnSubmitFunc;
protected:
	ConnectToForm() { }
	~ConnectToForm();
	void initialize(const OnSubmitFunc& onSubmitFunc);

	virtual void geometryChanged() override;
	virtual void display(int xOffset, int yOffset) override;
	virtual void highlight(W_CONTEXT *psContext) override;

public:
	static std::shared_ptr<ConnectToForm> make(const OnSubmitFunc& onSubmitFunc)
	{
		class make_shared_enabler: public ConnectToForm {};
		auto widget = std::make_shared<make_shared_enabler>();
		widget->initialize(onSubmitFunc);
		return widget;
	}

	void clicked(W_CONTEXT *psContext, WIDGET_KEY key = WKEY_PRIMARY) override;
	virtual std::shared_ptr<WIDGET> findMouseTargetRecursive(W_CONTEXT *psContext, WIDGET_KEY key, bool wasPressed) override;

	virtual int32_t idealWidth() override;
	virtual int32_t idealHeight() override;
private:
	bool isMouseOverFormOrChildren() const;
	int32_t buttonAreaWidth();
	void recalcLayout();

	void submitInputBox(bool asSpectator);
	void submit(WzString inputStr, bool asSpectator);

	std::shared_ptr<PopoverMenuWidget> buildJoinOptionsMenu();
	void displayJoinOptionsMenu(const std::shared_ptr<WIDGET> &psParent);

private:
	OnSubmitFunc onSubmitFunc;
	optional<UWORD> frontendImgID = nullopt;
	WzString joinTextStr;

	PIELIGHT inputBoxBackground;
	PIELIGHT inputBoxBackgroundHighlight;

	std::shared_ptr<W_EDITBOX> inputBox;
	WzText wzText;

	std::shared_ptr<PopoverWidget> optionsPopover;

	optional<UDWORD> lastFrameMouseIsOverRowOrChildren = nullopt;

	const int32_t minInputBoxHeight = 16;
	const int32_t inputBoxHorizontalPadding = 6;

	const int32_t verticalPadding = 5;
	const int32_t horizontalPadding = 8;

	const int32_t maxButtonTextAreaWidth = 100;

	const int32_t imageDimensions = 12;
	const int32_t horizontalPaddingBetweenButtonSplit = 6;
};

ConnectToForm::~ConnectToForm()
{
	if (optionsPopover)
	{
		optionsPopover->close();
	}
}

static std::shared_ptr<WzClickableOptionsChoiceWidget> addClickableJoinMenuItem(const std::shared_ptr<PopoverMenuWidget>& menu, const WzString& text, const std::function<void (WIDGET_KEY)>& onClick, bool disabled = false, iV_fonts fontId = font_regular)
{
	auto result = WzClickableOptionsChoiceWidget::make(fontId);
	result->setString(text);
	result->setTextAlignment(WLAB_ALIGNRIGHT);
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

std::shared_ptr<PopoverMenuWidget> ConnectToForm::buildJoinOptionsMenu()
{
	std::weak_ptr<ConnectToForm> weakSelf = std::dynamic_pointer_cast<ConnectToForm>(shared_from_this());

	auto popoverMenu = PopoverMenuWidget::make();

	auto addSpacerWidget = [&popoverMenu](int height){
		auto spacerWidget = std::make_shared<WIDGET>();
		spacerWidget->setGeometry(0, 0, 10, height);
		popoverMenu->addMenuItem(spacerWidget);
	};

	std::string lastIPServerName = war_getLastIpServerConnect();
	bool inputItemsDisabled = (inputBox->getString().isEmpty());
	bool displayStandardInputJoinItems = (!inputItemsDisabled || lastIPServerName.empty());

	if (displayStandardInputJoinItems)
	{
		addClickableJoinMenuItem(popoverMenu, _("Join as Player"), [weakSelf](WIDGET_KEY) {
			auto strongSelf = weakSelf.lock();
			ASSERT_OR_RETURN(, strongSelf != nullptr, "Null widget?");
			strongSelf->submitInputBox(false);
		})->setDisabled(inputItemsDisabled);
		addClickableJoinMenuItem(popoverMenu, _("Join as Spectator"), [weakSelf](WIDGET_KEY) {
			auto strongSelf = weakSelf.lock();
			ASSERT_OR_RETURN(, strongSelf != nullptr, "Null widget?");
			strongSelf->submitInputBox(true);
		})->setDisabled(inputItemsDisabled);
	}

	if (!lastIPServerName.empty())
	{
		if (displayStandardInputJoinItems)
		{
			addSpacerWidget(10);
		}

		// [Last IP: %s]
		addClickableJoinMenuItem(popoverMenu, WzString::format(_("Last IP: %s"), lastIPServerName.c_str()), nullptr, true);

		addClickableJoinMenuItem(popoverMenu, _("Join as Player"), [weakSelf, lastIPServerName](WIDGET_KEY) {
			auto strongSelf = weakSelf.lock();
			ASSERT_OR_RETURN(, strongSelf != nullptr, "Null widget?");
			strongSelf->submit(WzString::fromUtf8(lastIPServerName), false);
		});

		addClickableJoinMenuItem(popoverMenu, _("Join as Spectator"), [weakSelf, lastIPServerName](WIDGET_KEY) {
			auto strongSelf = weakSelf.lock();
			ASSERT_OR_RETURN(, strongSelf != nullptr, "Null widget?");
			strongSelf->submit(WzString::fromUtf8(lastIPServerName), true);
		});

		addSpacerWidget(2);

		addClickableJoinMenuItem(popoverMenu, _("Clear Last IP"), [](WIDGET_KEY) {
			war_setLastIpServerConnect("");
		}, false, font_small);
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

void ConnectToForm::displayJoinOptionsMenu(const std::shared_ptr<WIDGET> &psParent)
{
	auto popoverMenu = buildJoinOptionsMenu();

	std::weak_ptr<ConnectToForm> weakSelf = std::dynamic_pointer_cast<ConnectToForm>(shared_from_this());
	setState(WBUT_CLICKLOCK);

	if (optionsPopover)
	{
		optionsPopover->close();
	}
	optionsPopover = popoverMenu->openMenu(psParent, PopoverWidget::Alignment::RightOfParent, Vector2i(0, 1), [weakSelf]() {
		// close handler
		if (auto strongConnectToForm = weakSelf.lock())
		{
			strongConnectToForm->setState(0); // clear clicklock state
		}
	});
}

void displayConnectToEditBox(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset)
{
	// do not draw any background
}

constexpr int maxConnectToInputLength = 256;

static WzString removeLobbyGameIdPrefixPattern(const WzString& input)
{
	const std::string& stdStrInput = input.toStdString();

	// Find the first colon
	size_t colonPos = stdStrInput.find(':');

	// Must be at least one character before the colon
	if (colonPos == std::string::npos || colonPos == 0)
	{
		return input;
	}

	// Check for 1 or more characters of whitespace after the colon
	size_t afterColon = colonPos + 1;
	size_t firstNonSpace = afterColon;

	while (firstNonSpace < stdStrInput.length() && std::isspace(static_cast<unsigned char>(stdStrInput[firstNonSpace])))
	{
		firstNonSpace++;
	}

	// Must have at least one whitespace character after the colon
	// (This is to exclude detection of ip:port combinations)
	if (firstNonSpace == afterColon)
	{
		return input;
	}

	// Only return the remaining substring if it is non-empty
	if (firstNonSpace < stdStrInput.length())
	{
		return WzString::fromUtf8(stdStrInput.substr(firstNonSpace));
	}

	// Otherwise, return original
	return input;
}

void ConnectToForm::initialize(const OnSubmitFunc& _onSubmitFunc)
{
	onSubmitFunc = _onSubmitFunc;

	if (FrontImages != nullptr)
	{
		frontendImgID = IMAGE_CARET_DOWN_FILL;
	}

	inputBoxBackground = WZCOL_TRANSPARENT_BOX;
	inputBoxBackground.byte.a = inputBoxBackground.byte.a / 3;

	inputBoxBackgroundHighlight = WZCOL_TRANSPARENT_BOX;
	inputBoxBackgroundHighlight.byte.a = inputBoxBackgroundHighlight.byte.a / 2;

	auto weakSelf = std::weak_ptr<ConnectToForm>(std::dynamic_pointer_cast<ConnectToForm>(shared_from_this()));

	inputBox = std::make_shared<W_EDITBOX>();
	attach(inputBox);
	inputBox->setGeometry(horizontalPadding, 0, 280, minInputBoxHeight);
	inputBox->pBoxDisplay = displayConnectToEditBox;
	inputBox->setMaxStringSize(maxConnectToInputLength);
	inputBox->setPlaceholder(_("GameID or IP:port"));
	inputBox->setPlaceholderTextColor(WZCOL_TEXT_MEDIUM);

	inputBox->setOnPasteTransformFunc([](const WzString& pastedString) -> WzString {
		WzString result = pastedString;

		// Replace any \r, \n chars with a space
		result.replace(WzUniCodepoint::fromASCII('\r'), " ");
		result.replace(WzUniCodepoint::fromASCII('\n'), " ");

		// Trim any whitespace
		result = result.trimmed();

		// Check if text begins with "Lobby GameId: ", the standard prefix used when outputting the GameId on host / join
		// NOTE: The translated version of this prefix is output - someone might copy it to another who uses a different language
		// So use a heuristic: Check for "<prefix>: <possible gameid>" (key being the ":" followed by a space), then remove that prefix if found
		// (This should avoid interfering with ip:port or hostname:port strings, which fortunately should not have a space in them)
		auto withRemovedPrefix = removeLobbyGameIdPrefixPattern(result).trimmed();
		if (!withRemovedPrefix.isEmpty())
		{
			result = withRemovedPrefix;
		}

		return result;
	});

	inputBox->setOnReturnHandler([weakSelf](W_EDITBOX& widg) {
		auto strongParent = weakSelf.lock();
		ASSERT_OR_RETURN(, strongParent != nullptr, "No parent?");
		strongParent->submitInputBox(false);
	});

	joinTextStr = _("Join");
	wzText.setText(joinTextStr, font_regular);

	addOnClickHandler([weakSelf](W_BUTTON&) {
		auto strongConnectToForm = weakSelf.lock();
		ASSERT_OR_RETURN(, strongConnectToForm != nullptr, "Null form?");
		strongConnectToForm->displayJoinOptionsMenu(strongConnectToForm);
	});
}

void ConnectToForm::geometryChanged()
{
	recalcLayout();
}

void ConnectToForm::clicked(W_CONTEXT *psContext, WIDGET_KEY key)
{
	int clickLocationX = psContext->mx - x();

	// If click is over button area, forward to W_BUTTON impl
	if (clickLocationX >= (width() - buttonAreaWidth()))
	{
		W_BUTTON::clicked(psContext, key);
	}
	else if (clickLocationX <= inputBoxHorizontalPadding)
	{
		// padding area for left of input box - simulate click
		auto weakSelf = std::weak_ptr<ConnectToForm>(std::dynamic_pointer_cast<ConnectToForm>(shared_from_this()));
		widgScheduleTask([weakSelf]() {
			auto strongSelf = weakSelf.lock();
			ASSERT_OR_RETURN(, strongSelf != nullptr, "Widget gone?");
			W_CONTEXT context = W_CONTEXT::ZeroContext();
			strongSelf->inputBox->simulateClick(&context);
		});
	}
}

std::shared_ptr<WIDGET> ConnectToForm::findMouseTargetRecursive(W_CONTEXT *psContext, WIDGET_KEY key, bool wasPressed)
{
	auto mouseOverWidget = WIDGET::findMouseTargetRecursive(psContext, key, wasPressed);
	if (mouseOverWidget != nullptr)
	{
		// if findMouseTargetRecursive was called for this row, it means the mouse is over it
		// (see WIDGET::findMouseTargetRecursive)
		lastFrameMouseIsOverRowOrChildren = frameGetFrameNumber();
	}
	else
	{
		lastFrameMouseIsOverRowOrChildren = nullopt;
	}
	return mouseOverWidget;
}

bool ConnectToForm::isMouseOverFormOrChildren() const
{
	if (!lastFrameMouseIsOverRowOrChildren.has_value())
	{
		return false;
	}
	return lastFrameMouseIsOverRowOrChildren.value() == frameGetFrameNumber();
}

void ConnectToForm::highlight(W_CONTEXT *psContext)
{
	int mouseXLoc = psContext->mx - x();
	if (mouseXLoc <= inputBoxHorizontalPadding)
	{
		// silently ignoring button highlight when over this padding area that "belongs" to the inputBox
		return;
	}
	W_BUTTON::highlight(psContext);
}

void ConnectToForm::display(int xOffset, int yOffset)
{
	int x0 = xOffset + x();
	int y0 = yOffset + y();
	int w = width();

	bool isDown = (state & (WBUT_DOWN | WBUT_LOCK | WBUT_CLICKLOCK)) != 0;
	bool isDisabled = (state & WBUT_DISABLE) != 0;
	bool isMouseOverButtonArea = ((state & WBUT_HIGHLIGHT) != 0);
	bool isInputEditing = inputBox->isEditing();
	bool isHighlight = isMouseOverFormOrChildren() || isInputEditing;
	bool isMouseOverInputBoxArea = isMouseOverFormOrChildren() && !isMouseOverButtonArea;

	int buttonAreaW = buttonAreaWidth();

	// Draw background for input box area
	{
		int boxX0 = x0;
		int boxY0 = y0;
		int boxX1 = boxX0 + (width() - buttonAreaW);
		int boxY1 = boxY0 + height();
		pie_UniTransBoxFill(boxX0, boxY0, boxX1, boxY1, (isInputEditing) ? WZCOL_TRANSPARENT_BOX : (isHighlight) ? inputBoxBackgroundHighlight : inputBoxBackground);
	}

	// Draw background for button
	{
		int boxX0 = x0 + (w - buttonAreaW);
		int boxY0 = y0;
		int boxX1 = boxX0 + buttonAreaW;
		int boxY1 = boxY0 + height();
		pie_UniTransBoxFill(boxX0, boxY0, boxX1, boxY1, WZCOL_TRANSPARENT_BOX);
	}

	// Draw entire outer border
	{
		PIELIGHT borderColor = WZCOL_TEXT_MEDIUM;
		if (isInputEditing || isMouseOverInputBoxArea)
		{
			borderColor = WZCOL_TEXT_BRIGHT;
		}

		if (isDisabled)
		{
			borderColor.byte.a = (borderColor.byte.a / 2);
		}
		int boxX0 = x0;
		int boxY0 = y0;
		int boxX1 = boxX0 + width();
		int boxY1 = boxY0 + height();
		iV_Box(boxX0 + 1, boxY0 + 1, boxX1, boxY1, borderColor);
	}

	// Draw Button area outer border
	if (!isInputEditing && !isMouseOverInputBoxArea)
	{
		PIELIGHT borderColor = (isDown) ? WZCOL_TEXT_DARK : ((isMouseOverButtonArea) ? WZCOL_TEXT_BRIGHT : WZCOL_TEXT_MEDIUM);
		if (isDisabled)
		{
			borderColor.byte.a = (borderColor.byte.a / 2);
		}
		int boxX0 = x0 + (w - buttonAreaW);
		int boxY0 = y0;
		int boxX1 = boxX0 + buttonAreaW;
		int boxY1 = boxY0 + height();
		iV_Box(boxX0 + 1, boxY0 + 1, boxX1, boxY1, borderColor);
	}
	else
	{
		// outer border handled by the "entire" case - just draw the dividing line
		int boxX0 = x0 + (w - buttonAreaW);
		int boxY0 = y0;
		int boxY1 = boxY0 + height();
		iV_Line(boxX0 + 1, boxY0, boxX0 + 1, boxY1, (isMouseOverButtonArea) ? WZCOL_TEXT_BRIGHT : WZCOL_TEXT_MEDIUM);
	}

	PIELIGHT textColor = (isMouseOverButtonArea) ? WZCOL_TEXT_BRIGHT : WZCOL_TEXT_MEDIUM;
	if (isDisabled)
	{
		textColor.byte.a = (textColor.byte.a / 2);
	}

	// Draw image portion of button
	const int splitButtonImageAreaX0 = x0 + (w - horizontalPadding - imageDimensions - (horizontalPaddingBetweenButtonSplit / 2));
	{
		// Display the image centered in the image split area
		int imgPosX0 = splitButtonImageAreaX0 + (horizontalPaddingBetweenButtonSplit / 2);
		int imgPosY0 = y0 + (height() - imageDimensions) / 2;

		if (frontendImgID.has_value())
		{
			PIELIGHT imgColor = textColor;
			iV_DrawImageFileAnisotropicTint(FrontImages, frontendImgID.value(), imgPosX0, imgPosY0, Vector2f(imageDimensions, imageDimensions), imgColor);
		}
	}

	// Draw text portion of button
	{
		// Draw the main text
		int textX0 = std::max<int>(x0 + horizontalPadding, splitButtonImageAreaX0 - wzText.width());
		int textY0 = static_cast<int>(y0 + (height() - wzText.lineSize()) / 2 - float(wzText.aboveBase()));

		const int maxTextDisplayableWidth = splitButtonImageAreaX0 - textX0;
		int maxDisplayableMainTextWidth = maxTextDisplayableWidth;
		bool isTruncated = maxDisplayableMainTextWidth < wzText.width();
		if (isTruncated)
		{
			maxDisplayableMainTextWidth -= (iV_GetEllipsisWidth(wzText.getFontID()) + 2);
		}
		wzText.render(textX0, textY0, textColor, 0.0f, maxDisplayableMainTextWidth);
		if (isTruncated)
		{
			// Render ellipsis
			iV_DrawEllipsis(wzText.getFontID(), Vector2f(textX0 + maxDisplayableMainTextWidth + 2, textY0), textColor);
		}
	}
}

int32_t ConnectToForm::idealWidth()
{
	return 360;
}

int32_t ConnectToForm::idealHeight()
{
	return (verticalPadding * 2) + std::max<int>(iV_GetTextLineSize(font_regular), minInputBoxHeight);
}

int32_t ConnectToForm::buttonAreaWidth()
{
	return horizontalPadding + std::min<int32_t>(wzText.width(), maxButtonTextAreaWidth) + horizontalPaddingBetweenButtonSplit + imageDimensions + horizontalPadding;
}

void ConnectToForm::recalcLayout()
{
	int w = width();
	int h = height();

	// input box takes up the leftmost portion, minus the space reserved for the button area
	int inputBoxWidth = w - buttonAreaWidth() - (inputBoxHorizontalPadding * 2);
	if (inputBoxWidth > 0)
	{
		inputBox->setGeometry(inputBoxHorizontalPadding, 0, inputBoxWidth, h);
	}
}

void ConnectToForm::submit(WzString inputStr, bool asSpectator)
{
	ASSERT_OR_RETURN(, onSubmitFunc != nullptr, "Missing onSubmitFunc");
	onSubmitFunc(inputStr, asSpectator);
}

void ConnectToForm::submitInputBox(bool asSpectator)
{
	submit(inputBox->getString(), asSpectator);
	inputBox->setString("");
}

// MARK: - LobbyBrowser Form

class LobbyBrowser : public WIDGET
{
protected:
	LobbyBrowser() { }
	void initialize(const std::function<void()>& onBackButtonFunc);
	virtual void geometryChanged() override;
	virtual void display(int xOffset, int yOffset) override;
public:
	static std::shared_ptr<LobbyBrowser> make(const std::function<void()>& onBackButtonFunc)
	{
		class make_shared_enabler: public LobbyBrowser {};
		auto widget = std::make_shared<make_shared_enabler>();
		widget->initialize(onBackButtonFunc);
		return widget;
	}
	virtual ~LobbyBrowser();

	void triggerRefresh();

protected:
	virtual int32_t idealWidth() override;
	virtual int32_t idealHeight() override;

private:
	void joinLobbyGame(size_t idx, bool asSpectator);
	void setMotd(const std::string& motd);
	void recalcLobbyStatusOverlayLayout();

private:
	std::shared_ptr<W_BUTTON> makeBackButton();
	std::shared_ptr<WIDGET> createColumnHeaderLabel(const char* text, WzTextAlignment align, int minLeftPadding);
	std::shared_ptr<ScrollableTableWidget> createGamesList();
	void triggerAsyncGameListFetch();
	void processAsyncGameListFetchResults(netlobby::ListResult&& results);
	void populateTableFromGameList(bool force = false);
	std::vector<std::shared_ptr<WIDGET>> createLobbyGameRowColumnWidgets(size_t idx, const netlobby::GameListing& listing);

	std::shared_ptr<PopoverMenuWidget> createFiltersPopoverForm();
	void displayFiltersOverlay(const std::shared_ptr<WIDGET>& psParent);

private:
	std::shared_ptr<W_BUTTON> backButton;
	std::shared_ptr<ConnectToForm> connectToForm;
	std::shared_ptr<WzFrontendImageButton> filtersButton;
	std::shared_ptr<WzFrontendImageButton> refreshButton;
	std::shared_ptr<ScrollableTableWidget> table;
	std::vector<size_t> minimumColumnWidths;
	int minDesiredTableWidth = 0;
	std::shared_ptr<LobbyStatusOverlayWidget> lobbyStatusOverlayWidg;
	std::shared_ptr<ScrollableListWidget> lobbyStatusMessageContainer;

	std::vector<netlobby::GameListing> currentResults;
	bool filterIncompatible = true;
	bool filterEmpty = false;
	bool filterModded = false;
	bool filterIPv6Only = false;

	std::shared_ptr<PopoverMenuWidget> currentPopoverMenu;

	bool hasPendingFetchRequest = false;

	const int connectToFormHorizontalPadding = 15;
};

void LobbyBrowser::triggerRefresh()
{
	triggerAsyncGameListFetch();
}

void LobbyBrowser::triggerAsyncGameListFetch()
{
	if (hasPendingFetchRequest) { return; }

	if (NET_getLobbyDisabled())
	{
		lobbyStatusOverlayWidg->setString(_("There appears to be a game update available!"));
		recalcLobbyStatusOverlayLayout();
		lobbyStatusOverlayWidg->show();
		return;
	}

	refreshButton->setState(WBUT_DISABLE);
	lobbyStatusOverlayWidg->showIndeterminateIndicator(true);
	lobbyStatusOverlayWidg->showResetLobbyAddressButton(false);
	lobbyStatusOverlayWidg->setString("");
	recalcLobbyStatusOverlayLayout();
	lobbyStatusOverlayWidg->show(true);

	auto weakSelf = std::weak_ptr<LobbyBrowser>(std::static_pointer_cast<LobbyBrowser>(shared_from_this()));
	auto dispatchedListRequest = netlobby::EnumerateGames(NETgetLobbyserverAddress(), [weakSelf](netlobby::ListResult results) {
		wzAsyncExecOnMainThread([weakSelf, results = std::move(results)]() mutable {
			auto strongSelf = weakSelf.lock();
			if (strongSelf == nullptr)
			{
				debug(LOG_WZ, "Widget gone");
				// Parent widget is gone
				// (Can happen if the user closes this form before the async task returns results)
				return;
			}
			strongSelf->processAsyncGameListFetchResults(std::move(results));
		});
	}, 200);

	if (!dispatchedListRequest)
	{
		// Restore widgets config
		refreshButton->setState(0);
		lobbyStatusOverlayWidg->showIndeterminateIndicator(false);

		debug(LOG_INFO, "Failed to dispatch lobby list request");

		// Display error if no results
		if (currentResults.empty())
		{
			lobbyStatusOverlayWidg->setString(_("Error dispatching request"));
			lobbyStatusOverlayWidg->show();
		}
		recalcLobbyStatusOverlayLayout();
		return;
	}

	hasPendingFetchRequest = true;
}

void LobbyBrowser::joinLobbyGame(size_t idx, bool asSpectator)
{
	ASSERT_OR_RETURN(, idx < currentResults.size(), "Invalid idx: %zu", idx);

	clearActiveConsole();

	ingame.localOptionsReceived = false;								// note, we are awaiting options
	sstrcpy(game.name, currentResults[idx].details.name.toUtf8().c_str());		// store name

	ExpectedHostProperties expectedHostProps;
	expectedHostProps.hostPublicKey = base64Decode(currentResults[idx].details.host.publicIdentity);

	startLobbyJoiningAttempt(sPlayer, NETgetLobbyserverAddress(), currentResults[idx].gameId, asSpectator, expectedHostProps, &currentResults[idx].availableConnectionTypes);
}

void LobbyBrowser::setMotd(const std::string &motd)
{
	auto lobbyMOTDParagraph = std::make_shared<Paragraph>();
	lobbyMOTDParagraph->setGeometry(0, 0, lobbyStatusMessageContainer->calculateListViewWidth(), lobbyStatusMessageContainer->calculateListViewHeight());
	lobbyMOTDParagraph->setFont(font_regular);
	lobbyMOTDParagraph->setFontColour(WZCOL_TEXT_MEDIUM);
	lobbyMOTDParagraph->setTransparentToMouse(true);
	lobbyMOTDParagraph->addText(WzString::fromUtf8(motd));
	lobbyStatusMessageContainer->addItem(lobbyMOTDParagraph);
}

void LobbyBrowser::processAsyncGameListFetchResults(netlobby::ListResult&& results)
{
	hasPendingFetchRequest = false;

	refreshButton->setState(0);
	lobbyStatusOverlayWidg->showIndeterminateIndicator(false);

	if (!results.has_value())
	{
		if (results.error().errCode == "RATE_LIMITED" && !currentResults.empty())
		{
			// Have prior results, hit temporary rate limit - silently ignore
			return;
		}

		lobbyStatusOverlayWidg->setString(_("Failed to connect to lobby server"));
		if constexpr (fnv1a_hash(netlobby::GetDefaultLobbyAddress()) == LOB_DEFAULT_BEHAVIOR_CHECKSUM)
		{
			if (NETgetLobbyserverAddress() != netlobby::GetDefaultLobbyAddress())
			{
				lobbyStatusOverlayWidg->showResetLobbyAddressButton(true);
			}
		}
		recalcLobbyStatusOverlayLayout();

		table->clearRows();
		lobbyStatusMessageContainer->clear();
		currentResults.clear();
		return;
	}

	table->clearRows();
	lobbyStatusMessageContainer->clear();

	currentResults = std::move(results.value().games);
	populateTableFromGameList(true);

	// Display the lobby MOTD
	if (!results.value().lobbyMOTD.empty())
	{
		setMotd(results.value().lobbyMOTD);
	}
}

static inline optional<AllianceType> alliancesModeFromLobbyGameDetails(uint8_t alliancesMode)
{
	if (alliancesMode <= ALLIANCE_TYPE_MAX)
	{
		return static_cast<AllianceType>(alliancesMode);
	}
	return nullopt;
}

static inline optional<BLIND_MODE> blindModeFromLobbyGameDetails(uint8_t blindMode)
{
	if (blindMode <= static_cast<uint8_t>(BLIND_MODE_MAX))
	{
		return static_cast<BLIND_MODE>(blindMode);
	}
	return nullopt;
}

static bool isIPv6Only(const std::vector<netlobby::ConnectionType>& availableConnectionTypes)
{
	return !std::any_of(availableConnectionTypes.begin(), availableConnectionTypes.end(), [](const netlobby::ConnectionType& connType) -> bool {
		return connType.ipVersion.value_or(netlobby::IPVersion::IPv4) == netlobby::IPVersion::IPv4;
	});
}

void LobbyBrowser::populateTableFromGameList(bool force)
{
	if (!force && currentResults.empty())
	{
		// do nothing, so we don't clear any lobby error status message (ex. when changing filters)
		return;
	}

	table->clearRows();

	std::vector<size_t> currentMaxColumnWidths;
	currentMaxColumnWidths.resize(table->getNumColumns(), 0);

	auto weakSelf = std::weak_ptr<LobbyBrowser>(std::static_pointer_cast<LobbyBrowser>(shared_from_this()));

	bool foundGreaterVersion = false;

	// Populate table with the results
	for (size_t idx = 0; idx < currentResults.size(); idx++)
	{
		const auto& listing = currentResults[idx];
		const auto& gameInfo = listing.details;

		foundGreaterVersion = foundGreaterVersion || NETisGreaterVersion(gameInfo.netcodeVer.major, gameInfo.netcodeVer.minor);

		bool isCompatibleGame = NETisCorrectVersion(gameInfo.netcodeVer.major, gameInfo.netcodeVer.minor);
		if (filterIncompatible && !isCompatibleGame)
		{
			continue;
		}
		if (filterEmpty && gameInfo.players.current == 0 && !isBlindSimpleLobby(blindModeFromLobbyGameDetails(gameInfo.blindMode).value_or(BLIND_MODE::NONE)))
		{
			continue;
		}
		if (filterModded && !gameInfo.mods.empty())
		{
			continue;
		}
		if (filterIPv6Only && isIPv6Only(listing.availableConnectionTypes))
		{
			continue;
		}

		auto columnWidgets = createLobbyGameRowColumnWidgets(idx, listing);
		int32_t rowHeight = 0;
		for (size_t i = 0; i < columnWidgets.size(); ++i)
		{
			currentMaxColumnWidths[i] = std::max<size_t>(currentMaxColumnWidths[i], static_cast<size_t>(std::max<int>(columnWidgets[i]->idealWidth(), 0)));
			rowHeight = std::max<int32_t>(rowHeight, columnWidgets[i]->idealHeight());
		}

		auto row = TableRow::make(columnWidgets, rowHeight);
		row->setDisabledColor(pal_RGBA(0, 0, 0, 100));
		if (isCompatibleGame)
		{
			const bool blindSimpleLobby = isBlindSimpleLobby(blindModeFromLobbyGameDetails(gameInfo.blindMode).value_or(BLIND_MODE::NONE));
			row->setHighlightsOnMouseOver(true);
			row->setHighlightColor(pal_RGBA(5,29,245,200));
			row->setBackgroundColor(blindSimpleLobby ? pal_RGBA(0,20,130,50) : pal_RGBA(0,0,0,50));
		}
		else
		{
			row->setDisabled(true);
			row->setDisabledColor(pal_RGBA(0,0,0,120));
			WzString tooltip = _("Your version of Warzone is incompatible with this game.");
			tooltip += "\n";
			tooltip += WzString::format(_("Host Version: %s"), gameInfo.versionStr.toUtf8().c_str());
			row->setTip(tooltip.toUtf8());
		}
		row->addOnClickHandler([weakSelf, idx](W_BUTTON&) {
			auto strongSelf = weakSelf.lock();
			ASSERT_OR_RETURN(, strongSelf != nullptr, "Parent no longer exists?");
			strongSelf->joinLobbyGame(idx, false);
		});
		table->addRow(row);
	}

	if (table->getNumRows() == 0)
	{
		WzString omittedStr;
		if (currentResults.size() > 50)
		{
			omittedStr = _("(50+ omitted)");
		}
		else
		{
			omittedStr = WzString::format(_("(%u omitted)"), static_cast<unsigned int>(currentResults.size()));
		}

		if (!filterEmpty && !filterModded)
		{
			lobbyStatusOverlayWidg->setString(_("No games for your version"));
		}
		else
		{
			WzString statusStr = _("No games matching filters");
			statusStr += " " + omittedStr;
			lobbyStatusOverlayWidg->setString(statusStr);
		}

		if (NET_getLobbyDisabled() || getVersionCheckNewVersionAvailable().value_or(false) || foundGreaterVersion)
		{
			lobbyStatusOverlayWidg->setString(_("There appears to be a game update available!"));
		}

		lobbyStatusOverlayWidg->show();
	}
	else
	{
		lobbyStatusOverlayWidg->hide();
	}

	recalcLobbyStatusOverlayLayout();
}

std::vector<std::shared_ptr<WIDGET>> LobbyBrowser::createLobbyGameRowColumnWidgets(size_t idx, const netlobby::GameListing& listing)
{
	const auto& gameInfo = listing.details;
	std::vector<std::shared_ptr<WIDGET>> columnWidgets;

	std::shared_ptr<LobbyGameIconWidget> privateGameWidg;
	if (gameInfo.isPrivate)
	{
		privateGameWidg = LobbyGameIconWidget::make(IMAGE_LOCK);
	}
	else
	{
		privateGameWidg = LobbyGameIconWidget::make(nullopt);
	}
	privateGameWidg->setTransparentToClicks(true);
	columnWidgets.push_back(privateGameWidg);

	if (!isBlindSimpleLobby(blindModeFromLobbyGameDetails(gameInfo.blindMode).value_or(BLIND_MODE::NONE)))
	{
		auto playersWidg = LobbyGamePlayersWidget::make(gameInfo.players.current, gameInfo.players.max);
		playersWidg->setTransparentToClicks(true);
		columnWidgets.push_back(playersWidg);
	}
	else
	{
		UWORD frontendImgID = (gameInfo.players.current > 0) ? IMAGE_PERSON_FILL : IMAGE_PERSON;
		auto playersWidg = LobbyGameIconWidget::make(frontendImgID);
		if (gameInfo.players.current > 0)
		{
			playersWidg->setTip(astringf(_("%d players queued"), static_cast<int>(gameInfo.players.current)));
		}
		else
		{
			playersWidg->setTip(_("Matchmaking Room: Open"));
		}
		playersWidg->setCustomImageColor(WZCOL_TEXT_BRIGHT);
		playersWidg->setTransparentToClicks(true);
		columnWidgets.push_back(playersWidg);
	}

	auto nameWidg = LobbyGameNameWidget::make(gameInfo.name, gameInfo.host.name, gameInfo.host.publicIdentity);
	nameWidg->setTransparentToClicks(true);
	columnWidgets.push_back(nameWidg);

	auto mapWidg = LobbyGameMapWidget::make(gameInfo.map.name, alliancesModeFromLobbyGameDetails(gameInfo.alliancesMode), blindModeFromLobbyGameDetails(gameInfo.blindMode).value_or(BLIND_MODE::NONE));
	mapWidg->setTransparentToClicks(true);
	columnWidgets.push_back(mapWidg);

	auto modsWidg = LobbyGameModsWidget::make(
		gameInfo.mods,
		gameInfo.limits & static_cast<uint8_t>(netlobby::StructureLimits::NO_VTOLS),
		gameInfo.limits & static_cast<uint8_t>(netlobby::StructureLimits::NO_TANKS),
		gameInfo.limits & static_cast<uint8_t>(netlobby::StructureLimits::NO_BORGS)
	);
	modsWidg->setTransparentToClicks(true);
	columnWidgets.push_back(modsWidg);

	auto weakSelf = std::weak_ptr<LobbyBrowser>(std::static_pointer_cast<LobbyBrowser>(shared_from_this()));

	LobbyActionWidgets::OnSpectateHandlerFunc onSpectateFunc = nullptr;
	if (gameInfo.spectators.availableSlots() > 0)
	{
		onSpectateFunc = [weakSelf, idx]() {
			auto strongSelf = weakSelf.lock();
			ASSERT_OR_RETURN(, strongSelf != nullptr, "Parent no longer exists?");
			strongSelf->joinLobbyGame(idx, true);
		};
	}

	auto actionsWidg = LobbyActionWidgets::make(onSpectateFunc);
	columnWidgets.push_back(actionsWidg);

	return columnWidgets;
}

LobbyBrowser::~LobbyBrowser()
{
	if (currentPopoverMenu)
	{
		currentPopoverMenu->closeMenu();
	}
}

void LobbyBrowser::initialize(const std::function<void()>& onBackButtonFunc)
{
	auto weakSelf = std::weak_ptr<LobbyBrowser>(std::static_pointer_cast<LobbyBrowser>(shared_from_this()));

	filterIPv6Only = war_getLobbyFilterIPv6Only();

	backButton = makeBackButton();
	if (onBackButtonFunc)
	{
		backButton->addOnClickHandler([onBackButtonFunc](W_BUTTON& button) {
			widgScheduleTask([onBackButtonFunc]() {
				onBackButtonFunc();
			});
		});
	}
	attach(backButton);

	connectToForm = ConnectToForm::make([](WzString inputStr, bool asSpectator) {
		if (inputStr.isEmpty())
		{
			return;
		}
		auto result = joinGameFromConnectionStr(inputStr.toUtf8(), asSpectator);
		if (result.has_value() && result.value() == JoinGameFromConnectionStrOutcome::Attempting_IP_Join)
		{
			// Persist the input IP address
			war_setLastIpServerConnect(inputStr.toUtf8());
		}
	});
	connectToForm->setGeometry(0, 0, connectToForm->idealWidth(), connectToForm->idealHeight());
	attach(connectToForm);

	filtersButton = WzFrontendImageButton::make(IMAGE_GEAR);
	filtersButton->setString(_("Filters"));
	filtersButton->setPadding(7, 5);
	filtersButton->setBackgroundColor(WZCOL_TRANSPARENT_BOX);
	filtersButton->setGeometry(0, 0, filtersButton->idealWidth(), filtersButton->idealHeight());
	filtersButton->addOnClickHandler([weakSelf](W_BUTTON& but) {
		auto strongSelf = weakSelf.lock();
		ASSERT_OR_RETURN(, strongSelf != nullptr, "No parent?");
		strongSelf->displayFiltersOverlay(but.shared_from_this());
	});
	attach(filtersButton);

	refreshButton = WzFrontendImageButton::make(IMAGE_ARROW_RELOAD);
	refreshButton->setString(_("Refresh"));
	refreshButton->setPadding(6, 5);
	refreshButton->setBackgroundColor(WZCOL_TRANSPARENT_BOX);
	refreshButton->setGeometry(0, 0, refreshButton->idealWidth(), refreshButton->idealHeight());
	refreshButton->addOnClickHandler([weakSelf](W_BUTTON& but) {
		auto strongSelf = weakSelf.lock();
		ASSERT_OR_RETURN(, strongSelf != nullptr, "No parent?");
		strongSelf->triggerAsyncGameListFetch();
	});
	attach(refreshButton);

	// Create scrolling table
	table = createGamesList();
	attach(table);

	// Create progress widget
	lobbyStatusOverlayWidg = LobbyStatusOverlayWidget::make([weakSelf]() {
		// on reset lobby address button click
		NETsetLobbyserverAddress(netlobby::GetDefaultLobbyAddress());
		// trigger lobby re-fetch
		auto strongSelf = weakSelf.lock();
		ASSERT_OR_RETURN(, strongSelf != nullptr, "No parent?");
		strongSelf->triggerAsyncGameListFetch();
	});
	attach(lobbyStatusOverlayWidg, ChildZPos::Front);

	lobbyStatusMessageContainer = ScrollableListWidget::make();
	lobbyStatusMessageContainer->setPadding({15,15,15,15});
	attach(lobbyStatusMessageContainer);
	lobbyStatusMessageContainer->setGeometry(0, 0, 100, (iV_GetTextLineSize(font_regular) * 3) + 30);

	if (!NET_getLobbyDisabled())
	{
		// Start up background thread to gather lobby listing
		triggerAsyncGameListFetch();
	}
	else
	{
		lobbyStatusOverlayWidg->setString(_("There appears to be a game update available!"));
		lobbyStatusOverlayWidg->show();
		recalcLobbyStatusOverlayLayout();

		displayLobbyDisabledNotification();
	}
}

void LobbyBrowser::recalcLobbyStatusOverlayLayout()
{
	int w = width();

	int tableHeight = table->height();

	int lobbyStatusOverlayHeight = lobbyStatusOverlayWidg->idealHeight();
	int lobbyStatusOverlayY0 = (tableHeight - lobbyStatusOverlayHeight) / 2;
	lobbyStatusOverlayWidg->setGeometry(0, lobbyStatusOverlayY0, w, lobbyStatusOverlayHeight);
}

void LobbyBrowser::geometryChanged()
{
	int w = width();
	int h = height();

	int topButtonsY0 = 10;

	// left-aligned buttons

	int backButtonX0 = 10;
	backButton->setGeometry(backButtonX0, topButtonsY0, backButton->width(), backButton->height());

	// right-aligned buttons

	int buttonMaxHeight = std::max<int>(refreshButton->idealHeight(), filtersButton->idealHeight());

	int refreshButtonX0 = w - 10 - refreshButton->idealWidth();
	refreshButton->setGeometry(refreshButtonX0, topButtonsY0, refreshButton->idealWidth(), refreshButton->idealHeight());

	int filtersButtonX0 = refreshButton->x() - 5 - filtersButton->idealWidth();
	filtersButton->setGeometry(filtersButtonX0, topButtonsY0, filtersButton->idealWidth(), filtersButton->idealHeight());

	// connect-to form

	int connectWidgetX0 = backButton->x() + backButton->width() + connectToFormHorizontalPadding;
	int maxAvailableConnectToFormWidget = filtersButton->x() - connectWidgetX0 - connectToFormHorizontalPadding;
	connectToForm->setGeometry(connectWidgetX0, topButtonsY0, std::min<int>(connectToForm->idealWidth(), maxAvailableConnectToFormWidget), buttonMaxHeight);

	// top area height

	int maxTopAreaY1 = std::max<int>({backButton->y() + backButton->height(), connectToForm->y() + connectToForm->height(), refreshButton->y() + refreshButton->height(), filtersButton->y() + filtersButton->height()});

	// table

	int tablePosY0 = maxTopAreaY1 + 10;
	int tableHeight = h - tablePosY0 - lobbyStatusMessageContainer->height();
	table->setGeometry(0, tablePosY0, w, tableHeight);

	recalcLobbyStatusOverlayLayout(); // after modifying table layout + height

	// recalc column widths
	table->setMinimumColumnWidths(minimumColumnWidths);

	int lobbyStatusContainerY0 = h - lobbyStatusMessageContainer->height();
	lobbyStatusMessageContainer->setGeometry(0, lobbyStatusContainerY0, w, lobbyStatusMessageContainer->height());
}

void LobbyBrowser::display(int xOffset, int yOffset)
{
	int x0 = x() + xOffset;
	int y0 = y() + yOffset;

	// Draw background behind table
	int tableX0 = x0 + table->x();
	int tableY0 = y0 + table->y();
	int tableX1 = tableX0 + table->width();
	int tableY1 = tableY0 + table->height();
	pie_UniTransBoxFill(tableX0, tableY0, tableX1, tableY1, pal_RGBA(150,150,150,50));
	pie_UniTransBoxFill(tableX0, tableY0, tableX1, tableY1, pal_RGBA(0,0,0,70));
	pie_UniTransBoxFill(tableX0, tableY0, tableX1, tableY1, WZCOL_TRANSPARENT_BOX);

	// Draw darker area beneath where the lobby status message container is
	int lobbyStatusMessageContainerY0 = lobbyStatusMessageContainer->y();
	int lobbyStatusMessageContainerY1 = lobbyStatusMessageContainerY0 + lobbyStatusMessageContainer->height();
	pie_UniTransBoxFill(x0, y0 + lobbyStatusMessageContainerY0, x0 + width(), y0 + lobbyStatusMessageContainerY1, WZCOL_TRANSPARENT_BOX);
	pie_UniTransBoxFill(x0, y0 + lobbyStatusMessageContainerY0, x0 + width(), y0 + lobbyStatusMessageContainerY1, pal_RGBA(0,0,0,128));
}

int32_t LobbyBrowser::idealWidth()
{
	return std::max(750, minDesiredTableWidth);
}

int32_t LobbyBrowser::idealHeight()
{
	return 700;
}

#define WZLOBBY_TABLE_COL_PADDING_X 5
#define WZLOBBY_TABLE_ROW_SPACING 2

std::shared_ptr<W_BUTTON> LobbyBrowser::makeBackButton()
{
	auto psBackButton = std::make_shared<WzMultiButton>();
	psBackButton->setGeometry(0, 0, 30, 29);
	psBackButton->setTip(P_("menu", "Return"));
	psBackButton->imNormal = AtlasImage(FrontImages, IMAGE_RETURN);
	psBackButton->imDown = AtlasImage(FrontImages, IMAGE_RETURN_HI);
	psBackButton->doHighlight = IMAGE_RETURN_HI;
	psBackButton->tc = MAX_PLAYERS;
	psBackButton->alpha = 255;
	return psBackButton;
}

std::shared_ptr<WIDGET> LobbyBrowser::createColumnHeaderLabel(const char* text, WzTextAlignment align, int minLeftPadding)
{
	auto label = std::make_shared<W_LABEL>();
	label->setTextAlignment(align);
	label->setFontColour(WZCOL_TEXT_MEDIUM);
	label->setString(text);
	label->setGeometry(0, 0, label->getMaxLineWidth(), label->idealHeight());
	label->setCacheNeverExpires(true);
	label->setCanTruncate(true);
	return Margin(0, 0, 0, minLeftPadding).wrap(label);
}

std::shared_ptr<ScrollableTableWidget> LobbyBrowser::createGamesList()
{
	// Create column headers for games listing
	std::vector<TableColumn> columns;
	columns.push_back({createColumnHeaderLabel("", WLAB_ALIGNLEFT, 0), TableColumn::ResizeBehavior::FIXED_WIDTH});
	columns.push_back({createColumnHeaderLabel("", WLAB_ALIGNCENTRE, 0), TableColumn::ResizeBehavior::FIXED_WIDTH});
	columns.push_back({createColumnHeaderLabel(_("Name"), WLAB_ALIGNLEFT, 0), TableColumn::ResizeBehavior::RESIZABLE_AUTOEXPAND});
	columns.push_back({createColumnHeaderLabel(_("Details"), WLAB_ALIGNLEFT, 0), TableColumn::ResizeBehavior::RESIZABLE_AUTOEXPAND});
	columns.push_back({createColumnHeaderLabel(_("Mods"), WLAB_ALIGNLEFT, 0), TableColumn::ResizeBehavior::RESIZABLE_AUTOEXPAND});
	columns.push_back({createColumnHeaderLabel("", WLAB_ALIGNLEFT, 0), TableColumn::ResizeBehavior::FIXED_WIDTH});

	int maxIdealColumnHeight = iV_GetTextLineSize(font_regular) + 14;
	for (const auto& column : columns)
	{
		maxIdealColumnHeight = std::max(maxIdealColumnHeight, column.columnWidget->idealHeight());
	}
	minimumColumnWidths = { 16 + (WZLOBBY_WIDG_PADDING_X * 2), 40, 150, 100, 60, 30 };
	minDesiredTableWidth = std::accumulate(minimumColumnWidths.begin(), minimumColumnWidths.end(), 0);
	minDesiredTableWidth += (WZLOBBY_TABLE_COL_PADDING_X * 2) + (WZLOBBY_TABLE_COL_PADDING_X * 2 * (int)minimumColumnWidths.size());

	// Create table
	table = ScrollableTableWidget::make(columns, maxIdealColumnHeight);
	table->setColumnPadding(Vector2i(WZLOBBY_TABLE_COL_PADDING_X, 0));
	table->setItemSpacing(WZLOBBY_TABLE_ROW_SPACING);
	table->setBackgroundColor(pal_RGBA(0,0,0,0));
	table->setMinimumColumnWidths(minimumColumnWidths);
	table->setDrawColumnLines(false);

	return table;
}

static std::shared_ptr<WzAdvCheckbox> addPopoverMenuOptionsCheckbox(const std::shared_ptr<PopoverMenuWidget>& popoverMenu, const WzString& text, bool isChecked, bool isDisabled, const std::function<void (WzAdvCheckbox& button)>& onClickFunc)
{
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
}

std::shared_ptr<PopoverMenuWidget> LobbyBrowser::createFiltersPopoverForm()
{
	auto popoverMenu = PopoverMenuWidget::make();

	auto addOptionsCheckbox = [&popoverMenu](const WzString& text, bool isChecked, bool isDisabled, const std::function<void (WzAdvCheckbox& button)>& onClickFunc) -> std::shared_ptr<WzAdvCheckbox> {
		return addPopoverMenuOptionsCheckbox(popoverMenu, text, isChecked, isDisabled, onClickFunc);
	};

	auto weakSelf = std::weak_ptr<LobbyBrowser>(std::static_pointer_cast<LobbyBrowser>(shared_from_this()));

	addOptionsCheckbox(_("Hide Incompatible Games"), filterIncompatible, false, [weakSelf](WzAdvCheckbox& button){
		if (auto strongSelf = weakSelf.lock())
		{
			strongSelf->filterIncompatible = button.isChecked();
			strongSelf->populateTableFromGameList();
		}
	});
	addOptionsCheckbox(_("Hide Empty Games"), filterEmpty, false, [weakSelf](WzAdvCheckbox& button){
		if (auto strongSelf = weakSelf.lock())
		{
			strongSelf->filterEmpty = button.isChecked();
			strongSelf->populateTableFromGameList();
		}
	});
	addOptionsCheckbox(_("Hide Modded Games"), filterModded, false, [weakSelf](WzAdvCheckbox& button){
		if (auto strongSelf = weakSelf.lock())
		{
			strongSelf->filterModded = button.isChecked();
			strongSelf->populateTableFromGameList();
		}
	});
	addOptionsCheckbox(_("Hide IPv6-Only"), filterIPv6Only, false, [weakSelf](WzAdvCheckbox& button){
		if (auto strongSelf = weakSelf.lock())
		{
			bool newValue = button.isChecked();
			strongSelf->filterIPv6Only = newValue;
			war_setLobbyFilterIPv6Only(newValue);
			strongSelf->populateTableFromGameList();
		}
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

void LobbyBrowser::displayFiltersOverlay(const std::shared_ptr<WIDGET>& psParent)
{
	if (currentPopoverMenu)
	{
		currentPopoverMenu->closeMenu();
	}

	filtersButton->setState(WBUT_CLICKLOCK);
	currentPopoverMenu = createFiltersPopoverForm();
	auto psWeakSelf = std::weak_ptr<LobbyBrowser>(std::static_pointer_cast<LobbyBrowser>(shared_from_this()));
	currentPopoverMenu->openMenu(psParent, PopoverWidget::Alignment::LeftOfParent, Vector2i(0, 1), [psWeakSelf](){
		if (auto strongSelf = psWeakSelf.lock())
		{
			strongSelf->filtersButton->setState(0);
		}
	});
}

// MARK: - Public methods

std::shared_ptr<WIDGET> makeLobbyBrowser(const std::function<void()>& onBackButtonFunc)
{
	return LobbyBrowser::make(onBackButtonFunc);
}

bool refreshLobbyBrowser(const std::shared_ptr<WIDGET>& lobbyBrowserWidg)
{
	auto lobbyBrowser = std::dynamic_pointer_cast<LobbyBrowser>(lobbyBrowserWidg);
	ASSERT_OR_RETURN(false, lobbyBrowser != nullptr, "Not a LobbyBrowser");
	lobbyBrowser->triggerRefresh();
	return true;
}
