# iOS Xcode Build

This iOS port path is Xcode-first and no longer depends on the old Theos wrapper.
The iOS compiler, vcpkg triplets, and packaging helpers are isolated under `platforms/ios/` so normal macOS, Linux, Windows, and web builds keep using the upstream CMake paths.

What it does:
- configures the upstream `warzone2100/` project for `iPhoneOS`
- targets `arm64` for device builds by default
- sets the minimum OS to `iOS 16.0`
- defaults to Vulkan through MoltenVK for the Metal-backed iOS renderer
- keeps OpenGL ES available as an experimental renderer choice while leaving Vulkan/MoltenVK as the default
- signs the built executable with `ldid`
- generates `.ipa` packages
- syncs `data`, `docs`, and `locale` from `/Applications/Warzone 2100.app` so the iOS bundle carries the real desktop asset payload

Prerequisites:
- Install or build the desktop app before packaging iOS. By default the iOS staging scripts read `/Applications/Warzone 2100.app` and copy its `data`, `docs`, and `locale` payload into the iOS `.app`.
- If the desktop app is installed elsewhere, set `WARZONE_DESKTOP_APP_PATH` or pass the app path as the fourth helper argument:

```sh
WARZONE_DESKTOP_APP_PATH="$HOME/Applications/Warzone 2100.app" ./platforms/ios/build_xcode_ios.sh device
./platforms/ios/build_xcode_ios.sh device "" "" "$HOME/Applications/Warzone 2100.app"
```

Notes:
- `pkg/` stays in the source tree. It is upstream packaging metadata and does not block the iOS compile.
- macOS-only frameworks from the installed app are intentionally not copied into the iOS bundle.
- The installed MoltenVK iPhoneOS static library is currently `arm64` only. If you add an `arm64e` MoltenVK slice later, run with `IOS_DEVICE_ARCHS=arm64,arm64e`.

Build helper:

```sh
./platforms/ios/build_xcode_ios.sh
IOS_BUILD_CONFIGURATION=Debug ./platforms/ios/build_xcode_ios.sh device
```

Artifacts are written under `platforms/ios/build/artifacts/<device-or-simulator>/<configuration>/`.

GitHub Actions:
- `.github/workflows/ios_ipa_wip.yml` is a WIP CI artifact workflow, not a release workflow.
- It builds the device IPA and uploads it as a workflow artifact only.
- It runs on pushes and pull requests like the other CI workflows.
- It uses a local vendor copy, a `MOLTENVK_XCFRAMEWORK_ZIP_URL` secret, or Khronos' pinned `MoltenVK-ios.tar` release asset for MoltenVK.
- It does not create tags or GitHub releases.
