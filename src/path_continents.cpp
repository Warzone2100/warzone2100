#include "map.h"
#include "path_continents.h"
#include "fpath.h" // for fpathBlockingTile
#include "profiling.h"

// Convert a direction into an offset.
// dir 0 => x = 0, y = -1
static const std::array<Vector2i, 8> aDirOffset =
{
	Vector2i(0, 1),
	Vector2i(-1, 1),
	Vector2i(-1, 0),
	Vector2i(-1, -1),
	Vector2i(0, -1),
	Vector2i(1, -1),
	Vector2i(1, 0),
	Vector2i(1, 1),
};

// Flood fill a "continent".
// TODO take into account scroll limits and update continents on scroll limit changes
size_t PathContinents::mapFloodFill(int x, int y, ContinentId continent, uint8_t blockedBits, ContinentId* continentGrid)
{
	std::vector<Vector2i> open;
	open.push_back(Vector2i(x, y));
	// Set continent value for the root of the search.
	continentGrid[x + y * width] = continent;

	size_t visitedTiles = 0;

	while (!open.empty())
	{
		// Pop the first open node off the list for this iteration
		Vector2i pos = open.back();
		open.pop_back();

		// Add accessible neighboring tiles to the open list
		for (int i = 0; i < aDirOffset.size(); ++i)
		{
			// rely on the fact that all border tiles are inaccessible to avoid checking explicitly
			Vector2i npos = pos + aDirOffset[i];

			if (npos.x < 1 || npos.y < 1 || npos.x > mapWidth - 2 || npos.y > mapHeight - 2)
			{
				continue;
			}

			ContinentId& id = continentGrid[npos.x + npos.y * width];

			if (!(blockTile(npos.x, npos.y, AUX_MAP) & blockedBits) && id == 0)
			{
				// add to open list
				open.push_back(npos);
				// Set continent value
				id = continent;
			}
		}
	}
	return visitedTiles;
}

void PathContinents::clear() {
	limitedContinents = 0;
	hoverContinents = 0;
	for (size_t i = 0; i < landGrid.size(); i++) {
		landGrid[i] = 0;
	}
	for (size_t i = 0; i < hoverGrid.size(); i++) {
		hoverGrid[i] = 0;
	}
}

void PathContinents::generate() {
	WZ_PROFILE_SCOPE2(PathContinents, generate);
	/* Clear continents */
	clear();

	width = mapWidth;
	height = mapHeight;
	tileShift = TILE_SHIFT;

	landGrid.resize(width*height, ContinentId(0));
	hoverGrid.resize(width*height, ContinentId(0));

	/* Iterate over the whole map, looking for unset continents */
	for (int y = 1; y < height - 2; y++)
	{
		for (int x = 1; x < width - 2; x++)
		{
			if (landGrid[x + y * width] == 0 && !fpathBlockingTile(x, y, PROPULSION_TYPE_WHEELED))
			{
				mapFloodFill(x, y, 1 + limitedContinents++, WATER_BLOCKED | FEATURE_BLOCKED, landGrid.data());
			}
			else if (landGrid[x + y * width] == 0 && !fpathBlockingTile(x, y, PROPULSION_TYPE_PROPELLOR))
			{
				mapFloodFill(x, y, 1 + limitedContinents++, LAND_BLOCKED | FEATURE_BLOCKED, landGrid.data());
			}

			if (hoverGrid[x + y * width] && !fpathBlockingTile(x, y, PROPULSION_TYPE_HOVER))
			{
				mapFloodFill(x, y, 1 + hoverContinents++, FEATURE_BLOCKED, hoverGrid.data());
			}
		}
	}

	debug(LOG_MAP, "Found %d limited and %d hover continents", limitedContinents, hoverContinents);
}

PathContinents::ContinentId PathContinents::getLand(int x, int y) const {
	if (x >= 0 && y >= 0 && x < width && y < height) {
		return landGrid[x + y * width];
	}
	return 0;
}

PathContinents::ContinentId PathContinents::getHover(int x, int y) const {
	if (x >= 0 && y >= 0 && x < width && y < height) {
		return hoverGrid[x + y * width];
	}
	return 0;
}

PathContinents::ContinentId PathContinents::getLand(const Position& worldPos) const {
	int x = worldPos.x >> tileShift;
	int y = worldPos.y >> tileShift;
	return getLand(x, y);
}

PathContinents::ContinentId PathContinents::getHover(const Position& worldPos) const {
	int x = worldPos.x >> tileShift;
	int y = worldPos.y >> tileShift;
	return getHover(x, y);
}
