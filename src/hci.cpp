/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2024  Warzone 2100 Project

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
 * @file hci.c
 *
 * Functions for the in game interface.
 * (Human Computer Interface - thanks to Alex for the file name).
 *
 * Obviously HCI should mean "Hellish Code Incoming" -- Toksyuryel.
 *
 */

#include <cstring>
#include <algorithm>

#include "lib/framework/frame.h"
#include "lib/framework/stdio_ext.h"
#include "lib/gamelib/gtime.h"
#include "lib/ivis_opengl/bitimage.h"
#include "lib/ivis_opengl/pieblitfunc.h"
#include "lib/ivis_opengl/piepalette.h"
#include "lib/ivis_opengl/ivisdef.h"
#include "lib/ivis_opengl/screen.h"
#include "lib/netplay/netplay.h"

#include "input/manager.h"
#include "action.h"
#include "lib/sound/audio_id.h"
#include "lib/widget/label.h"
#include "lib/widget/bar.h"
#include "lib/widget/button.h"
#include "lib/widget/editbox.h"
#include "screens/helpscreen.h"
#include "cheat.h"
#include "console.h"
#include "design.h"
#include "display.h"
#include "display3d.h"
#include "edit3d.h"
#include "game.h"
#include "hci.h"
#include "ingameop.h"
#include "intdisplay.h"
#include "intelmap.h"
#include "intorder.h"
#include "loadsave.h"
#include "loop.h"
#include "order.h"
#include "mission.h"
#include "multimenu.h"
#include "multiplay.h"
#include "multigifts.h"
#include "research.h"
#include "transporter.h"
#include "main.h"
#include "template.h"
#include "wrappers.h"
#include "keybind.h"
#include "qtscript.h"
#include "chat.h"
#include "radar.h"
#include "hci/build.h"
#include "hci/research.h"
#include "hci/manufacture.h"
#include "hci/commander.h"
#include "notifications.h"
#include "hci/groups.h"
#include "screens/chatscreen.h"
#include "screens/guidescreen.h"
#include "hci/quickchat.h"
#include "warzoneconfig.h"

// Empty edit window
static bool secondaryWindowUp = false;
// Chat dialog
static bool ChatDialogUp = false;

#define NUMRETBUTS	7 // Number of reticule buttons.

struct BUTSTATE
{
	UDWORD id;
	bool Enabled;
	bool Hidden;
};

struct BUTOFFSET
{
	SWORD x;
	SWORD y;
};

static BUTOFFSET ReticuleOffsets[NUMRETBUTS] =  	// Reticule button form relative positions.
{
	{43, 47},	// RETBUT_CANCEL,
	{47, 15},	// RETBUT_FACTORY,
	{81, 33},	// RETBUT_RESEARCH,
	{81, 69},	// RETBUT_BUILD,
	{47, 87},	// RETBUT_DESIGN,
	{13, 69},	// RETBUT_INTELMAP,
	{13, 33},	// RETBUT_COMMAND,
};

static BUTSTATE ReticuleEnabled[NUMRETBUTS] =  	// Reticule button enable states.
{
	{IDRET_CANCEL, false, false},
	{IDRET_MANUFACTURE, false, false},
	{IDRET_RESEARCH, false, false},
	{IDRET_BUILD, false, false},
	{IDRET_DESIGN, false, false},
	{IDRET_INTEL_MAP, false, false},
	{IDRET_COMMAND, false, false},
};

static UDWORD	keyButtonMapping = 0;
static bool ReticuleUp = false;
static bool Refreshing = false;

/***************************************************************************************/
/*                  Widget ID numbers                                                  */

#define IDPOW_FORM			100		// power bar form

/* Option screen IDs */
#define IDOPT_FORM			1000		// The option form
#define IDOPT_CLOSE			1006		// The close button
#define IDOPT_LABEL			1007		// The Option screen label
#define IDOPT_PLAYERFORM	1008		// The player button form
#define IDOPT_PLAYERLABEL	1009		// The player form label
#define IDOPT_PLAYERSTART	1010		// The first player button
#define IDOPT_PLAYEREND		1030		// The last possible player button
#define IDOPT_QUIT			1031		// Quit the game
#define IDOPT_DROID			1037		// The place droid button
#define IDOPT_STRUCT		1038		// The place struct button
#define IDOPT_FEATURE		1039		// The place feature button
#define IDOPT_IVISFORM		1043		// iViS engine form
#define IDOPT_IVISLABEL		1044		// iViS form label

#define	IDPROX_START		120000		// The first proximity button
#define	IDPROX_END		129999		// The last proximity button

#define PROX_BUTWIDTH		9
#define PROX_BUTHEIGHT		9
/***************************************************************************************/
/*                  Widget Positions                                                   */

/* Reticule positions */
#define RET_BUTWIDTH		25
#define RET_BUTHEIGHT		28

#define WIDG_SPEC_SCREEN_ORDER_START		65530
#define WIDG_SPEC_SCREEN_ORDER_END			65532

/* The widget screen */
std::shared_ptr<W_SCREEN> psWScreen = nullptr;
std::shared_ptr<W_HELP_OVERLAY_SCREEN> psUIHelpOverlayScreen = nullptr;

INTMODE intMode;

/* Status of the positioning for the object placement */
static enum _edit_pos_mode
{
	IED_NOPOS,
	IED_POS,
} editPosMode;

OBJECT_MODE objMode;
std::shared_ptr<BaseObjectsController> interfaceController = nullptr;
std::shared_ptr<GroupsForum> groupsUI = nullptr;

/* The current stats list being used by the stats screen */
static BASE_STATS		**ppsStatsList;

/* The selected builder on the object screen when the build screen is displayed */
static DROID *psSelectedBuilder;

/* The button ID of an objects stat on the stat screen if it is locked down */
static UDWORD			statID;

/* The stats for the current getStructPos */
static BASE_STATS		*psPositionStats;

/* Store a list of stats pointers from the main structure stats */
static STRUCTURE_STATS	**apsStructStatsList;

/* Store a list of Template pointers for Droids that can be built */
std::vector<DROID_TEMPLATE *>   apsTemplateList;
std::list<DROID_TEMPLATE>       localTemplates;

/* Store a list of Feature pointers for features to be placed on the map */
static FEATURE_STATS	**apsFeatureStatsList;

/* Store a list of component stats pointers for the design screen */
UDWORD			numComponent;
COMPONENT_STATS	**apsComponentList;
UDWORD			numExtraSys;
COMPONENT_STATS	**apsExtraSysList;

/* Flags to check whether the power bars are currently on the screen */
static bool				powerBarUp = false;

/* Update functions to be called at an interval */
struct IntUpdateFunc
{
public:
	typedef std::function<void ()> UpdateFunc;
public:
	IntUpdateFunc(const UpdateFunc& func, uint32_t intervalTicks = GAME_TICKS_PER_SEC)
	: func(func)
	, intervalTicks(intervalTicks)
	, realTimeLastCalled(0)
	{ }
	~IntUpdateFunc() = default;
	IntUpdateFunc(IntUpdateFunc&&) = default;
	IntUpdateFunc(const IntUpdateFunc&) = delete;
	void operator=(const IntUpdateFunc&) = delete;
public:
	void callIfNeeded()
	{
		if (realTime - realTimeLastCalled >= intervalTicks)
		{
			if (func) { func(); }
			realTimeLastCalled = realTime;
		}
	}
public:
	std::function<void ()> func;
	uint32_t intervalTicks = GAME_TICKS_PER_SEC;
private:
	uint32_t realTimeLastCalled = 0;
};
static std::vector<IntUpdateFunc> intUpdateFuncs;

/***************************************************************************************/
/*              Function Prototypes                                                    */

/* Add the stats widgets to the widget screen */
/* If psSelected != NULL it specifies which stat should be hilited */
static bool intAddDebugStatsForm(BASE_STATS **ppsStatsList, UDWORD numStats);

/* Add the build widgets to the widget screen */
static bool intAddBuild();
/* Add the manufacture widgets to the widget screen */
static bool intAddManufacture();
/* Add the research widgets to the widget screen */
static bool intAddResearch();
/* Add the command droid widgets to the widget screen */
static bool intAddCommand();

/* Stop looking for a structure location */
static void intStopStructPosition();

/******************Power Bar Stuff!**************/

/* Set the shadow for the PowerBar */
static void intRunPower();

//proximity display stuff
static void processProximityButtons(UDWORD id);

// count the number of selected droids of a type
static SDWORD intNumSelectedDroids(UDWORD droidType);

// In-Game Options button Widget

class W_INGAMEOPTIONS_BUTTON : public W_BUTTON
{
protected:
	W_INGAMEOPTIONS_BUTTON()
	{ }
	void initialize();
public:
	static std::shared_ptr<W_INGAMEOPTIONS_BUTTON> make();

	void display(int xOffset, int yOffset) override;
	std::string getTip() override;

	int getMarginX() const { return marginX; }
	int getMarginTop() const { return marginTop; }

private:
	uint8_t currentAlphaValue() const;
	bool isHighlighted() const;

private:
	int marginX = 16;
	int marginTop = 18;
};

/***************************GAME CODE ****************************/

struct RETBUTSTATS
{
	int downTime = 0;
	WzString filename;
	WzString filenameDown;
	std::string tip;
	bool flashing = false;
	int flashTime = 0;
	std::shared_ptr<W_BUTTON> button = nullptr;
	playerCallbackFunc callbackFunc = nullptr;
};
static RETBUTSTATS retbutstats[NUMRETBUTS];

static bool buttonIsClickable(uint16_t id)
{
	bool enabled = intCheckReticuleButEnabled(id);
	return !gamePaused() && enabled;
}

static bool buttonIsHighlighted(W_BUTTON *p)
{
	bool clickable = buttonIsClickable(p->id);
	return (p->getState() & WBUT_HIGHLIGHT) != 0 && clickable;
}

void setReticuleFlash(int ButId, bool flash)
{
	if (MissionResUp)
	{
		return;
	}
	if (flash != retbutstats[ButId].flashing)
	{
		retbutstats[ButId].flashing = flash;
		retbutstats[ButId].flashTime = 0;
	}
}

// set up the button's size & hit-testing based on the dimensions of the "normal" image
void setReticuleButtonDimensions(W_BUTTON &button, const WzString &filename)
{
	AtlasImageDef *image = nullptr;
	if (!filename.isEmpty())
	{
		image = iV_GetImage(filename);
	}
	else
	{
		ASSERT(IntImages->imageDefs.size() >= IMAGE_RETICULE_GREY, "IMAGE_RETICULE_GREY isn't in IntImages?");
		if (IntImages->imageDefs.size() >= IMAGE_RETICULE_GREY)
		{
			image = &(IntImages->imageDefs[IMAGE_RETICULE_GREY]);
		}
	}

	if (image)
	{
		// set the button width/height based on the "normal" image dimensions (preserving the current x, y)
		button.setGeometry(button.x(), button.y(), image->Width / 2, image->Height / 2);

		// add a custom hit-testing function that uses a tighter bounding ellipse
		button.setCustomHitTest([](const WIDGET *psWidget, int x, int y) -> bool {

			// determine center of ellipse contained within the bounding rect
			float centerX = ((psWidget->x()) + (psWidget->x() + psWidget->width())) / 2.f;
			float centerY = ((psWidget->y()) + (psWidget->y() + psWidget->height())) / 2.f;

			// determine semi-major axis + semi-minor axis
			float axisX = psWidget->width() / 2.f;
			float axisY = psWidget->height() / 2.f;

			// Srivatsan (https://math.stackexchange.com/users/13425/srivatsan), Check if a point is within an ellipse, URL (version: 2011-10-27): https://math.stackexchange.com/q/76463
			float partX = (((float)x - centerX) * ((float)x - centerX)) / (axisX * axisX); // ((x - centerX)^2) / ((axisX)^2)
			float partY = (((float)y - centerY) * ((float)y - centerY)) / (axisY * axisY); // ((y - centerY)^2) / ((axisY)^2)
			return partX + partY <= 1.f;
		});
	}
}

optional<std::string> getReticuleButtonDisplayFilename(int ButId)
{
	ASSERT_OR_RETURN(nullopt, (ButId >= 0) && (ButId < NUMRETBUTS), "Invalid ButId: %d", ButId);
	return retbutstats[ButId].filename.toUtf8();
}

void setReticuleStats(int ButId, std::string tip, std::string filename, std::string filenameDown, const playerCallbackFunc& callbackFunc)
{
	ASSERT_OR_RETURN(, (ButId >= 0) && (ButId < NUMRETBUTS), "Invalid ButId: %d", ButId);

	retbutstats[ButId].tip = tip;
	retbutstats[ButId].filename = WzString::fromUtf8(filename.c_str());
	retbutstats[ButId].filenameDown = WzString::fromUtf8(filenameDown.c_str());
	retbutstats[ButId].downTime = 0;
	retbutstats[ButId].flashing = false;
	retbutstats[ButId].flashTime = 0;
	retbutstats[ButId].callbackFunc = callbackFunc;
	ReticuleEnabled[ButId].Enabled = false;

	if (!retbutstats[ButId].button) // not quite set up yet
	{
		return;
	}

	setReticuleButtonDimensions(*retbutstats[ButId].button, retbutstats[ButId].filename);

	if (!retbutstats[ButId].tip.empty())
	{
		retbutstats[ButId].button->setTip(retbutstats[ButId].tip);
	}

	if (!retbutstats[ButId].filename.isEmpty())
	{
		ReticuleEnabled[ButId].Enabled = true;
		retbutstats[ButId].button->setState(0);
	}
}

void setReticulesEnabled(bool enabled)
{
	for (int i = 0; i < NUMRETBUTS; i++)
	{
		if (retbutstats[i].filename.isEmpty()) {
			continue;
		}

		ReticuleEnabled[i].Enabled = enabled;

		if (!retbutstats[i].button) {
			continue;
		}
		if (enabled)
		{
			retbutstats[i].button->unlock();
		}
		else
		{
			unsigned state = retbutstats[i].button->getState();
			retbutstats[i].button->setState(state | WBUT_LOCK);
		}
	}
}

static void intDisplayReticuleButton(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset)
{
	const int x = xOffset + psWidget->x();
	const int y = yOffset + psWidget->y();
	int DownTime = retbutstats[psWidget->UserData].downTime;
	bool flashing = retbutstats[psWidget->UserData].flashing;
	int flashTime = retbutstats[psWidget->UserData].flashTime;
	ASSERT_OR_RETURN(, psWidget->type == WIDG_BUTTON, "Not a button");
	W_BUTTON *psButton = (W_BUTTON *)psWidget;
	bool butDisabled = psButton->state & WBUT_DISABLE;

	if (retbutstats[psWidget->UserData].filename.isEmpty() && !butDisabled)
	{
		butDisabled = true;
		retbutstats[psWidget->UserData].button->setState(WBUT_DISABLE);
	}

	if (butDisabled)
	{
		if (psWidget->UserData != RETBUT_CANCEL)
		{
			iV_DrawImage2("image_reticule_grey.png", x, y, psWidget->width(), psWidget->height());
		}
		else
		{
			iV_DrawImage2(retbutstats[psWidget->UserData].filenameDown, x, y, psWidget->width(), psWidget->height());
		}
		return;
	}

	bool Down = psButton->state & (WBUT_DOWN | WBUT_CLICKLOCK);
	if (Down && buttonIsClickable(psButton->id))
	{
		if ((DownTime < 1) && (psWidget->UserData != RETBUT_CANCEL))
		{
			iV_DrawImage2("image_reticule_butdown.png", x, y, psWidget->width(), psWidget->height());
		}
		else
		{
			iV_DrawImage2(retbutstats[psWidget->UserData].filenameDown, x, y, psWidget->width(), psWidget->height());
		}
		DownTime++;
		flashing = false;	// stop the reticule from flashing if it was
	}
	else
	{
		if (flashing && !gamePaused())
		{
			if (((realTime / 250) % 2) != 0)
			{
				iV_DrawImage2(retbutstats[psWidget->UserData].filenameDown, x, y, psWidget->width(), psWidget->height());
				flashTime = 0;
			}
			else
			{
				iV_DrawImage2(retbutstats[psWidget->UserData].filename, x, y, psWidget->width(), psWidget->height());
			}
			flashTime++;
		}
		else
		{
			iV_DrawImage2(retbutstats[psWidget->UserData].filename, x, y, psWidget->width(), psWidget->height());
			DownTime = 0;
		}
	}

	bool highlighted = buttonIsHighlighted(psButton);
	if (highlighted)
	{
		if (psWidget->UserData == RETBUT_CANCEL)
		{
			iV_DrawImage2("image_cancel_hilight.png", x - 1, y - 1, psWidget->width() + 2, psWidget->height() + 2);
		}
		else
		{
			iV_DrawImage2("image_reticule_hilight.png", x - 1, y - 1, psWidget->width() + 2, psWidget->height() + 2);
		}
	}
	retbutstats[psWidget->UserData].flashTime = flashTime;
	retbutstats[psWidget->UserData].flashing = flashing;
	retbutstats[psWidget->UserData].downTime = DownTime;
}

#define REPLAY_ACTION_BUTTONS_PADDING 5
#define REPLAY_ACTION_BUTTONS_SPACING 5
#define REPLAY_ACTION_BUTTONS_IMAGE_SIZE 16
#define REPLAY_ACTION_BUTTONS_WIDTH (REPLAY_ACTION_BUTTONS_IMAGE_SIZE + (REPLAY_ACTION_BUTTONS_PADDING * 2))
#define REPLAY_ACTION_BUTTONS_HEIGHT REPLAY_ACTION_BUTTONS_WIDTH

constexpr size_t WZ_MAX_REPLAY_FASTFORWARD_TICKS = 3;

class ReplayControllerWidget : public W_FORM
{
public:
	ReplayControllerWidget() : W_FORM() {}
	~ReplayControllerWidget() { }

public:
	static std::shared_ptr<ReplayControllerWidget> make();

public:
	virtual void display(int xOffset, int yOffset) override;
	virtual void geometryChanged() override;
	virtual void screenSizeDidChange(int oldWidth, int oldHeight, int newWidth, int newHeight) override;
	virtual void run(W_CONTEXT *psContext) override;

private:
	class W_REPLAY_CONTROL_BUTTON : public W_BUTTON
	{
	public:
		W_REPLAY_CONTROL_BUTTON(W_BUTINIT const *init)
		: W_BUTTON(init)
		{ }
		W_REPLAY_CONTROL_BUTTON()
		: W_BUTTON()
		{ }

		~W_REPLAY_CONTROL_BUTTON()
		{
			if (pButtonImage)
			{
				delete pButtonImage;
			}
		}

	public:
		void display(int xOffset, int yOffset) override
		{
			W_BUTTON *psButton = this;

			int x0 = psButton->x() + xOffset;
			int y0 = psButton->y() + yOffset;
			int x1 = x0 + psButton->width();
			int y1 = y0 + psButton->height();

			bool isDown = (psButton->getState() & (WBUT_DOWN | WBUT_LOCK | WBUT_CLICKLOCK)) != 0;
			bool isDisabled = (psButton->getState() & WBUT_DISABLE) != 0;
			bool isHighlight = !isDisabled && ((psButton->getState() & WBUT_HIGHLIGHT) != 0);

			// Display the button.
			auto light_border = pal_RGBA(255, 255, 255, 100);
			auto fill_color = isDown || isDisabled ? pal_RGBA(10, 0, 70, 200) : pal_RGBA(25, 0, 110, 175);
			iV_ShadowBox(x0, y0, x1, y1, 0, isDown ? pal_RGBA(0,0,0,0) : light_border, isDisabled ? light_border : WZCOL_FORM_DARK, fill_color);

			// Highlight
			if (isHighlight)
			{
				iV_Box(x0 + 2, y0 + 2, x1 - 2, y1 - 2, WZCOL_FORM_HILITE);
			}

			// Outline
			switch (outline)
			{
				case OutlineState::None:
					break;
				case OutlineState::Double:
					iV_Box(x0 + 3, y0 + 3, x1 - 3, y1 - 3, pal_RGBA(218, 207, 255, 200));
					// fall-through
				case OutlineState::Single:
					iV_Box(x0 + 1, y0 + 1, x1 - 1, y1 - 1, pal_RGBA(218, 207, 255, 200));
					break;
			}

			// Display the image, if present
			int imageLeft = x1 - REPLAY_ACTION_BUTTONS_PADDING - REPLAY_ACTION_BUTTONS_IMAGE_SIZE;
			int imageTop = (psButton->y() + yOffset) + REPLAY_ACTION_BUTTONS_PADDING;

			if (pButtonImage)
			{
				iV_DrawImageAnisotropic(*(pButtonImage), Vector2i(imageLeft, imageTop), Vector2f(0,0), Vector2f(REPLAY_ACTION_BUTTONS_IMAGE_SIZE, REPLAY_ACTION_BUTTONS_IMAGE_SIZE), 0.f, WZCOL_WHITE);
			}
		}
	public:
		gfx_api::texture* pButtonImage = nullptr;
		enum class OutlineState {
			None,
			Single,
			Double
		};
		OutlineState outline = OutlineState::None;
	};

	enum class ReplayGameStatus
	{
		PLAY,
		PAUSE,
	};
	ReplayGameStatus getReplayGameStatus() const
	{
		ReplayGameStatus status = (!gameTimeIsStopped()) ? ReplayGameStatus::PLAY : ReplayGameStatus::PAUSE;
		// sanity check
		return status;
	}

	void setReplayGameStatus(ReplayGameStatus newStatus)
	{
		switch (newStatus)
		{
			case ReplayGameStatus::PLAY:
				setAudioPause(false);
				setConsolePause(false);
				if (gameTimeIsStopped())
				{
					gameTimeStart();
				}
				break;
			case ReplayGameStatus::PAUSE:
				if (!gameTimeIsStopped())
				{
					gameTimeStop();
				}
				setAudioPause(true);
				setConsolePause(true);
				break;
		}

		auto sharedThis = std::dynamic_pointer_cast<ReplayControllerWidget>(shared_from_this());
		widgScheduleTask([sharedThis](){
			sharedThis->refreshButtonStates();
		});
	}

	void toggleFastForward(bool reverseDirection = false)
	{
		auto currFastForward = getMaxFastForwardTicks();
		size_t newFastForward = 0;
		if (currFastForward == 0)
		{
			newFastForward = (!reverseDirection) ? 1 : WZ_MAX_REPLAY_FASTFORWARD_TICKS;
		}
		else if (currFastForward == 1)
		{
			newFastForward = (!reverseDirection) ? WZ_MAX_REPLAY_FASTFORWARD_TICKS : 0;
		}
		else if (reverseDirection && currFastForward == WZ_MAX_REPLAY_FASTFORWARD_TICKS)
		{
			newFastForward = 1;
		}
		setMaxFastForwardTicks(newFastForward, true);

		auto sharedThis = std::dynamic_pointer_cast<ReplayControllerWidget>(shared_from_this());
		widgScheduleTask([sharedThis](){
			sharedThis->refreshButtonStates();
		});
	}

	void refreshButtonStates()
	{
		auto status = getReplayGameStatus();
		switch (status)
		{
			case ReplayGameStatus::PLAY:
				// play button shoud be disabled
				playButton->setState(WBUT_DISABLE);
				// pause + fast-forward buttons should be enabled
				pauseButton->setState(0);
				break;
			case ReplayGameStatus::PAUSE:
				// play button should be enabled
				playButton->setState(0);
				// pause + fast-forward buttons should be disabled
				pauseButton->setState(WBUT_DISABLE);
				break;
		}

		// fast-forward state is based on getMaxFastForwardTicks
		auto maxFastForwardTicks = getMaxFastForwardTicks();
		if (maxFastForwardTicks > 0)
		{
			// fast-forward is enabled
			fastForwardButton->outline = (maxFastForwardTicks >= WZ_MAX_REPLAY_FASTFORWARD_TICKS) ? W_REPLAY_CONTROL_BUTTON::OutlineState::Double : W_REPLAY_CONTROL_BUTTON::OutlineState::Single;
		}
		else
		{
			// normal speed
			fastForwardButton->outline = W_REPLAY_CONTROL_BUTTON::OutlineState::None;
		}
		lastUpdateTime = realTime;
	}

private:
	std::shared_ptr<W_REPLAY_CONTROL_BUTTON> makeReplayActionButton(const char* imagePath)
	{
		auto button = std::make_shared<W_REPLAY_CONTROL_BUTTON>();
		button->style |= WBUT_SECONDARY;
		button->FontID = font_regular;
		button->pButtonImage = WZ_Notification_Image(imagePath).loadImageToTexture();
		button->setGeometry(0, 0, REPLAY_ACTION_BUTTONS_WIDTH, REPLAY_ACTION_BUTTONS_HEIGHT);
		return button;
	}

private:
	std::shared_ptr<W_LABEL>					titleLabel;
	std::shared_ptr<W_REPLAY_CONTROL_BUTTON>	pauseButton;
	std::shared_ptr<W_REPLAY_CONTROL_BUTTON>	playButton;
	std::shared_ptr<W_REPLAY_CONTROL_BUTTON>	fastForwardButton;
	UDWORD										lastUpdateTime = 0;
	UDWORD										autoStateRefreshInterval = GAME_TICKS_PER_SEC * 3;
};

std::shared_ptr<ReplayControllerWidget> ReplayControllerWidget::make()
{
	auto result = std::make_shared<ReplayControllerWidget>();

	// Add title text
	result->titleLabel = std::make_shared<W_LABEL>();
	result->titleLabel->setFont(font_regular_bold, WZCOL_FORM_TEXT);
	result->titleLabel->setString(WzString::fromUtf8(_("Replay")));
	result->titleLabel->setGeometry(REPLAY_ACTION_BUTTONS_SPACING, REPLAY_ACTION_BUTTONS_SPACING, result->titleLabel->getMaxLineWidth(), iV_GetTextLineSize(font_regular_bold));
	result->titleLabel->setCacheNeverExpires(true);
	result->attach(result->titleLabel);
	result->titleLabel->setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
		auto psParent = std::dynamic_pointer_cast<ReplayControllerWidget>(psWidget->parent());
		ASSERT_OR_RETURN(, psParent != nullptr, "No parent");
		int x0 = psParent->width() - (psWidget->width() + REPLAY_ACTION_BUTTONS_SPACING);
		psWidget->setGeometry(x0, REPLAY_ACTION_BUTTONS_SPACING, psWidget->width(), psWidget->height());
	}));

	int buttonsY0 = result->titleLabel->y() + result->titleLabel->height() + REPLAY_ACTION_BUTTONS_SPACING;

	// Add pauseButton button
	result->pauseButton = result->makeReplayActionButton("images/replay/pause.png");
	result->pauseButton->setTip(_("Pause"));
	result->attach(result->pauseButton);
	result->pauseButton->addOnClickHandler([](W_BUTTON& button) {
		auto psParent = std::dynamic_pointer_cast<ReplayControllerWidget>(button.parent());
		ASSERT_OR_RETURN(, psParent != nullptr, "No parent");
		psParent->setReplayGameStatus(ReplayGameStatus::PAUSE);
	});
	result->pauseButton->move(0, buttonsY0);
	result->pauseButton->setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
		int x0 = REPLAY_ACTION_BUTTONS_SPACING;
		psWidget->setGeometry(x0, psWidget->y(), psWidget->width(), psWidget->height());
	}));

	// Add "play" button
	result->playButton = result->makeReplayActionButton("images/replay/play.png");
	result->playButton->setTip(_("Resume"));
	result->attach(result->playButton);
	result->playButton->addOnClickHandler([](W_BUTTON& button) {
		auto psParent = std::dynamic_pointer_cast<ReplayControllerWidget>(button.parent());
		ASSERT_OR_RETURN(, psParent != nullptr, "No parent");
		psParent->setReplayGameStatus(ReplayGameStatus::PLAY);
	});
	result->playButton->move(0, buttonsY0);
	result->playButton->setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
		auto psParent = std::dynamic_pointer_cast<ReplayControllerWidget>(psWidget->parent());
		ASSERT_OR_RETURN(, psParent != nullptr, "No parent");
		int x0 = psParent->pauseButton->x() + psParent->pauseButton->width() + REPLAY_ACTION_BUTTONS_SPACING;
		psWidget->setGeometry(x0, psWidget->y(), psWidget->width(), psWidget->height());
	}));

	// Add "fast-forwards" button
	result->fastForwardButton = result->makeReplayActionButton("images/replay/fast_forward.png");
	result->fastForwardButton->setTip(_("Fast-Forward"));
	result->attach(result->fastForwardButton);
	result->fastForwardButton->addOnClickHandler([](W_BUTTON& button) {
		auto psParent = std::dynamic_pointer_cast<ReplayControllerWidget>(button.parent());
		ASSERT_OR_RETURN(, psParent != nullptr, "No parent");
		psParent->toggleFastForward(button.getOnClickButtonPressed() == WKEY_SECONDARY);
	});
	result->fastForwardButton->move(0, buttonsY0);
	result->fastForwardButton->setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
		auto psParent = std::dynamic_pointer_cast<ReplayControllerWidget>(psWidget->parent());
		ASSERT_OR_RETURN(, psParent != nullptr, "No parent");
		int x0 = psParent->playButton->x() + psParent->playButton->width() + REPLAY_ACTION_BUTTONS_SPACING;
		psWidget->setGeometry(x0, psWidget->y(), psWidget->width(), psWidget->height());
	}));

	// set overall width
	int neededWidth = (REPLAY_ACTION_BUTTONS_SPACING * 4) + result->pauseButton->width() + result->playButton->width() + result->fastForwardButton->width();
	neededWidth = std::max<int>(neededWidth, REPLAY_ACTION_BUTTONS_SPACING + result->titleLabel->width() + REPLAY_ACTION_BUTTONS_SPACING);
	int neededHeight = result->pauseButton->y() + result->pauseButton->height() + REPLAY_ACTION_BUTTONS_SPACING;
	result->setGeometry(result->x(), result->y(), neededWidth, neededHeight);

	result->refreshButtonStates();

	return result;
}

void ReplayControllerWidget::display(int xOffset, int yOffset)
{
	// currently, no-op
}

void ReplayControllerWidget::geometryChanged()
{
	if (titleLabel)
	{
		titleLabel->callCalcLayout();
	}
	if (pauseButton)
	{
		pauseButton->callCalcLayout();
	}
	if (playButton)
	{
		playButton->callCalcLayout();
	}
	if (fastForwardButton)
	{
		fastForwardButton->callCalcLayout();
	}
}

void ReplayControllerWidget::screenSizeDidChange(int oldWidth, int oldHeight, int newWidth, int newHeight)
{
	// call default implementation (which ultimately propagates to all children
	W_FORM::screenSizeDidChange(oldWidth, oldHeight, newWidth, newHeight);
}

void ReplayControllerWidget::run(W_CONTEXT *psContext)
{
	if (realTime - lastUpdateTime >= autoStateRefreshInterval)
	{
		// trigger update call
		auto replayController = std::dynamic_pointer_cast<ReplayControllerWidget>(shared_from_this());
		widgScheduleTask([replayController](){
			replayController->refreshButtonStates();
		});
		lastUpdateTime = realTime;
	}
}

static std::shared_ptr<W_SCREEN> replayOverlayScreen;

bool createReplayControllerOverlay()
{
	ASSERT(psWScreen != nullptr, "psWScreen is not initialized yet?");
	ASSERT_OR_RETURN(false, replayOverlayScreen == nullptr, "Already initialized");

	// Initialize the replay overlay screen
	replayOverlayScreen = W_SCREEN::make();
	replayOverlayScreen->psForm->hide();

	// Create the Replay Controller form
	auto replayControllerForm = ReplayControllerWidget::make();
	replayOverlayScreen->psForm->attach(replayControllerForm);

	// Position the Replay Controller form
	replayControllerForm->setCalcLayout([](WIDGET *psWidget) {
		auto psOptionsButton = std::dynamic_pointer_cast<W_INGAMEOPTIONS_BUTTON>(widgFormGetFromID(psWScreen->psForm, IDRET_OPTIONS));
		bool visibleOptionsbutton = (psOptionsButton != nullptr && psOptionsButton->visible());
		int x0 = screenWidth - (visibleOptionsbutton ? psOptionsButton->getMarginX() + psOptionsButton->width() : 0) - psWidget->width() - (REPLAY_ACTION_BUTTONS_SPACING * 2);
		int y0 = 0;
		psWidget->move(std::max<int>(0, x0), y0);
	});

	widgRegisterOverlayScreenOnTopOfScreen(replayOverlayScreen, psWScreen);

	return true;
}

/* Initialise the in game interface */
bool intInitialise()
{
	for (auto &i : ReticuleEnabled)
	{
		i.Hidden = false;
	}

	widgSetTipColour(WZCOL_TOOLTIP_TEXT);

	WidgSetAudio(WidgetAudioCallback, ID_SOUND_BUTTON_CLICK_3, ID_SOUND_SELECT, ID_SOUND_BUILD_FAIL);

	/* Create storage for Structures that can be built */
	apsStructStatsList = (STRUCTURE_STATS **)malloc(sizeof(STRUCTURE_STATS *) * MAXSTRUCTURES);

	/* Create storage for Templates that can be built */
	apsTemplateList.clear();

	/* Create storage for the feature list */
	apsFeatureStatsList = (FEATURE_STATS **)malloc(sizeof(FEATURE_STATS *) * MAXFEATURES);

	/* Create storage for the component list */
	apsComponentList = (COMPONENT_STATS **)malloc(sizeof(COMPONENT_STATS *) * MAXCOMPONENT);

	/* Create storage for the extra systems list */
	apsExtraSysList = (COMPONENT_STATS **)malloc(sizeof(COMPONENT_STATS *) * MAXEXTRASYS);

	psSelectedBuilder = nullptr;

	if (!intInitialiseGraphics())
	{
		debug(LOG_ERROR, "Failed to initialize interface graphics");
		return false;
	}

	psWScreen = W_SCREEN::make();

	if (GetGameMode() == GS_NORMAL)
	{
		if (!intAddPower())
		{
			debug(LOG_ERROR, "Couldn't create power Bar widget(Out of memory ?)");
			return false;
		}
	}

	/* Note the current screen state */
	intMode = INT_NORMAL;

	if (!bInTutorial)
	{
		for (int comp = 0; comp < numStructureStats; comp++)
		{
			if (asStructureStats[comp].type == REF_DEMOLISH)
			{
				for (auto &apStructTypeList : apStructTypeLists)
				{
					apStructTypeList[comp] = AVAILABLE;
				}
			}
		}
	}

	BuildController::resetShowFavorites();

	if (NETisReplay() && GetGameMode() == GS_NORMAL)
	{
		createReplayControllerOverlay();
	}

	intUpdateFuncs.emplace_back([]() {
		widgForEachOverlayScreen([](const std::shared_ptr<W_SCREEN> &pScreen, uint16_t order) -> bool {
			if (pScreen != nullptr && order >= WIDG_SPEC_SCREEN_ORDER_START && order <= WIDG_SPEC_SCREEN_ORDER_END)
			{
				if (!((realSelectedPlayer <= MAX_CONNECTED_PLAYERS && NetPlay.players[realSelectedPlayer].isSpectator)
					&& (selectedPlayer <= MAX_CONNECTED_PLAYERS && NetPlay.players[selectedPlayer].isSpectator)))
				{
					widgRemoveOverlayScreen(pScreen);
					return false;
				}
			}
			return true;
		});
	}, GAME_TICKS_PER_SEC + GAME_TICKS_PER_SEC);

	// refresh defaults for quick chat
	quickChatInitInGame();

	return true;
}


/* Shut down the in game interface */
void interfaceShutDown()
{
	intRemoveIntelMapNoAnim(); // always call to ensure overlay screen is destroyed

	if (replayOverlayScreen)
	{
		widgRemoveOverlayScreen(replayOverlayScreen);
		replayOverlayScreen = nullptr;
	}

	groupsUI = nullptr;

	psWScreen = nullptr;

	free(apsStructStatsList);
	apsTemplateList.clear();
	free(apsFeatureStatsList);
	free(apsComponentList);
	free(apsExtraSysList);
	psSelectedBuilder = nullptr;

	psWScreen = nullptr;
	apsStructStatsList = nullptr;
	apsFeatureStatsList = nullptr;
	apsComponentList = nullptr;
	apsExtraSysList = nullptr;

	//obviously!
	ReticuleUp = false;

	for (auto& i : retbutstats)
	{
		i = RETBUTSTATS();
	}

	shutdownChatScreen();
	closeGuideScreen();
	ChatDialogUp = false;

	bAllowOtherKeyPresses = true;
}

static bool IntRefreshPending = false;
static bool IntGroupsRefreshPending = false;

// Set widget refresh pending flag.
//
void intRefreshScreen()
{
	IntRefreshPending = true;
}

bool intIsRefreshing()
{
	return Refreshing;
}

void intRefreshGroupsUI()
{
	IntGroupsRefreshPending = true;
}

void intInformInterfaceObjectRemoved(const BASE_OBJECT *psObj)
{
	if (psSelectedBuilder == psObj)
	{
		debug(LOG_INFO, "Selected builder removed");
		psSelectedBuilder = nullptr;
	}
	// intDoScreenRefresh handles refreshing the backing stores for any open interfaceController
}

bool intAddRadarWidget()
{
	auto radarWidget = getRadarWidget();
	ASSERT_OR_RETURN(false, radarWidget != nullptr, "Failed to get radar widget?");
	psWScreen->psForm->attach(radarWidget, WIDGET::ChildZPos::Back);
	return true;
}

// see if a delivery point is selected
static FLAG_POSITION *intFindSelectedDelivPoint()
{
	ASSERT_OR_RETURN(nullptr, selectedPlayer < MAX_PLAYERS, "Not supported selectedPlayer: %" PRIu32 "", selectedPlayer);

	for (const auto& psFlag : apsFlagPosLists[selectedPlayer])
	{
		if (psFlag->selected && psFlag->type == POS_DELIVERY)
		{
			return psFlag;
		}
	}

	return nullptr;
}

// Refresh widgets once per game cycle if pending flag is set.
//
void intDoScreenRefresh()
{
	if (IntGroupsRefreshPending && getGroupButtonEnabled())
	{
		if (groupsUI)
		{
			groupsUI->updateData();
		}
		IntGroupsRefreshPending = false;
	}

	if (!IntRefreshPending)
	{
		return;
	}

	Refreshing = true;

	if (interfaceController != nullptr)
	{
		interfaceController->refresh();
		IntRefreshPending = false;
		Refreshing = false;
		return;
	}

	size_t          objMajor = 0, statMajor = 0;
	FLAG_POSITION	*psFlag;

	if ((intMode == INT_OBJECT ||
		 intMode == INT_STAT ||
		 intMode == INT_CMDORDER ||
		 intMode == INT_ORDER ||
		 intMode == INT_TRANSPORTER) &&
		widgGetFromID(psWScreen, IDOBJ_FORM) != nullptr &&
		widgGetFromID(psWScreen, IDOBJ_FORM)->visible())
	{
		bool StatsWasUp = false;
		bool OrderWasUp = false;

		// If the stats form is up then remove it, but remember that it was up.
		if ((intMode == INT_STAT) && widgGetFromID(psWScreen, IDSTAT_FORM) != nullptr)
		{
			StatsWasUp = true;
		}

		// store the current tab position
		if (widgGetFromID(psWScreen, IDOBJ_TABFORM) != nullptr)
		{
			objMajor = ((ListTabWidget *)widgGetFromID(psWScreen, IDOBJ_TABFORM))->currentPage();
		}
		if (StatsWasUp)
		{
			statMajor = ((ListTabWidget *)widgGetFromID(psWScreen, IDSTAT_TABFORM))->currentPage();
		}
		// now make sure the stats screen isn't up
		if (widgGetFromID(psWScreen, IDSTAT_FORM) != nullptr)
		{
			intRemoveStatsNoAnim();
		}

		// see if there was a delivery point being positioned
		psFlag = intFindSelectedDelivPoint();

		// see if the commander order screen is up
		if ((intMode == INT_CMDORDER) &&
			(widgGetFromID(psWScreen, IDORDER_FORM) != nullptr))
		{
			OrderWasUp = true;
		}

		// set the tabs again
		if (widgGetFromID(psWScreen, IDOBJ_TABFORM) != nullptr)
		{
			((ListTabWidget *)widgGetFromID(psWScreen, IDOBJ_TABFORM))->setCurrentPage(objMajor);
		}

		if (widgGetFromID(psWScreen, IDSTAT_TABFORM) != nullptr)
		{
			((ListTabWidget *)widgGetFromID(psWScreen, IDSTAT_TABFORM))->setCurrentPage(statMajor);
		}

		if (psFlag != nullptr)
		{
			// need to restart the delivery point position
			startDeliveryPosition(psFlag);
		}

		// make sure the commander order screen is in the right state
		if ((intMode == INT_CMDORDER) &&
			!OrderWasUp &&
			(widgGetFromID(psWScreen, IDORDER_FORM) != nullptr))
		{
			intRemoveOrderNoAnim();
			if (statID)
			{
				widgSetButtonState(psWScreen, statID, 0);
			}
		}
	}

	// Refresh the transporter interface.
	intRefreshTransporter();

	// Refresh the order interface.
	intRefreshOrder();

	Refreshing = false;

	IntRefreshPending = false;
}


//hides the power bar from the display
void intHidePowerBar()
{
	//only hides the power bar if the player has requested no power bar
	if (!powerBarUp)
	{
		if (widgGetFromID(psWScreen, IDPOW_POWERBAR_T))
		{
			widgHide(psWScreen, IDPOW_POWERBAR_T);
		}
	}
}

/* Reset the widget screen to just the reticule */
void intResetScreen(bool NoAnim, bool skipMissionResultScreen /*= false*/)
{
	if (getWidgetsStatus() == false)
	{
		NoAnim = true;
	}

	for (auto& i : retbutstats)
	{
		if (i.button && ReticuleUp)
		{
			i.button->unlock();
		}
	}

	auto startingIntMode = intMode;

	switch (intMode)
	{
	case INT_DESIGN:
		gInputManager.contexts().popState();
		triggerEvent(TRIGGER_DESIGN_QUIT);
		break;
	case INT_INTELMAP:
		if (!bMultiPlayer)
		{
			gameTimeStart();
		}
		break;
	default:
		break;
	}

	intStopStructPosition();
	if (NoAnim)
	{
		intRemoveOrderNoAnim();
		intRemoveStatsNoAnim();
		intRemoveObjectNoAnim();
		intCloseInGameOptionsNoAnim();
		intCloseMultiMenuNoAnim();
		intRemoveIntelMapNoAnim();
		intRemoveTransNoAnim(true);
	}
	else
	{
		intRemoveOrder();
		intRemoveStats();
		intRemoveObject();
		intCloseMultiMenu();
		intRemoveIntelMap();
		intRemoveTrans(true);
	}
	if ((intMode == INT_MISSIONRES) && !skipMissionResultScreen)
	{
		intRemoveMissionResultNoAnim();
	}
	if (startingIntMode == INT_DESIGN)
	{
		intRemoveDesign();
	}
	intHidePowerBar();

	if (interfaceController)
	{
		interfaceController->prepareToClose();
	}
	interfaceController = nullptr;
	setSecondaryWindowUp(false);
	intMode = INT_NORMAL;
	//clearSelelection() sets IntRefreshPending = true by calling intRefreshScreen() but if we're doing this then we won't need to refresh - hopefully!
	IntRefreshPending = false;
	intShowGroupSelectionMenu();
}

void intOpenDebugMenu(OBJECT_TYPE id)
{
	if (gamePaused())
	{
		// All menu tabs will not work if they are opened, and the forms will fail to cleanup properly, if the game is paused.
		return;
	}

	intResetScreen(true);
	switch (id)
	{
	case OBJ_DROID:
		apsTemplateList.clear();
		for (auto &localTemplate : localTemplates)
		{
			apsTemplateList.push_back(&localTemplate);
		}
		ppsStatsList = (BASE_STATS **)&apsTemplateList[0]; // FIXME Ugly cast, and is undefined behaviour (strict-aliasing violation) in C/C++.
		objMode = IOBJ_DEBUG_DROID;
		intAddDebugStatsForm(ppsStatsList, apsTemplateList.size());
		intMode = INT_EDITSTAT;
		editPosMode = IED_NOPOS;
		break;
	case OBJ_STRUCTURE:
		for (unsigned i = 0; i < std::min<unsigned>(numStructureStats, MAXSTRUCTURES); ++i)
		{
			apsStructStatsList[i] = asStructureStats + i;
		}
		ppsStatsList = (BASE_STATS **)apsStructStatsList;
		objMode = IOBJ_DEBUG_STRUCTURE;
		intAddDebugStatsForm(ppsStatsList, std::min<unsigned>(numStructureStats, MAXSTRUCTURES));
		intMode = INT_EDITSTAT;
		editPosMode = IED_NOPOS;
		break;
	case OBJ_FEATURE:
		for (unsigned i = 0, end = std::min<unsigned>(asFeatureStats.size(), MAXFEATURES); i < end; ++i)
		{
			apsFeatureStatsList[i] = &asFeatureStats[i];
		}
		ppsStatsList = (BASE_STATS **)apsFeatureStatsList;
		intAddDebugStatsForm(ppsStatsList, std::min<unsigned>(asFeatureStats.size(), MAXFEATURES));
		intMode = INT_EDITSTAT;
		editPosMode = IED_NOPOS;
		break;
	default:
		ASSERT(false, "Unknown debug menu code");
		break;
	}
}

/* Process return codes from the object placement stats screen */
static void intProcessEditStats(UDWORD id)
{
	if (id >= IDSTAT_START && id <= IDSTAT_END)
	{
		/* Clicked on a stat button - need to look for a location for it */
		psPositionStats = ppsStatsList[id - IDSTAT_START];
		if (psPositionStats->hasType(STAT_TEMPLATE))
		{
			FLAG_POSITION debugMenuDroidDeliveryPoint;
			// Placing a droid from the debug menu, set up the flag. (This would probably be safe to do, even if we're placing something else.)
			debugMenuDroidDeliveryPoint.factoryType = REPAIR_FLAG;
			debugMenuDroidDeliveryPoint.factoryInc = 0;
			debugMenuDroidDeliveryPoint.player = selectedPlayer;
			debugMenuDroidDeliveryPoint.selected = false;
			startDeliveryPosition(&debugMenuDroidDeliveryPoint);
		}
		else
		{
			intStartStructPosition(psPositionStats);
		}
		editPosMode = IED_POS;
	}
	else if (id == IDSTAT_CLOSE)
	{
		intRemoveStats();
		intStopStructPosition();
		intMode = INT_NORMAL;
		objMode = IOBJ_NONE;
	}
}

static void reticuleCallback(int retbut)
{
	if (retbutstats[retbut].callbackFunc)
	{
		retbutstats[retbut].callbackFunc(selectedPlayer);
	}
}

/* Run the widgets for the in game interface */
INT_RETVAL intRunWidgets()
{
	bool			quitting = false;

	if (bLoadSaveUp && runLoadSave(true) && strlen(sRequestResult) > 0)
	{
		if (bRequestLoad)
		{
			loopMissionState = LMS_LOADGAME;
			sstrcpy(saveGameName, sRequestResult);
		}
		else
		{
			if (saveGame(sRequestResult, GTYPE_SAVE_START))
			{
				char msg[256] = {'\0'};

				sstrcpy(msg, _("GAME SAVED: "));
				sstrcat(msg, savegameWithoutExtension(saveGameName));
				addConsoleMessage(msg, LEFT_JUSTIFY, NOTIFY_MESSAGE);

				if (widgGetFromID(psWScreen, IDMISSIONRES_SAVE))
				{
					widgDelete(psWScreen, IDMISSIONRES_SAVE);
				}
			}
			else
			{
				ASSERT(false, "intRunWidgets: saveGame Failed");
				deleteSaveGame_classic(sRequestResult);
			}
		}
	}

	if (MissionResUp)
	{
		intRunMissionResult();
	}

	/* Run the current set of widgets */
	std::vector<unsigned> retIDs;
	if (!bLoadSaveUp)
	{
		WidgetTriggers const &triggers = widgRunScreen(psWScreen);
		for (const auto &trigger: triggers)
		{
			retIDs.push_back(trigger.widget->id);
		}
	}

	/* Run any periodic update functions */
	for (auto& update : intUpdateFuncs)
	{
		update.callIfNeeded();
	}

	/* We may need to trigger widgets with a key press */
	if (keyButtonMapping)
	{
		/* Set the appropriate id */
		retIDs.push_back(keyButtonMapping);

		/* Clear it so it doesn't trigger next time around */
		keyButtonMapping = 0;
	}

	/* Extra code for the power bars to deal with the shadow */
	if (powerBarUp)
	{
		intRunPower();
	}

	if (OrderUp)
	{
		intRunOrder();
	}

	/* Extra code for the design screen to deal with the shadow bar graphs */
	if (intMode == INT_DESIGN)
	{
		setSecondaryWindowUp(true);
		intRunDesign();
	}
	else if (intMode == INT_INTELMAP)
	{
		intRunIntelMap();
	}

	// Deal with any clicks.
	for (unsigned int retID : retIDs)
	{
		if (retID >= IDPROX_START && retID <= IDPROX_END)
		{
			processProximityButtons(retID);
			return INT_NONE;
		}

		bool selectedPlayerIsSpectator = (bMultiPlayer && NetPlay.players[selectedPlayer].isSpectator) || (selectedPlayer >= MAX_PLAYERS);

		switch (retID)
		{
		/*****************  Reticule buttons  *****************/

		case IDRET_COMMAND:
			if (selectedPlayerIsSpectator)
			{
				break;
			}
			intResetScreen(true);
			widgSetButtonState(psWScreen, IDRET_COMMAND, WBUT_CLICKLOCK);
			intAddCommand();
			reticuleCallback(RETBUT_COMMAND);
			break;

		case IDRET_BUILD:
			if (selectedPlayerIsSpectator)
			{
				break;
			}
			intResetScreen(true);
			widgSetButtonState(psWScreen, IDRET_BUILD, WBUT_CLICKLOCK);
			intAddBuild();
			reticuleCallback(RETBUT_BUILD);
			break;

		case IDRET_MANUFACTURE:
			if (selectedPlayerIsSpectator)
			{
				break;
			}
			intResetScreen(true);
			widgSetButtonState(psWScreen, IDRET_MANUFACTURE, WBUT_CLICKLOCK);
			intAddManufacture();
			reticuleCallback(RETBUT_FACTORY);
			break;

		case IDRET_RESEARCH:
			if (selectedPlayerIsSpectator)
			{
				break;
			}
			intResetScreen(true);
			widgSetButtonState(psWScreen, IDRET_RESEARCH, WBUT_CLICKLOCK);
			intAddResearch();
			reticuleCallback(RETBUT_RESEARCH);
			break;

		case IDRET_INTEL_MAP:
			// check if RMB was clicked
			if (widgGetButtonKey_DEPRECATED(psWScreen) == WKEY_SECONDARY)
			{
				//set the current message to be the last non-proximity message added
				setCurrentMsg();
				setMessageImmediate(true);
			}
			else
			{
				psCurrentMsg = nullptr;
			}
			addIntelScreen();
			reticuleCallback(RETBUT_INTELMAP);
			break;

		case IDRET_DESIGN:
			if (selectedPlayerIsSpectator)
			{
				break;
			}
			intResetScreen(true);
			widgSetButtonState(psWScreen, IDRET_DESIGN, WBUT_CLICKLOCK);
			/*add the power bar - for looks! */
			intShowPowerBar();
			intAddDesign(false);
			intMode = INT_DESIGN;
			gInputManager.contexts().pushState();
			gInputManager.contexts().makeAllInactive();
			reticuleCallback(RETBUT_DESIGN);
			triggerEvent(TRIGGER_MENU_DESIGN_UP);
			break;

		case IDRET_CANCEL:
			intResetScreen(false);
			psCurrentMsg = nullptr;
			reticuleCallback(RETBUT_CANCEL);
			break;

		/*Transporter button pressed - OFFWORLD Mission Maps ONLY *********/
		case IDTRANTIMER_BUTTON:
			addTransporterInterface(nullptr, true);
			break;

		case IDTRANS_LAUNCH:
			processLaunchTransporter();
			break;

		/* Catch the quit button here */
		case INTINGAMEOP_POPUP_QUIT:
		case IDMISSIONRES_QUIT:			// mission quit
		case INTINGAMEOP_QUIT:			// esc quit confirm
		case IDOPT_QUIT:						// options screen quit
			intCloseInGameOptions(false, false);
			intResetScreen(false);
			quitting = true;
			break;

		/* Default case passes remaining IDs to appropriate function */
		default:
			switch (intMode)
			{
			case INT_EDITSTAT:
				intProcessEditStats(retID);
				break;
			case INT_CMDORDER:
				if (objMode == IOBJ_COMMAND && retID != IDOBJ_TABFORM)
				{
					intProcessOrder(retID);
				}
				break;
			case INT_ORDER:
				intProcessOrder(retID);
				break;
			case INT_MISSIONRES:
				intProcessMissionResult(retID);
				break;
			case INT_INGAMEOP:
				intProcessInGameOptions(retID);
				break;
			case INT_MULTIMENU:
				// no-op here
				break;
			case INT_DESIGN:
				intProcessDesign(retID);
				break;
			case INT_INTELMAP:
				// no-op here
				break;
			case INT_TRANSPORTER:
				intProcessTransporter(retID);
				break;
			case INT_STAT:
			case INT_OBJECT:
			case INT_NORMAL:
				break;
			default:
				ASSERT(false, "intRunWidgets: unknown interface mode");
				break;
			}
			break;
		}
	}

	if (!quitting && retIDs.empty())
	{
		if ((intMode == INT_OBJECT || intMode == INT_STAT) && objMode == IOBJ_BUILDSEL)
		{
			// See if a position for the structure has been found
			Vector2i pos, pos2;
			if (found3DBuildLocTwo(pos, pos2))
			{
				// Send the droid off to build the structure assuming the droid
				// can get to the location chosen

				// Set the droid order
				if (intNumSelectedDroids(DROID_CONSTRUCT) == 0
					&& intNumSelectedDroids(DROID_CYBORG_CONSTRUCT) == 0
					&& psSelectedBuilder != nullptr && psSelectedBuilder->isConstructionDroid())
				{
					orderDroidStatsTwoLocDir(psSelectedBuilder, DORDER_LINEBUILD, (STRUCTURE_STATS *)psPositionStats, pos.x, pos.y, pos2.x, pos2.y, getBuildingDirection(), ModeQueue);
				}
				else
				{
					orderSelectedStatsTwoLocDir(selectedPlayer, DORDER_LINEBUILD, (STRUCTURE_STATS *)psPositionStats, pos.x, pos.y, pos2.x, pos2.y, getBuildingDirection(), ctrlShiftDown());
				}
				if (!quickQueueMode)
				{
					// Clear the object screen, only if we aren't immediately building something else
					intResetScreen(false);
				}

			}
			else if (found3DBuilding(pos))	//found building
			{
				//check droid hasn't died
				if ((psSelectedBuilder == nullptr) ||
					!psSelectedBuilder->died)
				{
					// Send the droid off to build the structure assuming the droid
					// can get to the location chosen

					// Don't allow derrick to be built on burning ground.
					if (((STRUCTURE_STATS *)psPositionStats)->type == REF_RESOURCE_EXTRACTOR)
					{
						if (fireOnLocation(pos.x, pos.y))
						{
							AddDerrickBurningMessage();
						}
					}
					// Set the droid order
					if (intNumSelectedDroids(DROID_CONSTRUCT) == 0
						&& intNumSelectedDroids(DROID_CYBORG_CONSTRUCT) == 0
						&& psSelectedBuilder != nullptr)
					{
						orderDroidStatsLocDir(psSelectedBuilder, DORDER_BUILD, (STRUCTURE_STATS *)psPositionStats, pos.x, pos.y, getBuildingDirection(), ModeQueue);
					}
					else
					{
						orderSelectedStatsLocDir(selectedPlayer, DORDER_BUILD, (STRUCTURE_STATS *)psPositionStats, pos.x, pos.y, getBuildingDirection(), ctrlShiftDown());
					}
				}

				if (!quickQueueMode)
				{
					// Clear the object screen, only if we aren't immediately building something else
					intResetScreen(false);
				}
			}
			if (buildState == BUILD3D_NONE)
			{
				intResetScreen(false);
			}
		}
		else if (intMode == INT_EDITSTAT && editPosMode == IED_POS)
		{
			/* Directly positioning some type of object */
			FLAG_POSITION flag;
			Vector2i pos = {INT32_MAX, INT32_MAX}, pos2 = {INT32_MAX, INT32_MAX};
			Vector2i size = {1, 1};
			STRUCTURE_TYPE type = REF_WALL;
			if (sBuildDetails.psStats && (found3DBuildLocTwo(pos, pos2) || found3DBuilding(pos)))
			{
				if (auto stats = castStructureStats(sBuildDetails.psStats))
				{
					size = stats->size(playerPos.r.y);
					type = stats->type;
				}
				if (pos2.x == INT32_MAX)
				{
					pos2 = pos;
				}
			}
			else if (deliveryReposFinished(&flag))
			{
				pos2 = pos = flag.coords;
			}

			if (pos.x != INT32_MAX)
			{
				auto lb = calcLineBuild(size, type, pos, pos2);
				for (int i = 0; i < lb.count; ++i)
				{
					pos = lb[i];
					/* See what type of thing is being put down */
					auto psBuilding = castStructureStats(psPositionStats);
					if (psBuilding && selectedPlayer < MAX_PLAYERS)
					{

						if (psBuilding->type == REF_DEMOLISH)
						{
							MAPTILE *psTile = mapTile(map_coord(pos.x), map_coord(pos.y));
							FEATURE *psFeature = (FEATURE *)psTile->psObject;
							STRUCTURE *psStructure = (STRUCTURE *)psTile->psObject;

							if (psStructure && psTile->psObject->type == OBJ_STRUCTURE)
							{
								//removeStruct(psStructure, true);
								SendDestroyStructure(psStructure);
							}
							else if (psFeature && psTile->psObject->type == OBJ_FEATURE)
							{
								removeFeature(psFeature);
							}
						}
						else
						{
							STRUCTURE tmp(generateNewObjectId(), selectedPlayer);
							STRUCTURE *psStructure = &tmp;
							tmp.state = SAS_NORMAL;
							tmp.pStructureType = (STRUCTURE_STATS *)psPositionStats;
							tmp.pos = {pos.x, pos.y, map_Height(pos.x, pos.y) + world_coord(1) / 10};

							// In multiplayer games be sure to send a message to the
							// other players, telling them a new structure has been
							// placed.
							SendBuildFinished(psStructure);
							// Send a text message to all players, notifying them of
							// the fact that we're cheating ourselves a new
							// structure.
							std::string msg = astringf(_("Player %u is cheating (debug menu) him/herself a new structure: %s."),
										selectedPlayer, getLocalizedStatsName(psStructure->pStructureType));
							sendInGameSystemMessage(msg.c_str());
							Cheated = true;
						}
					}
					else if (psPositionStats->hasType(STAT_FEATURE))
					{
						// Send a text message to all players, notifying them of the fact that we're cheating ourselves a new feature.
						std::string msg = astringf(_("Player %u is cheating (debug menu) him/herself a new feature: %s."),
									selectedPlayer, getLocalizedStatsName(psPositionStats));
						sendInGameSystemMessage(msg.c_str());
						Cheated = true;
						// Notify the other hosts that we've just built ourselves a feature
						//sendMultiPlayerFeature(result->psStats->subType, result->pos.x, result->pos.y, result->id);
						sendMultiPlayerFeature(((FEATURE_STATS *)psPositionStats)->ref, pos.x, pos.y, generateNewObjectId());
					}
					else if (psPositionStats->hasType(STAT_TEMPLATE))
					{
						std::string msg;
						DROID *psDroid = buildDroid((DROID_TEMPLATE *)psPositionStats, pos.x, pos.y, selectedPlayer, false, nullptr);
						cancelDeliveryRepos();
						if (psDroid)
						{
							addDroid(psDroid, apsDroidLists);

							// Send a text message to all players, notifying them of
							// the fact that we're cheating ourselves a new droid.
							msg = astringf(_("Player %u is cheating (debug menu) him/herself a new droid: %s."), selectedPlayer, psDroid->aName);

							triggerEventDroidBuilt(psDroid, nullptr);
						}
						else
						{
							// Send a text message to all players, notifying them of
							// the fact that we're cheating ourselves a new droid.
							msg = astringf(_("Player %u is cheating (debug menu) him/herself a new droid."), selectedPlayer);
						}
						sendInGameSystemMessage(msg.c_str());
						Cheated = true;
					}
					if (!quickQueueMode)
					{
						editPosMode = IED_NOPOS;
					}
				}
			}
		}
	}

	INT_RETVAL retCode = INT_NONE;
	if (quitting)
	{
		retCode = INT_QUIT;
	}
	else if (!retIDs.empty() || intMode == INT_EDIT || intMode == INT_MISSIONRES || isMouseOverSomeWidget(psWScreen))
	{
		retCode = INT_INTERCEPT;
	}

	const DebugInputManager& dbgInputManager = gInputManager.debugManager();
	if ((testPlayerHasLost() || testPlayerHasWon()) && !bMultiPlayer && intMode != INT_MISSIONRES && !dbgInputManager.debugMappingsAllowed())
	{
		debug(LOG_ERROR, "PlayerHasLost Or Won");
		intResetScreen(true);
		retCode = INT_QUIT;
	}
	return retCode;
}

/* Set the shadow for the PowerBar */
static void intRunPower()
{
	UDWORD				highlightedStatID;
	BASE_STATS			*psStat;
	UDWORD				quantity = 0;

	/* Find out which button was hilited */
	highlightedStatID = widgGetMouseOver(psWScreen);
	if (highlightedStatID >= IDSTAT_START && highlightedStatID <= IDSTAT_END)
	{
		psStat = ppsStatsList[highlightedStatID - IDSTAT_START];
		if (psStat->hasType(STAT_STRUCTURE))
		{
			//get the structure build points
			quantity = ((STRUCTURE_STATS *)apsStructStatsList[highlightedStatID -
			            IDSTAT_START])->powerToBuild;
		}
		else if (psStat->hasType(STAT_TEMPLATE))
		{
			//get the template build points
			quantity = calcTemplatePower(apsTemplateList[highlightedStatID - IDSTAT_START]);
		}

		//update the power bars
		intSetShadowPower(quantity);
	}
}

/* Set the map view point to the world coordinates x,y */
void intSetMapPos(UDWORD x, UDWORD y)
{
	setViewPos(map_coord(x), map_coord(y), true);
}


/* Sync the interface to an object */
// If psObj is NULL then reset interface displays.
//
// There should be two version of this function, one for left clicking and one got right.
//
void intObjectSelected(BASE_OBJECT *psObj)
{
	if (psObj)
	{
		setWidgetsStatus(true);
		switch (psObj->type)
		{
		case OBJ_DROID:
			if (!OrderUp)
			{
				intResetScreen(false);
				// changed to a BASE_OBJECT to accommodate the factories - AB 21/04/99
				intAddOrder(psObj);
				intMode = INT_ORDER;
			}
			else
			{
				// changed to a BASE_OBJECT to accommodate the factories - AB 21/04/99
				intAddOrder(psObj);
			}
			break;

		case OBJ_STRUCTURE:
			if (objMode != IOBJ_DEMOLISHSEL)
			{
				auto structure = castStructure(psObj);

				if (structure->status == SS_BUILT)
				{
					if (structure->isFactory())
					{
						intAddManufacture();
						break;
					}
					else if (structure->pStructureType->type == REF_RESEARCH)
					{
						intAddResearch();
						break;
					}
				}
			}
			intResetScreen(false);
			break;
		default:
			break;
		}
	}
	else
	{
		intResetScreen(false);
	}
}


/**
 * Start location selection, where the builder will build the structure
 */
void intStartConstructionPosition(DROID *builder, STRUCTURE_STATS *structure)
{
	psSelectedBuilder = builder;
	psPositionStats = structure;
	intStartStructPosition(structure);
}


/* Start looking for a structure location */
void intStartStructPosition(BASE_STATS *psStats)
{
	init3DBuilding(psStats, nullptr, nullptr);
}



/* Stop looking for a structure location */
static void intStopStructPosition()
{
	psSelectedBuilder = nullptr;

	/* Check there is still a struct position running */
	if ((intMode == INT_OBJECT || intMode == INT_STAT) && objMode == IOBJ_BUILDSEL)
	{
		objMode = IOBJ_NONE;
	}
	kill3DBuilding();
}


/* Display the widgets for the in game interface */
void intDisplayWidgets()
{
	/*draw the background for the design screen and the Intelligence screen*/
	if (intMode == INT_DESIGN || intMode == INT_INTELMAP)
	{
		// When will they ever learn!!!!
		if (!bMultiPlayer)
		{
			if (!bInTutorial)
			{
				screen_RestartBackDrop();
			}

			// We need to add the console messages to the intelmap for the tutorial so that it can display messages
			if ((intMode == INT_DESIGN) || (bInTutorial && intMode == INT_INTELMAP))
			{
				displayConsoleMessages();
			}
		}
	}

	bool desiredRadarVisibility = false;
	if (!gameUpdatePaused())
	{
		desiredRadarVisibility = radarVisible();

		cleanupOldBeaconMessages();

		/* Ensure that any text messages are displayed at bottom of screen */
		displayConsoleMessages();
	}

	auto radarWidget = getRadarWidget();
	if (radarWidget && (desiredRadarVisibility != radarWidget->visible()))
	{
		(desiredRadarVisibility) ? radarWidget->show() : radarWidget->hide();
	}

	widgDisplayScreen(psWScreen);

	if (bLoadSaveUp)
	{
		displayLoadSave();
	}
}

void intShowWidgetHelp()
{
	ASSERT_OR_RETURN(, psWScreen != nullptr, "psWScreen is null?");

	if (!psUIHelpOverlayScreen)
	{
		// Initialize the help overlay screen
		psUIHelpOverlayScreen = W_HELP_OVERLAY_SCREEN::make(
			// close handler
			[](std::shared_ptr<W_HELP_OVERLAY_SCREEN>){
				psUIHelpOverlayScreen.reset();

				if (!bMultiPlayer || !NetPlay.bComms)
				{
					if (gamePaused())
					{
						// unpause game

						/* Get it going again */
						setGamePauseStatus(false);
						setConsolePause(false);
						setScriptPause(false);
						setAudioPause(false);
						setScrollPause(false);

						/* And start the clock again */
						gameTimeStart();
					}
				}
			}
		);
	}

	if (!bMultiPlayer || !NetPlay.bComms)
	{
		if (!gamePaused())
		{
			// pause game
			setGamePauseStatus(true);
			setConsolePause(true);
			setScriptPause(true);
			setAudioPause(true);
			setScrollPause(true);

			/* And stop the clock */
			gameTimeStop();
		}
	}

	psUIHelpOverlayScreen->setHelpFromWidgets(psWScreen->psForm);
	widgRegisterOverlayScreenOnTopOfScreen(psUIHelpOverlayScreen, psWScreen);
}

bool intHelpOverlayIsUp()
{
	return psUIHelpOverlayScreen != nullptr;
}

/* Tell the interface when the screen has been resized */
void intScreenSizeDidChange(int oldWidth, int oldHeight, int newWidth, int newHeight)
{
	if (psWScreen == nullptr) return;
	psWScreen->screenSizeDidChange(oldWidth, oldHeight, newWidth, newHeight);
}

/* Are we in build select mode*/
bool intBuildSelectMode()
{
	return (objMode == IOBJ_BUILDSEL);
}

/* Are we in demolish select mode*/
bool intDemolishSelectMode()
{
	return (objMode == IOBJ_DEMOLISHSEL);
}

//Written to allow demolish order to be added to the queuing system
void intDemolishCancel()
{
	if (objMode == IOBJ_DEMOLISHSEL)
	{
		objMode = IOBJ_NONE;
	}
}

/* Tell the interface a research facility has completed a topic */
void intResearchFinished(STRUCTURE *psBuilding)
{
	ASSERT_OR_RETURN(, psBuilding != nullptr, "Invalid structure pointer");

	// just do a screen refresh
	intRefreshScreen();
}

void intAlliedResearchChanged()
{
	if ((intMode == INT_OBJECT || intMode == INT_STAT) && objMode == IOBJ_RESEARCH)
	{
		intRefreshScreen();
	}
}

void intGroupsChanged(optional<UBYTE> selectedGroup)
{
	if (groupsUI)
	{
		intRefreshGroupsUI();
		if (selectedGroup.has_value() && selectedGroup.value() != UBYTE_MAX)
		{
			groupsUI->updateSelectedGroup(selectedGroup.value());
		}
	}
}

void intGroupDamaged(UBYTE group, uint64_t additionalDamage, bool unitKilled)
{
	if (groupsUI)
	{
		groupsUI->addGroupDamageForCurrentTick(group, additionalDamage, unitKilled);
	}
}

void intCommanderGroupChanged(const DROID *psCommander) // psCommander may be null!
{
	intGroupsChanged(); // just trigger full group change event
}

void intCommanderGroupDamaged(const DROID *psCommander, uint64_t additionalDamage, bool unitKilled)
{
	if (groupsUI)
	{
		groupsUI->addCommanderGroupDamageForCurrentTick(psCommander, additionalDamage, unitKilled);
	}
}

bool intShowGroupSelectionMenu()
{
	bool isSpectator = (bMultiPlayer && selectedPlayer < NetPlay.players.size() && NetPlay.players[selectedPlayer].isSpectator);
	if (getGroupButtonEnabled() && !isSpectator)
	{
		GroupsForum* groupsUIFormWidg = (GroupsForum*)widgGetFromID(psWScreen, IDOBJ_GROUP);
		if (!groupsUIFormWidg)
		{
			if (!groupsUI)
			{
				groupsUI = GroupsForum::make();
			}
			psWScreen->psForm->attach(groupsUI);
		}

		groupsUI->show();
	}
	else
	{
		widgDelete(psWScreen, IDOBJ_GROUP);
		groupsUI.reset();
	}
	return true;
}

// MARK: W_INGAMEOPTIONS_BUTTON

std::shared_ptr<W_INGAMEOPTIONS_BUTTON> W_INGAMEOPTIONS_BUTTON::make()
{
	class make_shared_enabler: public W_INGAMEOPTIONS_BUTTON {};
	auto widget = std::make_shared<make_shared_enabler>();
	widget->initialize();
	return widget;
}

void W_INGAMEOPTIONS_BUTTON::initialize()
{
	id = IDRET_OPTIONS;
	setGeometry(0, 0, RET_BUTWIDTH, RET_BUTHEIGHT);
}

uint8_t W_INGAMEOPTIONS_BUTTON::currentAlphaValue() const
{
	uint32_t alphaPercentage = war_getOptionsButtonVisibility();

	if (isHighlighted())
	{
		alphaPercentage = 100;
	}

	return static_cast<uint8_t>((alphaPercentage * 255) / 100);
}

bool W_INGAMEOPTIONS_BUTTON::isHighlighted() const
{
	return ((getState() & WBUT_HIGHLIGHT) != 0) || InGameOpUp;
}

void W_INGAMEOPTIONS_BUTTON::display(int xOffset, int yOffset)
{
	const int x0 = xOffset + x();
	const int y0 = yOffset + y();
	bool butDisabled = getState() & WBUT_DISABLE;

	uint8_t alphaValue = currentAlphaValue();

	if (butDisabled)
	{
		iV_DrawImageTint(IntImages, IMAGE_RETICULE_GREY, x0, y0, pal_RGBA(255,255,255,alphaValue), Vector2f{width(), height()});
		return;
	}

	bool Down = getState() & (WBUT_DOWN | WBUT_CLICKLOCK);
	if (Down)
	{
		iV_DrawImageTint(IntImages, IMAGE_INGAMEOPTIONS_DOWN, x0, y0, pal_RGBA(255,255,255,alphaValue), Vector2f{width(), height()});
	}
	else
	{
		iV_DrawImageTint(IntImages, IMAGE_INGAMEOPTIONS_UP, x0, y0, pal_RGBA(255,255,255,alphaValue), Vector2f{width(), height()});
	}

	if (isHighlighted())
	{
		iV_DrawImageTint(IntImages, IMAGE_RETICULE_HILIGHT, x0, y0, pal_RGBA(255,255,255,alphaValue), Vector2f{width(), height()});
	}
}

std::string W_INGAMEOPTIONS_BUTTON::getTip()
{
	if (!InGameOpUp)
	{
		return  _("Open In-Game Options");
	}
	else
	{
		return  _("Close In-Game Options");
	}
}

bool intAddInGameOptionsButton()
{
	bool hiddenOptionsButton = war_getOptionsButtonVisibility() == 0;

	auto psExistingBut = widgGetFromID(psWScreen, IDRET_OPTIONS);
	if (psExistingBut != nullptr)
	{
		bool oldButtonHiddenStatus = !psExistingBut->visible();
		psExistingBut->show(!hiddenOptionsButton);

		// Trigger Re-position of the Replay Controller form
		if (replayOverlayScreen && (oldButtonHiddenStatus != hiddenOptionsButton))
		{
			replayOverlayScreen->screenSizeDidChange(screenWidth, screenHeight, screenWidth, screenHeight);
		}
		return false;
	}

	auto button = W_INGAMEOPTIONS_BUTTON::make();
	if (!button)
	{
		debug(LOG_ERROR, "Failed to create in game options button");
	}
	else
	{
		psWScreen->psForm->attach(button);
		setReticuleButtonDimensions(*button, "image_ingameoptions_up.png");
		button->setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
			auto psIngameOptButton = std::dynamic_pointer_cast<W_INGAMEOPTIONS_BUTTON>(psWidget->shared_from_this());
			ASSERT_OR_RETURN(, psIngameOptButton != nullptr, "Wrong button type?");
			int w = psWidget->width();
			int h = psWidget->height();
			int x0 = screenWidth - (w + psIngameOptButton->getMarginX());
			int y0 = psIngameOptButton->getMarginTop();
			psWidget->setGeometry(x0, y0, w, h);
		}));
		button->addOnClickHandler([](W_BUTTON&) {
			widgScheduleTask([](){
				kf_addInGameOptions();
			});
		});

		if (hiddenOptionsButton)
		{
			button->hide();
		}
	}

	// Trigger Re-position of the Replay Controller form
	if (replayOverlayScreen)
	{
		replayOverlayScreen->screenSizeDidChange(screenWidth, screenHeight, screenWidth, screenHeight);
	}

	return true;
}

void intHideInGameOptionsButton()
{
	auto psOptionsBut = widgGetFromID(psWScreen, IDRET_OPTIONS);
	if (psOptionsBut != nullptr)
	{
		psOptionsBut->hide();
	}
}

/* Add the reticule widgets to the widget screen */
bool intAddReticule()
{
	intAddInGameOptionsButton();

	if (ReticuleUp)
	{
		return true; // all fine
	}
	auto const &parent = psWScreen->psForm;
	auto retForm = std::make_shared<IntFormAnimated>(false);
	parent->attach(retForm);
	retForm->id = IDRET_FORM;
	retForm->setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
		psWidget->setGeometry(RET_X, RET_Y, RET_FORMWIDTH, RET_FORMHEIGHT);
	}));
	for (int i = 0; i < NUMRETBUTS; i++)
	{
		// Set the x,y members of a button widget initialiser
		//
		/* Default button data */
		W_BUTINIT sButInit;
		sButInit.formID = IDRET_FORM;
		sButInit.id = ReticuleEnabled[i].id;
		sButInit.width = RET_BUTWIDTH;
		sButInit.height = RET_BUTHEIGHT;
		sButInit.pDisplay = intDisplayReticuleButton;
		sButInit.x = ReticuleOffsets[i].x;
		sButInit.y = ReticuleOffsets[i].y;
		sButInit.pTip = retbutstats[i].tip;
		sButInit.style = WBUT_SECONDARY;
		sButInit.UserData = i;
		retbutstats[i].downTime = 0;
		retbutstats[i].flashing = false;
		retbutstats[i].flashTime = 0;
		auto pButton = widgAddButton(psWScreen, &sButInit);
		retbutstats[i].button = (pButton) ? std::dynamic_pointer_cast<W_BUTTON>(pButton->shared_from_this()) : nullptr;
		if (!retbutstats[i].button)
		{
			debug(LOG_ERROR, "Failed to add reticule button");
		}
		else
		{
			setReticuleButtonDimensions(*retbutstats[i].button, retbutstats[i].filename);
		}
	}
	ReticuleUp = true;
	return true;
}

void intRemoveReticule()
{
	if (ReticuleUp == true)
	{
		widgDelete(psWScreen, IDRET_FORM);		// remove reticule
		ReticuleUp = false;
	}
}

//toggles the Power Bar display on and off
void togglePowerBar()
{
	//toggle the flag
	powerBarUp = !powerBarUp;

	if (powerBarUp)
	{
		intShowPowerBar();
	}
	else
	{
		intHidePowerBar();
	}
}

/* Add the power bars to the screen */
bool intAddPower()
{
	W_BARINIT sBarInit;

	/* Add the trough bar */
	sBarInit.formID = 0;	//IDPOW_FORM;
	sBarInit.id = IDPOW_POWERBAR_T;
	//start the power bar off in view (default)
	sBarInit.style = WBAR_TROUGH;
	sBarInit.sCol = WZCOL_POWER_BAR;
	sBarInit.iRange = POWERBAR_SCALE;

	auto psBarGraph = std::make_shared<PowerBar>(&sBarInit);
	psBarGraph->setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
		psWidget->setGeometry((SWORD)POW_X, (SWORD)POW_Y, POW_BARWIDTH, iV_GetImageHeight(IntImages, IMAGE_PBAR_EMPTY));
	}));

	ASSERT_NOT_NULLPTR_OR_RETURN(false, psWScreen);
	ASSERT_NOT_NULLPTR_OR_RETURN(false, psWScreen->psForm);
	psWScreen->psForm->attach(psBarGraph);

	powerBarUp = true;
	return true;
}

/* Remove the build widgets from the widget screen */
void intRemoveObject()
{
	widgDelete(psWScreen, IDOBJ_TABFORM);

	// Start the window close animation.
	IntFormAnimated *Form = (IntFormAnimated *)widgGetFromID(psWScreen, IDOBJ_FORM);
	if (Form)
	{
		Form->closeAnimateDelete();
	}

	intHidePowerBar();
}


/* Remove the build widgets from the widget screen */
void intRemoveObjectNoAnim()
{
	widgDelete(psWScreen, IDOBJ_TABFORM);
	widgDelete(psWScreen, IDOBJ_FORM);

	intHidePowerBar();
}


/* Remove the stats widgets from the widget screen */
void intRemoveStats()
{
	widgDelete(psWScreen, IDSTAT_CLOSE);
	widgDelete(psWScreen, IDSTAT_TABFORM);
	setSecondaryWindowUp(false);

	// Start the window close animation.
	IntFormAnimated *Form = (IntFormAnimated *)widgGetFromID(psWScreen, IDSTAT_FORM);
	if (Form)
	{
		Form->closeAnimateDelete();
	}
}


/* Remove the stats widgets from the widget screen */
void intRemoveStatsNoAnim()
{
	widgDelete(psWScreen, IDSTAT_CLOSE);
	widgDelete(psWScreen, IDSTAT_TABFORM);
	widgDelete(psWScreen, IDSTAT_FORM);
	setSecondaryWindowUp(false);
}

void makeObsoleteButton(const std::shared_ptr<WIDGET> &parent)
{
	auto obsoleteButton = std::make_shared<MultipleChoiceButton>();
	parent->attach(obsoleteButton);
	obsoleteButton->id = IDSTAT_OBSOLETE_BUTTON;
	obsoleteButton->style |= WBUT_SECONDARY;
	obsoleteButton->setChoice(includeRedundantDesigns);
	obsoleteButton->setImages(false, MultipleChoiceButton::Images(AtlasImage(IntImages, IMAGE_OBSOLETE_HIDE_UP), AtlasImage(IntImages, IMAGE_OBSOLETE_HIDE_UP), AtlasImage(IntImages, IMAGE_OBSOLETE_HIDE_HI)));
	obsoleteButton->setTip(false, _("Hiding Obsolete Tech"));
	obsoleteButton->setImages(true,  MultipleChoiceButton::Images(AtlasImage(IntImages, IMAGE_OBSOLETE_SHOW_UP), AtlasImage(IntImages, IMAGE_OBSOLETE_SHOW_UP), AtlasImage(IntImages, IMAGE_OBSOLETE_SHOW_HI)));
	obsoleteButton->setTip(true, _("Showing Obsolete Tech"));
	obsoleteButton->move(4 + AtlasImage(IntImages, IMAGE_FDP_UP).width() + 4, STAT_SLDY);
}

/* Add the stats widgets to the widget screen */
/* If psSelected != NULL it specifies which stat should be hilited
   psOwner specifies which object is hilighted on the object bar for this stat*/
static bool intAddDebugStatsForm(BASE_STATS **_ppsStatsList, UDWORD numStats)
{
	// should this ever be called with psOwner == NULL?

	// Is the form already up?
	if (widgGetFromID(psWScreen, IDSTAT_FORM) != nullptr)
	{
		intRemoveStatsNoAnim();
	}

	// is the order form already up ?
	if (widgGetFromID(psWScreen, IDORDER_FORM) != nullptr)
	{
		intRemoveOrderNoAnim();
	}

	setSecondaryWindowUp(true);

	auto const &parent = psWScreen->psForm;

	/* Create the basic form */
	auto statForm = std::make_shared<IntFormAnimated>(false);
	parent->attach(statForm);
	statForm->id = IDSTAT_FORM;
	statForm->setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
		psWidget->setGeometry(STAT_X, STAT_Y, STAT_WIDTH, STAT_HEIGHT);
	}));

	/* Add the close button */
	W_BUTINIT sButInit;
	sButInit.formID = IDSTAT_FORM;
	sButInit.id = IDSTAT_CLOSE;
	sButInit.x = STAT_WIDTH - CLOSE_WIDTH;
	sButInit.y = 0;
	sButInit.width = CLOSE_WIDTH;
	sButInit.height = CLOSE_HEIGHT;
	sButInit.pTip = _("Close");
	sButInit.pDisplay = intDisplayImageHilight;
	sButInit.UserData = PACKDWORD_TRI(0, IMAGE_CLOSEHILIGHT , IMAGE_CLOSE);
	if (!widgAddButton(psWScreen, &sButInit))
	{
		return false;
	}

	// Add the tabbed form
	auto statList = IntListTabWidget::make();
	statForm->attach(statList);
	statList->id = IDSTAT_TABFORM;
	statList->setChildSize(STAT_BUTWIDTH, STAT_BUTHEIGHT);
	statList->setChildSpacing(STAT_GAP, STAT_GAP);
	int statListWidth = STAT_BUTWIDTH * 2 + STAT_GAP;
	statList->setGeometry((STAT_WIDTH - statListWidth) / 2, STAT_TABFORMY, statListWidth, STAT_HEIGHT - STAT_TABFORMY);

	/* Add the stat buttons */
	int nextButtonId = IDSTAT_START;

	W_BARINIT sBarInit;
	sBarInit.id = IDSTAT_TIMEBARSTART;
	sBarInit.x = STAT_TIMEBARX;
	sBarInit.width = STAT_PROGBARWIDTH;
	sBarInit.height = STAT_PROGBARHEIGHT;
	sBarInit.size = 50;
	sBarInit.sCol = WZCOL_ACTION_PROGRESS_BAR_MAJOR;
	sBarInit.sMinorCol = WZCOL_ACTION_PROGRESS_BAR_MINOR;

	statID = 0;
	for (unsigned i = 0; i < numStats; i++)
	{
		sBarInit.y = STAT_TIMEBARY;

		if (nextButtonId > IDSTAT_END)
		{
			//can't fit any more on the screen!
			debug(LOG_WARNING, "This is just a Warning! Max buttons have been allocated");
			break;
		}

		auto button = std::make_shared<IntStatsButton>();
		statList->attach(button);
		button->id = nextButtonId;
		button->style |= WFORM_SECONDARY;
		button->setStats(_ppsStatsList[i]);
		statList->addWidgetToLayout(button);

		BASE_STATS *Stat = _ppsStatsList[i];
		WzString tipString = getLocalizedStatsName(_ppsStatsList[i]);
		unsigned powerCost = 0;
		W_BARGRAPH *bar;
		if (Stat->hasType(STAT_STRUCTURE))  // It's a structure.
		{
			powerCost = ((STRUCTURE_STATS *)Stat)->powerToBuild;
			sBarInit.size = powerCost / POWERPOINTS_DROIDDIV;
			if (sBarInit.size > 100)
			{
				sBarInit.size = 100;
			}

			sBarInit.formID = nextButtonId;
			sBarInit.iRange = GAME_TICKS_PER_SEC;
			bar = widgAddBarGraph(psWScreen, &sBarInit);
			bar->setBackgroundColour(WZCOL_BLACK);
		}
		else if (Stat->hasType(STAT_TEMPLATE))  // It's a droid.
		{
			powerCost = calcTemplatePower((DROID_TEMPLATE *)Stat);
			sBarInit.size = powerCost / POWERPOINTS_DROIDDIV;
			if (sBarInit.size > 100)
			{
				sBarInit.size = 100;
			}

			sBarInit.formID = nextButtonId;
			sBarInit.iRange = GAME_TICKS_PER_SEC;
			bar = widgAddBarGraph(psWScreen, &sBarInit);
			bar->setBackgroundColour(WZCOL_BLACK);
		}
		WzString costString = WzString::fromUtf8(astringf(_("Cost: %u"), powerCost));
		tipString.append("\n");
		tipString.append(costString);
		button->setTip(tipString.toUtf8().c_str());

		/* Update the init struct for the next button */
		++nextButtonId;

		sBarInit.id += 1;
	}

	return true;
}

/* Return the stats for a research facility */
static BASE_STATS *getResearchStats(BASE_OBJECT *psObj)
{
	ASSERT_OR_RETURN(nullptr, psObj != nullptr && psObj->type == OBJ_STRUCTURE, "Invalid Structure pointer");
	STRUCTURE *psBuilding = (STRUCTURE *)psObj;
	RESEARCH_FACILITY *psResearchFacility = &psBuilding->pFunctionality->researchFacility;

	if (psResearchFacility->psSubjectPending != nullptr && !IsResearchCompleted(&asPlayerResList[psObj->player][psResearchFacility->psSubjectPending->index]))
	{
		return psResearchFacility->psSubjectPending;
	}

	return psResearchFacility->psSubject;
}

bool setController(std::shared_ptr<BaseObjectsController> controller, INTMODE newIntMode, OBJECT_MODE newObjMode)
{
	intResetScreen(true);

	if (!controller->showInterface())
	{
		debug(LOG_ERROR, "Failed to show interface");
		intResetScreen(true);
		return false;
	}

	intMode = newIntMode;
	objMode = newObjMode;
	interfaceController = controller;
	return true;
}

/* Add the build widgets to the widget screen */
static bool intAddBuild()
{
	if (bMultiPlayer && NetPlay.players[selectedPlayer].isSpectator) { return false; }
	return setController(std::make_shared<BuildController>(), INT_STAT, IOBJ_BUILD);
}

/* Add the manufacture widgets to the widget screen */
static bool intAddManufacture()
{
	if (bMultiPlayer && NetPlay.players[selectedPlayer].isSpectator) { return false; }
	return setController(std::make_shared<ManufactureController>(), INT_STAT, IOBJ_MANUFACTURE);
}

/* Add the research widgets to the widget screen */
static bool intAddResearch()
{
	if (bMultiPlayer && NetPlay.players[selectedPlayer].isSpectator) { return false; }
	return setController(std::make_shared<ResearchController>(), INT_STAT, IOBJ_RESEARCH);
}

/* Add the command droid widgets to the widget screen */
static bool intAddCommand()
{
	if (bMultiPlayer && NetPlay.players[selectedPlayer].isSpectator) { return false; }
	return setController(std::make_shared<CommanderController>(), INT_CMDORDER, IOBJ_COMMAND);
}

//sets up the Intelligence Screen as far as the interface is concerned
void addIntelScreen()
{
	if (intMode == INT_INTELMAP)
	{
		// screen is already up - do nothing
		return;
	}

	intResetScreen(false);
	intHideGroupSelectionMenu();

	intMode = INT_INTELMAP;

	//lock the reticule button
	widgSetButtonState(psWScreen, IDRET_INTEL_MAP, WBUT_CLICKLOCK);

	//add the power bar - for looks!
	intShowPowerBar();

	//add all the intelligence screen interface
	(void)intAddIntelMap();
}

//sets up the Transporter Screen as far as the interface is concerned
void addTransporterInterface(DROID *psSelected, bool onMission)
{
	// if psSelected = NULL add interface but if psSelected != NULL make sure its not flying
	if (!psSelected || (psSelected && !transporterFlying(psSelected)))
	{
		intResetScreen(false);
		intAddTransporter(psSelected, onMission);
		intMode = INT_TRANSPORTER;
	}
}

/*sets which list of structures to use for the interface*/
StructureList *interfaceStructList()
{
	if (selectedPlayer >= MAX_PLAYERS)
	{
		return nullptr;
	}

	if (offWorldKeepLists)
	{
		return &mission.apsStructLists[selectedPlayer];
	}
	else
	{
		return &apsStructLists[selectedPlayer];
	}
}


/*causes a reticule button to start flashing*/
void flashReticuleButton(UDWORD buttonID)
{
	//get the button for the id
	WIDGET *psButton = widgGetFromID(psWScreen, buttonID);
	if (psButton)
	{
		retbutstats[psButton->UserData].flashing = true;
	}
}

// stop a reticule button flashing
void stopReticuleButtonFlash(UDWORD buttonID)
{
	WIDGET	*psButton = widgGetFromID(psWScreen, buttonID);
	if (psButton)
	{
		retbutstats[psButton->UserData].flashTime = 0;
		retbutstats[psButton->UserData].flashing = false;
	}
}

// show selected widget from reticule menu
void intShowWidget(int buttonID)
{
	switch (buttonID)
	{
	case RETBUT_FACTORY:
		widgSetButtonState(psWScreen, IDRET_MANUFACTURE, WBUT_CLICKLOCK);
		intAddManufacture();
		reticuleCallback(RETBUT_FACTORY);
		break;
	case RETBUT_RESEARCH:
		widgSetButtonState(psWScreen, IDRET_RESEARCH, WBUT_CLICKLOCK);
		intAddResearch();
		reticuleCallback(RETBUT_RESEARCH);
		break;
	case RETBUT_BUILD:
		widgSetButtonState(psWScreen, IDRET_BUILD, WBUT_CLICKLOCK);
		intAddBuild();
		reticuleCallback(RETBUT_BUILD);
		break;
	case RETBUT_DESIGN:
		intResetScreen(true);
		widgSetButtonState(psWScreen, IDRET_DESIGN, WBUT_CLICKLOCK);
		/*add the power bar - for looks! */
		intShowPowerBar();
		intAddDesign(false);
		intMode = INT_DESIGN;
		gInputManager.contexts().pushState();
		gInputManager.contexts().makeAllInactive();
		reticuleCallback(RETBUT_DESIGN);
		triggerEvent(TRIGGER_MENU_DESIGN_UP);
		break;
	case RETBUT_INTELMAP:
		addIntelScreen();
		reticuleCallback(RETBUT_INTELMAP);
		break;
	case RETBUT_COMMAND:
		widgSetButtonState(psWScreen, IDRET_COMMAND, WBUT_CLICKLOCK);
		intAddCommand();
		reticuleCallback(RETBUT_COMMAND);
		break;
	default:
		intResetScreen(false);
		psCurrentMsg = nullptr;
		reticuleCallback(RETBUT_CANCEL);
		break;
	}
}

void intHideGroupSelectionMenu()
{
	if (groupsUI)
	{
		groupsUI->hide();
	}

	if (bMultiPlayer && selectedPlayer < NetPlay.players.size() && NetPlay.players[selectedPlayer].isSpectator)
	{
		// skip showing groups UI if selectedPlayer is spectator
		widgDelete(psWScreen, IDOBJ_GROUP);
		groupsUI.reset();
	}
}

//displays the Power Bar
void intShowPowerBar()
{
	if (bMultiPlayer && selectedPlayer < NetPlay.players.size() && NetPlay.players[selectedPlayer].isSpectator)
	{
		// skip showing power bar if selectedPlayer is spectator
		return;
	}

	//if its not already on display
	if (widgGetFromID(psWScreen, IDPOW_POWERBAR_T))
	{
		widgReveal(psWScreen, IDPOW_POWERBAR_T);
	}
}

//hides the power bar from the display - regardless of what player requested!
void forceHidePowerBar(bool forceSetPowerBarUpState)
{
	if (forceSetPowerBarUpState)
	{
		powerBarUp = false;
	}

	if (widgGetFromID(psWScreen, IDPOW_POWERBAR_T))
	{
		widgHide(psWScreen, IDPOW_POWERBAR_T);
	}
}


/* Add the Proximity message buttons */
bool intAddProximityButton(PROXIMITY_DISPLAY *psProxDisp, UDWORD inc)
{
	UDWORD				cnt;

	if (selectedPlayer >= MAX_PLAYERS)
	{
		return false;
	}

	W_FORMINIT sBFormInit;
	sBFormInit.formID = 0;
	sBFormInit.id = IDPROX_START + inc;

	// loop back and find a free one!
	if (sBFormInit.id >= IDPROX_END)
	{
		for (cnt = IDPROX_START; cnt < IDPROX_END; cnt++)
		{
			// go down the prox msgs and see if it's free.
			const auto& proxDispList = apsProxDisp[selectedPlayer];
			auto proxDispIt = std::find_if(proxDispList.begin(), proxDispList.end(), [cnt](PROXIMITY_DISPLAY* pd)
			{
				return pd->buttonID == cnt;
			});

			if (proxDispIt == proxDispList.end())	// value was unused.
			{
				sBFormInit.id = cnt;
				break;
			}
		}
		ASSERT_OR_RETURN(false, cnt != IDPROX_END, "Ran out of proximity displays");
	}
	ASSERT_OR_RETURN(false, sBFormInit.id < IDPROX_END, "Invalid proximity message button ID %d", (int)sBFormInit.id);

	sBFormInit.majorID = 0;
	sBFormInit.style = WFORM_CLICKABLE;
	sBFormInit.width = PROX_BUTWIDTH;
	sBFormInit.height = PROX_BUTHEIGHT;
	//the x and y need to be set up each time the button is drawn - see intDisplayProximityBlips

	sBFormInit.pDisplay = intDisplayProximityBlips;
	//set the data for this button
	sBFormInit.pUserData = psProxDisp;

	if (!widgAddForm(psWScreen, &sBFormInit))
	{
		return false;
	}

	//store the ID so we can detect which one has been clicked on
	psProxDisp->buttonID = sBFormInit.id;

	return true;
}


/*Remove a Proximity Button - when the message is deleted*/
void intRemoveProximityButton(PROXIMITY_DISPLAY *psProxDisp)
{
	ASSERT_OR_RETURN(, psProxDisp->buttonID >= IDPROX_START && psProxDisp->buttonID <= IDPROX_END, "Invalid proximity ID");
	widgDelete(psWScreen, psProxDisp->buttonID);
}

/*deals with the proximity message when clicked on*/
void processProximityButtons(UDWORD id)
{
	if (!doWeDrawProximitys())
	{
		return;
	}

	if (selectedPlayer >= MAX_PLAYERS) { return; /* no-op */ }

	//find which proximity display this relates to
	const auto& proxDispList = apsProxDisp[selectedPlayer];
	auto proxDispIt = std::find_if(proxDispList.begin(), proxDispList.end(), [id](PROXIMITY_DISPLAY* psProxDisp)
	{
		return psProxDisp->buttonID == id;
	});
	if (proxDispIt != proxDispList.end())
	{
		//if not been read - display info
		if (!(*proxDispIt)->psMessage->read)
		{
			displayProximityMessage(*proxDispIt);
		}
	}
}

/*	Fools the widgets by setting a key value */
void	setKeyButtonMapping(UDWORD	val)
{
	keyButtonMapping = val;
}

// count the number of selected droids of a type
static SDWORD intNumSelectedDroids(UDWORD droidType)
{
	SDWORD	num;

	if (selectedPlayer >= MAX_PLAYERS)
	{
		return 0;
	}

	num = 0;
	for (const DROID* psDroid : apsDroidLists[selectedPlayer])
	{
		if (psDroid->selected && psDroid->droidType == droidType)
		{
			num += 1;
		}
	}

	return num;
}

/*Checks to see if there are any research topics to do and flashes the button -
only if research facility is free*/
int intGetResearchState()
{
	if (selectedPlayer >= MAX_PLAYERS)
	{
		return 0;
	}

	bool resFree = false;
	StructureList* intStrList = interfaceStructList();
	if (intStrList)
	{
		for (STRUCTURE* psStruct : *intStrList)
		{
			if (psStruct->pStructureType->type == REF_RESEARCH &&
				psStruct->status == SS_BUILT &&
				getResearchStats(psStruct) == nullptr)
			{
				resFree = true;
				break;
			}
		}
	}

	int count = 0;
	if (resFree)
	{
		//calculate the list
		auto researchList = fillResearchList(selectedPlayer, nonstd::nullopt, MAXRESEARCH);
		count = researchList.size();
		for (int n = 0; n < researchList.size(); ++n)
		{
			for (int player = 0; player < MAX_PLAYERS; ++player)
			{
				if (aiCheckAlliances(player, selectedPlayer) && IsResearchStarted(&asPlayerResList[player][researchList[n]]))
				{
					--count;  // An ally is already researching this topic, so don't flash the button because of it.
					break;
				}
			}
		}
	}

	return count;
}

void intNotifyResearchButton(int prevState)
{
	int newState = intGetResearchState();
	if (newState > prevState)
	{
		// Set the research reticule button to flash.
		flashReticuleButton(IDRET_RESEARCH);
	}
	else if (newState == 0 && prevState > 0)
	{
		stopReticuleButtonFlash(IDRET_RESEARCH);
	}
}

// see if a reticule button is enabled
bool intCheckReticuleButEnabled(UDWORD id)
{
	for (auto &i : ReticuleEnabled)
	{
		if (i.id == id)
		{
			return i.Enabled;
		}
	}
	return false;
}

// Checks if a coordinate is over the build menu
bool CoordInBuild(int x, int y)
{
	// This measurement is valid for the menu, so the buildmenu_height
	// value is used to "nudge" it all upwards from the command menu.
	// FIXME: hardcoded value (?)
	const int buildmenu_height = 300;
	Vector2f pos;

	pos.x = x - RET_X;
	pos.y = y - RET_Y + buildmenu_height; // guesstimation

	if ((pos.x < 0 || pos.y < 0 || pos.x >= RET_FORMWIDTH || pos.y >= buildmenu_height) || !isSecondaryWindowUp())
	{
		return false;
	}

	return true;
}

// Our chat dialog for global & team communication
// \mode sets if global or team communication is wanted
void chatDialog(int mode, bool startWithQuickChatFocused)
{
	if (!ChatDialogUp)
	{
		ChatDialogUp = true;

		WzChatMode initialChatMode = (mode == CHAT_GLOB) ? WzChatMode::Glob : WzChatMode::Team;
		createChatScreen([]() {
			ChatDialogUp = false;
		}, initialChatMode, startWithQuickChatFocused);
	}
	else
	{
		debug(LOG_ERROR, "Tried to throw up chat dialog when we already had one up!");
	}
}

// If chat dialog is up
bool isChatUp()
{
	return ChatDialogUp;
}

// Helper call to see if we have builder/research/... window up or not.
bool isSecondaryWindowUp()
{
	return secondaryWindowUp;
}

void setSecondaryWindowUp(bool value)
{
	secondaryWindowUp = true;
}

void intSetShouldShowRedundantDesign(bool value)
{
	includeRedundantDesigns = value;
}

bool intGetShouldShowRedundantDesign()
{
	return includeRedundantDesigns;
}

void intShowInterface()
{
	intAddReticule();
	intShowPowerBar();
	intShowGroupSelectionMenu();
}

void intHideInterface(bool forceHidePowerbar)
{
	intRemoveReticule();
	if (forceHidePowerbar)
	{
		forceHidePowerBar();
	}
	else
	{
		intHidePowerBar();
	}
	intHideGroupSelectionMenu();
}
