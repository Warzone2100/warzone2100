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

#ifndef _INCLUDED_CSNAP_
#define _INCLUDED_CSNAP_

//#define SNAP_ERASABLE


typedef enum {
	SNAP_UP,
	SNAP_RIGHT,
	SNAP_DOWN,
	SNAP_LEFT,
	SNAP_NEAREST,
} SNAPDIRECTION;

typedef struct {
	SWORD	NDxBias,NDyBias;
	SWORD	UDxBias,UDyBias;
	SWORD	RDxBias,RDyBias;
	SWORD	DDxBias,DDyBias;
	SWORD	LDxBias,LDyBias;
} SNAPBIAS;

typedef struct {
	UDWORD FormID;
	UDWORD ID;
	SWORD SnapX;
	SWORD SnapY;
	SNAPBIAS *Bias;
} SNAPCOORD;

typedef struct {
	UWORD MaxSnaps;
	SWORD NumSnaps;
	SWORD CurrentSnap;
	UDWORD NewCurrentFormID;
	UDWORD NewCurrentID;
	SNAPCOORD *SnapCoords;
} CURSORSNAP;

extern SNAPBIAS DefaultBias;
extern SNAPBIAS FrontendBias;
extern SNAPBIAS ReticuleBias;
extern SNAPBIAS TabBias;

void SetMaxDist(SDWORD MaxX,SDWORD MaxY);

void snapInitVars(void);

void AllocateSnapBuffer(CURSORSNAP *SnapBuffer,UWORD MaxSnaps);
void ReleaseSnapBuffer(CURSORSNAP *SnapBuffer);

void StartCursorSnap(CURSORSNAP *SnapBuffer);
void AddCursorSnap(CURSORSNAP *SnapBuffer,SWORD PosX,SWORD PosY,UDWORD FormID,UDWORD ID,SNAPBIAS *Bias);
void RemoveCursorSnap(CURSORSNAP *SnapBuffer,UDWORD FormID);
void EnableAllCursorSnaps(void);
void DisableCursorSnapsExcept(UDWORD FormID);

void InitCursorSnap(CURSORSNAP *SnapBuffer,UWORD NumSnaps);
void SetCursorSnap(CURSORSNAP *SnapBuffer,UWORD Index,SWORD PosX,SWORD PosY,UDWORD FormID,UDWORD ID);

void GotoSnap(CURSORSNAP *SnapBuffer);
void GotoNextSnap(CURSORSNAP *SnapBuffer);
void GotoPreviousSnap(CURSORSNAP *SnapBuffer);
void GotoDirectionalSnap(CURSORSNAP *SnapBuffer,SNAPDIRECTION Direction,SWORD CurrX,SWORD CurrY);
void SetCurrentSnap(CURSORSNAP *SnapBuffer,UDWORD FormID);
void SetCurrentSnapID(CURSORSNAP *SnapBuffer,UDWORD ID);
void SnapToID(CURSORSNAP *SnapBuffer,UWORD snp);

extern void intSetCurrentCursorPosition(CURSORSNAP *Snap,UDWORD id);

#endif

UDWORD SnapGetFormID(CURSORSNAP *SnapBuffer);
UDWORD SnapGetID(CURSORSNAP *SnapBuffer);
