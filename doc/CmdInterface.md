# Warzone 2100 command interface

In version 4.2.0, an `--enablecmdinterface` CLI option was introduced that enables
partial administrative control of Warzone 2100 instance via `stdin` / `stderr`.

> `stdout` is already used by numerous other aspects of WZ's logging and other subsystems, so (for now) `stderr` is used for Command Interface output

All messages are sent in plain-text UTF-8 and end with 0x0a (`\n`).

## `stderr` state / response messages

`WZCMD: ` is for stdin command interface (and responses to commands)\
`WZCHATCMD: ` is for in-lobby chat commands and messages\
`WZEVENT: ` is for instance-related events like player join or game start

* `WZCMD: stdinReadReady`\
	`stdinReadReady` message signals support for stdin pipe commands
	(see section [`stdin`](#stdin-commands))

* `WZCMD error: <error message>`\
	`error` messages describe errors relevant to the command interface\
	(example: fail to read command from `stdin` will produce `WZCMD error: getline failed!`)

* `WZCMD info: <info message>`\
	`info` messages carry non-critical data, but can help check that
	`stdin` commands are executed correctly.

* `WZCHATCMD: <index> <ip> <hash> <b64pubkey> <b64name> <b64msg>`\
	Passes a lobby slash command message - to log use of slash commands or implement additional operations.\
	Fields after `WZCHATCMD: `:
	- `%i` **index** of player
	- `%s` IP address of player in plain text (v4/v6)
	- `%s` public hash string of the player (max 64 bytes)
	- `%s` base64-encoded public key of the player
	- `%s` base64-encoded name of the player
	- `%s` base64-encoded chat message content

* `WZEVENT: join approval needed: <joinid> <ip> <hash> <b64pubkey> <b64name> [spec|play]`\
	Passes a join request that requires approval (if `--async-join-approve` is enabled)\
	**Important: If the cmdinterface client does not respond to this request with a `join <approve/reject>` command in a timely manner, the joiner will be kicked (or drop).**\
	Fields:
	- `%s` a unique **joinid** to refer to the joining connection (should be passed to `join <approve/reject>`)
	- `%s` IP address of player in plain text (v4/v6)
	- `%s` public hash string of the player (max 64 bytes)
	- `%s` base64-encoded public key of the player
	- `%s` base64-encoded name of the player
	- `%s` either `spec` or `play` to distinguish whether the request is for joining spectators or players

* `WZEVENT: bancheck: <ip>`\
	Passes IP of connected player to initiate outside-check.

* `WZEVENT: lobbyid: <gid>`\
	Game id, announced as assigned from lobby.

* `WZEVENT: startMultiplayerGame`\
	Marks the start of loading of a multiplayer game.

* `WZEVENT: lag-kick: <index?position> <ip>`\
	Notifies about player being kicked from the game due to connection issues.

* `WZEVENT: lobbyerror (<code>): <b64 motd>`\
  `WZEVENT: lobbysocketerror: [b64 motd]`\
  `WZEVENT: lobbyerror (<code>): Cannot resolve lobby server: <socket error>`\
	Signals about lobby error. (motd is base64-encoded)

# `stdin` commands

`stdin` interface is super basic but at the same time a powerful tool for automation.

All messages **must end with `0x0a` (`\n`) in order to be processed!**

If state of interface buffer is unknown and/or corrupted, interface can send a few `0x0a` in order to clean reader buffer.

* `exit`\
	Shuts down **stdin reader interface**

* `admin add-hash <hash>`\
	Adds hash to the list of room admins

* `admin add-public-key <pkey>`\
	Adds public key to the list of room admins

* `admin remove <pkey|hash>`\
	Removes admin from room admins list by public key or hash

* `join <approve|reject> <joinid>`\
	Approve or reject an attempt to join the game.\
	Pass in the `<joinid>` received in a `WZEVENT: join approval needed` event.

* `ban ip <ip>`\
	Find and kick (with adding to ip banlist) player with specified ip
	(result of `WZEVENT: bancheck:` from outside)

* `permissions set connect:allow <pkey|hash>`\
	Allows player with the specified identity to connect (bypassing any ip bans)

* `permissions set connect:block <pkey|hash>`\
	Blocks the specified identity from connecting. Kicks any already-connected player with this identity.

* `permissions unset connect <pkey|hash>`\
	Removes any previously-set connect permission (allow / block) for the specified identity

* `set chat <allow|quickchat> <all|newjoin|host|pkey|hash>`\
	Allows or mutes chat.
	- Parameter 1: If "allow" is specified, allows all chat (both free chat and quick chat). If "quickchat" is specified, mutes / disallows free chat (but still allows quick chat).
	- Parameter 2: If "all" is specified instead of an identity, applies to all. If "newjoin" is specified instead of an identity, applies to future joins.
	
* `chat bcast <message [^\n]>`\
	Send system level message to the room from stdin.

* `shutdown now`\
	Trigger graceful shutdown of the game regardless of state.
