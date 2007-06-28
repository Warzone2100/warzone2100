#ifndef _bspfunc_h_
#define _bspfunc_h_

#include "ivisdef.h"
#include "bspimd.h"

/***************************************************************************/

void GetRealCameraPos(OBJPOS *Camera,SDWORD Distance, iVector *CameraLoc);
void DrawBSPIMD(iIMDShape *IMDdef, iVector *pPos);
PSBSPTREENODE InitNode(PSBSPTREENODE psBSPNode);
void GetPlane( iIMDShape *s, UDWORD PolygonID, PSPLANE psPlane );

#endif

