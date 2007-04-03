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

/* Standard library headers */
#include <physfs.h>
#include <string.h>

/* Warzone src and library headers */
#include "lib/framework/frame.h"
#include "lib/framework/strres.h"
#include "lib/framework/frameint.h"
#include "lib/ivis_common/ivisdef.h"
#include "lib/ivis_common/rendmode.h"
#include "lib/ivis_common/piestate.h"
#include "lib/script/script.h"
#include "lib/gamelib/gtime.h"
#include "map.h"
#include "edit2d.h"
#include "droid.h"
#include "action.h"
#include "game.h"
#include "research.h"
#include "power.h"
#include "player.h"
#include "projectile.h"
#include "loadsave.h"
#include "text.h"
#include "message.h"
#include "hci.h"
#include "display.h"
#include "display3d.h"
#include "map.h"
#include "effects.h"
#include "init.h"
#include "mission.h"
#include "scores.h"
#include "lib/sound/audio_id.h"
#include "anim_id.h"
#include "design.h"
#include "lighting.h"
#include "component.h"
#include "radar.h"
#include "cmddroid.h"
#include "formation.h"
#include "formationdef.h"
#include "warzoneconfig.h"
#include "multiplay.h"
#include "lib/netplay/netplay.h"
#include "frontend.h"
#include "levels.h"
#include "mission.h"
#include "geometry.h"
#include "lib/sound/audio.h"
#include "gateway.h"
#include "scripttabs.h"
#include "scriptextern.h"
#include "multistat.h"
#include "wrappers.h"


#define ALLOWSAVE
#define MAX_SAVE_NAME_SIZE_V19	40
#define MAX_SAVE_NAME_SIZE	60

#if (MAX_NAME_SIZE > MAX_SAVE_NAME_SIZE)
#error warning the current MAX_NAME_SIZE is to big for the save game
#endif

#define MAX_BODY			SWORD_MAX
#define SAVEKEY_ONMISSION	0x100

/**
 * The code is reusing some pointers as normal integer values apparently. This
 * should be fixed!
 */
#define FIXME_CAST_ASSIGN(TYPE, lval, rval) { TYPE* __tmp = (TYPE*) &lval; *__tmp = rval; }

UDWORD RemapPlayerNumber(UDWORD OldNumber);

typedef struct _game_save_header
{
	char		aFileType[4];
	UDWORD		version;
} GAME_SAVEHEADER;

typedef struct _droid_save_header
{
	char		aFileType[4];
	UDWORD		version;
	UDWORD		quantity;
} DROID_SAVEHEADER;

typedef struct _struct_save_header
{
	char		aFileType[4];
	UDWORD		version;
	UDWORD		quantity;
} STRUCT_SAVEHEADER;

typedef struct _template_save_header
{
	char		aFileType[4];
	UDWORD		version;
	UDWORD		quantity;
} TEMPLATE_SAVEHEADER;

typedef struct _feature_save_header
{
	char		aFileType[4];
	UDWORD		version;
	UDWORD		quantity;
} FEATURE_SAVEHEADER;

/* Structure definitions for loading and saving map data */
typedef struct {
	char aFileType[4];
	UDWORD version;
	UDWORD quantity;
} TILETYPE_SAVEHEADER;

/* Structure definitions for loading and saving map data */
typedef struct _compList_save_header
{
	char aFileType[4];
	UDWORD version;
	UDWORD quantity;
} COMPLIST_SAVEHEADER;

/* Structure definitions for loading and saving map data */
typedef struct _structList_save_header
{
	char aFileType[4];
	UDWORD version;
	UDWORD quantity;
} STRUCTLIST_SAVEHEADER;

typedef struct _research_save_header
{
	char aFileType[4];
	UDWORD version;
	UDWORD quantity;
} RESEARCH_SAVEHEADER;

typedef struct _message_save_header
{
	char aFileType[4];
	UDWORD version;
	UDWORD quantity;
} MESSAGE_SAVEHEADER;

typedef struct _proximity_save_header
{
	char aFileType[4];
	UDWORD version;
	UDWORD quantity;
} PROXIMITY_SAVEHEADER;

typedef struct _flag_save_header
{
	char aFileType[4];
	UDWORD version;
	UDWORD quantity;
} FLAG_SAVEHEADER;

typedef struct _production_save_header
{
	char aFileType[4];
	UDWORD version;
} PRODUCTION_SAVEHEADER;

typedef struct _structLimits_save_header
{
	char		aFileType[4];
	UDWORD		version;
	UDWORD		quantity;
} STRUCTLIMITS_SAVEHEADER;

typedef struct _command_save_header
{
	char aFileType[4];
	UDWORD version;
	UDWORD quantity;
} COMMAND_SAVEHEADER;

/* Sanity check definitions for the save struct file sizes */
#define GAME_HEADER_SIZE			8
#define DROID_HEADER_SIZE			12
#define DROIDINIT_HEADER_SIZE		12
#define STRUCT_HEADER_SIZE			12
#define TEMPLATE_HEADER_SIZE		12
#define FEATURE_HEADER_SIZE			12
#define TILETYPE_HEADER_SIZE		12
#define COMPLIST_HEADER_SIZE		12
#define STRUCTLIST_HEADER_SIZE		12
#define RESEARCH_HEADER_SIZE		12
#define MESSAGE_HEADER_SIZE			12
#define PROXIMITY_HEADER_SIZE		12
#define FLAG_HEADER_SIZE			12
#define PRODUCTION_HEADER_SIZE		8
#define STRUCTLIMITS_HEADER_SIZE	12
#define COMMAND_HEADER_SIZE			12


// general save definitions
#define MAX_LEVEL_SIZE 20

#define OBJECT_SAVE_V19 \
	char				name[MAX_SAVE_NAME_SIZE_V19]; \
	UDWORD				id; \
	UDWORD				x,y,z; \
	UDWORD				direction; \
	UDWORD				player; \
	BOOL				inFire; \
	UDWORD				burnStart; \
	UDWORD				burnDamage

#define OBJECT_SAVE_V20 \
	char				name[MAX_SAVE_NAME_SIZE]; \
	UDWORD				id; \
	UDWORD				x,y,z; \
	UDWORD				direction; \
	UDWORD				player; \
	BOOL				inFire; \
	UDWORD				burnStart; \
	UDWORD				burnDamage


typedef struct _save_component_v19
{
	char				name[MAX_SAVE_NAME_SIZE_V19];
} SAVE_COMPONENT_V19;

typedef struct _save_component
{
	char				name[MAX_SAVE_NAME_SIZE];
} SAVE_COMPONENT;

typedef struct _save_weapon_v19
{
	char				name[MAX_SAVE_NAME_SIZE_V19];
	UDWORD				hitPoints;  //- remove at some point
	UDWORD				ammo;
	UDWORD				lastFired;
} SAVE_WEAPON_V19;

typedef struct _save_weapon
{
	char				name[MAX_SAVE_NAME_SIZE];
	UDWORD				hitPoints;  //- remove at some point
	UDWORD				ammo;
	UDWORD				lastFired;
} SAVE_WEAPON;

typedef struct _savePower
{
	UDWORD		currentPower;
	UDWORD		extractedPower;
} SAVE_POWER;


#define GAME_SAVE_V7	\
	UDWORD	gameTime;	\
	UDWORD	GameType;		/* Type of game , one of the GTYPE_... enums. */ \
	SDWORD	ScrollMinX;		/* Scroll Limits */ \
	SDWORD	ScrollMinY; \
	UDWORD	ScrollMaxX; \
	UDWORD	ScrollMaxY; \
	char	levelName[MAX_LEVEL_SIZE]	//name of the level to load up when mid game

typedef struct save_game_v7
{
	GAME_SAVE_V7;
} SAVE_GAME_V7;

#define GAME_SAVE_V10	\
	GAME_SAVE_V7;		\
	SAVE_POWER	power[MAX_PLAYERS]

typedef struct save_game_v10
{
	GAME_SAVE_V10;
} SAVE_GAME_V10;

#define GAME_SAVE_V11	\
	GAME_SAVE_V10;		\
	iView currentPlayerPos

typedef struct save_game_v11
{
	GAME_SAVE_V11;
} SAVE_GAME_V11;

#define GAME_SAVE_V12	\
	GAME_SAVE_V11;		\
	UDWORD	missionTime;\
	UDWORD	saveKey

typedef struct save_game_v12
{
	GAME_SAVE_V12;
} SAVE_GAME_V12;

#define GAME_SAVE_V14			\
	GAME_SAVE_V12;				\
	SDWORD	missionOffTime;		\
	SDWORD	missionETA;			\
    UWORD   missionHomeLZ_X;	\
    UWORD   missionHomeLZ_Y;	\
	SDWORD	missionPlayerX;		\
	SDWORD	missionPlayerY;		\
	UWORD	iTranspEntryTileX[MAX_PLAYERS];	\
	UWORD	iTranspEntryTileY[MAX_PLAYERS];	\
	UWORD	iTranspExitTileX[MAX_PLAYERS];	\
	UWORD	iTranspExitTileY[MAX_PLAYERS];	\
	UDWORD 	aDefaultSensor[MAX_PLAYERS];	\
	UDWORD	aDefaultECM[MAX_PLAYERS];		\
	UDWORD	aDefaultRepair[MAX_PLAYERS]

typedef struct save_game_v14
{
	GAME_SAVE_V14;
} SAVE_GAME_V14;

#define GAME_SAVE_V15			\
	GAME_SAVE_V14;				\
	BOOL	offWorldKeepLists;\
	UBYTE	aDroidExperience[MAX_PLAYERS][MAX_RECYCLED_DROIDS];\
	UDWORD	RubbleTile;\
	UDWORD	WaterTile;\
	UDWORD	fogColour;\
	UDWORD	fogState

typedef struct save_game_v15
{
	GAME_SAVE_V15;
} SAVE_GAME_V15;

#define GAME_SAVE_V16			\
	GAME_SAVE_V15;				\
	LANDING_ZONE   sLandingZone[MAX_NOGO_AREAS]

typedef struct save_game_v16
{
	GAME_SAVE_V16;
} SAVE_GAME_V16;

#define GAME_SAVE_V17			\
	GAME_SAVE_V16;				\
	UDWORD   objId

typedef struct save_game_v17
{
	GAME_SAVE_V17;
} SAVE_GAME_V17;

#define GAME_SAVE_V18			\
	GAME_SAVE_V17;				\
	char		buildDate[MAX_STR_LENGTH];		\
	UDWORD		oldestVersion;	\
	UDWORD		validityKey

typedef struct save_game_v18
{
	GAME_SAVE_V18;
} SAVE_GAME_V18;

#define GAME_SAVE_V19			\
	GAME_SAVE_V18;				\
	UBYTE alliances[MAX_PLAYERS][MAX_PLAYERS];\
	UBYTE playerColour[MAX_PLAYERS];\
	UBYTE radarZoom

typedef struct save_game_v19
{
	GAME_SAVE_V19;
} SAVE_GAME_V19;

#define GAME_SAVE_V20			\
	GAME_SAVE_V19;				\
	UBYTE bDroidsToSafetyFlag;	\
	POINT	asVTOLReturnPos[MAX_PLAYERS]

typedef struct save_game_v20
{
	GAME_SAVE_V20;
} SAVE_GAME_V20;

#define GAME_SAVE_V22			\
	GAME_SAVE_V20;				\
	RUN_DATA	asRunData[MAX_PLAYERS]

typedef struct save_game_v22
{
	GAME_SAVE_V22;
} SAVE_GAME_V22;

#define GAME_SAVE_V24			\
	GAME_SAVE_V22;				\
	UDWORD reinforceTime;		\
	UBYTE bPlayCountDown;	\
	UBYTE bPlayerHasWon;	\
	UBYTE bPlayerHasLost;	\
	UBYTE dummy3

typedef struct save_game_v24
{
	GAME_SAVE_V24;
} SAVE_GAME_V24;

/*
#define GAME_SAVE_V27		\
	UDWORD	gameTime;		\
	UDWORD	GameType;		\
	SDWORD	ScrollMinX;		\
	SDWORD	ScrollMinY;		\
	UDWORD	ScrollMaxX;		\
	UDWORD	ScrollMaxY;		\
	char	levelName[MAX_LEVEL_SIZE];	\
	SAVE_POWER	power[MAX_PLAYERS];		\
	iView	currentPlayerPos;	\
	UDWORD	missionTime;	\
	UDWORD	saveKey;		\
	SDWORD	missionOffTime;	\
	SDWORD	missionETA;		\
    UWORD   missionHomeLZ_X;\
    UWORD   missionHomeLZ_Y;\
	SDWORD	missionPlayerX;	\
	SDWORD	missionPlayerY;	\
	UWORD	iTranspEntryTileX[MAX_PLAYERS];	\
	UWORD	iTranspEntryTileY[MAX_PLAYERS];	\
	UWORD	iTranspExitTileX[MAX_PLAYERS];	\
	UWORD	iTranspExitTileY[MAX_PLAYERS];	\
	UDWORD 	aDefaultSensor[MAX_PLAYERS];	\
	UDWORD	aDefaultECM[MAX_PLAYERS];		\
	UDWORD	aDefaultRepair[MAX_PLAYERS];	\
	BOOL	offWorldKeepLists;\
	UWORD	aDroidExperience[MAX_PLAYERS][MAX_RECYCLED_DROIDS];\
	UDWORD	RubbleTile;		\
	UDWORD	WaterTile;		\
	UDWORD	fogColour;		\
	UDWORD	fogState;		\
	LANDING_ZONE   sLandingZone[MAX_NOGO_AREAS];\
	UDWORD  objId;			\
	char	buildDate[MAX_STR_LENGTH];		\
	UDWORD	oldestVersion;	\
	UDWORD	validityKey;\
	UBYTE	alliances[MAX_PLAYERS][MAX_PLAYERS];\
	UBYTE	playerColour[MAX_PLAYERS];\
	UBYTE	radarZoom;		\
	UBYTE	bDroidsToSafetyFlag;	\
	POINT	asVTOLReturnPos[MAX_PLAYERS];\
	RUN_DATA asRunData[MAX_PLAYERS];\
	UDWORD	reinforceTime;	\
	UBYTE	bPlayCountDown;	\
	UBYTE	bPlayerHasWon;	\
	UBYTE	bPlayerHasLost;	\
	UBYTE	dummy3

typedef struct save_game_v27
{
	GAME_SAVE_V27;
} SAVE_GAME_V27;
*/
#define GAME_SAVE_V27			\
	GAME_SAVE_V24;				\
	UWORD	awDroidExperience[MAX_PLAYERS][MAX_RECYCLED_DROIDS]

typedef struct save_game_v27
{
	GAME_SAVE_V27;
} SAVE_GAME_V27;

#define GAME_SAVE_V29			\
	GAME_SAVE_V27;				\
	UWORD	missionScrollMinX;  \
	UWORD	missionScrollMinY;  \
	UWORD	missionScrollMaxX;  \
	UWORD	missionScrollMaxY

typedef struct save_game_v29
{
	GAME_SAVE_V29;
} SAVE_GAME_V29;

#define GAME_SAVE_V30			\
	GAME_SAVE_V29;				\
    SDWORD  scrGameLevel;       \
    UBYTE   bExtraVictoryFlag;  \
    UBYTE   bExtraFailFlag;     \
    UBYTE   bTrackTransporter

typedef struct save_game_v30
{
	GAME_SAVE_V30;
} SAVE_GAME_V30;

//extra code for the patch - saves out whether cheated with the mission timer
#define GAME_SAVE_V31           \
    GAME_SAVE_V30;				\
    SDWORD	missionCheatTime

typedef struct save_game_v31
{
	GAME_SAVE_V31;
} SAVE_GAME_V31;


// alexl. skirmish saves
#define GAME_SAVE_V33           \
    GAME_SAVE_V31;				\
	MULTIPLAYERGAME sGame;		\
	NETPLAY			sNetPlay;	\
	UDWORD			savePlayer;	\
	char			sPName[32];	\
	BOOL			multiPlayer;\
	UDWORD			sPlayer2dpid[MAX_PLAYERS]

typedef struct save_game_v33
{
	GAME_SAVE_V33;
} SAVE_GAME_V33;

#define GAME_SAVE_V34           \
    GAME_SAVE_V33;				\
	char		sPlayerName[MAX_PLAYERS][StringSize]

//Now holds AI names for multiplayer
typedef struct save_game_v34
{
	GAME_SAVE_V34;
} SAVE_GAME_V34;


typedef struct save_game
{
	GAME_SAVE_V34;
} SAVE_GAME;

#define TEMP_DROID_MAXPROGS	3
#define	SAVE_COMP_PROGRAM	8
#define SAVE_COMP_WEAPON	9

typedef struct _save_move_control
{
	UBYTE	Status;						// Inactive, Navigating or moving point to point status
	UBYTE	Mask;						// Mask used for the creation of this path
//	SBYTE	Direction;					// Direction object should be moving (0-7) 0=Up,1=Up-Right
//	SDWORD	Speed;						// Speed at which object moves along the movement list
	UBYTE	Position;	   				// Position in asPath
	UBYTE	numPoints;					// number of points in asPath
//	PATH_POINT	MovementList[TRAVELSIZE];
	PATH_POINT	asPath[TRAVELSIZE];		// Pointer to list of block X,Y coordinates.
										// When initialised list is terminated by (0xffff,0xffff)
										// Values prefixed by 0x8000 are pixel coordinates instead of
										// block coordinates
	SDWORD	DestinationX;				// DestinationX,Y should match objects current X,Y
	SDWORD	DestinationY;				//		location for this movement to be complete.
//   	UDWORD	Direction3D;				// *** not necessary
//	UDWORD	TargetDir;					// *** not necessary Direction the object should be facing
//	SDWORD	Gradient;					// Gradient of line
//	SDWORD	DeltaX;						// Distance from start to end position of current movement X
//	SDWORD	DeltaY;						// Distance from start to end position of current movement Y
//	SDWORD	XStep;						// Adjustment to the characters X position each movement
//	SDWORD	YStep;						// Adjustment to the characters Y position each movement
//	SDWORD	DestPixelX;					// Pixel coordinate destination for pixel movement (NOT the final X)
//	SDWORD	DestPixelY;					// Pixel coordiante destination for pixel movement (NOT the final Y)
	SDWORD	srcX,srcY,targetX,targetY;

	/* Stuff for John's movement update */
	FRACT	fx,fy;						// droid location as a fract
//	FRACT	dx,dy;						// x and y change for current direction
	// NOTE: this is supposed to replace Speed
	FRACT	speed;						// Speed of motion
	SWORD	boundX,boundY;				// Vector for the end of path boundary
	SWORD	dir;						// direction of motion (not the direction the droid is facing)

	SWORD	bumpDir;					// direction at last bump
	UDWORD	bumpTime;					// time of first bump with something
	UWORD	lastBump;					// time of last bump with a droid - relative to bumpTime
	UWORD	pauseTime;					// when MOVEPAUSE started - relative to bumpTime
	UWORD	bumpX,bumpY;				// position of last bump

	UDWORD	shuffleStart;				// when a shuffle started

	struct _formation	*psFormation;			// formation the droid is currently a member of

	/* vtol movement - GJ */
	SWORD	iVertSpeed;
	/* Watermelon:I need num of DROID_MAXWEAPS of iAttackRuns */
	UWORD	iAttackRuns[DROID_MAXWEAPS];
	/* Watermelon:guard radius */
	//UDWORD	iGuardRadius;

	/* Only needed for Alex's movement update ? */
//	UDWORD	timeStarted;
//	UDWORD	arrivalTime;
//	UDWORD	pathStarted;
//	BOOL	startedMoving;
//	UDWORD	lastTime;
//	BOOL	speedChange;
} SAVE_MOVE_CONTROL;


#define DROID_SAVE_V9		\
	OBJECT_SAVE_V19;			\
	SAVE_COMPONENT_V19	asBits[DROID_MAXCOMP]; \
	UDWORD		body;		\
	UBYTE		droidType;	\
	UDWORD		saveType;	\
	UDWORD		numWeaps;	\
	SAVE_WEAPON_V19	asWeaps[TEMP_DROID_MAXPROGS];	\
	UDWORD		numKills

typedef struct _save_droid_v9
{
	DROID_SAVE_V9;
} SAVE_DROID_V9;

/*save DROID SAVE 11 */
#define DROID_SAVE_V11		\
	OBJECT_SAVE_V19;			\
	SAVE_COMPONENT_V19	asBits[DROID_MAXCOMP]; \
	UDWORD		body;		\
	UBYTE		droidType;	\
	UBYTE		saveType;	\
	UDWORD		numWeaps;	\
	SAVE_WEAPON_V19	asWeaps[TEMP_DROID_MAXPROGS];	\
	UDWORD		numKills;	\
	UWORD	turretRotation;	\
	UWORD	turretPitch

typedef struct _save_droid_v11
{
	DROID_SAVE_V11;
} SAVE_DROID_V11;

#define DROID_SAVE_V12		\
	DROID_SAVE_V9;			\
	UWORD	turretRotation;	\
	UWORD	turretPitch;	\
	SDWORD	order;			\
	UWORD	orderX,orderY;	\
	UWORD	orderX2,orderY2;\
	UDWORD	timeLastHit;	\
	UDWORD	targetID;		\
	UDWORD	secondaryOrder;	\
	SDWORD	action;			\
	UDWORD	actionX,actionY;\
	UDWORD	actionTargetID;	\
	UDWORD	actionStarted;	\
	UDWORD	actionPoints;	\
	UWORD	actionHeight

typedef struct _save_droid_v12
{
	DROID_SAVE_V12;
} SAVE_DROID_V12;

#define DROID_SAVE_V14		\
	DROID_SAVE_V12;			\
	char	tarStatName[MAX_STR_SIZE];\
    UDWORD	baseStructID;	\
	UBYTE	group;			\
	UBYTE	selected;		\
	UBYTE	cluster_unused;		\
	UBYTE	visible[MAX_PLAYERS];\
	UDWORD	died;			\
	UDWORD	lastEmission

typedef struct _save_droid_v14
{
	DROID_SAVE_V14;
} SAVE_DROID_V14;

//DROID_SAVE_18 replaces DROID_SAVE_14
#define DROID_SAVE_V18		\
	DROID_SAVE_V12;			\
	char	tarStatName[MAX_SAVE_NAME_SIZE_V19];\
    UDWORD	baseStructID;	\
	UBYTE	group;			\
	UBYTE	selected;		\
	UBYTE	cluster_unused;		\
	UBYTE	visible[MAX_PLAYERS];\
	UDWORD	died;			\
	UDWORD	lastEmission

typedef struct _save_droid_v18
{
	DROID_SAVE_V18;
} SAVE_DROID_V18;


//DROID_SAVE_20 replaces all previous saves uses 60 character names
#define DROID_SAVE_V20		\
	OBJECT_SAVE_V20;			\
	SAVE_COMPONENT	asBits[DROID_MAXCOMP]; \
	UDWORD		body;		\
	UBYTE		droidType;	\
	UDWORD		saveType;	\
	UDWORD		numWeaps;	\
	SAVE_WEAPON	asWeaps[TEMP_DROID_MAXPROGS];	\
	UDWORD		numKills;	\
	UWORD	turretRotation;	\
	UWORD	turretPitch;	\
	SDWORD	order;			\
	UWORD	orderX,orderY;	\
	UWORD	orderX2,orderY2;\
	UDWORD	timeLastHit;	\
	UDWORD	targetID;		\
	UDWORD	secondaryOrder;	\
	SDWORD	action;			\
	UDWORD	actionX,actionY;\
	UDWORD	actionTargetID;	\
	UDWORD	actionStarted;	\
	UDWORD	actionPoints;	\
	UWORD	actionHeight;	\
	char	tarStatName[MAX_SAVE_NAME_SIZE];\
    UDWORD	baseStructID;	\
	UBYTE	group;			\
	UBYTE	selected;		\
	UBYTE	cluster_unused;		\
	UBYTE	visible[MAX_PLAYERS];\
	UDWORD	died;			\
	UDWORD	lastEmission

typedef struct _save_droid_v20
{
	DROID_SAVE_V20;
} SAVE_DROID_V20;

#define DROID_SAVE_V21		\
	DROID_SAVE_V20;			\
	UDWORD	commandId

typedef struct _save_droid_v21
{
	DROID_SAVE_V21;
} SAVE_DROID_V21;

#define DROID_SAVE_V24		\
	DROID_SAVE_V21;			\
	SDWORD	resistance;		\
	SAVE_MOVE_CONTROL	sMove;	\
	SWORD		formationDir;	\
	SDWORD		formationX;	\
	SDWORD		formationY

typedef struct _save_droid_v24
{
	DROID_SAVE_V24;
} SAVE_DROID_V24;

//Watermelon: I need DROID_SAVE_V99...
#define DROID_SAVE_V99		\
	OBJECT_SAVE_V20;			\
	SAVE_COMPONENT	asBits[DROID_MAXCOMP]; \
	UDWORD		body;		\
	UBYTE		droidType;	\
	UDWORD		saveType;	\
	UDWORD		numWeaps;	\
	SAVE_WEAPON	asWeaps[TEMP_DROID_MAXPROGS];	\
	UDWORD		numKills;	\
	UWORD	turretRotation[DROID_MAXWEAPS];	\
	UWORD	turretPitch[DROID_MAXWEAPS];	\
	SDWORD	order;			\
	UWORD	orderX,orderY;	\
	UWORD	orderX2,orderY2;\
	UDWORD	timeLastHit;	\
	UDWORD	targetID;		\
	UDWORD	secondaryOrder;	\
	SDWORD	action;			\
	UDWORD	actionX,actionY;\
	UDWORD	actionTargetID;	\
	UDWORD	actionStarted;	\
	UDWORD	actionPoints;	\
	UWORD	actionHeight;	\
	char	tarStatName[MAX_SAVE_NAME_SIZE];\
    UDWORD	baseStructID;	\
	UBYTE	group;			\
	UBYTE	selected;		\
	UBYTE	cluster_unused;		\
	UBYTE	visible[MAX_PLAYERS];\
	UDWORD	died;			\
	UDWORD	lastEmission

typedef struct _save_droid_v99
{
	DROID_SAVE_V99;
} SAVE_DROID_V99;

//Watermelon:V99 'test'
typedef struct _save_droid
{
	DROID_SAVE_V99;
	UDWORD	commandId;
	SDWORD	resistance;
	SAVE_MOVE_CONTROL	sMove;
	SWORD		formationDir;
	SDWORD		formationX;
	SDWORD		formationY;
} SAVE_DROID;


typedef struct _droidinit_save_header
{
	char		aFileType[4];
	UDWORD		version;
	UDWORD		quantity;
} DROIDINIT_SAVEHEADER;

typedef struct _save_droidinit
{
	OBJECT_SAVE_V19;
} SAVE_DROIDINIT;

/*
 *	STRUCTURE Definitions
 */

#define STRUCTURE_SAVE_V2 \
	OBJECT_SAVE_V19; \
	UBYTE				status; \
	SDWORD				currentBuildPts; \
	UDWORD				body; \
	UDWORD				armour; \
	UDWORD				resistance; \
	UDWORD				dummy1; \
	UDWORD				subjectInc;  /*research inc or factory prod id*/\
	UDWORD				timeStarted; \
	UDWORD				output; \
	UDWORD				capacity; \
	UDWORD				quantity

typedef struct _save_structure_v2
{
	STRUCTURE_SAVE_V2;
} SAVE_STRUCTURE_V2;

#define STRUCTURE_SAVE_V12 \
	STRUCTURE_SAVE_V2; \
	UDWORD				factoryInc;			\
	UBYTE				loopsPerformed;		\
	UDWORD				powerAccrued;		\
	UDWORD				dummy2;			\
	UDWORD				droidTimeStarted;	\
	UDWORD				timeToBuild;		\
	UDWORD				timeStartHold

typedef struct _save_structure_v12
{
	STRUCTURE_SAVE_V12;
} SAVE_STRUCTURE_V12;

#define STRUCTURE_SAVE_V14 \
	STRUCTURE_SAVE_V12; \
	UBYTE	visible[MAX_PLAYERS]

typedef struct _save_structure_v14
{
	STRUCTURE_SAVE_V14;
} SAVE_STRUCTURE_V14;

#define STRUCTURE_SAVE_V15 \
	STRUCTURE_SAVE_V14; \
	char	researchName[MAX_SAVE_NAME_SIZE_V19]

typedef struct _save_structure_v15
{
	STRUCTURE_SAVE_V15;
} SAVE_STRUCTURE_V15;

#define STRUCTURE_SAVE_V17 \
	STRUCTURE_SAVE_V15;\
	SWORD				currentPowerAccrued

typedef struct _save_structure_v17
{
	STRUCTURE_SAVE_V17;
} SAVE_STRUCTURE_V17;

#define STRUCTURE_SAVE_V20 \
	OBJECT_SAVE_V20; \
	UBYTE				status; \
	SDWORD				currentBuildPts; \
	UDWORD				body; \
	UDWORD				armour; \
	UDWORD				resistance; \
	UDWORD				dummy1; \
	UDWORD				subjectInc;  /*research inc or factory prod id*/\
	UDWORD				timeStarted; \
	UDWORD				output; \
	UDWORD				capacity; \
	UDWORD				quantity; \
	UDWORD				factoryInc;			\
	UBYTE				loopsPerformed;		\
	UDWORD				powerAccrued;		\
	UDWORD				dummy2;			\
	UDWORD				droidTimeStarted;	\
	UDWORD				timeToBuild;		\
	UDWORD				timeStartHold; \
	UBYTE				visible[MAX_PLAYERS]; \
	char				researchName[MAX_SAVE_NAME_SIZE]; \
	SWORD				currentPowerAccrued

typedef struct _save_structure_v20
{
	STRUCTURE_SAVE_V20;
} SAVE_STRUCTURE_V20;

#define STRUCTURE_SAVE_V21 \
	STRUCTURE_SAVE_V20; \
	UDWORD				commandId

typedef struct _save_structure_v21
{
	STRUCTURE_SAVE_V21;
} SAVE_STRUCTURE_V21;

typedef struct _save_structure
{
	STRUCTURE_SAVE_V21;
} SAVE_STRUCTURE;


//PROGRAMS NEED TO BE REMOVED FROM DROIDS - 7/8/98
// multiPlayerID for templates needs to be saved - 29/10/98
#define TEMPLATE_SAVE_V2 \
	char				name[MAX_SAVE_NAME_SIZE_V19]; \
	UDWORD				ref; \
	UDWORD				player; \
	UBYTE				droidType; \
	char				asParts[DROID_MAXCOMP][MAX_SAVE_NAME_SIZE_V19]; \
	UDWORD				numWeaps; \
	char				asWeaps[TEMP_DROID_MAXPROGS][MAX_SAVE_NAME_SIZE_V19]; \
	UDWORD				numProgs; \
	char				asProgs[TEMP_DROID_MAXPROGS][MAX_SAVE_NAME_SIZE_V19]

// multiPlayerID for templates needs to be saved - 29/10/98
#define TEMPLATE_SAVE_V14 \
	char				name[MAX_SAVE_NAME_SIZE_V19]; \
	UDWORD				ref; \
	UDWORD				player; \
	UBYTE				droidType; \
	char				asParts[DROID_MAXCOMP][MAX_SAVE_NAME_SIZE_V19]; \
	UDWORD				numWeaps; \
	char				asWeaps[TEMP_DROID_MAXPROGS][MAX_SAVE_NAME_SIZE_V19]; \
	UDWORD				multiPlayerID

#define TEMPLATE_SAVE_V20 \
	char				name[MAX_SAVE_NAME_SIZE]; \
	UDWORD				ref; \
	UDWORD				player; \
	UBYTE				droidType; \
	char				asParts[DROID_MAXCOMP][MAX_SAVE_NAME_SIZE]; \
	UDWORD				numWeaps; \
	char				asWeaps[TEMP_DROID_MAXPROGS][MAX_SAVE_NAME_SIZE]; \
	UDWORD				multiPlayerID



typedef struct _save_template_v2
{
	TEMPLATE_SAVE_V2;
} SAVE_TEMPLATE_V2;

typedef struct _save_template_v14
{
	TEMPLATE_SAVE_V14;
} SAVE_TEMPLATE_V14;

typedef struct _save_template_v20
{
	TEMPLATE_SAVE_V20;
} SAVE_TEMPLATE_V20;

typedef struct _save_template
{
	TEMPLATE_SAVE_V20;
} SAVE_TEMPLATE;


#define FEATURE_SAVE_V2 \
	OBJECT_SAVE_V19

typedef struct _save_feature_v2
{
	FEATURE_SAVE_V2;
} SAVE_FEATURE_V2;

#define FEATURE_SAVE_V14 \
	FEATURE_SAVE_V2; \
	UBYTE	visible[MAX_PLAYERS]

typedef struct _save_feature_v14
{
	FEATURE_SAVE_V14;
} SAVE_FEATURE_V14;

#define FEATURE_SAVE_V20 \
	OBJECT_SAVE_V20; \
	UBYTE	visible[MAX_PLAYERS]

typedef struct _save_feature_v20
{
	FEATURE_SAVE_V20;
} SAVE_FEATURE_V20;

typedef struct _save_feature
{
	FEATURE_SAVE_V20;
} SAVE_FEATURE;


#define COMPLIST_SAVE_V6 \
	char				name[MAX_SAVE_NAME_SIZE_V19]; \
	UBYTE				type; \
	UBYTE				state; \
	UBYTE				player

#define COMPLIST_SAVE_V20 \
	char				name[MAX_SAVE_NAME_SIZE]; \
	UBYTE				type; \
	UBYTE				state; \
	UBYTE				player


typedef struct _save_compList_v6
{
	COMPLIST_SAVE_V6;
} SAVE_COMPLIST_V6;

typedef struct _save_compList_v20
{
	COMPLIST_SAVE_V20;
} SAVE_COMPLIST_V20;

typedef struct _save_compList
{
	COMPLIST_SAVE_V20;
} SAVE_COMPLIST;





#define STRUCTLIST_SAVE_V6 \
	char				name[MAX_SAVE_NAME_SIZE_V19]; \
	UBYTE				state; \
	UBYTE				player

#define STRUCTLIST_SAVE_V20 \
	char				name[MAX_SAVE_NAME_SIZE]; \
	UBYTE				state; \
	UBYTE				player

typedef struct _save_structList_v6
{
	STRUCTLIST_SAVE_V6;
} SAVE_STRUCTLIST_V6;

typedef struct _save_structList_v20
{
	STRUCTLIST_SAVE_V20;
} SAVE_STRUCTLIST_V20;

typedef struct _save_structList
{
	STRUCTLIST_SAVE_V20;
} SAVE_STRUCTLIST;


#define RESEARCH_SAVE_V8 \
	char				name[MAX_SAVE_NAME_SIZE_V19]; \
	UBYTE				possible[MAX_PLAYERS]; \
	UBYTE				researched[MAX_PLAYERS]; \
	UDWORD				currentPoints[MAX_PLAYERS]

#define RESEARCH_SAVE_V20 \
	char				name[MAX_SAVE_NAME_SIZE]; \
	UBYTE				possible[MAX_PLAYERS]; \
	UBYTE				researched[MAX_PLAYERS]; \
	UDWORD				currentPoints[MAX_PLAYERS]


typedef struct _save_research_v8
{
	RESEARCH_SAVE_V8;
} SAVE_RESEARCH_V8;

typedef struct _save_research_v20
{
	RESEARCH_SAVE_V20;
} SAVE_RESEARCH_V20;

typedef struct _save_research
{
	RESEARCH_SAVE_V20;
} SAVE_RESEARCH;

typedef struct _save_message
{
	MESSAGE_TYPE	type;			//The type of message
	BOOL			bObj;
	char			name[MAX_STR_SIZE];
	UDWORD			objId;					//Id for Proximity messages!
	BOOL			read;					//flag to indicate whether message has been read
	UDWORD			player;					//which player this message belongs to

} SAVE_MESSAGE;

typedef struct _save_proximity
{
	POSITION_TYPE	type;				/*the type of position obj - FlagPos or ProxDisp*/
	UDWORD			frameNumber;		/*when the Position was last drawn*/
	UDWORD			screenX;			/*screen coords and radius of Position imd */
	UDWORD			screenY;
	UDWORD			screenR;
	UDWORD			player;				/*which player the Position belongs to*/
	BOOL			selected;			/*flag to indicate whether the Position */
	UDWORD			objId;					//Id for Proximity messages!
	UDWORD			radarX;					//Used to store the radar coords - if to be drawn
	UDWORD			radarY;
	UDWORD			timeLastDrawn;			//stores the time the 'button' was last drawn for animation
	UDWORD			strobe;					//id of image last used
	UDWORD			buttonID;				//id of the button for the interface

} SAVE_PROXIMITY;

typedef struct _save_flag_v18
{
	POSITION_TYPE	type;				/*the type of position obj - FlagPos or ProxDisp*/
	UDWORD			frameNumber;		/*when the Position was last drawn*/
	UDWORD			screenX;			/*screen coords and radius of Position imd */
	UDWORD			screenY;
	UDWORD			screenR;
	UDWORD			player;				/*which player the Position belongs to*/
	BOOL			selected;			/*flag to indicate whether the Position */
	Vector3i		coords;							//the world coords of the Position
	UBYTE		factoryInc;						//indicates whether the first, second etc factory
	UBYTE		factoryType;					//indicates whether standard, cyborg or vtol factory
	UBYTE		dummyNOTUSED;						//sub value. needed to order production points.
	UBYTE		dummyNOTUSED2;
} SAVE_FLAG_V18;

typedef struct _save_flag
{
	POSITION_TYPE	type;				/*the type of position obj - FlagPos or ProxDisp*/
	UDWORD			frameNumber;		/*when the Position was last drawn*/
	UDWORD			screenX;			/*screen coords and radius of Position imd */
	UDWORD			screenY;
	UDWORD			screenR;
	UDWORD			player;				/*which player the Position belongs to*/
	BOOL			selected;			/*flag to indicate whether the Position */
	Vector3i		coords;							//the world coords of the Position
	UBYTE		factoryInc;						//indicates whether the first, second etc factory
	UBYTE		factoryType;					//indicates whether standard, cyborg or vtol factory
	UBYTE		dummyNOTUSED;						//sub value. needed to order production points.
	UBYTE		dummyNOTUSED2;
	UDWORD		repairId;
} SAVE_FLAG;

//PRODUCTION_RUN		asProductionRun[NUM_FACTORY_TYPES][MAX_FACTORY][MAX_PROD_RUN];
typedef struct _save_production
{
	UBYTE						quantity;			//number to build
	UBYTE						built;				//number built on current run
	UDWORD						multiPlayerID;		//template to build
} SAVE_PRODUCTION;

#define STRUCTLIMITS_SAVE_V2 \
	char				name[MAX_SAVE_NAME_SIZE_V19]; \
	UBYTE				limit; \
	UBYTE				player

typedef struct _save_structLimits_v2
{
	STRUCTLIMITS_SAVE_V2;
} SAVE_STRUCTLIMITS_V2;

#define STRUCTLIMITS_SAVE_V20 \
	char				name[MAX_SAVE_NAME_SIZE]; \
	UBYTE				limit; \
	UBYTE				player

typedef struct _save_structLimits_v20
{
	STRUCTLIMITS_SAVE_V20;
} SAVE_STRUCTLIMITS_V20;

typedef struct _save_structLimits
{
	STRUCTLIMITS_SAVE_V20;
} SAVE_STRUCTLIMITS;


#define COMMAND_SAVE_V20 \
	UDWORD				droidID

typedef struct _save_command_v20
{
	COMMAND_SAVE_V20;
} SAVE_COMMAND_V20;

typedef struct _save_command
{
	COMMAND_SAVE_V20;
} SAVE_COMMAND;


/* The different types of droid */
typedef enum _droid_save_type
{
	DROID_NORMAL,	// Weapon droid
	DROID_ON_TRANSPORT,
} DROID_SAVE_TYPE;

/***************************************************************************/
/*
 *	Local Variables
 */
/***************************************************************************/
extern	UDWORD				objID;					// unique ID creation thing..

static UDWORD			saveGameVersion = 0;
static BOOL				saveGameOnMission = FALSE;
static SAVE_GAME		saveGameData;
static UDWORD			oldestSaveGameVersion = CURRENT_VERSION_NUM;
static UDWORD			validityKey = 0;

static UDWORD	savedGameTime;
static UDWORD	savedObjId;

//static UDWORD			HashedName;
//static STRUCTURE *psStructList;
//static FEATURE *psFeatureList;
//static FLAG_POSITION **ppsCurrentFlagPosLists;
static SDWORD	startX, startY;
static UDWORD   width, height;
static UDWORD	gameType;
static BOOL IsScenario;
//static BOOL LoadGameFromWDG;
/***************************************************************************/
/*
 *	Local ProtoTypes
 */
/***************************************************************************/
BOOL gameLoad(char *pFileData, UDWORD filesize);
static BOOL gameLoadV7(char *pFileData, UDWORD filesize);
static BOOL gameLoadV(char *pFileData, UDWORD filesize, UDWORD version);
static BOOL writeGameFile(char *pFileName, SDWORD saveType);
static BOOL writeMapFile(char *pFileName);

static BOOL loadSaveDroidInitV2(char *pFileData, UDWORD filesize,UDWORD quantity);

static BOOL loadSaveDroidInit(char *pFileData, UDWORD filesize);
static DROID_TEMPLATE *FindDroidTemplate(char *name,UDWORD player);

static BOOL loadSaveDroid(char *pFileData, UDWORD filesize, DROID **ppsCurrentDroidLists);
static BOOL loadSaveDroidV11(char *pFileData, UDWORD filesize, UDWORD numDroids, UDWORD version, DROID **ppsCurrentDroidLists);
static BOOL loadSaveDroidV19(char *pFileData, UDWORD filesize, UDWORD numDroids, UDWORD version, DROID **ppsCurrentDroidLists);
static BOOL loadSaveDroidV(char *pFileData, UDWORD filesize, UDWORD numDroids, UDWORD version, DROID **ppsCurrentDroidLists);
static BOOL loadDroidSetPointers(void);
static BOOL writeDroidFile(char *pFileName, DROID **ppsCurrentDroidLists);

static BOOL loadSaveStructure(char *pFileData, UDWORD filesize);
static BOOL loadSaveStructureV7(char *pFileData, UDWORD filesize, UDWORD numStructures);
static BOOL loadSaveStructureV19(char *pFileData, UDWORD filesize, UDWORD numStructures, UDWORD version);
static BOOL loadSaveStructureV(char *pFileData, UDWORD filesize, UDWORD numStructures, UDWORD version);
static BOOL loadStructSetPointers(void);
static BOOL writeStructFile(char *pFileName);

static BOOL loadSaveTemplate(char *pFileData, UDWORD filesize);
static BOOL loadSaveTemplateV7(char *pFileData, UDWORD filesize, UDWORD numTemplates);
static BOOL loadSaveTemplateV14(char *pFileData, UDWORD filesize, UDWORD numTemplates);
static BOOL loadSaveTemplateV(char *pFileData, UDWORD filesize, UDWORD numTemplates);
static BOOL writeTemplateFile(char *pFileName);

static BOOL loadSaveFeature(char *pFileData, UDWORD filesize);
static BOOL loadSaveFeatureV14(char *pFileData, UDWORD filesize, UDWORD numFeatures, UDWORD version);
static BOOL loadSaveFeatureV(char *pFileData, UDWORD filesize, UDWORD numFeatures, UDWORD version);
static BOOL writeFeatureFile(char *pFileName);

BOOL loadTerrainTypeMap(char *pFileData, UDWORD filesize);		// now used in gamepsx.c aswell
static BOOL writeTerrainTypeMapFile(char *pFileName);

static BOOL loadSaveCompList(char *pFileData, UDWORD filesize);
static BOOL loadSaveCompListV9(char *pFileData, UDWORD filesize, UDWORD numRecords, UDWORD version);
static BOOL loadSaveCompListV(char *pFileData, UDWORD filesize, UDWORD numRecords, UDWORD version);
static BOOL writeCompListFile(char *pFileName);

static BOOL loadSaveStructTypeList(char *pFileData, UDWORD filesize);
static BOOL loadSaveStructTypeListV7(char *pFileData, UDWORD filesize, UDWORD numRecords);
static BOOL loadSaveStructTypeListV(char *pFileData, UDWORD filesize, UDWORD numRecords);
static BOOL writeStructTypeListFile(char *pFileName);

static BOOL loadSaveResearch(char *pFileData, UDWORD filesize);
static BOOL loadSaveResearchV8(char *pFileData, UDWORD filesize, UDWORD numRecords);
static BOOL loadSaveResearchV(char *pFileData, UDWORD filesize, UDWORD numRecords);
static BOOL writeResearchFile(char *pFileName);

static BOOL loadSaveMessage(char *pFileData, UDWORD filesize, SWORD levelType);
static BOOL loadSaveMessageV(char *pFileData, UDWORD filesize, UDWORD numMessages, UDWORD version, SWORD levelType);
static BOOL writeMessageFile(char *pFileName);

#ifdef SAVE_PROXIMITY
static BOOL loadSaveProximity(char *pFileData, UDWORD filesize);
static BOOL loadSaveProximityV(char *pFileData, UDWORD filesize, UDWORD numMessages, UDWORD version);
static BOOL writeProximityFile(char *pFileName);
#endif

static BOOL loadSaveFlag(char *pFileData, UDWORD filesize);
static BOOL loadSaveFlagV(char *pFileData, UDWORD filesize, UDWORD numFlags, UDWORD version);
static BOOL writeFlagFile(char *pFileName);

static BOOL loadSaveProduction(char *pFileData, UDWORD filesize);
static BOOL loadSaveProductionV(char *pFileData, UDWORD filesize, UDWORD version);
static BOOL writeProductionFile(char *pFileName);

static BOOL loadSaveStructLimits(char *pFileData, UDWORD filesize);
static BOOL loadSaveStructLimitsV19(char *pFileData, UDWORD filesize, UDWORD numLimits);
static BOOL loadSaveStructLimitsV(char *pFileData, UDWORD filesize, UDWORD numLimits);
static BOOL writeStructLimitsFile(char *pFileName);

static BOOL loadSaveCommandLists(char *pFileData, UDWORD filesize);
static BOOL loadSaveCommandListsV(char *pFileData, UDWORD filesize, UDWORD numDroids);
static BOOL writeCommandLists(char *pFileName);

static BOOL writeScriptState(char *pFileName);

static BOOL getNameFromComp(UDWORD compType, char *pDest, UDWORD compIndex);

//adjust the name depending on type of save game and whether resourceNames are used
static BOOL getSaveObjectName(char *pName);

/* set the global scroll values to use for the save game */
static void setMapScroll(void);

static char *getSaveStructNameV19(SAVE_STRUCTURE_V17 *psSaveStructure)
{
	return(psSaveStructure->name);
}

static char *getSaveStructNameV(SAVE_STRUCTURE *psSaveStructure)
{
	return(psSaveStructure->name);
}

/*This just loads up the .gam file to determine which level data to set up - split up
so can be called in levLoadData when starting a game from a load save game*/

// -----------------------------------------------------------------------------------------
BOOL loadGameInit(char *pGameToLoad )
{
	char			*pFileData = NULL;
	UDWORD			fileSize;

	/* Load in the chosen file data */
	pFileData = DisplayBuffer;
	if (!loadFileToBuffer(pGameToLoad, pFileData, displayBufferSize, &fileSize))
	{
		debug( LOG_NEVER, "loadgame: Fail2\n" );
		goto error;
	}

	if (!gameLoad(pFileData, fileSize))
	{
		// FIXME Probably should never arrive here?
		debug( LOG_NEVER, "loadgame: Fail4\n" );
		goto error;
	}
	return TRUE;

error:
		debug( LOG_NEVER, "loadgame: ERROR\n" );

	/* Start the game clock */
	gameTimeStart();

//	if (multiPlayerInUse)
//	{
//		bMultiPlayer = TRUE;				// reenable multi player messages.
//		multiPlayerInUse = FALSE;
//	}
	return FALSE;
}


// -----------------------------------------------------------------------------------------
// Load a file from a save game into the psx.
// This is divided up into 2 parts ...
//
// if it is a level loaded up from CD then UserSaveGame will by false
// UserSaveGame ... Extra stuff to load after scripts
BOOL loadMissionExtras(char *pGameToLoad, SWORD levelType)
{
	char			aFileName[256];

	UDWORD			fileExten, fileSize;

	char			*pFileData = NULL;

	strcpy(aFileName, pGameToLoad);
	fileExten = strlen(pGameToLoad) - 3;
	aFileName[fileExten - 1] = '\0';
	strcat(aFileName, "/");

#ifdef SAVE_PROXIMITY
	if (saveGameVersion >= VERSION_11)
	{
		//if user save game then load up the proximity messages AFTER any droids or structures are loaded
		//if ((gameType == GTYPE_SAVE_START) ||
		//	(gameType == GTYPE_SAVE_MIDMISSION))
        //only do proximity messages is a mid-save game
		if (gameType == GTYPE_SAVE_MIDMISSION)
		{
			//load in the proximity list file
			aFileName[fileExten] = '\0';
			strcat(aFileName, "proxstate.bjo");
			// Load in the chosen file data
			pFileData = DisplayBuffer;
			if (loadFileToBufferNoError(aFileName, pFileData, displayBufferSize, &fileSize))
			{
				//load the proximity status data
				if (pFileData)
				{
					if (!loadSaveProximity(pFileData, fileSize))
					{
						debug( LOG_NEVER, "loadMissionExtras: Fail 2\n" );
						return FALSE;
					}
				}
			}

		}
}
#endif

//#ifdef NEW_SAVE //V11 Save
	if (saveGameVersion >= VERSION_11)
	{
		//if user save game then load up the messages AFTER any droids or structures are loaded
		if ((gameType == GTYPE_SAVE_START) ||
			(gameType == GTYPE_SAVE_MIDMISSION))
		{
			//load in the message list file
			aFileName[fileExten] = '\0';
			strcat(aFileName, "messtate.bjo");
			// Load in the chosen file data
			pFileData = DisplayBuffer;
			if (loadFileToBufferNoError(aFileName, pFileData, displayBufferSize, &fileSize))
			{
				//load the message status data
				if (pFileData)
				{
					if (!loadSaveMessage(pFileData, fileSize, levelType))
					{
						debug( LOG_NEVER, "loadMissionExtras: Fail 2\n" );
						return FALSE;
					}
				}
			}

		}
    }
//#endif

	return TRUE;
}


// -----------------------------------------------------------------------------------------
// UserSaveGame ... this is true when you are loading a players save game
BOOL loadGame(char *pGameToLoad, BOOL keepObjects, BOOL freeMem, BOOL UserSaveGame)
{
	char			aFileName[256];
	//OPENFILENAME		sOFN;
	UDWORD			fileExten, fileSize, pl;
	char			*pFileData = NULL;
	UDWORD			player, inc, i, j;
    DROID           *psCurr;
    UWORD           missionScrollMinX = 0, missionScrollMinY = 0,
		    missionScrollMaxX = 0, missionScrollMaxY = 0;

	debug( LOG_NEVER, "loadGame\n" );

	/* Stop the game clock */
	gameTimeStop();

	if ((gameType == GTYPE_SAVE_START) ||
		(gameType == GTYPE_SAVE_MIDMISSION))
	{
		gameTimeReset(savedGameTime);//added 14 may 98 JPS to solve kev's problem with no firing droids
        //need to reset the event timer too - AB 14/01/99
        eventTimeReset(savedGameTime/SCR_TICKRATE);
	}

	/* Clear all the objects off the map and free up the map memory */
	proj_FreeAllProjectiles();	//always clear this
	if (freeMem)
	{
		//clear out the audio
		audio_StopAll();

		freeAllDroids();
		freeAllStructs();
		freeAllFeatures();

	//	droidTemplateShutDown();
		if (psMapTiles)
		{
//			FREE(psMapTiles);
//			mapFreeTilesAndStrips();
		}
		if (aMapLinePoints)
		{
			FREE(aMapLinePoints);
		}
		//clear all the messages?
		releaseAllProxDisp();
	}


	if (!keepObjects)
	{
		//initialise the lists
		for (player = 0; player < MAX_PLAYERS; player++)
		{
			apsDroidLists[player] = NULL;
			apsStructLists[player] = NULL;
			apsFeatureLists[player] = NULL;
			apsFlagPosLists[player] = NULL;
			//clear all the messages?
			apsProxDisp[player] = NULL;
		}
		initFactoryNumFlag();
	}

	if (UserSaveGame)//always !keepObjects
	{
		//initialise the lists
		for (player = 0; player < MAX_PLAYERS; player++)
		{
			apsLimboDroids[player] = NULL;
			mission.apsDroidLists[player] = NULL;
			mission.apsStructLists[player] = NULL;
			mission.apsFeatureLists[player] = NULL;
			mission.apsFlagPosLists[player] = NULL;
		}

		//JPS 25 feb
		//initialise upgrades
		//initialise the structure upgrade arrays
		memset(asStructureUpgrade, 0, MAX_PLAYERS * sizeof(STRUCTURE_UPGRADE));
		memset(asWallDefenceUpgrade, 0, MAX_PLAYERS * sizeof(WALLDEFENCE_UPGRADE));
		memset(asResearchUpgrade, 0, MAX_PLAYERS * sizeof(RESEARCH_UPGRADE));
		memset(asPowerUpgrade, 0, MAX_PLAYERS * sizeof(POWER_UPGRADE));
		memset(asRepairFacUpgrade, 0, MAX_PLAYERS * sizeof(REPAIR_FACILITY_UPGRADE));
		memset(asProductionUpgrade, 0, MAX_PLAYERS * NUM_FACTORY_TYPES * sizeof(PRODUCTION_UPGRADE));
		memset(asReArmUpgrade, 0, MAX_PLAYERS * sizeof(REARM_UPGRADE));

		//initialise the upgrade structures
		memset(asWeaponUpgrade, 0, MAX_PLAYERS * NUM_WEAPON_SUBCLASS * sizeof(WEAPON_UPGRADE));
		memset(asSensorUpgrade, 0, MAX_PLAYERS * sizeof(SENSOR_UPGRADE));
		memset(asECMUpgrade, 0, MAX_PLAYERS * sizeof(ECM_UPGRADE));
		memset(asRepairUpgrade, 0, MAX_PLAYERS * sizeof(REPAIR_UPGRADE));
		memset(asBodyUpgrade, 0, MAX_PLAYERS * sizeof(BODY_UPGRADE) * BODY_TYPE);
		//JPS 25 feb
	}


	//Stuff added after level load to avoid being reset or initialised during load
	if (UserSaveGame)//always !keepObjects
	{
		if (saveGameVersion >= VERSION_11)//v21
		{
			//camera position
			disp3d_setView(&(saveGameData.currentPlayerPos));
		}

		if (saveGameVersion >= VERSION_12)
		{
			mission.startTime = saveGameData.missionTime;
		}

		//set the scroll varaibles
		startX = saveGameData.ScrollMinX;
		startY = saveGameData.ScrollMinY;
		width = saveGameData.ScrollMaxX - saveGameData.ScrollMinX;
		height = saveGameData.ScrollMaxY - saveGameData.ScrollMinY;
		gameType = saveGameData.GameType;

		if (saveGameVersion >= VERSION_11)
		{
			//camera position
			disp3d_setView(&(saveGameData.currentPlayerPos));
		}

		if (saveGameVersion >= VERSION_14)
		{
			//mission data
			mission.time		=	saveGameData.missionOffTime;
           	mission.ETA			=	saveGameData.missionETA;
			mission.homeLZ_X	=	saveGameData.missionHomeLZ_X;
			mission.homeLZ_Y	=	saveGameData.missionHomeLZ_Y;
			mission.playerX		=	saveGameData.missionPlayerX;
			mission.playerY		=	saveGameData.missionPlayerY;
			//mission data
			for (player = 0; player < MAX_PLAYERS; player++)
			{
				aDefaultSensor[player]				= saveGameData.aDefaultSensor[player];
				aDefaultECM[player]					= saveGameData.aDefaultECM[player];
				aDefaultRepair[player]				= saveGameData.aDefaultRepair[player];
                //check for self repair having been set
                if (aDefaultRepair[player] != 0 &&
                    asRepairStats[aDefaultRepair[player]].location == LOC_DEFAULT)
                {
                    enableSelfRepair((UBYTE)player);
                }
				mission.iTranspEntryTileX[player]	= saveGameData.iTranspEntryTileX[player];
				mission.iTranspEntryTileY[player]	= saveGameData.iTranspEntryTileY[player];
				mission.iTranspExitTileX[player]	= saveGameData.iTranspExitTileX[player];
				mission.iTranspExitTileY[player]	= saveGameData.iTranspExitTileY[player];
			}
		}

		if (saveGameVersion >= VERSION_15)//V21
		{
			offWorldKeepLists	= saveGameData.offWorldKeepLists;
			setRubbleTile(saveGameData.RubbleTile);
			setUnderwaterTile(saveGameData.WaterTile);
			if (saveGameData.fogState == 0)//no fog
			{
				pie_EnableFog(FALSE);
				fogStatus = 0;
			}
			else if (saveGameData.fogState == 1)//fog using old code assume background and depth
			{
				if (war_GetFog())
				{
					pie_EnableFog(TRUE);
				}
				else
				{
					pie_EnableFog(FALSE);
				}
				fogStatus = FOG_BACKGROUND + FOG_DISTANCE;
			}
			else//version 18+ fog
			{
				if (war_GetFog())
				{
					pie_EnableFog(TRUE);
				}
				else
				{
					pie_EnableFog(FALSE);
				}
				fogStatus = saveGameData.fogState;
				fogStatus &= FOG_FLAGS;
			}
			pie_SetFogColour(saveGameData.fogColour);
		}
		if (saveGameVersion >= VERSION_19)//V21
		{
			for(i=0; i<MAX_PLAYERS; i++)
			{
				for(j=0; j<MAX_PLAYERS; j++)
				{
					alliances[i][j] = saveGameData.alliances[i][j];
				}
			}
			for(i=0; i<MAX_PLAYERS; i++)
			{
				setPlayerColour(i,saveGameData.playerColour[i]);
			}
			SetRadarZoom(saveGameData.radarZoom);
		}

		if (saveGameVersion >= VERSION_20)//V21
		{
			setDroidsToSafetyFlag(saveGameData.bDroidsToSafetyFlag);
			for (inc = 0; inc < MAX_PLAYERS; inc++)
			{
				memcpy(&asVTOLReturnPos[inc],&(saveGameData.asVTOLReturnPos[inc]),sizeof(POINT));
			}
		}

		if (saveGameVersion >= VERSION_22)//V22
		{
			for (inc = 0; inc < MAX_PLAYERS; inc++)
			{
				memcpy(&asRunData[inc],&(saveGameData.asRunData[inc]),sizeof(RUN_DATA));
			}
		}

		if (saveGameVersion >= VERSION_24)//V24
		{
			missionSetReinforcementTime(saveGameData.reinforceTime);
			// horrible hack to catch savegames that were saving garbage into these fields
			if (saveGameData.bPlayCountDown <= 1)
			{
				setPlayCountDown(saveGameData.bPlayCountDown);
			}
			if (saveGameData.bPlayerHasWon <= 1)
			{
				setPlayerHasWon(saveGameData.bPlayerHasWon);
			}
			if (saveGameData.bPlayerHasLost <= 1)
			{
				setPlayerHasLost(saveGameData.bPlayerHasLost);
			}
/*			setPlayCountDown(saveGameData.bPlayCountDown);
			setPlayerHasWon(saveGameData.bPlayerHasWon);
			setPlayerHasLost(saveGameData.bPlayerHasLost);*/
		}

		if (saveGameVersion >= VERSION_27)//V27
		{
			for (player = 0; player < MAX_PLAYERS; player++)
			{
				for (inc = 0; inc < MAX_RECYCLED_DROIDS; inc++)
				{
					aDroidExperience[player][inc]	= saveGameData.awDroidExperience[player][inc];
				}
			}
		}
		else
		{
			for (player = 0; player < MAX_PLAYERS; player++)
			{
				for (inc = 0; inc < MAX_RECYCLED_DROIDS; inc++)
				{
					aDroidExperience[player][inc]	= saveGameData.aDroidExperience[player][inc];
				}
			}
		}
        if (saveGameVersion >= VERSION_30)
        {
            scrGameLevel = saveGameData.scrGameLevel;
            bExtraVictoryFlag = saveGameData.bExtraVictoryFlag;
            bExtraFailFlag = saveGameData.bExtraFailFlag;
            bTrackTransporter = saveGameData.bTrackTransporter;
        }

        //extra code added for the first patch (v1.1) to save out if mission time is not being counted
		if (saveGameVersion >= VERSION_31)
		{
			//mission data
			mission.cheatTime = saveGameData.missionCheatTime;
        }

		// skirmish saves.
		if (saveGameVersion >= VERSION_33)
		{
			PLAYERSTATS		playerStats;

			game			= saveGameData.sGame;
			NetPlay			= saveGameData.sNetPlay;
			selectedPlayer	= saveGameData.savePlayer;
			productionPlayer= selectedPlayer;
			bMultiPlayer	= saveGameData.multiPlayer;
			cmdDroidMultiExpBoost(TRUE);
			for(inc=0;inc<MAX_PLAYERS;inc++)
			{
				player2dpid[inc]=saveGameData.sPlayer2dpid[inc];
			}
			if(bMultiPlayer)
			{
				loadMultiStats(saveGameData.sPName,&playerStats);				// stats stuff
				setMultiStats(NetPlay.dpidPlayer,playerStats,FALSE);
				setMultiStats(NetPlay.dpidPlayer,playerStats,TRUE);
			}
        }


    }

	/* Get human and AI players names */
	if (saveGameVersion >= VERSION_34)
	{
		for(i=0;i<MAX_PLAYERS;i++)
		{
			(void)setPlayerName(i, saveGameData.sPlayerName[i]);
		}
	}

	//clear the player Power structs
	if ((gameType != GTYPE_SAVE_START) && (gameType != GTYPE_SAVE_MIDMISSION) &&
		(!keepObjects))
	{
		clearPlayerPower();
	}

	//initialise the scroll values
	//startX = startY = width = height = 0;

	//before loading the data - turn power off so don't get any power low warnings
	powerCalculated = FALSE;
	/* Load in the chosen file data */
/*
	pFileData = DisplayBuffer;
	if (!loadFileToBuffer(aFileName, pFileData, displayBufferSize, &fileSize))
	{
		DBPRINTF(("loadgame: Fail2\n"));
		goto error;
	}
	if (!gameLoad(pFileData, fileSize))
	{
		DBPRINTF(("loadgame: Fail4\n"));
		goto error;
	}
	//aFileName[fileExten - 1] = '\0';
	//strcat(aFileName, "/");
*/
	strcpy(aFileName, pGameToLoad);
	fileExten = strlen(aFileName) - 3;			// hack - !
	aFileName[fileExten - 1] = '\0';
	strcat(aFileName, "/");

	//the terrain type WILL only change with Campaign changes (well at the moment!)
	//if (freeMem) - this now works for Cam Start and Cam Change
    if ((gameType != GTYPE_SCENARIO_EXPAND) ||
		UserSaveGame)
	{
		LOADBARCALLBACK();	//		loadingScreenCallback();
		//load in the terrain type map
		aFileName[fileExten] = '\0';
		strcat(aFileName, "ttypes.ttp");
		/* Load in the chosen file data */
		pFileData = DisplayBuffer;
		if (!loadFileToBuffer(aFileName, pFileData, displayBufferSize, &fileSize))
		{
			debug( LOG_NEVER, "loadgame: Fail23\n" );
			goto error;
		}


		//load the terrain type data
		if (pFileData)
		{
			if (!loadTerrainTypeMap(pFileData, fileSize))
			{
				debug( LOG_NEVER, "loadgame: Fail25\n" );
				goto error;
			}
		}
	}

    //load up the Droid Templates BEFORE any structures are loaded
    LOADBARCALLBACK();	//	loadingScreenCallback();
	if (IsScenario==FALSE)
	{
		//NOT ANY MORE - use multiPlayerID (unique template id) to prevent duplicate's being loaded
		//Only want to clear the lists in the final version of the game

//#ifdef FINAL
		//first clear the templates
//		droidTemplateShutDown();
		// only free player 0 templates - keep all the others from the stats
//#define ALLOW_ACCESS_TEMPLATES
#ifndef ALLOW_ACCESS_TEMPLATES
		{
			DROID_TEMPLATE	*pTemplate, *pNext;
			for(pTemplate = apsDroidTemplates[0]; pTemplate != NULL;
				pTemplate = pNext)
			{
				pNext = pTemplate->psNext;
				HEAP_FREE(psTemplateHeap, pTemplate);
			}
			apsDroidTemplates[0] = NULL;
		}
#endif

		// In Multiplayer, clear templates out first.....
		if(	bMultiPlayer)
		{
			for(inc=0;inc<MAX_PLAYERS;inc++)
			{
				while(apsDroidTemplates[inc])				// clear the old template out.
				{
					DROID_TEMPLATE	*psTempl;
					psTempl = apsDroidTemplates[inc]->psNext;
					HEAP_FREE(psTemplateHeap, apsDroidTemplates[inc]);
					apsDroidTemplates[inc] = psTempl;
				}
			}
		}

//load in the templates
		LOADBARCALLBACK();	//	loadingScreenCallback();
		aFileName[fileExten] = '\0';
		strcat(aFileName, "templ.bjo");
		/* Load in the chosen file data */
		pFileData = DisplayBuffer;
		if (!loadFileToBuffer(aFileName, pFileData, displayBufferSize, &fileSize))
		{
			debug( LOG_NEVER, "loadgame: Fail20\n" );
			goto error;
		}
		//load the data into apsTemplates
		if (!loadSaveTemplate(pFileData, fileSize))
		{
			debug( LOG_NEVER, "loadgame: Fail22\n" );
			goto error;
		}
		//JPS 25 feb (reverse templates moved from here)
	}

	if (saveGameOnMission && UserSaveGame)
	{
		LOADBARCALLBACK();	//		loadingScreenCallback();

        //the scroll limits for the mission map have already been written
        if (saveGameVersion >= VERSION_29)
        {
            missionScrollMinX = (UWORD)mission.scrollMinX;
            missionScrollMinY = (UWORD)mission.scrollMinY;
            missionScrollMaxX = (UWORD)mission.scrollMaxX;
            missionScrollMaxY = (UWORD)mission.scrollMaxY;
        }

		//load the map and the droids then swap pointers
//		psMapTiles = NULL;
//		aMapLinePoints = NULL;
		//load in the map file
		aFileName[fileExten] = '\0';
		strcat(aFileName, "mission.map");
		/* Load in the chosen file data */
		pFileData = DisplayBuffer;
		if (loadFileToBufferNoError(aFileName, pFileData, displayBufferSize, &fileSize))
		{
			if (!mapLoad(pFileData, fileSize))
			{
				debug( LOG_NEVER, "loadgame: Fail7\n" );
				return(FALSE);
			}
		}

		//load in the visibility file
		aFileName[fileExten] = '\0';
		strcat(aFileName, "misvis.bjo");
		/* Load in the chosen file data */
		pFileData = DisplayBuffer;
		if (loadFileToBufferNoError(aFileName, pFileData, displayBufferSize, &fileSize))
		{
			//load the visibility data
			if (pFileData)
			{
				if (!readVisibilityData(pFileData, fileSize))
				{
					debug( LOG_NEVER, "loadgame: Fail33\n" );
					goto error;
				}
			}
		}

	// reload the objects that were in the mission list
//except droids these are always loaded directly to the mission.apsDroidList
/*
	*apsFlagPosLists[MAX_PLAYERS];
	asPower[MAX_PLAYERS];
*/
		LOADBARCALLBACK();	//		loadingScreenCallback();
		//load in the features -do before the structures
		aFileName[fileExten] = '\0';
		strcat(aFileName, "mfeat.bjo");
		/* Load in the chosen file data */
		pFileData = DisplayBuffer;
		if (!loadFileToBuffer(aFileName, pFileData, displayBufferSize, &fileSize))
		{
			debug( LOG_NEVER, "loadgame: Fail14\n" );
			goto error;
		}

		//load the data into apsFeatureLists
		if (!loadSaveFeature(pFileData, fileSize))
		{
			debug( LOG_NEVER, "loadgame: Fail16\n" );
			goto error;
		}

		//load in the mission structures
		aFileName[fileExten] = '\0';
		strcat(aFileName, "mstruct.bjo");
		/* Load in the chosen file data */
		pFileData = DisplayBuffer;
		if (!loadFileToBuffer(aFileName, pFileData, displayBufferSize, &fileSize))
		{
			debug( LOG_NEVER, "loadgame: Fail17\n" );
			goto error;
		}

		//load the data into apsStructLists
		if (!loadSaveStructure(pFileData, fileSize))
		{
			debug( LOG_NEVER, "loadgame: Fail19\n" );
			goto error;
		}

		LOADBARCALLBACK();	//		loadingScreenCallback();

		if (bMultiPlayer)
		{
			for(pl=0;pl<MAX_PLAYERS;pl++)// ajl. must do for every player to stop multiplay/pc players going gaga.
			{
			   //reverse the structure lists so the Research Facilities are in the same order as when saved
				reverseObjectList((BASE_OBJECT**)&apsStructLists[pl]);
			}
		}

		LOADBARCALLBACK();	//		loadingScreenCallback();
		//load in the mission droids
		aFileName[fileExten] = '\0';

		if (saveGameVersion < VERSION_27)//V27
		{
			strcat(aFileName, "mdroid.bjo");
		}
		else
		{
			strcat(aFileName, "munit.bjo");
		}
		/* Load in the chosen file data */
		pFileData = DisplayBuffer;
		if (loadFileToBufferNoError(aFileName, pFileData, displayBufferSize, &fileSize))
		{
			//load the data into mission.apsDroidLists
			//ppsCurrentDroidLists = mission.apsDroidLists;
			if (!loadSaveDroid(pFileData, fileSize, apsDroidLists))
			{
				debug( LOG_NEVER, "loadgame: Fail12\n" );
				goto error;
			}
		}

        /*after we've loaded in the units we need to redo the orientation because
        the direction may have been saved - we need to do it outside of the loop
        whilst the current map is valid for the units*/
        for (player = 0; player < MAX_PLAYERS; player++)
        {
            for (psCurr = apsDroidLists[player]; psCurr != NULL; psCurr = psCurr->psNext)
            {
            	if(!(psCurr->droidType == DROID_PERSON ||
                    //psCurr->droidType == DROID_CYBORG ||
                    cyborgDroid(psCurr) ||
                    psCurr->droidType == DROID_TRANSPORTER))
	            {
					if(psCurr->x != INVALID_XY)
					{
						updateDroidOrientation(psCurr);
					}
	            }
            }
        }

		LOADBARCALLBACK();	//		loadingScreenCallback();
		//load in the flag list file
		aFileName[fileExten] = '\0';
		strcat(aFileName, "mflagstate.bjo");
		// Load in the chosen file data
		pFileData = DisplayBuffer;
		if (!loadFileToBuffer(aFileName, pFileData, displayBufferSize, &fileSize))
		{
			debug( LOG_NEVER, "loadMissionExtras: Fail 3\n" );
			return FALSE;
		}


		//load the flag status data
		if (pFileData)
		{
			if (!loadSaveFlag(pFileData, fileSize))
			{
				debug( LOG_NEVER, "loadMissionExtras: Fail 4\n" );
				return FALSE;
			}
		}
		swapMissionPointers();

        //once the mission map has been loaded reset the mission scroll limits
        if (saveGameVersion >= VERSION_29)
        {
            mission.scrollMinX = missionScrollMinX;
            mission.scrollMinY = missionScrollMinY;
            mission.scrollMaxX = missionScrollMaxX;
            mission.scrollMaxY = missionScrollMaxY;
        }

    }


	//if Campaign Expand then don't load in another map
	if (gameType != GTYPE_SCENARIO_EXPAND)
	{
		LOADBARCALLBACK();	//		loadingScreenCallback();
		psMapTiles = NULL;
		aMapLinePoints = NULL;
		//load in the map file
		aFileName[fileExten] = '\0';
		strcat(aFileName, "game.map");
		/* Load in the chosen file data */
		pFileData = DisplayBuffer;
		if (!loadFileToBuffer(aFileName, pFileData, displayBufferSize, &fileSize))
		{
			debug( LOG_NEVER, "loadgame: Fail5\n" );
			goto error;
		}

		if (!mapLoad(pFileData, fileSize))
		{
			debug( LOG_NEVER, "loadgame: Fail7\n" );
			return(FALSE);
		}

#ifdef JOHN
		// load in the gateway map
/*		aFileName[fileExten] = '\0';
		strcat(aFileName, "gates.txt");
		// Load in the chosen file data
		pFileData = DisplayBuffer;
		if (!loadFileToBuffer(aFileName, pFileData, displayBufferSize, &fileSize))
		{
			DBPRINTF(("loadgame: Failed to load gates.txt\n"));
			goto error;
		}

		if (!gwLoadGateways(pFileData, fileSize))
		{
			DBPRINTF(("loadgame: failed to parse gates.txt"));
			return FALSE;
		}*/
#endif
	}

	//save game stuff added after map load

	LOADBARCALLBACK();	//	loadingScreenCallback();
	if (saveGameVersion >= VERSION_16)
	{
		for (inc = 0; inc < MAX_NOGO_AREAS; inc++)
		{
			setNoGoArea(saveGameData.sLandingZone[inc].x1, saveGameData.sLandingZone[inc].y1,
						saveGameData.sLandingZone[inc].x2, saveGameData.sLandingZone[inc].y2, (UBYTE)inc);
		}
	}



	//adjust the scroll range for the new map or the expanded map
	setMapScroll();

	//initialise the Templates' build and power points before loading in any droids
	initTemplatePoints();

	//if user save game then load up the research BEFORE any droids or structures are loaded
	if ((gameType == GTYPE_SAVE_START) ||
		(gameType == GTYPE_SAVE_MIDMISSION))
	{
		LOADBARCALLBACK();	//		loadingScreenCallback();
		//load in the research list file
		aFileName[fileExten] = '\0';
		strcat(aFileName, "resstate.bjo");
		// Load in the chosen file data
		pFileData = DisplayBuffer;
		if (!loadFileToBuffer(aFileName, pFileData, displayBufferSize, &fileSize))
		{
			debug( LOG_NEVER, "loadgame: Fail32\n" );
			goto error;
		}

		//load the research status data
		if (pFileData)
		{
			if (!loadSaveResearch(pFileData, fileSize))
			{
				debug( LOG_NEVER, "loadgame: Fail33\n" );
				goto error;
			}
		}
	}

	if(IsScenario==TRUE)
	{
		LOADBARCALLBACK();	//		loadingScreenCallback();
		//load in the droid initialisation file
		aFileName[fileExten] = '\0';
		strcat(aFileName, "dinit.bjo");
		/* Load in the chosen file data */
		pFileData = DisplayBuffer;
		if (!loadFileToBuffer(aFileName, pFileData, displayBufferSize, &fileSize))
		{
			debug( LOG_NEVER, "loadgame: Fail8\n" );
			goto error;
		}


		if(!loadSaveDroidInit(pFileData,fileSize))
		{
			debug( LOG_NEVER, "loadgame: Fail10\n" );
			goto error;
		}
	}
	else
	{
		LOADBARCALLBACK();	//		loadingScreenCallback();
		//load in the droids
		aFileName[fileExten] = '\0';
		if (saveGameVersion < VERSION_27)//V27
		{
			strcat(aFileName, "droid.bjo");
		}
		else
		{
			strcat(aFileName, "unit.bjo");
		}
		/* Load in the chosen file data */
		pFileData = DisplayBuffer;
		if (!loadFileToBuffer(aFileName, pFileData, displayBufferSize, &fileSize))
		{
			debug( LOG_NEVER, "loadgame: Fail11\n" );
			goto error;
		}

		//load the data into apsDroidLists
		//ppsCurrentDroidLists = apsDroidLists;
		if (!loadSaveDroid(pFileData, fileSize, apsDroidLists))
		{
			debug( LOG_NEVER, "loadgame: Fail12\n" );
			goto error;
		}

        /*after we've loaded in the units we need to redo the orientation because
        the direction may have been saved - we need to do it outside of the loop
        whilst the current map is valid for the units*/
        for (player = 0; player < MAX_PLAYERS; player++)
        {
            for (psCurr = apsDroidLists[player]; psCurr != NULL; psCurr = psCurr->psNext)
            {
            	if(!(psCurr->droidType == DROID_PERSON ||
                    //psCurr->droidType == DROID_CYBORG ||
                    cyborgDroid(psCurr) ||
                    psCurr->droidType == DROID_TRANSPORTER))
	            {
					if(psCurr->x != INVALID_XY)
					{
			            updateDroidOrientation(psCurr);
					}
	            }
            }
        }

        LOADBARCALLBACK();	//		loadingScreenCallback();
		if (saveGameVersion >= 12)
		{
			if (!saveGameOnMission)
			{
				//load in the mission droids
				aFileName[fileExten] = '\0';
				if (saveGameVersion < VERSION_27)//V27
				{
					strcat(aFileName, "mdroid.bjo");
				}
				else
				{
					strcat(aFileName, "munit.bjo");
				}
				/* Load in the chosen file data */
				pFileData = DisplayBuffer;
				if (loadFileToBufferNoError(aFileName, pFileData, displayBufferSize, &fileSize)) {
					//load the data into mission.apsDroidLists
					//ppsCurrentDroidLists = mission.apsDroidLists;
					if (!loadSaveDroid(pFileData, fileSize, mission.apsDroidLists))
					{
						debug( LOG_NEVER, "loadgame: Fail12\n" );
						goto error;
					}
				}

			}
		}
	}

	LOADBARCALLBACK();	//	loadingScreenCallback();
//21feb	if (saveGameOnMission && UserSaveGame)
//21feb	{
	if (saveGameVersion >= VERSION_23)
	{
		//load in the limbo droids
		aFileName[fileExten] = '\0';
		strcat(aFileName, "limbo.bjo");
		/* Load in the chosen file data */
		pFileData = DisplayBuffer;
		if (loadFileToBufferNoError(aFileName, pFileData, displayBufferSize, &fileSize))
		{
			//load the data into apsDroidLists
			//ppsCurrentDroidLists = apsLimboDroids;
			if (!loadSaveDroid(pFileData, fileSize, apsLimboDroids))
			{
				debug( LOG_NEVER, "loadgame: Fail12\n" );
				goto error;
			}
		}

	}

	LOADBARCALLBACK();	//	loadingScreenCallback();
	//load in the features -do before the structures
	aFileName[fileExten] = '\0';
	strcat(aFileName, "feat.bjo");
	/* Load in the chosen file data */
	pFileData = DisplayBuffer;
	if (!loadFileToBuffer(aFileName, pFileData, displayBufferSize, &fileSize))
	{
		debug( LOG_NEVER, "loadgame: Fail14\n" );
		goto error;
	}


	//load the data into apsFeatureLists
	if (!loadSaveFeature(pFileData, fileSize))
	{
		debug( LOG_NEVER, "loadgame: Fail16\n" );
		goto error;
	}

    //load droid templates moved from here to BEFORE any structures loaded in

//load in the structures
	LOADBARCALLBACK();	//	loadingScreenCallback();
	aFileName[fileExten] = '\0';
	strcat(aFileName, "struct.bjo");
	/* Load in the chosen file data */
	pFileData = DisplayBuffer;
	if (!loadFileToBuffer(aFileName, pFileData, displayBufferSize, &fileSize))
	{
		debug( LOG_NEVER, "loadgame: Fail17\n" );
		goto error;
	}
	//load the data into apsStructLists
	if (!loadSaveStructure(pFileData, fileSize))
	{
		debug( LOG_NEVER, "loadgame: Fail19\n" );
		goto error;
	}

	LOADBARCALLBACK();	//	loadingScreenCallback();
	if ((gameType == GTYPE_SAVE_START) ||
		(gameType == GTYPE_SAVE_MIDMISSION))
	{
		for(pl=0;pl<MAX_PLAYERS;pl++)	// ajl. must do for every player to stop multiplay/pc players going gaga.
		{
			//reverse the structure lists so the Research Facilities are in the same order as when saved
			reverseObjectList((BASE_OBJECT**)&apsStructLists[pl]);
		}
	}

	//if user save game then load up the current level for structs and components
	if ((gameType == GTYPE_SAVE_START) ||
		(gameType == GTYPE_SAVE_MIDMISSION))
	{
		LOADBARCALLBACK();	//		loadingScreenCallback();
		//load in the component list file
		aFileName[fileExten] = '\0';
		strcat(aFileName, "compl.bjo");
		/* Load in the chosen file data */
		pFileData = DisplayBuffer;
		if (!loadFileToBuffer(aFileName, pFileData, displayBufferSize, &fileSize))
		{
			debug( LOG_NEVER, "loadgame: Fail26\n" );
			goto error;
		}


		//load the component list data
		if (pFileData)
		{
			if (!loadSaveCompList(pFileData, fileSize))
			{
				debug( LOG_NEVER, "loadgame: Fail28\n" );
				goto error;
			}
		}
		LOADBARCALLBACK();	//		loadingScreenCallback();
		//load in the structure type list file
		aFileName[fileExten] = '\0';
		strcat(aFileName, "strtype.bjo");
		/* Load in the chosen file data */
		pFileData = DisplayBuffer;
		if (!loadFileToBuffer(aFileName, pFileData, displayBufferSize, &fileSize))
		{
			debug( LOG_NEVER, "loadgame: Fail29\n" );
			goto error;
		}


		//load the structure type list data
		if (pFileData)
		{
			if (!loadSaveStructTypeList(pFileData, fileSize))
			{
				debug( LOG_NEVER, "loadgame: Fail31\n" );
				goto error;
			}
		}
	}

	LOADBARCALLBACK();	//	loadingScreenCallback();

//#ifdef NEW_SAVE //V11 Save
	if (saveGameVersion >= VERSION_11)
	{
		//if user save game then load up the Visibility
		if ((gameType == GTYPE_SAVE_START) ||
			(gameType == GTYPE_SAVE_MIDMISSION))
		{
			//load in the visibility file
			aFileName[fileExten] = '\0';
			strcat(aFileName, "visstate.bjo");
			// Load in the chosen file data
			pFileData = DisplayBuffer;
			if (loadFileToBufferNoError(aFileName, pFileData, displayBufferSize, &fileSize))
			{
				//load the visibility data
				if (pFileData)
				{
					if (!readVisibilityData(pFileData, fileSize))
					{
						debug( LOG_NEVER, "loadgame: Fail33\n" );
						goto error;
					}
				}
			}

		}
	}
//#endif

	LOADBARCALLBACK();	//	loadingScreenCallback();

//#ifdef NEW_SAVE_V13 //V13 Save
	if (saveGameVersion > VERSION_12)
	{
		//if user save game then load up the Visibility
		if ((gameType == GTYPE_SAVE_START) ||
			(gameType == GTYPE_SAVE_MIDMISSION))
		{
			//load in the message list file
			aFileName[fileExten] = '\0';
			strcat(aFileName, "prodstate.bjo");
			// Load in the chosen file data
    		pFileData = DisplayBuffer;
			if (loadFileToBufferNoError(aFileName, pFileData, displayBufferSize, &fileSize))
			{
				//load the visibility data
				if (pFileData)
				{
					if (!loadSaveProduction(pFileData, fileSize))
					{
						debug( LOG_NEVER, "loadgame: Fail33\n" );
						goto error;
					}
				}
			}

		}
	}
//#endif
	LOADBARCALLBACK();	//	loadingScreenCallback();

	if (saveGameVersion > VERSION_12)
	{
		//if user save game then load up the FX
		if ((gameType == GTYPE_SAVE_START) ||
			(gameType == GTYPE_SAVE_MIDMISSION))
		{
			//load in the message list file
			aFileName[fileExten] = '\0';
			strcat(aFileName, "fxstate.bjo");
			// Load in the chosen file data
			pFileData = DisplayBuffer;
			if (loadFileToBufferNoError(aFileName, pFileData, displayBufferSize, &fileSize))
			{
				//load the fx data
				if (pFileData)
				{
					if (!readFXData(pFileData, fileSize))
					{
						debug( LOG_NEVER, "loadgame: Fail33\n" );
						goto error;
					}
				}
			}


		}
	}

	LOADBARCALLBACK();	//	loadingScreenCallback();

	if (saveGameVersion >= VERSION_16)
	{
		//if user save game then load up the FX
		if ((gameType == GTYPE_SAVE_START) ||
			(gameType == GTYPE_SAVE_MIDMISSION))
		{
			//load in the message list file
			aFileName[fileExten] = '\0';
			strcat(aFileName, "score.bjo");
			// Load in the chosen file data
			pFileData = DisplayBuffer;
			if (loadFileToBufferNoError(aFileName, pFileData, displayBufferSize, &fileSize))
			{
				//load the fx data
				if (pFileData)
				{
					if (!readScoreData(pFileData, fileSize))
					{
						debug( LOG_NEVER, "loadgame: Fail33\n" );
						goto error;
					}
				}
			}


		}
	}

	LOADBARCALLBACK();	//	loadingScreenCallback();

//#endif
//#ifdef NEW_SAVE_V12 //v12 Save
	if (saveGameVersion >= VERSION_12)
	{
		//if user save game then load up the flags AFTER any droids or structures are loaded
		if ((gameType == GTYPE_SAVE_START) ||
			(gameType == GTYPE_SAVE_MIDMISSION))
		{
			//load in the flag list file
			aFileName[fileExten] = '\0';
			strcat(aFileName, "flagstate.bjo");
			// Load in the chosen file data
			pFileData = DisplayBuffer;
			if (!loadFileToBuffer(aFileName, pFileData, displayBufferSize, &fileSize))
			{
				debug( LOG_NEVER, "loadMissionExtras: Fail 3\n");
				return FALSE;
			}


			//load the flag status data
			if (pFileData)
			{
				if (!loadSaveFlag(pFileData, fileSize))
				{
					debug( LOG_NEVER, "loadMissionExtras: Fail 4\n" );
					return FALSE;
				}
			}
		}
	}

	LOADBARCALLBACK();	//	loadingScreenCallback();

	if (saveGameVersion >= VERSION_21)
	{
		//rebuild the apsCommandDesignation AFTER all droids and structures are loaded
		if ((gameType == GTYPE_SAVE_START) ||
			(gameType == GTYPE_SAVE_MIDMISSION))
		{
			//load in the command list file
			aFileName[fileExten] = '\0';
			strcat(aFileName, "command.bjo");
			// Load in the chosen file data
			pFileData = DisplayBuffer;
			if (!loadFileToBuffer(aFileName, pFileData, displayBufferSize, &fileSize))
			{
				debug( LOG_NEVER, "loadMissionExtras: Fail 5\n" );
				return FALSE;
			}


			//load the command list data
			if (pFileData)
			{
				if (!loadSaveCommandLists(pFileData, fileSize))
				{
					debug( LOG_NEVER, "loadMissionExtras: Fail 6\n" );
					return FALSE;
				}
			}
		}
	}

	LOADBARCALLBACK();	//	loadingScreenCallback();

	if ((saveGameVersion >= VERSION_15) && UserSaveGame)
	{
		//load in the mission structures
		aFileName[fileExten] = '\0';
		strcat(aFileName, "limits.bjo");
		/* Load in the chosen file data */
		pFileData = DisplayBuffer;
		if (!loadFileToBuffer(aFileName, pFileData, displayBufferSize, &fileSize))
		{
			debug( LOG_NEVER, "loadgame: Fail17\n" );
			goto error;
		}

		//load the data into apsStructLists
		if (!loadSaveStructLimits(pFileData, fileSize))
		{
			debug( LOG_NEVER, "loadgame: Fail19\n" );
			goto error;
		}

		//set up the structure Limits
		setCurrentStructQuantity(FALSE);
	}
	else
	{
		//load in the structure limits
		//load the data into structLimits DONE IN SCRIPTS NOW so just init
		initStructLimits();

		//set up the structure Limits
		setCurrentStructQuantity(TRUE);
	}


	LOADBARCALLBACK();	//	loadingScreenCallback();

	//check that delivery points haven't been put down in invalid location
	checkDeliveryPoints(saveGameVersion);

	if ((gameType == GTYPE_SAVE_START) ||
		(gameType == GTYPE_SAVE_MIDMISSION))
	{
		LOADBARCALLBACK();	//	loadingScreenCallback();
		for(pl=0;pl<MAX_PLAYERS;pl++)	// ajl. must do for every player to stop multiplay/pc players going gaga.
		{
			//reverse the structure lists so the Research Facilities are in the same order as when saved
			reverseTemplateList((DROID_TEMPLATE**)&apsDroidTemplates[pl]);
		}

		LOADBARCALLBACK();	//		loadingScreenCallback();
		for(pl=0;pl<MAX_PLAYERS;pl++)
		{
			//reverse the droid lists so selections occur in the same order
			reverseObjectList((BASE_OBJECT**)&apsLimboDroids[pl]);
		}

		LOADBARCALLBACK();	//		loadingScreenCallback();
		for(pl=0;pl<MAX_PLAYERS;pl++)
		{
			//reverse the droid lists so selections occur in the same order
			reverseObjectList((BASE_OBJECT**)&apsDroidLists[pl]);
		}

		LOADBARCALLBACK();	//		loadingScreenCallback();
		for(pl=0;pl<MAX_PLAYERS;pl++)
		{
			//reverse the droid lists so selections occur in the same order
			reverseObjectList((BASE_OBJECT**)&mission.apsDroidLists[pl]);
		}

		LOADBARCALLBACK();	//		loadingScreenCallback();
		for(pl=0;pl<MAX_PLAYERS;pl++)
		{
			//reverse the struct lists so selections occur in the same order
			reverseObjectList((BASE_OBJECT**)&mission.apsStructLists[pl]);
		}

		LOADBARCALLBACK();	//		loadingScreenCallback();
		for(pl=0;pl<MAX_PLAYERS;pl++)
		{
			//reverse the droid lists so selections occur in the same order
			reverseObjectList((BASE_OBJECT**)&apsFeatureLists[pl]);
		}

		LOADBARCALLBACK();	//		loadingScreenCallback();
		for(pl=0;pl<MAX_PLAYERS;pl++)
		{
			//reverse the droid lists so selections occur in the same order
			reverseObjectList((BASE_OBJECT**)&mission.apsFeatureLists[pl]);
		}
	}

	/* Reset the player AI */
	playerReset();

	//turn power on for rest of game
	powerCalculated = TRUE;

	LOADBARCALLBACK();	//	loadingScreenCallback();

	if (saveGameVersion > VERSION_12)
	{
		if (!keepObjects)//only reset the pointers if they were set
		{
			//reset the object pointers in the droid target lists
			loadDroidSetPointers();
		}
	}

	LOADBARCALLBACK();	//	loadingScreenCallback();

	if (saveGameVersion > VERSION_20)
	{
		if (!keepObjects)//only reset the pointers if they were set
		{
			//reset the object pointers in the structure lists
			loadStructSetPointers();
		}
	}
	//don't need to do this anymore - AB 22/04/98
	//set up the power levels for each player if not
	/*if (!keepObjects)
	{
		clearPlayerPower();
		initPlayerPower();
	}*/

	//set all players to have some power at start - will be scripted!
	//newGameInitPower();

	//set these values to suitable for first map - will be scripted!
	//setLandingZone(10,51,12,53);

	//if user save game then reset the time - THIS SETS BOTH TIMERS - BEWARE IF YOU USE IT
	if ((gameType == GTYPE_SAVE_START) ||
		(gameType == GTYPE_SAVE_MIDMISSION))
	{
		ASSERT( gameTime == savedGameTime,"loadGame; game time modified during load" );
		gameTimeReset(savedGameTime);//added 14 may 98 JPS to solve kev's problem with no firing droids
        //need to reset the event timer too - AB 14/01/99
        eventTimeReset(savedGameTime/SCR_TICKRATE);

		//reset the objId for new objects
		if (saveGameVersion >= VERSION_17)
		{
			objID = savedObjId;
		}
	}

    //check the research button isn't flashing unnecessarily
    //cancel first
     stopReticuleButtonFlash(IDRET_RESEARCH);
    //then see if needs to be set
    intCheckResearchButton();

    //set up the mission countdown flag
    setMissionCountDown();

	/* Start the game clock */
	gameTimeStart();

	//after the clock has been reset need to check if any res_extractors are active
	checkResExtractorsActive();

//	if (multiPlayerInUse)
//	{
//		bMultiPlayer = TRUE;				// reenable multi player messages.
//		multiPlayerInUse = FALSE;
//	}
   //	initViewPosition();
	setViewAngle(INITIAL_STARTING_PITCH);
	setDesiredPitch(INITIAL_DESIRED_PITCH);

    //check if limbo_expand mission has changed to an expand mission for user save game (mid-mission)
    if (gameType == GTYPE_SAVE_MIDMISSION && missionLimboExpand())
    {
        /*when all the units have moved from the mission.apsDroidList then the
        campaign has been reset to an EXPAND type - OK so there should have
        been another flag to indicate this state has changed but its late in
        the day excuses...excuses...excuses*/
        if (mission.apsDroidLists[selectedPlayer] == NULL)
        {
            //set the mission type
            startMissionSave(LDS_EXPAND);
        }
    }

    //set this if come into a save game mid mission
    if (gameType == GTYPE_SAVE_MIDMISSION)
    {
        setScriptWinLoseVideo(PLAY_NONE);
    }

    //need to clear before setting up
    clearMissionWidgets();
    //put any widgets back on for the missions
    resetMissionWidgets();

	debug( LOG_NEVER, "loadGame: done\n" );

	return TRUE;

error:
		debug( LOG_NEVER, "loadgame: ERROR\n" );

	/* Clear all the objects off the map and free up the map memory */
	freeAllDroids();
	freeAllStructs();
	freeAllFeatures();
	droidTemplateShutDown();
	if (psMapTiles)
	{
//		FREE(psMapTiles);
//		mapFreeTilesAndStrips();
	}
	if (aMapLinePoints)
	{
		FREE(aMapLinePoints);
	}
	psMapTiles = NULL;
	aMapLinePoints = NULL;

	/*if (!loadFile("blank.map", &pFileData, &fileSize))
	{
		return FALSE;
	}

	if (!mapLoad(pFileData, fileSize))
	{
		return FALSE;
	}

	FREE(pFileData);*/

	/* Start the game clock */
	gameTimeStart();
//	if (multiPlayerInUse)
//	{
//		bMultiPlayer = TRUE;				// reenable multi player messages.
//		multiPlayerInUse = FALSE;
//	}

	return FALSE;
}
// -----------------------------------------------------------------------------------------
#ifdef ALLOWSAVE

// Modified by AlexL , now takes a filename, with no popup....
BOOL saveGame(char *aFileName, SDWORD saveType)
{
	UDWORD			fileExtension;
	DROID			*psDroid, *psNext;

	debug(LOG_WZ, "saveGame: %s", aFileName);

	fileExtension = strlen(aFileName) - 3;
	gameTimeStop();


	/* Write the data to the file */
	if (!writeGameFile(aFileName, saveType))
	{
		goto error;
	}

	//remove the file extension
	aFileName[strlen(aFileName) - 4] = '\0';

	//create dir will fail if directory already exists but don't care!
	(void) PHYSFS_mkdir(aFileName);

	//save the map file
	strcat(aFileName, "/game.map");
	/* Write the data to the file */
	if (!writeMapFile(aFileName))
	{
		goto error;
	}

	//create the droids filename
	aFileName[fileExtension] = '\0';
	strcat(aFileName, "unit.bjo");
	/*Write the current droid lists to the file*/
	//ppsCurrentDroidLists = apsDroidLists;
	if (!writeDroidFile(aFileName,apsDroidLists))
	{
		goto error;
	}

	//create the structures filename
	aFileName[fileExtension] = '\0';
	strcat(aFileName, "struct.bjo");
	/*Write the data to the file*/
	if (!writeStructFile(aFileName))
	{
		goto error;
	}
/*
    //we do this later on!!!!
	//create the production filename
	// aFileName[fileExtension] = '\0';
	strcat(aFileName, "prod.bjo");
	//Write the data to the file
	if (!writeProductionFile(aFileName))
	{
		goto error;
	}
*/

	//create the templates filename
	aFileName[fileExtension] = '\0';
	strcat(aFileName, "templ.bjo");
	/*Write the data to the file*/
	if (!writeTemplateFile(aFileName))
	{
		goto error;
	}

	//create the features filename
	aFileName[fileExtension] = '\0';
	strcat(aFileName, "feat.bjo");
	/*Write the data to the file*/
	if (!writeFeatureFile(aFileName))
	{
		goto error;
	}

	//create the terrain types filename
	aFileName[fileExtension] = '\0';
	strcat(aFileName, "ttypes.ttp");
	/*Write the data to the file*/
	if (!writeTerrainTypeMapFile(aFileName))
	{
		goto error;
	}

	//create the strucutLimits filename
	aFileName[fileExtension] = '\0';
	strcat(aFileName, "limits.bjo");
	/*Write the data to the file*/
	if (!writeStructLimitsFile(aFileName))
	{
		goto error;
	}

	//create the component lists filename
	aFileName[fileExtension] = '\0';
	strcat(aFileName, "compl.bjo");
	/*Write the data to the file*/
	if (!writeCompListFile(aFileName))
	{
		goto error;
	}
	//create the structure type lists filename
	aFileName[fileExtension] = '\0';
	strcat(aFileName, "strtype.bjo");
	/*Write the data to the file*/
	if (!writeStructTypeListFile(aFileName))
	{
		goto error;
	}

	//create the research filename
	aFileName[fileExtension] = '\0';
	strcat(aFileName, "resstate.bjo");
	/*Write the data to the file*/
	if (!writeResearchFile(aFileName))
	{
		goto error;
	}

//#ifdef NEW_SAVE //V11 Save
	//create the message filename
	aFileName[fileExtension] = '\0';
	strcat(aFileName, "messtate.bjo");
	/*Write the data to the file*/
	if (!writeMessageFile(aFileName))
	{
		goto error;
	}
//#endif

//#ifdef NEW_SAVE //V14 Save
	//create the proximity message filename
	aFileName[fileExtension] = '\0';
	strcat(aFileName, "proxstate.bjo");
	/*Write the data to the file*/
	if (!writeMessageFile(aFileName))
	{
		goto error;
	}
//#endif

//#ifdef NEW_SAVE //V11 Save
	//create the message filename
	aFileName[fileExtension] = '\0';
	strcat(aFileName, "visstate.bjo");
	/*Write the data to the file*/
	if (!writeVisibilityData(aFileName))
	{
		goto error;
	}
//#endif

//#ifdef NEW_SAVE_V13 //V13 Save
	//create the message filename
	aFileName[fileExtension] = '\0';
	strcat(aFileName, "prodstate.bjo");
	/*Write the data to the file*/
	if (!writeProductionFile(aFileName))
	{
		goto error;
	}

//#endif

//#ifdef FX_SAVE //added at V13 save
	//create the message filename
	aFileName[fileExtension] = '\0';
	strcat(aFileName, "fxstate.bjo");
	/*Write the data to the file*/
	if (!writeFXData(aFileName))
	{
		goto error;
	}
//#endif

	//added at V15 save
	//create the message filename
	aFileName[fileExtension] = '\0';
	strcat(aFileName, "score.bjo");
	/*Write the data to the file*/
	if (!writeScoreData(aFileName))
	{
		goto error;
	}
//#endif

//#ifdef NEW_SAVE //V12 Save
	//create the message filename
	aFileName[fileExtension] = '\0';
	strcat(aFileName, "flagstate.bjo");
	/*Write the data to the file*/
	if (!writeFlagFile(aFileName))
	{
		goto error;
	}
//#endif

//#ifdef NEW_SAVE //V21 Save
	//create the message filename
	aFileName[fileExtension] = '\0';
	strcat(aFileName, "command.bjo");
	/*Write the data to the file*/
	if (!writeCommandLists(aFileName))
	{
		goto error;
	}
//#endif

	//create the structLimits filename
	/*aFileName[sOFN.nFileExtension] = '\0';
	strcat(aFileName, "limits.bjo");*/
	/*Write the data to the file DONE IN SCRIPTS NOW*/
	/*if (!writeStructLimitsFile(aFileName))
	{
		goto error;
	}*/

	// save the script state if necessary
	if (saveType == GTYPE_SAVE_MIDMISSION)
	{
		aFileName[fileExtension-1] = '\0';
		strcat(aFileName, ".es");
		/*Write the data to the file*/
		if (!writeScriptState(aFileName))
		{
			goto error;
		}
	}

	//create the droids filename
	aFileName[fileExtension-1] = '\0';
	strcat(aFileName, "/munit.bjo");
	/*Write the swapped droid lists to the file*/
	//ppsCurrentDroidLists = mission.apsDroidLists;
	if (!writeDroidFile(aFileName, mission.apsDroidLists))
	{
		goto error;
	}

	//21feb now done always
	//create the limbo filename
	//clear the list
	if (saveGameVersion < VERSION_25)
	{
		for (psDroid = apsLimboDroids[selectedPlayer]; psDroid != NULL; psDroid = psNext)
		{
			psNext = psDroid->psNext;
			//limbo list invalidate XY
			psDroid->x = INVALID_XY;
			psDroid->y = INVALID_XY;
            //this is mainly for VTOLs
            psDroid->psBaseStruct = NULL;
            psDroid->cluster = 0;
			orderDroid(psDroid, DORDER_STOP);
        }
	}

	aFileName[fileExtension] = '\0';
	strcat(aFileName, "limbo.bjo");
	/*Write the swapped droid lists to the file*/
	//ppsCurrentDroidLists = apsLimboDroids;
	if (!writeDroidFile(aFileName, apsLimboDroids))
	{
		goto error;
	}

	if (saveGameOnMission )
	{
		//mission save swap the mission pointers and save the changes
		swapMissionPointers();
		//now save the map and droids


		//save the map file
		aFileName[fileExtension] = '\0';
		strcat(aFileName, "mission.map");
		/* Write the data to the file */
		if (!writeMapFile(aFileName))
		{
			goto error;
		}

		//save the map file
		aFileName[fileExtension] = '\0';
		strcat(aFileName, "misvis.bjo");
		/* Write the data to the file */
		if (!writeVisibilityData(aFileName))
		{
			goto error;
		}

		//create the structures filename
		aFileName[fileExtension] = '\0';
		strcat(aFileName, "mstruct.bjo");
		/*Write the data to the file*/
		if (!writeStructFile(aFileName))
		{
			goto error;
		}

		//create the features filename
		aFileName[fileExtension] = '\0';
		strcat(aFileName, "mfeat.bjo");
		/*Write the data to the file*/
		if (!writeFeatureFile(aFileName))
		{
			goto error;
		}

		//create the message filename
		aFileName[fileExtension] = '\0';
		strcat(aFileName, "mflagstate.bjo");
		/*Write the data to the file*/
		if (!writeFlagFile(aFileName))
		{
			goto error;
		}

		//mission save swap back so we can restart the game
		swapMissionPointers();
	}

	/* Start the game clock */
	gameTimeStart();
	return TRUE;

error:
	/* Start the game clock */
	gameTimeStart();

	return FALSE;
}

// -----------------------------------------------------------------------------------------
BOOL writeMapFile(char *pFileName)
{
	char *pFileData = NULL;
	UDWORD fileSize;
	BOOL status = TRUE;

	/* Get the save data */
	status = mapSave(&pFileData, &fileSize);

	if (status) {
		/* Write the data to the file */
		status = saveFile(pFileName, pFileData, fileSize);
	}
	if (pFileData != NULL) {
		FREE(pFileData);
	}
	return status;
}

#endif


// -----------------------------------------------------------------------------------------
BOOL gameLoad(char *pFileData, UDWORD filesize)
{
	GAME_SAVEHEADER			*psHeader;

	debug( LOG_WZ, "gameLoad" );

	/* Check the file type */
	psHeader = (GAME_SAVEHEADER *)pFileData;
	if (psHeader->aFileType[0] != 'g' || psHeader->aFileType[1] != 'a' ||
		psHeader->aFileType[2] != 'm' || psHeader->aFileType[3] != 'e')
	{
		debug( LOG_ERROR, "gameLoad: Incorrect file type" );
		abort();
		return FALSE;
	}

	/* GAME_SAVEHEADER */
	endian_udword(&psHeader->version);

	//increment to the start of the data
	pFileData += GAME_HEADER_SIZE;

	debug( LOG_NEVER, "gl .gam file is version %d\n", psHeader->version );
	//set main version Id from game file
	saveGameVersion = psHeader->version;


	/* Check the file version */
	if (psHeader->version < VERSION_7)
	{
		debug( LOG_ERROR, "gameLoad: unsupported save format version %d", psHeader->version );
		abort();
		return FALSE;
	}
	else if (psHeader->version < VERSION_9)
	{
		if (!gameLoadV7(pFileData, filesize))
		{
			return FALSE;
		}
	}
	else if (psHeader->version <= CURRENT_VERSION_NUM)
	{
		if (!gameLoadV(pFileData, filesize, psHeader->version))
		{
			return FALSE;
		}
	}
	else
	{
		debug( LOG_ERROR, "gameLoad: undefined save format version %d", psHeader->version );
		abort();
		return FALSE;
	}

//	DBMB(("IsScenario = %d\nfor the game that's being loaded.", IsScenario));

	return TRUE;
}

// Fix endianness of a savegame
static void endian_SaveGameV(SAVE_GAME* psSaveGame, UDWORD version) {
    UDWORD i,j;
	/* SAVE_GAME is GAME_SAVE_V33 */
	/* GAME_SAVE_V33 includes GAME_SAVE_V31 */
	if(version >= VERSION_33) {
		endian_udword(&psSaveGame->sGame.power);
		endian_uword(&psSaveGame->sGame.bytesPerSec);
		for(i = 0; i < MaxGames; i++) {
			endian_sdword(&psSaveGame->sNetPlay.games[i].desc.dwSize);
			endian_sdword(&psSaveGame->sNetPlay.games[i].desc.dwFlags);
			endian_sdword(&psSaveGame->sNetPlay.games[i].desc.dwMaxPlayers);
			endian_sdword(&psSaveGame->sNetPlay.games[i].desc.dwCurrentPlayers);
			endian_sdword(&psSaveGame->sNetPlay.games[i].desc.dwUser1);
			endian_sdword(&psSaveGame->sNetPlay.games[i].desc.dwUser2);
			endian_sdword(&psSaveGame->sNetPlay.games[i].desc.dwUser3);
			endian_sdword(&psSaveGame->sNetPlay.games[i].desc.dwUser4);
		}
		for(i = 0; i < MaxNumberOfPlayers; i++)
			endian_udword(&psSaveGame->sNetPlay.players[i].dpid);
		endian_udword(&psSaveGame->sNetPlay.playercount);
		endian_udword(&psSaveGame->sNetPlay.dpidPlayer);
		for(i = 0; i < 4; i++)
			endian_udword(&psSaveGame->sNetPlay.cryptKey[i]);
		endian_udword(&psSaveGame->savePlayer);
		for(i = 0; i < MAX_PLAYERS; i++)
			endian_udword(&psSaveGame->sPlayer2dpid[i]);
	}
	/* GAME_SAVE_V31 includes GAME_SAVE_V30 */
	if(version >= VERSION_31) {
		endian_sdword(&psSaveGame->missionCheatTime);
	}
	/* GAME_SAVE_V30 includes GAME_SAVE_V29 */
	if(version >= VERSION_30) {
		endian_sdword(&psSaveGame->scrGameLevel);
	}
	/* GAME_SAVE_V29 includes GAME_SAVE_V27 */
	if(version >= VERSION_29) {
		endian_uword(&psSaveGame->missionScrollMinX);
		endian_uword(&psSaveGame->missionScrollMinY);
		endian_uword(&psSaveGame->missionScrollMaxX);
		endian_uword(&psSaveGame->missionScrollMaxY);
	}
	/* GAME_SAVE_V27 includes GAME_SAVE_V24 */
	if(version >= VERSION_27) {
		for(i = 0; i < MAX_PLAYERS; i++)
			for(j = 0; j < MAX_RECYCLED_DROIDS; j++)
				endian_uword(&psSaveGame->awDroidExperience[i][j]);
	}
	/* GAME_SAVE_V24 includes GAME_SAVE_V22 */
	if(version >= VERSION_24) {
		endian_udword(&psSaveGame->reinforceTime);
	}
	/* GAME_SAVE_V22 includes GAME_SAVE_V20 */
	if(version >= VERSION_22) {
		for(i = 0; i < MAX_PLAYERS; i++) {
			endian_sdword(&psSaveGame->asRunData[i].sPos.x);
			endian_sdword(&psSaveGame->asRunData[i].sPos.y);
		}
	}
	/* GAME_SAVE_V20 includes GAME_SAVE_V19 */
	if(version >= VERSION_20) {
		for(i = 0; i < MAX_PLAYERS; i++) {
			endian_sdword(&psSaveGame->asVTOLReturnPos[i].x);
			endian_sdword(&psSaveGame->asVTOLReturnPos[i].y);
		}
	}
	/* GAME_SAVE_V19 includes GAME_SAVE_V18 */
	if(version >= VERSION_19) {
	}
	/* GAME_SAVE_V18 includes GAME_SAVE_V17 */
	if(version >= VERSION_18) {
		endian_udword(&psSaveGame->oldestVersion);
		endian_udword(&psSaveGame->validityKey);
	}
	/* GAME_SAVE_V17 includes GAME_SAVE_V16 */
	if(version >= VERSION_17) {
		endian_udword(&psSaveGame->objId);
	}
	/* GAME_SAVE_V16 includes GAME_SAVE_V15 */
	if(version >= VERSION_16) {
	}
	/* GAME_SAVE_V15 includes GAME_SAVE_V14 */
	if(version >= VERSION_15) {
		endian_udword(&psSaveGame->RubbleTile);
		endian_udword(&psSaveGame->WaterTile);
		endian_udword(&psSaveGame->fogColour);
		endian_udword(&psSaveGame->fogState);
	}
	/* GAME_SAVE_V14 includes GAME_SAVE_V12 */
	if(version >= VERSION_14) {
		endian_sdword(&psSaveGame->missionOffTime);
		endian_sdword(&psSaveGame->missionETA);
		endian_uword(&psSaveGame->missionHomeLZ_X);
		endian_uword(&psSaveGame->missionHomeLZ_Y);
		endian_sdword(&psSaveGame->missionPlayerX);
		endian_sdword(&psSaveGame->missionPlayerY);
		for(i = 0; i < MAX_PLAYERS; i++) {
			endian_uword(&psSaveGame->iTranspEntryTileX[i]);
			endian_uword(&psSaveGame->iTranspEntryTileY[i]);
			endian_uword(&psSaveGame->iTranspExitTileX[i]);
			endian_uword(&psSaveGame->iTranspExitTileY[i]);
			endian_udword(&psSaveGame->aDefaultSensor[i]);
			endian_udword(&psSaveGame->aDefaultECM[i]);
			endian_udword(&psSaveGame->aDefaultRepair[i]);
		}
	}
	/* GAME_SAVE_V12 includes GAME_SAVE_V11 */
	if(version >= VERSION_12) {
		endian_udword(&psSaveGame->missionTime);
		endian_udword(&psSaveGame->saveKey);
	}
	/* GAME_SAVE_V11 includes GAME_SAVE_V10 */
	if(version >= VERSION_11) {
		endian_sdword(&psSaveGame->currentPlayerPos.p.x);
		endian_sdword(&psSaveGame->currentPlayerPos.p.y);
		endian_sdword(&psSaveGame->currentPlayerPos.p.z);
		endian_sdword(&psSaveGame->currentPlayerPos.r.x);
		endian_sdword(&psSaveGame->currentPlayerPos.r.y);
		endian_sdword(&psSaveGame->currentPlayerPos.r.z);
	}
	/* GAME_SAVE_V10 includes GAME_SAVE_V7 */
	if(version >= VERSION_10) {
		for(i = 0; i < MAX_PLAYERS; i++) {
			endian_udword(&psSaveGame->power[i].currentPower);
			endian_udword(&psSaveGame->power[i].extractedPower);
		}
	}
	/* GAME_SAVE_V7 */
	if(version >= VERSION_7) {
		endian_udword(&psSaveGame->gameTime);
		endian_udword(&psSaveGame->GameType);
		endian_sdword(&psSaveGame->ScrollMinX);
		endian_sdword(&psSaveGame->ScrollMinY);
		endian_udword(&psSaveGame->ScrollMaxX);
		endian_udword(&psSaveGame->ScrollMaxY);
	}
}

// -----------------------------------------------------------------------------------------
// Get campaign number stuff is not needed in this form on the PSX (thank you very much)
static BOOL getCampaignV(char *pFileData, UDWORD filesize, UDWORD version)
{
	SAVE_GAME		*psSaveGame;
	UDWORD			sizeOfSaveGame = 0;
	UDWORD			campaign;

	debug(LOG_WZ, "getCampaignV: version=%d", version);

	psSaveGame = (SAVE_GAME *) pFileData;


	//size is now variable so only check old save games
	if (version < VERSION_14)
	{
		return 0;
	}
	else if (version <= VERSION_16)
	{
		sizeOfSaveGame = sizeof(SAVE_GAME_V16);
	}
	else if (version <= VERSION_17)
	{
		sizeOfSaveGame = sizeof(SAVE_GAME_V17);
	}
	else if (version <= VERSION_18)
	{
		sizeOfSaveGame = sizeof(SAVE_GAME_V18);
	}
	else if (version <= VERSION_19)
	{
		sizeOfSaveGame = sizeof(SAVE_GAME_V19);
	}
	else if (version <= VERSION_21)
	{
		sizeOfSaveGame = sizeof(SAVE_GAME_V20);
	}
	else if (version <= VERSION_23)
	{
		sizeOfSaveGame = sizeof(SAVE_GAME_V22);
	}
	else if (version <= VERSION_26)
	{
		sizeOfSaveGame = sizeof(SAVE_GAME_V24);
	}
	else if (version <= VERSION_28)
	{
		sizeOfSaveGame = sizeof(SAVE_GAME_V27);
	}
    else if (version <= VERSION_29)
    {
        sizeOfSaveGame = sizeof(SAVE_GAME_V29);
    }
    else if (version <= VERSION_30)
    {
        sizeOfSaveGame = sizeof(SAVE_GAME_V30);
    }
	else if (version <= VERSION_32)
    {
        sizeOfSaveGame = sizeof(SAVE_GAME_V31);
    }
	else if (version <= VERSION_33)
    {
        sizeOfSaveGame = sizeof(SAVE_GAME_V33);
    }
	else if (version <= CURRENT_VERSION_NUM)
	{
		sizeOfSaveGame = sizeof(SAVE_GAME);
	}

	if ((sizeOfSaveGame + GAME_HEADER_SIZE) > filesize)
	{
		debug( LOG_ERROR, "getCampaign: unexpected end of file (expected %d, have %d)" ,
			(sizeOfSaveGame + GAME_HEADER_SIZE) , filesize);
		abort();
		return FALSE;
	}

	endian_SaveGameV(psSaveGame, version);

//	savedGameTime = psSaveGame->gameTime;

	campaign = psSaveGame->saveKey;
	campaign &= (SAVEKEY_ONMISSION - 1);
	return campaign;
}

// -----------------------------------------------------------------------------------------
// Returns the campaign number  --- apparently this is does alot less than it look like
    /// it now does even less than it looks like on the psx ... cause its pc only
UDWORD getCampaign(char *pGameToLoad, BOOL *bSkipCDCheck)
{
	char *pFileData = NULL;
	UDWORD			fileSize;
	GAME_SAVEHEADER			*psHeader;

	debug(LOG_WZ, "getCampaign: %s", pGameToLoad);

	/* Load in the chosen file data */
	pFileData = DisplayBuffer;
	if (!loadFileToBuffer(pGameToLoad, pFileData, displayBufferSize, &fileSize))
	{
		debug( LOG_NEVER, "loadgame: Fail2\n" );
		return 0;
	}

	// loaded the .gam file so parse the data
	/* Check the file type */
	psHeader = (GAME_SAVEHEADER *)pFileData;
	if (psHeader->aFileType[0] != 'g' || psHeader->aFileType[1] != 'a' ||
		psHeader->aFileType[2] != 'm' || psHeader->aFileType[3] != 'e')
	{
		debug( LOG_ERROR, "getCampaign: Incorrect file type" );
		abort();
		return 0;
	}

	/* GAME_SAVEHEADER */
	endian_udword(&psHeader->version);

	//increment to the start of the data
	pFileData += GAME_HEADER_SIZE;

	debug( LOG_NEVER, "gl .gam file is version %d\n", psHeader->version );
	//set main version Id from game file
	saveGameVersion = psHeader->version;


	/* Check the file version */
	if (psHeader->version < VERSION_14)
	{
		return 0;
	}


	// what the arse bollocks is this
			// the campaign number is fine prior to saving
			// you save it out in a skirmish save and
			// then don't bother putting it back in again
			// when loading so it screws loads of stuff?!?
	// dont check skirmish saves.
	else if( psHeader->version >= VERSION_33)
	{
		if(((SAVE_GAME *)pFileData)->multiPlayer == 1)
		{
//			return 0;
			*bSkipCDCheck = TRUE;
		}
	}


	if (psHeader->version <= CURRENT_VERSION_NUM)
	{
		return getCampaignV(pFileData, fileSize, psHeader->version);
	}

	else
	{
		debug( LOG_ERROR, "getCampaign: undefined save format version %d", psHeader->version );
		abort();
		return 0;
	}

//	DBMB(("IsScenario = %d\nfor the game that's being loaded.", IsScenario));

	return 0;
}

// -----------------------------------------------------------------------------------------
void game_SetValidityKey(UDWORD keys)
{
	validityKey = validityKey|keys;
	return;
}

// -----------------------------------------------------------------------------------------
/* code specific to version 7 of a save game */
BOOL gameLoadV7(char *pFileData, UDWORD filesize)
{
	SAVE_GAME_V7				*psSaveGame;
	LEVEL_DATASET				*psNewLevel;

//	DBERROR(("gameLoadV7: this is and outdated save game"));

	psSaveGame = (SAVE_GAME_V7 *) pFileData;

	if ((sizeof(SAVE_GAME_V7) + GAME_HEADER_SIZE) > filesize)
	{
		debug( LOG_ERROR, "gameLoad: unexpected end of file" );
		abort();
		return FALSE;
	}

	/* GAME_SAVE_V7 */
	endian_udword(&psSaveGame->gameTime);
	endian_udword(&psSaveGame->GameType);
	endian_sdword(&psSaveGame->ScrollMinX);
	endian_sdword(&psSaveGame->ScrollMinY);
	endian_udword(&psSaveGame->ScrollMaxX);
	endian_udword(&psSaveGame->ScrollMaxY);

	savedGameTime = psSaveGame->gameTime;

	//set the scroll varaibles
	startX = psSaveGame->ScrollMinX;
	startY = psSaveGame->ScrollMinY;
	width = psSaveGame->ScrollMaxX - psSaveGame->ScrollMinX;
	height = psSaveGame->ScrollMaxY - psSaveGame->ScrollMinY;
	gameType = psSaveGame->GameType;
	//set IsScenario to TRUE if not a user saved game
	if (gameType == GTYPE_SAVE_START)
	{
		IsScenario = FALSE;
		//copy the level name across
		strcpy(pLevelName, psSaveGame->levelName);
		//load up the level dataset
		if (!levLoadData(pLevelName, saveGameName, gameType))
		{
			return FALSE;
		}
		// find the level dataset
		if (!levFindDataSet(pLevelName, &psNewLevel))
		{
			debug( LOG_ERROR, "gameLoadV6: couldn't find level data" );
			abort();
			return FALSE;
		}
		//check to see whether mission automatically starts
		//shouldn't be able to be any other value at the moment!
		if (psNewLevel->type == LDS_CAMSTART
			|| psNewLevel->type == LDS_BETWEEN
			|| psNewLevel->type == LDS_EXPAND
			|| psNewLevel->type == LDS_EXPAND_LIMBO
		    )
		{
			launchMission();
		}

	}
	else
	{
		IsScenario = TRUE;
	}

	return TRUE;
}

// -----------------------------------------------------------------------------------------
/* non specific version of a save game */
BOOL gameLoadV(char *pFileData, UDWORD filesize, UDWORD version)
{
	SAVE_GAME		*psSaveGame;
//	LEVEL_DATASET	*psNewLevel;
	UBYTE			inc;
	SDWORD			i, j;
	static	SAVE_POWER	powerSaved[MAX_PLAYERS];
	UDWORD			sizeOfSaveGame = 0;
	UDWORD			player;
	char			date[MAX_STR_LENGTH];

	debug(LOG_WZ, "gameLoadV: version %d", version);

	psSaveGame = &saveGameData;

	memcpy(psSaveGame, pFileData,sizeof(SAVE_GAME));

  	//VERSION_7 AND EARLIER LOADED SEPARATELY

	//size is now variable so only check old save games
	if (version <= VERSION_10)
	{
		sizeOfSaveGame = sizeof(SAVE_GAME_V10);
	}
	else if (version == VERSION_11)
	{
		sizeOfSaveGame = sizeof(SAVE_GAME_V11);
	}
	else if (version <= VERSION_12)
	{
		sizeOfSaveGame = sizeof(SAVE_GAME_V12);
	}
	else if (version <= VERSION_14)
	{
		sizeOfSaveGame = sizeof(SAVE_GAME_V14);
	}
	else if (version <= VERSION_15)
	{
		sizeOfSaveGame = sizeof(SAVE_GAME_V15);
	}
	else if (version <= VERSION_16)
	{
		sizeOfSaveGame = sizeof(SAVE_GAME_V16);
	}
	else if (version <= VERSION_17)
	{
		sizeOfSaveGame = sizeof(SAVE_GAME_V17);
	}
	else if (version <= VERSION_18)
	{
		sizeOfSaveGame = sizeof(SAVE_GAME_V18);
	}
	else if (version <= VERSION_19)
	{
		sizeOfSaveGame = sizeof(SAVE_GAME_V19);
	}
	else if (version <= VERSION_21)
	{
		sizeOfSaveGame = sizeof(SAVE_GAME_V20);
	}
	else if (version <= VERSION_23)
	{
		sizeOfSaveGame = sizeof(SAVE_GAME_V22);
	}
	else if (version <= VERSION_26)
	{
		sizeOfSaveGame = sizeof(SAVE_GAME_V24);
	}
	else if (version <= VERSION_28)
	{
		sizeOfSaveGame = sizeof(SAVE_GAME_V27);
	}
	else if (version <= VERSION_29)
	{
		sizeOfSaveGame = sizeof(SAVE_GAME_V29);
	}
	else if (version <= VERSION_30)
	{
		sizeOfSaveGame = sizeof(SAVE_GAME_V30);
	}
	else if (version <= VERSION_32)
    {
        sizeOfSaveGame = sizeof(SAVE_GAME_V31);
    }
	else if (version <= VERSION_33)
    {
        sizeOfSaveGame = sizeof(SAVE_GAME_V33);
    }
	else if (version <= CURRENT_VERSION_NUM)
	{
		sizeOfSaveGame = sizeof(SAVE_GAME);
	}

	if ((sizeOfSaveGame + GAME_HEADER_SIZE) > filesize)
	{
		debug( LOG_ERROR, "gameLoad: unexpected end of file" );
		abort();
		return FALSE;
	}
    endian_SaveGameV(psSaveGame, version);

	savedGameTime = psSaveGame->gameTime;

	if (version >= VERSION_12)
	{
		mission.startTime = psSaveGame->missionTime;
		if (psSaveGame->saveKey & SAVEKEY_ONMISSION)
		{
			saveGameOnMission = TRUE;
		}
		else
		{
			saveGameOnMission = FALSE;
		}

	}
	else
	{
		saveGameOnMission = FALSE;
	}
	//set the scroll varaibles
	startX = psSaveGame->ScrollMinX;
	startY = psSaveGame->ScrollMinY;
	width = psSaveGame->ScrollMaxX - psSaveGame->ScrollMinX;
	height = psSaveGame->ScrollMaxY - psSaveGame->ScrollMinY;
	gameType = psSaveGame->GameType;

	if (version >= VERSION_11)
	{
		//camera position
		disp3d_setView(&(psSaveGame->currentPlayerPos));
	}

	//load mission data from save game these values reloaded after load game

	if (version >= VERSION_14)
	{
		//mission data
		mission.time		=	psSaveGame->missionOffTime;
		mission.ETA			=	psSaveGame->missionETA;
		mission.homeLZ_X	=	psSaveGame->missionHomeLZ_X;
		mission.homeLZ_Y	=	psSaveGame->missionHomeLZ_Y;
		mission.playerX		=	psSaveGame->missionPlayerX;
		mission.playerY		=	psSaveGame->missionPlayerY;
		for (player = 0; player < MAX_PLAYERS; player++)
		{
			mission.iTranspEntryTileX[player]	= psSaveGame->iTranspEntryTileX[player];
			mission.iTranspEntryTileY[player]	= psSaveGame->iTranspEntryTileY[player];
			mission.iTranspExitTileX[player]	= psSaveGame->iTranspExitTileX[player];
			mission.iTranspExitTileY[player]	= psSaveGame->iTranspExitTileY[player];
			aDefaultSensor[player]				= psSaveGame->aDefaultSensor[player];
			aDefaultECM[player]					= psSaveGame->aDefaultECM[player];
			aDefaultRepair[player]				= psSaveGame->aDefaultRepair[player];
		}
	}

	if (version >= VERSION_15)
	{
		offWorldKeepLists	= psSaveGame->offWorldKeepLists;
		setRubbleTile(psSaveGame->RubbleTile);
		setUnderwaterTile(psSaveGame->WaterTile);
		if (psSaveGame->fogState == 0)//no fog
		{
			pie_EnableFog(FALSE);
			fogStatus = 0;
		}
		else if (psSaveGame->fogState == 1)//fog using old code assume background and depth
		{
			if (war_GetFog())
			{
				pie_EnableFog(TRUE);
			}
			else
			{
				pie_EnableFog(FALSE);
			}
			fogStatus = FOG_BACKGROUND + FOG_DISTANCE;
		}
		else//version 18+ fog
		{
			if (war_GetFog())
			{
				pie_EnableFog(TRUE);
			}
			else
			{
				pie_EnableFog(FALSE);
			}
			fogStatus = psSaveGame->fogState;
			fogStatus &= FOG_FLAGS;
		}
		pie_SetFogColour(psSaveGame->fogColour);
	}

	if (version >= VERSION_17)
	{
		objID = psSaveGame->objId;// this must be done before any new Ids added
		savedObjId = psSaveGame->objId;
	}

	if (version >= VERSION_18)//version 18
	{
		validityKey = psSaveGame->validityKey;
		oldestSaveGameVersion = psSaveGame->oldestVersion;
		if (oldestSaveGameVersion > version)
		{
			oldestSaveGameVersion = version;
			validityKey = validityKey|VALIDITYKEY_VERSION;
		}
		else if (oldestSaveGameVersion < version)
		{
			validityKey = validityKey|VALIDITYKEY_VERSION;
		}

		strcpy(date,__DATE__);
		ASSERT( strlen(date)<MAX_STR_LENGTH,"BuildDate; String error" );
		if (strcmp(psSaveGame->buildDate,date) != 0)
		{
//			ASSERT( gameType != GTYPE_SAVE_MIDMISSION,"Mid-game save out of date. Continue with caution." );
			debug( LOG_NEVER, "saveGame build date differs;\nsavegame %s\n build    %s\n", psSaveGame->buildDate, date );
			validityKey = validityKey|VALIDITYKEY_DATE;
			if (gameType == GTYPE_SAVE_MIDMISSION)
			{
				validityKey = validityKey|VALIDITYKEY_MID_GAME;
			}
		}
	}
	else
	{
		debug( LOG_NEVER, "saveGame build date differs;\nsavegame pre-Version 18 (%s)\n build    %s\n", psSaveGame->buildDate, date );
		oldestSaveGameVersion = 1;
		validityKey = VALIDITYKEY_DATE;
	}


	if (version >= VERSION_19)//version 19
	{
		for(i=0; i<MAX_PLAYERS; i++)
		{
			for(j=0; j<MAX_PLAYERS; j++)
			{
				alliances[i][j] = psSaveGame->alliances[i][j];
			}
		}
		for(i=0; i<MAX_PLAYERS; i++)
		{
			setPlayerColour(i,psSaveGame->playerColour[i]);
		}
		SetRadarZoom(psSaveGame->radarZoom);
	}

	if (version >= VERSION_20)//version 20
	{
		setDroidsToSafetyFlag(psSaveGame->bDroidsToSafetyFlag);
		for (inc = 0; inc < MAX_PLAYERS; inc++)
		{
			memcpy(&asVTOLReturnPos[inc],&(psSaveGame->asVTOLReturnPos[inc]),sizeof(POINT));
		}
	}

	if (version >= VERSION_22)//version 22
	{
		for (inc = 0; inc < MAX_PLAYERS; inc++)
		{
			memcpy(&asRunData[inc],&(psSaveGame->asRunData[inc]),sizeof(RUN_DATA));
		}
	}

	if (saveGameVersion >= VERSION_24)//V24
	{
		missionSetReinforcementTime(psSaveGame->reinforceTime);

		// horrible hack to catch savegames that were saving garbage into these fields
		if (psSaveGame->bPlayCountDown <= 1)
		{
			setPlayCountDown(psSaveGame->bPlayCountDown);
		}
		if (psSaveGame->bPlayerHasWon <= 1)
		{
			setPlayerHasWon(psSaveGame->bPlayerHasWon);
		}
		if (psSaveGame->bPlayerHasLost <= 1)
		{
			setPlayerHasLost(psSaveGame->bPlayerHasLost);
		}
	}

    if (saveGameVersion >= VERSION_29)
    {
        mission.scrollMinX = psSaveGame->missionScrollMinX;
        mission.scrollMinY = psSaveGame->missionScrollMinY;
        mission.scrollMaxX = psSaveGame->missionScrollMaxX;
        mission.scrollMaxY = psSaveGame->missionScrollMaxY;
    }

    if (saveGameVersion >= VERSION_30)
    {
        scrGameLevel = psSaveGame->scrGameLevel;
        bExtraVictoryFlag = psSaveGame->bExtraVictoryFlag;
        bExtraFailFlag = psSaveGame->bExtraFailFlag;
        bTrackTransporter = psSaveGame->bTrackTransporter;
    }

    if (saveGameVersion >= VERSION_31)
    {
        mission.cheatTime = psSaveGame->missionCheatTime;
    }

    for (player = 0; player < MAX_PLAYERS; player++)
	{
		for (inc = 0; inc < MAX_RECYCLED_DROIDS; inc++)
		{
			aDroidExperience[player][inc]	= 0;//clear experience before
		}
	}

	//set IsScenario to TRUE if not a user saved game
	if ((gameType == GTYPE_SAVE_START) ||
		(gameType == GTYPE_SAVE_MIDMISSION))
	{
		for (inc = 0; inc < MAX_PLAYERS; inc++)
		{
			powerSaved[inc].currentPower = psSaveGame->power[inc].currentPower;
			powerSaved[inc].extractedPower = psSaveGame->power[inc].extractedPower;
		}

		for (player = 0; player < MAX_PLAYERS; player++)
		{
			for (inc = 0; inc < MAX_RECYCLED_DROIDS; inc++)
			{
				aDroidExperience[player][inc]	= 0;//clear experience before building saved units
			}
		}


		IsScenario = FALSE;
		//copy the level name across
		strcpy(pLevelName, psSaveGame->levelName);
		//load up the level dataset
		if (!levLoadData(pLevelName, saveGameName, gameType))
		{
			return FALSE;
		}
		// find the level dataset
/*		if (!levFindDataSet(pLevelName, &psNewLevel))
		{
			DBERROR(("gameLoadV6: couldn't find level data"));
			return FALSE;
		}
		//check to see whether mission automatically starts
		if (gameType == GTYPE_SAVE_START)
		{
//			launchMission();
			if (!levLoadData(pLevelName, NULL, 0))
			{
				return FALSE;
			}
		}*/

		if (saveGameVersion >= VERSION_33)
		{
			PLAYERSTATS		playerStats;

			bMultiPlayer	= psSaveGame->multiPlayer;
			NetPlay			= psSaveGame->sNetPlay;
			selectedPlayer	= psSaveGame->savePlayer;
			productionPlayer = selectedPlayer;
			game			= psSaveGame->sGame;
			cmdDroidMultiExpBoost(TRUE);
			for(inc=0;inc<MAX_PLAYERS;inc++)
			{
				player2dpid[inc] = psSaveGame->sPlayer2dpid[inc];
			}
			if(bMultiPlayer)
			{
				loadMultiStats(psSaveGame->sPName,&playerStats);				// stats stuff
				setMultiStats(NetPlay.dpidPlayer,playerStats,FALSE);
				setMultiStats(NetPlay.dpidPlayer,playerStats,TRUE);
			}
		}

	}
	else
	{
		IsScenario = TRUE;
	}

	/* Get human and AI players names */
	if (saveGameVersion >= VERSION_34)
	{
		for(i=0;i<MAX_PLAYERS;i++)
			(void)setPlayerName(i, psSaveGame->sPlayerName[i]);
	}

    //don't adjust any power if a camStart (gameType is set to GTYPE_SCENARIO_START when a camChange saveGame is loaded)
    if (gameType != GTYPE_SCENARIO_START)
    {
	    //set the players power
	    for (inc = 0; inc < MAX_PLAYERS; inc++)
	    {
            //only overwrite selectedPlayer's power on a startMission save game
            if (gameType == GTYPE_SAVE_MIDMISSION || inc == selectedPlayer)
            {
    		    asPower[inc]->currentPower = powerSaved[inc].currentPower;
	    	    asPower[inc]->extractedPower = powerSaved[inc].extractedPower;
            }
		    //init the last structure
		    asPower[inc]->psLastPowered = NULL;
	    }
    }

	return TRUE;
}


// -----------------------------------------------------------------------------------------
/*
Writes the game specifics to a file
*/
BOOL writeGameFile(char *pFileName, SDWORD saveType)
{
	char *pFileData = NULL;
	UDWORD				fileSize, inc;
	UDWORD				player;
	SDWORD				i, j;
	GAME_SAVEHEADER		*psHeader;
	SAVE_GAME			*psSaveGame;
	LANDING_ZONE		*psLandingZone;
	char				date[MAX_STR_SIZE];
	BOOL status = TRUE;

	ASSERT( saveType == GTYPE_SAVE_START ||
			saveType == GTYPE_SAVE_MIDMISSION,
			"writeGameFile: invalid save type" );


	/* Allocate the data buffer */
	fileSize = GAME_HEADER_SIZE + sizeof(SAVE_GAME);
	pFileData = (char*)MALLOC(fileSize);
	if (pFileData == NULL)
	{
		debug( LOG_ERROR, "Out of memory" );
		abort();
		return FALSE;
	}

	/* Put the file header on the file */
	psHeader = (GAME_SAVEHEADER *)pFileData;
	psHeader->aFileType[0] = 'g';
	psHeader->aFileType[1] = 'a';
	psHeader->aFileType[2] = 'm';
	psHeader->aFileType[3] = 'e';
	psHeader->version = CURRENT_VERSION_NUM;

	psSaveGame = (SAVE_GAME*)(pFileData + GAME_HEADER_SIZE);

	// saveKeymissionIsOffworld
	psSaveGame->saveKey = getCampaignNumber();
	if (missionIsOffworld())
	{
		psSaveGame->saveKey |= SAVEKEY_ONMISSION;
		saveGameOnMission = TRUE;
	}
	else
	{
		saveGameOnMission = FALSE;
	}


	/* Put the save game data into the buffer */
	psSaveGame->gameTime = gameTime;
	psSaveGame->missionTime = mission.startTime;

	//put in the scroll data
	psSaveGame->ScrollMinX = scrollMinX;
	psSaveGame->ScrollMinY = scrollMinY;
	psSaveGame->ScrollMaxX = scrollMaxX;
	psSaveGame->ScrollMaxY = scrollMaxY;

	psSaveGame->GameType = saveType;

	//save the current level so we can load up the STARTING point of the mission
	if (strlen(pLevelName) > MAX_LEVEL_SIZE)
	{
		ASSERT( FALSE,
			"writeGameFile:Unable to save level name - too long (max20) - %s",
			pLevelName );
		goto error;
	}
	strcpy(psSaveGame->levelName, pLevelName);

	//save out the players power
	for (inc=0; inc < MAX_PLAYERS; inc++)
	{
		psSaveGame->power[inc].currentPower = asPower[inc]->currentPower;
		psSaveGame->power[inc].extractedPower = asPower[inc]->extractedPower;
	}

	//camera position
	disp3d_getView(&(psSaveGame->currentPlayerPos));

	//mission data
//	psSaveGame->missionStartTime =		mission.startTime;
	psSaveGame->missionOffTime =		mission.time;
	psSaveGame->missionETA =			mission.ETA;
    psSaveGame->missionCheatTime =		mission.cheatTime;
	psSaveGame->missionHomeLZ_X =		mission.homeLZ_X;
	psSaveGame->missionHomeLZ_Y =		mission.homeLZ_Y;
	psSaveGame->missionPlayerX =		mission.playerX;
	psSaveGame->missionPlayerY =		mission.playerY;
    psSaveGame->missionScrollMinX =     (UWORD)mission.scrollMinX;
    psSaveGame->missionScrollMinY =     (UWORD)mission.scrollMinY;
    psSaveGame->missionScrollMaxX =     (UWORD)mission.scrollMaxX;
    psSaveGame->missionScrollMaxY =     (UWORD)mission.scrollMaxY;

	psSaveGame->offWorldKeepLists = offWorldKeepLists;
	psSaveGame->RubbleTile	= getRubbleTileNum();
	psSaveGame->WaterTile	= getWaterTileNum();
	psSaveGame->fogColour	= pie_GetFogColour();
	psSaveGame->fogState	= fogStatus;
	if(pie_GetFogEnabled())
	{
		psSaveGame->fogState	= fogStatus | FOG_ENABLED;
	}

	for (player = 0; player < MAX_PLAYERS; player++)
	{
		psSaveGame->iTranspEntryTileX[player]	= mission.iTranspEntryTileX[player];
		psSaveGame->iTranspEntryTileY[player]	= mission.iTranspEntryTileY[player];
		psSaveGame->iTranspExitTileX[player]	= mission.iTranspExitTileX[player];
		psSaveGame->iTranspExitTileY[player]	= mission.iTranspExitTileY[player];
		psSaveGame->aDefaultSensor[player]		= aDefaultSensor[player];
		psSaveGame->aDefaultECM[player]			= aDefaultECM[player];
		psSaveGame->aDefaultRepair[player]		= aDefaultRepair[player];
		for (inc = 0; inc < MAX_RECYCLED_DROIDS; inc++)
		{
			psSaveGame->awDroidExperience[player][inc]	= aDroidExperience[player][inc];
		}
	}

	for (inc = 0; inc < MAX_NOGO_AREAS; inc++)
	{
		psLandingZone = getLandingZone(inc);
		psSaveGame->sLandingZone[inc].x1	= psLandingZone->x1;//incase struct changes
		psSaveGame->sLandingZone[inc].x2	= psLandingZone->x2;
		psSaveGame->sLandingZone[inc].y1	= psLandingZone->y1;
		psSaveGame->sLandingZone[inc].y2	= psLandingZone->y2;
	}

	//version 17
	psSaveGame->objId = objID;

	//version 18
	strcpy(date,__DATE__);
	ASSERT( strlen(date)<MAX_STR_LENGTH,"BuildDate; String error" );
	strcpy(psSaveGame->buildDate,date);
	psSaveGame->oldestVersion = oldestSaveGameVersion;
	psSaveGame->validityKey = validityKey;

	//version 19
	for(i=0; i<MAX_PLAYERS; i++)
	{
		for(j=0; j<MAX_PLAYERS; j++)
		{
			psSaveGame->alliances[i][j] = alliances[i][j];
		}
	}
	for(i=0; i<MAX_PLAYERS; i++)
	{
		psSaveGame->playerColour[i] = getPlayerColour(i);
	}
	psSaveGame->radarZoom = (UBYTE)GetRadarZoom();


	//version 20
	psSaveGame->bDroidsToSafetyFlag = (UBYTE)getDroidsToSafetyFlag();
	for (i = 0; i < MAX_PLAYERS; i++)
	{
		memcpy(&(psSaveGame->asVTOLReturnPos[i]),&asVTOLReturnPos[i],sizeof(POINT));
	}

	//version 22
	for (i = 0; i < MAX_PLAYERS; i++)
	{
		memcpy(&(psSaveGame->asRunData[i]),&asRunData[i],sizeof(RUN_DATA));
	}

	//version 24
	psSaveGame->reinforceTime = missionGetReinforcementTime();
	psSaveGame->bPlayCountDown = (UBYTE)getPlayCountDown();
	psSaveGame->bPlayerHasWon =  (UBYTE)testPlayerHasWon();
	psSaveGame->bPlayerHasLost = (UBYTE)testPlayerHasLost();

    //version 30
    psSaveGame->scrGameLevel = scrGameLevel;
    psSaveGame->bExtraFailFlag = (UBYTE)bExtraFailFlag;
    psSaveGame->bExtraVictoryFlag = (UBYTE)bExtraVictoryFlag;
    psSaveGame->bTrackTransporter = (UBYTE)bTrackTransporter;


	// version 33
	psSaveGame->sGame		= game;
	psSaveGame->savePlayer	= selectedPlayer;
	psSaveGame->multiPlayer = bMultiPlayer;
	psSaveGame->sNetPlay	= NetPlay;
	strcpy(psSaveGame->sPName,getPlayerName(selectedPlayer));
	for(inc=0;inc<MAX_PLAYERS;inc++)
	{
		psSaveGame->sPlayer2dpid[inc]= player2dpid[inc];
	}

	//version 34
	for(inc=0;inc<MAX_PLAYERS;inc++)
	{
		strcpy(psSaveGame->sPlayerName[inc],getPlayerName(inc));
	}

	/* SAVE_GAME is GAME_SAVE_V33 */
	/* GAME_SAVE_V33 includes GAME_SAVE_V31 */
	endian_udword(&psSaveGame->sGame.power);
	endian_uword(&psSaveGame->sGame.bytesPerSec);
	for(i = 0; i < MaxGames; i++) {
		endian_sdword(&psSaveGame->sNetPlay.games[i].desc.dwSize);
		endian_sdword(&psSaveGame->sNetPlay.games[i].desc.dwFlags);
		endian_sdword(&psSaveGame->sNetPlay.games[i].desc.dwMaxPlayers);
		endian_sdword(&psSaveGame->sNetPlay.games[i].desc.dwCurrentPlayers);
		endian_sdword(&psSaveGame->sNetPlay.games[i].desc.dwUser1);
		endian_sdword(&psSaveGame->sNetPlay.games[i].desc.dwUser2);
		endian_sdword(&psSaveGame->sNetPlay.games[i].desc.dwUser3);
		endian_sdword(&psSaveGame->sNetPlay.games[i].desc.dwUser4);
	}
	for(i = 0; i < MaxNumberOfPlayers; i++)
		endian_udword(&psSaveGame->sNetPlay.players[i].dpid);
	endian_udword(&psSaveGame->sNetPlay.playercount);
	endian_udword(&psSaveGame->sNetPlay.dpidPlayer);
	for(i = 0; i < 4; i++)
		endian_udword(&psSaveGame->sNetPlay.cryptKey[i]);
	endian_udword(&psSaveGame->savePlayer);
	for(i = 0; i < MAX_PLAYERS; i++)
		endian_udword(&psSaveGame->sPlayer2dpid[i]);
	/* GAME_SAVE_V31 includes GAME_SAVE_V30 */
	endian_sdword(&psSaveGame->missionCheatTime);
	/* GAME_SAVE_V30 includes GAME_SAVE_V29 */
	endian_sdword(&psSaveGame->scrGameLevel);
	/* GAME_SAVE_V29 includes GAME_SAVE_V27 */
	endian_uword(&psSaveGame->missionScrollMinX);
	endian_uword(&psSaveGame->missionScrollMinY);
	endian_uword(&psSaveGame->missionScrollMaxX);
	endian_uword(&psSaveGame->missionScrollMaxY);
	/* GAME_SAVE_V27 includes GAME_SAVE_V24 */
	for(i = 0; i < MAX_PLAYERS; i++)
		for(j = 0; j < MAX_RECYCLED_DROIDS; j++)
			endian_uword(&psSaveGame->awDroidExperience[i][j]);
	/* GAME_SAVE_V24 includes GAME_SAVE_V22 */
	endian_udword(&psSaveGame->reinforceTime);
	/* GAME_SAVE_V22 includes GAME_SAVE_V20 */
	for(i = 0; i < MAX_PLAYERS; i++) {
		endian_sdword(&psSaveGame->asRunData[i].sPos.x);
		endian_sdword(&psSaveGame->asRunData[i].sPos.y);
	}
	/* GAME_SAVE_V20 includes GAME_SAVE_V19 */
	for(i = 0; i < MAX_PLAYERS; i++) {
		endian_sdword(&psSaveGame->asVTOLReturnPos[i].x);
		endian_sdword(&psSaveGame->asVTOLReturnPos[i].y);
	}
	/* GAME_SAVE_V19 includes GAME_SAVE_V18 */
	/* GAME_SAVE_V18 includes GAME_SAVE_V17 */
	endian_udword(&psSaveGame->oldestVersion);
	endian_udword(&psSaveGame->validityKey);
	/* GAME_SAVE_V17 includes GAME_SAVE_V16 */
	endian_udword(&psSaveGame->objId);
	/* GAME_SAVE_V16 includes GAME_SAVE_V15 */
	/* GAME_SAVE_V15 includes GAME_SAVE_V14 */
	endian_udword(&psSaveGame->RubbleTile);
	endian_udword(&psSaveGame->WaterTile);
	endian_udword(&psSaveGame->fogColour);
	endian_udword(&psSaveGame->fogState);
	/* GAME_SAVE_V14 includes GAME_SAVE_V12 */
	endian_sdword(&psSaveGame->missionOffTime);
	endian_sdword(&psSaveGame->missionETA);
	endian_uword(&psSaveGame->missionHomeLZ_X);
	endian_uword(&psSaveGame->missionHomeLZ_Y);
	endian_sdword(&psSaveGame->missionPlayerX);
	endian_sdword(&psSaveGame->missionPlayerY);
	for(i = 0; i < MAX_PLAYERS; i++) {
		endian_uword(&psSaveGame->iTranspEntryTileX[i]);
		endian_uword(&psSaveGame->iTranspEntryTileY[i]);
		endian_uword(&psSaveGame->iTranspExitTileX[i]);
		endian_uword(&psSaveGame->iTranspExitTileY[i]);
		endian_udword(&psSaveGame->aDefaultSensor[i]);
		endian_udword(&psSaveGame->aDefaultECM[i]);
		endian_udword(&psSaveGame->aDefaultRepair[i]);
	}
	/* GAME_SAVE_V12 includes GAME_SAVE_V11 */
	endian_udword(&psSaveGame->missionTime);
	endian_udword(&psSaveGame->saveKey);
	/* GAME_SAVE_V11 includes GAME_SAVE_V10 */
	endian_sdword(&psSaveGame->currentPlayerPos.p.x);
	endian_sdword(&psSaveGame->currentPlayerPos.p.y);
	endian_sdword(&psSaveGame->currentPlayerPos.p.z);
	endian_sdword(&psSaveGame->currentPlayerPos.r.x);
	endian_sdword(&psSaveGame->currentPlayerPos.r.y);
	endian_sdword(&psSaveGame->currentPlayerPos.r.z);
	/* GAME_SAVE_V10 includes GAME_SAVE_V7 */
	for(i = 0; i < MAX_PLAYERS; i++) {
		endian_udword(&psSaveGame->power[i].currentPower);
		endian_udword(&psSaveGame->power[i].extractedPower);
	}
	/* GAME_SAVE_V7 */
	endian_udword(&psSaveGame->gameTime);
	endian_udword(&psSaveGame->GameType);
	endian_sdword(&psSaveGame->ScrollMinX);
	endian_sdword(&psSaveGame->ScrollMinY);
	endian_udword(&psSaveGame->ScrollMaxX);
	endian_udword(&psSaveGame->ScrollMaxY);

	/* GAME_SAVEHEADER */
	endian_udword(&psHeader->version);

	/* Write the data to the file */
	if (pFileData != NULL) {
		status = saveFile(pFileName, pFileData, fileSize);
		FREE(pFileData);
		return status;
	}

error:
	if (pFileData != NULL)
	{
		FREE(pFileData);
	}
	return FALSE;
}

// -----------------------------------------------------------------------------------------
// Process the droid initialisation file (dinit.bjo). Creates droids for
// the scenario being loaded. This is *NEVER* called for a user save game
//
BOOL loadSaveDroidInit(char *pFileData, UDWORD filesize)
{
	DROIDINIT_SAVEHEADER		*psHeader;

	/* Check the file type */
	psHeader = (DROIDINIT_SAVEHEADER *)pFileData;
	if (psHeader->aFileType[0] != 'd' || psHeader->aFileType[1] != 'i' ||
		psHeader->aFileType[2] != 'n' || psHeader->aFileType[3] != 't')	{
		debug( LOG_ERROR, "loadSaveUnitInit: Incorrect file type" );
		abort();
		return FALSE;
	}

	/* DROIDINIT_SAVEHEADER */
	endian_udword(&psHeader->version);
	endian_udword(&psHeader->quantity);

	//increment to the start of the data
	pFileData += DROIDINIT_HEADER_SIZE;

	/* Check the file version */
	if (psHeader->version < VERSION_7)
	{
		debug( LOG_ERROR, "UnitInit; unsupported save format version %d", psHeader->version );
		abort();
		return FALSE;
	}
	else if (psHeader->version <= CURRENT_VERSION_NUM)
	{
		if (!loadSaveDroidInitV2(pFileData, filesize, psHeader->quantity))
		{
			return FALSE;
		}
	}
	else
	{
		debug( LOG_ERROR, "UnitInit: undefined save format version %d", psHeader->version );
		abort();
		return FALSE;
	}

	return TRUE;
}



// -----------------------------------------------------------------------------------------
// Used for all droids
BOOL loadSaveDroidInitV2(char *pFileData, UDWORD filesize,UDWORD quantity)
{
	SAVE_DROIDINIT *pDroidInit;
	DROID_TEMPLATE *psTemplate;
	DROID *psDroid;
	UDWORD i;
	UDWORD NumberOfSkippedDroids = 0;

	pDroidInit = (SAVE_DROIDINIT*)pFileData;

	for(i=0; i<quantity; i++)
	{
		/* SAVE_DROIDINIT is OBJECT_SAVE_V19 */
		/* OBJECT_SAVE_V19 */
		endian_udword(&pDroidInit->id);
		endian_udword(&pDroidInit->x);
		endian_udword(&pDroidInit->y);
		endian_udword(&pDroidInit->z);
		endian_udword(&pDroidInit->direction);
		endian_udword(&pDroidInit->player);
		endian_udword(&pDroidInit->burnStart);
		endian_udword(&pDroidInit->burnDamage);

		pDroidInit->player=RemapPlayerNumber(pDroidInit->player);

		if (pDroidInit->player >= MAX_PLAYERS) {
			pDroidInit->player = MAX_PLAYERS-1;	// now don't lose any droids ... force them to be the last player
			NumberOfSkippedDroids++;
		}


		psTemplate = (DROID_TEMPLATE *)FindDroidTemplate(pDroidInit->name,pDroidInit->player);

		if(psTemplate==NULL)
		{

			debug( LOG_NEVER, "loadSaveUnitInitV2:\nUnable to find template for %s player %d", pDroidInit->name,pDroidInit->player );

#ifdef DEBUG
//			DumpDroidTemplates();
#endif

		}
		else
		{
			ASSERT( psTemplate != NULL,
				"loadSaveUnitInitV2: Invalid template pointer" );

// Need to set apCompList[pDroidInit->player][componenttype][compid] = AVAILABLE for each droid.

			{

				psDroid = buildDroid(psTemplate, (pDroidInit->x & (~TILE_MASK)) + TILE_UNITS/2, (pDroidInit->y  & (~TILE_MASK)) + TILE_UNITS/2,
					pDroidInit->player, FALSE);

				if (psDroid) {
					psDroid->id = pDroidInit->id;
					psDroid->direction = (UWORD)pDroidInit->direction;
					addDroid(psDroid, apsDroidLists);
				}
				else
				{

					debug( LOG_ERROR, "This droid cannot be built - %s", pDroidInit->name );
					abort();
				}
			}
		}
		pDroidInit++;
	}

//	powerCalculated = TRUE;



	if(NumberOfSkippedDroids) {
		debug( LOG_ERROR, "unitLoad: Bad Player number in %d unit(s)... assigned to the last player!\n", NumberOfSkippedDroids );
		abort();
	}
	return TRUE;
}


// -----------------------------------------------------------------------------------------
DROID_TEMPLATE *FindDroidTemplate(char *name,UDWORD player)
{
	UDWORD			TempPlayer;
	DROID_TEMPLATE *Template;
	UDWORD			id;

/*#ifdef RESOURCE_NAMES

	//get the name from the resource associated with it
	if (!strresGetIDNum(psStringRes, name, &id))
	{
		DBERROR(("Cannot find resource for template - %s", name));
		return NULL;
	}
	//get the string from the id
	name = strresGetString(psStringRes, id);

#endif*/

	//get the name from the resource associated with it
	if (!strresGetIDNum(psStringRes, name, &id))
	{
		debug( LOG_ERROR, "Cannot find resource for template - %s", name );
		abort();
		return NULL;
	}
	//get the string from the id
	name = strresGetString(psStringRes, id);

	for(TempPlayer=0; TempPlayer<MAX_PLAYERS; TempPlayer++) {
		Template = apsDroidTemplates[TempPlayer];

		while(Template) {

			//if(strcmp(name,Template->pName)==0) {
			if(strcmp(name,Template->aName)==0) {
//				DBPRINTF(("%s %d , %d\n",name,player,TempPlayer));
				return Template;
			}
			Template = Template->psNext;
		}
	}

	return NULL;
}



// -----------------------------------------------------------------------------------------
UDWORD RemapPlayerNumber(UDWORD OldNumber)
{
	return(OldNumber);
}

// -----------------------------------------------------------------------------------------
/*This is *ALWAYS* called by a User Save Game */
BOOL loadSaveDroid(char *pFileData, UDWORD filesize, DROID **ppsCurrentDroidLists)
{
	DROID_SAVEHEADER		*psHeader;

	/* Check the file type */
	psHeader = (DROID_SAVEHEADER *)pFileData;
	if (psHeader->aFileType[0] != 'd' || psHeader->aFileType[1] != 'r' ||
		psHeader->aFileType[2] != 'o' || psHeader->aFileType[3] != 'd')
	{
		debug( LOG_ERROR, "loadSaveUnit: Incorrect file type" );
		abort();
		return FALSE;
	}

	/* DROID_SAVEHEADER */
	endian_udword(&psHeader->version);
	endian_udword(&psHeader->quantity);

	//increment to the start of the data
	pFileData += DROID_HEADER_SIZE;

	/* Check the file version */
	if (psHeader->version < VERSION_9)
	{
		debug( LOG_ERROR, "UnitLoad; unsupported save format version %d", psHeader->version );
		abort();
		return FALSE;
	}
	else if (psHeader->version == VERSION_11)
	{
		if (!loadSaveDroidV11(pFileData, filesize, psHeader->quantity, psHeader->version, ppsCurrentDroidLists))
		{
			return FALSE;
		}
	}
	else if (psHeader->version <= VERSION_19)//old save name size
	{
		if (!loadSaveDroidV19(pFileData, filesize, psHeader->quantity, psHeader->version, ppsCurrentDroidLists))
		{
			return FALSE;
		}
	}
	else if (psHeader->version <= CURRENT_VERSION_NUM)
	{
		if (!loadSaveDroidV(pFileData, filesize, psHeader->quantity, psHeader->version, ppsCurrentDroidLists))
		{
			return FALSE;
		}
	}
	else
	{
		debug( LOG_ERROR, "UnitLoad: undefined save format version %d", psHeader->version );
		abort();
		return FALSE;
	}

	return TRUE;
}

// -----------------------------------------------------------------------------------------
static DROID* buildDroidFromSaveDroidV11(SAVE_DROID_V11* psSaveDroid)
{
	DROID_TEMPLATE			*psTemplate, sTemplate;
	DROID					*psDroid;
	BOOL					found;
	UDWORD					i;
	SDWORD					compInc;
	UDWORD					burnTime;

	psTemplate = &sTemplate;


	//set up the template
	//copy the values across

	strncpy(psTemplate->aName, psSaveDroid->name, DROID_MAXNAME);
	psTemplate->aName[DROID_MAXNAME-1]=0;
	//ignore the first comp - COMP_UNKNOWN
	found = TRUE;
	for (i=1; i < DROID_MAXCOMP; i++)
	{

		compInc = getCompFromName(i, psSaveDroid->asBits[i].name);
		if (compInc < 0)
		{

			debug( LOG_ERROR, "This component no longer exists - %s, the droid will be deleted", psSaveDroid->asBits[i].name );
			abort();
			found = FALSE;
			break;//continue;
		}
		psTemplate->asParts[i] = (UDWORD)compInc;
	}
	if (!found)
	{
		//ignore this record
		return NULL;
	}
	psTemplate->numWeaps = psSaveDroid->numWeaps;
	for (i=0; i < psSaveDroid->numWeaps; i++)
	{
		int weapon = getCompFromName(COMP_WEAPON, psSaveDroid->asWeaps[i].name);
		if( weapon < 0)
		{
			ASSERT(FALSE, "This component does not exist : %s", psSaveDroid->asWeaps[i].name );
			return NULL;
		}			
		psTemplate->asWeaps[i] = weapon;
	}

	psTemplate->buildPoints = calcTemplateBuild(psTemplate);
	psTemplate->powerPoints = calcTemplatePower(psTemplate);
	psTemplate->droidType = psSaveDroid->droidType;

	/*create the Droid */

	// ignore brains for now
	psTemplate->asParts[COMP_BRAIN] = 0;

	psDroid = buildDroid(psTemplate, psSaveDroid->x, psSaveDroid->y,
		psSaveDroid->player, FALSE);

	//copy the droid's weapon stats
	for (i=0; i < psDroid->numWeaps; i++)
	{
		if (psDroid->asWeaps[i].nStat > 0)
		{
			//only one weapon now
			psDroid->asWeaps[i].hitPoints = psSaveDroid->asWeaps[i].hitPoints;
			psDroid->asWeaps[i].ammo = psSaveDroid->asWeaps[i].ammo;
			psDroid->asWeaps[i].lastFired = psSaveDroid->asWeaps[i].lastFired;
		}
	}
	//copy the values across
	psDroid->id = psSaveDroid->id;
	//are these going to ever change from the values set up with?
//			psDroid->z = psSaveDroid->z;		// use the correct map height value

	psDroid->direction = (UWORD)psSaveDroid->direction;
	psDroid->body = psSaveDroid->body;
	if (psDroid->body > psDroid->originalBody)
	{
		psDroid->body = psDroid->originalBody;
	}

	psDroid->inFire = psSaveDroid->inFire;
	psDroid->burnDamage = psSaveDroid->burnDamage;
	burnTime = psSaveDroid->burnStart;
	psDroid->burnStart = burnTime;

	psDroid->numKills = (UWORD)psSaveDroid->numKills;
	//version 11
	for (i=0; i < psDroid->numWeaps; i++)
	{
		psDroid->turretRotation[i] = psSaveDroid->turretRotation;
		psDroid->turretPitch[i] = psSaveDroid->turretPitch;
	}


	psDroid->psGroup = NULL;
	psDroid->psGrpNext = NULL;

	return psDroid;
}

// -----------------------------------------------------------------------------------------
static DROID* buildDroidFromSaveDroidV19(SAVE_DROID_V18* psSaveDroid, UDWORD version)
{
	DROID_TEMPLATE			*psTemplate, sTemplate;
	DROID					*psDroid;
	SAVE_DROID_V14			*psSaveDroidV14;
	BOOL					found;
	UDWORD					i, id;
	SDWORD					compInc;
	UDWORD					burnTime;

	psTemplate = &sTemplate;

	psTemplate->pName = NULL;

	//set up the template
	//copy the values across

	strncpy(psTemplate->aName, psSaveDroid->name, DROID_MAXNAME);
	psTemplate->aName[DROID_MAXNAME-1]=0;

	//ignore the first comp - COMP_UNKNOWN
	found = TRUE;
	for (i=1; i < DROID_MAXCOMP; i++)
	{

		compInc = getCompFromName(i, psSaveDroid->asBits[i].name);

		if (compInc < 0)
		{

			debug( LOG_ERROR, "This component no longer exists - %s, the droid will be deleted", psSaveDroid->asBits[i].name );
			abort();

			found = FALSE;
			break;//continue;
		}
		psTemplate->asParts[i] = (UDWORD)compInc;
	}
	if (!found)
	{
		//ignore this record
		ASSERT( found,"buildUnitFromSavedUnit; failed to find weapon" );
		return NULL;
	}
	psTemplate->numWeaps = psSaveDroid->numWeaps;

	if (psSaveDroid->numWeaps > 0)
	{
		for(i = 0;i < psTemplate->numWeaps;i++)
		{
			int weapon = getCompFromName(COMP_WEAPON, psSaveDroid->asWeaps[i].name);
			if( weapon < 0)
			{
				ASSERT(FALSE, "This component does not exist : %s", psSaveDroid->asWeaps[i].name );
				return NULL;
			}			
			psTemplate->asWeaps[i] = weapon;
		}
	}

	psTemplate->buildPoints = calcTemplateBuild(psTemplate);
	psTemplate->powerPoints = calcTemplatePower(psTemplate);
	psTemplate->droidType = psSaveDroid->droidType;

	/*create the Droid */


	// ignore brains for now
	// not any *$&!!! more - JOHN
//	psTemplate->asParts[COMP_BRAIN] = 0;

	if(psSaveDroid->x == INVALID_XY)
	{
		psDroid = buildDroid(psTemplate, psSaveDroid->x, psSaveDroid->y,
			psSaveDroid->player, TRUE);
	}
	else if(psSaveDroid->saveType == DROID_ON_TRANSPORT)
	{
		psDroid = buildDroid(psTemplate, 0, 0,
			psSaveDroid->player, TRUE);
	}
	else
	{
		psDroid = buildDroid(psTemplate, psSaveDroid->x, psSaveDroid->y,
			psSaveDroid->player, FALSE);
	}

	if(psDroid == NULL)
	{
		ASSERT( FALSE,"buildUnitFromSavedUnit; failed to build unit" );
		return NULL;
	}


	//copy the droid's weapon stats
	for (i=0; i < psDroid->numWeaps; i++)
	{
		if (psDroid->asWeaps[i].nStat > 0)
		{
			psDroid->asWeaps[i].hitPoints = psSaveDroid->asWeaps[i].hitPoints;
			psDroid->asWeaps[i].ammo = psSaveDroid->asWeaps[i].ammo;
			psDroid->asWeaps[i].lastFired = psSaveDroid->asWeaps[i].lastFired;
		}
	}
	//copy the values across
	psDroid->id = psSaveDroid->id;
	//are these going to ever change from the values set up with?
//			psDroid->z = psSaveDroid->z;		// use the correct map height value

	psDroid->direction = (UWORD)psSaveDroid->direction;
    psDroid->body = psSaveDroid->body;
	if (psDroid->body > psDroid->originalBody)
	{
		psDroid->body = psDroid->originalBody;
	}

	psDroid->inFire = psSaveDroid->inFire;
	psDroid->burnDamage = psSaveDroid->burnDamage;
	burnTime = psSaveDroid->burnStart;
	psDroid->burnStart = burnTime;

	psDroid->numKills = (UWORD)psSaveDroid->numKills;
	//version 14
	psDroid->resistance = droidResistance(psDroid);

	if (version >= VERSION_11)//version 11
	{
		//Watermelon:make it back-compatible with older versions of save
		for (i=0; i < psDroid->numWeaps; i++)
		{
			psDroid->turretRotation[i] = psSaveDroid->turretRotation;
			psDroid->turretPitch[i] = psSaveDroid->turretPitch;
		}
	}
	if (version >= VERSION_12)//version 12
	{
		psDroid->order				= psSaveDroid->order;
		psDroid->orderX				= psSaveDroid->orderX;
		psDroid->orderY				= psSaveDroid->orderY;
		psDroid->orderX2				= psSaveDroid->orderX2;
		psDroid->orderY2				= psSaveDroid->orderY2;
		psDroid->timeLastHit			= psSaveDroid->timeLastHit;
		//rebuild the object pointer from the ID
		FIXME_CAST_ASSIGN(UDWORD, psDroid->psTarget, psSaveDroid->targetID);
		psDroid->secondaryOrder		= psSaveDroid->secondaryOrder;
		psDroid->action				= psSaveDroid->action;
		psDroid->actionX				= psSaveDroid->actionX;
		psDroid->actionY				= psSaveDroid->actionY;
		//rebuild the object pointer from the ID
		FIXME_CAST_ASSIGN(UDWORD, psDroid->psActionTarget[0], psSaveDroid->actionTargetID);
		psDroid->actionStarted		= psSaveDroid->actionStarted;
		psDroid->actionPoints		= psSaveDroid->actionPoints;
        //actionHeight has been renamed to powerAccrued - AB 7/1/99
        //psDroid->actionHeight		= psSaveDroid->actionHeight;
		psDroid->powerAccrued		= psSaveDroid->actionHeight;
		//added for V14

		psDroid->psGroup = NULL;
		psDroid->psGrpNext = NULL;
	}
	if ((version >= VERSION_14) && (version < VERSION_18))//version 14
	{

//warning V14 - v17 only
		//current Save Droid V18+ uses larger tarStatName
		//subsequent structure elements are not aligned between the two
		psSaveDroidV14 = (SAVE_DROID_V14*)psSaveDroid;
		if (psSaveDroidV14->tarStatName[0] == 0)
		{
			psDroid->psTarStats[0] = NULL;
		}
		else
	 	{
			id = getStructStatFromName(psSaveDroidV14->tarStatName);
			if (id != (UDWORD)-1)
			{
				psDroid->psTarStats[0] = (BASE_STATS*)&asStructureStats[id];
			}
			else
			{
				ASSERT( FALSE,"loadUnit TargetStat not found" );
				psDroid->psTarStats[0] = NULL;
                orderDroid(psDroid, DORDER_STOP);
			}
		}
//warning V14 - v17 only
		//rebuild the object pointer from the ID
		FIXME_CAST_ASSIGN(UDWORD, psDroid->psBaseStruct, psSaveDroidV14->baseStructID);
		psDroid->group = psSaveDroidV14->group;
		psDroid->selected = psSaveDroidV14->selected;
//20feb		psDroid->cluster = psSaveDroidV14->cluster;
		psDroid->died = psSaveDroidV14->died;
		psDroid->lastEmission =	psSaveDroidV14->lastEmission;
//warning V14 - v17 only
		for (i=0; i < MAX_PLAYERS; i++)
		{
			psDroid->visible[i]	= psSaveDroidV14->visible[i];
		}
//end warning V14 - v17 only
	}
	else if (version >= VERSION_18)//version 18
	{

		if (psSaveDroid->tarStatName[0] == 0)
		{
			psDroid->psTarStats[0] = NULL;
		}
		else
		{
			id = getStructStatFromName(psSaveDroid->tarStatName);
			if (id != (UDWORD)-1)
			{
				psDroid->psTarStats[0] = (BASE_STATS*)&asStructureStats[id];
			}
			else
			{
				ASSERT( FALSE,"loadUnit TargetStat not found" );
				psDroid->psTarStats[0] = NULL;
			}
		}
		//rebuild the object pointer from the ID
		FIXME_CAST_ASSIGN(UDWORD, psDroid->psBaseStruct, psSaveDroid->baseStructID);
		psDroid->group = psSaveDroid->group;
		psDroid->selected = psSaveDroid->selected;
//20feb		psDroid->cluster = psSaveDroid->cluster;
		psDroid->died = psSaveDroid->died;
		psDroid->lastEmission =	psSaveDroid->lastEmission;
		for (i=0; i < MAX_PLAYERS; i++)
		{
			psDroid->visible[i]	= psSaveDroid->visible[i];
		}
	}

	return psDroid;
}

// -----------------------------------------------------------------------------------------
//version 20 + after names change
static DROID* buildDroidFromSaveDroid(SAVE_DROID* psSaveDroid, UDWORD version)
{
	DROID_TEMPLATE			*psTemplate, sTemplate;
	DROID					*psDroid;
	BOOL					found;
	UDWORD					i, id;
	SDWORD					compInc;
	UDWORD					burnTime;

//	version;

	psTemplate = &sTemplate;

	psTemplate->pName = NULL;

	//set up the template
	//copy the values across

	strncpy(psTemplate->aName, psSaveDroid->name, DROID_MAXNAME);
	psTemplate->aName[DROID_MAXNAME-1]=0;
	//ignore the first comp - COMP_UNKNOWN
	found = TRUE;
	for (i=1; i < DROID_MAXCOMP; i++)
	{

		compInc = getCompFromName(i, psSaveDroid->asBits[i].name);

        //HACK to get the game to load when ECMs, Sensors or RepairUnits have been deleted
        if (compInc < 0 && (i == COMP_ECM || i == COMP_SENSOR || i == COMP_REPAIRUNIT))
        {
            //set the ECM to be the defaultECM ...
            if (i == COMP_ECM)
            {
                compInc = aDefaultECM[psSaveDroid->player];
            }
            else if (i == COMP_SENSOR)
            {
                compInc = aDefaultSensor[psSaveDroid->player];
            }
            else if (i == COMP_REPAIRUNIT)
            {
                compInc = aDefaultRepair[psSaveDroid->player];
            }
        }
		else if (compInc < 0)
		{

			debug( LOG_ERROR, "This component no longer exists - %s, the droid will be deleted", psSaveDroid->asBits[i].name );
			abort();

			found = FALSE;
			break;//continue;
		}
		psTemplate->asParts[i] = (UDWORD)compInc;
	}
	if (!found)
	{
		//ignore this record
		ASSERT( found,"buildUnitFromSavedUnit; failed to find weapon" );
		return NULL;
	}
	psTemplate->numWeaps = psSaveDroid->numWeaps;
	if (psSaveDroid->numWeaps > 0)
	{
		for(i = 0;i < psTemplate->numWeaps;i++)
		{
			int weapon = getCompFromName(COMP_WEAPON, psSaveDroid->asWeaps[i].name);
			if( weapon < 0)
			{
				ASSERT(FALSE, "This component does not exist : %s", psSaveDroid->asWeaps[i].name );
				return NULL;
			}			
			psTemplate->asWeaps[i] = weapon;
		}
	}

	psTemplate->buildPoints = calcTemplateBuild(psTemplate);
	psTemplate->powerPoints = calcTemplatePower(psTemplate);
	psTemplate->droidType = psSaveDroid->droidType;

	/*create the Droid */

	// ignore brains for now
	// not any *$&!!! more - JOHN
//	psTemplate->asParts[COMP_BRAIN] = 0;


	turnOffMultiMsg(TRUE);

	if(psSaveDroid->x == INVALID_XY)
	{
		psDroid = buildDroid(psTemplate, psSaveDroid->x, psSaveDroid->y,
			psSaveDroid->player, TRUE);
	}
	else if(psSaveDroid->saveType == DROID_ON_TRANSPORT)
	{
		psDroid = buildDroid(psTemplate, 0, 0,
			psSaveDroid->player, TRUE);
	}
	else
	{
		psDroid = buildDroid(psTemplate, psSaveDroid->x, psSaveDroid->y,
			psSaveDroid->player, FALSE);
	}

	if(psDroid == NULL)
	{
		ASSERT( FALSE,"buildUnitFromSavedUnit; failed to build unit" );
		return NULL;
	}

	turnOffMultiMsg(FALSE);


	//copy the droid's weapon stats
	for (i=0; i < psDroid->numWeaps; i++)
	{
		if (psDroid->asWeaps[i].nStat > 0)
		{
			psDroid->asWeaps[i].hitPoints = psSaveDroid->asWeaps[i].hitPoints;
			psDroid->asWeaps[i].ammo = psSaveDroid->asWeaps[i].ammo;
			psDroid->asWeaps[i].lastFired = psSaveDroid->asWeaps[i].lastFired;
		}
	}
	//copy the values across
	psDroid->id = psSaveDroid->id;
	//are these going to ever change from the values set up with?
//			psDroid->z = psSaveDroid->z;		// use the correct map height value

	psDroid->direction = (UWORD)psSaveDroid->direction;
	psDroid->body = psSaveDroid->body;
	if (psDroid->body > psDroid->originalBody)
	{
		psDroid->body = psDroid->originalBody;
	}

	psDroid->inFire = psSaveDroid->inFire;
	psDroid->burnDamage = psSaveDroid->burnDamage;
	burnTime = psSaveDroid->burnStart;
	psDroid->burnStart = burnTime;

	psDroid->numKills = (UWORD)psSaveDroid->numKills;
	//version 14
	psDroid->resistance = droidResistance(psDroid);

	//version 11
	//Watermelon:make it back-compatible with older versions of save
	for (i=0; i < psDroid->numWeaps; i++)
	{
		if (version >= VERSION_24)
		{
			psDroid->turretRotation[i] = psSaveDroid->turretRotation[i];
			psDroid->turretPitch[i] = psSaveDroid->turretPitch[i];
		}
		else
		{
			psDroid->turretRotation[i] = psSaveDroid->turretRotation[0];
			psDroid->turretPitch[i] = psSaveDroid->turretPitch[0];
		}
	}
	//version 12
	psDroid->order				= psSaveDroid->order;
	psDroid->orderX				= psSaveDroid->orderX;
	psDroid->orderY				= psSaveDroid->orderY;
	psDroid->orderX2				= psSaveDroid->orderX2;
	psDroid->orderY2				= psSaveDroid->orderY2;
	psDroid->timeLastHit			= psSaveDroid->timeLastHit;
	//rebuild the object pointer from the ID
	FIXME_CAST_ASSIGN(UDWORD, psDroid->psTarget[0], psSaveDroid->targetID);
	psDroid->secondaryOrder		= psSaveDroid->secondaryOrder;
	psDroid->action				= psSaveDroid->action;
	psDroid->actionX				= psSaveDroid->actionX;
	psDroid->actionY				= psSaveDroid->actionY;
	//rebuild the object pointer from the ID
	FIXME_CAST_ASSIGN(UDWORD, psDroid->psActionTarget[0], psSaveDroid->actionTargetID);
	psDroid->actionStarted		= psSaveDroid->actionStarted;
	psDroid->actionPoints		= psSaveDroid->actionPoints;
    //actionHeight has been renamed to powerAccrued - AB 7/1/99
    //psDroid->actionHeight		= psSaveDroid->actionHeight;
	psDroid->powerAccrued		= psSaveDroid->actionHeight;
	//added for V14


	//version 18
	if (psSaveDroid->tarStatName[0] == 0)
	{
		psDroid->psTarStats[0] = NULL;
	}
	else
	{
		id = getStructStatFromName(psSaveDroid->tarStatName);
		if (id != (UDWORD)-1)
		{
			psDroid->psTarStats[0] = (BASE_STATS*)&asStructureStats[id];
		}
		else
		{
			ASSERT( FALSE,"loadUnit TargetStat not found" );
			psDroid->psTarStats[0] = NULL;
		}
	}
	//rebuild the object pointer from the ID
	FIXME_CAST_ASSIGN(UDWORD, psDroid->psBaseStruct, psSaveDroid->baseStructID);
	psDroid->group = psSaveDroid->group;
	psDroid->selected = psSaveDroid->selected;
//20feb	psDroid->cluster = psSaveDroid->cluster;
	psDroid->died = psSaveDroid->died;
	psDroid->lastEmission =	psSaveDroid->lastEmission;
	for (i=0; i < MAX_PLAYERS; i++)
	{
		psDroid->visible[i]	= psSaveDroid->visible[i];
	}

	if (version >= VERSION_21)//version 21
	{
		if ( (psDroid->droidType != DROID_TRANSPORTER) &&
						 (psDroid->droidType != DROID_COMMAND) )
		{
			//rebuild group from command id in loadDroidSetPointers
			FIXME_CAST_ASSIGN(UDWORD, psDroid->psGroup, psSaveDroid->commandId);
			FIXME_CAST_ASSIGN(UDWORD, psDroid->psGrpNext, UDWORD_MAX);
		}
	}
	else
	{
		if ( (psDroid->droidType != DROID_TRANSPORTER) &&
						 (psDroid->droidType != DROID_COMMAND) )
		{
			//dont rebuild group from command id in loadDroidSetPointers
			psDroid->psGroup = NULL;
			psDroid->psGrpNext = NULL;
		}
	}

	if (version >= VERSION_24)//version 24
	{
		psDroid->resistance = (SWORD)psSaveDroid->resistance;
		memcpy(&psDroid->sMove, &psSaveDroid->sMove, sizeof(SAVE_MOVE_CONTROL));
		psDroid->sMove.fz= (FRACT)psDroid->z;
		if (psDroid->sMove.psFormation != NULL)
		{
			psDroid->sMove.psFormation = NULL;
//			psSaveDroid->formationDir;
//			psSaveDroid->formationX;
//			psSaveDroid->formationY;
			// join a formation if it exists at the destination
			if (formationFind(&psDroid->sMove.psFormation, psSaveDroid->formationX, psSaveDroid->formationY))
			{
				formationJoin(psDroid->sMove.psFormation, (BASE_OBJECT *)psDroid);
			}
			else
			{
				// no formation so create a new one
				if (formationNew(&psDroid->sMove.psFormation, FT_LINE, psSaveDroid->formationX, psSaveDroid->formationY,
						(SDWORD)psSaveDroid->formationDir))
				{
					formationJoin(psDroid->sMove.psFormation, (BASE_OBJECT *)psDroid);
				}
			}
		}
	}
	return psDroid;
}


// -----------------------------------------------------------------------------------------
BOOL loadDroidSetPointers(void)
{
	UDWORD		player,list;
	UDWORD		id;
	DROID		*psDroid, *psCommander;
	DROID		**ppsDroidLists[3];

	ppsDroidLists[0] = apsDroidLists;
	ppsDroidLists[1] = mission.apsDroidLists;
	ppsDroidLists[2] = apsLimboDroids;

	for(list = 0; list<3; list++)
	{
		debug( LOG_NEVER, "List %d\n", list );
		for(player = 0; player<MAX_PLAYERS; player++)
		{
			psDroid=(DROID *)ppsDroidLists[list][player];
			while (psDroid)
			{
				//Target rebuild the object pointer from the ID
				id = (UDWORD)(psDroid->psTarget[0]);
				ASSERT( id != 0xdddddddd,"LoadUnit found freed target" );
				if (id != UDWORD_MAX)
				{
					psDroid->psTarget[0]			= getBaseObjFromId(id);
					ASSERT( psDroid->psTarget[0] != NULL,"Saved Droid psTarget getBaseObjFromId() failed" );
					if (psDroid->psTarget[0] == NULL)
					{
						psDroid->order = DORDER_NONE;
					}
				}
				else
				{
					psDroid->psTarget[0] = NULL;//psSaveDroid->targetID
				}
				//ActionTarget rebuild the object pointer from the ID
				id = (UDWORD)(psDroid->psActionTarget[0]);
				ASSERT( id != 0xdddddddd,"LoadUnit found freed action target" );
				if (id != UDWORD_MAX)
				{
					psDroid->psActionTarget[0]			= getBaseObjFromId(id);
					ASSERT( psDroid->psActionTarget[0] != NULL,"Saved Droid psActionTarget getBaseObjFromId() failed" );
					if (psDroid->psActionTarget[0] == NULL)
					{
						psDroid->action = DACTION_NONE;
					}
				}
				else
				{
					psDroid->psActionTarget[0]	= NULL;//psSaveDroid->targetID
				}
				//BaseStruct rebuild the object pointer from the ID
				id = (UDWORD)(psDroid->psBaseStruct);
				ASSERT( id != 0xdddddddd,"LoadUnit found freed baseStruct" );
				if (id != UDWORD_MAX)
				{
					psDroid->psBaseStruct			= (STRUCTURE*)getBaseObjFromId(id);
					ASSERT( psDroid->psBaseStruct != NULL,"Saved Droid psBaseStruct getBaseObjFromId() failed" );
					if (psDroid->psBaseStruct == NULL)
					{
						psDroid->action = DACTION_NONE;
					}
				}
				else
				{
					psDroid->psBaseStruct = NULL;//psSaveDroid->targetID
				}
				if (saveGameVersion > VERSION_20)
				{
					//rebuild group for droids in command group from the commander ID
					if (((UDWORD)psDroid->psGrpNext) == UDWORD_MAX)
					{
						id = (UDWORD)(psDroid->psGroup);
						psDroid->psGroup = NULL;
						psDroid->psGrpNext = NULL;
						ASSERT( id != 0xdddddddd,"LoadUnit found freed commander" );
						if (id != UDWORD_MAX)
						{
							psCommander	= (DROID*)getBaseObjFromId(id);
							ASSERT( psCommander != NULL,"Saved Droid psCommander getBaseObjFromId() failed" );
							if (psCommander != NULL)
							{
								cmdDroidAddDroid(psCommander,psDroid);
							}
						}
					}
				}
				psDroid = psDroid->psNext;
			}
			/*
			for(psDroid = ppsDroidLists[list][i]; psDroid; psDroid = psDroid->psNext)
			{
				id = (UDWORD)(psDroid->psTarget);
				psDroid->psTarget = (id==UDWORD_MAX ? NULL : getBaseObjFromId(id));
				id = (UDWORD)(psDroid->psActionTarget);
				psDroid->psActionTarget = ( id==UDWORD_MAX ? NULL : getBaseObjFromId(id));
				ASSERT( ((UDWORD)psDroid->psTarget)!=UDWORD_MAX,"Found invalid target" );
				ASSERT( ((UDWORD)psDroid->psActionTarget)!=UDWORD_MAX,"Found invalid action target" );
				DBPRINTF(("psDroid->psTarget = %d\n",psDroid->psTarget));
			}
			*/
		}
	}

 /*
	for(player=0; player<MAX_PLAYERS; player++)
	{
		psDroid=(DROID *)apsDroidLists[player];
		while (psDroid)
		{
			//rebuild the object pointer from the ID
			id = (UDWORD)(psDroid->psTarget);
			if (id != UDWORD_MAX)
			{
				psDroid->psTarget			= getBaseObjFromId(id);
			}
			else
			{
				psDroid->psTarget			= NULL;//psSaveDroid->targetID
			}
			//rebuild the object pointer from the ID
			id = (UDWORD)(psDroid->psActionTarget);
			if (id != UDWORD_MAX)
			{
				psDroid->psActionTarget			= getBaseObjFromId(id);
			}
			else
			{
				psDroid->psActionTarget			= NULL;//psSaveDroid->targetID
			}
			psDroid = psDroid->psNext;
		}
	}
*/
	return TRUE;

}

// -----------------------------------------------------------------------------------------
/* code specific to version 11 of a save droid */
BOOL loadSaveDroidV11(char *pFileData, UDWORD filesize, UDWORD numDroids, UDWORD version, DROID **ppsCurrentDroidLists)
{
	SAVE_DROID_V11				*psSaveDroid, sSaveDroid;
//	DROID_TEMPLATE			*psTemplate, sTemplate;
	DROID					*psDroid;
	DROID_GROUP				*psCurrentTransGroup;
	UDWORD					count;
	UDWORD					NumberOfSkippedDroids=0;
	UDWORD					sizeOfSaveDroid = 0;
	DROID_GROUP				*psGrp;
	UBYTE	i;

	psCurrentTransGroup = NULL;

	psSaveDroid = &sSaveDroid;
	if (version <= VERSION_10)
	{
		sizeOfSaveDroid = sizeof(SAVE_DROID_V9);
	}
	else if (version == VERSION_11)
	{
		sizeOfSaveDroid = sizeof(SAVE_DROID_V11);
	}
	else if (version <= CURRENT_VERSION_NUM)
	{
		sizeOfSaveDroid = sizeof(SAVE_DROID_V12);
	}

	if ((sizeOfSaveDroid * numDroids + DROID_HEADER_SIZE) >
		filesize)
	{
		debug( LOG_ERROR, "unitLoad: unexpected end of file" );
		abort();
		return FALSE;
	}

	/* Load in the droid data */
	for (count = 0; count < numDroids; count ++, pFileData += sizeOfSaveDroid)
	{
		memcpy(psSaveDroid, pFileData, sizeOfSaveDroid);

		/* DROID_SAVE_V11 includes OBJECT_SAVE_V19, SAVE_WEAPON_V19 */
		endian_udword(&psSaveDroid->body);
		endian_udword(&psSaveDroid->numWeaps);
		endian_udword(&psSaveDroid->numKills);
		endian_uword(&psSaveDroid->turretRotation);
		endian_uword(&psSaveDroid->turretPitch);
		/* OBJECT_SAVE_V19 */
		endian_udword(&psSaveDroid->id);
		endian_udword(&psSaveDroid->x);
		endian_udword(&psSaveDroid->y);
		endian_udword(&psSaveDroid->z);
		endian_udword(&psSaveDroid->direction);
		endian_udword(&psSaveDroid->player);
		endian_udword(&psSaveDroid->burnStart);
		endian_udword(&psSaveDroid->burnDamage);
		for(i = 0; i < TEMP_DROID_MAXPROGS; i++) {
			/* SAVE_WEAPON_V19 */
			endian_udword(&psSaveDroid->asWeaps[i].hitPoints);
			endian_udword(&psSaveDroid->asWeaps[i].ammo);
			endian_udword(&psSaveDroid->asWeaps[i].lastFired);
		}

		// Here's a check that will allow us to load up save games on the playstation from the PC
		//  - It will skip data from any players after MAX_PLAYERS
		if (psSaveDroid->player >= MAX_PLAYERS)
		{
			NumberOfSkippedDroids++;
			psSaveDroid->player=MAX_PLAYERS-1;	// now don't lose any droids ... force them to be the last player
		}

		psDroid = buildDroidFromSaveDroidV11(psSaveDroid);

		if (psDroid == NULL)
		{
			debug( LOG_ERROR, "unitLoad: Template not found for unit\n" );
			abort();
		}
		else if (psSaveDroid->saveType == DROID_ON_TRANSPORT)
		{
   			//add the droid to the list
			for(i = 0;i < psDroid->numWeaps;i++)
			{
				psDroid->psTarget[i] = NULL;
				psDroid->psActionTarget[i] = NULL;
			}
			psDroid->psBaseStruct = NULL;
			ASSERT( psCurrentTransGroup != NULL,"loadSaveUnitV9; Transporter unit without group " );
			grpJoin(psCurrentTransGroup, psDroid);
		}
		else
		{
			//add the droid to the list
			addDroid(psDroid, ppsCurrentDroidLists);
		}

		if (psDroid != NULL)
		{
			if (psDroid->droidType == DROID_TRANSPORTER)
			{
				//set current TransPorter group
				if (!grpCreate(&psGrp))
				{
					debug( LOG_NEVER, "unit build: unable to create group\n" );
					return FALSE;
				}
				grpJoin(psGrp, psDroid);
				psCurrentTransGroup = psDroid->psGroup;
			}
		}
	}
	if (NumberOfSkippedDroids>0)
	{
		debug( LOG_ERROR, "unitLoad: Bad Player number in %d unit(s)... assigned to the last player!\n", NumberOfSkippedDroids );
		abort();
	}

	ppsCurrentDroidLists = NULL;//ensure it always gets set

	return TRUE;
}

// -----------------------------------------------------------------------------------------
/* code specific all versions upto from 12 to 19*/
BOOL loadSaveDroidV19(char *pFileData, UDWORD filesize, UDWORD numDroids, UDWORD version, DROID **ppsCurrentDroidLists)
{
	SAVE_DROID_V18				*psSaveDroid, sSaveDroid;
//	DROID_TEMPLATE			*psTemplate, sTemplate;
	DROID					*psDroid;
	DROID_GROUP				*psCurrentTransGroup;
	UDWORD					count;
	UDWORD					NumberOfSkippedDroids=0;
	UDWORD					sizeOfSaveDroid = 0;
	DROID_GROUP				*psGrp;
	UBYTE	i;

	psCurrentTransGroup = NULL;

	psSaveDroid = &sSaveDroid;
	if (version <= VERSION_10)
	{
		sizeOfSaveDroid = sizeof(SAVE_DROID_V9);
	}
	else if (version == VERSION_11)
	{
		sizeOfSaveDroid = sizeof(SAVE_DROID_V11);
	}
	else if (version == VERSION_12)
	{
		sizeOfSaveDroid = sizeof(SAVE_DROID_V12);
	}
	else if (version < VERSION_18)
	{
		sizeOfSaveDroid = sizeof(SAVE_DROID_V14);
	}
	else if (version <= CURRENT_VERSION_NUM)
	{
		sizeOfSaveDroid = sizeof(SAVE_DROID_V18);
	}

	if ((sizeOfSaveDroid * numDroids + DROID_HEADER_SIZE) >
		filesize)
	{
		debug( LOG_ERROR, "unitLoad: unexpected end of file" );
		abort();
		return FALSE;
	}

	/* Load in the droid data */
	for (count = 0; count < numDroids; count ++, pFileData += sizeOfSaveDroid)
	{
		memcpy(psSaveDroid, pFileData, sizeOfSaveDroid);

		/* DROID_SAVE_V18 includes DROID_SAVE_V12*/
		endian_udword(&psSaveDroid->baseStructID);
		endian_udword(&psSaveDroid->died);
		endian_udword(&psSaveDroid->lastEmission);
		/* DROID_SAVE_V12 includes DROID_SAVE_V9 */
		endian_uword(&psSaveDroid->turretRotation);
		endian_uword(&psSaveDroid->turretPitch);
		endian_sdword(&psSaveDroid->order);
		endian_uword(&psSaveDroid->orderX);
		endian_uword(&psSaveDroid->orderY);
		endian_uword(&psSaveDroid->orderX2);
		endian_uword(&psSaveDroid->orderY2);
		endian_udword(&psSaveDroid->timeLastHit);
		endian_udword(&psSaveDroid->targetID);
		endian_udword(&psSaveDroid->secondaryOrder);
		endian_sdword(&psSaveDroid->action);
		endian_udword(&psSaveDroid->actionX);
		endian_udword(&psSaveDroid->actionY);
		endian_udword(&psSaveDroid->actionTargetID);
		endian_udword(&psSaveDroid->actionStarted);
		endian_udword(&psSaveDroid->actionPoints);
		endian_uword(&psSaveDroid->actionHeight);
		/* DROID_SAVE_V9 includes OBJECT_SAVE_V19, SAVE_WEAPON_V19 */
		endian_udword(&psSaveDroid->body);
		endian_udword(&psSaveDroid->saveType);
		endian_udword(&psSaveDroid->numWeaps);
		endian_udword(&psSaveDroid->numKills);
		/* OBJECT_SAVE_V19 */
		endian_udword(&psSaveDroid->id);
		endian_udword(&psSaveDroid->x);
		endian_udword(&psSaveDroid->y);
		endian_udword(&psSaveDroid->z);
		endian_udword(&psSaveDroid->direction);
		endian_udword(&psSaveDroid->player);
		endian_udword(&psSaveDroid->burnStart);
		endian_udword(&psSaveDroid->burnDamage);
		for(i = 0; i < TEMP_DROID_MAXPROGS; i++) {
			/* SAVE_WEAPON_V19 */
			endian_udword(&psSaveDroid->asWeaps[i].hitPoints);
			endian_udword(&psSaveDroid->asWeaps[i].ammo);
			endian_udword(&psSaveDroid->asWeaps[i].lastFired);
		}

		// Here's a check that will allow us to load up save games on the playstation from the PC
		//  - It will skip data from any players after MAX_PLAYERS
		if (psSaveDroid->player >= MAX_PLAYERS)
		{
			NumberOfSkippedDroids++;
			psSaveDroid->player=MAX_PLAYERS-1;	// now don't lose any droids ... force them to be the last player
		}

		psDroid = buildDroidFromSaveDroidV19((SAVE_DROID_V18*)psSaveDroid, version);

		if (psDroid == NULL)
		{
			ASSERT( psDroid != NULL,"unitLoad: Failed to build new unit\n" );
		}
		else if (psSaveDroid->saveType == DROID_ON_TRANSPORT)
		{
  			//add the droid to the list
			psDroid->order = DORDER_NONE;
			psDroid->action = DACTION_NONE;
			if (psDroid->numWeaps > 0)
			{
				for(i = 0;i < psDroid->numWeaps;i++)
				{
					psDroid->psTarget[i] = NULL;
					psDroid->psActionTarget[i] = NULL;
				}
			}
			else
			{
				psDroid->psTarget[0] = NULL;
				psDroid->psActionTarget[0] = NULL;
			}
			psDroid->psBaseStruct = NULL;
			//add the droid to the list
			ASSERT( psCurrentTransGroup != NULL,"loadSaveUnitV9; Transporter unit without group " );
			grpJoin(psCurrentTransGroup, psDroid);
		}
		else
		{
			//add the droid to the list
			addDroid(psDroid, ppsCurrentDroidLists);
		}

		if (psDroid != NULL)
		{
			if (psDroid->droidType == DROID_TRANSPORTER)
			{
				//set current TransPorter group
				if (!grpCreate(&psGrp))
				{
					debug( LOG_NEVER, "unit build: unable to create group\n" );
					return FALSE;
				}
				grpJoin(psGrp, psDroid);
				psCurrentTransGroup = psDroid->psGroup;
			}
		}
	}
	if (NumberOfSkippedDroids>0)
	{
		debug( LOG_ERROR, "unitLoad: Bad Player number in %d unit(s)... assigned to the last player!\n", NumberOfSkippedDroids );
		abort();
	}

	ppsCurrentDroidLists = NULL;//ensure it always gets set

	return TRUE;
}

// -----------------------------------------------------------------------------------------
/* code for all versions after save name change v19*/
BOOL loadSaveDroidV(char *pFileData, UDWORD filesize, UDWORD numDroids, UDWORD version, DROID **ppsCurrentDroidLists)
{
	SAVE_DROID				*psSaveDroid, sSaveDroid;
//	DROID_TEMPLATE			*psTemplate, sTemplate;
	DROID					*psDroid;
	DROID_GROUP				*psCurrentTransGroup;
	UDWORD					count;
	UDWORD					NumberOfSkippedDroids=0;
	UDWORD					sizeOfSaveDroid = 0;
//	DROID_GROUP				*psGrp;
	UBYTE	i;

	psCurrentTransGroup = NULL;

	psSaveDroid = &sSaveDroid;
	if (version <= VERSION_20)
	{
		sizeOfSaveDroid = sizeof(SAVE_DROID_V20);
	}
	else if (version <= VERSION_23)
	{
		sizeOfSaveDroid = sizeof(SAVE_DROID_V21);
	}
	else if (version <= CURRENT_VERSION_NUM)
	{
		sizeOfSaveDroid = sizeof(SAVE_DROID);
	}

	if ((sizeOfSaveDroid * numDroids + DROID_HEADER_SIZE) >
		filesize)
	{
		debug( LOG_ERROR, "unitLoad: unexpected end of file" );
		abort();
		return FALSE;
	}

	/* Load in the droid data */
	for (count = 0; count < numDroids; count ++, pFileData += sizeOfSaveDroid)
	{
		memcpy(psSaveDroid, pFileData, sizeOfSaveDroid);

		/* DROID_SAVE_V24 includes DROID_SAVE_V21,
		 * SAVE_MOVE_CONTROL */
		endian_sdword(&psSaveDroid->resistance);
		endian_sword(&psSaveDroid->formationDir);
		endian_sdword(&psSaveDroid->formationX);
		endian_sdword(&psSaveDroid->formationY);
		/* DROID_SAVE_V21 includes DROID_SAVE_V20 */
		endian_udword(&psSaveDroid->commandId);
		/* DROID_SAVE_V20 includes OBJECT_SAVE_V20, SAVE_WEAPON */
		endian_udword(&psSaveDroid->body);
		endian_udword(&psSaveDroid->saveType);
		endian_udword(&psSaveDroid->numWeaps);
		endian_udword(&psSaveDroid->numKills);
		//Watermelon:'hack' to make it read turretRotation,Pitch table properly
		if( version == CURRENT_VERSION_NUM )
		{
			for(i = 0;i < psSaveDroid->numWeaps;i++)
			{
				endian_uword(&psSaveDroid->turretRotation[i]);
				endian_uword(&psSaveDroid->turretPitch[i]);
			}
		}
		else
		{
			endian_uword(&psSaveDroid->turretRotation[0]);
			endian_uword(&psSaveDroid->turretPitch[0]);
		}
		endian_sdword(&psSaveDroid->order);
		endian_uword(&psSaveDroid->orderX);
		endian_uword(&psSaveDroid->orderY);
		endian_uword(&psSaveDroid->orderX2);
		endian_uword(&psSaveDroid->orderY2);
		endian_udword(&psSaveDroid->timeLastHit);
		endian_udword(&psSaveDroid->targetID);
		endian_udword(&psSaveDroid->secondaryOrder);
		endian_sdword(&psSaveDroid->action);
		endian_udword(&psSaveDroid->actionX);
		endian_udword(&psSaveDroid->actionY);
		endian_udword(&psSaveDroid->actionTargetID);
		endian_udword(&psSaveDroid->actionStarted);
		endian_udword(&psSaveDroid->actionPoints);
		endian_uword(&psSaveDroid->actionHeight);
		endian_udword(&psSaveDroid->baseStructID);
		endian_udword(&psSaveDroid->died);
		endian_udword(&psSaveDroid->lastEmission);
		/* OBJECT_SAVE_V20 */
		endian_udword(&psSaveDroid->id);
		endian_udword(&psSaveDroid->x);
		endian_udword(&psSaveDroid->y);
		endian_udword(&psSaveDroid->z);
		endian_udword(&psSaveDroid->direction);
		endian_udword(&psSaveDroid->player);
		endian_udword(&psSaveDroid->burnStart);
		endian_udword(&psSaveDroid->burnDamage);
		/* SAVE_MOVE_CONTROL */
		endian_sdword(&psSaveDroid->sMove.DestinationX);
		endian_sdword(&psSaveDroid->sMove.DestinationY);
		endian_sdword(&psSaveDroid->sMove.srcX);
		endian_sdword(&psSaveDroid->sMove.srcY);
		endian_sdword(&psSaveDroid->sMove.targetX);
		endian_sdword(&psSaveDroid->sMove.targetY);
		endian_sword(&psSaveDroid->sMove.boundX);
		endian_sword(&psSaveDroid->sMove.boundY);
		endian_sword(&psSaveDroid->sMove.dir);
		endian_sword(&psSaveDroid->sMove.bumpDir);
		endian_udword(&psSaveDroid->sMove.bumpTime);
		endian_uword(&psSaveDroid->sMove.lastBump);
		endian_uword(&psSaveDroid->sMove.pauseTime);
		endian_uword(&psSaveDroid->sMove.bumpX);
		endian_uword(&psSaveDroid->sMove.bumpY);
		endian_udword(&psSaveDroid->sMove.shuffleStart);
		endian_sword(&psSaveDroid->sMove.iVertSpeed);
		if( version == CURRENT_VERSION_NUM )
		{
			for(i = 0;i < psSaveDroid->numWeaps;i++)
			{
				endian_uword(&psSaveDroid->sMove.iAttackRuns[i]);
			}
			//endian_uword(&psSaveDroid->sMove.iGuardRadius);
		}
		else
		{
			endian_uword(&psSaveDroid->sMove.iAttackRuns[0]);
		}
		for(i = 0; i < TEMP_DROID_MAXPROGS; i++) {
			/* SAVE_WEAPON */
			endian_udword(&psSaveDroid->asWeaps[i].hitPoints);
			endian_udword(&psSaveDroid->asWeaps[i].ammo);
			endian_udword(&psSaveDroid->asWeaps[i].lastFired);
		}

		// Here's a check that will allow us to load up save games on the playstation from the PC
		//  - It will skip data from any players after MAX_PLAYERS
		if (psSaveDroid->player >= MAX_PLAYERS)
		{
			NumberOfSkippedDroids++;
			psSaveDroid->player=MAX_PLAYERS-1;	// now don't lose any droids ... force them to be the last player
		}

		psDroid = buildDroidFromSaveDroid(psSaveDroid, version);

		if (psDroid == NULL)
		{
			ASSERT( psDroid != NULL,"unitLoad: Failed to build new unit\n" );
		}
		else if (psSaveDroid->saveType == DROID_ON_TRANSPORT)
		{
  			//add the droid to the list
			psDroid->order = DORDER_NONE;
			psDroid->action = DACTION_NONE;
			for(i = 0;i < psDroid->numWeaps;i++)
			{
				psDroid->psTarget[i] = NULL;
				psDroid->psActionTarget[i] = NULL;
			}
			psDroid->psBaseStruct = NULL;
			//add the droid to the list
			psDroid->psGroup = NULL;
			psDroid->psGrpNext = NULL;
			ASSERT( psCurrentTransGroup != NULL,"loadSaveUnitV9; Transporter unit without group " );
			grpJoin(psCurrentTransGroup, psDroid);
		}
		else if (psDroid->droidType == DROID_TRANSPORTER)
		{
				//set current TransPorter group
/*set in build droid
				if (!grpCreate(&psGrp))
				{
					DBPRINTF(("droid build: unable to create group\n"));
					return FALSE;
				}
				psDroid->psGroup = NULL;
				grpJoin(psGrp, psDroid);
*/
			psCurrentTransGroup = psDroid->psGroup;
			addDroid(psDroid, ppsCurrentDroidLists);
		}
		else
		{
			//add the droid to the list
			addDroid(psDroid, ppsCurrentDroidLists);
		}
	}
	if (NumberOfSkippedDroids>0)
	{
		debug( LOG_ERROR, "unitLoad: Bad Player number in %d unit(s)... assigned to the last player!\n", NumberOfSkippedDroids );
		abort();
	}

	ppsCurrentDroidLists = NULL;//ensure it always gets set

	return TRUE;
}

// -----------------------------------------------------------------------------------------
static BOOL buildSaveDroidFromDroid(SAVE_DROID* psSaveDroid, DROID* psCurr, DROID_SAVE_TYPE saveType)
{
	UDWORD				i;

#ifdef ALLOWSAVE

//			strcpy(psSaveDroid->name, psCurr->pName);

			/*want to store the resource ID string for compatibilty with
			different versions of save game - NOT HAPPENING - the name saved is
			the translated name - old versions of save games should load because
			templates are loaded from Access AND the save game so they should all
			still exist*/
			strcpy(psSaveDroid->name, psCurr->aName);

			//for (i=0; i < DROID_MAXCOMP; i++) not interested in first comp - COMP_UNKNOWN
			for (i=1; i < DROID_MAXCOMP; i++)
			{

				if (!getNameFromComp(i, psSaveDroid->asBits[i].name, psCurr->asBits[i].nStat))

				{
					//ignore this record
					break;
				}
			}
			psSaveDroid->body = psCurr->body;
//			psSaveDroid->activeWeapon = psCurr->activeWeapon;
			//psSaveDroid->numWeaps = psCurr->numWeaps;
			/*for (i=0; i < psCurr->numWeaps; i++)
			{
				if (!getNameFromComp(COMP_WEAPON, psSaveDroid->asWeaps[i].name, psCurr->asWeaps[i].nStat))
				{
					//ignore this record
					//continue;
					break;
				}
				psSaveDroid->asWeaps[i].hitPoints = psCurr->asWeaps[i].hitPoints;
				psSaveDroid->asWeaps[i].ammo = psCurr->asWeaps[i].ammo;
				psSaveDroid->asWeaps[i].lastFired = psCurr->asWeaps[i].lastFired;
			}*/
			//Watermelon:loop thru all weapons
            psSaveDroid->numWeaps = psCurr->numWeaps;
			for(i = 0;i < psCurr->numWeaps;i++)
			{
				if (psCurr->asWeaps[i].nStat > 0)
				{
					//there is only one weapon now

					if (getNameFromComp(COMP_WEAPON, psSaveDroid->asWeaps[i].name, psCurr->asWeaps[i].nStat))

					{
    					psSaveDroid->asWeaps[i].hitPoints = psCurr->asWeaps[i].hitPoints;
	    				psSaveDroid->asWeaps[i].ammo = psCurr->asWeaps[i].ammo;
		    			psSaveDroid->asWeaps[i].lastFired = psCurr->asWeaps[i].lastFired;
					}
				}
				else
				{

					psSaveDroid->asWeaps[i].name[i] = '\0';
				}
			}


            //save out experience level
			psSaveDroid->numKills		= psCurr->numKills;
			//version 11
			//Watermelon:endian_udword for new save format
			for(i = 0;i < psCurr->numWeaps;i++)
			{
				psSaveDroid->turretRotation[i] = psCurr->turretRotation[i];
				psSaveDroid->turretPitch[i]	= psCurr->turretPitch[i];
			}
			//version 12
			psSaveDroid->order			= psCurr->order;
			psSaveDroid->orderX			= psCurr->orderX;
			psSaveDroid->orderY			= psCurr->orderY;
			psSaveDroid->orderX2		= psCurr->orderX2;
			psSaveDroid->orderY2		= psCurr->orderY2;
			psSaveDroid->timeLastHit	= psCurr->timeLastHit;
			if (psCurr->psTarget[0] != NULL)
			{
				ASSERT( psCurr->psTarget[0]->id != 0xdddddddd,"SaveUnit found freed target" );
				if (psCurr->psTarget[0]->died <= 1)
				{
					psSaveDroid->targetID		= psCurr->psTarget[0]->id;
					if	(!checkValidId(psSaveDroid->targetID))
					{
						psSaveDroid->targetID		= UDWORD_MAX;
					}
				}
				else
				{
						psSaveDroid->targetID		= UDWORD_MAX;
				}
			}
			else
			{
				psSaveDroid->targetID		= UDWORD_MAX;
			}
			psSaveDroid->secondaryOrder = psCurr->secondaryOrder;
			psSaveDroid->action			= psCurr->action;
			psSaveDroid->actionX		= psCurr->actionX;
			psSaveDroid->actionY		= psCurr->actionY;
			if (psCurr->psActionTarget[0] != NULL)
			{
				ASSERT( psCurr->psActionTarget[0]->id != 0xdddddddd,"SaveUnit found freed action target" );
				if (psCurr->psActionTarget[0]->died <= 1)
				{
					psSaveDroid->actionTargetID		= psCurr->psActionTarget[0]->id;
					if	(!checkValidId(psSaveDroid->actionTargetID))
					{
						psSaveDroid->actionTargetID		= UDWORD_MAX;
					}
				}
				else
				{
						psSaveDroid->actionTargetID		= UDWORD_MAX;
				}

			}
			else
			{
				psSaveDroid->actionTargetID		= UDWORD_MAX;
			}
			psSaveDroid->actionStarted	= psCurr->actionStarted;
			psSaveDroid->actionPoints	= psCurr->actionPoints;
            //actionHeight has been renamed to powerAccrued - AB 7/1/99
			//psSaveDroid->actionHeight	= psCurr->actionHeight;
            psSaveDroid->actionHeight	= psCurr->powerAccrued;

			//version 14
			if (psCurr->psTarStats[0] != NULL)
			{
				ASSERT( strlen(psCurr->psTarStats[0]->pName) < MAX_NAME_SIZE,"writeUnitFile; psTarStat pName Error" );
				strcpy(psSaveDroid->tarStatName,psCurr->psTarStats[0]->pName);
			}
			else
			{
				strcpy(psSaveDroid->tarStatName,"");
			}
			if ((psCurr->psBaseStruct != NULL) && ((UDWORD)(psCurr->psBaseStruct) != UDWORD_MAX))
			{
				if (psCurr->psBaseStruct->died <= 1)
				{
					psSaveDroid->baseStructID		= psCurr->psBaseStruct->id;
					if (!checkValidId(psSaveDroid->baseStructID))
					{
						psSaveDroid->baseStructID		= UDWORD_MAX;
					}
				}
				else
				{
					psSaveDroid->baseStructID		= UDWORD_MAX;
				}
			}
			else
			{
				psSaveDroid->baseStructID		= UDWORD_MAX;
			}
			psSaveDroid->group = psCurr->group;
			psSaveDroid->selected = psCurr->selected;
//20feb			psSaveDroid->cluster = psCurr->cluster;
			psSaveDroid->died = psCurr->died;
			psSaveDroid->lastEmission = psCurr->lastEmission;
			for (i=0; i < MAX_PLAYERS; i++)
			{
				psSaveDroid->visible[i] = psCurr->visible[i];
			}

			//version 21
			if ((psCurr->psGroup) && (psCurr->droidType != DROID_COMMAND))
			{
				if (psCurr->psGroup->type == GT_COMMAND)
				{
					if (((DROID*)psCurr->psGroup->psCommander)->died <= 1)
					{
						psSaveDroid->commandId = ((DROID*)psCurr->psGroup->psCommander)->id;
						ASSERT( checkValidId(psSaveDroid->commandId),"SaveUnit pcCommander not found" );
					}
					else
					{
						psSaveDroid->commandId = UDWORD_MAX;
						ASSERT( FALSE,"SaveUnit pcCommander died" );
					}
				}
				else
				{
					psSaveDroid->commandId = UDWORD_MAX;
				}
			}
			else
			{
				psSaveDroid->commandId = UDWORD_MAX;
			}

			//version 24
			psSaveDroid->resistance = psCurr->resistance;
			memcpy(&psSaveDroid->sMove, &psCurr->sMove, sizeof(SAVE_MOVE_CONTROL));
			if (psSaveDroid->sMove.psFormation != NULL)
			{
				psSaveDroid->formationDir	= psCurr->sMove.psFormation->dir;
				psSaveDroid->formationX		= psCurr->sMove.psFormation->x;
				psSaveDroid->formationY		= psCurr->sMove.psFormation->y;
			}
			else
			{
				psSaveDroid->formationDir	= 0;
				psSaveDroid->formationX		= 0;
				psSaveDroid->formationY		= 0;
			}

			/*psSaveDroid->activeProg = psCurr->activeProg;
			psSaveDroid->numProgs = psCurr->numProgs;
			for (i=0; i < psCurr->numProgs; i++)
			{
				if (!getNameFromComp(COMP_PROGRAM, psSaveDroid->asProgs[i], psCurr->asProgs[i].psStats - asProgramStats))
				{
					//ignore this record
					continue;
				}
			}*/

//			psSaveDroid->sMove = psCurr->sMove;
//			psSaveDroid->sDisplay = psCurr->sDisplay;

			/* psSaveDroid->aiState = psCurr->sAI.state;
			if (psCurr->sAI.psTarget)
			{
				psSaveDroid->subjectID = psCurr->sAI.psTarget->id;
				psSaveDroid->subjectType = psCurr->sAI.psTarget->type;
				psSaveDroid->subjectPlayer = psCurr->sAI.psTarget->player;
			}
			else
			{
				psSaveDroid->subjectID = -1;
				psSaveDroid->subjectType = 0;
				psSaveDroid->subjectPlayer = 0;
			}
			if (psCurr->sAI.psSelectedWeapon)
			{
				psSaveDroid->weaponInc = psCurr->sAI.psSelectedWeapon->psStats->ref - REF_WEAPON_START;
			}
			else
			{
				psSaveDroid->weaponInc = 0;
			}
			if(psCurr->sAI.psStructToBuild)
			{
				psSaveDroid->structureInc = psCurr->sAI.psStructToBuild - asStructureStats;
			}
			else
			{
				psSaveDroid->structureInc = -1;
			}
			psSaveDroid->timeStarted = psCurr->sAI.timeStarted;
			psSaveDroid->pointsToAdd = psCurr->sAI.pointsToAdd; */

#endif
			psSaveDroid->id = psCurr->id;
			psSaveDroid->x = psCurr->x;
			psSaveDroid->y = psCurr->y;
			psSaveDroid->z = psCurr->z;
			psSaveDroid->direction = psCurr->direction;
			psSaveDroid->player = psCurr->player;
			psSaveDroid->inFire = psCurr->inFire;
			psSaveDroid->burnStart = psCurr->burnStart;
			psSaveDroid->burnDamage = psCurr->burnDamage;
			psSaveDroid->droidType = (UBYTE)psCurr->droidType;
			psSaveDroid->saveType = (UBYTE)saveType;//identifies special load cases

			/* SAVE_DROID is DROID_SAVE_V24 */
			/* DROID_SAVE_V24 includes DROID_SAVE_V21 */
			endian_sdword(&psSaveDroid->resistance);
			endian_sdword(&psSaveDroid->sMove.DestinationX);
			endian_sdword(&psSaveDroid->sMove.DestinationY);
			endian_sdword(&psSaveDroid->sMove.srcX);
			endian_sdword(&psSaveDroid->sMove.srcY);
			endian_sdword(&psSaveDroid->sMove.targetX);
			endian_sdword(&psSaveDroid->sMove.targetY);
			endian_fract(&psSaveDroid->sMove.fx);
			endian_fract(&psSaveDroid->sMove.fy);
			endian_fract(&psSaveDroid->sMove.speed);
			endian_sword(&psSaveDroid->sMove.boundX);
			endian_sword(&psSaveDroid->sMove.boundY);
			endian_sword(&psSaveDroid->sMove.dir);
			endian_sword(&psSaveDroid->sMove.bumpDir);
			endian_udword(&psSaveDroid->sMove.bumpTime);
			endian_uword(&psSaveDroid->sMove.lastBump);
			endian_uword(&psSaveDroid->sMove.pauseTime);
			endian_uword(&psSaveDroid->sMove.bumpX);
			endian_uword(&psSaveDroid->sMove.bumpY);
/* I don't think this is necessary.
			if(psSaveDroid->sMove.psFormation) {
				endian_sword(&psSaveDroid->sMove.psFormation->refCount);
				endian_sword(&psSaveDroid->sMove.psFormation->size);
				endian_sword(&psSaveDroid->sMove.psFormation->rankDist);
				endian_sword(&psSaveDroid->sMove.psFormation->dir);
				endian_sdword(&psSaveDroid->sMove.psFormation->x);
				endian_sdword(&psSaveDroid->sMove.psFormation->y);
				for(i = 0; i < F_MAXLINES; i++) {
					endian_sword(&psSaveDroid->sMove.psFormation->asLines[i].xoffset);
					endian_sword(&psSaveDroid->sMove.psFormation->asLines[i].yoffset);
					endian_sword(&psSaveDroid->sMove.psFormation->asLines[i].dir);
				}
				endian_sword(&psSaveDroid->sMove.psFormation->numLines);
				for(i = 0; i < F_MAXMEMBERS; i++)
					endian_sword(&psSaveDroid->sMove.psFormation->asMembers[i].dist);
				endian_udword(&psSaveDroid->sMove.psFormation->iSpeed);
			}
*/
			endian_sword(&psSaveDroid->sMove.iVertSpeed);

			for(i = 0;i < psSaveDroid->numWeaps;i++)
			{
				endian_uword(&psSaveDroid->sMove.iAttackRuns[i]);
			}
			//endian_uword(&psSaveDroid->sMove.iGuardRadius);

			endian_sword(&psSaveDroid->formationDir);
			endian_sdword(&psSaveDroid->formationX);
			endian_sdword(&psSaveDroid->formationY);
			/* DROID_SAVE_V21 includes DROID_SAVE_V20 */
			endian_udword(&psSaveDroid->commandId);
			/* DROID_SAVE_V20 includes OBJECT_SAVE_V20 */
			endian_udword(&psSaveDroid->body);
			endian_udword(&psSaveDroid->saveType);
			for(i = 0; i < TEMP_DROID_MAXPROGS; i++) {
				endian_udword(&psSaveDroid->asWeaps[i].hitPoints);
				endian_udword(&psSaveDroid->asWeaps[i].ammo);
				endian_udword(&psSaveDroid->asWeaps[i].lastFired);
			}
			endian_udword(&psSaveDroid->numKills);
			//Watermelon:endian_udword for new save format
			for(i = 0;i < psSaveDroid->numWeaps;i++)
			{
				endian_uword(&psSaveDroid->turretRotation[i]);
				endian_uword(&psSaveDroid->turretPitch[i]);
			}
			endian_udword(&psSaveDroid->numWeaps);
			endian_sdword(&psSaveDroid->order);
			endian_uword(&psSaveDroid->orderX);
			endian_uword(&psSaveDroid->orderY);
			endian_uword(&psSaveDroid->orderX2);
			endian_uword(&psSaveDroid->orderY2);
			endian_udword(&psSaveDroid->timeLastHit);
			endian_udword(&psSaveDroid->targetID);
			endian_udword(&psSaveDroid->secondaryOrder);
			endian_sdword(&psSaveDroid->action);
			endian_udword(&psSaveDroid->actionX);
			endian_udword(&psSaveDroid->actionY);
			endian_udword(&psSaveDroid->actionTargetID);
			endian_udword(&psSaveDroid->actionStarted);
			endian_udword(&psSaveDroid->actionPoints);
			endian_uword(&psSaveDroid->actionHeight);
			endian_udword(&psSaveDroid->baseStructID);
			endian_udword(&psSaveDroid->died);
			endian_udword(&psSaveDroid->lastEmission);
			/* OBJECT_SAVE_V20 */
			endian_udword(&psSaveDroid->id);
			endian_udword(&psSaveDroid->x);
			endian_udword(&psSaveDroid->y);
			endian_udword(&psSaveDroid->z);
			endian_udword(&psSaveDroid->direction);
			endian_udword(&psSaveDroid->player);
			endian_udword(&psSaveDroid->burnStart);
			endian_udword(&psSaveDroid->burnDamage);

			return TRUE;
}

// -----------------------------------------------------------------------------------------
/*
Writes the linked list of droids for each player to a file
*/
BOOL writeDroidFile(char *pFileName, DROID **ppsCurrentDroidLists)
{
	char *pFileData = NULL;
	UDWORD				fileSize, player, totalDroids=0;
	DROID				*psCurr;
	DROID				*psTrans;
	DROID_SAVEHEADER	*psHeader;
	SAVE_DROID			*psSaveDroid;
	BOOL status = TRUE;

	//total all the droids in the world
	for (player = 0; player < MAX_PLAYERS; player++)
	{
		for (psCurr = ppsCurrentDroidLists[player]; psCurr != NULL; psCurr = psCurr->psNext)
		{
			totalDroids++;
			// if transporter save any droids in the grp
			if (psCurr->droidType == DROID_TRANSPORTER)
			{
				psTrans = psCurr->psGroup->psList;
				if (((UDWORD)psTrans->psGrpNext) == 0xcdcdcdcd)
				{
					debug( LOG_ERROR, "transporter ->psGrpNext not reset" );
					abort();
				}
				else
				{
					for(psTrans = psTrans->psGrpNext; psTrans != NULL; psTrans = psTrans->psGrpNext)
					{
						totalDroids++;
					}
				}
			}
		}
	}

	/* Allocate the data buffer */
	fileSize = DROID_HEADER_SIZE + totalDroids*sizeof(SAVE_DROID);
	pFileData = (char*)MALLOC(fileSize);
	if (pFileData == NULL)
	{
		debug( LOG_ERROR, "Out of memory" );
		abort();
		return FALSE;
	}

	/* Put the file header on the file */
	psHeader = (DROID_SAVEHEADER *)pFileData;
	psHeader->aFileType[0] = 'd';
	psHeader->aFileType[1] = 'r';
	psHeader->aFileType[2] = 'o';
	psHeader->aFileType[3] = 'd';
	psHeader->version = CURRENT_VERSION_NUM;
	psHeader->quantity = totalDroids;

	endian_udword(&psHeader->version);
	endian_udword(&psHeader->quantity);

	psSaveDroid = (SAVE_DROID*)(pFileData + DROID_HEADER_SIZE);

	/* Put the droid data into the buffer */
	for (player = 0; player < MAX_PLAYERS; player++)
	{
		for(psCurr = ppsCurrentDroidLists[player]; psCurr != NULL; psCurr = psCurr->psNext)
		{
            //always save transporter droids that are in the mission list with an INVALID_XY
            if (psCurr->droidType == DROID_TRANSPORTER &&
                ppsCurrentDroidLists[player] == mission.apsDroidLists[player])
            {
                psCurr->x = INVALID_XY;
                psCurr->y = INVALID_XY;
            }

			buildSaveDroidFromDroid(psSaveDroid, psCurr, DROID_NORMAL);

			psSaveDroid = (SAVE_DROID *)((char *)psSaveDroid + sizeof(SAVE_DROID));
			// if transporter save any droids in the grp
			if (psCurr->droidType == DROID_TRANSPORTER)
			{
				psTrans = psCurr->psGroup->psList;
				for(psTrans = psCurr->psGroup->psList; psTrans != NULL; psTrans = psTrans->psGrpNext)
				{
					if (psTrans->droidType != DROID_TRANSPORTER)
					{
						buildSaveDroidFromDroid(psSaveDroid, psTrans, DROID_ON_TRANSPORT);
						psSaveDroid = (SAVE_DROID *)((char *)psSaveDroid + sizeof(SAVE_DROID));
					}
				}
			}
		}
	}

	ppsCurrentDroidLists = NULL;//ensure it always gets set

	/* Write the data to the file */
	if (pFileData != NULL) {
		status = saveFile(pFileName, pFileData, fileSize);
		FREE(pFileData);
		return status;
	}
	return FALSE;
}


// -----------------------------------------------------------------------------------------
BOOL loadSaveStructure(char *pFileData, UDWORD filesize)
{
	STRUCT_SAVEHEADER		*psHeader;

	/* Check the file type */
	psHeader = (STRUCT_SAVEHEADER *)pFileData;
	if (psHeader->aFileType[0] != 's' || psHeader->aFileType[1] != 't' ||
		psHeader->aFileType[2] != 'r' || psHeader->aFileType[3] != 'u')
	{
		debug( LOG_ERROR, "loadSaveStructure: Incorrect file type" );
		abort();
		return FALSE;
	}

	/* STRUCT_SAVEHEADER */
	endian_udword(&psHeader->version);
	endian_udword(&psHeader->quantity);

	//increment to the start of the data
	pFileData += STRUCT_HEADER_SIZE;

	/* Check the file version */
	if (psHeader->version < VERSION_7)
	{
		debug( LOG_ERROR, "StructLoad: unsupported save format version %d", psHeader->version );
		abort();
		return FALSE;
	}
	else if (psHeader->version < VERSION_9)
	{
		if (!loadSaveStructureV7(pFileData, filesize, psHeader->quantity))
		{
			return FALSE;
		}
	}
	else if (psHeader->version <= VERSION_19)
	{
		if (!loadSaveStructureV19(pFileData, filesize, psHeader->quantity, psHeader->version))
		{
			return FALSE;
		}
	}
	else if (psHeader->version <= CURRENT_VERSION_NUM)
	{
		if (!loadSaveStructureV(pFileData, filesize, psHeader->quantity, psHeader->version))
		{
			return FALSE;
		}
	}
	else
	{
		debug( LOG_ERROR, "StructLoad: undefined save format version %d", psHeader->version );
		abort();
		return FALSE;
	}

	return TRUE;
}


// -----------------------------------------------------------------------------------------
/* code specific to version 7 of a save structure */
BOOL loadSaveStructureV7(char *pFileData, UDWORD filesize, UDWORD numStructures)
{
	SAVE_STRUCTURE_V2		*psSaveStructure, sSaveStructure;
	STRUCTURE				*psStructure;
	REPAIR_FACILITY			*psRepair;
	STRUCTURE_STATS			*psStats = NULL;
	UDWORD					count, statInc;
	BOOL					found;
	UDWORD					NumberOfSkippedStructures=0;
	UDWORD					burnTime;

	psSaveStructure = &sSaveStructure;

	if ((sizeof(SAVE_STRUCTURE_V2) * numStructures + STRUCT_HEADER_SIZE) >
		filesize)
	{
		debug( LOG_ERROR, "structureLoad: unexpected end of file" );
		abort();
		return FALSE;
	}

	/* Load in the structure data */
	for (count = 0; count < numStructures; count ++, pFileData += sizeof(SAVE_STRUCTURE_V2))
	{
		memcpy(psSaveStructure, pFileData, sizeof(SAVE_STRUCTURE_V2));

		/* STRUCTURE_SAVE_V2 includes OBJECT_SAVE_V19 */
		endian_sdword(&psSaveStructure->currentBuildPts);
		endian_udword(&psSaveStructure->body);
		endian_udword(&psSaveStructure->armour);
		endian_udword(&psSaveStructure->resistance);
		endian_udword(&psSaveStructure->dummy1);
		endian_udword(&psSaveStructure->subjectInc);
		endian_udword(&psSaveStructure->timeStarted);
		endian_udword(&psSaveStructure->output);
		endian_udword(&psSaveStructure->capacity);
		endian_udword(&psSaveStructure->quantity);
		/* OBJECT_SAVE_V19 */
		endian_udword(&psSaveStructure->id);
		endian_udword(&psSaveStructure->x);
		endian_udword(&psSaveStructure->y);
		endian_udword(&psSaveStructure->z);
		endian_udword(&psSaveStructure->direction);
		endian_udword(&psSaveStructure->player);
		endian_udword(&psSaveStructure->burnStart);
		endian_udword(&psSaveStructure->burnDamage);

		psSaveStructure->player=RemapPlayerNumber(psSaveStructure->player);


		if (psSaveStructure->player >= MAX_PLAYERS)
		{
			psSaveStructure->player=MAX_PLAYERS-1;
			NumberOfSkippedStructures++;
		}
		//get the stats for this structure
		found = FALSE;


		if (!getSaveObjectName(psSaveStructure->name))
		{
			continue;
		}

		for (statInc = 0; statInc < numStructureStats; statInc++)
		{
			psStats = asStructureStats + statInc;
			//loop until find the same name

			if (!strcmp(psStats->pName, psSaveStructure->name))
			{
				found = TRUE;
				break;
			}
		}
		//if haven't found the structure - ignore this record!
		if (!found)
		{
			debug( LOG_ERROR, "This structure no longer exists - %s", getSaveStructNameV19((SAVE_STRUCTURE_V17*)psSaveStructure) );
			abort();
			continue;
		}
		/*create the Structure */
		//psStructure = buildStructure((asStructureStats + psSaveStructure->
		//	structureInc), psSaveStructure->x, psSaveStructure->y,
		//	psSaveStructure->player);

		//for modules - need to check the base structure exists
		if (IsStatExpansionModule(psStats))
		{
			psStructure = getTileStructure(psSaveStructure->x >> TILE_SHIFT,
				psSaveStructure->y >> TILE_SHIFT);
			if (psStructure == NULL)
			{
				debug( LOG_ERROR, "No owning structure for module - %s for player - %d", getSaveStructNameV19((SAVE_STRUCTURE_V17*)psSaveStructure), psSaveStructure->player );
				abort();
				//ignore this module
				continue;
			}
		}

        //check not too near the edge
        /*if (psSaveStructure->x <= TILE_UNITS || psSaveStructure->y <= TILE_UNITS)
        {
			DBERROR(("Structure being built too near the edge of the map"));
            continue;
        }*/

        //check not trying to build too near the edge
    	if(((psSaveStructure->x >> TILE_SHIFT) < TOO_NEAR_EDGE) || ((
            psSaveStructure->x >> TILE_SHIFT) > (mapWidth - TOO_NEAR_EDGE)))
        {
			debug( LOG_ERROR, "Structure %s, x coord too near the edge of the map. id - %d", getSaveStructNameV19((SAVE_STRUCTURE_V17*)psSaveStructure), psSaveStructure->id );
			abort();
            continue;
        }
    	if(((psSaveStructure->y >> TILE_SHIFT) < TOO_NEAR_EDGE) || ((
            psSaveStructure->y >> TILE_SHIFT) > (mapHeight - TOO_NEAR_EDGE)))
        {
			debug( LOG_ERROR, "Structure %s, y coord too near the edge of the map. id - %d", getSaveStructNameV19((SAVE_STRUCTURE_V17*)psSaveStructure), psSaveStructure->id );
			abort();
            continue;
        }

        psStructure = buildStructure(psStats, psSaveStructure->x, psSaveStructure->y,
			psSaveStructure->player,TRUE);
		if (!psStructure)
		{
			ASSERT( FALSE, "loadSaveStructure:Unable to create structure" );
			return FALSE;
		}

        /*The original code here didn't work and so the scriptwriters worked
        round it by using the module ID - so making it work now will screw up
        the scripts -so in ALL CASES overwrite the ID!*/
		//don't copy the module's id etc
		//if (IsStatExpansionModule(psStats)==FALSE)
		{
			//copy the values across
			psStructure->id = psSaveStructure->id;
			//are these going to ever change from the values set up with?
//			psStructure->z = (UWORD)psSaveStructure->z;
			psStructure->direction = (UWORD)psSaveStructure->direction;
		}


		psStructure->inFire = psSaveStructure->inFire;
		psStructure->burnDamage = psSaveStructure->burnDamage;
		burnTime = psSaveStructure->burnStart;
		psStructure->burnStart = burnTime;

		psStructure->status = psSaveStructure->status;
		if (psStructure->status ==SS_BUILT)
		{
			buildingComplete(psStructure);
		}

		//if not a save game, don't want to overwrite any of the stats so continue
		if (gameType != GTYPE_SAVE_START)
		{
			continue;
		}

		psStructure->currentBuildPts = (SWORD)psSaveStructure->currentBuildPts;
//		psStructure->body = psSaveStructure->body;
//		psStructure->armour = psSaveStructure->armour;
//		psStructure->resistance = psSaveStructure->resistance;
//		psStructure->repair = psSaveStructure->repair;
		switch (psStructure->pStructureType->type)
		{
			case REF_FACTORY:
				//NOT DONE AT PRESENT
				((FACTORY *)psStructure->pFunctionality)->capacity = (UBYTE)psSaveStructure->
					capacity;
				//((FACTORY *)psStructure->pFunctionality)->productionOutput = psSaveStructure->
				//	output;
				//((FACTORY *)psStructure->pFunctionality)->quantity = psSaveStructure->
				//	quantity;
				//((FACTORY *)psStructure->pFunctionality)->timeStarted = gameTime -
				//	savedGameTime - (psSaveStructure->timeStarted);
				//((FACTORY*)psStructure->pFunctionality)->timeToBuild = ((DROID_TEMPLATE *)
				//	psSaveStructure->subjectInc)->buildPoints / ((FACTORY *)psStructure->pFunctionality)->
				//	productionOutput;

				//adjust the module structures IMD
				if (psSaveStructure->capacity)
				{
					psStructure->sDisplay.imd = factoryModuleIMDs[psSaveStructure->
						capacity-1][0];
				}
				break;
			case REF_RESEARCH:
				((RESEARCH_FACILITY *)psStructure->pFunctionality)->capacity =
					psSaveStructure->capacity;
				((RESEARCH_FACILITY *)psStructure->pFunctionality)->researchPoints =
					psSaveStructure->output;
				((RESEARCH_FACILITY *)psStructure->pFunctionality)->timeStarted = (psSaveStructure->timeStarted);
				if (psSaveStructure->subjectInc != (UDWORD)-1)
				{
					((RESEARCH_FACILITY *)psStructure->pFunctionality)->psSubject = (BASE_STATS *)
						(asResearch + psSaveStructure->subjectInc);
					((RESEARCH_FACILITY*)psStructure->pFunctionality)->timeToResearch =
						(asResearch + psSaveStructure->subjectInc)->researchPoints /
						((RESEARCH_FACILITY *)psStructure->pFunctionality)->
						researchPoints;
				}
				//adjust the module structures IMD
				if (psSaveStructure->capacity)
				{
					psStructure->sDisplay.imd = researchModuleIMDs[psSaveStructure->
						capacity-1];
				}
				break;
			case REF_REPAIR_FACILITY: //CODE THIS SOMETIME
				psRepair = ((REPAIR_FACILITY *)psStructure->pFunctionality);
				psRepair->psDeliveryPoint = NULL;
				psRepair->psObj = NULL;
                psRepair->currentPtsAdded = 0;
				break;
		}
	}

	if (NumberOfSkippedStructures>0)
	{
		debug( LOG_ERROR, "structureLoad: invalid player number in %d structures ... assigned to the last player!\n\n", NumberOfSkippedStructures );
		abort();
	}

	return TRUE;
}

// -----------------------------------------------------------------------------------------
//return id of a research topic based on the name
static UDWORD getResearchIdFromName(char *pName)
{
	UDWORD inc;


	for (inc=0; inc < numResearch; inc++)
	{

		if (!strcmp(asResearch[inc].pName, pName))
		{
			return inc;
		}
	}

	debug( LOG_ERROR, "Unknown research - %s", pName );
	abort();
	return UDWORD_MAX;
}


// -----------------------------------------------------------------------------------------
/* code for version upto 19of a save structure */
BOOL loadSaveStructureV19(char *pFileData, UDWORD filesize, UDWORD numStructures, UDWORD version)
{
	SAVE_STRUCTURE_V17			*psSaveStructure, sSaveStructure;
	STRUCTURE				*psStructure;
	FACTORY					*psFactory;
	RESEARCH_FACILITY		*psResearch;
	REPAIR_FACILITY			*psRepair;
	STRUCTURE_STATS			*psStats = NULL;
    STRUCTURE_STATS			*psModule;
	UDWORD					capacity;
	UDWORD					count, statInc;
	BOOL					found;
	UDWORD					NumberOfSkippedStructures=0;
	UDWORD					burnTime;
	UDWORD					i;
	UDWORD					sizeOfSaveStruture;
	UDWORD					researchId;

	if (version < VERSION_12)
	{
		sizeOfSaveStruture = sizeof(SAVE_STRUCTURE_V2);
	}
	else if (version < VERSION_14)
	{
		sizeOfSaveStruture = sizeof(SAVE_STRUCTURE_V12);
	}
	else if (version <= VERSION_14)
	{
		sizeOfSaveStruture = sizeof(SAVE_STRUCTURE_V14);
	}
	else if (version <= VERSION_16)
	{
		sizeOfSaveStruture = sizeof(SAVE_STRUCTURE_V15);
	}
	else
	{
		sizeOfSaveStruture = sizeof(SAVE_STRUCTURE_V17);
	}

	psSaveStructure = &sSaveStructure;

	if ((sizeOfSaveStruture * numStructures + STRUCT_HEADER_SIZE) >
		filesize)
	{
		debug( LOG_ERROR, "structureLoad: unexpected end of file" );
		abort();
		return FALSE;
	}

	/* Load in the structure data */
	for (count = 0; count < numStructures; count ++, pFileData += sizeOfSaveStruture)
	{
		memcpy(psSaveStructure, pFileData, sizeOfSaveStruture);

		/* SAVE_STRUCTURE_V17 is STRUCTURE_SAVE_V17 */
		/* STRUCTURE_SAVE_V17 includes STRUCTURE_SAVE_V15 */
		endian_sword(&psSaveStructure->currentPowerAccrued);
		/* STRUCTURE_SAVE_V15 includes STRUCTURE_SAVE_V14 */
		/* STRUCTURE_SAVE_V14 includes STRUCTURE_SAVE_V12 */
		endian_udword(&psSaveStructure->factoryInc);
		endian_udword(&psSaveStructure->powerAccrued);
		endian_udword(&psSaveStructure->dummy2);
		endian_udword(&psSaveStructure->droidTimeStarted);
		endian_udword(&psSaveStructure->timeToBuild);
		endian_udword(&psSaveStructure->timeStartHold);
		/* STRUCTURE_SAVE_V12 includes STRUCTURE_SAVE_V2 */
		endian_sdword(&psSaveStructure->currentBuildPts);
		endian_udword(&psSaveStructure->body);
		endian_udword(&psSaveStructure->armour);
		endian_udword(&psSaveStructure->resistance);
		endian_udword(&psSaveStructure->dummy1);
		endian_udword(&psSaveStructure->subjectInc);
		endian_udword(&psSaveStructure->timeStarted);
		endian_udword(&psSaveStructure->output);
		endian_udword(&psSaveStructure->capacity);
		/* STRUCTURE_SAVE_V2 includes OBJECT_SAVE_V19 */
		endian_udword(&psSaveStructure->id);
		endian_udword(&psSaveStructure->x);
		endian_udword(&psSaveStructure->y);
		endian_udword(&psSaveStructure->z);
		endian_udword(&psSaveStructure->direction);
		endian_udword(&psSaveStructure->player);
		endian_udword(&psSaveStructure->burnStart);
		endian_udword(&psSaveStructure->burnDamage);

		psSaveStructure->player=RemapPlayerNumber(psSaveStructure->player);

		if (psSaveStructure->player >= MAX_PLAYERS)
		{
			psSaveStructure->player=MAX_PLAYERS-1;
			NumberOfSkippedStructures++;

		}
		//get the stats for this structure
		found = FALSE;


		if (!getSaveObjectName(psSaveStructure->name))
		{
			continue;
		}

		for (statInc = 0; statInc < numStructureStats; statInc++)
		{
			psStats = asStructureStats + statInc;
			//loop until find the same name

			if (!strcmp(psStats->pName, psSaveStructure->name))
			{
				found = TRUE;
				break;
			}
		}
		//if haven't found the structure - ignore this record!
		if (!found)
		{
			debug( LOG_ERROR, "This structure no longer exists - %s", getSaveStructNameV19(psSaveStructure) );
			abort();
			continue;
		}
		/*create the Structure */
		//for modules - need to check the base structure exists
		if (IsStatExpansionModule(psStats))
		{
			psStructure = getTileStructure(psSaveStructure->x >> TILE_SHIFT,
				psSaveStructure->y >> TILE_SHIFT);
			if (psStructure == NULL)
			{
				debug( LOG_ERROR, "No owning structure for module - %s for player - %d", getSaveStructNameV19(psSaveStructure), psSaveStructure->player );
				abort();
				//ignore this module
				continue;
			}
		}
        //check not too near the edge
        /*if (psSaveStructure->x <= TILE_UNITS || psSaveStructure->y <= TILE_UNITS)
        {
			DBERROR(("Structure being built too near the edge of the map"));
            continue;
        }*/
        //check not trying to build too near the edge
    	if(((psSaveStructure->x >> TILE_SHIFT) < TOO_NEAR_EDGE) || ((
            psSaveStructure->x >> TILE_SHIFT) > (mapWidth - TOO_NEAR_EDGE)))
        {
			debug( LOG_ERROR, "Structure %s, x coord too near the edge of the map. id - %d", getSaveStructNameV19((SAVE_STRUCTURE_V17*)psSaveStructure), psSaveStructure->id );
			abort();
            continue;
        }
    	if(((psSaveStructure->y >> TILE_SHIFT) < TOO_NEAR_EDGE) || ((
            psSaveStructure->y >> TILE_SHIFT) > (mapHeight - TOO_NEAR_EDGE)))
        {
			debug( LOG_ERROR, "Structure %s, y coord too near the edge of the map. id - %d", getSaveStructNameV19((SAVE_STRUCTURE_V17*)psSaveStructure), psSaveStructure->id );
			abort();
            continue;
        }

		psStructure = buildStructure(psStats, psSaveStructure->x, psSaveStructure->y,
			psSaveStructure->player,TRUE);
		if (!psStructure)
		{
			ASSERT( FALSE, "loadSaveStructure:Unable to create structure" );
			return FALSE;
		}

        /*The original code here didn't work and so the scriptwriters worked
        round it by using the module ID - so making it work now will screw up
        the scripts -so in ALL CASES overwrite the ID!*/
		//don't copy the module's id etc
		//if (IsStatExpansionModule(psStats)==FALSE)
		{
			//copy the values across
			psStructure->id = psSaveStructure->id;
			//are these going to ever change from the values set up with?
//			psStructure->z = (UWORD)psSaveStructure->z;
			psStructure->direction = (UWORD)psSaveStructure->direction;
		}

		psStructure->inFire = psSaveStructure->inFire;
		psStructure->burnDamage = psSaveStructure->burnDamage;
		burnTime = psSaveStructure->burnStart;
		psStructure->burnStart = burnTime;
		if (version >= VERSION_14)
		{
			for (i=0; i < MAX_PLAYERS; i++)
			{
				psStructure->visible[i] = psSaveStructure->visible[i];
                //set the Tile flag if visible for the selectedPlayer
                if ((i == selectedPlayer) && (psStructure->visible[i] == UBYTE_MAX))
                {
                    setStructTileDraw(psStructure);
                }
			}
		}
		psStructure->status = psSaveStructure->status;
		if (psStructure->status ==SS_BUILT)
		{
			buildingComplete(psStructure);
		}

		//if not a save game, don't want to overwrite any of the stats so continue
		if ((gameType != GTYPE_SAVE_START) &&
			(gameType != GTYPE_SAVE_MIDMISSION))
		{
			continue;
		}

		if (version <= VERSION_16)
		{
			if (psSaveStructure->currentBuildPts > SWORD_MAX)//old MAX_BODY
			{
				psSaveStructure->currentBuildPts = SWORD_MAX;
			}
			psStructure->currentBuildPts = (SWORD)psSaveStructure->currentBuildPts;
			psStructure->currentPowerAccrued = 0;
		}
		else
		{
			psStructure->currentBuildPts = (SWORD)psSaveStructure->currentBuildPts;
			psStructure->currentPowerAccrued = psSaveStructure->currentPowerAccrued;
		}
//		psStructure->repair = (UWORD)psSaveStructure->repair;
		switch (psStructure->pStructureType->type)
		{
			case REF_FACTORY:
            case REF_VTOL_FACTORY:
            case REF_CYBORG_FACTORY:
				//if factory save the current build info
				if (version >= VERSION_12)
				{
					psFactory = ((FACTORY *)psStructure->pFunctionality);
					psFactory->capacity = 0;//capacity reset during module build (UBYTE)psSaveStructure->capacity;
                    //this is set up during module build - if the stats have changed it will also set up with the latest value
					//psFactory->productionOutput = (UBYTE)psSaveStructure->output;
					psFactory->quantity = (UBYTE)psSaveStructure->quantity;
					psFactory->timeStarted = psSaveStructure->droidTimeStarted;
					psFactory->powerAccrued = psSaveStructure->powerAccrued;
					psFactory->timeToBuild = psSaveStructure->timeToBuild;
					psFactory->timeStartHold = psSaveStructure->timeStartHold;

					//adjust the module structures IMD
					if (psSaveStructure->capacity)
					{
					    psModule = getModuleStat(psStructure);
						capacity = psSaveStructure->capacity;
						//build the appropriate number of modules
						while (capacity)
						{
		                    buildStructure(psModule, psStructure->x, psStructure->y,
                                psStructure->player, FALSE);
                            capacity--;
						}

					}
				// imd set by building modules
				//	psStructure->sDisplay.imd = factoryModuleIMDs[psSaveStructure->capacity-1][0];

				//if factory reset the delivery points
					//this trashes the flag pos pointer but flag pos list is cleared when flags load
					//assemblyCheck
					FIXME_CAST_ASSIGN(UDWORD, psFactory->psAssemblyPoint, psSaveStructure->factoryInc);
					//if factory was building find the template from the unique ID
					if (psSaveStructure->subjectInc == UDWORD_MAX)
					{
						psFactory->psSubject = NULL;
					}
					else
					{
						psFactory->psSubject = (BASE_STATS*)
                            getTemplateFromMultiPlayerID(psSaveStructure->subjectInc);
                        //if the build has started set the powerAccrued =
                        //powerRequired to sync the interface
                        if (psFactory->timeStarted != ACTION_START_TIME &&
                            psFactory->psSubject)
                        {
                            psFactory->powerAccrued = ((DROID_TEMPLATE *)psFactory->
                                psSubject)->powerPoints;
                        }
					}
				}
				break;
			case REF_RESEARCH:
				psResearch = ((RESEARCH_FACILITY *)psStructure->pFunctionality);
				psResearch->capacity = 0;//capacity set when module loaded psSaveStructure->capacity;
                //this is set up during module build - if the stats have changed it will also set up with the latest value
                //psResearch->researchPoints = psSaveStructure->output;
				psResearch->powerAccrued = psSaveStructure->powerAccrued;
				//clear subject
				psResearch->psSubject = NULL;
				psResearch->timeToResearch = 0;
				psResearch->timeStarted = 0;
				//set the subject
				if (saveGameVersion >= VERSION_15)
				{
					if (psSaveStructure->subjectInc != UDWORD_MAX)
					{
						researchId = getResearchIdFromName(psSaveStructure->researchName);
						if (researchId != UDWORD_MAX)
						{
							psResearch->psSubject = (BASE_STATS *)(asResearch + researchId);
							psResearch->timeToResearch = (asResearch + researchId)->researchPoints / psResearch->researchPoints;
							psResearch->timeStarted = psSaveStructure->timeStarted;
						}
					}
					else
					{
						psResearch->psSubject = NULL;
						psResearch->timeToResearch = 0;
						psResearch->timeStarted = 0;
					}
				}
				else
				{
					psResearch->timeStarted = (psSaveStructure->timeStarted);
					if (psSaveStructure->subjectInc != UDWORD_MAX)
					{
						psResearch->psSubject = (BASE_STATS *)(asResearch + psSaveStructure->subjectInc);
						psResearch->timeToResearch = (asResearch + psSaveStructure->subjectInc)->researchPoints / psResearch->researchPoints;
					}

				}
                //if started research, set powerAccrued = powerRequired
                if (psResearch->timeStarted != ACTION_START_TIME && psResearch->
                    psSubject)
                {
                    psResearch->powerAccrued = ((RESEARCH *)psResearch->
                        psSubject)->researchPower;
                }
				//adjust the module structures IMD
				if (psSaveStructure->capacity)
				{
				    psModule = getModuleStat(psStructure);
					capacity = psSaveStructure->capacity;
					//build the appropriate number of modules
                    buildStructure(psModule, psStructure->x, psStructure->y, psStructure->player, FALSE);
//					psStructure->sDisplay.imd = researchModuleIMDs[psSaveStructure->capacity-1];
				}
				break;
			case REF_POWER_GEN:
				//adjust the module structures IMD
				if (psSaveStructure->capacity)
				{
				    psModule = getModuleStat(psStructure);
					capacity = psSaveStructure->capacity;
					//build the appropriate number of modules
                    buildStructure(psModule, psStructure->x, psStructure->y, psStructure->player, FALSE);
//					psStructure->sDisplay.imd = powerModuleIMDs[psSaveStructure->capacity-1];
				}
				break;
			case REF_RESOURCE_EXTRACTOR:
				((RES_EXTRACTOR *)psStructure->pFunctionality)->power = psSaveStructure->output;
                //if run out of power, then the res_extractor should be inactive
                if (psSaveStructure->output == 0)
                {
                    ((RES_EXTRACTOR *)psStructure->pFunctionality)->active = FALSE;
                }
				break;
			case REF_REPAIR_FACILITY: //CODE THIS SOMETIME
				if (version >= VERSION_19)
				{
					psRepair = ((REPAIR_FACILITY *)psStructure->pFunctionality);

					psRepair->power = ((REPAIR_DROID_FUNCTION *) psStructure->pStructureType->asFuncList[0])->repairPoints;
					psRepair->timeStarted = psSaveStructure->droidTimeStarted;
					psRepair->powerAccrued = psSaveStructure->powerAccrued;
                    psRepair->currentPtsAdded = 0;

					//if repair facility reset the delivery points
					//this trashes the flag pos pointer but flag pos list is cleared when flags load
					//assemblyCheck
					psRepair->psDeliveryPoint = NULL;
					//if factory was building find the template from the unique ID
					if (psSaveStructure->subjectInc == UDWORD_MAX)
					{
						psRepair->psObj = NULL;
					}
					else
					{
						psRepair->psObj = getBaseObjFromId(psSaveStructure->subjectInc);
                        //if the build has started set the powerAccrued =
                        //powerRequired to sync the interface
                        if (psRepair->timeStarted != ACTION_START_TIME &&
                            psRepair->psObj)
                        {
                            psRepair->powerAccrued = powerReqForDroidRepair((DROID*)psRepair->psObj);
                        }
					}
				}
				else
				{
					psRepair = ((REPAIR_FACILITY *)psStructure->pFunctionality);
                    //init so setAssemplyPoint check will re-allocate one
					psRepair->psObj = NULL;
                    psRepair->psDeliveryPoint = NULL;
				}
				break;
		}
		//get the base body points
		psStructure->body = (UWORD)structureBody(psStructure);
		if (psSaveStructure->body < psStructure->body)
		{
			psStructure->body = (UWORD)psSaveStructure->body;
		}
		//set the build status from the build points
		psStructure->currentBuildPts = (SWORD)psStructure->pStructureType->buildPoints;
		if (psSaveStructure->currentBuildPts < psStructure->currentBuildPts)
		{
			psStructure->currentBuildPts = (SWORD)psSaveStructure->currentBuildPts;
            psStructure->status = SS_BEING_BUILT;
		}
		else
		{
            psStructure->status = SS_BUILT;
//            buildingComplete(psStructure);//replaced by following switch
			switch (psStructure->pStructureType->type)
			{
				case REF_POWER_GEN:
					checkForResExtractors(psStructure);
					if(selectedPlayer == psStructure->player)
					{
						audio_PlayObjStaticTrack( (void *) psStructure, ID_SOUND_POWER_HUM );
					}
					break;
				case REF_RESOURCE_EXTRACTOR:
                    //only try and connect if power left in
                    if (((RES_EXTRACTOR *)psStructure->pFunctionality)->power != 0)
                    {
    					checkForPowerGen(psStructure);
	    				/* GJ HACK! - add anim to deriks */
		    			if (psStructure->psCurAnim == NULL)
			    		{
				    		psStructure->psCurAnim = animObj_Add(psStructure, ID_ANIM_DERIK, 0, 0);
					    }
                    }

					break;
				case REF_RESEARCH:
//21feb					intCheckResearchButton();
					break;
				default:
					//do nothing for factories etc
					break;
			}
		}
	}

	if (NumberOfSkippedStructures>0)
	{
		debug( LOG_ERROR, "structureLoad: invalid player number in %d structures ... assigned to the last player!\n\n", NumberOfSkippedStructures );
		abort();
	}

	return TRUE;
}


// -----------------------------------------------------------------------------------------
/* code for versions after version 20 of a save structure */
BOOL loadSaveStructureV(char *pFileData, UDWORD filesize, UDWORD numStructures, UDWORD version)
{
	SAVE_STRUCTURE			*psSaveStructure, sSaveStructure;
	STRUCTURE				*psStructure;
	FACTORY					*psFactory;
	RESEARCH_FACILITY		*psResearch;
	REPAIR_FACILITY			*psRepair;
	REARM_PAD				*psReArmPad;
	STRUCTURE_STATS			*psStats = NULL;
    STRUCTURE_STATS			*psModule;
	UDWORD					capacity;
	UDWORD					count, statInc;
	BOOL					found;
	UDWORD					NumberOfSkippedStructures=0;
	UDWORD					burnTime;
	UDWORD					i;
	UDWORD					sizeOfSaveStructure = 0;
	UDWORD					researchId;



	if (version <= VERSION_20)
	{
		sizeOfSaveStructure = sizeof(SAVE_STRUCTURE_V20);
	}
	else if (version <= CURRENT_VERSION_NUM)
	{
		sizeOfSaveStructure = sizeof(SAVE_STRUCTURE);
	}

	psSaveStructure = &sSaveStructure;

	if ((sizeOfSaveStructure * numStructures + STRUCT_HEADER_SIZE) >
		filesize)
	{
		debug( LOG_ERROR, "structureLoad: unexpected end of file" );
		abort();
		return FALSE;
	}

	/* Load in the structure data */
	for (count = 0; count < numStructures; count ++, pFileData += sizeOfSaveStructure)
	{
		memcpy(psSaveStructure, pFileData, sizeOfSaveStructure);

		/* SAVE_STRUCTURE is STRUCTURE_SAVE_V21 */
		/* STRUCTURE_SAVE_V21 includes STRUCTURE_SAVE_V20 */
		endian_udword(&psSaveStructure->commandId);
		/* STRUCTURE_SAVE_V20 includes OBJECT_SAVE_V20 */
		endian_sdword(&psSaveStructure->currentBuildPts);
		endian_udword(&psSaveStructure->body);
		endian_udword(&psSaveStructure->armour);
		endian_udword(&psSaveStructure->resistance);
		endian_udword(&psSaveStructure->dummy1);
		endian_udword(&psSaveStructure->subjectInc);
		endian_udword(&psSaveStructure->timeStarted);
		endian_udword(&psSaveStructure->output);
		endian_udword(&psSaveStructure->capacity);
		endian_udword(&psSaveStructure->quantity);
		endian_udword(&psSaveStructure->factoryInc);
		endian_udword(&psSaveStructure->powerAccrued);
		endian_udword(&psSaveStructure->dummy2);
		endian_udword(&psSaveStructure->droidTimeStarted);
		endian_udword(&psSaveStructure->timeToBuild);
		endian_udword(&psSaveStructure->timeStartHold);
		endian_sword(&psSaveStructure->currentPowerAccrued);
		/* OBJECT_SAVE_V20 */
		endian_udword(&psSaveStructure->id);
		endian_udword(&psSaveStructure->x);
		endian_udword(&psSaveStructure->y);
		endian_udword(&psSaveStructure->z);
		endian_udword(&psSaveStructure->direction);
		endian_udword(&psSaveStructure->player);
		endian_udword(&psSaveStructure->burnStart);
		endian_udword(&psSaveStructure->burnDamage);

		psSaveStructure->player=RemapPlayerNumber(psSaveStructure->player);

		if (psSaveStructure->player >= MAX_PLAYERS)
		{
			psSaveStructure->player=MAX_PLAYERS-1;
			NumberOfSkippedStructures++;

		}
		//get the stats for this structure
		found = FALSE;


		if (!getSaveObjectName(psSaveStructure->name))
		{
			continue;
		}

		for (statInc = 0; statInc < numStructureStats; statInc++)
		{
			psStats = asStructureStats + statInc;
			//loop until find the same name

			if (!strcmp(psStats->pName, psSaveStructure->name))
			{
				found = TRUE;
				break;
			}
		}
		//if haven't found the structure - ignore this record!
		if (!found)
		{
			debug( LOG_ERROR, "This structure no longer exists - %s", getSaveStructNameV(psSaveStructure) );
			abort();
			continue;
		}
		/*create the Structure */
		//for modules - need to check the base structure exists
		if (IsStatExpansionModule(psStats))
		{
			psStructure = getTileStructure(psSaveStructure->x >> TILE_SHIFT,
				psSaveStructure->y >> TILE_SHIFT);
			if (psStructure == NULL)
			{
				debug( LOG_ERROR, "No owning structure for module - %s for player - %d", getSaveStructNameV(psSaveStructure), psSaveStructure->player );
				abort();
				//ignore this module
				continue;
			}
		}
        //check not too near the edge
        /*if (psSaveStructure->x <= TILE_UNITS || psSaveStructure->y <= TILE_UNITS)
        {
			DBERROR(("Structure being built too near the edge of the map"));
            continue;
        }*/
        //check not trying to build too near the edge
    	if(((psSaveStructure->x >> TILE_SHIFT) < TOO_NEAR_EDGE) || ((
            psSaveStructure->x >> TILE_SHIFT) > (mapWidth - TOO_NEAR_EDGE)))
        {
			debug( LOG_ERROR, "Structure %s, x coord too near the edge of the map. id - %d", getSaveStructNameV((SAVE_STRUCTURE*)psSaveStructure), psSaveStructure->id );
			abort();
            continue;
        }
    	if(((psSaveStructure->y >> TILE_SHIFT) < TOO_NEAR_EDGE) || ((
            psSaveStructure->y >> TILE_SHIFT) > (mapHeight - TOO_NEAR_EDGE)))
        {
			debug( LOG_ERROR, "Structure %s, y coord too near the edge of the map. id - %d", getSaveStructNameV((SAVE_STRUCTURE*)psSaveStructure), psSaveStructure->id );
			abort();
            continue;
        }

		psStructure = buildStructure(psStats, psSaveStructure->x, psSaveStructure->y,
			psSaveStructure->player,TRUE);
		if (!psStructure)
		{
			ASSERT( FALSE, "loadSaveStructure:Unable to create structure" );
			return FALSE;
		}

        /*The original code here didn't work and so the scriptwriters worked
        round it by using the module ID - so making it work now will screw up
        the scripts -so in ALL CASES overwrite the ID!*/
		//don't copy the module's id etc
		//if (IsStatExpansionModule(psStats)==FALSE)
		{
			//copy the values across
			psStructure->id = psSaveStructure->id;
			//are these going to ever change from the values set up with?
//			psStructure->z = (UWORD)psSaveStructure->z;
			psStructure->direction = (UWORD)psSaveStructure->direction;
		}

		psStructure->inFire = psSaveStructure->inFire;
		psStructure->burnDamage = psSaveStructure->burnDamage;
		burnTime = psSaveStructure->burnStart;
		psStructure->burnStart = burnTime;
		for (i=0; i < MAX_PLAYERS; i++)
		{
			psStructure->visible[i] = psSaveStructure->visible[i];
            //set the Tile flag if visible for the selectedPlayer
            if ((i == selectedPlayer) && (psStructure->visible[i] == UBYTE_MAX))
            {
                setStructTileDraw(psStructure);
            }
		}

		psStructure->status = psSaveStructure->status;
		if (psStructure->status ==SS_BUILT)
		{
			buildingComplete(psStructure);
		}

		//if not a save game, don't want to overwrite any of the stats so continue
		if ((gameType != GTYPE_SAVE_START) &&
			(gameType != GTYPE_SAVE_MIDMISSION))
		{
			continue;
		}

		psStructure->currentBuildPts = (SWORD)psSaveStructure->currentBuildPts;
		psStructure->currentPowerAccrued = psSaveStructure->currentPowerAccrued;
        psStructure->resistance = (SWORD)psSaveStructure->resistance;
        //armour not ever adjusted...


//		psStructure->repair = (UWORD)psSaveStructure->repair;
		switch (psStructure->pStructureType->type)
		{
			case REF_FACTORY:
            case REF_VTOL_FACTORY:
            case REF_CYBORG_FACTORY:
				//if factory save the current build info
				psFactory = ((FACTORY *)psStructure->pFunctionality);
				psFactory->capacity = 0;//capacity reset during module build (UBYTE)psSaveStructure->capacity;
                //this is set up during module build - if the stats have changed it will also set up with the latest value
				//psFactory->productionOutput = (UBYTE)psSaveStructure->output;
				psFactory->quantity = (UBYTE)psSaveStructure->quantity;
				psFactory->timeStarted = psSaveStructure->droidTimeStarted;
				psFactory->powerAccrued = psSaveStructure->powerAccrued;
				psFactory->timeToBuild = psSaveStructure->timeToBuild;
				psFactory->timeStartHold = psSaveStructure->timeStartHold;

				//adjust the module structures IMD
				if (psSaveStructure->capacity)
				{
					psModule = getModuleStat(psStructure);
					capacity = psSaveStructure->capacity;
					//build the appropriate number of modules
					while (capacity)
					{
		                buildStructure(psModule, psStructure->x, psStructure->y,
                            psStructure->player, FALSE);
                        capacity--;
					}

				}
			// imd set by building modules
			//	psStructure->sDisplay.imd = factoryModuleIMDs[psSaveStructure->capacity-1][0];

			//if factory reset the delivery points
				//this trashes the flag pos pointer but flag pos list is cleared when flags load
				//assemblyCheck
				FIXME_CAST_ASSIGN(UDWORD, psFactory->psAssemblyPoint, psSaveStructure->factoryInc);
				//if factory was building find the template from the unique ID
				if (psSaveStructure->subjectInc == UDWORD_MAX)
				{
					psFactory->psSubject = NULL;
				}
				else
				{
					psFactory->psSubject = (BASE_STATS*)
                        getTemplateFromMultiPlayerID(psSaveStructure->subjectInc);
                    //if the build has started set the powerAccrued =
                    //powerRequired to sync the interface
                    if (psFactory->timeStarted != ACTION_START_TIME &&
                        psFactory->psSubject)
                    {
                        psFactory->powerAccrued = ((DROID_TEMPLATE *)psFactory->
                            psSubject)->powerPoints;
                    }
				}
				if (version >= VERSION_21)//version 21
				{
					//reset command id in loadStructSetPointers
					FIXME_CAST_ASSIGN(UDWORD, psFactory->psCommander, psSaveStructure->commandId);
				}
                //secondary order added - AB 22/04/99
                if (version >= VERSION_32)
                {
                    psFactory->secondaryOrder = psSaveStructure->dummy2;
                }
				break;
			case REF_RESEARCH:
				psResearch = ((RESEARCH_FACILITY *)psStructure->pFunctionality);
				psResearch->capacity = 0;//capacity set when module loaded psSaveStructure->capacity;
				//adjust the module structures IMD
				if (psSaveStructure->capacity)
				{
				    psModule = getModuleStat(psStructure);
					capacity = psSaveStructure->capacity;
					//build the appropriate number of modules
                    buildStructure(psModule, psStructure->x, psStructure->y, psStructure->player, FALSE);
				}
                //this is set up during module build - if the stats have changed it will also set up with the latest value
				psResearch->powerAccrued = psSaveStructure->powerAccrued;
				//clear subject
				psResearch->psSubject = NULL;
				psResearch->timeToResearch = 0;
				psResearch->timeStarted = 0;
                psResearch->timeStartHold = 0;
				//set the subject
				if (psSaveStructure->subjectInc != UDWORD_MAX)
				{
					researchId = getResearchIdFromName(psSaveStructure->researchName);
					if (researchId != UDWORD_MAX)
					{
						psResearch->psSubject = (BASE_STATS *)(asResearch + researchId);
						psResearch->timeToResearch = (asResearch + researchId)->researchPoints / psResearch->researchPoints;
						psResearch->timeStarted = psSaveStructure->timeStarted;
                        if (saveGameVersion >= VERSION_20)
                        {
                            psResearch->timeStartHold = psSaveStructure->timeStartHold;
                        }
					}
				}
                //if started research, set powerAccrued = powerRequired
                if (psResearch->timeStarted != ACTION_START_TIME && psResearch->
                    psSubject)
                {
                    psResearch->powerAccrued = ((RESEARCH *)psResearch->
                        psSubject)->researchPower;
                }
				break;
			case REF_POWER_GEN:
				//adjust the module structures IMD
				if (psSaveStructure->capacity)
				{
				    psModule = getModuleStat(psStructure);
					capacity = psSaveStructure->capacity;
					//build the appropriate number of modules
                    buildStructure(psModule, psStructure->x, psStructure->y, psStructure->player, FALSE);
//					psStructure->sDisplay.imd = powerModuleIMDs[psSaveStructure->capacity-1];
				}
				break;
			case REF_RESOURCE_EXTRACTOR:
				((RES_EXTRACTOR *)psStructure->pFunctionality)->power = psSaveStructure->output;
                //if run out of power, then the res_extractor should be inactive
                if (psSaveStructure->output == 0)
                {
                    ((RES_EXTRACTOR *)psStructure->pFunctionality)->active = FALSE;
                }
				break;
			case REF_REPAIR_FACILITY: //CODE THIS SOMETIME
/*
	// The group the droids to be repaired by this facility belong to
	struct _droid_group		*psGroup;
	struct _droid			*psGrpNext;
*/
				psRepair = ((REPAIR_FACILITY *)psStructure->pFunctionality);

				psRepair->power = ((REPAIR_DROID_FUNCTION *) psStructure->pStructureType->asFuncList[0])->repairPoints;
				psRepair->timeStarted = psSaveStructure->droidTimeStarted;
				psRepair->powerAccrued = psSaveStructure->powerAccrued;

				//if repair facility reset the delivery points
				//this trashes the flag pos pointer but flag pos list is cleared when flags load
				//assemblyCheck
				psRepair->psDeliveryPoint = NULL;
				//if  repair facility was  repairing find the object later
				FIXME_CAST_ASSIGN(UDWORD, psRepair->psObj, psSaveStructure->subjectInc);
                if (version < VERSION_27)
                {
                    psRepair->currentPtsAdded = 0;
                }
                else
                {
                    psRepair->currentPtsAdded = psSaveStructure->dummy2;
                }
				break;
			case REF_REARM_PAD:
				if (version >= VERSION_26)//version 26
				{
					psReArmPad = ((REARM_PAD *)psStructure->pFunctionality);
					psReArmPad->reArmPoints = psSaveStructure->output;//set in build structure ?
					psReArmPad->timeStarted = psSaveStructure->droidTimeStarted;
					//if  ReArm Pad was  rearming find the object later
					FIXME_CAST_ASSIGN(UDWORD, psReArmPad->psObj, psSaveStructure->subjectInc);
                    if (version < VERSION_28)
                    {
                        psReArmPad->currentPtsAdded = 0;
                    }
                    else
                    {
                        psReArmPad->currentPtsAdded = psSaveStructure->dummy2;
                    }
				}
				else
				{
                    psReArmPad = ((REARM_PAD *)psStructure->pFunctionality);
					psReArmPad->timeStarted = 0;
				}
				break;
		}
		//get the base body points
		psStructure->body = (UWORD)structureBody(psStructure);
		if (psSaveStructure->body < psStructure->body)
		{
			psStructure->body = (UWORD)psSaveStructure->body;
		}
		//set the build status from the build points
		psStructure->currentPowerAccrued = psSaveStructure->currentPowerAccrued;//22feb
		psStructure->currentBuildPts = (SWORD)psStructure->pStructureType->buildPoints;
		if (psSaveStructure->currentBuildPts < psStructure->currentBuildPts)
		{
			psStructure->currentBuildPts = (SWORD)psSaveStructure->currentBuildPts;
            psStructure->status = SS_BEING_BUILT;
		}
		else
		{
            psStructure->status = SS_BUILT;
//            buildingComplete(psStructure);//replaced by following switch
			switch (psStructure->pStructureType->type)
			{
				case REF_POWER_GEN:
					checkForResExtractors(psStructure);
					if(selectedPlayer == psStructure->player)
					{
						audio_PlayObjStaticTrack( (void *) psStructure, ID_SOUND_POWER_HUM );
					}
					break;
				case REF_RESOURCE_EXTRACTOR:
                    //only try and connect if power left in
                    if (((RES_EXTRACTOR *)psStructure->pFunctionality)->power != 0)
                    {
    					checkForPowerGen(psStructure);
	    				/* GJ HACK! - add anim to deriks */
		    			if (psStructure->psCurAnim == NULL)
			    		{
				    		psStructure->psCurAnim = animObj_Add(psStructure, ID_ANIM_DERIK, 0, 0);
					    }
                    }

					break;
				case REF_RESEARCH:
//21feb					intCheckResearchButton();
					break;
				default:
					//do nothing for factories etc
					break;
			}
		}
	}

	if (NumberOfSkippedStructures>0)
	{
		debug( LOG_ERROR, "structureLoad: invalid player number in %d structures ... assigned to the last player!\n\n", NumberOfSkippedStructures );
		abort();
	}

	return TRUE;
}

// -----------------------------------------------------------------------------------------
#ifdef ALLOWSAVE
/*
Writes the linked list of structure for each player to a file
*/
BOOL writeStructFile(char *pFileName)
{
	char *pFileData = NULL;
	UDWORD				fileSize, player, i, totalStructs=0;
	STRUCTURE			*psCurr;
	DROID_TEMPLATE		*psSubjectTemplate;
	FACTORY				*psFactory;
	REPAIR_FACILITY		*psRepair;
	REARM_PAD				*psReArmPad;
	STRUCT_SAVEHEADER *psHeader;
	SAVE_STRUCTURE		*psSaveStruct;
	FLAG_POSITION		*psFlag;
	UDWORD				researchId;
	BOOL status = TRUE;

	//total all the structures in the world
	for (player = 0; player < MAX_PLAYERS; player++)
	{
		for (psCurr = apsStructLists[player]; psCurr != NULL; psCurr = psCurr->psNext)
		{
			totalStructs++;
		}
	}

	/* Allocate the data buffer */
	fileSize = STRUCT_HEADER_SIZE + totalStructs*sizeof(SAVE_STRUCTURE);
	pFileData = (char*)MALLOC(fileSize);
	if (pFileData == NULL)
	{
		debug( LOG_ERROR, "Out of memory" );
		abort();
		return FALSE;
	}

	/* Put the file header on the file */
	psHeader = (STRUCT_SAVEHEADER *)pFileData;
	psHeader->aFileType[0] = 's';
	psHeader->aFileType[1] = 't';
	psHeader->aFileType[2] = 'r';
	psHeader->aFileType[3] = 'u';
	psHeader->version = CURRENT_VERSION_NUM;
	psHeader->quantity = totalStructs;

	endian_udword(&psHeader->version);
	endian_udword(&psHeader->quantity);

	psSaveStruct = (SAVE_STRUCTURE*)(pFileData + STRUCT_HEADER_SIZE);

	/* Put the structure data into the buffer */
	for (player = 0; player < MAX_PLAYERS; player++)
	{
		for(psCurr = apsStructLists[player]; psCurr != NULL; psCurr = psCurr->psNext)
		{

			strcpy(psSaveStruct->name, psCurr->pStructureType->pName);

			psSaveStruct->id = psCurr->id;


			psSaveStruct->x = psCurr->x;
			psSaveStruct->y = psCurr->y;
			psSaveStruct->z = psCurr->z;

			psSaveStruct->direction = psCurr->direction;
			psSaveStruct->player = psCurr->player;
			psSaveStruct->inFire = psCurr->inFire;
			psSaveStruct->burnStart = psCurr->burnStart;
			psSaveStruct->burnDamage = psCurr->burnDamage;
			//version 14
			for (i=0; i < MAX_PLAYERS; i++)
			{
				psSaveStruct->visible[i] = psCurr->visible[i];
			}
			//psSaveStruct->structureInc = psCurr->pStructureType - asStructureStats;
			psSaveStruct->status = psCurr->status;
			//check if body at max
			if (psCurr->body >= structureBody(psCurr))
			{
				psSaveStruct->body = MAX_BODY;
			}
			else
			{
				psSaveStruct->body = psCurr->body;
			}
			//check if buildpts at max
			if (psCurr->currentBuildPts >= (SWORD)psCurr->pStructureType->buildPoints)
			{
				psSaveStruct->currentBuildPts = MAX_BODY;
			}
			else
			{
				psSaveStruct->currentBuildPts = psCurr->currentBuildPts;
			}
			// no need to check power at max because it would be being built
			psSaveStruct->currentPowerAccrued = psCurr->currentPowerAccrued;

			psSaveStruct->armour = psCurr->armour;
			psSaveStruct->resistance = psCurr->resistance;
			psSaveStruct->subjectInc = UDWORD_MAX;
			psSaveStruct->timeStarted = 0;
			psSaveStruct->output = 0;
			psSaveStruct->capacity = 0;
			psSaveStruct->quantity = 0;
			if (psCurr->pFunctionality)
			{
				switch (psCurr->pStructureType->type)
				{
				case REF_FACTORY:
				case REF_CYBORG_FACTORY:
				case REF_VTOL_FACTORY:
					psFactory = ((FACTORY *)psCurr->pFunctionality);
					psSaveStruct->capacity	= psFactory->capacity;
                    //don't need to save this - it gets set up
					//psSaveStruct->output			= psFactory->productionOutput;
					psSaveStruct->quantity			= psFactory->quantity;
					psSaveStruct->droidTimeStarted	= psFactory->timeStarted;
					psSaveStruct->powerAccrued		= psFactory->powerAccrued;
					psSaveStruct->timeToBuild		= psFactory->timeToBuild;
					psSaveStruct->timeStartHold		= psFactory->timeStartHold;

    				if (psFactory->psSubject != NULL)
					{
						psSubjectTemplate = (DROID_TEMPLATE *)psFactory->psSubject;
						psSaveStruct->subjectInc = psSubjectTemplate->multiPlayerID;
					}
					else
					{
						psSaveStruct->subjectInc = UDWORD_MAX;
					}

					psFlag = ((FACTORY *)psCurr->pFunctionality)->psAssemblyPoint;
					if (psFlag != NULL)
					{
						psSaveStruct->factoryInc = psFlag->factoryInc;
					}
					else
					{
						psSaveStruct->factoryInc = UDWORD_MAX;
					}
					//version 21
					if (psFactory->psCommander)
					{
						psSaveStruct->commandId = psFactory->psCommander->id;
					}
					else
					{
						psSaveStruct->commandId = UDWORD_MAX;
					}
                    //secondary order added - AB 22/04/99
                    psSaveStruct->dummy2 = psFactory->secondaryOrder;

					break;
				case REF_RESEARCH:
					psSaveStruct->capacity = ((RESEARCH_FACILITY *)psCurr->
						pFunctionality)->capacity;
                    //don't need to save this - it gets set up
					//psSaveStruct->output = ((RESEARCH_FACILITY *)psCurr->
					//	pFunctionality)->researchPoints;
					psSaveStruct->powerAccrued = ((RESEARCH_FACILITY *)psCurr->
						pFunctionality)->powerAccrued;
					psSaveStruct->timeStartHold	= ((RESEARCH_FACILITY *)psCurr->
						pFunctionality)->timeStartHold;
    				if (((RESEARCH_FACILITY *)psCurr->pFunctionality)->psSubject)
					{
						psSaveStruct->subjectInc = 0;
						researchId = ((RESEARCH_FACILITY *)psCurr->pFunctionality)->
							psSubject->ref - REF_RESEARCH_START;
						ASSERT( strlen(asResearch[researchId].pName)<MAX_NAME_SIZE,"writeStructData: research name too long" );
						strcpy(psSaveStruct->researchName, asResearch[researchId].pName);
						psSaveStruct->timeStarted = ((RESEARCH_FACILITY *)psCurr->
							pFunctionality)->timeStarted;
					}
					else
					{
						psSaveStruct->subjectInc = UDWORD_MAX;
						psSaveStruct->researchName[0] = 0;
						psSaveStruct->timeStarted = 0;
					}
					psSaveStruct->timeToBuild = 0;
					break;
				case REF_POWER_GEN:
					psSaveStruct->capacity = ((POWER_GEN *)psCurr->
						pFunctionality)->capacity;
					break;
				case REF_RESOURCE_EXTRACTOR:
					psSaveStruct->output = ((RES_EXTRACTOR *)psCurr->
						pFunctionality)->power;
					break;
				case REF_REPAIR_FACILITY: //CODE THIS SOMETIME
					psRepair = ((REPAIR_FACILITY *)psCurr->pFunctionality);
					psSaveStruct->droidTimeStarted = psRepair->timeStarted;
					psSaveStruct->powerAccrued = psRepair->powerAccrued;
                    psSaveStruct->dummy2 = psRepair->currentPtsAdded;

					if (psRepair->psObj != NULL)
					{
						psSaveStruct->subjectInc = psRepair->psObj->id;
					}
					else
					{
						psSaveStruct->subjectInc = UDWORD_MAX;
					}

					psFlag = psRepair->psDeliveryPoint;
					if (psFlag != NULL)
					{
						psSaveStruct->factoryInc = psFlag->factoryInc;
					}
					else
					{
						psSaveStruct->factoryInc = UDWORD_MAX;
					}
					break;
				case REF_REARM_PAD:
					psReArmPad = ((REARM_PAD *)psCurr->pFunctionality);
					psSaveStruct->output = psReArmPad->reArmPoints;
					psSaveStruct->droidTimeStarted = psReArmPad->timeStarted;
                    psSaveStruct->dummy2 = psReArmPad->currentPtsAdded;
					if (psReArmPad->psObj != NULL)
					{
						psSaveStruct->subjectInc = psReArmPad->psObj->id;
					}
					else
					{
						psSaveStruct->subjectInc = UDWORD_MAX;
					}
					break;
				default: //CODE THIS SOMETIME
					ASSERT( FALSE,"Structure facility not saved" );
					break;
				}
			}

			/* SAVE_STRUCTURE is STRUCTURE_SAVE_V21 */
			/* STRUCTURE_SAVE_V21 includes STRUCTURE_SAVE_V20 */
			endian_udword(&psSaveStruct->commandId);
			/* STRUCTURE_SAVE_V20 includes OBJECT_SAVE_V20 */
			endian_sdword(&psSaveStruct->currentBuildPts);
			endian_udword(&psSaveStruct->body);
			endian_udword(&psSaveStruct->armour);
			endian_udword(&psSaveStruct->resistance);
			endian_udword(&psSaveStruct->dummy1);
			endian_udword(&psSaveStruct->subjectInc);
			endian_udword(&psSaveStruct->timeStarted);
			endian_udword(&psSaveStruct->output);
			endian_udword(&psSaveStruct->capacity);
			endian_udword(&psSaveStruct->quantity);
			endian_udword(&psSaveStruct->factoryInc);
			endian_udword(&psSaveStruct->powerAccrued);
			endian_udword(&psSaveStruct->dummy2);
			endian_udword(&psSaveStruct->droidTimeStarted);
			endian_udword(&psSaveStruct->timeToBuild);
			endian_udword(&psSaveStruct->timeStartHold);
			endian_sword(&psSaveStruct->currentPowerAccrued);
			/* OBJECT_SAVE_V20 */
			endian_udword(&psSaveStruct->id);
			endian_udword(&psSaveStruct->x);
			endian_udword(&psSaveStruct->y);
			endian_udword(&psSaveStruct->z);
			endian_udword(&psSaveStruct->direction);
			endian_udword(&psSaveStruct->player);
			endian_udword(&psSaveStruct->burnStart);
			endian_udword(&psSaveStruct->burnDamage);

			psSaveStruct = (SAVE_STRUCTURE *)((char *)psSaveStruct + sizeof(SAVE_STRUCTURE));
		}
	}

	/* Write the data to the file */
	if (pFileData != NULL) {
		status = saveFile(pFileName, pFileData, fileSize);
		FREE(pFileData);
		return status;
	}
	return FALSE;
}

// -----------------------------------------------------------------------------------------
BOOL loadStructSetPointers(void)
{
	UDWORD		player,list;
	FACTORY		*psFactory;
	REPAIR_FACILITY	*psRepair;
	REARM_PAD	*psReArmPad;
	STRUCTURE	*psStruct;
	DROID		*psCommander;
	STRUCTURE	**ppsStructLists[2];

	ppsStructLists[0] = apsStructLists;
	ppsStructLists[1] = mission.apsStructLists;

	for(list = 0; list<2; list++)
	{
		for(player = 0; player<MAX_PLAYERS; player++)
		{
			psStruct=(STRUCTURE *)ppsStructLists[list][player];
			while (psStruct)
			{
				if (psStruct->pFunctionality)
				{
					switch (psStruct->pStructureType->type)
					{
					case REF_FACTORY:
					case REF_CYBORG_FACTORY:
					case REF_VTOL_FACTORY:
						psFactory = ((FACTORY *)psStruct->pFunctionality);
						//there is a commander then has been temporarily removed
						//so put it back
						if ((UDWORD)psFactory->psCommander != UDWORD_MAX)
						{
							psCommander = (DROID*)getBaseObjFromId((UDWORD)psFactory->psCommander);
							psFactory->psCommander = NULL;
							ASSERT( psCommander != NULL,"loadStructSetPointers psCommander getBaseObjFromId() failed" );
							if (psCommander == NULL)
							{
								psFactory->psCommander = NULL;
							}
							else
							{
                                if (list == 1) //ie offWorld
                                {
                                    //don't need to worry about the Flag
                                    ((FACTORY *)psStruct->pFunctionality)->psCommander =
                                        psCommander;
                                }
                                else
                                {
								    assignFactoryCommandDroid(psStruct, psCommander);
                                }
							}
						}
						else
						{
							psFactory->psCommander = NULL;
						}
						break;
					case REF_REPAIR_FACILITY:
						psRepair = ((REPAIR_FACILITY *)psStruct->pFunctionality);
						if ((UDWORD)psRepair->psObj == UDWORD_MAX)
						{
							psRepair->psObj = NULL;
						}
						else
						{
							psRepair->psObj = getBaseObjFromId((UDWORD)psRepair->psObj);
							//if the build has started set the powerAccrued =
							//powerRequired to sync the interface
							if (psRepair->timeStarted != ACTION_START_TIME &&
								psRepair->psObj)
							{
								psRepair->powerAccrued = powerReqForDroidRepair((DROID*)psRepair->psObj);
							}
						}
						break;
					case REF_REARM_PAD:
						psReArmPad = ((REARM_PAD *)psStruct->pFunctionality);
						if (saveGameVersion >= VERSION_26)//version 26
						{
							if ((UDWORD)psReArmPad->psObj == UDWORD_MAX)
							{
								psReArmPad->psObj = NULL;
							}
							else
							{
								psReArmPad->psObj = getBaseObjFromId((UDWORD)psReArmPad->psObj);
							}
						}
						else
						{
							psReArmPad->psObj = NULL;
						}
					default:
						break;
					}
				}
				psStruct = psStruct->psNext;
			}
		}
	}
	return TRUE;
}

#endif

// -----------------------------------------------------------------------------------------
BOOL loadSaveFeature(char *pFileData, UDWORD filesize)
{
	FEATURE_SAVEHEADER		*psHeader;

	/* Check the file type */
	psHeader = (FEATURE_SAVEHEADER *)pFileData;
	if (psHeader->aFileType[0] != 'f' || psHeader->aFileType[1] != 'e' ||
		psHeader->aFileType[2] != 'a' || psHeader->aFileType[3] != 't')
	{
		debug( LOG_ERROR, "loadSaveFeature: Incorrect file type" );
		abort();
		return FALSE;
	}

	/* FEATURE_SAVEHEADER */
	endian_udword(&psHeader->version);
	endian_udword(&psHeader->quantity);

	//increment to the start of the data
	pFileData += FEATURE_HEADER_SIZE;

	/* Check the file version */
	if (psHeader->version < VERSION_7)
	{
		debug( LOG_ERROR, "FeatLoad: unsupported save format version %d", psHeader->version );
		abort();
		return FALSE;
	}
	else if (psHeader->version <= VERSION_19)
	{
		if (!loadSaveFeatureV14(pFileData, filesize, psHeader->quantity, psHeader->version))
		{
			return FALSE;
		}
	}
	else if (psHeader->version <= CURRENT_VERSION_NUM)
	{
		if (!loadSaveFeatureV(pFileData, filesize, psHeader->quantity, psHeader->version))
		{
			return FALSE;
		}
	}
	else
	{
		debug( LOG_ERROR, "FeatLoad: undefined save format version %d", psHeader->version );
		abort();
		return FALSE;
	}

	return TRUE;
}


// -----------------------------------------------------------------------------------------
/* code for all version 8 - 14 save features */
BOOL loadSaveFeatureV14(char *pFileData, UDWORD filesize, UDWORD numFeatures, UDWORD version)
{
	SAVE_FEATURE_V14			*psSaveFeature;
	FEATURE					*pFeature;
	UDWORD					count, i, statInc;
	FEATURE_STATS			*psStats = NULL;
	BOOL					found;
	UDWORD					sizeOfSaveFeature;

	if (version < VERSION_14)
	{
		sizeOfSaveFeature = sizeof(SAVE_FEATURE_V2);
	}
	else
	{
		sizeOfSaveFeature = sizeof(SAVE_FEATURE_V14);
	}


	if ((sizeOfSaveFeature * numFeatures + FEATURE_HEADER_SIZE) >
		filesize)
	{
		debug( LOG_ERROR, "featureLoad: unexpected end of file" );
		abort();
		return FALSE;
	}

	/* Load in the feature data */
	for (count = 0; count < numFeatures; count ++,
									pFileData += sizeOfSaveFeature)
	{
		psSaveFeature = (SAVE_FEATURE_V14*) pFileData;

		/* FEATURE_SAVE_V14 is FEATURE_SAVE_V2 */
		/* FEATURE_SAVE_V2 is OBJECT_SAVE_V19 */
		/* OBJECT_SAVE_V19 */
		endian_udword(&psSaveFeature->id);
		endian_udword(&psSaveFeature->x);
		endian_udword(&psSaveFeature->y);
		endian_udword(&psSaveFeature->z);
		endian_udword(&psSaveFeature->direction);
		endian_udword(&psSaveFeature->player);
		endian_udword(&psSaveFeature->burnStart);
		endian_udword(&psSaveFeature->burnDamage);

		/*if (psSaveFeature->featureInc > numFeatureStats)
		{
			DBERROR(("Invalid Feature Type - unable to load save game"));
			goto error;
		}*/
		//get the stats for this feature
		found = FALSE;


		if (!getSaveObjectName(psSaveFeature->name))
		{
			continue;
		}

		for (statInc = 0; statInc < numFeatureStats; statInc++)
		{
			psStats = asFeatureStats + statInc;
			//loop until find the same name

			if (!strcmp(psStats->pName, psSaveFeature->name))

			{
				found = TRUE;
				break;
			}
		}
		//if haven't found the feature - ignore this record!
		if (!found)
		{

			debug( LOG_ERROR, "This feature no longer exists - %s", psSaveFeature->name );
			abort();
			continue;
		}
		//create the Feature
		//buildFeature(asFeatureStats + psSaveFeature->featureInc,
		//	psSaveFeature->x, psSaveFeature->y);
		pFeature = buildFeature(psStats, psSaveFeature->x, psSaveFeature->y,TRUE);
		//will be added to the top of the linked list
		//pFeature = apsFeatureLists[0];
		if (!pFeature)
		{
			ASSERT( FALSE, "loadSaveFeature:Unable to create feature" );
			return FALSE;
		}
//DBPRINTF(("Loaded feature - id = %d @ %p\n",psSaveFeature->id,pFeature);
		//restore values
		pFeature->id = psSaveFeature->id;
		pFeature->direction = (UWORD)psSaveFeature->direction;
		pFeature->inFire = psSaveFeature->inFire;
		pFeature->burnDamage = psSaveFeature->burnDamage;
		if (version >= VERSION_14)
		{
			for (i=0; i < MAX_PLAYERS; i++)
			{
				pFeature->visible[i] = psSaveFeature->visible[i];
                //set the Tile flag if visible for the selectedPlayer
                if ((i == selectedPlayer) && (pFeature->visible[i] == UBYTE_MAX))
                {
                    setFeatTileDraw(pFeature);
                }
			}
		}

	}

	return TRUE;
}

// -----------------------------------------------------------------------------------------
/* code for all post version 7 save features */
BOOL loadSaveFeatureV(char *pFileData, UDWORD filesize, UDWORD numFeatures, UDWORD version)
{
	SAVE_FEATURE			*psSaveFeature;
	FEATURE					*pFeature;
	UDWORD					count, i, statInc;
	FEATURE_STATS			*psStats = NULL;
	BOOL					found;
	UDWORD					sizeOfSaveFeature;

//	version;

	sizeOfSaveFeature = sizeof(SAVE_FEATURE);

	if ((sizeOfSaveFeature * numFeatures + FEATURE_HEADER_SIZE) >
		filesize)
	{
		debug( LOG_ERROR, "featureLoad: unexpected end of file" );
		abort();
		return FALSE;
	}

	/* Load in the feature data */
	for (count = 0; count < numFeatures; count ++,
									pFileData += sizeOfSaveFeature)
	{
		psSaveFeature = (SAVE_FEATURE*) pFileData;

		/* FEATURE_SAVE is FEATURE_SAVE_V20 */
		/* FEATURE_SAVE_V20 is OBJECT_SAVE_V20 */
		/* OBJECT_SAVE_V20 */
		endian_udword(&psSaveFeature->id);
		endian_udword(&psSaveFeature->x);
		endian_udword(&psSaveFeature->y);
		endian_udword(&psSaveFeature->z);
		endian_udword(&psSaveFeature->direction);
		endian_udword(&psSaveFeature->player);
		endian_udword(&psSaveFeature->burnStart);
		endian_udword(&psSaveFeature->burnDamage);

		/*if (psSaveFeature->featureInc > numFeatureStats)
		{
			DBERROR(("Invalid Feature Type - unable to load save game"));
			goto error;
		}*/
		//get the stats for this feature
		found = FALSE;


		if (!getSaveObjectName(psSaveFeature->name))
		{
			continue;
		}

		for (statInc = 0; statInc < numFeatureStats; statInc++)
		{
			psStats = asFeatureStats + statInc;
			//loop until find the same name

			if (!strcmp(psStats->pName, psSaveFeature->name))
			{
				found = TRUE;
				break;
			}
		}
		//if haven't found the feature - ignore this record!
		if (!found)
		{

			debug( LOG_ERROR, "This feature no longer exists - %s", psSaveFeature->name );
			abort();

			continue;
		}
		//create the Feature
		//buildFeature(asFeatureStats + psSaveFeature->featureInc,
		//	psSaveFeature->x, psSaveFeature->y);
		pFeature = buildFeature(psStats, psSaveFeature->x, psSaveFeature->y,TRUE);
		//will be added to the top of the linked list
		//pFeature = apsFeatureLists[0];
		if (!pFeature)
		{
			ASSERT( FALSE, "loadSaveFeature:Unable to create feature" );
			return FALSE;
		}
//DBPRINTF(("Loaded feature - id = %d @ %p\n",psSaveFeature->id,pFeature);
		//restore values
		pFeature->id = psSaveFeature->id;
		pFeature->direction = (UWORD)psSaveFeature->direction;
		pFeature->inFire = psSaveFeature->inFire;
		pFeature->burnDamage = psSaveFeature->burnDamage;
		for (i=0; i < MAX_PLAYERS; i++)
		{
			pFeature->visible[i] = psSaveFeature->visible[i]	;
            //set the Tile flag if visible for the selectedPlayer
            if ((i == selectedPlayer) && (pFeature->visible[i] == UBYTE_MAX))
            {
                setFeatTileDraw(pFeature);
            }
		}
	}
	return TRUE;
}

// -----------------------------------------------------------------------------------------
/*
Writes the linked list of features to a file
*/
BOOL writeFeatureFile(char *pFileName)
{
	char *pFileData = NULL;
	UDWORD				fileSize, i, totalFeatures=0;
	FEATURE				*psCurr;
	FEATURE_SAVEHEADER	*psHeader;
	SAVE_FEATURE		*psSaveFeature;
	BOOL status = TRUE;

	//total all the features in the world
	for (psCurr = apsFeatureLists[0]; psCurr != NULL; psCurr = psCurr->psNext)
	{
		totalFeatures++;
	}

	/* Allocate the data buffer */
	fileSize = FEATURE_HEADER_SIZE + totalFeatures * sizeof(SAVE_FEATURE);
	pFileData = (char*)MALLOC(fileSize);
	if (pFileData == NULL)
	{
		debug( LOG_ERROR, "Out of memory" );
		abort();
		return FALSE;
	}

	/* Put the file header on the file */
	psHeader = (FEATURE_SAVEHEADER *)pFileData;
	psHeader->aFileType[0] = 'f';
	psHeader->aFileType[1] = 'e';
	psHeader->aFileType[2] = 'a';
	psHeader->aFileType[3] = 't';
	psHeader->version = CURRENT_VERSION_NUM;
	psHeader->quantity = totalFeatures;

	psSaveFeature = (SAVE_FEATURE*)(pFileData + FEATURE_HEADER_SIZE);

	/* Put the feature data into the buffer */
	for(psCurr = apsFeatureLists[0]; psCurr != NULL; psCurr = psCurr->psNext)
	{

		strcpy(psSaveFeature->name, psCurr->psStats->pName);

		psSaveFeature->id = psCurr->id;

//		psSaveFeature->x = psCurr->x - psCurr->psStats->baseWidth * TILE_UNITS / 2;
//		psSaveFeature->y = psCurr->y - psCurr->psStats->baseBreadth * TILE_UNITS / 2;
//		psSaveFeature->z = psCurr->z;

		psSaveFeature->x = psCurr->x;
		psSaveFeature->y = psCurr->y;
		psSaveFeature->z = psCurr->z;

		psSaveFeature->direction = psCurr->direction;
		psSaveFeature->inFire = psCurr->inFire;
		psSaveFeature->burnDamage = psCurr->burnDamage;
		for (i=0; i < MAX_PLAYERS; i++)
		{
			psSaveFeature->visible[i] = psCurr->visible[i];
		}

//		psSaveFeature->featureInc = psCurr->psStats - asFeatureStats;

		/* SAVE_FEATURE is FEATURE_SAVE_V20 */
		/* FEATURE_SAVE_V20 includes OBJECT_SAVE_V20 */
		/* OBJECT_SAVE_V20 */
		endian_udword(&psSaveFeature->id);
		endian_udword(&psSaveFeature->x);
		endian_udword(&psSaveFeature->y);
		endian_udword(&psSaveFeature->z);
		endian_udword(&psSaveFeature->direction);
		endian_udword(&psSaveFeature->player);
		endian_udword(&psSaveFeature->burnStart);
		endian_udword(&psSaveFeature->burnDamage);

		psSaveFeature = (SAVE_FEATURE *)((char *)psSaveFeature + sizeof(SAVE_FEATURE));
	}

	/* FEATURE_SAVEHEADER */
	endian_udword(&psHeader->version);
	endian_udword(&psHeader->quantity);

	/* Write the data to the file */
	if (pFileData != NULL) {
		status = saveFile(pFileName, pFileData, fileSize);
		FREE(pFileData);
		return status;
	}
	return FALSE;
}


// -----------------------------------------------------------------------------------------
BOOL loadSaveTemplate(char *pFileData, UDWORD filesize)
{
	TEMPLATE_SAVEHEADER		*psHeader;

	/* Check the file type */
	psHeader = (TEMPLATE_SAVEHEADER *)pFileData;
	if (psHeader->aFileType[0] != 't' || psHeader->aFileType[1] != 'e' ||
		psHeader->aFileType[2] != 'm' || psHeader->aFileType[3] != 'p')
	{
		debug( LOG_ERROR, "loadSaveTemplate: Incorrect file type" );
		abort();
		return FALSE;
	}

	/* TEMPLATE_SAVEHEADER */
	endian_udword(&psHeader->version);
	endian_udword(&psHeader->quantity);

	//increment to the start of the data
	pFileData += TEMPLATE_HEADER_SIZE;

	/* Check the file version */
	if (psHeader->version < VERSION_7)
	{
		debug( LOG_ERROR, "TemplateLoad: unsupported save format version %d", psHeader->version );
		abort();
		return FALSE;
	}
	else if (psHeader->version < VERSION_14)
	{
		if (!loadSaveTemplateV7(pFileData, filesize, psHeader->quantity))
		{
			return FALSE;
		}
	}
	else if (psHeader->version <= VERSION_19)
	{
		if (!loadSaveTemplateV14(pFileData, filesize, psHeader->quantity))
		{
			return FALSE;
		}
	}
	else if (psHeader->version <= CURRENT_VERSION_NUM)
	{
		if (!loadSaveTemplateV(pFileData, filesize, psHeader->quantity))
		{
			return FALSE;
		}
	}
	else
	{
		debug( LOG_ERROR, "TemplateLoad: undefined save format version %d", psHeader->version );
		abort();
		return FALSE;
	}
	return TRUE;
}

// -----------------------------------------------------------------------------------------
/* code specific to version 7 of a save template */
BOOL loadSaveTemplateV7(char *pFileData, UDWORD filesize, UDWORD numTemplates)
{
	SAVE_TEMPLATE_V2		*psSaveTemplate, sSaveTemplate;
	DROID_TEMPLATE			*psTemplate;
	UDWORD					count, i;
	SDWORD					compInc;
	BOOL					found;

	psSaveTemplate = &sSaveTemplate;

	if ((sizeof(SAVE_TEMPLATE_V2) * numTemplates + TEMPLATE_HEADER_SIZE) > filesize)
	{
		debug( LOG_ERROR, "templateLoad: unexpected end of file" );
		abort();
		return FALSE;
	}

	/* Load in the template data */
	for (count = 0; count < numTemplates; count ++, pFileData += sizeof(SAVE_TEMPLATE_V2))
	{
		memcpy(psSaveTemplate, pFileData, sizeof(SAVE_TEMPLATE_V2));

		/* SAVE_TEMPLATE_V2 is TEMPLATE_SAVE_V2 */
		/* TEMPLATE_SAVE_V2 */
		endian_udword(&psSaveTemplate->ref);
		endian_udword(&psSaveTemplate->player);
		endian_udword(&psSaveTemplate->numWeaps);
		endian_udword(&psSaveTemplate->numProgs);

		if (psSaveTemplate->player != 0)
		{
			// only load player 0 templates - the rest come from stats
			continue;
		}

		//create the Template
		if (!HEAP_ALLOC(psTemplateHeap, (void**) &psTemplate))
		{
			debug( LOG_ERROR, "Out of memory" );
			abort();
			goto error;
		}
		//copy the values across

		psTemplate->pName = NULL;
		strncpy(psTemplate->aName, psSaveTemplate->name, DROID_MAXNAME);
		psTemplate->aName[DROID_MAXNAME-1]=0;

		psTemplate->ref = psSaveTemplate->ref;
		psTemplate->droidType = psSaveTemplate->droidType;
		found = TRUE;
		//for (i=0; i < DROID_MAXCOMP; i++) - not intestested in the first comp - COMP_UNKNOWN
		for (i=1; i < DROID_MAXCOMP; i++)
		{
			//DROID_MAXCOMP has changed to remove COMP_PROGRAM so hack here to load old save games!
			if (i == SAVE_COMP_PROGRAM)
			{
				break;
			}

			compInc = getCompFromName(i, psSaveTemplate->asParts[i]);

			if (compInc < 0)
			{

				debug( LOG_ERROR, "This component no longer exists - %s, the template will be deleted", psSaveTemplate->asParts[i] );
				abort();

				found = FALSE;
				//continue;
				break;
			}
			psTemplate->asParts[i] = (UDWORD)compInc;
		}
		if (!found)
		{
			//ignore this record
			HEAP_FREE(psTemplateHeap, psTemplate);
			continue;
		}
		psTemplate->numWeaps = psSaveTemplate->numWeaps;
		found = TRUE;
		for (i=0; i < psTemplate->numWeaps; i++)
		{

			compInc = getCompFromName(COMP_WEAPON, psSaveTemplate->asWeaps[i]);

			if (compInc < 0)
			{

				debug( LOG_ERROR, "This weapon no longer exists - %s, the template will be deleted", psSaveTemplate->asWeaps[i] );
				abort();

				found = FALSE;
				//continue;
				break;
			}
			psTemplate->asWeaps[i] = (UDWORD)compInc;
		}
		if (!found)
		{
			//ignore this record
			HEAP_FREE(psTemplateHeap, psTemplate);
			continue;
		}

		// ignore brains and programs for now
		psTemplate->asParts[COMP_BRAIN] = 0;


		//calculate the total build points
		psTemplate->buildPoints = calcTemplateBuild(psTemplate);
		psTemplate->powerPoints = calcTemplatePower(psTemplate);

		//store it in the apropriate player' list
		psTemplate->psNext = apsDroidTemplates[psSaveTemplate->player];
		apsDroidTemplates[psSaveTemplate->player] = psTemplate;



	}

	return TRUE;

error:
	droidTemplateShutDown();
	return FALSE;
}

// -----------------------------------------------------------------------------------------
/* none specific version of a save template */
BOOL loadSaveTemplateV14(char *pFileData, UDWORD filesize, UDWORD numTemplates)
{
	SAVE_TEMPLATE_V14			*psSaveTemplate, sSaveTemplate;
	DROID_TEMPLATE			*psTemplate, *psDestTemplate;
	UDWORD					count, i;
	SDWORD					compInc;
	BOOL					found;

	psSaveTemplate = &sSaveTemplate;

	if ((sizeof(SAVE_TEMPLATE_V14) * numTemplates + TEMPLATE_HEADER_SIZE) > filesize)
	{
		debug( LOG_ERROR, "templateLoad: unexpected end of file" );
		abort();
		return FALSE;
	}

	/* Load in the template data */
	for (count = 0; count < numTemplates; count ++, pFileData += sizeof(SAVE_TEMPLATE_V14))
	{
		memcpy(psSaveTemplate, pFileData, sizeof(SAVE_TEMPLATE_V14));

		/* SAVE_TEMPLATE_V14 is TEMPLATE_SAVE_V14 */
		endian_udword(&psSaveTemplate->ref);
		endian_udword(&psSaveTemplate->player);
		endian_udword(&psSaveTemplate->numWeaps);
		endian_udword(&psSaveTemplate->multiPlayerID);

		//AT SOME POINT CHECK THE multiPlayerID TO SEE IF ALREADY EXISTS - IGNORE IF IT DOES

		if (psSaveTemplate->player != 0)
		{
			// only load player 0 templates - the rest come from stats
			continue;
		}

		//create the Template
		if (!HEAP_ALLOC(psTemplateHeap, (void**) &psTemplate))
		{
			debug( LOG_ERROR, "Out of memory" );
			abort();
			goto error;
		}
		//copy the values across

		psTemplate->pName = NULL;
		strncpy(psTemplate->aName, psSaveTemplate->name, DROID_MAXNAME);
		psTemplate->aName[DROID_MAXNAME-1]=0;


		psTemplate->ref = psSaveTemplate->ref;
		psTemplate->droidType = psSaveTemplate->droidType;
		psTemplate->multiPlayerID = psSaveTemplate->multiPlayerID;
		found = TRUE;
		//for (i=0; i < DROID_MAXCOMP; i++) - not intestested in the first comp - COMP_UNKNOWN
		for (i=1; i < DROID_MAXCOMP; i++)
		{
			//DROID_MAXCOMP has changed to remove COMP_PROGRAM so hack here to load old save games!
			if (i == SAVE_COMP_PROGRAM)
			{
				break;
			}

			compInc = getCompFromName(i, psSaveTemplate->asParts[i]);

			if (compInc < 0)
			{

				debug( LOG_ERROR, "This component no longer exists - %s, the template will be deleted", psSaveTemplate->asParts[i] );
				abort();

				found = FALSE;
				//continue;
				break;
			}
			psTemplate->asParts[i] = (UDWORD)compInc;
		}
		if (!found)
		{
			//ignore this record
			HEAP_FREE(psTemplateHeap, psTemplate);
			continue;
		}
		psTemplate->numWeaps = psSaveTemplate->numWeaps;
		found = TRUE;
		for (i=0; i < psTemplate->numWeaps; i++)
		{

			compInc = getCompFromName(COMP_WEAPON, psSaveTemplate->asWeaps[i]);

			if (compInc < 0)
			{
				debug( LOG_ERROR, "This weapon no longer exists - %s, the template will be deleted", psSaveTemplate->asWeaps[i] );
				abort();

				found = FALSE;
				//continue;
				break;
			}
			psTemplate->asWeaps[i] = (UDWORD)compInc;
		}
		if (!found)
		{
			//ignore this record
			HEAP_FREE(psTemplateHeap, psTemplate);
			continue;
		}

		// ignore brains and programs for now
		psTemplate->asParts[COMP_BRAIN] = 0;

		//calculate the total build points
		psTemplate->buildPoints = calcTemplateBuild(psTemplate);
		psTemplate->powerPoints = calcTemplatePower(psTemplate);

#ifdef NEW_SAVE_V14
//		if (psSaveTemplate->player == 0)
//		{
//			DBPRINTF(("building from save %s %d\n",psTemplate->aName,psTemplate->multiPlayerID));
//		}
#endif

		//store it in the apropriate player' list
		//if a template with the same multiplayerID exists overwrite it
		//else add this template to the top of the list
		psDestTemplate = apsDroidTemplates[psSaveTemplate->player];
		while (psDestTemplate != NULL)
		{
			if (psTemplate->multiPlayerID == psDestTemplate->multiPlayerID)
			{
				//whooh get rid of this one
				break;
			}
			psDestTemplate = psDestTemplate->psNext;
		}

		if (psDestTemplate != NULL)
		{
			psTemplate->psNext = psDestTemplate->psNext;//preserve the list
			memcpy(psDestTemplate,psTemplate,sizeof(DROID_TEMPLATE));
		}
		else
		{
			//add it to the top of the list
			psTemplate->psNext = apsDroidTemplates[psSaveTemplate->player];
			apsDroidTemplates[psSaveTemplate->player] = psTemplate;
		}




	}

	return TRUE;

error:
	droidTemplateShutDown();
	return FALSE;
}

// -----------------------------------------------------------------------------------------
/* none specific version of a save template */
BOOL loadSaveTemplateV(char *pFileData, UDWORD filesize, UDWORD numTemplates)
{
	SAVE_TEMPLATE			*psSaveTemplate, sSaveTemplate;
	DROID_TEMPLATE			*psTemplate, *psDestTemplate;
	UDWORD					count, i;
	SDWORD					compInc;
	BOOL					found;

	psSaveTemplate = &sSaveTemplate;

	if ((sizeof(SAVE_TEMPLATE) * numTemplates + TEMPLATE_HEADER_SIZE) > filesize)
	{
		debug( LOG_ERROR, "templateLoad: unexpected end of file" );
		abort();
		return FALSE;
	}

	/* Load in the template data */
	for (count = 0; count < numTemplates; count ++, pFileData += sizeof(SAVE_TEMPLATE))
	{
		memcpy(psSaveTemplate, pFileData, sizeof(SAVE_TEMPLATE));

		/* SAVE_TEMPLATE is TEMPLATE_SAVE_V20 */
		/* TEMPLATE_SAVE_V20 */
		endian_udword(&psSaveTemplate->ref);
		endian_udword(&psSaveTemplate->player);
		endian_udword(&psSaveTemplate->numWeaps);
		endian_udword(&psSaveTemplate->multiPlayerID);

		//AT SOME POINT CHECK THE multiPlayerID TO SEE IF ALREADY EXISTS - IGNORE IF IT DOES

		if (psSaveTemplate->player != 0 && !bMultiPlayer)
		{
			// only load player 0 templates - the rest come from stats
			continue;
		}

		//create the Template
		if (!HEAP_ALLOC(psTemplateHeap, (void**) &psTemplate))
		{
			debug( LOG_ERROR, "Out of memory" );
			abort();
			goto error;
		}
		//copy the values across

		psTemplate->pName = NULL;
		strncpy(psTemplate->aName, psSaveTemplate->name, DROID_MAXNAME);
		psTemplate->aName[DROID_MAXNAME-1]=0;


		psTemplate->ref = psSaveTemplate->ref;
		psTemplate->droidType = psSaveTemplate->droidType;
		psTemplate->multiPlayerID = psSaveTemplate->multiPlayerID;
		found = TRUE;
		//for (i=0; i < DROID_MAXCOMP; i++) - not intestested in the first comp - COMP_UNKNOWN
		for (i=1; i < DROID_MAXCOMP; i++)
		{
			//DROID_MAXCOMP has changed to remove COMP_PROGRAM so hack here to load old save games!
			if (i == SAVE_COMP_PROGRAM)
			{
				break;
			}

			compInc = getCompFromName(i, psSaveTemplate->asParts[i]);

            //HACK to get the game to load when ECMs, Sensors or RepairUnits have been deleted
            if (compInc < 0 && (i == COMP_ECM || i == COMP_SENSOR || i == COMP_REPAIRUNIT))
            {
                //set the ECM to be the defaultECM ...
                if (i == COMP_ECM)
                {
                    compInc = aDefaultECM[psSaveTemplate->player];
                }
                else if (i == COMP_SENSOR)
                {
                    compInc = aDefaultSensor[psSaveTemplate->player];
                }
                else if (i == COMP_REPAIRUNIT)
                {
                    compInc = aDefaultRepair[psSaveTemplate->player];
                }
            }
			else if (compInc < 0)
			{

				debug( LOG_ERROR, "This component no longer exists - %s, the template will be deleted", psSaveTemplate->asParts[i] );
				abort();

				found = FALSE;
				//continue;
				break;
			}
			psTemplate->asParts[i] = (UDWORD)compInc;
		}
		if (!found)
		{
			//ignore this record
			HEAP_FREE(psTemplateHeap, psTemplate);
			continue;
		}
		psTemplate->numWeaps = psSaveTemplate->numWeaps;
		found = TRUE;
		for (i=0; i < psTemplate->numWeaps; i++)
		{

			compInc = getCompFromName(COMP_WEAPON, psSaveTemplate->asWeaps[i]);

			if (compInc < 0)
			{

				debug( LOG_ERROR, "This weapon no longer exists - %s, the template will be deleted", psSaveTemplate->asWeaps[i] );
				abort();


				found = FALSE;
				//continue;
				break;
			}
			psTemplate->asWeaps[i] = (UDWORD)compInc;
		}
		if (!found)
		{
			//ignore this record
			HEAP_FREE(psTemplateHeap, psTemplate);
			continue;
		}

		//no! put brains back in 10Feb //ignore brains and programs for now
		//psTemplate->asParts[COMP_BRAIN] = 0;

		//calculate the total build points
		psTemplate->buildPoints = calcTemplateBuild(psTemplate);
		psTemplate->powerPoints = calcTemplatePower(psTemplate);

#ifdef NEW_SAVE_V14
//		if (psSaveTemplate->player == 0)
//		{
//			DBPRINTF(("building from save %s %d\n",psTemplate->aName,psTemplate->multiPlayerID));
//		}
#endif

		//store it in the apropriate player' list
		//if a template with the same multiplayerID exists overwrite it
		//else add this template to the top of the list
		psDestTemplate = apsDroidTemplates[psSaveTemplate->player];
		while (psDestTemplate != NULL)
		{
			if (psTemplate->multiPlayerID == psDestTemplate->multiPlayerID)
			{
				//whooh get rid of this one
				break;
			}
			psDestTemplate = psDestTemplate->psNext;
		}

		if (psDestTemplate != NULL)
		{
			psTemplate->psNext = psDestTemplate->psNext;//preserve the list
			memcpy(psDestTemplate,psTemplate,sizeof(DROID_TEMPLATE));
		}
		else
		{
			//add it to the top of the list
			psTemplate->psNext = apsDroidTemplates[psSaveTemplate->player];
			apsDroidTemplates[psSaveTemplate->player] = psTemplate;
		}




	}

	return TRUE;

error:
	droidTemplateShutDown();
	return FALSE;
}

// -----------------------------------------------------------------------------------------
/*
Writes the linked list of templates for each player to a file
*/
BOOL writeTemplateFile(char *pFileName)
{
	char *pFileData = NULL;
	UDWORD				fileSize, player, totalTemplates=0;
	DROID_TEMPLATE		*psCurr;
	TEMPLATE_SAVEHEADER	*psHeader;
	SAVE_TEMPLATE		*psSaveTemplate;
	UDWORD				i;
	BOOL status = TRUE;

	//total all the droids in the world
	for (player = 0; player < MAX_PLAYERS; player++)
	{
		for (psCurr = apsDroidTemplates[player]; psCurr != NULL; psCurr = psCurr->psNext)
		{
			totalTemplates++;
		}
	}

	/* Allocate the data buffer */
	fileSize = TEMPLATE_HEADER_SIZE + totalTemplates*sizeof(SAVE_TEMPLATE);
	pFileData = (char*)MALLOC(fileSize);
	if (pFileData == NULL)
	{
		debug( LOG_ERROR, "Out of memory" );
		abort();
		return FALSE;
	}

	/* Put the file header on the file */
	psHeader = (TEMPLATE_SAVEHEADER *)pFileData;
	psHeader->aFileType[0] = 't';
	psHeader->aFileType[1] = 'e';
	psHeader->aFileType[2] = 'm';
	psHeader->aFileType[3] = 'p';
	psHeader->version = CURRENT_VERSION_NUM;
	psHeader->quantity = totalTemplates;

	psSaveTemplate = (SAVE_TEMPLATE*)(pFileData + TEMPLATE_HEADER_SIZE);

	/* Put the template data into the buffer */
	for (player = 0; player < MAX_PLAYERS; player++)
	{
		for(psCurr = apsDroidTemplates[player]; psCurr != NULL; psCurr = psCurr->psNext)
		{




			//strcpy(psSaveTemplate->name, psCurr->pName);
			strcpy(psSaveTemplate->name, psCurr->aName);

			psSaveTemplate->ref = psCurr->ref;
			psSaveTemplate->player = player;
			psSaveTemplate->droidType = (UBYTE)psCurr->droidType;
			psSaveTemplate->multiPlayerID = psCurr->multiPlayerID;
#ifdef NEW_SAVE_V14
//			if (player == 0)
//			{
//				DBPRINTF(("saving %s %d \n",psCurr->aName,psCurr->multiPlayerID));
//			}
#endif
			//for (i=0; i < DROID_MAXCOMP; i++) not interested in first comp - COMP_UNKNOWN
			for (i=1; i < DROID_MAXCOMP; i++)
			{

				if (!getNameFromComp(i, psSaveTemplate->asParts[i], psCurr->asParts[i]))

				{
					//ignore this record
					break;
					//continue;
				}
			}
			psSaveTemplate->numWeaps = psCurr->numWeaps;
			for (i=0; i < psCurr->numWeaps; i++)
			{

				if (!getNameFromComp(COMP_WEAPON, psSaveTemplate->asWeaps[i], psCurr->asWeaps[i]))

				{
					//ignore this record
					//continue;
					break;
				}
			}

			/* SAVE_TEMPLATE is TEMPLATE_SAVE_V20 */
			/* TEMPLATE_SAVE_V20 */
			endian_udword(&psSaveTemplate->ref);
			endian_udword(&psSaveTemplate->player);
			endian_udword(&psSaveTemplate->numWeaps);
			endian_udword(&psSaveTemplate->multiPlayerID);

			psSaveTemplate = (SAVE_TEMPLATE *)((char *)psSaveTemplate + sizeof(SAVE_TEMPLATE));
		}
	}

	/* TEMPLATE_SAVEHEADER */
	endian_udword(&psHeader->version);
	endian_udword(&psHeader->quantity);

	/* Write the data to the file */
	if (pFileData != NULL) {
		status = saveFile(pFileName, pFileData, fileSize);
		FREE(pFileData);
		return status;
	}
	return FALSE;
}







// -----------------------------------------------------------------------------------------

// load up a terrain tile type map file
BOOL loadTerrainTypeMap(char *pFileData, UDWORD filesize)
{
	TILETYPE_SAVEHEADER	*psHeader;
	UDWORD				i;
	UWORD				*pType;

	if (filesize < TILETYPE_HEADER_SIZE)
	{
		debug( LOG_ERROR, "loadTerrainTypeMap: file too small" );
		abort();
		return FALSE;
	}

	// Check the header
	psHeader = (TILETYPE_SAVEHEADER *)pFileData;
	if (psHeader->aFileType[0] != 't' || psHeader->aFileType[1] != 't' ||
		psHeader->aFileType[2] != 'y' || psHeader->aFileType[3] != 'p')
	{
		debug( LOG_ERROR, "loadTerrainTypeMap: Incorrect file type" );
		abort();
		return FALSE;
	}

	/* TILETYPE_SAVEHEADER */
	endian_udword(&psHeader->version);
	endian_udword(&psHeader->quantity);

/*	Version doesn't matter for now
	if (psHeader->version != VERSION_2)
	{
		DBERROR(("loadTerrainTypeMap: Incorrect file version"));
		return FALSE;
	}*/

	// reset the terrain table
	memset(terrainTypes, 0, sizeof(terrainTypes));

	// Load the terrain type mapping
	pType = (UWORD *)(pFileData + TILETYPE_HEADER_SIZE);
	endian_uword(pType);
	for(i=0; i<psHeader->quantity; i++)
	{
		if (i >= MAX_TILE_TEXTURES)
		{
			debug( LOG_ERROR, "loadTerrainTypeMap: too many types" );
			abort();
			return FALSE;

		}
		if (*pType > TER_MAX)
		{
			debug( LOG_ERROR, "loadTerrainTypeMap: terrain type out of range" );
			abort();
			return FALSE;
		}

		terrainTypes[i] = (UBYTE)*pType;
		pType += 1;
		endian_uword(pType);
	}

	return TRUE;
}

// -----------------------------------------------------------------------------------------
// Write out the terrain type map
static BOOL writeTerrainTypeMapFile(char *pFileName)
{
	TILETYPE_SAVEHEADER		*psHeader;
	char *pFileData;
	UDWORD					fileSize, i;
	UWORD					*pType;

	// Calculate the file size
	fileSize = TILETYPE_HEADER_SIZE + sizeof(UWORD) * MAX_TILE_TEXTURES;
	pFileData = (char*)MALLOC(fileSize);
	if (!pFileData)
	{
		debug( LOG_ERROR, "writeTerrainTypeMapFile: Out of memory" );
		abort();
		return FALSE;
	}

	// Put the file header on the file
	psHeader = (TILETYPE_SAVEHEADER *)pFileData;
	psHeader->aFileType[0] = 't';
	psHeader->aFileType[1] = 't';
	psHeader->aFileType[2] = 'y';
	psHeader->aFileType[3] = 'p';
	psHeader->version = CURRENT_VERSION_NUM;
	psHeader->quantity = MAX_TILE_TEXTURES;

	pType = (UWORD *)(pFileData + TILETYPE_HEADER_SIZE);
	for(i=0; i<MAX_TILE_TEXTURES; i++)
	{
		*pType = terrainTypes[i];
		endian_uword(pType);
		pType += 1;
	}

	/* TILETYPE_SAVEHEADER */
	endian_udword(&psHeader->version);
	endian_udword(&psHeader->quantity);

	if (!saveFile(pFileName, pFileData, fileSize))
	{
		return FALSE;
	}
	FREE(pFileData);

	return TRUE;
}

// -----------------------------------------------------------------------------------------
// load up component list file
BOOL loadSaveCompList(char *pFileData, UDWORD filesize)
{
	COMPLIST_SAVEHEADER		*psHeader;

	/* Check the file type */
	psHeader = (COMPLIST_SAVEHEADER *)pFileData;
	if (psHeader->aFileType[0] != 'c' || psHeader->aFileType[1] != 'm' ||
		psHeader->aFileType[2] != 'p' || psHeader->aFileType[3] != 'l')
	{
		debug( LOG_ERROR, "loadSaveCompList: Incorrect file type" );
		abort();
		return FALSE;
	}

	/* COMPLIST_SAVEHEADER */
	endian_udword(&psHeader->version);
	endian_udword(&psHeader->quantity);

	//increment to the start of the data
	pFileData += COMPLIST_HEADER_SIZE;

	/* Check the file version */
	if (psHeader->version < VERSION_7)
	{
		debug( LOG_ERROR, "CompLoad: unsupported save format version %d", psHeader->version );
		abort();
		return FALSE;
	}
	else if (psHeader->version <= VERSION_19)
	{
		if (!loadSaveCompListV9(pFileData, filesize, psHeader->quantity, psHeader->version))
		{
			return FALSE;
		}
	}
	else if (psHeader->version <= CURRENT_VERSION_NUM)
	{
		if (!loadSaveCompListV(pFileData, filesize, psHeader->quantity, psHeader->version))
		{
			return FALSE;
		}
	}
	else
	{
		debug( LOG_ERROR, "CompLoad: undefined save format version %d", psHeader->version );
		abort();
		return FALSE;
	}

	return TRUE;
}

// -----------------------------------------------------------------------------------------
BOOL loadSaveCompListV9(char *pFileData, UDWORD filesize, UDWORD numRecords, UDWORD version)
{
	SAVE_COMPLIST_V6		*psSaveCompList;
	UDWORD				i;
	SDWORD				compInc;

	if ((sizeof(SAVE_COMPLIST_V6) * numRecords + COMPLIST_HEADER_SIZE) >
		filesize)
	{
		debug( LOG_ERROR, "CompListLoad: unexpected end of file" );
		abort();
		return FALSE;
	}

	// Load the data
	for (i = 0; i < numRecords; i++, pFileData += sizeof(SAVE_COMPLIST_V6))
	{
		psSaveCompList = (SAVE_COMPLIST_V6 *) pFileData;

		if (version < VERSION_9)
		{
			//DROID_MAXCOMP has changed to remove COMP_PROGRAM so hack here to load old save games!
			if (psSaveCompList->type == SAVE_COMP_PROGRAM)
			{
				//ignore this record
				continue;
			}
			if (psSaveCompList->type == SAVE_COMP_WEAPON)
			{
				//this typeNum has to be reset for lack of COMP_PROGRAM
				psSaveCompList->type = COMP_WEAPON;
			}
		}

		if (psSaveCompList->type > COMP_NUMCOMPONENTS)
		{
			//ignore this record
			continue;
		}

		compInc = getCompFromName(psSaveCompList->type, psSaveCompList->name);

		if (compInc < 0)
		{
			//ignore this record
			continue;
		}
		if (psSaveCompList->state != UNAVAILABLE && psSaveCompList->state !=
			AVAILABLE && psSaveCompList->state != FOUND)
		{
			//ignore this record
			continue;
		}
		if (psSaveCompList->player > MAX_PLAYERS)
		{
			//ignore this record
			continue;
		}
		//date is valid so set the state
		apCompLists[psSaveCompList->player][psSaveCompList->type][compInc] =
			psSaveCompList->state;
	}

	return TRUE;
}

// -----------------------------------------------------------------------------------------
BOOL loadSaveCompListV(char *pFileData, UDWORD filesize, UDWORD numRecords, UDWORD version)
{
	SAVE_COMPLIST		*psSaveCompList;
	UDWORD				i;
	SDWORD				compInc;

//	version;

	if ((sizeof(SAVE_COMPLIST) * numRecords + COMPLIST_HEADER_SIZE) >
		filesize)
	{
		debug( LOG_ERROR, "CompListLoad: unexpected end of file" );
		abort();
		return FALSE;
	}

	// Load the data
	for (i = 0; i < numRecords; i++, pFileData += sizeof(SAVE_COMPLIST))
	{
		psSaveCompList = (SAVE_COMPLIST *) pFileData;

		if (psSaveCompList->type > COMP_NUMCOMPONENTS)
		{
			//ignore this record
			continue;
		}

		compInc = getCompFromName(psSaveCompList->type, psSaveCompList->name);

		if (compInc < 0)
		{
			//ignore this record
			continue;
		}
		if (psSaveCompList->state != UNAVAILABLE && psSaveCompList->state !=
			AVAILABLE && psSaveCompList->state != FOUND)
		{
			//ignore this record
			continue;
		}
		if (psSaveCompList->player > MAX_PLAYERS)
		{
			//ignore this record
			continue;
		}
		//date is valid so set the state
		apCompLists[psSaveCompList->player][psSaveCompList->type][compInc] =
			psSaveCompList->state;
	}

	return TRUE;
}

// -----------------------------------------------------------------------------------------
// Write out the current state of the Comp lists per player
static BOOL writeCompListFile(char *pFileName)
{
	COMPLIST_SAVEHEADER		*psHeader;
	char *pFileData;
	SAVE_COMPLIST			*psSaveCompList;
	UDWORD					fileSize, totalComp, player, i;
	COMP_BASE_STATS			*psStats;

	// Calculate the file size
	totalComp = (numBodyStats + numWeaponStats + numConstructStats + numECMStats +
		numPropulsionStats + numSensorStats + numRepairStats + numBrainStats) * MAX_PLAYERS;
	fileSize = COMPLIST_HEADER_SIZE + (sizeof(SAVE_COMPLIST) * totalComp);
	//allocate the buffer space
	pFileData = (char*)MALLOC(fileSize);
	if (!pFileData)
	{
		debug( LOG_ERROR, "writeCompListFile: Out of memory" );
		abort();
		return FALSE;
	}

	// Put the file header on the file
	psHeader = (COMPLIST_SAVEHEADER *)pFileData;
	psHeader->aFileType[0] = 'c';
	psHeader->aFileType[1] = 'm';
	psHeader->aFileType[2] = 'p';
	psHeader->aFileType[3] = 'l';
	psHeader->version = CURRENT_VERSION_NUM;
	psHeader->quantity = totalComp;

	psSaveCompList = (SAVE_COMPLIST *) (pFileData + COMPLIST_HEADER_SIZE);

	//save each type of comp
	for (player = 0; player < MAX_PLAYERS; player++)
	{
		for(i = 0; i < numBodyStats; i++)
		{
			psStats = (COMP_BASE_STATS *)(asBodyStats + i);

			strcpy(psSaveCompList->name, psStats->pName);

			psSaveCompList->type = COMP_BODY;
			psSaveCompList->player = (UBYTE)player;
			psSaveCompList->state = apCompLists[player][COMP_BODY][i];
			psSaveCompList = (SAVE_COMPLIST *)((char *)psSaveCompList + sizeof(SAVE_COMPLIST));
		}
		for(i = 0; i < numWeaponStats; i++)
		{
			psStats = (COMP_BASE_STATS *)(asWeaponStats + i);

			strcpy(psSaveCompList->name, psStats->pName);

			psSaveCompList->type = COMP_WEAPON;
			psSaveCompList->player = (UBYTE)player;
			psSaveCompList->state = apCompLists[player][COMP_WEAPON][i];
			psSaveCompList = (SAVE_COMPLIST *)((char *)psSaveCompList + sizeof(SAVE_COMPLIST));
		}
		for(i = 0; i < numConstructStats; i++)
		{
			psStats = (COMP_BASE_STATS *)(asConstructStats + i);

			strcpy(psSaveCompList->name, psStats->pName);

			psSaveCompList->type = COMP_CONSTRUCT;
			psSaveCompList->player = (UBYTE)player;
			psSaveCompList->state = apCompLists[player][COMP_CONSTRUCT][i];
			psSaveCompList = (SAVE_COMPLIST *)((char *)psSaveCompList + sizeof(SAVE_COMPLIST));
		}
		for(i = 0; i < numECMStats; i++)
		{
			psStats = (COMP_BASE_STATS *)(asECMStats + i);

			strcpy(psSaveCompList->name, psStats->pName);

			psSaveCompList->type = COMP_ECM;
			psSaveCompList->player = (UBYTE)player;
			psSaveCompList->state = apCompLists[player][COMP_ECM][i];
			psSaveCompList = (SAVE_COMPLIST *)((char *)psSaveCompList + sizeof(SAVE_COMPLIST));
		}
		for(i = 0; i < numPropulsionStats; i++)
		{
			psStats = (COMP_BASE_STATS *)(asPropulsionStats + i);

			strcpy(psSaveCompList->name, psStats->pName);

			psSaveCompList->type = COMP_PROPULSION;
			psSaveCompList->player = (UBYTE)player;
			psSaveCompList->state = apCompLists[player][COMP_PROPULSION][i];
			psSaveCompList = (SAVE_COMPLIST *)((char *)psSaveCompList + sizeof(SAVE_COMPLIST));
		}
		for(i = 0; i < numSensorStats; i++)
		{
			psStats = (COMP_BASE_STATS *)(asSensorStats + i);

			strcpy(psSaveCompList->name, psStats->pName);

			psSaveCompList->type = COMP_SENSOR;
			psSaveCompList->player = (UBYTE)player;
			psSaveCompList->state = apCompLists[player][COMP_SENSOR][i];
			psSaveCompList = (SAVE_COMPLIST *)((char *)psSaveCompList + sizeof(SAVE_COMPLIST));
		}
		for(i = 0; i < numRepairStats; i++)
		{
			psStats = (COMP_BASE_STATS *)(asRepairStats + i);

			strcpy(psSaveCompList->name, psStats->pName);

			psSaveCompList->type = COMP_REPAIRUNIT;
			psSaveCompList->player = (UBYTE)player;
			psSaveCompList->state = apCompLists[player][COMP_REPAIRUNIT][i];
			psSaveCompList = (SAVE_COMPLIST *)((char *)psSaveCompList + sizeof(SAVE_COMPLIST));
		}
		for(i = 0; i < numBrainStats; i++)
		{
			psStats = (COMP_BASE_STATS *)(asBrainStats + i);

			strcpy(psSaveCompList->name, psStats->pName);

			psSaveCompList->type = COMP_BRAIN;
			psSaveCompList->player = (UBYTE)player;
			psSaveCompList->state = apCompLists[player][COMP_BRAIN][i];
			psSaveCompList = (SAVE_COMPLIST *)((char *)psSaveCompList + sizeof(SAVE_COMPLIST));
		}
	}

	/* COMPLIST_SAVEHEADER */
	endian_udword(&psHeader->version);
	endian_udword(&psHeader->quantity);

	if (!saveFile(pFileName, pFileData, fileSize))
	{
		return FALSE;
	}
	FREE(pFileData);

	return TRUE;
}

// -----------------------------------------------------------------------------------------
// load up structure type list file
BOOL loadSaveStructTypeList(char *pFileData, UDWORD filesize)
{
	STRUCTLIST_SAVEHEADER		*psHeader;

	/* Check the file type */
	psHeader = (STRUCTLIST_SAVEHEADER*)pFileData;
	if (psHeader->aFileType[0] != 's' || psHeader->aFileType[1] != 't' ||
		psHeader->aFileType[2] != 'r' || psHeader->aFileType[3] != 'l')
	{
		debug( LOG_ERROR, "loadSaveStructTypeList: Incorrect file type" );
		abort();
		return FALSE;
	}

	/* STRUCTLIST_SAVEHEADER */
	endian_udword(&psHeader->version);
	endian_udword(&psHeader->quantity);

	//increment to the start of the data
	pFileData += STRUCTLIST_HEADER_SIZE;

	/* Check the file version */
	if (psHeader->version < VERSION_7)
	{
		debug( LOG_ERROR, "StructTypeLoad: unsupported save format version %d", psHeader->version );
		abort();
		return FALSE;
	}
	else if (psHeader->version <= VERSION_19)
	{
		if (!loadSaveStructTypeListV7(pFileData, filesize, psHeader->quantity))
		{
			return FALSE;
		}
	}
	else if (psHeader->version <= CURRENT_VERSION_NUM)
	{
		if (!loadSaveStructTypeListV(pFileData, filesize, psHeader->quantity))
		{
			return FALSE;
		}
	}
	else
	{
		debug( LOG_ERROR, "StructTypeLoad: undefined save format version %d", psHeader->version );
		abort();
		return FALSE;
	}

	return TRUE;
}

// -----------------------------------------------------------------------------------------
BOOL loadSaveStructTypeListV7(char *pFileData, UDWORD filesize, UDWORD numRecords)
{
	SAVE_STRUCTLIST_V6		*psSaveStructList;
	UDWORD				i, statInc;
	STRUCTURE_STATS		*psStats;
	BOOL				found;

	if ((sizeof(SAVE_STRUCTLIST_V6) * numRecords + STRUCTLIST_HEADER_SIZE) >
		filesize)
	{
		debug( LOG_ERROR, "StructListLoad: unexpected end of file" );
		abort();
		return FALSE;
	}

	// Load the data
	for (i = 0; i < numRecords; i++, pFileData += sizeof(SAVE_STRUCTLIST_V6))
	{
		psSaveStructList = (SAVE_STRUCTLIST_V6 *) pFileData;

		found = FALSE;

		for (statInc = 0; statInc < numStructureStats; statInc++)
		{
			psStats = asStructureStats + statInc;
			//loop until find the same name

			if (!strcmp(psStats->pName, psSaveStructList->name))

			{
				found = TRUE;
				break;
			}
		}
		if (!found)
		{
			//ignore this record
			continue;
		}
		if (psSaveStructList->state != UNAVAILABLE && psSaveStructList->state !=
			AVAILABLE && psSaveStructList->state != FOUND)
		{
			//ignore this record
			continue;
		}
		if (psSaveStructList->player > MAX_PLAYERS)
		{
			//ignore this record
			continue;
		}
		//date is valid so set the state
		apStructTypeLists[psSaveStructList->player][statInc] =
			psSaveStructList->state;
	}

	return TRUE;
}

// -----------------------------------------------------------------------------------------
BOOL loadSaveStructTypeListV(char *pFileData, UDWORD filesize, UDWORD numRecords)
{
	SAVE_STRUCTLIST		*psSaveStructList;
	UDWORD				i, statInc;
	STRUCTURE_STATS		*psStats;
	BOOL				found;

	if ((sizeof(SAVE_STRUCTLIST) * numRecords + STRUCTLIST_HEADER_SIZE) >
		filesize)
	{
		debug( LOG_ERROR, "StructListLoad: unexpected end of file" );
		abort();
		return FALSE;
	}

	// Load the data
	for (i = 0; i < numRecords; i++, pFileData += sizeof(SAVE_STRUCTLIST))
	{
		psSaveStructList = (SAVE_STRUCTLIST *) pFileData;

		found = FALSE;

		for (statInc = 0; statInc < numStructureStats; statInc++)
		{
			psStats = asStructureStats + statInc;
			//loop until find the same name

			if (!strcmp(psStats->pName, psSaveStructList->name))

			{
				found = TRUE;
				break;
			}
		}
		if (!found)
		{
			//ignore this record
			continue;
		}
		if (psSaveStructList->state != UNAVAILABLE && psSaveStructList->state !=
			AVAILABLE && psSaveStructList->state != FOUND)
		{
			//ignore this record
			continue;
		}
		if (psSaveStructList->player > MAX_PLAYERS)
		{
			//ignore this record
			continue;
		}
		//date is valid so set the state
		apStructTypeLists[psSaveStructList->player][statInc] =
			psSaveStructList->state;
	}

	return TRUE;
}

// -----------------------------------------------------------------------------------------
// Write out the current state of the Struct Type List per player
static BOOL writeStructTypeListFile(char *pFileName)
{
	STRUCTLIST_SAVEHEADER	*psHeader;
	SAVE_STRUCTLIST			*psSaveStructList;
	char *pFileData;
	UDWORD					fileSize, player, i;
	STRUCTURE_STATS			*psStats;

	// Calculate the file size
	fileSize = STRUCTLIST_HEADER_SIZE + (sizeof(SAVE_STRUCTLIST) *
		numStructureStats * MAX_PLAYERS);

	//allocate the buffer space
	pFileData = (char*)MALLOC(fileSize);
	if (!pFileData)
	{
		debug( LOG_ERROR, "writeStructTypeListFile: Out of memory" );
		abort();
		return FALSE;
	}

	// Put the file header on the file
	psHeader = (STRUCTLIST_SAVEHEADER *)pFileData;
	psHeader->aFileType[0] = 's';
	psHeader->aFileType[1] = 't';
	psHeader->aFileType[2] = 'r';
	psHeader->aFileType[3] = 'l';
	psHeader->version = CURRENT_VERSION_NUM;
	psHeader->quantity = numStructureStats * MAX_PLAYERS;

	psSaveStructList = (SAVE_STRUCTLIST *) (pFileData + STRUCTLIST_HEADER_SIZE);
	//save each type of struct type
	for (player = 0; player < MAX_PLAYERS; player++)
	{
		psStats = asStructureStats;
		for(i = 0; i < numStructureStats; i++, psStats++)
		{

			strcpy(psSaveStructList->name, psStats->pName);
			psSaveStructList->state = apStructTypeLists[player][i];
			psSaveStructList->player = (UBYTE)player;
			psSaveStructList = (SAVE_STRUCTLIST *)((char *)psSaveStructList +
				sizeof(SAVE_STRUCTLIST));
		}
	}

	/* STRUCT_SAVEHEADER */
	endian_udword(&psHeader->version);
	endian_udword(&psHeader->quantity);

	if (!saveFile(pFileName, pFileData, fileSize))
	{
		return FALSE;
	}
	FREE(pFileData);

	return TRUE;
}


// -----------------------------------------------------------------------------------------
// load up saved research file
BOOL loadSaveResearch(char *pFileData, UDWORD filesize)
{
	RESEARCH_SAVEHEADER		*psHeader;

	/* Check the file type */
	psHeader = (RESEARCH_SAVEHEADER*)pFileData;
	if (psHeader->aFileType[0] != 'r' || psHeader->aFileType[1] != 'e' ||
		psHeader->aFileType[2] != 's' || psHeader->aFileType[3] != 'h')
	{
		debug( LOG_ERROR, "loadSaveResearch: Incorrect file type" );
		abort();
		return FALSE;
	}

	/* RESEARCH_SAVEHEADER */
	endian_udword(&psHeader->version);
	endian_udword(&psHeader->quantity);

	//increment to the start of the data
	pFileData += RESEARCH_HEADER_SIZE;

	/* Check the file version */
	if (psHeader->version < VERSION_8)
	{
		debug( LOG_ERROR, "ResearchLoad: unsupported save format version %d", psHeader->version );
		abort();
		return FALSE;
	}
	else if (psHeader->version <= VERSION_19)
	{
		if (!loadSaveResearchV8(pFileData, filesize, psHeader->quantity))
		{
			return FALSE;
		}
	}
	else if (psHeader->version <= CURRENT_VERSION_NUM)
	{
		if (!loadSaveResearchV(pFileData, filesize, psHeader->quantity))
		{
			return FALSE;
		}
	}
	else
	{
		debug( LOG_ERROR, "ResearchLoad: undefined save format version %d", psHeader->version );
		abort();
		return FALSE;
	}

	return TRUE;
}

// -----------------------------------------------------------------------------------------
BOOL loadSaveResearchV8(char *pFileData, UDWORD filesize, UDWORD numRecords)
{
	SAVE_RESEARCH_V8		*psSaveResearch;
	UDWORD				i, statInc;
	RESEARCH			*psStats;
	BOOL				found;
	UBYTE				playerInc;

	if ((sizeof(SAVE_RESEARCH_V8) * numRecords + RESEARCH_HEADER_SIZE) >
		filesize)
	{
		debug( LOG_ERROR, "loadSaveResearch: unexpected end of file" );
		abort();
		return FALSE;
	}

	// Load the data
	for (i = 0; i < numRecords; i++, pFileData += sizeof(SAVE_RESEARCH_V8))
	{
		psSaveResearch = (SAVE_RESEARCH_V8 *) pFileData;

		/* SAVE_RESEARCH_V8 is RESEARCH_SAVE_V8 */
		/* RESEARCH_SAVE_V8 */
		for(playerInc = 0; playerInc < MAX_PLAYERS; playerInc++)
			endian_udword(&psSaveResearch->currentPoints[playerInc]);

		found = FALSE;

		for (statInc = 0; statInc < numResearch; statInc++)
		{
			psStats = asResearch + statInc;
			//loop until find the same name

			if (!strcmp(psStats->pName, psSaveResearch->name))

			{
				found = TRUE;
				break;
			}
		}
		if (!found)
		{
			//ignore this record
			continue;
		}


		for (playerInc = 0; playerInc < MAX_PLAYERS; playerInc++)
		{
/* what did this do then ?
			if (psSaveResearch->researched[playerInc] != 0 &&
				psSaveResearch->researched[playerInc] != STARTED_RESEARCH &&
				psSaveResearch->researched[playerInc] != CANCELLED_RESEARCH &&
				psSaveResearch->researched[playerInc] != RESEARCHED)
			{
				//ignore this record
				continue; //to next player
			}
*/
			PLAYER_RESEARCH *psPlRes;

			psPlRes=&asPlayerResList[playerInc][statInc];

			// Copy the research status
			psPlRes->ResearchStatus=	(UBYTE)(psSaveResearch->researched[playerInc] & RESBITS);

			if (psSaveResearch->possible[playerInc]!=0)
				MakeResearchPossible(psPlRes);


			psPlRes->currentPoints = psSaveResearch->currentPoints[playerInc];

			//for any research that has been completed - perform so that upgrade values are set up
			if (psSaveResearch->researched[playerInc] == RESEARCHED)
			{
				researchResult(statInc, playerInc, FALSE);
			}
		}
	}

	return TRUE;
}

// -----------------------------------------------------------------------------------------
BOOL loadSaveResearchV(char *pFileData, UDWORD filesize, UDWORD numRecords)
{
	SAVE_RESEARCH		*psSaveResearch;
	UDWORD				i, statInc;
	RESEARCH			*psStats;
	BOOL				found;
	UBYTE				playerInc;

	if ((sizeof(SAVE_RESEARCH) * numRecords + RESEARCH_HEADER_SIZE) >
		filesize)
	{
		debug( LOG_ERROR, "loadSaveResearch: unexpected end of file" );
		abort();
		return FALSE;
	}

	// Load the data
	for (i = 0; i < numRecords; i++, pFileData += sizeof(SAVE_RESEARCH))
	{
		psSaveResearch = (SAVE_RESEARCH *) pFileData;

		/* SAVE_RESEARCH is RESEARCH_SAVE_V20 */
		/* RESEARCH_SAVE_V20 */
		for(playerInc = 0; playerInc < MAX_PLAYERS; playerInc++)
			endian_udword(&psSaveResearch->currentPoints[playerInc]);

		found = FALSE;

		for (statInc = 0; statInc < numResearch; statInc++)
		{
			psStats = asResearch + statInc;
			//loop until find the same name

			if (!strcmp(psStats->pName, psSaveResearch->name))

			{
				found = TRUE;
				break;
			}
		}
		if (!found)
		{
			//ignore this record
			continue;
		}


		for (playerInc = 0; playerInc < MAX_PLAYERS; playerInc++)
		{


			PLAYER_RESEARCH *psPlRes;

			psPlRes=&asPlayerResList[playerInc][statInc];

			// Copy the research status
			psPlRes->ResearchStatus=	(UBYTE)(psSaveResearch->researched[playerInc] & RESBITS);

			if (psSaveResearch->possible[playerInc]!=0)
				MakeResearchPossible(psPlRes);



			psPlRes->currentPoints = psSaveResearch->currentPoints[playerInc];

			//for any research that has been completed - perform so that upgrade values are set up
			if (psSaveResearch->researched[playerInc] == RESEARCHED)
			{
				researchResult(statInc, playerInc, FALSE);
			}
		}
	}

	return TRUE;
}

// -----------------------------------------------------------------------------------------
// Write out the current state of the Research per player
static BOOL writeResearchFile(char *pFileName)
{
	RESEARCH_SAVEHEADER		*psHeader;
	SAVE_RESEARCH			*psSaveResearch;
	char *pFileData;
	UDWORD					fileSize, player, i;
	RESEARCH				*psStats;

	// Calculate the file size
	fileSize = RESEARCH_HEADER_SIZE + (sizeof(SAVE_RESEARCH) *
		numResearch);

	//allocate the buffer space
	pFileData = (char*)MALLOC(fileSize);
	if (!pFileData)
	{
		debug( LOG_ERROR, "writeResearchFile: Out of memory" );
		abort();
		return FALSE;
	}

	// Put the file header on the file
	psHeader = (RESEARCH_SAVEHEADER *)pFileData;
	psHeader->aFileType[0] = 'r';
	psHeader->aFileType[1] = 'e';
	psHeader->aFileType[2] = 's';
	psHeader->aFileType[3] = 'h';
	psHeader->version = CURRENT_VERSION_NUM;
	psHeader->quantity = numResearch;

	endian_udword(&psHeader->version);
	endian_udword(&psHeader->quantity);

	psSaveResearch = (SAVE_RESEARCH *) (pFileData + RESEARCH_HEADER_SIZE);
	//save each type of reesearch
	psStats = asResearch;
	for(i = 0; i < numResearch; i++, psStats++)
	{

		strcpy(psSaveResearch->name, psStats->pName);

		for (player = 0; player < MAX_PLAYERS; player++)
		{
			psSaveResearch->possible[player] = (UBYTE)IsResearchPossible(&asPlayerResList[player][i]);
			psSaveResearch->researched[player] = (UBYTE)(asPlayerResList[player][i].ResearchStatus&RESBITS);
			psSaveResearch->currentPoints[player] = asPlayerResList[player][i].currentPoints;
		}

		for(player = 0; player < MAX_PLAYERS; player++)
			endian_udword(&psSaveResearch->currentPoints[player]);

		psSaveResearch = (SAVE_RESEARCH *)((char *)psSaveResearch +
			sizeof(SAVE_RESEARCH));
	}

	if (!saveFile(pFileName, pFileData, fileSize))
	{
		return FALSE;
	}
	FREE(pFileData);

	return TRUE;
}


// -----------------------------------------------------------------------------------------
//#ifdef NEW_SAVE //V11 Save
// load up saved message file
BOOL loadSaveMessage(char *pFileData, UDWORD filesize, SWORD levelType)
{
	MESSAGE_SAVEHEADER		*psHeader;

	/* Check the file type */
	psHeader = (MESSAGE_SAVEHEADER*)pFileData;
	if (psHeader->aFileType[0] != 'm' || psHeader->aFileType[1] != 'e' ||
		psHeader->aFileType[2] != 's' || psHeader->aFileType[3] != 's')
	{
		debug( LOG_ERROR, "loadSaveMessage: Incorrect file type" );
		abort();
		return FALSE;
	}

	/* MESSAGE_SAVEHEADER */
	endian_udword(&psHeader->version);
	endian_udword(&psHeader->quantity);

	//increment to the start of the data
	pFileData += MESSAGE_HEADER_SIZE;

	/* Check the file version */
	if (!loadSaveMessageV(pFileData, filesize, psHeader->quantity, psHeader->version, levelType))
	{
		return FALSE;
	}
	return TRUE;
}

// -----------------------------------------------------------------------------------------
BOOL loadSaveMessageV(char *pFileData, UDWORD filesize, UDWORD numMessages, UDWORD version, SWORD levelType)
{
	SAVE_MESSAGE	*psSaveMessage;
	MESSAGE			*psMessage;
	VIEWDATA		*psViewData = NULL;
	UDWORD			i, height;

	//clear any messages put in during level loads
	//freeMessages();

    //only clear the messages if its a mid save game
	if (gameType == GTYPE_SAVE_MIDMISSION)
    {
        freeMessages();
    }
    else if (gameType == GTYPE_SAVE_START)
    {
        //if we're loading in a CamStart or a CamChange then we're not interested in any saved messages
        if (levelType == LDS_CAMSTART || levelType == LDS_CAMCHANGE)
        {
            return TRUE;
        }

    }

	//check file
	if ((sizeof(SAVE_MESSAGE) * numMessages + MESSAGE_HEADER_SIZE) >
		filesize)
	{
		debug( LOG_ERROR, "loadSaveMessage: unexpected end of file" );
		abort();
		return FALSE;
	}

	// Load the data
	for (i = 0; i < numMessages; i++, pFileData += sizeof(SAVE_MESSAGE))
	{
		psSaveMessage = (SAVE_MESSAGE *) pFileData;

		/* SAVE_MESSAGE */
		endian_sdword((SDWORD*)&psSaveMessage->type);	/* FIXME: enum may not be this type! */
		endian_udword(&psSaveMessage->objId);
		endian_udword(&psSaveMessage->player);

		if (psSaveMessage->type == MSG_PROXIMITY)
		{
            //only load proximity if a mid-mission save game
            if (gameType == GTYPE_SAVE_MIDMISSION)
            {
			    if (psSaveMessage->bObj)
			    {
				    //proximity object so create get the obj from saved idy
				    psMessage = addMessage(psSaveMessage->type, TRUE, psSaveMessage->player);
				    if (psMessage)
				    {
					    psMessage->pViewData = (MSG_VIEWDATA *)getBaseObjFromId(psSaveMessage->objId);
				    }
			    }
			    else
			    {
				    //proximity position so get viewdata pointer from the name
				    psMessage = addMessage(psSaveMessage->type, FALSE, psSaveMessage->player);
				    if (psMessage)
				    {
					    psViewData = (VIEWDATA *)getViewData(psSaveMessage->name);
                        if (psViewData == NULL)
                        {
                            //skip this message
                            continue;
                        }
					    psMessage->pViewData = (MSG_VIEWDATA *)psViewData;
				    }
				    //check the z value is at least the height of the terrain
				    height = map_Height(((VIEW_PROXIMITY *)psViewData->pData)->x,
					    ((VIEW_PROXIMITY *)psViewData->pData)->y);
				    if (((VIEW_PROXIMITY *)psViewData->pData)->z < height)
				    {
					    ((VIEW_PROXIMITY *)psViewData->pData)->z = height;
				    }
			    }
            }
		}
		else
		{
            //only load Campaign/Mission if a mid-mission save game
            if (psSaveMessage->type == MSG_CAMPAIGN || psSaveMessage->type == MSG_MISSION)
            {
                if (gameType == GTYPE_SAVE_MIDMISSION)
                {
    			    // Research message // Campaign message // Mission Report messages
    	    		psMessage = addMessage(psSaveMessage->type, FALSE, psSaveMessage->player);
	    	    	if (psMessage)
		    	    {
			    	    psMessage->pViewData = (MSG_VIEWDATA *)getViewData(psSaveMessage->name);
			        }
                }
            }
            else
            {
    			// Research message
    	    	psMessage = addMessage(psSaveMessage->type, FALSE, psSaveMessage->player);
	    	    if (psMessage)
		    	{
			    	psMessage->pViewData = (MSG_VIEWDATA *)getViewData(psSaveMessage->name);
			    }
            }
		}
	}

	return TRUE;
}
// -----------------------------------------------------------------------------------------
// Write out the current messages per player
static BOOL writeMessageFile(char *pFileName)
{
	MESSAGE_SAVEHEADER		*psHeader;
	SAVE_MESSAGE			*psSaveMessage;
	char *pFileData;
	UDWORD					fileSize, player;
	MESSAGE					*psMessage;
	PROXIMITY_DISPLAY		*psProx;
	BASE_OBJECT				*psObj;
	UDWORD					numMessages = 0;
	VIEWDATA				*pViewData;

	// Calculate the file size
	for (player = 0; player < MAX_PLAYERS; player++)
	{
		for(psMessage = apsMessages[player]; psMessage != NULL;psMessage = psMessage->psNext)
		{
			numMessages++;
		}

	}

	fileSize = MESSAGE_HEADER_SIZE + (sizeof(SAVE_MESSAGE) *
		numMessages);


	//allocate the buffer space
	pFileData = (char*)MALLOC(fileSize);
	if (!pFileData)
	{
		debug( LOG_ERROR, "writeMessageFile: Out of memory" );
		abort();
		return FALSE;
	}

	// Put the file header on the file
	psHeader = (MESSAGE_SAVEHEADER *)pFileData;
	psHeader->aFileType[0] = 'm';
	psHeader->aFileType[1] = 'e';
	psHeader->aFileType[2] = 's';
	psHeader->aFileType[3] = 's';
	psHeader->version = CURRENT_VERSION_NUM;
	psHeader->quantity = numMessages;

	psSaveMessage = (SAVE_MESSAGE *) (pFileData + MESSAGE_HEADER_SIZE);
	//save each type of reesearch
	for (player = 0; player < MAX_PLAYERS; player++)
	{
		psMessage = apsMessages[player];
		for(psMessage = apsMessages[player]; psMessage != NULL;psMessage = psMessage->psNext)
		{
			psSaveMessage->type = psMessage->type;	//The type of message
			if (psMessage->type == MSG_PROXIMITY)
			{
				//get the matching proximity message
				for(psProx = apsProxDisp[player]; psProx != NULL; psProx = psProx->psNext)
				{
					//compare the pointers
					if (psProx->psMessage == psMessage)
					{
						break;
					}
				}
				ASSERT( psProx != NULL,"Save message; proximity display not found for message" );

				if (psProx->type == POS_PROXDATA)
				{
					//message has viewdata so store the name
					psSaveMessage->bObj = FALSE;
					pViewData = (VIEWDATA*)psMessage->pViewData;
					ASSERT( strlen(pViewData->pName) < MAX_STR_SIZE,"writeMessageFile; viewdata pName Error" );
					strcpy(psSaveMessage->name,pViewData->pName);	//Pointer to view data - if any - should be some!
				}
				else
				{
					//message has object so store ObjectId
					psSaveMessage->bObj = TRUE;
					psObj = (BASE_OBJECT*)psMessage->pViewData;
					psSaveMessage->objId = psObj->id;//should be unique for these objects
				}
			}
			else
			{
				psSaveMessage->bObj = FALSE;
				pViewData = (VIEWDATA*)psMessage->pViewData;
				ASSERT( strlen(pViewData->pName) < MAX_STR_SIZE,"writeMessageFile; viewdata pName Error" );
				strcpy(psSaveMessage->name,pViewData->pName);	//Pointer to view data - if any - should be some!
			}
			psSaveMessage->read = psMessage->read;			//flag to indicate whether message has been read
			psSaveMessage->player = psMessage->player;		//which player this message belongs to

			endian_sdword((SDWORD*)&psSaveMessage->type); /* FIXME: enum may be different type! */
			endian_udword(&psSaveMessage->objId);
			endian_udword(&psSaveMessage->player);

			psSaveMessage = (SAVE_MESSAGE *)((char *)psSaveMessage + 	sizeof(SAVE_MESSAGE));
		}
	}

	/* MESSAGE_SAVEHEADER */
	endian_udword(&psHeader->version);
	endian_udword(&psHeader->quantity);

	if (!saveFile(pFileName, pFileData, fileSize))
	{
		return FALSE;
	}
	FREE(pFileData);

	return TRUE;
}
//#endif

// -----------------------------------------------------------------------------------------
#ifdef SAVE_PROXIMITY_STUFF//V14 Save this is not done because all messages are rebuilt
// load up saved proximity file
BOOL loadSaveProximity(char *pFileData, UDWORD filesize)
{
	PROXIMITY_SAVEHEADER		*psHeader;

	/* Check the file type */
	psHeader = (PROXIMITY_SAVEHEADER*)pFileData;
	if (psHeader->aFileType[0] != 'm' || psHeader->aFileType[1] != 'e' ||
		psHeader->aFileType[2] != 's' || psHeader->aFileType[3] != 's')
	{
		debug( LOG_ERROR, "loadSaveProximity: Incorrect file type" );
		abort();
		return FALSE;
	}

	/* PROXIMITY_SAVEHEADER */
	endian_udword(&psHeader->version);
	endian_udword(&psHeader->quantity);

	//increment to the start of the data
	pFileData += PROXIMITY_HEADER_SIZE;

	/* Check the file version */
	if (!loadSaveProximityV(pFileData, filesize, psHeader->quantity, psHeader->version))
	{
		return FALSE;
	}
	return TRUE;
}

// -----------------------------------------------------------------------------------------
BOOL loadSaveProximityV(char *pFileData, UDWORD filesize, UDWORD numProximitys, UDWORD version)
{
	SAVE_PROXIMITY	*psSaveProximity;
	PROXIMITY_DISPLAY	*psProximity;
	UDWORD i;

	//clear any proximitys put in during level loads
	freeProximitys();

	//check file
	if ((sizeof(SAVE_PROXIMITY) * numProximitys + PROXIMITY_HEADER_SIZE) >
		filesize)
	{
		debug( LOG_ERROR, "loadSaveProximity: unexpected end of file" );
		abort();
		return FALSE;
	}

	// Load the data
	for (i = 0; i < numProximitys; i++, pFileData += sizeof(SAVE_PROXIMITY))
	{
		psSaveProximity = (SAVE_PROXIMITY *) pFileData;

		/* SAVE_PROXIMITY */
		endian_sdword(&psSaveProximity->type);
		endian_udword(&psSaveProximity->frameNumber);
		endian_udword(&psSaveProximity->screenX);
		endian_udword(&psSaveProximity->screenY);
		endian_udword(&psSaveProximity->screenR);
		endian_udword(&psSaveProximity->objId);
		endian_udword(&psSaveProximity->radarX);
		endian_udword(&psSaveProximity->radarY);
		endian_udword(&psSaveProximity->timeLastDrawn);
		endian_udword(&psSaveProximity->strobe);
		endian_udword(&psSaveProximity->buttonID);

		if (psSaveProximity->type == MSG_PROXIMITY)
		{
//			addProximity(psSaveProximity->type, psSaveProximity->proxPos, psSaveProximity->player);
		}
		else
		{
//			psProximity = addProximity(psSaveProximity->type, FALSE, psSaveProximity->player);
			if (psProximity)
			{
	//			psProximity->pViewData = getViewData(psSaveProximity->name);
			}
		}
	}

	return TRUE;
}
// -----------------------------------------------------------------------------------------
// Write out the current proximitys per player
static BOOL writeProximityFile(char *pFileName)
{
	PROXIMITY_SAVEHEADER		*psHeader;
	SAVE_PROXIMITY			*psSaveProximity;
	char *pFileData;
	UDWORD					fileSize, player;
	PROXIMITY_DISPLAY		*psProximity;
	UDWORD					numProximitys = 0;
	VIEWDATA				*pViewData;

	// Calculate the file size
	for (player = 0; player < MAX_PLAYERS; player++)
	{
		for(psProximity = apsProxDisp[player]; psProximity != NULL;psProximity = psProximity->psNext)
		{
			numProximitys++;
		}

	}

	fileSize = PROXIMITY_HEADER_SIZE + (sizeof(SAVE_PROXIMITY) *
		numProximitys);


	//allocate the buffer space
	pFileData = (char*)MALLOC(fileSize);
	if (!pFileData)
	{
		debug( LOG_ERROR, "writeProximityFile: Out of memory" );
		abort();
		return FALSE;
	}

	// Put the file header on the file
	psHeader = (PROXIMITY_SAVEHEADER *)pFileData;
	psHeader->aFileType[0] = 'm';
	psHeader->aFileType[1] = 'e';
	psHeader->aFileType[2] = 's';
	psHeader->aFileType[3] = 's';
	psHeader->version = CURRENT_VERSION_NUM;
	psHeader->quantity = numProximitys;

	psSaveProximity = (SAVE_PROXIMITY *) (pFileData + PROXIMITY_HEADER_SIZE);
	//save each type of reesearch
	for (player = 0; player < MAX_PLAYERS; player++)
	{
		psProximity = apsProxDisp[player];
		for(psProximity = apsProxDisp[player]; psProximity != NULL;psProximity = psProximity->psNext)
		{
			psSaveProximity->type		 = psProximity->type;	//The type of proximity
			psSaveProximity->frameNumber = psProximity->frameNumber;
			psSaveProximity->screenX	 = psProximity->screenX;
			psSaveProximity->screenY	 = psProximity->screenY;
			psSaveProximity->screenR	 = psProximity->screenR;
			psSaveProximity->player		 = psProximity->player;
			psSaveProximity->selected	 = psProximity->selected;
			psSaveProximity->objId		 = psProximity->psMessage->id;
			psSaveProximity->radarX		 = psProximity->radarX;
			psSaveProximity->radarY		 = psProximity->radarY;
			psSaveProximity->timeLastDrawn = psProximity->timeLastDrawn;
			psSaveProximity->strobe		 = psProximity->strobe;
			psSaveProximity->buttonID	 = psProximity->buttonID;
			psSaveProximity->player		 = psProximity->player;
			/* SAVE_PROXIMITY */
			endian_sdword(&psSaveProximity->type);
			endian_udword(&psSaveProximity->frameNumber);
			endian_udword(&psSaveProximity->screenX);
			endian_udword(&psSaveProximity->screenY);
			endian_udword(&psSaveProximity->screenR);
			endian_udword(&psSaveProximity->objId);
			endian_udword(&psSaveProximity->radarX);
			endian_udword(&psSaveProximity->radarY);
			endian_udword(&psSaveProximity->timeLastDrawn);
			endian_udword(&psSaveProximity->strobe);
			endian_udword(&psSaveProximity->buttonID);

			psSaveProximity = (SAVE_PROXIMITY *)((char *)psSaveProximity + sizeof(SAVE_PROXIMITY));
		}
	}

	/* PROXIMITY_SAVEHEADER */
	endian_udword(&psSaveHeader->version);
	endian_udword(&psSaveHeader->quantity);

	if (!saveFile(pFileName, pFileData, fileSize))
	{
		return FALSE;
	}
	FREE(pFileData);

	return TRUE;
}
#endif

// -----------------------------------------------------------------------------------------
//#ifdef NEW_SAVE //V11 Save
// load up saved flag file
BOOL loadSaveFlag(char *pFileData, UDWORD filesize)
{
	FLAG_SAVEHEADER		*psHeader;

	/* Check the file type */
	psHeader = (FLAG_SAVEHEADER*)pFileData;
	if (psHeader->aFileType[0] != 'f' || psHeader->aFileType[1] != 'l' ||
		psHeader->aFileType[2] != 'a' || psHeader->aFileType[3] != 'g')
	{
		debug( LOG_ERROR, "loadSaveflag: Incorrect file type" );
		abort();
		return FALSE;
	}

	/* FLAG_SAVEHEADER */
	endian_udword(&psHeader->version);
	endian_udword(&psHeader->quantity);

	//increment to the start of the data
	pFileData += FLAG_HEADER_SIZE;

	/* Check the file version */
	if (!loadSaveFlagV(pFileData, filesize, psHeader->quantity, psHeader->version))
	{
		return FALSE;
	}
	return TRUE;
}

// -----------------------------------------------------------------------------------------
BOOL loadSaveFlagV(char *pFileData, UDWORD filesize, UDWORD numflags, UDWORD version)
{
	SAVE_FLAG		*psSaveflag;
	FLAG_POSITION	*psflag;
	UDWORD			i;
	STRUCTURE*		psStruct;
	UDWORD			factoryToFind = 0;
	UDWORD			sizeOfSaveFlag;
//	version;//unreferenced

	//clear any flags put in during level loads
	freeAllFlagPositions();
	initFactoryNumFlag();//clear the factory masks, we will find the factories from their assembly points


	//check file
	if (version <= VERSION_18)
	{
		sizeOfSaveFlag = sizeof(SAVE_FLAG_V18);
	}
	else
	{
		sizeOfSaveFlag = sizeof(SAVE_FLAG);
	}

	if ((sizeOfSaveFlag * numflags + FLAG_HEADER_SIZE) > filesize)
	{
		debug( LOG_ERROR, "loadSaveflag: unexpected end of file" );
		abort();
		return FALSE;
	}

	// Load the data
	for (i = 0; i < numflags; i++, pFileData += sizeOfSaveFlag)
	{
		psSaveflag = (SAVE_FLAG *) pFileData;

		/* SAVE_FLAG */
		endian_sdword((SDWORD*) &psSaveflag->type); /* FIXME: enum may not be this type! */
		endian_udword(&psSaveflag->frameNumber);
		endian_udword(&psSaveflag->screenX);
		endian_udword(&psSaveflag->screenY);
		endian_udword(&psSaveflag->screenR);
		endian_udword(&psSaveflag->player);
		endian_sdword(&psSaveflag->coords.x);
		endian_sdword(&psSaveflag->coords.y);
		endian_sdword(&psSaveflag->coords.z);
		endian_udword(&psSaveflag->repairId);

		createFlagPosition(&psflag, psSaveflag->player);
		psflag->type = psSaveflag->type;				//The type of flag
		psflag->frameNumber = psSaveflag->frameNumber;	//when the Position was last drawn
		psflag->screenX = psSaveflag->screenX;			//screen coords and radius of Position imd
		psflag->screenY = psSaveflag->screenY;
		psflag->screenR = psSaveflag->screenR;
		psflag->player = psSaveflag->player;			//which player the Position belongs to
		psflag->selected = psSaveflag->selected;		//flag to indicate whether the Position
		psflag->coords = psSaveflag->coords;			//the world coords of the Position
		psflag->factoryInc = psSaveflag->factoryInc;	//indicates whether the first, second etc factory
		psflag->factoryType = psSaveflag->factoryType;	//indicates whether standard, cyborg or vtol factory

		if ((psflag->type == POS_DELIVERY) || (psflag->type == POS_TEMPDELIVERY))
		{
			if (psflag->factoryType == FACTORY_FLAG)
			{
				factoryToFind = REF_FACTORY;
			}
			else if (psflag->factoryType == CYBORG_FLAG)
			{
				factoryToFind = REF_CYBORG_FACTORY;
			}
			else if (psflag->factoryType == VTOL_FLAG)
			{
				factoryToFind = REF_VTOL_FACTORY;
			}
			else if (psflag->factoryType == REPAIR_FLAG)
			{
				factoryToFind = REF_REPAIR_FACILITY;
			}
			else
			{
				ASSERT( FALSE,"loadSaveFlagV delivery flag type not recognised?" );
			}

			if (factoryToFind == REF_REPAIR_FACILITY)
			{
				if (version > VERSION_18)
				{
					psStruct = (STRUCTURE*)getBaseObjFromId(psSaveflag->repairId);
					if (psStruct != NULL)
					{
						if (psStruct->type != OBJ_STRUCTURE)
						{
							ASSERT( FALSE,"loadFlag found duplicate Id for repair facility" );
						}
						else if (psStruct->pStructureType->type == REF_REPAIR_FACILITY)
						{
							if (psStruct->pFunctionality != NULL)
							{
								//this is the one so set it
								((REPAIR_FACILITY *)psStruct->pFunctionality)->psDeliveryPoint = psflag;
							}
						}
					}
				}
			}
			else
			{
				//okay find the player factory with this inc and set this as the assembly point
				for (psStruct = apsStructLists[psflag->player]; psStruct != NULL; psStruct = psStruct->psNext)
				{
					if (psStruct->pStructureType->type == factoryToFind)
					{
						if ((UDWORD)((FACTORY *)psStruct->pFunctionality)->psAssemblyPoint == psflag->factoryInc)
						{
							//this is the one so set it
							((FACTORY *)psStruct->pFunctionality)->psAssemblyPoint = psflag;
						}
					}
				}
			}
		}

		if (psflag->type == POS_DELIVERY)//dont add POS_TEMPDELIVERYs
		{
			addFlagPosition(psflag);
		}
		else if (psflag->type == POS_TEMPDELIVERY)//but make them real flags
		{
			psflag->type = POS_DELIVERY;
		}
	}

	resetFactoryNumFlag();//set the new numbers into the masks

	return TRUE;
}

// -----------------------------------------------------------------------------------------
// Write out the current flags per player
static BOOL writeFlagFile(char *pFileName)
{
	FLAG_SAVEHEADER		*psHeader;
	SAVE_FLAG			*psSaveflag;
	STRUCTURE			*psStruct;
	FACTORY				*psFactory;
	char				*pFileData;
	UDWORD				fileSize, player;
	FLAG_POSITION		*psflag;
	UDWORD				numflags = 0;


	// Calculate the file size
	for (player = 0; player < MAX_PLAYERS; player++)
	{
		for(psflag = apsFlagPosLists[player]; psflag != NULL;psflag = psflag->psNext)
		{
			numflags++;
		}

	}
	//and add the delivery points not in the list
	for (player = 0; player < MAX_PLAYERS; player++)
	{
		for(psStruct = apsStructLists[player]; psStruct != NULL; psStruct = psStruct->psNext)
		{
			if (psStruct->pFunctionality)
			{
				switch (psStruct->pStructureType->type)
				{
				case REF_FACTORY:
				case REF_CYBORG_FACTORY:
				case REF_VTOL_FACTORY:
					psFactory = ((FACTORY *)psStruct->pFunctionality);
					//if there is a commander then has been temporarily removed
					//so put it back
					if (psFactory->psCommander)
					{
						psflag = psFactory->psAssemblyPoint;
						if (psflag != NULL)
						{
							numflags++;
						}
					}
					break;
				default:
					break;
				}
			}
		}
	}

	fileSize = FLAG_HEADER_SIZE + (sizeof(SAVE_FLAG) *
		numflags);


	//allocate the buffer space
	pFileData = (char*)MALLOC(fileSize);
	if (!pFileData)
	{
		debug( LOG_ERROR, "writeflagFile: Out of memory" );
		abort();
		return FALSE;
	}

	// Put the file header on the file
	psHeader = (FLAG_SAVEHEADER *)pFileData;
	psHeader->aFileType[0] = 'f';
	psHeader->aFileType[1] = 'l';
	psHeader->aFileType[2] = 'a';
	psHeader->aFileType[3] = 'g';
	psHeader->version = CURRENT_VERSION_NUM;
	psHeader->quantity = numflags;

	psSaveflag = (SAVE_FLAG *) (pFileData + FLAG_HEADER_SIZE);
	//save each type of reesearch
	for (player = 0; player < MAX_PLAYERS; player++)
	{
		psflag = apsFlagPosLists[player];
		for(psflag = apsFlagPosLists[player]; psflag != NULL;psflag = psflag->psNext)
		{
			psSaveflag->type = psflag->type;				//The type of flag
			psSaveflag->frameNumber = psflag->frameNumber;	//when the Position was last drawn
			psSaveflag->screenX = psflag->screenX;			//screen coords and radius of Position imd
			psSaveflag->screenY = psflag->screenY;
			psSaveflag->screenR = psflag->screenR;
			psSaveflag->player = psflag->player;			//which player the Position belongs to
			psSaveflag->selected = psflag->selected;		//flag to indicate whether the Position
			psSaveflag->coords = psflag->coords;			//the world coords of the Position
			psSaveflag->factoryInc = psflag->factoryInc;	//indicates whether the first, second etc factory
			psSaveflag->factoryType = psflag->factoryType;	//indicates whether standard, cyborg or vtol factory
			if (psflag->factoryType == REPAIR_FLAG)
			{
				//get repair facility id
				psSaveflag->repairId = getRepairIdFromFlag(psflag);
			}

			/* SAVE_FLAG */
			endian_sdword((SDWORD*)&psSaveflag->type); /* FIXME: enum may be different type! */
			endian_udword(&psSaveflag->frameNumber);
			endian_udword(&psSaveflag->screenX);
			endian_udword(&psSaveflag->screenY);
			endian_udword(&psSaveflag->screenR);
			endian_udword(&psSaveflag->player);
			endian_sdword(&psSaveflag->coords.x);
			endian_sdword(&psSaveflag->coords.y);
			endian_sdword(&psSaveflag->coords.z);
			endian_udword(&psSaveflag->repairId);

			psSaveflag = (SAVE_FLAG *)((char *)psSaveflag + 	sizeof(SAVE_FLAG));
		}
	}
	//and add the delivery points not in the list
	for (player = 0; player < MAX_PLAYERS; player++)
	{
		for(psStruct = apsStructLists[player]; psStruct != NULL; psStruct = psStruct->psNext)
		{
			if (psStruct->pFunctionality)
			{
				switch (psStruct->pStructureType->type)
				{
				case REF_FACTORY:
				case REF_CYBORG_FACTORY:
				case REF_VTOL_FACTORY:
					psFactory = ((FACTORY *)psStruct->pFunctionality);
					//if there is a commander then has been temporarily removed
					//so put it back
					if (psFactory->psCommander)
					{
						psflag = psFactory->psAssemblyPoint;
						if (psflag != NULL)
						{
							psSaveflag->type = POS_TEMPDELIVERY;				//The type of flag
							psSaveflag->frameNumber = psflag->frameNumber;	//when the Position was last drawn
							psSaveflag->screenX = psflag->screenX;			//screen coords and radius of Position imd
							psSaveflag->screenY = psflag->screenY;
							psSaveflag->screenR = psflag->screenR;
							psSaveflag->player = psflag->player;			//which player the Position belongs to
							psSaveflag->selected = psflag->selected;		//flag to indicate whether the Position
							psSaveflag->coords = psflag->coords;			//the world coords of the Position
							psSaveflag->factoryInc = psflag->factoryInc;	//indicates whether the first, second etc factory
							psSaveflag->factoryType = psflag->factoryType;	//indicates whether standard, cyborg or vtol factory
							if (psflag->factoryType == REPAIR_FLAG)
							{
								//get repair facility id
								psSaveflag->repairId = getRepairIdFromFlag(psflag);
							}

							/* SAVE_FLAG */
							endian_sdword((SDWORD*)&psSaveflag->type); /* FIXME: enum may be different type! */
							endian_udword(&psSaveflag->frameNumber);
							endian_udword(&psSaveflag->screenX);
							endian_udword(&psSaveflag->screenY);
							endian_udword(&psSaveflag->screenR);
							endian_udword(&psSaveflag->player);
							endian_sdword(&psSaveflag->coords.x);
							endian_sdword(&psSaveflag->coords.y);
							endian_sdword(&psSaveflag->coords.z);
							endian_udword(&psSaveflag->repairId);

							psSaveflag = (SAVE_FLAG *)((char *)psSaveflag + 	sizeof(SAVE_FLAG));
						}
					}
					break;
				default:
					break;
				}
			}
		}
	}

	/* FLAG_SAVEHEADER */
	endian_udword(&psHeader->version);
	endian_udword(&psHeader->quantity);

	if (!saveFile(pFileName, pFileData, fileSize))
	{
		return FALSE;
	}
	FREE(pFileData);

	return TRUE;
}
//#endif

// -----------------------------------------------------------------------------------------
//#ifdef PRODUCTION
BOOL loadSaveProduction(char *pFileData, UDWORD filesize)
{
	PRODUCTION_SAVEHEADER		*psHeader;

	/* Check the file type */
	psHeader = (PRODUCTION_SAVEHEADER*)pFileData;
	if (psHeader->aFileType[0] != 'p' || psHeader->aFileType[1] != 'r' ||
		psHeader->aFileType[2] != 'o' || psHeader->aFileType[3] != 'd')
	{
		debug( LOG_ERROR, "loadSaveProduction: Incorrect file type" );
		abort();
		return FALSE;
	}

	/* PRODUCTION_SAVEHEADER */
	endian_udword(&psHeader->version);

	//increment to the start of the data
	pFileData += PRODUCTION_HEADER_SIZE;

	/* Check the file version */
	if (!loadSaveProductionV(pFileData, filesize, psHeader->version))
	{
		return FALSE;
	}
	return TRUE;
}

// -----------------------------------------------------------------------------------------
BOOL loadSaveProductionV(char *pFileData, UDWORD filesize, UDWORD version)
{
	SAVE_PRODUCTION	*psSaveProduction;
	PRODUCTION_RUN	*psCurrentProd;
	UDWORD			factoryType,factoryNum,runNum;
//	version;//unreferenced


	//check file
	if ((sizeof(SAVE_PRODUCTION) * NUM_FACTORY_TYPES * MAX_FACTORY * MAX_PROD_RUN + PRODUCTION_HEADER_SIZE) >
		filesize)
	{
		debug( LOG_ERROR, "loadSaveProduction: unexpected end of file" );
		abort();
		return FALSE;
	}

	//save each production run
	for (factoryType = 0; factoryType < NUM_FACTORY_TYPES; factoryType++)
	{
		for (factoryNum = 0; factoryNum < MAX_FACTORY; factoryNum++)
		{
			for (runNum = 0; runNum < MAX_PROD_RUN; runNum++)
			{
				psSaveProduction = (SAVE_PRODUCTION *)pFileData;
				psCurrentProd = &asProductionRun[factoryType][factoryNum][runNum];

				/* SAVE_PRODUCTION */
				endian_udword(&psSaveProduction->multiPlayerID);

				psCurrentProd->quantity = psSaveProduction->quantity;
				psCurrentProd->built = psSaveProduction->built;
				if (psSaveProduction->multiPlayerID != UDWORD_MAX)
				{
					psCurrentProd->psTemplate = getTemplateFromMultiPlayerID(psSaveProduction->multiPlayerID);
				}
				else
				{
					psCurrentProd->psTemplate = NULL;
				}
				pFileData += sizeof(SAVE_PRODUCTION);
			}
		}
	}

	return TRUE;
}

// -----------------------------------------------------------------------------------------
// Write out the current production figures for factories
static BOOL writeProductionFile(char *pFileName)
{
	PRODUCTION_SAVEHEADER	*psHeader;
	SAVE_PRODUCTION			*psSaveProduction;
	char				*pFileData;
	UDWORD				fileSize;
	PRODUCTION_RUN	*psCurrentProd;
	UDWORD				factoryType,factoryNum,runNum;

//PRODUCTION_RUN		asProductionRun[NUM_FACTORY_TYPES][MAX_FACTORY][MAX_PROD_RUN];


	fileSize = PRODUCTION_HEADER_SIZE + (sizeof(SAVE_PRODUCTION) *
		NUM_FACTORY_TYPES * MAX_FACTORY * MAX_PROD_RUN);


	//allocate the buffer space
	pFileData = (char*)MALLOC(fileSize);
	if (!pFileData)
	{
		debug( LOG_ERROR, "writeProductionFile: Out of memory" );
		abort();
		return FALSE;
	}

	// Put the file header on the file
	psHeader = (PRODUCTION_SAVEHEADER *)pFileData;
	psHeader->aFileType[0] = 'p';
	psHeader->aFileType[1] = 'r';
	psHeader->aFileType[2] = 'o';
	psHeader->aFileType[3] = 'd';
	psHeader->version = CURRENT_VERSION_NUM;

	psSaveProduction = (SAVE_PRODUCTION *) (pFileData + PRODUCTION_HEADER_SIZE);
	//save each production run
	for (factoryType = 0; factoryType < NUM_FACTORY_TYPES; factoryType++)
	{
		for (factoryNum = 0; factoryNum < MAX_FACTORY; factoryNum++)
		{
			for (runNum = 0; runNum < MAX_PROD_RUN; runNum++)
			{
				psCurrentProd = &asProductionRun[factoryType][factoryNum][runNum];
				psSaveProduction->quantity = psCurrentProd->quantity;
				psSaveProduction->built = psCurrentProd->built;
				psSaveProduction->multiPlayerID = UDWORD_MAX;
				if (psCurrentProd->psTemplate != NULL)
				{
					psSaveProduction->multiPlayerID = psCurrentProd->psTemplate->multiPlayerID;
				}

				/* SAVE_PRODUCTION */
				endian_udword(&psSaveProduction->multiPlayerID);

				psSaveProduction = (SAVE_PRODUCTION *)((char *)psSaveProduction + 	sizeof(SAVE_PRODUCTION));
			}
		}
	}

	/* PRODUCTION_SAVEHEADER */
	endian_udword(&psHeader->version);

	if (!saveFile(pFileName, pFileData, fileSize))
	{
		return FALSE;
	}
	FREE(pFileData);

	return TRUE;
}
//#endif



// -----------------------------------------------------------------------------------------
BOOL loadSaveStructLimits(char *pFileData, UDWORD filesize)
{
#ifdef ALLOWSAVE
	STRUCTLIMITS_SAVEHEADER		*psHeader;

	// Check the file type
	psHeader = (STRUCTLIMITS_SAVEHEADER *)pFileData;
	if (psHeader->aFileType[0] != 'l' || psHeader->aFileType[1] != 'm' ||
		psHeader->aFileType[2] != 't' || psHeader->aFileType[3] != 's')
	{
		debug( LOG_ERROR, "loadSaveStructLimits: Incorrect file type" );
		abort();
		return FALSE;
	}

	//increment to the start of the data
	pFileData += STRUCTLIMITS_HEADER_SIZE;

	/* STRUCTLIMITS_SAVEHEADER */
	endian_udword(&psHeader->version);
	endian_udword(&psHeader->quantity);

	// Check the file version
	if ((psHeader->version >= VERSION_15) && (psHeader->version <= VERSION_19))
	{
		if (!loadSaveStructLimitsV19(pFileData, filesize, psHeader->quantity))
		{
			return FALSE;
		}
	}
	else if (psHeader->version <= CURRENT_VERSION_NUM)
	{
		if (!loadSaveStructLimitsV(pFileData, filesize, psHeader->quantity))
		{
			return FALSE;
		}
	}
	else
#endif
	{
		debug( LOG_ERROR, "loadSaveStructLimits: Incorrect file format version" );
		abort();
		return FALSE;
	}
	return TRUE;
}

// -----------------------------------------------------------------------------------------
#ifdef ALLOWSAVE	  // !?**@?!
/* code specific to version 2 of saveStructLimits */
BOOL loadSaveStructLimitsV19(char *pFileData, UDWORD filesize, UDWORD numLimits)
{
	SAVE_STRUCTLIMITS_V2	*psSaveLimits;
	UDWORD					count, statInc;
	BOOL					found;
	STRUCTURE_STATS			*psStats;
	int SkippedRecords=0;

	psSaveLimits = (SAVE_STRUCTLIMITS_V2 *) MALLOC (sizeof(SAVE_STRUCTLIMITS_V2));
	if (!psSaveLimits)
	{
		debug( LOG_ERROR, "Out of memory" );
		abort();
		return FALSE;
	}

	if ((sizeof(SAVE_STRUCTLIMITS_V2) * numLimits + STRUCTLIMITS_HEADER_SIZE) >
		filesize)
	{
		debug( LOG_ERROR, "loadSaveStructLimits: unexpected end of file" );
		abort();
		return FALSE;
	}

	// Load in the data
	for (count = 0; count < numLimits; count ++,
									pFileData += sizeof(SAVE_STRUCTLIMITS_V2))
	{
		memcpy(psSaveLimits, pFileData, sizeof(SAVE_STRUCTLIMITS_V2));

		//get the stats for this structure
		found = FALSE;
		for (statInc = 0; statInc < numStructureStats; statInc++)
		{
			psStats = asStructureStats + statInc;
			//loop until find the same name
			if (!strcmp(psStats->pName, psSaveLimits->name))
			{
				found = TRUE;
				break;
			}
		}
		//if haven't found the structure - ignore this record!
		if (!found)
		{
			debug( LOG_ERROR, "The structure no longer exists. The limits have not been set! - %s", psSaveLimits->name );
			abort();
			continue;
		}

		if (psSaveLimits->player < MAX_PLAYERS)
		{
			asStructLimits[psSaveLimits->player][statInc].limit = psSaveLimits->limit;
		}
		else
		{
			SkippedRecords++;
		}

	}

	if (SkippedRecords>0)
	{
		debug( LOG_ERROR, "Skipped %d records in structure limits due to bad player number\n", SkippedRecords );
		abort();
	}
	FREE(psSaveLimits);
	return TRUE;
}

// -----------------------------------------------------------------------------------------
BOOL loadSaveStructLimitsV(char *pFileData, UDWORD filesize, UDWORD numLimits)
{
	SAVE_STRUCTLIMITS		*psSaveLimits;
	UDWORD					count, statInc;
	BOOL					found;
	STRUCTURE_STATS			*psStats;
	int SkippedRecords=0;

	psSaveLimits = (SAVE_STRUCTLIMITS*) MALLOC (sizeof(SAVE_STRUCTLIMITS));
	if (!psSaveLimits)
	{
		debug( LOG_ERROR, "Out of memory" );
		abort();
		return FALSE;
	}

	if ((sizeof(SAVE_STRUCTLIMITS) * numLimits + STRUCTLIMITS_HEADER_SIZE) >
		filesize)
	{
		debug( LOG_ERROR, "loadSaveStructLimits: unexpected end of file" );
		abort();
		return FALSE;
	}

	// Load in the data
	for (count = 0; count < numLimits; count ++,
									pFileData += sizeof(SAVE_STRUCTLIMITS))
	{
		memcpy(psSaveLimits, pFileData, sizeof(SAVE_STRUCTLIMITS));

		//get the stats for this structure
		found = FALSE;
		for (statInc = 0; statInc < numStructureStats; statInc++)
		{
			psStats = asStructureStats + statInc;
			//loop until find the same name
			if (!strcmp(psStats->pName, psSaveLimits->name))
			{
				found = TRUE;
				break;
			}
		}
		//if haven't found the structure - ignore this record!
		if (!found)
		{
			debug( LOG_ERROR, "The structure no longer exists. The limits have not been set! - %s", psSaveLimits->name );
			abort();
			continue;
		}

		if (psSaveLimits->player < MAX_PLAYERS)
		{
			asStructLimits[psSaveLimits->player][statInc].limit = psSaveLimits->limit;
		}
		else
		{
			SkippedRecords++;
		}

	}

	if (SkippedRecords>0)
	{
		debug( LOG_ERROR, "Skipped %d records in structure limits due to bad player number\n", SkippedRecords );
		abort();
	}
	FREE(psSaveLimits);
	return TRUE;
}
// -----------------------------------------------------------------------------------------
/*
Writes the list of structure limits to a file
*/
BOOL writeStructLimitsFile(char *pFileName)
{
	char *pFileData = NULL;
	UDWORD						fileSize, totalLimits=0, i, player;
	STRUCTLIMITS_SAVEHEADER		*psHeader;
	SAVE_STRUCTLIMITS			*psSaveLimit;
	STRUCTURE_STATS				*psStructStats;
	BOOL status = TRUE;

	totalLimits = numStructureStats * MAX_PLAYERS;

	// Allocate the data buffer
	fileSize = STRUCTLIMITS_HEADER_SIZE + (totalLimits * (sizeof(SAVE_STRUCTLIMITS)));
	pFileData = (char*)MALLOC(fileSize);
	if (pFileData == NULL)
	{
		debug( LOG_ERROR, "Out of memory" );
		abort();
		return FALSE;
	}

	// Put the file header on the file
	psHeader = (STRUCTLIMITS_SAVEHEADER *)pFileData;
	psHeader->aFileType[0] = 'l';
	psHeader->aFileType[1] = 'm';
	psHeader->aFileType[2] = 't';
	psHeader->aFileType[3] = 's';
	psHeader->version = CURRENT_VERSION_NUM;
	psHeader->quantity = totalLimits;

	psSaveLimit = (SAVE_STRUCTLIMITS*)(pFileData + STRUCTLIMITS_HEADER_SIZE);

	// Put the data into the buffer
	for (player = 0; player < MAX_PLAYERS ; player++)
	{
		psStructStats = asStructureStats;
		for(i = 0; i < numStructureStats; i++, psStructStats++)
		{
			strcpy(psSaveLimit->name, psStructStats->pName);
			psSaveLimit->limit = asStructLimits[player][i].limit;
			psSaveLimit->player = (UBYTE)player;
			psSaveLimit = (SAVE_STRUCTLIMITS *)((char *)psSaveLimit + sizeof(SAVE_STRUCTLIMITS));
		}
	}

	/* STRUCTLIMITS_SAVEHEADER */
	endian_udword(&psHeader->version);
	endian_udword(&psHeader->quantity);

	// Write the data to the file
	if (pFileData != NULL) {
		status = saveFile(pFileName, pFileData, fileSize);
		FREE(pFileData);
		return status;
	}
	return FALSE;
}

BOOL loadSaveCommandLists(char *pFileData, UDWORD filesize)
{
	COMMAND_SAVEHEADER		*psHeader;

	// Check the file type
	psHeader = (COMMAND_SAVEHEADER *)pFileData;
	if (psHeader->aFileType[0] != 'c' || psHeader->aFileType[1] != 'm' ||
		psHeader->aFileType[2] != 'n' || psHeader->aFileType[3] != 'd')
	{
		debug( LOG_ERROR, "loadSaveCommandList: Incorrect file type" );
		abort();
		return FALSE;
	}

	/* COMMAND_SAVEHEADER */
	endian_udword(&psHeader->version);
	endian_udword(&psHeader->quantity);

	//increment to the start of the data
	pFileData += COMMAND_HEADER_SIZE;

	// Check the file version
	if (psHeader->version <= CURRENT_VERSION_NUM)
	{
		if (!loadSaveCommandListsV(pFileData, filesize, psHeader->quantity))
		{
			return FALSE;
		}
	}
	else
	{
		debug( LOG_ERROR, "loadSaveCommandLists: Incorrect file format version" );
		abort();
		return FALSE;
	}
	return TRUE;
}


// -----------------------------------------------------------------------------------------
BOOL loadSaveCommandListsV(char *pFileData, UDWORD filesize, UDWORD numDroids)
{
	SAVE_COMMAND		*psSaveCommand;
	DROID				*psDroid;
	UDWORD				count;

	if ((sizeof(SAVE_COMMAND) * numDroids + COMMAND_HEADER_SIZE) >
		filesize)
	{
		debug( LOG_ERROR, "loadSaveCommand: unexpected end of file" );
		abort();
		return FALSE;
	}

	psSaveCommand = (SAVE_COMMAND*)pFileData;
	// Load in the data
	for (count = 0; count < numDroids; count++)
	{
		/* SAVE_COMMAND is COMMAND_SAVE_V20 */
		/* COMMAND_SAVE_V20 */
		endian_udword(&psSaveCommand[count].droidID);

		if (psSaveCommand[count].droidID != UDWORD_MAX)
		{
			psDroid = (DROID*)getBaseObjFromId(psSaveCommand[count].droidID);
			cmdDroidSetDesignator(psDroid);
		}
	}

	return TRUE;
}
// -----------------------------------------------------------------------------------------
/*
Writes the command lists to a file
*/
BOOL writeCommandLists(char *pFileName)
{
	char *pFileData = NULL;
	DROID						*psDroid;
	UDWORD						fileSize, totalDroids=0, player;
	COMMAND_SAVEHEADER			*psHeader;
	SAVE_COMMAND				*psSaveCommand;
	BOOL status = TRUE;

/*not list
	//total all the droids in the commndlists
	for (player = 0; player < MAX_PLAYERS; player++)
	{
		for (psDroid = apsCmdDesignator[player]; psDroid != NULL; psDroid = psDroid->psNext)
		{
			totalDroids++;
		}
	}
*/
	totalDroids = MAX_PLAYERS;

	// Allocate the data buffer
	fileSize = COMMAND_HEADER_SIZE + (totalDroids * (sizeof(SAVE_COMMAND)));
	pFileData = (char*)MALLOC(fileSize);
	if (pFileData == NULL)
	{
		debug( LOG_ERROR, "Out of memory" );
		abort();
		return FALSE;
	}

	// Put the file header on the file
	psHeader = (COMMAND_SAVEHEADER *)pFileData;
	psHeader->aFileType[0] = 'c';
	psHeader->aFileType[1] = 'm';
	psHeader->aFileType[2] = 'n';
	psHeader->aFileType[3] = 'd';
	psHeader->version = CURRENT_VERSION_NUM;
	psHeader->quantity = totalDroids;

	psSaveCommand = (SAVE_COMMAND*)(pFileData + COMMAND_HEADER_SIZE);

	// Put the data into the buffer
	for (player = 0; player < MAX_PLAYERS ; player++)
	{
/*not list
		for (psDroid = apsCmdDesignation[player]; psDroid != NULL; psDroid = psDroid->psNext)
		{
			psSaveCommand->droidID = psDroid->id;
			psSaveCommand = (SAVE_COMMAND*)((char *)psSaveCommand + sizeof(SAVE_COMMAND));
		}
*/
		psDroid = cmdDroidGetDesignator(player);
		if (psDroid != NULL)
		{
			psSaveCommand->droidID = psDroid->id;
		}
		else
		{
			psSaveCommand->droidID = UDWORD_MAX;
		}

		/* SAVE_COMMAND is COMMAND_SAVE_V20 */
		/* COMMAND_SAVE_V20 */
		endian_udword(&psSaveCommand->droidID);

		psSaveCommand = (SAVE_COMMAND*)((char *)psSaveCommand + sizeof(SAVE_COMMAND));
	}

	/* COMMAND_SAVEHEADER */
	endian_udword(&psHeader->version);
	endian_udword(&psHeader->quantity);

	// Write the data to the file
	if (pFileData != NULL) {
		status = saveFile(pFileName, pFileData, fileSize);
		FREE(pFileData);
		return status;
	}
	return FALSE;
}



// -----------------------------------------------------------------------------------------
// write the event state to a file on disk
static BOOL	writeScriptState(char *pFileName)
{
	char	*pBuffer;
	UDWORD	fileSize;

	if (!eventSaveState(3, &pBuffer, &fileSize))
	{
		return FALSE;
	}

	if (!saveFile(pFileName, pBuffer, fileSize))
	{
		return FALSE;
	}
	FREE(pBuffer);
	return TRUE;
}

#endif

// -----------------------------------------------------------------------------------------
// load the script state given a .gam name
BOOL loadScriptState(char *pFileName)
{
	char	*pFileData;
	UDWORD	fileSize;
	BOOL bHashed = FALSE;

	// change the file extension
	pFileName[strlen(pFileName)-4] = (char)0;
	strcat(pFileName, ".es");

	pFileData = DisplayBuffer;
	if (!loadFileToBuffer(pFileName, pFileData, displayBufferSize, &fileSize))
	{
		debug( LOG_ERROR, "loadScriptState: couldn't load %s", pFileName );
		abort();
		return FALSE;
	}

	if (saveGameVersion > VERSION_12)
	{
		bHashed = TRUE;
	}

	if (!eventLoadState(pFileData, fileSize, bHashed))
	{
		return FALSE;
	}

	return TRUE;
}


// -----------------------------------------------------------------------------------------
/* set the global scroll values to use for the save game */
static void setMapScroll(void)
{
	//if loading in a pre version5 then scroll values will not have been set up so set to max poss
	if (width == 0 && height == 0)
	{
		scrollMinX = 0;
		scrollMaxX = mapWidth;
		scrollMinY = 0;
		scrollMaxY = mapHeight;
		return;
	}
	scrollMinX = startX;
	scrollMinY = startY;
	scrollMaxX = startX + width;
	scrollMaxY = startY + height;
	//check not going beyond width/height of map
	if (scrollMaxX > (SDWORD)mapWidth)
	{
		scrollMaxX = mapWidth;
		debug( LOG_NEVER, "scrollMaxX was too big It has been set to map width" );
	}
	if (scrollMaxY > (SDWORD)mapHeight)
	{
		scrollMaxY = mapHeight;
		debug( LOG_NEVER, "scrollMaxY was too big It has been set to map height" );
	}
}


// -----------------------------------------------------------------------------------------
BOOL getSaveObjectName(char *pName)
{
#ifdef RESOURCE_NAMES

	UDWORD		id;

	//check not a user save game
	if (IsScenario)
	{
		//see if the name has a resource associated with it by trying to get the ID for the string
		if (!strresGetIDNum(psStringRes, pName, &id))
		{
			debug( LOG_ERROR, "Cannot find string resource %s", pName );
			abort();
			return FALSE;
		}

		//get the string from the id if one exists
		strcpy(pName, strresGetString(psStringRes, id));
	}
#endif

	return TRUE;
}

// -----------------------------------------------------------------------------------------
/*returns the current type of save game being loaded*/
UDWORD getSaveGameType(void)
{
	return gameType;
}


// -----------------------------------------------------------------------------------------
//copies a Stat name into a destination string for a given stat type and index
static BOOL getNameFromComp(UDWORD compType, char *pDest, UDWORD compIndex)
{
	BASE_STATS	*psStats;

	//allocate the stats pointer
	switch (compType)
	{
	case COMP_BODY:
		psStats = (BASE_STATS *)(asBodyStats + compIndex);
		break;
	case COMP_BRAIN:
		psStats = (BASE_STATS *)(asBrainStats + compIndex);
		break;
	case COMP_PROPULSION:
		psStats = (BASE_STATS *)(asPropulsionStats + compIndex);
		break;
	case COMP_REPAIRUNIT:
		psStats = (BASE_STATS*)(asRepairStats  + compIndex);
		break;
	case COMP_ECM:
		psStats = (BASE_STATS*)(asECMStats + compIndex);
		break;
	case COMP_SENSOR:
		psStats = (BASE_STATS*)(asSensorStats + compIndex);
		break;
	case COMP_CONSTRUCT:
		psStats = (BASE_STATS*)(asConstructStats + compIndex);
		break;
	/*case COMP_PROGRAM:
		psStats = (BASE_STATS*)(asProgramStats + compIndex);
		break;*/
	case COMP_WEAPON:
		psStats = (BASE_STATS*)(asWeaponStats + compIndex);
		break;
	default:
		debug( LOG_ERROR, "Invalid component type - game.c" );
		abort();
		return FALSE;
	}

	//copy the name into the destination string
	strcpy(pDest, psStats->pName);
	return TRUE;
}
// -----------------------------------------------------------------------------------------
// END


// draws the structures onto a completed map preview sprite.
BOOL plotStructurePreview(iTexture *backDropSprite, UBYTE scale, UDWORD offX, UDWORD offY)
{
	SAVE_STRUCTURE				sSave;  // close eyes now.
	SAVE_STRUCTURE				*psSaveStructure = &sSave; // assumes save_struct is larger than all previous ones...
	SAVE_STRUCTURE_V2			*psSaveStructure2 = (SAVE_STRUCTURE_V2*)&sSave;
	SAVE_STRUCTURE_V12			*psSaveStructure12= (SAVE_STRUCTURE_V12*)&sSave;
	SAVE_STRUCTURE_V14			*psSaveStructure14= (SAVE_STRUCTURE_V14*)&sSave;
	SAVE_STRUCTURE_V15			*psSaveStructure15= (SAVE_STRUCTURE_V15*)&sSave;
	SAVE_STRUCTURE_V17			*psSaveStructure17= (SAVE_STRUCTURE_V17*)&sSave;
	SAVE_STRUCTURE_V20			*psSaveStructure20= (SAVE_STRUCTURE_V20*)&sSave;
										// ok you can open them again..

	STRUCT_SAVEHEADER		*psHeader;
	char			aFileName[256];
	UDWORD			xx,yy,x,y,count,fileSize,sizeOfSaveStruture;
	char			*pFileData = NULL;
	LEVEL_DATASET	*psLevel;

	levFindDataSet(game.map, &psLevel);
	strcpy(aFileName,psLevel->apDataFiles[0]);
	aFileName[strlen(aFileName)-4] = '\0';
	strcat(aFileName, "/struct.bjo");

	pFileData = DisplayBuffer;
	if (!loadFileToBuffer(aFileName, pFileData, displayBufferSize, &fileSize))
	{
		debug( LOG_NEVER, "plotStructurePreview: Fail1\n" );
	}

	/* Check the file type */
	psHeader = (STRUCT_SAVEHEADER *)pFileData;
	if (psHeader->aFileType[0] != 's' || psHeader->aFileType[1] != 't' ||
		psHeader->aFileType[2] != 'r' || psHeader->aFileType[3] != 'u')
	{
		debug( LOG_ERROR, "plotStructurePreview: Incorrect file type" );
		abort();
		return FALSE;
	}

	/* STRUCT_SAVEHEADER */
	endian_udword(&psHeader->version);
	endian_udword(&psHeader->quantity);

	//increment to the start of the data
	pFileData += STRUCT_HEADER_SIZE;

	if (psHeader->version < VERSION_12)
	{
		sizeOfSaveStruture = sizeof(SAVE_STRUCTURE_V2);
	}
	else if (psHeader->version < VERSION_14)
	{
		sizeOfSaveStruture = sizeof(SAVE_STRUCTURE_V12);
	}
	else if (psHeader->version <= VERSION_14)
	{
		sizeOfSaveStruture = sizeof(SAVE_STRUCTURE_V14);
	}
	else if (psHeader->version <= VERSION_16)
	{
		sizeOfSaveStruture = sizeof(SAVE_STRUCTURE_V15);
	}
	else if (psHeader->version <= VERSION_19)
	{
		sizeOfSaveStruture = sizeof(SAVE_STRUCTURE_V17);
	}
	else if (psHeader->version <= VERSION_20)
	{
		sizeOfSaveStruture = sizeof(SAVE_STRUCTURE_V20);
	}
	else
	{
		sizeOfSaveStruture = sizeof(SAVE_STRUCTURE);
	}


	/* Load in the structure data */
	for (count = 0; count < psHeader-> quantity; count ++, pFileData += sizeOfSaveStruture)
	{
		if (psHeader->version < VERSION_12)
		{
			memcpy(psSaveStructure2, pFileData, sizeOfSaveStruture);
			xx = (psSaveStructure2->x >>TILE_SHIFT);
			yy = (psSaveStructure2->y >>TILE_SHIFT);
		}
		else if (psHeader->version < VERSION_14)
		{
			memcpy(psSaveStructure12, pFileData, sizeOfSaveStruture);
			xx = (psSaveStructure12->x >>TILE_SHIFT);
			yy = (psSaveStructure12->y >>TILE_SHIFT);
		}
		else if (psHeader->version <= VERSION_14)
		{
			memcpy(psSaveStructure14, pFileData, sizeOfSaveStruture);
			xx = (psSaveStructure14->x >>TILE_SHIFT);
			yy = (psSaveStructure14->y >>TILE_SHIFT);
		}
		else if (psHeader->version <= VERSION_16)
		{
			memcpy(psSaveStructure15, pFileData, sizeOfSaveStruture);
			xx = (psSaveStructure15->x >>TILE_SHIFT);
			yy = (psSaveStructure15->y >>TILE_SHIFT);
		}
		else if (psHeader->version <= VERSION_19)
		{
			memcpy(psSaveStructure17, pFileData, sizeOfSaveStruture);
			xx = (psSaveStructure17->x >>TILE_SHIFT);
			yy = (psSaveStructure17->y >>TILE_SHIFT);
		}
		else if (psHeader->version <= VERSION_20)
		{
			memcpy(psSaveStructure20, pFileData, sizeOfSaveStruture);
			xx = (psSaveStructure20->x >>TILE_SHIFT);
			yy = (psSaveStructure20->y >>TILE_SHIFT);
		}
		else
		{
			memcpy(psSaveStructure, pFileData, sizeOfSaveStruture);
			xx = (psSaveStructure->x >>TILE_SHIFT);
			yy = (psSaveStructure->y >>TILE_SHIFT);
		}

		for(x = (xx*scale);x < (xx*scale)+scale ;x++)
		{
			for(y = (yy*scale);y< (yy*scale)+scale ;y++)
			{
				backDropSprite->bmp[( (offY+y)*BACKDROP_WIDTH)+x+offX]=COL_RED;
			}
		}
	}
	return TRUE;

}
//======================================================
//draws stuff into our newer bitmap.
BOOL plotStructurePreview16(char *backDropSprite, UBYTE scale, UDWORD offX, UDWORD offY)
{
	SAVE_STRUCTURE				sSave;  // close eyes now.
	SAVE_STRUCTURE				*psSaveStructure = &sSave; // assumes save_struct is larger than all previous ones...
	SAVE_STRUCTURE_V2			*psSaveStructure2 = (SAVE_STRUCTURE_V2*)&sSave;
	SAVE_STRUCTURE_V12			*psSaveStructure12= (SAVE_STRUCTURE_V12*)&sSave;
	SAVE_STRUCTURE_V14			*psSaveStructure14= (SAVE_STRUCTURE_V14*)&sSave;
	SAVE_STRUCTURE_V15			*psSaveStructure15= (SAVE_STRUCTURE_V15*)&sSave;
	SAVE_STRUCTURE_V17			*psSaveStructure17= (SAVE_STRUCTURE_V17*)&sSave;
	SAVE_STRUCTURE_V20			*psSaveStructure20= (SAVE_STRUCTURE_V20*)&sSave;
										// ok you can open them again..

	STRUCT_SAVEHEADER		*psHeader;
	char			aFileName[256];
	UDWORD			xx,yy,x,y,count,fileSize,sizeOfSaveStruture;
	char			*pFileData = NULL;
	LEVEL_DATASET	*psLevel;

	levFindDataSet(game.map, &psLevel);
	strcpy(aFileName,psLevel->apDataFiles[0]);
	aFileName[strlen(aFileName)-4] = '\0';
	strcat(aFileName, "/struct.bjo");

	pFileData = DisplayBuffer;
	if (!loadFileToBuffer(aFileName, pFileData, displayBufferSize, &fileSize))
	{
		debug( LOG_NEVER, "plotStructurePreview16: Fail1\n" );
	}

	/* Check the file type */
	psHeader = (STRUCT_SAVEHEADER *)pFileData;
	if (psHeader->aFileType[0] != 's' || psHeader->aFileType[1] != 't' ||
		psHeader->aFileType[2] != 'r' || psHeader->aFileType[3] != 'u')
	{
		debug( LOG_ERROR, "plotStructurePreview16: Incorrect file type" );
		abort();
		return FALSE;
	}

	/* STRUCT_SAVEHEADER */
	endian_udword(&psHeader->version);
	endian_udword(&psHeader->quantity);

	//increment to the start of the data
	pFileData += STRUCT_HEADER_SIZE;

	if (psHeader->version < VERSION_12)
	{
		sizeOfSaveStruture = sizeof(SAVE_STRUCTURE_V2);
	}
	else if (psHeader->version < VERSION_14)
	{
		sizeOfSaveStruture = sizeof(SAVE_STRUCTURE_V12);
	}
	else if (psHeader->version <= VERSION_14)
	{
		sizeOfSaveStruture = sizeof(SAVE_STRUCTURE_V14);
	}
	else if (psHeader->version <= VERSION_16)
	{
		sizeOfSaveStruture = sizeof(SAVE_STRUCTURE_V15);
	}
	else if (psHeader->version <= VERSION_19)
	{
		sizeOfSaveStruture = sizeof(SAVE_STRUCTURE_V17);
	}
	else if (psHeader->version <= VERSION_20)
	{
		sizeOfSaveStruture = sizeof(SAVE_STRUCTURE_V20);
	}
	else
	{
		sizeOfSaveStruture = sizeof(SAVE_STRUCTURE);
	}


	/* Load in the structure data */
	for (count = 0; count < psHeader-> quantity; count ++, pFileData += sizeOfSaveStruture)
	{
		if (psHeader->version < VERSION_12)
		{
			memcpy(psSaveStructure2, pFileData, sizeOfSaveStruture);

			/* STRUCTURE_SAVE_V2 includes OBJECT_SAVE_V19 */
			endian_sdword(&psSaveStructure2->currentBuildPts);
			endian_udword(&psSaveStructure2->body);
			endian_udword(&psSaveStructure2->armour);
			endian_udword(&psSaveStructure2->resistance);
			endian_udword(&psSaveStructure2->dummy1);
			endian_udword(&psSaveStructure2->subjectInc);
			endian_udword(&psSaveStructure2->timeStarted);
			endian_udword(&psSaveStructure2->output);
			endian_udword(&psSaveStructure2->capacity);
			endian_udword(&psSaveStructure2->quantity);
			/* OBJECT_SAVE_V19 */
			endian_udword(&psSaveStructure2->id);
			endian_udword(&psSaveStructure2->x);
			endian_udword(&psSaveStructure2->y);
			endian_udword(&psSaveStructure2->z);
			endian_udword(&psSaveStructure2->direction);
			endian_udword(&psSaveStructure2->player);
			endian_udword(&psSaveStructure2->burnStart);
			endian_udword(&psSaveStructure2->burnDamage);

			xx = (psSaveStructure2->x >>TILE_SHIFT);
			yy = (psSaveStructure2->y >>TILE_SHIFT);
		}
		else if (psHeader->version < VERSION_14)
		{
			memcpy(psSaveStructure12, pFileData, sizeOfSaveStruture);

			/* STRUCTURE_SAVE_V12 includes STRUCTURE_SAVE_V2 */
			endian_udword(&psSaveStructure12->factoryInc);
			endian_udword(&psSaveStructure12->powerAccrued);
			endian_udword(&psSaveStructure12->droidTimeStarted);
			endian_udword(&psSaveStructure12->timeToBuild);
			endian_udword(&psSaveStructure12->timeStartHold);
			/* STRUCTURE_SAVE_V2 includes OBJECT_SAVE_V19 */
			endian_sdword(&psSaveStructure12->currentBuildPts);
			endian_udword(&psSaveStructure12->body);
			endian_udword(&psSaveStructure12->armour);
			endian_udword(&psSaveStructure12->resistance);
			endian_udword(&psSaveStructure12->dummy1);
			endian_udword(&psSaveStructure12->subjectInc);
			endian_udword(&psSaveStructure12->timeStarted);
			endian_udword(&psSaveStructure12->output);
			endian_udword(&psSaveStructure12->capacity);
			endian_udword(&psSaveStructure12->quantity);
			/* OBJECT_SAVE_V19 */
			endian_udword(&psSaveStructure12->id);
			endian_udword(&psSaveStructure12->x);
			endian_udword(&psSaveStructure12->y);
			endian_udword(&psSaveStructure12->z);
			endian_udword(&psSaveStructure12->direction);
			endian_udword(&psSaveStructure12->player);
			endian_udword(&psSaveStructure12->burnStart);
			endian_udword(&psSaveStructure12->burnDamage);

			xx = (psSaveStructure12->x >>TILE_SHIFT);
			yy = (psSaveStructure12->y >>TILE_SHIFT);
		}
		else if (psHeader->version <= VERSION_14)
		{
			memcpy(psSaveStructure14, pFileData, sizeOfSaveStruture);

			/* STRUCTURE_SAVE_V14 includes STRUCTURE_SAVE_V12 */
			/* STRUCTURE_SAVE_V12 includes STRUCTURE_SAVE_V2 */
			endian_udword(&psSaveStructure14->factoryInc);
			endian_udword(&psSaveStructure14->powerAccrued);
			endian_udword(&psSaveStructure14->droidTimeStarted);
			endian_udword(&psSaveStructure14->timeToBuild);
			endian_udword(&psSaveStructure14->timeStartHold);
			/* STRUCTURE_SAVE_V2 includes OBJECT_SAVE_V19 */
			endian_sdword(&psSaveStructure14->currentBuildPts);
			endian_udword(&psSaveStructure14->body);
			endian_udword(&psSaveStructure14->armour);
			endian_udword(&psSaveStructure14->resistance);
			endian_udword(&psSaveStructure14->dummy1);
			endian_udword(&psSaveStructure14->subjectInc);
			endian_udword(&psSaveStructure14->timeStarted);
			endian_udword(&psSaveStructure14->output);
			endian_udword(&psSaveStructure14->capacity);
			endian_udword(&psSaveStructure14->quantity);
			/* OBJECT_SAVE_V19 */
			endian_udword(&psSaveStructure14->id);
			endian_udword(&psSaveStructure14->x);
			endian_udword(&psSaveStructure14->y);
			endian_udword(&psSaveStructure14->z);
			endian_udword(&psSaveStructure14->direction);
			endian_udword(&psSaveStructure14->player);
			endian_udword(&psSaveStructure14->burnStart);
			endian_udword(&psSaveStructure14->burnDamage);

			xx = (psSaveStructure14->x >>TILE_SHIFT);
			yy = (psSaveStructure14->y >>TILE_SHIFT);
		}
		else if (psHeader->version <= VERSION_16)
		{
			memcpy(psSaveStructure15, pFileData, sizeOfSaveStruture);

			/* STRUCTURE_SAVE_V15 includes STRUCTURE_SAVE_V14 */
			/* STRUCTURE_SAVE_V14 includes STRUCTURE_SAVE_V12 */
			/* STRUCTURE_SAVE_V12 includes STRUCTURE_SAVE_V2 */
			endian_udword(&psSaveStructure15->factoryInc);
			endian_udword(&psSaveStructure15->powerAccrued);
			endian_udword(&psSaveStructure15->droidTimeStarted);
			endian_udword(&psSaveStructure15->timeToBuild);
			endian_udword(&psSaveStructure15->timeStartHold);
			/* STRUCTURE_SAVE_V2 includes OBJECT_SAVE_V19 */
			endian_sdword(&psSaveStructure15->currentBuildPts);
			endian_udword(&psSaveStructure15->body);
			endian_udword(&psSaveStructure15->armour);
			endian_udword(&psSaveStructure15->resistance);
			endian_udword(&psSaveStructure15->dummy1);
			endian_udword(&psSaveStructure15->subjectInc);
			endian_udword(&psSaveStructure15->timeStarted);
			endian_udword(&psSaveStructure15->output);
			endian_udword(&psSaveStructure15->capacity);
			endian_udword(&psSaveStructure15->quantity);
			/* OBJECT_SAVE_V19 */
			endian_udword(&psSaveStructure15->id);
			endian_udword(&psSaveStructure15->x);
			endian_udword(&psSaveStructure15->y);
			endian_udword(&psSaveStructure15->z);
			endian_udword(&psSaveStructure15->direction);
			endian_udword(&psSaveStructure15->player);
			endian_udword(&psSaveStructure15->burnStart);
			endian_udword(&psSaveStructure15->burnDamage);

			xx = (psSaveStructure15->x >>TILE_SHIFT);
			yy = (psSaveStructure15->y >>TILE_SHIFT);
		}
		else if (psHeader->version <= VERSION_19)
		{
			memcpy(psSaveStructure17, pFileData, sizeOfSaveStruture);

			/* STRUCTURE_SAVE_V17 includes STRUCTURE_SAVE_V15 */
			endian_sword(&psSaveStructure17->currentPowerAccrued);
			/* STRUCTURE_SAVE_V15 includes STRUCTURE_SAVE_V14 */
			/* STRUCTURE_SAVE_V14 includes STRUCTURE_SAVE_V12 */
			/* STRUCTURE_SAVE_V12 includes STRUCTURE_SAVE_V2 */
			endian_udword(&psSaveStructure17->factoryInc);
			endian_udword(&psSaveStructure17->powerAccrued);
			endian_udword(&psSaveStructure17->droidTimeStarted);
			endian_udword(&psSaveStructure17->timeToBuild);
			endian_udword(&psSaveStructure17->timeStartHold);
			/* STRUCTURE_SAVE_V2 includes OBJECT_SAVE_V19 */
			endian_sdword(&psSaveStructure17->currentBuildPts);
			endian_udword(&psSaveStructure17->body);
			endian_udword(&psSaveStructure17->armour);
			endian_udword(&psSaveStructure17->resistance);
			endian_udword(&psSaveStructure17->dummy1);
			endian_udword(&psSaveStructure17->subjectInc);
			endian_udword(&psSaveStructure17->timeStarted);
			endian_udword(&psSaveStructure17->output);
			endian_udword(&psSaveStructure17->capacity);
			endian_udword(&psSaveStructure17->quantity);
			/* OBJECT_SAVE_V19 */
			endian_udword(&psSaveStructure17->id);
			endian_udword(&psSaveStructure17->x);
			endian_udword(&psSaveStructure17->y);
			endian_udword(&psSaveStructure17->z);
			endian_udword(&psSaveStructure17->direction);
			endian_udword(&psSaveStructure17->player);
			endian_udword(&psSaveStructure17->burnStart);
			endian_udword(&psSaveStructure17->burnDamage);

			xx = (psSaveStructure17->x >>TILE_SHIFT);
			yy = (psSaveStructure17->y >>TILE_SHIFT);
		}
		else if (psHeader->version <= VERSION_20)
		{
			memcpy(psSaveStructure20, pFileData, sizeOfSaveStruture);

			/* STRUCTURE_SAVE_V20 includes OBJECT_SAVE_V20 */
			endian_sdword(&psSaveStructure20->currentBuildPts);
			endian_udword(&psSaveStructure20->body);
			endian_udword(&psSaveStructure20->armour);
			endian_udword(&psSaveStructure20->resistance);
			endian_udword(&psSaveStructure20->dummy1);
			endian_udword(&psSaveStructure20->subjectInc);
			endian_udword(&psSaveStructure20->timeStarted);
			endian_udword(&psSaveStructure20->output);
			endian_udword(&psSaveStructure20->capacity);
			endian_udword(&psSaveStructure20->quantity);
			endian_udword(&psSaveStructure20->factoryInc);
			endian_udword(&psSaveStructure20->powerAccrued);
			endian_udword(&psSaveStructure20->dummy2);
			endian_udword(&psSaveStructure20->droidTimeStarted);
			endian_udword(&psSaveStructure20->timeToBuild);
			endian_udword(&psSaveStructure20->timeStartHold);
			endian_sword(&psSaveStructure20->currentPowerAccrued);
			/* OBJECT_SAVE_V20 */
			endian_udword(&psSaveStructure20->id);
			endian_udword(&psSaveStructure20->x);
			endian_udword(&psSaveStructure20->y);
			endian_udword(&psSaveStructure20->z);
			endian_udword(&psSaveStructure20->direction);
			endian_udword(&psSaveStructure20->player);
			endian_udword(&psSaveStructure20->burnStart);
			endian_udword(&psSaveStructure20->burnDamage);

			xx = (psSaveStructure20->x >>TILE_SHIFT);
			yy = (psSaveStructure20->y >>TILE_SHIFT);
		}
		else
		{
			memcpy(psSaveStructure, pFileData, sizeOfSaveStruture);

			/* SAVE_STRUCTURE is STRUCTURE_SAVE_V21 */
			/* STRUCTURE_SAVE_V21 includes STRUCTURE_SAVE_V20 */
			endian_udword(&psSaveStructure->commandId);
			/* STRUCTURE_SAVE_V20 includes OBJECT_SAVE_V20 */
			endian_sdword(&psSaveStructure->currentBuildPts);
			endian_udword(&psSaveStructure->body);
			endian_udword(&psSaveStructure->armour);
			endian_udword(&psSaveStructure->resistance);
			endian_udword(&psSaveStructure->dummy1);
			endian_udword(&psSaveStructure->subjectInc);
			endian_udword(&psSaveStructure->timeStarted);
			endian_udword(&psSaveStructure->output);
			endian_udword(&psSaveStructure->capacity);
			endian_udword(&psSaveStructure->quantity);
			endian_udword(&psSaveStructure->factoryInc);
			endian_udword(&psSaveStructure->powerAccrued);
			endian_udword(&psSaveStructure->dummy2);
			endian_udword(&psSaveStructure->droidTimeStarted);
			endian_udword(&psSaveStructure->timeToBuild);
			endian_udword(&psSaveStructure->timeStartHold);
			endian_sword(&psSaveStructure->currentPowerAccrued);
			/* OBJECT_SAVE_V20 */
			endian_udword(&psSaveStructure->id);
			endian_udword(&psSaveStructure->x);
			endian_udword(&psSaveStructure->y);
			endian_udword(&psSaveStructure->z);
			endian_udword(&psSaveStructure->direction);
			endian_udword(&psSaveStructure->player);
			endian_udword(&psSaveStructure->burnStart);
			endian_udword(&psSaveStructure->burnDamage);

			xx = (psSaveStructure->x >>TILE_SHIFT);
			yy = (psSaveStructure->y >>TILE_SHIFT);
		}

		for(x = (xx*scale);x < (xx*scale)+scale ;x++)
		{
			for(y = (yy*scale);y< (yy*scale)+scale ;y++)
			{
				// 0xff0000 = red. use COL_LIGHTRED instead?
				backDropSprite[3 * (((offY + y) * BACKDROP_HACK_WIDTH) + x + offX)] = 0xff;
				backDropSprite[3 * (((offY + y) * BACKDROP_HACK_WIDTH) + x + offX) + 1] = 0x0;
				backDropSprite[3 * (((offY + y) * BACKDROP_HACK_WIDTH) + x + offX) + 2] = 0x0;
			}
		}
	}
	return TRUE;

}
