/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2012  Warzone 2100 Project

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

extern bool recvDroid               (NETQUEUE queue);
extern bool recvDroidInfo           (NETQUEUE queue);
extern bool recvDestroyDroid        (NETQUEUE queue);
extern bool recvDroidMove           (NETQUEUE queue);
extern bool recvDestroyStructure    (NETQUEUE queue);
extern bool recvBuildFinished       (NETQUEUE queue);
extern bool recvTemplate            (NETQUEUE queue);
extern bool recvDestroyFeature      (NETQUEUE queue);
extern bool recvPing                (NETQUEUE queue);
extern bool recvRequestDroid        (NETQUEUE queue);
extern bool recvTextMessage         (NETQUEUE queue);
extern bool recvDroidDisEmbark      (NETQUEUE queue);
extern bool recvColourRequest       (NETQUEUE queue);
extern bool recvPositionRequest     (NETQUEUE queue);
extern void recvOptions             (NETQUEUE queue);
extern void sendOptions             (void);

extern bool recvResearchStatus      (NETQUEUE queue);
extern bool recvLasSat              (NETQUEUE queue);
extern void recvStructureInfo       (NETQUEUE queue);
extern bool recvMapFileData         (NETQUEUE queue);
extern bool recvMapFileRequested    (NETQUEUE queue);


extern bool recvTextMessageAI       (NETQUEUE queue);  //AI multiplayer message
extern bool recvTeamRequest         (NETQUEUE queue);
extern bool recvReadyRequest        (NETQUEUE queue);

#endif // __INCLUDED_SRC_MULTIRECV_H__
