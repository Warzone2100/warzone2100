/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2010  Warzone 2100 Project

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
 * @file scores.c
 * Deals with all the mission results gubbins.
 * Alex W. McLean
*/

#include <string.h>

// --------------------------------------------------------------------
#include "lib/framework/frame.h"
#include "lib/framework/math_ext.h"
#include "lib/framework/strres.h"
#include "lib/framework/tagfile.h"
#include "lib/gamelib/gtime.h"
#include "console.h"
#include "scores.h"
#include "lib/ivis_common/pieblitfunc.h"
#include "lib/ivis_common/piedef.h"
#include "lib/ivis_common/piefunc.h"
#include "lib/ivis_common/piemode.h"
#include "lib/ivis_common/piestate.h"
#include "lib/ivis_common/piepalette.h"
#include "objects.h"
#include "droiddef.h"
#include "basedef.h"
#include "statsdef.h"
#include "hci.h"
#include "miscimd.h"
#include "lib/ivis_opengl/piematrix.h"
#include "display3d.h"
#include "mission.h"
#include "game.h"
#include "lib/sound/audio.h"
#include "lib/sound/audio_id.h"
#include "intimage.h"

#define	BAR_CRAWL_TIME	(GAME_TICKS_PER_SEC*3)

#define	MT_X_POS	(MISSIONRES_TITLE_X  + D_W + 140)
#define MT_Y_POS	(MISSIONRES_TITLE_Y  + D_H + 80)

#define DROID_LEVELS	9
#define MAX_BAR_LENGTH	100
#define LC_UPPER	100

#define LC_X	32
#define RC_X	320+32
#define	RANK_BAR_WIDTH	100
#define STAT_BAR_WIDTH	100

typedef enum
{
	STR_MR_UNITS_LOST,
	STR_MR_UNITS_KILLED,
	STR_MR_STR_LOST,
	STR_MR_STR_BLOWN_UP,
	STR_MR_UNITS_BUILT,
	STR_MR_UNITS_NOW,
	STR_MR_STR_BUILT,
	STR_MR_STR_NOW,

	STR_MR_LEVEL_ROOKIE,
	STR_MR_LEVEL_GREEN,
	STR_MR_LEVEL_TRAINED,
	STR_MR_LEVEL_REGULAR,
	STR_MR_LEVEL_VETERAN,
	STR_MR_LEVEL_CRACK,
	STR_MR_LEVEL_ELITE,
	STR_MR_LEVEL_SPECIAL,
	STR_MR_LEVEL_ACE
} MR_STRING;


// return translated string
static const char *getDescription(MR_STRING id)
{
	switch (id)
	{
		case STR_MR_UNITS_LOST   : return _("Own Units: %u");
		case STR_MR_UNITS_KILLED : return _("Enemy Units: %u");
		case STR_MR_STR_LOST     : return _("Own Structures: %u");
		case STR_MR_STR_BLOWN_UP : return _("Enemy Structures: %u");
		case STR_MR_UNITS_BUILT  : return _("Units Manufactured: %u");
		case STR_MR_UNITS_NOW    : return _("Total Units: %u");
		case STR_MR_STR_BUILT    : return _("Structures Built: %u");
		case STR_MR_STR_NOW      : return _("Total Structures: %u");

		case STR_MR_LEVEL_ROOKIE : return _("Rookie: %u");
		case STR_MR_LEVEL_GREEN  : return P_("rank", "Green: %u");
		case STR_MR_LEVEL_TRAINED: return _("Trained: %u");
		case STR_MR_LEVEL_REGULAR: return _("Regular: %u");
		case STR_MR_LEVEL_VETERAN: return _("Professional: %u");
		case STR_MR_LEVEL_CRACK  : return _("Veteran: %u");
		case STR_MR_LEVEL_ELITE  : return _("Elite: %u");
		case STR_MR_LEVEL_SPECIAL: return _("Special: %u");
		case STR_MR_LEVEL_ACE    : return _("Hero: %u");
	}

	// make compiler shut up
	return "";
}

static PIELIGHT getColour(int id)
{
	switch (id)
	{
		case STR_MR_STR_LOST	 :
		case STR_MR_UNITS_LOST   :
			return WZCOL_MENU_SCORE_LOSS;
		case STR_MR_STR_BLOWN_UP :
		case STR_MR_UNITS_KILLED :
			return WZCOL_MENU_SCORE_DESTROYED;
		case STR_MR_UNITS_BUILT  :
		case STR_MR_UNITS_NOW    :
		case STR_MR_STR_BUILT    :
		case STR_MR_STR_NOW      :
			return WZCOL_MENU_SCORE_BUILT;
		case STR_MR_LEVEL_ROOKIE :
		case STR_MR_LEVEL_GREEN  :
		case STR_MR_LEVEL_TRAINED:
		case STR_MR_LEVEL_REGULAR:
		case STR_MR_LEVEL_VETERAN:
		case STR_MR_LEVEL_CRACK  :
		case STR_MR_LEVEL_ELITE  :
		case STR_MR_LEVEL_SPECIAL:
		case STR_MR_LEVEL_ACE    :
			return WZCOL_MENU_SCORE_RANK;
		default			 :
			return WZCOL_BLACK;
	}
}

STAT_BAR	infoBars[]=
{
	{LC_X,100,STAT_BAR_WIDTH,16,10,STR_MR_UNITS_LOST,0,false,true,0},	// left column		STAT_UNIT_LOST
	{LC_X,120,STAT_BAR_WIDTH,16,20,STR_MR_UNITS_KILLED,0,false,true,0},			 //	STAT_UNIT_KILLED
	{LC_X,160,STAT_BAR_WIDTH,16,30,STR_MR_STR_LOST,0,false,true,0},				 //	STAT_STR_LOST
	{LC_X,180,STAT_BAR_WIDTH,16,40,STR_MR_STR_BLOWN_UP,0,false,true,0}, 			 //	STAT_STR_BLOWN_UP
	{LC_X,220,STAT_BAR_WIDTH,16,50,STR_MR_UNITS_BUILT,0,false,true,0}, 			 //	STAT_UNITS_BUILT
	{LC_X,240,STAT_BAR_WIDTH,16,60,STR_MR_UNITS_NOW,0,false,true,0}, 				 //	STAT_UNITS_NOW
	{LC_X,260,STAT_BAR_WIDTH,16,70,STR_MR_STR_BUILT,0,false,true,0}, 				 //	STAT_STR_BUILT
	{LC_X,280,STAT_BAR_WIDTH,16,80,STR_MR_STR_NOW,0,false,false,0}, 				 //	STAT_STR_NOW

	{RC_X,100,RANK_BAR_WIDTH,16,10,STR_MR_LEVEL_ROOKIE,0,false,true,0},	// right column	//	STAT_ROOKIE
	{RC_X,120,RANK_BAR_WIDTH,16,20,STR_MR_LEVEL_GREEN,0,false,true,0}, 			 		//	STAT_GREEN
	{RC_X,140,RANK_BAR_WIDTH,16,30,STR_MR_LEVEL_TRAINED,0,false,true,0}, 		 			//	STAT_TRAINED
	{RC_X,160,RANK_BAR_WIDTH,16,40,STR_MR_LEVEL_REGULAR,0,false,true,0}, 		 			//	STAT_REGULAR
	{RC_X,180,RANK_BAR_WIDTH,16,50,STR_MR_LEVEL_VETERAN,0,false,true,0}, 		 			//	STAT_VETERAN
	{RC_X,200,RANK_BAR_WIDTH,16,60,STR_MR_LEVEL_CRACK,0,false,true,0}, 			 		//	STAT_CRACK
	{RC_X,220,RANK_BAR_WIDTH,16,70,STR_MR_LEVEL_ELITE,0,false,true,0}, 			 		//	STAT_ELITE
	{RC_X,240,RANK_BAR_WIDTH,16,80,STR_MR_LEVEL_SPECIAL,0,false,true,0}, 		 			//	STAT_SPECIAL
	{RC_X,260,RANK_BAR_WIDTH,16,90,STR_MR_LEVEL_ACE,0,false,true,0}, 			 			//	STAT_ACE

	{0, 0, 0, 0, 0, 0, 0, false, false, 0}
};

// --------------------------------------------------------------------
static void constructTime(char *psText, UDWORD hours, UDWORD minutes, UDWORD seconds);
static void drawStatBars(void);
static void fillUpStats( void );
static void dispAdditionalInfo( void );
// --------------------------------------------------------------------

/* The present mission data */
static	MISSION_DATA	missionData;
static	UDWORD	dispST;
static	BOOL	bDispStarted = false;
static	char	text[255];
static	char	text2[255];
extern bool Cheated;

// --------------------------------------------------------------------
/* Initialise the mission data info - done before each mission */
BOOL	scoreInitSystem( void )
{
	missionData.unitsBuilt		= 0;
	missionData.unitsKilled		= 0;
	missionData.unitsLost		= 0;

	missionData.strBuilt		= 0;
	missionData.strKilled		= 0;
	missionData.strLost			= 0;

	missionData.artefactsFound	= 0;
	missionData.missionStarted	= gameTime; // total game time is just gameTime
	missionData.shotsOnTarget	= 0;
	missionData.shotsOffTarget	= 0;
	missionData.babasMowedDown	= 0;
	Cheated = false;
	bDispStarted = false;
	return(true);
}

// --------------------------------------------------------------------
// Updates a game statistic - more can be added if we need 'em
void	scoreUpdateVar( DATA_INDEX var )
{
	switch(var)
	{
	case	WD_UNITS_BUILT:
		missionData.unitsBuilt++;	// We've built another unit
		break;
	case	WD_UNITS_KILLED:
		missionData.unitsKilled++;	// We've destroyed an enemy unit
		break;
	case	WD_UNITS_LOST:
		missionData.unitsLost++;	// We've lost a unit
		break;
	case	WD_STR_BUILT:
		missionData.strBuilt++;		// Built a structure
		break;
	case	WD_STR_KILLED:
		missionData.strKilled++;	// Destroyed an enemy structure
		break;
	case	WD_STR_LOST:
		missionData.strLost++;		// Lost a structure
		break;
	case	WD_ARTEFACTS_FOUND:
		missionData.artefactsFound++;	// Got an artefact
		break;
	case	WD_MISSION_STARTED:
		missionData.missionStarted = gameTime;	// Init the mission start time
		break;									// Should be called once per mission
	case	WD_SHOTS_ON_TARGET:
		missionData.shotsOnTarget++;	// We hit something
		break;
	case	WD_SHOTS_OFF_TARGET:
		missionData.shotsOffTarget++;	// Missed something
		break;
	case	WD_BARBARIANS_MOWED_DOWN:
		missionData.babasMowedDown++;	// Ran over a barbarian
		break;
	default:
		debug( LOG_FATAL, "Weirdy variable request from scoreUpdateVar" );
		abort();
		break;
	}
}


// --------------------------------------------------------------------
void	scoreDataToScreen(void)
{
	drawStatBars();
}

// --------------------------------------------------------------------
/* Builds an ascii string for the passed in components 04:02:23 for example */
void	constructTime(char *psText, UDWORD hours, UDWORD minutes, UDWORD seconds)
{
UDWORD	index;
UDWORD	div;

	index = 0;
	// Hours do not have trailing zeros
	if(hours!=0)
	{
   		if(hours<10)
		{
			// Less than 10 hours
			psText[index++] = (UBYTE)('0'+ hours%10);
		}
		else if(hours<100)
		{
			// Over ten hours
			psText[index++] = (UBYTE)('0'+ hours/10);
			psText[index++] = (UBYTE)('0'+ hours%10);
		}
		else
		{
			// Over 100 hours - go outside people!!!!
			// build hours
			psText[index++] = (UBYTE)('0' + (hours/100));		// hmmmmmm....
			div = hours/100;
			psText[index++] = (UBYTE)('0' + (hours-(div*100))/10);	// nice
			psText[index++] = (UBYTE)('0' + hours%10);

		}
	   	// Put in the hrs/mins separator - only for non-zero hours
		psText[index++] = (UBYTE)(':');
	}


	// put in the minutes
	psText[index++] = (UBYTE)('0' + minutes/10);
	psText[index++] = (UBYTE)('0' + minutes%10);

	// mins/secs separator
	psText[index++] = (UBYTE)(':');


	// Put in the seconds
	psText[index++] = (UBYTE)('0' + seconds/10);
	psText[index++] = (UBYTE)('0' + seconds%10);

	// terminate the string
	psText[index] = '\0';
}
// --------------------------------------------------------------------
/* Builds an ascii string for the passed in time */
void	getAsciiTime( char *psText, UDWORD time )
{
UDWORD	hours,minutes,seconds;

	getTimeComponents(time,&hours,&minutes,&seconds);
	constructTime(psText,hours,minutes,seconds);
}


// -----------------------------------------------------------------------------------
static void drawStatBars(void)
{
UDWORD	index;
BOOL	bMoreBars;
UDWORD	x,y;
UDWORD	width,height;

	if(!bDispStarted)
	{
		bDispStarted = true;
		dispST = gameTime2;
		audio_PlayTrack(ID_SOUND_BUTTON_CLICK_5);
	}

	fillUpStats();

	pie_UniTransBoxFill(16 + D_W, MT_Y_POS - 16, pie_GetVideoBufferWidth() - D_W - 16, MT_Y_POS + 256+16, WZCOL_SCORE_BOX);
	iV_Box(16 + D_W, MT_Y_POS - 16, pie_GetVideoBufferWidth() - D_W - 16, MT_Y_POS + 256+16, WZCOL_SCORE_BOX_BORDER);

	iV_DrawText( _("Unit Losses"), LC_X + D_W, 80 + 16 + D_H );
	iV_DrawText( _("Structure Losses"), LC_X + D_W, 140 + 16 + D_H );
	iV_DrawText( _("Force Information"), LC_X + D_W, 200 + 16 + D_H );

	index = 0;
	bMoreBars = true;
	while(bMoreBars)
	{
		/* Is it time to display this bar? */
		if( infoBars[index].bActive)
		{
			/* Has it been queued before? */
			if(infoBars[index].bQueued == false)
			{
				/* Don't do this next time...! */
				infoBars[index].bQueued = true;

				/* Play a sound */
//				audio_PlayTrack(ID_SOUND_BUTTON_CLICK_5);
			}
			x = infoBars[index].topX+D_W;
			y = infoBars[index].topY+D_H;
			width = infoBars[index].width;
			height = infoBars[index].height;

			iV_Box(x, y, x + width, y + height, WZCOL_BLACK);

			/* Draw the background border box */
			pie_BoxFill(x - 1, y - 1, x + width + 1, y + height + 1, WZCOL_MENU_BACKGROUND);

			/* Draw the interior grey */
			pie_BoxFill(x, y, x + width, y + height, WZCOL_MENU_SCORES_INTERIOR);

			if( ((gameTime2 - dispST) > infoBars[index].queTime) )
			{
				/* Now draw amount filled */
				const float mul = (gameTime2 - dispST < BAR_CRAWL_TIME) ?
				                   (float)(gameTime2 - dispST) / (float)BAR_CRAWL_TIME
				                  : 1.f;

				const float length = (float)infoBars[index].percent / 100.f * (float)infoBars[index].width * mul;

				if((int)length > 4)
				{

					/* Black shadow */
					pie_BoxFill(x + 1, y + 3, x + length - 1, y + height - 1, WZCOL_MENU_SHADOW);

					/* Solid coloured bit */
					pie_BoxFill(x + 1, y + 2, x + length - 4, y + height - 4, getColour(index));
				}
			}
			/* Now render the text by the bar */
			sprintf(text, getDescription((MR_STRING)infoBars[index].stringID), infoBars[index].number);
			iV_DrawText(text, x + width + 16, y + 12);

			/* If we're beyond STAT_ROOKIE, then we're on rankings */
			if(index>=STAT_GREEN && index <= STAT_ACE)
			{
				iV_DrawImage(IntImages,(UWORD)(IMAGE_LEV_0 + (index - STAT_GREEN)),x-8,y+2);
			}


		}
		/* Move onto the next bar */
		index++;
		if(infoBars[index].topX == 0 && infoBars[index].topY == 0)
		{
			bMoreBars = false;
		}
	}
	dispAdditionalInfo();
}

// -----------------------------------------------------------------------------------
void	dispAdditionalInfo( void )
{

	/* We now need to display the mission time, game time,
		average unit experience level an number of artefacts found */

	/* Firstly, top of the screen, number of artefacts found */
	sprintf( text, _("ARTIFACTS RECOVERED: %d"), missionData.artefactsFound );
	iV_DrawText( text, (pie_GetVideoBufferWidth() - iV_GetTextWidth(text))/2, 300 + D_H );

	/* Get the mission result time in a string - and write it out */
	getAsciiTime( (char*)&text2, gameTime - missionData.missionStarted );
	sprintf( text, _("Mission Time - %s"), text2 );
	iV_DrawText( text, (pie_GetVideoBufferWidth() - iV_GetTextWidth(text))/2, 320 + D_H);

	/* Write out total game time so far */
	getAsciiTime( (char*)&text2, gameTime );
	sprintf( text, _("Total Game Time - %s"), text2 );
	iV_DrawText( text, (pie_GetVideoBufferWidth() - iV_GetTextWidth(text))/2, 340 + D_H );
	if (Cheated)
	{
		// A quick way to flash the text
		((gameTime2 / 250) % 2) ? iV_SetTextColour(WZCOL_RED) : iV_SetTextColour(WZCOL_YELLOW);
		sprintf( text, _("You cheated!"));
		iV_DrawText( text, (pie_GetVideoBufferWidth() - iV_GetTextWidth(text))/2, 360 + D_H );
		iV_SetTextColour(WZCOL_TEXT_BRIGHT);
	}
}

// -----------------------------------------------------------------------------------
void	fillUpStats( void )
{
	UDWORD	i;
	UDWORD	maxi,num;
	float	scaleFactor;
	UDWORD	length;
	UDWORD	numUnits;
	DROID	*psDroid;

	/* Do rankings first cos they're easier */
	for(i=0,maxi=0; i<DROID_LEVELS; i++)
	{
		num = getNumDroidsForLevel(i);
		if(num>maxi)
		{
			maxi = num;
		}
	}

	/* Make sure we got something */
	if(maxi == 0)
	{
		scaleFactor = 0.f;
	}
	else
	{
		scaleFactor = (float)RANK_BAR_WIDTH / maxi;
	}

	/* Scale for percent */
	for(i=0; i<DROID_LEVELS; i++)
	{
		length = scaleFactor * getNumDroidsForLevel(i);
		infoBars[STAT_ROOKIE+i].percent = PERCENT(length,RANK_BAR_WIDTH);
		infoBars[STAT_ROOKIE+i].number = getNumDroidsForLevel(i);
	}

	/* Now do the other stuff... */
	/* Units killed and lost... */
	maxi = MAX(missionData.unitsLost, missionData.unitsKilled);
	if (maxi == 0)
	{
		scaleFactor = 0.f;
	}
	else
	{
		scaleFactor = (float)STAT_BAR_WIDTH / maxi;
	}

	length = scaleFactor * missionData.unitsLost;
	infoBars[STAT_UNIT_LOST].percent = PERCENT(length,STAT_BAR_WIDTH);
	length = scaleFactor * missionData.unitsKilled;
	infoBars[STAT_UNIT_KILLED].percent = PERCENT(length,STAT_BAR_WIDTH);

	/* Now do the structure losses */
	maxi = MAX(missionData.strLost, missionData.strKilled);
	if (maxi == 0)
	{
		scaleFactor = 0.f;
	}
	else
	{
		scaleFactor = (float)STAT_BAR_WIDTH / maxi;
	}

	length = scaleFactor * missionData.strLost;
	infoBars[STAT_STR_LOST].percent = PERCENT(length,STAT_BAR_WIDTH);
	length = scaleFactor * missionData.strKilled;
	infoBars[STAT_STR_BLOWN_UP].percent = PERCENT(length,STAT_BAR_WIDTH);

	/* Finally the force information - need amount of droids as well*/
	for(psDroid = apsDroidLists[selectedPlayer], numUnits = 0; psDroid; psDroid = psDroid->psNext, numUnits++) {}

	for(psDroid = mission.apsDroidLists[selectedPlayer]; psDroid; psDroid = psDroid->psNext, numUnits++) {}

	maxi = MAX(missionData.unitsBuilt, missionData.strBuilt);
	maxi = MAX(maxi, numUnits);

	if (maxi == 0)
	{
		scaleFactor = 0.f;
	}
	else
	{
		scaleFactor = (float)STAT_BAR_WIDTH / maxi;
	}

	length = scaleFactor * missionData.unitsBuilt;
	infoBars[STAT_UNITS_BUILT].percent = PERCENT(length,STAT_BAR_WIDTH);
	length = scaleFactor * numUnits;
	infoBars[STAT_UNITS_NOW].percent = PERCENT(length,STAT_BAR_WIDTH);
	length = scaleFactor * missionData.strBuilt;
	infoBars[STAT_STR_BUILT].percent = PERCENT(length,STAT_BAR_WIDTH);

	/* Finally the numbers themselves */
	infoBars[STAT_UNIT_LOST].number = missionData.unitsLost;
	infoBars[STAT_UNIT_KILLED].number = missionData.unitsKilled;
	infoBars[STAT_STR_LOST].number = missionData.strLost;
	infoBars[STAT_STR_BLOWN_UP].number = missionData.strKilled;
	infoBars[STAT_UNITS_BUILT].number =	missionData.unitsBuilt;
	infoBars[STAT_UNITS_NOW].number = numUnits;
	infoBars[STAT_STR_BUILT].number = missionData.strBuilt;
}

static const char ScoreData_tag_definition[] = "tagdefinitions/savegame/score.def";
static const char ScoreData_file_identifier[] = "ScoreData";

// -----------------------------------------------------------------------------------
/* This will save out the score data */
bool writeScoreData(const char* fileName)
{
	if (!tagOpenWrite(ScoreData_tag_definition, fileName))
	{
		ASSERT(false, "writeScoreData: error while opening file (%s)", fileName);
		return false;
	}

	tagWriteString(0x01, ScoreData_file_identifier);

	// Dump the scores for the current player
	tagWrite(0x02, missionData.unitsBuilt);
	tagWrite(0x03, missionData.unitsKilled);
	tagWrite(0x04, missionData.unitsLost);
	tagWrite(0x05, missionData.strBuilt);
	tagWrite(0x06, missionData.strKilled);
	tagWrite(0x07, missionData.strLost);
	tagWrite(0x08, missionData.artefactsFound);
	tagWrite(0x09, missionData.missionStarted);
	tagWrite(0x0A, missionData.shotsOnTarget);
	tagWrite(0x0B, missionData.shotsOffTarget);
	tagWrite(0x0C, missionData.babasMowedDown);

	// Close the file
	tagClose();

	// Everything is just fine!
	return true;
}

// -----------------------------------------------------------------------------------
/* This will read in the score data */
bool readScoreData(const char* fileName)
{
	char strbuffer[25];

	if (!tagOpenRead(ScoreData_tag_definition, fileName))
	{
		debug(LOG_ERROR, "readFXData: error while opening file (%s)", fileName);
		return false;
	}

	// Read & verify the format header identifier
	tagReadString(0x01, sizeof(strbuffer), strbuffer);
	if (strncmp(strbuffer, ScoreData_file_identifier, sizeof(strbuffer)) != 0)
	{
		debug(LOG_ERROR, "readScoreData: Weird file type found (in file %s)? Has header string: %s", fileName, strbuffer);
		return false;
	}

	// Retrieve the score data for the current player
	missionData.unitsBuilt = tagRead(0x02);
	missionData.unitsKilled = tagRead(0x03);
	missionData.unitsLost = tagRead(0x04);
	missionData.strBuilt = tagRead(0x05);
	missionData.strKilled = tagRead(0x06);
	missionData.strLost = tagRead(0x07);
	missionData.artefactsFound = tagRead(0x08);
	missionData.missionStarted = tagRead(0x09);
	missionData.shotsOnTarget = tagRead(0x0A);
	missionData.shotsOffTarget = tagRead(0x0B);
	missionData.babasMowedDown = tagRead(0x0C);

	// Close the file
	tagClose();

	/* Hopefully everything's just fine by now */
	return true;
}
