/*
 * IntDisplay.c
 *
 * Callback and display functions for interface.
 *
 */

#include <stdio.h>
#include <math.h>
#include <assert.h>

#include "lib/framework/frame.h"
//#include "widget.h"

#include "objects.h"
#include "loop.h"
#include "edit2d.h"
#include "map.h"
#include "radar.h"
/* Includes direct access to render library */
#include "lib/ivis_common/ivisdef.h"
#include "lib/ivis_common/piestate.h"

#include "lib/ivis_common/piemode.h"			// ffs
#include "lib/ivis_common/pieclip.h"			// ffs
#include "lib/ivis_common/pieblitfunc.h"

// FIXME Direct iVis implementation include!
#include "lib/ivis_opengl/vid.h"
#include "lib/ivis_common/geo.h"

#include "display3d.h"
#include "edit3d.h"
#include "disp2d.h"
#include "structure.h"
#include "research.h"
#include "function.h"
#include "lib/gamelib/gtime.h"
#include "hci.h"
#include "stats.h"
#include "game.h"
#include "power.h"
#include "lib/sound/audio.h"
#include "audio_id.h"
#include "lib/framework/fractions.h"
#include "order.h"
#include "frontend.h"
#include "intimage.h"
#include "intdisplay.h"
#include "component.h"
#include "console.h"
#include "cmddroid.h"
#include "group.h"
#include "csnap.h"
#include "text.h"
#include "transporter.h"
#include "mission.h"



#include "multiplay.h"



// Is a clickable form widget hilited, either because the cursor is over it or it is flashing.
//
//#define formIsHilite(p) ( (((W_CLICKFORM*)p)->state & WCLICK_HILITE) || (((W_CLICKFORM*)p)->state & WCLICK_FLASHON) )
#define formIsHilite(p) 	(((W_CLICKFORM*)p)->state & WCLICK_HILITE)
#define formIsFlashing(p)	(((W_CLICKFORM*)p)->state & WCLICK_FLASHON)

// Is a button widget hilited, either because the cursor is over it or it is flashing.
//
//#define buttonIsHilite(p) ( (((W_BUTTON*)p)->state & WBUTS_HILITE) ||	(((W_BUTTON*)p)->state & WBUTS_FLASHON) )
#define buttonIsHilite(p) 	(((W_BUTTON*)p)->state & WBUTS_HILITE)
#define buttonIsFlashing(p)  (((W_BUTTON*)p)->state & WBUTS_FLASHON)



#define FORM_OPEN_ANIM_DURATION		(GAME_TICKS_PER_SEC/6) // Time duration for form open/close anims.

//number of pulses in the blip for the radar
#define NUM_PULSES			3

//the loop default value
#define DEFAULT_LOOP		1

static int FormOpenAudioID;	// ID of sfx to play when form opens.
static int FormCloseAudioID; // ID of sfx to play when form closes.
static int FormOpenCount;	// Count used to ensure only one sfx played when two forms opening.
static int FormCloseCount;	// Count used to ensure only one sfx played when two forms closeing.

BASE_STATS *CurrentStatsTemplate = NULL;

#define	DEFAULT_BUTTON_ROTATION (45)
#define BUT_TRANSPORTER_DIST (5000)
#define BUT_TRANSPORTER_SCALE (20)
#define BUT_TRANSPORTER_ALT (-50)



// Token look up table for matching IMD's to droid components.
//
/*TOKENID CompIMDIDs[]={
//COMP_BODY:
	{"Viper Body",IMD_BD_VIPER},
	{"Cobra Body",IMD_BD_COBRA},

//COMP_WEAPON:
	{"Single Rocket",IMD_TR_SINGROCK},
	{"Rocket Pod",IMD_TR_ROCKPOD},
	{"Light Machine Gun",IMD_TR_LGUN},
	{"Light Cannon",IMD_TR_LCAN},
	{"Heavy Cannon",IMD_TR_HCAN},

//COMP_PROPULSION:
	{"Wheeled Propulsion",IMD_PR_WHEELS},
	{"Tracked Propulsion",IMD_PR_TRACKS},
	{"Hover Propulsion",IMD_PR_HOVER},

//COMP_CONSTRUCT:
	{"Building Constructor",IMD_TR_BUILDER},

//COMP_REPAIRUNIT:
	{"Light Repair #1",IMD_TR_LGUN},

//COMP_ECM:
	{"Light ECM #1",IMD_TR_ECM},
	{"Heavy ECM #1",IMD_TR_ECM},

//COMP_SENSOR:
	{"EM Sensor",IMD_TR_SENS},
	{"Default Sensor",IMD_TR_SENS},

	{NULL,-1},
};*/


// Token look up table for matching Images and IMD's to research projects.
//
/*RESEARCHICON ResearchIMAGEIDs[]={
	{"Tracks",			IMAGE_RES_MINOR_TRACKS,		IMD_PR_TRACKS},
	{"Hovercraft",		IMAGE_RES_MINOR_HOVER,		IMD_PR_HOVER},
	{"Light Cannon",	IMAGE_RES_MINOR_HEAVYWEP,	IMD_TR_LCAN},
	{"Heavy Cannon",	IMAGE_RES_MINOR_HEAVYWEP,	IMD_TR_HCAN},
	{"Rocket Launcher",	IMAGE_RES_MINOR_ROCKET,		IMD_TR_SINGROCK},
	{"ECM PickUp",		IMAGE_RES_MINOR_ELECTRONIC,	IMD_TR_ECM},
	{"PlasCrete",		IMAGE_RES_MINOR_PLASCRETE,	IMD_PLASCRETE},
	{"EM Sensor",		IMAGE_RES_MINOR_ELECTRONIC,	IMD_TR_SENS},
	{" Rocket Pod",		IMAGE_RES_MINOR_ROCKET,		IMD_TR_ROCKPOD},

	{NULL,-1},
};*/


UDWORD ManuPower = 0;	// Power required to manufacture the current item.


// Display surfaces for rendered buttons.
BUTTON_SURFACE TopicSurfaces[NUM_TOPICSURFACES];
BUTTON_SURFACE ObjectSurfaces[NUM_OBJECTSURFACES];
BUTTON_SURFACE StatSurfaces[NUM_STATSURFACES];
BUTTON_SURFACE System0Surfaces[NUM_SYSTEM0SURFACES];


// Working buffers for rendered buttons.
RENDERED_BUTTON System0Buffers[NUM_SYSTEM0BUFFERS];	// References ObjectSurfaces.
//RENDERED_BUTTON System1Buffers[NUM_OBJECTBUFFERS];	// References ObjectSurfaces.
//RENDERED_BUTTON System2Buffers[NUM_OBJECTBUFFERS];	// References ObjectSurfaces.
RENDERED_BUTTON ObjectBuffers[NUM_OBJECTBUFFERS];	// References ObjectSurfaces.
RENDERED_BUTTON TopicBuffers[NUM_TOPICBUFFERS];		// References TopicSurfaces.
RENDERED_BUTTON StatBuffers[NUM_STATBUFFERS];		// References StatSurfaces.




// Get the first factory assigned to a command droid
STRUCTURE *droidGetCommandFactory(DROID *psDroid);

static SDWORD ButtonDrawXOffset;
static SDWORD ButtonDrawYOffset;


//static UDWORD DisplayQuantity = 1;
//static SDWORD ActualQuantity = -1;


// Set audio IDs for form opening/closing anims.
// Use -1 to dissable audio.
//
void SetFormAudioIDs(int OpenID,int CloseID)
{
	FormOpenAudioID = OpenID;
	FormCloseAudioID = CloseID;
	FormOpenCount = 0;
	FormCloseCount = 0;
}


// Widget callback to update the progress bar in the object stats screen.
//
void intUpdateProgressBar(struct _widget *psWidget, struct _w_context *psContext)
{
	BASE_OBJECT			*psObj;
	DROID				*Droid;
	STRUCTURE			*Structure;
	FACTORY				*Manufacture;
	RESEARCH_FACILITY	*Research;
	PLAYER_RESEARCH		*pPlayerRes;
	UDWORD				BuildPoints,Range, BuildPower;
	W_BARGRAPH			*BarGraph = (W_BARGRAPH*)psWidget;

	psObj = (BASE_OBJECT*)BarGraph->pUserData;	// Get the object associated with this widget.

	if (psObj == NULL)
	{
		BarGraph->style |= WIDG_HIDDEN;
		return;
	}

//	ASSERT((!psObj->died,"intUpdateProgressBar: object is dead"));
	if(psObj->died AND psObj->died != NOT_CURRENT_LIST)
	{
		return;
	}

	switch (psObj->type) {
		case OBJ_DROID:						// If it's a droid and...
			Droid = (DROID*)psObj;


			if(DroidIsBuilding(Droid)) {		// Is it building.
				ASSERT((Droid->asBits[COMP_CONSTRUCT].nStat,"intUpdateProgressBar: invalid droid type"));
				Structure = DroidGetBuildStructure(Droid);				// Get the structure it's building.
//				ASSERT((Structure != NULL,"intUpdateProgressBar : NULL Structure pointer."));
				if(Structure)
                {
				    //check if have all the power to build yet
                    BuildPower = structPowerToBuild(Structure);
				    //if (Structure->currentPowerAccrued < (SWORD)Structure->pStructureType->powerToBuild)
                    if (Structure->currentPowerAccrued < (SWORD)BuildPower)
				    {
					    //if not started building show how much power accrued
					    //Range = Structure->pStructureType->powerToBuild;
                        Range = BuildPower;
					    BuildPoints = Structure->currentPowerAccrued;
					    //set the colour of the bar to green
					    BarGraph->majorCol = COL_LIGHTGREEN;
					    //and change the tool tip
					    BarGraph->pTip = strresGetString(psStringRes, STR_INT_POWERACCRUED);
				    }
				    else
				    {
                        //show progress of build
    					Range =  Structure->pStructureType->buildPoints;	// And how long it takes to build.
                        BuildPoints = Structure->currentBuildPts;			// How near to completion.
					    //set the colour of the bar to yellow
					    BarGraph->majorCol = COL_YELLOW;
					    //and change the tool tip
					    BarGraph->pTip = strresGetString(psStringRes, STR_INT_BLDPROGRESS);
				    }
					if (BuildPoints > Range)
					{
						BuildPoints = Range;
					}
    				BarGraph->majorSize = (UWORD)PERNUM(WBAR_SCALE,BuildPoints,Range);
					BarGraph->style &= ~WIDG_HIDDEN;
				}
                else
                {
					BarGraph->majorSize = 0;
					BarGraph->style |= WIDG_HIDDEN;
				}
			} else {
				BarGraph->majorSize = 0;
				BarGraph->style |= WIDG_HIDDEN;
			}
			break;

		case OBJ_STRUCTURE:					// If it's a structure and...
			Structure = (STRUCTURE*)psObj;

			if(StructureIsManufacturing(Structure)) {			// Is it manufacturing.
				Manufacture = StructureGetFactory(Structure);
				//check started to build
				if (Manufacture->timeStarted == ACTION_START_TIME)
				{
					//BuildPoints = 0;
					//if not started building show how much power accrued
					Range = ((DROID_TEMPLATE *)Manufacture->psSubject)->powerPoints;
					BuildPoints = Manufacture->powerAccrued;
					//set the colour of the bar to green
					BarGraph->majorCol = COL_LIGHTGREEN;
					//and change the tool tip
					BarGraph->pTip = strresGetString(psStringRes, STR_INT_POWERACCRUED);
				}
				else
				{
    				Range = Manufacture->timeToBuild;
					//set the colour of the bar to yellow
					BarGraph->majorCol = COL_YELLOW;
					//and change the tool tip
					BarGraph->pTip = strresGetString(psStringRes, STR_INT_BLDPROGRESS);
					//if on hold need to take it into account
					if (Manufacture->timeStartHold)
					{
						BuildPoints = (gameTime - (Manufacture->timeStarted + (
							gameTime - Manufacture->timeStartHold))) /
							GAME_TICKS_PER_SEC;
					}
					else
					{
						BuildPoints = (gameTime - Manufacture->timeStarted) /
							GAME_TICKS_PER_SEC;
					}
				}
				if (BuildPoints > Range)
				{
					BuildPoints = Range;
				}
				BarGraph->majorSize = (UWORD)PERNUM(WBAR_SCALE,BuildPoints,Range);
				BarGraph->style &= ~WIDG_HIDDEN;
			} else if(StructureIsResearching(Structure)) {		// Is it researching.
				Research = StructureGetResearch(Structure);
				pPlayerRes = asPlayerResList[selectedPlayer] + ((RESEARCH *)Research->
					psSubject - asResearch);
                //this is no good if you change which lab is researching the topic and one lab is faster
				//Range = Research->timeToResearch;
                Range = ((RESEARCH *)((RESEARCH_FACILITY*)Structure->
                    pFunctionality)->psSubject)->researchPoints;
				//check started to research
				if (Research->timeStarted == ACTION_START_TIME)
				{
					//BuildPoints = 0;
					//if not started building show how much power accrued
					Range = ((RESEARCH *)Research->psSubject)->researchPower;
					BuildPoints = Research->powerAccrued;
					//set the colour of the bar to green
					BarGraph->majorCol = COL_LIGHTGREEN;
					//and change the tool tip
					BarGraph->pTip = strresGetString(psStringRes, STR_INT_POWERACCRUED);
				}
				else
				{
					//set the colour of the bar to yellow
					BarGraph->majorCol = COL_YELLOW;
					//and change the tool tip
					BarGraph->pTip = strresGetString(psStringRes, STR_INT_BLDPROGRESS);
					//if on hold need to take it into account
					if (Research->timeStartHold)
					{

						BuildPoints = ((RESEARCH_FACILITY*)Structure->pFunctionality)->
                            researchPoints * (gameTime - (Research->timeStarted + (
                            gameTime - Research->timeStartHold))) / GAME_TICKS_PER_SEC;


						BuildPoints+= pPlayerRes->currentPoints;


					}
					else
					{

				    	BuildPoints = ((RESEARCH_FACILITY*)Structure->pFunctionality)->
                            researchPoints * (gameTime - Research->timeStarted) /
                            GAME_TICKS_PER_SEC;

				    	BuildPoints+= pPlayerRes->currentPoints;


					}
				}
				if (BuildPoints > Range)
				{
					BuildPoints = Range;
				}
				BarGraph->majorSize = (UWORD)PERNUM(WBAR_SCALE,BuildPoints,Range);
				BarGraph->style &= ~WIDG_HIDDEN;
			} else {
				BarGraph->majorSize = 0;
				BarGraph->style |= WIDG_HIDDEN;
			}

			break;

		default:
			ASSERT((FALSE, "intUpdateProgressBar: invalid object type"));
	}
}


void intUpdateQuantity(struct _widget *psWidget, struct _w_context *psContext)
{
	BASE_OBJECT		*psObj;
	STRUCTURE		*Structure;
	DROID_TEMPLATE	*psTemplate;
	W_LABEL			*Label = (W_LABEL*)psWidget;
	UDWORD			Quantity, Remaining;

	psObj = (BASE_OBJECT*)Label->pUserData;	// Get the object associated with this widget.
	Structure = (STRUCTURE*)psObj;

	if( (psObj != NULL) &&
		(psObj->type == OBJ_STRUCTURE) && (StructureIsManufacturing(Structure)) )
	{
		ASSERT((!psObj->died,"intUpdateQuantity: object is dead"));

		/*Quantity = StructureGetFactory(Structure)->quantity;
		if (Quantity == NON_STOP_PRODUCTION)
		{
			Label->aText[0] = (UBYTE)('*');
			Label->aText[1] = (UBYTE)('\0');
		}
		else
		{
			Label->aText[0] = (UBYTE)('0'+Quantity / 10);
			Label->aText[1] = (UBYTE)('0'+Quantity % 10);
		}*/

		psTemplate = (DROID_TEMPLATE *)StructureGetFactory(Structure)->psSubject;
   		//Quantity = getProductionQuantity(Structure, psTemplate) -
		//					getProductionBuilt(Structure, psTemplate);
        Quantity = getProductionQuantity(Structure, psTemplate);
        Remaining = getProductionBuilt(Structure, psTemplate);
        if (Quantity > Remaining)
        {
            Quantity -= Remaining;
        }
        else
        {
            Quantity = 0;
        }
		if (Quantity)
		{
			Label->aText[0] = (UBYTE)('0'+Quantity / 10);
			Label->aText[1] = (UBYTE)('0'+Quantity % 10);
		}
		Label->style &= ~WIDG_HIDDEN;
	}
	else
	{
		Label->style |= WIDG_HIDDEN;
	}
}

//callback to display the factory number
void intAddFactoryInc(struct _widget *psWidget, struct _w_context *psContext)
{
	BASE_OBJECT			*psObj;
	STRUCTURE			*Structure;
	W_LABEL				*Label = (W_LABEL*)psWidget;

	// Get the object associated with this widget.
	psObj = (BASE_OBJECT*)Label->pUserData;
	if (psObj != NULL)
	{
		ASSERT((PTRVALID(psObj, sizeof(STRUCTURE)) && psObj->type == OBJ_STRUCTURE,
			"intAddFactoryInc: invalid structure pointer"));

		ASSERT((!psObj->died,"intAddFactoryInc: object is dead"));

		Structure = (STRUCTURE*)psObj;

		ASSERT(((Structure->pStructureType->type == REF_FACTORY OR
			Structure->pStructureType->type == REF_CYBORG_FACTORY OR
			Structure->pStructureType->type == REF_VTOL_FACTORY),
			"intAddFactoryInc: structure is not a factory"));

		Label->aText[0] = (UBYTE)('0' + (((FACTORY *)Structure->pFunctionality)->
			psAssemblyPoint->factoryInc+1));
		Label->aText[1] = (UBYTE)('\0');
		Label->style &= ~WIDG_HIDDEN;
	}
	else
	{
		Label->aText[0] = (UBYTE)0;
		Label->style |= WIDG_HIDDEN;
	}
}

//callback to display the production quantity number for a template
void intAddProdQuantity(struct _widget *psWidget, struct _w_context *psContext)
{
	BASE_STATS			*psStat;
	DROID_TEMPLATE		*psTemplate;
	STRUCTURE			*psStructure = NULL;
	BASE_OBJECT			*psObj = NULL;
	W_LABEL				*Label = (W_LABEL*)psWidget;
	UDWORD				quantity = 0;

	// Get the object associated with this widget.
	psStat = (BASE_STATS *)Label->pUserData;
	if (psStat != NULL)
	{
		ASSERT((PTRVALID(psStat, sizeof(DROID_TEMPLATE)),
			"intAddProdQuantity: invalid template pointer"));

		psTemplate = (DROID_TEMPLATE *)psStat;

		psObj = getCurrentSelected();
		if (psObj != NULL AND psObj->type == OBJ_STRUCTURE)
		{
			psStructure = (STRUCTURE *)psObj;
		}

		if (psStructure != NULL AND StructIsFactory(psStructure))
		{
		    quantity = getProductionQuantity(psStructure, psTemplate);
		}

		if (quantity != 0)
		{
			Label->aText[0] = (UBYTE)('0' + quantity);
			Label->aText[1] = (UBYTE)('\0');
			Label->style &= ~WIDG_HIDDEN;
		}
		else
		{
			Label->aText[0] = (UBYTE)0;
			Label->style |= WIDG_HIDDEN;
		}
	}
}

//callback to display the production loop quantity number for a factory
void intAddLoopQuantity(struct _widget *psWidget, struct _w_context *psContext)
{
	FACTORY				*psFactory = NULL;
	W_LABEL				*Label = (W_LABEL*)psWidget;

	//loop depends on the factory
	if (Label->pUserData != NULL)
	{
		psFactory = (FACTORY *)((STRUCTURE *)Label->pUserData)->pFunctionality;

		if (psFactory->quantity)
		{
			if (psFactory->quantity == INFINITE_PRODUCTION)
			{
				Label->aText[0] = (UBYTE)(7);
				Label->aText[1] = (UBYTE)('\0');
			}
			else
			{
				Label->aText[0] = (UBYTE)('0' + psFactory->quantity / 10);
				Label->aText[1] = (UBYTE)('0' + (psFactory->quantity + DEFAULT_LOOP) % 10);
				Label->aText[2] = (UBYTE)('\0');
			}
		}
		else
		{
			//set to default loop quantity
			Label->aText[0] = (UBYTE)('0');
			Label->aText[1] = (UBYTE)('0' + DEFAULT_LOOP);
			Label->aText[2] = (UBYTE)('\0');
		}
		Label->style &= ~WIDG_HIDDEN;
	}
	else
	{
		//hide the label if no factory
		Label->aText[0] = (UBYTE)0;
		Label->style |= WIDG_HIDDEN;
	}
}

// callback to update the command droid size label
void intUpdateCommandSize(struct _widget *psWidget, struct _w_context *psContext)
{
	BASE_OBJECT			*psObj;
	DROID				*psDroid;
	W_LABEL				*Label = (W_LABEL*)psWidget;

	// Get the object associated with this widget.
	psObj = (BASE_OBJECT*)Label->pUserData;
	if (psObj != NULL)
	{
		ASSERT((PTRVALID(psObj, sizeof(DROID)) && psObj->type == OBJ_DROID,
			"intUpdateCommandSize: invalid droid pointer"));

		ASSERT((!psObj->died,"intUpdateCommandSize: droid has died"));

		psDroid = (DROID *)psObj;

		ASSERT((psDroid->droidType == DROID_COMMAND,
			"intUpdateCommandSize: droid is not a command droid"));

		sprintf(Label->aText, "%d/%d", psDroid->psGroup ? grpNumMembers(psDroid->psGroup) : 0, cmdDroidMaxGroup(psDroid));
		Label->style &= ~WIDG_HIDDEN;
	}
	else
	{
		Label->aText[0] = (UBYTE)0;
		Label->style |= WIDG_HIDDEN;
	}
}

// callback to update the command droid experience
void intUpdateCommandExp(struct _widget *psWidget, struct _w_context *psContext)
{
	BASE_OBJECT			*psObj;
	DROID				*psDroid;
	W_LABEL				*Label = (W_LABEL*)psWidget;
	SDWORD				i, numStars;

	// Get the object associated with this widget.
	psObj = (BASE_OBJECT*)Label->pUserData;
	if (psObj != NULL)
	{
		ASSERT((PTRVALID(psObj, sizeof(DROID)) && psObj->type == OBJ_DROID,
			"intUpdateCommandSize: invalid droid pointer"));

		ASSERT((!psObj->died,"intUpdateCommandSize: droid has died"));

		psDroid = (DROID *)psObj;

		ASSERT((psDroid->droidType == DROID_COMMAND,
			"intUpdateCommandSize: droid is not a command droid"));

		numStars = cmdDroidGetLevel(psDroid);
		numStars = (numStars >= 1) ? (numStars - 1) : 0;
		for(i=0; i<numStars; i++)
		{
			Label->aText[i] = '*';
		}
		Label->aText[i] = '\0';
		Label->style &= ~WIDG_HIDDEN;
	}
	else
	{
		Label->aText[0] = (UBYTE)0;
		Label->style |= WIDG_HIDDEN;
	}
}

// callback to update the command droid factories
void intUpdateCommandFact(struct _widget *psWidget, struct _w_context *psContext)
{
	BASE_OBJECT			*psObj;
	DROID				*psDroid;
	W_LABEL				*Label = (W_LABEL*)psWidget;
	SDWORD				i,cIndex, start;

	// Get the object associated with this widget.
	psObj = (BASE_OBJECT*)Label->pUserData;
	if (psObj != NULL)
	{
		ASSERT((PTRVALID(psObj, sizeof(DROID)) && psObj->type == OBJ_DROID,
			"intUpdateCommandSize: invalid droid pointer"));

		ASSERT((!psObj->died,"intUpdateCommandSize: droid has died"));

		psDroid = (DROID *)psObj;

		ASSERT((psDroid->droidType == DROID_COMMAND,
			"intUpdateCommandSize: droid is not a command droid"));

		// see which type of factory this is for
		if (Label->id >= IDOBJ_COUNTSTART && Label->id < IDOBJ_COUNTEND)
		{
			start = DSS_ASSPROD_SHIFT;
		}
		else if (Label->id >= IDOBJ_CMDFACSTART && Label->id < IDOBJ_CMDFACEND)
		{
			start = DSS_ASSPROD_CYBORG_SHIFT;
		}
		else
		{
			start = DSS_ASSPROD_VTOL_SHIFT;
		}

		cIndex = 0;
		for(i=0; i<MAX_FACTORY; i++)
		{
			if ( psDroid->secondaryOrder & (1 << (i + start)) )
			{
				Label->aText[cIndex] = (STRING) ('0' + i + 1);
				cIndex += 1;
			}
		}
		Label->aText[cIndex] = '\0';
		Label->style &= ~WIDG_HIDDEN;
	}
	else
	{
		Label->aText[0] = (UBYTE)0;
		Label->style |= WIDG_HIDDEN;
	}
}


//#ifndef PSX
#define DRAW_POWER_BAR_TEXT TRUE
//#endif

#define BARXOFFSET	46

// Widget callback to update and display the power bar.
// !!!!!!!!!!!!!!!!!!!!!!ONLY WORKS ON A SIDEWAYS POWERBAR!!!!!!!!!!!!!!!!!
void intDisplayPowerBar(struct _widget *psWidget, UDWORD xOffset, UDWORD yOffset, UDWORD *pColours)
{
	W_BARGRAPH *BarGraph = (W_BARGRAPH*)psWidget;
	SDWORD		x0,y0;
	SDWORD		Avail,ManPow,realPower;
	SDWORD		Empty;
	SDWORD		BarWidth, textWidth = 0;
	SDWORD		iX,iY;
#if	DRAW_POWER_BAR_TEXT && !defined(PSX)
	static char		szVal[8];
#endif
		//SDWORD Used,Avail,ManPow;

//	asPower[selectedPlayer]->availablePower+=32;	// temp to test.


	ManPow = ManuPower / POWERBAR_SCALE;
	Avail = asPower[selectedPlayer]->currentPower / POWERBAR_SCALE;
	realPower = asPower[selectedPlayer]->currentPower - ManuPower;

	BarWidth = BarGraph->width;
#if	DRAW_POWER_BAR_TEXT && !defined(PSX)
    iV_SetFont(WFont);
	sprintf( szVal, "%d", realPower );
	textWidth = iV_GetTextWidth( szVal );
	BarWidth -= textWidth;
#endif



	/*Avail = asPower[selectedPlayer]->availablePower / POWERBAR_SCALE;
	Used = asPower[selectedPlayer]->usedPower / POWERBAR_SCALE;*/

	/*if (Used < 0)
	{
		Used = 0;
	}

	Total = Avail + Used;*/

	/*if(ManPow > Avail) {
		ManPow = Avail;
	}*/

	//Empty = BarGraph->width - Total;
	if (ManPow > Avail)
	{
		Empty = BarWidth - ManPow;
	}
	else
	{
		Empty = BarWidth - Avail;
	}

	//if(Total > BarGraph->width) {				// If total size greater than bar size then scale values.
	if(Avail > BarWidth) {
		//Used = PERNUM(BarGraph->width,Used,Total);
		//ManPow = PERNUM(BarGraph->width,ManPow,Total);
		//Avail = BarGraph->width - Used;
		ManPow = PERNUM(BarWidth,ManPow,Avail);
		Avail = BarWidth;
		Empty = 0;
	}

	if (ManPow > BarWidth)
	{
		ManPow = BarWidth;
		Avail = 0;
		Empty = 0;
	}

	x0 = xOffset + BarGraph->x;
	y0 = yOffset + BarGraph->y;




//	pie_SetDepthBufferStatus(DEPTH_CMP_ALWAYS_WRT_OFF);
	pie_SetDepthBufferStatus(DEPTH_CMP_ALWAYS_WRT_ON);
	pie_SetFogStatus(FALSE);


	iV_DrawTransImage(IntImages,IMAGE_PBAR_TOP,x0,y0);

#if	DRAW_POWER_BAR_TEXT && !defined(PSX)
	iX = x0 + 3;
	iY = y0 + 9;
#else

 	iX = x0;
 	iY = y0;

#endif

	x0 += iV_GetImageWidthNoCC(IntImages,IMAGE_PBAR_TOP);


	/* indent to allow text value */
	//draw used section
	/*iV_DrawImageRect(IntImages,IMAGE_PBAR_USED,
						x0,y0,
						0,0,
						Used, iV_GetImageHeight(IntImages,IMAGE_PBAR_USED));
	x0 += Used;*/

	//fill in the empty section behind text
	if(textWidth > 0)
	{
		iV_DrawImageRect(IntImages,IMAGE_PBAR_EMPTY,
							x0,y0,
							0,0,
							textWidth, iV_GetImageHeightNoCC(IntImages,IMAGE_PBAR_EMPTY));
		x0 += textWidth;
	}




	//draw required section
	if (ManPow > Avail)
	{
		//draw the required in red
		iV_DrawImageRect(IntImages,IMAGE_PBAR_USED,
							x0,y0,
							0,0,
							ManPow, iV_GetImageHeightNoCC(IntImages,IMAGE_PBAR_USED));
	}
	else
	{
		iV_DrawImageRect(IntImages,IMAGE_PBAR_REQUIRED,
							x0,y0,
							0,0,
							ManPow, iV_GetImageHeightNoCC(IntImages,IMAGE_PBAR_REQUIRED));
	}

	x0 += ManPow;

	//draw the available section if any!
	if(Avail-ManPow > 0)
	{
		iV_DrawImageRect(IntImages,IMAGE_PBAR_AVAIL,
							x0,y0,
							0,0,
							Avail-ManPow, iV_GetImageHeightNoCC(IntImages,IMAGE_PBAR_AVAIL));

		x0 += Avail-ManPow;
	}

	//fill in the rest with empty section
	if(Empty > 0)
	{
		iV_DrawImageRect(IntImages,IMAGE_PBAR_EMPTY,
							x0,y0,
							0,0,
							Empty, iV_GetImageHeightNoCC(IntImages,IMAGE_PBAR_EMPTY));
		x0 += Empty;
	}

	iV_DrawTransImage(IntImages,IMAGE_PBAR_BOTTOM,x0,y0);
	/* draw text value */




#if	DRAW_POWER_BAR_TEXT && !defined(PSX)
	iV_SetTextColour(-1);
	iV_DrawText( szVal, iX, iY );
#endif
}


// Widget callback to display a rendered status button, ie the progress of a manufacturing or
// building task.
//
void intDisplayStatusButton(struct _widget *psWidget, UDWORD xOffset, UDWORD yOffset, UDWORD *pColours)
{
	W_CLICKFORM         *Form = (W_CLICKFORM*)psWidget;
	BASE_OBJECT         *psObj;
	STRUCTURE           *Structure;
	DROID               *Droid;
	BOOL                Down;
	SDWORD              Image;
	BOOL                Hilight = FALSE;
	BASE_STATS          *Stats, *psResGraphic;
	RENDERED_BUTTON     *Buffer = (RENDERED_BUTTON*)Form->pUserData;
	UDWORD              IMDType = 0, compID;
	UDWORD              Player = selectedPlayer;			// changed by AJL for multiplayer.
	void                *Object;
	BOOL	            bOnHold = FALSE;

	OpenButtonRender((UWORD)(xOffset+Form->x), (UWORD)(yOffset+Form->y),(UWORD)Form->width,(UWORD)Form->height);

	Down = Form->state & (WCLICK_DOWN | WCLICK_LOCKED | WCLICK_CLICKLOCK);


//	if( (pie_GetRenderEngine() == ENGINE_GLIDE) || (IsBufferInitialised(Buffer)==FALSE) || (Form->state & WCLICK_HILITE) || (Form->state!=Buffer->State) ) {
	if( pie_Hardware() || (IsBufferInitialised(Buffer)==FALSE) || (Form->state & WCLICK_HILITE) || (Form->state!=Buffer->State) ) {

		Hilight = Form->state & WCLICK_HILITE;

		if(Hilight) {
			Buffer->ImdRotation += (UWORD) ((BUTTONOBJ_ROTSPEED*frameTime2) / GAME_TICKS_PER_SEC);
		}

		Hilight = formIsHilite(Form);	// Hilited or flashing.

		Buffer->State = Form->state;

//		Down = Form->state & (WCLICK_DOWN | WCLICK_LOCKED | WCLICK_CLICKLOCK);

		Object = NULL;
		Image = -1;
		psObj = (BASE_OBJECT*)Buffer->Data;	// Get the object associated with this widget.

		if (psObj && (psObj->died) AND (psObj->died != NOT_CURRENT_LIST))
		{
			// this may catch this horrible crash bug we've been having,
			// who knows?.... Shipping tomorrow, la de da :-)
			psObj = NULL;
			Buffer->Data = NULL;
			intRefreshScreen();
		}

		if(psObj) {
//			screenTextOut(64,48,"psObj: %p",psObj);
			switch (psObj->type) {
				case OBJ_DROID:						// If it's a droid...
					Droid = (DROID*)psObj;

					if(DroidIsBuilding(Droid)) {
						Structure = DroidGetBuildStructure(Droid);
//						DBPRINTF(("%p : %p",Droid,Structure));
						if(Structure) {
							Object = Structure;	//(void*)StructureGetIMD(Structure);
							IMDType = IMDTYPE_STRUCTURE;
							RENDERBUTTON_INITIALISED(Buffer);
						}
					} else if (DroidGoingToBuild(Droid)) {
						Stats = DroidGetBuildStats(Droid);
						ASSERT((Stats!=NULL,"intDisplayStatusButton : NULL Stats pointer."));
						Object = (void*)Stats;	//StatGetStructureIMD(Stats,selectedPlayer);
						Player = selectedPlayer;
						IMDType = IMDTYPE_STRUCTURESTAT;
						RENDERBUTTON_INITIALISED(Buffer);
					} else if (orderState(Droid, DORDER_DEMOLISH))
                    {
						Stats = (BASE_STATS *)structGetDemolishStat();
						ASSERT((Stats!=NULL,"intDisplayStatusButton : NULL Stats pointer."));
						Object = (void*)Stats;
						Player = selectedPlayer;
						IMDType = IMDTYPE_STRUCTURESTAT;
						RENDERBUTTON_INITIALISED(Buffer);
                    }
                    else if (Droid->droidType == DROID_COMMAND)
					{
						Structure = droidGetCommandFactory(Droid);
						if (Structure) {
							Object = Structure;
							IMDType = IMDTYPE_STRUCTURE;
							RENDERBUTTON_INITIALISED(Buffer);
						}
					}
					break;

				case OBJ_STRUCTURE:					// If it's a structure...
					Structure = (STRUCTURE*)psObj;
					switch(Structure->pStructureType->type) {
						case REF_FACTORY:
						case REF_CYBORG_FACTORY:
						case REF_VTOL_FACTORY:
							if(StructureIsManufacturing(Structure)) {
								IMDType = IMDTYPE_DROIDTEMPLATE;
								Object = (void*)FactoryGetTemplate(StructureGetFactory(Structure));
								RENDERBUTTON_INITIALISED(Buffer);
								if (StructureGetFactory(Structure)->timeStartHold)
								{
									bOnHold = TRUE;
								}
							}

							break;

						case REF_RESEARCH:
							if(StructureIsResearching(Structure)) {
								Stats = (BASE_STATS*)Buffer->Data2;
								if(Stats) {
									/*StatGetResearchImage(Stats,&Image,(iIMDShape**)&Object,FALSE);
									//if Object != NULL the there must be a IMD so set the object to
									//equal the Research stat
									if (Object != NULL)
									{
										Object = (void*)Stats;
									}
    								IMDType = IMDTYPE_RESEARCH;*/
	    							if (((RESEARCH_FACILITY *)Structure->
                                        pFunctionality)->timeStartHold)
		    						{
			    						bOnHold = TRUE;
				    				}
                                    StatGetResearchImage(Stats,&Image,(iIMDShape**)&Object,
                                        &psResGraphic, FALSE);
                                    if (psResGraphic)
                                    {
                                        //we have a Stat associated with this research topic
                                        if  (StatIsStructure(psResGraphic))
                                        {
                                            //overwrite the Object pointer
                                            Object = (void*)psResGraphic;
				                            Player = selectedPlayer;
                                            //this defines how the button is drawn
				                            IMDType = IMDTYPE_STRUCTURESTAT;
                                        }
                                        else
                                        {
            				                compID = StatIsComponent(psResGraphic);
				                            if (compID != COMP_UNKNOWN)
				                            {
                                                //this defines how the button is drawn
					                            IMDType = IMDTYPE_COMPONENT;
                                                //overwrite the Object pointer
					                            Object = (void*)psResGraphic;
				                            }
                                            else
                                            {
                                                ASSERT((FALSE,
                                                    "intDisplayStatsButton:Invalid Stat for research button"));
                                                Object = NULL;
                                                IMDType = IMDTYPE_RESEARCH;
                                            }
                                        }
                                    }
                                    else
                                    {
                                        //no Stat for this research topic so just use the graphic provided
                                        //if Object != NULL the there must be a IMD so set the object to
                                        //equal the Research stat
                                        if (Object != NULL)
                                        {
                                            Object = (void*)Stats;
        									IMDType = IMDTYPE_RESEARCH;
                                        }
                                    }
									RENDERBUTTON_INITIALISED(Buffer);
								}
//								Image = ResearchGetImage((RESEARCH_FACILITY*)Structure);
							}
							break;
					}
					break;

				default:
					ASSERT((FALSE, "intDisplayObjectButton: invalid structure type"));
			}
		} else
		{
			RENDERBUTTON_INITIALISED(Buffer);
		}

		ButtonDrawXOffset = ButtonDrawYOffset = 0;


		// Render the object into the button.
		if(Object) {
			if(Image >= 0) {
				RenderToButton(IntImages,(UWORD)Image,Object,Player,Buffer,Down,IMDType,TOPBUTTON);
			} else {
				RenderToButton(NULL,0,Object,Player,Buffer,Down,IMDType,TOPBUTTON);
			}
		} else if(Image >= 0) {
			RenderImageToButton(IntImages,(UWORD)Image,Buffer,Down,TOPBUTTON);
		} else {
			RenderBlankToButton(Buffer,Down,TOPBUTTON);
		}



//						RENDERBUTTON_INITIALISED(Buffer);
	}

//	DBPRINTF(("%d\n",iV_GetOTIndex_PSX());

	// Draw the button.
	RenderButton(psWidget,Buffer, xOffset+Form->x, yOffset+Form->y, TOPBUTTON,Down);



	CloseButtonRender();

	//need to flash the button if a factory is on hold production
	if (bOnHold)
	{
		if (((gameTime2/250) % 2) == 0)
		{
			iV_DrawTransImage(IntImages,IMAGE_BUT0_DOWN,xOffset+Form->x,yOffset+Form->y);
		}
		else
		{
			iV_DrawTransImage(IntImages,IMAGE_BUT_HILITE,xOffset+Form->x,yOffset+Form->y);
		}
	}
	else
	{
		if (Hilight)
		{

			iV_DrawTransImage(IntImages,IMAGE_BUT_HILITE,xOffset+Form->x,yOffset+Form->y);
		}
	}
}


// Widget callback to display a rendered object button.
//
void intDisplayObjectButton(struct _widget *psWidget, UDWORD xOffset, UDWORD yOffset, UDWORD *pColours)
{
	W_CLICKFORM *Form = (W_CLICKFORM*)psWidget;
	BASE_OBJECT *psObj;
	BOOL Down;
	BOOL Hilight = FALSE;
	RENDERED_BUTTON *Buffer = (RENDERED_BUTTON*)Form->pUserData;
	UDWORD IMDType = 0;
	void *Object;

	OpenButtonRender((UWORD)(xOffset+Form->x), (UWORD)(yOffset+Form->y),(UWORD)Form->width,(UWORD)(Form->height+9));

	Down = Form->state & (WCLICK_DOWN | WCLICK_LOCKED | WCLICK_CLICKLOCK);


//	if( (pie_GetRenderEngine() == ENGINE_GLIDE) || (IsBufferInitialised(Buffer)==FALSE) || (Form->state & WCLICK_HILITE) || (Form->state!=Buffer->State)  ) {
	if( pie_Hardware() || (IsBufferInitialised(Buffer)==FALSE) || (Form->state & WCLICK_HILITE) || (Form->state!=Buffer->State)  ) {

		Hilight = Form->state & WCLICK_HILITE;

		if(Hilight) {
			Buffer->ImdRotation += (UWORD) ((BUTTONOBJ_ROTSPEED*frameTime2) / GAME_TICKS_PER_SEC);
		}

		Hilight = formIsHilite(Form);	// Hilited or flashing.

		Buffer->State = Form->state;

		Object = NULL;
		psObj = (BASE_OBJECT*)Buffer->Data;	// Get the object associated with this widget.

		if (psObj && psObj->died AND psObj->died != NOT_CURRENT_LIST)
		{
			// this may catch this horrible crash bug we've been having,
			// who knows?.... Shipping tomorrow, la de da :-)
			psObj = NULL;
			Buffer->Data = NULL;
			intRefreshScreen();
		}

		if(psObj) {
			switch (psObj->type) {
				case OBJ_DROID:						// If it's a droid...
					IMDType = IMDTYPE_DROID;
					Object = (void*)psObj;
					break;

				case OBJ_STRUCTURE:					// If it's a structure...
					IMDType = IMDTYPE_STRUCTURE;
//					Object = (void*)StructureGetIMD((STRUCTURE*)psObj);
					Object = (void*)psObj;
					break;

				default:
					ASSERT((FALSE, "intDisplayStatusButton: invalid structure type"));
			}
		}

		ButtonDrawXOffset = ButtonDrawYOffset = 0;


		if(Object) {
			RenderToButton(NULL,0,Object,selectedPlayer,Buffer,Down,IMDType,BTMBUTTON);	// ajl, changed from 0 to selectedPlayer
		} else {
			RenderBlankToButton(Buffer,Down,BTMBUTTON);
		}


		RENDERBUTTON_INITIALISED(Buffer);
	}

	RenderButton(psWidget,Buffer, xOffset+Form->x, yOffset+Form->y, BTMBUTTON,Down);




	CloseButtonRender();

	if (Hilight)
	{

		iV_DrawTransImage(IntImages,IMAGE_BUTB_HILITE,xOffset+Form->x,yOffset+Form->y);
	}
}


// Widget callback to display a rendered stats button, ie the job selection window buttons.
//
void intDisplayStatsButton(struct _widget *psWidget, UDWORD xOffset, UDWORD yOffset, UDWORD *pColours)
{
	W_CLICKFORM     *Form = (W_CLICKFORM*)psWidget;
	BASE_STATS      *Stat, *psResGraphic;
	BOOL            Down;
	SDWORD          Image, compID;
	BOOL            Hilight = FALSE;
	RENDERED_BUTTON *Buffer = (RENDERED_BUTTON*)Form->pUserData;
	UDWORD          IMDType = 0;
//	UDWORD          IMDIndex = 0;
	UDWORD          Player = selectedPlayer;		// ajl, changed for multiplayer (from 0)
	void            *Object;

	OpenButtonRender((UWORD)(xOffset+Form->x), (UWORD)(yOffset+Form->y),(UWORD)Form->width,(UWORD)Form->height);

	Down = Form->state & (WCLICK_DOWN | WCLICK_LOCKED | WCLICK_CLICKLOCK);


//	if( (pie_GetRenderEngine() == ENGINE_GLIDE) || (IsBufferInitialised(Buffer)==FALSE) || (Form->state & WCLICK_HILITE) || (Form->state!=Buffer->State) ) {
	if( pie_Hardware() || (IsBufferInitialised(Buffer)==FALSE) || (Form->state & WCLICK_HILITE) || (Form->state!=Buffer->State) ) {


		Hilight = Form->state & WCLICK_HILITE;

		if(Hilight) {
			Buffer->ImdRotation += (UWORD) ((BUTTONOBJ_ROTSPEED*frameTime2) / GAME_TICKS_PER_SEC);
		}

		Hilight = formIsHilite(Form);

		Buffer->State = Form->state;

		Object = NULL;
		Image = -1;

		Stat = (BASE_STATS*)Buffer->Data;

		ButtonDrawXOffset = ButtonDrawYOffset = 0;

		if(Stat)
		{
			if(StatIsStructure(Stat))
			{
//				IMDType = IMDTYPE_STRUCTURE;
//				Object = (void*)StatGetStructureIMD(Stat,selectedPlayer);
				Object = (void*)Stat;
				Player = selectedPlayer;
				IMDType = IMDTYPE_STRUCTURESTAT;
			}
			else if(StatIsTemplate(Stat))
			{
				IMDType = IMDTYPE_DROIDTEMPLATE;
				Object = (void*)Stat;
			}
			else
			{
				//if(StatIsComponent(Stat))
				//{
				//	IMDType = IMDTYPE_COMPONENT;
				//	Shape = StatGetComponentIMD(Stat);
				//}
				compID = StatIsComponent(Stat); // This failes for viper body.
				if (compID != COMP_UNKNOWN)
				{
					IMDType = IMDTYPE_COMPONENT;
					Object = (void*)Stat;	//StatGetComponentIMD(Stat, compID);
				}
				else if(StatIsResearch(Stat))
				{
					/*IMDType = IMDTYPE_RESEARCH;
					StatGetResearchImage(Stat,&Image,(iIMDShape**)&Object,TRUE);
					//if Object != NULL the there must be a IMD so set the object to
					//equal the Research stat
					if (Object != NULL)
					{
						Object = (void*)Stat;
					}*/
                    StatGetResearchImage(Stat,&Image,(iIMDShape**)&Object, &psResGraphic, TRUE);
                    if (psResGraphic)
                    {
                        //we have a Stat associated with this research topic
                        if  (StatIsStructure(psResGraphic))
                        {
                            //overwrite the Object pointer
                            Object = (void*)psResGraphic;
				            Player = selectedPlayer;
                            //this defines how the button is drawn
				            IMDType = IMDTYPE_STRUCTURESTAT;
                        }
                        else
                        {
            				compID = StatIsComponent(psResGraphic);
				            if (compID != COMP_UNKNOWN)
				            {
                                //this defines how the button is drawn
					            IMDType = IMDTYPE_COMPONENT;
                                //overwrite the Object pointer
					            Object = (void*)psResGraphic;
				            }
                            else
                            {
                                ASSERT((FALSE,
                                    "intDisplayStatsButton:Invalid Stat for research button"));
                                Object = NULL;
                                IMDType = IMDTYPE_RESEARCH;
                            }
                        }
                    }
                    else
                    {
                        //no Stat for this research topic so just use the graphic provided
                        //if Object != NULL the there must be a IMD so set the object to
                        //equal the Research stat
                        if (Object != NULL)
                        {
                            Object = (void*)Stat;
                            IMDType = IMDTYPE_RESEARCH;
                        }
                    }
				}
			}

			if(Down)
			{
				CurrentStatsTemplate = Stat;
//				CurrentStatsShape = Object;
//				CurrentStatsIndex = (SWORD)IMDIndex;
			}

		}
		else
		{
			IMDType = IMDTYPE_COMPONENT;
			//BLANK button for now - AB 9/1/98
			Object = NULL;
			CurrentStatsTemplate = NULL;
//			CurrentStatsShape = NULL;
//			CurrentStatsIndex = -1;
		}


		if(Object) {
			if(Image >= 0) {
				RenderToButton(IntImages,(UWORD)Image,Object,Player,Buffer,Down,IMDType,TOPBUTTON);
  			} else {
				RenderToButton(NULL,0,Object,Player,Buffer,Down,IMDType,TOPBUTTON);
			}
		} else if(Image >= 0) {
			RenderImageToButton(IntImages,(UWORD)Image,Buffer,Down,TOPBUTTON);
		} else {
			RenderBlankToButton(Buffer,Down,TOPBUTTON);
		}




		RENDERBUTTON_INITIALISED(Buffer);
	}

	// Draw the button.
	RenderButton(psWidget,Buffer, xOffset+Form->x, yOffset+Form->y, TOPBUTTON,Down);


	CloseButtonRender();

	if (Hilight)
	{

		iV_DrawTransImage(IntImages,IMAGE_BUT_HILITE,xOffset+Form->x,yOffset+Form->y);
	}
}




void RenderToButton(IMAGEFILE *ImageFile,UWORD ImageID,void *Object,UDWORD Player,
					RENDERED_BUTTON *Buffer,BOOL Down, UDWORD IMDType, UDWORD buttonType)
{
	CreateIMDButton(ImageFile,ImageID,Object,Player,Buffer,Down,IMDType,buttonType);
}

void RenderImageToButton(IMAGEFILE *ImageFile,UWORD ImageID,RENDERED_BUTTON *Buffer,BOOL Down, UDWORD buttonType)
{
	CreateImageButton(ImageFile,ImageID,Buffer,Down,buttonType);
}

void RenderBlankToButton(RENDERED_BUTTON *Buffer,BOOL Down, UDWORD buttonType)
{
	CreateBlankButton(Buffer,Down,buttonType);
}




void AdjustTabFormSize(W_TABFORM *Form,UDWORD *x0,UDWORD *y0,UDWORD *x1,UDWORD *y1)
{
	/* Adjust for where the tabs are */
	if(Form->majorPos == WFORM_TABLEFT) {
		*x0 += Form->tabMajorThickness - Form->tabHorzOffset;
	} else if(Form->minorPos == WFORM_TABLEFT) {
		*x0 += Form->tabMinorThickness - Form->tabHorzOffset;
	}
	if(Form->majorPos == WFORM_TABRIGHT) {
		*x1 -= Form->tabMajorThickness - Form->tabHorzOffset;
	} else if(Form->minorPos == WFORM_TABRIGHT) {
		*x1 -= Form->tabMinorThickness - Form->tabHorzOffset;
	}
	if(Form->majorPos == WFORM_TABTOP) {
		*y0 += Form->tabMajorThickness - Form->tabVertOffset;
	} else if(Form->minorPos == WFORM_TABTOP) {
		*y0 += Form->tabMinorThickness - Form->tabVertOffset;
	}
	if(Form->majorPos == WFORM_TABBOTTOM) {
		*y1 -= Form->tabMajorThickness - Form->tabVertOffset;
	} else if(Form->minorPos == WFORM_TABBOTTOM) {
		*y1 -= Form->tabMinorThickness - Form->tabVertOffset;
	}
}


void intDisplayObjectForm(struct _widget *psWidget, UDWORD xOffset, UDWORD yOffset, UDWORD *pColours)
{
//	W_TABFORM *Form = (W_TABFORM*)psWidget;
//	UDWORD x0,y0,x1,y1;
//
//	x0 = xOffset+Form->x;
//	y0 = yOffset+Form->y;
//	x1 = x0 + Form->width;
//	y1 = y0 + Form->height;
//
//	AdjustTabFormSize(Form,&x0,&y0,&x1,&y1);
//
//	RenderWindowFrame(&FrameObject,x0,y0,x1-x0,y1-y0);
}



// Widget callback function to do the open form animation. Doesn't just open Plain Forms!!
//
void intOpenPlainForm(struct _widget *psWidget, UDWORD xOffset, UDWORD yOffset, UDWORD *pColours)
{
	W_TABFORM	*Form = (W_TABFORM*)psWidget;
	UDWORD		Tx0,Ty0,Tx1,Ty1;
	UDWORD		Range;
	UDWORD		Duration;
	UDWORD		APos;
	SDWORD		Ay0,Ay1;

	Tx0 = xOffset+Form->x;
	Ty0 = yOffset+Form->y;
	Tx1 = Tx0 + Form->width;
	Ty1 = Ty0 + Form->height;

	if(Form->animCount == 0) {
		if( (FormOpenAudioID >= 0) && (FormOpenCount == 0) ) {
			audio_PlayTrack(FormOpenAudioID);
			FormOpenCount++;
		}
		Form->Ax0 = (UWORD)Tx0;
		Form->Ax1 = (UWORD)Tx1;
		Form->Ay0 = (UWORD)(Ty0 + (Form->height/2) - 4);
		Form->Ay1 = (UWORD)(Ty0 + (Form->height/2) + 4);
		Form->startTime = gameTime2;
	} else {
		FormOpenCount = 0;
	}

	RenderWindowFrame(&FrameNormal,Form->Ax0,Form->Ay0,Form->Ax1-Form->Ax0,Form->Ay1-Form->Ay0);

	Form->animCount++;

	Range = (Form->height/2)-4;
	Duration = (gameTime2 - Form->startTime) << 16 ;
	APos = (Range * (Duration / FORM_OPEN_ANIM_DURATION) ) >> 16;

	Ay0 = Ty0 + (Form->height/2) - 4 - APos;
	Ay1 = Ty0 + (Form->height/2) + 4 + APos;

	if(Ay0 <= (SDWORD)Ty0)
	{
		Ay0 = Ty0;
	}

	if(Ay1 >= (SDWORD)Ty1)
	{
		Ay1 = Ty1;
	}
	Form->Ay0 = (UWORD)Ay0;
	Form->Ay1 = (UWORD)Ay1;

	if((Form->Ay0 == Ty0) && (Form->Ay1 == Ty1))
	{
		if (Form->pUserData != NULL)
		{
			Form->display = (WIDGET_DISPLAY)((SDWORD)Form->pUserData);
		}
		else
		{
			//default to display
			Form->display = intDisplayPlainForm;
		}
		Form->disableChildren = FALSE;
		Form->animCount = 0;
	}
}


// Widget callback function to do the close form animation.
//
void intClosePlainForm(struct _widget *psWidget, UDWORD xOffset, UDWORD yOffset, UDWORD *pColours)
{
	W_TABFORM *Form = (W_TABFORM*)psWidget;
	UDWORD Tx0,Ty0,Tx1,Ty1;
	UDWORD Range;
	UDWORD Duration;
	UDWORD APos;

	Tx0 = xOffset+Form->x;
	Tx1 = Tx0 + Form->width;
	Ty0 = yOffset+Form->y + (Form->height/2) - 4;
	Ty1 = yOffset+Form->y + (Form->height/2) + 4;

	if(Form->animCount == 0) {
		if( (FormCloseAudioID >= 0) && (FormCloseCount == 0) ){
			audio_PlayTrack(FormCloseAudioID);
			FormCloseCount++;
		}
		Form->Ax0 = (UWORD)(xOffset+Form->x);
		Form->Ay0 = (UWORD)(yOffset+Form->y);
		Form->Ax1 = (UWORD)(Form->Ax0 + Form->width);
		Form->Ay1 = (UWORD)(Form->Ay0 + Form->height);
		Form->startTime = gameTime2;
	} else {
		FormCloseCount = 0;
	}

	RenderWindowFrame(&FrameNormal,Form->Ax0,Form->Ay0,Form->Ax1-Form->Ax0,Form->Ay1-Form->Ay0);

	Form->animCount++;

	Range = (Form->height/2)-4;
	Duration = (gameTime2 - Form->startTime) << 16 ;
	APos = (Range * (Duration / FORM_OPEN_ANIM_DURATION) ) >> 16;

	Form->Ay0 = (UWORD)(yOffset + Form->y + APos);
	Form->Ay1 = (UWORD)(yOffset + Form->y + Form->height - APos);

	if(Form->Ay0 >= Ty0) {
		Form->Ay0 = (UWORD)Ty0;
	}
	if(Form->Ay1 <= Ty1) {
		Form->Ay1 = (UWORD)Ty1;
	}

	if((Form->Ay0 == Ty0) && (Form->Ay1 == Ty1)) {
		Form->pUserData = (void*)1;
		Form->animCount = 0;
	}
}



void intDisplayPlainForm(struct _widget *psWidget, UDWORD xOffset, UDWORD yOffset, UDWORD *pColours)
{
	W_TABFORM *Form = (W_TABFORM*)psWidget;
	UDWORD x0,y0,x1,y1;

	x0 = xOffset+Form->x;
	y0 = yOffset+Form->y;
	x1 = x0 + Form->width;
	y1 = y0 + Form->height;

	RenderWindowFrame(&FrameNormal,x0,y0,x1-x0,y1-y0);
}


void intDisplayStatsForm(struct _widget *psWidget, UDWORD xOffset, UDWORD yOffset, UDWORD *pColours)
{
	W_TABFORM *Form = (W_TABFORM*)psWidget;
	UDWORD x0,y0,x1,y1;

	x0 = xOffset+Form->x;
	y0 = yOffset+Form->y;
	x1 = x0 + Form->width;
	y1 = y0 + Form->height;

	AdjustTabFormSize(Form,&x0,&y0,&x1,&y1);

	RenderWindowFrame(&FrameNormal,x0,y0,x1-x0,y1-y0);
}


// Display an image for a widget.
//
void intDisplayImage(struct _widget *psWidget, UDWORD xOffset, UDWORD yOffset, UDWORD *pColours)
{
	UDWORD x = xOffset+psWidget->x;
	UDWORD y = yOffset+psWidget->y;

	iV_DrawTransImage(IntImages,(UWORD)(UDWORD)psWidget->pUserData,x,y);
}


//draws the mission clock - flashes when below a predefined time
void intDisplayMissionClock(struct _widget *psWidget, UDWORD xOffset, UDWORD yOffset, UDWORD *pColours)
{
	UDWORD  x = xOffset+psWidget->x;
	UDWORD  y = yOffset+psWidget->y;
    UDWORD  flash;

    //draw the background image
    iV_DrawTransImage(IntImages,(UWORD)UNPACKDWORD_TRI_B((UDWORD)psWidget->pUserData),x,y);
	//need to flash the timer when < 5 minutes remaining, but > 4 minutes
    flash = UNPACKDWORD_TRI_A((UDWORD)psWidget->pUserData);
	if (flash AND ((gameTime2/250) % 2) == 0)
    {
    	iV_DrawTransImage(IntImages,(UWORD)UNPACKDWORD_TRI_C((UDWORD)psWidget->pUserData),x,y);
	}
}



// Display one of two images depending on if the widget is hilighted by the mouse.
//
void intDisplayImageHilight(struct _widget *psWidget, UDWORD xOffset, UDWORD yOffset, UDWORD *pColours)
{
	UDWORD x = xOffset+psWidget->x;
	UDWORD y = yOffset+psWidget->y, flash;
	UWORD ImageID;
	BOOL Hilight = FALSE;

	switch(psWidget->type) {
		case WIDG_FORM:
			Hilight = formIsHilite(psWidget);
//			if( ((W_CLICKFORM*)psWidget)->state & WCLICK_HILITE) ||  {
//				Hilight = TRUE;
//			}
			break;

		case WIDG_BUTTON:
			Hilight = buttonIsHilite(psWidget);
//			if( ((W_BUTTON*)psWidget)->state & WBUTS_HILITE) {
//				Hilight = TRUE;
//			}
			break;

		case WIDG_EDITBOX:
			if( ((W_EDITBOX*)psWidget)->state & WEDBS_HILITE) {
				Hilight = TRUE;
			}
			break;

		case WIDG_SLIDER:
			if( ((W_SLIDER*)psWidget)->state & SLD_HILITE) {
				Hilight = TRUE;
			}
			break;

		default:
			Hilight = FALSE;
	}

	ImageID = (UWORD)UNPACKDWORD_TRI_C((UDWORD)psWidget->pUserData);


	//need to flash the button if Full Transporter
    flash = UNPACKDWORD_TRI_A((UDWORD)psWidget->pUserData);
	if (flash AND psWidget->id == IDTRANS_LAUNCH)
	{
		if (((gameTime2/250) % 2) == 0)
		{
    		iV_DrawTransImage(IntImages,(UWORD)UNPACKDWORD_TRI_B((UDWORD)psWidget->pUserData),x,y);
		}
		else
		{
        	iV_DrawTransImage(IntImages,ImageID,x,y);
		}
	}
    else
    {
       	iV_DrawTransImage(IntImages,ImageID,x,y);
        if(Hilight)
        {
	    	iV_DrawTransImage(IntImages,(UWORD)UNPACKDWORD_TRI_B((UDWORD)psWidget->pUserData),x,y);
	    }
    }

}


void GetButtonState(struct _widget *psWidget,BOOL *Hilight,UDWORD *Down,BOOL *Grey)
{
	switch(psWidget->type) {
		case WIDG_FORM:
			*Hilight = formIsHilite(psWidget);
//			if( ((W_CLICKFORM*)psWidget)->state & WCLICK_HILITE) {
//				Hilight = TRUE;
//			}
			if( ((W_CLICKFORM*)psWidget)->state & (WCLICK_DOWN | WCLICK_LOCKED | WCLICK_CLICKLOCK)) {
				*Down = 1;
			}
			if( ((W_CLICKFORM*)psWidget)->state & WCLICK_GREY) {
				*Grey = TRUE;
			}
			break;

		case WIDG_BUTTON:
			*Hilight = buttonIsHilite(psWidget);
//			if( ((W_BUTTON*)psWidget)->state & WBUTS_HILITE) {
//				*Hilight = TRUE;
//			}
			if( ((W_BUTTON*)psWidget)->state & (WBUTS_DOWN | WBUTS_LOCKED | WBUTS_CLICKLOCK)) {
				*Down = 1;
			}
			if( ((W_BUTTON*)psWidget)->state & WBUTS_GREY) {
				*Grey = TRUE;
			}
			break;

		case WIDG_EDITBOX:
			if( ((W_EDITBOX*)psWidget)->state & WEDBS_HILITE) {
				*Hilight = TRUE;
			}
			break;

		case WIDG_SLIDER:
			if( ((W_SLIDER*)psWidget)->state & SLD_HILITE) {
				*Hilight = TRUE;
			}
			if( ((W_SLIDER*)psWidget)->state & (WCLICK_DOWN | WCLICK_LOCKED | WCLICK_CLICKLOCK)) {
				*Down = 1;
			}
			break;

		default:
			*Hilight = FALSE;
	}
}


// Display one of two images depending on if the widget is hilighted by the mouse.
//
void intDisplayButtonHilight(struct _widget *psWidget, UDWORD xOffset, UDWORD yOffset, UDWORD *pColours)
{
	UDWORD x = xOffset+psWidget->x;
	UDWORD y = yOffset+psWidget->y;
	BOOL Hilight = FALSE;
	BOOL Grey = FALSE;
	UDWORD Down = 0;
	UWORD ImageID;

	GetButtonState(psWidget,&Hilight,&Down,&Grey);

//	switch(psWidget->type) {
//		case WIDG_FORM:
//			if( ((W_CLICKFORM*)psWidget)->state & WCLICK_HILITE) {
//				Hilight = TRUE;
//			}
//			if( ((W_CLICKFORM*)psWidget)->state & (WCLICK_DOWN | WCLICK_LOCKED | WCLICK_CLICKLOCK)) {
//				Down = 1;
//			}
//			if( ((W_CLICKFORM*)psWidget)->state & WCLICK_GREY) {
//				Grey = 1;
//			}
//			break;
//
//		case WIDG_BUTTON:
//			if( ((W_BUTTON*)psWidget)->state & WBUTS_HILITE) {
//				Hilight = TRUE;
//			}
//			if( ((W_BUTTON*)psWidget)->state & (WBUTS_DOWN | WBUTS_LOCKED | WBUTS_CLICKLOCK)) {
//				Down = 1;
//			}
//			if( ((W_BUTTON*)psWidget)->state & WBUTS_GREY) {
//				Grey = 1;
//			}
//			break;
//
//		case WIDG_EDITBOX:
//			if( ((W_EDITBOX*)psWidget)->state & WEDBS_HILITE) {
//				Hilight = TRUE;
//			}
//			break;
//
//		case WIDG_SLIDER:
//			if( ((W_SLIDER*)psWidget)->state & SLD_HILITE) {
//				Hilight = TRUE;
//			}
//			if( ((W_SLIDER*)psWidget)->state & (WCLICK_DOWN | WCLICK_LOCKED | WCLICK_CLICKLOCK)) {
//				Down = 1;
//			}
//			break;
//
//		default:
//			Hilight = FALSE;
//	}


	if(Grey) {
		ImageID = (UWORD)(UNPACKDWORD_TRI_A((UDWORD)psWidget->pUserData));
		Hilight = FALSE;
	} else {
		ImageID = (UWORD)(UNPACKDWORD_TRI_C((UDWORD)psWidget->pUserData) + Down);
	}

	iV_DrawTransImage(IntImages,ImageID,x,y);
	if(Hilight) {
		iV_DrawTransImage(IntImages,(UWORD)UNPACKDWORD_TRI_B((UDWORD)psWidget->pUserData),x,y);
	}

}


// Display one of two images depending on if the widget is hilighted by the mouse.
//
void intDisplayAltButtonHilight(struct _widget *psWidget, UDWORD xOffset, UDWORD yOffset, UDWORD *pColours)
{
	UDWORD x = xOffset+psWidget->x;
	UDWORD y = yOffset+psWidget->y;
	BOOL Hilight = FALSE;
	BOOL Grey = FALSE;
	UDWORD Down = 0;
	UWORD ImageID;

	GetButtonState(psWidget,&Hilight,&Down,&Grey);


	if(Grey) {
		ImageID = (UWORD)(UNPACKDWORD_TRI_A((UDWORD)psWidget->pUserData));
		Hilight = FALSE;
	} else {
		ImageID = (UWORD)(UNPACKDWORD_TRI_C((UDWORD)psWidget->pUserData) + Down);
	}

	iV_DrawTransImage(IntImages,ImageID,x,y);
	if(Hilight) {
		iV_DrawTransImage(IntImages,(UWORD)UNPACKDWORD_TRI_B((UDWORD)psWidget->pUserData),x,y);
	}

}


// Flash one of two images depending on if the widget is hilighted by the mouse.
//
void intDisplayButtonFlash(struct _widget *psWidget, UDWORD xOffset, UDWORD yOffset, UDWORD *pColours)
{
	UDWORD x = xOffset+psWidget->x;
	UDWORD y = yOffset+psWidget->y;
	BOOL Hilight = FALSE;
	UDWORD Down = 0;
	UWORD ImageID;

	ASSERT((psWidget->type == WIDG_BUTTON,"intDisplayButtonFlash : Not a button"));

	if( ((W_BUTTON*)psWidget)->state & WBUTS_HILITE)
	{
		Hilight = TRUE;
	}

	if( ((W_BUTTON*)psWidget)->state & (WBUTS_DOWN | WBUTS_LOCKED | WBUTS_CLICKLOCK))
	{
		Down = 1;
	}


	if ( Down && ((gameTime2/250) % 2 == 0) )
	{
		ImageID = (UWORD)(UNPACKDWORD_TRI_B((UDWORD)psWidget->pUserData));
		Hilight = FALSE;
	} else
	{
		ImageID = (UWORD)(UNPACKDWORD_TRI_C((UDWORD)psWidget->pUserData));
	}

	iV_DrawTransImage(IntImages,ImageID,x,y);



}

void intDisplayReticuleButton(struct _widget *psWidget, UDWORD xOffset, UDWORD yOffset, UDWORD *pColours)
{
	UDWORD	x = xOffset+psWidget->x;
	UDWORD	y = yOffset+psWidget->y;
	BOOL	Hilight = FALSE;
	BOOL	Down = FALSE;
	UBYTE	DownTime = (UBYTE)UNPACKDWORD_QUAD_C((UDWORD)psWidget->pUserData);
	UBYTE	Index = (UBYTE)UNPACKDWORD_QUAD_D((UDWORD)psWidget->pUserData);
	UBYTE	flashing = (UBYTE)UNPACKDWORD_QUAD_A((UDWORD)psWidget->pUserData);
	UBYTE	flashTime = (UBYTE)UNPACKDWORD_QUAD_B((UDWORD)psWidget->pUserData);
	UWORD	ImageID;

	ASSERT((psWidget->type == WIDG_BUTTON,"intDisplayReticuleButton : Not a button"));


//	iV_DrawTransImage(IntImages,ImageID,x,y);
	if(((W_BUTTON*)psWidget)->state & WBUTS_GREY) {
		iV_DrawTransImage(IntImages,IMAGE_RETICULE_GREY,x,y);
		return;
	}


	Down = ((W_BUTTON*)psWidget)->state & (WBUTS_DOWN | WBUTS_CLICKLOCK);
//	Hilight = ((W_BUTTON*)psWidget)->state & WBUTS_HILITE;
	Hilight = buttonIsHilite(psWidget);

	if(Down)
	{
		if((DownTime < 1) && (Index != IMAGE_CANCEL_UP))
		{
			ImageID = IMAGE_RETICULE_BUTDOWN;	// Do the button flash.
		}
		else
		{
			ImageID = (UWORD)(Index+1);					// It's down.
		}
		DownTime++;
		//stop the reticule from flashing if it was
		flashing = (UBYTE)FALSE;
	}
	else
	{
		//flashing button?
		if (flashing)
		{
//			if (flashTime < 2)
			if (((gameTime/250) % 2) == 0)
			{
				ImageID = (UWORD)(Index);//IMAGE_RETICULE_BUTDOWN;//a step in the right direction JPS 27-4-98
			}
			else
			{
				ImageID = (UWORD)(Index+1);
				flashTime = 0;
			}
			flashTime++;
		}
		else
		{
			DownTime = 0;
			ImageID = Index;						// It's up.
		}
	}



	iV_DrawTransImage(IntImages,ImageID,x,y);

	if(Hilight)
	{
		if (Index == IMAGE_CANCEL_UP)
		{
			iV_DrawTransImage(IntImages,IMAGE_CANCEL_HILIGHT,x,y);
		}
		else
		{
			iV_DrawTransImage(IntImages,IMAGE_RETICULE_HILIGHT,x,y);
		}
	}


	psWidget->pUserData = (void*)(PACKDWORD_QUAD(flashTime,flashing,DownTime,Index));
}


void intDisplayTab(struct _widget *psWidget,UDWORD TabType, UDWORD Position,
				   UDWORD Number,BOOL Selected,BOOL Hilight,UDWORD x,UDWORD y,UDWORD Width,UDWORD Height)
{
	TABDEF *Tab = (TABDEF*)psWidget->pUserData;

//	ASSERT((Number < 4,"intDisplayTab : Too many tabs."));
    //Number represents which tab we are on but not interested since they all look the same now - AB 25/01/99
	/*if(Number > 3) {
		Number = 3;
	}*/


	if(TabType == TAB_MAJOR) {
		//iV_DrawTransImage(IntImages,(UWORD)(Tab->MajorUp+Number),x,y);
        iV_DrawTransImage(IntImages,(UWORD)Tab->MajorUp,x,y);

		if(Hilight) {
			iV_DrawTransImage(IntImages,(UWORD)Tab->MajorHilight,x,y);
		} else if(Selected) {
			iV_DrawTransImage(IntImages,(UWORD)Tab->MajorSelected,x,y);
		}
	} else {
		//iV_DrawTransImage(IntImages,(UWORD)(Tab->MinorUp+Number),x,y);
        iV_DrawTransImage(IntImages,(UWORD)(Tab->MinorUp),x,y);

		if(Hilight) {
			iV_DrawTransImage(IntImages,Tab->MinorHilight,x,y);
		} else if(Selected) {
			iV_DrawTransImage(IntImages,Tab->MinorSelected,x,y);
		}
	}

}

//void intDisplaySystemTab(struct _widget *psWidget,UDWORD TabType, UDWORD Position,
//				   UDWORD Number,BOOL Selected,BOOL Hilight,UDWORD x,UDWORD y,UDWORD Width,UDWORD Height)
//{
//	TABDEF *Tab = (TABDEF*)psWidget->pUserData;
//#ifdef PSX
//	UWORD ImageID;
//#endif
//
////	ASSERT((Number < 4,"intDisplaySystemTab : Too many tabs."));
//
//	Number = Number%4;	// Make sure number never gets bigger than 3.
//
//#ifndef PSX
//	if(TabType == TAB_MAJOR)
//	{
//		iV_DrawTransImage(IntImages,(UWORD)(Tab->MajorUp+Number),x,y);
//
//		if(Hilight)
//		{
//			iV_DrawTransImage(IntImages,Tab->MajorHilight,x,y);
//		}
//		else if(Selected)
//		{
//			iV_DrawTransImage(IntImages,(UWORD)(Tab->MajorSelected+Number),x,y);
//		}
//	}
//	else
//	{
//		//ASSERT((FALSE,"intDisplaySystemTab : NOT CATERED FOR!!!"));
//		iV_DrawTransImage(IntImages,(UWORD)(Tab->MinorUp),x,y);
//
//		if(Hilight)
//		{
//			iV_DrawTransImage(IntImages,Tab->MinorHilight,x,y);
//		}
//		else if(Selected)
//		{
//			iV_DrawTransImage(IntImages,Tab->MinorSelected,x,y);
//		}
//	}
//#else
//	if(TabType == TAB_MAJOR)
//	{
//		if(Hilight)
//		{
//			iV_DrawTransImage(IntImages,Tab->MajorHilight,x,y);
//		}
//		else if(Selected)
//		{
//			iV_DrawTransImage(IntImages,(UWORD)(Tab->MajorSelected+Number),x,y);
//		}
//
//		ImageID = (UWORD)(Tab->MajorUp+Number);
//		iV_DrawTransImage(IntImages,ImageID,x,y);
//	}
//	else
//	{
//		if(Hilight)
//		{
//			iV_DrawTransImage(IntImages,Tab->MinorHilight,x,y);
//		}
//		else if(Selected)
//		{
//			iV_DrawTransImage(IntImages,Tab->MinorSelected,x,y);
//		}
//
//		ImageID = (UWORD)(Tab->MinorUp);
//		iV_DrawTransImage(IntImages,ImageID,x,y);
//	}
//
//	AddCursorSnap(&InterfaceSnap,
//					x+(iV_GetImageXOffset(IntImages,ImageID))+iV_GetImageWidth(IntImages,ImageID)/2,
//					y+(iV_GetImageYOffset(IntImages,ImageID))+iV_GetImageHeight(IntImages,ImageID)/2,
//					psWidget->formID,psWidget->id,NULL);
//#endif
//}

//static void intUpdateSliderCount(struct _widget *psWidget, struct _w_context *psContext)
//{
//	W_SLIDER *Slider = (W_SLIDER*)psWidget;
//	UDWORD Quantity = Slider->pos + 1;
//
//	W_LABEL *Label = (W_LABEL*)widgGetFromID(psWScreen,IDSTAT_SLIDERCOUNT);
//	Label->pUserData = (void*)Quantity;
//}

// Display one of three images depending on if the widget is currently depressed (ah!).
//
void intDisplayButtonPressed(struct _widget *psWidget, UDWORD xOffset,
							 UDWORD yOffset, UDWORD *pColours)
{
	W_BUTTON	*psButton = (W_BUTTON*)psWidget;
	UDWORD		x = xOffset+psButton->x;
	UDWORD		y = yOffset+psButton->y;
	UBYTE		Hilight = 0;
	UWORD		ImageID;

	if (psButton->state & (WBUTS_DOWN | WBUTS_LOCKED | WBUTS_CLICKLOCK))
	{
		ImageID = (UWORD)(UNPACKDWORD_TRI_A((UDWORD)psWidget->pUserData));
	}
	else
	{
		ImageID = (UWORD)(UNPACKDWORD_TRI_C((UDWORD)psWidget->pUserData));
	}

	Hilight = (UBYTE)buttonIsHilite(psButton);
//	if (psButton->state & WBUTS_HILITE)
//	{
//		Hilight = 1;
//	}



	iV_DrawTransImage(IntImages,ImageID,x,y);
	if (Hilight)
	{
		iV_DrawTransImage(IntImages,(UWORD)UNPACKDWORD_TRI_B((UDWORD)psWidget->
			pUserData),x,y);
	}


}

// Display DP images depending on factory and if the widget is currently depressed
void intDisplayDPButton(struct _widget *psWidget, UDWORD xOffset,
						UDWORD yOffset, UDWORD *pColours)
{
	W_BUTTON	*psButton = (W_BUTTON*)psWidget;
	STRUCTURE	*psStruct;
	UDWORD		x = xOffset+psButton->x;
	UDWORD		y = yOffset+psButton->y;
	UBYTE		hilight = 0, down = 0;
	UWORD		imageID;

	psStruct = psButton->pUserData;
	if (psStruct)
	{
		ASSERT((StructIsFactory(psStruct),
			"intDisplayDPButton: structure is not a factory"));

		if (psButton->state & (WBUTS_DOWN | WBUTS_LOCKED | WBUTS_CLICKLOCK))
		{
			down = TRUE;
		}

		hilight = (UBYTE)buttonIsHilite(psButton);
//		if (psButton->state & WBUTS_HILITE)
//		{
//			hilight = TRUE;
//		}

		switch(psStruct->pStructureType->type)
		{
		case REF_FACTORY:
			imageID = IMAGE_FDP_UP;
			break;
		case REF_CYBORG_FACTORY:
			imageID = IMAGE_CDP_UP;
			break;
		case REF_VTOL_FACTORY:
			imageID = IMAGE_VDP_UP;
			break;
		default:
			return;
		}


		iV_DrawTransImage(IntImages,imageID,x,y);
		if (hilight)
		{
            imageID++;
			iV_DrawTransImage(IntImages,(UWORD)imageID,x,y);
		}
		else if (down)
		{
            imageID--;
			iV_DrawTransImage(IntImages,(UWORD)imageID,x,y);
		}

	}
}


void intDisplaySlider(struct _widget *psWidget, UDWORD xOffset, UDWORD yOffset, UDWORD *pColours)
{
	W_SLIDER *Slider = (W_SLIDER*)psWidget;
	UDWORD x = xOffset+psWidget->x;
	UDWORD y = yOffset+psWidget->y;
	SWORD sx;
	//SWORD x0,y0, x1;

	iV_DrawTransImage(IntImages,IMAGE_SLIDER_BACK,x+STAT_SLD_OX,y+STAT_SLD_OY);

/*	x0 = (SWORD)(Slider->x + xOffset + Slider->barSize/2);
	y0 = (SWORD)(Slider->y + yOffset + Slider->height/2);
	x1 = (SWORD)(x0 + Slider->width - Slider->barSize);
	screenSetLineCacheColour(*(pColours + WCOL_DARK));
	screenDrawLine(x0,y0, x1,y0);
*/


//#ifndef PSX
	sx = (SWORD)((Slider->width - Slider->barSize)
	 			 * Slider->pos / Slider->numStops);
//#else
//	iV_SetOTIndex_PSX(iV_GetOTIndex_PSX()-1);
//	sx = (SWORD)((Slider->width-12 - Slider->barSize)
//	 			 * Slider->pos / Slider->numStops)+4;
//#endif

	iV_DrawTransImage(IntImages,IMAGE_SLIDER_BUT,x+sx,y-2);

//#ifdef PSX
//	AddCursorSnap(&InterfaceSnap,
//					x+iV_GetImageCenterX(IntImages,IMAGE_SLIDER_BACK),
//					y+iV_GetImageCenterY(IntImages,IMAGE_SLIDER_BACK),
//					psWidget->formID,psWidget->id,NULL);
//#endif
	//DisplayQuantity = Slider->pos + 1;
}


/* display highlighted edit box from left, middle and end edit box graphics */
void intDisplayEditBox(struct _widget *psWidget, UDWORD xOffset, UDWORD yOffset, UDWORD *pColours)
{

	W_EDITBOX	*psEditBox = (W_EDITBOX *) psWidget;
	UWORD		iImageIDLeft, iImageIDMid, iImageIDRight;
	UDWORD		iX, iY, iDX, iXRight;
	UDWORD		iXLeft = xOffset + psWidget->x,
				iYLeft = yOffset + psWidget->y;

	if ( psEditBox->state & WEDBS_HILITE )
	{
		iImageIDLeft  = IMAGE_DES_EDITBOXLEFTH;
		iImageIDMid   = IMAGE_DES_EDITBOXMIDH;
		iImageIDRight = IMAGE_DES_EDITBOXRIGHTH;
	}
	else
	{
		iImageIDLeft  = IMAGE_DES_EDITBOXLEFT;
		iImageIDMid   = IMAGE_DES_EDITBOXMID;
		iImageIDRight = IMAGE_DES_EDITBOXRIGHT;
	}

	/* draw left side of bar */
	iX = iXLeft;
	iY = iYLeft;
	iV_DrawTransImage( IntImages, iImageIDLeft, iX, iY );

	/* draw middle of bar */
	iX += iV_GetImageWidth( IntImages, iImageIDLeft );
	iDX = iV_GetImageWidth( IntImages, iImageIDMid );
	iXRight = xOffset + psWidget->width - iV_GetImageWidth( IntImages, iImageIDRight );
	while ( iX < iXRight )
	{
		iV_DrawTransImage( IntImages, iImageIDMid, iX, iY );
		iX += iDX;
	}

	/* draw right side of bar */
	iV_DrawTransImage( IntImages, iImageIDRight, iXRight, iY );

}





void intDisplayNumber(struct _widget *psWidget, UDWORD xOffset, UDWORD yOffset, UDWORD *pColours)
{
	W_LABEL		*Label = (W_LABEL*)psWidget;
	UDWORD		i = 0;
	UDWORD		x = Label->x + xOffset;
	UDWORD		y = Label->y + yOffset;
	UDWORD		Quantity;// = (UDWORD)Label->pUserData;
	STRUCTURE	*psStruct;
	FACTORY		*psFactory;

	//Quantity depends on the factory
	Quantity = 1;
	if (Label->pUserData != NULL)
	{
		psStruct = (STRUCTURE *)Label->pUserData;
		psFactory = (FACTORY *) psStruct->pFunctionality;
		//psFactory = (FACTORY *)((STRUCTURE *)Label->pUserData)->pFunctionality;
		//if (psFactory->psSubject)
		{
			Quantity = psFactory->quantity;
		}
		/*else
		{
			Quantity = 1;
		}*/
	}

		if(Quantity >= STAT_SLDSTOPS)
		{
			iV_DrawTransImage(IntImages,IMAGE_SLIDER_INFINITY,x+4,y);
		}
		else
		{
			Label->aText[0] = (UBYTE)('0' + Quantity / 10);
			Label->aText[1] = (UBYTE)('0' + Quantity % 10);
			Label->aText[2] = 0;

			while(Label->aText[i]) {

				iV_DrawTransImage(IntImages,(UWORD)(IMAGE_0 + (Label->aText[i]-'0')),x,y);
				x += iV_GetImageWidth(IntImages,(UWORD)(IMAGE_0 + (Label->aText[i]-'0')))+1;

				i++;
		}
	}
}


// Initialise all the surfaces,graphics etc. used by the interface.
//
void intInitialiseGraphics(void)
{
	// Initialise any bitmaps used by the interface.
	imageInitBitmaps();

	// Initialise button surfaces.
	InitialiseButtonData();
}


// Free up all surfaces,graphics etc. used by the interface.
//
void intDeleteGraphics(void)
{
	DeleteButtonData();
	imageDeleteBitmaps();
}

//#ifdef PSX
//// This sets up a test button for rendering on the playstation
//void InitialiseTestButton(UDWORD Width,UDWORD Height)
//{
//	TestButtonBuffer.InUse=FALSE;
//  	TestButtonBuffer.Surface = iV_SurfaceCreate(REND_SURFACE_USR,Width,Height,0,0,NULL);	// This allocates the surface in psx VRAM
//	ASSERT((TestButtonBuffer.Surface!=NULL,"intInitialise : Failed to create TestButton surface"));
//}
//
//#endif







// Initialise data for interface buttons.
//
void InitialiseButtonData(void)
{
	// Allocate surfaces for rendered buttons.
	UDWORD Width = (iV_GetImageWidth(IntImages,IMAGE_BUT0_UP)+3) & 0xfffffffc;	// Ensure width is whole number of dwords.
	UDWORD Height = iV_GetImageHeight(IntImages,IMAGE_BUT0_UP);
	UDWORD WidthTopic = (iV_GetImageWidth(IntImages,IMAGE_BUTB0_UP)+3) & 0xfffffffc;	// Ensure width is whole number of dwords.
	UDWORD HeightTopic = iV_GetImageHeight(IntImages,IMAGE_BUTB0_UP);

	UDWORD i;

	for(i=0; i<NUM_OBJECTSURFACES; i++) {
		ObjectSurfaces[i].Buffer = MALLOC(Width*Height);
		ASSERT((ObjectSurfaces[i].Buffer!=NULL,"intInitialise : Failed to allocate Object surface"));
		ObjectSurfaces[i].Surface = iV_SurfaceCreate(REND_SURFACE_USR,Width,Height,10,10,ObjectSurfaces[i].Buffer);
		ASSERT((ObjectSurfaces[i].Surface!=NULL,"intInitialise : Failed to create Object surface"));
	}

	for(i=0; i<NUM_OBJECTBUFFERS; i++) {
		RENDERBUTTON_NOTINUSE(&ObjectBuffers[i]);
		ObjectBuffers[i].ButSurf = &ObjectSurfaces[i%NUM_OBJECTSURFACES];
	}

	for(i=0; i<NUM_SYSTEM0SURFACES; i++) {
		System0Surfaces[i].Buffer = MALLOC(Width*Height);
		ASSERT((System0Surfaces[i].Buffer!=NULL,"intInitialise : Failed to allocate System0 surface"));
		System0Surfaces[i].Surface = iV_SurfaceCreate(REND_SURFACE_USR,Width,Height,10,10,System0Surfaces[i].Buffer);
		ASSERT((System0Surfaces[i].Surface!=NULL,"intInitialise : Failed to create System0 surface"));
	}

    for(i=0; i<NUM_SYSTEM0BUFFERS; i++) {
		RENDERBUTTON_NOTINUSE(&System0Buffers[i]);
		System0Buffers[i].ButSurf = &System0Surfaces[i%NUM_SYSTEM0SURFACES];
	}

	for(i=0; i<NUM_TOPICSURFACES; i++) {
		TopicSurfaces[i].Buffer = MALLOC(WidthTopic*HeightTopic);
		ASSERT((TopicSurfaces[i].Buffer!=NULL,"intInitialise : Failed to allocate Topic surface"));
		TopicSurfaces[i].Surface = iV_SurfaceCreate(REND_SURFACE_USR,WidthTopic,HeightTopic,10,10,TopicSurfaces[i].Buffer);
		ASSERT((TopicSurfaces[i].Surface!=NULL,"intInitialise : Failed to create Topic surface"));
	}

	for(i=0; i<NUM_TOPICBUFFERS; i++) {
		RENDERBUTTON_NOTINUSE(&TopicBuffers[i]);
		TopicBuffers[i].ButSurf = &TopicSurfaces[i%NUM_TOPICSURFACES];
	}

	for(i=0; i<NUM_STATSURFACES; i++) {
		StatSurfaces[i].Buffer = MALLOC(Width*Height);
		ASSERT((StatSurfaces[i].Buffer!=NULL,"intInitialise : Failed to allocate Stats surface"));
		StatSurfaces[i].Surface = iV_SurfaceCreate(REND_SURFACE_USR,Width,Height,10,10,StatSurfaces[i].Buffer);
		ASSERT((StatSurfaces[i].Surface!=NULL,"intInitialise : Failed to create Stat surface"));
	}

	for(i=0; i<NUM_STATBUFFERS; i++) {
		RENDERBUTTON_NOTINUSE(&StatBuffers[i]);
		StatBuffers[i].ButSurf = &StatSurfaces[i%NUM_STATSURFACES];
	}
}




void RefreshObjectButtons(void)
{
	UDWORD i;

	for(i=0; i<NUM_OBJECTBUFFERS; i++) {
		RENDERBUTTON_NOTINITIALISED(&ObjectBuffers[i]);
	}
}

void RefreshSystem0Buttons(void)
{
	UDWORD i;

	for(i=0; i<NUM_SYSTEM0BUFFERS; i++) {
		RENDERBUTTON_NOTINITIALISED(&System0Buffers[i]);
	}
}
void RefreshTopicButtons(void)
{
	UDWORD i;

	for(i=0; i<NUM_TOPICBUFFERS; i++) {
		RENDERBUTTON_NOTINITIALISED(&TopicBuffers[i]);
	}
}


void RefreshStatsButtons(void)
{
	UDWORD i;

	for(i=0; i<NUM_STATBUFFERS; i++) {
		RENDERBUTTON_NOTINITIALISED(&StatBuffers[i]);
	}
}


void ClearObjectBuffers(void)
{
	UDWORD i;

	for(i=0; i<NUM_OBJECTBUFFERS; i++) {
		ClearObjectButtonBuffer(i);
	}
}

void ClearTopicBuffers(void)
{
	UDWORD i;

	for(i=0; i<NUM_TOPICBUFFERS; i++) {
		ClearTopicButtonBuffer(i);
	}
}

void ClearObjectButtonBuffer(SDWORD BufferID)
{
	RENDERBUTTON_NOTINITIALISED(&ObjectBuffers[BufferID]);	//  what have I done
	RENDERBUTTON_NOTINUSE(&ObjectBuffers[BufferID]);
	ObjectBuffers[BufferID].Data = NULL;
	ObjectBuffers[BufferID].Data2 = NULL;
	ObjectBuffers[BufferID].ImdRotation = DEFAULT_BUTTON_ROTATION;
}

void ClearTopicButtonBuffer(SDWORD BufferID)
{
	RENDERBUTTON_NOTINITIALISED(&TopicBuffers[BufferID]);	//  what have I done
	RENDERBUTTON_NOTINUSE(&TopicBuffers[BufferID]);
	TopicBuffers[BufferID].Data = NULL;
	TopicBuffers[BufferID].Data2 = NULL;
	TopicBuffers[BufferID].ImdRotation = DEFAULT_BUTTON_ROTATION;
}

SDWORD GetObjectBuffer(void)
{
	SDWORD i;

	for(i=0; i<NUM_OBJECTBUFFERS; i++) {
		if( IsBufferInUse(&ObjectBuffers[i])==FALSE )
		{
			return i;
		}
	}

	return -1;
}

SDWORD GetTopicBuffer(void)
{
	SDWORD i;

	for(i=0; i<NUM_TOPICBUFFERS; i++) {
		if( IsBufferInUse(&TopicBuffers[i])==FALSE )
		{
			return i;
		}
	}

	return -1;
}

void ClearStatBuffers(void)
{
	UDWORD i;

	for(i=0; i<NUM_STATBUFFERS; i++) {
		RENDERBUTTON_NOTINITIALISED(&StatBuffers[i]);	//  what have I done
		RENDERBUTTON_NOTINUSE(&StatBuffers[i]);
		StatBuffers[i].Data = NULL;
		StatBuffers[i].ImdRotation = DEFAULT_BUTTON_ROTATION;
	}
}

SDWORD GetStatBuffer(void)
{
	SDWORD i;

	for(i=0; i<NUM_STATBUFFERS; i++) {
		if( IsBufferInUse(&StatBuffers[i])==FALSE )
 		{
	 		return i;
		}
	}

	return -1;
}

/*these have been set up for the Transporter - the design screen DOESN'T use them
NB On the PC there are 80!!!!!*/
void ClearSystem0Buffers(void)
{
	UDWORD i;

	for(i=0; i<NUM_SYSTEM0BUFFERS; i++) {
		ClearSystem0ButtonBuffer(i);
	}
}

void ClearSystem0ButtonBuffer(SDWORD BufferID)
{
	RENDERBUTTON_NOTINITIALISED(&System0Buffers[BufferID]);	//  what have I done
	RENDERBUTTON_NOTINUSE(&System0Buffers[BufferID]);
	System0Buffers[BufferID].Data = NULL;
	System0Buffers[BufferID].Data2 = NULL;
	System0Buffers[BufferID].ImdRotation = DEFAULT_BUTTON_ROTATION;
}

SDWORD GetSystem0Buffer(void)
{
	SDWORD i;

	for(i=0; i<NUM_SYSTEM0BUFFERS; i++)
	{
		if( IsBufferInUse(&System0Buffers[i])==FALSE )
		{
			return i;
		}
	}

	return -1;
}


// Free up data for interface buttons.
//
void DeleteButtonData(void)
{
	UDWORD i;
	for(i=0; i<NUM_OBJECTSURFACES; i++) {
		FREE(ObjectSurfaces[i].Buffer);
		iV_SurfaceDestroy(ObjectSurfaces[i].Surface);
	}

	for(i=0; i<NUM_TOPICSURFACES; i++) {
		FREE(TopicSurfaces[i].Buffer);
		iV_SurfaceDestroy(TopicSurfaces[i].Surface);
	}

	for(i=0; i<NUM_STATSURFACES; i++) {
		FREE(StatSurfaces[i].Buffer);
		iV_SurfaceDestroy(StatSurfaces[i].Surface);
	}

    for(i=0; i<NUM_SYSTEM0SURFACES; i++) {
		FREE(System0Surfaces[i].Buffer);
		iV_SurfaceDestroy(System0Surfaces[i].Surface);
	}
}




UWORD ButXPos = 0;
UWORD ButYPos = 0;
UWORD ButWidth,ButHeight;



void OpenButtonRender(UWORD XPos,UWORD YPos,UWORD Width,UWORD Height)
{
	if (pie_Hardware())
	{
		ButXPos = XPos;
		ButYPos = YPos;
		ButWidth = Width;
		ButHeight = Height;
		pie_Set2DClip(XPos,YPos,(UWORD)(XPos+Width),(UWORD)(YPos+Height));
	}
	else
	{
		ButXPos = 0;
		ButYPos = 0;
	}
}





void CloseButtonRender(void)
{


	if (pie_Hardware())
	{
		pie_Set2DClip(CLIP_BORDER,CLIP_BORDER,psRendSurface->width-CLIP_BORDER,psRendSurface->height-CLIP_BORDER);
	}


}



// Clear a button bitmap. ( copy the button background ).
//
void ClearButton(BOOL Down,UDWORD Size, UDWORD buttonType)
{
	if(Down)
	{
//		pie_ImageFileID(IntImages,(UWORD)(IMAGE_BUT0_DOWN+(Size*2)+(buttonType*6)),ButXPos,ButYPos);
		pie_ImageFileID(IntImages,(UWORD)(IMAGE_BUT0_DOWN+(buttonType*2)),ButXPos,ButYPos);
	}
	else
	{
//		pie_ImageFileID(IntImages,(UWORD)(IMAGE_BUT0_UP+(Size*2)+(buttonType*6)),ButXPos,ButYPos);
		pie_ImageFileID(IntImages,(UWORD)(IMAGE_BUT0_UP+(buttonType*2)),ButXPos,ButYPos);
	}
}

// Create a button by rendering an IMD object into it.
//
void CreateIMDButton(IMAGEFILE *ImageFile,UWORD ImageID,void *Object,UDWORD Player,RENDERED_BUTTON *Buffer,BOOL Down,
					 UDWORD IMDType,UDWORD buttonType)
{
	UDWORD Size;
	iVector Rotation,Position, NullVector;
	UDWORD ox,oy;
	BUTTON_SURFACE *ButSurf;
	UDWORD Radius;
	UDWORD basePlateSize;
	SDWORD scale;

	ButSurf = Buffer->ButSurf;

	if(Down) {
		ox = oy = 2;
	} else {
		ox = oy = 0;
	}

	if((IMDType == IMDTYPE_DROID) || (IMDType == IMDTYPE_DROIDTEMPLATE)) {	// The case where we have to render a composite droid.
		if (!pie_Hardware())
		{
			iV_RenderAssign(iV_MODE_SURFACE,ButSurf->Surface);
		}

		if(Down)
		{
			//the top button is smaller than the bottom button
			if (buttonType == TOPBUTTON)
			{
				pie_SetGeometricOffset(
					(ButXPos + iV_GetImageWidth(IntImages,IMAGE_BUT0_DOWN)/2) + ButtonDrawXOffset + 2,
					(ButYPos + iV_GetImageHeight(IntImages,IMAGE_BUT0_DOWN)/2) + 2 + 8 + ButtonDrawYOffset);
			}
			else
			{
				pie_SetGeometricOffset(
					(ButXPos + iV_GetImageWidth(IntImages,IMAGE_BUTB0_DOWN)/2) + ButtonDrawXOffset + 2,
					(ButYPos + iV_GetImageHeight(IntImages,IMAGE_BUTB0_DOWN)/2) + 2 + 12 + ButtonDrawYOffset);
			}
		}
		else
		{
			//the top button is smaller than the bottom button
			if (buttonType == TOPBUTTON)
			{
				pie_SetGeometricOffset(
					(ButXPos + iV_GetImageWidth(IntImages,IMAGE_BUT0_UP)/2) + ButtonDrawXOffset,
					(ButYPos + iV_GetImageHeight(IntImages,IMAGE_BUT0_UP)/2) + 8  + ButtonDrawYOffset);
			}
			else
			{
				pie_SetGeometricOffset(
					(ButXPos + iV_GetImageWidth(IntImages,IMAGE_BUT0_UP)/2) + ButtonDrawXOffset,
					(ButYPos + iV_GetImageHeight(IntImages,IMAGE_BUTB0_UP)/2) + 12  + ButtonDrawYOffset);
			}
		}

		if(IMDType == IMDTYPE_DROID)
		{
			Radius = getComponentDroidRadius((DROID*)Object);
		}
		else
		{
			Radius = getComponentDroidTemplateRadius((DROID_TEMPLATE*)Object);
		}

		Size = 2;
		scale = DROID_BUT_SCALE;
		ASSERT((Radius <= 128,"create PIE button big component found"));

		ClearButton(Down, Size, buttonType);

		Rotation.x = -30;
		Rotation.y = (UDWORD) Buffer->ImdRotation;
		Rotation.z = 0;

		NullVector.x = 0;
		NullVector.y = 0;
		NullVector.z = 0;

		if(IMDType == IMDTYPE_DROID)
		{
			if(((DROID*)Object)->droidType == DROID_TRANSPORTER) {
				Position.x = 0;
				Position.y = 0;//BUT_TRANSPORTER_ALT;
				Position.z = BUTTON_DEPTH;
				scale = DROID_BUT_SCALE/2;
			}
			else
			{
				Position.x = Position.y = 0;
				Position.z = BUTTON_DEPTH;
			}
		}
		else//(IMDType == IMDTYPE_DROIDTEMPLATE)
		{
			if(((DROID_TEMPLATE*)Object)->droidType == DROID_TRANSPORTER) {
				Position.x = 0;
				Position.y = 0;//BUT_TRANSPORTER_ALT;
				Position.z = BUTTON_DEPTH;
				scale = DROID_BUT_SCALE/2;
			}
			else
			{
				Position.x = Position.y = 0;
				Position.z = BUTTON_DEPTH;
			}
		}

		//lefthand display droid buttons
		if(IMDType == IMDTYPE_DROID)
		{
			displayComponentButtonObject((DROID*)Object,&Rotation,&Position,TRUE, scale);
		}
		else
		{


			displayComponentButtonTemplate((DROID_TEMPLATE*)Object,&Rotation,&Position,TRUE, scale);
		}

		if(!pie_Hardware())
		{
			iV_RenderAssign(iV_MODE_4101,&rendSurface);
		}
	}
	else
	{	// Just drawing a single IMD.
		if(!pie_Hardware())
		{
			iV_RenderAssign(iV_MODE_SURFACE,ButSurf->Surface);
		}



		if(Down)
		{
			if (buttonType == TOPBUTTON)
			{
				pie_SetGeometricOffset(
					(ButXPos + iV_GetImageWidth(IntImages,IMAGE_BUT0_DOWN)/2) + ButtonDrawXOffset + 2,
					(ButYPos + iV_GetImageHeight(IntImages,IMAGE_BUT0_DOWN)/2) + 2 + 8 + ButtonDrawYOffset);
			}
			else
			{
				pie_SetGeometricOffset(
					(ButXPos + iV_GetImageWidth(IntImages,IMAGE_BUTB0_DOWN)/2) + ButtonDrawXOffset + 2,
					(ButYPos + iV_GetImageHeight(IntImages,IMAGE_BUTB0_DOWN)/2) + 2 + 12 + ButtonDrawYOffset);
			}
		}
		else
		{
			if (buttonType == TOPBUTTON)
			{
				pie_SetGeometricOffset(
					(ButXPos + iV_GetImageWidth(IntImages,IMAGE_BUT0_UP)/2) + ButtonDrawXOffset,
					(ButYPos + iV_GetImageHeight(IntImages,IMAGE_BUT0_UP)/2) + 8  + ButtonDrawYOffset);
			}
			else
			{
				pie_SetGeometricOffset(
					(ButXPos + iV_GetImageWidth(IntImages,IMAGE_BUTB0_UP)/2) + ButtonDrawXOffset,
					(ButYPos + iV_GetImageHeight(IntImages,IMAGE_BUTB0_UP)/2) + 12  + ButtonDrawYOffset);
			}
		}

	// Decide which button grid size to use.
		if(IMDType == IMDTYPE_COMPONENT)
		{
			Radius = getComponentRadius((BASE_STATS*)Object);
			Size = 2;//small structure
			scale = rescaleButtonObject(Radius, COMP_BUT_SCALE, COMPONENT_RADIUS);
			//scale = COMP_BUT_SCALE;
			//ASSERT((Radius <= OBJECT_RADIUS,"Object too big for button - %s",
			//		((BASE_STATS*)Object)->pName));
		}
		else if(IMDType == IMDTYPE_RESEARCH)
		{
			Radius = getResearchRadius((BASE_STATS*)Object);
			if(Radius <= 100)
			{
				Size = 2;//small structure
				scale = rescaleButtonObject(Radius, COMP_BUT_SCALE, COMPONENT_RADIUS);
				//scale = COMP_BUT_SCALE;
			}
			else if(Radius <= 128)
			{
				Size = 2;//small structure
				scale = SMALL_STRUCT_SCALE;
			}
			else if(Radius <= 256)
			{
				Size = 1;//med structure
				scale = MED_STRUCT_SCALE;
			}
			else
			{
				Size = 0;
				scale = LARGE_STRUCT_SCALE;
			}
		}
		else if(IMDType == IMDTYPE_STRUCTURE)
		{
			basePlateSize = getStructureSize((STRUCTURE*)Object);
			if(basePlateSize == 1)
			{
				Size = 2;//small structure
				scale = SMALL_STRUCT_SCALE;
			}
			else if(basePlateSize == 2)
			{
				Size = 1;//med structure
				scale = MED_STRUCT_SCALE;
			}
			else
			{
				Size = 0;
				scale = LARGE_STRUCT_SCALE;
			}
		}
		else if(IMDType == IMDTYPE_STRUCTURESTAT)
		{
			basePlateSize= getStructureStatSize((STRUCTURE_STATS*)Object);
			if(basePlateSize == 1)
			{
				Size = 2;//small structure
				scale = SMALL_STRUCT_SCALE;
			}
			else if(basePlateSize == 2)
			{
				Size = 1;//med structure
				scale = MED_STRUCT_SCALE;
			}
			else
			{
				Size = 0;
				scale = LARGE_STRUCT_SCALE;
			}
		}
		else
		{

			Radius = ((iIMDShape*)Object)->sradius;

			if(Radius <= 128) {
				Size = 2;//small structure
				scale = SMALL_STRUCT_SCALE;
			} else if(Radius <= 256) {
				Size = 1;//med structure
				scale = MED_STRUCT_SCALE;
			} else {
				Size = 0;
				scale = LARGE_STRUCT_SCALE;
			}
		}



		ClearButton(Down,Size, buttonType);

		Rotation.x = -30;
		Rotation.y = (UWORD ) Buffer->ImdRotation;
		Rotation.z = 0;

		NullVector.x = 0;
		NullVector.y = 0;
		NullVector.z = 0;

		Position.x = 0;
		Position.y = 0;
		Position.z = BUTTON_DEPTH; //was 		Position.z = Radius*30;

		if(ImageFile) {
			iV_DrawTransImage(ImageFile,ImageID,ButXPos+ox,ButYPos+oy);
            //there may be an extra icon for research buttons now - AB 9/1/99
            /*if (IMDType == IMDTYPE_RESEARCH)
            {
                if (((RESEARCH *)Object)->subGroup != NO_RESEARCH_ICON)
                {
                    iV_DrawTransImage(ImageFile,((RESEARCH *)Object)->subGroup,ButXPos+ox + 40,ButYPos+oy);
                }
            }*/
		}


		pie_SetDepthBufferStatus(DEPTH_CMP_LEQ_WRT_ON);

		/* all non droid buttons */
		if(IMDType == IMDTYPE_COMPONENT) {
			displayComponentButton((BASE_STATS*)Object,&Rotation,&Position,TRUE, scale);
		} else if(IMDType == IMDTYPE_RESEARCH) {
			displayResearchButton((BASE_STATS*)Object,&Rotation,&Position,TRUE, scale);
		} else if(IMDType == IMDTYPE_STRUCTURE) {
			displayStructureButton((STRUCTURE*)Object,&Rotation,&Position,TRUE, scale);
		} else if(IMDType == IMDTYPE_STRUCTURESTAT) {
			displayStructureStatButton((STRUCTURE_STATS*)Object,Player,&Rotation,&Position,TRUE, scale);
		} else {
			displayIMDButton((iIMDShape*)Object,&Rotation,&Position,TRUE, scale);
		}

		pie_SetDepthBufferStatus(DEPTH_CMP_ALWAYS_WRT_ON);

		/* Reassign the render buffer to be back to normal */
		if(!pie_Hardware())
		{
			iV_RenderAssign(iV_MODE_4101,&rendSurface);
		}
	}
}


// Create a button by rendering an image into it.
//
void CreateImageButton(IMAGEFILE *ImageFile,UWORD ImageID,RENDERED_BUTTON *Buffer,BOOL Down, UDWORD buttonType)
{
	UDWORD ox,oy;

	BUTTON_SURFACE *ButSurf = Buffer->ButSurf;

	if(!pie_Hardware())
	{
		iV_RenderAssign(iV_MODE_SURFACE,ButSurf->Surface);
	}

	ox = oy = 0;
	/*if(Down)
	{
		ox = oy = 2;
	} */

	ClearButton(Down,0, buttonType);

	iV_DrawTransImage(ImageFile,ImageID,ButXPos+ox,ButYPos+oy);
//	DrawTransImageSR(Image,ox,oy);

	if(!pie_Hardware())
	{
		iV_RenderAssign(iV_MODE_4101,&rendSurface);
	}
}


// Create a blank button.
//
void CreateBlankButton(RENDERED_BUTTON *Buffer,BOOL Down, UDWORD buttonType)
{
	BUTTON_SURFACE *ButSurf = Buffer->ButSurf;
	UDWORD ox,oy;

	if(Down) {
		ox = oy = 1;
	} else {
		ox = oy = 0;
	}

	if(!pie_Hardware())
	{
		iV_RenderAssign(iV_MODE_SURFACE,ButSurf->Surface);
	}

	ClearButton(Down,0, buttonType);

	// Draw a question mark, bit of quick hack this.
	iV_DrawTransImage(IntImages,IMAGE_QUESTION_MARK,ButXPos+ox+10,ButYPos+oy+3);

	if(!pie_Hardware())
	{
		iV_RenderAssign(iV_MODE_4101,&rendSurface);
	}
}






// Render a button to display memory.
//
void RenderButton(struct _widget *psWidget,RENDERED_BUTTON *Buffer,UDWORD x,UDWORD y, UDWORD buttonType,BOOL Down)
{


	BUTTON_SURFACE *ButSurf = Buffer->ButSurf;
	UWORD ImageID;

	if(!pie_Hardware())
	{
		DrawBegin();

		if(Down) {
			if (buttonType == TOPBUTTON) {
				ImageID = IMAGE_BUT0_DOWN;
			} else {
				ImageID = IMAGE_BUTB0_DOWN;
			}
		} else {
			if (buttonType == TOPBUTTON) {
				ImageID = IMAGE_BUT0_UP;
			} else {
				ImageID = IMAGE_BUTB0_UP;
			}
		}

		iV_ppBitmapTrans((iBitmap*)ButSurf->Buffer,
					x,y,
					iV_GetImageWidth(IntImages,ImageID),iV_GetImageHeight(IntImages,ImageID),
					ButSurf->Surface->width);

		DrawEnd();
	}


}


// Returns TRUE if the droid is currently demolishing something or moving to demolish something.
//
BOOL DroidIsDemolishing(DROID *Droid)
{
	BASE_STATS	*Stats;
	STRUCTURE *Structure;
	UDWORD x,y;

	//if(droidType(Droid) != DROID_CONSTRUCT) return FALSE;
    if (!(droidType(Droid) == DROID_CONSTRUCT OR droidType(Droid) ==
        DROID_CYBORG_CONSTRUCT))
    {
        return FALSE;
    }

	if(orderStateStatsLoc(Droid, DORDER_DEMOLISH,&Stats,&x,&y)) {	// Moving to demolish location?

		return TRUE;

	} else if( orderStateObj(Droid, DORDER_DEMOLISH,(BASE_OBJECT**)&Structure) ) {	// Is demolishing?

		return TRUE;
	}

	return FALSE;
}

// Returns TRUE if the droid is currently repairing another droid.
BOOL DroidIsRepairing(DROID *Droid)
{
	BASE_OBJECT *psObject;

    //if(droidType(Droid) != DROID_REPAIR)
    if (!(droidType(Droid) == DROID_REPAIR OR droidType(Droid) ==
        DROID_CYBORG_REPAIR))
    {
        return FALSE;
    }

    if (orderStateObj(Droid, DORDER_DROIDREPAIR, &psObject))
    {
		return TRUE;
	}

	return FALSE;
}

// Returns TRUE if the droid is currently building something.
//
BOOL DroidIsBuilding(DROID *Droid)
{
	BASE_STATS	*Stats;
	STRUCTURE *Structure;
	UDWORD x,y;

	//if(droidType(Droid) != DROID_CONSTRUCT) return FALSE;
    if (!(droidType(Droid) == DROID_CONSTRUCT OR
        droidType(Droid) == DROID_CYBORG_CONSTRUCT))
    {
        return FALSE;
    }

	if(orderStateStatsLoc(Droid, DORDER_BUILD,&Stats,&x,&y)) {	// Moving to build location?

		return FALSE;

	} else if( orderStateObj(Droid, DORDER_BUILD,(BASE_OBJECT**)&Structure) ||	// Is building or helping?
			orderStateObj(Droid, DORDER_HELPBUILD,(BASE_OBJECT**)&Structure) ) {

//		DBPRINTF(("%p : %d %d\n",Droid,orderStateObj(Droid, DORDER_BUILD,(BASE_OBJECT**)&Structure),
//						orderStateObj(Droid, DORDER_HELPBUILD,(BASE_OBJECT**)&Structure)));

		return TRUE;
	}

	return FALSE;
}


// Returns TRUE if the droid has been ordered build something ( but has'nt started yet )
//
BOOL DroidGoingToBuild(DROID *Droid)
{
	BASE_STATS	*Stats;
	UDWORD x,y;

	//if(droidType(Droid) != DROID_CONSTRUCT) return FALSE;
    if (!(droidType(Droid) == DROID_CONSTRUCT OR
        droidType(Droid) == DROID_CYBORG_CONSTRUCT))
    {
        return FALSE;
    }

	if(orderStateStatsLoc(Droid, DORDER_BUILD,&Stats,&x,&y)) {	// Moving to build location?
		return TRUE;
	}

	return FALSE;
}


// Get the structure for a structure which a droid is currently building.
//
STRUCTURE *DroidGetBuildStructure(DROID *Droid)
{
	STRUCTURE *Structure;

	if(!orderStateObj(Droid, DORDER_BUILD,(BASE_OBJECT**)&Structure)) {
		orderStateObj(Droid, DORDER_HELPBUILD,(BASE_OBJECT**)&Structure);
	}

	return Structure;
}

// Get the first factory assigned to a command droid
STRUCTURE *droidGetCommandFactory(DROID *psDroid)
{
	SDWORD		inc;
	STRUCTURE	*psCurr;

	for(inc = 0; inc < MAX_FACTORY; inc++)
	{
		if ( psDroid->secondaryOrder & (1 << (inc + DSS_ASSPROD_SHIFT)) )
		{
			// found an assigned factory - look for it in the lists
			for (psCurr = apsStructLists[psDroid->player]; psCurr; psCurr = psCurr->psNext)
			{
				if ( (psCurr->pStructureType->type == REF_FACTORY) &&
					 ( ((FACTORY *)psCurr->pFunctionality)->
								psAssemblyPoint->factoryInc == inc ) )
				{
					return psCurr;
				}
			}
		}
		if ( psDroid->secondaryOrder & (1 << (inc + DSS_ASSPROD_CYBORG_SHIFT)) )
		{
			// found an assigned factory - look for it in the lists
			for (psCurr = apsStructLists[psDroid->player]; psCurr; psCurr = psCurr->psNext)
			{
				if ( (psCurr->pStructureType->type == REF_CYBORG_FACTORY) &&
					 ( ((FACTORY *)psCurr->pFunctionality)->
								psAssemblyPoint->factoryInc == inc ) )
				{
					return psCurr;
				}
			}
		}
		if ( psDroid->secondaryOrder & (1 << (inc + DSS_ASSPROD_VTOL_SHIFT)) )
		{
			// found an assigned factory - look for it in the lists
			for (psCurr = apsStructLists[psDroid->player]; psCurr; psCurr = psCurr->psNext)
			{
				if ( (psCurr->pStructureType->type == REF_VTOL_FACTORY) &&
					 ( ((FACTORY *)psCurr->pFunctionality)->
								psAssemblyPoint->factoryInc == inc ) )
				{
					return psCurr;
				}
			}
		}
	}

	return NULL;
}

// Get the stats for a structure which a droid is going to ( but not yet ) building.
//
BASE_STATS *DroidGetBuildStats(DROID *Droid)
{
	BASE_STATS *Stats;
	UDWORD x,y;

	if(orderStateStatsLoc(Droid, DORDER_BUILD,&Stats,&x,&y)) {	// Moving to build location?
		return Stats;
	}

	return NULL;
}

iIMDShape *DroidGetIMD(DROID *Droid)
{
	return Droid->sDisplay.imd;
}

/*UDWORD DroidGetIMDIndex(DROID *Droid)
{
	return Droid->imdNum;
}*/

BOOL StructureIsManufacturing(STRUCTURE *Structure)
{
	return ((Structure->pStructureType->type == REF_FACTORY OR
			Structure->pStructureType->type == REF_CYBORG_FACTORY OR
			Structure->pStructureType->type == REF_VTOL_FACTORY) &&
			((FACTORY*)Structure->pFunctionality)->psSubject);
}

FACTORY *StructureGetFactory(STRUCTURE *Structure)
{
	return (FACTORY*)Structure->pFunctionality;
}

BOOL StructureIsResearching(STRUCTURE *Structure)
{
	return (Structure->pStructureType->type == REF_RESEARCH) &&
			((RESEARCH_FACILITY*)Structure->pFunctionality)->psSubject;
}

RESEARCH_FACILITY *StructureGetResearch(STRUCTURE *Structure)
{
	return (RESEARCH_FACILITY*)Structure->pFunctionality;
}


iIMDShape *StructureGetIMD(STRUCTURE *Structure)
{
//	return buildingIMDs[aBuildingIMDs[Structure->player][Structure->pStructureType->type]];
	return Structure->pStructureType->pIMD;
}


DROID_TEMPLATE *FactoryGetTemplate(FACTORY *Factory)
{
	return (DROID_TEMPLATE*)Factory->psSubject;
}

//iIMDShape *TemplateGetIMD(DROID_TEMPLATE *Template,UDWORD Player)
//{
////	return droidIMDs[GetIMDFromTemplate(Template,Player)];
//	return NULL;
//}
//
///*UDWORD TemplateGetIMDIndex(DROID_TEMPLATE *Template,UDWORD Player)
//{
//	return GetIMDFromTemplate(Template,Player);
//}*/
//
//SDWORD ResearchGetImage(RESEARCH_FACILITY *Research)
//{
//	return 0;	//IMAGE_RESITEM;
//}


BOOL StatIsStructure(BASE_STATS *Stat)
{
	return (Stat->ref >= REF_STRUCTURE_START && Stat->ref <
				REF_STRUCTURE_START + REF_RANGE);
}

BOOL StatIsFeature(BASE_STATS *Stat)
{
	return (Stat->ref >= REF_FEATURE_START && Stat->ref <
				REF_FEATURE_START + REF_RANGE);
}

iIMDShape *StatGetStructureIMD(BASE_STATS *Stat,UDWORD Player)
{
	(void)Player;
	//return buildingIMDs[aBuildingIMDs[Player][((STRUCTURE_STATS*)Stat)->type]];
	return ((STRUCTURE_STATS*)Stat)->pIMD;
}

BOOL StatIsTemplate(BASE_STATS *Stat)
{
	return (Stat->ref >= REF_TEMPLATE_START &&
				 Stat->ref < REF_TEMPLATE_START + REF_RANGE);
}

//iIMDShape *StatGetTemplateIMD(BASE_STATS *Stat,UDWORD Player)
//{
//	return TemplateGetIMD((DROID_TEMPLATE*)Stat,Player);
//}
//
///*UDWORD StatGetTemplateIMDIndex(BASE_STATS *Stat,UDWORD Player)
//{
//	return TemplateGetIMDIndex((DROID_TEMPLATE*)Stat,Player);
//}*/

SDWORD StatIsComponent(BASE_STATS *Stat)
{
	if(Stat->ref >= REF_BODY_START &&
				 Stat->ref < REF_BODY_START + REF_RANGE) {
		//return TRUE;
		return COMP_BODY;
	}

	if(Stat->ref >= REF_BRAIN_START &&
				 Stat->ref < REF_BRAIN_START + REF_RANGE) {
		//return TRUE;
		return COMP_BRAIN;
	}

	if(Stat->ref >= REF_PROPULSION_START &&
				 Stat->ref < REF_PROPULSION_START + REF_RANGE) {
		//return TRUE;
		return COMP_PROPULSION;
	}

	if(Stat->ref >= REF_WEAPON_START &&
				 Stat->ref < REF_WEAPON_START + REF_RANGE) {
		//return TRUE;
		return COMP_WEAPON;
	}

	if(Stat->ref >= REF_SENSOR_START &&
				 Stat->ref < REF_SENSOR_START + REF_RANGE) {
		//return TRUE;
		return COMP_SENSOR;
	}

	if(Stat->ref >= REF_ECM_START &&
				 Stat->ref < REF_ECM_START + REF_RANGE) {
		//return TRUE;
		return COMP_ECM;
	}

	if(Stat->ref >= REF_CONSTRUCT_START &&
				 Stat->ref < REF_CONSTRUCT_START + REF_RANGE) {
		//return TRUE;
		return COMP_CONSTRUCT;
	}

	if(Stat->ref >= REF_REPAIR_START &&
				 Stat->ref < REF_REPAIR_START + REF_RANGE) {
		//return TRUE;
		return COMP_REPAIRUNIT;
	}

	//return FALSE;
	return COMP_UNKNOWN;
}

//iIMDShape *StatGetComponentIMD(BASE_STATS *Stat)
//iIMDShape *StatGetComponentIMD(BASE_STATS *Stat, SDWORD compID)
BOOL StatGetComponentIMD(BASE_STATS *Stat, SDWORD compID,iIMDShape **CompIMD,iIMDShape **MountIMD)
{
	WEAPON_STATS		*psWStat;
	/*SWORD ID;

	ID = GetTokenID(CompIMDIDs,Stat->pName);
	if(ID >= 0) {
		return componentIMDs[ID];
	}

	ASSERT((0,"StatGetComponent : Unknown component"));*/

//	COMP_BASE_STATS *CompStat = (COMP_BASE_STATS *)Stat;
//	DBPRINTF(("%s\n",Stat->pName));

	*CompIMD = NULL;
	*MountIMD = NULL;

	switch (compID)
	{
	case COMP_BODY:
		*CompIMD = ((COMP_BASE_STATS *)Stat)->pIMD;
		return TRUE;
//		return ((COMP_BASE_STATS *)Stat)->pIMD;

	case COMP_BRAIN:
//		ASSERT(( ((UBYTE*)Stat >= (UBYTE*)asCommandDroids) &&
//				 ((UBYTE*)Stat < (UBYTE*)asCommandDroids + sizeof(asCommandDroids)),
//				 "StatGetComponentIMD: This 'BRAIN_STATS' is actually meant to be a 'COMMAND_DROID'"));

//		psWStat = asWeaponStats + ((COMMAND_DROID *)Stat)->nWeapStat;
		psWStat = ((BRAIN_STATS *)Stat)->psWeaponStat;
		*MountIMD = psWStat->pMountGraphic;
		*CompIMD = psWStat->pIMD;
		return TRUE;

	case COMP_WEAPON:
		*MountIMD = ((WEAPON_STATS*)Stat)->pMountGraphic;
		*CompIMD = ((COMP_BASE_STATS *)Stat)->pIMD;
		return TRUE;

	case COMP_SENSOR:
		*MountIMD = ((SENSOR_STATS*)Stat)->pMountGraphic;
		*CompIMD = ((COMP_BASE_STATS *)Stat)->pIMD;
		return TRUE;

	case COMP_ECM:
		*MountIMD = ((ECM_STATS*)Stat)->pMountGraphic;
		*CompIMD = ((COMP_BASE_STATS *)Stat)->pIMD;
		return TRUE;

	case COMP_CONSTRUCT:
		*MountIMD = ((CONSTRUCT_STATS*)Stat)->pMountGraphic;
		*CompIMD = ((COMP_BASE_STATS *)Stat)->pIMD;
		return TRUE;

	case COMP_PROPULSION:
		*CompIMD = ((COMP_BASE_STATS *)Stat)->pIMD;
		return TRUE;

	case COMP_REPAIRUNIT:
		*MountIMD = ((REPAIR_STATS*)Stat)->pMountGraphic;
		*CompIMD = ((COMP_BASE_STATS *)Stat)->pIMD;
		return TRUE;

	default:
		//COMP_UNKNOWN should be an error
		ASSERT((FALSE, "StatGetComponent : Unknown component"));
	}

	return FALSE;
}


BOOL StatIsResearch(BASE_STATS *Stat)
{
	return (Stat->ref >= REF_RESEARCH_START && Stat->ref <
				REF_RESEARCH_START + REF_RANGE);
}

//void StatGetResearchImage(BASE_STATS *psStat, SDWORD *Image,iIMDShape **Shape, BOOL drawTechIcon)
void StatGetResearchImage(BASE_STATS *psStat, SDWORD *Image, iIMDShape **Shape,
                          BASE_STATS **ppGraphicData, BOOL drawTechIcon)
{
	*Image = -1;
	if (drawTechIcon)
	{
		if (((RESEARCH *)psStat)->iconID != NO_RESEARCH_ICON)
		{
			*Image = ((RESEARCH *)psStat)->iconID;
		}
	}
    //if the research has a Stat associated with it - use this as display in the button
    if (((RESEARCH *)psStat)->psStat)
    {
        *ppGraphicData = ((RESEARCH *)psStat)->psStat;
        //make sure the IMDShape is initialised
        *Shape = NULL;
    }
    else
    {
        //no stat so just just the IMD associated with the research
	    *Shape = ((RESEARCH *)psStat)->pIMD;
        //make sure the stat is initialised
        *ppGraphicData = NULL;
    }
}

// Find a token in the specified token list and return it's ID.
//
/*SWORD GetTokenID(TOKENID *Tok,STRING *Token)
{
	while(Tok->Token!=NULL) {
		if(strcmp(Tok->Token,Token) == 0) {
			return Tok->ID;
		}
		Tok++;
	}

	//test for all - AB
//	return IMD_DEFAULT;
	return -1;
}*/

// Find a token in the specified token list and return it's Index.
//
/*SWORD FindTokenID(TOKENID *Tok,STRING *Token)
{
	SWORD Index = 0;
	while(Tok->Token!=NULL) {
		if(strcmp(Tok->Token,Token) == 0) {
			return Index;
		}
		Index++;
		Tok++;
	}

	return -1;
}*/

//void intDisplayBorderForm(struct _widget *psWidget, UDWORD xOffset, UDWORD yOffset, UDWORD *pColours)
//{
//	W_TABFORM *Form = (W_TABFORM*)psWidget;
//	UDWORD x0,y0,x1,y1;
//
//	x0 = xOffset+Form->x;
//	y0 = yOffset+Form->y;
//	x1 = x0 + Form->width;
//	y1 = y0 + Form->height;
//
//	AdjustTabFormSize(Form,&x0,&y0,&x1,&y1);
//
//	RenderWindowFrame(&FrameNormal,x0,y0,x1-x0,y1-y0);
//}

#define	DRAW_BAR_TEXT	1




/* Draws a stats bar for the design screen */
void intDisplayStatsBar(struct _widget *psWidget, UDWORD xOffset, UDWORD yOffset, UDWORD *pColours)
{
	W_BARGRAPH		*BarGraph = (W_BARGRAPH*)psWidget;
	SDWORD			x0, y0, iX, iY;
	static char		szVal[6], szCheckWidth[6] = "00000";

	x0 = xOffset + BarGraph->x;
	y0 = yOffset + BarGraph->y;

//	//draw the background image
//	iV_DrawTransImage(IntImages,IMAGE_DES_STATSBACK,x0,y0);

	//increment for the position of the level indicator
	x0 += 3;
	y0 += 3;

	/* indent to allow text value */
#if	DRAW_BAR_TEXT && !defined(PSX)
	iX = x0 + iV_GetTextWidth( szCheckWidth );
	iY = y0 + (iV_GetImageHeight(IntImages,IMAGE_DES_STATSCURR) - iV_GetTextLineSize())/2 -
					iV_GetTextAboveBase();
#else
	iX = x0;
	iY = y0;
#endif

	//draw current value section
	iV_DrawImageRect( IntImages, IMAGE_DES_STATSCURR, iX, y0, 0, 0,
			BarGraph->majorSize, iV_GetImageHeight(IntImages,IMAGE_DES_STATSCURR));

	/* draw text value */
#if	DRAW_BAR_TEXT && !defined(PSX)
	sprintf( szVal, "%d", BarGraph->iValue );
	iV_SetTextColour(-1);
	iV_DrawText( szVal, x0, iY );
#endif

	//draw the comparison value - only if not zero
	if (BarGraph->minorSize != 0)
	{
		y0 -= 1;
		iV_DrawTransImage(IntImages,IMAGE_DES_STATSCOMP,iX+BarGraph->minorSize ,y0);
	}
}






/* Draws a Template Power Bar for the Design Screen */
void intDisplayDesignPowerBar(struct _widget *psWidget, UDWORD xOffset,
							  UDWORD yOffset, UDWORD *pColours)
{
	W_BARGRAPH      *BarGraph = (W_BARGRAPH*)psWidget;
	SDWORD		    x0, y0, iX, iY;
    UDWORD          width, barWidth;
	static char		szVal[6], szCheckWidth[6] = "00000";
    UBYTE           arbitaryOffset;

	x0 = xOffset + BarGraph->x;
	y0 = yOffset + BarGraph->y;

    //this is a % so need to work out how much of the bar to draw
    /*
	// If power required is greater than Design Power bar then set to max
	if (BarGraph->majorSize > BarGraph->width)
	{
		BarGraph->majorSize = BarGraph->width;
	}*/

	DrawBegin();

	//draw the background image
	iV_DrawImage(IntImages,IMAGE_DES_POWERBAR_LEFT,x0,y0);
	iV_DrawImage(IntImages,IMAGE_DES_POWERBAR_RIGHT,
		//xOffset+psWidget->width-iV_GetImageWidth(IntImages, IMAGE_DES_POWERBAR_RIGHT),y0);
        x0 + psWidget->width-iV_GetImageWidth(IntImages, IMAGE_DES_POWERBAR_RIGHT),y0);

	//increment for the position of the bars within the background image
    arbitaryOffset = 3;
	x0 += arbitaryOffset;
	y0 += arbitaryOffset;

	/* indent to allow text value */
#if	DRAW_BAR_TEXT && !defined(PSX)
	iX = x0 + iV_GetTextWidth( szCheckWidth );
	iY = y0 + (iV_GetImageHeight(IntImages,IMAGE_DES_STATSCURR) - iV_GetTextLineSize())/2 -
					iV_GetTextAboveBase();
#else
	iX = x0;
	iY = y0;
#endif

    //adjust the width based on the text drawn
    barWidth = BarGraph->width - (iX - x0 + arbitaryOffset);
    width = BarGraph->majorSize * barWidth / 100;
    //quick check that don't go over the end - ensure % is not > 100
	if (width > barWidth)
	{
		width = barWidth;
	}

	//draw current value section
	iV_DrawImageRect(IntImages,IMAGE_DES_STATSCURR,	iX, y0, 0, 0,
						//BarGraph->majorSize, iV_GetImageHeight(IntImages,IMAGE_DES_STATSCURR));
                        width, iV_GetImageHeight(IntImages,IMAGE_DES_STATSCURR));

	/* draw text value */
#if	DRAW_BAR_TEXT && !defined(PSX)
	sprintf( szVal, "%d", BarGraph->iValue );
	iV_SetTextColour(-1);
	iV_DrawText( szVal, x0, iY );
#endif


	//draw the comparison value - only if not zero
	if (BarGraph->minorSize != 0)
	{
		y0 -= 1;
        width = BarGraph->minorSize * barWidth / 100;
        if (width > barWidth)
        {
            width = barWidth;
        }
		//iV_DrawTransImage(IntImages,IMAGE_DES_STATSCOMP,x0+BarGraph->minorSize ,y0);
        iV_DrawTransImage(IntImages, IMAGE_DES_STATSCOMP, iX + width ,y0);
	}

	DrawEnd();
}




// Widget callback function to play an audio track.
//
#define WIDGETBEEPGAP (200)	// 200 milliseconds between each beep please
void WidgetAudioCallback(int AudioID)
{
	static	SDWORD LastTimeAudio;
	if(AudioID >= 0) {
//		DBPRINTF(("%d\n",AudioID));

		SDWORD TimeSinceLastWidgetBeep;

		// Don't allow a widget beep if one was made in the last WIDGETBEEPGAP milliseconds
		// This stops double beeps happening (which seems to happen all the time)
		TimeSinceLastWidgetBeep=gameTime2-LastTimeAudio;
		if (TimeSinceLastWidgetBeep<0 || TimeSinceLastWidgetBeep>WIDGETBEEPGAP)
		{
			LastTimeAudio=gameTime2;
			audio_PlayTrack(AudioID);
//			DBPRINTF(("AudioID %d\n",AudioID));
		}
	}
}


// Widget callback to display a contents button for the Transporter
void intDisplayTransportButton(struct _widget *psWidget, UDWORD xOffset,
						  UDWORD yOffset, UDWORD *pColours)
{
	W_CLICKFORM			*Form = (W_CLICKFORM*)psWidget;
	BOOL				Down;
	BOOL				Hilight = FALSE;
	RENDERED_BUTTON		*Buffer = (RENDERED_BUTTON*)Form->pUserData;
	DROID				*psDroid = NULL;
    UDWORD              gfxId;

	OpenButtonRender((UWORD)(xOffset+Form->x), (UWORD)(yOffset+Form->y),(UWORD)Form->width,
		(UWORD)Form->height);

	Down = Form->state & (WCLICK_DOWN | WCLICK_LOCKED | WCLICK_CLICKLOCK);

    //allocate this outside of the if so the rank icons are always draw
    psDroid = (DROID*)Buffer->Data;
    //there should always be a droid associated with the button
	ASSERT((PTRVALID(psDroid, sizeof(DROID)),
		"intDisplayTransportButton: invalid droid pointer"));


/*	if( (pie_GetRenderEngine() == ENGINE_GLIDE) || (IsBufferInitialised(Buffer)==FALSE) || (Form->state & WCLICK_HILITE) ||
	(Form->state!=Buffer->State) )
*/
	if( pie_Hardware() || (IsBufferInitialised(Buffer)==FALSE) || (Form->state & WCLICK_HILITE) ||
		(Form->state!=Buffer->State) )


	{
		Hilight = Form->state & WCLICK_HILITE;

		if(Hilight)
		{
			Buffer->ImdRotation += (UWORD) ((BUTTONOBJ_ROTSPEED*frameTime2) / GAME_TICKS_PER_SEC);
		}

		Hilight = formIsHilite(Form);

		Buffer->State = Form->state;

		//psDroid = (DROID*)Buffer->Data;

		//there should always be a droid associated with the button
		//ASSERT((PTRVALID(psDroid, sizeof(DROID)),
		//	"intDisplayTransportButton: invalid droid pointer"));

		if (psDroid)
		{
			RenderToButton(NULL,0,psDroid,psDroid->player,Buffer,Down,IMDTYPE_DROID,TOPBUTTON);
		}
		else
		{
			RenderBlankToButton(Buffer,Down,TOPBUTTON);
		}
		RENDERBUTTON_INITIALISED(Buffer);
	}

	// Draw the button.
	RenderButton(psWidget, Buffer, xOffset+Form->x, yOffset+Form->y, TOPBUTTON, Down);


	CloseButtonRender();



	if (Hilight)
	{
		iV_DrawTransImage(IntImages,IMAGE_BUT_HILITE,xOffset+Form->x,yOffset+Form->y);
	}

    //if (psDroid AND missionIsOffworld()) Want this on all reInforcement missions
    if (psDroid AND missionForReInforcements())
    {
        //add the experience level for each droid
        gfxId = getDroidRankGraphic(psDroid);
	    if(gfxId != UDWORD_MAX)
	    {

		    /* Render the rank graphic at the correct location */
		    /* Render the rank graphic at the correct location */
		    iV_DrawTransImage(IntImages,(UWORD)gfxId,xOffset+Form->x+50,yOffset+Form->y+30);

	    }
    }


}



/*draws blips on radar to represent Proximity Display and damaged structures*/
void drawRadarBlips()
{
	PROXIMITY_DISPLAY	*psProxDisp;
//	STRUCTURE			*psBuilding;
	FEATURE				*psFeature;
	//VIEW_PROXIMITY		*pViewProximity;
	//SDWORD				x, y;
	UWORD				imageID;
	UDWORD				VisWidth, VisHeight, delay = 150;
	PROX_TYPE			proxType;

/*#ifndef PSX
	SDWORD				radarX,radarY;		// for multiplayer blips
	//FEATURE				*psFeature;			// ditto. Needed always now!
#endif*/

	VisWidth = RADWIDTH;
	VisHeight = RADHEIGHT;

	/* Go through all the proximity Displays*/
	for (psProxDisp = apsProxDisp[selectedPlayer]; psProxDisp != NULL;
		psProxDisp = psProxDisp->psNext)
	{
		//check it is within the radar coords
		if (psProxDisp->radarX > 0 AND psProxDisp->radarX < VisWidth AND
			psProxDisp->radarY > 0 AND psProxDisp->radarY < VisHeight)
		{
			//pViewProximity = (VIEW_PROXIMITY*)psProxDisp->psMessage->
			//	pViewData->pData;
			if (psProxDisp->type == POS_PROXDATA)
			{
				proxType = ((VIEW_PROXIMITY*)((VIEWDATA *)psProxDisp->psMessage->
					pViewData)->pData)->proxType;
			}
			else
			{
				psFeature = (FEATURE *)psProxDisp->psMessage->pViewData;
				if (psFeature AND psFeature->psStats->subType == FEAT_OIL_RESOURCE)
				{
					proxType = PROX_RESOURCE;
				}
				else
				{
					proxType = PROX_ARTEFACT;
				}
			}

			//draw the 'blips' on the radar - use same timings as radar blips
			//if the message is read - don't animate
			if (psProxDisp->psMessage->read)
			{

				//imageID = (UWORD)(IMAGE_RAD_ENM3 + (pViewProximity->
				//	proxType * (NUM_PULSES + 1)));
				imageID = (UWORD)(IMAGE_RAD_ENM3 + (proxType * (NUM_PULSES + 1)));

			}
			else
			{
				//draw animated
				if ((gameTime2 - psProxDisp->timeLastDrawn) > delay)
				{
					psProxDisp->strobe++;
					if (psProxDisp->strobe > (NUM_PULSES-1))
					{
						psProxDisp->strobe = 0;
					}
					psProxDisp->timeLastDrawn = gameTime2;
				}

				//imageID = (UWORD)(IMAGE_RAD_ENM1 + psProxDisp->strobe + (
				//	pViewProximity->proxType * (NUM_PULSES + 1)));
				imageID = (UWORD)(IMAGE_RAD_ENM1 + psProxDisp->strobe + (
					proxType * (NUM_PULSES + 1)));

			}
			//draw the 'blip'

			iV_DrawImage(IntImages,imageID, psProxDisp->radarX + RADTLX,
							psProxDisp->radarY + RADTLY);
		}
	}

	/*
	for (psBuilding = apsStructLists[selectedPlayer]; psBuilding != NULL;
		psBuilding = psBuilding->psNext)
	{
		//check it is within the radar coords
		if (psBuilding->radarX > 0 AND psBuilding->radarX < VisWidth AND
			psBuilding->radarY > 0 AND psBuilding->radarY < VisHeight)
		{
			//check if recently damaged
			if (psBuilding->timeLastHit)
			{
				if (((gameTime/250) % 2) == 0)
				{
					imageID = IMAGE_RAD_ENMREAD;
				}
				else
				{
					imageID = 0;//IMAGE_RAD_ENM1;
				}
				//turn blips off if been on for long enough
				if (psBuilding->timeLastHit + ATTACK_CB_PAUSE < gameTime)
				{
					psBuilding->timeLastHit = 0;
				}
				//draw the 'blip'
				if (imageID)
				{
#ifndef PSX
					iV_DrawTransImage(IntImages,imageID, psBuilding->radarX + RADTLX,
						psBuilding->radarY + RADTLY);
#else
					iV_DrawTransImage(IntImages,imageID, psBuilding->radarX*2 + RADTLX,
									psBuilding->radarY*2 + RADTLY);
#endif
				}
			}
		}
	}
	*/

// deathmatch code
//#ifndef PSX
//	if(bMultiPlayer && (game.type == DMATCH))
//	{
//		for (psFeature = apsFeatureLists[0]; psFeature != NULL; psFeature =
//			psFeature->psNext)
//		{
//			if( psFeature->psStats->subType == FEAT_GEN_ARTE)	// it's an artifact.
//			{
//				worldPosToRadarPos(psFeature->x >> TILE_SHIFT ,
//								   psFeature->y >> TILE_SHIFT,  &radarX,&radarY);
//				if (radarX > 0 && radarX < (SDWORD)VisWidth &&			// it's visable.
//					radarY > 0 && radarY < (SDWORD)VisHeight)
//				{
//					iV_DrawTransImage(IntImages,(UWORD)(IMAGE_RAD_ENM3),radarX + RADTLX, radarY + RADTLY);
//				}
//			}
//		}
//	}
//#endif

}

/*draws blips on world to represent Proximity Messages - no longer the Green Arrow!*/
/*void drawProximityBlips()
{
	PROXIMITY_DISPLAY	*psProxDisp;
	VIEW_PROXIMITY		*pViewProximity;
	UWORD				imageID;
	UDWORD				VisWidth, VisHeight, delay = 150;

	// Go through all the proximity Displays
	for (psProxDisp = apsProxDisp[selectedPlayer]; psProxDisp != NULL;
		psProxDisp = psProxDisp->psNext)
	{
		pViewProximity = (VIEW_PROXIMITY*)psProxDisp->psMessage->pViewData->pData;
		// Is the Message worth rendering?
		if(clipXY(pViewProximity->x,pViewProximity->y))
		{
			//if the message is read - don't draw
			if (!psProxDisp->psMessage->read)
			{
				//draw animated - use same timings as radar blips
				if ((gameTime2 - psProxDisp->timeLastDrawn) > delay)
				{
					psProxDisp->strobe++;
					if (psProxDisp->strobe > (NUM_PULSES-1))
					{
						psProxDisp->strobe = 0;
					}
					psProxDisp->timeLastDrawn = gameTime2;
				}
				imageID = (UWORD)(IMAGE_GAM_ENM1 + psProxDisp->strobe +
					(pViewProximity->proxType * (NUM_PULSES + 1)));
			}
			//draw the 'blip'
			iV_DrawTransImage(IntImages,imageID, psProxDisp->screenX, psProxDisp->screenY);
		}
	}
}*/

/*Displays the proximity messages blips over the world*/
void intDisplayProximityBlips(struct _widget *psWidget, UDWORD xOffset,
					UDWORD yOffset, UDWORD *pColours)
{
	W_CLICKFORM			*psButton = (W_CLICKFORM*)psWidget;
	PROXIMITY_DISPLAY	*psProxDisp = (PROXIMITY_DISPLAY *)psButton->pUserData;
	MESSAGE				*psMsg = psProxDisp->psMessage;
	//BOOL				Hilight = FALSE;
//	UWORD				imageID;
//	UDWORD				delay = 100;
	//VIEW_PROXIMITY		*pViewProximity;
	SDWORD				x = 0, y = 0;

	ASSERT((psMsg->type == MSG_PROXIMITY, "Invalid message type"));

	//if no data - ignore message
	if (psMsg->pViewData == NULL)
	{
		return;
	}
	//pViewProximity = (VIEW_PROXIMITY*)psProxDisp->psMessage->pViewData->pData;
	if (psProxDisp->type == POS_PROXDATA)
	{
		x = ((VIEW_PROXIMITY*)((VIEWDATA *)psProxDisp->psMessage->pViewData)->pData)->x;
		y = ((VIEW_PROXIMITY*)((VIEWDATA *)psProxDisp->psMessage->pViewData)->pData)->y;
	}
	else if (psProxDisp->type == POS_PROXOBJ)
	{
		x = ((BASE_OBJECT *)psProxDisp->psMessage->pViewData)->x;
		y = ((BASE_OBJECT *)psProxDisp->psMessage->pViewData)->y;
	}

	//if not within view ignore message
	//if (!clipXY(pViewProximity->x, pViewProximity->y))
	if (!clipXY(x, y))
	{
		return;
	}

	/*Hilight = psButton->state & WBUTS_HILITE;

	//if hilighted
	if (Hilight)
	{
		imageID = IMAGE_DES_ROAD;
		//set the button's x/y so that can be clicked on
		psButton->x = (SWORD)psProxDisp->screenX;
		psButton->y = (SWORD)psProxDisp->screenY;

		//draw the 'button'
		iV_DrawTransImage(IntImages,imageID, psButton->x, psButton->y);
		return;
	}*/

	//if the message is read - don't draw
	if (!psMsg->read)
	{
		//draw animated
		/*
		if ((gameTime2 - psProxDisp->timeLastDrawn) > delay)
		{
			psProxDisp->strobe++;
			if (psProxDisp->strobe > (NUM_PULSES-1))
			{
				psProxDisp->strobe = 0;
			}
			psProxDisp->timeLastDrawn = gameTime2;
		}
		imageID = (UWORD)(IMAGE_GAM_ENM1 + psProxDisp->strobe +
			(pViewProximity->proxType * (NUM_PULSES + 1)));
		*/
		//set the button's x/y so that can be clicked on
		psButton->x = (SWORD)(psProxDisp->screenX - psButton->width/2);
		psButton->y = (SWORD)(psProxDisp->screenY - psButton->height/2);
/*
		//draw the 'button'
		iV_DrawTransImage(IntImages,imageID, psProxDisp->screenX,
			psProxDisp->screenY);
			*/
	}
}




static UDWORD sliderMousePos(	W_SLIDER *Slider )
{
	return (widgGetFromID(psWScreen,Slider->formID)->x + Slider->x)
			+ ((Slider->pos * Slider->width) / Slider->numStops );
}


static UWORD sliderMouseUnit(W_SLIDER *Slider)
{
	UWORD posStops = (UWORD)(Slider->numStops / 20);

	if(posStops==0 || Slider->pos == 0 || Slider->pos == Slider->numStops)
	{
		return 1;
	}

	if(Slider->pos < posStops)
	{
		return (Slider->pos);
	}

	if(Slider->pos > (Slider->numStops-posStops))
	{
		return (UWORD)(Slider->numStops-Slider->pos);
	}
	return posStops;
}

void intUpdateQuantitySlider(struct _widget *psWidget, struct _w_context *psContext)
{
	W_SLIDER *Slider = (W_SLIDER*)psWidget;

	if(Slider->state & SLD_HILITE)
	{
		if(keyDown(KEY_LEFTARROW))
		{
			if(Slider->pos > 0)
			{
				Slider->pos = (UWORD)(Slider->pos - sliderMouseUnit(Slider));
				bUsingSlider = TRUE;
				SetMousePos(0,sliderMousePos(Slider),mouseY());	// move mouse
			}
		}
		else if(keyDown(KEY_RIGHTARROW))
		{
			if(Slider->pos < Slider->numStops)
			{
				Slider->pos = (UWORD)(Slider->pos + sliderMouseUnit(Slider));
				bUsingSlider = TRUE;

				SetMousePos(0,sliderMousePos(Slider),mouseY());	// move mouse
			}
		}
	}
}

void intUpdateOptionText(struct _widget *psWidget, struct _w_context *psContext)
{
}



void intDisplayResSubGroup(struct _widget *psWidget, UDWORD xOffset, UDWORD yOffset, UDWORD *pColours)
{
	W_LABEL		*Label = (W_LABEL*)psWidget;
	UDWORD		x = Label->x + xOffset;
	UDWORD		y = Label->y + yOffset;
	RESEARCH    *psResearch = (RESEARCH *)Label->pUserData;

    if (psResearch->subGroup != NO_RESEARCH_ICON)
    {
	    iV_DrawTransImage(IntImages,psResearch->subGroup,x,y);
    }
}

void intDisplayAllyIcon(struct _widget *psWidget, UDWORD xOffset, UDWORD yOffset, UDWORD *pColours)
{
	W_LABEL		*Label =  (W_LABEL*)psWidget;
//	UDWORD		i = Label->pUserData;
	UDWORD		x = Label->x + xOffset;
	UDWORD		y = Label->y + yOffset;
//	char		str[2];

    iV_DrawTransImage(IntImages,IMAGE_DES_BODYPOINTS,x,y);

//	iV_SetTextColour(-1);
//	sprintf(&str,"%d",i);
//	pie_DrawText(&str, x+6, y-1 );

}
