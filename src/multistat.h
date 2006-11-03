/*
 *  multistat.h
 *
 *   Alex Lee
 *
 * Pumpkin Studios, Eidos PLC, Jan98.
 * Definitions for multi player statistics and scores for league tables.
 * Also Definitions for saved Arena Forces to enable teams to be saved to disk
 */

typedef struct{
	DWORD		played;						/// propogated stats.
	DWORD		wins;
	DWORD		loses;
	DWORD		totalKills;
	SDWORD		totalScore;

	DWORD		recentKills;				// score/kills in last game.
	SDWORD		recentScore;

	DWORD		killsToAdd;					// things to add next time score is updated.
	SDWORD		scoreToAdd;

} PLAYERSTATS, *LPPLAYERSTATS;

typedef struct{								// complete info.
	char		name[255];
	PLAYERSTATS	stats;
	UBYTE		padding[4];					// used to fill encryption array & confuse enduser.
}SAVEDPLAYERSTATS, *LPSAVEDPLAYERSTATS;


typedef struct _forcemember {
	DROID_TEMPLATE		*pTempl;			// pointer to template to use for this droid
	struct _forcemember	*psNext;			// Pointer to next template
}FORCE_MEMBER;

typedef struct _force {
	DROID_TEMPLATE	*pForceTemplates;
	FORCE_MEMBER	*pMembers;
}FORCE;

extern FORCE Force;														// the selected force.

//  Force defs.
extern BOOL	removeFromForce			(UDWORD number);					// remove a droid from force
extern BOOL	addToForce				(DROID_TEMPLATE  *templ);			// add a droid (templ) to force
extern void	useTheForce				(BOOL bAddTempl);					// place the force in the game
extern BOOL	saveForce				(char *name,FORCE *pfForce);
extern BOOL	loadForce				(char *name);

// stat defs
extern BOOL			saveMultiStats			(char *sFName, char *sPlayerName,PLAYERSTATS *playerStats);	// to disk
extern BOOL			loadMultiStats			(char *sPlayerName,PLAYERSTATS *playerStats);					// form disk
extern PLAYERSTATS	getMultiStats			(UDWORD player,BOOL bLocal);									// get from net
extern BOOL			setMultiStats			(DWORD playerDPID, PLAYERSTATS plStats,BOOL bLocal);			// send to net.
extern void			updateMultiStatsDamage	(UDWORD attacker, UDWORD defender, UDWORD inflicted);
extern void			updateMultiStatsGames	(void);
extern void			updateMultiStatsWins	(void);
extern void			updateMultiStatsLoses	(void);
extern void			updateMultiStatsKills	(BASE_OBJECT *psKilled,UDWORD player);

