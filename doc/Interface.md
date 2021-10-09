# Warzone 2100 pipe text interface

In version 4.2.0-beta1 flag `enablecmdinterface` was introduced that enabled
partial administrative control of Warzone 2100 instance via pipes/stdout.

All messages is sent in plain-text UTF-8 compatability mode and end with 0x0a.

## `stdout` messages
`WZCMD: ` is for stdin command interface\
`WZCHATCMD: ` is for in-lobby chat commands and messages\
`WZEVENT: ` is for instance-related events like player join or game start

* `WZCMD: stdinReadReady`\
	`stdinReadReady` message signals support for stdin pipe commands
	(see section `stdin`)

* `WZCMD error: <error message>`\
	`error` messages describe errors relevant to the pipe interface
	(example: fail to read command from `stdin` will
	produce `WZCMD error: getline failed!`)

* `WZCMD info: <info message>`\
	`info` messages carry non-critical data but can help check that
	`stdin` commands are executed correctly.

* `WZCHATCMD: <index> <ip> <hash> <pkey> <b64name> <b64msg>`\
	Chat passby message. Used to store chat messages in machine readable format.
	Fields after `WZCHATCMD: `:
	- `%i` **index** of player
	- `%s` IP address of player in plain text (v4/v6)
	- `%s` public hash string of the player (max 64 bytes)
	- `%s` base64-encoded public key of the player
	- `%s` base64-encoded name of the player
	- `%s` base64-encoded chat message content

* `WZEVENT: bancheck: <ip>`\
	Passes IP of connected player to initiate outside-check.

* `WZEVENT: lobbyid: <gid>`\
	Game id, announced as assigned from lobby.

* `WZEVENT: startMultiplayerGame`\
	Marks start of loading of multiplayer game.

* `WZEVENT: lag-kick: <index?position> <ip>`\
	Notifies about player being kicked from the game due to connection issues.

* `WZEVENT: lobbyerror (<code>): <b64 motd>`\
  `WZEVENT: lobbysocketerror: [b64 motd]`\
  `WZEVENT: lobbyerror (<code>): Cannot resolve lobby server: <socket error>`\
	Signals about lobby error. (motd is not base64encoded on tag 4.2.0-beta1)

# `stdin` commands
`stdin` interface is super basic but at the same time powerful tool for
automation.

All messages **must end with 0x0a in order to be processed!**
If state of interface buffer is unknown and/or corrupted, interface can send few
0x0a in order to clean reader buffer.

* `exit`\
	Shutdowns **reader interface**

* `admin add-hash <hash>`\
	Adds hash to the list of room admins

* `admin add-public-key <pkey>`\
	Adds public key to the list of room admins

* `admin remove <pkey|hash>`\
	Removes admin from room admins list by public key or hash
	(on tag 4.2.0-beta1 removal by hash does not work)

* `ban ip <ip>`\
	Find and kick (with adding to ip banlist) player with specified ip
	(result of `WZEVENT: bancheck:` from outside)

* `chat bcast <message [^\n]>`\
	Send system level message to the room from stdin.

* `shutdown now`\
	Trigger graceful shutdown of the game regardless of state.

