/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2019  Warzone 2100 Project

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

bool recvDroid(NETQUEUE queue);
bool recvDroidInfo(NETQUEUE queue);
bool recvDestroyDroid(NETQUEUE queue);
bool recvDroidMove(NETQUEUE queue);
bool recvDestroyStructure(NETQUEUE queue);
bool recvBuildFinished(NETQUEUE queue);
bool recvTemplate(NETQUEUE queue);
bool recvDestroyFeature(NETQUEUE queue);
bool recvPing(NETQUEUE queue);
bool recvRequestDroid(NETQUEUE queue);
bool recvTextMessage(NETQUEUE queue);
bool recvDroidDisEmbark(NETQUEUE queue);
bool recvColourRequest(NETQUEUE queue);
bool recvPositionRequest(NETQUEUE queue);
void recvOptions(NETQUEUE queue);
void sendOptions();

bool recvResearchStatus(NETQUEUE queue);
bool recvLasSat(NETQUEUE queue);
void recvStructureInfo(NETQUEUE queue);
bool recvMapFileData(NETQUEUE queue);
bool recvMapFileRequested(NETQUEUE queue);

bool recvTextMessageAI(NETQUEUE queue);         //AI multiplayer message
bool recvTeamRequest(NETQUEUE queue);
bool recvReadyRequest(NETQUEUE queue);

#endif // __INCLUDED_SRC_MULTIRECV_H__
