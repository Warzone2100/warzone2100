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

#include "lib/framework/frame.h"

/***************************************************************************/
/***************************************************************************/
/*
 *	Global Definitions
 */
/***************************************************************************/

#define PI 3.141592654

/***************************************************************************/
/*
 *	Global Type Definitions
 */
/***************************************************************************/
typedef Sint8 int8;
typedef Sint16 int16;
typedef Sint32 int32;
typedef Uint8 uint8;
typedef Uint16 uint16;
typedef Uint32 uint32;

//*************************************************************************
//
// Simple derived types
//
//*************************************************************************
typedef struct { Sint32 left, top, right, bottom; } iClip;
typedef Uint8 iBitmap;
typedef struct { Uint8 r, g, b; } iColour;
typedef BOOL iBool;
typedef struct { Sint32 x, y; } iPoint;
typedef struct { Uint32 width, height, depth; iBitmap *bmp; } iSprite;
typedef iColour iPalette[256];
typedef struct { Uint8 r, g, b, p; } iRGB8;
typedef struct { Uint16 r, g, b, p; } iRGB16;
typedef struct { Uint32 r, g, b, p; } iRGB32;
typedef struct { Sint8 x, y; } iPoint8;
typedef struct { Sint16 x, y; } iPoint16;
typedef struct { Sint32 x, y; } iPoint32;
typedef struct { Sint32 x, y, z; } iVector;
typedef struct { double x, y, z; } iVectorf;
typedef struct { Sint32 xshift, width, height; iBitmap *bmp;
			iColour *pPal; iBool bColourKeyed; } iTexture;
typedef struct { Sint32 x, y, z, u, v; uint8 g; } iVertex;
typedef struct { float x,y,z; } PIEVECTORF;
typedef struct { iVector p, r; } iView;

#endif // _pieTypes_h
