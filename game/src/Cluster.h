/*
 * Cluster.h
 *
 * Form droids and structures into clusters
 *
 */
#ifndef _cluster_h
#define _cluster_h

// maximum number of clusters in a game
#define CLUSTER_MAX		UBYTE_MAX

// distance between units for them to be in the same cluster
//#define CLUSTER_DIST	(TILE_UNITS*4)

// cluster information flags
#define CLUSTER_PLAYER_MASK		0x07
#define CLUSTER_DROID			0x08
#define CLUSTER_STRUCTURE		0x10

// Indirect the cluster ID to an actual cluster number
extern UBYTE	aClusterMap[CLUSTER_MAX];

// number of droids in a cluster
extern UWORD	aClusterUsage[CLUSTER_MAX];

// whether a cluster can be seen by a player
extern UBYTE	aClusterVisibility[CLUSTER_MAX];

// when a cluster was last attacked
extern UDWORD	aClusterAttacked[CLUSTER_MAX];

// information about the cluster
extern UBYTE	aClusterInfo[CLUSTER_MAX];

// initialise the cluster system
extern void clustInitialise(void);

// update routine for the cluster system
extern void clusterUpdate(void);

// tell the cluster system about a new droid
extern void clustNewDroid(DROID *psDroid);

// tell the cluster system about a new structure
extern void clustNewStruct(STRUCTURE *psStruct);

// update the cluster information for an object
extern void clustUpdateObject(BASE_OBJECT * psObj);

// update all objects from a list belonging to a specific cluster
extern void clustUpdateCluster(BASE_OBJECT *psList, SDWORD cluster);

// remove an object from the cluster system
extern void clustRemoveObject(BASE_OBJECT *psObj);

// display the current clusters
extern void clustDisplay(void);

// tell the cluster system that an objects visibility has changed
extern void clustObjectSeen(BASE_OBJECT *psObj, BASE_OBJECT *psViewer);

// tell the cluster system that an object has been attacked
extern void clustObjectAttacked(BASE_OBJECT *psObj);

// get the cluster ID for an object
extern SDWORD clustGetClusterID(BASE_OBJECT *psObj);

// get the actual cluster number from a cluster ID
extern SDWORD clustGetClusterFromID(SDWORD clusterID);

// find the center of a cluster
extern void clustGetCenter(BASE_OBJECT *psObj, SDWORD *px, SDWORD *py);

// initialise iterating a cluster
extern void clustInitIterate(SDWORD clusterID);

// iterate a cluster
extern BASE_OBJECT *clustIterate(void);

// reset the visibility for all clusters for a particular player
extern void clustResetVisibility(SDWORD player);

#endif

