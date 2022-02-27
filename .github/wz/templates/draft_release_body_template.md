### Do not publish this release until all of the assets have been uploaded by CI, and you've verified them according to the Release process. (Then remove this text block.)
--------

`warzone2100_win_(arch)_installer.exe` -- The main Windows build (full installer)
`warzone2100_win_(arch)_portable.exe` -- The portable version of the Windows build (self contained -- install it anywhere!)
`warzone2100_macOS_universal.zip`  -- For macOS (10.10+)
`warzone2100_ubuntu18.04_amd64.deb` -- For Ubuntu 18.04
`warzone2100_ubuntu20.04_amd64.deb` -- For Ubuntu 20.04
`warzone2100_src.tar.xz` -- The tarball for Linux / BSD / whatever systems. (full source / data)

**IMPORTANT: To build from source, use the warzone2100_src.tar.xz tarball.**
> The auto-generated GitHub "_Source code (zip) / (tar.gz)_" links do **_not_** include the Git-based autorevision information.

To install the .deb files, use `sudo apt install`. Example:
`sudo apt install ./warzone2100_ubuntu20.04_amd64.deb`
