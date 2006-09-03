#ifndef _tex_
#define _tex_

#include "ivi.h"

//*************************************************************************



#define iV_TEX_MAX		64



//*************************************************************************

#define iV_TEXTEX(i)	((iTexture *) (&_TEX_PAGE[(i)].tex))
#define iV_TEXPAGE(i)	((iTexPage *) (&_TEX_PAGE[(i)]))
#define iV_TEXBMP(i)	((iBitmap *) (&_TEX_PAGE[(i)].tex.bmp))
#define iV_TEXWIDTH(i)	(_TEX_PAGE[(i)].tex.width)
#define iV_TEXHEIGHT(i) (_TEX_PAGE[(i)].tex.height)
#define iV_TEXNAME(i)	((char *) (&_TEX_PAGE[(i)].name))
#define iV_TEXTYPE(i)	(_TEX_PAGE[(i)].type)


//*************************************************************************

typedef struct
{
	iTexture	tex;
	uint8		type;
	char		name[80];
	unsigned int textPage3dfx;
	int		bResource;	// Was page provided by resource handler?
}
iTexPage;

//*************************************************************************
extern int _TEX_INDEX;
extern iTexPage	_TEX_PAGE[iV_TEX_MAX];

//*************************************************************************

int iV_GetTexture(char *filename);
extern int pie_ReloadTexPage(STRING *filename, char *pBuffer);
extern int pie_AddBMPtoTexPages(iSprite* s, STRING *filename, int type, iBool bColourKeyed, iBool bResource);
void pie_ChangeTexPage(int tex_index, iSprite* s, int type, iBool bColourKeyed, iBool bResource);
extern void pie_TexInit(void);

//*************************************************************************

extern void pie_TexShutDown(void);
BOOL FindTextureNumber(UDWORD TexNum,int* TexPage);

#endif
