/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2009  Warzone Resurrection Project

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

#ifdef __cplusplus
extern "C"
{
#endif //__cplusplus

/* Research struct type definitions */
typedef enum
{
	TC_MAJOR,
	TC_MINOR,
} TECH_CODE;

typedef struct research_stats
{
	STATS_BASE;
	UBYTE			techCode;
	//char			*pTechnologyName;	/* Text name of the group the research is a
	//									   member of */
//	char			*pSubGroupName;		/* Text name of the Subgroup to which the research belongs */
	UWORD       	subGroup;			/* Subgroup of the item - an iconID from 'Framer' to depict in the button*/

	UWORD			researchPoints;		/* Number of research points required to
										   complete the research */
	UDWORD			researchPower;		/* Power cost to research */
	UBYTE			keyTopic;			/* Flag to indicate whether in single player
										   this topic must be explicitly enabled*/
	UBYTE			storeCount;			/* used to load in the following lists*/
	UBYTE			numPRRequired;
	//UDWORD			*pPRList;			/* List of research pre-requisites */
	//needs to be UWORD sized for Patches
	UWORD			*pPRList;			/* List of research pre-requisites */
	//UBYTE			*pPRList;			/* List of research pre-requisites */
	UBYTE			numStructures;
	//UDWORD			*pStructList;		/* List of structures that when built would
	UWORD			*pStructList;		/* List of structures that when built would
										   enable this research */
	UBYTE			numFunctions;
	struct _function	**pFunctionList; /* List of functions that can be performed
										   on completion of research */
	UBYTE			numRedStructs;
	//UDWORD			*pRedStructs;		/* List of Structures that become redundant */
	UWORD			*pRedStructs;		/* List of Structures that become redundant */
	UBYTE			numRedArtefacts;
	COMPONENT_STATS	**pRedArtefacts;	/*List of Artefacts that become redundant */
	UBYTE			numStructResults;
	//UDWORD			*pStructureResults;	/*List of Structures that are possible after
	UWORD			*pStructureResults;	/*List of Structures that are possible after
										  this research */
	UBYTE			numArteResults;
	COMPONENT_STATS	**pArtefactResults;	/*List of Artefacts that are possible after
										  this research*/
	COMPONENT_STATS	**pReplacedArtefacts;/*List of artefacts that are replaced by the above
										  result - not necessarily any! 1 to 1 relation with
										  above list */
	struct _viewdata	*pViewData;		/*data used to display a message in the
										  Intelligence Screen*/
	UWORD			iconID;				/* the ID from 'Framer' for which graphic to draw in interface*/
	BASE_STATS      *psStat;            /* A stat used to define which graphic is
	                                       drawn instead of the two fields below*/
	iIMDShape		*pIMD;		/* the IMD to draw for this research topic */
	iIMDShape		*pIMD2;		/* the 2nd IMD for base plates/turrets*/
} WZ_DECL_MAY_ALIAS RESEARCH;

typedef struct _player_research
{


	UDWORD		currentPoints;			// If the research has been suspended then this value contains the number of points generated at the suspension/cancel point
										// normally it is null

	UBYTE		ResearchStatus;			// Bit flags   ...  see below

	bool            possible;                       ///< is the research possible ... so can enable topics vis scripts


//	UBYTE		possible;				/* Flag to specify whether the research is possible - so
//										   can enable topics vis scripts */
//	UBYTE		researched;				/* Flag to specify whether the research is
//										   complete	*/
} PLAYER_RESEARCH;

#define STARTED_RESEARCH	0x01		// research in progress
#define CANCELLED_RESEARCH	0x02		// research has been canceled
#define RESEARCHED			0x04		// research is complete
#define RESBITS (STARTED_RESEARCH|CANCELLED_RESEARCH|RESEARCHED)

static inline bool IsResearchPossible(const PLAYER_RESEARCH* research)
{
	return research->possible;
}

static inline void MakeResearchPossible(PLAYER_RESEARCH* research)
{
	research->possible = true;
}

#define IsResearchCompleted(x)   ((x)->ResearchStatus&RESEARCHED)

#define MakeResearchCompleted(x)	((x)->ResearchStatus=((x)->ResearchStatus&(~RESBITS))|RESEARCHED  )

#define IsResearchCancelled(x)   ((x)->ResearchStatus&CANCELLED_RESEARCH)
#define MakeResearchCancelled(x)	((x)->ResearchStatus=((x)->ResearchStatus&(~RESBITS))|CANCELLED_RESEARCH)

#define IsResearchStarted(x)   ((x)->ResearchStatus&STARTED_RESEARCH)
#define MakeResearchStarted(x)	((x)->ResearchStatus=((x)->ResearchStatus&(~RESBITS))|STARTED_RESEARCH  )

// clear all bits in the status except for the possible bit
#define ResetResearchStatus(x) ((x)->ResearchStatus&=(~RESBITS))

#ifdef __cplusplus
}
#endif //__cplusplus

#endif // __INCLUDED_RESEARCHDEF_H__
