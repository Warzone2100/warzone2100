/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2020  Warzone 2100 Project

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
 *  Definitions for flowfield pathfinding.
 */

#ifndef __INCLUDED_SRC_FLOWFIELD_H__
#define __INCLUDED_SRC_FLOWFIELD_H__

#include "fpath.h"
#include "movedef.h"
#include "lib/framework/frame.h" // for statsdef.h
#include "statsdef.h"
#include <deque>
#include <glm/gtx/transform.hpp>

/*
	Concept: http://www.gameaipro.com/GameAIPro/GameAIPro_Chapter23_Crowd_Pathfinding_and_Steering_Using_Flow_Field_Tiles.pdf
*/

///////////////////////////////////////////////////////////////////// SCRATCHPAD
// Useful functions found:
// map_TileHeightSurface(x, y) -> height
// map_coord(x, y) -> tile
// world_coord(tile) -> x, y
// getTileMaxMin <- maybe?
// fpathBlockingTile(x, y, PROPULSION_TYPE_WHEELED) - is field blocking for given propulsion
// mapTile(x, y) -> MAPTILE
// worldTile(x, y) -> MAPTILE
// TileIsOccupied <- maybe, to check if there is a building on that tile
// TileIsKnownOccupied <- like above
// tileIsExplored

// MAPTILE.limitedContinent - if I understand it correctly, if there is no ground path between two tiles, then limitedContinent1 != limitedContinent2
// MAPTILE.hoverContinent - like above, but what is continent for hover?
// extern SDWORD	mapWidth, mapHeight;
// extern MAPTILE *psMapTiles;

// isDanger = auxTile(x, y, type.owner) & AUXBITS_THREAT;

/////////////////////////////////////////////////////////////////////


// TODO: avoiding tiles marked as "threat" (only AI, or anyone? It would be nice if your own droids would prefer to avoid enemy when retreating)
// TODO: maybe prefer visible tiles, or just discovered tiles (for player)
// Both things would go into integration field probably. Note that adding visibility stuff would quickly require most integration and flow fields to be thrown away, since visibility changes all the time.

/// Enable flowfield pathfinding.
void flowfieldEnable();
/// Checks if flowfield pathfinding is enabled.
bool isFlowfieldEnabled();

/// Initialises flowfield pathfinding for a map.
void flowfieldInit();
/// Deinitialises flowfield pathfinding.
void flowfieldDestroy();

/// update cost field: mark a tile as impassable
void markTileAsImpassable(uint8_t x, uint8_t y, PROPULSION_TYPE prop);

/// update cost field: mark as passable again
void markTileAsDefaultCost(uint8_t x, uint8_t y, PROPULSION_TYPE prop);

/// Returns true and populates flowfieldId if a flowfield exists for the specified target.
bool tryGetFlowfieldForTarget(unsigned int targetX, unsigned int targetY, PROPULSION_TYPE propulsion, unsigned int &flowfieldId);
/// Starts to generate a flowfield for the specified target.
void calculateFlowfieldAsync(unsigned int targetX, unsigned int targetY, PROPULSION_TYPE propulsion);
/// Returns true and populates vector if a directional vector exists for the specified flowfield and target position.
bool tryGetFlowfieldVector(unsigned int flowfieldId, uint8_t x, uint8_t y, Vector2f& vector);

/// Draw debug data for flowfields.
void debugDrawFlowfields(const glm::mat4 &mvp);

/// Initialise infrastructure (threads ...) for flowfield pathfinding.
bool ffpathInitialise();
/// Shut down infrastructure (threads ...) for flowfield pathfinding.
void ffpathShutdown();

#endif // __INCLUDED_SRC_FLOWFIELD_H__
