# Building Warzone 2100 for Android

## Prerequisites

| Tool | Version |
|------|---------|
| Android SDK | API 36 (Android 16) |
| Android NDK | 26.3.11579264 (r26c) |
| CMake | 3.22.1+ (bundled with NDK) |
| JDK | 17 (Temurin recommended) |
| Gradle | 8.7 (via wrapper) |
| AGP | 8.6.0 |
| vcpkg | current main |

## SDL3 Java Layer

The files under `app/src/main/java/org/libsdl/app/` are derived from the
SDL3 **3.2.x** `android-project/` template. If you update SDL3, re-copy the
matching Java files from the SDL3 source tree:

```
SDL3/android-project/app/src/main/java/org/libsdl/app/
```

Pin the SDL3 version to match what vcpkg provides (`vcpkg.json` →
`sdl3` dependency baseline). Run `vcpkg show sdl3` to confirm the exact version.

## SDL3 Shared Library

SDL3 must be built for `arm64-v8a` and placed in:

```
app/src/main/jniLibs/arm64-v8a/libSDL3.so
```

The simplest way to obtain it is to build SDL3 via vcpkg for the
`arm64-android` triplet, then copy the resulting `.so`:

```sh
export ANDROID_NDK_HOME=/path/to/ndk
vcpkg install sdl3 --triplet arm64-android
cp vcpkg/installed/arm64-android/lib/libSDL3.so \
   platforms/android/app/src/main/jniLibs/arm64-v8a/libSDL3.so
```

## Gradle Wrapper JAR

The `gradle-wrapper.jar` binary is **not** stored in git. Generate it once on
a machine with Gradle installed:

```sh
cd platforms/android
gradle wrapper --gradle-version 8.7 --distribution-type bin
```

This creates `gradle/wrapper/gradle-wrapper.jar` locally.

## Environment Variables

```sh
export ANDROID_HOME=/path/to/android-sdk
export ANDROID_NDK_HOME=/path/to/android-sdk/ndk/26.3.11579264
export VCPKG_ROOT=/path/to/vcpkg
export VCPKG_DEFAULT_TRIPLET=arm64-android
```

## vcpkg: arm64-android Triplet

Install vcpkg dependencies for Android:

```sh
cd /path/to/warzone2100
$VCPKG_ROOT/vcpkg install --triplet arm64-android
```

vcpkg will use `ANDROID_NDK_HOME` (or `ANDROID_NDK_ROOT`) to find the NDK
toolchain automatically.

## Building the APK

```sh
cd platforms/android
./gradlew assembleDebug
```

The debug APK will be in:
```
app/build/outputs/apk/debug/app-debug.apk
```

For a release build:
```sh
./gradlew assembleRelease
```

## Installing on Device / Emulator

```sh
adb install app/build/outputs/apk/debug/app-debug.apk
```

## Phase B / C Notes

- **Phase B** (asset loading): `data/` archives must be staged into
  `app/src/main/assets/` and `src/main.cpp scanDataDirs()` must register them
  via `SDL_GetAndroidInternalStoragePath()` or direct APK zip mounting.
- **Phase C** (touch input): Implement touch→mouse shim in `main_sdl.cpp`,
  add on-screen controls for keyboard-only actions (Ctrl, Tab, etc.).
