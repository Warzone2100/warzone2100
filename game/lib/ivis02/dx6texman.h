/***************************************************************************/
/*
 * dx6TexMan.h
 *
 * renderer control for pumpkin library functions.
 *
 */
/***************************************************************************/

#ifndef _dx6TexMan_h
#define _dx6TexMan_h

/***************************************************************************/

#include "frame.h"


/***************************************************************************/
/*
 *	Global Definitions
 */
/***************************************************************************/

/***************************************************************************/
/*
 *	Global Variables
 */
/***************************************************************************/

/***************************************************************************/
/*
 *	Global ProtoTypes
 */
/***************************************************************************/
extern BOOL dtm_Initialise(void);
extern BOOL dtm_ReleaseTextures(void);
extern BOOL dtm_RestoreTextures(void);
extern BOOL dtm_ReloadAllTextures(void);
extern BOOL dtm_ReLoadTexture(SDWORD i);
extern void dtm_SetTexturePage(SDWORD i);
extern BOOL dtm_LoadTexSurface( iTexture *psIvisTex, SDWORD index );
extern BOOL dtm_LoadRadarSurface(BYTE* radarBuffer);
extern SDWORD dtm_GetRadarTexImageSize(void);

extern void dx6_SetBilinear( BOOL bBilinearOn );
#endif // 
