/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2017  Warzone 2100 Project

	Warzone 2100 is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	Warzone 2100 is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Warzone 2100; if not, write to the Free Software
	Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
*/
/** @file
 *  Functions for the display/functionality of the Transporter
 */

#ifndef __INCLUDED_SRC_TRANSPORTER_H__
#define __INCLUDED_SRC_TRANSPORTER_H__

#include "lib/widget/widget.h"

#define IDTRANS_FORM			9000	//The Transporter base form
#define IDTRANS_CONTENTFORM		9003	//The Transporter Contents form
#define IDTRANS_DROIDS			9006	//The Droid base form
#define IDTRANS_LAUNCH			9010	//The Transporter Launch button
#define	IDTRANS_CAPACITY		9500	//The Transporter capacity label

//defines how much space is on the Transporter
#define TRANSPORTER_CAPACITY		10

/// how much cargo capacity a droid takes up
int transporterSpaceRequired(const DROID *psDroid);

//initialises Transporter variables
extern void initTransporters(void);
// Refresh the transporter screen.
extern bool intRefreshTransporter(void);
/*Add the Transporter Interface*/
extern bool intAddTransporter(DROID *psSelected, bool offWorld);
/* Remove the Transporter widgets from the screen */
extern void intRemoveTrans(void);
extern void intRemoveTransNoAnim(void);
/* Process return codes from the Transporter Screen*/
extern void intProcessTransporter(UDWORD id);

/*Adds a droid to the transporter, removing it from the world*/
extern void transporterAddDroid(DROID *psTransporter, DROID *psDroidToAdd);
void transporterRemoveDroid(DROID *psTransport, DROID *psDroid, QUEUE_MODE mode);
/*check to see if the droid can fit on the Transporter - return true if fits*/
bool checkTransporterSpace(DROID const *psTransporter, DROID const *psAssigned, bool mayFlash = true);
/*calculates how much space is remaining on the transporter - allows droids to take
up different amount depending on their body size - currently all are set to one!*/
int calcRemainingCapacity(const DROID *psTransporter);

extern bool transporterIsEmpty(const DROID *psTransporter);

/*launches the defined transporter to the offworld map*/
extern bool launchTransporter(DROID *psTransporter);

/*checks how long the transporter has been travelling to see if it should
have arrived - returns true when there*/
extern bool updateTransporter(DROID *psTransporter);

extern void intUpdateTransCapacity(WIDGET *psWidget, W_CONTEXT *psContext);

/* Remove the Transporter Launch widget from the screen*/
extern void intRemoveTransporterLaunch(void);

//process the launch transporter button click
extern void processLaunchTransporter(void);

extern SDWORD	bobTransporterHeight(void);

/*This is used to display the transporter button and capacity when at the home base ONLY*/
extern bool intAddTransporterLaunch(DROID *psDroid);

/* set current transporter (for script callbacks) */
extern void transporterSetScriptCurrent(DROID *psTransporter);

/* get current transporter (for script callbacks) */
extern DROID *transporterGetScriptCurrent(void);

/*called when a Transporter has arrived back at the LZ when sending droids to safety*/
extern void resetTransporter(void);

/* get time transporter launch button was pressed */
extern UDWORD transporterGetLaunchTime(void);

/*set the time for the Launch*/
extern void transporterSetLaunchTime(UDWORD time);

extern void flashMissionButton(UDWORD buttonID);
extern void stopMissionButtonFlash(UDWORD buttonID);
/*checks the order of the droid to see if its currenly flying*/
extern bool transporterFlying(DROID *psTransporter);
//initialise the flag to indicate the first transporter has arrived - set in startMission()
extern void initFirstTransporterFlag(void);

#endif // __INCLUDED_SRC_TRANSPORTER_H__
