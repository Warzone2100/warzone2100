/*
 * RayCast.h
 *
 * Raycaster functions
 */
#ifndef _raycast_h
#define _raycast_h

#define NUM_RAYS		360

#define RAY_ANGLE		((float)(2*PI/NUM_RAYS))

#define RAY_LENGTH		(TILE_UNITS * 5)

// maximum length for a visiblity ray
#define RAY_MAXLEN	0x7ffff

/* Initialise the visibility rays */
extern BOOL rayInitialise(void);

/* The raycast intersection callback.
 * Return FALSE if no more points are required, TRUE otherwise
 */
typedef BOOL (*RAY_CALLBACK)(SDWORD x, SDWORD y, SDWORD dist);

/* cast a ray from x,y (world coords) at angle ray (0-NUM_RAYS) */
extern void rayCast(UDWORD x, UDWORD y, UDWORD ray, UDWORD length,
					RAY_CALLBACK callback);

// Calculate the angle to cast a ray between two points
extern UDWORD rayPointsToAngle(SDWORD x1,SDWORD y1, SDWORD x2,SDWORD y2);

/* Distance of a point from a line.
 * NOTE: This is not 100% accurate - it approximates to get the square root
 *
 * This is based on Graphics Gems II setion 1.3
 */
extern SDWORD rayPointDist(SDWORD x1,SDWORD y1, SDWORD x2,SDWORD y2,
						   SDWORD px,SDWORD py);

// Calculates the maximum height and distance found along a line from any
// point to the edge of the grid
extern void	getBestPitchToEdgeOfGrid(UDWORD x, UDWORD y, UDWORD direction, SDWORD *pitch);

extern void	getPitchToHighestPoint( UDWORD x, UDWORD y, UDWORD direction, 
								   UDWORD thresholdDistance, SDWORD *pitch );


#endif

