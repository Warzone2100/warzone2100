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
 * droid.h
 *
 * Definitions for the droid object.
 *
 */
#ifndef _droid_h
#define _droid_h

#include "objectdef.h"
#include "lib/gamelib/gtime.h"

#define OFF_SCREEN 9999		// world->screen check - alex

#define REPAIRLEV_LOW	50	// percentage of body points remaining at which to repair droid automatically.
#define REPAIRLEV_HIGH	75	// ditto, but this will repair much sooner..

#define	DROID_EXPLOSION_SPREAD_X	(TILE_UNITS/2 - (rand()%TILE_UNITS))
#define	DROID_EXPLOSION_SPREAD_Y	(rand()%TILE_UNITS)
#define	DROID_EXPLOSION_SPREAD_Z	(TILE_UNITS/2 - (rand()%TILE_UNITS))

/*defines the % to decrease the illumination of a tile when building - gets set
back when building is destroyed*/
//#define FOUNDATION_ILLUMIN		50

#define DROID_RESISTANCE_FACTOR     30

#define MAX_RECYCLED_DROIDS		32

//storage
extern DROID_TEMPLATE			*apsDroidTemplates[MAX_PLAYERS];


/* The range for neighbouring objects */
#define NAYBOR_RANGE		(TILE_UNITS*9)	//range of lancer, BB, TK etc

//used to stop structures being built too near the edge and droids being placed down - pickATile
#define TOO_NEAR_EDGE	3

/* Info stored for each droid neighbour */
typedef struct _naybor_info
{
	BASE_OBJECT		*psObj;			// The neighbouring object
	UDWORD			distSqr;		// The square of the distance to the object
	//UDWORD			dist;			// The distance to the object
} NAYBOR_INFO;

typedef enum
{
	NO_FREE_TILE,
	FREE_TILE,
	HALF_FREE_TILE
} PICKTILE;

/* Store for the objects near the droid currently being updated */
#define MAX_NAYBORS		120
extern NAYBOR_INFO		asDroidNaybors[MAX_NAYBORS];
extern UDWORD			numNaybors;

// the structure that was last hit
extern DROID	*psLastDroidHit;

extern UWORD	aDroidExperience[MAX_PLAYERS][MAX_RECYCLED_DROIDS];


// initialise droid module
extern BOOL droidInit(void);

extern void	removeDroidBase(DROID *psDel);

// refresh the naybor list
// this only does anything if the naybor list is out of date
extern void droidGetNaybors(DROID *psDroid);

extern BOOL loadDroidTemplates(const char *pDroidData, UDWORD bufferSize);
extern BOOL loadDroidWeapons(const char *pWeaponData, UDWORD bufferSize);

/*initialise the template build and power points */
extern void initTemplatePoints(void);

/*Builds an instance of a Structure - the x/y passed in are in world coords.*/
extern DROID* buildDroid(DROID_TEMPLATE *pTemplate, UDWORD x, UDWORD y,
						 UDWORD player, BOOL onMission);

/* Set the asBits in a DROID structure given it's template. */
extern void droidSetBits(DROID_TEMPLATE *pTemplate,DROID *psDroid);

// calculate the experience level of a droid
extern SDWORD droidCalcExp(DROID *psDroid);

/* Calculate the weight of a droid from it's template */
extern UDWORD calcDroidWeight(DROID_TEMPLATE *psTemplate);

/* Calculate the power points required to build/maintain a droid */
extern UDWORD calcDroidPower(DROID *psDroid);

/* Calculate the body points of a droid from it's template */
extern UDWORD calcTemplateBody(DROID_TEMPLATE *psTemplate, UBYTE player);

/* Calculate the base body points of a droid without upgrades*/
extern UDWORD calcDroidBaseBody(DROID *psDroid);

/* Calculate the base speed of a droid from it's template */
extern UDWORD calcDroidBaseSpeed(DROID_TEMPLATE *psTemplate, UDWORD weight, UBYTE player);

/* Calculate the speed of a droid over a terrain */
extern UDWORD calcDroidSpeed(UDWORD baseSpeed, UDWORD terrainType, UDWORD propIndex);

/* Calculate the points required to build the template */
extern UDWORD calcTemplateBuild(DROID_TEMPLATE *psTemplate);

/* Calculate the power points required to build/maintain the droid */
extern UDWORD	calcTemplatePower(DROID_TEMPLATE *psTemplate);

// return whether a template is for an IDF droid
BOOL templateIsIDF(DROID_TEMPLATE *psTemplate);

// return whether a droid is IDF
BOOL idfDroid(DROID *psDroid);

/* Do damage to a droid */

//Watermelon:added int angle,no need to use float since the difference between them is superficial.
extern BOOL droidDamage(DROID *psDroid, UDWORD damage, UDWORD weaponClass,UDWORD weaponSubClass, int angle);


/* The main update routine for all droids */
extern void droidUpdate(DROID *psDroid);

/* Set up a droid to build a structure - returns true if successful */
extern BOOL droidStartBuild(DROID *psDroid);

/* Sets a droid to start demolishing - returns true if successful */
extern BOOL	droidStartDemolishing( DROID *psDroid );

/* Update a construction droid while it is demolishing
   returns TRUE while demolishing */
extern BOOL	droidUpdateDemolishing( DROID *psDroid );

/* Sets a droid to start repairing - returns true if successful */
extern BOOL	droidStartRepair( DROID *psDroid );

/* Update a construction droid while it is repairing
   returns TRUE while repairing */
extern BOOL	droidUpdateRepair( DROID *psDroid );

/*Start a Repair Droid working on a damaged droid - returns TRUE if successful*/
extern BOOL droidStartDroidRepair( DROID *psDroid );

/*Updates a Repair Droid working on a damaged droid - returns TRUE whilst repairing*/
extern BOOL droidUpdateDroidRepair(DROID *psRepairDroid);

/*checks a droids current body points to see if need to self repair*/
extern void droidSelfRepair(DROID *psDroid);

/* Update a construction droid while it is building
   returns TRUE while building continues */
extern BOOL droidUpdateBuild(DROID *psDroid);

/*Start a EW weapon droid working on a low resistance structure*/
extern BOOL droidStartRestore( DROID *psDroid );

/*continue restoring a structure*/
extern BOOL droidUpdateRestore( DROID *psDroid );

// recycle a droid (retain it's experience and some of it's cost)
extern void recycleDroid(DROID *psDel);

/* Release all resources associated with a droid */
extern void droidRelease(DROID *psDroid);

/* Remove a droid and free it's memory */
extern void destroyDroid(DROID *psDel);

/* Same as destroy droid except no graphical effects */
extern void	vanishDroid(DROID *psDel);

/* Burn a barbarian then destroy it */
extern void droidBurn( DROID * psDroid );

/* Remove a droid from the apsDroidLists so doesn't update or get drawn etc*/
//returns TRUE if successfully removed from the list
extern BOOL droidRemove(DROID *psDroid, DROID *pList[MAX_PLAYERS]);

//free the storage for the droid templates
extern BOOL droidTemplateShutDown(void);

/* Return the type of a droid */
extern DROID_TYPE droidType(DROID *psDroid);

/* Return the type of a droid from it's template */
extern DROID_TYPE droidTemplateType(DROID_TEMPLATE *psTemplate);

//fills the list with Templates that can be manufactured in the Factory - based on size
extern UDWORD fillTemplateList(DROID_TEMPLATE **pList, STRUCTURE *psFactory, UDWORD limit);

extern void assignDroidsToGroup(UDWORD	playerNumber, UDWORD groupNumber);

extern BOOL activateGroup(UDWORD playerNumber, UDWORD groupNumber);

extern UDWORD	getNumDroidsForLevel(UDWORD	level);

extern BOOL activateGroupAndMove(UDWORD playerNumber, UDWORD groupNumber);
/* calculate muzzle tip location in 3d world added int weapon_slot to fix the always slot 0 hack*/
extern BOOL calcDroidMuzzleLocation(DROID *psDroid, Vector3i *muzzle, int weapon_slot);

/* gets a template from its name - relies on the name being unique */
extern DROID_TEMPLATE* getTemplateFromName(char *pName);
/*getTemplateFromSinglePlayerID gets template for unique ID  searching one players list */
extern DROID_TEMPLATE* getTemplateFromSinglePlayerID(UDWORD multiPlayerID, UDWORD player);
/*getTemplateFromMultiPlayerID gets template for unique ID  searching all lists */
extern DROID_TEMPLATE* getTemplateFromMultiPlayerID(UDWORD multiPlayerID);

// finds a droid for the player and sets it to be the current selected droid
extern BOOL selectDroidByID(UDWORD id, UDWORD player);

/* Droid experience stuff */
extern UDWORD	getDroidLevel(DROID *psDroid);
extern const char *getDroidLevelName(DROID *psDroid);

// Get a droid's name.
extern char *droidGetName(DROID *psDroid);

// Set a droid's name.
extern void droidSetName(DROID *psDroid, const char *pName);

// Set a templates name.
extern void templateSetName(DROID_TEMPLATE *psTemplate,char *pName);

// returns true when no droid on x,y square.
extern BOOL	noDroid					(UDWORD x, UDWORD y);				// true if no droid at x,y
// returns true if it's a sensible place to put that droid.
extern BOOL	sensiblePlace			(SDWORD x, SDWORD y);				// true if x,y is an ok place
// returns an x/y coord to place a droid
extern BOOL pickATile				(UDWORD *x0,UDWORD *y0, UBYTE numIterations);
extern PICKTILE pickHalfATile		(UDWORD *x, UDWORD *y, UBYTE numIterations);
extern BOOL	pickATile2				(UDWORD *x, UDWORD *y, UDWORD numIterations);
extern	BOOL	normalPAT(UDWORD x, UDWORD y);
extern	BOOL	zonedPAT(UDWORD x, UDWORD y);
extern	BOOL	pickATileGen(UDWORD *x, UDWORD *y, UBYTE numIterations,
					 BOOL (*function)(UDWORD x, UDWORD y));
extern	BOOL	pickATileGenThreat(UDWORD *x, UDWORD *y, UBYTE numIterations, SDWORD threatRange,
					 SDWORD player, BOOL (*function)(UDWORD x, UDWORD y));


//initialises the droid movement model
extern void initDroidMovement(DROID *psDroid);

/* Looks through the players list of droids to see if any of them are
building the specified structure - returns TRUE if finds one*/
extern BOOL checkDroidsBuilding(STRUCTURE *psStructure);

/* Looks through the players list of droids to see if any of them are
demolishing the specified structure - returns TRUE if finds one*/
extern BOOL checkDroidsDemolishing(STRUCTURE *psStructure);

/* checks the structure for type and capacity and orders the droid to build
a module if it can - returns TRUE if order is set */
extern BOOL buildModule(DROID *psDroid, STRUCTURE *psStruct,BOOL bCheckPower);

/*Deals with building a module - checking if any droid is currently doing this
 - if so, helping to build the current one*/
extern void setUpBuildModule(DROID *psDroid);

/*return the name to display for the interface given a DROID structure*/
extern char* getDroidName(DROID *psDroid);

/*return the name to display for the interface - we don't know if this is
a string ID or something the user types in*/
extern char* getTemplateName(DROID_TEMPLATE *psTemplate);

/* Just returns true if the droid's present body points aren't as high as the original*/
extern BOOL	droidIsDamaged(DROID *psDroid);

/* Returns currently active (selected) group */
extern UDWORD	getSelectedGroup( void );
extern void	setSelectedGroup(UDWORD groupNumber);
extern UDWORD	getSelectedCommander( void );
extern void	setSelectedCommander(UDWORD commander);


extern BOOL getDroidResourceName(char *pName);


/*checks to see if an electronic warfare weapon is attached to the droid*/
extern BOOL electronicDroid(DROID *psDroid);

/*checks to see if the droid is currently being repaired by another*/
extern BOOL droidUnderRepair(DROID *psDroid);

//count how many Command Droids exist in the world at any one moment
extern UBYTE checkCommandExist(UBYTE player);

/* Set up a droid to clear a wrecked building feature - returns true if successful */
extern BOOL droidStartClearing( DROID *psDroid );
/* Update a construction droid while it is clearing
   returns TRUE while continues */
extern BOOL droidUpdateClearing( DROID *psDroid );

/*For a given repair droid, check if there are any damaged droids within
a defined range*/
extern BASE_OBJECT * checkForRepairRange(DROID *psDroid,DROID *psTarget);

//access function
extern BOOL vtolDroid(DROID *psDroid);
/*returns TRUE if a VTOL Weapon Droid which has completed all runs*/
extern BOOL vtolEmpty(DROID *psDroid);
/*Checks a vtol for being fully armed and fully repaired to see if ready to
leave reArm pad */
extern BOOL  vtolHappy(DROID *psDroid);
/*this mends the VTOL when it has been returned to home base whilst on an
offworld mission*/
extern void mendVtol(DROID *psDroid);
/*checks if the droid is a VTOL droid and updates the attack runs as required*/
extern void updateVtolAttackRun(DROID *psDroid, int weapon_slot);
/*returns a count of the base number of attack runs for the weapon attached to the droid*/
extern UWORD   getNumAttackRuns(DROID *psDroid, int weapon_slot);
//assign rearmPad to the VTOL
extern void assignVTOLPad(DROID *psNewDroid, STRUCTURE *psReArmPad);
// true if a vtol is waiting to be rearmed by a particular rearm pad
extern BOOL vtolReadyToRearm(DROID *psDroid, STRUCTURE *psStruct);
// true if a vtol droid currently returning to be rearmed
extern BOOL vtolRearming(DROID *psDroid);
// true if a droid is currently attacking
extern BOOL droidAttacking(DROID *psDroid);
// see if there are any other vtols attacking the same target
// but still rearming
extern BOOL allVtolsRearmed(DROID *psDroid);

/*compares the droid sensor type with the droid weapon type to see if the
FIRE_SUPPORT order can be assigned*/
extern BOOL droidSensorDroidWeapon(BASE_OBJECT *psObj, DROID *psDroid);

// return whether a droid has a CB sensor on it
extern BOOL cbSensorDroid(DROID *psDroid);

// give a droid from one player to another - used in Electronic Warfare and multiplayer
extern DROID * giftSingleDroid(DROID *psD, UDWORD to);
/*calculates the electronic resistance of a droid based on its experience level*/
extern SWORD   droidResistance(DROID *psDroid);

/*this is called to check the weapon is 'allowed'. Check if VTOL, the weapon is
direct fire. Also check numVTOLattackRuns for the weapon is not zero - return
TRUE if valid weapon*/
extern BOOL checkValidWeaponForProp(DROID_TEMPLATE *psTemplate);

extern const char *getDroidNameForRank(UDWORD rank);

/*called when a Template is deleted in the Design screen*/
extern void deleteTemplateFromProduction(DROID_TEMPLATE *psTemplate, UBYTE player);

// Select a droid and do any necessary housekeeping.
extern void SelectDroid(DROID *psDroid);

// De-select a droid and do any necessary housekeeping.
extern void DeSelectDroid(DROID *psDroid);

/*calculate the power cost to repair a droid*/
extern UWORD powerReqForDroidRepair(DROID *psDroid);

/*power cost for One repair point*/
extern UWORD repairPowerPoint(DROID *psDroid);

/* audio finished callback */
extern BOOL droidAudioTrackStopped( void *psObj );

/*returns TRUE if droid type is one of the Cyborg types*/
extern BOOL cyborgDroid(DROID *psDroid);

/* assert if droid is bad */
#ifdef DEBUG
#define CHECK_DROID(droid) \
do { \
	unsigned int i; \
\
	assert(droid != NULL); \
\
	/* Make sure to know we actually have a droid in our hands */ \
	assert(droid->type == OBJ_DROID); \
\
	assert(droid->direction <= 360.0f && droid->direction >= 0.0f); \
	assert(droid->numWeaps < DROID_MAXWEAPS); \
	assert(droid->listSize < ORDER_LIST_MAX); \
\
	for (i = 0; i < DROID_MAXWEAPS; ++i) \
		assert(droid->turretRotation[i] <= 360); \
\
	for (i = 0; i < DROID_MAXWEAPS; ++i) \
		assert(droid->asWeaps[i].lastFired <= gameTime); \
\
	for (i = 0; i < DROID_MAXWEAPS; ++i) \
		if (droid->psActionTarget[i]) \
			assert(droid->psActionTarget[i]->direction >= 0.0f); \
} while (0);
#else
#define CHECK_DROID(x)
#endif

#endif
