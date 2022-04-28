# wzmaplib
A self-contained library for loading / parsing / exporting WZ map formats.

- [Notes for developers](#notes-for-developers)
- [WzMap::IOProvider](#wzmapioprovider)
- [WzMap::LoggingProtocol](#wzmaploggingprotocol)
- [**Maps**](#maps)
   - [Loading a map](#loading-a-map)
   - [Saving a map](#saving-a-map)

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
static std::unique_ptr<WzMap::Map> WzMap::Map::loadFromPath(const std::string& mapFolderPath, WzMap::MapType mapType, uint32_t mapMaxPlayers, uint32_t seed, bool previewOnly = false, std::shared_ptr<WzMap::LoggingProtocol> logger = nullptr, std::shared_ptr<WzMap::IOProvider> mapIO = std::shared_ptr<WzMap::IOProvider>(new WzMap::StdIOProvider()));
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
