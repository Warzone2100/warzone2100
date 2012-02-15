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

struct RENDER_STATE
{
					bool				fogEnabled;
					bool				fog;
					PIELIGHT			fogColour;
					SDWORD				texPage;
					REND_MODE			rendMode;
					bool				keyingOn;
};

void rendStatesRendModeHack();  // Sets rendStates.rendMode = REND_ALPHA; (Added during merge, since the renderStates is now static.)

/***************************************************************************/
/*
 *	Global Variables
 */
/***************************************************************************/

extern unsigned int pieStateCount;

/***************************************************************************/
/*
 *	Global ProtoTypes
 */
/***************************************************************************/
extern void pie_SetDefaultStates(void);//Sets all states
extern void pie_SetDepthBufferStatus(DEPTH_MODE depthMode);
extern void pie_SetDepthOffset(float offset);
//fog available
extern void pie_EnableFog(bool val);
extern bool pie_GetFogEnabled(void);
//fog currently on
extern void pie_SetFogStatus(bool val);
extern bool pie_GetFogStatus(void);
extern void pie_SetFogColour(PIELIGHT colour);
extern PIELIGHT pie_GetFogColour(void) WZ_DECL_PURE;
extern void pie_UpdateFogDistance(float begin, float end);
//render states
extern void pie_SetTexturePage(SDWORD num);
extern void pie_SetAlphaTest(bool keyingOn);
extern void pie_SetRendMode(REND_MODE rendMode);

// Shaders control center
extern bool pie_GetShaderAvailability(void);
extern void pie_SetShaderAvailability(bool);
bool pie_LoadShaders(void);
// Actual shaders (we do not want to export these calls)
void pie_DeactivateShader(void);
void pie_DeactivateFallback(void);
void pie_ActivateShader(SHADER_MODE shaderMode, iIMDShape* shape, PIELIGHT teamcolour, PIELIGHT colour);
void pie_ActivateFallback(SHADER_MODE shaderMode, iIMDShape* shape, PIELIGHT teamcolour, PIELIGHT colour);
void pie_SetShaderStretchDepth(float stretch);
void pie_SetShaderTime(uint32_t shaderTime);
void pie_SetShaderEcmEffect(bool value);

/* Errors control routine */
#define glErrors() \
	_glerrors(__FUNCTION__, __FILE__, __LINE__)

extern bool _glerrors(const char *, const char *, int);

#endif // _pieState_h
