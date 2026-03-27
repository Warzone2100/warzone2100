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

#include <numeric>
#include <vector>
#include <optional>
#include <unordered_set>

// MARK: - Lobby Status Overlay Widget

class LobbyStatusOverlayWidget : public WIDGET
{
protected:
	void initialize();
	virtual void geometryChanged() override;
	virtual void display(int xOffset, int yOffset) override;
public:
	static std::shared_ptr<LobbyStatusOverlayWidget> make()
	{
		class make_shared_enabler: public LobbyStatusOverlayWidget {};
		auto widget = std::make_shared<make_shared_enabler>();
		widget->initialize();
		return widget;
	}
	void showIndeterminateIndicator(bool show);
	virtual void setString(WzString string) override;
	virtual int32_t idealWidth() override;
	virtual int32_t idealHeight() override;
private:
	void recalcLayout();
private:
	std::shared_ptr<W_LABEL> textLabel;
	std::shared_ptr<WIDGET> indeterminateIndicator;
};

void LobbyStatusOverlayWidget::initialize()
{
	textLabel = std::make_shared<W_LABEL>();
	textLabel->setFont(font_medium_bold, WZCOL_TEXT_MEDIUM);
	textLabel->setTextAlignment(WLAB_ALIGNTOP);
	textLabel->setCanTruncate(true);
	textLabel->setTransparentToClicks(true);
	attach(textLabel);

	indeterminateIndicator = createJoiningIndeterminateProgressWidget(font_medium_bold);
	indeterminateIndicator->show(false);
	attach(indeterminateIndicator);
}

int32_t LobbyStatusOverlayWidget::idealWidth()
{
	return std::max<int32_t>(textLabel->idealWidth(), indeterminateIndicator->idealWidth());
}

int32_t LobbyStatusOverlayWidget::idealHeight()
{
	return iV_GetTextLineSize(font_medium_bold) + indeterminateIndicator->idealHeight();
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
		nextWidgY0 = textLabel->height();
	}

	int indicatorX0 = (w - indeterminateIndicator->idealWidth()) / 2;
	indeterminateIndicator->setGeometry(indicatorX0, nextWidgY0, indeterminateIndicator->idealWidth(), indeterminateIndicator->idealHeight());
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
	indeterminateIndicator->show(show);
}

void LobbyStatusOverlayWidget::setString(WzString string)
{
	textLabel->setString(string);
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
	void initialize(const WzString& roomName, const WzString& hostName);
	virtual void geometryChanged() override;
public:
	static std::shared_ptr<LobbyGameNameWidget> make(const WzString& roomName, const WzString& hostName)
	{
		class make_shared_enabler: public LobbyGameNameWidget {};
		auto widget = std::make_shared<make_shared_enabler>();
		widget->initialize(roomName, hostName);
		return widget;
	}
	void updateValue(const WzString& roomName, const WzString& hostName);
	virtual int32_t idealWidth() override;
	virtual int32_t idealHeight() override;
private:
	std::shared_ptr<W_LABEL> roomNameWidg;
	std::shared_ptr<W_LABEL> hostNameWidg;
};

void LobbyGameNameWidget::initialize(const WzString& roomName, const WzString& hostName)
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

private:
	std::shared_ptr<W_BUTTON> makeBackButton();
	std::shared_ptr<WIDGET> createColumnHeaderLabel(const char* text, WzTextAlignment align, int minLeftPadding);
	std::shared_ptr<ScrollableTableWidget> createGamesList();
	void triggerAsyncGameListFetch();
	void processAsyncGameListFetchResults(netlobby::ListResult&& results);
	void populateTableFromGameList();
	std::vector<std::shared_ptr<WIDGET>> createLobbyGameRowColumnWidgets(size_t idx, const netlobby::GameListing& listing);

	std::shared_ptr<PopoverMenuWidget> createFiltersPopoverForm();
	void displayFiltersOverlay(const std::shared_ptr<WIDGET>& psParent);

private:
	std::shared_ptr<W_BUTTON> backButton;
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
		lobbyStatusOverlayWidg->show();
		return;
	}

	refreshButton->setState(WBUT_DISABLE);
	lobbyStatusOverlayWidg->showIndeterminateIndicator(true);
	lobbyStatusOverlayWidg->setString("");
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

	startLobbyJoiningAttempt(sPlayer, NETgetLobbyserverAddress(), currentResults[idx].gameId, asSpectator, expectedHostProps);
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

		table->clearRows();
		lobbyStatusMessageContainer->clear();
		currentResults.clear();
		return;
	}

	table->clearRows();
	lobbyStatusMessageContainer->clear();

	currentResults = std::move(results.value().games);
	populateTableFromGameList();

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

void LobbyBrowser::populateTableFromGameList()
{
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

	auto nameWidg = LobbyGameNameWidget::make(gameInfo.name, gameInfo.host.name);
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

	filtersButton = WzFrontendImageButton::make(IMAGE_GEAR);
	filtersButton->setString(_("Filters"));
	filtersButton->setPadding(6, 5);
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
	lobbyStatusOverlayWidg = LobbyStatusOverlayWidget::make();
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

		displayLobbyDisabledNotification();
	}
}

void LobbyBrowser::geometryChanged()
{
	int w = width();
	int h = height();

	int topButtonsY0 = 10;

	int backButtonX0 = 10;
	backButton->setGeometry(backButtonX0, topButtonsY0, backButton->width(), backButton->height());

	int refreshButtonX0 = w - 10 - refreshButton->idealWidth();
	refreshButton->setGeometry(refreshButtonX0, topButtonsY0, refreshButton->idealWidth(), refreshButton->idealHeight());

	int filtersButtonX0 = refreshButton->x() - 5 - filtersButton->idealWidth();
	filtersButton->setGeometry(filtersButtonX0, topButtonsY0, filtersButton->idealWidth(), filtersButton->idealHeight());

	int maxTopAreaY1 = std::max<int>({backButton->y() + backButton->height(), refreshButton->y() + refreshButton->height(), filtersButton->y() + filtersButton->height()});

	int tablePosY0 = maxTopAreaY1 + 10;
	int tableHeight = h - tablePosY0 - lobbyStatusMessageContainer->height();
	table->setGeometry(0, tablePosY0, w, tableHeight);

	int lobbyStatusOverlayHeight = lobbyStatusOverlayWidg->idealHeight();
	int lobbyStatusOverlayY0 = (tableHeight - lobbyStatusOverlayHeight) / 2;
	lobbyStatusOverlayWidg->setGeometry(0, lobbyStatusOverlayY0, w, lobbyStatusOverlayHeight);

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

std::shared_ptr<PopoverMenuWidget> LobbyBrowser::createFiltersPopoverForm()
{
	auto popoverMenu = PopoverMenuWidget::make();

	auto addOptionsCheckbox = [&popoverMenu](const WzString& text, bool isChecked, bool isDisabled, const std::function<void (WzAdvCheckbox& button)>& onClickFunc) -> std::shared_ptr<WzAdvCheckbox> {
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
