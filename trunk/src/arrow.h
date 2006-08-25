/***************************************************************************/

BOOL	arrowInit( void );
void	arrowShutDown( void );
BOOL	arrowAdd( SDWORD iBaseX, SDWORD iBaseY, SDWORD iBaseZ, SDWORD iHeadX,
					SDWORD iHeadY, SDWORD iHeadZ, UBYTE iColour );
void	arrowDrawAll( void );
extern void	draw3dLine(iVector *src, iVector *dest, UBYTE col);

/***************************************************************************/
