
// Screen coordinate based targeting system. Maintains a list of on screen objects
// and provides various functions for aquiring targets from this list.
//

#include <stdio.h>
#include <assert.h>
#include <frame.h>
#include <gtime.h>
#include <animobj.h>

#include "statsdef.h"
#include "base.h"
#include "featuredef.h"
#include "weapons.h"
#include "structuredef.h"
#include "display.h"
#include "visibility.h"
#include "ObjectDef.h"
#include "droid.h"

#include "geo.h"
#include "imd.h"
#include "vid.h"

#ifdef PSX
#include "vpsx.h"
#include "primatives.h"
#endif

#include "target.h"

extern UDWORD selectedPlayer;

#define TARGET_RESOURCES		// Allow targeting of resources.
#define TARGET_WALLS			// Allow targeting of walls.

#define MAX_TARGETS	32

typedef struct {
	UWORD	Type;
	BASE_OBJECT *psObj;
} TARGET;

static BOOL FoundCurrent;
static UWORD NumTargets;
static UWORD TargetCurrent;
static UDWORD TargetCurrentID;
static TARGET TargetList[MAX_TARGETS];
static UDWORD TargetEndTime;
static BASE_OBJECT *TargetingObject;
static UWORD TargetingType;

extern UDWORD currentGameFrame;

// Initialise the targeting system.
//
void targetInitialise(void)
{
	TargetCurrent = 0;
	TargetCurrentID = UDWORD_MAX;
}


// Reset the target list, call once per frame.
//
void targetOpenList(BASE_OBJECT *psTargeting)
{
	NumTargets = 0;
	FoundCurrent = FALSE;
	TargetingObject = psTargeting;
}


void targetCloseList(void)
{
	if(!FoundCurrent) {
//		targetAquireNew();
	}
//DBPRINTF(("%d %d %d\n",NumTargets,TargetCurrent,TargetCurrentID);
}


//// Aquire a new target.
////
//BASE_OBJECT *targetAquireNew(void)
//{
//	BASE_OBJECT *psObj;
//
//	// First try and target the nearest threat.
//	if( (psObj=targetAquireNearestObj(TargetingObject,TARGET_TYPE_THREAT)) != NULL) {
//		DBPRINTF(("Targeting threat\n"));
//		return psObj;
//	}
//
//	// if that failed then attempt to target the nearest object.
//	if( (psObj=targetAquireNearestObj(TargetingObject,TARGET_TYPE_ANY)) != NULL) {
//		DBPRINTF(("Targeting any\n"));
//		return psObj;
//	}
//
//	TargetCurrent = 0;
//	TargetCurrentID = UDWORD_MAX;
//
//	return NULL;
//}

/*
	DROID_WEAPON,		// Weapon droid
	DROID_SENSOR,		// Sensor droid
	DROID_ECM,			// ECM droid
	DROID_CONSTRUCT,	// Constructor droid
	DROID_PERSON,		// person
	DROID_CYBORG,		// cyborg-type thang
	DROID_TRANSPORTER,	// guess what this is!
	DROID_COMMAND,		// Command droid
	DROID_REPAIR,		// Repair droid
	DROID_DEFAULT,		// Default droid
	DROID_ANY,			// Any droid, Only used as a parameter for intGotoNextDroidType(droidtype).
*/

// Tell the targeting system what type of droid is doing the targeting.
//
void targetSetTargetable(UWORD DroidType)
{
	TargetingType = DroidType;
}


#ifdef WIN32
void	targetAdd(BASE_OBJECT *psObj)
{
	UNUSEDPARAMETER(psObj);
}
#else
// Call to add an object to the target list.
// Assumes the "psObj->sDisplay.screen?" values have been filled in.
//
void targetAdd(BASE_OBJECT *psObj)
{
	UWORD Type = 0;
	BOOL ObjTargetable;

	if(TargetingObject == NULL) {
		return;
	}

	if(psObj->sDisplay.frameNumber != currentGameFrame) {
		return;
	}

	// If it's behind the targeting object..
	if(psObj->sDisplay.screenY > TargetingObject->sDisplay.screenY) {
		// Then ignore it.
		return;
	}

	ObjTargetable = FALSE;

	// If it's allied with the player then ignore it.
	if( aiCheckAlliances(selectedPlayer,psObj->player) ) {
		return;
	}


	if(TargetingType == DROID_CONSTRUCT OR TargetingType == DROID_CYBORG_CONSTRUCT) {
		// Construction droids only target there own structures and certain features.
		if( ((psObj->type == OBJ_STRUCTURE) && (psObj->player == selectedPlayer)) ||
			(psObj->type == OBJ_FEATURE) ) {
			ObjTargetable = TRUE;

			if(psObj->type == OBJ_FEATURE) {
				FEATURE *psFeature = (FEATURE*)psObj;
				if( (psFeature->psStats->subType != FEAT_OIL_RESOURCE) &&
					(psFeature->psStats->subType != FEAT_GEN_ARTE) &&
					(psFeature->psStats->subType != FEAT_OIL_DRUM) ) {
					ObjTargetable = FALSE;
				}
			}

			if(psObj->type == OBJ_STRUCTURE) {
				STRUCTURE *psStruct = (STRUCTURE*)psObj;
				// Only target if damaged,not fully built,an upgrade is available or can demolish.
				if( (buildingDamaged(psStruct)) ||			// Damaged?
					(psStruct->status != SS_BUILT) ||		// Not fully built?
					(buildModule(NULL,psStruct,FALSE)) ||	// Upgrade available?
					(((DROID*)TargetingObject)->psTarStats == 
						(BASE_STATS*)structGetDemolishStat())) {	// Demolish available?

					ObjTargetable = TRUE;
				} else {
					ObjTargetable = FALSE;
				}
			}

		} else if( (psObj->type == OBJ_DROID) && (psObj->player == selectedPlayer) ) {
			// Construction droids can also target the transporter.
			if( ((DROID*)psObj)->droidType == DROID_TRANSPORTER) {
				ObjTargetable = TRUE;
			}
		}
	} else if(TargetingType == DROID_REPAIR OR
        TargetingType == DROID_CYBORG_REPAIR) {
		// Repair droids can only target other friendly droids that are damaged.
		if( (psObj != TargetingObject) && (psObj->type == OBJ_DROID) && (psObj->player == selectedPlayer)) {
			if(droidIsDamaged((DROID*)psObj)) {
				ObjTargetable = TRUE;
			}
		}
	} else if( (psObj->player != selectedPlayer) || (psObj->type == OBJ_FEATURE) ) {
		// All other droids can only target enemy strucures,droids or damagable features.
		ObjTargetable = TRUE;

		if(psObj->type == OBJ_FEATURE) {
			FEATURE *psFeature = (FEATURE*)psObj;
			if( (!psFeature->psStats->damageable) &&
				(psFeature->psStats->subType != FEAT_GEN_ARTE) &&
				(psFeature->psStats->subType != FEAT_OIL_DRUM) ) {
				ObjTargetable = FALSE;
			}
		}
	} else if( (psObj->type == OBJ_DROID) && (psObj->player == selectedPlayer) ) {
		// Any droid can target the transporter.
		if( ((DROID*)psObj)->droidType == DROID_TRANSPORTER) {
			ObjTargetable = TRUE;
		}
	}

//	// If it's not the selected player or it's a feature..
//	if( (psObj->player != selectedPlayer) || (psObj->type == OBJ_FEATURE) ) {
	if(ObjTargetable) {

		// If it's a feature..
		if(psObj->type == OBJ_FEATURE) {
			FEATURE *psFeature = (FEATURE*)psObj;
			// and it's not damagable..
			if(!psFeature->psStats->damageable) {
				// and it's not an artefact or oil resource..
				if( (psFeature->psStats->subType != FEAT_OIL_RESOURCE) &&
					(psFeature->psStats->subType != FEAT_GEN_ARTE) &&
					(psFeature->psStats->subType != FEAT_OIL_DRUM) ) {
					// then ignore it.
					return;
				}
			}

			if(psFeature->psStats->subType == FEAT_OIL_RESOURCE) {
				Type |= TARGET_TYPE_RESOURCE;
//				Type |= TARGET_TYPE_ARTIFACT;
			} else if(psFeature->psStats->subType == FEAT_GEN_ARTE) {
				Type |= TARGET_TYPE_ARTIFACT;
//				Type |= TARGET_TYPE_RESOURCE;
			} else if(psFeature->psStats->subType == FEAT_OIL_DRUM) {
				Type |= TARGET_TYPE_ARTIFACT;
			} else {
				Type |= TARGET_TYPE_FEATURE;
			}
		// If it's a structure..
		} else if(psObj->type == OBJ_STRUCTURE) {
			STRUCTURE *psStructure = (STRUCTURE*)psObj;
#ifndef TARGET_WALLS
			// and it's a wall..
			if( (psStructure->pStructureType->type == REF_WALL) ||
				(psStructure->pStructureType->type == REF_WALLCORNER) ) {
				// Ignore it.
				return;
			}
#endif
			if( (psStructure->pStructureType->type == REF_WALL) ||
				(psStructure->pStructureType->type == REF_WALLCORNER) ) {
				Type |= TARGET_TYPE_WALL;
			} else {
				Type |= TARGET_TYPE_STRUCTURE;
			}
			if(psStructure->player != selectedPlayer) {
				// Structure with weapons? Make it a threat.
				//if (psStructure->numWeaps > 0) {
                if (psStructure->asWeaps[0].nStat > 0) {
					Type |= TARGET_TYPE_THREAT;
				}
			} else {
				Type |= TARGET_TYPE_FRIEND;
			}
		// If it's a droid
		} else if(psObj->type == OBJ_DROID) {
			Type |= TARGET_TYPE_DROID;
			if(psObj->player != selectedPlayer) {
//			if( (psObj->player != selectedPlayer) && !aiCheckAlliances(selectedPlayer,psObj->player) ) {
				// Droid's are automaticly a threat!
				Type |= TARGET_TYPE_THREAT;
			} else {
				Type |= TARGET_TYPE_FRIEND;
			}
		}

		// If it's on screen then add it to the target list.
		if( (psObj->sDisplay.screenX < 640) && (psObj->sDisplay.screenY < 480) ) {
			// If no line of sight..
			if(!visibleObject(TargetingObject,psObj)) {
				// then ignore it.
				return;
			}

			if(NumTargets < MAX_TARGETS) {
				TargetList[NumTargets].Type = Type;
				TargetList[NumTargets].psObj = psObj;
//				if( (psObj->id == TargetCurrentID) || (TargetCurrentID == UDWORD_MAX) ) {
				if(psObj->id == TargetCurrentID) {
//					if(TargetCurrentID == UDWORD_MAX) {
//						targetStartAnim();
//					}
					TargetCurrent = NumTargets;
					TargetCurrentID = psObj->id;
//printf("set2 %d\n",TargetCurrentID);
					FoundCurrent = TRUE;
				}
				NumTargets++;
			} else {
				DBPRINTF(("Target list overflow\n"));
			}
		}
	}
}
#endif

// Call whenever an object is removed. probably don't need this as
// the list is rebuilt every frame any way.
//
void targetKilledObject(BASE_OBJECT *psObj)
{
	UNUSEDPARAMETER(psObj);
}


//// Aquire the target nearest to the specified screen coords.
////
//BASE_OBJECT *targetAquireNearestScreen(SWORD x,SWORD y,UWORD TargetType)
//{
//	UWORD i;
//	UWORD Nearesti = 0;
//	UDWORD NearestDsq = UDWORD_MAX;
//	UDWORD dx,dy;
//	UDWORD Dsq;
//	BASE_OBJECT *NearestObj = NULL;
//	BASE_OBJECT *psObj;
//
//	for(i=0; i<NumTargets; i++) {
//		psObj = TargetList[i].psObj;
//		dx = abs(psObj->sDisplay.screenX - x);
//		dy = abs(psObj->sDisplay.screenY - y);
//		Dsq = dx*dx+dy*dy;
//		if(Dsq < NearestDsq) {
//			if(TargetList[i].Type & TargetType) {
//				NearestDsq = Dsq;
//				Nearesti = i;
//				NearestObj = psObj;
//			}
//		}
//	}
//
//	if(NearestObj != NULL) {
//		TargetCurrent = Nearesti;
//		if(TargetCurrentID != NearestObj->id) {
//			TargetCurrentID = NearestObj->id;
//			targetStartAnim();
//		}
//	}
//
//	return NearestObj;
//}


// Aquire the target nearest the vector from x,y to the top of the screen.
//
BASE_OBJECT *targetAquireNearestView(SWORD x,SWORD y,UWORD TargetType)
{
	UWORD i;
	UWORD Nearesti = 0;
//	UDWORD NearestDsq = UDWORD_MAX;
	UDWORD NearestDx = UDWORD_MAX;
	UDWORD dx,dy;
//	UDWORD Dsq;
	BASE_OBJECT *NearestObj = NULL;
	BASE_OBJECT *psObj;

	for(i=0; i<NumTargets; i++) {
		psObj = TargetList[i].psObj;
		dx = abs(psObj->sDisplay.screenX - x);
		dy = abs(psObj->sDisplay.screenY - y);
//		Dsq = dx*dx+dy*dy*4;
		dx += dy/2;
		if(dx < NearestDx) {
			if(TargetList[i].Type & TargetType) {
				NearestDx = dx;
				Nearesti = i;
				NearestObj = psObj;
			}
		}

//		if(Dsq < NearestDsq) {
//			if(TargetList[i].Type & TargetType) {
//				NearestDsq = Dsq;
//				Nearesti = i;
//				NearestObj = psObj;
//			}
//		}
	}

	if(NearestObj != NULL) {
		TargetCurrent = Nearesti;
		if(TargetCurrentID != NearestObj->id) {
//printf("set1 %d\n",TargetCurrentID);
			TargetCurrentID = NearestObj->id;
			targetStartAnim();
		}
	} else {
		TargetCurrentID = UDWORD_MAX;
	}

	return NearestObj;
}


//// Aquire the target nearest to the specified object.
////
//BASE_OBJECT *targetAquireNearestObj(BASE_OBJECT *psObj,UWORD TargetType)
//{
//	if(psObj != NULL) {
//		return targetAquireNearestScreen(psObj->sDisplay.screenX,psObj->sDisplay.screenY,TargetType);
//	} else {
//		return NULL;
//	}
//}


// Aquire the target nearest to the specified object.
//
BASE_OBJECT *targetAquireNearestObjView(BASE_OBJECT *psObj,UWORD TargetType)
{
	if(psObj != NULL) {
		return targetAquireNearestView((SWORD)(psObj->sDisplay.screenX),
										(SWORD)(psObj->sDisplay.screenY)
										,TargetType);
	} else {
		return NULL;
	}
}


//// Aquire the next target in the target list.
////
//BASE_OBJECT *targetAquireNext(UWORD TargetType)
//{
//	BASE_OBJECT *Target = NULL;
//
//	if(NumTargets) {
//		UWORD OldCurrent = TargetCurrent;
//
//		TargetCurrent++;
//
//		while( (Target == NULL) && (TargetCurrent < NumTargets) ) {
//			// Target is of required type?
//			if(TargetList[TargetCurrent].Type & TargetType) {
//				Target = TargetList[TargetCurrent].psObj;
//			}
//
//			TargetCurrent++;
//		}
//
//
//		if(Target == NULL) {
//			TargetCurrent = 0;
//
//			while( (Target == NULL) && (TargetCurrent < OldCurrent) ) {
//				// Target is of required type?
//				if(TargetList[TargetCurrent].Type & TargetType) {
//					Target = TargetList[TargetCurrent].psObj;
//				}
//
//				TargetCurrent++;
//			}
//		}
//
//		if(TargetCurrent >= NumTargets) {
//			TargetCurrent = 0;
//		}
//	}
//
//	if(Target != NULL) {
//		if(TargetCurrentID != Target->id) {
//			TargetCurrentID = Target->id;
//			targetStartAnim();
//		}
//	}
//
//	return Target;
//}


// Get the currently targeted object.
//
BASE_OBJECT *targetGetCurrent(void)
{
	if(TargetCurrentID != UDWORD_MAX) {
		return TargetList[TargetCurrent].psObj;
	}

	return NULL;
}


// Start the box zoom animation.
//
void targetStartAnim(void)
{
	TargetEndTime = gameTime+GAME_TICKS_PER_SEC/2;
}


// Display a marker over the current target.
//
void targetMarkCurrent(void)
{
	SWORD x,y;
	SWORD Offset;
	SWORD x0,y0,x1,y1;

//printf("%d\n",TargetCurrentID);
	if(TargetCurrentID == UDWORD_MAX) {
		return;
	}

	x = (SWORD)(TargetList[TargetCurrent].psObj->sDisplay.screenX);
	y = (SWORD)(TargetList[TargetCurrent].psObj->sDisplay.screenY);

	// Make it zoom in.
	if(TargetEndTime > gameTime) {
		Offset =(SWORD)(16+(TargetEndTime-gameTime)/2);
	} else {
		Offset = 16;
	}

#ifdef PSX
	iV_SetOTIndex_PSX(OT2D_EXTREMEBACK);
#endif

	x0 = (SWORD)(x-Offset);
	y0 = (SWORD)(y-Offset);
	x1 = (SWORD)(x+Offset);
	y1 = (SWORD)(y+Offset);

	iV_Line(x0,y0,x0+8,y0,COL_YELLOW);
	iV_Line(x0,y0,x0,y0+8,COL_YELLOW);

	iV_Line(x1,y0,x1-8,y0,COL_YELLOW);
	iV_Line(x1,y0,x1,y0+8,COL_YELLOW);

	iV_Line(x1,y1,x1-8,y1,COL_YELLOW);
	iV_Line(x1,y1,x1,y1-8,COL_YELLOW);

	iV_Line(x0,y1,x0+8,y1,COL_YELLOW);
	iV_Line(x0,y1,x0,y1-8,COL_YELLOW);
}


