/***************************************************************************/
/*! \file animobj.h
 * \brief Animation object types and function headers
 *
 * Gareth Jones 14/11/97
 */
/***************************************************************************/

#ifndef _ANIMOBJ_H_
#define _ANIMOBJ_H_

/***************************************************************************/

#include "anim.h"

/***************************************************************************/

#define	ANIM_MAX_COMPONENTS		10

/***************************************************************************/
/* forward struct declarations */

struct ANIM_OBJECT;
struct COMPONENT_OBJECT;

/***************************************************************************/
/* typedefs */

typedef void (* ANIMOBJDONEFUNC) ( struct ANIM_OBJECT *psObj );
typedef BOOL (* ANIMOBJDIEDTESTFUNC) ( void *psParent );

/***************************************************************************/
/* struct member macros */

#define	COMPONENT_ELEMENTS(pointerType)		\
	VECTOR3D	position;					\
	VECTOR3D	orientation;				\
	void		*psParent;					\
	iIMDShape	*psShape;

#define	ANIM_OBJECT_ELEMENTS(pointerType)					\
	UWORD				uwID;								\
	ANIM3D				*psAnim;							\
	void				*psParent;							\
	UDWORD				udwStartTime;						\
	UDWORD				udwStartDelay;						\
	UWORD				uwCycles;							\
	BOOL				bVisible;							\
	ANIMOBJDONEFUNC		pDoneFunc;							\
	/* this must be the last entry in this structure */		\
	COMPONENT_OBJECT	apComponents[ANIM_MAX_COMPONENTS];

/***************************************************************************/

typedef struct COMPONENT_OBJECT
{
	COMPONENT_ELEMENTS( struct COMPONENT_OBJECT )
}
COMPONENT_OBJECT;

typedef struct ANIM_OBJECT
{
	struct ANIM_OBJECT	*psNext;

	/* this must be the last entry in this structure */
	ANIM_OBJECT_ELEMENTS( struct ANIM_OBJECT )
}
ANIM_OBJECT;

/***************************************************************************/

BOOL			animObj_Init( ANIMOBJDIEDTESTFUNC pDiedFunc );
void			animObj_Update( void );
BOOL			animObj_Shutdown( void );
void			animObj_SetDoneFunc( ANIM_OBJECT *psObj,
										ANIMOBJDONEFUNC pDoneFunc );

/* uwCycles=0 for infinite looping */
ANIM_OBJECT *	animObj_Add( void *pParentObj, int iAnimID,
								UDWORD udwStartDelay, UWORD uwCycles );
BOOL			animObj_Remove( ANIM_OBJECT **ppsObj, int iAnimID );

ANIM_OBJECT *	animObj_GetFirst( void );
ANIM_OBJECT *	animObj_GetNext( void );
ANIM_OBJECT *	animObj_Find( void *pParentObj, int iAnimID );
UWORD			animObj_GetFrame3D( ANIM_OBJECT *psObj, UWORD uwObj,
									VECTOR3D *psPos, VECTOR3D *psVecRot,
									VECTOR3D *psVecScale );

/***************************************************************************/

#endif	/* _ANIMOBJ_H_ */

/***************************************************************************/
