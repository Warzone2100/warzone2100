# Autohost configuration

Running the game with the `--autohost` command line argument opens a game automatically. Combined with `--headless` and `--nosound` it allows to run a dedicated server to host a game. See also `PlayerKeys.md` to add administrators with `--addlobbyadminhash` and `--addlobbyadminpublickey`.

The autohost option requires a configuration file set in your data directory, under the `autohost` directory. For example : `warzone2100 --autohost=my_host` will look for the configuration file `autohost/my_host`.

This configuration file is a json file, with multiple entries. If the game fails to read that file, it will log an error and run a default game.

## The `challenge` object

The `challenge` object defines the game parameters for a multiplayer game.

* `map` holds the map name. It must match the name set in it's `.lev` under the `level` entry. Most of the time it matches the name displayed in game, custom maps build with FlaME can have a `-T1` suffix.
* `maxPlayers` sets the number of player slots, including AIs.
* `scavengers` sets which scavengers should be in game : `0` for none, `1` for basic scavengers, `2` for ultimate scavengers.
* `alliances` sets the alliance mode. `0` for free for all, `1` for allow alliances, `2` for fixed teams, `3` for fixed teams without research sharing.
* `powerLevel` sets the power generation rate. `0` for low, `1` for medium, `2` for high.
* `bases` sets the starting base. `0` for no base, `1` for small base, `2` for advanced base.
* `name` your game name, as it will be shown in the lobby.
* `techLevel` sets the starting technology level. `1` for level 1 (wheel), `2` for level 2 (water mill), `3` for level 3 (chip), `4` for level 4 (computer).
* `spectatorHost` when `true` or `1`, the host will spectate the game. When `false` or `0`, the host will play the game.
* `openSpectatorSlots` defines how much spectator slots are opened (one more is opened for the host when spectating).
* `allowPositionChange` is deprecated, use the `locked` object instead.

## the `locked` object

The `locked` object sets which game parameters can be changed by the host and room administrators. Those values can be `true` or `1` if the parameter cannot be changed, or `false` or `0`. When an entry is not set, it is unlocked (same as `false`).

* `power` locks the `powerLevel` value.
* `alliances` locks to the `alliances` value.
* `teams` prevents player from switching teams.
* `difficulty` locks the AI difficulty.
* `ai` locks AI scripts.
* `scavengers` locks the `scavengers` value.
* `position` prevents player from changing their starting location.
* `bases` locks the `bases` value.

## Player objects

Each player slot can be customized, starting from 0. The first slot will be defined in the `player_0` object, second slot in `player_1` and so on. Each can have the following entries:

* `position` move the player to an other slot, starting from 0.
* `team` holds the team number, starting from 0.
* `ai` when set, contains the name of the AI to use. This must match the property `js` in the ai `.json` file.
* `difficulty` sets the difficulty for an AI. It can be one of `Easy`, `Medium`, `Hard` or `Insane`.
* `name` sets a custom name for the AI.

## Sample file

```
{
	"locked": {
		"power": false,
		"alliances": true,
		"teams": true,
		"difficulty": false,
		"ai": false,
		"scavengers": true,
		"position": false,
		"bases": true
	},
	"challenge": {
		"map": "Sk-Mountain",
		"maxPlayers": 4,
		"scavengers": 0,
		"alliances": 2,
		"powerLevel": 1,
		"bases": 1,
		"name": "2v2 sample game",
		"techLevel": 1,
		"spectatorHost": true,
		"openSpectatorSlots": 2,
		"allowPositionChange": true
	},
	"player_0": {
		"team": 0
	},
	"player_1": {
		"team": 0
	},
	"player_2": {
		"team": 1
	},
	"player_3": {
		"team": 1
	}
}
```
