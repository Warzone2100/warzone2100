# Warzone 2100 Configuration

Warzone 2100 uses its own subdirectory in a user's home directory to save
configuration data, save files and certain other things. Additionally you can
use this directory to place custom maps and mods so the game can find them. The
location of this directory depends on the operating system.

> [!TIP]
> The easy way to find the configuration directory is to:
> 1. Launch Warzone 2100
> 2. Click "Options"
> 3. Click the small "Open Configuration Directory" link in the bottom-left

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

Typical `%APPDATA%` path:
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

## Configuration file

The configuration file is just called 'config' and contains several configuration
options, some of them can be changed by using command-line options or using
the in-game menus, others can only be changed by editing the file manually.

If at any point you did something wrong, you can delete the old configuration
file and just restart Warzone 2100. Then the game will regenerate a new
configuration file with default values.
