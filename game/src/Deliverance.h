/*
 * Deliverance.h
 *
 * Global defines for the game
 *
 */
#ifndef _deliverance_h
#define _deliverance_h

#ifdef WIN32
#define MAX_PLAYERS 8		/*Utterly arbitrary at the moment!! */
#else
#define MAX_PLAYERS 4		/*Utterly arbitrary at the moment!! -good job because 8 will take up far too much memory on the PSX */
#endif

#define UNITS_PER_PLAYER	5

/* TC said I could do this - GJ */
#ifdef WIN32
#define MAX_NAME_SIZE		60
#else
#define MAX_NAME_SIZE		40
#endif

#define MAX_STR_SIZE		20
#define	MAX_STR_LENGTH		256


#ifdef WIN32
#define UNUSEDPARAMETER(p) p
#else
//#define UNUSEDPARAMETER(var) static void *const use_##var = (&use_##var, &var, 0)
#define UNUSEDPARAMETER(var) (void)var


#endif


//used to flag for some script code whilst work in progress
#define SCRIPTS

#endif

