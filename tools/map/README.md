# maptools (CLI)
Warzone 2100 Map Tools

#### Usage: `maptools [OPTIONS] [SUBCOMMAND]`

| [OPTION]  | Description |
| :-------- | :---------- |
| `-h`,`--help` | Print help message and exit |
| `-v`,`--verbose` | Verbose output |

| [SUBCOMMAND] | Description |
| :--- | :--- |
| `maptools convert` | Convert a map from one format to another |
| `maptools genpreview` | Generate a map preview PNG |

## `maptools convert`

#### Usage: `maptools convert [OPTIONS] inputmapdir outputmapdir`

Both `inputmapdir` and `outputmapdir` must exist.

| [OPTION]  | Description | Values | Required |
| :-------- | :---------- | :----- | :------- |
| `-h`,`--help` | Print help message and exit | | |
| `-t`,`--maptype` | Map type | ENUM:value in {`campaign`,`skirmish`} | DEFAULTS to `skirmish` |
| `-p`,`--maxplayers` | Map max players | UINT:INT in [1 - 10] | REQUIRED |                             
| `-f`,`--format` | Output map format | ENUM:value in { `bjo`, `json`, `jsonv2`, `latest`} | REQUIRED |
| `-o`,`--output` | Output map directory | TEXT:DIR | REQUIRED <sup>(may also be specified as positional parameter)</sup> |

### Output Map Formats (+ Compatibility)
| [format] | Description | flaME | WZ < 3.4 | WZ 3.4+ | WZ 4.1+ |
| :------- | :---------- | ----- | -------- | ------- | ------- |
| `bjo` | Binary .BJO | :white_check_mark: | :ballot_box_with_check: | :heavy_check_mark:<sup>*</sup> | :heavy_check_mark:<sup>*</sup> |
| `json` | JSONv1 | | | :white_check_mark: | :heavy_check_mark:<sup>*</sup> |
| `jsonv2` | JSONv2 | | | | :white_check_mark: |
| `latest` | _currently, an alias for `jsonv2`_ | | | | :white_check_mark: |

<sup>*</sup> _Loadable by this version, but not recommended. May be missing features / have trade-offs._
> Note: When converting from a newer format to an older format, it is recommended that you enable `--verbose` mode, as there are certain conditions in which a 1-to-1 conversion is not possible and adjustments may be made by the converter.

## `maptools genpreview`

#### Usage: `maptools genpreview [OPTIONS] inputmapdir output`

Both `inputmapdir` and the parent directory for the output filename (`output`) must exist.

| [OPTION]  | Description | Values | Required |
| :-------- | :---------- | :----- | :------- |
| `-h`,`--help` | Print help message and exit | | |
| `-t`,`--maptype` | Map type | ENUM:value in {`campaign`,`skirmish`} | DEFAULTS to `skirmish` |
| `-p`,`--maxplayers` | Map max players | UINT:INT in [1 - 10] | REQUIRED |                             
| `-o`,`--output` | Output PNG filename (+ path) | TEXT:FILE(\*.png) | REQUIRED <sup>(may also be specified as positional parameter)</sup> |
