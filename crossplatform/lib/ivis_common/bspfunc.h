#ifndef _bspfunc_h_
#define _bspfunc_h_

#include "ivisdef.h"
#include "bspimd.h"

/***************************************************************************/

extern void GetRealCameraPos(OBJPOS *Camera,SDWORD Distance, iVector *CameraLoc);

extern void DrawBSPIMD( iIMDShape *IMDdef,iVector * pPos); //, iIMDPoly *ScrVertices);

extern PSBSPTREENODE InitNode(PSBSPTREENODE psBSPNode);

extern void GetPlane( iIMDShape *s, UDWORD PolygonID, PSPLANE psPlane );


#endif

