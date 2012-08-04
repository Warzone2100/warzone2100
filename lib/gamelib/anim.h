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
/*! \file anim.h
 * \brief Animation types and function headers
 *
 * Gareth Jones 11/7/97
 */

#ifndef _ANIM_H_
#define _ANIM_H_


#include <physfs.h>

#include "lib/framework/types.h"
#include "lib/ivis_opengl/imd.h"

#define	ANIM_MAX_STR			256
#define	ANIM_DELAYED	0xFFFE
#define	NO_ANIM			0xFFFD
#define	NO_IMD			0xFFFC


enum ANIM_MODE
{
	ANIM_3D_FRAMES = 1,
	ANIM_3D_TRANS
};


struct ANIM_STATE;
struct BASEANIM;

#define ANIM_BASE_ELEMENTS						\
	char				szFileName[ANIM_MAX_STR];	\
	char				animType;				\
	UWORD				uwID;					\
	UWORD				uwFrameRate;			\
	UWORD				uwStates;				\
	UWORD				uwObj;					\
	UWORD				uwAnimTime;				\
	ANIM_MODE			ubType;					\
	struct ANIM_STATE	*psStates;				\
	struct BASEANIM		*psNext;


struct ANIM_STATE
{
	UWORD				uwFrame;		/* frame to play           */
	Vector3i			vecPos;
	Vector3i			vecAngle;
	Vector3i			vecScale;
};

struct BASEANIM
{
	char                    szFileName[ANIM_MAX_STR];
	char                    animType;
	UWORD                   uwID;
	UWORD                   uwFrameRate;
	UWORD                   uwStates;
	UWORD                   uwObj;
	UWORD                   uwAnimTime;
	ANIM_MODE               ubType;
	ANIM_STATE *            psStates;
	BASEANIM *              psNext;
};

struct ANIM3D : public BASEANIM
{
	iIMDShape *             psFrames;
	iIMDShape **            apFrame;
};


bool		anim_Init(void);
bool		anim_Shutdown(void);
BASEANIM *	anim_LoadFromBuffer(char *pBuffer, UDWORD size);
BASEANIM *	anim_LoadFromFile(PHYSFS_file* fileHandle);
void		anim_ReleaseAnim(BASEANIM *psAnim);
bool		anim_Create3D(char szPieFileName[], UWORD uwFrames, UWORD uwFrameRate, UWORD uwObj,
		              ANIM_MODE ubType, UWORD uwID);
void		anim_BeginScript(void);
bool		anim_EndScript(void);
bool		anim_AddFrameToAnim(int iFrame, Vector3i vecPos, Vector3i vecRot, Vector3i vecScale);
BASEANIM *	anim_GetAnim(UWORD uwAnimID);
UWORD		anim_GetAnimID(char *szName);
iIMDShape *	anim_GetShapeFromID(UWORD uwID);
UWORD           anim_GetFrame3D(ANIM3D *psAnim, UWORD uwObj, UDWORD udwGraphicsTime, UDWORD udwStartTime,
		                UDWORD udwStartDelay, Vector3i *psVecPos, Vector3i *psVecRot,
		                Vector3i *psVecScale);
void		anim_SetVals(char szFileName[], UWORD uwAnimID);

#endif	/* _ANIM_H_ */
