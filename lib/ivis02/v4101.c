//*************************************************************************
//*** v4101.c DDX 640x480x256
//*
//* V2.2 10-05-95.23-08-95.02-12-96
//*
//******

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SDL/SDL.h>

#include "lib/ivis_common/ivi.h"
#include "lib/ivis_common/rendmode.h"
#include "lib/ivis_common/piemode.h"
#include "lib/ivis_common/bug.h"
#include "lib/ivis_common/piepalette.h"
#include "piematrix.h" // clockwise check
#include "lib/ivis_common/pieclip.h"

#include "screen.h"

#include "lib/framework/frame.h"
#include "lib/framework/frameint.h"

char	colour = 0;

/* A whole load of function prototypes that I've added - Alex */
/* Draw's a dotted line */
void iV_StrobeLine(int x1, int y1, int x2, int y2);
/* Textured gouraud sahded triangle plotter */
void _tgtriangle_4101(iVertex *vrt, iTexture  *tex);
/* Textured gouraud shaded polygon plotter */
void _tgpolygon_4101(int npoints, iVertex *vrt, iTexture  *tex);
/* Textured gouraud shaed horizontal lines */
static void _tghline_4101(int x1, int x2, int y, fixed btx1, fixed btx2, fixed bty1, fixed bty2, fixed bg1, fixed bg2, int xshift, iBitmap *tex);
/* Textured level shaded horizontal lines */
static void _tfhline_4101(int x1, int x2, int y, fixed btx1, fixed btx2, fixed bty1, fixed bty2, int xshift, iBitmap *tex, int shade);
/* Textured transparent levels shaded polygons */
void _ttspolygon_4101(int npoints, iVertex *vrt, iTexture  *tex, int brightness);
/* Textured level shaded polygons */
void _tspolygon_4101(int npoints, iVertex *vrt, iTexture  *tex, int lightValue);
/* Textured level shaded triangles */
void _tstriangle_4101(iVertex *vrt, iTexture  *tex, int brightness);
/* Textured transparent level shaded horizontal line */
static void _ttfhline_4101(int x1, int x2, int y, fixed btx1, fixed btx2, fixed bty1, fixed bty2, int xshift, iBitmap *tex, int shade);
/* Textured transaprent level shaded triangle */
void _ttstriangle_4101(iVertex *vrt, iTexture  *tex, int brightness);

//*************************************************************************

#ifdef _DEBUG
	#define ACCURATE
#endif

//*************************************************************************

#define WIDTH_4101	(pie_GetVideoBufferWidth())
#define HEIGHT_4101	(pie_GetVideoBufferHeight())
#define DEPTH_4101	8
#define SIZE_4101		(WIDTH_4101 * HEIGHT_4101)
#define BSHIFT			11

//*************************************************************************

#define USE_THLINE																\
	int __x, __dx, __col;														\
	fixed __du, __dv;																\
	uint8 *__pp;																	\
	int _x1_, _x2_, _y_;															\
	fixed _btx1_, _btx2_, _bty1_, _bty2_;									\
	int _xshift_;																	\
	iBitmap *_tex_;


#define USE_THLINE_ANYWIDTH													\
	int __x, __dx, __col;														\
	fixed __du, __dv;																\
	uint8 *__pp;																	\
	int _x1_, _x2_, _y_;															\
	fixed _btx1_, _btx2_, _bty1_, _bty2_;									\
	int _xwidth_;																	\
	iBitmap *_tex_;

#define USE_HLINE																	\
	int __dx, __i, __m;															\
	uint8 *__bp;																	\
	uint32 *__dp;																	\
	int _x1_, _x2_, _y_;															\
	uint32 _col_;


#define _THLINE_A																	\
if (_x1_ > _x2_) {																\
	iV_SWAP(_x1_,_x2_);															\
	iV_SWAP(_btx1_,_btx2_);														\
	iV_SWAP(_bty1_,_bty2_);														\
}																						\
__dx = (_x2_ - _x1_ ) + 1;														\
__du = (_btx2_ - _btx1_) / __dx;												\
__dv = (_bty2_ - _bty1_) / __dx;												\
__pp = psRendSurface->buffer + psRendSurface->scantable[_y_] + _x1_;			\
for (__x = __dx; __x>0; __x--, __pp++) {									\
	__col = *((uint8 *)_tex_ + ((_bty1_ >> iV_DIVSHIFT)<<_xshift_)+(_btx1_>>iV_DIVSHIFT));	\
	*__pp = __col;																	\
	_btx1_ += __du;																\
	_bty1_ += __dv;																\
}


#define _TTHLINE_ANYWIDTH_A													\
if (_x1_ > _x2_) {																\
	iV_SWAP(_x1_,_x2_);															\
	iV_SWAP(_btx1_,_btx2_);														\
	iV_SWAP(_bty1_,_bty2_);														\
}																						\
__dx = (_x2_ - _x1_ ) + 1;														\
__du = (_btx2_ - _btx1_) / __dx;												\
__dv = (_bty2_ - _bty1_) / __dx;												\
__pp = psRendSurface->buffer + psRendSurface->scantable[_y_] + _x1_;			\
for (__x = __dx; __x>0; __x--, __pp++) {									\
	__col = *((uint8 *)_tex_ + ((_bty1_ >> iV_DIVSHIFT) * _xwidth_)+(_btx1_>>iV_DIVSHIFT));	\
	*__pp = __col;																	\
	_btx1_ += __du;																\
	_bty1_ += __dv;																\
}


#define _THLINE_C																	\
if (_x1_ > _x2_) {																\
	iV_SWAP(_x1_,_x2_);															\
	iV_SWAP(_btx1_,_btx2_);														\
	iV_SWAP(_bty1_,_bty2_);														\
}																						\
__dx = _x2_- _x1_;																\
__du = (((_btx2_-_btx1_)+iV_DIVMULTP)>>iV_DIVSHIFT) * _iVPRIM_DIVTABLE[__dx];	\
__dv = (((_bty2_-_bty1_)+iV_DIVMULTP)>>iV_DIVSHIFT) * _iVPRIM_DIVTABLE[__dx];	\
__pp = psRendSurface->buffer + psRendSurface->scantable[_y_] + _x1_;			\
for (__x = __dx; __x>0; __x--, __pp++) {									\
	__col = *((uint8 *)_tex_ + ((_bty1_ >> iV_DIVSHIFT)<<_xshift_)+(_btx1_>>iV_DIVSHIFT));	\
	*__pp = __col;																	\
	_btx1_ += __du;																\
	_bty1_ += __dv;																\
}


#define _TTHLINE_A																\
if (_x1_ > _x2_) 																\
{																				\
	iV_SWAP(_x1_,_x2_);															\
	iV_SWAP(_btx1_,_btx2_);														\
	iV_SWAP(_bty1_,_bty2_);														\
}																				\
__dx = (_x2_ - _x1_ ) + 1;														\
__du = (((_btx2_-_btx1_)+iV_DIVMULTP)>>iV_DIVSHIFT) * _iVPRIM_DIVTABLE[__dx];	\
__dv = (((_bty2_-_bty1_)+iV_DIVMULTP)>>iV_DIVSHIFT) * _iVPRIM_DIVTABLE[__dx];	\
__pp = psRendSurface->buffer + psRendSurface->scantable[_y_] + _x1_;			\
	if(pie_GetDitherStatus())													\
	{																			\
	 	if( (_y_ & 0x01) ^ (_x1_ &0x01) )										\
		{																		\
			__pp++;																\
		}																		\
		for (__x = __dx; __x>2; __x-=2, __pp+=2) 								\
		{																		\
			__col = *((uint8 *)_tex_ + ((_bty1_ >> iV_DIVSHIFT)<<_xshift_)+(_btx1_>>iV_DIVSHIFT));	\
			if (__col) *__pp = __col;																\
			_btx1_ += (__du*2);																		\
			_bty1_ += (__dv*2);																		\
		}																							\
	}																								\
	else																							\
	{																								\
		for (__x = __dx; __x>2; __x--, __pp++) 													\
		{																							\
		__col = *((uint8 *)_tex_ + ((_bty1_ >> iV_DIVSHIFT)<<_xshift_)+(_btx1_>>iV_DIVSHIFT));		\
		if (__col) *__pp = __col;																	\
		_btx1_ += (__du);																			\
		_bty1_ += (__dv); 																			\
		}																							\
	}
/*
//__du = (_btx2_ - _btx1_) / __dx;
//__dv = (_bty2_ - _bty1_) / __dx;
#define _TTHLINE_A																\
if (_x1_ > _x2_) {																\
	iV_SWAP(_x1_,_x2_);															\
	iV_SWAP(_btx1_,_btx2_);														\
	iV_SWAP(_bty1_,_bty2_);														\
}																						\
__dx = (_x2_ - _x1_ ) + 1;														\
__du = (_btx2_ - _btx1_) / __dx;												\
__dv = (_bty2_ - _bty1_) / __dx;												\
__pp = psRendSurface->buffer + psRendSurface->scantable[_y_] + _x1_;			\
for (__x = __dx; __x>0; __x--, __pp++) {									\
	__col = *((uint8 *)_tex_ + ((_bty1_ >> iV_DIVSHIFT)<<_xshift_)+(_btx1_>>iV_DIVSHIFT));	\
	if (__col) *__pp = __col;													\
	_btx1_ += __du;																\
	_bty1_ += __dv;																\
}
*/
#define _TTHLINE_C																\
if (_x1_ > _x2_) {																\
	iV_SWAP(_x1_,_x2_);															\
	iV_SWAP(_btx1_,_btx2_);														\
	iV_SWAP(_bty1_,_bty2_);														\
}																						\
__dx = _x2_- _x1_;																\
__du = (((_btx2_-_btx1_)+iV_DIVMULTP)>>iV_DIVSHIFT) * _iVPRIM_DIVTABLE[__dx];	\
__dv = (((_bty2_-_bty1_)+iV_DIVMULTP)>>iV_DIVSHIFT) * _iVPRIM_DIVTABLE[__dx];	\
__pp = psRendSurface->buffer + psRendSurface->scantable[_y_] + _x1_;			\
for (__x = __dx; __x>0; __x--, __pp++) {									\
	__col = *((uint8 *)_tex_ + ((_bty1_ >> iV_DIVSHIFT)<<_xshift_)+(_btx1_>>iV_DIVSHIFT));	\
	if (__col) *__pp = __col;													\
	_btx1_ += __du;																\
	_bty1_ += __dv;																\
}

#define _HLINE																		\
_col_ |= _col_<<8;																\
_col_ |= _col_<<16;																\
if (_x2_<_x1_)																		\
	iV_SWAP(_x1_,_x2_);															\
__bp = psRendSurface->buffer + _x1_ + psRendSurface->scantable[_y_];			\
__dx = _x2_ - _x1_;																\
if (__dx<8) {																		\
	for (__i=__dx; __i>=0; __i--)												\
		*__bp++ = _col_;															\
} else {																				\
	__m = __dx & 3;																\
	__dx -= __m;																	\
	if (__m<4) {																	\
		for (__i=__m; __i>=0; __i--)											\
			*__bp++ = (uint8) _col_;											\
	}																					\
	__dp = (uint32 *) __bp;														\
	for (__i=(__dx>>2); __i>0; __i--)										\
		*__dp++ = _col_;															\
}

/*
#ifdef ACCURATE
	#define _THLINE	_THLINE_A
	#define _TTHLINE	_TTHLINE_A
#else
	#define _THLINE	_THLINE_C
	#define _TTHLINE	_TTHLINE_C
#endif
*/
	#define _THLINE	_THLINE_A
	#define _TTHLINE	_TTHLINE_A

//*************************************************************************
//***
//*
//* on exit: 	TRUE == success, FASLE == failure
//*
//******

iBool _mode_4101(void)

{
int	width;

	int i;

	iV_DEBUG0("v4101[_mode_4101] = setup successful\n");

	// set surface attributes

	rendSurface.buffer		= 0;
	rendSurface.flags		= REND_SURFACE_SCREEN;
	rendSurface.usr			= iV_MODE_4101;
	rendSurface.size		= SIZE_4101;
	rendSurface.width		= pie_GetVideoBufferWidth();
	rendSurface.height		= pie_GetVideoBufferHeight();
	rendSurface.xcentre		= pie_GetVideoBufferWidth()/2;
	rendSurface.ycentre		= pie_GetVideoBufferHeight()/2;
	rendSurface.clip.left	= 0;
	rendSurface.clip.top	= 0;
	rendSurface.clip.right	= pie_GetVideoBufferWidth();
	rendSurface.clip.bottom	= pie_GetVideoBufferHeight();
	rendSurface.xpshift		= 10;
	rendSurface.ypshift		= 10;

	width = pie_GetVideoBufferWidth();
	for (i=0; i<iV_SCANTABLE_MAX; i++)
		rendSurface.scantable[i] = i * width;

	return TRUE;
}

//*************************************************************************
//*** tidy-up, free allocated VESA buffer
//*
//******

void _close_4101(void)

{

	rendSurface.buffer = NULL;
	rendSurface.flags = REND_SURFACE_UNDEFINED;
	rendSurface.usr = REND_UNDEFINED;
	rendSurface.size = 0;

	iV_DEBUG0("X1\n");

#ifdef iV_DDX

//	_release_objects();


/*
  repeated shutdown ?????? error tekneek
	if (_iVW_hWND) {
		DestroyWindow(_iVW_hWND);
		iV_DEBUG0("X3\n");
	}

*/

	iV_DEBUG0("X2\n");

	iV_DEBUG0("v4101[_close_4101] = close successful\n");

#endif

}

//*************************************************************************
//*** set palette register
//*
//* params	i = colour index (0..255)
//* 			r = red component (0..64)
//* 			g = green component (0..64)
//* 			b = blue component (0..64)
//*
//******



//*************************************************************************
//*** wait for vertical retrace
//*
//******

void _vsync_4101(void)

{
}

//*************************************************************************
//*** switch to non-displayed screen bank
//*
//******

void _bank_off_4101(void)

{
        SDL_Surface *screen = screenGetSDL();

        SDL_LockSurface(screen);
	psRendSurface->buffer = (unsigned char *) screen->pixels;
}

//*************************************************************************
//*** display off-screen bank
//*
//******

void _bank_on_4101(void)

{
        SDL_UnlockSurface(screenGetSDL());
}

//*************************************************************************
//*** clear prim
//*
//* params	colour = screen background colour
//*
//******

void _clear_4101(uint32 colour)

{
	int i, size, top, bottom;
	uint32 *p1, *p2;

	// ensure even pointers and size (match _bank_on_4101)

	top = psRendSurface->clip.top & 0xfffe;
	bottom = (psRendSurface->clip.bottom + 1) & 0xfffe;
	size = psRendSurface->scantable[(bottom-top)];

	p1 = (uint32 *) ((uint8 *) psRendSurface->buffer + psRendSurface->scantable[top]);
	p2 = (uint32 *) ((uint8 *) (((uint8 *) p1) + (size>>1)));


	colour |= colour<<8;
	colour |= colour<<16;

	for (i=size>>3; i>0; i--)
		*p1++ = *p2++ = colour;

}

//* prims *****************************************************************


//*************************************************************************
//*** fill horizontal line
//*
//* params	x1 		= left edge
//* 			x2			= right edge
//* 			y			= hline y
//* 			colour 	= hline colour
//******

void _hline_4101(int x1, int x2, int y, uint32 colour)

{
	int d, i, m;
	uint8 *bp;
	uint32 *dp;


	colour |= colour<<8;
	colour |= colour<<16;

	if (x2<x1)
		iV_SWAP(x1,x2);

/*
	if (x1>psRendSurface->clip.right || x2<psRendSurface->clip.left ||
			y<psRendSurface->clip.top	|| y>psRendSurface->clip.bottom)
		return;

	if (x1<psRendSurface->clip.left)
		x1 = psRendSurface->clip.left;
	if (x2>psRendSurface->clip.right)
		x2 = psRendSurface->clip.right;
*/


	bp = psRendSurface->buffer + x1 + psRendSurface->scantable[y];

	d = x2 - x1;

	if (d<8) {
		for (i=d; i>=0; i--)
			*bp++ = colour;
	} else {
		m = d & 3;
		d -= m;

		if (m<4) {
			for (i=m; i>=0; i--)
				*bp++ = (uint8) colour;
		}

		dp = (uint32 *) bp;

		for (i=(d>>2); i>0; i--)
			*dp++ = colour;
	}
}

//*************************************************************************
//*** plot vertical line
//*
//* params	y1 		= top y
//* 			y2	 		= bottom y
//* 			x			= vline x
//* 			colour	= vline colour
//******

void _vline_4101(int y1, int y2, int x, uint32 colour)

{
	int i, d;
	uint8 *p;

	if (y2<y1)
		iV_SWAP(y1,y2);

	p = psRendSurface->buffer + x + psRendSurface->scantable[y1];

	d = y2 - y1;

	for (i=d; i>=0; i--) {
		*p = colour;
		p += WIDTH_4101;
	}
}


//*************************************************************************
//*** plot pixel
//*
//* params	x 			= pixel x
//*		  	y			= pixel y
//* 			colour	= pixel colour
//******

void _pixel_4101(int x, int y, uint32 colour)

{
	uint8 *p;


	p = psRendSurface->buffer + x + psRendSurface->scantable[y];

	*p = colour;

}


//*************************************************************************
//*** plot filled box
//*
//* params	x1,y1		= box top left-hand corner
//* 			x2,y2		= box bottom right-hand corner
//* 			colour	= box fill colour
//******

void _boxf_4101(int x1, int y1, int x2, int y2, uint32 colour)

{
	int w, h, i, j, d, b, lineb_w, lined_w;
	iPointer db;


	db.bp = (uint8 *) psRendSurface->buffer + x1 + psRendSurface->scantable[y1];

	w = x2 - x1+1;
	h = y2 - y1+1;

	colour |= colour<<8;
	colour |= colour<<16;


	// width < 8, use store-byte mode

	if (w<8) {
		lineb_w = WIDTH_4101 - w;
		for (i=h; i>0; i--) {
			for (j=w; j>0; j--)
				*db.bp++ = (uint8) colour;
			db.bp += lineb_w;
		}

	// width >= 8, use store-doubleword mode

	} else {
		b = w & 0x3;
		d = w & 0xfffc;
		lined_w = (WIDTH_4101 - d)>>2;
		d >>= 2;
		for (i=h; i>0; i--) {
			for (j=d; j>0; j--)
				*db.dp++ = colour;

			if (b==3) {
				*db.bp++ = (uint8) colour;
				*db.bp++ = (uint8) colour;
				*db.bp++ = (uint8) colour;
			} else if (b==2) {
				*db.bp++ = (uint8) colour;
				*db.bp++ = (uint8) colour;
			} else if (b==1) {
				*db.bp++ = (uint8) colour;
			}
			db.dp += lined_w;
			db.bp -= b;
		}
	}
}

//**************************************************************************
//*** plot box
//*
//* params	x1,y1		= box top left-hand corner
//* 			x2,y2		= box bottom right-hand corner
//* 			colour	= box colour
//******

void _box_4101(int x1, int y1, int x2, int y2, uint32 colour)

{

	_hline_4101(x1,x2,y1,colour);
	_hline_4101(x1,x2,y2,colour);
	_vline_4101(y1,y2,x1,colour);
	_vline_4101(y1,y2,x2,colour);

}


//*************************************************************************
//*** plot line
//*
//* params	x1,y1		= line point 1
//* 			x2,y2		= line point 2
//* 			colour	= line colour
//******

void _line_4101(int x1, int y1, int x2, int y2, uint32 colour)

{
	int dx, dy, dy2, dy2dx2, e, xinc, xinc2, yinc, yinc2, tyinc, t;
	uint8 *p1, *p2;
	iBool even_w;


	p1 = psRendSurface->buffer + x1 + psRendSurface->scantable[y1];
	p2 = psRendSurface->buffer + x2 + psRendSurface->scantable[y2];

	dx = x2 - x1;

	if (dx<0) {
		xinc = -1;
		dx = -dx;
	} else
		xinc = 1;

	dy = y2 - y1;

	tyinc = WIDTH_4101;

	if (dy<0) {
		dy = -dy;
		tyinc = -WIDTH_4101;
	}

	if (dx>dy)
		yinc = tyinc;
	else {
		yinc = xinc;
		xinc = tyinc;
		t = dx;
		dx = dy;
		dy = t;
	}

	dy2 = dy<<1;
	dy2dx2 = dy2 - (dx<<1);
	e = dy2 - dx;

	xinc2 = -xinc;
	yinc2 = -yinc;

	even_w = ((dx & 1) == 0);
	if (even_w)	dx--;

	dx >>= 1;

	while (dx-->=0) {
		*p1 = colour;
		*p2 = colour;
		p1 += xinc;
		p2 += xinc2;
		if (e>0) {
			p1 += yinc;
			p2 += yinc2;
			e += dy2dx2;
		} else
			e += dy2;
	}

	if (even_w)	*p1 = colour;
}

void iV_StrobeLine(int x1, int y1, int x2, int y2)

{
	int dx, dy, dy2, dy2dx2, e, xinc, xinc2, yinc, yinc2, tyinc, t;
	uint8 *p1, *p2;
	iBool even_w;
	char	storecol;


	storecol = colour;
	p1 = psRendSurface->buffer + x1 + psRendSurface->scantable[y1];
	p2 = psRendSurface->buffer + x2 + psRendSurface->scantable[y2];

	dx = x2 - x1;

	if (dx<0) {
		xinc = -1;
		dx = -dx;
	} else
		xinc = 1;

	dy = y2 - y1;

	tyinc = WIDTH_4101;

	if (dy<0) {
		dy = -dy;
		tyinc = -WIDTH_4101;
	}

	if (dx>dy)
		yinc = tyinc;
	else {
		yinc = xinc;
		xinc = tyinc;
		t = dx;
		dx = dy;
		dy = t;
	}

	dy2 = dy<<1;
	dy2dx2 = dy2 - (dx<<1);
	e = dy2 - dx;

	xinc2 = -xinc;
	yinc2 = -yinc;

	even_w = ((dx & 1) == 0);
	if (even_w)	dx--;

	dx >>= 1;

	while (dx-->=0) {
		if(colour)
		{
			*p1 = colour;
			*p2 = colour;
		}
		p1 += xinc;
		p2 += xinc2;
		if (e>0) {
			p1 += yinc;
			p2 += yinc2;
			e += dy2dx2;
		} else
			e += dy2;
			colour = ~colour;
	}

	if (even_w)	*p1 = colour;
	colour = ~storecol;
}

//*************************************************************************
//*** plot anti-aliased line
//*
//* params	x1,y1		= line point 1
//* 			x2,y2		= line point 2
//* 			colour	= line colour
//******

void _aaline_4101(int x1, int y1, int x2, int y2, uint32 colour)

{

}


//*************************************************************************
//*** plot circle
//*
//* params	x,y		= circle centre
//* 			r	 		= radius
//* 			colour	= circle colour
//******

void _circle_4101(int x, int y, int r, uint32 colour)

{
	int xwidth, rwidth, xs, e;
	uint8 *op, *p;

	op = p = psRendSurface->buffer + x + psRendSurface->scantable[y];

	xwidth = x * WIDTH_4101;
	rwidth = r * WIDTH_4101;

	e = (3 - r)<<1;
	xs = 0;

	while (xs++<=r) {
		*(p+rwidth+xs) = colour;
		*(p+r+xwidth) = colour;
		*(p+r-xwidth) = colour;
		*(p-rwidth+xs) = colour;
		*(p-rwidth-xs) = colour;
		*(p-r-xwidth) = colour;
		*(p-r+xwidth) = colour;
		*(p+rwidth-xs) = colour;
		if (e>0) {
			r--;
			e += ((xs-r)<<2);
		} else
			e += (xs<<2);
	}
}

//*************************************************************************
//*** plot filled circle
//*
//* params	x,y		= circle centre
//* 			r			= radius
//* 			colour	= circle colour
//******

void _circlef_4101(int x, int y, int r, uint32 colour)

{
}

//*************************************************************************
//*** plot texture-mapped triangle
//*
//* params	vert 	= list triangle vertices (clockwise order)
//*			tex	= texture to apply
//*
//******

void _ttriangle_4101(iVertex *vrt, iTexture  *tex)

{
	int i, miny, li, dxl, dyl, dxr, dyr, y;
	fixed gradl, gradr, gradxl, gradxr, gradyl, gradyr;
	fixed bxl, bxr;
	int32 dtx, dty;
	fixed btxl, btyl, btxr, btyr;
	iVertex nvrt[3];

	USE_THLINE;

	for (miny = iV_DIVMULTP, i=0, li=0; i<3; i++) {
		if (vrt[i].y < miny) {
			miny = vrt[i].y;
			li = i;
		}
	}

	for (i=0; i<3; i++) {
		nvrt[i].x = vrt[li].x;
		nvrt[i].y = vrt[li].y;
		nvrt[i].u = vrt[li].u;
		nvrt[i].v = vrt[li].v;
		if (++li >= 3) li = 0;
	}

	dxl = nvrt[2].x - nvrt[0].x;
	dyl = nvrt[2].y - nvrt[0].y;
	dxr = nvrt[1].x - nvrt[0].x;
	dyr = nvrt[1].y - nvrt[0].y;
	gradl = _iVPRIM_DIVTABLE[dyl] * dxl;
	gradr = _iVPRIM_DIVTABLE[dyr] * dxr;
	bxl = bxr = ((int32)nvrt[0].x << iV_DIVSHIFT) + iV_DIVMULTP_2;
	y = nvrt[0].y;

	dtx = nvrt[2].u - nvrt[0].u;
	dty = nvrt[2].v - nvrt[0].v;
	gradxl = _iVPRIM_DIVTABLE[dyl] * dtx;
	gradyl = _iVPRIM_DIVTABLE[dyl] * dty;
	dtx = nvrt[1].u - nvrt[0].u;
	dty = nvrt[1].v - nvrt[0].v;
	gradxr = _iVPRIM_DIVTABLE[dyr] * dtx;
	gradyr = _iVPRIM_DIVTABLE[dyr] * dty;
	btxl = btxr = ((int32)nvrt[0].u << iV_DIVSHIFT);
	btyl = btyr = ((int32)nvrt[0].v << iV_DIVSHIFT);


	if (dyl < dyr) {
		while (dyl-- > 0) {
			bxl += gradl;
			bxr += gradr;
			btxl += gradxl;
			btyl += gradyl;
			btxr += gradxr;
			btyr += gradyr;

			_x1_ 		= bxl >> iV_DIVSHIFT;
			_x2_ 		= bxr >> iV_DIVSHIFT;
			_y_		= y++;
			_btx1_ 	= btxl;
			_btx2_   = btxr;
			_bty1_	= btyl;
			_bty2_	= btyr;
			_xshift_	= tex->xshift;
			_tex_		= tex->bmp;

			_THLINE;

/*
			_thline_4101(bxl >> iV_DIVSHIFT,bxr >> iV_DIVSHIFT,y++,btxl,btxr,
					btyl,btyr,tex->xshift,tex->bmp);
*/
		}


		dxl = nvrt[1].x - nvrt[2].x;
		dyl = nvrt[1].y - nvrt[2].y;
		gradl = _iVPRIM_DIVTABLE[dyl] * dxl;
		bxl = ((int32)nvrt[2].x << iV_DIVSHIFT) + iV_DIVMULTP_2;

		dtx = nvrt[1].u - nvrt[2].u;
		dty = nvrt[1].v - nvrt[2].v;
		gradxl = _iVPRIM_DIVTABLE[dyl] * dtx;
		gradyl = _iVPRIM_DIVTABLE[dyl] * dty;
		btxl = ((int32)nvrt[2].u << iV_DIVSHIFT);
		btyl = ((int32)nvrt[2].v << iV_DIVSHIFT);

		while (dyl-- > 0) {
			bxl += gradl;
			bxr += gradr;
			btxl += gradxl;
			btyl += gradyl;
			btxr += gradxr;
			btyr += gradyr;

			_x1_ 		= bxl >> iV_DIVSHIFT;
			_x2_ 		= bxr >> iV_DIVSHIFT;
			_y_		= y++;
			_btx1_ 	= btxl;
			_btx2_   = btxr;
			_bty1_	= btyl;
			_bty2_	= btyr;
			_xshift_	= tex->xshift;
			_tex_		= tex->bmp;

			_THLINE;

/*
			_thline_4101(bxl >> iV_DIVSHIFT,bxr >> iV_DIVSHIFT,y++,btxl,btxr,
					btyl,btyr,tex->xshift,tex->bmp);
*/
		}
	} else {
		while (dyr-- > 0) {
			bxl += gradl;
			bxr += gradr;
			btxl += gradxl;
			btyl += gradyl;
			btxr += gradxr;
			btyr += gradyr;

			_x1_ 		= bxl >> iV_DIVSHIFT;
			_x2_ 		= bxr >> iV_DIVSHIFT;
			_y_		= y++;
			_btx1_ 	= btxl;
			_btx2_   = btxr;
			_bty1_	= btyl;
			_bty2_	= btyr;
			_xshift_	= tex->xshift;
			_tex_		= tex->bmp;

			_THLINE;
/*
			_thline_4101(bxl >> iV_DIVSHIFT,bxr >> iV_DIVSHIFT,y++,btxl,btxr,
					btyl,btyr,tex->xshift,tex->bmp);
*/
		}

		dxr = nvrt[2].x - nvrt[1].x;
		dyr = nvrt[2].y - nvrt[1].y;
		gradr = _iVPRIM_DIVTABLE[dyr] * dxr;
		bxr = ((int32)nvrt[1].x << iV_DIVSHIFT) + iV_DIVMULTP_2;

		dtx = nvrt[2].u - nvrt[1].u;
		dty = nvrt[2].v - nvrt[1].v;
		gradxr = _iVPRIM_DIVTABLE[dyr] * dtx;
		gradyr = _iVPRIM_DIVTABLE[dyr] * dty;
		btxr = ((int32)nvrt[1].u << iV_DIVSHIFT);
		btyr = ((int32)nvrt[1].v << iV_DIVSHIFT);

		while (dyr-- > 0) {
			bxl += gradl;
			bxr += gradr;
			btxl += gradxl;
			btyl += gradyl;
			btxr += gradxr;
			btyr += gradyr;

			_x1_ 		= bxl >> iV_DIVSHIFT;
			_x2_ 		= bxr >> iV_DIVSHIFT;
			_y_		= y++;
			_btx1_ 	= btxl;
			_btx2_   = btxr;
			_bty1_	= btyl;
			_bty2_	= btyr;
			_xshift_	= tex->xshift;
			_tex_		= tex->bmp;

			_THLINE;
/*
			_thline_4101(bxl >> iV_DIVSHIFT,bxr >> iV_DIVSHIFT,y++,btxl,btxr,
					btyl,btyr,tex->xshift,tex->bmp);
*/
		}
	}
}

//*************************************************************************
//*** plot texture-mapped triangle (0 is transparent)
//*
//* params	vrt 	= list triangle vertices (clockwise order)
//*			tex	= texture to apply
//*
//******

void _tttriangle_4101(iVertex *vrt, iTexture  *tex)

{
	int i, miny, li, dxl, dyl, dxr, dyr, y;
	fixed gradl, gradr, gradxl, gradxr, gradyl, gradyr;
	fixed bxl, bxr;
	int32 dtx, dty;
	fixed btxl, btxr, btyl, btyr;
	iVertex nvrt[3];

	USE_THLINE;

	for (miny = iV_DIVMULTP, i=0, li=0; i<3; i++) {
		if (vrt[i].y < miny) {
			miny = vrt[i].y;
			li = i;
		}
	}

	for (i=0; i<3; i++) {
		nvrt[i].x = vrt[li].x;
		nvrt[i].y = vrt[li].y;
		nvrt[i].u = vrt[li].u;
		nvrt[i].v = vrt[li].v;
		if (++li >= 3) li = 0;
	}

	dxl = nvrt[2].x - nvrt[0].x;
	dyl = nvrt[2].y - nvrt[0].y;
	dxr = nvrt[1].x - nvrt[0].x;
	dyr = nvrt[1].y - nvrt[0].y;
	gradl = _iVPRIM_DIVTABLE[dyl] * dxl;
	gradr = _iVPRIM_DIVTABLE[dyr] * dxr;
	bxl = bxr = ((int32)nvrt[0].x << iV_DIVSHIFT) + iV_DIVMULTP_2;
	y = nvrt[0].y;

	dtx = nvrt[2].u - nvrt[0].u;
	dty = nvrt[2].v - nvrt[0].v;
	gradxl = _iVPRIM_DIVTABLE[dyl] * dtx;
	gradyl = _iVPRIM_DIVTABLE[dyl] * dty;
	dtx = nvrt[1].u - nvrt[0].u;
	dty = nvrt[1].v - nvrt[0].v;
	gradxr = _iVPRIM_DIVTABLE[dyr] * dtx;
	gradyr = _iVPRIM_DIVTABLE[dyr] * dty;
	btxl = btxr = ((int32)nvrt[0].u << iV_DIVSHIFT);
	btyl = btyr = ((int32)nvrt[0].v << iV_DIVSHIFT);


	if (dyl < dyr) {
		while (dyl-- > 0) {
			bxl += gradl;
			bxr += gradr;
			btxl += gradxl;
			btyl += gradyl;
			btxr += gradxr;
			btyr += gradyr;

			_x1_ = bxl >> iV_DIVSHIFT;
			_x2_ = bxr >> iV_DIVSHIFT;
			_y_	= y++;
			_btx1_ = btxl;
			_btx2_ = btxr;
			_bty1_ = btyl;
			_bty2_ = btyr;
			_xshift_ = tex->xshift;
			_tex_ = tex->bmp;

			_TTHLINE;
/*
			_tthline_4101(bxl >> iV_DIVSHIFT,bxr >> iV_DIVSHIFT,y++,btxl,btxr,
					btyl,btyr,tex->xshift,tex->bmp);
*/
		}


		dxl = nvrt[1].x - nvrt[2].x;
		dyl = nvrt[1].y - nvrt[2].y;
		gradl = _iVPRIM_DIVTABLE[dyl] * dxl;
		bxl = ((int32)nvrt[2].x << iV_DIVSHIFT) + iV_DIVMULTP_2;

		dtx = nvrt[1].u - nvrt[2].u;
		dty = nvrt[1].v - nvrt[2].v;
		gradxl = _iVPRIM_DIVTABLE[dyl] * dtx;
		gradyl = _iVPRIM_DIVTABLE[dyl] * dty;
		btxl = ((int32)nvrt[2].u << iV_DIVSHIFT);
		btyl = ((int32)nvrt[2].v << iV_DIVSHIFT);

		while (dyl-- > 0) {
			bxl += gradl;
			bxr += gradr;
			btxl += gradxl;
			btyl += gradyl;
			btxr += gradxr;
			btyr += gradyr;

			_x1_ = bxl >> iV_DIVSHIFT;
			_x2_ = bxr >> iV_DIVSHIFT;
			_y_	= y++;
			_btx1_ = btxl;
			_btx2_ = btxr;
			_bty1_ = btyl;
			_bty2_ = btyr;
			_xshift_ = tex->xshift;
			_tex_ = tex->bmp;

			_TTHLINE;
/*
			_tthline_4101(bxl >> iV_DIVSHIFT,bxr >> iV_DIVSHIFT,y++,btxl,btxr,
					btyl,btyr,tex->xshift,tex->bmp);
*/
		}
	} else {
		while (dyr-- > 0) {
			bxl += gradl;
			bxr += gradr;
			btxl += gradxl;
			btyl += gradyl;
			btxr += gradxr;
			btyr += gradyr;

			_x1_ = bxl >> iV_DIVSHIFT;
			_x2_ = bxr >> iV_DIVSHIFT;
			_y_	= y++;
			_btx1_ = btxl;
			_btx2_ = btxr;
			_bty1_ = btyl;
			_bty2_ = btyr;
			_xshift_ = tex->xshift;
			_tex_ = tex->bmp;

			_TTHLINE;
/*
			_tthline_4101(bxl >> iV_DIVSHIFT,bxr >> iV_DIVSHIFT,y++,btxl,btxr,
					btyl,btyr,tex->xshift,tex->bmp);
*/
		}

		dxr = nvrt[2].x - nvrt[1].x;
		dyr = nvrt[2].y - nvrt[1].y;
		gradr = _iVPRIM_DIVTABLE[dyr] * dxr;
		bxr = ((int32)nvrt[1].x << iV_DIVSHIFT) + iV_DIVMULTP_2;

		dtx = nvrt[2].u - nvrt[1].u;
		dty = nvrt[2].v - nvrt[1].v;
		gradxr = _iVPRIM_DIVTABLE[dyr] * dtx;
		gradyr = _iVPRIM_DIVTABLE[dyr] * dty;
		btxr = ((int32)nvrt[1].u << iV_DIVSHIFT);
		btyr = ((int32)nvrt[1].v << iV_DIVSHIFT);

		while (dyr-- > 0) {
			bxl += gradl;
			bxr += gradr;
			btxl += gradxl;
			btyl += gradyl;
			btxr += gradxr;
			btyr += gradyr;

			_x1_ = bxl >> iV_DIVSHIFT;
			_x2_ = bxr >> iV_DIVSHIFT;
			_y_	= y++;
			_btx1_ = btxl;
			_btx2_ = btxr;
			_bty1_ = btyl;
			_bty2_ = btyr;
			_xshift_ = tex->xshift;
			_tex_ = tex->bmp;

			_TTHLINE;
/*
			_tthline_4101(bxl >> iV_DIVSHIFT,bxr >> iV_DIVSHIFT,y++,btxl,btxr,
					btyl,btyr,tex->xshift,tex->bmp);
*/
		}
	}
}


//*************************************************************************
//*** plot gouraud-shaded triangle
//*
//* params 	pts = triangle vertices
//*
//******

void _gtriangle_4101(iVertex *pts)

{
}


//*************************************************************************
//*** plot flat-shaded triangle
//*
//* params	vrt = triangle vertices
//*
//******

void _ftriangle_4101(iVertex *vrt, uint32 col)

{

	int i, miny, li, dxl, dyl, dxr, dyr, y;
	fixed gradl, gradr, bxl, bxr;
	iVertex nvrt[3];

	USE_HLINE;



	for (miny = iV_DIVMULTP, i=0, li=0; i<3; i++) {
		if (vrt[i].y < miny) {
			miny = vrt[i].y;
			li = i;
		}
	}

	for (i=0; i<3; i++) {
		nvrt[i].x = vrt[li].x;
		nvrt[i].y = vrt[li].y;
		if (++li >= 3) li = 0;
	}

	dxl = nvrt[2].x - nvrt[0].x;
	dyl = nvrt[2].y - nvrt[0].y;
	dxr = nvrt[1].x - nvrt[0].x;
	dyr = nvrt[1].y - nvrt[0].y;
	gradl = _iVPRIM_DIVTABLE[dyl] * dxl;
	gradr = _iVPRIM_DIVTABLE[dyr] * dxr;
	bxl = bxr = ((int32)nvrt[0].x << iV_DIVSHIFT) + iV_DIVMULTP_2;
	y = nvrt[0].y;


	if (dyl < dyr) {
		while (dyl-- > 0) {
			bxl += gradl;
			bxr += gradr;

			_x1_ = bxl >> iV_DIVSHIFT;
			_x2_ = bxr >> iV_DIVSHIFT;
			_y_ = y++;
			_col_ = col;

			_HLINE;
/*
			_hline_4101(bxl >> iV_DIVSHIFT, bxr >> iV_DIVSHIFT, y++, col);
*/
		}


		dxl = nvrt[1].x - nvrt[2].x;
		dyl = nvrt[1].y - nvrt[2].y;
		gradl = _iVPRIM_DIVTABLE[dyl] * dxl;
		bxl = ((int32)nvrt[2].x << iV_DIVSHIFT) + iV_DIVMULTP_2;

		while (dyl-- > 0) {
			bxl += gradl;
			bxr += gradr;

			_x1_ = bxl >> iV_DIVSHIFT;
			_x2_ = bxr >> iV_DIVSHIFT;
			_y_ = y++;
			_col_ = col;

			_HLINE;
/*
			_hline_4101(bxl >> iV_DIVSHIFT, bxr >> iV_DIVSHIFT, y++, col);
*/
		}
	} else {
		while (dyr-- > 0) {
			bxl += gradl;
			bxr += gradr;

			_x1_ = bxl >> iV_DIVSHIFT;
			_x2_ = bxr >> iV_DIVSHIFT;
			_y_ = y++;
			_col_ = col;

			_HLINE;
/*
			_hline_4101(bxl >> iV_DIVSHIFT, bxr >> iV_DIVSHIFT, y++, col);
*/
		}

		dxr = nvrt[2].x - nvrt[1].x;
		dyr = nvrt[2].y - nvrt[1].y;
		gradr = _iVPRIM_DIVTABLE[dyr] * dxr;
		bxr = ((int32)nvrt[1].x << iV_DIVSHIFT) + iV_DIVMULTP_2;

		while (dyr-- > 0) {
			bxl += gradl;
			bxr += gradr;

			_x1_ = bxl >> iV_DIVSHIFT;
			_x2_ = bxr >> iV_DIVSHIFT;
			_y_ = y++;
			_col_ = col;

			_HLINE;
/*
			_hline_4101(bxl >> iV_DIVSHIFT, bxr >> iV_DIVSHIFT, y++, col);
*/
		}
	}
}


void _ttquad_4101(iVertex *vrt, iTexture  *tex)

{
}

void _tquad_4101(iVertex *vrt, iTexture  *tex)

{
}

void _fquad_4101(iVertex *vrt)

{
}

void _gquad_4101(iVertex *vrt)

{
}


//*************************************************************************
//*** plot texture-mapped polygon.
//*
//* params	npoints	= number of points in poly
//* 							(max set by iV_POLY_MAX_POINTS in vid.h)
//*			vrt 		= list poly vertices (clockwise order)
//*			tex		= texture to apply
//*
//******

void _tpolygon_4101(int npoints, iVertex *vrt, iTexture  *tex)

{
	static iVertex nvrt[iV_POLY_MAX_POINTS];
	int i, miny, li, ri, dxl, dyl, dxr, dyr, y;
	fixed gradl, gradr, gradxl, gradxr, gradyl, gradyr;
	fixed bxl, bxr;
	int32 dtx, dty;
	fixed btxl, btxr, btyl, btyr;

	USE_THLINE;



	for (miny = iV_DIVMULTP, i=0, li=0; i<npoints; i++) {
		if (vrt[i].y < miny) {
			miny = vrt[i].y;
			li = i;
		}
	}

	for (i=0; i<npoints; i++) {
		nvrt[i].x = vrt[li].x;
		nvrt[i].y = vrt[li].y;
		nvrt[i].u = vrt[li].u;
		nvrt[i].v = vrt[li].v;
		if (++li >= npoints) li = 0;
	}

	dxl = nvrt[npoints-1].x - nvrt[0].x;
	dyl = nvrt[npoints-1].y - nvrt[0].y;
	dxr = nvrt[1].x - nvrt[0].x;
	dyr = nvrt[1].y - nvrt[0].y;
	gradl = _iVPRIM_DIVTABLE[dyl] * dxl;
	gradr = _iVPRIM_DIVTABLE[dyr] * dxr;
	bxl = bxr = ((int32)nvrt[0].x << iV_DIVSHIFT) + iV_DIVMULTP_2;
	y = nvrt[0].y;

	dtx = nvrt[npoints-1].u - nvrt[0].u;
	dty = nvrt[npoints-1].v - nvrt[0].v;
	gradxl = _iVPRIM_DIVTABLE[dyl] * dtx;
	gradyl = _iVPRIM_DIVTABLE[dyl] * dty;
	dtx = nvrt[1].u - nvrt[0].u;
	dty = nvrt[1].v - nvrt[0].v;
	gradxr = _iVPRIM_DIVTABLE[dyr] * dtx;
	gradyr = _iVPRIM_DIVTABLE[dyr] * dty;
	btxl = btxr = ((int32)nvrt[0].u << iV_DIVSHIFT);
	btyl = btyr = ((int32)nvrt[0].v << iV_DIVSHIFT);

	li = npoints-1;
	ri = 1;
	npoints -= 1;


	for (;;) {
		if (dyl < dyr) {
			dyr -= dyl;
			while (dyl-- > 0) {
				bxl += gradl;
				bxr += gradr;
				btxl += gradxl;
				btyl += gradyl;
				btxr += gradxr;
				btyr += gradyr;

				_x1_ 		= bxl >> iV_DIVSHIFT;
				_x2_ 		= bxr >> iV_DIVSHIFT;
				_y_		= y++;
				_btx1_ 	= btxl;
				_btx2_   = btxr;
				_bty1_	= btyl;
				_bty2_	= btyr;
				_xshift_	= tex->xshift;
				_tex_		= tex->bmp;

				_THLINE;

/*
				if (_x1_ > _x2_) {
					iV_SWAP(_x1_,_x2_);
					iV_SWAP(_btx1_,_btx2_);
					iV_SWAP(_bty1_,_bty2_);
				}

				 __dx = _x2_- _x1_;
				 __du = (((_btx2_-_btx1_)+iV_DIVMULTP)>>iV_DIVSHIFT) * _iVPRIM_DIVTABLE[__dx];
				 __dv = (((_bty2_-_bty1_)+iV_DIVMULTP)>>iV_DIVSHIFT) * _iVPRIM_DIVTABLE[__dx];
				 __pp = psRendSurface->buffer + psRendSurface->scantable[_y_] + _x1_;

			 	 for (__x = __dx; __x>0; __x--, __pp++) {
				 	__col = *((uint8 *)_tex_ + ((_bty1_ >> iV_DIVSHIFT)<<_xshift_)+(_btx1_>>iV_DIVSHIFT));
				 	*__pp = __col;
				 	_btx1_ += __du;
				 	_bty1_ += __dv;
				 }
*/

/*
				_thline_4101(bxl >> iV_DIVSHIFT,bxr >> iV_DIVSHIFT,y++,btxl,btxr,
						btyl,btyr,tex->xshift,tex->bmp);
*/
			}

			if (--npoints==0) return;

			dxl = nvrt[li-1].x - nvrt[li].x;
			dyl = nvrt[li-1].y - nvrt[li].y;
			gradl = _iVPRIM_DIVTABLE[dyl] * dxl;
			bxl = ((int32)nvrt[li].x << iV_DIVSHIFT) + iV_DIVMULTP_2;

			dtx = nvrt[li-1].u - nvrt[li].u;
			dty = nvrt[li-1].v - nvrt[li].v;
			gradxl = _iVPRIM_DIVTABLE[dyl] * dtx;
			gradyl = _iVPRIM_DIVTABLE[dyl] * dty;
			btxl = ((int32)nvrt[li].u << iV_DIVSHIFT);
			btyl = ((int32)nvrt[li].v << iV_DIVSHIFT);

			li--;

		} else {
			dyl-=dyr;
			while (dyr-- > 0) {
				bxl += gradl;
				bxr += gradr;
				btxl += gradxl;
				btyl += gradyl;
				btxr += gradxr;
				btyr += gradyr;

				_x1_ 		= bxl >> iV_DIVSHIFT;
				_x2_ 		= bxr >> iV_DIVSHIFT;
				_y_		= y++;
				_btx1_ 	= btxl;
				_btx2_   = btxr;
				_bty1_	= btyl;
				_bty2_	= btyr;
				_xshift_	= tex->xshift;
				_tex_		= tex->bmp;

				_THLINE;
 /*

				if (_x1_ > _x2_) {
					iV_SWAP(_x1_,_x2_);
					iV_SWAP(_btx1_,_btx2_);
					iV_SWAP(_bty1_,_bty2_);
				}

				 __dx = _x2_- _x1_;
				 __du = (((_btx2_-_btx1_)+iV_DIVMULTP)>>iV_DIVSHIFT) * _iVPRIM_DIVTABLE[__dx];
				 __dv = (((_bty2_-_bty1_)+iV_DIVMULTP)>>iV_DIVSHIFT) * _iVPRIM_DIVTABLE[__dx];
				 __pp = psRendSurface->buffer + psRendSurface->scantable[_y_] + _x1_;

			 	 for (__x = __dx; __x>0; __x--, __pp++) {
				 	__col = *((uint8 *)_tex_ + ((_bty1_ >> iV_DIVSHIFT)<<_xshift_)+(_btx1_>>iV_DIVSHIFT));
				 	*__pp = __col;
				 	_btx1_ += __du;
				 	_bty1_ += __dv;
				 }
*/

/*
				_thline_4101(bxl >> iV_DIVSHIFT,bxr >> iV_DIVSHIFT,y++,btxl,btxr,
						btyl,btyr,tex->xshift,tex->bmp);
*/
			}

			if (--npoints == 0) return;

			dxr = nvrt[ri+1].x - nvrt[ri].x;
			dyr = nvrt[ri+1].y - nvrt[ri].y;
			gradr = _iVPRIM_DIVTABLE[dyr] * dxr;
			bxr = ((int32)nvrt[ri].x << iV_DIVSHIFT) + iV_DIVMULTP_2;

			dtx = nvrt[ri+1].u - nvrt[ri].u;
			dty = nvrt[ri+1].v - nvrt[ri].v;
			gradxr = _iVPRIM_DIVTABLE[dyr] * dtx;
			gradyr = _iVPRIM_DIVTABLE[dyr] * dty;
			btxr = ((int32)nvrt[ri].u << iV_DIVSHIFT);
			btyr = ((int32)nvrt[ri].v << iV_DIVSHIFT);

			ri++;
		}
	}
}


//*************************************************************************
//*** plot transparent texture-mapped polygon (colour 0 is transparent)
//*
//* params	npoints	= number of points in poly
//* 							(max set by iV_POLY_MAX_POINTS in vid.h)
//*			vrt 		= list poly vertices (clockwise order)
//*			tex		= texture to apply
//*
//******

void _ttpolygon_4101(int npoints, iVertex *vrt, iTexture  *tex)

{
	static iVertex nvrt[iV_POLY_MAX_POINTS];
	int i, miny, li, ri, dxl, dyl, dxr, dyr, y;
	fixed gradl, gradr, gradxl, gradxr, gradyl, gradyr;
	fixed bxl, bxr;
	int32 dtx, dty;
	fixed btxl, btxr, btyl, btyr;

	USE_THLINE;



	for (miny = iV_DIVMULTP, i=0, li=0; i<npoints; i++) {
		if (vrt[i].y < miny) {
			miny = vrt[i].y;
			li = i;
		}
	}

	for (i=0; i<npoints; i++) {
		nvrt[i].x = vrt[li].x;
		nvrt[i].y = vrt[li].y;
		nvrt[i].u = vrt[li].u;
		nvrt[i].v = vrt[li].v;
		if (++li >= npoints) li = 0;
	}

	dxl = nvrt[npoints-1].x - nvrt[0].x;
	dyl = nvrt[npoints-1].y - nvrt[0].y;
	dxr = nvrt[1].x - nvrt[0].x;
	dyr = nvrt[1].y - nvrt[0].y;
	gradl = _iVPRIM_DIVTABLE[dyl] * dxl;
	gradr = _iVPRIM_DIVTABLE[dyr] * dxr;
	bxl = bxr = ((int32)nvrt[0].x << iV_DIVSHIFT) + iV_DIVMULTP_2;
	y = nvrt[0].y;

	dtx = nvrt[npoints-1].u - nvrt[0].u;
	dty = nvrt[npoints-1].v - nvrt[0].v;
	gradxl = _iVPRIM_DIVTABLE[dyl] * dtx;
	gradyl = _iVPRIM_DIVTABLE[dyl] * dty;
	dtx = nvrt[1].u - nvrt[0].u;
	dty = nvrt[1].v - nvrt[0].v;
	gradxr = _iVPRIM_DIVTABLE[dyr] * dtx;
	gradyr = _iVPRIM_DIVTABLE[dyr] * dty;
	btxl = btxr = ((int32)nvrt[0].u << iV_DIVSHIFT);
	btyl = btyr = ((int32)nvrt[0].v << iV_DIVSHIFT);

	li = npoints-1;
	ri = 1;
	npoints -= 1;


	for (;;) {
		if (dyl < dyr) {
			dyr -= dyl;
			while (dyl-- > 0) {
				bxl += gradl;
				bxr += gradr;
				btxl += gradxl;
				btyl += gradyl;
				btxr += gradxr;
				btyr += gradyr;

				_x1_ = bxl >> iV_DIVSHIFT;
				_x2_ = bxr >> iV_DIVSHIFT;
				_y_	= y++;
				_btx1_ = btxl;
				_btx2_ = btxr;
				_bty1_ = btyl;
				_bty2_ = btyr;
				_xshift_ = tex->xshift;
				_tex_ = tex->bmp;

				_TTHLINE;
/*
				_tthline_4101(bxl >> iV_DIVSHIFT,bxr >> iV_DIVSHIFT,y++,btxl,btxr,
						btyl,btyr,tex->xshift,tex->bmp);
*/
			}

			if (--npoints==0) return;

			dxl = nvrt[li-1].x - nvrt[li].x;
			dyl = nvrt[li-1].y - nvrt[li].y;
			gradl = _iVPRIM_DIVTABLE[dyl] * dxl;
			bxl = ((int32)nvrt[li].x << iV_DIVSHIFT) + iV_DIVMULTP_2;

			dtx = nvrt[li-1].u - nvrt[li].u;
			dty = nvrt[li-1].v - nvrt[li].v;
			gradxl = _iVPRIM_DIVTABLE[dyl] * dtx;
			gradyl = _iVPRIM_DIVTABLE[dyl] * dty;
			btxl = ((int32)nvrt[li].u << iV_DIVSHIFT);
			btyl = ((int32)nvrt[li].v << iV_DIVSHIFT);

			li--;

		} else {
			dyl-=dyr;
			while (dyr-- > 0) {
				bxl += gradl;
				bxr += gradr;
				btxl += gradxl;
				btyl += gradyl;
				btxr += gradxr;
				btyr += gradyr;

				_x1_ = bxl >> iV_DIVSHIFT;
				_x2_ = bxr >> iV_DIVSHIFT;
				_y_	= y++;
				_btx1_ = btxl;
				_btx2_ = btxr;
				_bty1_ = btyl;
				_bty2_ = btyr;
				_xshift_ = tex->xshift;
				_tex_ = tex->bmp;

				_TTHLINE;
/*
				_tthline_4101(bxl >> iV_DIVSHIFT,bxr >> iV_DIVSHIFT,y++,btxl,btxr,
						btyl,btyr,tex->xshift,tex->bmp);
*/
			}

			if (--npoints == 0) return;

			dxr = nvrt[ri+1].x - nvrt[ri].x;
			dyr = nvrt[ri+1].y - nvrt[ri].y;
			gradr = _iVPRIM_DIVTABLE[dyr] * dxr;
			bxr = ((int32)nvrt[ri].x << iV_DIVSHIFT) + iV_DIVMULTP_2;

			dtx = nvrt[ri+1].u - nvrt[ri].u;
			dty = nvrt[ri+1].v - nvrt[ri].v;
			gradxr = _iVPRIM_DIVTABLE[dyr] * dtx;
			gradyr = _iVPRIM_DIVTABLE[dyr] * dty;
			btxr = ((int32)nvrt[ri].u << iV_DIVSHIFT);
			btyr = ((int32)nvrt[ri].v << iV_DIVSHIFT);

			ri++;
		}
	}
}

//*************************************************************************
//*** plot transparent texture-mapped polygon (colour 0 is transparent)
//* (ANY WIDTH TEXTURE PAGE!)
//* params	npoints	= number of points in poly
//* 							(max set by iV_POLY_MAX_POINTS in vid.h)
//*			vrt 		= list poly vertices (clockwise order)
//*			tex		= texture to apply
//*
//******

void _ttwpolygon_4101(int npoints, iVertex *vrt, iTexture  *tex)

{
	static iVertex nvrt[iV_POLY_MAX_POINTS];
	int i, miny, li, ri, dxl, dyl, dxr, dyr, y;
	fixed gradl, gradr, gradxl, gradxr, gradyl, gradyr;
	fixed bxl, bxr;
	int32 dtx, dty;
	fixed btxl, btxr, btyl, btyr;

	USE_THLINE_ANYWIDTH;



	for (miny = iV_DIVMULTP, i=0, li=0; i<npoints; i++) {
		if (vrt[i].y < miny) {
			miny = vrt[i].y;
			li = i;
		}
	}

	for (i=0; i<npoints; i++) {
		nvrt[i].x = vrt[li].x;
		nvrt[i].y = vrt[li].y;
		nvrt[i].u = vrt[li].u;
		nvrt[i].v = vrt[li].v;
		if (++li >= npoints) li = 0;
	}

	dxl = nvrt[npoints-1].x - nvrt[0].x;
	dyl = nvrt[npoints-1].y - nvrt[0].y;
	dxr = nvrt[1].x - nvrt[0].x;
	dyr = nvrt[1].y - nvrt[0].y;
	gradl = _iVPRIM_DIVTABLE[dyl] * dxl;
	gradr = _iVPRIM_DIVTABLE[dyr] * dxr;
	bxl = bxr = ((int32)nvrt[0].x << iV_DIVSHIFT) + iV_DIVMULTP_2;
	y = nvrt[0].y;

	dtx = nvrt[npoints-1].u - nvrt[0].u;
	dty = nvrt[npoints-1].v - nvrt[0].v;
	gradxl = _iVPRIM_DIVTABLE[dyl] * dtx;
	gradyl = _iVPRIM_DIVTABLE[dyl] * dty;
	dtx = nvrt[1].u - nvrt[0].u;
	dty = nvrt[1].v - nvrt[0].v;
	gradxr = _iVPRIM_DIVTABLE[dyr] * dtx;
	gradyr = _iVPRIM_DIVTABLE[dyr] * dty;
	btxl = btxr = ((int32)nvrt[0].u << iV_DIVSHIFT);
	btyl = btyr = ((int32)nvrt[0].v << iV_DIVSHIFT);

	li = npoints-1;
	ri = 1;
	npoints -= 1;


	for (;;) {
		if (dyl < dyr) {
			dyr -= dyl;
			while (dyl-- > 0) {
				bxl += gradl;
				bxr += gradr;
				btxl += gradxl;
				btyl += gradyl;
				btxr += gradxr;
				btyr += gradyr;

				_x1_ = bxl >> iV_DIVSHIFT;
				_x2_ = bxr >> iV_DIVSHIFT;
				_y_	= y++;
				_btx1_ = btxl;
				_btx2_ = btxr;
				_bty1_ = btyl;
				_bty2_ = btyr;
				_xwidth_ = tex->width;
				_tex_ = tex->bmp;

				_TTHLINE_ANYWIDTH_A;
			}

			if (--npoints==0) return;

			dxl = nvrt[li-1].x - nvrt[li].x;
			dyl = nvrt[li-1].y - nvrt[li].y;
			gradl = _iVPRIM_DIVTABLE[dyl] * dxl;
			bxl = ((int32)nvrt[li].x << iV_DIVSHIFT) + iV_DIVMULTP_2;

			dtx = nvrt[li-1].u - nvrt[li].u;
			dty = nvrt[li-1].v - nvrt[li].v;
			gradxl = _iVPRIM_DIVTABLE[dyl] * dtx;
			gradyl = _iVPRIM_DIVTABLE[dyl] * dty;
			btxl = ((int32)nvrt[li].u << iV_DIVSHIFT);
			btyl = ((int32)nvrt[li].v << iV_DIVSHIFT);

			li--;

		} else {
			dyl-=dyr;
			while (dyr-- > 0) {
				bxl += gradl;
				bxr += gradr;
				btxl += gradxl;
				btyl += gradyl;
				btxr += gradxr;
				btyr += gradyr;

				_x1_ = bxl >> iV_DIVSHIFT;
				_x2_ = bxr >> iV_DIVSHIFT;
				_y_	= y++;
				_btx1_ = btxl;
				_btx2_ = btxr;
				_bty1_ = btyl;
				_bty2_ = btyr;
				_xwidth_ = tex->width;
				_tex_ = tex->bmp;

				_TTHLINE_ANYWIDTH_A;
			}

			if (--npoints == 0) return;

			dxr = nvrt[ri+1].x - nvrt[ri].x;
			dyr = nvrt[ri+1].y - nvrt[ri].y;
			gradr = _iVPRIM_DIVTABLE[dyr] * dxr;
			bxr = ((int32)nvrt[ri].x << iV_DIVSHIFT) + iV_DIVMULTP_2;

			dtx = nvrt[ri+1].u - nvrt[ri].u;
			dty = nvrt[ri+1].v - nvrt[ri].v;
			gradxr = _iVPRIM_DIVTABLE[dyr] * dtx;
			gradyr = _iVPRIM_DIVTABLE[dyr] * dty;
			btxr = ((int32)nvrt[ri].u << iV_DIVSHIFT);
			btyr = ((int32)nvrt[ri].v << iV_DIVSHIFT);

			ri++;
		}
	}
}

//**************************************************************************
//*** plot flat-shaded polygon
//*
//* params	npoints	= number of polygon vertices
//* 			vrt		= polygon vertices
//*
//******

void _fpolygon_4101(int npoints, iVertex *vrt, uint32 col)

{
	static iVertex nvrt[iV_POLY_MAX_POINTS];
	int i, miny, li, ri, dxl, dyl, dxr, dyr, y;
	fixed gradl, gradr, bxl, bxr;

	USE_HLINE;


	for (miny = iV_DIVMULTP, i=0, li=0; i<npoints; i++) {
		if (vrt[i].y < miny) {
			miny = vrt[i].y;
			li = i;
		}
	}

	for (i=0; i<npoints; i++) {
		nvrt[i].x = vrt[li].x;
		nvrt[i].y = vrt[li].y;
		if (++li >= npoints) li = 0;
	}

	dxl = nvrt[npoints-1].x - nvrt[0].x;
	dyl = nvrt[npoints-1].y - nvrt[0].y;
	dxr = nvrt[1].x - nvrt[0].x;
	dyr = nvrt[1].y - nvrt[0].y;
	gradl = _iVPRIM_DIVTABLE[dyl] * dxl;
	gradr = _iVPRIM_DIVTABLE[dyr] * dxr;
	bxl = bxr = ((int32)nvrt[0].x << iV_DIVSHIFT) + iV_DIVMULTP_2;
	y = nvrt[0].y;

	li = npoints-1;
	ri = 1;
	npoints -= 1;


	for (;;) {
		if (dyl < dyr) {
			dyr -= dyl;
			while (dyl-- > 0) {
				bxl += gradl;
				bxr += gradr;

				_x1_ = bxl >> iV_DIVSHIFT;
				_x2_ = bxr >> iV_DIVSHIFT;
				_y_ = y++;
				_col_ = col;

				_HLINE;
/*
				_hline_4101(bxl >> iV_DIVSHIFT,bxr >> iV_DIVSHIFT,y++,col);
*/
			}

			if (--npoints==0) return;

			dxl = nvrt[li-1].x - nvrt[li].x;
			dyl = nvrt[li-1].y - nvrt[li].y;
			gradl = _iVPRIM_DIVTABLE[dyl] * dxl;
			bxl = ((int32)nvrt[li].x << iV_DIVSHIFT) + iV_DIVMULTP_2;

			li--;

		} else {
			dyl-=dyr;
			while (dyr-- > 0) {
				bxl += gradl;
				bxr += gradr;

				_x1_ = bxl >> iV_DIVSHIFT;
				_x2_ = bxr >> iV_DIVSHIFT;
				_y_ = y++;
				_col_ = col;

				_HLINE;
/*
				_hline_4101(bxl >> iV_DIVSHIFT,bxr >> iV_DIVSHIFT,y++,col);
*/
			}

			if (--npoints == 0) return;

			dxr = nvrt[ri+1].x - nvrt[ri].x;
			dyr = nvrt[ri+1].y - nvrt[ri].y;
			gradr = _iVPRIM_DIVTABLE[dyr] * dxr;
			bxr = ((int32)nvrt[ri].x << iV_DIVSHIFT) + iV_DIVMULTP_2;

			ri++;
		}
	}
}


//*************************************************************************
//*** plot gouraud-shaded polygon
//*
//* params	npoints	= number of polygon vertices
//* 			vrt	 	= polygon vertices
//******

void _gpolygon_4101(int npoints, iVertex *vrt)

{
}


//*************************************************************************
//*** plot triangle (texture-mapped, gouraud-shaded, flat-shaded)
//*
//* params 	vrt = triangle vertices
//*
//******

void _triangle_4101(iVertex *vrt, iTexture *tex, uint32 type)

{
/*
	if (colour & iV_POLY_TEXT) {
		// _ttriangle_4101(vrt);
	} else if (colour & iV_POLY_FLAT) {
		_ftriangle_4101(vrt);
	} else if (colour & iV_POLY_GOUR) {
		_gtriangle_4101(vrt);
	}
*/
}

//*************************************************************************
//*** plot quad (texture-mapped, gouraud-shaded, flat-shaded)
//*
//* params 	vrt = triangle vertices
//*
//******

void _quad_4101(iVertex *vrt, iTexture *tex, uint32 type)

{
/*
	if (colour & iV_POLY_TEXT) {
		// _ttriangle_4101(vrt);
	} else if (colour & iV_POLY_FLAT) {
		_ftriangle_4101(vrt);
	} else if (colour & iV_POLY_GOUR) {
		_gtriangle_4101(vrt);
	}
*/
}

//*************************************************************************
//*** plot polygon (texture-mapped, gouraud-shaded, flat-shaded)
//*
//* params	npoints	= number of polygon vertices
//* 			vrt 		= polygon vertices
//******

void _polygon_4101(int npoints, iVertex *vrt, iTexture *tex, uint32 type)

{
/*
	if (colour & iV_POLY_TEXT) {
		_fpolygon_4101(npoints,vrt);
	} else if (colour & iV_POLY_FLAT) {
		_fpolygon_4101(npoints,vrt);
	} else if (colour & iV_POLY_GOUR) {
		_fpolygon_4101(npoints,vrt);
	}
*/
}

//*************************************************************************
//*** get bitmap from screen buffer
//*
//* params	bmp	= buffer to store bitmap
//*			x 		= screen x position
//*			y 		= screen y position
//*			w 		= bitmap width
//*			h 		= bitmap height
//*
//******

void _gbitmap_4101(iBitmap *bmp, int x, int y, int w, int h)

{
	int i, j, d, b, lineb_w, lined_w;
	uint8 *bp;
	uint32 *dp;
	iPointer dbmp;


	bp = (uint8 *) psRendSurface->buffer + x + psRendSurface->scantable[y];

	// width < 8, use get-byte mode

	if (w<8) {
		lineb_w = WIDTH_4101 - w;
		for (i=0; i<h; i++) {
			for (j=0; j<w; j++) {
				*bmp++ = *bp++;
			}
			bp += lineb_w;
		}

	// width >= 8, use get-doubleword mode

	} else {
		b = w & 0x3;
		d = w & 0xfffc;
		lineb_w = WIDTH_4101 - b;
		lined_w = (WIDTH_4101 - d)>>2;
		dp =  (uint32 *) bp;
 		dbmp.dp = (uint32 *) bmp;
		bp += d;
		d >>= 2;
		for (i=0; i<h; i++) {
			for (j=0; j<d; j++) {
				*dbmp.dp++ = *dp++;
			}

			// tidy edges

			if (b==3) {
				*dbmp.bp++ = *bp++;
				*dbmp.bp++ = *bp++;
				*dbmp.bp++ = *bp++;
				bp += lineb_w;
			} else if (b==2) {
				*dbmp.bp++ = *bp++;
				*dbmp.bp++ = *bp++;
				bp += lineb_w;
			} else {
				*dbmp.bp++ = *bp++;
				bp += lineb_w;
			}
			dp += lined_w;
		}
	}
}

//*************************************************************************
//*** plot bitmap colour 0 is tranparent
//*
//* params		bmp 	= pointer to bitmap
//*				x  	= bitmap screen x position
//*				y	 	= bitmap screen y position
//*				w 		= bitmap width
//*				h 		= bitmap height
//*				ow		= bitmap original width (when clipped) else == w
//******

void _tbitmap_4101(iBitmap *bmp, int x, int y, int w, int h, int ow)

{
	int i, j, lineb_w;
	uint8 *bp;
	uint8 a;

	lineb_w = WIDTH_4101 - w;

	bp = (uint8 *) psRendSurface->buffer + x + psRendSurface->scantable[y];

	for (i=0; i<h; i++) {
		for (j=0; j<w; j++) {
			a = *bmp++;
			if (a)
				*bp = a;
			bp++;
		}
		bmp += (ow - w);
		bp += lineb_w;
	}
}

//*************************************************************************
//*** plot bitmap shadow (polt colour 0 for every pixel >0 in bitmap)
//*
//* params		bmp 	= pointer to bitmap
//*				x  	= bitmap screen x position
//*				y	 	= bitmap screen y position
//*				w 		= bitmap width
//*				h 		= bitmap height
//*				ow		= bitmap original width (when clipped) else == w
//*
//******

void _sbitmap_4101(iBitmap *bmp, int x, int y, int w, int h, int ow)

{
	int i, j, lineb_w;
	uint8 *bp;
	uint8 a;

	lineb_w = WIDTH_4101 - w;

	bp = (uint8 *) psRendSurface->buffer + x + psRendSurface->scantable[y];

	for (i=0; i<h; i++) {
		for (j=0; j<w; j++) {
			a = *bmp++;
			if (a)
				*bp = 0;
			bp++;
		}
		bmp += (ow - w);
		bp += lineb_w;
	}
}

//*************************************************************************
//*** plot bitmap
//*
//* params		bmp 	= pointer to bitmap
//*				x  	= bitmap screen x position
//*				y	 	= bitmap screen y position
//*				w 		= bitmap width
//*				h 		= bitmap height
//*				ow		= bitmap original width (when clipped) else == w
//******

void _bitmap_4101(iBitmap *bmp, int x, int y, int w, int h, int ow)

{
	int i, j, d, b, lineb_w, lined_w;
	uint8 *bp;
	uint32 *dp;
	iPointer dbmp;


	bp = (uint8 *) psRendSurface->buffer + x + psRendSurface->scantable[y];

	// width < 8, use store-byte mode

	if (w<8) {
		lineb_w = WIDTH_4101 - w;
		for (i=0; i<h; i++) {
			for (j=0; j<w; j++) {
				*bp++ = *bmp++;
			}
			bmp += (ow - w);
			bp += lineb_w;
		}

	// width >= 8, use store-doubleword mode

	} else {
		b = w & 0x3;
		d = w & 0xfffc;
		lineb_w = WIDTH_4101 - b;
		lined_w = (WIDTH_4101 - d)>>2;
		dp =  (uint32 *) bp;
 		dbmp.dp = (uint32 *) bmp;
		bp += d;
		d >>= 2;
		for (i=0; i<h; i++) {
			for (j=0; j<d; j++) {
				*dp++ = *dbmp.dp++;
			}

			// tidy edges

			if (b==3) {
				*bp++ = *dbmp.bp++;
				*bp++ = *dbmp.bp++;
				*bp++ = *dbmp.bp++;
				bp += lineb_w;
			} else if (b==2) {
				*bp++ = *dbmp.bp++;
				*bp++ = *dbmp.bp++;
				bp += lineb_w;
			} else if (b==1) {
				*bp++ = *dbmp.bp++;
				bp += lineb_w;
			}
			dp += lined_w;
			dbmp.bp += (ow - w);
		}
	}
}

//*************************************************************************
//*** plot bitmap
//*
//* params		bmp 	= pointer to bitmap
//*				x  	= bitmap screen x position
//*				y	 	= bitmap screen y position
//*				w 		= bitmap width
//*				h 		= bitmap height
//*				ow		= bitmap original width (when clipped) else == w
//*				ColourIndex = Colour to use for all pixels.
//******

void _bitmapcolour_4101(iBitmap *bmp, int x, int y, int w, int h, int ow,int ColourIndex)
{
	int i, j, lineb_w;
	uint8 *bp;
//	uint32 *dp;
//	iPointer dbmp;


	bp = (uint8 *) psRendSurface->buffer + x + psRendSurface->scantable[y];

	lineb_w = WIDTH_4101 - w;
	for (i=0; i<h; i++) {
		for (j=0; j<w; j++) {
			*bp++ = ColourIndex;
		}
		bmp += (ow - w);
		bp += lineb_w;
	}
}

//*************************************************************************
//*** plot bitmap
//*
//* params		bmp 	= pointer to bitmap
//*				x  	= bitmap screen x position
//*				y	 	= bitmap screen y position
//*				w 		= bitmap width
//*				h 		= bitmap height
//*				ow		= bitmap original width (when clipped) else == w
//*				ColourIndex = Colour to use for non zero pixels.
//******
void _tbitmapcolour_4101(iBitmap *bmp, int x, int y, int w, int h, int ow,int ColourIndex)
{
	int i, j, lineb_w;
	uint8 *bp;
//	uint32 *dp;
//	iPointer dbmp;


	bp = (uint8 *) psRendSurface->buffer + x + psRendSurface->scantable[y];

	lineb_w = WIDTH_4101 - w;
	for (i=0; i<h; i++) {
		for (j=0; j<w; j++) {
			if(*bmp++) {
				*bp = ColourIndex;
			}
			bp++;
		}
		bmp += (ow - w);
		bp += lineb_w;
	}
}





//*************************************************************************
//*** plot bitmap rotated 90 DEG clockwise
//*
//* params		bmp 	= pointer to bitmap
//*				x  	= bitmap screen x position
//*				y	 	= bitmap screen y position
//*				w 		= bitmap width
//*				h 		= bitmap height
//*				ow		= bitmap original width (when clipped) else == w
//*
//******

void _bitmapr90_4101(iBitmap *bmp, int x, int y, int w, int h, int ow)

{
	int i, j;
	uint8 *srcbp, *bp;



	srcbp = bp = (uint8 *) psRendSurface->buffer + x + (w-1) + psRendSurface->scantable[y];

	for (j=0; j<h; j++) {

		bp = srcbp;

		for (i=0; i<w; i++) {
			*bp = *bmp++;
			bp += WIDTH_4101;
		}

		srcbp -= 1;
		bmp += (ow - w);
	}
}

//*************************************************************************
//*** plot bitmap rotated 270 DEG clockwise
//*
//* params		bmp 	= pointer to bitmap
//*				x  	= bitmap screen x position
//*				y	 	= bitmap screen y position
//*				w 		= bitmap width
//*				h 		= bitmap height
//*				ow		= bitmap original width (when clipped) else == w
//*
//******

void _bitmapr270_4101(iBitmap *bmp, int x, int y, int w, int h, int ow)

{
	int i, j;
	uint8 *srcbp, *bp;



	srcbp = bp = (uint8 *) psRendSurface->buffer + x + psRendSurface->scantable[y+h-1];

	for (j=0; j<h; j++) {

		bp = srcbp;

		for (i=0; i<w; i++) {
			*bp = *bmp++;
			bp -= WIDTH_4101;
		}

		srcbp++;
		bmp += (ow - w);
	}
}

//*************************************************************************
//*** plot bitmap rotated by 180 DEG clockwise
//*
//* params	bmp 	= pointer to bitmap
//* 			x   	= bitmap screen x position
//* 			y 		= bitmap screen y position
//* 			w 		= bitmap width
//* 			h 		= bitmap height
//* 			tw		= target width (>0)
//* 			th		= target height (>0)
//******

void _bitmapr180_4101(iBitmap *bmp, int x, int y, int w, int h, int ow)

{
	int i, j;
	uint8 *bp, *p;

	bp = (uint8 *) psRendSurface->buffer + x + w - 1  + psRendSurface->scantable[y+h-1];

	for (j=0; j<h; j++) {

		p = bp;

		for (i=0; i<w; i++)
			*p-- = *bmp++;

		bp -= WIDTH_4101;
		bmp += (ow - w);
	}
}

//*************************************************************************
//*** plot and resize bitmap
//*
//* params	bmp 	= pointer to bitmap
//* 			x   	= bitmap screen x position
//* 			y 		= bitmap screen y position
//* 			w 		= bitmap width
//* 			h 		= bitmap height
//* 			tw		= target width (>0)
//* 			th		= target height (>0)
//******

void _rbitmap_4101(iBitmap *bmp, int x, int y, int w, int h, int tw, int th)

{
	int i, j;
	fixed xacc, yacc, ix, iy;
	uint8 *bp, *bbmp, *p;

	bp = (uint8 *) psRendSurface->buffer + x + psRendSurface->scantable[y];

	xacc = (w<<BSHIFT)/tw;
	yacc = (h<<BSHIFT)/th;

	for (i=0, iy=0; i<th; iy += yacc, i++) {
		bbmp = bmp + (iy>>BSHIFT) * w;
		p = bp;

		for (j=0, ix=0; j<tw; ix += xacc, j++)
			*p++ = *(bbmp+(ix>>BSHIFT));

		bp += WIDTH_4101;
	}
}

//*************************************************************************
//*** plot and resize bitmap rotated by 90 DEG clockwise
//*
//* params	bmp 	= pointer to bitmap
//* 			x   	= bitmap screen x position
//* 			y 		= bitmap screen y position
//* 			w 		= bitmap width
//* 			h 		= bitmap height
//* 			tw		= target width (>0)
//* 			th		= target height (>0)
//******

void _rbitmapr90_4101(iBitmap *bmp, int x, int y, int w, int h, int tw, int th)

{
	int i, j;
	fixed xacc, yacc, ix, iy;
	uint8 *bp, *bbmp, *p;

	bp = (uint8 *) psRendSurface->buffer + x + (w-1) + psRendSurface->scantable[y];

	xacc = (w<<BSHIFT)/tw;
	yacc = (h<<BSHIFT)/th;

	for (i=0, iy=0; i<th; iy += yacc, i++) {
		bbmp = bmp + (iy>>BSHIFT) * w;
		p = bp;

		for (j=0, ix=0; j<tw; ix += xacc, j++) {
			*p = *(bbmp+(ix>>BSHIFT));
			p += WIDTH_4101;
		}
		bp--;
	}
}

//*************************************************************************
//*** plot and resize bitmap rotated by 180 DEG clockwise
//*
//* params	bmp 	= pointer to bitmap
//* 			x   	= bitmap screen x position
//* 			y 		= bitmap screen y position
//* 			w 		= bitmap width
//* 			h 		= bitmap height
//* 			tw		= target width (>0)
//* 			th		= target height (>0)
//******

void _rbitmapr180_4101(iBitmap *bmp, int x, int y, int w, int h, int tw, int th)

{
	int i, j;
	fixed xacc, yacc, ix, iy;
	uint8 *bp, *bbmp, *p;

	bp = (uint8 *) psRendSurface->buffer + x + w - 1  + psRendSurface->scantable[y+h-1];

	xacc = (w<<BSHIFT)/tw;
	yacc = (h<<BSHIFT)/th;

	for (i=0, iy=0; i<th; iy += yacc, i++) {
		bbmp = bmp + (iy>>BSHIFT) * w;
		p = bp;

		for (j=0, ix=0; j<tw; ix += xacc, j++)
			*p-- = *(bbmp+(ix>>BSHIFT));

		bp -= WIDTH_4101;
	}
}

//*************************************************************************
//*** plot and resize bitmap rotated by 270 DEG clockwise
//*
//* params	bmp 	= pointer to bitmap
//* 			x   	= bitmap screen x position
//* 			y 		= bitmap screen y position
//* 			w 		= bitmap width
//* 			h 		= bitmap height
//* 			tw		= target width (>0)
//* 			th		= target height (>0)
//******

void _rbitmapr270_4101(iBitmap *bmp, int x, int y, int w, int h, int tw, int th)

{
	int i, j;
	fixed xacc, yacc, ix, iy;
	uint8 *bp, *bbmp, *p;

	bp = (uint8 *) psRendSurface->buffer + x + psRendSurface->scantable[y+h-1];

	xacc = (w<<BSHIFT)/tw;
	yacc = (h<<BSHIFT)/th;

	for (i=0, iy=0; i<th; iy += yacc, i++) {
		bbmp = bmp + (iy>>BSHIFT) * w;
		p = bp;

		for (j=0, ix=0; j<tw; ix += xacc, j++) {
			*p = *(bbmp+(ix>>BSHIFT));
			p -= WIDTH_4101;
		}

		bp++;
	}
}

//*************************************************************************
//*** plot texture-mapped gouraud-shaded triangle
//*
//* params	vert 	= list triangle vertices (clockwise order)
//*			tex	= texture to apply
//*
//******

void _tgtriangle_4101(iVertex *vrt, iTexture  *tex)

{
	int i, miny, li, dxl, dyl, dxr, dyr, y;
	fixed gradl, gradr, gradxl, gradxr, gradyl, gradyr;
	fixed bxl, bxr;
	int32 dtx, dty;
	fixed btxl, btyl, btxr, btyr, dgl, dgr, bgl, bgr;
	iVertex nvrt[3];
  // 	return;

  //	if ( !pie_CLOCKWISE( vrt[0].x, vrt[0].y, vrt[1].x, vrt[1].y, vrt[2].x, vrt[2].y ) )
  //		return;

	for (miny = iV_DIVMULTP, i=0, li=0; i<3; i++) {
		if (vrt[i].y < miny) {
			miny = vrt[i].y;
			li = i;
		}
	}


	for (i=0; i<3; i++) {
		nvrt[i].x = vrt[li].x;
		nvrt[i].y = vrt[li].y;
		nvrt[i].u = vrt[li].u;
		nvrt[i].v = vrt[li].v;
		nvrt[i].g = vrt[li].g;
		if (++li >= 3) li = 0;
	}

	// scanline
	dxl = nvrt[2].x - nvrt[0].x;
	dyl = nvrt[2].y - nvrt[0].y;
	dxr = nvrt[1].x - nvrt[0].x;
	dyr = nvrt[1].y - nvrt[0].y;
	gradl = _iVPRIM_DIVTABLE[dyl] * dxl;
	gradr = _iVPRIM_DIVTABLE[dyr] * dxr;
	bxl = bxr = ((int32)nvrt[0].x << iV_DIVSHIFT) + iV_DIVMULTP_2;
	y = nvrt[0].y;


	// gouraud
	dgl = _iVPRIM_DIVTABLE[dyl] * (nvrt[2].g - nvrt[0].g);
	dgr = _iVPRIM_DIVTABLE[dyr] * (nvrt[1].g - nvrt[0].g);
	bgl = bgr = nvrt[0].g << iV_DIVSHIFT;

	// texture
	dtx = nvrt[2].u - nvrt[0].u;
	dty = nvrt[2].v - nvrt[0].v;
	gradxl = _iVPRIM_DIVTABLE[dyl] * dtx;
	gradyl = _iVPRIM_DIVTABLE[dyl] * dty;
	dtx = nvrt[1].u - nvrt[0].u;
	dty = nvrt[1].v - nvrt[0].v;
	gradxr = _iVPRIM_DIVTABLE[dyr] * dtx;
	gradyr = _iVPRIM_DIVTABLE[dyr] * dty;
	btxl = btxr = ((int32)nvrt[0].u << iV_DIVSHIFT);
	btyl = btyr = ((int32)nvrt[0].v << iV_DIVSHIFT);

	if (dyl < dyr) {
		while (dyl-- > 0) {
			bxl += gradl;
			bxr += gradr;
			btxl += gradxl;
			btyl += gradyl;
			btxr += gradxr;
			btyr += gradyr;

			bgl += dgl;
			bgr += dgr;

			if (bgl == bgr) {
				_tfhline_4101(bxl >> iV_DIVSHIFT,bxr >> iV_DIVSHIFT,y++,btxl,btxr,
							btyl,btyr,tex->xshift,tex->bmp,bgl>>iV_DIVSHIFT);
			} else {
				_tghline_4101(bxl >> iV_DIVSHIFT,bxr >> iV_DIVSHIFT,y++,btxl,btxr,
						btyl,btyr,bgl,bgr,tex->xshift,tex->bmp);
			}


		}


		dxl = nvrt[1].x - nvrt[2].x;
		dyl = nvrt[1].y - nvrt[2].y;
		gradl = _iVPRIM_DIVTABLE[dyl] * dxl;
		bxl = ((int32)nvrt[2].x << iV_DIVSHIFT) + iV_DIVMULTP_2;


		dgl = _iVPRIM_DIVTABLE[dyl] * (nvrt[1].g - nvrt[2].g);
		bgl = nvrt[2].g << iV_DIVSHIFT;



		dtx = nvrt[1].u - nvrt[2].u;
		dty = nvrt[1].v - nvrt[2].v;
		gradxl = _iVPRIM_DIVTABLE[dyl] * dtx;
		gradyl = _iVPRIM_DIVTABLE[dyl] * dty;
		btxl = ((int32)nvrt[2].u << iV_DIVSHIFT);
		btyl = ((int32)nvrt[2].v << iV_DIVSHIFT);

		while (dyl-- > 0) {
			bxl += gradl;
			bxr += gradr;
			btxl += gradxl;
			btyl += gradyl;
			btxr += gradxr;
			btyr += gradyr;

			bgl += dgl;
			bgr += dgr;

			if (bgl == bgr) {
				_tfhline_4101(bxl >> iV_DIVSHIFT,bxr >> iV_DIVSHIFT,y++,btxl,btxr,btyl,btyr,tex->xshift,tex->bmp,bgl>>iV_DIVSHIFT);
			} else {
				_tghline_4101(bxl >> iV_DIVSHIFT,bxr >> iV_DIVSHIFT,y++,btxl,btxr,
						btyl,btyr,bgl,bgr,tex->xshift,tex->bmp);
			}

		}
	} else {
		while (dyr-- > 0) {
			bxl += gradl;
			bxr += gradr;
			btxl += gradxl;
			btyl += gradyl;
			btxr += gradxr;
			btyr += gradyr;

			bgl += dgl;
			bgr += dgr;

			if (bgl == bgr) {
				_tfhline_4101(bxl >> iV_DIVSHIFT,bxr >> iV_DIVSHIFT,y++,btxl,btxr,
						btyl,btyr,tex->xshift,tex->bmp,bgl>>iV_DIVSHIFT);

			} else {
				_tghline_4101(bxl >> iV_DIVSHIFT,bxr >> iV_DIVSHIFT,y++,btxl,btxr,
						btyl,btyr,bgl,bgr,tex->xshift,tex->bmp);
			}


		}

		dxr = nvrt[2].x - nvrt[1].x;
		dyr = nvrt[2].y - nvrt[1].y;
		gradr = _iVPRIM_DIVTABLE[dyr] * dxr;
		dgr = _iVPRIM_DIVTABLE[dyr] * (nvrt[2].g - nvrt[1].g);
		bgr = nvrt[1].g << iV_DIVSHIFT;
		bxr = ((int32)nvrt[1].x << iV_DIVSHIFT) + iV_DIVMULTP_2;

		dtx = nvrt[2].u - nvrt[1].u;
		dty = nvrt[2].v - nvrt[1].v;
		gradxr = _iVPRIM_DIVTABLE[dyr] * dtx;
		gradyr = _iVPRIM_DIVTABLE[dyr] * dty;
		btxr = ((int32)nvrt[1].u << iV_DIVSHIFT);
		btyr = ((int32)nvrt[1].v << iV_DIVSHIFT);

		while (dyr-- > 0) {
			bxl += gradl;
			bxr += gradr;
			btxl += gradxl;
			btyl += gradyl;
			btxr += gradxr;
			btyr += gradyr;

			bgl += dgl;
			bgr += dgr;

			if (bgl == bgr) {
				_tfhline_4101(bxl >> iV_DIVSHIFT,bxr >> iV_DIVSHIFT,y++,btxl,btxr,
						btyl,btyr,tex->xshift,tex->bmp,bgl>>iV_DIVSHIFT);
			} else {
				_tghline_4101(bxl >> iV_DIVSHIFT,bxr >> iV_DIVSHIFT,y++,btxl,btxr,
						btyl,btyr,bgl,bgr,tex->xshift,tex->bmp);
			}

		}
	}
}

//*************************************************************************
//*** plot texture-mapped gouraud-shaded horizontal line (linear mapping)
//*
//* params	x1 							= scanline left screen x
//*			x2 							= scanline right screen x
//*			btx1, btx2, bty1, bty2 	= tex coefficients shifted by iV_DIVSHIFT
//*			bg1, bg2						= gouraud shading coefficients shifted by iV_DIVSHIFT
//*			xshift						= width of bitmap as a shift
//*			tex							= bitmap
//******

static void _tghline_4101(int x1, int x2, int y, fixed btx1, fixed btx2, fixed bty1, fixed bty2, fixed bg1, fixed bg2, int xshift, iBitmap *tex)

{
	int x, dx, col;
	fixed txs, tys, gs;
	uint8 *pp;


	if (x1 > x2) {
		iV_SWAP(x1,x2);
		iV_SWAP(btx1,btx2);
		iV_SWAP(bty1,bty2);
		iV_SWAP(bg1, bg2);
	}

#ifdef ACCURATE
	// use this for higher accuracy if required
	// dx = (x2 - x1)+1 (+1) avoids test for when dx == 0
	dx = (x2 - x1)+1; // if no iV_SWAP this fails when dx == -1 !
	txs = (((btx2-btx1))) / dx;
	tys = (bty2-bty1) /dx;
	gs = (bg2 - bg1) / dx;
#else
	dx = x2-x1;
	// add 1 (DIVMULTP) instead of .5 (saves adding .5 to big_tx in poly
	txs = (((btx2-btx1)+iV_DIVMULTP)>>iV_DIVSHIFT) * _iVPRIM_DIVTABLE[dx];
	tys = (((bty2-bty1)+iV_DIVMULTP)>>iV_DIVSHIFT) * _iVPRIM_DIVTABLE[dx];
	gs = ((bg2 - bg1) >> iV_DIVSHIFT) * _iVPRIM_DIVTABLE[dx];
#endif


	pp = psRendSurface->buffer + psRendSurface->scantable[y] + x1;

	//for (x = x1; x < x2; x++, pp++) {
	for (x = dx; x >0; x--, pp++)
	{
		col = *((uint8 *)tex+((bty1 >> iV_DIVSHIFT)<<xshift)+(btx1>>iV_DIVSHIFT));

		/* Be sure and remove this */
		if (bg1 < 0)
		{
			bg1 = 0;
		}
	  	/* Remove */

//		col = iV_PALETTE_SHADE(col,(bg1 >> iV_DIVSHIFT));
		col = palShades[col*PALETTE_SHADE_LEVEL+(bg1>>iV_DIVSHIFT)];
		*pp = col;

		btx1 += txs;
		bty1 += tys;
		bg1 += gs;
	}
}



//*************************************************************************
//*** plot texture-mapped flat-shaded horizontal line (linear mapping)
//*
//* params	x1 							= scanline left screen x
//*			x2 							= scanline right screen x
//*			btx1, btx2, bty1, bty2 	= tex coefficients shifted by iV_DIVSHIFT
//*			xshift						= width of bitmap as a shift
//*			tex							= bitmap
//*			shade							= colour shade (0..iV_PALETTE_SHADE_LEVEL-1)
//******

static void _tfhline_4101(int x1, int x2, int y, fixed btx1, fixed btx2, fixed bty1, fixed bty2, int xshift, iBitmap *tex, int shade)

{
	int x, dx, col;
	fixed txs, tys;
	uint8 *pp;


	if (x1 > x2) {
		iV_SWAP(x1,x2);
		iV_SWAP(btx1,btx2);
		iV_SWAP(bty1,bty2);
	}

#ifdef ACCURATE
	// use this for higher accuracy if required
	// dx = (x2 - x1)+1 (+1) avoids test for when dx == 0
	dx = (x2 - x1)+1; // if no iV_SWAP this fails when dx == -1 !
	txs = (((btx2-btx1))) / dx;
	tys = (((bty2-bty1))) /dx;
#else
	dx = x2-x1;
	// add 1 (DIVMULTP) instead of .5 (saves adding .5 to big_tx in poly
	txs = (((btx2-btx1)+iV_DIVMULTP)>>iV_DIVSHIFT) * _iVPRIM_DIVTABLE[dx];
	tys = (((bty2-bty1)+iV_DIVMULTP)>>iV_DIVSHIFT) * _iVPRIM_DIVTABLE[dx];
#endif

	pp = psRendSurface->buffer + psRendSurface->scantable[y] + x1;

	//for (x = x1; x < x2; x++, pp++) {
	for (x = dx; x >0; x--, pp++) {
		col = *((uint8 *)tex+((bty1 >> iV_DIVSHIFT)<<xshift)+(btx1>>iV_DIVSHIFT));
//		col = iV_PALETTE_SHADE(col,shade);
		col = palShades[col*PALETTE_SHADE_LEVEL+shade];
//		col = transLookup[col][*pp];
		*pp = col;

		btx1 += txs;
		bty1 += tys;
	}
}

//*************************************************************************
//*** plot texture-mapped flat-shaded transparent horizontal line (linear mapping)
//*	Alex
//* params	x1 							= scanline left screen x
//*			x2 							= scanline right screen x
//*			btx1, btx2, bty1, bty2 	= tex coefficients shifted by iV_DIVSHIFT
//*			xshift						= width of bitmap as a shift
//*			tex							= bitmap
//*			shade							= colour shade (0..iV_PALETTE_SHADE_LEVEL-1)
//******

static void _ttfhline_4101(int x1, int x2, int y, fixed btx1, fixed btx2, fixed bty1, fixed bty2, int xshift, iBitmap *tex, int shade)

{
	int x, dx, col;
	fixed txs, tys;
	uint8 *pp;


	if (x1 > x2) {
		iV_SWAP(x1,x2);
		iV_SWAP(btx1,btx2);
		iV_SWAP(bty1,bty2);
	}

#ifdef ACCURATE
	// use this for higher accuracy if required
	// dx = (x2 - x1)+1 (+1) avoids test for when dx == 0
	dx = (x2 - x1)+1; // if no iV_SWAP this fails when dx == -1 !
	txs = (((btx2-btx1))) / dx;
	tys = (((bty2-bty1))) /dx;
#else
	dx = x2-x1;
	// add 1 (DIVMULTP) instead of .5 (saves adding .5 to big_tx in poly
	txs = (((btx2-btx1)+iV_DIVMULTP)>>iV_DIVSHIFT) * _iVPRIM_DIVTABLE[dx];
	tys = (((bty2-bty1)+iV_DIVMULTP)>>iV_DIVSHIFT) * _iVPRIM_DIVTABLE[dx];
#endif

	pp = psRendSurface->buffer + psRendSurface->scantable[y] + x1;

	//for (x = x1; x < x2; x++, pp++) {
	for (x = dx; x >0; x--, pp++) {
		col = *((uint8 *)tex+((bty1 >> iV_DIVSHIFT)<<xshift)+(btx1>>iV_DIVSHIFT));
//		col = iV_PALETTE_SHADE(col,shade);
		col = palShades[col*PALETTE_SHADE_LEVEL+shade];
		if(col)
		{
			*pp = col;
		}

		btx1 += txs;
		bty1 += tys;
	}
}

//*************************************************************************
/* Draw a textured gouraud shaded polygon */
/* Adapted from Sam's straight textured polygon draw */
/* Alex's version - with comments and everything!, July, 1997 */
void _tgpolygon_4101(int npoints, iVertex *vrt, iTexture  *tex)

{
	static iVertex nvrt[iV_POLY_MAX_POINTS];
	int i, miny, li, ri, dxl, dyl, dxr, dyr, y;
	fixed gradl, gradr, gradxl, gradxr, gradyl, gradyr;
	fixed bxl, bxr;
	int32 dtx, dty;
	fixed btxl, btxr, btyl, btyr;
	/* G variables are gradient vars - I are illumination vars */
	fixed gourGLeft, gourGRight, gourILeft, gourIRight;

   //	return;
	/*	We can assume that there's more than 3 sides as this routine will only
		be called if the clipper returns sides > 3. Therefore we can reject the drawing
		of any polygons if the first 3 sided sub-part (a triangle) is anticlockwise.
		This is vrt[0..3]. There's no worries indexing up to 2 as we know the shape is
		bigger than a triangle */

   //	if ( !pie_CLOCKWISE( vrt[0].x, vrt[0].y, vrt[1].x, vrt[1].y, vrt[2].x, vrt[2].y ) )
  //	{
  //		return;
  //	}

	for (miny = iV_DIVMULTP, i=0, li=0; i<npoints; i++) {
		if (vrt[i].y < miny) {
			miny = vrt[i].y;
			li = i;
		}
	}

	/* Take a copy of the vertice information - ask Sam why? */
	for (i=0; i<npoints; i++) {
		nvrt[i].x = vrt[li].x;
		nvrt[i].y = vrt[li].y;
		nvrt[i].u = vrt[li].u;
		nvrt[i].v = vrt[li].v;
		nvrt[i].g = vrt[li].g;
		if (++li >= npoints) li = 0;
	}

	/* Set up increments, gradients */
	dxl = nvrt[npoints-1].x - nvrt[0].x;
	dyl = nvrt[npoints-1].y - nvrt[0].y;
	dxr = nvrt[1].x - nvrt[0].x;
	dyr = nvrt[1].y - nvrt[0].y;
	gradl = _iVPRIM_DIVTABLE[dyl] * dxl;
	gradr = _iVPRIM_DIVTABLE[dyr] * dxr;
	bxl = bxr = ((int32)nvrt[0].x << iV_DIVSHIFT) + iV_DIVMULTP_2;
	y = nvrt[0].y;

	//---------------------------------------------------------------------------
	/* Set up the gouraud variables */
	gourGLeft = _iVPRIM_DIVTABLE[dyl] * (nvrt[npoints-1].g - nvrt[0].g);
	gourGRight = _iVPRIM_DIVTABLE[dyr] * (nvrt[1].g - nvrt[0].g);
	/* Initial illumination values */
	gourILeft = gourIRight = nvrt[0].g << iV_DIVSHIFT;
	//---------------------------------------------------------------------------

	dtx = nvrt[npoints-1].u - nvrt[0].u;
	dty = nvrt[npoints-1].v - nvrt[0].v;
	gradxl = _iVPRIM_DIVTABLE[dyl] * dtx;
	gradyl = _iVPRIM_DIVTABLE[dyl] * dty;
	dtx = nvrt[1].u - nvrt[0].u;
	dty = nvrt[1].v - nvrt[0].v;
	gradxr = _iVPRIM_DIVTABLE[dyr] * dtx;
	gradyr = _iVPRIM_DIVTABLE[dyr] * dty;
	btxl = btxr = ((int32)nvrt[0].u << iV_DIVSHIFT);
	btyl = btyr = ((int32)nvrt[0].v << iV_DIVSHIFT);

	li = npoints-1;
	ri = 1;
	npoints -= 1;


	for (;;) {
		if (dyl < dyr) {
			dyr -= dyl;
			while (dyl-- > 0) {
 				/* We're going down the left hand side of the triangle */
				bxl += gradl;
				bxr += gradr;
				btxl += gradxl;
				btyl += gradyl;
				btxr += gradxr;
				btyr += gradyr;
	//---------------------------------------------------------------------------
				gourILeft += gourGLeft;
				gourIRight += gourGRight;
	//---------------------------------------------------------------------------

				/* Flat shade the scan line segment if light levels at edge are the same */
				if (gourILeft == gourIRight)
				{
					_tfhline_4101(bxl >> iV_DIVSHIFT,bxr >> iV_DIVSHIFT,y++,btxl,btxr,
							btyl,btyr,tex->xshift,tex->bmp,gourILeft>>iV_DIVSHIFT);
				}
				/* Otherwise there's disparate values for illumination - so gouraud shade */
				else
				{
					_tghline_4101(bxl >> iV_DIVSHIFT,bxr >> iV_DIVSHIFT,y++,btxl,btxr,
							btyl,btyr,gourILeft,gourIRight,tex->xshift,tex->bmp);
				}
			}

			/* Have we finished */
			if (--npoints==0) return;

			dxl = nvrt[li-1].x - nvrt[li].x;
			dyl = nvrt[li-1].y - nvrt[li].y;
			gradl = _iVPRIM_DIVTABLE[dyl] * dxl;
			bxl = ((int32)nvrt[li].x << iV_DIVSHIFT) + iV_DIVMULTP_2;

			dtx = nvrt[li-1].u - nvrt[li].u;
			dty = nvrt[li-1].v - nvrt[li].v;
			gradxl = _iVPRIM_DIVTABLE[dyl] * dtx;
			gradyl = _iVPRIM_DIVTABLE[dyl] * dty;
			btxl = ((int32)nvrt[li].u << iV_DIVSHIFT);
			btyl = ((int32)nvrt[li].v << iV_DIVSHIFT);
	//---------------------------------------------------------------------------
			gourGLeft = _iVPRIM_DIVTABLE[dyl] * (nvrt[li-1].g - nvrt[li].g);
			gourILeft = nvrt[li].g << iV_DIVSHIFT;
	//---------------------------------------------------------------------------

			li--;

		} else {
			dyl-=dyr;
			while (dyr-- > 0) {
				bxl += gradl;
				bxr += gradr;
				btxl += gradxl;
				btyl += gradyl;
				btxr += gradxr;
				btyr += gradyr;
	//---------------------------------------------------------------------------
				gourILeft += gourGLeft;
				gourIRight += gourGRight;
	//---------------------------------------------------------------------------

				/* See above */
				if (gourILeft == gourIRight)
				{
					_tfhline_4101(bxl >> iV_DIVSHIFT,bxr >> iV_DIVSHIFT,y++,btxl,btxr,
							btyl,btyr,tex->xshift,tex->bmp,gourILeft>>iV_DIVSHIFT);
				}
				else
				{
					_tghline_4101(bxl >> iV_DIVSHIFT,bxr >> iV_DIVSHIFT,y++,btxl,btxr,
						btyl,btyr,gourILeft,gourIRight,tex->xshift,tex->bmp);
				}
		}

			if (--npoints == 0) return;

			dxr = nvrt[ri+1].x - nvrt[ri].x;
			dyr = nvrt[ri+1].y - nvrt[ri].y;
			gradr = _iVPRIM_DIVTABLE[dyr] * dxr;
			bxr = ((int32)nvrt[ri].x << iV_DIVSHIFT) + iV_DIVMULTP_2;

			dtx = nvrt[ri+1].u - nvrt[ri].u;
			dty = nvrt[ri+1].v - nvrt[ri].v;
			gradxr = _iVPRIM_DIVTABLE[dyr] * dtx;
			gradyr = _iVPRIM_DIVTABLE[dyr] * dty;
			btxr = ((int32)nvrt[ri].u << iV_DIVSHIFT);
			btyr = ((int32)nvrt[ri].v << iV_DIVSHIFT);
	//---------------------------------------------------------------------------
			gourGRight = _iVPRIM_DIVTABLE[dyr] * (nvrt[ri+1].g - nvrt[ri].g);
			gourIRight = nvrt[ri].g << iV_DIVSHIFT;
	//---------------------------------------------------------------------------
			ri++;
		}
	}
}


//*************************************************************************
/* Draw a textured level shaded polygon */
/* Adapted from Sam's straight textured polygon draw */
/* Alex's version - with comments and everything!, July, 1997 */
void _tspolygon_4101(int npoints, iVertex *vrt, iTexture  *tex, int brightness)

{
	static iVertex nvrt[iV_POLY_MAX_POINTS];
	int i, miny, li, ri, dxl, dyl, dxr, dyr, y;
	fixed gradl, gradr, gradxl, gradxr, gradyl, gradyr;
	fixed bxl, bxr;
	int32 dtx, dty;
	fixed btxl, btxr, btyl, btyr;

	/*	We can assume that there's more than 3 sides as this routine will only
		be called if the clipper returns sides > 3. Therefore we can reject the drawing
		of any polygons if the first 3 sided sub-part (a triangle) is anticlockwise.
		This is vrt[0..3]. There's no worries indexing up to 2 as we know the shape is
		bigger than a triangle */

	for (miny = iV_DIVMULTP, i=0, li=0; i<npoints; i++) {
		if (vrt[i].y < miny) {
			miny = vrt[i].y;
			li = i;
		}
	}

	/* Take a copy of the vertice information - ask Sam why? */
	for (i=0; i<npoints; i++) {
		nvrt[i].x = vrt[li].x;
		nvrt[i].y = vrt[li].y;
		nvrt[i].u = vrt[li].u;
		nvrt[i].v = vrt[li].v;
		nvrt[i].g = vrt[li].g;
		if (++li >= npoints) li = 0;
	}

	/* Set up increments, gradients */
	dxl = nvrt[npoints-1].x - nvrt[0].x;
	dyl = nvrt[npoints-1].y - nvrt[0].y;
	dxr = nvrt[1].x - nvrt[0].x;
	dyr = nvrt[1].y - nvrt[0].y;
	gradl = _iVPRIM_DIVTABLE[dyl] * dxl;
	gradr = _iVPRIM_DIVTABLE[dyr] * dxr;
	bxl = bxr = ((int32)nvrt[0].x << iV_DIVSHIFT) + iV_DIVMULTP_2;
	y = nvrt[0].y;

	//---------------------------------------------------------------------------

	dtx = nvrt[npoints-1].u - nvrt[0].u;
	dty = nvrt[npoints-1].v - nvrt[0].v;
	gradxl = _iVPRIM_DIVTABLE[dyl] * dtx;
	gradyl = _iVPRIM_DIVTABLE[dyl] * dty;
	dtx = nvrt[1].u - nvrt[0].u;
	dty = nvrt[1].v - nvrt[0].v;
	gradxr = _iVPRIM_DIVTABLE[dyr] * dtx;
	gradyr = _iVPRIM_DIVTABLE[dyr] * dty;
	btxl = btxr = ((int32)nvrt[0].u << iV_DIVSHIFT);
	btyl = btyr = ((int32)nvrt[0].v << iV_DIVSHIFT);

	li = npoints-1;
	ri = 1;
	npoints -= 1;


	for (;;) {
		if (dyl < dyr) {
			dyr -= dyl;
			while (dyl-- > 0) {
 				/* We're going down the left hand side of the triangle */
				bxl += gradl;
				bxr += gradr;
				btxl += gradxl;
				btyl += gradyl;
				btxr += gradxr;
				btyr += gradyr;

					_tfhline_4101(bxl >> iV_DIVSHIFT,bxr >> iV_DIVSHIFT,y++,btxl,btxr,
							btyl,btyr,tex->xshift,tex->bmp,brightness);
			}

			/* Have we finished */
			if (--npoints==0) return;

			dxl = nvrt[li-1].x - nvrt[li].x;
			dyl = nvrt[li-1].y - nvrt[li].y;
			gradl = _iVPRIM_DIVTABLE[dyl] * dxl;
			bxl = ((int32)nvrt[li].x << iV_DIVSHIFT) + iV_DIVMULTP_2;

			dtx = nvrt[li-1].u - nvrt[li].u;
			dty = nvrt[li-1].v - nvrt[li].v;
			gradxl = _iVPRIM_DIVTABLE[dyl] * dtx;
			gradyl = _iVPRIM_DIVTABLE[dyl] * dty;
			btxl = ((int32)nvrt[li].u << iV_DIVSHIFT);
			btyl = ((int32)nvrt[li].v << iV_DIVSHIFT);
			li--;

		} else {
			dyl-=dyr;
			while (dyr-- > 0) {
				bxl += gradl;
				bxr += gradr;
				btxl += gradxl;
				btyl += gradyl;
				btxr += gradxr;
				btyr += gradyr;

					_tfhline_4101(bxl >> iV_DIVSHIFT,bxr >> iV_DIVSHIFT,y++,btxl,btxr,
							btyl,btyr,tex->xshift,tex->bmp,brightness);
		}

			if (--npoints == 0) return;

			dxr = nvrt[ri+1].x - nvrt[ri].x;
			dyr = nvrt[ri+1].y - nvrt[ri].y;
			gradr = _iVPRIM_DIVTABLE[dyr] * dxr;
			bxr = ((int32)nvrt[ri].x << iV_DIVSHIFT) + iV_DIVMULTP_2;

			dtx = nvrt[ri+1].u - nvrt[ri].u;
			dty = nvrt[ri+1].v - nvrt[ri].v;
			gradxr = _iVPRIM_DIVTABLE[dyr] * dtx;
			gradyr = _iVPRIM_DIVTABLE[dyr] * dty;
			btxr = ((int32)nvrt[ri].u << iV_DIVSHIFT);
			btyr = ((int32)nvrt[ri].v << iV_DIVSHIFT);
			ri++;
		}
	}
}


//*************************************************************************
//*** plot texture-mapped level-shaded triangle
//*
//* params	vert 	= list triangle vertices (clockwise order)
//*			tex	= texture to apply
//*
//******

void _tstriangle_4101(iVertex *vrt, iTexture  *tex, int brightness)

{
	int i, miny, li, dxl, dyl, dxr, dyr, y;
	fixed gradl, gradr, gradxl, gradxr, gradyl, gradyr;
	fixed bxl, bxr;
	int32 dtx, dty;
	fixed btxl, btyl, btxr, btyr;
	iVertex nvrt[3];

	for (miny = iV_DIVMULTP, i=0, li=0; i<3; i++) {
		if (vrt[i].y < miny) {
			miny = vrt[i].y;
			li = i;
		}
	}

	for (i=0; i<3; i++) {
		nvrt[i].x = vrt[li].x;
		nvrt[i].y = vrt[li].y;
		nvrt[i].u = vrt[li].u;
		nvrt[i].v = vrt[li].v;
		nvrt[i].g = vrt[li].g;
		if (++li >= 3) li = 0;
	}

	// scanline
	dxl = nvrt[2].x - nvrt[0].x;
	dyl = nvrt[2].y - nvrt[0].y;
	dxr = nvrt[1].x - nvrt[0].x;
	dyr = nvrt[1].y - nvrt[0].y;
	gradl = _iVPRIM_DIVTABLE[dyl] * dxl;
	gradr = _iVPRIM_DIVTABLE[dyr] * dxr;
	bxl = bxr = ((int32)nvrt[0].x << iV_DIVSHIFT) + iV_DIVMULTP_2;
	y = nvrt[0].y;

	// texture
	dtx = nvrt[2].u - nvrt[0].u;
	dty = nvrt[2].v - nvrt[0].v;
	gradxl = _iVPRIM_DIVTABLE[dyl] * dtx;
	gradyl = _iVPRIM_DIVTABLE[dyl] * dty;
	dtx = nvrt[1].u - nvrt[0].u;
	dty = nvrt[1].v - nvrt[0].v;
	gradxr = _iVPRIM_DIVTABLE[dyr] * dtx;
	gradyr = _iVPRIM_DIVTABLE[dyr] * dty;
	btxl = btxr = ((int32)nvrt[0].u << iV_DIVSHIFT);
	btyl = btyr = ((int32)nvrt[0].v << iV_DIVSHIFT);

	if (dyl < dyr) {
		while (dyl-- > 0) {
			bxl += gradl;
			bxr += gradr;
			btxl += gradxl;
			btyl += gradyl;
			btxr += gradxr;
			btyr += gradyr;

			_tfhline_4101(bxl >> iV_DIVSHIFT,bxr >> iV_DIVSHIFT,y++,btxl,btxr,
							btyl,btyr,tex->xshift,tex->bmp,brightness);

		}


		dxl = nvrt[1].x - nvrt[2].x;
		dyl = nvrt[1].y - nvrt[2].y;
		gradl = _iVPRIM_DIVTABLE[dyl] * dxl;
		bxl = ((int32)nvrt[2].x << iV_DIVSHIFT) + iV_DIVMULTP_2;

		dtx = nvrt[1].u - nvrt[2].u;
		dty = nvrt[1].v - nvrt[2].v;
		gradxl = _iVPRIM_DIVTABLE[dyl] * dtx;
		gradyl = _iVPRIM_DIVTABLE[dyl] * dty;
		btxl = ((int32)nvrt[2].u << iV_DIVSHIFT);
		btyl = ((int32)nvrt[2].v << iV_DIVSHIFT);

		while (dyl-- > 0) {
			bxl += gradl;
			bxr += gradr;
			btxl += gradxl;
			btyl += gradyl;
			btxr += gradxr;
			btyr += gradyr;

				_tfhline_4101(bxl >> iV_DIVSHIFT,bxr >> iV_DIVSHIFT,y++,btxl,btxr,btyl,btyr,tex->xshift,tex->bmp,brightness);
		}
	} else {
		while (dyr-- > 0) {
			bxl += gradl;
			bxr += gradr;
			btxl += gradxl;
			btyl += gradyl;
			btxr += gradxr;
			btyr += gradyr;

				_tfhline_4101(bxl >> iV_DIVSHIFT,bxr >> iV_DIVSHIFT,y++,btxl,btxr,
						btyl,btyr,tex->xshift,tex->bmp,brightness);
		}

		dxr = nvrt[2].x - nvrt[1].x;
		dyr = nvrt[2].y - nvrt[1].y;
		gradr = _iVPRIM_DIVTABLE[dyr] * dxr;
		bxr = ((int32)nvrt[1].x << iV_DIVSHIFT) + iV_DIVMULTP_2;

		dtx = nvrt[2].u - nvrt[1].u;
		dty = nvrt[2].v - nvrt[1].v;
		gradxr = _iVPRIM_DIVTABLE[dyr] * dtx;
		gradyr = _iVPRIM_DIVTABLE[dyr] * dty;
		btxr = ((int32)nvrt[1].u << iV_DIVSHIFT);
		btyr = ((int32)nvrt[1].v << iV_DIVSHIFT);

		while (dyr-- > 0) {
			bxl += gradl;
			bxr += gradr;
			btxl += gradxl;
			btyl += gradyl;
			btxr += gradxr;
			btyr += gradyr;

				_tfhline_4101(bxl >> iV_DIVSHIFT,bxr >> iV_DIVSHIFT,y++,btxl,btxr,
						btyl,btyr,tex->xshift,tex->bmp,brightness);
		}
	}
}

//*************************************************************************
//*** plot texture-mapped level-shaded transparent triangle (0 is trans)
//*
//* params	vert 	= list triangle vertices (clockwise order)
//*			tex	= texture to apply
//*
//******

void _ttstriangle_4101(iVertex *vrt, iTexture  *tex, int brightness)

{
	int i, miny, li, dxl, dyl, dxr, dyr, y;
	fixed gradl, gradr, gradxl, gradxr, gradyl, gradyr;
	fixed bxl, bxr;
	int32 dtx, dty;
	fixed btxl, btyl, btxr, btyr;
	iVertex nvrt[3];

	for (miny = iV_DIVMULTP, i=0, li=0; i<3; i++) {
		if (vrt[i].y < miny) {
			miny = vrt[i].y;
			li = i;
		}
	}

	for (i=0; i<3; i++) {
		nvrt[i].x = vrt[li].x;
		nvrt[i].y = vrt[li].y;
		nvrt[i].u = vrt[li].u;
		nvrt[i].v = vrt[li].v;
		nvrt[i].g = vrt[li].g;
		if (++li >= 3) li = 0;
	}

	// scanline
	dxl = nvrt[2].x - nvrt[0].x;
	dyl = nvrt[2].y - nvrt[0].y;
	dxr = nvrt[1].x - nvrt[0].x;
	dyr = nvrt[1].y - nvrt[0].y;
	gradl = _iVPRIM_DIVTABLE[dyl] * dxl;
	gradr = _iVPRIM_DIVTABLE[dyr] * dxr;
	bxl = bxr = ((int32)nvrt[0].x << iV_DIVSHIFT) + iV_DIVMULTP_2;
	y = nvrt[0].y;

	// texture
	dtx = nvrt[2].u - nvrt[0].u;
	dty = nvrt[2].v - nvrt[0].v;
	gradxl = _iVPRIM_DIVTABLE[dyl] * dtx;
	gradyl = _iVPRIM_DIVTABLE[dyl] * dty;
	dtx = nvrt[1].u - nvrt[0].u;
	dty = nvrt[1].v - nvrt[0].v;
	gradxr = _iVPRIM_DIVTABLE[dyr] * dtx;
	gradyr = _iVPRIM_DIVTABLE[dyr] * dty;
	btxl = btxr = ((int32)nvrt[0].u << iV_DIVSHIFT);
	btyl = btyr = ((int32)nvrt[0].v << iV_DIVSHIFT);

	if (dyl < dyr) {
		while (dyl-- > 0) {
			bxl += gradl;
			bxr += gradr;
			btxl += gradxl;
			btyl += gradyl;
			btxr += gradxr;
			btyr += gradyr;

			_ttfhline_4101(bxl >> iV_DIVSHIFT,bxr >> iV_DIVSHIFT,y++,btxl,btxr,
							btyl,btyr,tex->xshift,tex->bmp,brightness);

		}


		dxl = nvrt[1].x - nvrt[2].x;
		dyl = nvrt[1].y - nvrt[2].y;
		gradl = _iVPRIM_DIVTABLE[dyl] * dxl;
		bxl = ((int32)nvrt[2].x << iV_DIVSHIFT) + iV_DIVMULTP_2;

		dtx = nvrt[1].u - nvrt[2].u;
		dty = nvrt[1].v - nvrt[2].v;
		gradxl = _iVPRIM_DIVTABLE[dyl] * dtx;
		gradyl = _iVPRIM_DIVTABLE[dyl] * dty;
		btxl = ((int32)nvrt[2].u << iV_DIVSHIFT);
		btyl = ((int32)nvrt[2].v << iV_DIVSHIFT);

		while (dyl-- > 0) {
			bxl += gradl;
			bxr += gradr;
			btxl += gradxl;
			btyl += gradyl;
			btxr += gradxr;
			btyr += gradyr;

			_ttfhline_4101(bxl >> iV_DIVSHIFT,bxr >> iV_DIVSHIFT,y++,btxl,btxr,btyl,btyr,tex->xshift,tex->bmp,brightness);
		}
	} else {
		while (dyr-- > 0) {
			bxl += gradl;
			bxr += gradr;
			btxl += gradxl;
			btyl += gradyl;
			btxr += gradxr;
			btyr += gradyr;

			_ttfhline_4101(bxl >> iV_DIVSHIFT,bxr >> iV_DIVSHIFT,y++,btxl,btxr,
						btyl,btyr,tex->xshift,tex->bmp,brightness);
		}

		dxr = nvrt[2].x - nvrt[1].x;
		dyr = nvrt[2].y - nvrt[1].y;
		gradr = _iVPRIM_DIVTABLE[dyr] * dxr;
		bxr = ((int32)nvrt[1].x << iV_DIVSHIFT) + iV_DIVMULTP_2;

		dtx = nvrt[2].u - nvrt[1].u;
		dty = nvrt[2].v - nvrt[1].v;
		gradxr = _iVPRIM_DIVTABLE[dyr] * dtx;
		gradyr = _iVPRIM_DIVTABLE[dyr] * dty;
		btxr = ((int32)nvrt[1].u << iV_DIVSHIFT);
		btyr = ((int32)nvrt[1].v << iV_DIVSHIFT);

		while (dyr-- > 0) {
			bxl += gradl;
			bxr += gradr;
			btxl += gradxl;
			btyl += gradyl;
			btxr += gradxr;
			btyr += gradyr;

			_ttfhline_4101(bxl >> iV_DIVSHIFT,bxr >> iV_DIVSHIFT,y++,btxl,btxr,
						btyl,btyr,tex->xshift,tex->bmp,brightness);
		}
	}
}

//*************************************************************************
/* Draw a textured level shaded transaprent polygon 0 is trans! */
void _ttspolygon_4101(int npoints, iVertex *vrt, iTexture  *tex, int brightness)

{
	static iVertex nvrt[iV_POLY_MAX_POINTS];
	int i, miny, li, ri, dxl, dyl, dxr, dyr, y;
	fixed gradl, gradr, gradxl, gradxr, gradyl, gradyr;
	fixed bxl, bxr;
	int32 dtx, dty;
	fixed btxl, btxr, btyl, btyr;

	/*	We can assume that there's more than 3 sides as this routine will only
		be called if the clipper returns sides > 3. Therefore we can reject the drawing
		of any polygons if the first 3 sided sub-part (a triangle) is anticlockwise.
		This is vrt[0..3]. There's no worries indexing up to 2 as we know the shape is
		bigger than a triangle */

	for (miny = iV_DIVMULTP, i=0, li=0; i<npoints; i++) {
		if (vrt[i].y < miny) {
			miny = vrt[i].y;
			li = i;
		}
	}

	/* Take a copy of the vertice information - ask Sam why? */
	for (i=0; i<npoints; i++) {
		nvrt[i].x = vrt[li].x;
		nvrt[i].y = vrt[li].y;
		nvrt[i].u = vrt[li].u;
		nvrt[i].v = vrt[li].v;
		nvrt[i].g = vrt[li].g;
		if (++li >= npoints) li = 0;
	}

	/* Set up increments, gradients */
	dxl = nvrt[npoints-1].x - nvrt[0].x;
	dyl = nvrt[npoints-1].y - nvrt[0].y;
	dxr = nvrt[1].x - nvrt[0].x;
	dyr = nvrt[1].y - nvrt[0].y;
	gradl = _iVPRIM_DIVTABLE[dyl] * dxl;
	gradr = _iVPRIM_DIVTABLE[dyr] * dxr;
	bxl = bxr = ((int32)nvrt[0].x << iV_DIVSHIFT) + iV_DIVMULTP_2;
	y = nvrt[0].y;

	//---------------------------------------------------------------------------

	dtx = nvrt[npoints-1].u - nvrt[0].u;
	dty = nvrt[npoints-1].v - nvrt[0].v;
	gradxl = _iVPRIM_DIVTABLE[dyl] * dtx;
	gradyl = _iVPRIM_DIVTABLE[dyl] * dty;
	dtx = nvrt[1].u - nvrt[0].u;
	dty = nvrt[1].v - nvrt[0].v;
	gradxr = _iVPRIM_DIVTABLE[dyr] * dtx;
	gradyr = _iVPRIM_DIVTABLE[dyr] * dty;
	btxl = btxr = ((int32)nvrt[0].u << iV_DIVSHIFT);
	btyl = btyr = ((int32)nvrt[0].v << iV_DIVSHIFT);

	li = npoints-1;
	ri = 1;
	npoints -= 1;


	for (;;) {
		if (dyl < dyr) {
			dyr -= dyl;
			while (dyl-- > 0) {
 				/* We're going down the left hand side of the triangle */
				bxl += gradl;
				bxr += gradr;
				btxl += gradxl;
				btyl += gradyl;
				btxr += gradxr;
				btyr += gradyr;

				_ttfhline_4101(bxl >> iV_DIVSHIFT,bxr >> iV_DIVSHIFT,y++,btxl,btxr,
							btyl,btyr,tex->xshift,tex->bmp,brightness);
			}

			/* Have we finished */
			if (--npoints==0) return;

			dxl = nvrt[li-1].x - nvrt[li].x;
			dyl = nvrt[li-1].y - nvrt[li].y;
			gradl = _iVPRIM_DIVTABLE[dyl] * dxl;
			bxl = ((int32)nvrt[li].x << iV_DIVSHIFT) + iV_DIVMULTP_2;

			dtx = nvrt[li-1].u - nvrt[li].u;
			dty = nvrt[li-1].v - nvrt[li].v;
			gradxl = _iVPRIM_DIVTABLE[dyl] * dtx;
			gradyl = _iVPRIM_DIVTABLE[dyl] * dty;
			btxl = ((int32)nvrt[li].u << iV_DIVSHIFT);
			btyl = ((int32)nvrt[li].v << iV_DIVSHIFT);
			li--;

		} else {
			dyl-=dyr;
			while (dyr-- > 0) {
				bxl += gradl;
				bxr += gradr;
				btxl += gradxl;
				btyl += gradyl;
				btxr += gradxr;
				btyr += gradyr;

				_ttfhline_4101(bxl >> iV_DIVSHIFT,bxr >> iV_DIVSHIFT,y++,btxl,btxr,
							btyl,btyr,tex->xshift,tex->bmp,brightness);
		}

			if (--npoints == 0) return;

			dxr = nvrt[ri+1].x - nvrt[ri].x;
			dyr = nvrt[ri+1].y - nvrt[ri].y;
			gradr = _iVPRIM_DIVTABLE[dyr] * dxr;
			bxr = ((int32)nvrt[ri].x << iV_DIVSHIFT) + iV_DIVMULTP_2;

			dtx = nvrt[ri+1].u - nvrt[ri].u;
			dty = nvrt[ri+1].v - nvrt[ri].v;
			gradxr = _iVPRIM_DIVTABLE[dyr] * dtx;
			gradyr = _iVPRIM_DIVTABLE[dyr] * dty;
			btxr = ((int32)nvrt[ri].u << iV_DIVSHIFT);
			btyr = ((int32)nvrt[ri].v << iV_DIVSHIFT);
			ri++;
		}
	}
}


