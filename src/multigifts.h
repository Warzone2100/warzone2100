/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2015  Warzone 2100 Project

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
 *  multiplayer game, gifts and deathmatch relevant funcs.
 */

#ifndef __INCLUDED_SRC_MULTIGIFTS_H__
#define __INCLUDED_SRC_MULTIGIFTS_H__

extern void requestAlliance(uint8_t from, uint8_t to, bool prop, bool allowAudio);
extern void breakAlliance(uint8_t p1, uint8_t p2, bool prop, bool allowAudio);
extern void formAlliance(uint8_t p1, uint8_t p2, bool prop, bool allowAudio, bool allowNotification);
extern void sendAlliance(uint8_t from, uint8_t to, uint8_t state, int32_t value);
extern bool recvAlliance(NETQUEUE queue, bool allowAudio);                  // Was declared in multirecv.h, too.
extern void	createTeamAlliances(void);

extern bool sendGift(uint8_t type, uint8_t to);
extern bool recvGift(NETQUEUE queue);

extern void technologyGiveAway(const STRUCTURE *pS);
extern void recvMultiPlayerFeature(NETQUEUE queue);
extern void sendMultiPlayerFeature(uint32_t ref, uint32_t x, uint32_t y, uint32_t id);

bool pickupArtefact(int toPlayer, int fromPlayer);
void giftPower(uint8_t from, uint8_t to, uint32_t amount, bool send);
extern void giftRadar(uint8_t from, uint8_t to, bool send);

#define RADAR_GIFT		1
#define DROID_GIFT		2
#define RESEARCH_GIFT	3
#define POWER_GIFT		4
#define STRUCTURE_GIFT	5
#define AUTOGAME_GIFT   6   // Notify others that we are now being controled by the AI

#endif // __INCLUDED_SRC_MULTIGIFTS_H__
