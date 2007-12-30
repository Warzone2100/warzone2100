/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2007  Warzone Resurrection Project

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
/*
 * 	MultiRecv.h
 *
 * header for multiplay files that recv netmsgs.
 * this is a little lower level than multiplay.h
 * so we don't include it in other warzone stuff
 * to avoid a load of warnings
 */

extern BOOL recvDroid				(void);
extern BOOL recvDroidInfo			(NETMSG *pMsg);
extern BOOL recvDestroyDroid		(void);
extern BOOL recvGroupOrder			(NETMSG *pMsg);
extern BOOL recvDroidMove			(NETMSG *pMsg);
extern BOOL receiveWholeDroid		(NETMSG *pMsg);
extern BOOL recvDestroyStructure	();
//extern BOOL RecvBuild				(NETMSG *pMsg);
extern BOOL recvBuildStarted		();
extern BOOL recvBuildFinished		();
//extern BOOL receiveWholeStructure	(NETMSG *pMsg);
extern BOOL recvTemplate			(NETMSG *pMsg);
extern BOOL recvDestroyFeature		(NETMSG *pMsg);
extern BOOL recvDemolishFinished	();
extern BOOL recvPing				();
extern BOOL	recvRequestDroid		(NETMSG *pMsg);
extern BOOL recvTextMessage			(NETMSG *pMsg);
extern BOOL recvDroidSecondary		(NETMSG *pMsg);
extern BOOL recvDroidSecondaryAll	(NETMSG *pMsg);
extern BOOL recvDroidEmbark         (NETMSG *pMsg);
extern BOOL recvDroidDisEmbark      (NETMSG *pMsg);
//extern BOOL recvCommandDroid		(NETMSG *pMsg);
extern BOOL recvDroidCheck			();
extern BOOL recvStructureCheck		();
extern BOOL recvPowerCheck			();
extern BOOL recvAlliance			(BOOL allowAudio);
//extern BOOL multiPlayerRequest		(NETMSG *pMsg);
extern BOOL recvColourRequest		(NETMSG *pMsg);
extern BOOL sendWholeDroid			(DROID	*pD, UDWORD dest);
//extern BOOL sendWholeStructure		(STRUCTURE *pS, DPID dest);
extern void recvOptions				(NETMSG *pMsg);
extern void sendOptions				(UDWORD dest, UDWORD play);
extern BOOL recvScoreSubmission		();
extern BOOL recvDestroyExtra		(NETMSG *pMsg);
extern BOOL	recvAudioMsg			(NETMSG *pMsg);
extern BOOL recvHappyVtol			(NETMSG *pMsg);
extern BOOL recvResearchStatus		(NETMSG *pMsg);
extern BOOL recvLasSat				();
extern BOOL	recvMapFileData				(NETMSG *pMsg);
extern BOOL	recvMapFileRequested		(NETMSG *pMsg);


extern BOOL recvTextMessageAI		(NETMSG *pMsg);		//AI multiplayer message
extern BOOL	recvTeamRequest			(NETMSG *pMsg);
