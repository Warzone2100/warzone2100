#ifndef _vsr_
#define _vsr_

#include "ivi.h"
extern iBool _mode_sr(void);
extern void _close_sr(void);
extern void _vsync_sr(void);
extern void _bank_off_sr(void);
extern void _bank_on_sr(void);
extern void _palette_sr(int i, int r, int g, int b);
extern void _palette_setup_sr(void);
extern void _clear_sr(uint32 colour);
extern void _hline_sr(int x1, int x2, int y, uint32 colour);
extern void _vline_sr(int y1, int y2, int x, uint32 colour);
extern void _line_sr(int x1, int y1, int x2, int y2, uint32 colour);
extern void _aaline_sr(int x1, int y1, int x2, int y2, uint32 colour);
extern void _pixel_sr(int x, int y, uint32 colour);
extern void _circle_sr(int x, int y, int r, uint32 colour);
extern void _circlef_sr(int x, int y, int r, uint32 colour);
extern void _boxf_sr(int x1, int y1, int x2, int y2, uint32 colour);
extern void _box_sr(int x1, int y1, int x2, int y2, uint32 colour);

extern void _ftriangle_sr(iVertex *vrt, uint32 col);
extern void _gtriangle_sr(iVertex *vrt);
extern void _ttriangle_sr(iVertex *vrt, iTexture  *tex);
extern void _tttriangle_sr(iVertex *vrt, iTexture  *tex);
extern void _triangle_sr(iVertex *vrt, iTexture *tex, uint32 type);

extern void _fpolygon_sr(int npoints, iVertex *vrt, uint32 col);
extern void _gpolygon_sr(int npoints, iVertex *vrt);
extern void _tpolygon_sr(int npoints, iVertex *vrt, iTexture  *tex);
extern void _ttpolygon_sr(int npoints, iVertex *vrt, iTexture  *tex);
extern void _polygon_sr(int npoints, iVertex *vrt, iTexture *tex, uint32 type);

extern void _fquad_sr(iVertex *vrt);
extern void _gquad_sr(iVertex *vrt);
extern void _tquad_sr(iVertex *vrt, iTexture *tex);
extern void _ttquad_sr(iVertex *vrt, iTexture  *tex);
extern void _quad_sr(iVertex *vrt, iTexture *tex, uint32 type);

extern void _bitmap_sr(iBitmap *bmp, int x, int y, int w, int h, int ow);
extern void _bitmapcolour_sr(iBitmap *bmp, int x, int y, int w, int h, int ow,int ColourIndex);
extern void _tbitmapcolour_sr(iBitmap *bmp, int x, int y, int w, int h, int ow,int ColourIndex);
extern void _rbitmap_sr(iBitmap *bmp, int x, int y, int w, int h, int tw, int th);
extern void _rbitmapr90_sr(iBitmap *bmp, int x, int y, int w, int h, int tw, int th);
extern void _rbitmapr180_sr(iBitmap *bmp, int x, int y, int w, int h, int tw, int th);
extern void _rbitmapr270_sr(iBitmap *bmp, int x, int y, int w, int h, int tw, int th);

extern void _gbitmap_sr(iBitmap *bmp, int x, int y, int w, int h);
extern void _tbitmap_sr(iBitmap *bmp, int x, int y, int w, int h, int ow);
extern void _sbitmap_sr(iBitmap *bmp, int x, int y, int w, int h, int ow);
extern void _bitmapr90_sr(iBitmap *bmp, int x, int y, int w, int h, int ow);
extern void _bitmapr180_sr(iBitmap *bmp, int x, int y, int w, int h, int ow);
extern void _bitmapr270_sr(iBitmap *bmp, int x, int y, int w, int h, int ow);

#endif
