/*
	This file is part of Warzone 2100.
	Copyright (C) 2005-2023 Warzone 2100 Project

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
 *  Routines for pathfinding-related debug rendering.
 */


#include "lib/ivis_opengl/pieblitfunc.h"
#include "lib/ivis_opengl/piematrix.h"
#include "lib/ivis_opengl/textdraw.h"

#include "droid.h"
#include "objmem.h"
#include "drawPath.h"
#include "display3d.h"

#include "map.h"

#include "astar.h"

// alpha fully opaque
static constexpr PIELIGHT WZ_WHITE {0xFF, 0xFF, 0xFF, 0xFF};

void debugRenderText (const char *txt, int vert_idx)
{
	const int TEXT_SPACEMENT = 20;
	WzText t(WzString (txt), font_regular);
	t.render(20, 80 + TEXT_SPACEMENT * vert_idx, WZ_WHITE);
}

static void drawLines(const glm::mat4& mvp, std::vector<Vector3i> pts, PIELIGHT color)
{
	std::vector<glm::ivec4> grid2D;
	for (int i = 0; i < pts.size(); i += 2)
	{
		Vector2i _a, _b;
		pie_RotateProjectWithPerspective(&pts[i],     mvp, &_a);
		pie_RotateProjectWithPerspective(&pts[i + 1], mvp, &_b);
		grid2D.push_back({_a.x, _a.y, _b.x, _b.y});
	}
	iV_Lines(grid2D, color);
}

// draw a square where half of sidelen goes in each direction
void drawSquare (const glm::mat4 &mvp, int sidelen, int startX, int startY, int height, PIELIGHT color)
{
	iV_PolyLine({
		{ startX - (sidelen / 2), height, -startY - (sidelen / 2) },
		{ startX - (sidelen / 2), height, -startY + (sidelen / 2) },
		{ startX + (sidelen / 2), height, -startY + (sidelen / 2) },
		{ startX + (sidelen / 2), height, -startY - (sidelen / 2) },
		{ startX - (sidelen / 2), height, -startY - (sidelen / 2) },
	}, mvp, color);
}

// no half-side translation
// world coordinates
static void drawSquare2 (const glm::mat4 &mvp, int sidelen, int startX, int startY, int height, PIELIGHT color)
{
	iV_PolyLine({
		{ startX, height, -startY },
		{ startX, height, -startY - (sidelen) },
		{ startX + (sidelen), height, -startY - (sidelen) },
		{ startX + (sidelen), height, -startY },
		{ startX, height, -startY},
	}, mvp, color);
}

void debugDrawImpassableTiles(const PathBlockingMap& bmap, const iView& viewPos, const glm::mat4 &mvp, int tiles = 6)
{
	PathCoord pc = bmap.worldToMap(viewPos.p.x, viewPos.p.z);
	const auto playerXTile = pc.x;
	const auto playerYTile = pc.y;
	const int tileSize = bmap.tileSize();
	const int slice = tileSize / 8;
	const int terrainOffset = 10;

	std::vector<Vector3i> pts;

 	for (int dx = -tiles; dx <= tiles; dx++)
	{
		for (int dy = -tiles; dy <=tiles; dy++)
		{
			const auto mapx = playerXTile + dx;
			const auto mapy = playerYTile + dy;
			auto w = bmap.mapToWorld(mapx, mapy);
			auto height = map_TileHeight(mapx, mapy) + terrainOffset;

			if (bmap.isBlocked(mapx, mapy))
			{
				drawSquare2 (mvp, 128, w.x, w.y, height, WZCOL_RED);
			}

			if (bmap.isDangerous(mapx, mapy))
			{
				// 45 degrees lines
				for (int p = slice/2; p < tileSize; p += slice)
				{
					// Bottom left corner
					pts.push_back({w.x, height, -(w.y + p)});
					pts.push_back({w.x + p, height, -w.y});

					// Top right corner
					pts.push_back({w.x + tileSize, height, -(w.y + p)});
					pts.push_back({w.x + p, height, -w.y - tileSize});
				}
			}
		}
	}

	if (!pts.empty())
		drawLines (mvp, pts, WZCOL_YELLOW);
}

void drawDroidPath(DROID *psDroid, const glm::mat4& viewMatrix, const glm::mat4 &perspectiveViewMatrix)
{
	auto status = psDroid->sMove.Status;
	if (status == MOVEINACTIVE || status == MOVEPAUSE || status == MOVEWAITROUTE || psDroid->sMove.asPath.empty())
		return;

	const int terrainOffset = 5;

	std::vector<Vector3i> pastPoints, futurePoints;

	Vector3i prev;
	int currentPt = psDroid->sMove.pathIndex;
	for (int i = 0; i < psDroid->sMove.asPath.size(); i++) {
		Vector2i pt2 = psDroid->sMove.asPath[i];
		auto height = map_TileHeight(map_coord(pt2.x), map_coord(pt2.y)) + terrainOffset;
		Vector3i pt3(pt2.x, height, -pt2.y);

		if (i > 0) {
			if (i < currentPt) {
				pastPoints.push_back(prev);
				pastPoints.push_back(pt3);
			} else if (i >= currentPt) {
				futurePoints.push_back(prev);
				futurePoints.push_back(pt3);
			}
		}
		prev = pt3;
	}

	if (futurePoints.size() > 1)
		drawLines (perspectiveViewMatrix, futurePoints, WZCOL_GREEN);
	if (pastPoints.size() > 1)
		drawLines (perspectiveViewMatrix, pastPoints, WZCOL_YELLOW);

	auto height = map_TileHeight(map_coord(psDroid->sMove.target.x), map_coord(psDroid->sMove.target.y)) + terrainOffset;
	drawSquare(perspectiveViewMatrix, 32, psDroid->sMove.target.x, psDroid->sMove.target.y, height, WZCOL_GREEN);
}

/// Draws path tree from the context.
void drawContext(const PathfindContext& context, const glm::mat4 &mvp)
{
	const int terrainOffset = 5;
	const int contextIteration = context.iteration;
	std::vector<Vector3i> pts;
	for (int y = 0; y < context.height; y++)
	{
		for (int x = 0; x < context.width; x++)
		{
			const PathExploredTile& tile = context.tile(PathCoord(x, y));
			if (tile.iteration != contextIteration)
				continue;
			auto heightStart = map_TileHeight(x, y) + terrainOffset;
			Vector3i start(world_coord(x), heightStart, -world_coord(y));
			pts.push_back(start);
			auto heightEnd = map_TileHeight(x - tile.dx/64, y - tile.dy/64) + terrainOffset;
			Vector3i end(world_coord(x) - tile.dx , heightEnd, -world_coord(y) + tile.dy);
			pts.push_back(end);
		}
	}
	if (!pts.empty())
		drawLines(mvp, pts, WZCOL_LGREEN);
}

static bool _drawImpassableTiles = false;
static bool _drawContextTree = false;
static bool _drawDroidPath = false;

extern std::list<PathfindContext> fpathContexts;

void drawPathCostLayer(int player, const iView& playerViewPos, const glm::mat4& viewMatrix, const glm::mat4 &perspectiveViewMatrix)
{
	if (player >= MAX_PLAYERS)
		return;

	if (_drawImpassableTiles) {
		PathBlockingMap bmap;
		PathBlockingType pbtype;
		pbtype.owner = player;
		pbtype.propulsion = PROPULSION_TYPE_WHEELED;
		fillBlockingMap(bmap, pbtype);
		debugDrawImpassableTiles(bmap, playerViewPos, perspectiveViewMatrix, 6);
	}

	if (_drawDroidPath)
	{
		for (DROID *psDroid = apsDroidLists[player]; psDroid; psDroid = psDroid->psNext)
		{
			if (psDroid->selected)
			{
				drawDroidPath(psDroid, viewMatrix, perspectiveViewMatrix);
			}
		}
	}

	if (_drawContextTree && !fpathContexts.empty())
	{
		drawContext(fpathContexts.back(), perspectiveViewMatrix);
	}
}

void kf_ToggleShowPath()
{
	addConsoleMessage(_("Path display toggled."), DEFAULT_JUSTIFY, SYSTEM_MESSAGE);
	_drawDroidPath = !_drawDroidPath;
}

void kf_ShowPathStat() {
	_drawContextTree = !_drawContextTree;
}

void kf_ShowPathBlocking() {
	_drawImpassableTiles = !_drawImpassableTiles;
}
