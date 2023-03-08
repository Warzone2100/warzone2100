Warzone 2100
============

[Homepage of the Warzone 2100 Project](https://wz2100.net/)

Origins
-------

Warzone 2100, released in 1999 and developed by Pumpkin Studios, is a
ground-breaking and innovative 3D real-time strategy game.

In 2004 Eidos, in collaboration with Pumpkin Studios, decided to release
the source for the game under the terms of the GNU GPL, including everything
but the music and in-game video sequences, which were released later.

State of the game
-----------------

After the liberation of the Warzone 2100 source-code on December 6th, 2004, all
proprietary technologies have been replaced with open-source counterparts.

Currently, the supported platforms are Linux, Windows, and macOS. There are reports
that Warzone 2100 is also working on BSD.

Installation
------------

Visit https://wz2100.net for the latest stable release for Windows, macOS and Ubuntu.

Note for videos in Ubuntu: For important information during the game, download the videos manually. Assuming the game is installed in the standard `~/.local/share/` folder, use these commands:
```shell
mkdir ~/.local/share/warzone2100
wget https://github.com/Warzone2100/wz-sequences/releases/download/v3/standard-quality-en-sequences.wz -O ~/.local/share/warzone2100/sequences.wz
```

Latest development builds
-------------------------


[![Windows Build Status](https://img.shields.io/github/actions/workflow/status/Warzone2100/warzone2100/CI_windows.yml?branch=master&label=Windows&logo=windows)](https://github.com/Warzone2100/warzone2100/actions?query=workflow%3AWindows+branch%3Amaster+event%3Apush)
 [![macOS Build Status](https://img.shields.io/github/actions/workflow/status/Warzone2100/warzone2100/CI_macos.yml?branch=master&label=macOS&logo=apple)](https://github.com/Warzone2100/warzone2100/actions?query=workflow%3AmacOS+branch%3Amaster+event%3Apush)
 [![Ubuntu Build Status](https://img.shields.io/github/actions/workflow/status/Warzone2100/warzone2100/CI_ubuntu.yml?branch=master&label=Ubuntu&logo=ubuntu&logoColor=FFFFFF)](https://github.com/Warzone2100/warzone2100/actions?query=workflow%3AUbuntu+branch%3Amaster+event%3Apush)
 [![Fedora Build Status](https://img.shields.io/github/actions/workflow/status/Warzone2100/warzone2100/CI_fedora.yml?branch=master&label=Fedora&logo=fedora&logoColor=FFFFFF)](https://github.com/Warzone2100/warzone2100/actions?query=workflow%3AFedora+branch%3Amaster+event%3Apush)
 [![FreeBSD Build Status](https://img.shields.io/cirrus/github/Warzone2100/warzone2100/master?label=FreeBSD&logo=FreeBSD)](https://cirrus-ci.com/github/Warzone2100/warzone2100/master)
 [![Drone Cloud CI ARM64 Build Status](https://img.shields.io/drone/build/Warzone2100/warzone2100/master?label=ARM64%20Linux)](https://cloud.drone.io/Warzone2100/warzone2100)
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
1. View the **[latest successful macOS builds](https://github.com/Warzone2100/warzone2100/actions?query=workflow%3AmacOS+branch%3Amaster+event%3Apush+is%3Asuccess)**.
2. Select the latest workflow run in the table / list.
   This should display a list of **Artifacts** from the run.
3. Download the `warzone2100_macOS_universal` or `warzone2100_macOS_universal_novideos` artifact (depending on whether you want the full app bundle or not).
> Note: A free GitHub account is currently required to download the artifacts.

### Ubuntu

How to get the latest Ubuntu development builds:
1. View the **[latest successful Ubuntu builds](https://github.com/Warzone2100/warzone2100/actions?query=workflow%3AUbuntu+branch%3Amaster+event%3Apush+is%3Asuccess)**.
2. Select the latest workflow run in the table / list.
   This should display a list of **Artifacts** from the run.
3. Download the appropriate `warzone2100_ubuntu<version>_amd64_deb` artifact.
   - If you are running Ubuntu 18.04: `warzone2100_ubuntu18.04_amd64_deb`
   - If you are running Ubuntu 20.04: `warzone2100_ubuntu20.04_amd64_deb`
> Note: A free GitHub account is currently required to download the artifacts.
4. Extract the contents of the downloaded .zip (`warzone2100_ubuntu<version>_amd64.deb`) to your Desktop.
5. Execute the following commands in Terminal:
```shell
cd ~/Desktop
sudo apt install ./warzone2100_ubuntu<version>_amd64.deb
```
6. Download the video for crucial information during the game, for more see "Videos" section. Assuming the game is installed in the standard `~/.local/share/` folder, use this command (update `warzone2100-<version>`):
```shell
wget https://github.com/Warzone2100/wz-sequences/releases/download/v3/standard-quality-en-sequences.wz -O ~/.local/share/warzone2100-<version>/sequences.wz
```

### Linux (from source)

Clone this Git repo and build, following the instructions under:
[How to Build](#how-to-build)

> Development builds are a snapshot of the current state of development, from the
> latest (successfully-built) commit. Help testing these builds is always welcomed,
> but they should be considered a work-in-progress.

### Videos
You can download videos from [here](https://github.com/Warzone2100/wz-sequences/releases/tag/v3), or [here](https://sourceforge.net/projects/warzone2100/files/warzone2100/Videos/). You will need to rename the downloaded file to `sequences.wz`, and place it into your Warzone 2100 directory, as described above.
Note that `.wz` files are just `.zip` in disguise, you can rename it and extract the content if wish to inspect them.

Reporting bugs
--------------

This game still has bugs and if you run into one, please use the GitHub bugtracker
(https://github.com/Warzone2100/warzone2100/issues) to report the bug. In order to fix
those bugs more quickly, we require that you follow these rules:

   1. If the game crashes you may save a memory dump. Please do so and upload it
      when reporting the bug. (Linux locates that file at /tmp/warzone2100.gdmp,
      Windows at /Program Files/Warzone 2100/warzone2100.RPT, macOS by
      clicking "Details" in the crash error message)
      A self created backtrace is just as useful.
   2. Give as much information about what you were doing before the crash/bug
      occurred.
   3. Try to reproduce the bug and add a description of the process to your bug
      report.
   4. You may even upload save files. These consist of one or two file(s) and
      one folder. All two/three are named after your savegame (e.g.
      MySaveGame.es, MySaveGame.gam and the folder MySaveGame).
   5. Bug reports are not submit-and-forget. It may be that you forgot some
      information or forgot to upload a file. So it is also in your interest to
      watch the bug-report after it has been submitted. Additionally, you can enable
      e-mails of comments to your bug report.

Configuration
-------------

Warzone 2100 uses its own subdirectory in a user's home directory to save
configuration data, save files and certain other things. Additionally you can
use this directory to place custom maps and mods so the game can find them. The
location of this directory depends on the operating system.

### Warzone 2100 directory under GNU/Linux

Under GNU/Linux, Warzone 2100 conforms to the [XDG base directory spec](https://standards.freedesktop.org/basedir-spec/basedir-spec-latest.html).

By default, the directory `warzone2100-<version>` can be found in your home directory
under the path `~/.local/share/`.
(If the `XDG_DATA_HOME` environment variable is defined, the Warzone 2100 folder will
be located within `$XDG_DATA_HOME`.)

The leading dot in the `.local` part of the path indicates that it is a hidden
directory, so depending on your configuration you may not be able to see it.
However, you can still access it by typing the path into your address bar.

### Warzone 2100 directory under Windows

The directory `Warzone 2100 Project\Warzone 2100 <version>` is located under the
`%APPDATA%` folder.

Typical `%APPDATA%` paths:
- Windows XP: `\Documents and Settings\$USER$\Application Data`
- Windows Vista+: `\Users\$USER$\AppData\Roaming`

Hence, the default path for the Warzone 2100 configuration data on Windows Vista+ would be:
`C:\Users\$USER$\AppData\Roaming\Warzone 2100 Project\Warzone 2100 <version>\`

By default, the `%APPDATA%` folder is hidden. Entering:
`%APPDATA%\Warzone 2100 Project\` into the address bar of Windows Explorer
will browse to your Warzone 2100 directory.

### Warzone 2100 directory under macOS

The directory `Warzone 2100 <version>` can be found in your home directory at:
`~/Library/Application Support/`

By default, recent version of macOS hide your account's Library folder. To view it in
**Finder**, hold down the **Option (⌥)** key while clicking the **Go** menu, and your Library folder
will appear as a menu choice.

### Configuration file

The configuration file is just called 'config' and contains several configuration
options, some of them can be changed by using command-line options or using
the in-game menus, others can only be changed by editing the file manually.

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

Note: Some options have corresponding entries in the configuration file and will
persist from one start of Warzone 2100 to the next.

Multiplaying via internet
-------------------------

There are two methods to start a multiplayer game via the internet: using the host's
IP or using the lobby server. Make sure you are able to communicate on TCP ports
2100 and 9999. Note that for port forwarding, you only need to configure your
router to forward port 2100.

You can choose whether to connect via Lobby or IP:

* If you choose IP, Warzone 2100 asks you for the IP address of the host and
  will try to connect to that IP.
* If you choose Lobby, Warzone 2100 will connect to the lobby server, as long as
  the lobby-server-address in your config file has not been changed.

You will see a list of games from which you can select.

You can kick unwanted players out of a game before it begins by clicking left on
them while holding the right mouse button.

When you are hosting a game it will automatically be listed on the lobby server.
If you do not want your games to be listed on the lobby-server, you should
change the entry "masterserver_name=lobby.wz2100.net" in your config to some-
thing invalid, for example: "nomasterserverplease".

If you then want to see the games that are listed on the lobby server you may
enter "lobby.wz2100.net" when prompted to enter the host's IP or change the
entry in the config file back. You will have to restart Warzone 2100 in order
for config changes to take effect.

Cheats
------

Like many other games, Warzone 2100 features a certain set of cheats that can be
used to have an advantage in the singleplayer-campaign and skirmish-games, or to
just help mod-makers and map-makers with testing. Cheats do not work in multiplayer,
unless all players agree to it.

Cheats are likely to contain or trigger bugs, so use with care, especially
during campaign.

### Entering cheat mode

To be able cheats while in-game, press shift and backspace simultaneously.
An on-screen message should appear telling you that cheat mode has been enabled.
You can disable it using the same key combination again.

Pressing ctrl+o opens up the debug menu, which is useful for inspecting the
game state, or just messing around.

### Cheat commands

After activating cheat mode, cheats can be entered using the normal chat-
function. Cheats are ordered by their use and where they can be used.

There are many cheat commands. Some examples:

* `biffer baker` - Your units do more damage and are stronger
* `double up` - Your units are twice as strong
* `give all` - Allows you to build and research everything
* `work harder` - All currently active research topics are instantly researched
* `research all` - Everything is researched instantly
* `let me win` - You win the current campaign mission
* `superpower` - Gives you maximum power
* `teach us` - Gives experience to selected units
* `makemehero` - Makes selected units Heroes
* `clone wars` - Clones selected units (x10)
* `clone wars!` - Clones selected units (x40)
* `clone wars!!` - Clones selected units (x135)
* `kill` - Kills selected object
* `john kettley` - Toggles weather conditions
* `get off my land` - Kills all enemy units and structures
* `showfps` - Shows average FPS
* `reload me` - Reloads selected weapon instantly
* `damage me` - Selected object takes 20% damage
* `showunits` - Displays unit count information
* `showorders` - Displays unit order/action state
* `droidinfo` - Shows unit stats

All available commands can be found in this file: https://github.com/Warzone2100/warzone2100/blob/master/src/cheat.cpp

Modding information
-------------------

Warzone 2100 AI, maps and campaign can be scripted using JavaScript.

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
  ```shell
  git clone https://github.com/Warzone2100/warzone2100.git
  cd warzone2100
  git fetch --tags
  git submodule update --init --recursive
  ```
  > Note: Initializing submodules is required.

Do **not** use GitHub's "Download Zip" option, as it **does not contain submodules** or the Git-based autorevision information.

### Linux

* Prerequisites
   * Compiling tools (ex. CMake, GCC/G++/Clang, ninja-build)
   * Archiving tools (ex. zip, p7zip)
   * Various libraries:
      * [SDL](https://www.libsdl.org) ≥ 2.0.5 _(strongly recommended: ≥ 2.0.20)_
      * [PhysicsFS](https://icculus.org/physfs/) ≥ 2.0.3-3 _(strongly recommended: ≥ 3.0.2)_
      * [libpng](https://www.libpng.org/pub/png/libpng.html) ≥ 1.2
      * [libtheora](https://theora.org)
      * [libvorbis](https://xiph.org/vorbis)
      * [libogg](https://xiph.org/ogg/)
      * [opus](https://github.com/xiph/opus)
      * [Freetype](https://www.freetype.org/) _(strongly recommended: ≥ 2.10.4)_
      * [Harfbuzz](https://github.com/harfbuzz/harfbuzz) ≥ 1.0 _(strongly recommended: ≥ 3.3.0)_
      * [fribidi](https://github.com/fribidi/fribidi)
      * [OpenAL-Soft](https://openal-soft.org)
      * [libcurl](https://curl.haxx.se/libcurl/) _(strongly recommended: ≥ 7.58.0)_
      * [libsodium](https://github.com/jedisct1/libsodium) ≥ 1.0.14
      * [SQLite](https://www.sqlite.org/index.html) ≥ 3.14
   * For language support: [Gettext](https://www.gnu.org/software/gettext/)
   * To generate documentation: [Asciidoctor](https://asciidoctor.org) ≥ 1.5.3
   * To build with Vulkan support: the full [Vulkan SDK](https://vulkan.lunarg.com/sdk/home) _(strongly recommended: ≥ 1.2.148.1)_
* **Installing prerequisites:**
   * Using `get-dependencies_linux.sh`:
      1. Specify one of the linux distros supported by the script: (`ubuntu`, `fedora`, `alpine`, `archlinux`, `opensuse-tumbleweed`) _REQUIRED_
      2. Specify a mode: (`build-all` (default), `build-dependencies`) _OPTIONAL_

      Example:
      ```shell
      sudo ./get-dependencies_linux.sh ubuntu build-dependencies
      ```
   * Manually (Ubuntu 18.04)+:
     ```shell
     sudo apt-get -u update
     sudo apt-get -y install git gcc g++ clang cmake libc-dev dpkg-dev ninja-build zip unzip pkg-config gettext asciidoctor
     sudo apt-get -y install libpng-dev libsdl2-dev libopenal-dev libphysfs-dev libvorbis-dev libtheora-dev libxrandr-dev libfribidi-dev libfreetype6-dev libfribidi-dev libharfbuzz-dev libfontconfig1-dev libcurl4-gnutls-dev gnutls-dev libsodium-dev libsqlite3-dev
     ```
   * Manually (Fedora):
     ```shell
     sudo dnf -y update && dnf clean all
     sudo dnf -y install git gcc gcc-c++ cmake ninja-build p7zip gettext rubygem-asciidoctor
     sudo dnf -y install libpng-devel SDL2-devel openal-soft-devel physfs-devel libogg-devel libvorbis-devel libtheora-devel freetype-devel fribidi harfbuzz-devel libcurl-devel openssl-devel libsodium-devel sqlite-devel
     ```
* **Building from the command-line:**
   1. Starting from the _parent_ directory of the warzone2100 source code (which is assumed to be in a folder named `warzone2100`), create a **sibling** build directory:
      ```shell
      mkdir build
      ```
   2. Change directory into the sibling `build` directory:
      ```shell
      cd build
      ```
   3. Run CMake configure to generate the build files:
      ```shell
      cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_INSTALL_PREFIX:PATH=~/wz/install -GNinja ../warzone2100
      ```
      > - [Modify the `CMAKE_INSTALL_PREFIX` parameter value as desired](https://cmake.org/cmake/help/latest/variable/CMAKE_INSTALL_PREFIX.html) to configure the base installation path.
      > - The `../warzone2100` path at the end should point to the warzone2100 source directory.
   4. Run CMake build:
      ```shell
      cmake --build . --target install
      ```

### Windows using MSVC

* Prerequisites
   * **Visual Studio 2019** (Visual Studio 2015-2017 may work, but 2019 is strongly encouraged)
      - If you do not already have Visual Studio installed, you can download the free **Visual Studio Community** from: https://developer.microsoft.com/en-us/windows/downloads
      - IMPORTANT: You need the fully-featured Visual Studio IDE. “Visual Studio Code” does not include the necessary support for building C++ Windows apps.
   * **CMake 3.10+** (https://cmake.org/)
   * **Git** (if not building from a release source archive)
   * **7-Zip** (https://www.7-zip.org)
   * **Vulkan SDK 1.2.148.1+** (https://vulkan.lunarg.com/sdk/home)
      - Required only if you want to build with Vulkan support.
* **Preparing to build:**
   Build dependencies are provided via [vcpkg](https://github.com/Microsoft/vcpkg) from Microsoft.
   * Run the `get-dependencies_win.ps1` script from powershell in order to download and build the dependencies.
* **Building from the command-line:**
   * Change directory to the warzone2100 repo directory
   * Configure
      * Visual Studio 2019: `cmake -H. -DCMAKE_TOOLCHAIN_FILE=vcpkg\scripts\buildsystems\vcpkg.cmake -Bbuild -G "Visual Studio 16 2019"`
      * Visual Studio 2017: `cmake -H. -DCMAKE_TOOLCHAIN_FILE=vcpkg\scripts\buildsystems\vcpkg.cmake -Bbuild -G "Visual Studio 15 2017"`
      * Visual Studio 2015: `cmake -H. -DCMAKE_TOOLCHAIN_FILE=vcpkg\scripts\buildsystems\vcpkg.cmake -Bbuild -G "Visual Studio 14 2015"`
   * Build
      * Release: `cmake --build build --config Release`
      * Debug: `cmake --build build --config Debug`
* **Building using Visual Studio 2019:**
   1. Open Visual Studio 2019
   2. Open the warzone2100 folder using **File** > **Open** > **Folder...**
      - Allow Visual Studio some time to load the project and retrieve information from CMake.
   3. Create a VS CMake settings JSON file using **CMake** > **Change CMake settings**. You can also reach this dialog by clicking "Manage Configurations" in the configuration dropdown in the toolbar. Make sure the CMake components in Visual Studio are installed (by running the Visual Studio Installer).
      - This creates `CMakeSettings.json`
   4. Add the following variables to `CMakeSettings.json`:
      - To `cmakeCommandArgs`, add: `-DCMAKE_TOOLCHAIN_FILE=vcpkg\scripts\buildsystems\vcpkg.cmake`
      - Note: Visual Studio automatically escapes and turns each `\` into `\\`
   5. After letting Visual Studio re-run CMake configure with the new settings, you can build using the **CMake** menu.

### macOS
See [macosx/README.md](macosx/README.md)

Licensing
-------------------

Warzone 2100 is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2 of the License, or (at your option) any later version.

[![SPDX-License-Identifier: GPL-2.0-or-later](https://img.shields.io/static/v1?label=SPDX-License-Identifier&message=GPL-2.0-or-later&color=blue&logo=open-source-initiative&logoColor=white&logoWidth=10&style=flat-square)](COPYING)

More information: [COPYING.README](COPYING.README), [COPYING.NONGPL](COPYING.NONGPL)
