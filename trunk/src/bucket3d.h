/* bucket3D.h */

#ifndef _bucket3d_h
#define _bucket3d_h

#define		BUCKET

typedef enum _render_type
{
	RENDER_DROID,
	RENDER_STRUCTURE,
	RENDER_FEATURE,
	RENDER_PROXMSG,
	RENDER_PROJECTILE,
	RENDER_PROJECTILE_TRANSPARENT,
	RENDER_SHADOW,
	RENDER_ANIMATION,
	RENDER_EXPLOSION,
	RENDER_EFFECT,
	RENDER_GRAVITON,
	RENDER_SMOKE,
	RENDER_TILE,
	RENDER_WATERTILE,
	RENDER_MIST,
	RENDER_DELIVPOINT,
	RENDER_PARTICLE
} RENDER_TYPE;

typedef struct _tile_bucket
{
	UDWORD	i;
	UDWORD	j;
	SDWORD	depth;
}
TILE_BUCKET;

//function prototypes

/* reset object list */
extern BOOL bucketSetupList(void);

/* add an object to the current render list */
extern BOOL bucketAddTypeToList(RENDER_TYPE objectType, void* object);

/* render Objects in list */
extern BOOL bucketRenderCurrentList(void);
extern SDWORD	worldMax,worldMin;


#endif
