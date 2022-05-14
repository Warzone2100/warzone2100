# maptools (CLI)
Warzone 2100 Map Tools

- [`maptools package`](#maptools-package)
- [`maptools map`](#maptools-map)
- [Output Level Info Formats](#output-level-info-formats)
- [Output Map Formats](#output-map-formats)

A CLI for converting Warzone 2100 maps between different map formats, extracting information, and generating preview images.

`maptools` has subcommands to deal with both map packages (ex. `.wz` files) and directly with map folders.

#### Usage: `maptools [OPTIONS] [SUBCOMMAND]`

| [OPTION]  | Description |
| :-------- | :---------- |
| `-h`,`--help` | Print help message and exit |
| `-v`,`--verbose` | Verbose output |

| [SUBCOMMAND] | Description |
| :--- | :--- |
| [`package`](#maptools-package) | Manipulating a map package (ex. `<map>.wz` file) |
| [`map`](#maptools-map) | Manipulating a map folder |

# `maptools package`

> NOTE: To operate directly on map packages / archives (i.e. `.wz` files), `maptools` must be compiled with `libzip` support.

#### Usage: `maptools package [OPTIONS] [SUBCOMMAND]`

| [SUBCOMMAND] | Description |
| :--- | :--- |
| [`convert`](#maptools-package-convert) | Convert a map from one format to another |
| [`genpreview`](#maptools-package-genpreview) | Generate a map preview PNG |
| [`info`](#maptools-package-info) | Extract info / stats from a map package |

## `maptools package convert`

Convert a map from one format to another

#### Usage: `maptools package convert [OPTIONS] input output`

> `input` must exist, and must be a map package (.wz package, or extracted package folder)
> 
> `output` should not exist

| [OPTION]  | Description | Values | Required |
| :-------- | :---------- | :----- | :------- |
| `-h`,`--help` | Print help message and exit | | |
| `-l`,`--levelformat` | [Output level info format](#output-level-info-formats) | ENUM:value in {`lev`, `json`, `latest`} | DEFAULTS to `latest` |
| `-f`,`--format` | [Output map format](#output-map-formats) | ENUM:value in { `bjo`, `json`, `jsonv2`, `latest`} | REQUIRED |
| `-i`,`--input` | Input map package (.wz package, or extracted package folder) | TEXT:PATH | REQUIRED <sup>(may also be specified as positional parameter)</sup> |
| `-o`,`--output` | Output path | TEXT:PATH | REQUIRED <sup>(may also be specified as positional parameter)</sup> |
| `--preserve-mods` | Copy other files from the original map package (i.e. the extra files / modifications in a map-mod) | | |
| `--output-uncompressed` | Output uncompressed to a folder (not in a .wz file) | | |

> Note: When converting a script-generated map:
> - If the output format is `jsonv2` (or later) the map script will be preserved
> - If the output format is `bjo` or `json`, a warning will be output and the script-generated map will be converted to a static map

## `maptools package genpreview`

Generate a map preview PNG

#### Usage: `maptools package genpreview [OPTIONS] input output`

> `input` must exist, and must be a map package (.wz package, or extracted package folder)
> 
> `output` should not exist, and should end with `.png`

| [OPTION]  | Description | Values | Required |
| :-------- | :---------- | :----- | :------- |
| `-h`,`--help` | Print help message and exit | | |
| `-i`,`--input` | Input map package (.wz package, or extracted package folder) | TEXT:PATH | REQUIRED <sup>(may also be specified as positional parameter)</sup> |
| `-o`,`--output` | Output PNG filename (+ path) | TEXT:PATH | REQUIRED <sup>(may also be specified as positional parameter)</sup> |

## `maptools package info`

Extract info / stats from a map package to JSON

#### Usage: `maptools package info [OPTIONS] input`

> `input` must exist, and must be a map package (.wz package, or extracted package folder)

| [OPTION]  | Description | Values | Required |
| :-------- | :---------- | :----- | :------- |
| `-h`,`--help` | Print help message and exit | | |
| `-i`,`--input` | Input map package (.wz package, or extracted package folder) | TEXT:PATH | REQUIRED <sup>(may also be specified as positional parameter)</sup> |
| `-o`,`--output` | Output filename (+ path) | TEXT:PATH | |

> If `--output` is not specified, the JSON result is output to stdout

# `maptools map`

#### Usage: `maptools map [OPTIONS] [SUBCOMMAND]`

| [SUBCOMMAND] | Description |
| :--- | :--- |
| [`convert`](#maptools-map-convert) | Convert a map from one format to another |
| [`genpreview`](#maptools-map-genpreview) | Generate a map preview PNG |

## `maptools map convert`

#### Usage: `maptools map convert [OPTIONS] inputmapdir outputmapdir`

Both `inputmapdir` and `outputmapdir` must exist.

| [OPTION]  | Description | Values | Required |
| :-------- | :---------- | :----- | :------- |
| `-h`,`--help` | Print help message and exit | | |
| `-t`,`--maptype` | Map type | ENUM:value in {`campaign`,`skirmish`} | DEFAULTS to `skirmish` |
| `-p`,`--maxplayers` | Map max players | UINT:INT in [1 - 10] | REQUIRED |
| `-f`,`--format` | [Output map format](#output-map-formats) | ENUM:value in { `bjo`, `json`, `jsonv2`, `latest`} | REQUIRED |
| `-i`,`--input` | Input map directory | TEXT:DIR | REQUIRED <sup>(may also be specified as positional parameter)</sup> |
| `-o`,`--output` | Output map directory | TEXT:DIR | REQUIRED <sup>(may also be specified as positional parameter)</sup> |

## `maptools map genpreview`

#### Usage: `maptools map genpreview [OPTIONS] inputmapdir output`

Both `inputmapdir` and the parent directory for the output filename (`output`) must exist.

| [OPTION]  | Description | Values | Required |
| :-------- | :---------- | :----- | :------- |
| `-h`,`--help` | Print help message and exit | | |
| `-t`,`--maptype` | Map type | ENUM:value in {`campaign`,`skirmish`} | DEFAULTS to `skirmish` |
| `-p`,`--maxplayers` | Map max players | UINT:INT in [1 - 10] | REQUIRED |
| `-i`,`--input` | Input map directory | TEXT:DIR | REQUIRED <sup>(may also be specified as positional parameter)</sup> |
| `-o`,`--output` | Output PNG filename (+ path) | TEXT:FILE(\*.png) | REQUIRED <sup>(may also be specified as positional parameter)</sup> |

# Output Level Info Formats
| [format] | Description | flaME | WZ < 3.4 | WZ 3.4+ | WZ 4.1+ | WZ 4.3+ |
| :------- | :---------- | ----- | -------- | ------- | ------- | ------- |
| `lev` | `<map>.addon.lev` / `<map>.xplayers.lev` | :white_check_mark: | :white_check_mark: | :white_check_mark: | :white_check_mark: | :heavy_check_mark:<sup>*</sup> |
| `json` | `level.json` | | | | | :white_check_mark: |
| `latest` | _currently, an alias for `json`_ | | | | | :white_check_mark: |

<sup>*</sup> _Loadable by this version, but not recommended. May be missing features / have trade-offs._

# Output Map Formats
| [format] | Description | flaME | WZ < 3.4 | WZ 3.4+ | WZ 4.1+ |
| :------- | :---------- | ----- | -------- | ------- | ------- |
| `bjo` | Binary .BJO | :white_check_mark: | :ballot_box_with_check: | :heavy_check_mark:<sup>*</sup> | :heavy_check_mark:<sup>*</sup> |
| `json` | JSONv1 | | | :white_check_mark: | :heavy_check_mark:<sup>*</sup> |
| `jsonv2` | JSONv2 | | | | :white_check_mark: |
| `latest` | _currently, an alias for `jsonv2`_ | | | | :white_check_mark: |

<sup>*</sup> _Loadable by this version, but not recommended. May be missing features / have trade-offs._
> Note: When converting from a newer format to an older format, it is recommended that you enable `--verbose` mode, as there are certain conditions in which a 1-to-1 conversion is not possible and adjustments may be made by the converter.
