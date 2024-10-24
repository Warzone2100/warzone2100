# Autorating server

When a player enters a multiplayer room, the host sends a request to the autorating url to check for custom rating (icon, medal, stars and note).

To activate autorating, you must set `autorating` to `true` in the host's configuration file. The url is defined in `autoratingurlv2`. By default it points to `https://wz2100-autohost.net/rating/`.

When autorating is not activated or the response is incorrect or empty, the default local rating is used.

Autorating is propagated to clients (when hosting) regardless of them having their own requesting enabled.
In case they do have it enabled, their version will override host-provided one.

## Autorating request

HTTP(s) request is sent to the url defined in the configuration file, with the following headers:

* `WZ-Player-Hash` sha256 hash of the player's public key.
* `WZ-Player-Key` player's hex encoded public key.
* `WZ-Locale` locale of the requesting client (to send back translated information).
* `WZ-Version` game version, as in bottom right corner in menus.

See `PlayerKeys.md` for more information about player keys.

## Autorating response

The server must return a response code 200 and some data in json format. Otherwise the response is ignored. The json data must be a single json object with the following properties:

* `autohoster` (boolean): if the player is a dedicated hoster. It will have an hoster icon instead of a medal and no stars.
* `dummy` (boolean): if there aren't enough data to rate the player. It will show a pacifier medal and no stars.
* `star` (array of three integers): the code of each star, top to bottom. `0`: none, `1`: gold, `2`: silver, `3`: bronze.
* `medal` (integer): the code of the medal to display. `0`: none, `1`: gold, `2`: silver, `3`: bronze.
* `level` (integer): 0-8, the player's level to show beside the medal (even for dummy and autohoster). They are the same as unit experience level in-game. `0` for none, up to 8 for each experience level.
* `details` (string): notes to display in the player's tooltip.
* `elo` (string): the text to display under the player's name.
* `name` (string): provides leaderboard name of the player in lobby (optional) (ignored if empty) (will be replacing player's name).
* `tag` (string): provides an tag of the player in lobby (optional) (ignored if empty) (will be displayed in top right corner).
* `nameTextColorOverride` (array of three integers): overrides name color in lobby (rgb 0-255) ([255,255,255] will result in default coloring) (optional).
* `eloTextColorOverride` (array of three integers): overrides elo text color in lobby (rgb 0-255) ([255,255,255] will result in default coloring) (optional).
* `tagTextColorOverride` (array of three integers): overrides tag text color in lobby (rgb 0-255) ([255,255,255] will result in default coloring) (optional).

### Response sample

```
{
  "dummy": false,
  "star": [3,2,1],
  "medal": 2,
  "level": 4,
  "elo": "ELO: 1283",
  "autohoster": false,
  "details": "Played 264 games, win rate: 53%",
  "name": "Flex seal",
  "tag": "Admin",
  "nameTextColorOverride": [255,255,255],
  "tagTextColorOverride": [51,255,51],
  "eloTextColorOverride": [51,255,51]
}
```

## Overriding medals and stars

Mods can change the icons for medals and stars, but it will only apply when the mod is already loaded locally when Warzone 2100 is launched. Players that join a modded game won't see the changes.

* `Gold medal`: `images/frontend/image_medal_gold.png`
* `Silver medal`: `images/frontend/image_medal_silver.png`
* `Bronze medal`: `images/frontend/image_medal_bronze.png`
* `Pacifier medal`: `images/frontend/image_medal_dummy.png`
* `Hoster medal`: `images/frontend/image_player_pc.png`
* `Gold star`: `images/frontend/image_multirank1.png`
* `Silver star`: `images/frontend/image_multirank2.png`
* `Bronze star`: `images/frontend/image_multirank3.png`

Level icons are shared with unit experience level icon, in `images/intfac/image_lev_*.png`.
