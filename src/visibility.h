#ifndef _visibility_h
#define _visibility_h

/* Terrain types that could obscure LOS */
#define LOS_MASK	0 /*TER_STONE*/

/* The distance under which visibility is automatic */
#define BASE_VISIBILITY  (5*TILE_UNITS/2)

// initialise the visibility stuff
extern BOOL visInitialise(void);

extern BOOL visTilesPending(BASE_OBJECT *psObj);

/* Check which tiles can be seen by an object */
extern void visTilesUpdate(BASE_OBJECT *psObj,BOOL SpreadLoad);

/* Check whether psViewer can see psTarget
 * psViewer should be an object that has some form of sensor,
 * currently droids and structures.
 * psTarget can be any type of BASE_OBJECT (e.g. a tree).
 */
extern BOOL visibleObject(BASE_OBJECT *psViewer, BASE_OBJECT *psTarget);

/* Check whether psViewer can see psTarget.
 * struckBlock controls whether structures block LOS
 */
extern BOOL visibleObjectBlock(BASE_OBJECT *psViewer, BASE_OBJECT *psTarget,
							   BOOL structBlock);

// Do visibility check, but with walls completely blocking LOS.
extern BOOL visibleObjWallBlock(BASE_OBJECT *psViewer, BASE_OBJECT *psTarget);

// Find the wall that is blocking LOS to a target (if any)
extern BOOL visGetBlockingWall(BASE_OBJECT *psViewer, BASE_OBJECT *psTarget, STRUCTURE **ppsWall);

extern void	processVisibility(BASE_OBJECT *psCurr);

// calculate the power at a given distance from a sensor/ecm
extern UDWORD visCalcPower(UDWORD x1,UDWORD y1, UDWORD x2,UDWORD y2, UDWORD power, UDWORD range);

// update the visibility reduction
extern void visUpdateLevel(void);

extern	void	setUnderTilesVis(BASE_OBJECT *psObj, UDWORD player);


// sensorrangedisplay.
extern BOOL	bDisplaySensorRange;
extern void stopSensorDisplay(void);
extern void startSensorDisplay(void);




// fast test for whether obj2 is in range of obj1
static inline BOOL visObjInRange(BASE_OBJECT *psObj1, BASE_OBJECT *psObj2, SDWORD range)
{
	SDWORD	xdiff,ydiff, distSq, rangeSq;

	xdiff = (SDWORD)psObj1->x - (SDWORD)psObj2->x;
	if (xdiff < 0)
	{
		xdiff = -xdiff;
	}
	if (xdiff > range)
	{
		// too far away, reject
		return FALSE;
	}

	ydiff = (SDWORD)psObj1->y - (SDWORD)psObj2->y;
	if (ydiff < 0)
	{
		ydiff = -ydiff;
	}
	if (ydiff > range)
	{
		// too far away, reject
		return FALSE;
	}

	distSq = xdiff*xdiff + ydiff*ydiff;
	rangeSq = range*range;
	if (distSq > rangeSq)
	{
		// too far away, reject
		return FALSE;
	}

	return TRUE;
}


#endif
