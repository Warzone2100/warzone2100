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

	$Revision$
	$Id$
	$HeadURL$
*/

#ifndef __INCLUDED_HEIGHTMAP_H__
#define __INCLUDED_HEIGHTMAP_H__

#include "pcxhandler.h"
#include "bmphandler.h"
#include "grdland.h"
#include <list>
#include "chnkio.h"
#include "devmap.h"
#include "fileparse.h"
#include "dibdraw.h"
#include "directx.h"
#include "geometry.h"
#include "ddimage.h"

#define MAXTILES 8192		// Max number of tiles.

#define GRABRADIUS	8
#define MAXSELECTEDVERTS	256
#define MAX3DOBJECTS	512
#define OVERSCAN	(4)
#define MAX_TILETEXTURES (8192)
#define MAX_SCRIPTNAME (32)	// Max number of characters for an object instances script name.
#define MAX_SCROLLLIMITS (16) // Max number of scroll limit definitions.
#define MAX_GATEWAYS (256)	// Max number of gateways.

#define MAX_MAPWIDTH (250)
#define MAX_MAPHEIGHT (250)

struct CWorldInfo {
	DWORD NumStructures[MAX_PLAYERS];
	DWORD NumWalls[MAX_PLAYERS];
	DWORD NumDroids[MAX_PLAYERS];
	DWORD NumFeatures;
	DWORD NumObjects;
};


// Gateway vertex types.
enum {
	GATE_POSSTART,
	GATE_POSEND,
};

// Gateway flags.
#define GATEF_GROUND 1
#define GATEF_WATER 2
#define GATEF_AIR 4

struct GateWay {
	BOOL Selected;
	UWORD Flags;
	UBYTE x0,y0;
	UBYTE x1,y1;
};


#define MAX_OBJNAMES	2048

struct ObjNames {
	char *IDString;
	char *Name;
};


struct SelVertex {
	DWORD	TileIndex;
	DWORD	VertIndex;
	float	z;
};

extern SelVertex	SelVerticies[MAXSELECTEDVERTS];

/*
 Tile flags.

 11111100 00000000
 54321098 76543210
 tttttttt .rryxfvh

  rr	Texture rotation 0=0, 1=90, 2=180, 3=270.
  v		Vertex ( triangle direction ) flip.
  y		Y texture flip.
  x     X texture flip.
  f     Diagonal flip.
  h     Hide.
  .		Spare
  t		Tile type ie Grass,Sand etc...
*/
// Now defined in TileTypes.h
//#define TF_SHOW			0
//#define	TF_HIDE			1
//#define TF_VERTEXFLIP	2
//#define TF_TEXTUREFLIPX	4
//#define TF_TEXTUREFLIPY	8
//#define TF_TEXTUREROTMASK	0x30
//#define TF_TEXTUREROTSHIFT	4
//#define TF_TEXTUREROT90		(1<<TF_TEXTUREROTSHIFT)
//#define TF_TEXTUREROT180	(2<<TF_TEXTUREROTSHIFT)
//#define	TF_TEXTUREROT270	(3<<TF_TEXTUREROTSHIFT)
//#define TF_TEXTURESPARE0	128
//
//#define TF_TYPEMASK			0xff00
//#define TF_TYPESHIFT        8
//#define TF_TYPEGRASS		512
//#define TF_TYPESTONE		1024
//#define TF_TYPESAND			2048
//#define TF_TYPEWATER		4096
//#define TF_TYPESPARE1		8192
//#define TF_TYPESPARE2		16384
//#define TF_TYPESPARE3		32768
//#define TF_TYPEALL			0xff00

//#define TF_ROT90		(1<<TF_TEXTUREROTSHIFT)
//#define TF_ROT180	(2<<TF_TEXTUREROTSHIFT)
//#define	TF_ROT270	(3<<TF_TEXTUREROTSHIFT)


struct CTile
{
	DWORD		TMapID;			// Texture index into list of type TextureDef.
	DWORD		Flags;			// Flags for rotation, flipping etc. ( see TF_.... above ).
	D3DVECTOR	Position;
	UBYTE		Height[4];			// A height for each vertex.
	D3DVECTOR	VertexNormals[4];	// Normal for each vertex ( for shading ).
	D3DVECTOR	FaceNormals[2];	// Normal for each triangle ( for the plane equation ).
	float		Offsets[2];		// Offset for each triangle ( for the plane equation ).
	DWORD		ZoneID;
};

struct TextureDef {
	DWORD		TextureID;
	D3DVALUE	u0,v0;
	D3DVALUE	u1,v1;
	D3DVALUE	u2,v2;
	D3DVALUE	u3,v3;
};

//#define	MAPSIZE	256	//128
#define	DRAWRADIUS	15	//8
//#define	SECTORSPERROW	64
//#define	TILESPERSECTORROW	(MAPSIZE/SECTORSPERROW)
#define	SECTORDRAWRADIUS	15	//8
#define MINDRAWRADIUS 4
#define MAXDRAWRADIUS 32

struct CMapSector {
	D3DVERTEX	BoundingBox[8];	// This sectors bounding box.
	DWORD		TileCount;		// Number of tiles in this sector.
	DWORD		Hidden;			// Number of tiles in this sector that are hidden.
	DWORD		TextureChanges;	// Number of texture state changes in this sector.
	DWORD		*TileIndices;	// List of tile indecies for this sector.
};

struct CTextureList {
	char Name[256];
	DWORD Width;
	DWORD Height;
	DWORD NumTiles;
	DWORD TextureID;
};


struct CTriangle {
	DWORD Flags;
	DWORD TAFrames;
	DWORD TARate;
	DWORD TAWidth;
	DWORD TAHeight;
	D3DVERTEX	v0,v1,v2;
};


enum {
	IMD_FEATURE,
	IMD_STRUCTURE,
	IMD_DROID,
	IMD_OBJECT,
};


struct CScrollLimits {
	DWORD UniqueID;
	char ScriptName[MAX_SCRIPTNAME];
	int MinX,MinZ;
	int MaxX,MaxZ;
};


struct LimitRect {
	float x0,z0;
	float x1,z1;
};


struct C3DObjectInstance {
	DWORD ObjectID;			// ID of C3DObject to draw for this instance.
	DWORD UniqueID;			// Unique ID.
	DWORD PlayerID;			// Player ID.
	char ScriptName[MAX_SCRIPTNAME];	// Script name.
	BOOL Selected;			// Is it selected.
	D3DVECTOR Position;		// Position and rotation.
	D3DVECTOR Rotation;
};


struct C3DObject {
	char *Name;
	char *Description;
	int RealName;
	int TypeID;
	int StructureID;	// Index into structure stats list if it's a structure.
	int PlayerID;		// Player number if it's a structure or a droid.
	BOOL Flanged;
	BOOL Snap;
	int ColourKeyIndex;
	DWORD TextureID;
	DWORD NumTris;
	D3DVECTOR Smallest;
	D3DVECTOR Largest;
	D3DVECTOR Center;
	float Radius;
	float SRadius;
	CTriangle *Triangle;
	C3DObject *AttachedObject;
};



#define MAX_PLAYERS 8		/*Utterly arbitrary at the moment!! */
#define UNITS_PER_PLAYER	5
#define MAX_NAME_SIZE 40


typedef struct _nob_save_header
{
	STRING		aFileType[4];
	UDWORD		version;
	UDWORD		quantity;
} NOB_SAVEHEADER;

typedef struct _nob_entry
{
	DWORD id;
	SDWORD XPos,YPos,ZPos;
	SDWORD XRot,YRot,ZRot;
} NOB_ENTRY;




typedef struct _droid_save_header
{
	STRING		aFileType[4];
	UDWORD		version;
	UDWORD		quantity;
} DROID_SAVEHEADER;

typedef struct _droidinit_save_header
{
	STRING		aFileType[4];
	UDWORD		version;
	UDWORD		quantity;
} DROIDINIT_SAVEHEADER;

typedef struct _struct_save_header
{
	STRING		aFileType[4];
	UDWORD		version;
	UDWORD		quantity;
} STRUCT_SAVEHEADER;

typedef struct _template_save_header
{
	STRING		aFileType[4];
	UDWORD		version;
	UDWORD		quantity;
} TEMPLATE_SAVEHEADER;

typedef struct _feature_save_header
{
	STRING		aFileType[4];
	UDWORD		version;
	UDWORD		quantity;
} FEATURE_SAVEHEADER;


#define MAX_NAME_SIZE 40

#define OBJECT_SAVE \
	STRING				name[MAX_NAME_SIZE]; \
	UDWORD				id; \
	UDWORD				x,y,z; \
	UDWORD				direction; \
	UDWORD				player; \
	BOOL				inFire; \
	UDWORD				burnStart; \
	UDWORD				burnDamage 

enum {
	GTYPE_SCENARIO_START,
	GTYPE_SCENARIO_EXPAND,
	GTYPE_MISSION,
	GTYPE_SAVED,
};

typedef struct _game_save_header
{
	STRING		aFileType[4];
	UDWORD		version;
} GAME_SAVEHEADER;

#define GAME_SAVE_V2 \
	UDWORD	gameTime		/*only need to save the time difference for everything
							  but for now save it this way since need to save something 
	                          for the save_game*/
typedef struct save_game_v2
{
	GAME_SAVE_V2;
} SAVE_GAME_V2;

#define GAME_SAVE_V3 \
	GAME_SAVE_V2; \
	SDWORD	x; 				/*starting x point within the map for this game*/ \
	SDWORD	y;				/*starting y point within the map for this game*/ \
	UDWORD	width;			/*width of the map for this game*/ \
	UDWORD	height			/*height of the map for this game*/

#define MAX_LIMITS 8

#define GAME_SAVE_V4 \
	GAME_SAVE_V3; \
	BOOL IsScenario 		/* TRUE if the game is a scenario created by the editor as apposed*/
							/* to a game saved by a player. */

#define GAME_SAVE_V5 \
	GAME_SAVE_V3; \
	UDWORD GameType; \
	UDWORD ScrollMinX; \
	UDWORD ScrollMinY; \
	UDWORD ScrollMaxX; \
	UDWORD ScrollMaxY

#define MAX_LEVEL_SIZE	20

#define GAME_SAVE_V7 \
	UDWORD	gameTime; 		/*only need to save the time difference for everything
							  but for now save it this way since need to save something 
	                          for the save_game*/ \
	UDWORD GameType; \
	UDWORD ScrollMinX; \
	UDWORD ScrollMinY; \
	UDWORD ScrollMaxX; \
	UDWORD ScrollMaxY; \
	UBYTE  LevelName[MAX_LEVEL_SIZE]

typedef struct _savePower
{
	UDWORD		currentPower;
	UDWORD		extractedPower;
} SAVE_POWER;

//#define GAME_SAVE_V11 \
//	UDWORD	gameTime; \ 
//	UDWORD	GameType;		/* Type of game , one of the GTYPE_... enums. */ \
//	UDWORD	ScrollMinX;		/* Scroll Limits */ \
//	UDWORD	ScrollMinY; \
//	UDWORD	ScrollMaxX; \
//	UDWORD	ScrollMaxY; \
//	STRING	levelName[MAX_LEVEL_SIZE];	/* name of the level to load up when mid game */ \
//	SAVE_POWER	power[MAX_PLAYERS]; \
//	iView currentPlayerPos

typedef struct _save_game
{
	GAME_SAVE_V7;
} SAVE_GAME;


typedef enum COMPONENT_TYPE
{
	//COMP_ARMOUR,
	COMP_BODY,
	COMP_BRAIN,
	//COMP_POWERPLANT,
	COMP_PROPULSION,
	COMP_REPAIRUNIT,
	COMP_ECM,
	COMP_SENSOR,
	COMP_CONSTRUCT,
	COMP_PROGRAM,
	COMP_WEAPON,

	COMP_NUMCOMPONENTS
} COMPONENT_TYPE;

#define NULL_COMP	(-1)

/* The number of components in the asParts / asBits arrays */
#define DROID_MAXCOMP		(COMP_NUMCOMPONENTS - 2)

/* The maximum number of droid weapons and programs */
#define DROID_MAXWEAPS		3
#define DROID_MAXPROGS		3


typedef struct _save_component
{
	STRING				name[MAX_NAME_SIZE];
	UDWORD				hitPoints;
} SAVE_COMPONENT;

typedef struct _save_weapon
{
	STRING				name[MAX_NAME_SIZE];
	UDWORD				hitPoints;
	UDWORD				ammo;
	UDWORD				lastFired;
} SAVE_WEAPON;	

typedef struct _save_droid
{
	OBJECT_SAVE;
	SAVE_COMPONENT		asBits[DROID_MAXCOMP];
	UDWORD				body;
	SDWORD				activeWeapon;		// The currently selected weapon
	UDWORD				numWeaps;
	SAVE_WEAPON			asWeaps[DROID_MAXWEAPS];
	SDWORD				activeProg;			// The currently running program
	UDWORD				numProgs;
	STRING				asProgs[DROID_MAXPROGS][MAX_NAME_SIZE];
} SAVE_DROID;

typedef enum _struct_states
{
	SS_BEING_BUILT,
	SS_BUILT,
	SS_FUNCTION
} STRUCT_STATES;

typedef enum {
	NT_DEFAULTNORMALS,
	NT_FACENORMALS,
	NT_SMOOTHNORMALS,
} NORMALTYPE;

typedef struct _save_template
{
	CHAR				name[MAX_NAME_SIZE];
	UDWORD				ref;
	UDWORD				player;
	SDWORD				asParts[DROID_MAXCOMP];
	UDWORD				numWeaps;
	UDWORD				asWeaps[DROID_MAXWEAPS];
	UDWORD				numProgs;
	UDWORD				asProgs[DROID_MAXPROGS];
} SAVE_TEMPLATE;

typedef struct _save_feature
{
	OBJECT_SAVE;
} SAVE_FEATURE;

typedef struct _save_droidinit
{
	OBJECT_SAVE;
} SAVE_DROIDINIT;

/* Sanity check definitions for the save struct file sizes */
#define GAME_HEADER_SIZE		8
#define DROID_HEADER_SIZE		12
#define DROIDINIT_HEADER_SIZE	12
#define STRUCT_HEADER_SIZE		12
#define TEMPLATE_HEADER_SIZE	12
#define FEATURE_HEADER_SIZE		12

#define	STRUCTURE_STATS_SIZE	25	// Number of fields in one line of structure stats.
#define	FEATURE_STATS_SIZE		11	// Number of fields in one line of feature stats.
#define DROID_TEMPLATE_STATS_SIZE 12 // Number of fields in one line of droid template stats.

typedef struct _save_structure
{
	OBJECT_SAVE;
//	UDWORD				structureInc;
	UBYTE				status;
	UDWORD				currentBuildPts;
	UDWORD				body;
	UDWORD				armour;
	UDWORD				resistance;
	UDWORD				repair;
	SDWORD				subjectInc;
	UDWORD				timeStarted;
	UDWORD				output;
	UDWORD				capacity;
	UDWORD				quantity;
} SAVE_STRUCTURE;


//ALL components and structures and research topics have a tech level to which they belong
typedef enum _tech_level
{
	TECH_LEVEL_ONE,
	TECH_LEVEL_TWO,
	TECH_LEVEL_THREE,
	TECH_LEVEL_ONE_TWO,
	TECH_LEVEL_TWO_THREE,
    TECH_LEVEL_ALL,

	MAX_TECH_LEVELS
} TECH_LEVEL;

//this structure is used to hold the permenant stats for each type of building
struct _structure_stats {
	char StructureName[128];
	char Foundation[128];
	char Type[128];
	char ecmType[128];
	char sensorType[128];
	char IMDName[8][128];
	char BaseIMD[128];

	int		ref;				/* Unique ID of the item */
	int		type;				/* the type of structure */
	TECH_LEVEL	techLevel;		/* technology level of the structure */
	int		terrainType;		/*The type of terrain the structure has to be 
									  built next to - may be none*/
	int		baseWidth;			/*The width of the base in tiles*/
	int		baseBreadth;		/*The breadth of the base in tiles*/
	int		foundationType;		/*The type of foundation for the structure*/
	int		buildPoints;		/*The number of build points required to build
									  the structure*/
	int		height;				/*The height above/below the terrain - negative
									  values denote below the terrain*/
	int		armourValue;		/*The armour value for the structure - can be 
									  upgraded as in droids*/
	int		bodyPoints;			/*The structure's body points - A structure goes
									  off-line when 50% of its body points are lost*/
	int		repairSystem;		/*The repair system points are added to the body
									  points until fully restored . The points are 
									  then added to the Armour Points*/
	int		powerToBuild;		/*How much power the structure requires to build*/ 
	int		minimumPower;		/*The minimum power requirement to start building
								      the structure*/
	int		resistance;			/*The number used to determine whether a 
									  structure can resist an enemy takeover*/
	int		quantityLimit;		/*The maximum number that a player can have - 
									  0 = no limit 1 = only 1 allowed etc*/
	int		sizeModifier;		/*The larger the target, the easier to hit*/

	int		weaponSlots;		/*Number of weapons that can be attached to the
									  building*/
	int		numWeaps;			/*Number of weapons for default */
	int		defaultWeap;		/*The default weapon*/

	int		numFuncs;			/*Number of functions for default*/
	int		defaultFunc;		/*The default function*/
};

typedef enum _feature_type
{
	FEAT_BUILD_WRECK,
	FEAT_HOVER,
	FEAT_TANK,
	FEAT_GEN_ARTE,
	FEAT_OIL_RESOURCE,
	FEAT_BOULDER,
	FEAT_VEHICLE,
	FEAT_DROID,
	FEAT_LOS_OBJ,
} FEATURE_TYPE;


#define STATS_BASE \
	UDWORD			ref;			/* Unique ID of the item */ \
	STRING			*pName			/* Text name of the item */ 

/* Stats for a feature */
struct _feature_stats
{
	STATS_BASE;

	FEATURE_TYPE	subType;		/* The type of feature */

	char IMDName[128];

	UDWORD			baseWidth;			/*The width of the base in tiles*/
	UDWORD			baseBreadth;		/*The breadth of the base in tiles*/

	char featureName[256];
   	char type[256];
   	char compType[256];
 	char compName[256];

	/* component type activated if a FEAT_GEN_ARTE */
//	UDWORD			compType;			// type of component activated
	UDWORD			compIndex;			// index of component

	BOOL			tileDraw;			/* Flag to indicated whether the tile needs to be drawn
										   TRUE = draw tile */
	BOOL			allowLOS;			/* Flag to indicate whether the feature allows the LOS
										   TRUE = can see through the feature */
	BOOL			visibleAtStart;		/* Flag to indicate whether the feature is visible at
										   the start of the mission */
	BOOL			damageable;			// Whether the feature can be blown up
	UDWORD			body;				// Number of body points
	UDWORD			armour;				// Feature armour
};


struct DroidTemplate {
	char Name[256];
};


#define	FIX_X	1
#define	FIX_Y	2
#define FIX_Z	4


// Structures and defines for SeedFill().
typedef int Pixel;		/* 1-channel frame buffer assumed */

struct FRect {			/* window: a discrete 2-D rectangle */
    int x0, y0;			/* xmin and ymin */
    int x1, y1;			/* xmax and ymax (inclusive) */
};

struct Segment {
	int y;			//                                                        
	int xl;			// Filled horizontal segment of scanline y for xl<=x<=xr. 
	int xr;			// Parent segment was on line y-dy.  dy=1 or -1           
	int dy;			//
};

//#define MAX 10000		/* max depth of stack */
//
//#define PUSH(Y, XL, XR, DY)	/* push new segment on stack */ \
//    if (sp<stack+MAX && Y+(DY)>=win->y0 && Y+(DY)<=win->y1) \
//    {sp->y = Y; sp->xl = XL; sp->xr = XR; sp->dy = DY; sp++;}
//
//#define POP(Y, XL, XR, DY)	/* pop segment off stack */ \
//    {sp--; Y = sp->y+(DY = sp->dy); XL = sp->xl; XR = sp->xr;}

class CHeightMap : public CChnkIO
{
	public:
		SLONG m_MapWidth;
		SLONG m_MapHeight;
		CTile *m_MapTiles;

		char m_TileTextureName[256];
		char m_ObjectNames[256];
		int m_BrushHeightMode[16];
		int m_BrushHeight[16];
		int m_BrushRandomRange[16];
		int m_BrushTiles[16][16];
		int m_BrushFlags[16][16];
		int m_TileTypes[128];
		BOOL m_EnablePlayers[MAX_PLAYERS];
		int m_NumNames;
		ObjNames m_ObjNames[MAX_OBJNAMES];
		BOOL m_UseRealNames;
		int m_SelectionBox0;
		int m_SelectionX0;
		int m_SelectionY0;
		int m_SelectionBox1;
		int m_SelectionX1;
		int m_SelectionY1;
		int m_SelectionWidth;
		int m_SelectionHeight;

		BOOL m_TerrainMorph;
		BOOL m_IgnoreDroids;
		BOOL m_Flatten;
		BOOL m_NoObjectSnap;
		DWORD m_NewObjectID;
		char* m_FeatureSet;
		int m_RenderPlayerID;

		CHeightMap(CDirectDraw* DirectDrawView,
		           CGeometry* DirectMaths,
		           CMatManager* MatManager,
		           SLONG MapWidth = 128,
		           SLONG MapHeight = 128,
		           SLONG TileWidth = 128,
		           SLONG TileHeight = 128,
		           SLONG TextureSize = 64);
		~CHeightMap();

		CGrdLandIO *Read(FILE *Stream);
		void EnableGouraud(BOOL Enable);
		BOOL AddTexture(char *TextureName);
		BOOL InitialiseTextures(DWORD NumTextures,char **TextureList,DWORD TextureWidth,DWORD TextureHeight);
		void CopyTexture(UBYTE *SourceBits,int SourcePitch,int SourceX,int SourceY,
								 UBYTE *DestBits,int DestPitch,int DestX,int DestY,int TileSize);
		void InitialiseTextMaps();
		char** GetTextureList();
		DWORD GetNumTextures();
		void SetTextureMap(DWORD TexNum,DWORD PageNum,DWORD x,DWORD y);
		void DrawHeightMap(D3DVECTOR &CameraRotation,D3DVECTOR &CameraPosition);
		void Draw3DGrid(D3DVECTOR &CameraRotation,D3DVECTOR &CameraPosition);
		void Draw3DTiles(D3DVECTOR &CameraRotation,D3DVECTOR &CameraPosition);
		void DrawTile(DWORD TileNum,D3DVECTOR &CameraRotation,D3DVECTOR &CameraPosition);
		void Draw3DSectors(D3DVECTOR &CameraRotation,D3DVECTOR &CameraPosition);
		void Draw3DVerticies(D3DVECTOR &CameraRotation,D3DVECTOR &CameraPosition);
		float GetVisibleRadius();
		void DrawSea(D3DVECTOR &CameraRotation,D3DVECTOR &CameraPosition);
		void Draw2DMap(CDIBDraw *DIBDraw,DDImage **Images,int NumImages,int ScrollX, int ScrollY);
		void DrawTileAttribute(CDIBDraw *DIBDraw,int XPos,int YPos,DWORD Flags);

		int	FindVerticies(D3DVECTOR &CameraRotation,D3DVECTOR &CameraPosition,
								DWORD XPos,DWORD YPos);
		int	SelectFace(D3DVECTOR &CameraRotation,D3DVECTOR &CameraPosition,
								DWORD XPos,DWORD YPos);
		int Select2DFace(DWORD XPos,DWORD YPos,int ScrollX,int ScrollY);

		void SetMapSize(DWORD MapWidth,DWORD MapHeight);
		void SetTileSize(DWORD TileWidth,DWORD TileHeight);
		void SetTextureSize(DWORD TextureWidth,DWORD TextureHeight);
		void SetTileVisible(DWORD TileNum,DWORD Flag);
		DWORD GetTileVisible(DWORD TileNum);
		DWORD GetTileFlags(DWORD MapX,DWORD MapY);
		DWORD GetTileFlags(DWORD TileNum);
		void SetTileFlags(DWORD TileNum,DWORD Flags);
		void SetTileType(DWORD TileNum,DWORD Type);
		BOOL GetTileGateway(int x,int y);
		void SetTileGateway(int x,int y,BOOL IsGateway);
		BOOL GetTileGateway(DWORD TileNum);
		void SetTileGateway(DWORD TileNum,BOOL IsGateway);
		DWORD GetTileType(DWORD MapX,DWORD MapY);
		DWORD GetTileType(DWORD TileNum);
		void SetTextureID(DWORD TileNum,DWORD TMapID);
		void SetVertexFlip(DWORD TileNum,DWORD VertexFlip);
		void SetTextureFlip(DWORD TileNum,BOOL FlipX,BOOL FlipY);
		void SetTextureRotate(DWORD TileNum,DWORD Rotate);
		void RaiseTile(int Index,float dy);
		void SetTileHeight(int Index,float Height);
		float GetTileHeight(int Index);
		void SetVertexHeight(DWORD TileNum,DWORD Index,float y);
		void SetVertexHeight(CTile *Tile,DWORD Index,float y);
		BOOL WriteHeightMap(char *FileName);
		BOOL SetHeightFromBitmap(char *FileName);
		void ApplyRandomness(int Selected,UDWORD SelFlags);
		void FillMap(DWORD Selected,DWORD TextureID,DWORD Type,DWORD Flags);
#if 0
		void FloodFill(SWORD x,SWORD y);
#else
		Pixel PixelRead(int x,int y);
		BOOL PixelCompare(int x,int y,DWORD Tid,DWORD Type,DWORD Flags);
		void PixelWrite(int x,int y,Pixel nv,DWORD Type,DWORD Flags);
		void SeedFill(int x, int y, FRect *win, Pixel nv,DWORD Type,DWORD Flags);
#endif

		inline CTile *GetMapTiles()
		{
			return m_MapTiles;
		}

		unsigned int GetMapWidth()
		{
			return m_MapWidth;
		}

		unsigned int GetMapHeight()
		{
			return m_MapHeight;
		}

		void GetMapSize(unsigned int& MapWidth, unsigned int& MapHeight);
		void GetTileSize(DWORD *TileWidth,DWORD *TileHeight);
		DWORD GetTextureID(DWORD TileNum);
		DWORD GetVertexFlip(DWORD TileNum);
		BOOL GetTextureFlipX(DWORD TileNum);
		BOOL GetTextureFlipY(DWORD TileNum);
		DWORD GetTextureRotate(DWORD TileNum);
		float GetVertexHeight(DWORD TileNum,DWORD Index);
		void GetTextureSize(DWORD *TextureHeight,DWORD *TextureWidth);
		inline DWORD GetHeightScale()
		{
			return m_HeightScale;
		}

		void SetHeightScale(DWORD HeightScale);
		float GetHeight(float x, float y);
		float GetInterpolatedHeight(float xPos,float yPos);
		inline DWORD GetSeaLevel()
		{
			return m_SeaLevel;
		}

		inline void SetSeaLevel(DWORD SeaLevel)
		{
			m_SeaLevel = SeaLevel;
		}

		void FixTileVerticies(CTile *Tile,SLONG x,SLONG z,DWORD Flags);
		void FixTilePositions();
		void FixTileNormals(CTile *Tile);
		void FixTileTextures(CTile *Tile);
		void FixTextureIDS();
//		void SmoothNormals();
		UDWORD GetTileFlipType(SDWORD MapX, SDWORD MapY);
		void AddNormal(SDWORD MapX, SDWORD MapY,UDWORD AddedNormals,D3DVECTOR *SummedVector);
		void SmoothNormals();


		void InitialiseSectors();

		void SwitchTriDirection(DWORD TileNum);
		inline CTile *GetTile(DWORD TileNum)
		{
			return &m_MapTiles[TileNum];
		}

		void SetDrawRadius(SLONG Radius) { m_DrawRadius = Radius; }
		
		inline SLONG GetDrawRadius()
		{
			return m_DrawRadius;
		}

		TextureDef	m_TextureMaps[MAX_TILETEXTURES];	// Should be allocated.

		void InitSmallestLargest(D3DVECTOR &Smallest,D3DVECTOR &Largest);
		void UpdateSmallest(D3DVECTOR &v,D3DVECTOR &Smallest);
		void UpdateLargest(D3DVECTOR &v,D3DVECTOR &Largest);

		// Public functions related to objects.
		BOOL ReadFeatureStats(const char* ScriptFile, const char* IMDDir, const char* TextDir);
		BOOL ReadStructureStats(const char* ScriptFile, char *IMDDir,char *TextDir);
		TECH_LEVEL CHeightMap::SetTechLevel(char *pLevel);
		BOOL ReadTemplateStats(const char* ScriptFile, char* IMDDir, char* TextDir);
		BOOL ReadObjectNames(char *FileName);
		int MatchObjName(char *IDString);

	private:
		BOOL ReadObjects(fileParser& Parser,char *Begin,char *End,int TypeID);
		BOOL ReadMisc(fileParser& Parser,char *Begin,char *End);
		BOOL ReadFeatures(fileParser& Parser,char *Begin,char *End);
		BOOL ReadStructures(fileParser& Parser,char *Begin,char *End);
		BOOL ReadTemplates(fileParser& Parser,char *Begin,char *End);

	public:
		BOOL ReadIMDObjects(char *ScriptFile);
//		BOOL ReadObjects(char *ScripFile);
		inline DWORD GetNumIMD()
		{
			return m_Num3DObjects;
		}

		BOOL ReadIMD(const char* FileName, char *Description, const char* TextDir,int TypeID,BOOL Flanged = FALSE,BOOL Snap = FALSE,int ColourKeyIndex = FALSE,NORMALTYPE NType = NT_DEFAULTNORMALS,
					 int StructureIndex = 0,int PlayerIndex = 0,C3DObject *Object=NULL);
		void RenderIMD(C3DObject *Object);
		void RenderFlatIMD(C3DObject *Object);
		void RenderTerrainMorphIMD(C3DObject *Object,D3DVECTOR *Position,D3DVECTOR *Rotation);
		void DrawIMD(DWORD ObjectID,D3DVECTOR &Rotation,D3DVECTOR &Position,
					 D3DVECTOR &CameraRotation,D3DVECTOR &CameraPosition,BOOL GroundSnap=FALSE);
		void DrawIMDStats(C3DObjectInstance *Data,
							 D3DVECTOR &CameraRotation,D3DVECTOR &CameraPosition);
		void DrawStats(HDC hdc,int x,int y,C3DObject *Object,C3DObjectInstance *Data);
		void DrawIMDSphere(DWORD ObjectID,D3DVECTOR &Rotation,D3DVECTOR &Position,
							 D3DVECTOR &CameraRotation,D3DVECTOR &CameraPosition,
							 DWORD MatID,BOOL GroundSnap,UBYTE Red,UBYTE Green,UBYTE Blue);
		void DrawIMDBox(DWORD ObjectID,D3DVECTOR &Rotation,D3DVECTOR &Position,
		 			 D3DVECTOR &CameraRotation,D3DVECTOR &CameraPosition,DWORD MatID,BOOL GroundSnap,D3DCOLOR Colour);
		void DrawIMDFootprint2D(CDIBDraw *DIBDraw,C3DObjectInstance *Instance,
									int ScrollX,int ScrollY,COLORREF Colour,RECT *Clip);
		void DrawIMDBox2D(CDIBDraw *DIBDraw,C3DObjectInstance *Instance,
									int ScrollX,int ScrollY,COLORREF Colour,RECT *Clip);
//		void DrawIMDBox2D(CDIBDraw *DIBDraw,DWORD ObjectID,
//									D3DVECTOR &Rotation,D3DVECTOR &Position,
//									int ScrollX,int ScrollY,COLORREF Colour);
		BOOL ObjectHit3D(DWORD ObjectID,D3DVECTOR &Rotation,D3DVECTOR &Position,
	    			 D3DVECTOR &CameraRotation,D3DVECTOR &CameraPosition,BOOL GroundSnap,
	    			 int HitX,int HitY);
		SDWORD ObjectHit3DSphere(DWORD ObjectID,D3DVECTOR &Rotation,D3DVECTOR &Position,
					 D3DVECTOR &CameraRotation,D3DVECTOR &CameraPosition,BOOL GroundSnap,
					 int HitX,int HitY);
		BOOL ObjectHit2D(DWORD ObjectID,D3DVECTOR &Rotation,D3DVECTOR &Position,
										int ScrollX,int ScrollY,
										int HitX,int HitY);

		void Delete3DObjects();
		void Delete3DObjectInstances();

		char *GetObjectInstanceScriptName(int Index);
		void SetObjectInstanceScriptName(int Index,char *ScriptName);

		DWORD AddObject(DWORD ObjectID, const D3DVECTOR& Rotation, const D3DVECTOR& Position, DWORD PlayerID);
		DWORD AddObject(DWORD ObjectID, const D3DVECTOR& Rotation, const D3DVECTOR& Position, DWORD UniqueID,
						DWORD PlayerID, const char *ScriptName);
		void RemoveObject(DWORD Index);
		void Select3DObject(DWORD Index);
		void DeSelect3DObject(DWORD Index);
		void SelectAll3DObjects();
		void DeSelectAll3DObjects();
		void Get3DObjectRotation(DWORD Index,D3DVECTOR &Rotation);
		void Set3DObjectRotation(DWORD Index,D3DVECTOR &Rotation);
		void DeleteSelected3DObjects();
//		char **GetObjectNames() { return m_ObjectNames; }

		BOOL GetObjectInstanceFlanged(int Index);

		BOOL GetObjectInstanceSnap(int Index);

		char *GetObjectName(int Index);	// { return m_3DObjects[Index].Name; }
		char *GetObjectName(C3DObject *Object);
		char *GetObjectInstanceName(int Index);	// { return m_3DObjects[Index].Name; }
		char *GetObjectInstanceDescription(int Index);

		int GetObjectType(int Index);
		int GetObjectPlayer(int Index);
		inline BOOL GetObjectFlanged(int Index)
		{
			return m_3DObjects[Index].Flanged;
		}

		int GetObjectInstancePlayerID(int Index);
		int GetObjectInstanceUniqueID(int Index);
		void SetObjectInstancePlayerID(int Index,int PlayerID);

		char *GetObjectTypeName(int Index);
		char *GetObjectInstanceTypeName(int Index);

		int GetObjectID(int Index);

		inline DWORD GetNum3DObjects()
		{
			return m_Num3DObjects;
		}

		inline DWORD GetNumObjects()
		{
			return m_TotalInstances;
		}

		void DrawObjects(D3DVECTOR &CameraRotation,D3DVECTOR &CameraPosition,BOOL Boxed=FALSE);
		void DrawObjects2D(CDIBDraw *DIBDraw,int ScrollX,int ScrollY,RECT *Clip);
		C3DObjectInstance *GetObjectPointer(DWORD Index);
		void SnapObject(DWORD Index);
		int FindObjectHit3D(D3DVECTOR &CameraRotation,D3DVECTOR &CameraPosition,int HitX,int HitY);
		int FindObjectHit2D(int ScrollX,int ScrollY,int HitX,int HitY);
		void DrawRadarObjects(CDIBDraw *DIBDraw,int Scale);
		void SetObjectTileFlags(DWORD Index,DWORD Flags);
		void SetObjectTileFlags(DWORD Flags);
		void SetObjectTileHeights(DWORD Index);
		void SetObjectTileHeights();
		void Set3DObjectPosition(DWORD Index,D3DVECTOR &Position);
		void Get3DObjectPosition(DWORD Index,D3DVECTOR &Position);

		void CountObjects(int Exclude,int Include);

		BOOL WriteObjectList(FILE *Stream);
		BOOL WriteObjectList(FILE *Stream,UWORD StartX,UWORD StartY,UWORD Width,UWORD Height);

		BOOL ReadObjectList(FILE *Stream);
		LONG GetStructureID(char *Name,LONG PlayerID);
		LONG GetObjectID(char *Name);

//		BOOL ReadDeliveranceMap(FILE *Stream);
//		BOOL ReadDeliveranceFeatures(FILE *Stream);

		int CountObjectsOfType(int Type,int Exclude,int Include);
		
		inline CWorldInfo *GetWorldInfo()
		{
			return &m_WorldInfo;
		}

		void GetLimitRect(int Index,LimitRect &Limit);
		BOOL InLimit(float x,float z,int Index);
		BOOL IncludeIt(float x,float z,int Exclude,int Include);

		BOOL WriteDeliveranceGame(FILE *Stream,UDWORD GameType,int LimIndex);
		BOOL WriteDeliveranceTileTypes(FILE *Stream);
		BOOL WriteDeliveranceMap(FILE *Stream);
		BOOL WriteDeliveranceFeatures(FILE *Stream,UDWORD GameType,int Exclude,int Include);
		BOOL WriteDeliveranceStructures(FILE *Stream,UDWORD GameType,int Exclude,int Include);
		BOOL WriteDeliveranceDroidInit(FILE *Stream,UDWORD GameType,int Exclude,int Include);
		BOOL WriteDeliveranceDroids(FILE *Stream);
		BOOL WriteDeliveranceTemplates(FILE *Stream);

//	BOOL WriteNecromancerMap(FILE *Stream);
//	BOOL WriteNecromancerObjects(FILE *Stream);

		void SetTileHeightUndo(int Index,float Height);

		void SetUniqueIDs();
		BOOL CheckUniqueIDs();
		BOOL CheckUniqueScriptNames();

		void InitialiseScrollLimits();
		void AddScrollLimit(int MinX, int MinZ, int MaxX, int MaxZ, const char* ScriptName);
		void SetScrollLimit(int Index, int MinX, int MinZ, int MaxX, int MaxZ, const char* ScriptName);
		void AddScrollLimit(int MinX, int MinZ, int MaxX, int MaxZ, DWORD UniqueID, const char* ScriptName);
		void DeleteAllScrollLimits();
		unsigned int FindScrollLimit(unsigned int UniqueID);
		unsigned int FindScrollLimit(const char* ScriptName);
		void DeleteScrollLimit(unsigned int Index);
		void DrawScrollLimits(D3DVECTOR &CameraRotation,D3DVECTOR &CameraPosition);
		BOOL CheckLimitsWithin(int ExcludeIndex,int IncludeIndex);
		BOOL CheckUniqueLimitsScriptNames();
		BOOL WriteScrollLimits(FILE *Stream,int StartX,int StartY,int Width,int Height);
		BOOL ReadScrollLimits(FILE *Stream);
		BOOL WriteDeliveranceLimits(FILE *Stream);
		
		inline int GetNumScrollLimits()
		{
			return m_NumScrollLimits;
		}

		const std::list<CScrollLimits>& GetScrollLimits() const
		{
			return m_ScrollLimits;
		}

		
		template <typename InputIterator>
		void SetScrollLimits(InputIterator first, InputIterator last)
		{
			m_ScrollLimits.assign(first, last);
		}

		void InitialiseGateways();
		int AddGateway(int x0,int y0,int x1,int y1);
		void DeleteGateway(int Index);
		void DeleteAllGateways();
		void DeSelectGateways();
		void SelectGateway(int Index);
		int FindGateway(int x,int y);
		void SetGateway(int Index,int x0,int y0,int x1,int y1);
		BOOL TileIsBlockingLand(int x,int y);
		BOOL TileIsBlockingWater(int x,int y);
		BOOL TileIsBlocking(int x,int y,int Water);
		BOOL CheckGatewayBlockingTiles(int Index);
		BOOL CheckGatewayOverlap(int CurIndex,int x0,int y0,int x1,int y1);
		BOOL WriteGateways(FILE *Stream,int StartX,int StartY,int Width,int Height);
		BOOL ReadGateways(FILE *Stream);
		BOOL WriteDeliveranceGateways(FILE *Stream);
		void DisplayGateways3D(D3DVECTOR &CameraRotation,D3DVECTOR &CameraPosition);
		void DisplayGateways2D(CDIBDraw *DIBDraw,int ScrollX, int ScrollY,RECT *Clip);
		
		inline const std::list<GateWay>& GetGateWays()
		{
			return m_Gateways;
		}

		inline int GetNumGateways() const
		{
			return m_NumGateways;
		}

		BOOL GetGateway(int Index, int& x0, int& y0, int& x1, int& y1);
		void SetMapZoneID(int TileNum,int ID);
		void SetMapZoneID(int x,int z,int ID);

		BOOL SetTileIDsFromBitmap(char *FullPath);
		BOOL WriteTileIDMap(char *FullPath);

		BOOL ObjectIsStructure(int ObjectID);
		BOOL StructureIsDefense(int ObjectID);
		BOOL StructureIsWall(int ObjectID);

		void ClearSelectionBox();
		void SetSelectionBox0(int TileID);
		void SetSelectionBox1(int TileID);
		void SetSelectionBox(int TileID,int Width,int Height);
		BOOL SelectionBoxValid();

		inline int GetSelectionBox0()
		{
			return m_SelectionBox0;
		}

		inline int GetSelectionBox1()
		{
			return m_SelectionBox1;
		}

		void ClipSelectionBox();

		void XFlipObjects(int x0,int y0,int x1,int y1);
		void YFlipObjects(int x0,int y0,int x1,int y1);
		void SetRenderPlayerID(DWORD PlayerID) { m_RenderPlayerID = PlayerID; }

	private:
		int m_MaxTileID;
		int m_NumGateways;
		std::list<GateWay> m_Gateways;

		int m_NumScrollLimits;
		std::list<CScrollLimits> m_ScrollLimits;

		SLONG m_DrawRadius;
		void HeightToV0(UBYTE Height,D3DVECTOR *Offset,D3DVECTOR *Result);
		void HeightToV1(UBYTE Height,D3DVECTOR *Offset,D3DVECTOR *Result);
		void HeightToV2(UBYTE Height,D3DVECTOR *Offset,D3DVECTOR *Result);
		void HeightToV3(UBYTE Height,D3DVECTOR *Offset,D3DVECTOR *Result);
		void FHeightToV0(UBYTE Height,D3DVECTOR *Offset,D3DVECTOR *Result);
		void FHeightToV1(UBYTE Height,D3DVECTOR *Offset,D3DVECTOR *Result);
		void FHeightToV2(UBYTE Height,D3DVECTOR *Offset,D3DVECTOR *Result);
		void FHeightToV3(UBYTE Height,D3DVECTOR *Offset,D3DVECTOR *Result);
		void InitialiseBoundingBox(CMapSector *Sector);
		BOOL BoundsCheck(CMapSector *Sector);
		void SetTileVertexUV(CTile *Tile,D3DVERTEX &v0,D3DVERTEX &v1,D3DVERTEX &v2,D3DVERTEX &v3);
		CMapSector	*m_MapSectors;
		CDirectDraw* m_DDView;
		CGeometry* m_DirectMaths;
		CMatManager* m_MatManager;
		DWORD	m_NumSourceTextures;
		DWORD	m_NumTextures;
		DWORD	m_MaxTMapID;
		char	**m_TextureList;
		CTextureList m_Texture[256];
		SLONG	m_TileWidth;
		SLONG	m_TileHeight;
		SLONG	m_SectorsPerRow;
		SLONG	m_SectorsPerColumn;
		SLONG	m_TilesPerSectorRow;
		SLONG	m_TilesPerSectorColumn;
		DWORD	m_TextureWidth;
		DWORD	m_TextureHeight;
		SLONG	m_NumPoints;
		SLONG	m_HeightMult;
		SLONG	m_ViewRadius;
		SLONG	m_XRes,m_YRes;
		DWORD	m_NumTiles;
		DWORD	m_NumSectors;
		DWORD	m_HeightScale;
		DWORD	m_DefaultMaterial;
		DWORD	m_SeaMaterial;
		DWORD	m_SeaLevel;
		DWORD	m_LineMaterial;
		DWORD	m_LineMaterial2;
		BOOL	m_Gouraud;
		CWorldInfo m_WorldInfo;

		DWORD	m_Num3DObjects;
//	char **m_ObjectNames;
		C3DObject m_3DObjects[MAX3DOBJECTS];

		int m_TotalInstances;
		std::list<C3DObjectInstance> m_Objects;

		int m_NumStructures;
		_structure_stats *m_Structures;

		int m_NumFeatures;
		_feature_stats *m_Features;

		int m_NumTemplates;
		DroidTemplate *m_Templates;
};

#endif // __INCLUDED_HEIGHTMAP_H__
