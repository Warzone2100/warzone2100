/***************************************************************************/
/*
 * warzoneConfig.c
 *
 * warzone Global configuration functions.
 *
 */
/***************************************************************************/

#include "frame.h"
#include "warzoneConfig.h"
#include "pieState.h"


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
	WAR_REND_MODE	renderMode;
//	TEX_MODE	textureMode;
	SEQ_MODE	seqMode;
	BOOL		bFog;
	BOOL		bTranslucent;
	BOOL		bAdditive;
	SWORD		effectsLevel;
	char		DDrawDriverName[256];
	char		D3DDriverName[256];
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
	warGlobs.renderMode = REND_MODE_SOFTWARE;
	pie_SetFogCap(FOG_CAP_UNDEFINED);//set here and reset in clParse or loadConfig
	pie_SetTexCap(TEX_CAP_UNDEFINED);//set here and reset in clParse or loadConfig
	war_SetFog(FALSE);//set here and reset in clParse or loadConfig
	war_SetTranslucent(FALSE);//set here and reset in clParse or loadConfig
	war_SetAdditive(FALSE);//set here and reset in clParse or loadConfig
}

/***************************************************************************/
/***************************************************************************/
void war_SetRendMode(WAR_REND_MODE rendMode)
{
	if (warGlobs.renderMode != rendMode)
	{
		warGlobs.renderMode = rendMode;
		if (rendMode == REND_MODE_SOFTWARE)
		{
			war_SetFog(FALSE);//set here and turned off in clParse or loadConfig
			war_SetTranslucent(FALSE);//set here and turned off in clParse or loadConfig
			war_SetAdditive(FALSE);//set here and turned off in clParse or loadConfig
		}
		else
		{
			war_SetFog(TRUE);//set here and turned off in clParse or loadConfig
			war_SetTranslucent(TRUE);//set here and turned off in clParse or loadConfig
			war_SetAdditive(TRUE);//set here and turned off in clParse or loadConfig
		}
	}
}

WAR_REND_MODE war_GetRendMode(void)
{
	return warGlobs.renderMode;
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

