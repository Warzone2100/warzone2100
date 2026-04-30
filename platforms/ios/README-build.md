# iOS Xcode Build

This iOS port path is Xcode-first and no longer depends on the old Theos wrapper.
The iOS compiler, vcpkg triplets, and packaging helpers are isolated under `platforms/ios/` so normal macOS, Linux, Windows, and web builds keep using the upstream CMake paths.

What it does:
- configures the upstream `warzone2100/` project for `iPhoneOS`
- targets `arm64` for device builds by default
- sets the minimum OS to `iOS 16.0`
- defaults to Vulkan through MoltenVK for the Metal-backed iOS renderer
- forces Vulkan/MoltenVK on iOS because the SDL OpenGL ES path can present a blank frame on simulator
- signs the built executable with `ldid`
- generates `.ipa` packages
- syncs `data`, `docs`, and `locale` from `/Applications/Warzone 2100.app` so the iOS bundle carries the real desktop asset payload

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
- `.github/workflows/ios_ipa_wip.yml` is a WIP/merge validation workflow, not a release workflow.
- It builds the device IPA and uploads it as a workflow artifact only.
- It does not create tags or GitHub releases.
