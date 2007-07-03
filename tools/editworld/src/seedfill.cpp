// Structures and defines for SeedFill().
typedef int Pixel;		/* 1-channel frame buffer assumed */

struct FRect {			/* window: a discrete 2-D rectangle */
    int x0, y0;			/* xmin and ymin */
    int x1, y1;			/* xmax and ymax (inclusive) */
};

struct Segment {
	int y;			//                                                        
	int xl;			// Filled horizontal segment of scanline y for xl<=x<=xr. 
	int xr;			// Parent segment was on line y-dy.  dy=1 or -1           
	int dy;			//
};

#define MAX 10000		/* max depth of stack */

#define PUSH(Y, XL, XR, DY)	/* push new segment on stack */ \
    if (sp<stack+MAX && Y+(DY)>=win->y0 && Y+(DY)<=win->y1) \
    {sp->y = Y; sp->xl = XL; sp->xr = XR; sp->dy = DY; sp++;}

#define POP(Y, XL, XR, DY)	/* pop segment off stack */ \
    {sp--; Y = sp->y+(DY = sp->dy); XL = sp->xl; XR = sp->xr;}



void CHeightMap::FillMap(DWORD Selected,DWORD TextureID,DWORD Type,DWORD Flags)
{
	SWORD y = (SWORD)(Selected / m_MapWidth);
	SWORD x = (SWORD)(Selected - (y*m_MapWidth));

	FRect Rect;
	Rect.x0 = 0;
	Rect.y0 = 0;
	Rect.x1 = m_MapWidth-1;
	Rect.y1 = m_MapHeight-1;

	SeedFill(x,y,&Rect,0,Type,Flags);
	SeedFill(x,y,&Rect,TextureID,Type,Flags);
}

Pixel CHeightMap::PixelRead(int x,int y)
{
	return m_MapTiles[(y*m_MapWidth)+x].TMapID;
}


BOOL CHeightMap::PixelCompare(int x,int y,DWORD Tid,DWORD Type,DWORD Flags)
{
	if( (m_MapTiles[(y*m_MapWidth)+x].TMapID == Tid) &&
//		(m_MapTiles[(y*m_MapWidth)+x].Flags & TF_TYPEMASK == Type) &&
		(m_MapTiles[(y*m_MapWidth)+x].Flags & TF_TEXTUREMASK == Flags) ) {
		return TRUE;
	}

	return FALSE;
}


void CHeightMap::PixelWrite(int x,int y,Pixel nv,DWORD Type,DWORD Flags)
{
	assert(x >=0);
	assert(y >=0);
	assert(x < m_MapWidth);
	assert(y < m_MapHeight);

	m_MapTiles[(y*m_MapWidth)+x].TMapID = nv;
//	m_MapTiles[(y*m_MapWidth)+x].Flags &= ~TF_TYPEMASK;
//	m_MapTiles[(y*m_MapWidth)+x].Flags |= Type;

	m_MapTiles[(y*m_MapWidth)+x].Flags &= TF_GEOMETRYMASK;	// | TF_TYPEMASK;
	m_MapTiles[(y*m_MapWidth)+x].Flags |= Flags;

	ApplyRandomness((y*m_MapWidth)+x,Flags);
}

/*
 * fill: set the pixel at (x,y) and all of its 4-connected neighbors
 * with the same pixel value to the new pixel value nv.
 * A 4-connected neighbor is a pixel above, below, left, or right of a pixel.
 */
void CHeightMap::SeedFill(int x, int y, FRect *win, Pixel nv,DWORD Type,DWORD Flags)
{
    int l, x1, x2, dy;
    Pixel ov;							/* old pixel value */
    Segment stack[MAX], *sp = stack;	/* stack of filled segments */

    ov = PixelRead(x, y);		/* read pv at seed point */

    if (ov==nv || x<win->x0 || x>win->x1 || y<win->y0 || y>win->y1) {
		return;
	}

    PUSH(y, x, x, 1);			/* needed in some cases */
    PUSH(y+1, x, x, -1);		/* seed segment (popped 1st) */

    while (sp>stack) {
		/* pop segment off stack and fill a neighboring scan line */
		POP(y, x1, x2, dy);
		/*
		 * segment of scan line y-dy for x1<=x<=x2 was previously filled,
		 * now explore adjacent pixels in scan line y
		 */
		for (x=x1; x>=win->x0 && PixelRead(x, y)==ov; x--) {
			PixelWrite(x, y, nv,Type,Flags);
		}

		if (x>=x1) {
			goto skip;
		}

		l = x+1;
		if (l<x1) {
			PUSH(y, l, x1-1, -dy);		/* leak on left? */
		}
		x = x1+1;

		do {
			for (; x<=win->x1 && PixelRead(x, y)==ov; x++) {
				PixelWrite(x, y, nv,Type,Flags);
			}

			PUSH(y, l, x-1, dy);

			if (x>x2+1) {
				PUSH(y, x2+1, x-1, -dy);	/* leak on right? */
			}

skip:
			for (x++; x<=x2 && PixelRead(x, y)!=ov; x++);
			l = x;
		} while (x<=x2);
    }
}

