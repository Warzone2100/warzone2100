# Building Warzone for macOS

## Prerequisites:

> For convenience, you will probably want either [Homebrew](https://brew.sh) or [Macports](https://www.macports.org/install.php) installed for setting up certain prerequisites. If you don't have either yet, **Homebrew** is recommended.

1. **macOS 10.12+**
    - While building may work on prior versions of macOS, it is only tested on macOS 10.12+.

2. **Xcode 8 / 9**
    - If you do not have Xcode 8.3+ you can get it for free at the [Mac App Store](https://itunes.apple.com/us/app/xcode/id497799835?mt=12) or [Apple's website](http://developer.apple.com/technology/xcode.html).

3. **Gettext** (required to compile the translations and language files)
    - If you have [Homebrew](https://brew.sh) installed, you can use the following command in Terminal:
        ```shell
        brew install gettext
        ```
        > The build scripts work perfectly with the default (keg-only) Homebrew install of gettext.
    - If you have [Macports](https://www.macports.org/install.php) installed, you can use the following command in Terminal:
        ```shell
        sudo port install gettext
        ```

4. **AsciiDoc** & **LaTeX** (required to build the documentation / help files)
    - AsciiDoc
        - If you have [Homebrew](https://brew.sh) installed, you can use the following command in Terminal:
            ```shell
            brew install asciidoc
            ```
        - If you have [Macports](https://www.macports.org/install.php) installed, you can use the following command in Terminal:
            ```shell
            sudo port install asciidoc
            ```
    - LaTeX
        - We recommend installing `Basic TeX` directly from: https://tug.org/mactex/morepackages.html
        - If you so choose, you can install the full `MacTeX` package instead of `Basic TeX`, but the full package is not required for the WZ macOS build.

## Building:

The macOS port is built using an Xcode project that automatically downloads and builds all necessary external libraries:
`$(SOURCE_DIR)/macosx/Warzone.xcodeproj`

**To build the game from the command-line:**

1. `cd` into the `macosx` folder

2. Run the following command:
    ```shell
    xcodebuild -project Warzone.xcodeproj -target Warzone -configuration Release
    ```

You can also simply open the project in Xcode, and build the `Warzone` scheme. (You may want to tweak it to build in a Release configuration - the default for that scheme in "Run" mode is Debug, for easier development.)

**Build configurations:**

There are two build configurations available.  `Release` is compiled normally (with optimizations), while `Debug` sets the `DEBUG` preprocessor flag and automatically sets the game's debugging options to the equivalent of `--debug all` when launched, unless overwritten with a `--debug` command-line option.  The debugging output can be viewed with Console.app.

## Deployment:

The macOS port produces a 64-bit [self-contained application bundle](https://developer.apple.com/library/content/documentation/CoreFoundation/Conceptual/CFBundles/BundleTypes/BundleTypes.html#//apple_ref/doc/uid/10000123i-CH101-SW13) that requires **macOS 10.9+** to run.

## Additional Information:

The macOS port supports [sandboxing](https://developer.apple.com/library/content/documentation/Security/Conceptual/AppSandboxDesignGuide/AboutAppSandbox/AboutAppSandbox.html), but this requires [code-signing](https://developer.apple.com/library/content/documentation/Security/Conceptual/CodeSigningGuide/Introduction/Introduction.html) to be configured in the Xcode project.
(By default, code-signing is disabled in the Xcode project.)
