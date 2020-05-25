Release Checklist
=================

Preliminary Testing
-------------------
While the CI builds on GitHub (via `GitHub Actions`, `Azure Pipelines`, etc) can be used to check that it compiles fine on Linux, macOS & Windows, it can't actually tell you if it works.
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

```c
static int NETCODE_VERSION_MAJOR = 6;                // major netcode version, used for compatibility check
static int NETCODE_VERSION_MINOR = 0x22;             // minor netcode version, used for compatibility check
```

Commit the (above) changes to master

```
git commit -p
```

This will trigger CI builds for all OSes.


Verify the builds all complete
------------------------------

Wait for all CI builds to complete, and verify they have completed successfully.

> The following images may not update immediately - even if refreshing the page.
> To find the latest status, click on each image, or **view the Checks / Status for the commit on GitHub**.

[![Ubuntu](https://github.com/Warzone2100/warzone2100/workflows/Ubuntu/badge.svg?branch=master&event=push)](https://github.com/Warzone2100/warzone2100/actions?query=workflow%3AUbuntu+branch%3Amaster+event%3Apush)
[![Fedora](https://github.com/Warzone2100/warzone2100/workflows/Fedora/badge.svg?branch=master&event=push)](https://github.com/Warzone2100/warzone2100/actions?query=workflow%3AFedora+branch%3Amaster+event%3Apush)
[![Windows](https://github.com/Warzone2100/warzone2100/workflows/Windows/badge.svg?branch=master&event=push)](https://github.com/Warzone2100/warzone2100/actions?query=workflow%3AWindows+branch%3Amaster+event%3Apush)
[![Azure Build Status](https://dev.azure.com/wz2100/warzone2100/_apis/build/status/Build?branchName=master)](https://dev.azure.com/wz2100/warzone2100/_build/latest?definitionId=1&branchName=master)
[![Travis CI Build Status](https://travis-ci.org/Warzone2100/warzone2100.svg?branch=master)](https://travis-ci.org/Warzone2100/warzone2100)

Can be flakey:
- FreeBSD builds: [![FreeBSD Build Status](https://api.cirrus-ci.com/github/Warzone2100/warzone2100.svg?branch=master)](https://cirrus-ci.com/github/Warzone2100/warzone2100)


Testing
-------
You should locally test whether the master branch of the game actually runs, in both single & skirmish and multiplayer.
The latest CI (master branch) builds may be downloaded from:
* [Windows Builds](https://github.com/Warzone2100/warzone2100/actions?query=workflow%3AWindows+branch%3Amaster+event%3Apush)
   * Select the latest "Windows" workflow run, download the `warzone2100_win_*_portable` and `warzone2100_win_*_installer` artifacts.
   * (Note: These will be a .zip _of_ the .exe. Only the .exe gets automatically uploaded on release.)
   * Test both the portable and regular installers!
* [MacOS Build](https://dev.azure.com/wz2100/warzone2100/_build/latest?definitionId=1&branchName=master)
   * Under "Artifacts:", click the "\<N\> published" link, then select the `warzone2100_macOS` artifact.
   * (Note: This may be a .zip _of_ a .zip. Only the inner .zip gets automatically uploaded on release.)
* [Source Tarball](https://github.com/Warzone2100/warzone2100/actions?query=workflow%3AUbuntu+branch%3Amaster+event%3Apush)
   * Select the latest "Ubuntu" workflow run, download the `warzone2100_src` artifact.
   * (Note: This will be a .zip _of_ the .tar.xz. Only the .tar.xz gets automatically uploaded on release.)

### Test building with the source tarball on Linux

Use the Linux build instructions to build via the source tarball you downloaded above.
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


Create the tag for the new release
----------------------------------

Since everything works (since you tested it), it is time to make the **annotated** tag. Verify you are on the appropriate branch + commit, then tag it:

    git tag -a 3.3.0
    git push
    git push origin 3.3.0

Where `3.3.0` is the name of the tag.

> ### Always forwards
> _Do **NOT** re-use an existing tag / tag that was previously used!_
> If you make a mistake, or a bug is found, (etc), increment the version _again_.

> ### Annotated tags ONLY
> You must use `git tag -a <version>` to create an annotated tag. Lightweight tags will not work correctly.

GitHub will then **automatically**:
- create a Draft release in **[GitHub Releases](https://github.com/Warzone2100/warzone2100/releases)** for the new tag
   - (may take a minute or so to complete)
- trigger the CI builds


Download & verify the tag / release builds
------------------------------------------

- [x] **Wait for the CI builds to complete for the new tag.**
   - Once the CI builds are complete, they will be automatically uploaded to the draft GitHub Release.

<blockquote>
<sub>
- If something goes wrong with the automatic artifact uploading, it should be possible to manually
download the appropriate artifact(s) following the same instructions you used to download the master
branch artifacts above.
<br />
- Just make sure you download artifacts from the CI runs for the <b>tag</b> you created.
<br />
<b>You should <i>NOT</i> have to manually upload artifacts to the GitHub Release</b> (unless
the CI -> GitHub integration breaks).
</sub>
</blockquote>
<br />

- [x] Verify all release assets have been uploaded by CI to the draft GitHub Release.
- [x] Download each release asset and verify:
   - [x] Download the Windows builds from the draft release:
      - [x] `warzone2100_win_x86.DEBUGSYMBOLS.7z`
      - [x] `warzone2100_win_x86_installer.exe`
      - [x] `warzone2100_win_x86_portable.exe`
   - [x] Verify the SHA512 hashes towards the end of the [Windows workflow's](https://github.com/Warzone2100/warzone2100/actions?query=workflow%3AWindows+event%3Apush) log match the files you downloaded.
      - [x] **Make sure you are viewing the build for the _tag_ you just made**
      - [x] Select the appropriate job (x86, etc).
      - [x] Expand the "Compare Build Outputs" step, which outputs the SHA512 hashes at the bottom of its output.
   - [x] Download the macOS build from the draft release:
      - [x] `warzone2100_macOS.zip`
   - [x] Verify the SHA512 hash in the [Azure Pipelines](https://dev.azure.com/wz2100/warzone2100/_build?definitionId=1&_a=summary) build log matches the file you downloaded.
      - [x] **Make sure you are viewing the build for the _tag_ you just made**
      - [x] Select the `macOS [Xcode 10.3, macOS 10.14 SDK]` job
      - [x] Scroll down to the `Output build info` step, which outputs the SHA512 at the bottom of its output.
   - [x] Download the tag's source tarball from the draft release:
      - [x] `warzone2100_src.tar.xz`
   - [x] Verify the SHA512 hash towards the end of the [Ubuntu workflow's](https://github.com/Warzone2100/warzone2100/actions?query=workflow%3AUbuntu+event%3Apush+) ("Package Source" job) log matches the file you downloaded.
      - [x] **Make sure you are viewing the build for the _tag_ you just made**
      - [x] Select the "Package Source" job
      - [x] Expand the "Rename Tarball & Output Info" step, which outputs the SHA512 at the bottom of its output.
- [x] Test each release asset to make sure they work, and show the proper version # (which should match the tag).


Edit & Publish the Release
--------------------------

Once you have verified all the release assets:
- [x] Click the "Edit" button next to the draft GitHub Release.
- [x] Remove the top text block from the release notes / description (as it instructs).
- [x] Click the green "Publish Release" button.

This will automatically:
* Make the new release available on GitHub
* Trigger automatic mirroring of the new release to SourceForge

### Revert the changes to configure.ac

Then you should revert the changes you made to `configure.ac` and `lib/netplay/netplay.cpp`, so that git master again becomes git master.


Post-Release checklist
----------------------
 * Add a new milestone to https://github.com/Warzone2100/warzone2100/milestones
 * (See below for details) Update the lobby server for the new release. See ~lobby/wzlobbyserver-ng/wzlobby/protocol/protocol3.py and ~lobby/wzlobbyserver-ng/wzlobby/settings.py and ~lobby/wzlobbyserver-ng/wzlobby/gamedb.py and then restart the lobby via Monit.
 * (See below for details) Update /home/lobby/gamecheck.wz2100.net/gamechecker/bottle/app.py -- the game version checker -- to include the version you have just created. Restart it with «uwsgi --reload /var/run/gamecheck.wz2100.net-master.pid».
 * Tell everyone about it in the forums. You can use the build_tools/changelog2bbcode.sh script to massage the changelog into BBCode.
 * Send mail about it on the developer mailing list.
 * Change the title on our IRC channels about the new release.
 * Ask for a raise for doing all this work that nobody else wanted to do, and you got suckered into doing it.

And, I am sure that people will spread the word about this new release at the following sites & others.
 * [ModDb](http://www.moddb.com/games/275/warzone-2100), [Softonic](http://warzone-2100.en.softonic.com/), [Gamershell](http://www.gamershell.com/news), [Gamedev](http://www.gamedev.net/community/forums/forum.asp?forum_id=6), [Reddit](www.reddit.com/r/warzone2100)

### Updating the version numbers on the server

    sudo su - lobby
    workon wzlobby
    cd ~
    nano -w wzlobbyserver-ng/wzlobby/protocol/protocol3.py
    nano -w wzlobbyserver-ng/wzlobby/settings.py
    nano -w wzlobbyserver-ng/wzlobby/gamedb.py
    nano -w gamecheck.wz2100.net/gamechecker/bottle/app.py

Then use monit to restart the lobby server.
