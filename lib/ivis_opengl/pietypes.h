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
*/
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
#include "lib/framework/vector.h"

/***************************************************************************/
/*
 *	Global Definitions (CONSTANTS)
 */
/***************************************************************************/
#define STRETCHED_Z_SHIFT		10 // stretchs z range for (1000 to 4000) to (8000 to 32000)
#define MAX_Z				(32000) // raised to 32000 from 6000 when stretched
#define MIN_STRETCHED_Z			256
#define LONG_WAY			(1<<15)
#define INTERFACE_DEPTH		(MAX_Z - 1)
#define BUTTON_DEPTH		2000 // will be stretched to 16000

#define OLD_TEXTURE_SIZE_FIX 256.0f

//Render style flags for all pie draw functions
#define pie_ECM                 0x1
#define pie_TRANSLUCENT         0x2
#define pie_ADDITIVE            0x4
#define pie_FORCE_FOG			0x8
#define pie_HEIGHT_SCALED       0x10
#define pie_RAISE               0x20
#define pie_BUTTON              0x40
#define pie_SHADOW              0x80
#define pie_STATIC_SHADOW       0x100
#define pie_PREMULTIPLIED       0x200

#define pie_RAISE_SCALE			256

enum LIGHTING_TYPE
{
	LIGHT_EMISSIVE,
	LIGHT_AMBIENT,
	LIGHT_DIFFUSE,
	LIGHT_SPECULAR,
	LIGHT_MAX
};

enum REND_MODE
{
	REND_ALPHA,
	REND_ADDITIVE,
	REND_OPAQUE,
	REND_MULTIPLICATIVE,
	REND_PREMULTIPLIED,
};

enum DEPTH_MODE
{
	DEPTH_CMP_LEQ_WRT_ON,
	DEPTH_CMP_ALWAYS_WRT_ON,
	DEPTH_CMP_LEQ_WRT_OFF,
	DEPTH_CMP_ALWAYS_WRT_OFF
};

enum TEXPAGE_TYPE
{
	TEXPAGE_NONE = -1,
	TEXPAGE_EXTERN = -2
};

enum SHADER_MODE
{
	SHADER_NONE,
	SHADER_COMPONENT,
	SHADER_BUTTON,
	SHADER_MAX
};

//*************************************************************************
//
// Simple derived types
//
//*************************************************************************
struct iView
{
	Vector3i p, r;
};

struct iV_Image
{
	unsigned int width, height, depth;
	unsigned char *bmp;
};

#endif // _pieTypes_h
