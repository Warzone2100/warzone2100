/*
 * CmdDroidDef.h
 *
 * Typedef's for command droids
 *
 */
#ifndef _cmddroiddef_h
#define _cmddroiddef_h

// the maximum number of command droids per side
#define MAX_CMDDROIDS	5

typedef struct _command_droid
{
	COMPONENT_STATS;		// define the command droid as a COMPONENT
							// so it fits into the design screen

	UDWORD			died;
	SWORD			aggression;
	SWORD			survival;
	SWORD			nWeapStat;
	UWORD			kills;
	struct _droid	*psDroid;
} COMMAND_DROID;


#endif


