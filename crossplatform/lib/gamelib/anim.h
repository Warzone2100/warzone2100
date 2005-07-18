/***************************************************************************/
/*
 * Anim.h
 *
 * Animation types and function headers
 *
 * Gareth Jones 11/7/97
 */
/***************************************************************************/

#ifndef _ANIM_H_
#define _ANIM_H_

/***************************************************************************/

#include "types.h"
#include "imd.h"
#include "maxpidef.h"

/***************************************************************************/

#define	ANIM_MAX_STR			256
#define	ANIM_DELAYED	0xFFFE
#define	NO_ANIM			0xFFFD
#define	NO_IMD			0xFFFC

/***************************************************************************/

enum{ ANIM_2D, ANIM_3D_FRAMES, ANIM_3D_TRANS };

/***************************************************************************/

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
	UBYTE				ubType;					\
	struct ANIM_STATE	*psStates;				\
	struct BASEANIM		*psNext;

/* ensure ANIM2D/3D structs same size */
#define ANIM_2D_ELEMENTS						\
	ANIM_BASE_ELEMENTS							\
	iSprite		*psFrames;						\
	UWORD		uwBmapWidth;	/* width of container bitmap */

/* ensure ANIM2D/3D structs same size */
#define ANIM_3D_ELEMENTS						\
	ANIM_BASE_ELEMENTS							\
	iIMDShape	*psFrames;						\
	iIMDShape	**apFrame;

/***************************************************************************/

typedef struct VECTOR3D
{
	SDWORD	x, y, z;
}
VECTOR3D;

typedef struct ANIM_STATE
{
	UWORD				uwFrame;		/* frame to play           */
	VECTOR3D			vecPos;
	VECTOR3D			vecAngle;
	VECTOR3D			vecScale;
}
ANIM_STATE;

typedef struct BASEANIM
{
	ANIM_BASE_ELEMENTS
}
BASEANIM;

typedef struct ANIM2D
{
	ANIM_2D_ELEMENTS
}
ANIM2D;

typedef struct ANIM3D
{
	ANIM_3D_ELEMENTS
}
ANIM3D;


/***************************************************************************/

typedef void * (* GETSHAPEFUNC) ( STRING *pStr );

typedef struct ANIMGLOBALS
{
	BASEANIM		*psAnimList;
	UWORD			uwCurObj, uwCurState;
	GETSHAPEFUNC	pGetShapeFunc;
}
ANIMGLOBALS;



/***************************************************************************/

BOOL		anim_Init( GETSHAPEFUNC  );
BOOL		anim_Shutdown( void );
BASEANIM *	anim_LoadFromBuffer( UBYTE *pBuffer, UDWORD size );
void		anim_ReleaseAnim( BASEANIM *psAnim );
BOOL		anim_Create3D( char szPieFileName[], UWORD uwFrames,
							UWORD uwFrameRate, UWORD uwObj,
							UBYTE ubType, UWORD uwID );
void		anim_BeginScript( void );
BOOL		anim_EndScript( void );
BOOL		anim_AddFrameToAnim( int iFrame, VECTOR3D vecPos,
									VECTOR3D vecRot, VECTOR3D vecScale );
BASEANIM *	anim_GetAnim( UWORD uwAnimID );
UWORD		anim_GetAnimID( char *szName );
iIMDShape *	anim_GetShapeFromID( UWORD uwID );
UWORD		anim_GetFrame3D( ANIM3D *psAnim, UWORD uwObj, UDWORD udwGameTime,
								UDWORD udwStartTime, UDWORD udwStartDelay,
								VECTOR3D *psVecPos, VECTOR3D *psVecRot,
								VECTOR3D *psVecScale );
void		anim_SetVals( char szFileName[], UWORD uwAnimID );

/***************************************************************************/

#endif	/* _ANIM_H_ */

/***************************************************************************/
