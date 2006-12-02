/***************************************************************************/
/*
 * pieMode.h
 *
 * renderer control for pumpkin library functions.
 *
 */
/***************************************************************************/

#ifndef _pieMode_h
#define _pieMode_h

/***************************************************************************/

#include "lib/framework/frame.h"


/***************************************************************************/
/*
 *	Global Definitions
 */
/***************************************************************************/
typedef	enum	CLEAR_MODE
				{
					CLEAR_OFF,
					CLEAR_OFF_AND_NO_BUFFER_DOWNLOAD,
					CLEAR_BLACK,
					CLEAR_FOG,
				}
				CLEAR_MODE;


/***************************************************************************/
/*
 *	Global Variables
 */
/***************************************************************************/

extern Sint32	_iVPRIM_DIVTABLE[];

/***************************************************************************/
/*
 *	Global ProtoTypes
 */
/***************************************************************************/
extern BOOL pie_Initialise(void);
extern void pie_ShutDown(void);
extern void pie_ScreenFlip(CLEAR_MODE ClearMode);
extern void pie_Clear(UDWORD colour);
extern void pie_GlobalRenderBegin(void);
extern void pie_GlobalRenderEnd(BOOL bForceClearToBlack);
extern void pie_LocalRenderBegin(void);
extern void pie_LocalRenderEnd(void);
extern UDWORD	pie_GetResScalingFactor( void );

#endif
