/* Lighting.h - Alex */
#include "lib/ivis_common/pietypes.h"

#define FOG_FLAGS		7
#define FOG_BACKGROUND	1
#define FOG_DISTANCE	2
#define FOG_GROUND		4
#define FOG_ENABLED		8

extern UDWORD	fogStatus;

typedef enum _lightcols
{
LIGHT_RED,
LIGHT_GREEN,
LIGHT_BLUE,
LIGHT_YELLOW,
LIGHT_WHITE
}LIGHT_COLOUR;

typedef struct _light
{
iVector	position;
UBYTE	type;
UDWORD	range;
UDWORD	colour;
} LIGHT;

extern void	processLight(LIGHT *psLight);
//extern void	initLighting( void );
extern void initLighting(UDWORD x1, UDWORD y1, UDWORD x2, UDWORD y2);
extern void	lightValueForTile(UDWORD tileX, UDWORD tileY);
extern void	calcTileIllum(UDWORD tileX, UDWORD tileY);
extern void	doBuildingLights( void );
extern iVector	theSun;
extern UDWORD	lightDoFogAndIllumination(UBYTE brightness, SDWORD dx, SDWORD dz, UDWORD* pSpecular);
extern void	calcDroidIllumination(DROID *psDroid);
//darkens down the tiles that are outside the scroll limits
extern void setScrollLimitLighting(void);


#ifdef ALEXM
extern void	findSunVector( void );
extern void	showSunOnTile(UDWORD x, UDWORD y);
#endif


