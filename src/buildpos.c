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
 * BuildPos.c
 *
 * Find a position for a structure to be built
 *
 */

#if 0

#include "frame.h"
#include "objects.h"
#include "map.h"
#include "cluster.h"

#include "buildpos.h"


// the directions for placing structures
enum
{
	BPD_TOP,
	BPD_BOTTOM,
	BPD_LEFT,
	BPD_RIGHT,

	BPD_MAX,
};


// find a clear location for a new structure next to another one
BOOL bposGetLocByStruct(STRUCTURE *psStruct, STRUCTURE_STATS *psStats,
						SDWORD centerx, SDWORD centery, SDWORD *px, SDWORD *py)
{
	RECT	sStructPos;
	SDWORD	xdiff,ydiff;
	SDWORD	i, dir, chosendir, maxdist, currdist;
	BOOL	done[BPD_MAX];

	// find the tiles the structure covers
	sStructPos.top = (SDWORD)psStruct->y -
			((SDWORD)psStruct->pStructureType->baseBreadth * TILE_UNITS)/2;
	sStructPos.bottom = sStructPos.top +
			(SDWORD)psStruct->pStructureType->baseBreadth * TILE_UNITS - 1;
	sStructPos.left = (SDWORD)psStruct->x -
			((SDWORD)psStruct->pStructureType->baseWidth * TILE_UNITS)/2;
	sStructPos.right = sStructPos.left +
			(SDWORD)psStruct->pStructureType->baseWidth * TILE_UNITS - 1;

	// try and find a valid location for the new structure next to this one
	// loop round the four possible directions choosing the one nearest to the center first
	memset(done, 0, sizeof(done));
	for(i=0; i<BPD_MAX; i++)
	{
		// find the direction which hasn't been tried yet that is nearest to the center
		chosendir = BPD_MAX;
		maxdist = SDWORD_MAX;
		for (dir=0; dir<BPD_MAX; dir++)
		{
			if (!done[dir])
			{
				switch (dir)
				{
				case BPD_TOP:
					xdiff = centerx - (((sStructPos.right - sStructPos.left)/2) + sStructPos.left);
					ydiff = centery - sStructPos.top;
					break;
				case BPD_LEFT:
					xdiff = centerx - sStructPos.left;
					ydiff = centery - (((sStructPos.bottom - sStructPos.top)/2) + sStructPos.top);
					break;
				case BPD_RIGHT:
					xdiff = centerx - sStructPos.right;
					ydiff = centery - (((sStructPos.bottom - sStructPos.top)/2) + sStructPos.top);
					break;
				case BPD_BOTTOM:
					xdiff = centerx - (((sStructPos.right - sStructPos.left)/2) + sStructPos.left);
					ydiff = centery - sStructPos.bottom;
					break;
				}
				currdist = xdiff*xdiff + ydiff*ydiff;
				if (currdist < maxdist)
				{
					chosendir = dir;
					maxdist = currdist;
				}
			}
		}
		done[chosendir] = TRUE;

		// now try placing the structure at the chosen position
		switch (chosendir)
		{
		case BPD_TOP:
			// above
			*px = sStructPos.left >> TILE_SHIFT;
			*py = (sStructPos.top >> TILE_SHIFT) - (SDWORD)psStats->baseBreadth - 1;
			if (validLocation((BASE_STATS *)psStats, (UDWORD)*px, (UDWORD) *py))
			{
				return TRUE;
			}
			break;

		case BPD_LEFT:
			// left
			*px = (sStructPos.left >> TILE_SHIFT) - (SDWORD)psStats->baseWidth - 1;
			*py = sStructPos.top >> TILE_SHIFT;
			if (validLocation((BASE_STATS *)psStats, (UDWORD)*px, (UDWORD) *py))
			{
				return TRUE;
			}
			break;

		case BPD_RIGHT:
			// right
			*px = (sStructPos.right >> TILE_SHIFT) + 2;
			*py = sStructPos.top >> TILE_SHIFT;
			if (validLocation((BASE_STATS *)psStats, (UDWORD)*px, (UDWORD) *py))
			{
				return TRUE;
			}
			break;

		case BPD_BOTTOM:
			// bottom
			*px = sStructPos.left >> TILE_SHIFT;
			*py = (sStructPos.bottom >> TILE_SHIFT) + 2;
			if (validLocation((BASE_STATS *)psStats, (UDWORD)*px, (UDWORD) *py))
			{
				return TRUE;
			}
			break;
		}
	}

	return FALSE;
}

// callback function for bposChooseStructure
typedef BOOL (*BPOS_CHOOSECB)(STRUCTURE *, SDWORD, STRUCTURE_STATS *);

// function to loop through the structure list looking for a suitable
// structure to build next to
BOOL bposChooseStructure(SDWORD player, SDWORD cluster, STRUCTURE_STATS *psStats,
						 SDWORD centerx, SDWORD centery,
						 SDWORD *px, SDWORD *py,
						 BPOS_CHOOSECB choose)
{
	SDWORD		xdiff,ydiff, currdist, maxdist, x,y;
	STRUCTURE	*psCurr;
	BOOL		found;

	// find the nearest structure that matches the conditions
	maxdist = SDWORD_MAX;
	found = FALSE;
	for (psCurr=apsStructLists[player]; psCurr; psCurr=psCurr->psNext)
	{
		if (choose(psCurr, cluster, psStats))
		{
			xdiff = (SDWORD)psCurr->x - centerx;
			ydiff = (SDWORD)psCurr->y - centery;
			currdist = xdiff*xdiff + ydiff*ydiff;
			if (currdist < maxdist)
			{
				if (bposGetLocByStruct(psCurr, psStats, centerx,centery, &x,&y))
				{
					maxdist = currdist;
					found = TRUE;
					*px = x;
					*py = y;
				}
			}
		}
	}

	return found;
}

// the structure choice callbacks
BOOL bposChooseSameType(STRUCTURE *psStruct, SDWORD cluster, STRUCTURE_STATS *psStats)
{
	return (psStruct->cluster == cluster) && (psStruct->pStructureType == psStats);
}

BOOL bposChooseSameSize(STRUCTURE *psStruct, SDWORD cluster, STRUCTURE_STATS *psStats)
{
	return (psStruct->cluster == cluster) &&
			(psStruct->pStructureType->type != REF_WALL) &&
			(psStruct->pStructureType->type != REF_WALLCORNER) &&
			(psStruct->pStructureType->baseWidth == psStats->baseWidth) &&
			(psStruct->pStructureType->baseBreadth == psStats->baseBreadth);
}

BOOL bposChooseNotWall(STRUCTURE *psStruct, SDWORD cluster, STRUCTURE_STATS *psStats)
{
	return (psStruct->cluster == cluster) &&
			(psStruct->pStructureType->type != REF_WALL) &&
			(psStruct->pStructureType->type != REF_WALLCORNER);
}

BOOL bposChooseWall(STRUCTURE *psStruct, SDWORD cluster, STRUCTURE_STATS *psStats)
{
	return (psStruct->cluster == cluster) &&
			(psStruct->pStructureType->type == REF_WALL);
}

// find a location for a new building as part of a cluster
BOOL bposGetLocation(SDWORD player, SDWORD clusterID, STRUCTURE_STATS *psStats, SDWORD *px, SDWORD *py)
{
	SDWORD		cluster, centerx,centery;
	STRUCTURE	*psCurr;

	ASSERT( (player >= 0) && (player < MAX_PLAYERS),
		"bposGetLocation: invalid player" );

	// find the actual cluster number for the cluster ID
	cluster = clustGetClusterFromID(clusterID);

	// find the center of the cluster
	for (psCurr=apsStructLists[player]; psCurr; psCurr=psCurr->psNext)
	{
		if (psCurr->cluster == cluster)
		{
			clustGetCenter((BASE_OBJECT *)psCurr, &centerx, &centery);
			break;
		}
	}
	if (psCurr == NULL)
	{
		// failed to find any structures in the cluster - quit
		return FALSE;
	}

	// see if there is a structure of the same type in the cluster
/*	for (psCurr=apsStructLists[player]; psCurr; psCurr=psCurr->psNext)
	{
		if ((psCurr->cluster == cluster) && (psCurr->pStructureType == psStats))
		{
			// found a structure of the same type, see if it can be built next to
			if (bposGetLocByStruct(psCurr, psStats, centerx,centery, px,py))
			{
				return TRUE;
			}
		}
	}*/
	if (bposChooseStructure(player, cluster, psStats, centerx,centery, px,py,
							bposChooseSameType))
	{
		return TRUE;
	}

	// now see if there is a structure of the same size in the cluster
/*	for (psCurr=apsStructLists[player]; psCurr; psCurr=psCurr->psNext)
	{
		if ((psCurr->cluster == cluster) &&
			(psCurr->pStructureType->type != REF_WALL) &&
			(psCurr->pStructureType->type != REF_WALLCORNER) &&
			(psCurr->pStructureType->baseWidth == psStats->baseWidth) &&
			(psCurr->pStructureType->baseBreadth == psStats->baseBreadth))
		{
			// found a structure of the same size, see if it can be built next to
			if (bposGetLocByStruct(psCurr, psStats, centerx,centery, px,py))
			{
				return TRUE;
			}
		}
	}*/
	if (bposChooseStructure(player, cluster, psStats, centerx,centery, px,py,
							bposChooseSameSize))
	{
		return TRUE;
	}


	// now just try and find something that isn't a wall
/*	for (psCurr=apsStructLists[player]; psCurr; psCurr=psCurr->psNext)
	{
		if ((psCurr->cluster == cluster) &&
			(psCurr->pStructureType->type != REF_WALL) &&
			(psCurr->pStructureType->type != REF_WALLCORNER))
		{
			// found a structure
			if (bposGetLocByStruct(psCurr, psStats, centerx,centery, px,py))
			{
				return TRUE;
			}
		}
	}*/
	if (bposChooseStructure(player, cluster, psStats, centerx,centery, px,py,
							bposChooseNotWall))
	{
		return TRUE;
	}


	// finally just put it next to a wall
/*	for (psCurr=apsStructLists[player]; psCurr; psCurr=psCurr->psNext)
	{
		if ((psCurr->cluster == cluster) &&
			(psCurr->pStructureType->type == REF_WALL))
		{
			// found a structure
			if (bposGetLocByStruct(psCurr, psStats, centerx,centery, px,py))
			{
				return TRUE;
			}
		}
	}*/
	if (bposChooseStructure(player, cluster, psStats, centerx,centery, px,py,
							bposChooseWall))
	{
		return TRUE;
	}


	return FALSE;
}

#endif
