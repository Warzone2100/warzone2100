
#include "frame.h"
#include "ivisdef.h"
#include "fbf.h"
#include "bug.h"

#include "ivispatch.h"

//#include <stdio.h>
//#include <stdlib.h>
//#include <io.h>
//#include "pcx.h"
//#include "pal.h"
//#include "bug.h"
//#include "ivisheap.h"

//*************************************************************************

#define PCX_BUFFER_SIZE		65536
//#define PCX_PIXEL(s,x,y)	(* ((uint8 *) ((s)->bmp + (x) + ((y) * (s)->width))))

//*************************************************************************

typedef struct {
	uint8	manufacturer;
	uint8	version;
	uint8	encoding;
	uint8	bits_per_pixel;
	int16	xmin,ymin;
	int16	xmax,ymax;
	int16	hres, vres;
	uint8	palette16[48];
	uint8	reserved;
	uint8	colour_planes;
	int16	bytes_per_line;
	int16	palette_type;
	uint8	filler[58];
} iPCX_header;

//*************************************************************************

static int 	_PCX_FI;
static int8	*_PCX_MI;
static iBool _PCX_MEM;

//*************************************************************************


static int _pcx_read_int8(void)
{

	if (_PCX_MEM)
		return *_PCX_MI++;
	else
		return (iV_FileGet(_PCX_FI));
}


//*************************************************************************

static void _pcx_info(char *filename, iPCX_header *h)

{

	iV_DEBUG1("pcx[pcx_info] = '%s' - PCX file header:\n",filename);
	iV_DEBUG1(	"         Manufacturer     %4d\n",h->manufacturer);
	iV_DEBUG12(	"         Version          %4d\n"
					"         Encoding         %4d\n"
					"         Bits/pixel       %4d\n"
					"         Xmin, Ymin       %4d,%4d\n"
					"         Xmax, Ymax       %4d,%4d\n"
					"         H, V resolution  %4d,%4d\n"
					"         Colour plane     %4d\n"
					"         Bytes/line       %4d\n"
					"         Palette type     %4d\n",
			h->version,
			h->encoding,
			h->bits_per_pixel,
			h->xmin,h->ymin,
			h->xmax,h->ymax,
			h->hres, h->vres,
			h->colour_planes,
			h->bytes_per_line,
			h->palette_type);
}

//*************************************************************************

static void _load_image(iBitmap *bmp, long size, unsigned int depth) {
	int data, num_bytes;
	long count;

	count = 0;

	while (count < size) {

		data = _pcx_read_int8();

		if ((data & 0xc0) == 0xc0) {
			num_bytes = data & 0x3f;
			data  = _pcx_read_int8();
		} else {
			num_bytes = 1;
		}

		while ((num_bytes-- > 0) && ((count += depth) < size)) {
			*bmp = data;
			bmp += depth;
		}
	}
}

//*************************************************************************

static void _load_palette(iColour *pal)

{
	int i;

	iV_FileSeek(_PCX_FI,-769,iV_FBF_SEEK_END);

	if (_pcx_read_int8() == 0x0C) {
		for (i=0; i<256; i++) {
			pal[i].r = _pcx_read_int8();
			pal[i].g = _pcx_read_int8();
			pal[i].b = _pcx_read_int8();
		}
	}
}

//*************************************************************************

static void _load_palette_mem(iColour *pal)

{
	int i;

	if (_pcx_read_int8() == 0x0C) {
		for (i=0; i<256; i++) {
			pal[i].r = _pcx_read_int8();
			pal[i].g = _pcx_read_int8();
			pal[i].b = _pcx_read_int8();
		}
	}
}

//*************************************************************************

static void _colorize_image(iBitmap *bmp, long size, unsigned int depth, iPalette p) {
	unsigned int i;

	if (depth == 3) {
		for (i = 0; i < size; i += 3) {
			unsigned int c = bmp[i];

			bmp[i] = p[c].r;
			bmp[i+1] = p[c].g;
			bmp[i+2] = p[c].b;
		}
	} else if (depth == 4) {
		for (i = 0; i < size; i += 4) {
			unsigned int c = bmp[i];

			if (c == 0) {
				bmp[i] = 127;
				bmp[i+1] = 127;
				bmp[i+2] = 127;
				bmp[i+3] = 0;
			} else {
				bmp[i] = p[c].r;
				bmp[i+1] = p[c].g;
				bmp[i+2] = p[c].b;
				bmp[i+3] = 255;
			}
		}
	} else {
	}
}

//*************************************************************************

BOOL pie_PCXLoadToBuffer(char *file, iSprite *s, iColour* pal)
{
	iPCX_header header;
	int i;
	long bsize;
	uint8 *hp;


	_PCX_MEM = FALSE;


	iV_DEBUG1("pcx[PCXLoad] = loading pcx file '%s':\n",file);

	if ((_PCX_FI = iV_FileOpen(file,iV_FBF_MODE_R,51200)) <0) {
		iV_DEBUG1("pcx[PCXLoad] = could not open pcx file %d\n",_PCX_FI);
		return FALSE;
	}


	hp = (uint8 *) &header;

	for (i=0; i<128; i++)
		*hp++ = _pcx_read_int8();

	_pcx_info(file,&header);

	if ((header.manufacturer != 10) && (header.version != 5)) {
		iV_FileClose(_PCX_FI);
		return FALSE;
	}

	if (header.bits_per_pixel != 8) {
		iV_FileClose(_PCX_FI);
		return FALSE;
	}

	if (s->width != (header.xmax - header.xmin)+1)
	{
		iV_FileClose(_PCX_FI);
		return FALSE;
	}

	if (s->height != (header.ymax - header.ymin)+1)
	{
		iV_FileClose(_PCX_FI);
		return FALSE;
	}

	bsize = s->height * s->width * 4;

	{
		iPalette p;

		_load_image(s->bmp, bsize, 4);
		_load_palette(p);
		_colorize_image(s->bmp, bsize, 4, p);

	}

	iV_FileClose(_PCX_FI);

	iV_DEBUG3("pcx[iV_PCXLoad] = file '%s' %dx%d load successful\n",file,
						s->width,s->height);

	return TRUE;
}

iBool iV_PCXLoad(char *file, iSprite *s, iColour *pal) 
{
	iPCX_header header;
	int i;
	long bsize;
	uint8 *hp;


	_PCX_MEM = FALSE;


	iV_DEBUG1("pcx[PCXLoad] = loading pcx file '%s':\n",file);

	if ((_PCX_FI = iV_FileOpen(file,iV_FBF_MODE_R,51200)) <0) {
		iV_DEBUG1("pcx[PCXLoad] = could not open pcx file %d\n",_PCX_FI);
		return FALSE;
	}


	hp = (uint8 *) &header;

	for (i=0; i<128; i++)
		*hp++ = _pcx_read_int8();

	_pcx_info(file,&header);

	if ((header.manufacturer != 10) && (header.version != 5)) {
		iV_FileClose(_PCX_FI);
		return FALSE;
	}

	if (header.bits_per_pixel != 8) {
		iV_FileClose(_PCX_FI);
		return FALSE;
	}

	s->width = (header.xmax - header.xmin)+1;
	s->height = (header.ymax - header.ymin)+1;

	bsize = s->height * s->width;

	if ((s->bmp = (uint8 *) iV_HeapAlloc(bsize)) == NULL) {
		iV_FileClose(_PCX_FI);
		return FALSE;
	}
	//iV_DEBUG1("PCX LOAD [%s] size=%d ptr=%p\n",file,bsize,s->bmp);

	_load_image(s->bmp,bsize, 1);

	if (pal) {
		iV_DEBUG1("warning palette is being loaded for %s",file);
 		_load_palette(pal);
	}

	iV_FileClose(_PCX_FI);

	//iV_DEBUG1("pcx[iV_PCXLoad] = file '%s' %dx%d load successful\n",file, s->width,s->height);

	return TRUE;
}


iBool iV_PCXLoadMem(int8 *pcximge, iSprite *s, iColour *pal) {
	iPCX_header header;
	int i;
	long bsize;
	uint8 *hp;

	_PCX_MEM = TRUE;
	_PCX_MI = pcximge;


	iV_DEBUG1("pcx[PCXLoadMem] = loading pcx from memory %p\n",_PCX_MI);

	hp = (uint8 *) &header;

	for (i=0; i<128; i++)
		*hp++ = _pcx_read_int8();

	if ((header.manufacturer != 10) && (header.version != 5))
		return FALSE;

	if (header.bits_per_pixel != 8)
		return FALSE;

	s->width = (header.xmax - header.xmin)+1;
	s->height = (header.ymax - header.ymin)+1;

	bsize = s->height * s->width * 4;

	if ((s->bmp = (uint8 *) iV_HeapAlloc(bsize)) == NULL) {
		return FALSE;
	}

	{
		iPalette p;

		_load_image(s->bmp, bsize, 4);
		_load_palette_mem(p);
		_colorize_image(s->bmp, bsize, 4, p);
	}

	//iV_DEBUG1("pcx[iV_PCXLoadMEM] = mem %p %dx%d load successful\n",_PCX_MI, s->width,s->height);

	return TRUE;
}

BOOL pie_PCXLoadMemToBuffer(int8 *pcximge, iSprite *s, iColour *pal)

{
	iPCX_header header;
	int i;
	long bsize;
	uint8 *hp;


	_PCX_MEM = TRUE;
	_PCX_MI = pcximge;


	iV_DEBUG1("pcx[PCXLoadMem] = loading pcx from memory %p\n",_PCX_MI);


	hp = (uint8 *) &header;

	for (i=0; i<128; i++)
		*hp++ = _pcx_read_int8();

	if ((header.manufacturer != 10) && (header.version != 5))
		return FALSE;

	if (header.bits_per_pixel != 8)
		return FALSE;

	if (s->width != (header.xmax - header.xmin)+1)
	{
		iV_FileClose(_PCX_FI);
		return FALSE;
	}

	if (s->height != (header.ymax - header.ymin)+1)
	{
		iV_FileClose(_PCX_FI);
		return FALSE;
	}

	bsize = s->height * s->width * 4;

	{
		iPalette p;

		_load_image(s->bmp, bsize, 4);
		_load_palette_mem(p);
		_colorize_image(s->bmp, bsize, 4, p);

	}

	iV_DEBUG3("pcx[iV_PCXLoadMEM] = mem %p %dx%d load successful\n",_PCX_MI,
						s->width,s->height);

	return TRUE;
}

