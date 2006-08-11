/*
	MapDisplay - Renders the world view necessary for the intelligence map
	Alex McLean, Pumpkin Studios, EIDOS Interactive, 1997

	Makes heavy use of the functions available in display3d.c. Could have
	messed about with display3d.c to make to world render dual purpose, but
	it's neater as a separate file, as the intelligence map has special requirements
	and overlays and needs to render to a specified buffer for later use.
*/

/* ----------------------------------------------------------------------------------------- */
/* Included files */
#include <stdio.h>

/* Includes direct access to render library */
#include "lib/ivis_common/ivisdef.h"
#include "lib/ivis_common/piedef.h"
#include "lib/ivis_common/piestate.h"

#include "lib/ivis_common/piemode.h"
// FIXME Direct iVis implementation include!
#include "lib/ivis_opengl/pietexture.h"

// FIXME Direct iVis implementation include!
#include "lib/ivis_opengl/piematrix.h"
// FIXME Direct iVis implementation include!
#include "lib/ivis_common/rendmode.h"

#include "map.h"
#include "mapdisplay.h"
#include "component.h"
#include "disp2d.h"
#include "display3d.h"
#include "hci.h"
#include "intelmap.h"
#include "intimage.h"
#include "lib/gamelib/gtime.h"

#include "texture.h"
#include "intdisplay.h"

extern UWORD ButXPos;	// From intDisplay.c
extern UWORD ButYPos;
extern UWORD ButWidth,ButHeight;
extern BOOL		godMode;

#define MAX_MAP_GRID	32
#define ROTATE_TIME	(2*GAME_TICKS_PER_SEC)

/* ----------------------------------------------------------------------------------------- */
/* Function prototypes */

/*	Sets up the intelligence map by allocating the necessary memory and assigning world
	variables for the renderer to work with */
//void		setUpIntelMap		(UDWORD width, UDWORD height);
/* Draws the intelligence map to the already setup buffer */
//void		renderIntelMap		(iVector *location, iVector *viewVector, UDWORD elevation);

/* Frees up the memory we've used */
//void		releaseIntelMap		( void );

/* Draw a tile on the grid */
//void		drawMapTile				(SDWORD i, SDWORD j);//line draw nolonger used

/* Clears the map buffer prior to drawing in it */
//static	void	clearMapBuffer(iSurface *surface);
//clear text message background with gray fill
//static void clearIntelText(iSurface *pSurface);

/*fills the map buffer with intelColours prior to drawing in it*/
//static	void	fillMapBuffer(iSurface *surface);

void		tileLayouts(int texture);

//fill the intelColours array with the colours used for the background
//static void setUpIntelColours(void);
/* ----------------------------------------------------------------------------------------- */

SDWORD	elevation;
iVector mapPos, mapView;
//static	iVector	oldPos, oldView;
POINT  sP1,sP2,sP3,sP4;
POINT  *psP1,*psP2,*psP3,*psP4,*psPTemp;

/*Flag to switch code for bucket sorting in renderFeatures etc
  for the renderMapToBuffer code */
  /*This is no longer used but may be useful for testing so I've left it in - maybe
  get rid of it eventually? - AB 1/4/98*/
BOOL	doBucket = TRUE;

#define MAX_INTEL_SHADES		20

//colours used to 'paint' the background of 3D view
UDWORD	intelColours[MAX_INTEL_SHADES];

/* ----------------------------------------------------------------------------------------- */
/* Functions */
iSurface*	setUpMapSurface(UDWORD width, UDWORD height)
{
void		*bufSpace;
iSurface	*pMapSurface;


	/*	Release the old buffer if necessary - we may use many different intel maps
		before resetting the game back to init/close */
	//releaseIntelMap();

	/* Get the required memory for the render surface */
	bufSpace = MALLOC(width*height);

	//initialise the buffer
	memset(bufSpace, 0, (width*height));

	/* Exit if we can't get it! */
	ASSERT((bufSpace!=NULL,"Can't get the memory for the map buffer"));

	/* Build our new surface */
	pMapSurface = iV_SurfaceCreate(REND_SURFACE_USR, width, height, 10, 10,bufSpace);

	/* Exit if we can't get it! */
	ASSERT((pMapSurface!=NULL,"Whoa - can't make surface for map"));

	//set up the intel colours
	//setUpIntelColours();

	/*	Return a pointer to our surface - from this they can get the rendered buffer
		as well as info about width and height etc. */
	return(pMapSurface);
}

void	releaseMapSurface(iSurface *pSurface)
{
	/* Free up old alloaction if necessary */
	if(pSurface!=NULL)
	{

		/* Free up old buffer if necessary */
		if(pSurface->buffer!=NULL)
		{
			FREE(pSurface->buffer);
		}

		FREE(pSurface);
	}
}

void	tileLayouts(int texture)
{
	/* Store the source rect as four points */
	sP1.x = 0;
	sP1.y = 0;
	sP2.x = 63;
	sP2.y = 0;
	sP3.x = 63; sP3.y = 63; sP4.x = 0; 	sP4.y = 63;

	/* Store pointers to the points */
	psP1 = &sP1; psP2 = &sP2; psP3 = &sP3; psP4 = &sP4;

	if (texture & TILE_XFLIP)
	{
		psPTemp = psP1; psP1 = psP2; psP2 = psPTemp; psPTemp = psP3; psP3 = psP4; psP4 = psPTemp;
	}
	if (texture & TILE_YFLIP)
	{
 		psPTemp = psP1;	psP1 = psP4; psP4 = psPTemp; psPTemp = psP2; psP2 = psP3; psP3 = psPTemp;
	}

	switch ((texture & TILE_ROTMASK) >> TILE_ROTSHIFT)
	{
	case 1:
 		psPTemp = psP1; psP1 = psP4; psP4 = psP3; psP3 = psP2; 	psP2 = psPTemp;
		break;
	case 2:
		psPTemp = psP1; psP1 = psP3; psP3 = psPTemp; psPTemp = psP4; psP4 = psP2; psP2 = psPTemp;
		break;
	case 3:
		psPTemp = psP1; psP1 = psP2; psP2 = psP3; psP3 = psP4; psP4 = psPTemp;
		break;
	}
}

/* renders the Research IMDs into the surface - used by message display in
Intelligence Map */
void renderResearchToBuffer(iSurface *pSurface, RESEARCH *psResearch,
                            UDWORD OriginX, UDWORD OriginY)
{
	UDWORD   angle = 0;

    BASE_STATS      *psResGraphic;
    UDWORD          compID, IMDType;
	iVector         Rotation,Position;
	UDWORD          basePlateSize, Radius;
    SDWORD          scale = 0;

	// Set identity (present) context
	pie_MatBegin();

	pie_SetGeometricOffset(OriginX+10,OriginY+10);

	// Pitch down a bit
	//pie_MatRotX(DEG(-30));

    // Rotate round
	// full rotation once every 2 seconds..
	angle = (gameTime2 % ROTATE_TIME) * 360 / ROTATE_TIME;

    Position.x = 0;
	Position.y = 0;
	Position.z = BUTTON_DEPTH;

    // Rotate round
	Rotation.x = -30;
	Rotation.y = angle;
	Rotation.z = 0;

    //draw the IMD for the research
    if (psResearch->psStat)
    {
        //we have a Stat associated with this research topic
        if  (StatIsStructure(psResearch->psStat))
        {
            //this defines how the button is drawn
			IMDType = IMDTYPE_STRUCTURESTAT;
            psResGraphic = psResearch->psStat;
            //set up the scale
			basePlateSize= getStructureStatSize((STRUCTURE_STATS*)psResearch->psStat);
			if(basePlateSize == 1)
			{
				scale = RESEARCH_COMPONENT_SCALE / 2;
                /*HACK HACK HACK!
                if its a 'tall thin (ie tower)' structure stat with something on
                the top - offset the position to show the object on top*/
                if (((STRUCTURE_STATS*)psResearch->psStat)->pIMD->nconnectors AND
                    getStructureStatHeight((STRUCTURE_STATS*)psResearch->psStat) > TOWER_HEIGHT)
                {
                    Position.y -= 30;
                }
			}
			else if(basePlateSize == 2)
			{
				scale = RESEARCH_COMPONENT_SCALE / 4;
			}
			else
			{
				scale = RESEARCH_COMPONENT_SCALE / 5;
			}
        }
        else
        {
            compID = StatIsComponent(psResearch->psStat);
			if (compID != COMP_UNKNOWN)
			{
                //this defines how the button is drawn
	    		IMDType = IMDTYPE_COMPONENT;
                psResGraphic = psResearch->psStat;
		    	scale = RESEARCH_COMPONENT_SCALE;
		    }
            else
            {
                ASSERT((FALSE, "intDisplayMessageButton: invalid stat"));
                IMDType = IMDTYPE_RESEARCH;
                psResGraphic = (BASE_STATS *)psResearch;
            }
        }
    }
    else
    {
        //no Stat for this research topic so use the research topic to define what is drawn
        psResGraphic = (BASE_STATS *)psResearch;
        IMDType = IMDTYPE_RESEARCH;
    }

    //scale the research according to size of IMD
    if (IMDType == IMDTYPE_RESEARCH)
    {
       	Radius = getResearchRadius((BASE_STATS*)psResGraphic);
		if(Radius <= 100)
		{
			scale = RESEARCH_COMPONENT_SCALE / 2;
		}
		else if(Radius <= 128)
		{
			scale = RESEARCH_COMPONENT_SCALE / 3;
		}
		else if(Radius <= 256)
		{
			scale = RESEARCH_COMPONENT_SCALE / 4;
		}
		else
		{
			scale = RESEARCH_COMPONENT_SCALE / 5;
		}
    }


	/* display the IMDs */
	if(IMDType == IMDTYPE_COMPONENT) {
		displayComponentButton(psResGraphic,&Rotation,&Position,TRUE, scale);
	} else if(IMDType == IMDTYPE_RESEARCH) {
		displayResearchButton(psResGraphic,&Rotation,&Position,TRUE, scale);
	} else if(IMDType == IMDTYPE_STRUCTURESTAT) {
		displayStructureStatButton((STRUCTURE_STATS *)psResGraphic,selectedPlayer,&Rotation,
            &Position,TRUE, scale);
	} else {
		ASSERT((FALSE, "renderResearchToBuffer: Unknown PIEType"));
	}

	// close matrix context
	pie_MatEnd();
}














