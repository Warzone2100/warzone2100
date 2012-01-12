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
/*! \file animobj.h
 * \brief Animation object types and function headers
 *
 * Gareth Jones 14/11/97
 */
/***************************************************************************/

#ifndef _ANIMOBJ_H_
#define _ANIMOBJ_H_

#include "anim.h"


#define	ANIM_MAX_COMPONENTS		10


typedef void (* ANIMOBJDONEFUNC) ( struct ANIM_OBJECT *psObj );
typedef bool (* ANIMOBJDIEDTESTFUNC) ( void *psParent );

struct COMPONENT_OBJECT
{
	Vector3i                position;
	Vector3i                orientation;
	void *                  psParent;
	iIMDShape *             psShape;
};

struct ANIM_OBJECT
{
	ANIM_OBJECT *           psNext;

	UWORD                   uwID;
	ANIM3D *                psAnim;
	void *                  psParent;
	UDWORD                  udwStartTime;
	UDWORD                  udwStartDelay;
	UWORD                   uwCycles;
	bool                    bVisible;
	ANIMOBJDONEFUNC         pDoneFunc;
	/* this must be the last entry in this structure */
	COMPONENT_OBJECT        apComponents[ANIM_MAX_COMPONENTS];
};


bool			animObj_Init( ANIMOBJDIEDTESTFUNC pDiedFunc );
void			animObj_Update( void );
bool			animObj_Shutdown( void );
void			animObj_SetDoneFunc( ANIM_OBJECT *psObj,
										ANIMOBJDONEFUNC pDoneFunc );

/* uwCycles=0 for infinite looping */
ANIM_OBJECT *	animObj_Add( void *pParentObj, int iAnimID,
								UDWORD udwStartDelay, UWORD uwCycles );
bool animObj_Remove(ANIM_OBJECT* psObj, int iAnimID);

ANIM_OBJECT *	animObj_GetFirst( void );
ANIM_OBJECT *	animObj_GetNext( void );
ANIM_OBJECT *	animObj_Find( void *pParentObj, int iAnimID );

#endif	/* _ANIMOBJ_H_ */
