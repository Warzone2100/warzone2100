/*
** Copyright (c) 1995, 3Dfx Interactive, Inc.
** All Rights Reserved.
**
** This is UNPUBLISHED PROPRIETARY SOURCE CODE of 3Dfx Interactive, Inc.;
** the contents of this file may not be disclosed to third parties, copied or
** duplicated in any form, in whole or in part, without the prior written
** permission of 3Dfx Interactive, Inc.
**
** RESTRICTED RIGHTS LEGEND:
** Use, duplication or disclosure by the Government is subject to restrictions
** as set forth in subdivision (c)(1)(ii) of the Rights in Technical Data
** and Computer Software clause at DFARS 252.227-7013, and/or in similar or
** successor clauses in the FAR, DOD or NASA FAR Supplement. Unpublished -
** rights reserved under the Copyright Laws of the United States.
**
** $Header: /devel/cvg/glide/src/glideutl.h 11    1/07/98 11:18a Atai $
** $Log: /devel/cvg/glide/src/glideutl.h $
 * 
 * 11    1/07/98 11:18a Atai
 * remove GrMipMapInfo and GrGC.mm_table in glide3
 * 
 * 10    1/06/98 6:47p Atai
 * undo grSplash and remove gu routines
 * 
 * 9     1/05/98 6:04p Atai
 * move 3df gu related data structure from glide.h to glideutl.h
 * 
 * 8     12/18/97 2:13p Peter
 * fogTable cataclysm
 * 
 * 7     12/15/97 5:52p Atai
 * disable obsolete glide2 api for glide3
 * 
 * 6     8/14/97 5:32p Pgj
 * remove dead code per GMT
 * 
 * 5     6/12/97 5:19p Pgj
 * Fix bug 578
 * 
 * 4     3/05/97 9:36p Jdt
 * Removed guFbWriteRegion added guEncodeRLE16
 * 
 * 3     1/16/97 3:45p Dow
 * Embedded fn protos in ifndef FX_GLIDE_NO_FUNC_PROTO 
*/

/* Glide Utility routines */

#ifndef __GLIDEUTL_H__
#define __GLIDEUTL_H__

#ifdef __cplusplus
extern "C" {
#endif

#if defined(GLIDE3) && defined(GLIDE3_ALPHA)
/*
** 3DF texture file structs
*/

typedef struct
{
  FxU32               width, height;
  int                 small_lod, large_lod;
  GrAspectRatio_t     aspect_ratio;
  GrTextureFormat_t   format;
} Gu3dfHeader;

typedef struct
{
  FxU8  yRGB[16];
  FxI16 iRGB[4][3];
  FxI16 qRGB[4][3];
  FxU32 packed_data[12];
} GuNccTable;

typedef struct {
    FxU32 data[256];
} GuTexPalette;

typedef union {
    GuNccTable   nccTable;
    GuTexPalette palette;
} GuTexTable;

typedef struct
{
  Gu3dfHeader  header;
  GuTexTable   table;
  void        *data;
  FxU32        mem_required;    /* memory required for mip map in bytes. */
} Gu3dfInfo;

#endif

#ifndef FX_GLIDE_NO_FUNC_PROTO
/*
** rendering functions
*/

#ifndef GLIDE3_ALPHA
FX_ENTRY void FX_CALL
guAADrawTriangleWithClip( const GrVertex *a, const GrVertex
                         *b, const GrVertex *c);

FX_ENTRY void FX_CALL
guDrawTriangleWithClip(
                       const GrVertex *a,
                       const GrVertex *b,
                       const GrVertex *c
                       );

FX_ENTRY void FX_CALL
guDrawPolygonVertexListWithClip( int nverts, const GrVertex vlist[] );

/*
** hi-level rendering utility functions
*/
FX_ENTRY void FX_CALL
guAlphaSource( GrAlphaSource_t mode );

FX_ENTRY void FX_CALL
guColorCombineFunction( GrColorCombineFnc_t fnc );

FX_ENTRY int FX_CALL
guEncodeRLE16( void *dst, 
               void *src, 
               FxU32 width, 
               FxU32 height );

FX_ENTRY FxU16 * FX_CALL
guTexCreateColorMipMap( void );
#endif /* !GLIDE3_ALPHA */

#ifdef GLIDE3
FX_ENTRY void FX_CALL 
guGammaCorrectionRGB( FxFloat red, FxFloat green, FxFloat blue );
#endif

/*
** fog stuff
*/
FX_ENTRY float FX_CALL
guFogTableIndexToW( int i );

FX_ENTRY void FX_CALL
guFogGenerateExp( GrFog_t fogtable[], float density );

FX_ENTRY void FX_CALL
guFogGenerateExp2( GrFog_t fogtable[], float density );

FX_ENTRY void FX_CALL
guFogGenerateLinear(GrFog_t fogtable[],
                    float nearZ, float farZ );

/*
** endian stuff
*/
#ifndef GLIDE3_ALPHA
FX_ENTRY FxU32 FX_CALL
guEndianSwapWords( FxU32 value );

FX_ENTRY FxU16 FX_CALL
guEndianSwapBytes( FxU16 value );
#endif /* !GLIDE3_ALPHA */

/*
** hi-level texture manipulation tools.
*/
FX_ENTRY FxBool FX_CALL
gu3dfGetInfo( const char *filename, Gu3dfInfo *info );

FX_ENTRY FxBool FX_CALL
gu3dfLoad( const char *filename, Gu3dfInfo *data );

#endif /* FX_GLIDE_NO_FUNC_PROTO */

#ifdef __cplusplus
}
#endif

#endif /* __GLIDEUTL_H__ */
