/*
 *Config.c  saves your favourite options to the Registry.
 * 
 */

#include "frame.h"

#include "objmem.h"
#include "display.h"	// gamma
#include "track.h"		// audio
#include "cdaudio.h"	// audio
#include "piestate.h"	// setgamma.
#include "warzoneConfig.h"	// renderMode
#include "component.h"
#include "text.h"
#include "seqdisp.h"
#include "difficulty.h"
#include "netplay.h"
#include "display3d.h"
#include "multiplay.h"
#include "ai.h"
#include "AdvVis.h"
#include "mixer.h"
#include "HCI.h"
#include "Fpath.h"
#include "d3drender.h"

// ////////////////////////////////////////////////////////////////////////////

#define DEFAULTFXVOL	80
#define DEFAULTCDVOL	60
#define DEFAULTGAMMA	25
#define DEFAULTSCROLL	800
#define DEFAULTMAPNAME	"Rush"

// ////////////////////////////////////////////////////////////////////////////
BOOL openWarzoneKey			(VOID);
BOOL closeWarzoneKey		(VOID);
BOOL getWarzoneKeyNumeric	(STRING *pName,DWORD  *val);
BOOL getWarzoneKeyString	(STRING *pName,STRING *pString);
BOOL getWarzoneKeyBinary	(STRING *pName,UCHAR  *pData,UDWORD *pSize);
BOOL setWarzoneKeyNumeric	(STRING *pName,DWORD val);
BOOL setWarzoneKeyString	(STRING *pName,STRING *pString);
BOOL setWarzoneKeyBinary	(STRING *pName, VOID *pData, UDWORD size);
BOOL loadRenderMode			(VOID);
BOOL loadConfig				(BOOL bResourceAvailable);
BOOL saveConfig				(VOID);

static HKEY		ghWarzoneKey=NULL;

extern  char	sForceName[256];
extern	UBYTE	sPlayer[128];

BOOL	bAllowSubtitles=FALSE;

// ////////////////////////////////////////////////////////////////////////////
BOOL loadConfig(BOOL bResourceAvailable)
{
	DWORD	val;
	STRING	sBuf[255];

	if(!openWarzoneKey())
	{
		return FALSE;
	}

	//  options screens.
	// //////////////////////////

	// //////////////////////////
	// subtitles
	if( getWarzoneKeyNumeric("allowsubtitles",&val)) 
	{
		bAllowSubtitles = val;
		if (!bAllowSubtitles)
		{
			setWarzoneKeyNumeric("subtitles",(DWORD)TRUE);			// subtitles
		}
	}

	// //////////////////////////
	// fx vol
	if( getWarzoneKeyNumeric("fxvol",&val)) 
	{
		mixer_SetWavVolume((SDWORD)val);
	}
	
	// //////////////////////////
	// cdvol
	if( getWarzoneKeyNumeric("cdvol",&val))		
	{
		mixer_SetCDVolume((SDWORD)val);
	}

	// //////////////////////////
	// gamma
	if(getWarzoneKeyNumeric("gamma",&val))		
	{
		gamma = (float)val/(float)25;
		if(gamma<0.5)  gamma = (float).5;
		pie_SetGammaValue(gamma);
	}
	else
	{
		gamma = (float)DEFAULTGAMMA/(float)25;
		pie_SetGammaValue(gamma);
		if(gamma<0.5)  gamma = (float).5;
		setWarzoneKeyNumeric("gamma",DEFAULTGAMMA);		
	}
	
	// //////////////////////////
	// scroll
	if(getWarzoneKeyNumeric("scroll",&val))		
	{
		scroll_speed_accel = val;
	}
	else
	{
		scroll_speed_accel = DEFAULTSCROLL;
		setWarzoneKeyNumeric("scroll",DEFAULTSCROLL);
	}

	// //////////////////////////
	// screen shake
	if(getWarzoneKeyNumeric("shake",&val))		
	{
		setShakeStatus(val);
	}
	else
	{
		setShakeStatus(TRUE);
		setWarzoneKeyNumeric("shake",TRUE);
	}

	// //////////////////////////
	// invert mouse
	if(getWarzoneKeyNumeric("mouseflip",&val))		
	{
		setInvertMouseStatus(val);
	}
	else
	{
		setInvertMouseStatus(TRUE);
		setWarzoneKeyNumeric("mouseflip",TRUE);
	}

	// //////////////////////////
	// sequences
	if(getWarzoneKeyNumeric("sequences",&val))		
	{
		war_SetSeqMode(val);
	}
	else
	{
		war_SetSeqMode(SEQ_FULL);
	}

	// //////////////////////////
	// subtitles
	if(getWarzoneKeyNumeric("subtitles",&val))		
	{
		seq_SetSubtitles(val);
	}
	else
	{
		seq_SetSubtitles(TRUE);
	}

	// //////////////////////////
	// difficulty

	if(getWarzoneKeyNumeric("difficulty",&val))		
	{
		setDifficultyLevel(val);
	}
	else
	{
		setDifficultyLevel(DL_NORMAL);
		setWarzoneKeyNumeric("difficulty",DL_NORMAL);
	}

	
	if(getWarzoneKeyNumeric("barmode",&val) && (val<3) && (val>=0) )		
	{
		barMode = val;
	}
	else
	{
		barMode = BAR_FULL;
//		setDifficultyLevel(DL_NORMAL);
		setWarzoneKeyNumeric("barmode",BAR_FULL);
	}


	// //////////////////////////
	// use vis fog
	if(getWarzoneKeyNumeric("visfog",&val))
	{
		if(val)
		{
			war_SetFog(FALSE);
			avSetStatus(TRUE);
		}
		else
		{
			avSetStatus(FALSE);
			war_SetFog(TRUE);
		}
	}
	else
	{
		avSetStatus(FALSE);
		war_SetFog(TRUE);
		setWarzoneKeyNumeric("visfog",0);
	}

	// //////////////////////////
	// favourite colour
	if(!bMultiPlayer)
	{
		initPlayerColours();	// clear current maps.
		if(getWarzoneKeyNumeric("colour",&val))	
		{
			setPlayerColour(0,val);
		}
		else
		{
			setPlayerColour(0,0);
			setWarzoneKeyNumeric("colour",0);
		}
	}


	// reopen the build menu after placing a structure
	if(getWarzoneKeyNumeric("reopenBuild",&val))		
	{
		intReopenBuild(val);
	}
	else
	{
		intReopenBuild(FALSE);
		setWarzoneKeyNumeric("reopenBuild",FALSE);
	}

	// the maximum route processing per frame
/*	if(getWarzoneKeyNumeric("maxRoute",&val))		
	{
		fpathSetMaxRoute(val);
	}
	else
	{
		fpathSetMaxRoute(FPATH_MAX_ROUTE_INIT);
		setWarzoneKeyNumeric("maxRoute",FPATH_MAX_ROUTE_INIT);
	}*/

	// //////////////////////////
	//	getWarzoneKey("mouse",&val, 0);		// mouse
	//	multitype // alliance // power // base // limits // tech
	// keymaps

	// /////////////////////////
	//  multiplayer stuff.
	// /////////////////////////

	if(bResourceAvailable)
	{
		// game name
		if(!NetPlay.bLobbyLaunched && !gameSpy.bGameSpy)
		{
			if(getWarzoneKeyString("gameName",(char*)&sBuf))		
			{
				strcpy(game.name,sBuf);
			}
			else
			{
				strcpy(game.name,strresGetString(psStringRes, STR_GAME_NAME));
				setWarzoneKeyString("gameName",game.name);
			}
		}

		// player name
		if(!NetPlay.bLobbyLaunched && !gameSpy.bGameSpy)	// name will be set for us.
		{
			if(getWarzoneKeyString("playerName",(char*)&sBuf))		
			{
				strcpy((STRING*)sPlayer,sBuf);
			}
			else
			{
				strcpy((STRING*)sPlayer,strresGetString(psStringRes, STR_PLAYER_NAME));
				setWarzoneKeyString("playerName",(STRING*)sPlayer);
			}
		}
	}

	// map name
	if(getWarzoneKeyString("mapName",(char*)&sBuf))		
	{
		strcpy(game.map,sBuf);
	}
	else
	{
		strcpy(game.map,DEFAULTMAPNAME);
		setWarzoneKeyString("mapName",game.map);
	}

	
	// modem to use.
	if(getWarzoneKeyNumeric("modemId",&val))		
	{
		ingame.modem= val;
	}
	else
	{
		ingame.modem= 0;
		setWarzoneKeyNumeric("modemId",ingame.modem);
	}


	// power
	if(getWarzoneKeyNumeric("power",&val))		
	{
		game.power = val;
	}
	else
	{
		game.power = LEV_MED;
		setWarzoneKeyNumeric("power",game.power);
	}

	// fog
	if(getWarzoneKeyNumeric("fog",&val))		
	{
		game.fog= val;
	}
	else
	{
		game.fog= TRUE;
		setWarzoneKeyNumeric("fog",game.fog);
	}

	//type
	if(getWarzoneKeyNumeric("type",&val))		
	{
		game.type = (UBYTE)val;
	}
	else
	{
		game.type = CAMPAIGN;
		setWarzoneKeyNumeric("type",game.type);
	}

	//base
	if(getWarzoneKeyNumeric("base",&val))		
	{
		game.base = (UBYTE)val;
	}
	else
	{
		game.base = CAMP_BASE;
		setWarzoneKeyNumeric("base",game.base);
	}

	//limit
	if(getWarzoneKeyNumeric("limit",&val))		
	{
		game.limit= (UBYTE)val;
	}
	else
	{
		game.limit = NOLIMIT;
		setWarzoneKeyNumeric("limit",game.limit);
	}

	//maxplay
	if(getWarzoneKeyNumeric("maxPlay",&val))		
	{
		game.maxPlayers = (UBYTE)val;
	}
	else
	{
		game.maxPlayers = 4;
		setWarzoneKeyNumeric("maxPlay",game.maxPlayers);
	}

	//compplay
	if(getWarzoneKeyNumeric("compPlay",&val))		
	{
		game.bComputerPlayers= val;
	}
	else
	{
		game.bComputerPlayers = FALSE;
		setWarzoneKeyNumeric("compPlay",game.bComputerPlayers);
	}

	//alliance
	if(getWarzoneKeyNumeric("alliance",&val))		
	{
		game.alliance = (UBYTE)val;
	}
	else
	{
		game.alliance = NO_ALLIANCES;
		setWarzoneKeyNumeric("alliance",game.alliance);
	}
		
	// force name
	if(getWarzoneKeyString("forceName",(char*)&sBuf))		
	{
		strcpy(	sForceName,sBuf);
	}
	else
	{
		strcpy(sForceName,"Default");
		setWarzoneKeyString("forceName",sForceName);
	}


	// favourite phrases
	if(getWarzoneKeyString("phrase0",ingame.phrases[0]))		
	{
		getWarzoneKeyString("phrase1",ingame.phrases[1]);
		getWarzoneKeyString("phrase2",ingame.phrases[2]);
		getWarzoneKeyString("phrase3",ingame.phrases[3]);
		getWarzoneKeyString("phrase4",ingame.phrases[4]);
	}
	else
	{
		memset(&ingame.phrases,0,sizeof(ingame.phrases));
		setWarzoneKeyString("phrase0",ingame.phrases[0]);
		setWarzoneKeyString("phrase1",ingame.phrases[1]);
		setWarzoneKeyString("phrase2",ingame.phrases[2]);
		setWarzoneKeyString("phrase3",ingame.phrases[3]);
		setWarzoneKeyString("phrase4",ingame.phrases[4]);
	}


	return closeWarzoneKey();
}

BOOL loadRenderMode()
{
	DWORD	val;

	if(!openWarzoneKey())
	{
		return FALSE;
	}

	// renderMode
	if(getWarzoneKeyNumeric("renderMode",&val))		
	{
		war_SetRendMode(val);
		if (val == REND_MODE_HAL)//d3d
		{
			if(getWarzoneKeyNumeric("d3dFog",&val))
			{
				war_SetFog(val);
			}

//			if(getWarzoneKeyNumeric("d3dTrans",&val))
//			{
//				switch(val)
//				{
//				case 0:
//					war_SetTranslucent(FALSE);
//					war_SetAdditive(FALSE);
//					break;
//				case 1:
//					war_SetTranslucent(TRUE);
//					war_SetAdditive(FALSE);
//					break;
//				default:
//					war_SetTranslucent(TRUE);
//					war_SetAdditive(TRUE);
//					break;
//				}
//			}

		}
	}
	else
	{
		setWarzoneKeyNumeric("renderMode", war_GetRendMode());
		if (war_GetRendMode() == REND_MODE_HAL)//d3d
		{
				setWarzoneKeyNumeric("d3dFog",war_GetFog());

//				if (war_GetAdditive())
//				{
//					setWarzoneKeyNumeric("d3dTrans",2);
//				}
//				else if (war_GetTranslucent())
//				{
//					setWarzoneKeyNumeric("d3dTrans",1);
//				}
//				else
//				{
//					setWarzoneKeyNumeric("d3dTrans",0);
//				}

		}
	}


	// now load the desired res..
	// note that we only do this if we havent changed renderer..
	if(getWarzoneKeyNumeric("resolution",&val))		
	{	
		switch(val)
		{
		case 640:
			pie_SetVideoBufferWidth(640);
			pie_SetVideoBufferHeight(480);
			break;
		case 800:
			pie_SetVideoBufferWidth(800);
			pie_SetVideoBufferHeight(600);
			break;
		case 960:
			pie_SetVideoBufferWidth(960);
			pie_SetVideoBufferHeight(720);
			break;
		case 1024:
			pie_SetVideoBufferWidth(1024);
			pie_SetVideoBufferHeight(768);
			break;
		case 1152:
			pie_SetVideoBufferWidth(1152);
			pie_SetVideoBufferHeight(864);
			break;
		case 1280:
			pie_SetVideoBufferWidth(1280);
			pie_SetVideoBufferHeight(1024);
			break;
		}
	}

	// now load the desired res..
	// note that we only do this if we havent changed renderer..
	if(getWarzoneKeyNumeric("resolution",&val))		
	{	
		switch(val)
		{
		case 640:
			pie_SetVideoBufferWidth(640);
			pie_SetVideoBufferHeight(480);
			break;
		case 800:
			pie_SetVideoBufferWidth(800);
			pie_SetVideoBufferHeight(600);
			break;
		case 960:
			pie_SetVideoBufferWidth(960);
			pie_SetVideoBufferHeight(720);
			break;
		case 1024:
			pie_SetVideoBufferWidth(1024);
			pie_SetVideoBufferHeight(768);
			break;
		case 1152:
			pie_SetVideoBufferWidth(1152);
			pie_SetVideoBufferHeight(864);
			break;
		case 1280:
			pie_SetVideoBufferWidth(1280);
			pie_SetVideoBufferHeight(1024);
			break;
		}
	}

	// NVidia texel offset hacks
	if(getWarzoneKeyNumeric("TexelOffsetOn",&val))		
	{
		D3DSetTexelOffsetState( val );
	}
	else
	{
		D3DSetTexelOffsetState( TRUE );
		setWarzoneKeyNumeric("TexelOffsetOn",1);
	}


	return closeWarzoneKey();
}	
// ////////////////////////////////////////////////////////////////////////////
BOOL saveConfig()
{

	DBPRINTF(("Writing prefs to registry\n"));

	if(!openWarzoneKey())
	{
		return FALSE;
	}

	// //////////////////////////
	// fxvol and cdvol
	setWarzoneKeyNumeric("fxvol", mixer_GetWavVolume());
	setWarzoneKeyNumeric("cdvol", mixer_GetCDVolume());

	// note running rendermode
	// ENGINE_GLIDE etc.
	setWarzoneKeyNumeric("renderMode", war_GetRendMode());
	if (war_GetRendMode() == REND_MODE_HAL)//d3d
	{
			setWarzoneKeyNumeric("d3dFog",war_GetFog());

//			if (war_GetAdditive())
//			{
//				setWarzoneKeyNumeric("d3dTrans",2);
//			}
//			else if (war_GetTranslucent())
//			{
//				setWarzoneKeyNumeric("d3dTrans",1);
//			}
//			else
//			{
//				setWarzoneKeyNumeric("d3dTrans",0);
//			}

	}

	// res.
	setWarzoneKeyNumeric("resolution",pie_GetVideoBufferWidth());


	// dont save out the cheat mode.
	if(getDifficultyLevel()==DL_KILLER OR getDifficultyLevel() == DL_TOUGH)
	{
		setDifficultyLevel(DL_NORMAL);
	}
	setWarzoneKeyNumeric(	"allowSubtitles",(DWORD)bAllowSubtitles			);
	setWarzoneKeyNumeric(	"gamma",		(DWORD)(gamma*25)				);			// gamma
	setWarzoneKeyNumeric(	"scroll",		(DWORD)scroll_speed_accel		);			// scroll
	setWarzoneKeyNumeric(	"difficulty",	getDifficultyLevel()			);			// level	
	setWarzoneKeyNumeric(	"barmode",		(DWORD)barMode							);			//energybars		
	setWarzoneKeyNumeric(	"visfog",		(DWORD)(!war_GetFog())			);			// fogtype
	setWarzoneKeyNumeric(	"shake",		(DWORD)(getShakeStatus())		);			// screenshake
	setWarzoneKeyNumeric(	"mouseflip",	(DWORD)(getInvertMouseStatus())	);			// flipmouse
	setWarzoneKeyNumeric(	"sequences",	(DWORD)(war_GetSeqMode()	)	);			// sequences
	setWarzoneKeyNumeric(	"subtitles",	(DWORD)(seq_GetSubtitles()	)	);			// subtitles
	setWarzoneKeyNumeric(	"reopenBuild",	(DWORD)(intGetReopenBuild())	);			// build menu
//	setWarzoneKeyNumeric(	"maxRoute",		(DWORD)(fpathGetMaxRoute())		);			// maximum routing

	if(!bMultiPlayer)
	{
		setWarzoneKeyNumeric("colour",		(DWORD)getPlayerColour(0)		);			// favourite colour.
	}
	else
	{
		DBPRINTF(("Writing multiplay prefs to registry\n"));
		if(NetPlay.bHost && ingame.localJoiningInProgress)
		{
			setWarzoneKeyString("gameName",	game.name);				//  last hosted game
		}
		setWarzoneKeyString(	"mapName",	game.map);				//  map name	
		setWarzoneKeyNumeric(	"power",	game.power);			// power
		setWarzoneKeyNumeric(	"type",		game.type);				// game type
		setWarzoneKeyNumeric(	"base",		game.base);				// size of base
		setWarzoneKeyNumeric(	"fog",		game.fog);				// fog 'o war
		setWarzoneKeyNumeric(	"limit",	game.limit);			// limits 
		setWarzoneKeyNumeric(	"maxPlay",	game.maxPlayers);		// max no of players
		setWarzoneKeyNumeric(	"compPlay",	game.bComputerPlayers);	// allow pc players
		setWarzoneKeyNumeric(	"alliance",	game.alliance);			// allow alliances
		setWarzoneKeyNumeric(	"modemId",	ingame.modem);			// modem to use.
		setWarzoneKeyString(	"forceName",sForceName);			// force
		setWarzoneKeyString(	"playerName",(STRING*)sPlayer);		// player name
		setWarzoneKeyString(	"phrase0",	ingame.phrases[0] );	// phrases
		setWarzoneKeyString(	"phrase1",	ingame.phrases[1] );	
		setWarzoneKeyString(	"phrase2",	ingame.phrases[2] );	
		setWarzoneKeyString(	"phrase3",	ingame.phrases[3] );	
		setWarzoneKeyString(	"phrase4",	ingame.phrases[4] );	
	}

	return closeWarzoneKey();
}

// ////////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////////

BOOL openWarzoneKey()
{
	DWORD	result,ghGameDisp;						// key created or opened
	char	keyname[256] = "SOFTWARE\\Pumpkin Studios\\Warzone2100\\";

	result = RegCreateKeyEx(						// create/open key.
		HKEY_LOCAL_MACHINE,							// handle of an open key 
		keyname,									// address of subkey name 
		0,											// reserved 
	    NULL,										// address of class string 
		REG_OPTION_NON_VOLATILE,					// special options flag 
	    KEY_ALL_ACCESS,								// desired security access 
		NULL,										// address of key security structure 
		&ghWarzoneKey,								// address of buffer for opened handle  
		&ghGameDisp									// address of disposition value buffer 
		);

	if(result == ERROR_SUCCESS)
	{
		return TRUE;
	}
	return FALSE;
}

// ////////////////////////////////////////////////////////////////////////////
BOOL closeWarzoneKey()
{
	if( RegCloseKey(ghWarzoneKey) == ERROR_SUCCESS)
	{
		return TRUE;
	}
	return FALSE;
}

// ////////////////////////////////////////////////////////////////////////////
BOOL getWarzoneKeyNumeric	(STRING *pName,DWORD *val)
{
	UCHAR	result[16];			// buffer
	DWORD	resultsize=16;		// buffersize
	DWORD	type;				// type of reg entry.

	// check guid exists
	if(RegQueryValueEx(ghWarzoneKey,pName,NULL, &type, (UCHAR*)&result,&resultsize)	!=  ERROR_SUCCESS)
	{
//		RegCloseKey(ghWarzoneKey);
		return FALSE;
	}
	if (type == REG_DWORD)
	{
		memcpy(val,&result[0],sizeof(DWORD));
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

// ////////////////////////////////////////////////////////////////////////////
BOOL getWarzoneKeyString	(STRING *pName,STRING *pString)
{
	UCHAR	result[255];		// buffer
	DWORD	resultsize=255;		// buffersize
	DWORD	type;				// type of reg entry.

	// check guid exists
	if(RegQueryValueEx(ghWarzoneKey,pName,NULL, &type, (UCHAR*)&result,&resultsize)	!=  ERROR_SUCCESS)
	{
	//	RegCloseKey(ghWarzoneKey);
		return FALSE;
	}
	if(type == REG_SZ)
	{
		strcpy(pString,(CHAR*)result);		
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

// ////////////////////////////////////////////////////////////////////////////
BOOL getWarzoneKeyBinary	(STRING *pName,UCHAR *pData,UDWORD *pSize)
{
	UCHAR	result[2048];		// buffer
	DWORD	resultsize=2048;	// buffersize
	DWORD	type;				// type of reg entry.

	// check guid exists
	if(RegQueryValueEx(ghWarzoneKey,pName,NULL, &type, (UCHAR*)&result,&resultsize)	!=  ERROR_SUCCESS)
	{
	//	RegCloseKey(ghWarzoneKey);
		*pSize =0;
		return FALSE;
	}
	if(type == REG_BINARY)
	{
		memcpy(pData,&result[0],resultsize);
		*pSize = resultsize;
		return TRUE;
	}
	else
	{
		pSize = 0;
		return FALSE;
	}
}




// ////////////////////////////////////////////////////////////////////////////
BOOL setWarzoneKeyNumeric(STRING *pName,DWORD val)
{

	RegSetValueEx(ghWarzoneKey, pName ,0,REG_DWORD ,(UBYTE*) &val, sizeof(val) );
	return TRUE;
}

// ////////////////////////////////////////////////////////////////////////////
BOOL setWarzoneKeyString(STRING *pName, STRING *pString)
{
	RegSetValueEx(ghWarzoneKey, pName ,0, REG_SZ, (UBYTE*)pString, strlen(pString) );
	return TRUE;
}

// ////////////////////////////////////////////////////////////////////////////
BOOL setWarzoneKeyBinary(STRING *pName, VOID *pData, UDWORD size)
{
	RegSetValueEx(ghWarzoneKey, pName ,0, REG_BINARY, pData, size );
	return TRUE;
}
