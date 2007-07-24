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

	$Revision$
	$Id$
	$HeadURL$
*/


#ifdef __cplusplus

struct ZoneMapHeader {
	UWORD version;
	UWORD numStrips;
	UWORD numZones;
	UWORD pad;
};

void giSetMapData(CHeightMap *MapData);
BOOL giCreateZones(void);
void giDeleteZones(void);
void giUpdateMapZoneIDs(void);
void giClearGatewayFlags(void);
BOOL giWriteZones(FILE *Stream);

#else

int giGetMapWidth(void);
int giGetMapHeight(void);
void giSetGatewayFlag(int x,int y,BOOL IsGateway);
BOOL giIsClifface(int x,int y);
BOOL giIsWater(int x,int y);
BOOL giIsGateway(int x,int y);
int giGetNumGateways(void);
BOOL giGetGateway(int Index,int *x0,int *y0,int *x1,int *y1);

#endif
