/***************************************************************************/
/*
 * pieTypes.h
 *
 * type defines for simple pies.
 *
 */
/***************************************************************************/

#ifndef _pieTypes_h
#define _pieTypes_h

#include "frame.h"

#ifdef PSX
#define PIEPSX
//#define FAT_N_FAST_IMD	// pad out iVector with extra word to make it compatible with SVECTOR.
#endif
/***************************************************************************/
/***************************************************************************/
/*
 *	Global Definitions
 */
/***************************************************************************/

#define PI 	  					3.141592654

/***************************************************************************/
/*
 *	Global Macros
 */
/***************************************************************************/

/***************************************************************************/
/*
 *	Global Type Definitions
 */
/***************************************************************************/
typedef signed char int8;
typedef signed short int16;
typedef int int32;
typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;

//*************************************************************************
//
// Simple derived types
//
//*************************************************************************
typedef struct {int left, top, right, bottom;} iClip;
typedef uint8 iBitmap;
typedef struct {uint8 r, g, b;} iColour;
typedef int iBool;
typedef struct {int32 x, y;} iPoint;
typedef struct {int width, height; iBitmap *bmp;} iSprite;
typedef iColour iPalette[256];
typedef struct {uint8 r, g, b, p;} iRGB8;
typedef struct {uint16 r, g, b, p;} iRGB16;
typedef struct {uint32 r, g, b, p;} iRGB32;
typedef struct {int8 x, y;} iPoint8;
typedef struct {int16 x, y;} iPoint16;
typedef struct {int32 x, y;} iPoint32;

#ifndef PIEPSX			// was    #ifdef WIN32
	typedef struct {int32 x, y, z;} iVector;
	typedef struct {double x, y, z;} iVectorf;
	typedef struct {int xshift, width, height; iBitmap *bmp;
					iColour *pPal; iBool bColourKeyed; } iTexture;
	typedef struct {int32 x, y, z, u, v; uint8 g;} iVertex; 
#else  // Basically its a playstation ...
#ifdef IVECTOR_EQU_SVECTOR
	typedef struct {SWORD x, y, z, pad;} iVector;
#else
	typedef struct {SWORD x, y, z;} iVector;
#endif
	typedef struct {FRACT x, y, z;} iVectorf;




//	typedef struct {int xshift, width, height; iBitmap *bmp; SBYTE num; uint16 tpage; uint16 clut; iBool bColourKeyed; } iTexture;

	typedef struct
	{
		SBYTE num;		// texture page id value ... Page-8-player...   would be stored as 8  ... -1 for not defined
		RECT VRAMpos;	// Location in VRAM that this texture page is located
		uint16 tpage;	// Id of the texture page
		uint16 clut;	// Clut ID of the texture page
// 	UDWORD	TextureMode;	// 4/8/16 bit ... Is this needed ?
#ifdef PIETOOL
		int width,height,xshift;
		iBitmap *bmp;
		iBool bColourKeyed;

#endif
	} iTexture;


	// on the playstation we store the texture page ID, and the u,v coords withing the texture page
	typedef struct 
		{
		uint8 u,v;	// 8 bit u,v coords are all that is needed on the playstation
	//	uint8 g;	// Lighting values (? - needed)
		} iVertex; 
#endif
typedef struct {FRACT x,y,z;} PIEVECTORF;
typedef struct {iVector p, r;} iView;

#endif // _pieTypes_h
