/* 
 * mplayer.c
 * mplayer specific code. Keep it out of the main game!
 */

#include "frame.h"
#include "droid.h"
#include "netplay.h"
#include "multiplay.h"
#include "multistat.h"

#include "mpdpxtra.h"

// submission fields.
// obveall stats , based on local store 
#define MP_SCORE		1
#define MP_KILLS		2
#define MP_PLAYS		3
#define MP_WINS			4
#define MP_LOSES		5

// stats about this game only. More secure.
#define MP_GAME_SCORE	6
#define MP_GAME_KILLS	7	
#define MP_GAME_WIN		8			// should be 0 or 1.
#define MP_GAME_LOSE	9			// should be 0 or 1.

// submit score routines.
BOOL mplayerSubmit(void)
{
#ifdef INC_MPLAYER
	PLAYERSTATS stats,stats2;
	MPPLAYERID	mpID;

	if(!NetPlay.bLobbyLaunched)
	{
		return FALSE;
	}

	// just do it, it wont moan if it can't.....
	if( MPDPXTRA_Init() == MPDPXTRAERR_OK)
	{
		stats = getMultiStats(selectedPlayer,FALSE);

		// should use MPDPXTRA_AddScoreResultEx
		// and also use MPDPXTRA_DPIDToMPPLAYERID when the player joins.

		loadMultiStats(NetPlay.players[selectedPlayer].name, &stats2);

#if 0
		MPDPXTRA_AddScoreResult( NetPlay.players[selectedPlayer].dpid, 0, MP_SCORE,	stats.totalScore);
		MPDPXTRA_AddScoreResult( NetPlay.players[selectedPlayer].dpid, 0, MP_KILLS,	stats.totalKills);
		MPDPXTRA_AddScoreResult( NetPlay.players[selectedPlayer].dpid, 0, MP_PLAYS,	stats.played);
		MPDPXTRA_AddScoreResult( NetPlay.players[selectedPlayer].dpid, 0, MP_WINS,	stats.wins);
		MPDPXTRA_AddScoreResult( NetPlay.players[selectedPlayer].dpid, 0, MP_LOSES,	stats.loses);

		// now add stats for this game only. (more secure method)
		MPDPXTRA_AddScoreResult( NetPlay.players[selectedPlayer].dpid, 0, MP_GAME_SCORE,(stats.totalScore - stats2.totalScore));
		MPDPXTRA_AddScoreResult( NetPlay.players[selectedPlayer].dpid, 0, MP_GAME_KILLS,(stats.totalKills - stats2.totalKills));
		MPDPXTRA_AddScoreResult( NetPlay.players[selectedPlayer].dpid, 0, MP_GAME_WIN,	(stats.wins - stats2.wins));
		MPDPXTRA_AddScoreResult( NetPlay.players[selectedPlayer].dpid, 0, MP_GAME_LOSE,	(stats.loses - stats2.loses));

#else
		MPDPXTRA_DPIDToMPPLAYERID(NetPlay.players[selectedPlayer].dpid, &mpID);
		MPDPXTRA_AddScoreResultEx( mpID, 0, MP_SCORE,	stats.totalScore);
		MPDPXTRA_AddScoreResultEx( mpID, 0, MP_KILLS,	stats.totalKills);
		MPDPXTRA_AddScoreResultEx( mpID, 0, MP_PLAYS,	stats.played);
		MPDPXTRA_AddScoreResultEx( mpID, 0, MP_WINS,	stats.wins);
		MPDPXTRA_AddScoreResultEx( mpID, 0, MP_LOSES,	stats.loses);

		// now add stats for this game only. (more secure method)
		MPDPXTRA_AddScoreResultEx( mpID, 0, MP_GAME_SCORE,(stats.totalScore - stats2.totalScore));
		MPDPXTRA_AddScoreResultEx( mpID, 0, MP_GAME_KILLS,(stats.totalKills - stats2.totalKills));
		MPDPXTRA_AddScoreResultEx( mpID, 0, MP_GAME_WIN,	(stats.wins - stats2.wins));
		MPDPXTRA_AddScoreResultEx( mpID, 0, MP_GAME_LOSE,	(stats.loses - stats2.loses));
#endif



		//  submit to server.
		MPDPXTRA_SaveScoreResults();

		MPDPXTRA_Destroy();
	}

#endif
	return TRUE;
}
