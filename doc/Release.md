Release Checklist
=================

Preliminary Testing
-------------------
While the CI builds on GitHub (via `Travis-CI` and `AppVeyor`) can be used to check that it compiles fine on Linux, macOS & Windows, it can't actually tell you if it works.
You should locally test whether the latest commit's build actually runs, in both single & skirmish and multiplayer!
Of course, it is always nice to gather up enough volunteers to test before you actually make releases.

Starting off
------------
We currently do releases off git master.

    git pull origin master

Bump version numbers
--------------------
The following files _need editing_ to have the _correct version_.

### ChangeLog

Edit the `ChangeLog` to be in sync with the latest changes. Also make sure to put the correct date and version there (the date that you make the release of course).

### configure.ac

Make sure to put the correct version number in the second parameter of AC_INIT.
The AC_INIT directive should be somewhere at the top of the file.
In this case, we use `3.3.0` for this release. It can be `4.3_beta 1` or whatever.

    AC_INIT([Warzone 2100],[3.3.0],[http://wz2100.net/],[warzone2100])

Make it default to _release_ builds.

    # Add later for stricter checking: -Wextra -Wmissing-declarations -Wstrict-prototypes
    AC_ARG_ENABLE([debug],
    AS_HELP_STRING([--enable-debug[=yes/relaxed/profile/debugprofile/no]],[Compile debug version [[yes]]]),
    [ enable_debug=${enableval} ], [ enable_debug=yes ])

change it to:

    [ enable_debug=${enableval} ], [ enable_debug=no ])
    ...
    AC_MSG_CHECKING([whether to compile in debug mode])
    AC_MSG_RESULT([${enable_debug}])

### lib/netplay/netplay.cpp

Every release should increment at least `NETCODE_VERSION_MINOR` by 1, to prevent any issues with data or code changes.
It is very important that this number is in sequential order from the last release, as the lobby server needs sane data to identify versions.

`NETCODE_VERSION_MAJOR` is used the following way.
 * master will have this set to `0x1000`
 * bugfixes will have this set to `0xB00`
 * 3.1 will have this set to `0x2000`
 * releases based off branch master will be `0x10A0`
 * releases based off branch bugfixes will be `0xBA0`
 * releases based off branch 3.1 will be `0x20A0`
 * new branches will be `+0x1000` higher than the original value.

    static int NETCODE_VERSION_MAJOR = 6;                // major netcode version, used for compatibility check
    static int NETCODE_VERSION_MINOR = 0x22;             // minor netcode version, used for compatibility check

Commit the (above) changes to master

    git commit -p

This will trigger CI builds for all OSes.

Verify the builds all complete
------------------------------

Wait for the Travis-CI and AppVeyor CI builds to complete, and verify they have completed successfully.

Testing
-------
You should locally test whether the game actually runs, in both single & skirmish and multiplayer.
The CI builds may be downloaded from:
* [AppVeyor](https://ci.appveyor.com/project/Warzone2100/warzone2100/branch/master)
* [MacOS Build](http://buildbot.wz2100.net/files/CI/master/)

### Test building with the source tarball on Linux

Use the Linux build instructions to build via the source tarball you downloaded from the buildbot.
Verify that the build succeeds (and that no files are missing from the tarball).
Then proceed with normal game testing.

### Test the Windows portable & regular installers

Verify that each installer works, and the game runs. (Start by testing the portable installer.)

### Test the crash handler

On both Linux & Windows, you can also test the crash handler now.
Note, it is better to test the portable version on windows, since then you will always be starting with a virgin config directory.
 * Test the crash handler via the --crash command line option, to make sure it produces a good dump file!
 * You will see a assert() in debug builds, and we do NOT want to release debug builds.
 * If you are testing the crash handler in debug builds, use --noassert to skip it.
 * '''The Crash dumps it produces should be sane.'''

### Test the game

TBD

### Done with the builds!

Since everything works (since you tested it), it is time to make the tag. Verify you are on the appropriate branch + commit, then tag it:

    git tag -a 3.3.0
    git push
    git push origin --tags

Where `3.3.0` is the name of the tag.

GitHub will then automatically trigger the CI builds via Travis-CI and AppVeyor. Wait for the CI builds to complete for the new tag.

Download, verify, & upload the builds
-------------------------------------

* Download the Windows build results from [AppVeyor](https://ci.appveyor.com/project/Warzone2100/warzone2100/) (see the build artifacts). (Note: Make sure you are viewing the build for the tag you just made.)
   * `warzone2100-<TAG>_x86.DEBUGSYMBOLS.7z`
   * `warzone2100-<TAG>_x86_installer.zip`
   * `warzone2100-<TAG>_x86_portable.zip`
* Extract the installer `.exe`s from the `.zip` files you downloaded.
* Verify the SHA512 hashes in the AppVeyor Console (build log) match those of the installer `.exe`s you extracted.
* Upload the installer `.exe`s (plus the `*.DEBUGSYMBOLS.7z`) to [SourceForge](https://sourceforge.net/projects/warzone2100/files/releases/)
* Download the macOS build from [buildbot](http://buildbot.wz2100.net/files/CI/<TAG>/macOS/)
   * `warzone2100-<TAG>_macOS.zip`
* Verify the SHA512 hash towards the end of the Travis-CI build's ("macOS" job) log matches the file you downloaded.
* Upload the macOS `.zip` to [sourceforge](https://sourceforge.net/projects/warzone2100/files/releases/)
* Download the source tarball from [buildbot](http://buildbot.wz2100.net/files/CI/<TAG>/src/)
   * `warzone2100-<TAG>_src.tar.xz`
* Verify the SHA512 hash towards the end of the Travis-CI build's ("Package Source" job) log matches the file you downloaded.
* Upload the source tarball to [sourceforge](https://sourceforge.net/projects/warzone2100/files/releases/)

Additionally:
* Go to [GitHub releases](https://github.com/Warzone2100/warzone2100/releases)
* Click the "Draft a new release" button
* Select the '''existing''' tag (the tag that you created above)
* Enter the version # as the release title.
* Upload the same files that you uploaded to SourceForge.

### Revert the changes to configure.ac

Then you should revert the changes you made to `configure.ac`, so that git master again becomes git master.

Go to [trac](http://developer.wz2100.net/admin/ticket/versions) to add the new version to the bug tracker.

Time to publish the release!
----------------------------
 * Add new version to the Bug Tracker, just in case people find bugs and want to report them.
 * Upload the tarball and both windows builds to our host.
 * (See below for details) Update the lobby server for the new release. See ~lobby/wzlobbyserver-ng/wzlobby/protocol/protocol3.py and ~lobby/wzlobbyserver-ng/wzlobby/settings.py and ~lobby/wzlobbyserver-ng/wzlobby/gamedb.py and then restart the lobby via  Monit.
 * (See below for details) Update /home/lobby/gamecheck.wz2100.net/gamechecker/bottle/app.py -- the game version checker -- to include the version you have just created. Restart it with «uwsgi --reload /var/run/gamecheck.wz2100.net-master.pid».
 * Tell everyone about it in the forums. You can use the build_tools/changelog2bbcode.sh script to massage the changelog into BBCode.
 * Send mail about it on the developer mailing list.
 * Change the title on our IRC channels about the new release.
 * Ask for a raise for doing all this work that nobody else wanted to do, and you got suckered into doing it.

And, I am sure that people will spread the word about this new release at the following sites & others.
 * [ModDb](http://www.moddb.com/games/275/warzone-2100), [Softonic](http://warzone-2100.en.softonic.com/), [Gamershell](http://www.gamershell.com/news), [Gamedev](http://www.gamedev.net/community/forums/forum.asp?forum_id=6), [Reddit](www.reddit.com/r/warzone2100)

### Update version numbers in Trac

Add the new release, mark previous release as "(unsupported)" and update the new release as latest version in resolution dropdown:

    http://developer.wz2100.net/admin/ticket/versions
    http://developer.wz2100.net/admin/ticket/resolution

### Updating the version numbers on the server

    sudo su - lobby
    workon wzlobby
    cd ~
    nano -w wzlobbyserver-ng/wzlobby/protocol/protocol3.py
    nano -w wzlobbyserver-ng/wzlobby/settings.py
    nano -w wzlobbyserver-ng/wzlobby/gamedb.py
    nano -w gamecheck.wz2100.net/gamechecker/bottle/app.py

Then use monit to restart the lobby server.
