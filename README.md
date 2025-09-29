<h1 align="center">
  <img src="icons/warzone2100.large.png" alt="Warzone 2100">
  <br />
  Warzone 2100
</h1>

<p align="center">Warzone 2100 is a <b>free, open source, 3D real-time strategy game</b> with a story-driven single-player campaign, online multi-player, offline skirmish, and more.</p>

# Highlights

- Remastered single-player campaign with expanding persistent bases, away missions, and more
- Online multiplayer with up to 10 players, free-for-all or team-play, for massive battles
- Local skirmish with AI bots, for endless replayability
- Extensive tech tree with over 400 different technologies
- Customizable units with a flexible design system that enables a wide variety of possible tactics
- Cross-platform multiplayer
- Support for multiple graphics backends (OpenGL, OpenGL ES, Vulkan)
- 100% free and open source

For more info, see: [Homepage of the Warzone 2100 Project](https://wz2100.net/)

# Installation

**Visit https://wz2100.net for the latest stable release for Windows, macOS and Linux.**

> Note for videos in Ubuntu: For important information during the game, download the videos manually. Assuming the game is installed in the standard `~/.local/share/` folder, use these commands:
> ```shell
> mkdir ~/.local/share/warzone2100
> wget https://github.com/Warzone2100/wz-sequences/releases/download/v3/standard-quality-en-sequences.wz -O ~/.local/share/warzone2100/sequences.wz
> ```

# Origins

Warzone 2100, was originally developed by Pumpkin Studios, and released in 1999 as a
ground-breaking and innovative 3D real-time strategy game.

In 2004, Eidos (in collaboration with Pumpkin Studios) decided to release the source for the game
under the terms of the GNU GPL - followed later by the remaining music & video files.

It has been developed, maintained, and improved by the community ever since, under the banner of
the “Warzone 2100 Project”.

# State of the game

After the liberation of the Warzone 2100 source-code on December 6th, 2004, all
proprietary technologies have been replaced with open-source counterparts, and
extensive improvements and additions have been made throughout while preserving
what made the original release so great.

Development continues on GitHub, and bug reports & contributions are welcome!

# Latest development builds

[![Windows Build Status](https://img.shields.io/github/actions/workflow/status/Warzone2100/warzone2100/CI_windows.yml?branch=master&label=Windows&logo=windows)](https://github.com/Warzone2100/warzone2100/actions?query=workflow%3AWindows+branch%3Amaster+event%3Apush)
 [![macOS Build Status](https://img.shields.io/github/actions/workflow/status/Warzone2100/warzone2100/CI_macos.yml?branch=master&label=macOS&logo=apple)](https://github.com/Warzone2100/warzone2100/actions?query=workflow%3AmacOS+branch%3Amaster+event%3Apush)
 [![Ubuntu Build Status](https://img.shields.io/github/actions/workflow/status/Warzone2100/warzone2100/CI_ubuntu.yml?branch=master&label=Ubuntu&logo=ubuntu&logoColor=FFFFFF)](https://github.com/Warzone2100/warzone2100/actions?query=workflow%3AUbuntu+branch%3Amaster+event%3Apush)
 [![Fedora Build Status](https://img.shields.io/github/actions/workflow/status/Warzone2100/warzone2100/CI_fedora.yml?branch=master&label=Fedora&logo=fedora&logoColor=FFFFFF)](https://github.com/Warzone2100/warzone2100/actions?query=workflow%3AFedora+branch%3Amaster+event%3Apush)
 [![WebAssembly Build Status](https://img.shields.io/github/actions/workflow/status/Warzone2100/warzone2100/CI_emscripten.yml?branch=master&label=WebASM&logo=webassembly&logoColor=FFFFFF)](https://github.com/Warzone2100/warzone2100/actions?query=workflow%3AEmscripten+branch%3Amaster+event%3Apush)
 [![FreeBSD Build Status](https://img.shields.io/cirrus/github/Warzone2100/warzone2100/master?label=FreeBSD&logo=FreeBSD)](https://cirrus-ci.com/github/Warzone2100/warzone2100/master)
 [![Packaging status](https://repology.org/badge/tiny-repos/warzone2100.svg)](https://repology.org/project/warzone2100/versions)

### Windows

How to get the latest Windows development builds:
1. View the **[latest successful Windows builds](https://github.com/Warzone2100/warzone2100/actions?query=workflow%3AWindows+branch%3Amaster+event%3Apush+is%3Asuccess)**.
2. Select the latest workflow run in the table / list.
   This should display a list of **Artifacts** from the run.
3. Download the `warzone2100_win_installer` artifact.
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
   - If you are running Ubuntu 22.04: `warzone2100_ubuntu22.04_amd64_deb`
   - If you are running Ubuntu 24.04: `warzone2100_ubuntu24.04_amd64_deb`
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

# Reporting bugs

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
   4. You may even upload save files. These consist of one file and
      one folder. Both are named after your savegame (e.g. MySaveGame.gam and
      the folder MySaveGame).
   5. Bug reports are not submit-and-forget. It may be that you forgot some
      information or forgot to upload a file. So it is also in your interest to
      watch the bug-report after it has been submitted. Additionally, you can enable
      e-mails of comments to your bug report.

# Configuration

Warzone 2100 uses its own subdirectory in a user's home directory to save
configuration data, save files and certain other things. Additionally you can
use this directory to place custom maps and mods so the game can find them. The
location of this directory depends on the operating system.

> [!TIP]
> The easy way to find the configuration directory is to:
> 1. Launch Warzone 2100
> 2. Click "Options"
> 3. Click the small "Open Configuration Directory" link in the bottom-left

### Configuration file

The configuration file is just called 'config' and contains several configuration
options, some of them can be changed by using command-line options or using
the in-game menus, others can only be changed by editing the file manually.

If at any point you did something wrong, you can delete the old configuration
file and just restart Warzone 2100. Then the game will regenerate a new
configuration file with default values.

# Command-line options

Warzone 2100 can be started with different options and arguments. For a list
of these options, run the game with the --help option.

Notes: These options all have two dashes (--), not one dash only (-). Also,
if the option has an argument, you need to separate the option and its argument
with a '=' sign - spaces do not work.

Note: Some options have corresponding entries in the configuration file and will
persist from one start of Warzone 2100 to the next.

# Multiplaying via internet

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

# Cheats

Warzone 2100 has many built-in cheat codes:
* [Warzone 2100 Cheats](doc/Cheats.md)

# Modding information

Warzone 2100 AI, maps and campaign can be scripted using JavaScript.

Links to further information
* [Scripting](doc/Scripting.md)
* [Model format](doc/PIE.md)
* [Animation](doc/Animation.md)

# How to build

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
      * [PhysicsFS](https://icculus.org/physfs/) ≥ 2.1 _(strongly recommended: ≥ 3.0.2)_
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
      * [libzip](https://github.com/nih-at/libzip) _(strongly recommended: ≥ 1.10.1)_
      * [libprotobuf](https://github.com/protocolbuffers/protobuf) (if compiling with GNS support)
   * For language support: [Gettext](https://www.gnu.org/software/gettext/)
   * To generate documentation: [Asciidoctor](https://asciidoctor.org) ≥ 1.5.3
   * To build with Vulkan support: the full [Vulkan SDK](https://vulkan.lunarg.com/sdk/home) _(strongly recommended: ≥ 1.2.148.1)_
* **Installing prerequisites:**
   * Using `get-dependencies_linux.sh`:
      1. Specify one of the linux distros supported by the script: (`ubuntu`, `fedora`, `alpine`, `archlinux`, `opensuse-tumbleweed`, `gentoo`, `debian`, `raspberrypios`) _REQUIRED_
      2. Specify a mode: (`build-all` (default), `build-dependencies`) _OPTIONAL_

      Example:
      ```shell
      sudo ./get-dependencies_linux.sh ubuntu build-dependencies
      ```
* **Building from the command-line:**
   1. Starting from the _parent_ directory of the warzone2100 repository, create an [out-of-source](https://cmake.org/cmake/help/book/mastering-cmake/chapter/Getting%20Started.html#directory-structure) build directory:
      ```shell
      mkdir build
      ```
   2. Change directory into the `build` directory:
      ```shell
      cd build
      ```
   3. Run CMake configure to generate the build files:
      ```shell
      cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_INSTALL_PREFIX:PATH=~/wz/install -GNinja ../warzone2100
      ```
      > - [Modify the `CMAKE_INSTALL_PREFIX` parameter value as desired](https://cmake.org/cmake/help/latest/variable/CMAKE_INSTALL_PREFIX.html) to configure the base installation path.
      > - The `../warzone2100` path at the end should point to the warzone2100 repo directory. This example assumes that the repo directory and the build directory are siblings, and that the repo was cloned into a directory named `warzone2100`.
   4. Run CMake build:
      ```shell
      cmake --build . --target install
      ```

### Windows using MSVC

See [platforms/windows/README-build.md](platforms/windows/README-build.md)

### macOS
See [platforms/macos/README.md](platforms/macos/README.md)

# Licensing

Warzone 2100 is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2 of the License, or (at your option) any later version.

[![SPDX-License-Identifier: GPL-2.0-or-later](https://img.shields.io/static/v1?label=SPDX-License-Identifier&message=GPL-2.0-or-later&color=blue&logo=open-source-initiative&logoColor=white&logoWidth=10&style=flat-square)](COPYING)

More information: [COPYING.README](COPYING.README), [COPYING.NONGPL](COPYING.NONGPL)

# Special thanks

Free code signing for Windows provided by [SignPath.io](https://signpath.io), certificate by [SignPath Foundation](https://signpath.org).
