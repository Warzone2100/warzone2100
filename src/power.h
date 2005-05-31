/*
 * Power.h
 *
 * Definitions for the Power Functionality.
 *
 */
#ifndef _power_h
#define _power_h

#define OILDRUM_POWER		100			// free power on collection of oildrum.

#define	POWER_PER_CYCLE		5

#define REPAIR_POWER_FACTOR 1/5          //% used to determine the power cost of repairing a droid
                                         //definately DON'T WANT the brackets round 1/2 - it will equate to zero!
#define POWER_FACTOR        100          //used to multiply all repair calculations by to avaoid rounding errors

//OLD PLAYER_POWER
//typedef struct _player_power
//{
//	UDWORD		availablePower;		/* quantity that can be used from the Generators */
//	UDWORD		extractedPower;		/* quantity being extracted but not converted 
//									   by a Generator */
//	SDWORD		capacity;			/* the spare capacity of the generators */
//	SDWORD		usedPower;			/* quantity currently being used */
//} PLAYER_POWER;


//NEW PLAYER_POWER
typedef struct _player_power
{
	//UDWORD		initialPower;	    HAVEN'T FOUND A USE FOR IT YET! AB 26/8/98	
									    /* what the initial power level is - set 
									    in script not sure if will need it, but 
									    keeping for now*/
	UDWORD		currentPower;		    /* the current amount of power avaialble 
									    to the player*/
	UDWORD		extractedPower;		    /* the power extracted but not converted */
	struct _base_object *psLastPowered;	/*the last object that received power 
									    before it ran out*/
} PLAYER_POWER;

/*allocate the space for the playerPower*/
extern BOOL allocPlayerPower(void);

/*clear the playerPower */
extern void clearPlayerPower(void);

/*initialise the PlayerPower list */
//extern initPlayerPower(void);

/*reset the power levels when a power_gen or resource_extractor is destroyed */
extern BOOL resetPlayerPower(UDWORD player, STRUCTURE *psStruct);

/*Free the space used for playerPower */
extern void releasePlayerPower(void);

/*check the available power*/
extern BOOL checkPower(UDWORD player, UDWORD quantity, BOOL playAudio);

/*subtract the power required */
extern BOOL usePower(UDWORD player, UDWORD quantity);

//return the power when a structure/droid is deliberately destroyed
extern void addPower(UDWORD player, UDWORD quantity);

/* Each Resource Extractor yields EXTRACT_POINTS per second until there are none
   left in the resource. */
//extern void updateExtractedPower(STRUCTURE	*psBuilding);
extern UDWORD updateExtractedPower(STRUCTURE	*psBuilding);

/* Update current power based on what was extracted during the last cycle and 
   what Power Generators exist */
extern void updatePlayerPower(UDWORD player);

// used in multiplayer to force power levels.
extern void setPower(UDWORD player, UDWORD avail); 

/*resets the power levels for all players when power is turned back on*/
void powerCalc(BOOL on);

/*sets the initial value for the power*/
void setPlayerPower(UDWORD power, UDWORD player);

/*Temp function to give all players some power when a new game has been loaded*/
void newGameInitPower(void);

//informs the power array that a Object has been destroyed
extern void powerDestroyObject(BASE_OBJECT *psObject);

//extern void spreadPower(UBYTE player);
/*accrue the power in the facilities that require it*/
extern BOOL accruePower(BASE_OBJECT *psObject);
//returns the relevant list based on OffWorld or OnWorld for the accruePower function
//extern STRUCTURE* powerUpdateStructList(UBYTE player);

/*inform the players power struct that the last Object to receive power has changed*/
extern void updateLastPowered(BASE_OBJECT *psObject,UBYTE player);

/*	Returns the next res. Ext. in the list from the one passed in. returns 1st one
	in list if passed in is NULL and NULL if there's none?
*/
extern STRUCTURE *getRExtractor(STRUCTURE *psStruct);

/*checks if the object to be powered next - returns TRUE if power*/
extern BOOL getLastPowered(BASE_OBJECT *psStructure);

/*defines which structure types draw power - returns TRUE if use power*/
extern BOOL structUsesPower(STRUCTURE *psStruct);
/*defines which droid types draw power - returns TRUE if use power*/
extern BOOL droidUsesPower(DROID *psDroid);

//won't bother with this on PSX unless starts being used too much!

//this is a check cos there is a problem with the power but not sure where!!
extern void powerCheck(BOOL bBeforePowerUsed, UBYTE player);


extern PLAYER_POWER		*asPower[MAX_PLAYERS];

//flag used to check for power calculations to be done or not
extern	BOOL			powerCalculated;


//Disused Functions...
/*Update the extracted power if necessary */
//extern void extractedPowerUpdate(STRUCTURE *psBuilding);

/*update the generator capacity if necessary */
//extern void capacityUpdate(STRUCTURE * psBuilding);

#endif //power.h
