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

#ifndef __INCLUDED_SRC_KEYBIND_H__
#define __INCLUDED_SRC_KEYBIND_H__

#include "console.h"
#include "selection.h"
#include "orderdef.h"
#include "lib/framework/fixedpoint.h"

#define	MAP_ZOOM_RATE_MAX	(1000)
#define	MAP_ZOOM_RATE_MIN	(200)
#define	MAP_ZOOM_RATE_DEFAULT	(500)
#define	MAP_ZOOM_RATE_STEP	(50)

#define MAP_PITCH_RATE		(SPIN_SCALING/SECS_PER_SPIN)

extern int scrollDirUpDown;
extern int scrollDirLeftRight;

// --------------- All those keyboard mappable functions */
void kf_HalveHeights();
void kf_DebugDroidInfo();
void kf_BuildInfo();
void kf_ToggleFPS();			//FPS counter NOT same as kf_Framerate! -Q
void kf_ToggleUnitCount();		// Display units built / lost / produced counter
void kf_ToggleSamples();		// Displays # of sound samples in Queue/list.
void kf_ToggleOrders();		//displays unit's Order/action state.
void kf_FrameRate();
void kf_ShowNumObjects();
void kf_ToggleRadar();
void kf_TogglePower();
void kf_RecalcLighting();
void kf_ScreenDump();
void kf_AllAvailable();
void kf_TriFlip();
void kf_ToggleFog();
void kf_ToggleShadows();
void kf_ToggleCamera();
void kf_RaiseTile();
void kf_LowerTile();
void kf_MapCheck();
MappableFunction kf_Zoom(const int multiplier);
void kf_RotateLeft();
void kf_RotateRight();
void kf_RotateBuildingCW();
void kf_RotateBuildingACW();
void kf_PitchBack();
void kf_PitchForward();
void kf_ResetPitch();
void kf_ShowMappings();
void kf_SelectGrouping(UDWORD groupNumber);
MappableFunction kf_SelectGrouping_N(const unsigned int n);
MappableFunction kf_AssignGrouping_N(const unsigned int n);
MappableFunction kf_AddGrouping_N(const unsigned int n);
void kf_ToggleDroidInfo();
void kf_addInGameOptions();
void kf_addMultiMenu();
MappableFunction kf_JumpToMapMarker(const unsigned int x, const unsigned int z, const int yaw);
void kf_TogglePowerBar();
void kf_ToggleDebugMappings();
void kf_PrioritizeDebugMappings();
void kf_ToggleLevelEditor();
void kf_ToggleGodMode();
MappableFunction kf_ScrollCamera(const int horizontal, const int vertical);
void kf_SeekNorth();
void kf_MaxScrollLimits();
void kf_toggleTrapCursor();
void kf_TogglePauseMode();

void kf_ToggleEnergyBars();
void kf_FinishAllResearch();
void kf_FinishResearch();
void kf_ToggleOverlays();
void kf_ChooseOptions();
void kf_ChooseCommand();
void kf_ChooseManufacture();
void kf_ChooseResearch();
void kf_ChooseBuild();
void kf_ChooseDesign();
void kf_ChooseIntelligence();
void kf_ChooseCancel();
void kf_ToggleWeather();
void kf_KillSelected();
void kf_SendGlobalMessage();
void kf_SendTeamMessage();
void kf_ToggleConsole();
void kf_ToggleTeamChat();
MappableFunction kf_SelectUnits(const SELECTIONTYPE selectionType, const SELECTION_CLASS selectionClass = SELECTION_CLASS::DS_BY_TYPE, const bool bOnScreen = false);
MappableFunction kf_SelectNoGroupUnits(const SELECTIONTYPE selectionType, const SELECTION_CLASS selectionClass = SELECTION_CLASS::DS_BY_TYPE, const bool bOnScreen = false);
MappableFunction kf_SetDroid(const SECONDARY_ORDER order, const SECONDARY_STATE state);
MappableFunction kf_OrderDroid(const DroidOrderType order);

void kf_CentreOnBase();
void kf_ToggleFog();
void kf_MoveToLastMessagePos();
void kf_RightOrderMenu();

extern bool bAllowOtherKeyPresses;

void kf_TriggerRayCast();
void kf_ToggleFormationSpeedLimiting();
void kf_ToggleSensorDisplay();
void kf_JumpToResourceExtractor();
MappableFunction kf_JumpToUnits(const DROID_TYPE droidType);
void kf_AddHelpBlip();
void kf_ToggleProximitys();

void kf_JumpToUnassignedUnits();
void kf_ToggleVisibility();
MappableFunction kf_RadarZoom(const int multiplier);
MappableFunction kf_SelectNextFactory(const STRUCTURE_TYPE factoryType, const bool bJumpToSelected = false);
MappableFunction kf_SelectNextPowerStation(const bool bJumpToSelected = false);
MappableFunction kf_SelectNextResearch(const bool bJumpToSelected = false);
void kf_ToggleConsoleDrop();
void kf_ToggleShakeStatus();
void kf_ToggleMouseInvert();
void kf_BifferBaker();
void kf_SetEasyLevel();
void kf_SetNormalLevel();
void kf_DoubleUp();
void kf_UpThePower();
void kf_MaxPower();
void kf_KillEnemy();
void kf_ToggleMissionTimer();
void kf_TraceObject();

void kf_SetHardLevel();
MappableFunction kf_SelectCommander_N(const unsigned int n);

void kf_ToggleShowGateways();
void kf_ToggleShowPath();

void kf_FaceNorth();
void kf_FaceSouth();
void kf_FaceEast();
void kf_FaceWest();
void kf_ToggleRadarJump();
void kf_MovePause();

void kf_SpeedUp();
void kf_SlowDown();
void kf_NormalSpeed();

void kf_TeachSelected();
void kf_MakeMeHero();
void kf_Unselectable();
void kf_CloneSelected(int);
void kf_Reload();

#define SPIN_SCALING	(360*DEG_1)
#define	SECS_PER_SPIN	2
#define MAP_SPIN_RATE	(SPIN_SCALING/SECS_PER_SPIN)

void kf_ToggleRadarTerrain();            //radar terrain
void kf_ToggleRadarAllyEnemy();          //enemy/ally color toggle

void kf_TileInfo();

void kf_NoAssert();

void kf_RevealMapAtPos();

bool runningMultiplayer();

void kf_ForceDesync();
void kf_PowerInfo();
void kf_BuildNextPage();
void kf_BuildPrevPage();
void kf_DamageMe();
void kf_AutoGame();

void kf_PerformanceSample();

void kf_QuickSave();
void kf_QuickLoad();

void kf_ToggleFullscreen();

void enableGodMode();

void keybindShutdown();
#endif // __INCLUDED_SRC_KEYBIND_H__
