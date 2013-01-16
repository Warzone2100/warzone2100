/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2013  Warzone 2100 Project

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

#ifndef __INCLUDED_DEVMAP_H__
#define __INCLUDED_DEVMAP_H__

//
// Map file structures and definitions extracted from Deliverance Map.h and Map.c
//

// A few Deliverance specific typedefs.

typedef unsigned	char	UBYTE;
typedef	signed		char	SBYTE;
typedef signed		char	STRING;
typedef	unsigned	short	UWORD;
typedef	signed		short	SWORD;
typedef	unsigned	int		UDWORD;
typedef	signed		int		SDWORD;

typedef unsigned short BASE_OBJECT;
// BASE_OBJECT is defined differently in the game but for the sake of the editor it can just
// be a long word.


/* Structure definitions for loading and saving map data */
typedef struct {
	STRING		aFileType[4];
	UDWORD		version;
	UDWORD		quantity;
} TILETYPE_SAVEHEADER;

typedef struct {
	STRING		aFileType[4];
	UDWORD		version;
	UDWORD		quantity;
} LIMITS_SAVEHEADER;

typedef struct {
	UDWORD LimitID;
	UWORD MinX;
	UWORD MinZ;
	UWORD MaxX;
	UWORD MaxZ;
} LIMITS;

typedef struct _map_save_header
{
	STRING		aFileType[4];
	UDWORD		version;
	UDWORD		width;
	UDWORD		height;
} MAP_SAVEHEADER;
typedef struct _map_save_tile
{
	UWORD		texture;
	UBYTE		height;
} MAP_SAVETILE;
typedef struct _gateway_save_header
{
	UDWORD		version;
	UDWORD		numGateways;
} GATEWAY_SAVEHEADER;
typedef struct _gateway_save
{
	UBYTE	x0,y0,x1,y1;
} GATEWAY_SAVE;



//typedef struct _map_save_tilev1
//{
//	UDWORD		texture;
//	UBYTE		type;
//	UBYTE		height;
//} MAP_SAVETILEV1;

/* Structure definitions for loading and saving map data */
/*(typedef struct _map_save_header
{
	STRING		aFileType[4];
	UDWORD		version;
	UDWORD		width;
	UDWORD		height;
} MAP_SAVEHEADER;

typedef struct _map_save_tile
{
	UDWORD		texture;
	UBYTE		type;
	UBYTE		height;
} MAP_SAVETILE;
*/
/* Sanity check definitions for the save struct file sizes */
#define TILETYPE_SAVEHEADER_SIZE 12
#define LIMITS_SAVEHEADER_SIZE 12
#define SAVE_HEADER_SIZE	16
#define SAVE_TILE_SIZE		3
#define SAVE_TILE_SIZEV1	6

//#define SAVE_HEADER_SIZE	16
//#define SAVE_TILE_SIZE		6

/* The different types of terrain as far as the game is concerned */
typedef enum _terrain_type
{
	TER_GRASS = 0x1,
	TER_STONE = 0x2,
	TER_SAND  = 0x4,
	TER_WATER = 0x8,
	TER_OBJECT	= 0x80,
	TER_ALL   = 0xff,
} TERRAIN_TYPE;

/* change these if you change above - maybe wrap up in enumerate? */
#define	TERRAIN_TYPES	4

#define MAX_PLAYERS 8		/*Utterly arbitrary at the moment!! */

/* Flags for whether texture tiles are flipped in X and Y or rotated */
//#define TILE_XFLIP		0x80000000
//#define TILE_YFLIP		0x40000000
//#define TILE_NUMMASK	0x0fffffff
//#define TILE_ROTMASK	0x30000000
//#define TILE_ROTSHIFT	28

#define TILE_XFLIP		0x8000
#define TILE_YFLIP		0x4000
#define TILE_ROTMASK	0x3000
#define TILE_ROTSHIFT	12
#define TILE_TRIFLIP	0x0800	// This bit describes the direction the tile is split into 2 triangles.
#define TILE_NUMMASK	0x07ff
#define TILE_BITMASK	0xF800

///* Information stored with each tile */
//typedef struct _tile
//{
//	TERRAIN_TYPE	type;			// The terrain type for the tile
//	UBYTE			height;			// The height at the top left of the tile
//	BASE_OBJECT		*psObject;		// Any object sitting on the location (e.g. building)
//	BOOL			tileVisible[MAX_PLAYERS]; // Which players can see the tile?
//	UDWORD			texture;		// Which graphics texture is on this tile
//									// This is also used to store the tile flip flags
//	UDWORD			onFire;			// Is tile on fire?
//	UDWORD			rippleIndex;	// Current value in ripple table?
//} TILE;

/* The maximum map size */
#define MAP_MAXWIDTH	256
#define MAP_MAXHEIGHT	256

#endif // __INCLUDED_DEVMAP_H__
