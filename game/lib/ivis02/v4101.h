#ifndef _v4101_
#define _v4101_


#include "ivisdef.h"
extern iBool _mode_4101(void);
extern void _close_4101(void);
extern void _vsync_4101(void);
extern void _bank_off_4101(void);
extern void _bank_on_4101(void);
extern void _palette_4101(int i, int r, int g, int b);
extern void _clear_4101(uint32 colour);
extern void _hline_4101(int x1, int x2, int y, uint32 colour);
extern void _vline_4101(int y1, int y2, int x, uint32 colour);
extern void _line_4101(int x1, int y1, int x2, int y2, uint32 colour);
extern void _aaline_4101(int x1, int y1, int x2, int y2, uint32 colour);
extern void _pixel_4101(int x, int y, uint32 colour);
extern void _circle_4101(int x, int y, int r, uint32 colour);
extern void _circlef_4101(int x, int y, int r, uint32 colour);
extern void _boxf_4101(int x1, int y1, int x2, int y2, uint32 colour);
extern void _box_4101(int x1, int y1, int x2, int y2, uint32 colour);

extern void _ftriangle_4101(iVertex *vrt, uint32 col);
extern void _tgtriangle_4101(iVertex *vrt, iTexture  *tex);
extern void _gtriangle_4101(iVertex *vrt);
extern void _ttriangle_4101(iVertex *vrt, iTexture  *tex);
extern void _tttriangle_4101(iVertex *vrt, iTexture  *tex);
extern void _triangle_4101(iVertex *vrt, iTexture *tex, uint32 type);

extern void _fpolygon_4101(int npoints, iVertex *vrt, uint32 col);
extern void _gpolygon_4101(int npoints, iVertex *vrt);
extern void _tpolygon_4101(int npoints, iVertex *vrt, iTexture  *tex);
extern void _tgpolygon_4101(int npoints, iVertex *vrt, iTexture  *tex);
extern void _tspolygon_4101(int npoints, iVertex *vrt, iTexture  *tex, int lightValue);
extern void _ttpolygon_4101(int npoints, iVertex *vrt, iTexture  *tex);
extern void _ttwpolygon_4101(int npoints, iVertex *vrt, iTexture  *tex);
extern void _polygon_4101(int npoints, iVertex *vrt, iTexture *tex, uint32 type);
extern void _tstriangle_4101(iVertex *vrt, iTexture  *tex, int brightness);
extern void _ttspolygon_4101(int npoints, iVertex *vrt, iTexture  *tex, int brightness);
extern void _ttstriangle_4101(iVertex *vrt, iTexture  *tex, int brightness);


extern void _fquad_4101(iVertex *vrt);
extern void _gquad_4101(iVertex *vrt);
extern void _tquad_4101(iVertex *vrt, iTexture *tex);
extern void _ttquad_4101(iVertex *vrt, iTexture  *tex);
extern void _quad_4101(iVertex *vrt, iTexture *tex, uint32 type);

extern void _bitmap_4101(iBitmap *bmp, int x, int y, int w, int h, int ow);
extern void _tbitmapcolour_4101(iBitmap *bmp, int x, int y, int w, int h, int ow,int ColourIndex);
extern void _bitmapcolour_4101(iBitmap *bmp, int x, int y, int w, int h, int ow,int ColourIndex);

extern void _rbitmap_4101(iBitmap *bmp, int x, int y, int w, int h, int tw, int th);
extern void _rbitmapr90_4101(iBitmap *bmp, int x, int y, int w, int h, int tw, int th);
extern void _rbitmapr180_4101(iBitmap *bmp, int x, int y, int w, int h, int tw, int th);
extern void _rbitmapr270_4101(iBitmap *bmp, int x, int y, int w, int h, int tw, int th);

extern void _gbitmap_4101(iBitmap *bmp, int x, int y, int w, int h);
extern void _tbitmap_4101(iBitmap *bmp, int x, int y, int w, int h, int ow);
extern void _sbitmap_4101(iBitmap *bmp, int x, int y, int w, int h, int ow);
extern void _bitmapr90_4101(iBitmap *bmp, int x, int y, int w, int h, int ow);
extern void _bitmapr180_4101(iBitmap *bmp, int x, int y, int w, int h, int ow);
extern void _bitmapr270_4101(iBitmap *bmp, int x, int y, int w, int h, int ow);
extern void iV_StrobeLine(int x1, int y1, int x2, int y2);



#endif
