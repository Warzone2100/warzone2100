/*
 * FindPath.c
 *
 * Routing code.
 *
 */

/* Turn printf's */
//#define DEBUG_GROUP1
#include "frame.h"

#include "objects.h"
#include "map.h"
#include "findpath.h"
#include "gtime.h"
#include "geometry.h"

#include "fractions.h"

#define TURN_RATE 220

#if(0)	   // Don't need any of findpath anymore !!!!!


void	setDirection(MOVE_CONTROL *MoveData);
UDWORD	calcJourney(UDWORD	x1, UDWORD y1, UDWORD x2, UDWORD y2, UDWORD speed);
BOOL NewMovementUpdate(BASE_OBJECT *Obj, MOVE_CONTROL *MoveData);
BOOL AALineOfSight (SDWORD StartX, SDWORD StartY, SDWORD EndX, SDWORD EndY, UBYTE Mask);

/* check whether a tile can be travelled over */
BOOL blockingTile(UDWORD x, UDWORD y, UDWORD mask)
{
	MAPTILE	*psTile;

	mask = mask;

	psTile = mapTile(x,y);
//	if (!(psTile->type & mask) ||

//psor	if (!(TERRAIN_TYPE(psTile) & mask) OR
//psor		(!(mask & TER_OBJECT) && psTile->psObject))

/*	if (!(TERRAIN_TYPE(psTile) & mask) OR
		(!(mask & TER_OBJECT) && TILE_OCCUPIED(psTile)))*/
	if (mask == 0 &&// TILE_OCCUPIED(psTile))
		(TILE_OCCUPIED(psTile) || TERRAIN_TYPE(psTile) == TER_CLIFFFACE))
	{
		return TRUE;
	}
	return FALSE;   
}

void FindPath (SDWORD StartX, SDWORD StartY, SDWORD DestinationX, 
					  SDWORD DestinationY, MOVE_CONTROL *MoveData,	
					  UBYTE Mask)
{
PATH_POINT Route[TRAVELSIZE+2];
PATH_POINT LeftHug[WALLHUGSIZE+2];
PATH_POINT RightHug[WALLHUGSIZE+2];
PATH_POINT *Movement;
UDWORD	RoutePosition,Count,CountMarker;
SDWORD	PixelX,PixelY,BlockX,BlockY;				// Tracked variables containning the current position
SDWORD	BlockDX,BlockDY;							// Destination Block X,Y coordiantes.
SDWORD	LastBX,LastBY;								// Last Block coordiantes processed
SDWORD	DeltaX, DeltaY, XStep, YStep, Gradient;		// The line moving parameters
SDWORD	TravelDirection;							// Block movement direction when error occours
SDWORD	DirectionalConvertor [16] =
				{ 0,4,0,0,2,3,0,1,0,0,0,0,6,5,0,7 };
SDWORD	RNum,LNum;
SDWORD	MovementIndex;
  
/* WARNING - there's a lot of block move/pixel move conflicts going on here - they're 
   resolved in the movement update routine by masking out higher bits - read the commented
   out code below and it should be clearer - Alex . This whole findpath routine HAS to be 
   re-written from scratch.......:-( */

						  
	Movement=MoveData->MovementList;
	MoveData->Position=0;
	MoveData->Mask=Mask;
	MoveData->Status=MOVENAVIGATE;
	MovementIndex=0;

	// Clear the initial setting
	Movement[MovementIndex].XCoordinate=0;

	
	/*  WASTE OF TIME....?
	// Is starting pixel position inside the middle of a square???
	if (((StartX & TILE_MASK) != TILE_UNITS/2) ||
		((StartY & TILE_MASK) != TILE_UNITS/2) )
	{
		// Leave a position free for inserting of specific pixel coordinates.
		Movement[MovementIndex].XCoordinate=-1;
		Movement[MovementIndex].YCoordinate=-1;
		MovementIndex++;
	}
 */
   	/*	WHY??!?!!? MAKES NO ODDS?!
	// Calculate the start and destination coordinates rounding off to centre of a block
	StartX=(StartX & (~TILE_MASK)) + (TILE_UNITS/2);
	StartY=(StartY & (~TILE_MASK)) + (TILE_UNITS/2);
	DestinationX=(DestinationX & (~TILE_MASK)) + (TILE_UNITS/2);
	DestinationY=(DestinationY & (~TILE_MASK)) + (TILE_UNITS/2);
	*/

	// Set current pixel and block coordinates + Destination Block coordinates
	PixelX=StartX;
	PixelY=StartY;
	BlockX=StartX/TILE_UNITS;
	BlockY=StartY/TILE_UNITS;
	BlockDX=DestinationX/TILE_UNITS;
	BlockDY=DestinationY/TILE_UNITS;

	// Initialise pixel line movement
	DeltaX=DestinationX-StartX;
	DeltaY=DestinationY-StartY;

	Gradient=0;
	if (DeltaY==0)
		Gradient=-1;

	XStep=1;
	if (DeltaX<0)
		{
		DeltaX=-DeltaX;
		XStep=-1;
		}

	YStep=1;
	if (DeltaY<0)
		{
		DeltaY=-DeltaY;
		YStep=-1;
		}
	// Set the final coordinates in the movement structure.
	MoveData->DestinationX=BlockDX;
	MoveData->DestinationY=BlockDY;

	// Initialise route structure
	Route[0].XCoordinate=BlockX;
	Route[0].YCoordinate=BlockY;
	RoutePosition=1;

	// Build list of block points to destination block
	while ( ((BlockX!=BlockDX) || (BlockY!=BlockDY)) && RoutePosition<TRAVELSIZE )
	{
		LastBX=BlockX;
		LastBY=BlockY;
		// Move along pixel line until we enter a new square.
		do
		{
			if (Gradient>=0)
			{
				PixelY+=YStep;
				Gradient-=DeltaX;
			}
			else
			{
				PixelX+=XStep;
				Gradient+=DeltaY;
			}
		} while ( ((PixelX/TILE_UNITS)==BlockX) && ((PixelY/TILE_UNITS)==BlockY) );

		// Set the block coordinates of the new block.
		BlockX=PixelX/TILE_UNITS;
		BlockY=PixelY/TILE_UNITS;

		if (!blockingTile(BlockX,BlockY, Mask))
		{
			// Store the new block coordinate
			Route[RoutePosition].XCoordinate=BlockX;
			Route[RoutePosition].YCoordinate=BlockY;
			RoutePosition++;
			// No - So force a re-iteration.
			continue;
		}
		else
		{
			// Store the last valid position
			Route[RoutePosition].XCoordinate=LastBX;
			Route[RoutePosition].YCoordinate=LastBY;
			RoutePosition++;

			if (RoutePosition>=TRAVELSIZE)
				continue;

			// Calculate direction of travel
			TravelDirection= (((BlockX-LastBX) & 3) <<2);
			TravelDirection+= ((BlockY-LastBY) & 3);
			TravelDirection= DirectionalConvertor[TravelDirection];

			while (blockingTile(BlockX,BlockY, Mask))
			{
				do
				{
					if (Gradient>=0)
					{
						PixelY+=YStep;
						Gradient-=DeltaX;
					}
					else
					{
						PixelX+=XStep;
						Gradient+=DeltaY;
					}
				} while ( ((PixelX/TILE_UNITS)==BlockX) && 
						((PixelY/TILE_UNITS)==BlockY) );

				BlockX=(SWORD)(PixelX/TILE_UNITS);
				BlockY=(SWORD)(PixelY/TILE_UNITS);

			}

			RNum=WallHug (RightHug, 1, LastBX, LastBY, BlockX, BlockY, 
				TravelDirection, Mask);
	
			LNum=WallHug (LeftHug, -1, LastBX, LastBY, BlockX, BlockY, 
				TravelDirection, Mask);

			// Now copy the best wall hug path into the overall path
  			if (RNum<=LNum)
  				for (Count=0; Count<(UDWORD)RNum; Count++)
  				{
  					Route[RoutePosition].XCoordinate=RightHug[Count].XCoordinate;
  					Route[RoutePosition].YCoordinate=RightHug[Count].YCoordinate;
  					RoutePosition++;
					// Terminate if we have reached the max buffer size
  					if (RoutePosition>=TRAVELSIZE)
  						break;
  				}
  			else
  				for (Count=0; Count<(UDWORD)LNum; Count++)
  				{
  					Route[RoutePosition].XCoordinate=LeftHug[Count].XCoordinate;
  					Route[RoutePosition].YCoordinate=LeftHug[Count].YCoordinate;
  					RoutePosition++;

					// Terminate if we have reached the max buffer size
  					if (RoutePosition>=TRAVELSIZE)
  						break;
  				}

			if (RoutePosition<TRAVELSIZE)
			{
				// Re-set the movement tracked line gradient
				PixelX= ((BlockX*TILE_UNITS) + (TILE_UNITS/2));
				PixelY= ((BlockY*TILE_UNITS) + (TILE_UNITS/2));

				DeltaX=DestinationX-PixelX;
				DeltaY=DestinationY-PixelY;

				Gradient=0;
				if (DeltaY==0)
					Gradient=-1;

				XStep=1;
				if (DeltaX<0)
				{
					DeltaX=-DeltaX;
					XStep=-1;
				}

				YStep=1;
				if (DeltaY<0)
				{
					DeltaY=-DeltaY;
					YStep=-1;
				}
			}
		}
	}


	// Has the coordinate buffer array been filled?
	if (RoutePosition<TRAVELSIZE &&
		(Route[RoutePosition-1].XCoordinate!=BlockX || 
		Route[RoutePosition-1].YCoordinate!=BlockY) )
		{
		// Save away the destination position (Last block in the sequence)
		Route[RoutePosition].XCoordinate=BlockX;
		Route[RoutePosition].YCoordinate=BlockY;
		RoutePosition++;
		}


	// *** Now optimise the path ***
	BlockX=StartX/TILE_UNITS;
	BlockY=StartY/TILE_UNITS;
	RoutePosition--;
	CountMarker=0;

	do
	{
		// Starting from the last position in the list search backwards testing for line of
		// sight with the first position.
		for (Count=RoutePosition; Count>CountMarker; Count--)
		{
			// Can we see from the current position to the last position.

			if (LineOfSight ( BlockX, BlockY,
							Route[Count].XCoordinate, Route[Count].YCoordinate, Mask) == TRUE)
				{
				Movement[MovementIndex].XCoordinate=Route[Count].XCoordinate;
				Movement[MovementIndex].YCoordinate=Route[Count].YCoordinate;
				MovementIndex++;
				break;
				}
			}

		if (Count == CountMarker)
		{
			/* it's all gone tits up - try to recover */
			Count = RoutePosition;
		}
		else
		{
			// Make last visible position the current position.
			BlockX=Route[Count].XCoordinate;
			BlockY=Route[Count].YCoordinate;
			CountMarker=Count;
		}
	}
	while ( (Count!=RoutePosition) && (MovementIndex<(MAXMOVES-2) ) );

/* Mr M. Carr has a lot to answer for.......... 

	// Did we start this function with the starting coordinates off centre of the block?
	if (Movement[0].XCoordinate==-1)
		{
		// So the first movement the character should do is a pixel movement to the centre of the block
		Movement[0].XCoordinate=StartX | PIXELMOVEMENT;
		Movement[0].YCoordinate=StartY | PIXELMOVEMENT;
		}
*/
	Movement[MovementIndex].XCoordinate=-1;
	Movement[MovementIndex].YCoordinate=-1;
	MoveData->startedMoving = FALSE;
 	MoveData->pathStarted = gameTime;
	return;
}


SDWORD WallHug (PATH_POINT *WallPoints, SBYTE DirectionAdjustment,
				SDWORD StartX, SDWORD StartY, SDWORD EndX, SDWORD EndY, SDWORD Direction,
				UBYTE Mask)
{
SDWORD	WallPosition,Count;
BOOL	ErrorFlag, ArrivalFlag;
SDWORD	DPAdjustment [16] = { 0,-1,1,-1,1,0,1,1,0,1,-1,1,-1,0,-1,-1 };		// Directional Position Adjustment

	// Initialise tracking variables
	WallPosition=0;
	ArrivalFlag=FALSE;

	do
		{
#if 0
		// Are we within 1 block position of the required destination point?
		if ( (StartX-EndX)<=1 && (StartX-EndX)>=-1 )
			if ( (StartY-EndY)<=1 && (StartY-EndY)>=-1 )
				{
				// We have reached the end so store away the last coordinate
				WallPoints[WallPosition].XCoordinate=EndX;
				WallPoints[WallPosition].YCoordinate=EndY;
				WallPosition++;

				// Set we have arrived
				ArrivalFlag=TRUE;
				continue;
				}
#else
		/* rewritten to stop when we get to the end, not before - John */
		if (StartX == EndX && StartY == EndY)
		{
			// Set we have arrived
			ArrivalFlag=TRUE;
			continue;  // While loop instead of do? - John.
		}
#endif
		// Setup the error checking flag
		ErrorFlag=TRUE;

		// If all 8 directions fail then there is no way out of this current position.
		for (Count=0; (Count<8) && (ErrorFlag==TRUE); Count++)
			{
			// Move the direction indicator around the current square
			Direction+=DirectionAdjustment;
			Direction&=7;

			if ((Direction & 1) == 0 &&
				!blockingTile(StartX + DPAdjustment[Direction*2],
						     StartY + DPAdjustment[(Direction*2)+1],
							 Mask))
				// Yes so set the appropriate flag
				ErrorFlag=FALSE;
			}

		// Move to the next valid square around the obstacle.
		StartX+=DPAdjustment[(Direction*2)];
		StartY+=DPAdjustment[(Direction*2)+1];


		// Store the new coordinate away
		WallPoints[WallPosition].XCoordinate=StartX;
		WallPoints[WallPosition].YCoordinate=StartY;
		WallPosition++;

		// Find direction we moved in to get here
		Direction = (Direction+4) & 7;
		}
	while ( (ArrivalFlag==FALSE) && (WallPosition<(WALLHUGSIZE-1)) );

	return (WallPosition);
}

BOOL LineOfSight (SDWORD StartX, SDWORD StartY, SDWORD EndX, SDWORD EndY, UBYTE Mask)

{
SDWORD	DeltaX,DeltaY, Gradient, XStep,YStep;
SDWORD	PixelX,PixelY,BlockX,BlockY;
BOOL	obstacleFlag;


	// Are the two block coordinates right next to each other?
	if ( (StartX-EndX)<=1 && (StartX-EndX)>=-1 )
		if ( (StartY-EndY)<=1 && (StartY-EndY)>=-1 )
			return (TRUE);

	// Calculate actual pixel positions.
	BlockX=StartX;
	BlockY=StartY;
	PixelX= ((StartX*TILE_UNITS) + (TILE_UNITS/2));
	PixelY= ((StartY*TILE_UNITS) + (TILE_UNITS/2));

	// Calculate line movement variables
	DeltaX= (((EndX*TILE_UNITS)+ (TILE_UNITS/2) )-PixelX);
	DeltaY= (((EndY*TILE_UNITS)+ (TILE_UNITS/2) )-PixelY);

	Gradient=0;
	if (DeltaY==0)
		Gradient=-1;

	XStep=1;
	if (DeltaX<0)
		{
		DeltaX=-DeltaX;
		XStep=-1;
		}

	YStep=1;
	if (DeltaY<0)
		{
		DeltaY=-DeltaY;
		YStep=-1;
		}

	// Set no-obstacle encountered
	obstacleFlag=TRUE;

	while ( ((BlockX!=EndX) || (BlockY!=EndY)) && obstacleFlag==TRUE )
		{
		// Find next block in path towards destination block
		do
			{
			if (Gradient>=0)
				{
				PixelY+=YStep;
				Gradient-=DeltaX;
				}
			else
				{
				PixelX+=XStep;
				Gradient+=DeltaY;
				}
			}
		while ( ((PixelX/TILE_UNITS)==BlockX) && ((PixelY/TILE_UNITS)==BlockY) );

		// Move to new block
		BlockX=(SWORD)(PixelX/TILE_UNITS);
		BlockY=(SWORD)(PixelY/TILE_UNITS);

		if (blockingTile(BlockX, BlockY,Mask))
				// No Way it's to hot to handle. Set flag for error.
				obstacleFlag=FALSE;
		}

	return (obstacleFlag);
}

BOOL AALineOfSight (SDWORD StartX, SDWORD StartY, SDWORD EndX, SDWORD EndY, UBYTE Mask)

{
	BOOL	los;
	UDWORD	numPoints, i;

	Mask = Mask;

	/* Get the tiles for the line */
	mapCalcAALine(StartX, StartY, EndX,EndY, &numPoints);

	/* Scan the points for obstacles */
	los = TRUE;
	for(i=0; i<numPoints; i++)
	{
//psor		if ((!(Mask & TER_OBJECT) && aMapLinePoints[i].psTile->psObject) ||
//			if ((!(Mask & TER_OBJECT) && TILE_OCCUPIED(aMapLinePoints[i].psTile)) ||
//			!(aMapLinePoints[i].psTile->type & Mask))
//			!(TERRAIN_TYPE(aMapLinePoints[i].psTile) & Mask))
		if (TILE_OCCUPIED(aMapLinePoints[i].psTile) ||
			TERRAIN_TYPE(aMapLinePoints[i].psTile) == TER_CLIFFFACE)
		{
			los = FALSE;
			break;
		}
	}

	return los;
}


/* Setup the movement data for a new target - allows an intermediate target to be
 * set without altering the the path.
 */
void MoveToTarget(BASE_OBJECT *psObj, MOVE_CONTROL *psMoveCtrl,
				  UDWORD targetX, UDWORD targetY)
{
		/* Record what time we started on this leg of the journey */
		psMoveCtrl->timeStarted = gameTime;

		/* Set off our starting points */
		psMoveCtrl->srcX = psObj->x;
		psMoveCtrl->srcY = psObj->y;

		psMoveCtrl->targetX = targetX;
		psMoveCtrl->targetY = targetY;

		setDirection(psMoveCtrl);
		psObj->direction = psMoveCtrl->Direction;

		/* Find out the ETA of this leg of the journey */
		psMoveCtrl->arrivalTime=
				calcJourney(psMoveCtrl->srcX,psMoveCtrl->srcY,
							psMoveCtrl->targetX,psMoveCtrl->targetY,
							psMoveCtrl->Speed);

		/* We have our waypoint - now move on a pixel basis */
		psMoveCtrl->Status = MOVEPOINTTOPOINT;
		psMoveCtrl->TargetDir = calcDirection(psMoveCtrl->srcX,psMoveCtrl->srcY,
											  psMoveCtrl->targetX,psMoveCtrl->targetY);
}



/* Turn towards a target */
void TurnToTarget(BASE_OBJECT *Obj, MOVE_CONTROL *MoveData,
				  UDWORD tarX, UDWORD tarY)
{
	UDWORD	dDif;

	/* Set up the turn data */
	MoveData->TargetDir = calcDirection(Obj->x,Obj->y, tarX,tarY);
	MoveData->Status = MOVETURN;
	MoveData->timeStarted = gameTime;
	dDif = dirDiff(MoveData->Direction3D, MoveData->TargetDir);
	MoveData->arrivalTime = dDif * GAME_TICKS_PER_SEC/TURN_RATE + gameTime;

	DBP1(("StartDir: %d  TargetDir: %d\n", MoveData->Direction3D, MoveData->TargetDir));
}


/* Return whether to decrement or increment a direction
   in order to get to the other direction quickest */
static void updateTurn(MOVE_CONTROL *psMove)
{
	SDWORD  diff, newDir, tarDirDif;

	tarDirDif = TURN_RATE * ((SDWORD)psMove->arrivalTime - (SDWORD)gameTime)
							/ GAME_TICKS_PER_SEC;
	diff = psMove->TargetDir - psMove->Direction3D;

	if (diff > 0)
	{
		/* Target dir is greater than current */
		if (diff < 180)
		{
			/* Simple case - just increase towards target */
			psMove->Direction3D = psMove->TargetDir - tarDirDif;
		}
		else
		{
			/* effectively decrease to target, but could go over 0 boundary */
			newDir = (SDWORD)psMove->TargetDir + tarDirDif;
			if (newDir >= 360)
			{
				newDir -= 360;
			}
			psMove->Direction3D = (UDWORD)newDir;
		}
	}
	else
	{
		if (diff > -180)
		{
			/* Simple case - just decrease towards target */
			psMove->Direction3D = psMove->TargetDir + tarDirDif;
		}
		else
		{
			/* effectively increase to target, but could go over 0 boundary */
			newDir = (SDWORD)psMove->TargetDir - tarDirDif;
			if (newDir < 0)
			{
				newDir += 360;
			}
			psMove->Direction3D = (UDWORD)newDir;
		}
	}
	ASSERT((psMove->Direction3D < 360,
		"updateTurn: droid direction out of range"));	
}







/* Spangly new movement routine that moves things at same real world speed regardless
   of frame rate - provided clock has sufficient resolution */
BOOL NewMovementUpdate(BASE_OBJECT *Obj, MOVE_CONTROL *MoveData)
{
PATH_POINT	*Movement;
UDWORD		timeSoFar;
SDWORD		xDif,yDif;
UDWORD		tarX,tarY;
FRACT		fraction;

#ifdef DEBUG
//	gameTimeStop();
#endif

	/* First, check to see if we actually have a path to pursue */
	if (MoveData->Status == MOVEINACTIVE)
	{
		MoveData->startedMoving = FALSE;
		return(TRUE);
	}

	/* Get this droid's waypoint list */
	Movement = MoveData->MovementList;

 	/* Is this our first time round? */
	if (!MoveData->startedMoving)
	{
		/* Tell system that we're on our way - reset timers etc */
		MoveData->startedMoving = TRUE;
		/* New a new waypoint */
		MoveData->Status = MOVENAVIGATE;
		}
 
	/* Below is where the probbies lie.....cos of first check for completion */
	/* Some dodgy things occur here - cos of first move centering.....		 */

	/* Do we need a new destination? */
	tarX = Movement[MoveData->Position].XCoordinate * TILE_UNITS
							+ TILE_UNITS/2;
	tarY = Movement[MoveData->Position].YCoordinate * TILE_UNITS
							+ TILE_UNITS/2;
	if (MoveData->Status == MOVENAVIGATE || MoveData->Status == MOVEPAUSE)
	{
		TurnToTarget(Obj, MoveData, tarX,tarY);
	}
 
	/* Are we turning to face the new direction */
	if (MoveData->Status == MOVETURN || MoveData->Status == MOVETURNSTOP)
	{
		if (MoveData->arrivalTime <= gameTime)
		{
			if (MoveData->Status == MOVETURN)
			{
				/* Set up the new movement */
				MoveData->Direction3D = MoveData->TargetDir;
				MoveToTarget(Obj, MoveData, tarX,tarY);
			}
			else
			{
				MoveData->Status = MOVEINACTIVE;
				return TRUE;
			}
		}
		else
		{
			/* Figure out how far we should have turned */
			updateTurn(MoveData);
			DBP1(("     update %d\n", MoveData->Direction3D));
		}
	}

	/* we're already moving to a new block */
	if (MoveData->Status==MOVEPOINTTOPOINT)
	{
		/* Find out how long we've been on this stage */
		timeSoFar = gameTime - MoveData->timeStarted;
		xDif = (MoveData->targetX-MoveData->srcX);
		yDif = (MoveData->targetY-MoveData->srcY);

		/* Calculate how FAR ALONG this stage we SHOULD be */


		fraction = FRACTdiv(MAKEFRACT(timeSoFar),MAKEFRACT(MoveData->arrivalTime));
 
   


		/* Should be have arrived by now? */
		if (timeSoFar>=MoveData->arrivalTime)
		{
			/* If so, then get there NOW... */
			Obj->x = MoveData->targetX;
			Obj->y = MoveData->targetY;
	   
			/* And tell system to get to the next journey waypoint */
			MoveData->Position++;
			MoveData->Status = MOVENAVIGATE;
			MoveData->startedMoving = FALSE;
	   
		}
   		else
		{
			/* Set to the correct fractional distance along this waypoint stage */
			
			Obj->x = MoveData->srcX+(MAKEINT(fraction*xDif));
			Obj->y = MoveData->srcY+(MAKEINT(fraction*yDif));
		} 
	}

	/* Have we got there? */
	if (Obj->x == (UDWORD)MoveData->DestinationX AND 
		Obj->y == (UDWORD)MoveData->DestinationY)
	{
		MoveData->Status = MOVEINACTIVE;
		MoveData->lastTime = gameTime-MoveData->pathStarted;
		MoveData->startedMoving = FALSE;
		return(TRUE);
	}
	/* Are we at the end of the list? - we've got there*/
	if (Movement[MoveData->Position].XCoordinate == -1)
	{
		MoveData->Status = MOVEINACTIVE;
		MoveData->lastTime = gameTime-MoveData->pathStarted;
		MoveData->startedMoving = FALSE;
		return(TRUE);
	}

#ifdef DEBUG
//	gameTimeStart();
#endif

	/* We're not there yet */
	return(FALSE);
}

UDWORD	calcJourney(UDWORD	x1, UDWORD y1, UDWORD x2, UDWORD y2, UDWORD speed)
{
FRACT	xDif,yDif;
FRACT	eta;
FRACT	length;

	/* Get differences between start and end points */
	xDif = MAKEFRACT(abs(x1-x2));
	yDif = MAKEFRACT(abs(y1-y2));

	/* Find the length between these two points */

	length = fSQRT(FRACTmul(xDif,xDif) + FRACTmul(yDif,yDif));


	/* And how long should that take, given the passed in speed */
	eta = ((length/speed)*GAME_TICKS_PER_SEC);

	/* Convert to natural number N+, and return */
	return(MAKEINT(eta));
}
													
void	setDirection(MOVE_CONTROL *MoveData)
{
SDWORD	x0,x1,y0,y1;
UDWORD	xVect,yVect;
UDWORD	xComp,yComp;
BOOL	found = FALSE;
int i;
UDWORD vect2Follow[COMPASS_POINTS][3]=
{
{POS,POS,SOUTHEAST},
{NEG,NEG,NORTHWEST},
{POS,NEG,NORTHEAST},
{NEG,POS,SOUTHWEST},
{POS,NIL,EAST},
{NEG,NIL,WEST},
{NIL,POS,SOUTH},
{NIL,NEG,NORTH}
};
	
	x0 = MoveData->srcX;
	y0 = MoveData->srcY;
	x1 = MoveData->targetX;
	y1 = MoveData->targetY;

	if (x0!=x1)
	{
		if(x0<x1)
			xVect = POS;
		else
			xVect = NEG;

	}
	else
		xVect = NIL;

	if (y0!=y1)
	{
		if (y0<y1)
			yVect = POS;
		else
			yVect = NEG;

	}
	else
		yVect = NIL;

	i=0;
	while (!found AND i<8)
	{
		xComp = vect2Follow[i][0];
		yComp = vect2Follow[i][1];
		if (xComp == xVect AND yComp == yVect)
		{
			found = TRUE;
		}
	i++;
	}

	if (i<9)
	{
	i--;
	MoveData->Direction = (char)vect2Follow[i][2];
	}
	else
	{
	MoveData->Direction = (char)0;
	}
}

























#endif






/* Return the difference in directions */
UDWORD dirDiff(SDWORD start, SDWORD end)
{
	SDWORD retval, diff;

	diff = end - start;

	if (diff > 0)
	{
		if (diff < 180)
		{
			retval = diff;
		}
		else
		{
			retval = 360 - diff;
		}
	}
	else
	{
		if (diff > -180)
		{
			retval = - diff;
		}
		else
		{
			retval = 360 + diff;
		}
	}

	ASSERT((retval >=0 && retval <=180,
		"dirDiff: result out of range"));

	return retval;
}
