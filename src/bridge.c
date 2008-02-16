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
#include "lib/framework/frame.h"
#include "map.h"
#include "hci.h"
#include "mapdisplay.h"
#include "display3d.h"
#include "lib/ivis_common/ivisdef.h" //ivis matrix code
#include "lib/ivis_common/piedef.h" //pie render
#include "lib/ivis_opengl/piematrix.h"
#include "lib/ivis_common/piepalette.h"
#include "miscimd.h"
#include "effects.h"
#include "bridge.h"

/**
 * @file bridge.c
 * Bridges - currently unused.
 * 
 * Handles rendering and placement of bridging sections for
 * traversing water and ravines?! My guess is this won't make it into
 * the final game, but we'll see...
 * Alex McLean, Pumpkin Studios EIDOS Interactive, 1998.
 * 
 * He was right. It did not make the final game. The code below
 * is unused. Can it be reused? - Per, 2007
 */

/*
Returns TRUE or FALSE as to whether a bridge is valid.
For it to be TRUE - all intervening points must be lower than the start
and end points. We can also check other stuff here like what it's going
over. Also, it has to be between a minimum and maximum length and
one of the axes must share the same values.
*/
BOOL	bridgeValid(UDWORD startX,UDWORD startY, UDWORD endX, UDWORD endY)
{
BOOL	xBridge, yBridge;
UDWORD	bridgeLength;
UDWORD	startHeight,endHeight,sectionHeight;
UDWORD	i;

	/* Establish axes allignment */
	xBridge = ( (startX == endX) ? TRUE : FALSE );
	yBridge = ( (startY == endY) ? TRUE : FALSE );

	/* At least one axis must be constant */
	if (!xBridge && !yBridge)
	{
		/*	Bridge isn't straight - this shouldn't have been passed
			in, but better safe than sorry! */
		return(FALSE);
	}

	/* Get the bridge length */
	bridgeLength = ( xBridge ? abs(startY-endY) : abs(startX-endX) );

	/* check it's not too long or short */
	if(bridgeLength<MINIMUM_BRIDGE_SPAN || bridgeLength>MAXIMUM_BRIDGE_SPAN)
	{
		/* Cry out */
		return(FALSE);
	}

	/*	Check intervening tiles to see if they're lower
	so first get the start and end heights */
	startHeight = mapTile(startX,startY)->height;
	endHeight = mapTile(endX,endY)->height;

	/*
		Don't whinge about this piece of code please! It's nice and short
		and is called very infrequently. Could be made slightly faster.
	*/
	for(i = ( xBridge ? MIN(startY, endY) : MIN(startX, endX) );
		i < ( xBridge ? MAX(startY, endY) : MAX(startX, endX) ); i++)
	{
		/* Get the height of a bridge section */
		sectionHeight = mapTile((xBridge ? startX : startY),i)->height;
		/* Is it higher than BOTH end points? */
		if( sectionHeight > MAX(startHeight,endHeight) )
		{
			/* Cry out */
			return(FALSE);
		}
	}
	/* Everything's just fine */
	return(TRUE);
}


/*
	This function will actually draw a wall section
	Slightly different from yer basic structure draw in that
	it's not alligned to the terrain as bridge sections sit
	at a height stored in their structure - as they're above the ground
	and wouldn't be much use if they weren't, bridge wise.
*/
BOOL	renderBridgeSection(STRUCTURE *psStructure)
{
	SDWORD			structX,structY,structZ;
	SDWORD			rx,rz;
	//iIMDShape		*imd;
	Vector3i dv;

			/* Bomb out if it's not visible and there's no active god mode */
			if(!psStructure->visible[selectedPlayer] && !godMode)
			{
				return(FALSE);
			}

			/* Get it's x and y coordinates so we don't have to deref. struct later */
			structX = psStructure->pos.x;
			structY = psStructure->pos.y;
			structZ = psStructure->pos.z;

			/* Establish where it is in the world */
			dv.x = (structX - player.p.x) - terrainMidX*TILE_UNITS;
			dv.z = terrainMidY*TILE_UNITS - (structY - player.p.z);
			dv.y = structZ;

			/* Push the indentity matrix */
			pie_MatBegin();

			/* Translate */
			pie_TRANSLATE(dv.x,dv.y,dv.z);

			/* Get the x,z translation components */
			rx = map_round(player.p.x);
			rz = map_round(player.p.z);

			/* Translate */
			pie_TRANSLATE(rx,0,-rz);

			pie_Draw3DShape(psStructure->sDisplay.imd, 0, 0, WZCOL_WHITE, WZCOL_BLACK, 0, 0);

			pie_MatEnd();
			return(TRUE);
}

/*
	This will work out all the info you need about the bridge including
	length - height to set sections at in order to allign to terrain and
	what you need to alter start and end terrain heights by to establish to
	connection.
*/
void	getBridgeInfo(UDWORD startX,UDWORD startY,UDWORD endX, UDWORD endY, BRIDGE_INFO *info)
{
BOOL	xBridge,yBridge;
UDWORD	startHeight,endHeight;
BOOL	startHigher;

	/* Copy over the location coordinates */
	info->startX = startX;
	info->startY = startY;
	info->endX = endX;
	info->endY = endY;

	/* Get the heights of the start and end positions */
	startHeight = map_TileHeight(startX,startY);
	endHeight = map_TileHeight(endX,endY);

	/* Find out which is higher */
	startHigher = (startHeight>=endHeight ? TRUE : FALSE);

	/* If the start position is higher */
	if(startHigher)
	{
		/* Inform structure */
		info->startHighest = TRUE;

		/* And the end position needs raising */
		info->heightChange = startHeight - endHeight;
	}
	/* Otherwise, the end position is lower */
	else
	{
		/* Inform structure */
		info->startHighest = FALSE;
		/* So we need to raise the start position */
		info->heightChange = endHeight - startHeight;
	}

	/* Establish axes allignment */
	/* Only one of these can occur otherwise
	bridge is one square big */
	xBridge = ( (startX == endX) ? TRUE : FALSE );
	yBridge = ( (startY == endY) ? TRUE : FALSE );

	/*
		Set the bridge's height.
		Note that when the bridge is built BOTH tile heights need
		to be set to the agreed value on their bridge trailing edge
		(x,y) and (x,y+1) is constant X and (x,y) and (x+1,y) if constant
		Y
	*/
	if(startHigher)
	{
		info->bridgeHeight = map_TileHeight(startX,startY);
	}
	else
	{
		info->bridgeHeight = map_TileHeight(endX,endY);
	}

	/* We've got a bridge of constant X */
	if(xBridge)
	{
		info->bConstantX = TRUE;
		info->bridgeLength = abs(startY-endY);
	}
	/* We've got a bridge of constant Y */
	else if(yBridge)
	{
		info->bConstantX = FALSE;
		info->bridgeLength = abs(startX-endX);
	}
	else
	{
		debug( LOG_ERROR, "Weirdy Bridge requested - no axes allignment" );
		abort();
	}
}

void testBuildBridge(UDWORD startX, UDWORD startY, UDWORD endX, UDWORD endY)
{
	BRIDGE_INFO	bridge;
	UDWORD	i;
	Vector3i dv;

	if(bridgeValid(startX,startY,endX,endY))
	{
		getBridgeInfo(startX,startY,endX,endY,&bridge);
		if(bridge.bConstantX)
		{

			for(i = MIN(bridge.startY, bridge.endY); i < (MAX(bridge.startY, bridge.endY) + 1); i++)
			{
		   		dv.x = ((bridge.startX*128)+64);
		   		dv.z = ((i*128)+64);
				dv.y = bridge.bridgeHeight;
				addEffect(&dv,EFFECT_SMOKE,SMOKE_TYPE_DRIFTING,FALSE,NULL,0);
//				addExplosion(&dv,TYPE_EXPLOSION_SMOKE_CLOUD,NULL);
			}
		}
		else
		{
			for(i = MIN(bridge.startX, bridge.endX); i < (MAX(bridge.startX,bridge.endX) + 1); i++)
			{
		   		dv.x = ((i*128)+64);
		   		dv.z = ((bridge.startY*128)+64);
				dv.y = bridge.bridgeHeight;
				addEffect(&dv,EFFECT_SMOKE,SMOKE_TYPE_DRIFTING,FALSE,NULL,0);
//				addExplosion(&dv,TYPE_EXPLOSION_SMOKE_CLOUD,NULL);
			}
		}
			/* Flatten the start tile */
			setTileHeight(bridge.startX,bridge.startY,bridge.bridgeHeight);
			setTileHeight(bridge.startX,bridge.startY+1,bridge.bridgeHeight);
			setTileHeight(bridge.startX+1,bridge.startY,bridge.bridgeHeight);
			setTileHeight(bridge.startX+1,bridge.startY+1,bridge.bridgeHeight);

			/* Flatten the end tile */
			setTileHeight(bridge.endX,bridge.endY,bridge.bridgeHeight);
			setTileHeight(bridge.endX,bridge.endY+1,bridge.bridgeHeight);
			setTileHeight(bridge.endX+1,bridge.endY,bridge.bridgeHeight);
			setTileHeight(bridge.endX+1,bridge.endY+1,bridge.bridgeHeight);
	}
	else
	{
		getBridgeInfo(startX,startY,endX,endY,&bridge);
		if(bridge.bConstantX)
		{
			for(i = MIN(bridge.startY, bridge.endY); i < (MAX(bridge.startY, bridge.endY) + 1); i++)
			{
		   		dv.x = ((bridge.startX*128)+64);
		   		dv.z = ((i*128)+64);
				dv.y = bridge.bridgeHeight;
				addEffect(&dv,EFFECT_EXPLOSION,EXPLOSION_TYPE_SMALL,FALSE,NULL,0);
//				addExplosion(&dv,TYPE_EXPLOSION_MED,NULL);
			}
		}
		else
		{
			for(i = MIN(bridge.startX,bridge.endX); i < (MAX(bridge.startX, bridge.endX) + 1); i++)
			{
		   		dv.x = ((i*128)+64);
		   		dv.z = ((bridge.startY*128)+64);
				dv.y = bridge.bridgeHeight;
				addEffect(&dv,EFFECT_EXPLOSION,EXPLOSION_TYPE_SMALL,FALSE,NULL,0);
//				addExplosion(&dv,TYPE_EXPLOSION_MED,NULL);
			}
		}

	}
}



