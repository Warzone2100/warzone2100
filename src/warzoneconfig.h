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

#include "frame.h"

/***************************************************************************/
/*
 *	Global Definitions
 */
/***************************************************************************/
typedef	enum	SEQ_MODE
				{
					SEQ_FULL,
					SEQ_SMALL,
					SEQ_SKIP
				}
				SEQ_MODE;

/***************************************************************************/
/*
 *	Global ProtoTypes
 */
/***************************************************************************/
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
extern void war_SetPlayAudioCDs(BOOL b);
extern BOOL war_GetPlayAudioCDs(void);
extern void war_SetAllowSubtitles(BOOL);
extern BOOL war_GetAllowSubtitles(void);
extern void war_setFullscreen(BOOL);
extern BOOL war_getFullscreen(void);

#endif // _warzoneConfig_h
