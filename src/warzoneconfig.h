/***************************************************************************/
/*
 * warzoneConfig.h
 *
 * warzone Global configuration functions.
 *
 */
/***************************************************************************/

#ifndef _warzoneConfig_h
#define _warzoneConfig_h


/***************************************************************************/

#include "frame.h"

/***************************************************************************/
/*
 *	Global Definitions
 */
/***************************************************************************/
typedef	enum	WAR_REND_MODE
				{
					REND_MODE_SOFTWARE,
					REND_MODE_GLIDE,
					REND_MODE_RGB,
					REND_MODE_HAL,
					REND_MODE_HAL2,
					REND_MODE_REF,
				}
				WAR_REND_MODE;

/*
typedef	enum	TEX_MODE
				{
					TEX_1MEG,
					TEX_2MEG,
					TEX_4MEG,
					TEX_8BIT
				}
				TEX_MODE;
*/

typedef	enum	SEQ_MODE
				{
					SEQ_FULL,
					SEQ_SMALL,
					SEQ_SKIP
				}
				SEQ_MODE;
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
extern void war_SetRendMode(WAR_REND_MODE mode);
extern WAR_REND_MODE war_GetRendMode(void);
extern void	war_SetDefaultStates(void);
extern void war_SetFog(BOOL val);
extern BOOL war_GetFog(void);
extern void war_SetTranslucent(BOOL val);
extern BOOL war_GetTranslucent(void);
extern void war_SetAdditive(BOOL val);
extern BOOL war_GetAdditive(void);
extern void war_SetSeqMode(SEQ_MODE mode);
extern SEQ_MODE war_GetSeqMode(void);
extern void war_SetDirectDrawDeviceName(char* pDDDeviceName);
extern char* war_GetDirectDrawDeviceName(void);
extern void war_SetDirect3DDeviceName(char* pD3DDeviceName);
extern char* war_GetDirect3DDeviceName(void);

#endif // _warzoneConfig_h
