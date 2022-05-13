# wzmaplib
A self-contained library for loading / parsing / exporting WZ map formats.

- [Notes for developers](#notes-for-developers)
- [WzMap::IOProvider](#wzmapioprovider)
- [WzMap::LoggingProtocol](#wzmaploggingprotocol)
- [**Maps**](#maps)
   - [Loading a map](#loading-a-map)
   - [Saving a map](#saving-a-map)
- [**Map Packages**](#map-packages)
   - [Loading and exporting map packages](#loading-and-exporting-map-packages)
   - [Saving a new map package](#saving-a-new-map-package)
- [**Map Stats**](#map-stats)
   - [Extracting stats / info from a map](#extracting-stats--info-from-a-map)

## Notes for developers:

- It should be possible to `add_subdirectory(<wz_root>/lib/wzmaplib)` directly from an external project and successfully build and link to just `wzmaplib` without building the entire rest of Warzone 2100.
   ```cmake
   add_subdirectory("warzone2100/lib/wzmaplib" EXCLUDE_FROM_ALL)
   target_link_libraries(myproject PRIVATE wzmaplib)
   ```
- Once you link to `wzmaplib`, the public headers should be accessible via includes like:
   ```cpp
   #include <wzmaplib/map.h>
   #include <wzmaplib/map_preview.h>
   ```
- An example project that uses `wzmaplib` is available in the [`tools/map`](/tools/map) directory.

## WzMap::IOProvider:

An abstraction used to support different IO implementations. A default implementation - `WzMap::StdIOProvider` - is provided that uses C stdio.
When using the high-level interfaces, you do not need to pass an IOProvider unless you want to override the default.
(Warzone 2100 provides its own implementation that uses PhysFS, to support map archives.)

## WzMap::LoggingProtocol:

An abstraction used to support logging of info / warnings / errors when loading, saving, and processing maps.

# Maps:

Warzone 2100 has evolved its map format over time:

- Many "classic" maps, such as those made by older tools like the `FlaME` map editor, were saved in a binary data format ("BINARY_OLD").
   - a `game.map` file that contains the map tile texture + height info, as well as gateway information
   - a `ttypes.ttp` file that maps terrain tile textures to in-game terrain types (which affects game simulation behavior)
   - `dinit.bjo`, `feat.bjo`, `struct.bjo` files that contain the initial droid, feature, and structure information for the map
- This was followed by an initial JSON format ("JSON_v1"), which used JSON files for the droid / structure / feature files. The `game.map` and `ttypes.ttp` files are unchanged from the prior format.
- In WZ 4.1+, a new JSON format ("JSON_v2") was added that restructured the droid / structure / feature JSON files, and extended the `game.map` to support full-range (16-bit) tile heights.

Additionally, in WZ 4.0+, a "script-generated" map format was added:
- This replaces all\* the map data files with a single `game.js` that uses a limited set of APIs to generate and provide map data. (\*The only additional file is the `ttypes.ttp` file.)

`wzmaplib` can load all of these formats, and export to many of them.

## Loading a map:

#### High-level interface
```cpp
static std::shared_ptr<WzMap::Map> WzMap::Map::loadFromPath(const std::string& mapFolderPath, WzMap::MapType mapType, uint32_t mapMaxPlayers, uint32_t seed, bool previewOnly = false, std::shared_ptr<WzMap::LoggingProtocol> logger = nullptr, std::shared_ptr<WzMap::IOProvider> mapIO = std::shared_ptr<WzMap::IOProvider>(new WzMap::StdIOProvider()));
```
Then utilize the various methods of the returned `WzMap::Map` instance to obtain the mapData, structures, droids, features, etc.
For script-generated maps, the script is run on construction of the `WzMap::Map` object. For fixed map formats, the data is lazy-loaded on first request.

#### Lower-level interface
Loading just the `game.map` / map data:
```cpp
std::shared_ptr<WzMap::MapData> WzMap::loadMapData(const std::string &mapFile, IOProvider& mapIO, LoggingProtocol* pCustomLogger = nullptr);
```
Loading the terrain type data:
```cpp
std::unique_ptr<WzMap::TerrainTypeData> WzMap::loadTerrainTypes(const std::string &filename, IOProvider& mapIO, LoggingProtocol* pCustomLogger = nullptr);
```

## Saving a map:

#### High-level interface
```cpp
static bool WzMap::Map::exportMapToPath(WzMap::Map& map, const std::string& mapFolderPath, WzMap::MapType mapType, uint32_t mapMaxPlayers, WzMap::OutputFormat format, std::shared_ptr<WzMap::LoggingProtocol> logger = nullptr, std::shared_ptr<WzMap::IOProvider> mapIO = std::shared_ptr<WzMap::IOProvider>(new WzMap::StdIOProvider()));
```

#### Lower-level interface
Saving just the `game.map` / map data:
```cpp
bool WzMap::writeMapData(const WzMap::MapData& map, const std::string &filename, WzMap::IOProvider& mapIO, WzMap::OutputFormat format, WzMap::LoggingProtocol* pCustomLogger = nullptr);
```
Saving the terrain type data:
```cpp
bool WzMap::writeTerrainTypes(const WzMap::TerrainTypeData& ttypeData, const std::string& filename, WzMap::IOProvider& mapIO, WzMap::OutputFormat format, WzMap::LoggingProtocol* pCustomLogger = nullptr);
```

# Map Packages:

Map packages are `.wz` files (i.e. `.zip` files renamed to `.wz`) that contain a map's level information and map data.

The old / classic map package format uses a `<map name>.addon.lev` or  `<map name>.xplayers.lev` file in the root of the archive to describe level details, and a map folder under `multiplay/maps/<map name>` which contains all the map files & data.

The new / modern map package format places a `level.json` file (which replaces the old `.lev` files) and the map files & data in a single, self-contained directory (ex. `multiplay/maps/<map name>`) inside the archive.

`wzmaplib` provides `WzMap::MapPackage` as a high-level interface for loading map packages and exporting map packages, and permits converting between old and modern formats.

### Loading and exporting map packages:

For `WzMap::MapPackage::loadPackage` and `WzMap::MapPackage::exportMapPackageFiles`, the default `StdIOProvider` will assume that `pathToMapPackage` is a path to an extracted map package (i.e. standard filesystem I/O).

If you'd like to operate directly on zip files / `.wz` files / map package archives, you can instead initialize a `WzMapZipIO` provider via :
```cpp
auto zipIOProvider = WzMapZipIO::openZipArchiveFS("<path to zip or wz file in platform-dependent notation>");
```

> Note: You will need to ensure that the [`libzip`](https://libzip.org/) dependency is installed, compile the [`plugins/ZipIOProvider.cpp`](plugins/ZipIOProvider.cpp) file, and include `plugins/ZipIOProvider.h`

And then pass the initialized zipIOProvider into `loadPackage` / `exportMapPackageFiles` (and set `pathToMapPackage` / `basePath` to the root - i.e. `"/"` or `""`).

### Saving a new map package:

To construct a new `MapPackage` instance from new map data (which can then be exported), use the constructor:
```cpp
MapPackage(const LevelDetails& levelDetails, MapType mapType, std::shared_ptr<Map> map)
```

This requires you to construct a `WzMap::Map` instance first, with the map data, and a `LevelDetails` instance.

`LevelDetails` should always include:
- `name` (the level / map name)
- `type` (the map type)
   - note: only `MapType::SKIRMISH` is fully supported at this time
- `players` (the maximum number of players the map is designed for)
- `tileset` (the base level data set used for terrain and tiles)
   - see `enum class MAP_TILESET` in `terrain_type.h`
- `mapFolderPath` (the path to the folder containing the map data)
   - generally, you will want to set this to `multiplay/maps/<name>`
- `createdDate` (the date the map was created / saved - in `YYYY-MM-DD` format)
- `author` (the author name)
- `license` (an [SPDX license identifer or expression](https://spdx.org/licenses/))
   - examples:
      - `CC0-1.0`
      - `GPL-2.0-or-later`
      - `CC-BY-3.0 OR GPL-2.0-or-later`
      - `CC-BY-SA-3.0 OR GPL-2.0-or-later`

# Map Stats:

`wzmaplib` provides a `calculateMapStats` function (on both `Map` and `MapPackage`) to extract various statistics and info from a map.

See the `MapStats` struct in `map_stats.h`:

```cpp
struct MapStats
{
	struct StartEquality
	{
		// Whether all players have equal starting units (quantity and type)
		bool units = true;
		// Whether all players have equal starting structures (of all types - buildings, walls, etc)
		bool structures = true;
		// Whether all players have equal starting resource extractors (i.e. oil derricks)
		bool resourceExtractors = true;
		// Whether all players have equal starting power generators
		bool powerGenerators = true;
		// Whether all players have equal starting factories (of all types)
		bool factories = true;
		// Whether all players have equal starting research centers
		bool researchCenters = true;
	};

public:
	// Basic map properties
	uint32_t mapWidth = 0;
	uint32_t mapHeight = 0;
	// Scavenger counts
	uint32_t scavengerUnits = 0;
	uint32_t scavengerStructs = 0;
	uint32_t scavengerFactories = 0;
	uint32_t scavengerResourceExtractors = 0;
	// The total number of oil resources (oil well features) on the map
	uint32_t oilWellsTotal = 0;
	// Per-player counts of various starting droids and structs
	// NOTE: If the corresponding playerBalance value is false, these are equivalent to the "minimum" of that entity type per player
	uint32_t resourceExtractorsPerPlayer = 0;
	uint32_t powerGeneratorsPerPlayer = 0;
	uint32_t regFactoriesPerPlayer = 0;
	uint32_t vtolFactoriesPerPlayer = 0;
	uint32_t cyborgFactoriesPerPlayer = 0;
	uint32_t researchCentersPerPlayer = 0;
	// An analysis of the starting balance for all players
	StartEquality playerBalance;
public:
	inline bool hasScavengers() const { return scavengerUnits > 0 || scavengerStructs > 0; }
};
```

### Extracting stats / info from a map:

If you have a valid `WzMap::MapPackage`, call:
```cpp
optional<WzMap::MapStats> calculateMapStats(MapStatsConfiguration statsConfig = MapStatsConfiguration());
```

If you only have a `WzMap::Map`, call:
```cpp
optional<WzMap::MapStats> calculateMapStats(uint32_t mapMaxPlayers, MapStatsConfiguration statsConfig = MapStatsConfiguration());
```
(Passing in the appropriate `mapMaxPlayers`.)

In either case, you can accept the default built-in `MapStatsConfiguration` (which, currently, matches the base WZ 4.3+ stats), or you can utilize the `WzMap::MapStatsConfiguration::loadFrom*JSON` functions to explicitly load the data from stats .json files.

Using the `MapPackage` version of `calculateMapStats` will automatically load valid stats overrides from a map mod package. (You do not have to do this yourself.)
