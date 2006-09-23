/*
 *Config.c  saves your favourite options to the Registry.
 *
 */

#include "lib/framework/frame.h"
#include "lib/framework/strres.h"

#include "configuration.h"
#include "lib/framework/configfile.h"
#include "objmem.h"
#include "display.h"	// gammaValue
#include "lib/sound/track.h"		// audio
#include "lib/sound/cdaudio.h"	// audio
#include "lib/ivis_common/piestate.h"	// setgamma.
#include "warzoneconfig.h"	// renderMode
#include "component.h"
#include "text.h"
#include "seqdisp.h"
#include "difficulty.h"
#include "lib/netplay/netplay.h"
#include "display3d.h"
#include "multiplay.h"
#include "ai.h"
#include "advvis.h"
#include "lib/sound/mixer.h"
#include "hci.h"
#include "fpath.h"

#include "radar.h" //because of bDrawRadarTerrain

// ////////////////////////////////////////////////////////////////////////////

#define DEFAULTFXVOL	80
#define DEFAULTCDVOL	60
#define DEFAULTGAMMA	25
#define DEFAULTSCROLL	800
#define DEFAULTMAPNAME	"Rush"

extern  char	sForceName[256];
extern	UBYTE	sPlayer[128];

extern void registry_clear(void); // from configfile.c

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
	if(getWarzoneKeyNumeric("allowsubtitles", &val))
	{
		war_SetAllowSubtitles(val);
	}

	// //////////////////////////
	// voice vol
	if(getWarzoneKeyNumeric("voicevol", &val))
	{
		mixer_SetWavVolume((SDWORD)val);//was val
	}

	// //////////////////////////
	// fx vol
	if(getWarzoneKeyNumeric("fxvol", &val))
	{
		mixer_Set3dWavVolume((SDWORD)val);//was val
	}

	// //////////////////////////
	// cdvol
	if(getWarzoneKeyNumeric("cdvol", &val))
	{
		mixer_SetCDVolume((SDWORD)val);
	}

	if (getWarzoneKeyNumeric("playaudiocds", &val)) {
		war_SetPlayAudioCDs(val);
	}

	// //////////////////////////
	// gamma
	if(getWarzoneKeyNumeric("gamma", &val))
	{
		gammaValue =(float)val/(float)25;
		if(gammaValue<0.5)gammaValue =(float).5;
		pie_SetGammaValue(gammaValue);
	}
	else
	{
		gammaValue =(float)DEFAULTGAMMA/(float)25;
		pie_SetGammaValue(gammaValue);
		if(gammaValue<0.5)gammaValue =(float).5;
		setWarzoneKeyNumeric("gamma", DEFAULTGAMMA);
	}

	// //////////////////////////
	// scroll
	if(getWarzoneKeyNumeric("scroll", &val))
	{
		scroll_speed_accel = val;
	}
	else
	{
		scroll_speed_accel = DEFAULTSCROLL;
		setWarzoneKeyNumeric("scroll", DEFAULTSCROLL);
	}

	// //////////////////////////
	// screen shake
	if(getWarzoneKeyNumeric("shake", &val))
	{
		setShakeStatus(val);
	}
	else
	{
		setShakeStatus(TRUE);
		setWarzoneKeyNumeric("shake", TRUE);
	}

	// //////////////////////////
	// draw shadows
	if(getWarzoneKeyNumeric("shadows", &val))
	{
		setDrawShadows(val);
	}
	else
	{
		setDrawShadows(TRUE);
		setWarzoneKeyNumeric("shadows", TRUE);
	}

	// //////////////////////////
	// enable sound
	if(getWarzoneKeyNumeric("sound", &val))
	{
		war_setSoundEnabled( val );
	}
	else
	{
		war_setSoundEnabled( TRUE );
		setWarzoneKeyNumeric( "sound", TRUE );
	}

	// //////////////////////////
	// invert mouse
	if(getWarzoneKeyNumeric("mouseflip", &val))
	{
		setInvertMouseStatus(val);
	}
	else
	{
		setInvertMouseStatus(TRUE);
		setWarzoneKeyNumeric("mouseflip", TRUE);
	}

	// //////////////////////////
	// sequences
	if(getWarzoneKeyNumeric("sequences", &val))
	{
		war_SetSeqMode(val);
	}
	else
	{
		war_SetSeqMode(SEQ_FULL);
	}

	// //////////////////////////
	// subtitles
	if(getWarzoneKeyNumeric("subtitles", &val))
	{
		seq_SetSubtitles(val);
	}
	else
	{
		seq_SetSubtitles(TRUE);
	}

	// //////////////////////////
	// difficulty

	if(getWarzoneKeyNumeric("difficulty", &val))
	{
		setDifficultyLevel(val);
	}
	else
	{
		setDifficultyLevel(DL_NORMAL);
		setWarzoneKeyNumeric("difficulty", DL_NORMAL);
	}


	if(getWarzoneKeyNumeric("barmode", &val)&&(val<3)&&(val>=0))
	{
		barMode = val;
	}
	else
	{
		barMode = BAR_FULL;
//		setDifficultyLevel(DL_NORMAL);
		setWarzoneKeyNumeric("barmode", BAR_FULL);
	}


	// //////////////////////////
	// use vis fog
	if(getWarzoneKeyNumeric("visfog", &val))
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
		setWarzoneKeyNumeric("visfog", 0);
	}

	// //////////////////////////
	// favourite colour
	if(!bMultiPlayer)
	{
		initPlayerColours();	// clear current maps.
		if(getWarzoneKeyNumeric("colour", &val))
		{
			setPlayerColour(0, val);
		}
		else
		{
			setPlayerColour(0, 0);
			setWarzoneKeyNumeric("colour", 0);
		}
	}


	// reopen the build menu after placing a structure
	if(getWarzoneKeyNumeric("reopenBuild", &val))
	{
		intReopenBuild(val);
	}
	else
	{
		intReopenBuild(FALSE);
		setWarzoneKeyNumeric("reopenBuild", FALSE);
	}

	// the maximum route processing per frame
/*	if(getWarzoneKeyNumeric("maxRoute", &val))
	{
		fpathSetMaxRoute(val);
	}
	else
	{
		fpathSetMaxRoute(FPATH_MAX_ROUTE_INIT);
		setWarzoneKeyNumeric("maxRoute", FPATH_MAX_ROUTE_INIT);
	}*/

	// //////////////////////////
	//	getWarzoneKey("mouse", &val, 0);		// mouse
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
				strcpy(game.name, sBuf);
			}
			else
			{
				strcpy(game.name, strresGetString(psStringRes, STR_GAME_NAME));
				setWarzoneKeyString("gameName", game.name);
			}
		}

		// player name
		if(!NetPlay.bLobbyLaunched && !gameSpy.bGameSpy)// name will be set for us.
		{
			if(getWarzoneKeyString("playerName",(char*)&sBuf))
			{
				strcpy((STRING*)sPlayer, sBuf);
			}
			else
			{
				strcpy((STRING*)sPlayer, strresGetString(psStringRes, STR_PLAYER_NAME));
				setWarzoneKeyString("playerName",(STRING*)sPlayer);
			}
		}
	}

	// map name
	if(getWarzoneKeyString("mapName",(char*)&sBuf))
	{
		strcpy(game.map, sBuf);
	}
	else
	{
		strcpy(game.map, DEFAULTMAPNAME);
		setWarzoneKeyString("mapName", game.map);
	}


	// modem to use.
	if(getWarzoneKeyNumeric("modemId", &val))
	{
		ingame.modem= val;
	}
	else
	{
		ingame.modem= 0;
		setWarzoneKeyNumeric("modemId", ingame.modem);
	}


	// power
	if(getWarzoneKeyNumeric("power", &val))
	{
		game.power = val;
	}
	else
	{
		game.power = LEV_MED;
		setWarzoneKeyNumeric("power", game.power);
	}

	// fog
	if(getWarzoneKeyNumeric("fog", &val))
	{
		game.fog= val;
	}
	else
	{
		game.fog= TRUE;
		setWarzoneKeyNumeric("fog", game.fog);
	}

	//type
	if(getWarzoneKeyNumeric("type", &val))
	{
		game.type =(UBYTE)val;
	}
	else
	{
		game.type = CAMPAIGN;
		setWarzoneKeyNumeric("type", game.type);
	}

	//base
	if(getWarzoneKeyNumeric("base", &val))
	{
		game.base =(UBYTE)val;
	}
	else
	{
		game.base = CAMP_BASE;
		setWarzoneKeyNumeric("base", game.base);
	}

	//limit
	if(getWarzoneKeyNumeric("limit", &val))
	{
		game.limit=(UBYTE)val;
	}
	else
	{
		game.limit = NOLIMIT;
		setWarzoneKeyNumeric("limit", game.limit);
	}

	//maxplay
	if(getWarzoneKeyNumeric("maxPlay", &val))
	{
		game.maxPlayers =(UBYTE)val;
	}
	else
	{
		game.maxPlayers = 4;
		setWarzoneKeyNumeric("maxPlay", game.maxPlayers);
	}

	//compplay
	if(getWarzoneKeyNumeric("compPlay", &val))
	{
		game.bComputerPlayers= val;
	}
	else
	{
		game.bComputerPlayers = FALSE;
		setWarzoneKeyNumeric("compPlay", game.bComputerPlayers);
	}

	//alliance
	if(getWarzoneKeyNumeric("alliance", &val))
	{
		game.alliance =(UBYTE)val;
	}
	else
	{
		game.alliance = NO_ALLIANCES;
		setWarzoneKeyNumeric("alliance", game.alliance);
	}

	// force name
	if(getWarzoneKeyString("forceName",(char*)&sBuf))
	{
		strcpy(sForceName, sBuf);
	}
	else
	{
		strcpy(sForceName, "Default");
		setWarzoneKeyString("forceName", sForceName);
	}


	// favourite phrases
	if(getWarzoneKeyString("phrase0", ingame.phrases[0]))
	{
		getWarzoneKeyString("phrase1", ingame.phrases[1]);
		getWarzoneKeyString("phrase2", ingame.phrases[2]);
		getWarzoneKeyString("phrase3", ingame.phrases[3]);
		getWarzoneKeyString("phrase4", ingame.phrases[4]);
	}
	else
	{
		memset(&ingame.phrases, 0, sizeof(ingame.phrases));
		setWarzoneKeyString("phrase0", ingame.phrases[0]);
		setWarzoneKeyString("phrase1", ingame.phrases[1]);
		setWarzoneKeyString("phrase2", ingame.phrases[2]);
		setWarzoneKeyString("phrase3", ingame.phrases[3]);
		setWarzoneKeyString("phrase4", ingame.phrases[4]);
	}

	// enemy/allies radar view
	if(getWarzoneKeyNumeric("radarObjectMode", &val))
	{
		bEnemyAllyRadarColor =(BOOL)val;
	} else {
		bEnemyAllyRadarColor = FALSE;
		setWarzoneKeyNumeric("radarObjectMode", (DWORD)bEnemyAllyRadarColor);
	}

	// no-terrain radar view
	if(getWarzoneKeyNumeric("radarTerrainMode", &val))
	{
		bDrawRadarTerrain =(BOOL)val;
	} else {
		bDrawRadarTerrain = TRUE;
		setWarzoneKeyNumeric("radarTerrainMode", (DWORD)bDrawRadarTerrain);
	}

	return closeWarzoneKey();
}

BOOL loadRenderMode(void)
{
	char str[32];
	DWORD val;
	unsigned int w, h;

	if( !openWarzoneKey() ) {
		return FALSE;
	}

	if( getWarzoneKeyNumeric("fullscreen", &val) ) {
		war_setFullscreen(val);
	}

	// now load the desired res..
	// note that we only do this if we havent changed renderer..
	if( getWarzoneKeyString("resolution", str)
		&& sscanf(str, "%ix%i", &w, &h) == 2 ) {
			pie_SetVideoBuffer(w, h);
	}

	return closeWarzoneKey();
}

// ////////////////////////////////////////////////////////////////////////////
BOOL saveConfig(void)
{

	debug( LOG_WZ, "Writing prefs to registry\n" );

	if(!openWarzoneKey())
	{
		return FALSE;
	}

	// //////////////////////////
	// voicevol, fxvol and cdvol
	setWarzoneKeyNumeric("voicevol", mixer_GetWavVolume());
	setWarzoneKeyNumeric("fxvol", mixer_Get3dWavVolume());
	setWarzoneKeyNumeric("cdvol", mixer_GetCDVolume());
	setWarzoneKeyNumeric("playaudiocds", war_GetPlayAudioCDs());

	// res.
	{
		char buf[32];

		sprintf(buf, "%ix%i",
			      pie_GetVideoBufferWidth(),
				 pie_GetVideoBufferHeight());
		setWarzoneKeyString("resolution", buf);
	}

	setWarzoneKeyNumeric("fullscreen", war_getFullscreen());

	// dont save out the cheat mode.
	if(getDifficultyLevel()==DL_KILLER OR getDifficultyLevel()== DL_TOUGH)
	{
		setDifficultyLevel(DL_NORMAL);
	}
	setWarzoneKeyNumeric("allowSubtitles", war_GetAllowSubtitles());
	setWarzoneKeyNumeric("gamma",(DWORD)(gammaValue*25));			// gamma
	setWarzoneKeyNumeric("scroll",(DWORD)scroll_speed_accel);		// scroll
	setWarzoneKeyNumeric("difficulty", getDifficultyLevel());		// level
	setWarzoneKeyNumeric("barmode",(DWORD)barMode);			//energybars
	setWarzoneKeyNumeric("visfog",(DWORD)(!war_GetFog()));			// fogtype
	setWarzoneKeyNumeric("shake",(DWORD)(getShakeStatus()));		// screenshake
	setWarzoneKeyNumeric("mouseflip",(DWORD)(getInvertMouseStatus()));	// flipmouse
	setWarzoneKeyNumeric("shadows",(DWORD)(getDrawShadows()));	// shadows
	setWarzoneKeyNumeric("sequences",(DWORD)(war_GetSeqMode()));		// sequences
	setWarzoneKeyNumeric("subtitles",(DWORD)(seq_GetSubtitles()));		// subtitles
	setWarzoneKeyNumeric("reopenBuild",(DWORD)(intGetReopenBuild()));	// build menu
//	setWarzoneKeyNumeric("maxRoute",(DWORD)(fpathGetMaxRoute()));			// maximum routing

	setWarzoneKeyNumeric("radarObjectMode",(DWORD)bEnemyAllyRadarColor);    // enemy/allies radar view
	setWarzoneKeyNumeric("radarTerrainMode",(DWORD)bDrawRadarTerrain);

	if(!bMultiPlayer)
	{
		setWarzoneKeyNumeric("colour",(DWORD)getPlayerColour(0));			// favourite colour.
	}
	else
	{
		debug( LOG_NEVER, "Writing multiplay prefs to registry\n" );
		if(NetPlay.bHost && ingame.localJoiningInProgress)
		{
			setWarzoneKeyString("gameName", game.name);				//  last hosted game
		}
		setWarzoneKeyString("mapName", game.map);				//  map name
		setWarzoneKeyNumeric("power", game.power);			// power
		setWarzoneKeyNumeric("type", game.type);				// game type
		setWarzoneKeyNumeric("base", game.base);				// size of base
		setWarzoneKeyNumeric("fog", game.fog);				// fog 'o war
		setWarzoneKeyNumeric("limit", game.limit);			// limits
		setWarzoneKeyNumeric("maxPlay", game.maxPlayers);		// max no of players
		setWarzoneKeyNumeric("compPlay", game.bComputerPlayers);	// allow pc players
		setWarzoneKeyNumeric("alliance", game.alliance);			// allow alliances
		setWarzoneKeyNumeric("modemId", ingame.modem);			// modem to use.
		setWarzoneKeyString("forceName", sForceName);			// force
		setWarzoneKeyString("playerName",(STRING*)sPlayer);		// player name
		setWarzoneKeyString("phrase0", ingame.phrases[0]);	// phrases
		setWarzoneKeyString("phrase1", ingame.phrases[1]);
		setWarzoneKeyString("phrase2", ingame.phrases[2]);
		setWarzoneKeyString("phrase3", ingame.phrases[3]);
		setWarzoneKeyString("phrase4", ingame.phrases[4]);
	}

	return closeWarzoneKey();
}

void closeConfig( void )
{
	registry_clear();
}
