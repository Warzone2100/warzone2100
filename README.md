Warzone 2100
============

[Homepage of the Warzone 2100 Project](http://wz2100.net/)

Origins
-------

Warzone 2100, released in 1999 and developed by Pumpkin Studios, was a
ground-breaking and innovative 3D real-time strategy game.

In 2004 Eidos, in collaboration with Pumpkin Studios, decided to release
the source for the game under the terms of the GNU GPL, including everything
but the music and in-game video sequences, which were released later.

State of the game
-----------------

After the liberation of the Warzone 2100 source-code on December 6th, 2004, all
proprietary technologies have been replaced with open-source counterparts.

Right now supported platforms are Linux and Windows. It should be possible to
build it on MacOS X, and there are reports that Warzone 2100 is working on BSD,
too.

Reporting bugs
--------------

This game still has bugs and if you run into one, please use the bugtracker
(http://developer.wz2100.net/) to report this bug. In order to faster fix
those bugs we require that you follow these rules:

   1. If the game crashes you may save a memory dump. Please do so and upload it
      when reporting the bug. (Linux locates that file at /tmp/warzone2100.gdmp,
      Windows at /Program Files/Warzone 2100/warzone2100.RPT, Mac OS X by
      clicking "Details" in the crash error message)
      A self created backtrace is just as useful.
   2. Give as much information about what you were doing before the crash/bug
      occured.
   3. Try to reproduce the bug and add a description of the process to your bug-
      report.
   4. You may even upload save-games. These consist of one or two file(s) and
      one folder. All two/three are named after your save-game (e.g.
      MySaveGame.es, MySaveGame.gam and the folder MySaveGame).
   5. Bug-reports are not submit-and-forget. It may be that you forgot some
      information or forgot to upload a file. So it is, too, in your interest to
      watch the bug-report after it has been submitted. Additionally you receive
      e-mails of comments to your bug-report.

Configuration
-------------

Warzone 2100 uses an own sub-directory in a user's home directory to save
configuration data, save-games and certain other things. Additionally you can
use this directory to place custom maps and mods so the game can find them. The
location of this directory depends on the operating system.

### Warzone directory under GNU/Linux

Under GNU/Linux the warzone-dir can be found in your home-directory, it is called
".warzone2100-<version>". The leading dot indicates that it is a hidden
directory so depending on your configuration you may not be able to see it.
However, you can still access it by typing the path into your address-bar.

### Warzone directory under Windows

The directory is called "Warzone 2100 <version>" and is located in "My
Documents".

### Warzone directory under Mac OS X

The directory "Warzone 2100 <version>" can be found in your home-directory at:
~/Library/Application Support/

### Configuration file

The configuration file is just called 'config' and contains several configuration
options, some of them can be changed by using command-line options or using
the in-game menus, others can only be changed by editing the file by hand.

If at any point you did something wrong, you can delete the old configuration
file and just restart Warzone 2100. Then the game will regenerate a new 
configuration file with default values.

Command-line options
--------------------

Warzone 2100 can be started with different options and arguments. For a list
of these options, run the game with the --help option.

Notes: These options all have two dashes (--), not one dash only (-). Also,
if the option has an argument, you need to separate the option and its argument
with a '=' sign - spaces do not work.

Note: Some options have corresponding entries in the configuration-file and will
persist from one start of Warzone 2100 to the next.

Multiplaying via internet
-------------------------

There are two methods to start a multiplayer-game via internet: using the host's
IP or using the lobby-server. Make sure to be able to communicate on TCP-ports
2100 and 9999. Note that for port forwarding, you only need to configure your
router to forward port 2100.

You can choose whether to connect via Lobby or IP:

* If you choose IP, Warzone 2100 asks you for the ip-address of the host and
  will try to connect to that IP.
* If you choose Lobby, Warzone 2100 will connect to the lobby server, as long as
  the lobby-server-address in your config-file has not been changed.

You will see a list of games from which you can select one.

You can kick unwanted players out of not-yet-started game by clicking left on
them while holding the right mouse button.

When you are hosting a game it will automatically be listed on the lobby-server.
If you do not want your games to be listed on the lobby-server, you have to
change the entry "masterserver_name=lobby.wz2100.net" in your config to some-
thing invalid as "nomasterserverplease".

If you then want to see the games that are listed on the lobby-server you may
enter "lobby.wz2100.net" when prompted to enter the host's IP or change the
entry in the config-file back. You will have to restart Warzone 2100 in order
for config-changes to take effect.

Cheats
------

Like many other games Warzone 2100 features a certain set of cheats that can be
used to have an advantage in the singleplayer-campaign and skirmish-games, or to
just help mod- and map-makers with testing. Cheats do not work in multiplayer,
unless all players agree to it.

Cheats are likely to contain or trigger bugs, so use with care, especially
during campaign.

### Entering cheat mode

To be able cheats while in-game, press shift and backspace simultaneously.
An on-screen message should appear telling you that cheat-mode has been enabled.
You can disable it using the same key combination again.

Pressing ctrl+o opens up the debug menu, which is useful for inspecting the
game state, or just messing around.

### Cheat commands

After activating cheat-mode cheats can be entered using the normal chat-
function. Cheats are ordered by their use and where they can be used.

There are many cheat commands. Some examples:

* "biffer baker" - Your units do more damage and are stronger
* "double up" - Your units are twice as strong
* "give all" - Allows you to build and research everything
* "work harder" - All currently active research topics are instantly researched
* "research all" - Everything is researched instantly
* "let me win" - You win the current campaign mission
* "superpower" - Gives you maximum power

Modding information
-------------------

Warzone AI, maps and campaign can be scripted using javascript.

Links to further information
* [Model format](doc/PIE.md)
* [Animation](doc/Animation.md)

How to build
-------------------

### Linux
See http://developer.wz2100.net/wiki/CompileGuideLinux

### Windows Cross compile
See http://developer.wz2100.net/wiki/CompileGuideWindows/Cross

### Windows using MSVC

* You need Visual Studio 2015 or 2017 (recommended) CMake (https://cmake.org/) and QT 5.9.1 (https://www.qt.io/)
* Build dependencies are provided with vcpkg project from Microsoft. Run "get-dependencies.ps1" script from powershell in order to download source and build them.
* From now you can use cmake the canonical way or directly from Visual Studio 2017 ; the only requirement is to pass the CMAKE_TOOLCHAIN_FILE=vcpkg/scripts/buildsystem/vcpkg.cmake variable.
* The canonical way is the command 'cmake -H. -DCMAKE_TOOLCHAIN_FILE=vcpkg\scripts\buildsystem\vcpkg.cmake -Bbuild -G "Visual Studio 14 2015" '
* With VS2017 open the warzone 2100 folder, then  "CMake menu > Change Cmake settings" will create a json file. There add "-DCMAKE_TOOLCHAIN_FILE=vcpkg\\scripts\\buildsystems\\vcpkg.cmake" to cmakeCommandArgs.
