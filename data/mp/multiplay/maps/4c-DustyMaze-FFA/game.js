/******************
 * Misc utils.    *
 ******************/

function assert(cond, msg) {
    if (!cond) {
        log(msg);
        throw new Error();
    }
}

/******************
 * Configuration. *
 ******************/

const Symmetry = {
    CENTRAL2P: "central2p",
    CENTRAL4P: "central4p",
    BILATERAL: "bilateral"
};

const symmetry = Symmetry.CENTRAL4P; // Symmetry type.

const mapWidth = 118; // Width of the map, in tiles.
const mapHeight = 118; // Height of the map, in tiles.
const mapArea = mapWidth * mapHeight; // Total number of tiles on the map.

const borderSize = 4; // Number of reserved tiles at the map border.

const mazeWidth = 5; // Width of the maze, in cells.
const mazeHeight = 5; // Height of the maze, in cells.
const mazeBaseWidth = 2; // Player base width, in cells.
const mazeBaseHeight = 2; // Player base height, in cells.
const mazeNumExits = 2; // Number of passages between player quarters.

// Number of cells in the center where no oils are placed.
// Avoids maps on which control of the center decides the game.
// This only appears to be a problem on 2v2 maps.
const oilFreeWidth = (symmetry == Symmetry.BILATERAL ? 2 : 0);
const oilFreeHeight = (symmetry == Symmetry.BILATERAL ? 1 : 0);

const tileSize = 11; // Size of maze cell, in tiles.

if (symmetry == Symmetry.CENTRAL2P) {
    assert(tileSize * mazeWidth + 2 * borderSize == mapWidth,
       "Tile size not fitting horizontally");
} else {
    assert(tileSize * 2 * mazeWidth + 2 * borderSize == mapWidth,
       "Tile size not fitting horizontally");
}

assert(tileSize * 2 * mazeHeight + 2 * borderSize == mapHeight,
       "Tile size not fitting vertically");

// Player base borders, in tiles.
const baseLeft = borderSize + 2;
const baseTop = borderSize + 2;
const baseRight = borderSize + mazeBaseWidth * tileSize - 2;
const baseBottom = borderSize + mazeBaseHeight * tileSize - 2;

const wallHeight = 256; // How tall the walls are.
const baseHeight = 48; // How elevated the player base is.

// Shortcuts for map tile textures.
const Tile = {
    BLUE: 0,
    ROCK: 8,
    DARK_CONCRETE_SIDE: 12,
    WATER: 17,
    DARK_CONCRETE: 18,
    CLIFF: 19,
    DIRT: 22,
    FEATURE1: 28,
    FEATURE2: 32,
    FEATURE3: 48,
    GRASS: 50,
    CONCRETE: 51,
    CRATER: 55,
};


/********************
 * Maze generation. *
 ********************/

const Wall = {
    NONE: "none",
    BLOCKED: "blocked",
    SCAVENGERS: "scavengers",
    WATER: "water",
    CLIFF: "cliff",
};

function isHardWall(wall) {
    return wall == Wall.CLIFF || wall == Wall.WATER;
}

const Side = {
    TOP: "top",
    LEFT: "left",
    BOTTOM: "bottom",
    RIGHT: "right",
};

class WallId {
    constructor(x, y, direction) {
        this.x = x;
        this.y = y;
        this.direction = direction;
    }
}

class Cell {
    constructor() {
        this.wall = {}
        this.wall[Side.BOTTOM] = Wall.CLIFF;
        this.wall[Side.RIGHT] = Wall.CLIFF;
        this.group = null;
        this.hasOil = false;
        this.hasScavengers = false;
    }

    getDeadEndDirection() {
        let numEntrances = 0;
        let answer = null;
        for (let side of [Side.BOTTOM, Side.TOP, Side.LEFT, Side.RIGHT]) {
            if (!isHardWall(this.wall[side])) {
                numEntrances += 1;
                answer = side;
            }
        }
        if (numEntrances != 1)
            return null;

        return answer;
    }

    isDeadEnd() {
        return !!this.getDeadEndDirection();
    }
}

class Maze {
    constructor(width, height, baseWidth, baseHeight, numExits, symmetry,
                oilFreeWidth, oilFreeHeight) {
        this.width = width;
        this.height = height;

        this.cell = [];
        for (let x = 0; x < width; ++x) {
            this.cell[x] = [];
            for (let y = 0; y < height; ++y) {
                this.cell[x][y] = new Cell();
            }
        }

        // Clear the area reserved for the player start location.
        for (let x = 0; x < baseWidth - 1; ++x)
            for (let y = 0; y < baseHeight; ++y)
                this.cell[x][y].wall[Side.RIGHT] = Wall.NONE;
        for (let x = 0; x < baseWidth; ++x)
            for (let y = 0; y < baseHeight - 1; ++y)
                this.cell[x][y].wall[Side.BOTTOM] = Wall.NONE;

        // Generate the maze using randomized Kruskal's algorithm.
        // First, separate all cell into disjoint sets.
        // We simply assign a group id to each cell; cell with the same id
        // belong to the same set. This is not an efficient implementation
        // but our mazes are tiny so that'll do.
        let groupIdx = 0;
        for (let x = 0; x < width; ++x)
            for (let y = 0; y < height; ++y) {
                this.cell[x][y].group = groupIdx;
                ++groupIdx;
            }

        // The starting area is already connected. Represent that in the sets.
        for (let x = 0; x < baseWidth; ++x)
            for (let y = 0; y < baseHeight; ++y)
                this.cell[x][y].group = 0;

        // Then, create a list of all editable walls.
        let walls = [];
        for (let x = 0; x < width - 1; ++x) {
            for (let y = 0; y < height - 1; ++y) {
                if (x < baseWidth && y < baseHeight)
                    continue;

                walls.push(new WallId(x, y, Side.RIGHT));
                walls.push(new WallId(x, y, Side.BOTTOM));
            }
        }
        for (let x = 0; x < width - 1; ++x)
            walls.push(new WallId(x, height - 1, Side.RIGHT));
        for (let y = 0; y < height - 1; ++y)
            walls.push(new WallId(width - 1, y, Side.BOTTOM));
        for (let x = 0; x < baseWidth; ++x)
            walls.push(new WallId(x, baseHeight - 1, Side.BOTTOM));
        for (let y = 0; y < baseHeight; ++y)
            walls.push(new WallId(baseWidth - 1, y, Side.RIGHT));

        // Shuffle that list using Durstenfeld shuffle algorithm.
        for (let i = walls.length - 1; i > 0; --i) {
            const j = gameRand(i + 1);
            [walls[i], walls[j]] = [walls[j], walls[i]];
        }

        // Generate the minimal spanning tree (aka our maze) by removing walls
        // in the specific random order obtained above.
        for (let i = 0; i < walls.length; ++i) {
            let wallId = walls[i];
            let dir = wallId.direction;

            let cell1 = this.cell[wallId.x][wallId.y];
            let cell2 = (() => {
                switch (dir) {
                    case Side.RIGHT:
                        return this.cell[wallId.x + 1][wallId.y];
                    case Side.BOTTOM:
                        return this.cell[wallId.x][wallId.y + 1];
                }
            })();

            let group1 = cell1.group;
            let group2 = cell2.group;

            if (group1 == group2) {
                // This wall will stay closed. Turn it into water
                // with 10% probability.
                if (gameRand(10) == 0) {
                    cell1.wall[dir] = Wall.WATER;
                }

                continue;
            }

            // Remove the wall. Block with scavengers at 5% probability.
            switch (gameRand(20)) {
                // FIXME: "Block with features" is currently disabled.
                // The AI doesn't know how to deal with it. I also didn't
                // figure out how to use any of the bulky blocking features
                // (such as urban 3x3 skyscrapers) so in multiplayer it achieves
                // nothing as it can be demolished too easily with your
                // first machinegun.
                /*
                case 0:
                    cell1.wall[dir] = Wall.BLOCKED;
                    break;
                */
                case 1:
                    cell1.wall[dir] = Wall.SCAVENGERS;
                    break;
                default:
                    cell1.wall[dir] = Wall.NONE;
                    break;
            }

            // Mark mazes as connected. This is the slow part that makes
            // the overall algorithm quadratic over the maze area.
            // Which is acceptable.
            for (let x = 0; x < width; ++x)
                for (let y = 0; y < height; ++y)
                    if (this.cell[x][y].group == group2)
                        this.cell[x][y].group = group1;
        }

        // Add a few random entrances to the square. This method can
        // accidentally generate less than numExits exits if some exits
        // overlap. Which is fine, it even adds extra suspense.
        // At least one exit is always generated.
        for (let i = 0; i < numExits; ++i) {
            switch (symmetry) {
                case Symmetry.CENTRAL2P: {
                    let j = gameRand(width);
                    this.cell[j][height - 1].wall[Side.BOTTOM] = Wall.NONE;
                    this.cell[width - 1 - j][height - 1].wall[Side.BOTTOM] = Wall.NONE;
                    break;
                }
                case Symmetry.CENTRAL4P: {
                    let j = gameRand(height);
                    this.cell[width - 1][j].wall[Side.RIGHT] = Wall.NONE;
                    this.cell[j][height - 1].wall[Side.BOTTOM] = Wall.NONE;
                    break;
                }
                case Symmetry.BILATERAL: {
                    let j1 = gameRand(height);
                    this.cell[width - 1][j1].wall[Side.RIGHT] = Wall.NONE;
                    let j2 = gameRand(width);
                    this.cell[j2][height - 1].wall[Side.BOTTOM] = Wall.NONE;
                    break;
                }
            }
        }

        // Avoid generating offensive symbols of hate in the middle of the map.
        if (symmetry == Symmetry.CENTRAL4P) {
            let cell = this.cell[width - 1][height - 1];
            let topCell = this.cell[width - 1][height - 2];
            let leftCell = this.cell[width - 2][height - 1];

            let topWall = topCell.wall[Side.BOTTOM];
            let leftWall = leftCell.wall[Side.RIGHT];
            let bottomWall = cell.wall[Side.BOTTOM];
            let rightWall = cell.wall[Side.RIGHT];

            if (isHardWall(bottomWall) && isHardWall(rightWall) && 
                    isHardWall(topWall) != isHardWall(leftWall)) {
                topCell.wall[Side.BOTTOM] = Wall.NONE;
                leftCell.wall[Side.RIGHT] = Wall.NONE;
            }
        }

        // Auto-fill TOP and LEFT wall information according to the
        // BOTTOM and RIGHT walls of the corresponding neighbors.
        for (let x = 0; x < width; ++x) {
            for (let y = 0; y < height; ++y) {
                if (x > 0) {
                    this.cell[x][y].wall[Side.LEFT] =
                        this.cell[x - 1][y].wall[Side.RIGHT];
                } else {
                    this.cell[x][y].wall[Side.LEFT] = Wall.CLIFF;
                } if (y > 0) {
                    this.cell[x][y].wall[Side.TOP] =
                        this.cell[x][y - 1].wall[Side.BOTTOM];
                } else {
                    this.cell[x][y].wall[Side.TOP] = Wall.CLIFF;
                }
            }
        }

        // Place oil resources in dead ends.
        let placedOils = [];
        for (let x = 0; x < width; ++x) {
            for (let y = 0; y < height; ++y) {
                if (x + oilFreeWidth >= width && y + oilFreeHeight >= height)
                    continue;

                if (this.cell[x][y].isDeadEnd()) {
                    this.cell[x][y].hasOil = true;
                    placedOils.push([x, y]);
                }
            }
        }

        // Place one full scavenger base near one of the oils.
        if (placedOils.length > 0) {
            let idx = gameRand(placedOils.length);
            let x = placedOils[idx][0];
            let y = placedOils[idx][1];
            this.cell[x][y].hasScavengers = true;
        }
    }
}


/****************************
 * Maze to map translation. *
 ****************************/

// Magic constants corresponding to the game engine's constants.
const TILE_XFLIP = 0x8000;
const TILE_YFLIP = 0x4000;
const TILE_ROT = 0x1000;

const DROID_ROT = 0x4000;

// Basic data structures of the map.
let texture = Array(mapArea).fill(Tile.GRASS);
let heightmap = Array(mapArea).fill(0);
let structures = [];
let droids = [];
let features = [];

function iterSymmetricTiles(x, y, callback) {
    callback(x, y, 0, 0, 0);

    switch (symmetry) {
        case Symmetry.CENTRAL2P:
            callback(mapWidth - 1 - x, mapHeight - 1 - y, 2 * DROID_ROT, TILE_YFLIP, 1);
            break;
        case Symmetry.CENTRAL4P:
            assert(mapWidth == mapHeight,
                "Central symmetry incompatible with non-square maps");
            callback(mapWidth - 1 - y, x, DROID_ROT, TILE_ROT, 1);
            callback(mapWidth - 1 - x, mapHeight - 1 - y, 2 * DROID_ROT, 2 * TILE_ROT, 2);
            callback(y, mapHeight - 1 - x, 3 * DROID_ROT, 3 * TILE_ROT, 3);
            break;
        case Symmetry.BILATERAL:
            callback(mapWidth - 1 - x, y, 0, TILE_XFLIP, 1);
            callback(x, mapHeight - 1 - y, 2 * DROID_ROT, TILE_YFLIP, 3);
            callback(mapWidth - 1 - x, mapHeight - 1 - y, 2 * DROID_ROT, TILE_XFLIP | TILE_YFLIP, 2);
            break;

    }
}

function setTexture(tex, origX, origY, rot, xflip) {
    iterSymmetricTiles(origX, origY, (x, y, _, tileRotFlip) => {
        let tileRot = (tileRotFlip & (3 * TILE_ROT)) / TILE_ROT;
        let tileXflip = tileRotFlip & TILE_XFLIP;
        let tileYflip = tileRotFlip & TILE_YFLIP;

        tileRot = (tileRot + rot) % 4;
        tileXflip = tileXflip ^ (xflip * TILE_XFLIP);

        texture[mapWidth * y + x] = tex | (tileRot * TILE_ROT) | tileXflip | tileYflip;
    });
}

function setTileHeight(h, origX, origY) {
    iterSymmetricTiles(origX, origY, (x, y) => {
        heightmap[mapWidth * y + x] = h;
        if (x < mapWidth - 1)
            heightmap[mapWidth * y + (x + 1)] = h;
        if (y < mapHeight - 1)
            heightmap[mapWidth * (y + 1) + x] = h;
        if (x < mapWidth - 1 && y < mapHeight - 1)
            heightmap[mapWidth * (y + 1) + (x + 1)] = h;
    });
}

function addStructure(name, origX, origY, size = 1, modules = 0, rot = null) {
    iterSymmetricTiles(origX, origY, (x, y, rot, _, idx) => {
        let offsetX = (idx == 1 || idx == 2) ? 0 : ((size + 1) % 2);
        let offsetY = (idx == 2 || idx == 3) ? 0 : ((size + 1) % 2);
        let dir = (rot === null) ? 0 : ((rot + idx) % 4);
        structures.push({
            name: name,
            position: [128 * (x + offsetX), 128 * (y + offsetY)],
            direction: 0x4000 * dir,
            modules: modules,
            player: idx
        });
    });
}

function addScavStructure(name, origX, origY) {
    iterSymmetricTiles(origX, origY, (x, y, rot, _, idx) => {
        structures.push({
            name: name,
            position: [128 * x, 128 * y],
            direction: rot,
            modules: 0,
            player: 7
        });
    });
}

function addDroid(name, origX, origY) {
    iterSymmetricTiles(origX, origY, (x, y, rot, _, idx) => {
        droids.push({
            name: name,
            position: [128 * x + 64, 128 * y + 64],
            direction: rot,
            player: idx
        });
    });
}

function addFeature(name, origX, origY, size = 1) {
    iterSymmetricTiles(origX, origY, (x, y, rot) => {
        features.push({
            name: name,
            position: [128 * x + 64, 128 * y + 64],
            direction: rot
        });
    });
}

function randomizeTile(x, y) {
    if (x <= baseRight + 1 && y <= baseBottom + 1)
        return;

    let variant = gameRand(500);
    switch (true) {
        case (variant < 75):
            setTexture(Tile.ROCK, x, y);
            setTileHeight(8, x, y);
            break;
        case (variant < 125):
            setTexture(Tile.BLUE, x, y);
            setTileHeight(24, x, y);
            break;
        case (variant == 492):
            setTexture(Tile.FEATURE3, x, y, /*rot=*/gameRand(3));
            break;
        case (variant == 493):
            setTexture(Tile.FEATURE2, x, y, /*rot=*/gameRand(3));
            break;
        case (variant == 494):
            setTexture(Tile.FEATURE1, x, y, /*rot=*/gameRand(3));
            break;
        case (variant == 495):
            setTexture(Tile.CRATER, x, y, /*rot=*/gameRand(3));
            break;
        case (variant == 496):
        case (variant == 497):
            addFeature("Tree3", x, y);
            break;
        case (variant == 498):
        case (variant == 499):
            addFeature("TreeSnow3", x, y);
            break;
        default:
            break;
    }
}

// Invoke the callback for every map tile that corresponds to the wall
// of maze cell '(x, y)' at side 'side', at (layer - 0.5) distance from
// the border of the cell.
function iterWallTile(x, y, side, layer, callback) {
    switch (side) {
        case Side.RIGHT:
            for (let i = 0; i < tileSize; ++i) {
                callback(borderSize + (x + 1) * tileSize - 1 - layer,
                         borderSize + y * tileSize + i, i);
            }
            break;
        case Side.BOTTOM:
            for (let i = 0; i < tileSize; ++i) { 
                callback(borderSize + x * tileSize + i,
                         borderSize + (y + 1) * tileSize - 1 - layer, i);
            }
            break;
        case Side.LEFT:
            for (let i = 0; i < tileSize; ++i) {
                callback(borderSize + x * tileSize + layer,
                         borderSize + y * tileSize + i, i);
            }
            break;
        case Side.TOP:
            for (let i = 0; i < tileSize; ++i) { 
                callback(borderSize + x * tileSize + i,
                         borderSize + y * tileSize + layer, i);
            }
            break;
    }
}

function placeWall(cellX, cellY, side, wall) {
    iterWallTile(cellX, cellY, side, 0, (x, y, idx) => {
        if (idx == 0 || idx == tileSize - 1) {
            setTexture(Tile.CLIFF, x, y);
            setTileHeight(wallHeight, x, y);
            return;
        }
        if (idx == 1 || idx == tileSize - 2) {
            setTexture(Tile.CLIFF, x, y);
            return;
        }
        switch (wall) {
            case Wall.NONE:
                randomizeTile(x, y);
                return;

            case Wall.CLIFF:
                setTexture(Tile.CLIFF, x, y);
                setTileHeight(wallHeight, x, y);
                return;

            case Wall.WATER:
                setTexture(Tile.WATER, x, y);
                return;

            case Wall.BLOCKED:
                addFeature("BarbHUT", x, y);
                return;

            case Wall.SCAVENGERS:
                if (x <= baseRight + tileSize && y <= baseBottom + tileSize) {
                    // Avoid placing scavengers in shooting range of the player
                    // even if the generator suggests so.
                    return;
                }
                switch (idx) {
                    case 2:
                    case tileSize - 3:
                        addScavStructure("A0CannonTower", x, y);
                        break;
                    case 4:
                    case tileSize - 5:
                        addScavStructure("A0BaBaFlameTower", x, y);
                        break;
                }
                return;
        }
    });
    iterWallTile(cellX, cellY, side, /*layer=*/1, (x, y, idx) => {
        if (idx <= 1 || idx >= tileSize - 2) {
            setTexture(Tile.CLIFF, x, y);
            return;
        }
        switch (wall) {
            case Wall.WATER:
            case Wall.BLOCKED:
            case Wall.NONE:
                if ((cellX == mazeBaseWidth - 1 && cellY < mazeBaseHeight &&
                        side == Side.RIGHT) || 
                    (cellX < mazeBaseWidth && cellY == mazeBaseHeight - 1 &&
                        side == Side.BOTTOM)) {

                    switch (idx) {
                        case 2:
                        case tileSize - 3:
                            addStructure("WallTower02", x, y);
                            break;

                        case 3:
                        case tileSize - 4:
                            addStructure("GuardTower6", x, y);
                            break;
                    }
                } else if (wall == Wall.NONE) {
                    randomizeTile(x, y);
                }
                return;
            case Wall.SCAVENGERS:
                return;
            case Wall.CLIFF:
                setTexture(Tile.CLIFF, x, y);
                return;
        }
    });
}

// Generate the maze.
let maze = new Maze(mazeWidth, mazeHeight, mazeBaseWidth, mazeBaseHeight,
                    mazeNumExits, symmetry, oilFreeWidth, oilFreeHeight);

// Place maze walls.
for (let x = 0; x < mazeWidth; ++x) {
    for (let y = 0; y < mazeHeight; ++y) {
        let cell = maze.cell[x][y];
        placeWall(x, y, Side.RIGHT, cell.wall[Side.RIGHT]);
        placeWall(x, y, Side.BOTTOM, cell.wall[Side.BOTTOM]);
        placeWall(x, y, Side.LEFT, cell.wall[Side.LEFT]);
        placeWall(x, y, Side.TOP, cell.wall[Side.TOP]);
    }
}

// Fill player base with concrete.
for (let x = baseLeft + 1; x < baseRight - 1; ++x) {
    for (let y = baseTop + 1; y < baseBottom - 1; ++y) {
        setTexture(Tile.DARK_CONCRETE, x, y);
        setTileHeight(baseHeight, x, y);
    }
}
for (let x = baseLeft + 1; x < baseRight - 1; ++x) {
    setTexture(Tile.DARK_CONCRETE_SIDE, x, baseBottom - 1, /*rot=*/3);
    setTexture(Tile.DARK_CONCRETE_SIDE, x, baseTop, /*rot=*/1);
}
for (let y = baseTop + 1; y < baseBottom - 1; ++y) {
    setTexture(Tile.DARK_CONCRETE_SIDE, baseLeft, y, /*rot=*/0);
    setTexture(Tile.DARK_CONCRETE_SIDE, baseRight - 1, y, /*rot=*/2);
}

// Paint border walls.
for (let x = 0; x < borderSize; ++x) {
    for (let y = 0; y < mapHeight / 2; ++y) {
        setTexture(Tile.CLIFF, x, y);
        setTileHeight(wallHeight, x, y);
    }
}
if (symmetry == Symmetry.CENTRAL2P) {
    for (let x = mapWidth - 1 - borderSize; x < mapWidth; ++x) {
        for (let y = 0; y < mapHeight / 2; ++y) {
            setTexture(Tile.CLIFF, x, y);
            setTileHeight(wallHeight, x, y);
        }
    }
    for (let x = 0; x < mapWidth; ++x) {
        for (let y = 0; y < borderSize; ++y) {
            setTexture(Tile.CLIFF, x, y);
            setTileHeight(wallHeight, x, y);
        }
    }

} else {
    for (let x = 0; x < mapWidth / 2; ++x) {
        for (let y = 0; y < borderSize; ++y) {
            setTexture(Tile.CLIFF, x, y);
            setTileHeight(wallHeight, x, y);
        }
    }
}

// Place oils and scavengers. Add some terrain variety.
for (let x = 0; x < mazeWidth; ++x) {
    for (let y = 0; y < mazeHeight; ++y) {
        let cell = maze.cell[x][y];
        let midX = borderSize + Math.floor((x + 0.5) * tileSize);
        let midY = borderSize + Math.floor((y + 0.5) * tileSize);

        let addedScavDerrick = false;
        if (cell.hasOil) {
            addFeature("OilResource", midX, midY);
            setTexture(Tile.CONCRETE, midX, midY);
            setTexture(Tile.CONCRETE, midX - 1, midY);
            setTexture(Tile.CONCRETE, midX, midY - 1);
            setTexture(Tile.CONCRETE, midX + 1, midY);
            setTexture(Tile.CONCRETE, midX, midY + 1);
            for (side in cell.wall) {
                if (cell.wall[side] == Wall.SCAVENGERS) {
                    addScavStructure("A0ResourceExtractor", midX, midY);
                    addedScavDerrick = true;
                }
            }
        }

        if (cell.hasScavengers) {
            if (!addedScavDerrick)
                addScavStructure("A0ResourceExtractor", midX, midY);

            switch (cell.getDeadEndDirection()) {
                case Side.LEFT:
                case Side.RIGHT:
                    addScavStructure("A0BaBaPowerGenerator", midX, midY - 2);
                    addScavStructure("A0BaBaFactory", midX, midY + 2);
                    break;
                case Side.TOP:
                case Side.BOTTOM:
                default:
                    addScavStructure("A0BaBaPowerGenerator", midX - 2, midY);
                    addScavStructure("A0BaBaFactory", midX + 2, midY);
                    break;
            }

            addScavStructure("A0BaBaBunker", midX - 2, midY - 2);
            addScavStructure("A0BaBaBunker", midX - 2, midY + 2);
            addScavStructure("A0BaBaBunker", midX + 2, midY - 2);
            addScavStructure("A0BaBaBunker", midX + 2, midY + 2);

            for (let x = midX - 2; x <= midX + 2; ++x)
                for (let y = midY - 2; y <= midY + 2; ++y)
                    if (gameRand(4) == 0)
                        setTexture(Tile.DIRT, x, y);
                    else
                        setTexture(Tile.CONCRETE, x, y);

        } else if (x >= mazeBaseWidth || y >= mazeBaseHeight) {
            let fillRadius = Math.ceil((tileSize - 4) / 2) - 1;
            for (let x = midX - fillRadius; x <= midX + fillRadius; ++x) {
                for (let y = midY - fillRadius; y <= midY + fillRadius; ++y) {
                    if (x != midX || y != midY) {
                        randomizeTile(x, y);
                    }
                }
            }

        }
        
    }
}

// Add base structures and trucks.
addStructure("A0CommandCentre", 7, 7, /*size=*/2);

addStructure("A0ResourceExtractor", 7, 10);
addStructure("A0ResourceExtractor", 7, 12);
addStructure("A0ResourceExtractor", 10, 7);
addStructure("A0ResourceExtractor", 12, 7);

addStructure("A0LightFactory", 15, 8, /*size=*/3, /*modules=*/2);
addStructure("A0LightFactory", 19, 8, /*size=*/3, /*modules=*/2);

addStructure("A0PowerGenerator", 7, 14, /*size=*/2, /*modules=*/1);
addStructure("A0ResearchFacility", 7, 17, /*size=*/2, /*modules=*/1);
addStructure("A0ResearchFacility", 7, 20, /*size=*/2, /*modules=*/1);

addDroid("ConstructionDroid", 9, 10);
addDroid("ConstructionDroid", 10, 9);


// Return the data.
setMapData(mapWidth, mapHeight, texture, heightmap, structures, droids, features);
