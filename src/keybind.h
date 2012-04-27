/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2012  Warzone 2100 Project

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
extern void	kf_HalveHeights( void );
extern void	kf_DebugDroidInfo( void );
extern void	kf_BuildInfo( void );
extern void	kf_ToggleFPS(void);			//FPS counter NOT same as kf_Framerate! -Q
extern void	kf_ToggleSamples(void);		// Displays # of sound samples in Queue/list.
extern void kf_ToggleOrders(void);		//displays unit's Order/action state.
extern void kf_ToggleLevelName(void);
extern void	kf_FrameRate( void );
extern void	kf_ShowNumObjects( void );
extern void	kf_ToggleRadar( void );
extern void	kf_TogglePower( void );
extern void	kf_RecalcLighting( void );
extern void	kf_ScreenDump( void );
extern void	kf_AllAvailable( void );
extern void	kf_TriFlip( void );
extern void	kf_ToggleWidgets( void );
extern void	kf_ToggleBackgroundFog( void );
extern void	kf_ToggleDistanceFog( void );
extern void	kf_ToggleMistFog( void );
extern void	kf_ToggleFog( void );
extern void	kf_ToggleShadows( void );
extern void	kf_ToggleCamera( void );
extern void	kf_RaiseTile( void );
extern void	kf_LowerTile( void );
extern void	kf_MapCheck( void );
extern void	kf_SystemClose( void );
extern void	kf_ZoomOut( void );
void            kf_ZoomOutStep();
extern void	kf_ZoomIn( void );
void            kf_ZoomInStep();
extern void	kf_ShrinkScreen( void );
extern void	kf_ExpandScreen( void );
extern void	kf_RotateLeft( void );
extern void	kf_RotateRight( void );
extern void	kf_PitchBack( void );
extern void	kf_PitchForward( void );
extern void	kf_ResetPitch( void );
extern void	kf_ToggleDimension( void );
extern void	kf_ShowMappings( void );
extern void	kf_SelectGrouping( UDWORD groupNumber );
extern void	kf_SelectGrouping_0( void );
extern void	kf_SelectGrouping_1( void );
extern void	kf_SelectGrouping_2( void );
extern void	kf_SelectGrouping_3( void );
extern void	kf_SelectGrouping_4( void );
extern void	kf_SelectGrouping_5( void );
extern void	kf_SelectGrouping_6( void );
extern void	kf_SelectGrouping_7( void );
extern void	kf_SelectGrouping_8( void );
extern void	kf_SelectGrouping_9( void );
extern void	kf_AssignGrouping_0( void );
extern void	kf_AssignGrouping_1( void );
extern void	kf_AssignGrouping_2( void );
extern void	kf_AssignGrouping_3( void );
extern void	kf_AssignGrouping_4( void );
extern void	kf_AssignGrouping_5( void );
extern void	kf_AssignGrouping_6( void );
extern void	kf_AssignGrouping_7( void );
extern void	kf_AssignGrouping_8( void );
extern void	kf_AssignGrouping_9( void );
extern void	kf_SelectMoveGrouping( void );
extern void	kf_ToggleDroidInfo( void );
extern void	kf_addInGameOptions( void );
extern void	kf_AddMissionOffWorld( void );
extern void	kf_EndMissionOffWorld( void );
extern void	kf_NewPlayerPower( void );
extern void	kf_addMultiMenu( void );
extern void	kf_multiAudioStart(void);
extern void	kf_multiAudioStop(void);
extern void	kf_JumpToMapMarker( void );
extern void	kf_TogglePowerBar( void );
extern void	kf_ToggleDebugMappings( void );
extern void	kf_ToggleGodMode( void );
extern void	kf_SeekNorth( void );
extern void	kf_MaxScrollLimits( void );
extern void	kf_LevelSea( void );
extern void	kf_TestWater( void );
extern void	kf_toggleTrapCursor(void);
extern void	kf_TogglePauseMode( void );
extern void	kf_ToggleDemoMode( void );
extern void	kf_ToggleRadarAllign( void );

extern void	kf_ToggleEnergyBars( void );
extern void	kf_FinishAllResearch( void );
extern void	kf_FinishResearch( void );
extern void	kf_ToggleOverlays( void );
extern void	kf_ChooseOptions( void );
extern void	kf_ChooseCommand( void );
extern void	kf_ChooseManufacture( void );
extern void	kf_ChooseResearch( void );
extern void	kf_ChooseBuild( void );
extern void	kf_ChooseDesign( void );
extern void	kf_ChooseIntelligence( void );
extern void	kf_ChooseCancel( void );
extern void	kf_ToggleWeather( void );
extern void	kf_KillSelected(void);
extern void	kf_ShowGridInfo(void);
extern void	kf_SendTextMessage(void);
extern void	kf_SelectPlayer( void );
extern void	kf_ToggleDrivingMode( void );
extern void	kf_ToggleConsole( void );
extern void	kf_SelectAllOnScreenUnits( void );
extern void	kf_SelectAllUnits( void );
extern void	kf_SelectAllVTOLs( void );
extern void	kf_SelectAllHovers( void );
extern void	kf_SelectAllWheeled( void );
extern void	kf_SelectAllTracked( void );
extern void	kf_SelectAllHalfTracked( void );
void kf_SelectAllCyborgs();
void kf_SelectAllEngineers();
void kf_SelectAllMechanics();
void kf_SelectAllTransporters();
void kf_SelectAllRepairTanks();
void kf_SelectAllSensorUnits();
void kf_SelectAllTrucks();
extern void	kf_SelectAllCombatUnits( void );
void kf_SelectAllLandCombatUnits();
void kf_SelectAllCombatCyborgs();
extern void	kf_SelectAllSameType( void );

extern void	kf_SetDroidRangeShort( void );
extern void	kf_SetDroidRangeDefault( void );
extern void	kf_SetDroidRangeLong( void );

extern void	kf_SetDroidRetreatMedium( void );
extern void	kf_SetDroidRetreatHeavy( void );
extern void	kf_SetDroidRetreatNever( void );

extern void	kf_SetDroidAttackAtWill( void );
extern void	kf_SetDroidAttackReturn( void );
extern void	kf_SetDroidAttackCease( void );

extern void	kf_SetDroidMoveHold( void );
extern void	kf_SetDroidMoveGuard( void );
extern void	kf_SetDroidMovePursue( void ); //not there?
extern void	kf_SetDroidMovePatrol( void ); // not there?

extern void	kf_SetDroidReturnToBase( void );
extern void	kf_SetDroidGoToTransport( void );
extern void	kf_SetDroidGoForRepair( void );
extern void	kf_SetDroidRecycle( void );
extern void	kf_ScatterDroids( void );
extern void	kf_CentreOnBase( void );
extern void	kf_ToggleFog( void );
extern void	kf_MoveToLastMessagePos( void );
extern void	kf_SelectAllDamaged( void );
extern void	kf_RightOrderMenu( void );

extern bool	bAllowOtherKeyPresses;

extern void	kf_TriggerRayCast( void );
extern void	kf_ToggleFormationSpeedLimiting( void );
extern void	kf_ToggleSensorDisplay( void );		//Was commented out.  Re-enabled --Q 5/10/05
extern void	kf_SensorDisplayOn( void );
extern void	kf_SensorDisplayOff( void );
extern void	kf_JumpToResourceExtractor( void );
extern void	kf_JumpToRepairUnits( void );
extern void	kf_JumpToConstructorUnits( void );
extern void	kf_JumpToCommandUnits( void );
extern void	kf_JumpToSensorUnits( void );
extern void	kf_AddHelpBlip( void );				//Add a beacon
extern void	kf_ToggleProximitys( void );

extern void	kf_JumpToUnassignedUnits( void );
extern void	kf_TriggerShockWave( void );
extern void	kf_ToggleVisibility( void );
extern void	kf_RadarZoomIn( void );
extern void	kf_RadarZoomOut( void );
extern void	kf_SelectNextFactory(void);
extern void	kf_SelectNextCyborgFactory(void);
extern void	kf_SelectNextPowerStation(void);
extern void	kf_SelectNextResearch(void);
extern void	kf_ToggleConsoleDrop( void );
extern void	kf_ToggleShakeStatus( void );
extern void	kf_ToggleMouseInvert( void );
extern void	kf_SetKillerLevel( void );
extern void	kf_SetEasyLevel( void );
extern void	kf_SetNormalLevel( void );
extern void	kf_SetToughUnitsLevel( void );
extern void	kf_UpThePower( void );
extern void	kf_MaxPower( void );
extern void	kf_KillEnemy( void );
extern void	kf_ToggleMissionTimer( void );
extern void	kf_TraceObject( void );

extern void	kf_SetHardLevel( void );
extern void	kf_SelectCommander_0( void );
extern void	kf_SelectCommander_1( void );
extern void	kf_SelectCommander_2( void );
extern void	kf_SelectCommander_3( void );
extern void	kf_SelectCommander_4( void );
extern void	kf_SelectCommander_5( void );
extern void	kf_SelectCommander_6( void );
extern void	kf_SelectCommander_7( void );
extern void	kf_SelectCommander_8( void );
extern void	kf_SelectCommander_9( void );
void kf_ToggleReopenBuildMenu( void );

extern void	kf_ToggleShowGateways(void);
extern void	kf_ToggleShowPath(void);

// dirty but necessary
extern	char	sTextToSend[MAX_CONSOLE_STRING_LENGTH];
extern	void	kf_FaceNorth(void);
extern	void	kf_FaceSouth(void);
extern	void	kf_FaceEast(void);
extern	void	kf_FaceWest(void);
extern	void	kf_ToggleRadarJump( void );
extern	void	kf_MovePause( void );

extern void kf_SpeedUp( void );
extern void kf_SlowDown( void );
extern void kf_NormalSpeed( void );

extern void kf_CloneSelected( void );
extern void kf_Reload( void );

extern void kf_ToggleLogical(void);

#define SPIN_SCALING	(360*DEG_1)
#define	SECS_PER_SPIN	2
#define MAP_SPIN_RATE	(SPIN_SCALING/SECS_PER_SPIN)

void kf_ToggleRadarTerrain( void );          //radar terrain
void kf_ToggleRadarAllyEnemy( void );        //enemy/ally color toggle

void kf_TileInfo(void);

void kf_NoAssert(void);

extern void	kf_ToggleWatchWindow( void );

bool runningMultiplayer(void);

void    kf_ForceDesync(void);
void	kf_PowerInfo( void );
void	kf_BuildNextPage( void );
void	kf_BuildPrevPage( void );
extern void kf_DamageMe(void);
extern void kf_AutoGame(void);

#endif // __INCLUDED_SRC_KEYBIND_H__
