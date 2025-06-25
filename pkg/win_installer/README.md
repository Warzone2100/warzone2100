# WZ2100 Windows Installer

The new combined universal Windows installer for Warzone 2100:
- Supports all architectures (x64, arm64, and x86)
- Offers "all users" (admin privs required) and "current-user-only" installs
- Offers Advanced Options and much more installation customization
- Supports (optionally) portable installs
- Supports easier future updates

# Technical Details

## Build Types

Release builds, branch builds, and other builds (PRs, etc) are all treated as different types of builds.

Different types of builds can coexist side-by-side by default (i.e. when using the Standard Install mode).

When trying to install multiple versions of the _**same**_ type of build:
- Standard Install Mode: Will install to a single location, with a single uninstall entry, and running new installers will update that install.
  - This is useful in the default case (you have an install that's the "latest" release, or latest master branch, etc). Downloading and installing new builds will update it (if you install those as a Standard Install).
- Side-by-Side Install Mode: Will install that version to a unique location, with its own uninstall entry, but will share the settings / saves with other Standard & Side-by-Side installs of its type.
  - This is useful if you want to install specific (usually, older) versions of WZ alongside the "Standard" install (which is presumably the latest).

> [!TIP]
> If you just want to have the latest release build and the latest "master" branch development build installed, you don't need to change anything.
>
> A "release" install and "master branch" install will always coexist side-by-side because they are different types of builds.

## Command-Line Options

Beyond the [standard Inno Setup command-line options](https://jrsoftware.org/ishelp/topic_setupcmdline.htm), the Warzone 2100 installer supports several custom options:

### Specifying the Install Mode:

The default install mode is Standard (which is recommended in most cases), but you can also specify one of:

- `/sidebyside`
  - Enables side-by-side install mode (allowing multiple versions of the same build type to be easily installed)
- `/portable`
  - Enables portable install mode

### Specifying the Target Architecture:

The default behavior is to install the architecture that matches the system. (For example, on x64 Windows it would install x64 binaries.)

If for some reason you want to change this (usually only for extraction or development testing purposes), you can use:

- `/targetarch=arch`
  - Explicitly specifies an architecture to install
  - Where `arch` is one of: `x64`, `arm64`, `x86`

## Building the Installer

Requirements:
- [Inno Setup](https://jrsoftware.org/isinfo.php) 6.4.3+
- Properly-staged multi-arch Warzone 2100 build output (see the install script comments for more details)
