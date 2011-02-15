/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2020  Warzone 2100 Project

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
/*
 * loadsave.c
 * load and save Popup screens.
 *
 * these don't actually do any loading or saving, but just
 * return a filename to use for the ops.
 */

#include <physfs.h>
#include "lib/framework/physfs_ext.h"
#include <ctime>

#include "lib/framework/frame.h"
#include "lib/framework/input.h"
#include "lib/framework/stdio_ext.h"
#include "lib/framework/wztime.h"
#include "lib/widget/button.h"
#include "lib/widget/editbox.h"
#include "lib/widget/widget.h"
#include "lib/ivis_opengl/piepalette.h"		// for predefined colours.
#include "lib/ivis_opengl/bitimage.h"
#include "lib/ivis_opengl/pieblitfunc.h"		// for boxfill
#include "hci.h"
#include "loadsave.h"
#include "multiplay.h"
#include "game.h"
#include "lib/sound/audio_id.h"
#include "lib/sound/audio.h"
#include "frontend.h"
#include "main.h"
#include "display.h"
#include "lib/netplay/netplay.h"
#include "loop.h"
#include "intdisplay.h"
#include "mission.h"
#include "lib/gamelib/gtime.h"
#include "console.h"
#include "keybind.h"
#include "qtscript.h"
#include "clparse.h"
#include "ingameop.h"

#define totalslots 36			// saves slots
#define slotsInColumn 12		// # of slots in a column
#define totalslotspace 64		// guessing 64 max chars for filename.

#define LOADSAVE_X				D_W + 16
#define LOADSAVE_Y				D_H + 5
#define LOADSAVE_W				610
#define LOADSAVE_H				215

#define MAX_SAVE_NAME			60

#define LOADSAVE_HGAP			9
#define LOADSAVE_VGAP			9
#define LOADSAVE_BANNER_DEPTH	40 		//top banner which displays either load or save

#define LOADENTRY_W				((LOADSAVE_W / 3 )-(3 * LOADSAVE_HGAP))
#define LOADENTRY_H				(LOADSAVE_H -(5 * LOADSAVE_VGAP )- (LOADSAVE_BANNER_DEPTH+LOADSAVE_VGAP) ) /5

#define ID_LOADSAVE				21000
#define LOADSAVE_FORM			ID_LOADSAVE+1		// back form.
#define LOADSAVE_CANCEL			ID_LOADSAVE+2		// cancel but.
#define LOADSAVE_LABEL			ID_LOADSAVE+3		// load/save
#define LOADSAVE_BANNER			ID_LOADSAVE+4		// banner.

#define LOADENTRY_START			ID_LOADSAVE+10		// each of the buttons.
#define LOADENTRY_END			ID_LOADSAVE+10 +totalslots  // must have unique ID hmm -Q

#define SAVEENTRY_EDIT			ID_LOADSAVE + totalslots + totalslots		// save edit box. must be highest value possible I guess. -Q
#define AUTOSAVE_CAM_DIR "savegames/campaign/auto"
#define AUTOSAVE_SKI_DIR "savegames/skirmish/auto"

// ////////////////////////////////////////////////////////////////////////////
static void displayLoadBanner(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset);
static void displayLoadSlot(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset);

struct LoadSaveDisplayLoadSlotCache {
	std::string fullText;
	WzText wzText;
};

static std::shared_ptr<W_SCREEN> psRequestScreen = nullptr; // Widget screen for requester
static bool modeLoad;
static bool modeReplay;
static bool modeIngame;
static	UDWORD		chosenSlotId;

bool				bLoadSaveUp = false;        // true when interface is up and should be run.
char				saveGameName[256];          //the name of the save game to load from the front end
char				sRequestResult[PATH_MAX];   // filename returned;
bool				bRequestLoad = false;
bool				autosaveEnabled = true;
bool                bRequestLoadReplay = false;
LOADSAVE_MODE		bLoadSaveMode;
static const char *savedTitle;
static const char *sSaveGameExtension = ".gam";
static const char *sSaveReplayExtension = ".wzrp";

// ////////////////////////////////////////////////////////////////////////////
// return whether the specified filename looks like a saved game file, i.e. ends with .gam
static bool isASavedGamefile(const char *filename, const char *extension)
{
	static const size_t saveGameExtensionLength = strlen(extension);

	if (nullptr == filename)
	{
		return false;
	}

	size_t filenameLength = strlen(filename);
	if (filenameLength <= saveGameExtensionLength)
	{
		// reject filename of insufficient length to contain "<anything>.gam"
		return false;
	}
	return 0 == strcmp(filename + filenameLength - saveGameExtensionLength, extension);
}

// ////////////////////////////////////////////////////////////////////////////
// return whether the save screen was displayed in the mission results screen
bool saveInMissionRes()
{
	return bLoadSaveMode == SAVE_MISSIONEND;
}

// ////////////////////////////////////////////////////////////////////////////
// return whether the save screen was displayed in the middle of a mission
bool saveMidMission()
{
	return bLoadSaveMode == SAVE_INGAME_MISSION;
}

void loadSaveScreenSizeDidChange(unsigned int oldWidth, unsigned int oldHeight, unsigned int newWidth, unsigned int newHeight)
{
	if (psRequestScreen == nullptr) return;
	psRequestScreen->screenSizeDidChange(oldWidth, oldHeight, newWidth, newHeight);
}

// ////////////////////////////////////////////////////////////////////////////
bool addLoadSave(LOADSAVE_MODE savemode, const char *title)
{
	char NewSaveGamePath[PATH_MAX] = {'\0'};
	bLoadSaveMode = savemode;
	savedTitle = title;
	UDWORD			slotCount;

	// Static as these are assigned to the widget buttons by reference
	static char	sSlotCaps[totalslots][totalslotspace];
	static char	sSlotTips[totalslots][totalslotspace];

	bool bLoad = true;
	bool bReplay = false;
	switch (savemode)
	{
	case LOAD_FRONTEND_MISSION:
	case LOAD_INGAME_MISSION:
	case LOAD_MISSIONEND:
		ssprintf(NewSaveGamePath, "%s%s/", SaveGamePath, "campaign");
		break;
	case LOAD_FRONTEND_SKIRMISH:
	case LOAD_INGAME_SKIRMISH:
		ssprintf(NewSaveGamePath, "%s%s/", SaveGamePath, "skirmish");
		break;
	case LOAD_FRONTEND_MISSION_AUTO:
	case LOAD_INGAME_MISSION_AUTO:
	case LOAD_MISSIONEND_AUTO:
		ssprintf(NewSaveGamePath, "%s%s/", SaveGamePath, "campaign/auto");
		break;
	case LOAD_FRONTEND_SKIRMISH_AUTO:
	case LOAD_INGAME_SKIRMISH_AUTO:
		ssprintf(NewSaveGamePath, "%s%s/", SaveGamePath, "skirmish/auto");
		break;
	case SAVE_MISSIONEND:
	case SAVE_INGAME_MISSION:
		ssprintf(NewSaveGamePath, "%s%s/", SaveGamePath, "campaign");
		bLoad = false;
		break;
	case SAVE_INGAME_SKIRMISH:
		ssprintf(NewSaveGamePath, "%s%s/", SaveGamePath, "skirmish");
		bLoad = false;
		break;
	case LOADREPLAY_FRONTEND_SKIRMISH:
		ssprintf(NewSaveGamePath, "%s%s/", ReplayPath, "skirmish");
		bReplay = true;
		break;
	default:
		ASSERT("Invalid load/save mode!", "Invalid load/save mode!");
		ssprintf(NewSaveGamePath, "%s%s/", SaveGamePath, "campaign");
		break;
	}

	bool bIngame = bLoadSaveMode == LOAD_INGAME_MISSION || bLoadSaveMode == SAVE_INGAME_MISSION
	    || bLoadSaveMode == LOAD_INGAME_SKIRMISH || bLoadSaveMode == SAVE_INGAME_SKIRMISH
	    || bLoadSaveMode == LOAD_INGAME_MISSION_AUTO || bLoadSaveMode == LOAD_INGAME_SKIRMISH_AUTO;

	modeLoad = bLoad;
	modeIngame = bIngame;
	modeReplay = bReplay;
	debug(LOG_SAVE, "called (%d, %s)", bLoad, title);

	if (bIngame)
	{
		if (!bMultiPlayer || !NetPlay.bComms)
		{
			gameTimeStop();
			if (GetGameMode() == GS_NORMAL)
			{
				// just display the 3d, no interface
				displayWorld();
			}

			setGamePauseStatus(true);
			setAllPauseStates(true);
		}

		forceHidePowerBar();
		intRemoveReticule();
	}

	psRequestScreen = W_SCREEN::make();

	auto const &parent = psRequestScreen->psForm;

	/* add a form to place the tabbed form on */
	// we need the form to be long enough for all resolutions, so we take the total number of items * height
	// and * the gaps, add the banner, and finally, the fudge factor ;)
	auto loadSaveForm = std::make_shared<IntFormAnimated>();
	parent->attach(loadSaveForm);
	loadSaveForm->id = LOADSAVE_FORM;
	loadSaveForm->setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
		psWidget->setGeometry(LOADSAVE_X, LOADSAVE_Y, LOADSAVE_W, slotsInColumn * (LOADENTRY_H + LOADSAVE_HGAP) + LOADSAVE_BANNER_DEPTH + 20);
	}));

	// Add Banner
	W_FORMINIT sFormInit;
	sFormInit.formID = LOADSAVE_FORM;
	sFormInit.id = LOADSAVE_BANNER;
	sFormInit.x = LOADSAVE_HGAP;
	sFormInit.y = LOADSAVE_VGAP;
	sFormInit.width = LOADSAVE_W - (2 * LOADSAVE_HGAP);
	sFormInit.height = LOADSAVE_BANNER_DEPTH;
	sFormInit.pDisplay = displayLoadBanner;
	sFormInit.UserData = bLoad;
	widgAddForm(psRequestScreen, &sFormInit);

	// add cancel.
	W_BUTINIT sButInit;
	sButInit.formID = LOADSAVE_BANNER;
	sButInit.x = 8;
	sButInit.y = 10;
	sButInit.width		= iV_GetImageWidth(IntImages, IMAGE_NRUTER);
	sButInit.height		= iV_GetImageHeight(IntImages, IMAGE_NRUTER);
	sButInit.UserData	= PACKDWORD_TRI(0, IMAGE_NRUTER , IMAGE_NRUTER);

	sButInit.id = LOADSAVE_CANCEL;
	sButInit.style = WBUT_PLAIN;
	sButInit.pTip = _("Close");
	sButInit.pDisplay = intDisplayImageHilight;
	widgAddButton(psRequestScreen, &sButInit);

	// Add Banner Label
	W_LABINIT sLabInit;
	sLabInit.formID = LOADSAVE_BANNER;
	sLabInit.FontID = font_large;
	sLabInit.id		= LOADSAVE_LABEL;
	sLabInit.style	= WLAB_ALIGNCENTRE;
	sLabInit.x		= 0;
	sLabInit.y		= 0;
	sLabInit.width	= LOADSAVE_W - (2 * LOADSAVE_HGAP);	//LOADSAVE_W;
	sLabInit.height = LOADSAVE_BANNER_DEPTH;		//This looks right -Q
	sLabInit.pText	= WzString::fromUtf8(title);
	widgAddLabel(psRequestScreen, &sLabInit);

	// add slots
	sButInit = W_BUTINIT();
	sButInit.formID		= LOADSAVE_FORM;
	sButInit.style		= WBUT_PLAIN;
	sButInit.width		= LOADENTRY_W;
	sButInit.height		= LOADENTRY_H;
	sButInit.pDisplay	= displayLoadSlot;
	sButInit.initPUserDataFunc = []() -> void * { return new LoadSaveDisplayLoadSlotCache(); };
	sButInit.onDelete = [](WIDGET *psWidget) {
		assert(psWidget->pUserData != nullptr);
		delete static_cast<LoadSaveDisplayLoadSlotCache *>(psWidget->pUserData);
		psWidget->pUserData = nullptr;
	};

	for (slotCount = 0; slotCount < totalslots; slotCount++)
	{
		sButInit.id		= slotCount + LOADENTRY_START;

		if (slotCount < slotsInColumn)
		{
			sButInit.x	= 22 + LOADSAVE_HGAP;
			sButInit.y	= (SWORD)((LOADSAVE_BANNER_DEPTH + (2 * LOADSAVE_VGAP)) + (
			                          slotCount * (LOADSAVE_VGAP + LOADENTRY_H)));
		}
		else if (slotCount >= slotsInColumn && (slotCount < (slotsInColumn * 2)))
		{
			sButInit.x	= 22 + (2 * LOADSAVE_HGAP + LOADENTRY_W);
			sButInit.y	= (SWORD)((LOADSAVE_BANNER_DEPTH + (2 * LOADSAVE_VGAP)) + (
			                          (slotCount % slotsInColumn) * (LOADSAVE_VGAP + LOADENTRY_H)));
		}
		else
		{
			sButInit.x	= 22 + (3 * LOADSAVE_HGAP + (2 * LOADENTRY_W));
			sButInit.y	= (SWORD)((LOADSAVE_BANNER_DEPTH + (2 * LOADSAVE_VGAP)) + (
			                          (slotCount % slotsInColumn) * (LOADSAVE_VGAP + LOADENTRY_H)));
		}
		widgAddButton(psRequestScreen, &sButInit);
	}

	// fill slots.
	slotCount = 0;

	// The first slot is for [auto] or [..]
	{
		W_BUTTON *button;

		button = (W_BUTTON *)widgGetFromID(psRequestScreen, LOADENTRY_START + slotCount);
		if (bLoadSaveMode >= LOAD_FRONTEND_MISSION_AUTO)
		{
			button->pTip = _("Parent directory");
			button->pText = "[..]";
		}
		else
		{
			button->pTip = modeLoad? _("Autosave directory") : _("Autosave directory (not allowed for saving)");
			button->pText = "[auto]";
		}
		slotCount++;
	}

	debug(LOG_SAVE, "Searching \"%s\" for savegames", NewSaveGamePath);

	// add savegame filenames minus extensions to buttons

	struct SaveGameNamesAndTimes
	{
		std::string name;
		time_t savetime;

		SaveGameNamesAndTimes(std::string name, time_t savetime)
		: name(std::move(name))
		, savetime(savetime)
		{ }
	};

	std::vector<SaveGameNamesAndTimes> saveGameNamesAndTimes;

	char const *extension = bReplay? sSaveReplayExtension : sSaveGameExtension;
	WZ_PHYSFS_enumerateFiles(NewSaveGamePath, [&NewSaveGamePath, &saveGameNamesAndTimes, extension](char *i) -> bool {
		char savefile[256];
		time_t savetime;

		if (!isASavedGamefile(i, extension))
		{
			// If it doesn't, move on to the next filename
			return true;
		}

		debug(LOG_SAVE, "We found [%s]", i);

		/* Figure save-time */
		snprintf(savefile, sizeof(savefile), "%s/%s", NewSaveGamePath, i);
		savetime = WZ_PHYSFS_getLastModTime(savefile);

		(i)[strlen(i) - strlen(extension)] = '\0'; // remove .gam extension

		saveGameNamesAndTimes.emplace_back(i, savetime);
		return true;
	});

	// Sort the save games so that the most recent one appears first
	std::sort(saveGameNamesAndTimes.begin(),
			  saveGameNamesAndTimes.end(),
			  [](SaveGameNamesAndTimes& a, SaveGameNamesAndTimes& b) { return a.savetime > b.savetime; });

	// Now store the sorted save game names to the buttons
	slotCount = 1;
	(void)std::all_of(saveGameNamesAndTimes.begin(), saveGameNamesAndTimes.end(), [&](SaveGameNamesAndTimes& saveGameNameAndTime)
		{
			/* Set the button-text and tip text (the save time) into static storage */
			sstrcpy(sSlotCaps[slotCount], saveGameNameAndTime.name.c_str());
			auto newtime = getLocalTime(saveGameNameAndTime.savetime);
			strftime(sSlotTips[slotCount], sizeof(sSlotTips[slotCount]), "%F %H:%M:%S", &newtime);

			/* Add a button that references the static strings */
			W_BUTTON* button = (W_BUTTON*)widgGetFromID(psRequestScreen, LOADENTRY_START + slotCount);
			button->pTip = sSlotTips[slotCount];
			button->pText = WzString::fromUtf8(sSlotCaps[slotCount]);
			slotCount++;

			return (slotCount < totalslots);
		}
	);

	bLoadSaveUp = true;
	return true;
}

// ////////////////////////////////////////////////////////////////////////////
bool closeLoadSave(bool goBack)
{
	bLoadSaveUp = false;

	if (modeIngame)
	{
		if (goBack)
		{
			intReopenMenuWithoutUnPausing();
		}
		if (!bMultiPlayer || !NetPlay.bComms)
		{
			setGameUpdatePause(false);
			if (!goBack)
			{
				gameTimeStart();
				setGamePauseStatus(false);
				setAllPauseStates(false);
			}
		}

		intAddReticule();
		intShowPowerBar();
	}

	psRequestScreen = nullptr;
	// need to "eat" up the return key so it don't pass back to game.
	inputLoseFocus();
	return true;
}

bool closeLoadSaveOnShutdown()
{
	// Workaround for ensuring psRequestScreen is freed on shutdown - ideally this whole screen could probably be implemented as an overlay screen now and simplified
	bLoadSaveUp = false;
	psRequestScreen = nullptr;
	return true;
}

/***************************************************************************
	Delete a savegame.  fileName should be a .gam extension save game
	filename reference.  We delete this file, any .es file with the same
	name, and any files in the directory with the same name.
***************************************************************************/
void deleteSaveGame(char *fileName)
{
	ASSERT(strlen(fileName) < MAX_STR_LENGTH, "deleteSaveGame; save game name too long");

	PHYSFS_delete(fileName);
	fileName[strlen(fileName) - 4] = '\0'; // strip extension

	strcat(fileName, ".es");					// remove script data if it exists.
	PHYSFS_delete(fileName);
	fileName[strlen(fileName) - 3] = '\0'; // strip extension

	// check for a directory and remove that too.
	WZ_PHYSFS_enumerateFiles(fileName, [fileName](const char *i) -> bool {
		char del_file[PATH_MAX];

		// Construct the full path to the file by appending the
		// filename to the directory it is in.
		snprintf(del_file, sizeof(del_file), "%s/%s", fileName, i);

		debug(LOG_SAVE, "Deleting [%s].", del_file);

		// Delete the file
		if (!PHYSFS_delete(del_file))
		{
			debug(LOG_ERROR, "Warning [%s] could not be deleted due to PhysicsFS error: %s", del_file, WZ_PHYSFS_getLastError());
		}
		return true; // continue
	});

	if (!PHYSFS_delete(fileName))		// now (should be)empty directory
	{
		debug(LOG_ERROR, "Warning directory[%s] could not be deleted because %s", fileName, WZ_PHYSFS_getLastError());
	}
}

char lastSavePath[PATH_MAX];
bool lastSaveMP;
static time_t lastSaveTime;

static bool findLastSaveFrom(const char *path)
{
	bool found = false;

	WZ_PHYSFS_enumerateFiles(path, [&path, &found](const char *i) -> bool {
		char savefile[PATH_MAX];
		time_t savetime;

		if (!isASavedGamefile(i, sSaveGameExtension))
		{
			// If it doesn't, move on to the next filename
			return true;
		}
		/* Figure save-time */
		snprintf(savefile, sizeof(savefile), "%s/%s", path, i);
		savetime = WZ_PHYSFS_getLastModTime(savefile);
		if (difftime(savetime, lastSaveTime) > 0.0)
		{
			lastSaveTime = savetime;
			strcpy(lastSavePath, savefile);
			found = true;
		}
		return true;
	});
	return found;
}

bool findLastSave()
{
	char NewSaveGamePath[PATH_MAX] = {'\0'};
	bool foundMP, foundCAM;

	lastSaveTime = 0;
	lastSaveMP = false;
	lastSavePath[0] = '\0';
	ssprintf(NewSaveGamePath, "%scampaign/", SaveGamePath);
	foundCAM = findLastSaveFrom(NewSaveGamePath);
	ssprintf(NewSaveGamePath, "%scampaign/auto/", SaveGamePath);
	foundCAM |= findLastSaveFrom(NewSaveGamePath);
	ssprintf(NewSaveGamePath, "%sskirmish/", SaveGamePath);
	foundMP = findLastSaveFrom(NewSaveGamePath);
	ssprintf(NewSaveGamePath, "%sskirmish/auto/", SaveGamePath);
	foundMP |= findLastSaveFrom(NewSaveGamePath);
	if (foundMP)
	{
		lastSaveMP = true;
	}
	return foundMP | foundCAM;
}

static WzString suggestSaveName(const char *NewSaveGamePath)
{
	const WzString levelName = getLevelName();
	const std::string cheatedSuffix = Cheated ? _("cheated") : "";
	char saveNamePartial[64] = "\0";

	if (bLoadSaveMode == SAVE_MISSIONEND || bLoadSaveMode == SAVE_INGAME_MISSION)
	{
		std::string campaignName;
		if (levelName.startsWith("CAM_1") || levelName.startsWith("SUB_1"))
		{
			campaignName = "Alpha";
		}
		else if (levelName.startsWith("CAM_2") || levelName.startsWith("SUB_2"))
		{
			campaignName = "Beta";
		}
		else if (levelName.startsWith("CAM_3") || levelName.startsWith("SUB_3"))
		{
			campaignName = "Gamma";
		}
		ssprintf(saveNamePartial, "%s %s %s", campaignName.c_str(), levelName.toStdString().c_str(),
				 cheatedSuffix.c_str());
	}
	else if (bLoadSaveMode == SAVE_INGAME_SKIRMISH)
	{
		int humanPlayers = 0;
		for (int i = 0; i < MAX_PLAYERS; i++)
		{
			if (isHumanPlayer(i))
			{
				humanPlayers++;
			}
		}

		ssprintf(saveNamePartial, "%s %dp %s", levelName.toStdString().c_str(), humanPlayers, cheatedSuffix.c_str());
	}

	WzString saveName = WzString(saveNamePartial).trimmed();
	int similarSaveGames = 0;
	WZ_PHYSFS_enumerateFiles(NewSaveGamePath, [&similarSaveGames, &saveName](const char *fileName) -> bool {
		if (isASavedGamefile(fileName, sSaveGameExtension) && WzString(fileName).startsWith(saveName))
		{
			similarSaveGames++;
		}
		return true;
	});


	if (similarSaveGames > 0)
	{
		saveName += " " + WzString::number(similarSaveGames + 1);
	}

	return saveName;
}

static void runLoadSaveCleanup(bool resetWidgets, bool goBack)
{
	closeLoadSave(goBack);
	bRequestLoad = false;
	bRequestLoadReplay = false;
	if (!goBack && resetWidgets && widgGetFromID(psWScreen, IDMISSIONRES_FORM) == nullptr)
	{
		resetMissionWidgets();
	}
}

static void runLoadCleanup()
{
	bRequestLoadReplay = modeReplay;
	bRequestLoad = !modeReplay;
	if (modeReplay)
	{
		// Load a replay.
		closeLoadSave();
		return;
	}

	// Load a savegame.
	unsigned campaign = getCampaign(sRequestResult);
	setCampaignNumber(campaign);
	debug(LOG_WZ, "Set campaign for %s to %u", sRequestResult, campaign);
	closeLoadSave();
}

// ////////////////////////////////////////////////////////////////////////////
// Returns true if cancel pressed or a valid game slot was selected.
// if when returning true strlen(sRequestResult) != 0 then a valid game slot was selected
// otherwise cancel was selected..
bool runLoadSave(bool bResetMissionWidgets)
{
	static char     sDelete[PATH_MAX];
	char NewSaveGamePath[PATH_MAX] = {'\0'};

	WidgetTriggers const &triggers = widgRunScreen(psRequestScreen);
	unsigned id = triggers.empty() ? 0 : triggers.front().widget->id; // Just use first click here, since the next click could be on another menu.

	sstrcpy(sRequestResult, "");					// set returned filename to null;

	if (id == LOADSAVE_CANCEL || CancelPressed())
	{
		runLoadSaveCleanup(bResetMissionWidgets, true);
		return true;
	}
	if (bMultiPlayer)
	{
		if (bLoadSaveMode >= LOAD_FRONTEND_MISSION_AUTO)
		{
			ssprintf(NewSaveGamePath, "%s%s/auto/", SaveGamePath, "skirmish");
		}
		else
		{
			ssprintf(NewSaveGamePath, "%s%s/", SaveGamePath, "skirmish");
		}
	}
	else
	{
		if (bLoadSaveMode >= LOAD_FRONTEND_MISSION_AUTO)
		{
			ssprintf(NewSaveGamePath, "%s%s/auto/", SaveGamePath, "campaign");
		}
		else
		{
			ssprintf(NewSaveGamePath, "%s%s/", SaveGamePath, "campaign");
		}
	}
	if (id == LOADENTRY_START && modeLoad) // [auto] or [..], ignore click for saves
	{
		int iLoadSaveMode = (int)bLoadSaveMode; // for evil integer arithmetics
		bLoadSaveMode = (enum LOADSAVE_MODE)(iLoadSaveMode ^ 16); // toggle _AUTO bit
		closeLoadSave(); // decrement game time / pause counters
		addLoadSave(bLoadSaveMode, savedTitle); // restart in other directory
		return false;
	}
	// clicked a load entry
	else if (id > LOADENTRY_START  &&  id <= LOADENTRY_END)
	{
		W_BUTTON *slotButton = (W_BUTTON *)widgGetFromID(psRequestScreen, id);

		if (modeLoad)								// Loading, return that entry.
		{
			if (!slotButton->pText.isEmpty())
			{
				ssprintf(sRequestResult, "%s%s%s", NewSaveGamePath, ((W_BUTTON *)widgGetFromID(psRequestScreen, id))->pText.toUtf8().c_str(), sSaveGameExtension);
			}
			else
			{
				return false;				// clicked on an empty box
			}

			runLoadCleanup();
			return true;
		}
		else //  SAVING!add edit box at that position.
		{

			if (! widgGetFromID(psRequestScreen, SAVEENTRY_EDIT))
			{
				WIDGET *parent = widgGetFromID(psRequestScreen, LOADSAVE_FORM);

				// add blank box.
				auto saveEntryEdit = std::make_shared<W_EDITBOX>();
				parent->attach(saveEntryEdit);
				saveEntryEdit->id = SAVEENTRY_EDIT;
				saveEntryEdit->setGeometry(slotButton->geometry());
				saveEntryEdit->setString(slotButton->getString());
				saveEntryEdit->setBoxColours(WZCOL_MENU_LOAD_BORDER, WZCOL_MENU_LOAD_BORDER, WZCOL_MENU_BACKGROUND);

				if (!slotButton->pText.isEmpty())
				{
					ssprintf(sDelete, "%s%s%s", NewSaveGamePath, slotButton->pText.toUtf8().c_str(), sSaveGameExtension);
				}
				else
				{
					WzString suggestedSaveName = suggestSaveName(NewSaveGamePath);
					saveEntryEdit->setString(suggestedSaveName);
					sstrcpy(sDelete, "");
				}

				slotButton->hide();  // hide the old button
				chosenSlotId = id;

				// auto click in the edit box we just made.
				W_CONTEXT context = W_CONTEXT::ZeroContext();
				context.mx			= mouseX();
				context.my			= mouseY();
				saveEntryEdit->clicked(&context);
			}
		}
	}

	// finished entering a name.
	if (id == SAVEENTRY_EDIT)
	{
		char sTemp[MAX_STR_LENGTH];

		if (!keyPressed(KEY_RETURN) && !keyPressed(KEY_KPENTER))						// enter was not pushed, so not a vaild entry.
		{
			widgDelete(psRequestScreen, SAVEENTRY_EDIT);	//unselect this box, and go back ..
			widgReveal(psRequestScreen, chosenSlotId);
			return true;
		}


		// scan to see if that game exists in another slot, if so then fail.
		sstrcpy(sTemp, widgGetString(psRequestScreen, id));

		for (unsigned i = LOADENTRY_START; i < LOADENTRY_END; ++i)
		{
			if (i != chosenSlotId)
			{

				if (!((W_BUTTON *)widgGetFromID(psRequestScreen, i))->pText.isEmpty()
				    && strcmp(sTemp, ((W_BUTTON *)widgGetFromID(psRequestScreen, i))->pText.toUtf8().c_str()) == 0)
				{
					widgDelete(psRequestScreen, SAVEENTRY_EDIT);	//unselect this box, and go back ..
					widgReveal(psRequestScreen, chosenSlotId);
					audio_PlayTrack(ID_SOUND_BUILD_FAIL);
					return true;
				}
			}
		}


		// return with this name, as we've edited it.
		if (strlen(widgGetString(psRequestScreen, id)))
		{
			sstrcpy(sTemp, widgGetString(psRequestScreen, id));
			removeWildcards(sTemp);
			snprintf(sRequestResult, sizeof(sRequestResult), "%s%s%s", NewSaveGamePath, sTemp, sSaveGameExtension);
			if (strlen(sDelete) != 0)
			{
				deleteSaveGame(sDelete);	//only delete game if a new game fills the slot
			}
		}

		runLoadSaveCleanup(bResetMissionWidgets, false);
		return true;
	}

	return false;
}

// ////////////////////////////////////////////////////////////////////////////
// should be done when drawing the other widgets.
bool displayLoadSave()
{
	widgDisplayScreen(psRequestScreen);	// display widgets.
	return true;
}


// ////////////////////////////////////////////////////////////////////////////
// char HANDLER, replaces dos wildcards in a string with harmless chars.
void removeWildcards(char *pStr)
{
	UDWORD i;

	// Remember never to allow: < > : " / \ | ? *

	// Whitelist: Get rid of any characters except:
	// a-z A-Z 0-9 - + ! , = ^ @ # $ % & ' ( ) [ ] (and space and unicode characters ≥ 0x80)
	for (i = 0; i < strlen(pStr); i++)
	{
		if (!isalnum(pStr[i])
		    && (pStr[i] != ' ' || i == 0 || pStr[i - 1] == ' ')
		    // We allow spaces as long as they aren't the first char, or two spaces in a row
		    && pStr[i] != '-'
		    && pStr[i] != '+'
		    && pStr[i] != '[' && pStr[i] != ']'
		    && (pStr[i] & 0x80) != 0x80 // á é í ó ú α β γ δ ε
		   )
		{
			pStr[i] = '_';
		}
	}

	if (strlen(pStr) >= MAX_SAVE_NAME)
	{
		pStr[MAX_SAVE_NAME - 1] = 0;
	}
	else if (strlen(pStr) == 0)
	{
		pStr[0] = '!';
		pStr[1] = 0;
		return;
	}
	// Trim trailing spaces
	for (i = strlen(pStr); i > 0 && pStr[i - 1] == ' '; --i)
	{}
	pStr[i] = 0;

	// If that leaves us with a blank string, replace with '!'
	if (pStr[0] == 0)
	{
		pStr[0] = '!';
		pStr[1] = 0;
	}
}

// ////////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////////
// DISPLAY FUNCTIONS

static void displayLoadBanner(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset)
{
	PIELIGHT col;
	int x = xOffset + psWidget->x();
	int y = yOffset + psWidget->y();

	if (psWidget->pUserData)
	{
		col = WZCOL_GREEN;
	}
	else
	{
		col = WZCOL_MENU_LOAD_BORDER;
	}

	pie_BoxFill(x, y, x + psWidget->width(), y + psWidget->height(), col);
	pie_BoxFill(x + 2, y + 2, x + psWidget->width() - 2, y + psWidget->height() - 2, WZCOL_MENU_BACKGROUND);
}

// ////////////////////////////////////////////////////////////////////////////
static void displayLoadSlot(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset)
{
	// Any widget using displayLoadSlot must have its pUserData initialized to a (LoadSaveDisplayLoadSlotCache*)
	assert(psWidget->pUserData != nullptr);
	LoadSaveDisplayLoadSlotCache& cache = *static_cast<LoadSaveDisplayLoadSlotCache *>(psWidget->pUserData);

	int x = xOffset + psWidget->x();
	int y = yOffset + psWidget->y();
	char  butString[64];

	drawBlueBox(x, y, psWidget->width(), psWidget->height());  //draw box

	if (!((W_BUTTON *)psWidget)->pText.isEmpty())
	{
		sstrcpy(butString, ((W_BUTTON *)psWidget)->pText.toUtf8().c_str());

		if (cache.fullText != butString)
		{
			// Update cache
			while (iV_GetTextWidth(butString, font_regular) > psWidget->width())
			{
				butString[strlen(butString) - 1] = '\0';
			}
			cache.wzText.setText(butString, font_regular);
			cache.fullText = butString;
		}

		cache.wzText.render(x + 4, y + 17, WZCOL_FORM_TEXT);
	}
}

void drawBlueBoxInset(UDWORD x, UDWORD y, UDWORD w, UDWORD h)
{
	pie_BoxFill(x, y, x + w, y + h, WZCOL_MENU_BORDER);
	pie_BoxFill(x + 1, y + 1, x + w - 1, y + h - 1, WZCOL_MENU_BACKGROUND);
}

/**
 * Same as drawBlueBoxInset, but the rectangle is overflown by one pixel.
 */
void drawBlueBox(UDWORD x, UDWORD y, UDWORD w, UDWORD h)
{
	drawBlueBoxInset(x - 1, y - 1, w + 2, h + 2);
}

static void freeAutoSaveSlot(const char *path)
{
	char **i, **files;
	files = PHYSFS_enumerateFiles(path);
	ASSERT_OR_RETURN(, files, "PHYSFS_enumerateFiles(\"%s\") failed: %s", path, WZ_PHYSFS_getLastError());
	int nfiles = 0;
	for (i = files; *i != nullptr; ++i)
	{
		if (!isASavedGamefile(*i, sSaveGameExtension))
		{
			// If it doesn't, move on to the next filename
			continue;
		}
		nfiles++;
	}
	if (nfiles < totalslots)
	{
		PHYSFS_freeList(files);
		return;
	}

	// too many autosaves, let's delete the oldest
	char oldestSavePath[PATH_MAX];
	time_t oldestSaveTime = time(nullptr);
	for (i = files; *i != nullptr; ++i)
	{
		char savefile[PATH_MAX];

		if (!isASavedGamefile(*i, sSaveGameExtension))
		{
			// If it doesn't, move on to the next filename
			continue;
		}
		/* Figure save-time */
		snprintf(savefile, sizeof(savefile), "%s/%s", path, *i);
		time_t savetime = WZ_PHYSFS_getLastModTime(savefile);
		if (difftime(savetime, oldestSaveTime) < 0.0)
		{
			oldestSaveTime = savetime;
			strcpy(oldestSavePath, savefile);
		}
	}
	PHYSFS_freeList(files);
	deleteSaveGame(oldestSavePath);
}

bool autoSave()
{
	// Bail out if we're running a _true_ multiplayer game or are playing a tutorial/debug/cheating/autogames
	const DebugInputManager& dbgInputManager = gInputManager.debugManager();
	if (!autosaveEnabled || runningMultiplayer() || bInTutorial || dbgInputManager.debugMappingsAllowed() || Cheated || autogame_enabled())
	{
		return false;
	}
	const char *dir = bMultiPlayer? AUTOSAVE_SKI_DIR : AUTOSAVE_CAM_DIR;
	freeAutoSaveSlot(dir);

	time_t now = time(nullptr);
	struct tm timeinfo = getLocalTime(now);
	char savedate[PATH_MAX];
	strftime(savedate, sizeof(savedate), "%F_%H%M%S", &timeinfo);

	std::string withoutTechlevel = mapNameWithoutTechlevel(getLevelName());
	char savefile[PATH_MAX];
	snprintf(savefile, sizeof(savefile), "%s/%s_%s.gam", dir, withoutTechlevel.c_str(), savedate);
	if (saveGame(savefile, GTYPE_SAVE_MIDMISSION))
	{
		console("AutoSave %s", savefile);
		return true;
	}
	else
	{
		console("AutoSave %s failed", savefile);
		return false;
	}
}
