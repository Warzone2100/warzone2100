/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2010  Warzone 2100 Project

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
 * pieState.h
 *
 * render State controlr all pumpkin image library functions.
 *
 */
/***************************************************************************/

#ifndef _piestate_h
#define _piestate_h


/***************************************************************************/

#include "lib/framework/frame.h"
#include "piedef.h"

/***************************************************************************/
/*
 *	Global Definitions
 */
/***************************************************************************/

typedef	enum	REND_MODE
				{
					REND_ALPHA,
					REND_ADDITIVE,
					REND_OPAQUE,
					REND_MULTIPLICATIVE
				}
				REND_MODE;

typedef	enum	DEPTH_MODE
				{
					DEPTH_CMP_LEQ_WRT_ON,
					DEPTH_CMP_ALWAYS_WRT_ON,
					DEPTH_CMP_LEQ_WRT_OFF,
					DEPTH_CMP_ALWAYS_WRT_OFF
				}
				DEPTH_MODE;

typedef	enum	TRANSLUCENCY_MODE
				{
					TRANS_DECAL,
					TRANS_FILTER,
					TRANS_ALPHA,
					TRANS_ADDITIVE,
					TRANS_MULTIPLICATIVE
				}
				TRANSLUCENCY_MODE;

typedef struct	RENDER_STATE
				{
					BOOL				fogEnabled;
					BOOL				fog;
					PIELIGHT			fogColour;
					SDWORD				texPage;
					REND_MODE			rendMode;
					BOOL				keyingOn;
					TRANSLUCENCY_MODE	transMode;
				}
				RENDER_STATE;

typedef enum
{
	TEXPAGE_NONE = -1,
	TEXPAGE_EXTERN = -2
} TEXPAGE_TYPE;

typedef enum
{
	SHADER_NONE,
	SHADER_TCMASK,
	SHADER_TCMASK_FOGGED,
	SHADER_MAX
} SHADER_MODE;

/***************************************************************************/
/*
 *	Global Variables
 */
/***************************************************************************/

extern unsigned int pieStateCount;
extern RENDER_STATE rendStates;

/***************************************************************************/
/*
 *	Global ProtoTypes
 */
/***************************************************************************/
extern void pie_SetDefaultStates(void);//Sets all states
extern void pie_SetDepthBufferStatus(DEPTH_MODE depthMode);
extern void pie_SetDepthOffset(float offset);
//fog available
extern void pie_EnableFog(BOOL val);
extern BOOL pie_GetFogEnabled(void);
//fog currently on
extern void pie_SetFogStatus(BOOL val);
extern BOOL pie_GetFogStatus(void);
extern void pie_SetFogColour(PIELIGHT colour);
extern PIELIGHT pie_GetFogColour(void) WZ_DECL_PURE;
extern void pie_UpdateFogDistance(float begin, float end);
//render states
extern void pie_SetTexturePage(SDWORD num);
extern void pie_SetAlphaTest(BOOL keyingOn);
extern void pie_SetRendMode(REND_MODE rendMode);

extern void pie_InitColourMouse(IMAGEFILE* img, const uint16_t cursorIDs[CURSOR_MAX]);
extern void pie_SetMouse(CURSOR cursor, bool coloured);
extern void pie_DrawMouse(unsigned int X, unsigned int Y);
extern void pie_ShowMouse(bool visible);

extern void pie_SetTranslucencyMode(TRANSLUCENCY_MODE transMode);

/* Actually in piestate.c */

// Shaders control center
extern bool pie_GetShadersStatus(void);
extern void pie_SetShadersStatus(bool);
bool pie_LoadShaders(void);
// Actual shaders (we do not want to export these calls)
void pie_DeactivateShader(void);
void pie_ActivateShader_TCMask(PIELIGHT teamcolour, SDWORD maskpage);

/* Actually in piedraw.c */

// Lighting cotrols
extern void pie_SetLightingState(bool);
extern bool pie_GetLightingState(void);

/* Errors control routine */
#define glErrors() \
	_glerrors(__FUNCTION__, __FILE__, __LINE__)

extern bool _glerrors(const char *, const char *, int);


#endif // _pieState_h
