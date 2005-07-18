/*
 * MissionDef.h
 *
 * Structure definitions for Mission
 *
 */
#ifndef _missiondef_h
#define _missiondef_h

//mission types

//used to set the reinforcement time on hold whilst the Transporter is unable to land
//hopefully they'll never need to set it this high for other reasons!
#define SCR_LZ_COMPROMISED_TIME     999999
//this is used to compare the value passed in from the scripts with which is multiplied by 100
#define LZ_COMPROMISED_TIME         99999900        

typedef struct _landing_zone
{
	UBYTE	x1;
	UBYTE	y1;
	UBYTE	x2;
	UBYTE	y2;
} LANDING_ZONE;

//storage structure for values that need to be kept between missions
typedef struct _mission
{
	//MISSION_TYPE		type;							//defines which start and end functions to use
	UDWORD				type;							//defines which start and end functions to use - see levels_type in levels.h
	MAPTILE				*psMapTiles;					//the original mapTiles
	TILE_COORD			*aMapLinePoints;				//the original mapLinePoints
	UDWORD				mapWidth;						//the original mapWidth
	UDWORD				mapHeight;						//the original mapHeight
	struct _gateway		*psGateways;					//the gateway list
	UBYTE				**apRLEZones;					//the RLE map zones
	SDWORD				gwNumZones;						//the number of map zones
	UBYTE				*aNumEquiv;						//zone equivalence data
	UBYTE				**apEquivZones;
	UBYTE				*aZoneReachable;
	UDWORD				scrollMinX;						//scroll coords for original map
	UDWORD				scrollMinY;
	UDWORD				scrollMaxX;
	UDWORD				scrollMaxY;
	STRUCTURE					*apsStructLists[MAX_PLAYERS];	//original object lists
	DROID						*apsDroidLists[MAX_PLAYERS];
	FEATURE						*apsFeatureLists[MAX_PLAYERS];
	//struct _proximity_display	*apsProxDisp[MAX_PLAYERS];
	FLAG_POSITION				*apsFlagPosLists[MAX_PLAYERS];
	PLAYER_POWER				asPower[MAX_PLAYERS];

	UDWORD				startTime;			//time the mission started
	SDWORD				time;				//how long the mission can last
											// < 0 = no limit
	SDWORD				ETA;				//time taken for reinforcements to arrive
											// < 0 = none allowed
   	UDWORD				cheatTime;			//time the cheating started (mission time-wise!)

	//LANDING_ZONE		homeLZ;
    UWORD               homeLZ_X;           //selectedPlayer's LZ x and y
    UWORD               homeLZ_Y;
	SDWORD				playerX;			//original view position
	SDWORD				playerY;

	/* transporter entry/exit tiles */
	UWORD				iTranspEntryTileX[MAX_PLAYERS];
	UWORD				iTranspEntryTileY[MAX_PLAYERS];
	UWORD				iTranspExitTileX[MAX_PLAYERS];
	UWORD				iTranspExitTileY[MAX_PLAYERS];

} MISSION;

#endif
