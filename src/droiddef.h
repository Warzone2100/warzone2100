/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2010  Warzone 2100 Project

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
/** \file
 *  Definitions for droids.
 */

#ifndef __INCLUDED_DROIDDEF_H__
#define __INCLUDED_DROIDDEF_H__

#include "lib/gamelib/animobj.h"

#include "stringdef.h"
#include "actiondef.h"
#include "basedef.h"
#include "movedef.h"
#include "orderdef.h"
#include "statsdef.h"
#include "weapondef.h"

#ifdef __cplusplus
extern "C"
{
#endif //__cplusplus

/*!
 * The number of components in the asParts / asBits arrays.
 * Weapons are stored seperately, thus the maximum index into the array
 * is 1 smaller than the number of components.
 */
#define DROID_MAXCOMP (COMP_NUMCOMPONENTS - 1)

/* The maximum number of droid weapons */
#define DROID_MAXWEAPS		3
#define	DROID_DAMAGE_SCALING	400
// This should really be logarithmic
#define	CALC_DROID_SMOKE_INTERVAL(x) ((((100-x)+10)/10) * DROID_DAMAGE_SCALING)

//defines how many times to perform the iteration on looking for a blank location
#define LOOK_FOR_EMPTY_TILE		20
//used to get a location next to a droid - withinh one tile
#define LOOK_NEXT_TO_DROID		8


/* The different types of droid */
// NOTE, if you add to, or change this list then you'll need
// to update the DroidSelectionWeights lookup table in Display.c
typedef enum _droid_type
{
	DROID_WEAPON,           ///< Weapon droid
	DROID_SENSOR,           ///< Sensor droid
	DROID_ECM,              ///< ECM droid
	DROID_CONSTRUCT,        ///< Constructor droid
	DROID_PERSON,           ///< person
	DROID_CYBORG,           ///< cyborg-type thang
	DROID_TRANSPORTER,      ///< guess what this is!
	DROID_COMMAND,          ///< Command droid
	DROID_REPAIR,           ///< Repair droid
	DROID_DEFAULT,          ///< Default droid
	DROID_CYBORG_CONSTRUCT, ///< cyborg constructor droid - new for update 28/5/99
	DROID_CYBORG_REPAIR,    ///< cyborg repair droid - new for update 28/5/99
	DROID_CYBORG_SUPER,     ///< cyborg repair droid - new for update 7/6/99
	DROID_ANY,              ///< Any droid. Only used as a parameter for intGotoNextDroidType(DROID_TYPE).
} DROID_TYPE;

typedef struct _component
{
	UBYTE           nStat;          ///< Allowing a maximum of 255 stats per file
} COMPONENT;

// maximum number of queued orders
#define ORDER_LIST_MAX		10

typedef struct _order_list
{
	DROID_ORDER     order;
	void*           psOrderTarget;  ///< this needs to cope with objects and stats
	UWORD           x, y, x2, y2;   ///< line build requires two sets of coords
	uint16_t        direction;      ///< Needed to align structures with viewport.
} ORDER_LIST;

typedef struct _droid_template
{
	// On the PC the pName entry in STATS_BASE is redundant and can be assumed to be NULL,
	STATS_BASE;						/* basic stats */

	/// on the PC this contains the full editable UTF-8 encoded name of the template
	char            aName[MAX_STR_LENGTH];

	UBYTE           NameVersion;                //< Version number used in name (e.g. Viper Mk "I" would be stored as 1 - Viper Mk "X" as 10)

	/*!
	 * The droid components.
	 *
	 * This array is indexed by COMPONENT_TYPE so the ECM would be accessed
	 * using asParts[COMP_ECM].
	 * COMP_BRAIN is an index into the asCommandDroids array NOT asBrainStats
	 *
	 * Weapons are stored in asWeaps, _not_ here at index COMP_WEAPON! (Which is the reason we do not have a COMP_NUMCOMPONENTS sized array here.)
	 */
	SDWORD          asParts[DROID_MAXCOMP];

	UDWORD          buildPoints;                ///< total build points required to manufacture the droid
	UDWORD          powerPoints;                ///< total power points required to build/maintain the droid
	UDWORD          storeCount;                 ///< used to load in weaps and progs

	/* The weapon systems */
	UDWORD          numWeaps;                   ///< Number of weapons
	UDWORD          asWeaps[DROID_MAXWEAPS];    ///< weapon indices

	DROID_TYPE      droidType;                  ///< The type of droid
	UDWORD          multiPlayerID;              ///< multiplayer unique descriptor(cant use id's for templates). Used for save games as well now - AB 29/10/98
	struct _droid_template* psNext;             ///< Pointer to next template
	bool		prefab;                     ///< Not player designed, not saved, never delete or change
} WZ_DECL_MAY_ALIAS DROID_TEMPLATE;

typedef struct DROID
{
	/* The common structure elements for all objects */
	BASE_ELEMENTS(struct DROID);

	/// UTF-8 name of the droid. This is generated from the droid template and cannot be changed by the game player after creation.
	char            aName[MAX_STR_LENGTH];

	DROID_TYPE      droidType;                      ///< The type of droid

	/** Holds the specifics for the component parts - allows damage
	 *  per part to be calculated. Indexed by COMPONENT_TYPE.
	 *  Weapons need to be dealt with separately.
	 *  COMP_BRAIN is an index into the asCommandDroids array NOT asBrainStats
	 */
	COMPONENT       asBits[DROID_MAXCOMP];

	/* The other droid data.  These are all derived from the components
	 * but stored here for easy access
	 */
	UDWORD          weight;
	UDWORD          baseSpeed;                      ///< the base speed dependant on propulsion type
	UDWORD          originalBody;                   ///< the original body points
	uint32_t        experience;
	UBYTE           NameVersion;                    ///< Version number used for generating on-the-fly names (e.g. Viper Mk "I" would be stored as 1 - Viper Mk "X" as 10)  - copied from droid template

	int		lastFrustratedTime;		///< Set when eg being stuck; used for eg firing indiscriminately at map features to clear the way (note: signed, so wrap arounds after 24.9 days)

	SWORD           resistance;                     ///< used in Electronic Warfare

	UDWORD          numWeaps;                       ///< Watermelon:Re-enabled this,I need this one in droid.c
	WEAPON          asWeaps[DROID_MAXWEAPS];

	// The group the droid belongs to
	struct _droid_group* psGroup;
	struct DROID*  psGrpNext;
	struct _structure* psBaseStruct;                ///< a structure that this droid might be associated with. For VTOLs this is the rearming pad
	// queued orders
	SDWORD          listSize;
	ORDER_LIST      asOrderList[ORDER_LIST_MAX];    ///< The range [0; listSize - 1] corresponds to synchronised orders, and the range [listPendingBegin; listPendingEnd - 1] corresponds to the orders that will remain, once all orders are synchronised.
	unsigned        listPendingBegin;               ///< Index of first order which will not be erased by a pending order. After all messages are processed, the orders in the range [listPendingBegin; listPendingEnd - 1] will remain.
	unsigned        listPendingEnd;                 ///< Index of last order which might not yet have been synchronised, plus one.

	/* Order data */
	DROID_ORDER     order;
	UWORD           orderX, orderY;
	UWORD           orderX2, orderY2;
	uint16_t        orderDirection;

	BASE_OBJECT*    psTarget;                       ///< Order target
	BASE_STATS*     psTarStats;                     ///< What to build etc
#ifdef DEBUG
	// these are to help tracking down dangling pointers
	char            targetFunc[MAX_EVENT_NAME_LEN];
	int             targetLine;
	char            actionTargetFunc[DROID_MAXWEAPS][MAX_EVENT_NAME_LEN];
	int             actionTargetLine[DROID_MAXWEAPS];
	char            baseFunc[MAX_EVENT_NAME_LEN];
	int             baseLine;
#endif

	// secondary order data
	UDWORD          secondaryOrder;

	/* Action data */
	DROID_ACTION    action;
	UDWORD          actionX, actionY;
	BASE_OBJECT*    psActionTarget[DROID_MAXWEAPS]; ///< Action target object
	UDWORD          actionStarted;                  ///< Game time action started
	UDWORD          actionPoints;                   ///< number of points done by action since start

	UDWORD          expectedDamage;                 ///< Expected damage to be caused by all currently incoming projectiles. This info is shared between all players,
	                                                ///< but shouldn't make a difference unless 3 mutual enemies happen to be fighting each other at the same time.

	UBYTE           illumination;

	/* Movement control data */
	MOVE_CONTROL    sMove;
	SPACETIME       prevSpacetime;                  ///< Location of droid in previous tick.
	uint8_t		blockedBits;			///< Bit set telling which tiles block this type of droid (TODO)

	/* anim data */
	ANIM_OBJECT     *psCurAnim;
	SDWORD          iAudioID;

	// Synch checking
	void *          gameCheckDroid;                 ///< Last PACKAGED_CHECK, for synchronisation use only (see multisync.c). TODO Make synch perfect, so that this isn't needed at all.
} WZ_DECL_MAY_ALIAS DROID;

#ifdef __cplusplus
}
#endif //__cplusplus

#endif // __INCLUDED_DROIDDEF_H__
