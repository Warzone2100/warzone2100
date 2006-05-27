#ifndef _advvis_h
#define _advvis_h

#include "lib/framework/types.h"
#include "base.h"

extern void	avInformOfChange(SDWORD x, SDWORD y);
extern void	avUpdateTiles( void );
extern UDWORD avGetObjLightLevel( BASE_OBJECT *psObj, UDWORD origLevel);
extern void	avGetStats(UDWORD *considered, UDWORD *ignored, UDWORD *calculated);
extern void	setRevealStatus( BOOL val );
extern BOOL	getRevealStatus( void );
extern void	preProcessVisibility( void );
extern void	avSetStatus(BOOL var);

#endif
