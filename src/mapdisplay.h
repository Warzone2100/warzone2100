#ifndef _mapdisplay_h
#define _mapdisplay_h

#include "lib/ivis_common/pietypes.h"

/* MapDisplay.h */

#define BUFFER_GRIDX	22
#define BUFFER_GRIDY	22


/*Flag to switch code for bucket sorting in renderFeatures etc
  for the renderMapToBuffer code */
  /*This is no longer used but may be useful for testing so I've left it in - maybe
  get rid of it eventually? - AB 1/4/98*/
extern BOOL	doBucket;
extern BOOL	godMode;

// The surface to render Message information into for the Intelligence Screen
extern iSurface	*mapSurface;

/*	Sets up a map surface by allocating the necessary memory and assigning world
	variables for the renderer to work with */
extern iSurface* setUpMapSurface(UDWORD width, UDWORD height);

/* renders up to two IMDs into the surface - used by message display in Intelligence Map */
extern void renderIMDToBuffer(struct iSurface *pSurface, struct iIMDShape *pIMD,
							  struct iIMDShape *pIMD2, UDWORD WindowX,UDWORD WindowY,
							  UDWORD OriginX,UDWORD OriginY);
extern void renderResearchToBuffer(iSurface *pSurface, RESEARCH *psResearch,
                            UDWORD OriginX, UDWORD OriginY);


extern void	releaseMapSurface(struct iSurface *pSurface);



#endif

