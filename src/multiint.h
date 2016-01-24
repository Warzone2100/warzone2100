/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2013  Warzone 2100 Project

	Warzone 2100 is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	Warzone 2100 is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Warzone 2100; if not, write to the Free Software
	Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
*/
/** @file
 *  Interface defines/externs for warzone frontend.
 */

#ifndef __INCLUDED_SRC_MULTIINT_H__
#define __INCLUDED_SRC_MULTIINT_H__

#include "lib/netplay/netplay.h"
#include "lib/widget/widgbase.h"

#define MAX_LEN_AI_NAME   40
#define AI_CUSTOM        127
#define AI_OPEN           -2
#define AI_CLOSED         -1
#define AI_NOT_FOUND     -99

void readAIs();	///< step 1, load AI definition files
void loadMultiScripts();	///< step 2, load the actual AI scripts
const char *getAIName(int player);	///< only run this -after- readAIs() is called
int matchAIbyName(const char *name);	///< only run this -after- readAIs() is called
int getNextAIAssignment(const char *name);

extern LOBBY_ERROR_TYPES getLobbyError(void);
extern void setLobbyError(LOBBY_ERROR_TYPES error_type);

extern	void	runConnectionScreen(void);
extern	bool	startConnectionScreen(void);
extern	void	intProcessConnection(UDWORD id);

extern	void	runGameFind(void);
extern	void	startGameFind(void);

void updateLimitFlags(void);

extern	void	runMultiOptions(void);
extern	bool	startMultiOptions(bool bReenter);
extern	void	frontendMultiMessages(void);

bool addMultiBut(W_SCREEN *screen, UDWORD formid, UDWORD id, UDWORD x, UDWORD y, UDWORD width, UDWORD height, const char *tipres, UDWORD norm, UDWORD down, UDWORD hi, unsigned tc = MAX_PLAYERS);
bool changeColour(unsigned player, int col, bool isHost);
extern	char	sPlayer[128];

extern bool bHosted;

void	kickPlayer(uint32_t player_id, const char *reason, LOBBY_ERROR_TYPES type);
void	addPlayerBox(bool);			// players (mid) box
void loadMapPreview(bool hideInterface);


// ////////////////////////////////////////////////////////////////
// CONNECTION SCREEN

#define CON_TYPESID_START	10105
#define CON_TYPESID_END		10128

#define CON_SETTINGS		10130
#define CON_SETTINGS_LABEL	10131
#define CON_SETTINGSX		220 + D_W
#define	CON_SETTINGSY		190 + D_H
#define CON_SETTINGSWIDTH	200
#define CON_SETTINGSHEIGHT	100

#define CON_PASSWORD_LABEL	10132

#define CON_OK				10101
#define CON_OKX				CON_SETTINGSWIDTH-MULTIOP_OKW*2-13
#define CON_OKY				CON_SETTINGSHEIGHT-MULTIOP_OKH-3

#define CON_CANCEL			10102

#define CON_PHONE			10132
#define CON_PHONEX			20

#define CON_IP				10133
#define CON_IPX				20
#define CON_IPY				45

#define CON_IP_CANCEL		10134

//for clients
#define CON_PASSWORD		10139
#define CON_PASSWORDYES		10141
#define CON_PASSWORDNO		10142


// ////////////////////////////////////////////////////////////////
// GAME FIND SCREEN

#define GAMES_GAMESTART		10201
#define GAMES_GAMEEND		GAMES_GAMESTART+20
#define GAMES_GAMEWIDTH		540
#define GAMES_GAMEHEIGHT	28
// We can have a max of 4 icons for status, current icon size if 36x25.
#define GAMES_STATUS_START 393
#define GAMES_GAMENAME_START 2
#define GAMES_MAPNAME_START 173
#define GAMES_MODNAME_START 173 + 6		// indent a bit
#define GAMES_PLAYERS_START 360

// ////////////////////////////////////////////////////////////////
// GAME OPTIONS SCREEN

#define MULTIOP_PLAYERS			10231
#define MULTIOP_PLAYERSX		360
#define MULTIOP_PLAYERSY		1
#define MULTIOP_PLAYER_START		10232		//list of players
#define MULTIOP_PLAYER_END		10249
#define MULTIOP_PLAYERSW		263
#define MULTIOP_PLAYERSH		364

#define MULTIOP_ROW_WIDTH		246

//Team chooser
#define MULTIOP_TEAMS_START		102310			//List of teams
#define MULTIOP_TEAMS_END		102341
#define MULTIOP_TEAMSWIDTH		29
#define	MULTIOP_TEAMSHEIGHT		38

#define MULTIOP_TEAMCHOOSER_FORM	102800
#define MULTIOP_TEAMCHOOSER			102810
#define MULTIOP_TEAMCHOOSER_END         102841
#define MULTIOP_TEAMCHOOSER_KICK	10289

// 'Ready' button
#define MULTIOP_READY_FORM_ID		102900
#define MULTIOP_READY_START             (MULTIOP_READY_FORM_ID + MAX_PLAYERS + 1)
#define	MULTIOP_READY_END               (MULTIOP_READY_START + MAX_PLAYERS - 1)
#define MULTIOP_READY_WIDTH			41
#define MULTIOP_READY_HEIGHT		38

#define MULTIOP_PLAYERWIDTH		245
#define	MULTIOP_PLAYERHEIGHT	38

#define MULTIOP_OPTIONS			10250
#define MULTIOP_OPTIONSX		40
#define MULTIOP_OPTIONSY		1
#define MULTIOP_OPTIONSW		284
#define MULTIOP_OPTIONSH		364

#define MULTIOP_EDITBOXW		196
#define	MULTIOP_EDITBOXH		30

#define	MULTIOP_BLUEFORMW		226

#define	MROW1					4
#define	MROW2					MROW1+MULTIOP_EDITBOXH
#define	MROW3					MROW2+MULTIOP_EDITBOXH
#define	MROW4					MROW3+MULTIOP_EDITBOXH
#define MROW5					MROW4+38
#define	MROW6					MROW5+29
#define	MROW7					MROW6+29
#define	MROW8					MROW7+29
#define	MROW9					MROW8+29
#define	MROW10					MROW9+32
#define	MROW11					MROW10+36
#define	MROW12					MROW11+40

#define MCOL0					50
#define MCOL1					(MCOL0+26+10)	// rem 10 for 4 lines.
#define MCOL2					(MCOL1+38)
#define MCOL3					(MCOL2+38)
#define MCOL4					(MCOL3+38)

#define MULTIOP_PNAME_ICON		10252
#define MULTIOP_PNAME			10253
#define MULTIOP_GNAME_ICON		10254
#define MULTIOP_GNAME			10255
#define MULTIOP_MAP_ICON		10258
#define MULTIOP_MAP				10259
#define MULTIOP_MAP_MOD			21013	// Warning, do not use sequential numbers until code is fixed.

#define MULTIOP_CAMPAIGN		10261
#define MULTIOP_SKIRMISH		10263


#define MULTIOP_CLEAN			10267
#define MULTIOP_BASE			10268
#define MULTIOP_DEFENCE			10269

#define MULTIOP_ALLIANCE_N		10270
#define MULTIOP_ALLIANCE_Y		10271
#define MULTIOP_ALLIANCE_TEAMS	102710		//locked teams

#define MULTIOP_POWLEV_LOW		10272
#define MULTIOP_POWLEV_MED		10273
#define MULTIOP_POWLEV_HI		10274

#define MULTIOP_REFRESH			10275

#define MULTIOP_HOST			10276
#define MULTIOP_HOST_BUT		0xf0f0
#define MULTIOP_HOSTX			5

#define MULTIOP_FILTER_TOGGLE   30277

#define MULTIOP_STRUCTLIMITS	21277	// we are using 10277 already
#define MULTIOP_LIMITS_BUT		0xf0d0

#define MULTIOP_CANCELX			6
#define MULTIOP_CANCELY			6

#define MULTIOP_CHATBOX			10278
#define MULTIOP_CHATBOXX		MULTIOP_OPTIONSX
#define MULTIOP_CHATBOXY		364
#define MULTIOP_CHATBOXW		((MULTIOP_PLAYERSX+MULTIOP_PLAYERSW) - MULTIOP_OPTIONSX)
#define MULTIOP_CHATBOXH		115

#define MULTIOP_CONSOLEBOX		0x1A001		// TODO: these should be enums!
#define MULTIOP_CONSOLEBOXX		MULTIOP_OPTIONSX
#define MULTIOP_CONSOLEBOXY		422
#define MULTIOP_CONSOLEBOXW		((MULTIOP_PLAYERSX + MULTIOP_PLAYERSW) - MULTIOP_OPTIONSX)
#define MULTIOP_CONSOLEBOXH		54

#define MULTIOP_CHATEDIT		10279
#define MULTIOP_CHATEDITX		4
#define	MULTIOP_CHATEDITY		MULTIOP_CHATBOXH-14
#define	MULTIOP_CHATEDITW		MULTIOP_CHATBOXW-8
#define MULTIOP_CHATEDITH		9

#define MULTIOP_COLCHOOSER_FORM         10280
#define MULTIOP_COLCHOOSER              102711 //10281
#define MULTIOP_COLCHOOSER_END          102742 //10288

#define MULTIOP_LIMIT			10292	// 2 for this (+label)
#define MULTIOP_GAMETYPE		10294
#define MULTIOP_POWER			10296
#define MULTIOP_ALLIANCES		10298
#define MULTIOP_BASETYPE		10300

#define MULTIOP_SKSLIDE			102842 //10313
#define MULTIOP_SKSLIDE_END		102873 //10320

#define MULTIOP_MAP_PREVIEW 920000
#define MULTIOP_MAP_BUT		920002

#define MULTIOP_PASSWORD	920010
#define MULTIOP_PASSWORD_BUT 920012
#define MULTIOP_PASSWORD_EDIT 920013

#define MULTIOP_NO_SOMETHING            10331
#define MULTIOP_NO_SOMETHINGX           3
#define MULTIOP_NO_SOMETHINGY           MROW5

#define MULTIOP_COLOUR_START		10332
#define MULTIOP_COLOUR_END		(MULTIOP_COLOUR_START + MAX_PLAYERS)
#define MULTIOP_COLOUR_WIDTH		31

#define MULTIOP_AI_FORM			(MULTIOP_COLOUR_END + 1)
#define MULTIOP_AI_START		(MULTIOP_AI_FORM + 1)
#define MULTIOP_AI_END			(MULTIOP_AI_START * MAX_PLAYERS)
#define MULTIOP_AI_OPEN			(MULTIOP_AI_END + 1)
#define MULTIOP_AI_CLOSED		(MULTIOP_AI_END + 2)

#define MULTIOP_DIFFICULTY_INIT_START	(MULTIOP_AI_END + 3)
#define	MULTIOP_DIFFICULTY_INIT_END	(MULTIOP_DIFFICULTY_INIT_START + MAX_PLAYERS)

#define MULTIOP_DIFFICULTY_CHOOSE_START	(MULTIOP_DIFFICULTY_INIT_END + 1)
#define MULTIOP_DIFFICULTY_CHOOSE_END	(MULTIOP_DIFFICULTY_CHOOSE_START + MAX_PLAYERS)


// ///////////////////////////////
// Many Button Variations..

#define	CON_NAMEBOXWIDTH		CON_SETTINGSWIDTH-CON_PHONEX
#define	CON_NAMEBOXHEIGHT		15

#define MULTIOP_OKW			37
#define MULTIOP_OKH			24

#define MULTIOP_BUTW			35
#define MULTIOP_BUTH			24

#endif // __INCLUDED_SRC_MULTIINT_H__
