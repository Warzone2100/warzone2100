/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2007  Warzone Resurrection Project

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
/*
 * Bullet.h
 *
 * Structure definitions for bullets.  A "bullet" is created whenever a
 * weapon is fired, and stores the visual effects and delayed damage etc of
 * a weapon.
 *
 */
#ifndef _bullet_h
#define _bullet_h

#include "objectdef.h"

/* The active bullets */
extern PROJ_OBJECT *psActiveBullets;

//used for passing data to the checkBurnDamage function
typedef struct _fire_box
{
	SWORD	x1, y1;
	SWORD	x2, y2;
	SWORD	rad;
} FIRE_BOX;


/* Initilise the bullet system */
extern BOOL bulletInitialise(void);

/* Shut Down the bullet system */
extern void bulletShutdown(void);

/* Return an empty bullet structure */
extern BOOL getBullet(PROJ_OBJECT **ppsBullet);

/* Release a bullet structure for later reuse */
extern void releaseBullet(PROJ_OBJECT *psBullet);

/* Release all the bullets in the game */
extern void freeAllBullets(void);

/* Update the current bullet */
extern void updateBullet(PROJ_OBJECT *psBullet);

/*Apply the damage to an object due to fire range*/
extern void checkBurnDamage(BASE_OBJECT* apsList, PROJ_OBJECT* psBullet, FIRE_BOX* pFireBox);


#endif

