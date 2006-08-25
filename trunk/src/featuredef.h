/*
 * FeatureDef.h
 *
 * Structure definitions for features
 *
 */
#ifndef _featuredef_h
#define _featuredef_h

typedef enum _feature_type
{
	FEAT_BUILD_WRECK,
	FEAT_HOVER,
	FEAT_TANK,
	FEAT_GEN_ARTE,
	FEAT_OIL_RESOURCE,
	FEAT_BOULDER,
	FEAT_VEHICLE,
	FEAT_BUILDING,
	FEAT_DROID,
	FEAT_LOS_OBJ,
	FEAT_OIL_DRUM,
	FEAT_TREE,
    FEAT_SKYSCRAPER,
	//FEAT_MESA,	no longer used
	//FEAT_MESA2,	
	//FEAT_CLIFF,
	//FEAT_STACK,
	//FEAT_BUILD_WRECK1,
	//FEAT_BUILD_WRECK2,
	//FEAT_BUILD_WRECK3,
	//FEAT_BUILD_WRECK4,
	//FEAT_BOULDER1,
	//FEAT_BOULDER2,
	//FEAT_BOULDER3,
	//FEAT_FUTCAR,
	//FEAT_FUTVAN,
} FEATURE_TYPE;

/* Stats for a feature */
typedef struct _feature_stats
{
	STATS_BASE;

	FEATURE_TYPE	subType;		/* The type of feature */

	iIMDShape		*psImd;				// Graphic for the feature
	UWORD			baseWidth;			/*The width of the base in tiles*/
	UWORD			baseBreadth;		/*The breadth of the base in tiles*/

	//done in script files now
	/* component type activated if a FEAT_GEN_ARTE */
	//UDWORD			compType;			// type of component activated
	//UDWORD			compIndex;			// index of component

	BOOL			tileDraw;			/* Flag to indicated whether the tile needs to be drawn
										   TRUE = draw tile */
	BOOL			allowLOS;			/* Flag to indicate whether the feature allows the LOS
										   TRUE = can see through the feature */
	BOOL			visibleAtStart;		/* Flag to indicate whether the feature is visible at
										   the start of the mission */
	BOOL			damageable;			// Whether the feature can be blown up
	UDWORD			body;				// Number of body points
	UDWORD			armour;				// Feature armour
} FEATURE_STATS;

typedef struct _feature
{
	/* The common structure elements for all objects */
	BASE_ELEMENTS(struct _feature);
	FEATURE_STATS	*psStats;
	UDWORD			startTime;		/*time the feature was created - valid for 
									  wrecked droids and structures */
	UDWORD			body;			/* current body points */
	UWORD			gfxScaling;			// how much to scale the graphic by - for variation - spice of life 'n all that

	UDWORD			timeLastHit;
	BOOL			bTargetted;

} FEATURE;

#endif

