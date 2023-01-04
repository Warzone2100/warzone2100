Release Checklist
=================

Preliminary Testing
-------------------
While the CI builds on GitHub (via `GitHub Actions`, etc) can be used to check that it compiles fine on Linux, macOS & Windows, it can't actually tell you if it works.
You should locally test whether the latest commit's build actually runs, in both single & skirmish and multiplayer!
Of course, it is always nice to gather up enough volunteers to test before you actually make releases.


Starting off
------------
We do releases off of the git `master` branch.

```shell
git pull origin master
```


Update the changelog
--------------------
The following files should be edited prior to release:

### ChangeLog

Edit the `ChangeLog` to be in sync with the latest changes. Also make sure to put the correct date and version there (the date that you make the release of course).


Commit the (above) changes to master

```shell
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
[![macOS](https://github.com/Warzone2100/warzone2100/workflows/macOS/badge.svg?branch=master&event=push)](https://github.com/Warzone2100/warzone2100/actions?query=workflow%3AmacOS+branch%3Amaster+event%3Apush)
[![Drone Cloud CI ARM64 Build Status](https://img.shields.io/drone/build/Warzone2100/warzone2100/master?label=ARM64%20Linux)](https://cloud.drone.io/Warzone2100/warzone2100)

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
* [macOS Build](https://github.com/Warzone2100/warzone2100/actions?query=workflow%3AmacOS+branch%3Amaster+event%3Apush)
   * Select the latest "macOS" workflow run, download the `warzone2100_macOS_universal` and `warzone2100_macOS_universal_novideos` artifacts.
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
 * Test the crash handler via the `--crash` command line option, to make sure it produces a good dump file!
 * You will see a assert() in debug builds, and we do NOT want to release debug builds.
 * If you are testing the crash handler in debug builds, use `--noassert` to skip it.
 * '''The Crash dumps it produces should be sane.'''

### Test the game

TBD

### Done with the builds!


Create the tag for the new release
----------------------------------

Since everything works (since you tested it), it is time to make the **annotated** tag. Verify you are on the appropriate branch + commit, then tag it:

```shell
git tag -a 4.0.0
git push
git push origin 4.0.0
```

Where `4.0.0` is the name of the tag.

> ### Expected tag version format
> The expected tag version format is: `<SEMVER>(<prereleaseversion>)`
>
> Where `<SEMVER>` is `#.#.#` (examples: `4.0.0`, `4.0.1`, `4.9.12`)
>
> And the _optional_ `<prereleaseversion>` is a hyphen-prefixed pre-release identifier + version like: `-beta1` or `-rc1`
>
> #### Valid tag examples:
> - Normal releases: `4.0.0`, `4.0.1`, `4.0.22`
> - Pre-releases: `4.0.1-beta1`, `4.0.1-rc2`

> ### Always forwards
> _Do **NOT** re-use an existing tag / tag that was previously used!_
> If you make a mistake, or a bug is found, (etc), increment the version _again_.

> ### Annotated tags ONLY
> You must use `git tag -a <version>` to create an annotated tag. Lightweight tags will not work correctly.

GitHub will then **automatically**:
- create a Draft release in **[GitHub Releases](https://github.com/Warzone2100/warzone2100/releases)** for the new tag
   - (may take a minute or so to complete)
   - (uses [/.github/workflows/draft_tag_release.yml](/.github/workflows/draft_tag_release.yml))
- trigger the CI builds (as `workflow_run` event runs, after the "Draft Tag Release" workflow completes)


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
- Just make sure you download artifacts from the CI runs for the <b>tag</b> you created. This will be in "[`workflow_run`](https://github.com/Warzone2100/warzone2100/actions?query=branch%3Amaster+event%3Aworkflow_run)" event CI runs.
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
      - [x] `warzone2100_win_x64.DEBUGSYMBOLS.7z`
      - [x] `warzone2100_win_x64_installer.exe`
      - [x] `warzone2100_win_x64_portable.exe`
   - [x] Verify the SHA512 hashes towards the end of the most recent [`workflow_run` Windows workflow's](https://github.com/Warzone2100/warzone2100/actions?query=workflow%3AWindows+event%3Aworkflow_run) log match the files you downloaded.
      - [x] **Make sure you are viewing the build for the _tag_ you just made**
      - [x] Select the appropriate job (x86, x64, etc).
      - [x] Expand the "Compare Build Outputs" step, which outputs the SHA512 hashes at the bottom of its output.
   - [x] Download the macOS build from the draft release:
      - [x] `warzone2100_macOS.zip`
   - [x] Verify the SHA512 hash towards the end of the most recent [`workflow_run` macOS workflow's](https://github.com/Warzone2100/warzone2100/actions?query=workflow%3AmacOS+event%3Aworkflow_run) log matches the file you downloaded.
      - [x] **Make sure you are viewing the build for the _tag_ you just made**
      - [x] Select the `Package Universal Binary` job
      - [x] Scroll down to the `Output Build Info` step(s), which output the SHA512 at the bottom of their output.
   - [x] Download the tag's source tarball from the draft release:
      - [x] `warzone2100_src.tar.xz`
   - [x] Verify the SHA512 hash towards the end of the most recent [`workflow_run` Ubuntu workflow's](https://github.com/Warzone2100/warzone2100/actions?query=workflow%3AUbuntu+event%3Aworkflow_run) ("Package Source" job) log matches the file you downloaded.
      - [x] **Make sure you are viewing the build for the _tag_ you just made**
      - [x] Select the "Package Source" job
      - [x] Expand the "Rename Tarball & Output Info" step, which outputs the SHA512 at the bottom of its output.
- [x] Test each release asset to make sure they work, and show the proper version # (which should match the tag).


Edit & Publish the Release
--------------------------

Once you have verified all the release assets:
- [x] Click the "Edit" button next to the draft GitHub Release.
- [x] Remove the top text block from the release notes / description (as it instructs).
- [x] If this is a beta release, ensure that "This is a pre-release" is **checked**.
- [x] If this is **NOT** a beta release, ensure that "This is a pre-release" is **unchecked**.
- [x] Click the green "Publish Release" button.

That's it! Now just...

Sit Back and Watch the Magic
----------------------------

The release automation process will then:
* Make the new release available on GitHub
* Mirror the new release to SourceForge
   - (see: [/.github/workflows/mirror_release_sourceforge.yml](/.github/workflows/mirror_release_sourceforge.yml))
* Update the website
   - (see: [/.github/workflows/release.yml](/.github/workflows/release.yml#L12), wz2100.net repo's [fetch_latest_release_info.yml](https://github.com/Warzone2100/wz2100.net/blob/master/.github/workflows/fetch_latest_release_info.yml))
* Update the update-data
   - (see: [/.github/workflows/release.yml](/.github/workflows/release.yml#L25), update-data repo's [generate_updates_json.yml](https://github.com/Warzone2100/update-data/blob/master/.github/workflows/generate_updates_json.yml))
* Inform the lobby server of the new version
   - (see: the bottom of update-data repo's [generate_updates_json.yml](https://github.com/Warzone2100/update-data/blob/master/.github/workflows/generate_updates_json.yml))

Post-Release checklist
----------------------
- [x] Add a new milestone to https://github.com/Warzone2100/warzone2100/milestones
- [x] Tell everyone about it in the forums. You can use the `build_tools/changelog2bbcode.sh` script to massage the changelog into BBCode.
- [x] Send mail about it on the developer mailing list.
- [x] Change the title on our IRC channels about the new release.
- [x] Ask for a raise for doing all this work that nobody else wanted to do, and you got suckered into doing it.

And, I am sure that people will spread the word about this new release at the following sites & others.
 * [Reddit](https://www.reddit.com/r/warzone2100)
 * [ModDb](https://www.moddb.com/games/warzone-2100)
 * [Softonic](https://warzone-2100.en.softonic.com/)
 * [Gamershell](http://www.gamershell.com/news)
 * [Gamedev](http://www.gamedev.net/community/forums/forum.asp?forum_id=6)
