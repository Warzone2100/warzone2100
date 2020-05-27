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

Right now supported platforms are Linux, Windows, and macOS. There are reports 
that Warzone 2100 is working on BSD, too.

Latest development builds
-------------------------

[![Windows Build Status](https://img.shields.io/github/workflow/status/Warzone2100/warzone2100/Windows/master?label=Windows&logo=windows)](https://github.com/Warzone2100/warzone2100/actions?query=workflow%3AWindows+branch%3Amaster+event%3Apush)
 [![macOS Build Status](https://img.shields.io/azure-devops/build/wz2100/27126038-ce95-47c8-8726-aaba57651f31/1/master?label=macOS&logo=apple&logoColor=FFFFFF)](https://dev.azure.com/wz2100/warzone2100/_build/latest?definitionId=1&branchName=master)
 [![Ubuntu Build Status](https://img.shields.io/github/workflow/status/Warzone2100/warzone2100/Ubuntu/master?label=Ubuntu&logo=ubuntu&logoColor=FFFFFF)](https://github.com/Warzone2100/warzone2100/actions?query=workflow%3AUbuntu+branch%3Amaster+event%3Apush)
 [![Fedora Build Status](https://img.shields.io/github/workflow/status/Warzone2100/warzone2100/Fedora/master?label=Fedora&logo=fedora&logoColor=FFFFFF)](https://github.com/Warzone2100/warzone2100/actions?query=workflow%3AFedora+branch%3Amaster+event%3Apush)
 [![FreeBSD Build Status](https://img.shields.io/cirrus/github/Warzone2100/warzone2100/master?label=FreeBSD&logo=FreeBSD)](https://cirrus-ci.com/github/Warzone2100/warzone2100/master)
 [![Travis CI Build Status](https://img.shields.io/travis/Warzone2100/warzone2100/master?label=ARM64%20%2F%20Misc)](https://travis-ci.org/Warzone2100/warzone2100)
 [![Packaging status](https://repology.org/badge/tiny-repos/warzone2100.svg)](https://repology.org/project/warzone2100/versions)

### Windows

How to get the latest Windows development builds:
1. View the **[latest successful Windows builds](https://github.com/Warzone2100/warzone2100/actions?query=workflow%3AWindows+branch%3Amaster+event%3Apush+is%3Asuccess)**.
2. Select the latest workflow run in the table / list.
   This should display a list of **Artifacts** from the run.
3. Download the `warzone2100_win_x86_portable` or `warzone2100_win_x86_installer` artifact (depending on whether you want the portable build or the full / regular installer).
> Note: A free GitHub account is currently required to download the artifacts.

### macOS

How to get the latest macOS development builds:
1. View the **[latest macOS build](https://dev.azure.com/wz2100/warzone2100/_build/latest?definitionId=1&branchName=master)**.
2. Under the **`Artifacts:`** column there should be a link titled `<#> published`.
   Click it to view the build artifacts.
3. Download the `warzone2100_macOS` artifact (a download icon will appear to the right side of the row when you mouse-over).

### Ubuntu 18.04+

How to get the latest Ubuntu development builds:
1. View the **[latest successful Ubuntu builds](https://github.com/Warzone2100/warzone2100/actions?query=workflow%3AUbuntu+branch%3Amaster+event%3Apush+is%3Asuccess)**.
2. Select the latest workflow run in the table / list.
   This should display a list of **Artifacts** from the run.
3. Download the `warzone2100_ubuntu_amd64_deb` artifact.
> Note: A free GitHub account is currently required to download the artifacts.
4. Extract the contents of the downloaded .zip (`warzone2100_ubuntu_amd64.deb`) to your Desktop.
5. Execute the following commands in Terminal:
```
cd ~/Desktop
sudo apt install ./warzone2100_ubuntu_amd64.deb
```

### Linux (from source)

Clone this Git repo and build, following the instructions under:
[How to Build](#how-to-build)

> Development builds are a snapshot of the current state of development, from the 
> latest (successfully-built) commit. Help testing these builds is always welcomed,
> but they should be considered a work-in-progress.

Reporting bugs
--------------

This game still has bugs and if you run into one, please use the GitHub bugtracker
(https://github.com/Warzone2100/warzone2100/issues) to report this bug. In order to faster fix
those bugs we require that you follow these rules:

   1. If the game crashes you may save a memory dump. Please do so and upload it
      when reporting the bug. (Linux locates that file at /tmp/warzone2100.gdmp,
      Windows at /Program Files/Warzone 2100/warzone2100.RPT, macOS by
      clicking "Details" in the crash error message)
      A self created backtrace is just as useful.
   2. Give as much information about what you were doing before the crash/bug
      occurred.
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

Under GNU/Linux, Warzone conforms to the [XDG base directory spec](https://standards.freedesktop.org/basedir-spec/basedir-spec-latest.html).

By default, the directory `warzone2100-<version>` can be found in your home-directory 
under the path `~/.local/share/`.
(If the `XDG_DATA_HOME` environment variable is defined, the Warzone folder will
be located within `$XDG_DATA_HOME`.)

The leading dot in the `.local` part of the path indicates that it is a hidden
directory, so depending on your configuration you may not be able to see it. 
However, you can still access it by typing the path into your address-bar.

### Warzone directory under Windows

The directory `Warzone 2100 Project\Warzone 2100 <version>` is located under the 
`%APPDATA%` folder.

Typical `%APPDATA%` paths:
- Windows XP: `\Documents and Settings\$USER$\Application Data`
- Windows Vista+: `\Users\$USER$\AppData\Roaming`

Hence, the default path for the Warzone configuration data on Windows Vista+ would be:
`C:\Users\$USER$\AppData\Roaming\Warzone 2100 Project\Warzone 2100 <version>\`

By default, the `%APPDATA%` folder is hidden. Entering:
`%APPDATA%\Warzone 2100 Project\` into the address bar of Windows Explorer
will browse to your Warzone directory.

### Warzone directory under macOS

The directory `Warzone 2100 <version>` can be found in your home-directory at:
`~/Library/Application Support/`

By default, recent version of macOS hide your account's Library folder. To view it in
**Finder**, hold down the **Option (⌥)** key while clicking the **Go** menu, and your Library folder
will appear as a menu choice.

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
* [Scripting](doc/Scripting.md)
* [Model format](doc/PIE.md)
* [Animation](doc/Animation.md)

How to build
-------------------

### Getting the Source

To properly build the game, either:
- Download a release `tar.xz`, which contains all the source code and revision information.

  _OR_

- Clone the Git repo:
  ```
  git clone https://github.com/Warzone2100/warzone2100.git
  cd warzone2100
  git fetch --tags
  git submodule update --init --recursive
  ```
  > Note: Initializing submodules is required.

Do **not** use GitHub's "Download Zip" option, as it does not contain submodules or the Git-based autorevision information.

### Linux
See http://developer.wz2100.net/wiki/CompileGuideLinux

### Windows Cross compile
See http://developer.wz2100.net/wiki/CompileGuideWindows/Cross

### Windows using MSVC

* Prerequisites
   * **Visual Studio 2017** (Visual Studio 2015 may work, but 2017 is strongly encouraged)
      - If you do not already have Visual Studio installed, you can download the free **Visual Studio Community** from: https://developer.microsoft.com/en-us/windows/downloads
      - IMPORTANT: You need the fully-featured Visual Studio IDE. “Visual Studio Code” does not include the necessary support for building C++ Windows apps.
   * **CMake 3.10+** (https://cmake.org/)
   * **Qt 5.9.1+** (https://www.qt.io/)
     - Here's a direct link to the [Qt 5.9.9 installer](https://download.qt.io/archive/qt/5.9/5.9.9/).
     - You will probably need to register for a Qt account during the installation, along with activating your email.
     - Required components to install: MSVC 2015, Qt Script (even though it's labelled as deprecated). The MSVC 2015 components will work on MSVC 2017 / 2019, and 32-bit versions work on 64-bit platforms.
   * **Git** (if not building from a release source archive)
   * **7-Zip** (http://www.7-zip.org)
* **Preparing to build:**
   Build dependencies are provided via [vcpkg](https://github.com/Microsoft/vcpkg) from Microsoft.
   * Run the `get-dependencies_win.ps1` script from powershell in order to download and build the dependencies.
* **Building from the command-line:**
   * Change directory to the warzone2100 repo directory
   * Configure
      * Visual Studio 2017: `cmake -H. -DCMAKE_TOOLCHAIN_FILE=vcpkg\scripts\buildsystems\vcpkg.cmake -Bbuild -G "Visual Studio 15 2017"`
      * Visual Studio 2015: `cmake -H. -DCMAKE_TOOLCHAIN_FILE=vcpkg\scripts\buildsystems\vcpkg.cmake -Bbuild -G "Visual Studio 14 2015"`
   * Build
      * Release: `cmake --build build --config Release`
      * Debug: `cmake --build build --config Debug`
* **Building using Visual Studio 2017:**
   1. Open Visual Studio 2017
   2. Open the warzone2100 folder using **File** > **Open** > **Folder...**
      - Allow Visual Studio some time to load the project and retrieve information from CMake.
   3. Create a VS CMake settings JSON file using **CMake** > **Change CMake settings**. You can also reach this dialog by clicking "Manage Configurations" in the configuration dropdown in the toolbar. Make sure the CMake components in Visual Studio are installed (by running the Visual Studio Installer).
      - This creates `CMakeSettings.json`
   4. Add the following variables to `CMakeSettings.json`:
      - To `cmakeCommandArgs`, add: `-DCMAKE_TOOLCHAIN_FILE=vcpkg\scripts\buildsystems\vcpkg.cmake -DCMAKE_PREFIX_PATH=C:\Qt\Qt5.9.9\5.9.9\msvc2015` (check your Qt installation path)
      - If you're running a 64-bit platform, you need to set the compilation to 32-bit by setting `inheritEnvironments` to `[ "msvc_x86_x64" ]`
      - Note: Visual Studio automatically escapes and turns each `\` into `\\`
   5. After letting Visual Studio re-run CMake configure with the new settings, you can build using the **CMake** menu.

### macOS
See [macosx/README.md](macosx/README.md)
