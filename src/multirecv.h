/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2011  Warzone 2100 Project

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

extern BOOL recvDroid               (NETQUEUE queue);
extern BOOL recvDroidInfo           (NETQUEUE queue);
extern BOOL recvDestroyDroid        (NETQUEUE queue);
extern BOOL recvDroidMove           (NETQUEUE queue);
extern BOOL recvDestroyStructure    (NETQUEUE queue);
extern BOOL recvBuildFinished       (NETQUEUE queue);
extern BOOL recvTemplate            (NETQUEUE queue);
extern BOOL recvDestroyFeature      (NETQUEUE queue);
extern BOOL recvDemolishFinished    (NETQUEUE queue);
extern BOOL recvPing                (NETQUEUE queue);
extern BOOL recvRequestDroid        (NETQUEUE queue);
extern BOOL recvTextMessage         (NETQUEUE queue);
extern BOOL recvDroidEmbark         (NETQUEUE queue);
extern BOOL recvDroidDisEmbark      (NETQUEUE queue);
extern BOOL recvDroidCheck          (NETQUEUE queue);
extern BOOL recvStructureCheck      (NETQUEUE queue);
extern BOOL recvPowerCheck          (NETQUEUE queue);
extern BOOL recvColourRequest       (NETQUEUE queue);
extern BOOL recvPositionRequest     (NETQUEUE queue);
extern void recvOptions             (NETQUEUE queue);
extern void sendOptions             (void);

extern BOOL recvResearchStatus      (NETQUEUE queue);
extern BOOL recvLasSat              (NETQUEUE queue);
extern void recvStructureInfo       (NETQUEUE queue);
extern BOOL recvMapFileData         (NETQUEUE queue);
extern BOOL recvMapFileRequested    (NETQUEUE queue);


extern BOOL recvTextMessageAI       (NETQUEUE queue);  //AI multiplayer message
extern BOOL recvTeamRequest         (NETQUEUE queue);
extern BOOL recvReadyRequest        (NETQUEUE queue);

#endif // __INCLUDED_SRC_MULTIRECV_H__
