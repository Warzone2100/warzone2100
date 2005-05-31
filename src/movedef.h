/*
 * MoveDef.h
 *
 * Structure definitions for movement structures.
 *
 */
#ifndef _movedef_h
#define _movedef_h


#define TRAVELSIZE			100


typedef struct _path_point
{
//	SDWORD		XCoordinate,YCoordinate;
	UBYTE		x,y;
} PATH_POINT;


typedef struct _move_control
{
	UBYTE	Status;						// Inactive, Navigating or moving point to point status
	UBYTE	Mask;						// Mask used for the creation of this path	
//	SBYTE	Direction;					// Direction object should be moving (0-7) 0=Up,1=Up-Right
//	SDWORD	Speed;						// Speed at which object moves along the movement list
	UBYTE	Position;	   				// Position in asPath
	UBYTE	numPoints;					// number of points in asPath
//	PATH_POINT	MovementList[TRAVELSIZE];
	PATH_POINT	asPath[TRAVELSIZE];		// Pointer to list of block X,Y coordinates.
										// When initialised list is terminated by (0xffff,0xffff)
										// Values prefixed by 0x8000 are pixel coordinates instead of
										// block coordinates
	SDWORD	DestinationX;				// DestinationX,Y should match objects current X,Y
	SDWORD	DestinationY;				//		location for this movement to be complete.
//   	UDWORD	Direction3D;				// *** not necessary
//	UDWORD	TargetDir;					// *** not necessary Direction the object should be facing
//	SDWORD	Gradient;					// Gradient of line
//	SDWORD	DeltaX;						// Distance from start to end position of current movement X
//	SDWORD	DeltaY;						// Distance from start to end position of current movement Y
//	SDWORD	XStep;						// Adjustment to the characters X position each movement
//	SDWORD	YStep;						// Adjustment to the characters Y position each movement
//	SDWORD	DestPixelX;					// Pixel coordinate destination for pixel movement (NOT the final X)
//	SDWORD	DestPixelY;					// Pixel coordiante destination for pixel movement (NOT the final Y)
	SDWORD	srcX,srcY,targetX,targetY;

	/* Stuff for John's movement update */
	FRACT	fx,fy;						// droid location as a fract
//	FRACT	dx,dy;						// x and y change for current direction
	// NOTE: this is supposed to replace Speed
	FRACT	speed;						// Speed of motion
	SWORD	boundX,boundY;				// Vector for the end of path boundary
	SWORD	dir;						// direction of motion (not the direction the droid is facing)

	SWORD	bumpDir;					// direction at last bump
	UDWORD	bumpTime;					// time of first bump with something
	UWORD	lastBump;					// time of last bump with a droid - relative to bumpTime
	UWORD	pauseTime;					// when MOVEPAUSE started - relative to bumpTime
	UWORD	bumpX,bumpY;				// position of last bump

	UDWORD	shuffleStart;				// when a shuffle started

	struct _formation	*psFormation;			// formation the droid is currently a member of

	/* vtol movement - GJ */
	SWORD	iVertSpeed;
	UWORD	iAttackRuns;

	// added for vtol movement

	FRACT	fz;


	/* Only needed for Alex's movement update ? */
//	UDWORD	timeStarted;
//	UDWORD	arrivalTime;
//	UDWORD	pathStarted;
//	BOOL	startedMoving;
//	UDWORD	lastTime;
//	BOOL	speedChange;
} MOVE_CONTROL;


#endif


