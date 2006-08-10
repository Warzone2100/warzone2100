/***************************************************************************/
/*
 * piedef.h
 *
 * type defines for all pumpkin image library functions.
 *
 */
/***************************************************************************/

#ifndef _piedef_h
#define _piedef_h

/***************************************************************************/

#include "lib/framework/frame.h"
#include "ivisdef.h"
#include "ivispatch.h"

/***************************************************************************/
/*
 *	Global Definitions (CONSTANTS)
 */
/***************************************************************************/
#define DEG_360	65536
#define DEG_1	(DEG_360/360)
#define DEG_2	(DEG_360/180)
#define DEG_60	(DEG_360/6)
#define DEG(X)	(DEG_1 * (X))

#define FP12_SHIFT			12
#define FP12_MULTIPLIER			(1<<12)
#define STRETCHED_Z_SHIFT		10		//stretchs z range for (1000 to 4000) to (8000 to 32000)
#define	MAX_Z				(32000.0f)	//raised to 32000 from 6000 when stretched
#define	INV_MAX_Z			(0.00003125f)	//1/32000
#define MIN_STRETCHED_Z			256
#define	LONG_WAY			(1<<15)
#define	LONG_TEST			(1<<14)
#define INTERFACE_DEPTH			(MAX_Z - 1.0f)
#define INTERFACE_DEPTH_3DFX		(65535)
#define INV_INTERFACE_DEPTH_3DFX	(1.0f/65535.0f)
#define BUTTON_DEPTH			(2000)		 //will be stretched to 16000

#define TEXTURE_SIZE			(256.0f)
#define INV_TEX_SIZE			(0.00390625f)



#define MAX_FILE_PATH		256
#define pie_MAX_POLY_SIZE	16

//Effects
#define pie_MAX_BRIGHT_LEVEL 255
#define pie_BRIGHT_LEVEL_200 200
#define pie_BRIGHT_LEVEL_180 180
#define pie_DROID_BRIGHT_LEVEL 192

//Render style flags for all pie draw functions
#define pie_FLAG_MASK			0xffff
#define pie_FLAT			0x1
#define pie_TRANSLUCENT			0x2
#define pie_ADDITIVE			0x4
#define pie_NO_BILINEAR			0x8
#define pie_HEIGHT_SCALED		0x10
#define pie_RAISE			0x20
#define pie_BUTTON			0x40

#define pie_RAISE_SCALE			256

#define pie_BAND			0x80
#define pie_BAND_RED			0x90
#define pie_BAND_GREEN			0xa0
#define pie_BAND_YELLOW			0xb0
#define pie_BAND_BLUE			0xc0

#define pie_DRAW_DISC			0x800
#define pie_DRAW_DISC_RED		0x900
#define pie_DRAW_DISC_GREEN		0xa00
#define pie_DRAW_DISC_YELLOW		0xb00
#define pie_DRAW_DISC_BLUE		0xc00

#define pie_GLOW			0x8000
#define pie_GLOW_RED			0x9000
#define pie_GLOW_GREEN			0xa000
#define pie_GLOW_YELLOW			0xb000
#define pie_GLOW_BLUE			0xc000
#define pie_GLOW_STRENGTH		63

#define pie_MAX_POINTS			512
#define pie_MAX_POLYS			512
#define pie_MAX_POLY_VERTS		10

#define pie_FILLRED			16
#define pie_FILLGREEN			16
#define pie_FILLBLUE			128
#define pie_FILLTRANS			128

#define MAX_UB_LIGHT			((UBYTE)255)
#define MIN_UB_LIGHT			((UBYTE)0)
#define MAX_LIGHT			0xffffffff

/***************************************************************************/
/*
 *	Global Definitions (MACROS)
 */
/***************************************************************************/
#define pie_MIN(a,b)	(((a) < (b)) ? (a) : (b))
#define pie_MAX(a,b)	(((a) > (b)) ? (a) : (b))
#define pie_ABS(a)		(((a) < 0) ? (-(a)) : (a))

#define pie_ADDLIGHT(l,x)						\
(((l)->byte.r > (MAX_UB_LIGHT - (x))) ? ((l)->byte.r = MAX_UB_LIGHT) : ((l)->byte.r +=(x)));		\
(((l)->byte.g > (MAX_UB_LIGHT - (x))) ? ((l)->byte.g = MAX_UB_LIGHT) : ((l)->byte.g +=(x)));		\
(((l)->byte.b > (MAX_UB_LIGHT - (x))) ? ((l)->byte.b = MAX_UB_LIGHT) : ((l)->byte.b +=(x)));

#define pie_SUBTRACTLIGHT(l,x)						\
(((l->byte.r) < (x)) ? ((l->byte.r) = MIN_UB_LIGHT) : ((l->byte.r) -=(x)));		\
(((l->byte.g) < (x)) ? ((l->byte.g) = MIN_UB_LIGHT) : ((l->byte.g) -=(x)));		\
(((l->byte.b) < (x)) ? ((l->byte.b) = MIN_UB_LIGHT) : ((l->byte.b) -=(x)));


/***************************************************************************/
/*
 *	Global Definitions (STRUCTURES)
 */
/***************************************************************************/

typedef struct {UBYTE b, g, r, a;} PIELIGHTBYTES; //for byte fields in a DWORD
typedef union  {PIELIGHTBYTES byte; UDWORD argb;} PIELIGHT;
typedef struct {UBYTE r, g, b, a;} PIEVERTLIGHT;
typedef struct {SDWORD sx, sy, sz; UWORD tu, tv; PIELIGHT light, specular;} PIEVERTEX;

typedef struct {float d3dx, d3dy, d3dz;} PIEPIXEL;

typedef struct {SWORD x, y, w, h;} PIERECT; //screen rectangle
typedef struct {SDWORD texPage; SWORD tu, tv, tw, th;} PIEIMAGE; //an area of texture
typedef struct {UDWORD pieFlag; PIELIGHT colour, specular; UBYTE light, trans, scale, height;} PIESTYLE; //render style for pie draw functions

typedef struct {long n; char msge[240];} iError;
typedef int32 fixed;

// This is the new resource loaded structure (TEXPAGE)
typedef struct
{
	iSprite *Texture;
	iPalette *Palette;
} TEXTUREPAGE;


	typedef struct {
		UDWORD flags;
		SDWORD nVrts;
		void *pVrts;
		iTexAnim *pTexAnim;
	} PIED3DPOLY;
	typedef struct {
		UDWORD flags;
		SDWORD nVrts;
		PIEVERTEX *pVrts;
		iTexAnim *pTexAnim;
	} PIEPOLY;


/***************************************************************************/
/*
 *	Global Variables
 */
/***************************************************************************/

/***************************************************************************/
/*
 *	Global ProtoTypes
 */
/***************************************************************************/
extern void pie_Draw3DShape(iIMDShape *shape, int frame, int team, UDWORD colour, UDWORD specular, int pieFlag, int pieData);
//extern void pie_Draw3DIntelShape(iIMDShape *shape, int frame, int team, UDWORD colour, UDWORD specular, int pieFlag, int pieData);
//extern void pie_Draw3DNowShape(iIMDShape *shape, int frame, int team, UDWORD col, UDWORD spec, int pieFlag, int pieFlagData);
extern void pie_DrawImage(PIEIMAGE *image, PIERECT *dest, PIESTYLE *style);
extern void pie_DrawImage270(PIEIMAGE *image, PIERECT *dest, PIESTYLE *style);

//PIEVERTEX line draw for all hardware modes
extern void pie_DrawLine(SDWORD x0, SDWORD y0, SDWORD x1, SDWORD y1, UDWORD colour, BOOL bclip);
//iVetrex triangle draw for software modes
extern void pie_DrawTriangle(iVertex *pv, iTexture* texPage, UDWORD renderFlags, iPoint *offset);
//PIEVERTEX poly draw for all hardware modes
extern void pie_DrawPoly(SDWORD numVrts, PIEVERTEX *aVrts, SDWORD texPage, void* psEffects);
//PIEVERTEX triangle draw (glide specific)
extern void	pie_DrawFastTriangle(PIEVERTEX *v1, PIEVERTEX *v2, PIEVERTEX *v3, iTexture* texPage, int pieFlag, int pieFlagData);

extern void pie_GetResetCounts(SDWORD* pPieCount, SDWORD* pTileCount, SDWORD* pPolyCount, SDWORD* pStateCount);

extern void SetBSPObjectPos(SDWORD x,SDWORD y,SDWORD z);
extern void SetBSPCameraPos(SDWORD x,SDWORD y,SDWORD z);

//piedraw functions used in piefunc.c
extern void pie_D3DPoly(PIED3DPOLY *poly);

// PNG
BOOL pie_PNGLoadMemToBuffer(int8 *pngimage, iSprite *s, iColour *pal);
iBool pie_PNGLoadMem(int8 *pngimage, iSprite *s, iColour *pal);

//necromancer
extern void pie_DrawTile(PIEVERTEX *pv0, PIEVERTEX *pv1, PIEVERTEX *pv2, PIEVERTEX *pv3,  SDWORD texPage);

void SetBSPObjectRot(SDWORD Yaw, SDWORD Pitch);

void pie_BeginLighting(float x, float y, float z);
void pie_EndLighting(void);
void pie_RemainingPasses(void);

void pie_CleanUp( void );

#endif // _piedef_h
