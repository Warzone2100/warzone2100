/*
 * Transporter.h
 *
 * Functions for the display/functionality of the Transporter
 */

#ifndef _transporter_h
#define _transporter_h

#include "widget.h"

#define IDTRANS_FORM			9000	//The Transporter base form
#define IDTRANS_CONTENTFORM		9003	//The Transporter Contents form
#define IDTRANS_DROIDS			9006	//The Droid base form
#define IDTRANS_LAUNCH			9010	//The Transporter Launch button
#define	IDTRANS_CAPACITY		9500	//The Transporter capacity label

//initialises Transporter variables
extern void initTransporters(void);
// Refresh the transporter screen.
extern BOOL intRefreshTransporter(void);
/*Add the Transporter Interface*/
extern BOOL intAddTransporter(DROID *psSelected, BOOL offWorld);
/* Remove the Transporter widgets from the screen */
extern void intRemoveTrans(void);
extern void intRemoveTransNoAnim(void);
/* Process return codes from the Transporter Screen*/
extern void intProcessTransporter(UDWORD id);

/*Adds a droid to the transporter, removing it from the world*/
extern void transporterAddDroid(DROID *psTransporter, DROID *psDroidToAdd);
/*check to see if the droid can fit on the Transporter - return TRUE if fits*/
extern BOOL checkTransporterSpace(DROID *psTransporter, DROID *psAssigned);
/*calculates how much space is remaining on the transporter - allows droids to take 
up different amount depending on their body size - currently all are set to one!*/
extern UDWORD calcRemainingCapacity(DROID *psTransporter);

/*launches the defined transporter to the offworld map*/
extern BOOL launchTransporter(DROID *psTransporter);

/*checks how long the transporter has been travelling to see if it should 
have arrived - returns TRUE when there*/
extern BOOL updateTransporter(DROID *psTransporter);

// Order all selected droids to embark all avaialable transporters.
extern BOOL OrderDroidsToEmbark(void);
// Order a single droid to embark any available transporters.
extern BOOL OrderDroidToEmbark(DROID *psDroid);

extern void intUpdateTransCapacity(struct _widget *psWidget, struct _w_context *psContext);

/* Remove the Transporter Launch widget from the screen*/
extern void intRemoveTransporterLaunch(void);

//process the launch transporter button click
extern void processLaunchTransporter(void);

extern SDWORD	bobTransporterHeight( void );

/*This is used to display the transporter button and capacity when at the home base ONLY*/
extern BOOL intAddTransporterLaunch(DROID *psDroid);

/* set current transporter (for script callbacks) */
extern void transporterSetScriptCurrent( DROID *psTransporter );

/* get current transporter (for script callbacks) */
extern DROID * transporterGetScriptCurrent( void );

/* check whether transporter on mission */
//extern BOOL transporterOnMission( void );

/*called when a Transporter has arrived back at the LZ when sending droids to safety*/
extern void resetTransporter(DROID *psTransporter);

/* get time transporter launch button was pressed */
extern UDWORD transporterGetLaunchTime( void );

/*set the time for the Launch*/
extern void transporterSetLaunchTime(UDWORD time);

extern void flashMissionButton(UDWORD buttonID);
extern void stopMissionButtonFlash(UDWORD buttonID);
/*checks the order of the droid to see if its currenly flying*/
extern BOOL transporterFlying(DROID *psTransporter);
//initialise the flag to indicate the first transporter has arrived - set in startMission()
extern void initFirstTransporterFlag(void);

#endif
