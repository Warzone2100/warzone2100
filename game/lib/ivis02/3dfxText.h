#ifndef _3dfxtext_h
#define _3dfxtext_h
#ifdef INC_GLIDE
#include "dGlide.h"


extern void	init3dfxTexMemory( void );
extern void	free3dfxTexMemory( void );
//extern GrLOD_t	getLODVal(short dim);
//extern GrAspectRatio_t getAspectRatio(short width, short height);
//extern FxU32 alloc3dfxTexPage(void);
extern void gl_SetupTexturing(void);
extern int	gl_GetLastPageDownLoaded(void);
extern int	gl_downLoad8bitTexturePage(UBYTE* bitmap,UWORD Width,UWORD Height);
extern int	gl_Reload8bitTexturePage(UBYTE* bitmap,UWORD Width,UWORD Height,SDWORD index);
extern BOOL gl_RestoreTextures(void);
extern void showCurrentTextPage( void );
extern void showSpecificTextPage( UDWORD num );

extern BOOL gl_SelectTexturePage( UDWORD num);

#endif
#endif