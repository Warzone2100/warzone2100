#ifndef _display3ddef_h
#define _display3ddef_h

#define TILE_WIDTH 64
#define TILE_HEIGHT 64
#define TILE_SIZE (TILE_WIDTH*TILE_HEIGHT)

#define	RADTLX		(OBJ_BACKX + OBJ_BACKWIDTH + BASE_GAP + 1 +D_W)	// Paul's settings (492+12)
#define	RADTLY		(RET_Y + 1)									// Paul's settings (332-17)
#define	RADWIDTH	128
#define RADHEIGHT	128
#define	RADBRX		(RADTLX + RADWIDTH)	-1
#define	RADBRY		(RADTLY + RADHEIGHT) -1
//assigned to variable visibleXtiles, visibleYTiles 25/02/98 AB
#ifdef WIN32
#define VISIBLE_XTILES	32	
#define VISIBLE_YTILES	32

#define MIN_TILE_X		(VISIBLE_XTILES/4)
#define MAX_TILE_X		((3*VISIBLE_XTILES)/4)

#define MIN_TILE_Y		(VISIBLE_YTILES/4)
#define MAX_TILE_Y		((3*VISIBLE_YTILES)/4)

#else
#define VISIBLE_XTILES	22	// This will break on the psx if you change it to any other value !!!!!!
#define VISIBLE_YTILES	22

#define MIN_TILE_X		(VISIBLE_XTILES/4)
#define MAX_TILE_X		((3*VISIBLE_XTILES)/4)

#define MIN_TILE_Y		(VISIBLE_YTILES/4)
#define MAX_TILE_Y		((3*VISIBLE_YTILES)/4)
#endif



#define LAND_XGRD	(VISIBLE_XTILES + 1)
#define LAND_YGRD	(VISIBLE_YTILES + 1)
#ifdef PSX
#define DISTANCE	1716
#else
#define DISTANCE	(2600)
#define START_DISTANCE	(2000)
#endif

#define MINDISTANCE	(DISTANCE - DISTANCE/3)

#define NUM_TILES	100		//5 pages of 16 tiles.

#define BOX_PULSE_SIZE	10
extern UBYTE	boxPulseColours[BOX_PULSE_SIZE]; 

#endif