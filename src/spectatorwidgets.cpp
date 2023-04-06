/*
	This file is part of Warzone 2100.
	Copyright (C) 2021  Warzone 2100 Project

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
/**
 * @file
 * Functions for spectator widgets
 */

#include "spectatorwidgets.h"

#include "lib/widget/button.h"
#include "lib/widget/table.h"
#include "lib/widget/label.h"

#include "frontend.h"
#include "lib/ivis_opengl/pieblitfunc.h"
#include "lib/ivis_opengl/bitimage.h"
#include "hci.h"
#include "intimage.h"
#include "intdisplay.h"
#include "multiplay.h"
#include "power.h"
#include "multistat.h"
#include "notifications.h"
#include "component.h"
#include "loop.h" // for getNumDroids

#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <functional>

#define SPEC_STATS_WINDOW_TITLE_HEIGHT 24
#define SPEC_STATS_WINDOW_BOTTOM_PADDING 5
#define MAX_PLAYER_NAME_COLUMN_WIDTH 120
#define PLAYER_COLOR_COL_SIZE 8
#define INFO_UPDATE_INTERVAL_TICKS GAME_TICKS_PER_SEC

class WzWeaponGradesColumnManager;

class SpectatorStatsView: public W_FORM
{
protected:
	SpectatorStatsView();

public:
	static std::shared_ptr<SpectatorStatsView> make();

public:
	virtual void display(int xOffset, int yOffset) override;
	virtual void geometryChanged() override;
	virtual int32_t idealWidth() override;
	virtual int32_t idealHeight() override;
private:
	std::shared_ptr<TableRow> newPlayerStatsRow(uint32_t playerIdx, int rowHeight = 0);
	void resizeTableColumnWidths();
	std::pair<std::vector<size_t>, size_t> getMaxTableColumnDataWidths();
private:
	std::shared_ptr<W_BUTTON> optionsButton;
	std::shared_ptr<W_BUTTON> closeButton;
	std::shared_ptr<ScrollableTableWidget> table;
	std::shared_ptr<WzWeaponGradesColumnManager> weaponGradesManager;
	size_t playerNameColIdx = 1;
};

static std::shared_ptr<W_SCREEN> statsOverlay = nullptr;
static std::shared_ptr<W_BUTTON> specStatsButton = nullptr;
static std::shared_ptr<SpectatorStatsView> globalStatsForm = nullptr;

#define SPEC_STATS_BUTTON_X RET_X
#define SPEC_STATS_BUTTON_Y 20

bool specLayerInit(bool showButton /*= true*/)
{
	if (selectedPlayer < NetPlay.players.size() && NetPlay.players[selectedPlayer].isSpectator
			 && realSelectedPlayer < NetPlay.players.size() && NetPlay.players[realSelectedPlayer].isSpectator)
	{
		if (!statsOverlay)
		{
			statsOverlay = W_SCREEN::make();
			statsOverlay->psForm->hide(); // hiding the root form does not stop display of children, but *does* prevent it from accepting mouse over itself - i.e. basically makes it transparent
		}
		widgRegisterOverlayScreen(statsOverlay, std::numeric_limits<uint16_t>::max() - 3);

		// Add the spec stats button
		if (showButton && !specStatsButton)
		{
			int x = SPEC_STATS_BUTTON_X;
			int y = SPEC_STATS_BUTTON_Y;
			int width = iV_GetImageWidth(IntImages, IMAGE_SPECSTATS_UP);
			int height = iV_GetImageHeight(IntImages, IMAGE_SPECSTATS_UP);
			ASSERT_OR_RETURN(true, width > 0 && height > 0, "Possibly missing specstats button image?");

			specStatsButton = std::make_shared<W_BUTTON>();
			specStatsButton->displayFunction = intDisplayImageHilight;
			specStatsButton->UserData = PACKDWORD_TRI(0, IMAGE_SPECSTATS_DOWN, IMAGE_SPECSTATS_UP);
			specStatsButton->pTip = _("Show Player Stats");
			specStatsButton->addOnClickHandler([](W_BUTTON& button){
				widgScheduleTask([](){
					specStatsViewCreate();
				});
			});
			specStatsButton->setGeometry(x, y, width, height);
			statsOverlay->psForm->attach(specStatsButton);
		}

		return true;
	}

	return false;
}

void specStatsViewClose()
{
	if (globalStatsForm)
	{
		globalStatsForm->deleteLater();
		globalStatsForm = nullptr;
	}

	// Show the spec stats button
	if (specStatsButton)
	{
		specStatsButton->show();
	}
}

void specToggleOverlays()
{
	if (globalStatsForm)
	{
		specStatsViewClose();
	}
	else if (selectedPlayer < NetPlay.players.size() && NetPlay.players[selectedPlayer].isSpectator
			 && realSelectedPlayer < NetPlay.players.size() && NetPlay.players[realSelectedPlayer].isSpectator)
	{
		specStatsViewCreate();
	}
}

bool specStatsViewShutdown()
{
	if (statsOverlay)
	{
		widgRemoveOverlayScreen(statsOverlay);
	}
	specStatsViewClose();
	specStatsButton = nullptr; // widget is owned by parent form, so will be detached / destroyed there as long as we don't keep a ref
	statsOverlay = nullptr;
	return true;
}

void specStatsViewCreate()
{
	if (realSelectedPlayer >= NetPlay.players.size() || !NetPlay.players[realSelectedPlayer].isSpectator)
	{
		if (statsOverlay)
		{
			widgRemoveOverlayScreen(statsOverlay);
		}
		return;
	}
	if (!statsOverlay)
	{
		specLayerInit();
		if (!statsOverlay)
		{
			return;
		}
	}
	if (!globalStatsForm)
	{
		globalStatsForm = SpectatorStatsView::make();
		if (globalStatsForm)
		{
			statsOverlay->psForm->attach(globalStatsForm);
		}
	}
	// Hide the spec stats button
	if (specStatsButton)
	{
		specStatsButton->hide();
	}
}

SpectatorStatsView::SpectatorStatsView()
: W_FORM()
{ }

void SpectatorStatsView::display(int xOffset, int yOffset)
{
	int x0 = x() + xOffset;
	int y0 = y() + yOffset;
	pie_UniTransBoxFill(x0, y0, x0 + width(), y0 + height(), pal_RGBA(0, 0, 0, 80));
}

static std::shared_ptr<W_LABEL> createColHeaderLabel(const char* text)
{
	auto label = std::make_shared<W_LABEL>();
	label->setString(text);
	label->setGeometry(0, 0, label->getMaxLineWidth(), 0);
	label->setCacheNeverExpires(true);
	return label;
}

class WzThrottledUpdateLabel : public W_LABEL
{
public:
	typedef std::function<void (W_LABEL& label)> UpdateFunc;
public:
	WzThrottledUpdateLabel(const UpdateFunc& updateFunc, uint32_t updateInterval)
	: W_LABEL()
	, updateFunc(updateFunc)
	, updateInterval(updateInterval)
	{ }
public:
	static std::shared_ptr<WzThrottledUpdateLabel> make(const UpdateFunc& updateFunc, uint32_t updateInterval)
	{
		auto newLabel = std::make_shared<WzThrottledUpdateLabel>(updateFunc, updateInterval);
		newLabel->setFont(font_regular, WZCOL_FORM_LIGHT);
		newLabel->setCanTruncate(false);
		newLabel->setTransparentToClicks(true);
		if (updateFunc)
		{
			updateFunc(*newLabel);
		}
		newLabel->setGeometry(0, 0, newLabel->getMaxLineWidth(), iV_GetTextLineSize(font_regular));
		return newLabel;
	}
public:
	virtual void run(W_CONTEXT *pContext) override
	{
		W_LABEL::run(pContext);
		if (gameTime - lastGameTimeUpdated >= updateInterval)
		{
			if (updateFunc) { updateFunc(*this); }
			lastGameTimeUpdated = gameTime;
		}
	}
private:
	UpdateFunc updateFunc;
	uint32_t updateInterval = 0;
	uint32_t lastGameTimeUpdated = 0;
};

static gfx_api::texture* loadImageForArmorCol(bool thermal)
{
	const char* imagePath = "images/intfac/armor_kinetic.png";
	if (thermal)
	{
		imagePath = "images/intfac/armor_thermal.png";
	}
	WZ_Notification_Image img(imagePath);
	return img.loadImageToTexture();
}

#define WZ_CLEANUP_VIEW(id) \
widgScheduleTask([](){ \
	specS##id##ewShutdown(); \
});

static gfx_api::texture* loadImageForWeapSubclass(WEAPON_SUBCLASS subClass)
{
	const char* imagePath = nullptr;
	switch (subClass)
	{
		case WSC_MGUN:
			imagePath = "images/intfac/wsc_mgun.png";
			break;
		case WSC_CANNON:
			imagePath = "images/intfac/wsc_cannon.png";
			break;
		case WSC_MORTARS:
			imagePath = "images/intfac/wsc_mortars.png";
			break;
		case WSC_MISSILE:
			imagePath = "images/intfac/wsc_missile.png";
			break;
		case WSC_ROCKET:
			imagePath = "images/intfac/wsc_rocket.png";
			break;
		case WSC_ENERGY:
			imagePath = "images/intfac/wsc_energy.png";
			break;
		case WSC_GAUSS:
			imagePath = "images/intfac/wsc_gauss.png";
			break;
		case WSC_FLAME:
			imagePath = "images/intfac/wsc_flame.png";
			break;
		//case WSC_CLOSECOMBAT:
		case WSC_HOWITZERS:
			imagePath = "images/intfac/wsc_howitzers.png";
			break;
		case WSC_ELECTRONIC:
			imagePath = "images/intfac/wsc_electronic.png";
			break;
		case WSC_AAGUN:
			imagePath = "images/intfac/wsc_aagun.png";
			break;
		case WSC_SLOWMISSILE:
			imagePath = "images/intfac/wsc_slowmissile.png";
			break;
		case WSC_SLOWROCKET:
			imagePath = "images/intfac/wsc_slowrocket.png";
			break;
		case WSC_LAS_SAT:
			imagePath = "images/intfac/wsc_las_sat.png";
			break;
		case WSC_BOMB:
			imagePath = "images/intfac/wsc_bomb.png";
			break;
		case WSC_COMMAND:
			imagePath = "images/intfac/wsc_command.png";
			break;
		case WSC_EMP:
			imagePath = "images/intfac/wsc_emp.png";
			break;
		case WSC_NUM_WEAPON_SUBCLASSES:	/** The number of enumerators in this enum.	 */
			break;
	}
	ASSERT_OR_RETURN(nullptr, imagePath != nullptr, "No image path");
	WZ_Notification_Image img(imagePath);
	return img.loadImageToTexture();
}

class WzCenteredColumnIcon: public W_BUTTON
{
public:
	WzCenteredColumnIcon(gfx_api::texture* pImageTexture, int displayWidth, int displayHeight)
	: W_BUTTON()
	, pImageTexture(pImageTexture)
	, displayWidth(displayWidth)
	, displayHeight(displayHeight)
	{ }
	virtual ~WzCenteredColumnIcon()
	{
		if (pImageTexture)
		{
			delete pImageTexture;
			pImageTexture = nullptr;
		}
	}

	virtual void display(int xOffset, int yOffset) override
	{
		int x0 = x() + xOffset;
		int y0 = y() + yOffset;

		if (pImageTexture)
		{
			int imageX0 = x0 + (width() - displayWidth) / 2;
			int imageY0 = y0 + (height() - displayHeight) / 2;
			iV_DrawImageAnisotropic(*pImageTexture, Vector2i(imageX0, imageY0), Vector2f(0, 0), Vector2f(displayWidth, displayHeight), 0.f, pal_RGBA(255, 255, 255, 200));
		}
	}
private:
	gfx_api::texture* pImageTexture = nullptr;
	int displayWidth = 0;
	int displayHeight = 0;
};

#define WEAP_GRADES_COL_WIDTH 16
#define WEAP_GRADES_COL_IMAGE_WIDTH 16
#define WEAP_GRADES_COL_IMAGE_HEIGHT 16
#define WEAP_GRADES_COL_PADDING 6

class WzWeaponGradesColumnManager: public WIDGET
{
protected:
	WzWeaponGradesColumnManager()
	: WIDGET()
	{ }
public:
	static std::shared_ptr<WzWeaponGradesColumnManager> make()
	{
		class make_shared_enabler: public WzWeaponGradesColumnManager {};
		auto result = std::make_shared<make_shared_enabler>();

		result->updateData();
		result->idealHeightForEachRow = iV_GetTextLineSize(font_regular);
		result->setGeometry(0, 0, 64, std::max(result->idealHeightForEachRow, WEAP_GRADES_COL_WIDTH));

		return result;
	}

	virtual void run(W_CONTEXT *pContext) override
	{
		if (gameTime - lastGameTimeUpdated >= updateInterval)
		{
			updateData();
			lastGameTimeUpdated = gameTime;
			if (!NetPlay.players[realSelectedPlayer].isSpectator)
			{
				WZ_CLEANUP_VIEW(tatsVi);
			}
		}
	}

	virtual void display(int xOffset, int yOffset) override
	{
		// no-op
	}

	virtual void geometryChanged() override
	{
		for (auto& icon : columnIcons)
		{
			icon.second->callCalcLayout();
		}
	}

	virtual int32_t idealWidth() override
	{
		int32_t numColumns = static_cast<int32_t>(visibleColumnOrder.size());
		return (numColumns * WEAP_GRADES_COL_WIDTH) + (std::max(numColumns - 1, 0) * WEAP_GRADES_COL_PADDING);
	}

	virtual int32_t idealHeight() override
	{
		return std::max(idealHeightForEachRow, WEAP_GRADES_COL_WIDTH);
	}

public:
	typedef std::function<void (WzWeaponGradesColumnManager&)> OnChangeFunc;
	void addOnIdealWidthChangeFunc(const OnChangeFunc& handler)
	{
		if (!handler) { return; }
		onIdealWidthChangeFuncs.push_back(handler);
	}

	int getIdealHeightForEachRow() const { return idealHeightForEachRow; }
	const std::vector<WEAPON_SUBCLASS>& getVisibleColumnOrder() const
	{
		return visibleColumnOrder;
	}
	uint32_t getMaxGradeOfAllPlayers(WEAPON_SUBCLASS subClass) const
	{
		return weapSubclassInfo[subClass].maxNumWeaponGrade;
	}
	const std::unordered_set<uint32_t>& getPlayersWithMaxGrade(WEAPON_SUBCLASS subClass) const
	{
		return weapSubclassInfo[subClass].playersWithMaxGrade;
	}
	int getColumnLeftPositionFromIndex(size_t colIndex)
	{
		return colIndex * (WEAP_GRADES_COL_WIDTH + WEAP_GRADES_COL_PADDING);
	}
	WzRect getColumnRectFromIndex(size_t colIndex)
	{
		return WzRect(getColumnLeftPositionFromIndex(colIndex), 0, WEAP_GRADES_COL_WIDTH, WEAP_GRADES_COL_IMAGE_HEIGHT);
	}
	std::shared_ptr<WzCachedText> getWzCachedTextForNumber(uint32_t number)
	{
		// should be self-limiting because there are only a limited number of grades for any weapon
		auto it = cachedNumberTexts.find(number);
		if (it != cachedNumberTexts.end())
		{
			return it->second;
		}
		auto newCachedText = std::make_shared<WzCachedText>(WzString::number(number), font_regular);
		cachedNumberTexts[number] = newCachedText;
		return newCachedText;
	}

private:
	void buildWeaponSubclassInfo()
	{
		for (int i = 0; i < WSC_NUM_WEAPON_SUBCLASSES; ++i)
		{
			for (uint32_t playerIdx = 0; playerIdx < game.maxPlayers; ++playerIdx)
			{
				uint32_t gradeForPlayer = getNumWeaponImpactClassUpgrades(playerIdx, static_cast<WEAPON_SUBCLASS>(i));
				if (gradeForPlayer == weapSubclassInfo[i].maxNumWeaponGrade)
				{
					weapSubclassInfo[i].playersWithMaxGrade.insert(playerIdx);
				}
				else if (gradeForPlayer > weapSubclassInfo[i].maxNumWeaponGrade)
				{
					weapSubclassInfo[i].playersWithMaxGrade.clear();
					weapSubclassInfo[i].maxNumWeaponGrade = gradeForPlayer;
					weapSubclassInfo[i].playersWithMaxGrade.insert(playerIdx);
				}
			}
		}
	}
	bool hiddenColumn(WEAPON_SUBCLASS subClass)
	{
		return false;
	}
	void setColImageWidgetPosition(WEAPON_SUBCLASS subClass, size_t columnIdx)
	{
		auto it = columnIcons.find(subClass);
		if (it == columnIcons.end())
		{
			auto result = columnIcons.insert(ColumnIconsMap::value_type(subClass, WzWeapColumnIcon::make(subClass)));
			ASSERT(result.second, "Something is very wrong");
			it = result.first;
			attach(it->second);
			it->second->setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
				auto psParent = std::dynamic_pointer_cast<WzWeaponGradesColumnManager>(psWidget->parent());
				ASSERT_OR_RETURN(, psParent != nullptr, "null parent");
				psWidget->setGeometry(psWidget->x(), 0, WEAP_GRADES_COL_IMAGE_WIDTH, psParent->height());
			}));
		}
		it->second->setGeometry(getColumnLeftPositionFromIndex(columnIdx), it->second->y(), it->second->width(), it->second->height());
	}
	void calculateColumnPositions()
	{
		visibleColumnOrder.clear();
		for (size_t i = 0; i < weapSubclassInfo.size(); ++i)
		{
			auto subClass = static_cast<WEAPON_SUBCLASS>(i);
			const auto& info = weapSubclassInfo[i];
			if (hiddenColumn(subClass) || info.maxNumWeaponGrade == 0)
			{
				// hide child widget
				auto it = columnIcons.find(subClass);
				if (it != columnIcons.end())
				{
					it->second->hide();
				}
				continue;
			}
			visibleColumnOrder.push_back(subClass);
			// reposition child widget for icon
			setColImageWidgetPosition(subClass, visibleColumnOrder.size() - 1);
		}
	}
	void updateData()
	{
		size_t idealWidthBefore = idealWidth();
		buildWeaponSubclassInfo();
		calculateColumnPositions();
		for (auto& cachedText : cachedNumberTexts)
		{
			cachedText.second->tick();
		}
		if (idealWidth() != idealWidthBefore)
		{
			for (auto& func : onIdealWidthChangeFuncs)
			{
				if (!func) { continue; }
				func(*this);
			}
		}
	}
private:
	struct WeaponSubclassInfo
	{
		uint32_t maxNumWeaponGrade = 0;
		std::unordered_set<uint32_t> playersWithMaxGrade;
	};
	class WzWeapColumnIcon: public WzCenteredColumnIcon
	{
	protected:
		WzWeapColumnIcon(gfx_api::texture* pImageTexture, int displayWidth, int displayHeight)
		: WzCenteredColumnIcon(pImageTexture, displayWidth, displayHeight)
		{ }
	public:
		static std::shared_ptr<WzWeapColumnIcon> make(WEAPON_SUBCLASS subClass)
		{
			class make_shared_enabler: public WzWeapColumnIcon {
			public:
				make_shared_enabler(gfx_api::texture* pImageTexture, int displayWidth, int displayHeight)
				: WzWeapColumnIcon(pImageTexture, displayWidth, displayHeight)
				{ }
			};
			// Load the required image for display
			auto pImageTexture = loadImageForWeapSubclass(subClass);

			auto result = std::make_shared<make_shared_enabler>(pImageTexture, WEAP_GRADES_COL_IMAGE_WIDTH, WEAP_GRADES_COL_IMAGE_HEIGHT);
			result->subClass = subClass;
			return result;
		}

		virtual std::string getTip() override
		{
			std::string tip = _("Weapon Grade");
			tip += "\n";
			tip += getWeaponSubClass(subClass);
			return tip;
		}
	private:
		WEAPON_SUBCLASS subClass;
	};
	struct WeaponSubclassHasher
	{
		std::size_t operator()(const WEAPON_SUBCLASS& k) const
		{
			return std::hash<uint32_t>()(static_cast<uint32_t>(k));
		}
	};
private:
	std::vector<WeaponSubclassInfo> weapSubclassInfo = std::vector<WeaponSubclassInfo>(WSC_NUM_WEAPON_SUBCLASSES);
	std::vector<WEAPON_SUBCLASS> visibleColumnOrder;

	std::unordered_map<uint32_t, std::shared_ptr<WzCachedText>> cachedNumberTexts;
	typedef std::unordered_map<WEAPON_SUBCLASS, std::shared_ptr<WzWeapColumnIcon>, WeaponSubclassHasher> ColumnIconsMap;
	ColumnIconsMap columnIcons;

	std::vector<OnChangeFunc> onIdealWidthChangeFuncs;

	uint32_t updateInterval = 0;
	uint32_t lastGameTimeUpdated = 0;
	int idealHeightForEachRow = 0;
	optional<uint32_t> lastMouseOverColumn;
};

class WeaponGradesCol: public WIDGET
{
public:
	WeaponGradesCol(const std::shared_ptr<WzWeaponGradesColumnManager>& manager, uint32_t playerIdx)
	: WIDGET()
	, manager(manager)
	, playerIdx(playerIdx)
	{ }

public:
	virtual void display(int xOffset, int yOffset) override
	{
		ASSERT_OR_RETURN(, manager != nullptr, "null manager");
		int x0 = x() + xOffset;

		const auto& visibleColumnOrder = manager->getVisibleColumnOrder();
		for (size_t i = 0; i < visibleColumnOrder.size(); ++i)
		{
			WEAPON_SUBCLASS subClass = visibleColumnOrder[i];
			const auto& playersWithMaxGrade = manager->getPlayersWithMaxGrade(subClass);
			bool behindOtherPlayers = playersWithMaxGrade.count(playerIdx) == 0;
			auto text = manager->getWzCachedTextForNumber(getNumWeaponImpactClassUpgrades(playerIdx, subClass));
			auto colRect = manager->getColumnRectFromIndex(i);
			int posX0 = x0 + colRect.left() + (colRect.width() - (*text)->width()) / 2;
			int textBoundingBoxOffset_y = yOffset + y() + (height() - (*text)->lineSize()) / 2;
			float fy = float(textBoundingBoxOffset_y) - float((*text)->aboveBase());
			PIELIGHT textColor = (!behindOtherPlayers) ? WZCOL_TEXT_BRIGHT : WZCOL_TEXT_MEDIUM;
			(*text)->render(posX0, fy, textColor);
		}
	}
	virtual int32_t idealWidth() override
	{
		ASSERT_OR_RETURN(0, manager != nullptr, "null manager");
		return manager->idealWidth();
	}
private:
	std::shared_ptr<WzWeaponGradesColumnManager> manager;
	uint32_t playerIdx;
};

class PlayerColorColumn: public WIDGET
{
public:
	PlayerColorColumn(uint32_t playerIdx)
	: WIDGET()
	, playerIdx(playerIdx)
	{ }

public:
	virtual void display(int xOffset, int yOffset) override
	{
		int x0 = x() + xOffset;
		int y0 = y() + yOffset;

		int posX0 = x0 + 1;
		int posY0 = y0;

		PIELIGHT playerColor = pal_GetTeamColour(getPlayerColour(playerIdx));
		playerColor.byte.a = 220;
		pie_UniTransBoxFill(posX0, posY0, posX0 + width(), posY0 + (height() - 1), playerColor);
	}
	virtual int32_t idealHeight() override
	{
		return PLAYER_COLOR_COL_SIZE + 1;
	}
	virtual int32_t idealWidth() override
	{
		return PLAYER_COLOR_COL_SIZE;
	}
private:
	uint32_t playerIdx;
};

std::shared_ptr<TableRow> SpectatorStatsView::newPlayerStatsRow(uint32_t playerIdx, int rowHeight /*= 0*/)
{
	   std::vector<std::shared_ptr<WIDGET>> columnWidgets;

#define ADJUST_LABEL_COLOR_FOR_PLAYERS() \
if ((!NetPlay.players[playerIdx].allocated && NetPlay.players[playerIdx].ai < 0) || NetPlay.players[playerIdx].isSpectator) \
{ \
	label.setFontColour(WZCOL_TEXT_MEDIUM); \
}
		// Player color widget
		auto playerColWidget = std::make_shared<PlayerColorColumn>(playerIdx);
		playerColWidget->setGeometry(0, 0, PLAYER_COLOR_COL_SIZE, PLAYER_COLOR_COL_SIZE + 1);
		columnWidgets.push_back(playerColWidget);

		// Player Name widget
		columnWidgets.push_back(WzThrottledUpdateLabel::make([playerIdx](W_LABEL& label){
			label.setString(getPlayerName(playerIdx));
			ADJUST_LABEL_COLOR_FOR_PLAYERS();
		}, INFO_UPDATE_INTERVAL_TICKS));
		// Power info
		columnWidgets.push_back(WzThrottledUpdateLabel::make([playerIdx](W_LABEL& label){
			WzString powerStr = WzString::number(getPower(playerIdx)) + "/" + WzString::fromUtf8(getApproxPowerGeneratedPerSecForDisplay(playerIdx));
			label.setString(powerStr);
			ADJUST_LABEL_COLOR_FOR_PLAYERS();
		}, INFO_UPDATE_INTERVAL_TICKS));
		// PowerLost
		columnWidgets.push_back(WzThrottledUpdateLabel::make([playerIdx](W_LABEL& label){
			label.setString(WzString::number(getMultiStats(playerIdx).recentPowerLost));
			ADJUST_LABEL_COLOR_FOR_PLAYERS();
		}, INFO_UPDATE_INTERVAL_TICKS));
		// Kills
		columnWidgets.push_back(WzThrottledUpdateLabel::make([playerIdx](W_LABEL& label){
			label.setString(WzString::number(getMultiStats(playerIdx).recentKills));
			ADJUST_LABEL_COLOR_FOR_PLAYERS();
		}, INFO_UPDATE_INTERVAL_TICKS));
		// Units (as in F5)
		columnWidgets.push_back(WzThrottledUpdateLabel::make([playerIdx](W_LABEL& label){
			label.setString(WzString::number(getNumDroids(playerIdx) + getNumTransporterDroids(playerIdx)));
			ADJUST_LABEL_COLOR_FOR_PLAYERS();
		}, INFO_UPDATE_INTERVAL_TICKS));
		// Kinetic Armor (T/C)
		columnWidgets.push_back(WzThrottledUpdateLabel::make([playerIdx](W_LABEL& label){
			WzString armorStr = WzString::number(getNumBodyClassArmourUpgrades(playerIdx, BodyClass::Tank)) + "/" +  WzString::number(getNumBodyClassArmourUpgrades(playerIdx, BodyClass::Cyborg));
			label.setString(armorStr);
			ADJUST_LABEL_COLOR_FOR_PLAYERS();
		}, INFO_UPDATE_INTERVAL_TICKS));
		// Thermal Armor (T/C)
		columnWidgets.push_back(WzThrottledUpdateLabel::make([playerIdx](W_LABEL& label){
			WzString armorStr = WzString::number(getNumBodyClassThermalArmourUpgrades(playerIdx, BodyClass::Tank)) + "/" +  WzString::number(getNumBodyClassThermalArmourUpgrades(playerIdx, BodyClass::Cyborg));
			label.setString(armorStr);
			ADJUST_LABEL_COLOR_FOR_PLAYERS();
		}, INFO_UPDATE_INTERVAL_TICKS));
		// Weapon Grades
		auto weapGradesCol = std::make_shared<WeaponGradesCol>(weaponGradesManager, playerIdx);
		weapGradesCol->setGeometry(0, 0, weaponGradesManager->idealWidth(), weaponGradesManager->getIdealHeightForEachRow());
		columnWidgets.push_back(weapGradesCol);

		return TableRow::make(columnWidgets, rowHeight);
   }

std::shared_ptr<SpectatorStatsView> SpectatorStatsView::make()
{
	class make_shared_enabler: public SpectatorStatsView {};
	auto result = std::make_shared<make_shared_enabler>();

	// make minimizable
	result->enableMinimizing("Player Stats", WZCOL_FORM_TEXT);

	// Main "parent" form
	int newFormWidth = FRONTEND_TOPFORM_WIDEW;
	result->setGeometry((pie_GetVideoBufferWidth() - newFormWidth) / 2, 10, newFormWidth, pie_GetVideoBufferHeight() - 50);
	result->userMovable = true;

	// Add the "minimize" button
	auto minimizeButton = makeFormTransparentCornerButton("\u21B8", 10, pal_RGBA(0, 0, 0, 60)); // ↸
	result->attach(minimizeButton);
	minimizeButton->addOnClickHandler([](W_BUTTON& button){
		auto psParent = std::dynamic_pointer_cast<SpectatorStatsView>(button.parent());
		ASSERT_OR_RETURN(, psParent != nullptr, "No parent");
		widgScheduleTask([psParent](){
			psParent->toggleMinimized();
		});
	});

	// Add the "close" button
	W_BUTINIT sButInit;
	sButInit.id = IDOBJ_CLOSE;
	sButInit.pTip = _("Close");
	sButInit.pDisplay = intDisplayImageHilight;
	sButInit.UserData = PACKDWORD_TRI(0, IMAGE_CLOSEHILIGHT, IMAGE_CLOSE);
	result->closeButton = std::make_shared<W_BUTTON>(&sButInit);
	result->attach(result->closeButton);
	result->closeButton->addOnClickHandler([](W_BUTTON& button){
		auto psParent = std::dynamic_pointer_cast<SpectatorStatsView>(button.parent());
		ASSERT_OR_RETURN(, psParent != nullptr, "No parent");
		widgScheduleTask([](){
			specStatsViewClose();
		});
	});
	result->closeButton->setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
		auto psParent = std::dynamic_pointer_cast<SpectatorStatsView>(psWidget->parent());
		ASSERT_OR_RETURN(, psParent != nullptr, "No parent");
		psWidget->setGeometry(psParent->width() - CLOSE_WIDTH, 0, CLOSE_WIDTH, CLOSE_HEIGHT);
	}));

	if (selectedPlayer >= NetPlay.players.size() || !NetPlay.players[selectedPlayer].isSpectator || !NetPlay.players[realSelectedPlayer].isSpectator) { return nullptr; }

	auto createArmorColWidget = [](bool thermal) {
		// Load the required image for display
		auto pImageTexture = loadImageForArmorCol(thermal);
		auto result = std::make_shared<WzCenteredColumnIcon>(pImageTexture, WEAP_GRADES_COL_IMAGE_WIDTH, WEAP_GRADES_COL_IMAGE_HEIGHT);
		return result;
	};

	// Create column headers for stats table
	auto colorLabel = createColHeaderLabel("");
	auto playerLabel = createColHeaderLabel(_("Player"));
	auto powerRateLabel = createColHeaderLabel(_("Power/Rate"));
	powerRateLabel->setTip(_("Current Power / Power Per Second"));
	auto powerLostLabel = createColHeaderLabel(_("PowerLost"));
	auto killsLabel = createColHeaderLabel(_("Kills"));
	auto unitsLabel = createColHeaderLabel(_("Units"));
	auto armorLabel = createArmorColWidget(false);
	armorLabel->setTip(std::string(_("Kinetic Armor")) + "\n" + _("(Tanks / Cyborgs)"));
	auto thermalArmorLabel = createArmorColWidget(true);
	thermalArmorLabel->setTip(std::string(_("Thermal Armor")) + "\n" + _("(Tanks / Cyborgs)"));
	result->weaponGradesManager = WzWeaponGradesColumnManager::make();
	std::vector<TableColumn> columns {
		{colorLabel, TableColumn::ResizeBehavior::FIXED_WIDTH},
		{playerLabel, TableColumn::ResizeBehavior::RESIZABLE},
		{powerRateLabel, TableColumn::ResizeBehavior::RESIZABLE},
		{powerLostLabel, TableColumn::ResizeBehavior::RESIZABLE},
		{killsLabel, TableColumn::ResizeBehavior::RESIZABLE},
		{unitsLabel, TableColumn::ResizeBehavior::RESIZABLE},
		{armorLabel, TableColumn::ResizeBehavior::FIXED_WIDTH},
		{thermalArmorLabel, TableColumn::ResizeBehavior::FIXED_WIDTH},
		{result->weaponGradesManager, TableColumn::ResizeBehavior::FIXED_WIDTH},
	};
	const size_t weaponGradesManagerColIdx = columns.size() - 1;
	std::vector<size_t> minimumColumnWidths;
	for (auto& column : columns)
	{
		minimumColumnWidths.push_back(static_cast<size_t>(std::max<int>(column.columnWidget->idealWidth(), 0)));
	}
	minimumColumnWidths[0] = PLAYER_COLOR_COL_SIZE;
	auto psWeakMain = std::weak_ptr<SpectatorStatsView>(result);
	result->weaponGradesManager->addOnIdealWidthChangeFunc([psWeakMain, weaponGradesManagerColIdx](WzWeaponGradesColumnManager& manager) {
		auto psStrongMain = psWeakMain.lock();
		ASSERT_OR_RETURN(, psStrongMain != nullptr, "Null main view");
		psStrongMain->table->setMinimumColumnWidth(weaponGradesManagerColIdx, manager.idealWidth());
		psStrongMain->callCalcLayout();
		psStrongMain->resizeTableColumnWidths();
	});

	// Create + attach "Triggers" scrollable table view
	result->table = ScrollableTableWidget::make(columns);
	result->attach(result->table);
	result->table->setBackgroundColor(pal_RGBA(0, 0, 0, 25));
	result->table->setMinimumColumnWidths(minimumColumnWidths);

	// Add rows for all players
	std::vector<uint32_t> playerIndexes;
	for (uint32_t player = 0; player < std::min<uint32_t>(game.maxPlayers, MAX_PLAYERS); ++player)
	{
		if (isHumanPlayer(player) || NetPlay.players[player].ai >= 0)
		{
			playerIndexes.push_back(player);
		}
		if (player == selectedPlayer && !NetPlay.players[player].isSpectator)
		{
			return nullptr;
		}
	}
	// sort by player position
	std::sort(playerIndexes.begin(), playerIndexes.end(), [](uint32_t playerA, uint32_t playerB) -> bool {
		return NetPlay.players[playerA].position < NetPlay.players[playerB].position;
	});
	for (auto player : playerIndexes)
	{
		if (player == selectedPlayer && !NetPlay.players[player].isSpectator)
		{
			return nullptr;
		}
		result->table->addRow(result->newPlayerStatsRow(player));
	}

	result->table->setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
		auto psParent = std::dynamic_pointer_cast<SpectatorStatsView>(psWidget->parent());
		ASSERT_OR_RETURN(, psParent != nullptr, "No parent");
		int oldWidth = psWidget->width();
		int y0 = SPEC_STATS_WINDOW_TITLE_HEIGHT;
		psWidget->setGeometry(0, y0, psParent->width(), std::min<int>(psParent->height() - (y0 + SPEC_STATS_WINDOW_BOTTOM_PADDING), psParent->table->idealHeight()));

		if (oldWidth != psWidget->width())
		{
			psParent->resizeTableColumnWidths();
		}
	}));

	// layout calculations for main form
	result->setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
		auto psStrongSelf = std::dynamic_pointer_cast<SpectatorStatsView>(psWidget->shared_from_this());
		ASSERT_OR_RETURN(, psStrongSelf != nullptr, "Null??");
		// update the width
		int newWidth = std::min<int>(pie_GetVideoBufferWidth() - 50, psStrongSelf->idealWidth());
		// update the height
		int newHeight = std::min<int>(pie_GetVideoBufferHeight() - 50, psStrongSelf->idealHeight());
		// update the x if necessary to ensure the entire form is visible
		int x0 = std::min(psWidget->x(), pie_GetVideoBufferWidth() - newWidth);
		// update the y if necessary to ensure the entire form is visible
		int y0 = std::min(psWidget->y(), pie_GetVideoBufferHeight() - newHeight);

		psWidget->setGeometry(x0, y0, newWidth, newHeight);
	}));

	return result;
}

std::pair<std::vector<size_t>, size_t> SpectatorStatsView::getMaxTableColumnDataWidths()
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

void SpectatorStatsView::resizeTableColumnWidths()
{
	auto maxColumnDataWidthsResult = getMaxTableColumnDataWidths();
	table->changeColumnWidths(maxColumnDataWidthsResult.first);
}

void SpectatorStatsView::geometryChanged()
{
	if (closeButton)
	{
		closeButton->callCalcLayout();
	}
	if (table)
	{
		table->callCalcLayout();
	}
}

int32_t SpectatorStatsView::idealWidth()
{
	ASSERT_OR_RETURN(0, table != nullptr, "Table is null");
	auto maxColumnDataWidthsResult = getMaxTableColumnDataWidths();
	return table->getTableWidthNeededForTotalColumnWidth(table->getNumColumns(), maxColumnDataWidthsResult.second);
}

int32_t SpectatorStatsView::idealHeight()
{
	return SPEC_STATS_WINDOW_TITLE_HEIGHT + ((table) ? table->idealHeight() : 0) + SPEC_STATS_WINDOW_BOTTOM_PADDING;
}
