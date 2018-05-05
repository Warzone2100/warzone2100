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
/** \file
 *  Definitions for research data.
 */

#ifndef __INCLUDED_RESEARCHDEF_H__
#define __INCLUDED_RESEARCHDEF_H__

#include "lib/framework/frame.h"
#include "statsdef.h"
#include "lib/framework/wzconfig.h"

struct VIEWDATA;

/* Research struct type definitions */
enum TECH_CODE
{
	TC_MAJOR,
	TC_MINOR,
};

struct RES_COMP_REPLACEMENT
{
	COMPONENT_STATS *pOldComponent;
	COMPONENT_STATS *pNewComponent;
};

struct RESEARCH : public BASE_STATS
{
	UBYTE			techCode;
	UWORD       	subGroup;			/* Subgroup of the item - an iconID from 'Framer' to depict in the button*/

	UWORD			researchPoints;		/* Number of research points required to
										   complete the research */
	UDWORD			researchPower;		/* Power cost to research */
	UBYTE			keyTopic;			/* Flag to indicate whether in single player
										   this topic must be explicitly enabled*/
	std::vector<UWORD>	pPRList;		///< List of research pre-requisites
	std::vector<UWORD>	pStructList;		///< List of structures that when built would enable this research
	std::vector<UWORD>	pRedStructs;		///< List of Structures that become redundant
	std::vector<COMPONENT_STATS *> pRedArtefacts;	///< List of Artefacts that become redundant
	std::vector<UWORD>	pStructureResults;	///< List of Structures that are possible after this research
	std::vector<COMPONENT_STATS *> componentResults;	///< List of Components that are possible after this research
	std::vector<RES_COMP_REPLACEMENT> componentReplacement;	///< List of Components that are automatically replaced with new onew after research
	nlohmann::json	results;		///< Research upgrades
	VIEWDATA                *pViewData;             ///< Data used to display a message in the Intelligence Screen
	UWORD			iconID;				/* the ID from 'Framer' for which graphic to draw in interface*/
	BASE_STATS      *psStat;   /* A stat used to define which graphic is drawn instead of the two fields below */
	iIMDShape		*pIMD;		/* the IMD to draw for this research topic */
	iIMDShape		*pIMD2;		/* the 2nd IMD for base plates/turrets*/
	int index;		///< Unique index for this research, set incrementally

	RESEARCH() : pViewData(nullptr), iconID(0), psStat(nullptr), pIMD(nullptr), pIMD2(nullptr) {}
};

struct PLAYER_RESEARCH
{
	UDWORD		currentPoints;			// If the research has been suspended then this value contains the number of points generated at the suspension/cancel point
	// normally it is null

	UBYTE		ResearchStatus;			// Bit flags   ...  see below

	bool            possible;                       ///< is the research possible ... so can enable topics vis scripts
};

#define STARTED_RESEARCH           0x01            // research in progress
#define CANCELLED_RESEARCH         0x02            // research has been canceled
#define RESEARCHED                 0x04            // research is complete
#define CANCELLED_RESEARCH_PENDING 0x08            // research almost cancelled, waiting for GAME_RESEARCHSTATUS message to be processed.
#define STARTED_RESEARCH_PENDING   0x10            // research almost in progress, waiting for GAME_RESEARCHSTATUS message to be processed.
#define RESBITS (STARTED_RESEARCH|CANCELLED_RESEARCH|RESEARCHED)
#define RESBITS_PENDING_ONLY (STARTED_RESEARCH_PENDING|CANCELLED_RESEARCH_PENDING)
#define RESBITS_PENDING (RESBITS|RESBITS_PENDING_ONLY)

static inline bool IsResearchPossible(const PLAYER_RESEARCH *research)
{
	return research->possible;
}

static inline void MakeResearchPossible(PLAYER_RESEARCH *research)
{
	research->possible = true;
}

static inline bool IsResearchCompleted(PLAYER_RESEARCH const *x)
{
	return (x->ResearchStatus & RESEARCHED)                                                != 0;
}
static inline bool IsResearchCancelled(PLAYER_RESEARCH const *x)
{
	return (x->ResearchStatus & CANCELLED_RESEARCH)                                        != 0;
}
static inline bool IsResearchStarted(PLAYER_RESEARCH const *x)
{
	return (x->ResearchStatus & STARTED_RESEARCH)                                          != 0;
}
/// Pending means not yet synchronised, so only permitted to affect the UI, not the game state.
static inline bool IsResearchCancelledPending(PLAYER_RESEARCH const *x)
{
	return (x->ResearchStatus & RESBITS_PENDING_ONLY)                                      != 0 ?
	       (x->ResearchStatus & CANCELLED_RESEARCH_PENDING)                                != 0 :
	       (x->ResearchStatus & CANCELLED_RESEARCH)                                        != 0;
}
static inline bool IsResearchStartedPending(PLAYER_RESEARCH const *x)
{
	return (x->ResearchStatus & RESBITS_PENDING_ONLY)                                      != 0 ?
	       (x->ResearchStatus & STARTED_RESEARCH_PENDING)                                  != 0 :
	       (x->ResearchStatus & STARTED_RESEARCH)                                          != 0;
}

static inline void MakeResearchCompleted(PLAYER_RESEARCH *x)
{
	x->ResearchStatus &= ~RESBITS_PENDING;
	x->ResearchStatus |= RESEARCHED;
}
static inline void MakeResearchCancelled(PLAYER_RESEARCH *x)
{
	x->ResearchStatus &= ~RESBITS_PENDING;
	x->ResearchStatus |= CANCELLED_RESEARCH;
}
static inline void MakeResearchStarted(PLAYER_RESEARCH *x)
{
	x->ResearchStatus &= ~RESBITS_PENDING;
	x->ResearchStatus |= STARTED_RESEARCH;
}
/// Pending means not yet synchronised, so only permitted to affect the UI, not the game state.
static inline void MakeResearchCancelledPending(PLAYER_RESEARCH *x)
{
	x->ResearchStatus &= ~RESBITS_PENDING_ONLY;
	x->ResearchStatus |= CANCELLED_RESEARCH_PENDING;
}
static inline void MakeResearchStartedPending(PLAYER_RESEARCH *x)
{
	x->ResearchStatus &= ~RESBITS_PENDING_ONLY;
	x->ResearchStatus |= STARTED_RESEARCH_PENDING;
}
static inline void ResetPendingResearchStatus(PLAYER_RESEARCH *x)
{
	x->ResearchStatus &= ~RESBITS_PENDING_ONLY;
}

/// clear all bits in the status except for the possible bit
static inline void ResetResearchStatus(PLAYER_RESEARCH *x)
{
	x->ResearchStatus &= ~RESBITS_PENDING;
}

#endif // __INCLUDED_RESEARCHDEF_H__
