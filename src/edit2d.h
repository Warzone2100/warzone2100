#ifdef DISP2D

		// only needed on PC
/*
 * Edit2D.h
 *
 * Interface to the 2d editing routines.
 */
#ifndef _edit2d_h
#define _edit2d_h

/* Whether to show the terrain type on the map */
extern BOOL showTerrain;

/* Initialise the 2D editing module */
extern BOOL ed2dInitialise(void);

/* ShutDown the 2D editing module */
extern void ed2dShutDown(void);

/* Process input for 2D editing */
extern BOOL ed2dProcessInput(void);

/* Display the editing state */
extern void ed2dDisplay(void);

/* Load in a new map */
extern BOOL ed2dLoadMapFile(void);

/* Save the current map */
extern BOOL ed2dSaveMapFile(void);

/* Write the data to the file */
extern BOOL writeMapFile(STRING *pFileName);

#endif



#endif
