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
#include "lib/framework/file.h"
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
#include "game.h"
#include "version.h"
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
// Note: this is left for backward compatibiiliy. Remove it later.
static bool isASavedGamefile(const char *filename, const char *extension)
{
	if (nullptr == filename)
	{
		return false;
	}

	size_t filenameLength = strlen(filename);
	size_t extensionLength = strlen(extension);
	// reject filename of insufficient length to contain "<anything>.gam"
	return extensionLength < filenameLength && 0 == strcmp(filename + filenameLength - extensionLength, extension);
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

static std::string getNewSaveGamePathFromMode(LOADSAVE_MODE savemode)
{
	std::string result;

	switch (savemode)
	{
	case LOAD_FRONTEND_MISSION:
	case LOAD_INGAME_MISSION:
	case LOAD_MISSIONEND:
		result = astringf("%s%s/", SaveGamePath, "campaign");
		break;
	case LOAD_FRONTEND_SKIRMISH:
	case LOAD_INGAME_SKIRMISH:
		result = astringf("%s%s/", SaveGamePath, "skirmish");
		break;
	case LOAD_FRONTEND_MISSION_AUTO:
	case LOAD_INGAME_MISSION_AUTO:
	case LOAD_MISSIONEND_AUTO:
		result = astringf("%s%s/", SaveGamePath, "campaign/auto");
		break;
	case LOAD_FRONTEND_SKIRMISH_AUTO:
	case LOAD_INGAME_SKIRMISH_AUTO:
		result = astringf("%s%s/", SaveGamePath, "skirmish/auto");
		break;
	case SAVE_MISSIONEND:
	case SAVE_INGAME_MISSION:
		result = astringf("%s%s/", SaveGamePath, "campaign");
		break;
	case SAVE_INGAME_SKIRMISH:
		result = astringf("%s%s/", SaveGamePath, "skirmish");
		break;
	case LOADREPLAY_FRONTEND_SKIRMISH:
		result = astringf("%s%s/", ReplayPath, "skirmish");
		break;
	case LOADREPLAY_FRONTEND_MULTI:
		result = astringf("%s%s/", ReplayPath, "multiplay");
		break;
	default:
		ASSERT("Invalid load/save mode!", "Invalid load/save mode!");
		result = astringf("%s%s/", SaveGamePath, "campaign");
		break;
	}

	return result;
}

// ////////////////////////////////////////////////////////////////////////////
bool addLoadSave(LOADSAVE_MODE savemode, const char *title)
{
	std::string NewSaveGamePath;
	bLoadSaveMode = savemode;
	savedTitle = title;
	UDWORD			slotCount;

	// Static as these are assigned to the widget buttons by reference
	static char	sSlotCaps[totalslots][totalslotspace];
	static char	sSlotTips[totalslots][totalslotspace];

	bool bLoad = true;
	bool bReplay = false;
	NewSaveGamePath = getNewSaveGamePathFromMode(savemode);
	switch (savemode)
	{
	case SAVE_MISSIONEND:
	case SAVE_INGAME_MISSION:
	case SAVE_INGAME_SKIRMISH:
		bLoad = false;
		break;
	case LOADREPLAY_FRONTEND_SKIRMISH:
	case LOADREPLAY_FRONTEND_MULTI:
		bReplay = true;
		break;
	default:
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

	debug(LOG_SAVE, "Searching \"%s\" for savegames", NewSaveGamePath.c_str());

	// add savegame filenames minus extensions to buttons

	struct SaveGameNamesAndTimes
	{
		std::string name;
		int64_t epoch;

		SaveGameNamesAndTimes(std::string name, int64_t epoch)
		: name(std::move(name))
		, epoch(epoch)
		{ }
	};

	std::vector<SaveGameNamesAndTimes> saveGameNamesAndTimes;
	auto latestTagResult = version_extractVersionNumberFromTag(version_getLatestTag());
	ASSERT(latestTagResult.has_value(), "No extractable latest tag?? - Please try re-downloading the latest official source bundle");
	const TagVer buildTagVer = latestTagResult.value_or(TagVer());
	try
	{		
		WZ_PHYSFS_enumerateFolders(NewSaveGamePath, [NewSaveGamePath, &buildTagVer, &saveGameNamesAndTimes](const char* dirName){
			if (strcmp(dirName, "auto") == 0)
			{
				return true; // continue
			}
			const auto saveInfoFilename = std::string(NewSaveGamePath) + dirName + "/save-info.json";

			// avoid spamming stdout if doesn't exist
			if (!PHYSFS_exists(saveInfoFilename.c_str())) return true;

			const auto saveInfoDataOpt = parseJsonFile(saveInfoFilename.c_str());
			if (saveInfoDataOpt.has_value())
			{
				const auto saveInfoData = saveInfoDataOpt.value();
				// decide what savegames are viewable/loadable
				// assume that we can safely load games older than current version
				const auto saveLatestTagArray = saveInfoData.at("latestTagArray").get<std::vector<uint16_t>>();
				TagVer saveTagVer(saveLatestTagArray, "");
				if (saveTagVer <= buildTagVer)
				{
					// looks like loadable
					saveGameNamesAndTimes.emplace_back(std::string(dirName), saveInfoData.at("epoch").get<int64_t>());
				}
				else
				{
					debug(LOG_SAVEGAME, "not showing savegame '%s', because version is higher than this game", saveInfoFilename.c_str());
				}
			}
			return true;
		});
	} catch( nlohmann::json::exception &e)
	{
		debug(LOG_ERROR, "can't load game: %s", e.what());
		// continue, because still may find old .gam to load
	}

	if (bReplay)
	{
		char const *extension = sSaveReplayExtension;
		size_t extensionLen = strlen(extension);
		// Note: this is left for backward compatibility reasons.
		// Previously added old .gam saves. Now it exists to look for replay files.
		WZ_PHYSFS_enumerateFiles(NewSaveGamePath.c_str(), [NewSaveGamePath, &saveGameNamesAndTimes, extension, extensionLen](const char *i) -> bool {
			char savefile[256];
			time_t savetime;

			if (!isASavedGamefile(i, extension))
			{
				// If it doesn't, move on to the next filename
				return true;
			}

			debug(LOG_SAVE, "We found [%s]", i);

			/* Figure save-time */
			snprintf(savefile, sizeof(savefile), "%s/%s", NewSaveGamePath.c_str(), i);
			savetime = WZ_PHYSFS_getLastModTime(savefile);

			size_t lenIWithoutExtension = strlen(i) - extensionLen;
			std::string fileNameWithoutExtension(i, lenIWithoutExtension);
			for(auto &el: saveGameNamesAndTimes)
			{
				// only add if doesn't exist yet
				if (el.name.compare(fileNameWithoutExtension) == 0)
				{
					return true; // move to next
				}
			}
			saveGameNamesAndTimes.emplace_back(std::move(fileNameWithoutExtension), savetime);
			return true;
		});
	}

	// Sort the save games so that the most recent one appears first
	std::sort(saveGameNamesAndTimes.begin(),
			  saveGameNamesAndTimes.end(),
			  [](SaveGameNamesAndTimes& a, SaveGameNamesAndTimes& b) { return a.epoch > b.epoch; });

	// Now store the sorted save game names to the buttons
	slotCount = 1;
	(void)std::all_of(saveGameNamesAndTimes.begin(), saveGameNamesAndTimes.end(), [&](SaveGameNamesAndTimes& saveGameNameAndTime)
		{
			/* Set the button-text and tip text (the save time) into static storage */
			sstrcpy(sSlotCaps[slotCount], saveGameNameAndTime.name.c_str());
			auto newtime = getLocalTime(saveGameNameAndTime.epoch);
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
// TODO: Replace the old uses of .gam files with the savegame folder and call deleteSaveGame directly?
void deleteSaveGame_classic(const char *gamFileName)
{
	ASSERT_OR_RETURN(, gamFileName != nullptr, "Null gamFileName");
	std::string gamFolderPath = gamFileName;

	// Old method of stripping file extension - makes lots of assumptions (like there *is* a file extension and it has a length of 4 including the period)
	ASSERT_OR_RETURN(, gamFolderPath.size() > 4, "Input gamFileName too short?: %s", gamFileName);
	gamFolderPath.resize(gamFolderPath.size() - 4);

	deleteSaveGame(gamFolderPath);
}

void deleteSaveGame(std::string saveGameFolderPath)
{
	// Remove any trailing path separators (/)
	while (!saveGameFolderPath.empty() && (saveGameFolderPath.rfind("/", std::string::npos) == (saveGameFolderPath.length() - 1)))
	{
		saveGameFolderPath.resize(saveGameFolderPath.length() - 1); // Remove trailing path separators
	}

	// First, check if some old files in the parent directory (named the same as this directory, but with specific extensions) exist
	// (since we removed trailing directory separators above, we can just append the appropriate file extensions)
	std::string oldParentFile = saveGameFolderPath + ".gam";
	if (PHYSFS_exists(WzString::fromUtf8(oldParentFile)) != 0)
	{
		PHYSFS_delete(oldParentFile.c_str());
	}
	oldParentFile = saveGameFolderPath + ".es"; // ancient script data file
	if (PHYSFS_exists(WzString::fromUtf8(oldParentFile)) != 0)
	{
		PHYSFS_delete(oldParentFile.c_str());
	}

	// check for a directory and remove that too.
	WZ_PHYSFS_enumerateFiles(saveGameFolderPath.c_str(), [saveGameFolderPath](const char *i) -> bool {
		// Construct the full path to the file by appending the
		// filename to the directory it is in.
		std::string del_file = astringf("%s/%s", saveGameFolderPath.c_str(), i);

		debug(LOG_SAVE, "Deleting [%s].", del_file.c_str());

		// Delete the file
		if (!PHYSFS_delete(del_file.c_str()))
		{
			debug(LOG_ERROR, "Warning [%s] could not be deleted due to PhysicsFS error: %s", del_file.c_str(), WZ_PHYSFS_getLastError());
		}
		return true; // continue
	});

	if (WZ_PHYSFS_isDirectory(saveGameFolderPath.c_str()) && !PHYSFS_delete(saveGameFolderPath.c_str()))		// now (should be)empty directory
	{
		debug(LOG_ERROR, "Warning directory[%s] could not be deleted because %s", saveGameFolderPath.c_str(), WZ_PHYSFS_getLastError());
	}
}

SaveGamePath_t lastSavePath;
bool lastSaveMP;
static int64_t lastSaveTime;

static bool findLastSaveFrom(SAVEGAME_LOC loc)
{
	bool found = false;
	const char *path = SaveGameLocToPath[loc];
	debug(LOG_SAVEGAME, "looking for last save in %s", path);
	const std::string pathToCommonSaveDir = std::string(path);
	try
	{
		WZ_PHYSFS_enumerateFolders(pathToCommonSaveDir, [&found, &pathToCommonSaveDir, loc] (const char * dirName) {
			if (strcmp(dirName, "auto") == 0)
			{
				// skip "auto" folder, it will be iterated on its own
				return true;
			}
			const std::string pathToSaveInfo = pathToCommonSaveDir + + "/" + std::string(dirName) + "/save-info.json";
			if (PHYSFS_exists(pathToSaveInfo.c_str()))
			{
				const auto saveInfoDataOpt = parseJsonFile(pathToSaveInfo.c_str());
				if (!saveInfoDataOpt.has_value())
				{
					debug(LOG_SAVEGAME, "weird directory without save-info.json: %s", dirName);
					return true;
				}
				const auto saveInfo = saveInfoDataOpt.value();
				const auto epoch = saveInfo.at("epoch").get<int64_t>();
				if (epoch > lastSaveTime)
				{
					lastSaveTime = epoch;
					found = true;
					lastSavePath.loc = loc;
					lastSavePath.gameName = std::string(dirName);
					debug(LOG_SAVEGAME, "found last saved game: %s%s", pathToCommonSaveDir.c_str(), lastSavePath.gameName.c_str());
				}
			}
			return true;
		});
	} catch (nlohmann::json::exception &e)
	{
		debug(LOG_ERROR, "find last save failed:%s", e.what());
		return false;
	}

	return found;
}

bool findLastSave()
{
	bool foundMP, foundCAM;
	lastSaveTime = 0;
	foundCAM = findLastSaveFrom(SAVEGAME_LOC_CAM);
	foundCAM |= findLastSaveFrom(SAVEGAME_LOC_CAM_AUTO);
	foundMP = findLastSaveFrom(SAVEGAME_LOC_SKI);
	foundMP |= findLastSaveFrom(SAVEGAME_LOC_SKI_AUTO);
	lastSaveMP = foundMP;
	return foundMP | foundCAM;
}

static WzString suggestSaveName(const char *saveGamePath)
{
	const WzString levelName = getLevelName();
	const std::string levelNameStr = levelName.toStdString();
	const std::string cheatedSuffix = Cheated ? _("cheated") : "";
	char saveNamePartial[64] = "\0";

	if (!bMultiPlayer)
	{
		std::string campaignName;
		std::string missionName = levelName.toStdString();
		std::string endName = "End"; //last mission for one the campaigns
		std::vector<std::string> levelNames = {};

		if (levelName.startsWith("CAM_1") || levelName.startsWith("SUB_1"))
		{
			campaignName = "Alpha";
			levelNames = {
				"CAM_1A", "CAM_1B", "SUB_1_1", "SUB_1_2", "SUB_1_3", "CAM_1C",
				"CAM_1CA", "SUB_1_4A", "SUB_1_5", "CAM_1A-C", "SUB_1_7", "SUB_1_D", "CAM_1END"
			};
		}
		else if (levelName.startsWith("CAM_2") || levelName.startsWith("SUB_2"))
		{
			campaignName = "Beta";
			levelNames = {
				"CAM_2A", "SUB_2_1", "CAM_2B", "SUB_2_2", "CAM_2C", "SUB_2_5",
				"SUB_2D", "SUB_2_6", "SUB_2_7", "SUB_2_8", "CAM_2END"
			};
		}
		else if (levelName.startsWith("CAM_3") || levelName.startsWith("CAM3") || levelName.startsWith("SUB_3"))
		{
			campaignName = "Gamma";
			levelNames = {
				"CAM_3A", "SUB_3_1", "CAM_3B", "SUB_3_2", "CAM3A-B",
				"CAM3C", "CAM3A-D1", "CAM3A-D2", "CAM_3_4"
			};
		}

		for (size_t i = 0; i < levelNames.size(); ++i)
		{
			std::string fullMapName = levelNames[i];
			if (levelNameStr.find(fullMapName) != std::string::npos)
			{
				bool endsWithS = levelNameStr[levelNameStr.length() - 1] == 'S';
				if (!endsWithS && (levelNameStr.compare(fullMapName) != 0))
				{
					continue;
				}
				missionName = std::to_string(i + 1);
				if ((i + 1) == levelNames.size())
				{
					missionName = endName;
				}
				if ((missionName.compare(endName) == 0) && !levelName.startsWith("CAM_3")) //Gamma end is an exception
				{
					break;
				}
				if (endsWithS)
				{
					missionName += " Base"; // The Project's base for the campaign map.
				}
				break;
			}
		}

		ssprintf(saveNamePartial, "%s %s %s", campaignName.c_str(), missionName.c_str(), cheatedSuffix.c_str());
	}
	else
	{
		ssprintf(saveNamePartial, "%s %s", mapNameWithoutTechlevel(levelNameStr.c_str()).c_str(), cheatedSuffix.c_str());
	}

	WzString saveName = WzString(saveNamePartial).trimmed();
	int similarSaveGames = 0;
	WZ_PHYSFS_enumerateFolders(saveGamePath, [&saveName, &similarSaveGames](const char *dirName) {
		std::string dirNameStr = WzString(dirName).toStdString();
		std::string saveNameStr = saveName.toStdString();
		size_t pos = dirNameStr.find(saveNameStr);

		if (pos == std::string::npos)
		{
			return true; //An original save name
		}

		std::string restOfSaveName = dirNameStr.substr(pos);
		if (restOfSaveName.compare(saveNameStr) == 0)
		{
			++similarSaveGames; //direct match
			return true;
		}

		//chop off at the last space+number, if it exists, and compare again
		size_t lastSpace = restOfSaveName.find_last_of(" ");
		if (lastSpace != std::string::npos)
		{
			if (isdigit(restOfSaveName[lastSpace + 1]))
			{
				std::string tempStr = restOfSaveName.substr(0, lastSpace);
				if (tempStr.compare(saveNameStr) == 0)
				{
					++similarSaveGames;
					return true;
				}
			}
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
	std::string		NewSaveGamePath;

	WidgetTriggers const &triggers = widgRunScreen(psRequestScreen);
	unsigned id = triggers.empty() ? 0 : triggers.front().widget->id; // Just use first click here, since the next click could be on another menu.

	sstrcpy(sRequestResult, "");					// set returned filename to null;

	if (id == LOADSAVE_CANCEL || CancelPressed())
	{
		runLoadSaveCleanup(bResetMissionWidgets, true);
		return true;
	}
	NewSaveGamePath = getNewSaveGamePathFromMode(bLoadSaveMode);
	if (id == LOADENTRY_START && modeLoad && !modeReplay) // [auto] or [..], ignore click for saves
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
			if (slotButton->pText.isEmpty())
			{
				return false;  // clicked on an empty box
			}
			ssprintf(sRequestResult, "%s%s%s", NewSaveGamePath.c_str(), ((W_BUTTON *)widgGetFromID(psRequestScreen, id))->pText.toUtf8().c_str(), modeReplay? sSaveReplayExtension : sSaveGameExtension);

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
					ssprintf(sDelete, "%s%s%s", NewSaveGamePath.c_str(), slotButton->pText.toUtf8().c_str(), sSaveGameExtension);
				}
				else
				{
					WzString suggestedSaveName = suggestSaveName(NewSaveGamePath.c_str());
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
					audio_PlayBuildFailedOnce();
					return true;
				}
			}
		}


		// return with this name, as we've edited it.
		if (strlen(widgGetString(psRequestScreen, id)))
		{
			sstrcpy(sTemp, widgGetString(psRequestScreen, id));
			removeWildcards(sTemp);
			snprintf(sRequestResult, sizeof(sRequestResult), "%s%s%s", NewSaveGamePath.c_str(), sTemp, sSaveGameExtension);
			if (strlen(sDelete) != 0)
			{
				deleteSaveGame_classic(sDelete);	//only delete game if a new game fills the slot
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

// Note: remove later at some point
// returns true if something was deleted
static bool freeAutoSaveSlot_old(const char *path)
{
	return WZ_PHYSFS_cleanupOldFilesInFolder(path, sSaveGameExtension, -1, [](const char *fileName){ deleteSaveGame_classic(fileName); return true; }) > 0;
}

static void freeAutoSaveSlot(SAVEGAME_LOC loc)
{
	const char *path = SaveGameLocToPath[loc];
	int64_t oldestEpoch = INT64_MAX;
	std::string oldestKey;
	unsigned count = 0;
	try
	{
		WZ_PHYSFS_enumerateFolders(path, [path, &oldestKey, &oldestEpoch, &count](const char* dirName){
			if (!dirName) { return true; }
			if (strcmp(dirName, "auto") == 0)
			{
				return true; // continue
			}
			count++;
			const auto saveInfoFilename = std::string(path) + "/" + dirName + "/save-info.json";
			auto saveInfoDataOpt = parseJsonFile(saveInfoFilename.c_str());
			if (!saveInfoDataOpt.has_value() || !PHYSFS_exists(saveInfoFilename.c_str()))
			{
				// nothing to do, this has been handled by old routine
				return true;
			}
			const auto saveInfoData = saveInfoDataOpt.value();
			if (!saveInfoData.is_object())
			{
				// just overwrite this (apparently) corrupt autosave
				oldestEpoch = 0;
				oldestKey = std::string(dirName);
				return true;
			}
			const auto epoch = saveInfoData.at("epoch").get<int64_t>();
			if (epoch < oldestEpoch)
			{
				oldestEpoch = epoch;
				oldestKey = std::string(dirName);
			}
			return true;
		});
	} catch (nlohmann::json::exception &e)
	{
		debug(LOG_ERROR, "failed to remove an autosave game: %s", e.what());
		return;
	}

	if (count < totalslots)
	{
		return;
	}
	ASSERT_OR_RETURN(, oldestKey.size() > 0, "Bug: oldestKey can't be empty here");
	char savefile[PATH_MAX];
	snprintf(savefile, sizeof(savefile), "%s/%s.gam", path, oldestKey.c_str());
	debug(LOG_SAVEGAME, "deleting the oldest autosave file %s", savefile);
	deleteSaveGame_classic(savefile);
}

bool autoSave()
{
	// Bail out if we're running a _true_ multiplayer game or are playing a tutorial/debug/cheating/autogames
	const DebugInputManager& dbgInputManager = gInputManager.debugManager();
	if (!autosaveEnabled || runningMultiplayer() || bInTutorial || dbgInputManager.debugMappingsAllowed() || Cheated || autogame_enabled())
	{
		return false;
	}
	// Bail out if we're running a replay
	if (NETisReplay())
	{
		return false;
	}
	const char *dir = bMultiPlayer ? SAVEGAME_SKI_AUTO : SAVEGAME_CAM_AUTO;
	// Backward compatibility: remove later
	if (!freeAutoSaveSlot_old(dir))
	{
		// no old .gam found: check for new saves
		freeAutoSaveSlot(bMultiPlayer ? SAVEGAME_LOC_SKI_AUTO : SAVEGAME_LOC_CAM_AUTO);
	}
	
	time_t now = time(nullptr);
	struct tm timeinfo = getLocalTime(now);
	char savedate[PATH_MAX];
	strftime(savedate, sizeof(savedate), "%F_%H%M%S", &timeinfo);

	std::string suggestedName = suggestSaveName(dir).toStdString();
	char savefile[PATH_MAX];
	snprintf(savefile, sizeof(savefile), "%s/%s_%s.gam", dir, suggestedName.c_str(), savedate);
	if (saveGame(savefile, GTYPE_SAVE_MIDMISSION))
	{
		console(_("AutoSave %s"), savegameWithoutExtension(savefile));
		return true;
	}
	else
	{
		console(_("AutoSave %s failed"), savegameWithoutExtension(savefile));
		return false;
	}
}
