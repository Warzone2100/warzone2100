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

#include "frame.h"
#include "piedef.h"

/***************************************************************************/
/*
 *	Global Definitions
 */
/***************************************************************************/

typedef	enum	REND_ENGINE
				{
					ENGINE_UNDEFINED,
					ENGINE_4101,
					ENGINE_OPENGL
				}
				REND_ENGINE;

typedef	enum	REND_MODE
				{
					REND_GOURAUD_TEX,
					REND_ALPHA_TEX,
					REND_ADDITIVE_TEX,
					REND_TEXT,
					REND_ALPHA_TEXT,
					REND_FLAT,
					REND_ALPHA_FLAT,
					REND_ALPHA_ITERATED,
					REND_FILTER_FLAT,
					REND_FILTER_ITERATED			
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
					TRANS_DECAL_FOG,
					TRANS_FILTER,
					TRANS_ALPHA,
					TRANS_ADDITIVE
				}
				TRANSLUCENCY_MODE;

typedef	enum	FOG_CAP
				{
					FOG_CAP_NO,
					FOG_CAP_GREY,
					FOG_CAP_COLOURED,
					FOG_CAP_UNDEFINED
				}
				FOG_CAP;

typedef	enum	TEX_CAP
				{
					TEX_CAP_2M,
					TEX_CAP_8BIT,
					TEX_CAP_FULL,
					TEX_CAP_UNDEFINED
				}
				TEX_CAP;

#define NO_TEXPAGE -1

/***************************************************************************/
/*
 *	Global Variables
 */
/***************************************************************************/

extern SDWORD	pieStateCount;

/***************************************************************************/
/*
 *	Global ProtoTypes
 */
/***************************************************************************/
extern void pie_SetDefaultStates();//Sets all states
//render engine
extern void pie_SetRenderEngine(REND_ENGINE rendEngine);
extern REND_ENGINE pie_GetRenderEngine(void);
extern BOOL	pie_Hardware(void);
extern void pie_SetDepthBufferStatus(DEPTH_MODE depthMode);
extern void pie_SetGammaValue(float val);
//renderer capability
extern void pie_SetTranslucent(BOOL val);
extern BOOL pie_Translucent(void);
extern void pie_SetAdditive(BOOL val);
extern BOOL pie_Additive(void);
extern void pie_SetFogCap(FOG_CAP val);
extern FOG_CAP pie_GetFogCap(void);
extern void pie_SetTexCap(TEX_CAP val);
extern TEX_CAP pie_GetTexCap(void);
//fog available
extern void pie_EnableFog(BOOL val);
extern BOOL pie_GetFogEnabled(void);
//fog currently on
extern void pie_SetFogStatus(BOOL val);
extern BOOL pie_GetFogStatus(void);
extern void pie_SetFogColour(UDWORD colour);
extern UDWORD pie_GetFogColour(void);
//render states
extern void pie_SetTexturePage(SDWORD num);
extern void pie_SetBilinear(BOOL bilinearOn);
extern BOOL pie_GetBilinear(void);
extern void pie_SetColourKeyedBlack(BOOL keyingOn);
extern void pie_SetRendMode(REND_MODE rendMode);
extern void pie_SetColour(UDWORD val);
extern UDWORD pie_GetColour(void);
//mouse states
extern void pie_DrawMouse(SDWORD x, SDWORD y);
extern void pie_SetMouse(IMAGEFILE *ImageFile,UWORD ImageID);
extern UDWORD	pie_GetMouseID( void );
extern BOOL	pie_SwirlyBoxes( void );
extern void	pie_SetSwirlyBoxes( BOOL val );
extern BOOL	pie_WaveBlit( void );
extern void	pie_SetWaveBlit( BOOL val );
void pie_ResetStates(void);//Sets all states

#endif // _pieState_h
