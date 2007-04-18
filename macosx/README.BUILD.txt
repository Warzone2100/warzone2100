This directory contains support files for the MacOS X port of Warzone 2100.
Since April, 2007, The MacOS X port has been built using an Xcode project
that automatically downloads and builds all necessary external libraries.
Also, since that time, the MacOS X port has produced a universal binary
and has required MacOS X 10.4 "Tiger" to run.

To build the game, just run the following command:
  xcodebuild -project Warzone -target Warzone -configuration Release

