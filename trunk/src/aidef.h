/*
 * AIDef.h
 *
 * Structure definitions for the AI system
 *
 */
#ifndef _aidef_h
#define _aidef_h

typedef enum _ai_state
{
	AI_PAUSE,			// do no ai
	AI_ATTACK,			// attacking a target
	AI_MOVETOTARGET,	// moving to a target
	AI_MOVETOLOC,		// moving to a location
	AI_MOVETOSTRUCT,	// moving to a structure to build
	AI_BUILD,			// build a Structure
} AI_STATE;

typedef struct _ai_data
{
	AI_STATE				state;
	struct _base_object		*psTarget;
/*	removed 27.2.98		john.
	struct _weapon			*psSelectedWeapon;
	struct _structure_stats	*psStructToBuild;
	UDWORD					timeStarted;				//time a function was started -eg build, produce
	//UDWORD					pointsToAdd;		NOT USED AT PRESENT 13/08/97
	UDWORD					structX, structY;			// location of StructToBuild
	UDWORD					moveX,moveY;				// location target for movement	
	struct _droid			*psGroup;					// The list pointer for the droids group
	*/
} AI_DATA;


#endif

