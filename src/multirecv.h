/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2009  Warzone Resurrection Project

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
 *  header for multiplay files that receive netmessages
 *  this is a little lower level than multiplay.h
 *  so we don't include it in other warzone stuff
 *  to avoid a load of warnings
 */

#ifndef __INCLUDED_SRC_MULTIRECV_H__
#define __INCLUDED_SRC_MULTIRECV_H__

#ifdef __cplusplus
extern "C"
{
#endif //__cplusplus

extern BOOL recvDroid				(void);
extern BOOL recvDroidInfo			(void);
extern BOOL recvDestroyDroid		(void);
extern BOOL recvGroupOrder			(void);
extern BOOL recvDroidMove			(void);
extern BOOL recvDestroyStructure	(void);
extern BOOL recvBuildStarted		(void);
extern BOOL recvBuildFinished		(void);
extern BOOL recvTemplate			(void);
extern BOOL recvDestroyFeature		(void);
extern BOOL recvDemolishFinished	(void);
extern BOOL recvPing				(void);
extern BOOL	recvRequestDroid		(void);
extern BOOL recvTextMessage			(void);
extern BOOL recvDroidSecondary		(void);
extern BOOL recvDroidSecondaryAll	(void);
extern BOOL recvDroidEmbark         (void);
extern BOOL recvDroidDisEmbark      (void);
extern BOOL recvDroidCheck			(void);
extern BOOL recvStructureCheck		(void);
extern BOOL recvPowerCheck			(void);
extern BOOL recvAlliance			(BOOL allowAudio);
extern BOOL recvColourRequest		(void);
extern BOOL recvPositionRequest		(void);
extern void recvOptions				(void);
extern void sendOptions				(void);

extern BOOL recvHappyVtol			(void);
extern BOOL recvResearchStatus		(void);
extern BOOL recvLasSat				(void);
extern BOOL	recvMapFileData				(void);
extern BOOL	recvMapFileRequested		(void);


extern BOOL recvTextMessageAI		(void);		//AI multiplayer message
extern BOOL	recvTeamRequest		(void);
extern BOOL	recvReadyRequest		(void);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif // __INCLUDED_SRC_MULTIRECV_H__
