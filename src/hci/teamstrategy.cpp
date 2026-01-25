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
#include "teamstrategy.h"
#include "lib/widget/widgint.h"
#include "lib/widget/label.h"
#include "lib/widget/table.h"
#include "lib/widget/margin.h"
#include "lib/widget/scrollablelist.h"
#include "lib/widget/button.h"
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

#include <array>
#include <chrono>
#include <deque>

// MARK: - Globals and helper functions

constexpr int COL_PADDING_X = 2;
constexpr int MAX_TEAMSTRAT_COLUMN_HEADERS_HEIGHT = 120;

bool to_WzStrategyPlanningState(uint8_t value, WzStrategyPlanningState& output)
{
#if defined(__GNUC__)
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wtype-limits"
#endif
	if (value >= static_cast<uint8_t>(WzStrategyPlanningState::NotSpecified) && value <= static_cast<uint8_t>(WzStrategyPlanningState::Never))
	{
		output = static_cast<WzStrategyPlanningState>(value);
		return true;
	}
#if defined(__GNUC__)
# pragma GCC diagnostic pop
#endif
	return false;
}

WzStrategyPlanningState cycleWzStrategyPlanningState(WzStrategyPlanningState state, bool directionForward = true)
{
	switch (state)
	{
		case WzStrategyPlanningState::Never:
			return (directionForward) ? WzStrategyPlanningState::NotSpecified : WzStrategyPlanningState::HeavilyPlanned;
		case WzStrategyPlanningState::NotSpecified:
			return (directionForward) ? WzStrategyPlanningState::Planned : WzStrategyPlanningState::Never;
		case WzStrategyPlanningState::Planned:
			return (directionForward) ? WzStrategyPlanningState::HeavilyPlanned : WzStrategyPlanningState::NotSpecified;
		case WzStrategyPlanningState::HeavilyPlanned:
			return (directionForward) ? WzStrategyPlanningState::Never : WzStrategyPlanningState::Planned;
	}
	return WzStrategyPlanningState::NotSpecified; // silence warning
}

const char* getUnitTypeDisplayName(WzStrategyPlanningUnitTypes unitType)
{
	switch (unitType)
	{
		case WzStrategyPlanningUnitTypes::Tanks:
			return _("Tanks");
		case WzStrategyPlanningUnitTypes::VTOL:
			return _("VTOL");
		case WzStrategyPlanningUnitTypes::Cyborg:
			return _("Cyborgs");
	}
	return ""; // silence compiler warning
}

struct WzStrategyPlanningDetails
{
	WzStrategyPlanningState state;
	bool localPlayerHasSeen = true;

	void reset()
	{
		state = WzStrategyPlanningState::NotSpecified;
		localPlayerHasSeen = true;
	}
};

struct WzPlayerStrategyPlan
{
	typedef std::array<WzStrategyPlanningDetails, WSC_NUM_WEAPON_SUBCLASSES> WeaponPlans;
	typedef std::array<WzStrategyPlanningDetails, WzStrategyPlanningUnitTypes_NUM_TYPES> UnitPlans;

	WeaponPlans weaponPlans;
	UnitPlans unitPlans;

	bool hasBeenSet = false;

	void reset();
};

std::array<WzPlayerStrategyPlan, MAX_PLAYERS> playerStrategyPlans; // NOTE: Only filled out for teammates, as only teammates send their information

// MARK: -

int32_t checkedGetPlayerTeam(int32_t i)
{
	return alliancesSetTeamsBeforeGame(game.alliance) ? NetPlay.players[i].team : i;
}

void sendStrategyPlanUpdate(uint32_t forPlayer)
{
	ASSERT_OR_RETURN(, forPlayer < playerStrategyPlans.size(), "Unexpected player: %" PRIu32, forPlayer);
	ASSERT_OR_RETURN(, myResponsibility(forPlayer), "We are not responsible for player: %" PRIu32, forPlayer);
	WzPlayerStrategyPlan& currentPlayerPlan = playerStrategyPlans[forPlayer];

	// build parts of the messages
	std::vector<uint8_t> weaponStates;
	weaponStates.reserve(currentPlayerPlan.weaponPlans.size());
	for (const auto& weaponPlan : currentPlayerPlan.weaponPlans)
	{
		weaponStates.push_back(static_cast<uint8_t>(weaponPlan.state));
	}
	std::vector<uint8_t> unitStates;
	unitStates.reserve(currentPlayerPlan.unitPlans.size());
	for (const auto& unitPlan : currentPlayerPlan.unitPlans)
	{
		unitStates.push_back(static_cast<uint8_t>(unitPlan.state));
	}

	// send updates to every (human) team member
	auto sendingPlayerTeam = checkedGetPlayerTeam(forPlayer);
	for (int32_t player = 0; player < std::min<int32_t>(game.maxPlayers, MAX_PLAYERS); ++player)
	{
		if (player == forPlayer)
		{
			continue;
		}
		if (checkedGetPlayerTeam(player) != sendingPlayerTeam)
		{
			continue;
		}
		// Only send to active (connected) human players - *NOT* AIs (which would effectively just have to send this to the host)
		if (isHumanPlayer(player))
		{
			auto w = NETbeginEncodeSecured(NETnetQueue(player), NET_TEAM_STRATEGY);
			if (w)
			{
				auto& wref = *w;
				uint32_t sender = forPlayer;
				NETuint32_t(wref, sender);
				NETbytes(wref, weaponStates);
				NETbytes(wref, unitStates);
				NETend(wref);
			}
		}
	}
}

bool recvStrategyPlanUpdate(NETQUEUE queue)
{
	uint32_t sender = MAX_CONNECTED_PLAYERS;
	std::vector<uint8_t> weaponStates;
	std::vector<uint8_t> unitStates;

	auto r = NETbeginDecodeSecured(queue, NET_TEAM_STRATEGY);
	if (!r)
	{
		return false;
	}
	auto& rref = *r;
	NETuint32_t(rref, sender);
	NETbytes(rref, weaponStates);
	NETbytes(rref, unitStates);
	NETend(rref);

	if (whosResponsible(sender) != queue.index)
	{
		return false;
	}

	if (sender >= playerStrategyPlans.size() || !NetPlay.players[sender].allocated)
	{
		return false;
	}

	auto selectedPlayerTeam = checkedGetPlayerTeam(selectedPlayer);
	if (checkedGetPlayerTeam(sender) != selectedPlayerTeam)
	{
		return false;
	}

	// update any changed data
	WzPlayerStrategyPlan& targetPlayerPlan = playerStrategyPlans[sender];

	for (size_t i = 0; i < targetPlayerPlan.weaponPlans.size() && i < weaponStates.size(); ++i)
	{
		WzStrategyPlanningState newState;
		if (!to_WzStrategyPlanningState(weaponStates[i], newState))
		{
			continue;
		}
		if (newState != targetPlayerPlan.weaponPlans[i].state)
		{
			targetPlayerPlan.weaponPlans[i].state = newState;
			targetPlayerPlan.weaponPlans[i].localPlayerHasSeen = false;
		}
	}

	for (size_t i = 0; i < targetPlayerPlan.unitPlans.size() && i < unitStates.size(); ++i)
	{
		WzStrategyPlanningState newState;
		if (!to_WzStrategyPlanningState(unitStates[i], newState))
		{
			continue;
		}
		if (newState != targetPlayerPlan.unitPlans[i].state)
		{
			targetPlayerPlan.unitPlans[i].state = newState;
			targetPlayerPlan.unitPlans[i].localPlayerHasSeen = false;
		}
	}

	targetPlayerPlan.hasBeenSet = true;

	return true;
}

void strategyPlansInit()
{
	for (auto& playerStrategy : playerStrategyPlans)
	{
		playerStrategy.reset();
	}
}

void WzPlayerStrategyPlan::reset()
{
	for (size_t i = 0; i < weaponPlans.size(); ++i)
	{
		weaponPlans[i].reset();
	}
	for (size_t i = 0; i < unitPlans.size(); ++i)
	{
		unitPlans[i].reset();
	}
	hasBeenSet = false;
}

// MARK: -

class WzTeamStrategyColumnImagesManager;

class WzTeamStrategyColumnImageRef
{
public:
	WzTeamStrategyColumnImageRef(WEAPON_SUBCLASS subClass, const std::shared_ptr<WzTeamStrategyColumnImagesManager>& sharedImageManager)
	: weaponSubclass(subClass)
	, sharedImageManager(sharedImageManager)
	{ }

	WzTeamStrategyColumnImageRef(WzStrategyPlanningUnitTypes unitType, const std::shared_ptr<WzTeamStrategyColumnImagesManager>& sharedImageManager)
	: unitType(unitType)
	, sharedImageManager(sharedImageManager)
	{ }

public:
	gfx_api::texture* getImageTexture();

private:
	optional<WEAPON_SUBCLASS> weaponSubclass;
	optional<WzStrategyPlanningUnitTypes> unitType;
	std::shared_ptr<WzTeamStrategyColumnImagesManager> sharedImageManager;
};

class WzTeamStrategyColumnImagesManager : public std::enable_shared_from_this<WzTeamStrategyColumnImagesManager>
{
public:
	~WzTeamStrategyColumnImagesManager();
public:
	WzTeamStrategyColumnImageRef getWeaponSubclassImageRef(WEAPON_SUBCLASS subClass);
	WzTeamStrategyColumnImageRef getUnitTypeImageRef(WzStrategyPlanningUnitTypes unitType);
public:
	gfx_api::texture* getWeaponSubclassImageTexture(WEAPON_SUBCLASS subClass);
	gfx_api::texture* getUnitTypeImageTexture(WzStrategyPlanningUnitTypes unitType);
	void renderNeverSymbol(int x, int y, int width, int height);
private:
	std::vector<gfx_api::texture*> loadedWeaponSubclassImages = std::vector<gfx_api::texture*>(WSC_NUM_WEAPON_SUBCLASSES, nullptr);
	std::vector<gfx_api::texture*> loadedUnitTypeImages = std::vector<gfx_api::texture*>(WzStrategyPlanningUnitTypes_NUM_TYPES, nullptr);
	WzText neverSymbol;
};

class TeamStrategyView: public W_FORM
{
protected:
	TeamStrategyView();
	virtual ~TeamStrategyView();

public:
	static std::shared_ptr<TeamStrategyView> make(bool allowAIs);
	void setUpdatingPlayers(bool enabled);
	bool getUpdatingPlayer() { return updatingPlayers; }
	std::shared_ptr<WzTeamStrategyColumnImagesManager> getSharedColumnImagesManager() const { return columnImagesManager; }
	void setBackgroundColor(PIELIGHT color);

public:
	virtual void display(int xOffset, int yOffset) override;
	virtual void geometryChanged() override;
	virtual int32_t idealWidth() override;
	virtual int32_t idealHeight() override;
private:
	std::shared_ptr<TableRow> newPlayerStrategyRow(uint32_t playerIdx, const std::vector<WEAPON_SUBCLASS>& displayedWeaponSubclasses, const std::unordered_set<WzStrategyPlanningUnitTypes>& disabledUnitTypes);
	void resizeTableColumnWidths();
	void updateNoTeamLabelContents();
	std::pair<std::vector<size_t>, size_t> getMaxTableColumnDataWidths();
private:
	std::shared_ptr<W_LABEL> description;
	std::shared_ptr<W_LABEL> instructions;
	std::shared_ptr<ScrollableTableWidget> table;
	std::shared_ptr<WzTeamStrategyColumnImagesManager> columnImagesManager;
	std::shared_ptr<W_LABEL> noTeammatesLabel;
	size_t playerNameColIdx = 0;
	bool updatingPlayers = false;
	PIELIGHT backgroundColor = pal_RGBA(0,0,0,0);
};

WzTeamStrategyColumnImagesManager::~WzTeamStrategyColumnImagesManager()
{
	for (auto pTexture : loadedWeaponSubclassImages)
	{
		if (pTexture)
		{
			delete pTexture;
		}
	}
	loadedWeaponSubclassImages.clear();

	for (auto pTexture : loadedUnitTypeImages)
	{
		if (pTexture)
		{
			delete pTexture;
		}
	}
	loadedUnitTypeImages.clear();
}

gfx_api::texture* WzTeamStrategyColumnImageRef::getImageTexture()
{
	if (weaponSubclass.has_value())
	{
		return sharedImageManager->getWeaponSubclassImageTexture(weaponSubclass.value());
	}
	else if (unitType.has_value())
	{
		return sharedImageManager->getUnitTypeImageTexture(unitType.value());
	}
	debug(LOG_ERROR, "Invalid reference");
	return nullptr;
}

WzTeamStrategyColumnImageRef WzTeamStrategyColumnImagesManager::getWeaponSubclassImageRef(WEAPON_SUBCLASS subClass)
{
	return WzTeamStrategyColumnImageRef(subClass, shared_from_this());
}

WzTeamStrategyColumnImageRef WzTeamStrategyColumnImagesManager::getUnitTypeImageRef(WzStrategyPlanningUnitTypes unitType)
{
	return WzTeamStrategyColumnImageRef(unitType, shared_from_this());
}

gfx_api::texture* WzTeamStrategyColumnImagesManager::getWeaponSubclassImageTexture(WEAPON_SUBCLASS subClass)
{
	if (!loadedWeaponSubclassImages[subClass])
	{
		// load image for the first time
		auto pLoadedImage = loadImageForWeapSubclass(subClass);
		loadedWeaponSubclassImages[subClass] = pLoadedImage;
	}
	return loadedWeaponSubclassImages[subClass];
}

static gfx_api::texture* loadImageForUnitType(WzStrategyPlanningUnitTypes unitType)
{
	const char* imagePath = nullptr;
	switch (unitType)
	{
		case WzStrategyPlanningUnitTypes::Tanks:
			imagePath = "images/intfac/unittype_tank.png";
			break;
		case WzStrategyPlanningUnitTypes::VTOL:
			imagePath = "images/intfac/unittype_vtol.png";
			break;
		case WzStrategyPlanningUnitTypes::Cyborg:
			imagePath = "images/intfac/unittype_cyborg.png";
			break;
	}
	ASSERT_OR_RETURN(nullptr, imagePath != nullptr, "No image path");
	WZ_Notification_Image img(imagePath);
	return img.loadImageToTexture();
}

gfx_api::texture* WzTeamStrategyColumnImagesManager::getUnitTypeImageTexture(WzStrategyPlanningUnitTypes unitType)
{
	if (!loadedUnitTypeImages[static_cast<uint8_t>(unitType)])
	{
		// load image for the first time
		auto pLoadedImage = loadImageForUnitType(unitType);
		loadedUnitTypeImages[static_cast<uint8_t>(unitType)] = pLoadedImage;
	}
	return loadedUnitTypeImages[static_cast<uint8_t>(unitType)];
}

void WzTeamStrategyColumnImagesManager::renderNeverSymbol(int x, int y, int width, int height)
{
	neverSymbol.setText("x", font_regular_bold);
	int x0 = x + ((width - neverSymbol.width()) / 2);
	int y0 = y + ((height - neverSymbol.lineSize()) / 2);
	neverSymbol.render(x0, y0 - float(neverSymbol.aboveBase()), WZCOL_RED);
}

// MARK: - WzTeamStrategyIconColumnHeader

constexpr int WZ_TEAMSTRAT_COLUMN_HEADER_INTERNAL_PADDING = 4;
constexpr int WZ_TEAMSTRAT_COLUMN_HEADER_PADDING_Y = 4;
constexpr int WZ_TEAMSTRAT_COLUMN_HEADER_PADDING_X = 1;
constexpr int WZ_TEAMSTRAT_COLUMN_HEADER_IMAGE_DISPLAY_SIZE = 16;

class WzTeamStrategyIconColumnHeader: public WIDGET
{
protected:
	WzTeamStrategyIconColumnHeader(const WzTeamStrategyColumnImageRef& columnImageRef);

public:
	static std::shared_ptr<WzTeamStrategyIconColumnHeader> make(const WzString& text, const WzTeamStrategyColumnImageRef& columnImageRef, bool disabled = false);
public:
	virtual void display(int xOffset, int yOffset) override;
	virtual int32_t idealWidth() override;
	virtual int32_t idealHeight() override;
	void setTip(std::string string) override
	{
		pTip = string;
	}
	std::string getTip() override
	{
		return pTip;
	}

private:
	WzText columnText;
	WzTeamStrategyColumnImageRef columnImageRef;
	std::string pTip;
	bool disabledColumn = false;
};

WzTeamStrategyIconColumnHeader::WzTeamStrategyIconColumnHeader(const WzTeamStrategyColumnImageRef& columnImageRef)
: columnImageRef(columnImageRef)
{ }

std::shared_ptr<WzTeamStrategyIconColumnHeader> WzTeamStrategyIconColumnHeader::make(const WzString& text, const WzTeamStrategyColumnImageRef& columnImageRef, bool disabled)
{
	class make_shared_enabler: public WzTeamStrategyIconColumnHeader
	{
	public:
		make_shared_enabler(const WzTeamStrategyColumnImageRef& columnImageRef)
		: WzTeamStrategyIconColumnHeader(columnImageRef)
		{ }
	};
	auto result = std::make_shared<make_shared_enabler>(columnImageRef);

	result->columnText.setText(text, font_regular);
	result->disabledColumn = disabled;

	return result;
}

void WzTeamStrategyIconColumnHeader::display(int xOffset, int yOffset)
{
	int x0 = x() + xOffset;
	int y0 = y() + yOffset;

	// Draw the column image (at the bottom of the widget
	PIELIGHT iconColor = (!disabledColumn) ? pal_RGBA(255, 255, 255, 200) : pal_RGBA(200, 200, 200, 100);
	auto pImageTexture = columnImageRef.getImageTexture();
	if (pImageTexture)
	{
		int imageX0 = x0 + (width() - WZ_TEAMSTRAT_COLUMN_HEADER_IMAGE_DISPLAY_SIZE) / 2;
		int imageY0 = y0 + (height() - (WZ_TEAMSTRAT_COLUMN_HEADER_PADDING_Y + WZ_TEAMSTRAT_COLUMN_HEADER_IMAGE_DISPLAY_SIZE));
		iV_DrawImageAnisotropic(*pImageTexture, Vector2i(imageX0, imageY0), Vector2f(0, 0), Vector2f(WZ_TEAMSTRAT_COLUMN_HEADER_IMAGE_DISPLAY_SIZE, WZ_TEAMSTRAT_COLUMN_HEADER_IMAGE_DISPLAY_SIZE), 0.f, iconColor);
	}
	// Draw the text, sideways
	int textX0 = ((width() - columnText.lineSize()) / 2) + (-columnText.aboveBase());
	int textY0 = height() - (WZ_TEAMSTRAT_COLUMN_HEADER_PADDING_Y + WZ_TEAMSTRAT_COLUMN_HEADER_IMAGE_DISPLAY_SIZE + WZ_TEAMSTRAT_COLUMN_HEADER_INTERNAL_PADDING);
	int maxTextDisplayableWidth = textY0 - WZ_TEAMSTRAT_COLUMN_HEADER_PADDING_Y;
	bool willTruncate = maxTextDisplayableWidth < columnText.width();
	if (willTruncate)
	{
		maxTextDisplayableWidth -= (iV_GetEllipsisWidth(columnText.getFontID()) + 2);
	}
	PIELIGHT textColor = (!disabledColumn) ? WZCOL_TEXT_DARK : WZCOL_GREY;
	columnText.render(x0 + textX0, y0 + textY0, textColor, 270.f, maxTextDisplayableWidth);

	if (willTruncate)
	{
		// Render ellipsis, also sideways
		iV_DrawEllipsis(columnText.getFontID(), Vector2f(x0 + textX0, (y0 + textY0) - (maxTextDisplayableWidth + 2)), textColor, 270.f);
	}
}

int32_t WzTeamStrategyIconColumnHeader::idealWidth()
{
	return WZ_TEAMSTRAT_COLUMN_HEADER_PADDING_X + WZ_TEAMSTRAT_COLUMN_HEADER_IMAGE_DISPLAY_SIZE + WZ_TEAMSTRAT_COLUMN_HEADER_PADDING_X;
}

int32_t WzTeamStrategyIconColumnHeader::idealHeight()
{
	return WZ_TEAMSTRAT_COLUMN_HEADER_PADDING_Y + WZ_TEAMSTRAT_COLUMN_HEADER_IMAGE_DISPLAY_SIZE + WZ_TEAMSTRAT_COLUMN_HEADER_INTERNAL_PADDING + columnText.width() + WZ_TEAMSTRAT_COLUMN_HEADER_PADDING_Y;
}

// MARK: - WzTeamStrategyButton

class WzTeamStrategyButton: public W_BUTTON
{
protected:
	WzTeamStrategyButton(const std::shared_ptr<TeamStrategyView>& _parentStrategyView, const WzTeamStrategyColumnImageRef& imageRef, int displayWidth, int displayHeight)
	: parentStrategyView(_parentStrategyView)
	, sharedImageManager(_parentStrategyView->getSharedColumnImagesManager())
	, imageRef(imageRef)
	, displayWidth(displayWidth)
	, displayHeight(displayHeight)
	{
		style |= WBUT_SECONDARY;
	}
public:
	// special click-handling functions - subclasses should override
	virtual void clickPrimary() {}
	virtual bool clickSecondary(bool synthesizedFromHold) { return false; }

	virtual WzStrategyPlanningState currentState() = 0;
public:
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

	bool clickHeld(W_CONTEXT *psContext, WIDGET_KEY key) override
	 {
		if (key == WKEY_PRIMARY)
		{
			return clickSecondary(true);
		}
		return false;
	 }

	virtual void display(int xOffset, int yOffset) override;
	virtual int32_t idealWidth() override;
	virtual int32_t idealHeight() override;
protected:
	std::weak_ptr<TeamStrategyView> parentStrategyView;
	std::shared_ptr<WzTeamStrategyColumnImagesManager> sharedImageManager;
	WzTeamStrategyColumnImageRef imageRef;
	int displayWidth;
	int displayHeight;
};

int32_t WzTeamStrategyButton::idealWidth()
{
	return WZ_TEAMSTRAT_COLUMN_HEADER_PADDING_X + WZ_TEAMSTRAT_COLUMN_HEADER_IMAGE_DISPLAY_SIZE + WZ_TEAMSTRAT_COLUMN_HEADER_PADDING_X;
}

int32_t WzTeamStrategyButton::idealHeight()
{
	return WZ_TEAMSTRAT_COLUMN_HEADER_PADDING_Y + WZ_TEAMSTRAT_COLUMN_HEADER_IMAGE_DISPLAY_SIZE + WZ_TEAMSTRAT_COLUMN_HEADER_PADDING_Y;
}

void WzTeamStrategyButton::display(int xOffset, int yOffset)
{
	int x0 = x() + xOffset;
	int y0 = y() + yOffset;

	auto strategyState = currentState();

//	bool isDown = (state & (WBUT_DOWN | WBUT_LOCK | WBUT_CLICKLOCK)) != 0;
	bool isDisabled = (state & WBUT_DISABLE) != 0;
	bool isHighlight = (state & WBUT_HIGHLIGHT) != 0;

	if (isHighlight && !isDisabled)
	{
		pie_UniTransBoxFill(x0, y0, x0 + width(), y0 + height(), pal_RGBA(0, 0, 255, 50));
	}

	// An icon / image
	int imageX0 = x0 + (width() - displayWidth) / 2;
	int imageY0 = y0 + (height() - displayHeight) / 2;
	if (strategyState == WzStrategyPlanningState::Planned || strategyState == WzStrategyPlanningState::HeavilyPlanned)
	{
		auto pImageTexture = imageRef.getImageTexture();
		if (pImageTexture)
		{
			iV_DrawImageAnisotropic(*pImageTexture, Vector2i(imageX0, imageY0), Vector2f(0, 0), Vector2f(displayWidth, displayHeight), 0.f, pal_RGBA(255, 255, 255, 200));
		}
	}
	if (strategyState == WzStrategyPlanningState::Never)
	{
		sharedImageManager->renderNeverSymbol(x0, y0, width(), height());
	}
	// (optional) A full inner border (if state == extra selected)
	if (strategyState == WzStrategyPlanningState::HeavilyPlanned)
	{
		iV_Box(imageX0 - 1, imageY0 - 1, imageX0 + displayWidth + 1, imageY0 + displayHeight + 1, pal_RGBA(252, 132, 3, 200));
	}
}

class WzTeamStrategyWeaponButton: public WzTeamStrategyButton
{
protected:
	WzTeamStrategyWeaponButton(const std::shared_ptr<TeamStrategyView>& parentStrategyView, const WzTeamStrategyColumnImageRef& imageRef, int displayWidth, int displayHeight)
	: WzTeamStrategyButton(parentStrategyView, imageRef, displayWidth, displayHeight)
	{ }
public:
	static std::shared_ptr<WzTeamStrategyWeaponButton> make(uint32_t playerIdx, WEAPON_SUBCLASS subClass, const std::shared_ptr<TeamStrategyView>& parentStrategyView);
public:
	void clickPrimary() override
	{
		handleClick(true);
	}
	bool clickSecondary(bool synthesizedFromHold) override
	{
		return handleClick(false);
	}
protected:
	virtual WzStrategyPlanningState currentState() override { return playerStrategyPlans[playerIdx].weaponPlans[subClass].state; }
private:
	bool handleClick(bool primary)
	{
		if (playerIdx != selectedPlayer)
		{
			return false;
		}

		// Change the actual stored value for this player + weapon subclass
		auto newState = cycleWzStrategyPlanningState(playerStrategyPlans[playerIdx].weaponPlans[subClass].state, primary);
		playerStrategyPlans[playerIdx].weaponPlans[subClass].state = newState;
		playerStrategyPlans[playerIdx].hasBeenSet = true;

		if (auto strongParent = parentStrategyView.lock())
		{
			if (strongParent->getUpdatingPlayer())
			{
				// Send the change to other team players
				sendStrategyPlanUpdate(selectedPlayer);
			}
		}

		return true;
	}
private:
	uint32_t playerIdx;
	WEAPON_SUBCLASS subClass;
};

class WzTeamStrategyUnitTypeButton: public WzTeamStrategyButton
{
protected:
	WzTeamStrategyUnitTypeButton(const std::shared_ptr<TeamStrategyView>& parentStrategyView, const WzTeamStrategyColumnImageRef& imageRef, int displayWidth, int displayHeight)
	: WzTeamStrategyButton(parentStrategyView, imageRef, displayWidth, displayHeight)
	{ }
public:
	static std::shared_ptr<WzTeamStrategyUnitTypeButton> make(uint32_t playerIdx, WzStrategyPlanningUnitTypes unitType, const std::shared_ptr<TeamStrategyView>& parentStrategyView);
public:
	void clickPrimary() override
	{
		handleClick(true);
	}
	bool clickSecondary(bool synthesizedFromHold) override
	{
		return handleClick(false);
	}
protected:
	virtual WzStrategyPlanningState currentState() override { return playerStrategyPlans[playerIdx].unitPlans[static_cast<size_t>(unitType)].state; }
private:
	bool handleClick(bool primary)
	{
		if (playerIdx != selectedPlayer)
		{
			return false;
		}

		// Change the actual stored value for this player + weapon subclass
		auto newState = cycleWzStrategyPlanningState(playerStrategyPlans[playerIdx].unitPlans[static_cast<size_t>(unitType)].state, primary);
		playerStrategyPlans[playerIdx].unitPlans[static_cast<size_t>(unitType)].state = newState;
		playerStrategyPlans[playerIdx].hasBeenSet = true;

		if (auto strongParent = parentStrategyView.lock())
		{
			if (strongParent->getUpdatingPlayer())
			{
				// Send the change to other team players
				sendStrategyPlanUpdate(selectedPlayer);
			}
		}

		return true;
	}
private:
	uint32_t playerIdx;
	WzStrategyPlanningUnitTypes unitType;
};

#if defined(WZ_CC_GNU) && !defined(WZ_CC_INTEL) && !defined(WZ_CC_CLANG) && (7 <= __GNUC__) && ( __GNUC__ < 9)
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wnull-dereference"
#endif

std::shared_ptr<WzTeamStrategyWeaponButton> WzTeamStrategyWeaponButton::make(uint32_t playerIdx, WEAPON_SUBCLASS subClass, const std::shared_ptr<TeamStrategyView>& parentStrategyView)
{
	auto imageRef = parentStrategyView->getSharedColumnImagesManager()->getWeaponSubclassImageRef(subClass);

	class make_shared_enabler: public WzTeamStrategyWeaponButton
	{
	public:
		make_shared_enabler(const std::shared_ptr<TeamStrategyView>& parentStrategyView, const WzTeamStrategyColumnImageRef& imageRef, int displayWidth, int displayHeight)
		: WzTeamStrategyWeaponButton(parentStrategyView, imageRef, displayWidth, displayHeight)
		{ }
	};
	auto result = std::make_shared<make_shared_enabler>(parentStrategyView, imageRef, WZ_TEAMSTRAT_COLUMN_HEADER_IMAGE_DISPLAY_SIZE, WZ_TEAMSTRAT_COLUMN_HEADER_IMAGE_DISPLAY_SIZE);

	result->playerIdx = playerIdx;
	result->subClass = subClass;

	return result;
}

std::shared_ptr<WzTeamStrategyUnitTypeButton> WzTeamStrategyUnitTypeButton::make(uint32_t playerIdx, WzStrategyPlanningUnitTypes unitType, const std::shared_ptr<TeamStrategyView>& parentStrategyView)
{
	auto imageRef = parentStrategyView->getSharedColumnImagesManager()->getUnitTypeImageRef(unitType);

	class make_shared_enabler: public WzTeamStrategyUnitTypeButton
	{
	public:
		make_shared_enabler(const std::shared_ptr<TeamStrategyView>& parentStrategyView, const WzTeamStrategyColumnImageRef& imageRef, int displayWidth, int displayHeight)
		: WzTeamStrategyUnitTypeButton(parentStrategyView, imageRef, displayWidth, displayHeight)
		{ }
	};
	auto result = std::make_shared<make_shared_enabler>(parentStrategyView, imageRef, WZ_TEAMSTRAT_COLUMN_HEADER_IMAGE_DISPLAY_SIZE, WZ_TEAMSTRAT_COLUMN_HEADER_IMAGE_DISPLAY_SIZE);

	result->playerIdx = playerIdx;
	result->unitType = unitType;

	return result;
}

#if defined(WZ_CC_GNU) && !defined(WZ_CC_INTEL) && !defined(WZ_CC_CLANG) && (7 <= __GNUC__) && ( __GNUC__ < 9)
# pragma GCC diagnostic pop
#endif

// MARK: - TeamStrategyView

constexpr int32_t MAX_PLAYER_NAME_COLUMN_WIDTH = 120;

TeamStrategyView::TeamStrategyView()
: columnImagesManager(std::make_shared<WzTeamStrategyColumnImagesManager>())
{ }

TeamStrategyView::~TeamStrategyView()
{ }

void TeamStrategyView::display(int xOffset, int yOffset)
{
	int x0 = x() + xOffset;
	int y0 = y() + yOffset;
	if (!backgroundColor.isTransparent())
	{
		pie_UniTransBoxFill(x0, y0, x0 + width(), y0 + height(), backgroundColor);
	}
}

void TeamStrategyView::geometryChanged()
{
	if (noTeammatesLabel)
	{
		noTeammatesLabel->callCalcLayout();
	}
	if (table)
	{
		table->callCalcLayout();
	}
}

void TeamStrategyView::updateNoTeamLabelContents()
{
	WzString noTeammatesStr = _("No teammates.");
	if (gameTime <= GAME_TICKS_PER_UPDATE)
	{
		noTeammatesStr += "\n";
		noTeammatesStr += _("Waiting for other teams to finish planning.");
	}
	noTeammatesLabel->setFormattedString(noTeammatesStr, UINT32_MAX, font_regular);
	noTeammatesLabel->setGeometry(0, 0, noTeammatesLabel->getMaxLineWidth(), noTeammatesLabel->requiredHeight());
}

class PlayerNameLabel : public W_LABEL
{
protected:
	PlayerNameLabel()
	: W_LABEL()
	{ }

	void initialize(uint32_t playerIdx_)
	{
		playerIdx = playerIdx_;
		setFont((playerIdx == selectedPlayer) ? font_regular_bold : font_regular);
		setString(getPlayerName(playerIdx));
		setGeometry(0, 0, getMaxLineWidth(), requiredHeight());
		setTransparentToMouse(true);
		setCanTruncate(true);
	}
public:
	static std::shared_ptr<PlayerNameLabel> make(uint32_t playerIdx)
	{
		class make_shared_enabler: public PlayerNameLabel {};
		auto result = std::make_shared<make_shared_enabler>();
		result->initialize(playerIdx);
		return result;
	}

	virtual void run(W_CONTEXT *) override
	{
		// Update the player name (ex. once revealed in a blind_lobby game)
		setString(getPlayerName(playerIdx));
	}
private:
	uint32_t playerIdx = 0;
};

std::shared_ptr<TableRow> TeamStrategyView::newPlayerStrategyRow(uint32_t playerIdx, const std::vector<WEAPON_SUBCLASS>& displayedWeaponSubclasses, const std::unordered_set<WzStrategyPlanningUnitTypes>& disabledUnitTypes)
{
	std::vector<std::shared_ptr<WIDGET>> columnWidgets;

	std::shared_ptr<TeamStrategyView> parentStrategyView = std::dynamic_pointer_cast<TeamStrategyView>(shared_from_this());

	int verticalRowPadding = (playerIdx == selectedPlayer) ? 10 : 3;

	// Player Name widget
	auto playerNameLabel = PlayerNameLabel::make(playerIdx);
	auto wrappedPlayerNameWidget = Margin(verticalRowPadding, 0).wrap(playerNameLabel);
	columnWidgets.push_back(wrappedPlayerNameWidget);

	int rowHeight = wrappedPlayerNameWidget->idealHeight();

	// All of the weapon columns
	for (auto subClass : displayedWeaponSubclasses)
	{
		auto weaponSubclassWidget = WzTeamStrategyWeaponButton::make(playerIdx, subClass, parentStrategyView);
		weaponSubclassWidget->setGeometry(0, 0, weaponSubclassWidget->idealWidth(), weaponSubclassWidget->idealHeight());
		rowHeight = std::max(rowHeight, weaponSubclassWidget->height());
		columnWidgets.push_back(weaponSubclassWidget);
	}

	// Add blank label (to fill the "Units:" column
	columnWidgets.push_back(std::make_shared<W_LABEL>());

	// Unit Types
	for (size_t i = 0; i < WzStrategyPlanningUnitTypes_NUM_TYPES; ++i)
	{
		auto unitType = static_cast<WzStrategyPlanningUnitTypes>(i);
		auto unitTypeWidget = WzTeamStrategyUnitTypeButton::make(playerIdx, unitType, parentStrategyView);
		unitTypeWidget->setGeometry(0, 0, unitTypeWidget->idealWidth(), unitTypeWidget->idealHeight());
		if (disabledUnitTypes.count(unitType) > 0)
		{
			unitTypeWidget->setState(WBUT_DISABLE);
		}
		rowHeight = std::max(rowHeight, unitTypeWidget->height());
		columnWidgets.push_back(unitTypeWidget);
	}

	auto row = TableRow::make(columnWidgets, rowHeight);
	row->setDisabledColor(WZCOL_FORM_DARK);
	if (playerIdx == selectedPlayer)
	{
		row->setDrawBorder(pal_RGBA(27, 42, 250, 200));
		row->setHighlightsOnMouseOver(true);
	}
	else
	{
		// other player rows start off as disabled
		row->setDisabled(true);
	}
	return row;
}


static std::shared_ptr<WIDGET> createColHeaderLabel(const char* text, int minLeftPadding)
{
	auto label = std::make_shared<W_LABEL>();
	label->setTextAlignment(WLAB_ALIGNBOTTOMRIGHT);
	label->setString(text);
	label->setGeometry(0, 0, label->getMaxLineWidth(), 0);
	label->setCacheNeverExpires(true);
	return Margin(0, 2, WZ_TEAMSTRAT_COLUMN_HEADER_PADDING_Y, minLeftPadding).wrap(label);
}

std::shared_ptr<TeamStrategyView> TeamStrategyView::make(bool allowAIs)
{
	if (!gameHasTeamStrategyView())
	{
		return nullptr;
	}

	if (selectedPlayer >= MAX_PLAYERS || (selectedPlayer < MAX_CONNECTED_PLAYERS && NetPlay.players[selectedPlayer].isSpectator))
	{
		return nullptr;
	}

	// Check if current player has teammates
	auto selectedPlayerTeam = checkedGetPlayerTeam(selectedPlayer);
	std::vector<uint32_t> playersOnSameTeamAsSelectedPlayer;
	for (int32_t player = 0; player < std::min<int32_t>(game.maxPlayers, MAX_PLAYERS); ++player)
	{
		if (player == selectedPlayer)
		{
			continue;
		}
		if (checkedGetPlayerTeam(player) != selectedPlayerTeam)
		{
			continue;
		}
		// Only include human players (or future AIs that actually set a strategy)
		if (isHumanPlayer(player) // is an active (connected) human player
			|| NetPlay.players[player].difficulty == AIDifficulty::HUMAN // was a human player (probably disconnected)
			|| (allowAIs && playerStrategyPlans[player].hasBeenSet) // an AI that set a strategy (only would happen in-game, not pre-game waiting screen)
			)
		{
			playersOnSameTeamAsSelectedPlayer.push_back(static_cast<uint32_t>(player));
		}
	}

	class make_shared_enabler: public TeamStrategyView {};
	auto result = std::make_shared<make_shared_enabler>();

	// Create column headers for stats table
	std::vector<TableColumn> columns;
	// "Weapons:" (right-aligned)
	columns.push_back({createColHeaderLabel(_("Weapons:"), 20), TableColumn::ResizeBehavior::FIXED_WIDTH});
	// Add weapon subclass columns
	std::vector<WEAPON_SUBCLASS> displayedWeaponSubclasses;
	for (size_t i = 0; i < WSC_NUM_WEAPON_SUBCLASSES; ++i)
	{
		auto subClass = static_cast<WEAPON_SUBCLASS>(i);
		// Possible Future TODO: Check if currently-loaded stats actually use this weapon subclass?
		displayedWeaponSubclasses.push_back(subClass);
	}
	for (auto subClass : displayedWeaponSubclasses)
	{
		auto weaponColumHeader = WzTeamStrategyIconColumnHeader::make(getWeaponSubClassDisplayName(subClass, true), result->columnImagesManager->getWeaponSubclassImageRef(subClass));
		weaponColumHeader->setTip(getWeaponSubClassDisplayName(subClass, false));
		weaponColumHeader->setGeometry(0, 0, weaponColumHeader->idealWidth(), weaponColumHeader->idealHeight());
		columns.push_back({weaponColumHeader, TableColumn::ResizeBehavior::FIXED_WIDTH});
	}
	// "Units": (right-aligned)
	columns.push_back({createColHeaderLabel(_("Units:"), 20), TableColumn::ResizeBehavior::FIXED_WIDTH});
	// Add unit types columns
	std::unordered_set<WzStrategyPlanningUnitTypes> disabledUnitTypes;
	for (size_t i = 0; i < WzStrategyPlanningUnitTypes_NUM_TYPES; ++i)
	{
		auto unitType = static_cast<WzStrategyPlanningUnitTypes>(i);
		bool typeDisabled = false;
		switch (unitType)
		{
			case WzStrategyPlanningUnitTypes::Tanks:
				typeDisabled = (ingame.flags & MPFLAGS_NO_TANKS) != 0;
				break;
			case WzStrategyPlanningUnitTypes::VTOL:
				typeDisabled = (ingame.flags & MPFLAGS_NO_VTOLS) != 0;
				break;
			case WzStrategyPlanningUnitTypes::Cyborg:
				typeDisabled = (ingame.flags & MPFLAGS_NO_CYBORGS) != 0;
				break;
		}
		if (typeDisabled)
		{
			disabledUnitTypes.insert(unitType);
		}
		auto unitTypeColumHeader = WzTeamStrategyIconColumnHeader::make(getUnitTypeDisplayName(unitType), result->columnImagesManager->getUnitTypeImageRef(unitType), typeDisabled);
		unitTypeColumHeader->setTip(getUnitTypeDisplayName(unitType));
		unitTypeColumHeader->setGeometry(0, 0, unitTypeColumHeader->idealWidth(), unitTypeColumHeader->idealHeight());
		columns.push_back({unitTypeColumHeader, TableColumn::ResizeBehavior::FIXED_WIDTH});
	}

	int maxIdealColumnHeight = 0;
	std::vector<size_t> minimumColumnWidths;
	for (auto& column : columns)
	{
		minimumColumnWidths.push_back(static_cast<size_t>(std::max<int>(column.columnWidget->idealWidth(), 0)));
		maxIdealColumnHeight = std::max(maxIdealColumnHeight, column.columnWidget->idealHeight());
	}

	// Create + attach "Strategy" scrollable table view
	int strategyTableColumnHeaderHeight = std::min(maxIdealColumnHeight, MAX_TEAMSTRAT_COLUMN_HEADERS_HEIGHT);
	result->table = ScrollableTableWidget::make(columns, strategyTableColumnHeaderHeight);
	result->table->setColumnPadding(Vector2i(COL_PADDING_X, 0));
	result->attach(result->table);
	result->table->setBackgroundColor(pal_RGBA(0, 0, 0, 0));
	result->table->setMinimumColumnWidths(minimumColumnWidths);
	result->table->setDrawColumnLines(true);

	// Add row for ourselves
	result->table->addRow(result->newPlayerStrategyRow(selectedPlayer, displayedWeaponSubclasses, disabledUnitTypes));

	// Add rows for all players on same team as selectedPlayer
	// sort by player position
	std::sort(playersOnSameTeamAsSelectedPlayer.begin(), playersOnSameTeamAsSelectedPlayer.end(), [](uint32_t playerA, uint32_t playerB) -> bool {
		return NetPlay.players[playerA].position < NetPlay.players[playerB].position;
	});
	for (auto player : playersOnSameTeamAsSelectedPlayer)
	{
		if (player == selectedPlayer)
		{
			continue;
		}
		result->table->addRow(result->newPlayerStrategyRow(player, displayedWeaponSubclasses, disabledUnitTypes));
	}
	if (playersOnSameTeamAsSelectedPlayer.empty())
	{
		// Create a label below the table, since there are no other teammates to display
		result->noTeammatesLabel = std::make_shared<W_LABEL>();
		result->noTeammatesLabel->setTextAlignment(WLAB_ALIGNLEFT);
		result->noTeammatesLabel->setCacheNeverExpires(true);
		result->noTeammatesLabel->setFontColour(WZCOL_TEXT_MEDIUM);
		result->updateNoTeamLabelContents();
		result->attach(result->noTeammatesLabel);
		result->noTeammatesLabel->setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
			auto psParent = std::dynamic_pointer_cast<TeamStrategyView>(psWidget->parent());
			ASSERT_OR_RETURN(, psParent != nullptr, "No parent");
			// should update the formatted string with the new maximum width
			psParent->updateNoTeamLabelContents();
			int y0 = psParent->height() - psWidget->height();
			psWidget->setGeometry(0, y0, psWidget->width(), psWidget->height());
		}));
	}

	result->table->setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
		auto psParent = std::dynamic_pointer_cast<TeamStrategyView>(psWidget->parent());
		ASSERT_OR_RETURN(, psParent != nullptr, "No parent");
		int tableHeight = psParent->height();
		if (psParent->noTeammatesLabel)
		{
			tableHeight -= psParent->noTeammatesLabel->requiredHeight();
		}
		psWidget->setGeometry(0, 0, psParent->width(), tableHeight);
	}));

	return result;
}

void TeamStrategyView::setUpdatingPlayers(bool enabled)
{
	if (enabled == updatingPlayers)
	{
		return;
	}
	updatingPlayers = enabled;
	if (updatingPlayers)
	{
		// send any local updates to other team members
		sendStrategyPlanUpdate(selectedPlayer);

		// enable all other player rows
		for (size_t i = 1; i < table->getNumRows(); ++i)
		{
			table->setRowDisabled(i, false);
		}
	}
	else
	{
		// disable all other player rows
		for (size_t i = 1; i < table->getNumRows(); ++i)
		{
			table->setRowDisabled(i, true);
		}
	}
}

void TeamStrategyView::setBackgroundColor(PIELIGHT color)
{
	backgroundColor = color;
}

std::pair<std::vector<size_t>, size_t> TeamStrategyView::getMaxTableColumnDataWidths()
{
	size_t totalNeededColumnWidth = 0;
	std::vector<size_t> maxColumnDataWidths;
	auto& minimumColumnWidths = table->getMinimumColumnWidths();
	ASSERT(minimumColumnWidths.size() == table->getNumColumns(), "Number of minimum column widths does not match number of columns!");
	for (size_t colIdx = 0; colIdx < table->getNumColumns(); ++colIdx)
	{
		int32_t maxIdealContentWidth = table->getColumnMaxContentIdealWidth(colIdx);
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

int32_t TeamStrategyView::idealWidth()
{
	ASSERT_OR_RETURN(0, table != nullptr, "Table is null");
	auto maxColumnDataWidthsResult = getMaxTableColumnDataWidths();
	return table->getTableWidthNeededForTotalColumnWidth(table->getNumColumns(), maxColumnDataWidthsResult.second);
}

int32_t TeamStrategyView::idealHeight()
{
	return table->idealHeight() + (noTeammatesLabel ? noTeammatesLabel->requiredHeight() : 0);
}

// MARK: -

// Returns true if there are *any* teams with more than one human player
bool gameHasTeamStrategyView(bool allowAIs)
{
	std::unordered_set<int32_t> teamsWithIncludedPlayers;
	for (int32_t player = 0; player < std::min<int32_t>(game.maxPlayers, MAX_PLAYERS); ++player)
	{
		// Check type of player
		if (isHumanPlayer(player) // is an active (connected) human player
			|| NetPlay.players[player].difficulty == AIDifficulty::HUMAN // was a human player (probably disconnected)
			|| (allowAIs && playerStrategyPlans[player].hasBeenSet) // an AI that set a strategy (only would happen in-game, not pre-game waiting screen)
			)
		{
			auto teamNumber = checkedGetPlayerTeam(player);
			auto result = teamsWithIncludedPlayers.insert(teamNumber);
			if (!result.second)
			{
				// more than one included player on a team
				return true;
			}
		}
	}
	return false;
}

std::shared_ptr<WIDGET> createTeamStrategyView(bool allowAIs)
{
	return TeamStrategyView::make(allowAIs);
}

bool transformTeamStrategyViewMode(const std::shared_ptr<WIDGET>& view, bool updatingPlayers)
{
	auto pTeamStrategyView = std::dynamic_pointer_cast<TeamStrategyView>(view);
	if (!pTeamStrategyView)
	{
		return false;
	}

	pTeamStrategyView->setUpdatingPlayers(updatingPlayers);
	return true;
}

bool teamStrategyViewSetBackgroundColor(const std::shared_ptr<WIDGET>& view, PIELIGHT color)
{
	auto pTeamStrategyView = std::dynamic_pointer_cast<TeamStrategyView>(view);
	if (!pTeamStrategyView)
	{
		return false;
	}

	pTeamStrategyView->setBackgroundColor(color);
	return true;
}

void aiInformTeamOfStrategy(uint32_t aiPlayerIdx, const std::unordered_map<WEAPON_SUBCLASS, WzStrategyPlanningState>& weaponStrategy, const std::unordered_map<WzStrategyPlanningUnitTypes, WzStrategyPlanningState>& unitTypesStrategy)
{
	ASSERT_OR_RETURN(, aiPlayerIdx < MAX_PLAYERS, "Invalid aiPlayerIdx: %" PRIu32, aiPlayerIdx);
	ASSERT_OR_RETURN(, NetPlay.players[aiPlayerIdx].ai >= 0, "Not an AI player?: %" PRIu32, aiPlayerIdx);
	ASSERT_OR_RETURN(, whosResponsible(aiPlayerIdx) == realSelectedPlayer, "We are not responsible for AI player?: %" PRIu32, aiPlayerIdx);

	playerStrategyPlans[aiPlayerIdx].reset();

	for (auto it : weaponStrategy)
	{
		playerStrategyPlans[aiPlayerIdx].weaponPlans[it.first].state = it.second;
	}
	for (auto it : unitTypesStrategy)
	{
		playerStrategyPlans[aiPlayerIdx].unitPlans[static_cast<size_t>(it.first)].state = it.second;
	}

	playerStrategyPlans[aiPlayerIdx].hasBeenSet = true;

	sendStrategyPlanUpdate(aiPlayerIdx);
}
