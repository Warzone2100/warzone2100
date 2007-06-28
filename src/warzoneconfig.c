/***************************************************************************/
/*
 * warzoneConfig.c
 *
 * warzone Global configuration functions.
 *
 */
/***************************************************************************/

#include "frame.h"
#include "warzoneconfig.h"
#include "piestate.h"
#include "advvis.h"

/***************************************************************************/
/*
 *	Global Variables
 */
/***************************************************************************/


/***************************************************************************/
/*
 *	Local Definitions
 */
/***************************************************************************/

typedef struct _warzoneGlobals
{
	SEQ_MODE	seqMode;
	BOOL		bFog;
	BOOL		bTranslucent;
	BOOL		bAdditive;
	SWORD		effectsLevel;
	char		DDrawDriverName[256];
	char		D3DDriverName[256];
	BOOL		allowSubtitles;
	BOOL		playAudioCDs;
	BOOL		Fullscreen;
} WARZONE_GLOBALS;

/***************************************************************************/
/*
 *	Local Variables
 */
/***************************************************************************/

static WARZONE_GLOBALS	warGlobs;//STATIC use or write an access function if you need any of this

/***************************************************************************/
/*
 *	Local ProtoTypes
 */
/***************************************************************************/

/***************************************************************************/
/*
 *	Source
 */
/***************************************************************************/
void war_SetDefaultStates(void)//Sets all states
{
	//set those here and reset in clParse or loadConfig
	pie_SetFogCap(FOG_CAP_UNDEFINED);
	pie_SetTexCap(TEX_CAP_UNDEFINED);
	war_SetFog(FALSE);
	war_SetTranslucent(FALSE);
	war_SetAdditive(FALSE);

	war_SetPlayAudioCDs(TRUE);
}

void war_SetPlayAudioCDs(BOOL b) {
	warGlobs.playAudioCDs = b;
}

BOOL war_GetPlayAudioCDs(void) {
	return warGlobs.playAudioCDs;
}

void war_SetAllowSubtitles(BOOL b) {
	warGlobs.allowSubtitles = b;
}

BOOL war_GetAllowSubtitles(void) {
	return warGlobs.allowSubtitles;
}

void war_setFullscreen(BOOL b) {
	warGlobs.Fullscreen = b;
}

BOOL war_getFullscreen(void) {
	return warGlobs.Fullscreen;
}

/***************************************************************************/
/***************************************************************************/
void war_SetFog(BOOL val)
{
	if (warGlobs.bFog != val)
	{
		warGlobs.bFog = val;
	}
	if (warGlobs.bFog == TRUE)
	{
		setRevealStatus(FALSE);
	}
	else
	{
		setRevealStatus(TRUE);
		pie_SetFogColour(0);
	}
}

BOOL war_GetFog(void)
{
	return  warGlobs.bFog;
}

/***************************************************************************/
/***************************************************************************/
void war_SetTranslucent(BOOL val)
{
	pie_SetTranslucent(val);
	if (warGlobs.bTranslucent != val)
	{
		warGlobs.bTranslucent = val;
	}
}

BOOL war_GetTranslucent(void)
{
	return  warGlobs.bTranslucent;
}

/***************************************************************************/
/***************************************************************************/
void war_SetAdditive(BOOL val)
{
	pie_SetAdditive(val);
	if (warGlobs.bAdditive != val)
	{
		warGlobs.bAdditive = val;
	}
}

BOOL war_GetAdditive(void)
{
	return  warGlobs.bAdditive;
}

/***************************************************************************/
/***************************************************************************/
void war_SetSeqMode(SEQ_MODE mode)
{
	warGlobs.seqMode = mode;
}

SEQ_MODE war_GetSeqMode(void)
{
	return  warGlobs.seqMode;
}

/***************************************************************************/
/***************************************************************************/
void war_SetDirectDrawDeviceName(char* pDDDeviceName)
{
	ASSERT((strlen(pDDDeviceName) < 255,"DirectDraw device string exceeds max string length."));
	if (strlen(pDDDeviceName) >= 255)
	{
		pDDDeviceName[255] = 0;
	}
	strcpy((char*)(warGlobs.DDrawDriverName),pDDDeviceName);
}

char* war_GetDirectDrawDeviceName(void)
{
	return (char*)(warGlobs.DDrawDriverName);
}

/***************************************************************************/
/***************************************************************************/
void war_SetDirect3DDeviceName(char* pD3DDeviceName)
{
	ASSERT((strlen(pD3DDeviceName) < 255,"Direct3D device string exceeds max string length."));
	if (strlen(pD3DDeviceName) >= 255)
	{
		pD3DDeviceName[255] = 0;
	}
	strcpy((char*)(warGlobs.D3DDriverName),pD3DDeviceName);
}

char* war_GetDirect3DDeviceName(void)
{
	return (char*)(warGlobs.D3DDriverName);
}

