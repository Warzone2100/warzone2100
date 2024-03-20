# Warzone 2100 Map Format

## Layout

The Warzone 2100 map file format consists of th following sections:

1. Header
2. Terrain Atlas
3. Decal Atlas
4. Terrain Data

## Header Format


| Offset |    Datatype     |                              Description                              |
|--------|-----------------|-----------------------------------------------------------------------|
| 0x0000 | char[4] literal | Identifier of file format "MAP\0"                                     |
| 0x0004 | uint32          | Version of file (0x32)                                                |
| 0x0008 | uint32          | Width of map in number of vertices  \<width\>                            |
| 0x000C | uint32          | Height of map in number of vertices \<height\>                          |
| 0x0010 | int32           | Scale exponent of heightmap XY compared to world tiles (2^scale) \<scale\> |
| 0x0014 | float32         | LSb value of heightmap Z data compared to world units                       |
| 0x0018 | uint16          | Level of water in level when water is present (0 if not used)         |
| 0x001A | uint16          | Number of entries in terrain atlas \<numTerrain\>                       |
| 0x001C | uint16          | Number of entries in decal atlas \<numDecal\>                           |
| 0x001E | uint16          | Max length of any atlas entry \<maxLength\>                           |
| 0x0020 | int32           | Offset of terrain types from start of file \<startTerrainAtlas\>        |
| 0x0024 | int32           | Offset of decal types from start of file \<startDecalAtlas\>          |
| 0x0028 | int32           | Offset of terrain from start of file \<startTerrainData\>          |

Notes:
- Map will be (width-1)*(height-1) tiles in size
- The scale indicates how many mesh tiles will fit into one terrain tile

## Terrain Atlas Format

|       Relative Offset       | Datatype |                              Description                               |
|-----------------------------|----------|------------------------------------------------------------------------|
| 0x0000                      | float32  | Scale of the source data in world units for terrain type 0             |
| 0x0004                      | C string | Null terminated string for the name of the resource for terrain type 0 |
| 0x0004 + last string length | float32  | Scale of the source data in world units for terrain type 1             |
| 0x0008 + last string length | C string | Null terminated string for the name of the resource for terrain type 1 |
| ...                         | float32  | Scale of the source data in world units for terrain type n             |
| ...                         | C string | Null terminated string for the name of the resource for terrain type n |



## Decal Atlas


|       Relative Offset       | Datatype |                              Description                               |
|-----------------------------|----------|------------------------------------------------------------------------|
| 0x0000                      | uint8    | X size of the source data in world units for decal type 0              |
| 0x0001                      | uint8    | Y size of the source data in world units for decal type 0              |
| 0x0002                      | C string | Null terminated string for the name of the resource for terrain type 0 |
| 0x0002 + last string length | uint8    | X size of the source data in world units for decal type 1              |
| 0x0003 + last string length | uint8    | Y size of the source data in world units for decal type 1              |
| 0x0004 + last string length | C string | Null terminated string for the name of the resource for terrain type 1 |
| ...                         | uint8    | X size of the source data in world units for decal type n              |
| ...                         | uint8    | Y size of the source data in world units for decal type n              |
| ...                         | C string | Null terminated string for the name of the resource for terrain type n |


## Terrain Data

Array of heightmap data. Array is in C order and shape (width, height) with a total number of entries of width * height and each entry is a `uint16` which encodes the height of the terrain at that point. Following this is the array of tile data. Tile map data is a simple struct that encodes the terrain type, decal type and orientation, and whether there is water above this tile or not. The shape of the tile map data is (width / 2^scale - 1, height / 2^scale - 1), so the total number of entries is (width * 2^scale - 1) * (height * 2^scale - 1).

For example, a vanilla map would have a scale exponent of 0 and a maximum size of 256 x 256. This heightmap would have 65,536 entries, and the tilemap would have 65,025 entries. A "high definition" map might have a scale exponent of -3 (8 heightmap tiles per world tile in each direction). For this example lets also use a world unit width of 255 x 255. The heightmap data would have 268,435,456 entries and shape (16384, 16384). The tilemap data would still have 65025 entries since it is still the same world size. 


### Heightmap Data

|     Relative Offset     | Datatype |                    Description                    |
|-------------------------|----------|---------------------------------------------------|
| 0x0000                  | uint16   | Height of top left map vertex (0,0)               |
| 0x0002                  | uint16   | Height of next vertex (0,1)                       |
| ...                     | ...      | ...                                               |
| 0x0002 * height         | uint16   | Height of bottom left map vertex (0, height-1)    |
| ...                     | ...      | ...                                               |
| 0x0002 * height * width | uint16   | Height of bottom right map vertex (width, height) |


### Tilemap Data


Each Entry of the tilemap array will have the following structure:

| Relative Offset | Datatype |                     Description                     |
|-----------------|----------|-----------------------------------------------------|
| 0x0000          | uint16   | Index of terrain type                               |
| 0x0002          | uint16   | Index of decal type (0xFFFF if none)                |
| 0x0004          | uint8    | Numerator of tile traversal cost/speed multiplier   |
| 0x0005          | uint8    | Denominator of tile traversal cost/speed multiplier |
| 0x0006          | uint8    | Hue of preview/minimap tile                         |
| 0x0007          | uint8    | Flags for tile (described below)                    |

The flag bitfield for the tile will have the following structure:

|      Bit 7      | Bit 6  | Bit 5  |     Bits 3-4      | Bits 2-0 |
|-----------------|--------|--------|-------------------|----------|
| Has water above | Flip X | Flip Y | Rotation of decal | Decal Z-order |

Rotation and placement axis is the center of the world tile. Each world tile can only have one decal homed to it, but for decals that span multiple world tiles, overlapping decals can be created. The priority is decided by the Z-order specified in the least 3 significant bits.