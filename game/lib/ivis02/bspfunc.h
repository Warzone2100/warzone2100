#ifndef _bspfunc_h_
#define _bspfunc_h_

#include "ivisdef.h"
#include "bspImd.h"

/***************************************************************************/

extern void GetRealCameraPos(OBJPOS *Camera,SDWORD Distance, iVector *CameraLoc);


#ifdef PSX
extern void DrawBSPIMD( iIMDShape *IMDdef,iVector * pPos, SCRVERTEX *ScrVertices);
#else
extern void DrawBSPIMD( iIMDShape *IMDdef,iVector * pPos, iIMDPoly *ScrVertices);
#endif


extern PSBSPTREENODE InitNode(PSBSPTREENODE psBSPNode);

extern void GetPlane( iIMDShape *s, UDWORD PolygonID, PSPLANE psPlane );


#endif

