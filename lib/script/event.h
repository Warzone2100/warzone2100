/*
 * Event.h
 *
 * Interface to the event management system.
 *
 */
#ifndef _event_h
#define _event_h

/* The number of values in a context value chunk */
#define CONTEXT_VALS 20

/* One chunk of variables for a script context */
typedef struct _val_chunk
{
	INTERP_VAL		asVals[CONTEXT_VALS];

	struct _val_chunk *psNext;
} VAL_CHUNK;


/* The number of links in a context event link chunk */
#define CONTEXT_LINKS	10
/* One chunk of event links for a script context */
typedef struct _link_chunk
{
	SWORD			aLinks[CONTEXT_LINKS];

	struct _link_chunk *psNext;
} LINK_CHUNK;


/* The data needed within an object to run a script */
typedef struct _script_context
{
	SCRIPT_CODE		*psCode;		// The actual script to run
	VAL_CHUNK		*psGlobals;		// The objects copy of the global variables
	SDWORD			triggerCount;	// Number of currently active triggers
	SWORD			release;		// Whether to release the context when there are no triggers
	SWORD			id;

	struct _script_context *psNext;
} SCRIPT_CONTEXT;

// The Event initialisation data
typedef struct _event_init
{
	UWORD		valInit, valExt;	// value chunk init values
	UWORD		trigInit, trigExt;	// trigger chunk init values
	UWORD		contInit, contExt;	// context chunk init values
} EVENT_INIT;

/*
 * A currently active trigger.
 * If the type of the triggger == TR_PAUSE, the trigger number stored is the
 * index of the trigger to replace this one when the event restarts
 */
typedef struct _active_trigger
{
	UDWORD				testTime;
	SCRIPT_CONTEXT		*psContext;
	SWORD				type;			// enum - TRIGGER_TYPE
	SWORD				trigger;
	UWORD				event;
	UWORD				offset;

	struct _active_trigger *psNext;
} ACTIVE_TRIGGER;


// The list of currently active triggers
extern ACTIVE_TRIGGER	*psTrigList;

// The list of callback triggers
extern ACTIVE_TRIGGER	*psCallbackList;

// The currently allocated contexts
extern SCRIPT_CONTEXT	*psContList;


/* Initialise the event system */
extern BOOL eventInitialise(EVENT_INIT *psInit);

// Shutdown the event system
extern void eventShutDown(void);

// add a TR_PAUSE trigger to the event system.
extern BOOL eventAddPauseTrigger(SCRIPT_CONTEXT *psContext, UDWORD event, UDWORD offset,
						  UDWORD time);

// Load a trigger into the system from a save game
extern BOOL eventLoadTrigger(UDWORD time, SCRIPT_CONTEXT *psContext,
					  SDWORD type, SDWORD trigger, UDWORD event, UDWORD offset);

//resets the event timer - updateTime
extern void eventTimeReset(UDWORD initTime);

#endif

