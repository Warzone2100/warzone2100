/*
 * Feature.h
 *
 * Definitions for the feature structures.
 *
 */
#ifndef _feature_h
#define _feature_h

#include "objectdef.h"
#define ONEMIN			(1000 * 60)
#define WRECK_LIFETIME	(0)	//they're just not there anymore!!!!! Ye ha!

/* The statistics for the features */
extern FEATURE_STATS	*asFeatureStats;
extern UDWORD			numFeatureStats;

//Value is stored for easy access to this feature in destroyDroid()/destroyStruct()
//extern UDWORD			droidFeature;
extern UDWORD			structFeature;
extern UDWORD			oilResFeature;

/* Load the feature stats */
extern BOOL loadFeatureStats(char *pFeatureData, UDWORD bufferSize);

/* Release the feature stats memory */
extern void featureStatsShutDown(void);

// Set the tile no draw flags for a structure
extern void setFeatTileDraw(FEATURE *psFeat);

/* Create a feature on the map */
extern FEATURE * buildFeature(FEATURE_STATS *psStats, UDWORD x, UDWORD y,BOOL FromSave);

/* Release the resources associated with a feature */
extern void featureRelease(FEATURE *psFeature);

/* Update routine for features */
extern void featureUpdate(FEATURE *psFeat);

// free up a feature with no visual effects
extern void removeFeature(FEATURE *psDel);

/* Remove a Feature and free it's memory */
extern void destroyFeature(FEATURE *psDel);

/* get a feature stat id from its name */
extern SDWORD getFeatureStatFromName( char *pName );

/*looks around the given droid to see if there is any building 
wreckage to clear*/
extern FEATURE	* checkForWreckage(DROID *psDroid);

extern BOOL featureDamage(FEATURE *psFeature, UDWORD damage, UDWORD weaponSubClass);

#endif

