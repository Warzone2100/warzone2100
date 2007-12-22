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
 * multigifts.h
 *
 * multiplayer game, gifts and deathmatch relevant funcs.
 */

extern void requestAlliance		(uint8_t from, uint8_t to, BOOL prop, BOOL allowAudio);
extern void breakAlliance		(uint8_t p1, uint8_t p2, BOOL prop, BOOL allowAudio);
extern void formAlliance		(uint8_t p1, uint8_t p2, BOOL prop, BOOL allowAudio, BOOL allowNotification);
extern void sendAlliance		(uint8_t from, uint8_t to, uint8_t state, int32_t value);
extern BOOL recvAlliance		(BOOL allowAudio);
extern void	createTeamAlliances	(void);

extern BOOL sendGift			(uint8_t type, uint8_t to);
extern BOOL recvGift			();

extern void technologyGiveAway				(STRUCTURE *pS);
extern void recvMultiPlayerRandomArtifacts	();
extern void addMultiPlayerRandomArtifacts	(uint8_t quantity, FEATURE_TYPE type);
extern void processMultiPlayerArtifacts		(void);

extern void	giftArtifact					(UDWORD owner,UDWORD x,UDWORD y);
extern BOOL	addOilDrum						(uint8_t count);
extern void	giftPower						(uint8_t from, uint8_t to, BOOL send);
extern void giftRadar						(uint8_t from, uint8_t to, BOOL send);

#define RADAR_GIFT		1
#define DROID_GIFT		2
#define RESEARCH_GIFT	3
#define POWER_GIFT		4
#define STRUCTURE_GIFT	5	// Unused
