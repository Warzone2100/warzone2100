/*
 * Cursor handling for keyboard/dpad control
 * Pumpkin Studios 98
 */

#include <stdio.h>
#include <math.h>

#include "frame.h"
#include "widget.h"
#include "deliverance.h"
#include "fractions.h"

#include "piestate.h"
#include "pieclip.h"

#include "csnap.h"
#include "audio_id.h"

#define V_BIAS 8
#define H_BIAS 8


SNAPBIAS DefaultBias =
{
	1,1,	// Nearest
	8,1,	// Up
	1,8,	// Right
	8,1,	// Down
	1,8,	// Left
};


SNAPBIAS FrontendBias = 
{
	1,1,	// Nearest
	1,1,	// Up
	1,1,	// Right
	1,1,	// Down
	1,1,	// Left
};


SNAPBIAS ReticuleBias = 
{
	1,1,	// Nearest
	1,1,	// Up
	1,1,	// Right
	1,1,	// Down
	1,1,	// Left
};


SNAPBIAS TabBias = 
{
	1,1,	// Nearest
	1,8,	// Up
	1,1,	// Right
	1,8,	// Down
	1,1,	// Left
};


static SDWORD MaxXDist = 64;
static SDWORD MaxYDist = 64;

static UDWORD EnabledFormID = 0;
extern W_SCREEN *psWScreen;					//The widget screen


void SetMaxDist(SDWORD MaxX,SDWORD MaxY)
{
	MaxXDist = MaxX;
	MaxYDist = MaxY;
}


void snapInitVars(void)
{
	EnabledFormID = 0;
}


void AllocateSnapBuffer(CURSORSNAP *SnapBuffer,UWORD MaxSnaps)
{
	SnapBuffer->SnapCoords = MALLOC(sizeof(CURSORSNAP)*MaxSnaps);
	SnapBuffer->MaxSnaps = MaxSnaps;
	SnapBuffer->NumSnaps = 0;
	SnapBuffer->CurrentSnap = 0;
	SnapBuffer->NewCurrentFormID = 0;
	SnapBuffer->NewCurrentID = 0;
}


void ReleaseSnapBuffer(CURSORSNAP *SnapBuffer)
{
	FREE(SnapBuffer->SnapCoords);
}


void StartCursorSnap(CURSORSNAP *SnapBuffer)
{
	SnapBuffer->NumSnaps = 0;
//	SnapBuffer->CurrentSnap = 0;
}


void InitCursorSnap(CURSORSNAP *SnapBuffer,UWORD NumSnaps)
{
	SnapBuffer->NumSnaps = NumSnaps;
	SnapBuffer->CurrentSnap = 0;
}


void AddCursorSnap(CURSORSNAP *SnapBuffer,SWORD PosX,SWORD PosY,UDWORD FormID,UDWORD ID,SNAPBIAS *Bias)
{
//	UWORD i;
	int Index = -1;

#ifdef SNAP_ERASABLE
	for(i=0; i<SnapBuffer->NumSnaps; i++) {
		if(SnapBuffer->SnapCoords[i].FormID == 0) {
			Index = i;
		}
	}
#endif

	if(Index < 0) {
		ASSERT((SnapBuffer->NumSnaps < SnapBuffer->MaxSnaps,"AddCursorSnap: MAXCURSORSNAPS Exceeded"));
		Index = SnapBuffer->NumSnaps;
		SnapBuffer->NumSnaps++;
	}

	SnapBuffer->SnapCoords[Index].SnapX = PosX;
	SnapBuffer->SnapCoords[Index].SnapY = PosY;
	SnapBuffer->SnapCoords[Index].FormID = FormID;
	SnapBuffer->SnapCoords[Index].ID = ID;
	if(Bias == NULL) {
		SnapBuffer->SnapCoords[Index].Bias = &DefaultBias;
	} else {
		SnapBuffer->SnapCoords[Index].Bias = Bias;
	}

// If a new current snap was requested then see if this one meets its requirements.
	if(SnapBuffer->NewCurrentFormID) {
		if(FormID == SnapBuffer->NewCurrentFormID) {
			SnapBuffer->NewCurrentFormID = 0;
			SnapBuffer->CurrentSnap = Index;
		}
	} else if(SnapBuffer->NewCurrentID) {
		if(ID == SnapBuffer->NewCurrentID) {
//			DBPRINTF(("Found New ID %d\n",SnapBuffer->NewCurrentID);
			SnapBuffer->NewCurrentID = 0;
			SnapBuffer->CurrentSnap = Index;
			GotoSnap(SnapBuffer);
		} else {
//			DBPRINTF(("Not Found New ID %d\n",SnapBuffer->NewCurrentID);
		}
	}
//	DBPRINTF(("%d : %d,%d\n",Index,SnapBuffer->SnapCoords[Index].SnapX,SnapBuffer->SnapCoords[Index].SnapY);
}


void RemoveCursorSnap(CURSORSNAP *SnapBuffer,UDWORD FormID)
{
#ifdef SNAP_ERASABLE
	int i;

	for(i=0; i<SnapBuffer->NumSnaps; i++) {
		if(SnapBuffer->SnapCoords[i].FormID == FormID) {
			SnapBuffer->SnapCoords[i].FormID = 0;
		}
	}
#endif
}


// Given a widget id get it's screen extents.
// NOTE that this function is slow and should be called infrequently,
// ideally only during widget initialisation.
//
BOOL widgGetScreenExtents(UDWORD ID,int *sx,int *sy,int *sw,int *sh)
{
	struct _widget *psWidget = widgGetFromID(psWScreen,ID);

	if(psWidget != NULL) {
		int x,y,w,h;

		x = psWidget->x;
		y = psWidget->y;
		w = psWidget->width;
		h = psWidget->height;

		while(psWidget->formID) {
			struct _widget *psParent = widgGetFromID(psWScreen,psWidget->formID);
			if(psParent) {
				x += psParent->x;
				y += psParent->y;
			}

			psWidget = psParent;
		}

		*sx = x; *sy = y;
		*sw = w; *sh = h;

		return TRUE;
	}

	return FALSE;
}


// Given a widget form id, make the snap that matches it the current one next frame.
//
void SetCurrentSnapFormID(CURSORSNAP *SnapBuffer,UDWORD FormID)
{
	{
		int x,y,w,h;
		SnapBuffer->NewCurrentFormID = FormID;

		if(FormID) {
			// Get the screen extents of the specified form and move the mouse there.
			if(widgGetScreenExtents(FormID,&x,&y,&w,&h) == TRUE) {
		//		DBPRINTF(("%d %d,%d %d\n",x,y,w,h);
				SetMousePos(0,x+w/2,y+h/2);
			}
		}
	}
}


// Given a widget id, make the snap that matches it the current one next frame.
//
void SetCurrentSnapID(CURSORSNAP *SnapBuffer,UDWORD ID)
{
	{
		int x,y,w,h;

		SnapBuffer->NewCurrentID = ID;
		if(ID) {
			// Get the screen extents of the specified widget and move the mouse there.
			if(widgGetScreenExtents(ID,&x,&y,&w,&h) == TRUE) {
		//		DBPRINTF(("%d %d,%d %d\n",x,y,w,h);
				SetMousePos(0,x+w/2,y+h/2);
			}
		}
	}
}


void EnableAllCursorSnaps(void)
{
	EnabledFormID = 0;
}


void DisableCursorSnapsExcept(UDWORD FormID)
{
	EnabledFormID = FormID;
}


void SetCursorSnap(CURSORSNAP *SnapBuffer,UWORD Index,SWORD PosX,SWORD PosY,UDWORD FormID,UDWORD ID)
{
	ASSERT((Index < SnapBuffer->NumSnaps,"SetCursorSnap: Index out of range"));

	SnapBuffer->SnapCoords[Index].SnapX = PosX;
	SnapBuffer->SnapCoords[Index].SnapY = PosY;
	SnapBuffer->SnapCoords[Index].FormID = FormID;
	SnapBuffer->SnapCoords[Index].ID = ID;
}


void GotoSnap(CURSORSNAP *SnapBuffer)
{
	if(SnapBuffer->NumSnaps > 0) {
		SetMousePos(0,SnapBuffer->SnapCoords[SnapBuffer->CurrentSnap].SnapX,
					SnapBuffer->SnapCoords[SnapBuffer->CurrentSnap].SnapY);
	}
}


void GotoNextSnap(CURSORSNAP *SnapBuffer)
{
	if(SnapBuffer->NumSnaps > 0) {
		SnapBuffer->CurrentSnap++;
		if(SnapBuffer->CurrentSnap >= SnapBuffer->NumSnaps) {
			SnapBuffer->CurrentSnap = 0;
		}
	}

	SetMousePos(0,SnapBuffer->SnapCoords[SnapBuffer->CurrentSnap].SnapX,
					SnapBuffer->SnapCoords[SnapBuffer->CurrentSnap].SnapY);
}


void GotoPreviousSnap(CURSORSNAP *SnapBuffer)
{
	if(SnapBuffer->NumSnaps > 0) {
		SnapBuffer->CurrentSnap--;
		if(SnapBuffer->CurrentSnap <0) {
			SnapBuffer->CurrentSnap = SnapBuffer->NumSnaps-1;
		}
	}

	SetMousePos(0,SnapBuffer->SnapCoords[SnapBuffer->CurrentSnap].SnapX,
					SnapBuffer->SnapCoords[SnapBuffer->CurrentSnap].SnapY);
}


void GotoDirectionalSnap(CURSORSNAP *SnapBuffer,SNAPDIRECTION Direction,SWORD CurrX,SWORD CurrY)
{
	UWORD i;
	UWORD CurrentSnap = SnapBuffer->CurrentSnap;
	SWORD ThisX = SnapBuffer->SnapCoords[CurrentSnap].SnapX;
	SWORD ThisY = SnapBuffer->SnapCoords[CurrentSnap].SnapY;
	SWORD NearestSnap = -1;
	SWORD NearestDist = 32767;
	SWORD dx,dy;
	SWORD Dist;


	if(SnapBuffer->CurrentSnap < 0) {
		SnapBuffer->CurrentSnap = SnapBuffer->NumSnaps-1;
	}
	if(SnapBuffer->CurrentSnap >= SnapBuffer->NumSnaps) {
		SnapBuffer->CurrentSnap = 0;
	}

//	DBPRINTF(("NumSnaps %d %d %d\n",SnapBuffer->NumSnaps,Direction,EnabledFormID);

	for(i=0; i<SnapBuffer->NumSnaps; i++) {
		SNAPBIAS *Bias = SnapBuffer->SnapCoords[CurrentSnap].Bias;	//i].Bias;

#ifdef SNAP_ERASABLE
		if( ((i != CurrentSnap) || (Direction == SNAP_NEAREST)) && (SnapBuffer->SnapCoords[i].FormID) ) {
#else
		if( ((i != CurrentSnap) || (Direction == SNAP_NEAREST)) &&
			((SnapBuffer->SnapCoords[i].FormID == EnabledFormID) || (EnabledFormID == 0)) ) {
#endif
			dx = SnapBuffer->SnapCoords[i].SnapX - ThisX;
			dy = SnapBuffer->SnapCoords[i].SnapY - ThisY;

			if(Direction == SNAP_NEAREST) {

				dx = SnapBuffer->SnapCoords[i].SnapX - CurrX;
				dy = SnapBuffer->SnapCoords[i].SnapY - CurrY;

				Dist = abs(dx*Bias->NDxBias) + abs(dy*Bias->NDyBias);
				if(Dist < NearestDist) {
					NearestSnap = i;
					NearestDist = Dist;
				}

			} else if(Direction == SNAP_UP) {

				if(dy >= 0) {
//					DBPRINTF(("Skip SNAP_UP\n");
					continue;
				}

				dx = abs(dx);
				dy = abs(dy);

				Dist = dx*Bias->UDxBias + dy*Bias->UDyBias;
//DBPRINTF(("SNAP_UP %d %d : %d %d : %d\n",abs(dx),abs(dy),abs(dx*Bias->UDxBias),abs(dy*Bias->UDyBias),Dist);
				if((Dist < NearestDist) && (dx < MaxXDist)) {
					NearestSnap = i;
					NearestDist = Dist;
				}

			} else if(Direction == SNAP_RIGHT) {

				if(dx <= 0) {
//					DBPRINTF(("Skip SNAP_RIGHT\n");
					continue;
				}

				dx = abs(dx);
				dy = abs(dy);

				Dist = dx*Bias->RDxBias + dy*Bias->RDyBias;
//DBPRINTF(("SNAP_RIGHT %d %d : %d %d : %d\n",abs(dx),abs(dy),abs(dx*Bias->RDxBias),abs(dy*Bias->RDyBias),Dist);
				if((Dist < NearestDist) && (dy < MaxYDist)) {
					NearestSnap = i;
					NearestDist = Dist;
				}

			} else if(Direction == SNAP_DOWN) {

				if(dy <= 0) {
//					DBPRINTF(("Skip SNAP_DOWN\n");
					continue;
				}

				dx = abs(dx);
				dy = abs(dy);

				Dist = dx*Bias->DDxBias + dy*Bias->DDyBias;
//DBPRINTF(("SNAP_DOWN %d %d : %d %d : %d\n",abs(dx),abs(dy),abs(dx*Bias->DDxBias),abs(dy*Bias->DDyBias),Dist));
				if((Dist < NearestDist) && (dx < MaxXDist)) {
					NearestSnap = i;
					NearestDist = Dist;
				}

			} else if(Direction == SNAP_LEFT) {

				if(dx >= 0) {
//					DBPRINTF(("Skip SNAP_LEFT\n");
					continue;
				}


				dx = abs(dx);
				dy = abs(dy);
				Dist = dx*Bias->LDxBias + dy*Bias->LDyBias;
//DBPRINTF(("SNAP_LEFT %d %d : %d %d : %d\n",abs(dx),abs(dy),abs(dx*Bias->LDxBias),abs(dy*Bias->LDyBias),Dist);
				if((Dist < NearestDist) && (dy < MaxYDist)) {
					NearestSnap = i;
					NearestDist = Dist;
				}

			}
		}
	}

	if(NearestSnap >= 0) {
		SnapBuffer->CurrentSnap = NearestSnap;
		SetMousePos(0,SnapBuffer->SnapCoords[NearestSnap].SnapX,
					SnapBuffer->SnapCoords[NearestSnap].SnapY);
	}
}


void SnapToID(CURSORSNAP *SnapBuffer,UWORD snp)
{
	SnapBuffer->CurrentSnap = snp;
}


void SnapCursorTo(UWORD x,UWORD y)
{
	SetMousePos(0,x,y);
}


UDWORD SnapGetFormID(CURSORSNAP *SnapBuffer)
{
   return SnapBuffer->SnapCoords[SnapBuffer->CurrentSnap].FormID;
}


UDWORD SnapGetID(CURSORSNAP *SnapBuffer)
{
   return SnapBuffer->SnapCoords[SnapBuffer->CurrentSnap].ID;
}
