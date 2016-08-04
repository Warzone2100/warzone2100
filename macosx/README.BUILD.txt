The minimum requirements to build Warzone are System 10.6 and Xcode 3.2.6.


If you do not have Xcode 3.2.x you can get it for free at the Mac App Store or Apple's website.
https://itunes.apple.com/us/app/xcode/id497799835?mt=12

http://developer.apple.com/technology/xcode.html
You will need a free ADC Membership to download Xcode.


This directory contains support files for the Mac OS X port of Warzone 2100.
Since April, 2007, The Mac OS X port has been built using an Xcode project that automatically downloads and builds all necessary external libraries.
The Mac OS X port produces a 64 bit binary and requires Mac OS X 10.6 to run.

To build the game, just run the following command:
	xcodebuild -project Warzone.xcodeproj -target Warzone -configuration Release

There are two build configurations available.  `Release` is compiled normally, while `Debug` sets the `DEBUG` preprocessor flag and automatically sets the game's debugging options to the equivalent of `--debug all` when launched, unless overwritten with a `--debug` command-line option.  The debugging output can be viewed with Console.app.
