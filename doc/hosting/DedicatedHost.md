Running a dedicated host
========================

Warzone 2100 can run in background with a few command line arguments and some game configuration files.

Setting the server and running the game manually
------------------------------------------------

### Running the game from the command line

#### Basic command

```
warzone2100 --autohost=<host file> --gameport=<port number> --startplayers=<players> --headless --nosound
```

This command will run a game without front-end for a specific game configuration.

* `--autohost` indicates which game configuration to use. See `AutohostConfig.md` to create a file. Always use `spectatorHost: true`, the host cannot play.
* `--gameport` gives the port number to use. By default it will try to run on port 2100. Each concurrent games must have distinct ports.
* `--startplayers` holds the number of players required to start the game.
* `--headless` disables the GUI, `--nosound` disables the sound.

A default game will start if the autohost file cannot be read or contains errors. If the requested port is not available, the command will fail and stop.

#### Extended command

You can provide more arguments to fine-tune your environment.

* `--configdir=<directory>` will read the configuration file, maps, mods and store logs in the given directory. You can create a default one by providing this option on your regular game, changing your name to create the host keys and copy the newly created directory to your server.
* `--enablelobbyslashcmd` allows administrators to use slash commands to modify the settings (see below to add administrators).
* `--addlobbyadminhash=<hash>` grants admin right to the given player, identified by their hash. This argument must be repeated to add multiple administrators. See `PlayerKeys.md` for details about hashes.
* `--addlobbyadminpublickey=<public key>` grants admin rights to the given player, by their public key. This argument must be repeated to add multiple administrators. See `PlayerKeys.md` for details about keys.


#### Advanced usage

* `--enablecmdinterface=<stdin|unixsocket:path>` enables the command interface. See [/doc/CmdInterface.md](/doc/CmdInterface.md)
* `--autohost-not-ready` starts the host (autohost) as not ready, even if it's a spectator host. Should usually be combined with usage of the cmdinterface to trigger host ready via the `set host ready 1` command (or the game will never start!)


### Checking your firewall

If your server starts but nobody can join, check that your firewall is accepting incoming TCP connections on the given port.

#### With nftables

Check your current rules with `nft list ruleset`. If it is not empty check that the input chain accepts the ports you use for Warzone 2100, especially if your input chain drops unknown ports. For example:

```
table inet filter {
    chain input {
        [System stuff]
        tcp dport { 2100-2105 } ct state new accept # Warzone 2100
        counter drop
    }
    [Other chains]
}
```

### Managing a banlist

You can provide a file with banned IP which contains one ban per line. It can include wildcards.

Put this `banlist.txt` file at the root of your config directory and it will be automatically loaded at game start.


Scripting the server
--------------------

### Linux: Bash + Systemd

#### Configuring the bash scripts

The scripts under `linux_scripts` will start a game automatically by picking the next available port and select a random map from a pool. Those scripts can be installed anywhere but must be kept in the same directory.

Prerequisites:

* `bash` to run the script.
* `sed` to pick a map randomly.
* `ss` or `netstat` to check for an available port.
* `systemd` to run the scripts automatically.

Copy `common_sample.sh` to `common.sh` and update the few variables according to your environment.

Then copy and edit `game_sample.sh` for each game type you would like to run.

And try to run the server by manually calling the script, if you copied `game_sample.sh` to `game.sh`:

```
bash game.sh
```

#### Automatically run the scripts with systemd

Copy and rename a `wz2100host_sample.service` to `/etc/systemd/system/`, one for each simultaneously opened game and edit those few lines:

* `WorkingDirectory=<path>` set it to the directory where the bash scripts are.
* `ExecStart=bash <file path>` set to the game script.
* `User=<user>` pick the system user that will run the game.

Then for each service created, enable and start the service. Enabling the service will automatically run it at boot and starting the service runs it manually without requiring to reboot.

```
systemctl enable <renamed_wz2100host.service>
systemctl start <renamed_wz2100host.service>
```

You can then check the status for each game with `systemctl status <renamed_wz2100host.service>`.
