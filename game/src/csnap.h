
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

void SnapCursorTo(UWORD x,UWORD y);

extern void intSetCurrentCursorPosition(CURSORSNAP *Snap,UDWORD id);

#endif

UDWORD SnapGetFormID(CURSORSNAP *SnapBuffer);
UDWORD SnapGetID(CURSORSNAP *SnapBuffer);
