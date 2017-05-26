/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2017  Warzone 2100 Project

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
#include "lib/framework/fixedpoint.h"

// --------------- All those keyboard mappable functions */
void kf_HalveHeights();
void kf_DebugDroidInfo();
void kf_BuildInfo();
void kf_ToggleFPS();			//FPS counter NOT same as kf_Framerate! -Q
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
void kf_ToggleBackgroundFog();
void kf_ToggleDistanceFog();
void kf_ToggleMistFog();
void kf_ToggleFog();
void kf_ToggleShadows();
void kf_ToggleCamera();
void kf_RaiseTile();
void kf_LowerTile();
void kf_MapCheck();
void kf_SystemClose();
void kf_ZoomOut();
void kf_ZoomOutStep();
void kf_ZoomIn();
void kf_ZoomInStep();
void kf_ShrinkScreen();
void kf_ExpandScreen();
void kf_RotateLeft();
void kf_RotateRight();
void kf_PitchBack();
void kf_PitchForward();
void kf_ResetPitch();
void kf_ToggleDimension();
void kf_ShowMappings();
void kf_SelectGrouping(UDWORD groupNumber);
void kf_SelectGrouping_0();
void kf_SelectGrouping_1();
void kf_SelectGrouping_2();
void kf_SelectGrouping_3();
void kf_SelectGrouping_4();
void kf_SelectGrouping_5();
void kf_SelectGrouping_6();
void kf_SelectGrouping_7();
void kf_SelectGrouping_8();
void kf_SelectGrouping_9();
void kf_AssignGrouping_0();
void kf_AssignGrouping_1();
void kf_AssignGrouping_2();
void kf_AssignGrouping_3();
void kf_AssignGrouping_4();
void kf_AssignGrouping_5();
void kf_AssignGrouping_6();
void kf_AssignGrouping_7();
void kf_AssignGrouping_8();
void kf_AssignGrouping_9();
void kf_SelectMoveGrouping();
void kf_ToggleDroidInfo();
void kf_addInGameOptions();
void kf_AddMissionOffWorld();
void kf_EndMissionOffWorld();
void kf_NewPlayerPower();
void kf_addMultiMenu();
void kf_multiAudioStart();
void kf_multiAudioStop();
void kf_JumpToMapMarker();
void kf_TogglePowerBar();
void kf_ToggleDebugMappings();
void kf_ToggleGodMode();
void kf_SeekNorth();
void kf_MaxScrollLimits();
void kf_LevelSea();
void kf_TestWater();
void kf_toggleTrapCursor();
void kf_TogglePauseMode();
void kf_ToggleRadarAllign();

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
void kf_ShowGridInfo();
void kf_SendGlobalMessage();
void kf_SendTeamMessage();
void kf_SelectPlayer();
void kf_ToggleConsole();
void kf_ToggleTeamChat();
void kf_SelectAllOnScreenUnits();
void kf_SelectAllUnits();
void kf_SelectAllVTOLs();
void kf_SelectAllArmedVTOLs();
void kf_SelectAllHovers();
void kf_SelectAllWheeled();
void kf_SelectAllTracked();
void kf_SelectAllHalfTracked();
void kf_SelectAllCyborgs();
void kf_SelectAllEngineers();
void kf_SelectAllMechanics();
void kf_SelectAllTransporters();
void kf_SelectAllRepairTanks();
void kf_SelectAllSensorUnits();
void kf_SelectAllTrucks();
void kf_SelectAllCombatUnits();
void kf_SelectAllLandCombatUnits();
void kf_SelectAllCombatCyborgs();
void kf_SelectAllSameType();

void kf_SetDroidRetreatMedium();
void kf_SetDroidRetreatHeavy();
void kf_SetDroidRetreatNever();

void kf_SetDroidAttackAtWill();
void kf_SetDroidAttackReturn();
void kf_SetDroidAttackCease();

void kf_SetDroidOrderHold();
void kf_SetDroidOrderStop();

void kf_SetDroidMoveGuard();
void kf_SetDroidMovePursue();   //not there?
void kf_SetDroidMovePatrol();   // not there?

void kf_SetDroidReturnToBase();
void kf_SetDroidGoToTransport();
void kf_SetDroidGoForRepair();
void kf_SetDroidRecycle();
void kf_ScatterDroids();
void kf_CentreOnBase();
void kf_ToggleFog();
void kf_MoveToLastMessagePos();
void kf_SelectAllDamaged();
void kf_RightOrderMenu();

extern bool bAllowOtherKeyPresses;

void kf_TriggerRayCast();
void kf_ToggleFormationSpeedLimiting();
void kf_ToggleSensorDisplay();		//Was commented out.  Re-enabled --Q 5/10/05
void kf_JumpToResourceExtractor();
void kf_JumpToRepairUnits();
void kf_JumpToConstructorUnits();
void kf_JumpToCommandUnits();
void kf_JumpToSensorUnits();
void kf_AddHelpBlip();				//Add a beacon
void kf_ToggleProximitys();

void kf_JumpToUnassignedUnits();
void kf_TriggerShockWave();
void kf_ToggleVisibility();
void kf_RadarZoomIn();
void kf_RadarZoomOut();
void kf_SelectNextFactory();
void kf_SelectNextCyborgFactory();
void kf_SelectNextPowerStation();
void kf_SelectNextResearch();
void kf_ToggleConsoleDrop();
void kf_ToggleMouseInvert();
void kf_SetKillerLevel();
void kf_SetEasyLevel();
void kf_SetNormalLevel();
void kf_SetToughUnitsLevel();
void kf_UpThePower();
void kf_MaxPower();
void kf_KillEnemy();
void kf_ToggleMissionTimer();
void kf_TraceObject();

void kf_SetHardLevel();
void kf_SelectCommander_0();
void kf_SelectCommander_1();
void kf_SelectCommander_2();
void kf_SelectCommander_3();
void kf_SelectCommander_4();
void kf_SelectCommander_5();
void kf_SelectCommander_6();
void kf_SelectCommander_7();
void kf_SelectCommander_8();
void kf_SelectCommander_9();
void kf_ToggleReopenBuildMenu();

void kf_ToggleShowGateways();
void kf_ToggleShowPath();

// dirty but necessary
extern char sTextToSend[MAX_CONSOLE_STRING_LENGTH];

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

#endif // __INCLUDED_SRC_KEYBIND_H__
