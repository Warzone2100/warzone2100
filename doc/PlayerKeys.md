# Player keys

Players are identified by their public key. Which is stored along their private key in their `.sta2` file for each name, under the `multiplay/players` directory.

Every time a player changes their name in game, an other pair of keys is generated and stored in a new `.sta2` file for that name along some other local statistics.

The public keys are shown in game to uniquely identify players outside their name. They are also sent to the autorating server, and used to identify administrators with the `--addlobbyadminhash` and `--addlobbyadminpublickey` command line arguments.

## Format

Both private and public keys are in binary format. The `sta2` file contains the private and public key on the third line, encoded together in base64.

The player's public key is displayed and sent in base64 format.

The player's hash is the sha256 digest of the public key, in hexadecimal format.

## Where to find them

Players can display their own hash and public key in the lobby by issuing the `/me` command.

The hash is also partly visible in-game and can be found within WZlogs when a player joins a game. The public key of every player met is also stored locally in `known_players.db` along their name.
