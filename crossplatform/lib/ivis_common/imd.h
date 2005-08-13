#ifndef _imd_
#define _imd_


#include "ivisdef.h"

#define IMD_NAME				"IMD"
#define PIE_NAME				"PIE"  // Pumpkin image export data file
#define IMD_VER				1
#define PIE_VER				2



#ifdef BSPIMD

#define BSPPOLYID_MAXPOLYID (65534)
#define BSPPOLYID_TERMINATE (BSPPOLYID_MAXPOLYID+1)	// This is used as a terminator

#include "bspimd.h"
#include "frame.h"

#endif

//*************************************************************************

#define iV_IMD_MAX_POINTS	256
#define iV_IMD_MAX_POLYS	256
#define iV_IMD_MAX_TEXANIM	256

//*************************************************************************

// polygon flags	b0..b7: col, b24..b31: anim index


#define iV_IMD_TEX			0x00000200
#define iV_IMD_TEXANIM		0x00004000	// iV_IMD_TEX must be set also
#define iV_IMD_PSXTEX		0x00008000	// - use playstation texture allocation method
#define iV_IMD_BSPFRESH		0x00010000	// Freshly created by the BSP 
#define iV_IMD_NOHALFPSXTEX 0x00020000

// shape override flags

// Are any of these really used anymore ?

#define iV_IMD_XEFFECT			0x1	// if this is set DO NOT PROCESS as a normal 3d pie ! (its a psx effect)
#define iV_IMD_XEFFECT_VPLANE	0x2	// Sprite effect drawn in view plane ( always facing player )
#define iV_IMD_XEFFECT_WPLANE	0x4	// Sprite effect drawn in world plane ( always parallel to terrain )
#define iV_IMD_XEFFECT_PROJECTILE	0x8	// Projectile effect.
#define iV_IMD_BINARY	0x10		// This Pie file was loaded as binary ... when freeing up do not free the pointers ... treat as one big chunk
#define iV_IMD_NOCULLSOME 0x20		// Some polys in the object are drawn without culling.

#define iV_IMD_XFLAT			0x00000100
#define iV_IMD_XTEX			0x00000200
#define iV_IMD_XWIRE			0x00000400
#define iV_IMD_XTRANS		0x00000800
#define iV_IMD_XGOUR			0x00001000
#define iV_IMD_XNOCUL		0x00002000
#define iV_IMD_XOUTLINE		0x00004000
#define iV_IMD_XFIXVIEW		0x00008000

// extended draw routines flags

#define iV_IMDX_AXES			0x00000001
#define iV_IMDX_BOUNDXZ		0x00000002
#define iV_IMDX_CHECKPOINT	0x00000004
#define iV_IMDX_POLYNUMBER	0x00000008
#define iV_IMDX_SPHERE		0x00000010
#define iV_IMDX_XGRID		0x00000020
#define iV_IMDX_YGRID		0x00000040
#define iV_IMDX_ZGRID		0x00000080
#define iV_IMDX_GRID			(iV_IMDX_XGRID | iV_IMDX_YGRID | iV_IMDX_ZGRID)


//*************************************************************************

extern BOOL iV_setImagePath(STRING *path);
extern iIMDShape *iV_IMDLoad(STRING *filename, iBool palkeep);
extern iIMDShape *iV_ProcessIMD(STRING **ppFileData, STRING *FileDataEnd, 
                                STRING *IMDpath, STRING *PCXpath, iBool palkeep);
iIMDShape *iV_ProcessBPIE(iIMDShape *, UDWORD size);

extern iBool iV_IMDSave(STRING *filename, iIMDShape *s, BOOL PieIMD);
extern void iV_IMDDebug(iIMDShape *s);

extern void iV_IMDRelease(iIMDShape *s);

// How high up do we want to stop looking 
#define DROID_VIS_UPPER	100

// How low do we stop looking? 
#define DROID_VIS_LOWER	10

/* not for PIEDRAW
extern void iV_IMDDrawTextured(iIMDShape *s);
extern void iV_PIEDraw(iIMDShape *s,int frame);
extern void iV_IMDDrawTexturedEnv(iIMDShape *shape, iTexture *env);
extern int iV_IMDDrawTexturedExtended(iIMDShape *shape, iPoint *point2d, uint32 flags);
extern void iV_IMDDrawWire(iIMDShape *shape, uint32 col);
extern void iV_IMDDrawWireExtended(iIMDShape *shape, uint32 col, uint32 flags);
extern void iV_IMDDrawTexturedHeightScaled(iIMDShape *shape, float scale);

// utils *****************************************************************

extern void iV_IMDRotateProject(iIMDShape *shape, iPoint *points2d);
extern void iV_IMDDraw2D(iIMDShape *shape, iPoint *points2d, uint32 col);
extern int iV_IMDPointInShape2D(iIMDShape *shape, iPoint *points2d, iPoint *point);
extern iBool iV_IMDPointInBoundXZ(iIMDShape *s, iPoint *point);
extern void iV_IMDDrawTextureRaise(iIMDShape *shape, float scale);
extern void iV_IMDDrawTexturedShade(iIMDShape *shape, int lightLevel);
*/

void tpInit(void);
void tpAddPIE(STRING *FileName, iIMDShape *pIMD);

#endif
