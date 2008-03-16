/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2007  Warzone Resurrection Project

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
 * Loop.c
 *
 * The main game loop
 *
 */
#include "lib/framework/frame.h"
#include "lib/framework/input.h"
#include "lib/framework/strres.h"

#include "lib/ivis_common/rendmode.h"
#include "lib/ivis_common/piestate.h" //ivis render code
#include "lib/ivis_common/piemode.h"
// FIXME Direct iVis implementation include!
#include "lib/ivis_common/rendmode.h" //ivis render code
#include "lib/ivis_opengl/screen.h"

#include "lib/gamelib/gtime.h"
#include "lib/gamelib/animobj.h"
#include "lib/script/script.h"
#include "lib/sound/audio.h"
#include "lib/sound/cdaudio.h"
#include "lib/sound/mixer.h"

#include "loop.h"

#include "objects.h"
#include "display.h"
#include "map.h"
#include "hci.h"
#include "ingameop.h"
#include "miscimd.h"
#include "effects.h"
#include "radar.h"
#include "projectile.h"
#include "console.h"
#include "power.h"
#include "message.h"
#include "bucket3d.h"
#include "display3d.h"
#include "warzoneconfig.h"

#include "multiplay.h" //ajl
#include "scripttabs.h"
#include "levels.h"
#include "visibility.h"
#include "multimenu.h"
#include "intelmap.h"
#include "loadsave.h"
#include "game.h"
#include "multijoin.h"

#include "intimage.h"
#include "resource.h"
#include "seqdisp.h"
#include "mission.h"
#include "warcam.h"
#include "lighting.h"
#include "mapgrid.h"
#include "edit3d.h"
#include "drive.h"
#include "target.h"
#include "fpath.h"
#include "scriptextern.h"
#include "cluster.h"
#include "cmddroid.h"
#include "keybind.h"
#include "wrappers.h"

#include "warzoneconfig.h"

#ifdef DEBUG
#include "objmem.h"
#endif

static void fireWaitingCallbacks(void);

/*
 * Global variables
 */
unsigned int loopPieCount;
unsigned int loopTileCount;
unsigned int loopPolyCount;
unsigned int loopStateChanges;

/*
 * local variables
 */
static BOOL paused=FALSE;
static BOOL video=FALSE;
static BOOL bQuitVideo=FALSE;
static	SDWORD clearCount = 0;

//holds which pause is valid at any one time
typedef struct _pause_state
{
	unsigned gameUpdatePause	: 1;
	unsigned audioPause			: 1;
	unsigned scriptPause		: 1;
	unsigned scrollPause		: 1;
	unsigned consolePause		: 1;
} PAUSE_STATE;

static PAUSE_STATE	pauseState;
static	UDWORD	numDroids[MAX_PLAYERS];
static	UDWORD	numMissionDroids[MAX_PLAYERS];
static	UDWORD	numTransporterDroids[MAX_PLAYERS];
static	UDWORD	numCommandDroids[MAX_PLAYERS];
static	UDWORD	numConstructorDroids[MAX_PLAYERS];
// flag to signal a quick exit from the game
static SDWORD videoMode;

LOOP_MISSION_STATE		loopMissionState = LMS_NORMAL;

// this is set by scrStartMission to say what type of new level is to be started
SDWORD	nextMissionType = LDS_NONE;//MISSION_NONE;

 /* Force 3D display */
UDWORD	mcTime;
extern BOOL		godMode;

/* The main game loop */
GAMECODE gameLoop(void)
{
	DROID		*psCurr, *psNext;
	STRUCTURE	*psCBuilding, *psNBuilding;
	FEATURE		*psCFeat, *psNFeat;
	UDWORD		i,widgval;
	BOOL		quitting=FALSE;
	INT_RETVAL	intRetVal;
	int	        clearMode = 0;

	if (!war_GetFog())
	{
		PIELIGHT black;

		// set the fog color to black (RGB)
		// the fogbox will get this color
		black.argb = 0;
		black.byte.a = 255;
		pie_SetFogColour(black);
	}
	if(getDrawShadows())
	{
		clearMode |= CLEAR_SHADOW;
	}
	if (loopMissionState == LMS_SAVECONTINUE)
	{
		pie_SetFogStatus(FALSE);
		clearMode = CLEAR_BLACK;
	}
	pie_ScreenFlip(clearMode);//gameloopflip

	HandleClosingWindows();	// Needs to be done outside the pause case.

	audio_Update();

	if (!paused)
	{
		if (!scriptPaused())
		{
			/* Update the event system */
			if (!bInTutorial)
			{
				eventProcessTriggers(gameTime/SCR_TICKRATE);
			}
			else
			{
				eventProcessTriggers(gameTime2/SCR_TICKRATE);
			}
		}

		/* Run the in game interface and see if it grabbed any mouse clicks */
	  	if( (!rotActive) && getWidgetsStatus() &&
			(dragBox3D.status != DRAG_DRAGGING) &&
			(wallDrag.status != DRAG_DRAGGING) )
		{
			intRetVal = intRunWidgets();
		}
		else
		{
			intRetVal = INT_NONE;
		}

		//don't process the object lists if paused or about to quit to the front end
		if (!(gameUpdatePaused() || intRetVal == INT_QUIT))
		{
			if( dragBox3D.status != DRAG_DRAGGING
				&& wallDrag.status != DRAG_DRAGGING
				&& ( intRetVal == INT_INTERCEPT
					|| ( radarOnScreen
						 && CoordInRadar(mouseX(), mouseY())
						 && getHQExists(selectedPlayer) ) ) )
			{
				frameSetCursorFromRes(IDC_DEFAULT);
				intRetVal = INT_INTERCEPT;
			}

#ifdef DEBUG
			// check all flag positions for duplicate delivery points
			checkFactoryFlags();
#endif
			//handles callbacks for positioning of DP's
			process3DBuilding();

			// Update the base movement stuff
			moveUpdateBaseSpeed();

			// Update the visibility change stuff
			visUpdateLevel();

			// do the grid garbage collection
			gridGarbageCollect();

			//update the findpath system
			fpathUpdate();

			// update the cluster system
			clusterUpdate();

			// update the command droids
			cmdDroidUpdate();

			if(getDrivingStatus())
			{
				driveUpdate();
			}

			//ajl. get the incoming netgame messages and process them.
			if(bMultiPlayer)
			{
				multiPlayerLoop();
			}

			fireWaitingCallbacks(); //Now is the good time to fire waiting callbacks (since interpreter is off now)

			for(i = 0; i < MAX_PLAYERS; i++)
			{
				//update the current power available for a player
				updatePlayerPower(i);

				//this is a check cos there is a problem with the power but not sure where!!
				powerCheck(TRUE, (UBYTE)i);

				//set the flag for each player
				setHQExists(FALSE, i);
				setSatUplinkExists(FALSE, i);

				numCommandDroids[i] = 0;
				numConstructorDroids[i] = 0;
				numDroids[i]=0;
				numTransporterDroids[i]=0;

				for(psCurr = apsDroidLists[i]; psCurr; psCurr = psNext)
				{
					/* Copy the next pointer - not 100% sure if the droid could get destroyed
					but this covers us anyway */
					psNext = psCurr->psNext;
					droidUpdate(psCurr);

					// update the droid counts
					numDroids[i]++;
					switch (psCurr->droidType)
					{
						case DROID_COMMAND:
							numCommandDroids[i] += 1;
							break;
						case DROID_CONSTRUCT:
						case DROID_CYBORG_CONSTRUCT:
							numConstructorDroids[i] += 1;
							break;
						case DROID_TRANSPORTER:
							if( (psCurr->psGroup != NULL) )
							{
								numTransporterDroids[i] += psCurr->psGroup->refCount-1;
							}
							break;
						default:
							break;
					}
				}

				numMissionDroids[i]=0;
				for(psCurr = mission.apsDroidLists[i]; psCurr; psCurr = psNext)
				{
					/* Copy the next pointer - not 100% sure if the droid could
					get destroyed but this covers us anyway */
					psNext = psCurr->psNext;
					missionDroidUpdate(psCurr);
					numMissionDroids[i]++;
					switch (psCurr->droidType)
					{
						case DROID_COMMAND:
							numCommandDroids[i] += 1;
							break;
						case DROID_CONSTRUCT:
						case DROID_CYBORG_CONSTRUCT:
							numConstructorDroids[i] += 1;
							break;
						case DROID_TRANSPORTER:
							if( (psCurr->psGroup != NULL) )
							{
								numTransporterDroids[i] += psCurr->psGroup->refCount-1;
							}
							break;
						default:
							break;
					}
				}
				for(psCurr = apsLimboDroids[i]; psCurr; psCurr = psNext)
				{
					/* Copy the next pointer - not 100% sure if the droid could
					get destroyed but this covers us anyway */
					psNext = psCurr->psNext;

					// count the type of units
					switch (psCurr->droidType)
					{
						case DROID_COMMAND:
							numCommandDroids[i] += 1;
							break;
						case DROID_CONSTRUCT:
						case DROID_CYBORG_CONSTRUCT:
							numConstructorDroids[i] += 1;
							break;
						default:
							break;
					}
				}

				/*set this up AFTER droidUpdate so that if trying to building a
				new one, we know whether one exists already*/
				setLasSatExists(FALSE, i);
				for (psCBuilding = apsStructLists[i]; psCBuilding; psCBuilding = psNBuilding)
				{
					/* Copy the next pointer - not 100% sure if the structure could get destroyed but this covers us anyway */
					psNBuilding = psCBuilding->psNext;
					structureUpdate(psCBuilding);
					//set animation flag
					if (psCBuilding->pStructureType->type == REF_HQ &&
						psCBuilding->status == SS_BUILT)
					{
						setHQExists(TRUE, i);
					}
					if (psCBuilding->pStructureType->type == REF_SAT_UPLINK &&
						psCBuilding->status == SS_BUILT)
					{
						setSatUplinkExists(TRUE, i);
					}
					//don't wait for the Las Sat to be built - can't build another if one is partially built
					if (asWeaponStats[psCBuilding->asWeaps[0].nStat].
						weaponSubClass == WSC_LAS_SAT)
					{
						setLasSatExists(TRUE, i);
					}
				}
				for (psCBuilding = mission.apsStructLists[i]; psCBuilding;
						psCBuilding = psNBuilding)
				{
					/* Copy the next pointer - not 100% sure if the structure could get destroyed but this covers us anyway. It shouldn't do since its not even on the map!*/
					psNBuilding = psCBuilding->psNext;
					missionStructureUpdate(psCBuilding);
					if (psCBuilding->pStructureType->type == REF_HQ &&
						psCBuilding->status == SS_BUILT)
					{
						setHQExists(TRUE, i);
					}
					if (psCBuilding->pStructureType->type == REF_SAT_UPLINK &&
						psCBuilding->status == SS_BUILT)
					{
						setSatUplinkExists(TRUE, i);
					}
					//don't wait for the Las Sat to be built - can't build another if one is partially built
					if (asWeaponStats[psCBuilding->asWeaps[0].nStat].
						weaponSubClass == WSC_LAS_SAT)
					{
						setLasSatExists(TRUE, i);
					}
				}
				//this is a check cos there is a problem with the power but not sure where!!
				powerCheck(FALSE, (UBYTE)i);
			}

			missionTimerUpdate();

			proj_UpdateAll();

			for(psCFeat = apsFeatureLists[0]; psCFeat; psCFeat = psNFeat)
			{
				psNFeat = psCFeat->psNext;
				featureUpdate(psCFeat);
			}

			/* update animations */
			animObj_Update();

			objmemUpdate();
		}
		if (!consolePaused())
		{
			/* Process all the console messages */
			updateConsoleMessages();
		}
		if (!scrollPaused() && !getWarCamStatus() && dragBox3D.status != DRAG_DRAGGING && intMode != INT_INGAMEOP )
		{
			scroll();
		}
	}
	else // paused
	{
		intRetVal = INT_NONE;
		if (loop_GetVideoStatus())
		{
			bQuitVideo = !seq_UpdateFullScreenVideo(NULL);
		}

		if(dragBox3D.status != DRAG_DRAGGING)
		{
			scroll();
		}

		if(InGameOpUp)		// ingame options menu up, run it!
		{
			widgval = widgRunScreen(psWScreen);
			intProcessInGameOptions(widgval);
			if(widgval == INTINGAMEOP_QUIT_CONFIRM)
			{
				if(gamePaused())
				{
					kf_TogglePauseMode();
				}
				intRetVal = INT_QUIT;
			}
		}

		if(bLoadSaveUp && runLoadSave(TRUE) && strlen(sRequestResult))
		{
			debug( LOG_NEVER, "Returned %s", sRequestResult );
			if(bRequestLoad)
			{
				loopMissionState = LMS_LOADGAME;
				strlcpy(saveGameName, sRequestResult, sizeof(saveGameName));
			}
			else
			{
				if (saveInMissionRes())
				{
					if (saveGame(sRequestResult, GTYPE_SAVE_START))
					{
						addConsoleMessage(_("GAME SAVED!"), LEFT_JUSTIFY);
					}
					else
					{
						ASSERT( FALSE,"Mission Results: saveGame Failed" );
						deleteSaveGame(sRequestResult);
					}
				}
				else if (bMultiPlayer || saveMidMission())
				{
					if (saveGame(sRequestResult, GTYPE_SAVE_MIDMISSION))//mid mission from [esc] menu
					{
						addConsoleMessage(_("GAME SAVED!"), LEFT_JUSTIFY);
					}
					else
					{
						ASSERT(!"saveGame(sRequestResult, GTYPE_SAVE_MIDMISSION) failed", "Mid Mission: saveGame Failed" );
						deleteSaveGame(sRequestResult);
					}
				}
				else
				{
					ASSERT( FALSE, "Attempt to save game with incorrect load/save mode" );
				}
			}
		}
	}

	/* Check for quit */
	if (intRetVal == INT_QUIT)
	{
		if (!loop_GetVideoStatus())
		{
			//quitting from the game to the front end
			//so get a new backdrop
			quitting = TRUE;

			pie_LoadBackDrop(SCREEN_RANDOMBDROP);
		}
		else //if in video mode esc kill video
		{
			bQuitVideo = TRUE;
		}
	}
	if (!loop_GetVideoStatus() && !quitting)
	{
		if (!gameUpdatePaused())
		{
			if (dragBox3D.status != DRAG_DRAGGING
			 && wallDrag.status != DRAG_DRAGGING)
			{
				ProcessRadarInput();
			}
			processInput();

			//no key clicks or in Intelligence Screen
			if (intRetVal == INT_NONE && !InGameOpUp)
			{
				processMouseClickInput();
			}
			displayWorld();
		}
		/* Display the in game interface */
		pie_SetDepthBufferStatus(DEPTH_CMP_ALWAYS_WRT_ON);
		pie_SetFogStatus(FALSE);

		if(bMultiPlayer && bDisplayMultiJoiningStatus)
		{
			intDisplayMultiJoiningStatus(bDisplayMultiJoiningStatus);
			setWidgetsStatus(FALSE);
		}

		if(getWidgetsStatus())
		{
			intDisplayWidgets();
		}
		pie_SetDepthBufferStatus(DEPTH_CMP_LEQ_WRT_ON);
		pie_SetFogStatus(TRUE);
	}

	/* Check for toggling video playbackmode */
	if (bQuitVideo && loop_GetVideoStatus())
	{
		seq_StopFullScreenVideo();
		bQuitVideo = FALSE;
	}

	pie_GetResetCounts(&loopPieCount, &loopTileCount, &loopPolyCount, &loopStateChanges);

	if (fogStatus & FOG_BACKGROUND)
	{
		if (loopMissionState == LMS_SAVECONTINUE)
		{
			pie_SetFogStatus(FALSE);
			clearMode = CLEAR_BLACK;
		}
	}
	else
	{
		clearMode = CLEAR_BLACK;//force to black 3DFX
	}

	if (!quitting)
	{
			/* Check for toggling display mode */
			if ((keyDown(KEY_LALT) || keyDown(KEY_RALT)) && keyPressed(KEY_RETURN))
			{
				screenToggleMode();
			}
	}

	// deal with the mission state
	switch (loopMissionState)
	{
		case LMS_CLEAROBJECTS:
			missionDestroyObjects();
			setScriptPause(TRUE);
			loopMissionState = LMS_SETUPMISSION;
			break;

		case LMS_NORMAL:
			// default
			break;
		case LMS_SETUPMISSION:
			setScriptPause(FALSE);
			if (!setUpMission(nextMissionType))
			{
				return GAMECODE_QUITGAME;
			}
			break;
		case LMS_SAVECONTINUE:
			// just wait for this to be changed when the new mission starts
			clearMode = CLEAR_BLACK;
			break;
		case LMS_NEWLEVEL:
			//nextMissionType = MISSION_NONE;
			nextMissionType = LDS_NONE;
			return GAMECODE_NEWLEVEL;
			break;
		case LMS_LOADGAME:
			return GAMECODE_LOADGAME;
			break;
		default:
			ASSERT( FALSE, "unknown loopMissionState" );
			break;
	}

	if (quitting)
	{
		pie_SetFogStatus(FALSE);
		pie_ScreenFlip(CLEAR_BLACK);//gameloopflip
		/* Check for toggling display mode */
		if ((keyDown(KEY_LALT) || keyDown(KEY_RALT)) && keyPressed(KEY_RETURN))
		{
			screenToggleMode();
		}
		return GAMECODE_QUITGAME;
	}
	else if (loop_GetVideoStatus())
	{
		audio_StopAll();
		return GAMECODE_PLAYVIDEO;
	}

	return GAMECODE_CONTINUE;
}

/* The video playback loop */
void videoLoop(void)
{
	bool bVolKilled = false;
	float originalVolume = 0.0;

	// There is something really odd here. - Per
	static BOOL bActiveBackDrop = FALSE;

#ifdef DEBUG
	// Surely this should be: bActiveBackDrop = screen_GetBackDrop(); ?? - Per
	// That would look odd, I tried it. Eg. intelligence screen is not redrawn correctly. -- DevU
	screen_GetBackDrop();
#endif

	if (loop_GetVideoStatus())
	{
		bQuitVideo = !seq_UpdateFullScreenVideo(NULL);
	}

	if ( (keyPressed(KEY_ESC) || mouseReleased(MOUSE_LMB) || bQuitVideo) && !seq_AnySeqLeft() )
	{
		/* zero volume before video quit - restore later */
		originalVolume = sound_GetUIVolume();
		sound_SetUIVolume(0.0);
		bVolKilled = true;
	}

	//toggling display mode disabled in video mode
	// Check for quit
	if (keyPressed(KEY_ESC) || mouseReleased(MOUSE_LMB))
	{
		seq_StopFullScreenVideo();
		bQuitVideo = FALSE;

		// remove the intelligence screen if necessary
		if (messageIsImmediate())
		{
			intResetScreen(TRUE);
			setMessageImmediate(FALSE);
		}
		//Script callback for video over
		//don't do the callback if we're playing the win/lose video
		if (!getScriptWinLoseVideo())
		{
			eventFireCallbackTrigger((TRIGGER_TYPE)CALL_VIDEO_QUIT);
		}
		else
		{
			displayGameOver(getScriptWinLoseVideo() == PLAY_WIN);
		}
		pie_ScreenFlip(CLEAR_BLACK);// videoloopflip extra mar10

		if (bActiveBackDrop)
		{
			// We never get here presently - Per
 			screen_RestartBackDrop();
		}
	}

	//if the video finished...
	if (bQuitVideo)
	{
		seq_StopFullScreenVideo();
		bQuitVideo = FALSE;

		//set the next video off - if any
		if (seq_AnySeqLeft())
		{
			pie_ScreenFlip(CLEAR_BLACK);// videoloopflip extra mar10

			if (bActiveBackDrop)
			{
				screen_RestartBackDrop();
			}
		 	//bClear = CLEAR_BLACK;
			seq_StartNextFullScreenVideo();
		}
		else
		{
			// remove the intelligence screen if necessary
			if (messageIsImmediate())
			{
				intResetScreen(TRUE);
				setMessageImmediate(FALSE);
			}
			//don't do the callback if we're playing the win/lose video
			if (!getScriptWinLoseVideo())
			{
				eventFireCallbackTrigger((TRIGGER_TYPE)CALL_VIDEO_QUIT);
			}
			else
			{
				displayGameOver(getScriptWinLoseVideo() == PLAY_WIN);
			}
			pie_ScreenFlip(CLEAR_BLACK);// videoloopflip extra mar10

			if (bActiveBackDrop)
			{
				// We never get here presently - Per
				screen_RestartBackDrop();
			}
		}
	}

	if( clearCount < 1)
	{
		if (screen_GetBackDrop())
		{
			bActiveBackDrop = TRUE;
			screen_StopBackDrop();
		}
		else
		{
			bActiveBackDrop = FALSE;
			screen_StopBackDrop();
		}
	}

	clearCount++;

	pie_ScreenFlip(CLEAR_BLACK);// videoloopflip

	/* restore volume after video quit */
	if (bVolKilled)
	{
		sound_SetUIVolume(originalVolume);
	}
}


void loop_SetVideoPlaybackMode(void)
{
	videoMode += 1;
	paused = TRUE;
	video = TRUE;
	clearCount = 0;
	gameTimeStop();
	pie_SetFogStatus(FALSE);
	audio_StopAll();
}


void loop_ClearVideoPlaybackMode(void)
{
	videoMode -=1;
	paused = FALSE;
	video = FALSE;
	gameTimeStart();
	pie_SetFogStatus(TRUE);
	cdAudio_Resume();
	ASSERT( videoMode == 0, "loop_ClearVideoPlaybackMode: out of sync." );
}


SDWORD loop_GetVideoMode(void)
{
	return videoMode;
}

BOOL loop_GetVideoStatus(void)
{
	return video;
}

BOOL gamePaused( void )
{
	return paused;
}

void setGamePauseStatus( BOOL val )
{
	paused = val;
}

BOOL gameUpdatePaused(void)
{
	return pauseState.gameUpdatePause;
}
BOOL audioPaused(void)
{
	return pauseState.audioPause;
}
BOOL scriptPaused(void)
{
	return pauseState.scriptPause;
}
BOOL scrollPaused(void)
{
	return pauseState.scrollPause;
}
BOOL consolePaused(void)
{
	return pauseState.consolePause;
}

void setGameUpdatePause(BOOL state)
{
	pauseState.gameUpdatePause = state;
	if (state)
	{
	   	screen_RestartBackDrop();
	}
	else
	{
		screen_StopBackDrop();
	}
}
void setAudioPause(BOOL state)
{
	pauseState.audioPause = state;
}
void setScriptPause(BOOL state)
{
	pauseState.scriptPause = state;
}
void setScrollPause(BOOL state)
{
	pauseState.scrollPause = state;
}
void setConsolePause(BOOL state)
{
	pauseState.consolePause = state;
}

//set all the pause states to the state value
void setAllPauseStates(BOOL state)
{
	setGameUpdatePause(state);
	setAudioPause(state);
	setScriptPause(state);
	setScrollPause(state);
	setConsolePause(state);
}

UDWORD	getNumDroids(UDWORD player)
{
	return(numDroids[player]);
}

UDWORD	getNumTransporterDroids(UDWORD player)
{
	return(numTransporterDroids[player]);
}

UDWORD	getNumMissionDroids(UDWORD player)
{
	return(numMissionDroids[player]);
}

UDWORD	getNumCommandDroids(UDWORD player)
{
	return numCommandDroids[player];
}

UDWORD	getNumConstructorDroids(UDWORD player)
{
	return numConstructorDroids[player];
}


// increase the droid counts - used by update factory to keep the counts in sync
void incNumDroids(UDWORD player)
{
	numDroids[player] += 1;
}
void incNumCommandDroids(UDWORD player)
{
	numCommandDroids[player] += 1;
}
void incNumConstructorDroids(UDWORD player)
{
	numConstructorDroids[player] += 1;
}

/* Fire waiting beacon messages which we couldn't run before */
static void fireWaitingCallbacks(void)
{
	BOOL bOK = TRUE;

	while(!isMsgStackEmpty() && bOK)
	{
		bOK = msgStackFireTop();
		if(!bOK){
			ASSERT(FALSE, "fireWaitingCallbacks: msgStackFireTop() failed (stack count: %d)", msgStackGetCount());
		}
	}
}
