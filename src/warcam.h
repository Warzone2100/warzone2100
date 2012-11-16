/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2012  Warzone 2100 Project

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

#ifndef __INCLUDED_SRC_WARCAM_H__
#define __INCLUDED_SRC_WARCAM_H__

#include "lib/ivis_opengl/pietypes.h"
#include "objectdef.h"

#define X_UPDATE 0x1
#define Y_UPDATE 0x2
#define Z_UPDATE 0x4

#define CAM_X_AND_Y	(X_UPDATE + Y_UPDATE)

#define CAM_ALL (X_UPDATE + Y_UPDATE + Z_UPDATE)

#define ACCEL_CONSTANT 12.0f
#define VELOCITY_CONSTANT 4.0f
#define ROT_ACCEL_CONSTANT 4.0f
#define ROT_VELOCITY_CONSTANT 2.5f

/* The different tracking states */
enum WARSTATUS
{
CAM_INACTIVE,
CAM_REQUEST,
CAM_TRACKING,
CAM_RESET,
CAM_TRACK_OBJECT,
CAM_TRACK_LOCATION
};

/* Storage for old viewnagles etc */
struct WARCAM
{
	WARSTATUS status;
	UDWORD trackClass;
	UDWORD lastUpdate;
	iView oldView;

	Vector3f acceleration;
	Vector3f velocity;
	Vector3f position;

	Vector3f rotation;
	Vector3f rotVel;
	Vector3f rotAccel;

	UDWORD oldDistance;
	BASE_OBJECT *target;
};

/* Externally referenced functions */
extern void	initWarCam			( void );
extern void	setWarCamActive		( bool status );
extern bool	getWarCamStatus		( void );
extern void camToggleStatus		( void );
extern bool processWarCam		( void );
extern void	camToggleInfo		( void );
extern void	requestRadarTrack	( SDWORD x, SDWORD y );
extern bool	getRadarTrackingStatus( void );
extern void	toggleRadarAllignment( void );
extern void	camInformOfRotation ( Vector3i *rotation );
extern BASE_OBJECT *camFindDroidTarget(void);
extern DROID *getTrackingDroid( void );
extern SDWORD	getPresAngle( void );
extern UDWORD	getNumDroidsSelected( void );
extern void	camAllignWithTarget(BASE_OBJECT *psTarget);

#endif // __INCLUDED_SRC_WARCAM_H__
