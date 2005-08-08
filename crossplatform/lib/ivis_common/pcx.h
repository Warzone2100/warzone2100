#ifndef _pcx_
#define _pcx_

#include "ivisdef.h"

extern iBool iV_PCXLoad(char *file, iSprite *s, iColour *pal);
extern BOOL pie_PCXLoadToBuffer(char *file, iSprite *s, iColour *pal);
extern iBool iV_PCXLoadMem(int8 *pcximge, iSprite *s, iColour *pal);
extern BOOL pie_PCXLoadMemToBuffer(int8 *pcximge, iSprite *s, iColour *pal);
//extern iBool iV_PCXSave(char *file, iSprite *s, iColour *pal);

#endif /* _pcx_ */
