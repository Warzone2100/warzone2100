# Warzone 2100 — Android (API 36 / Android 16) Port

## Context

Warzone 2100 is a large desktop C++17 RTS (~790 source files, CMake build) with
**no Android support today**: no `platforms/android/`, no Gradle project, no NDK
toolchain wiring, no JNI bridge, and no `WZ_OS_ANDROID`. The goal is a **fully
playable Android port targeting Android 16 (API 36)**.

Two hard realities shape this plan:

1. The engine is already well-positioned: it uses **SDL3** (which has first-class
   Android support), has a graphics abstraction (`gfx_api`) with a working
   **OpenGL ES 3** path (already used for Emscripten), and abstracts input and
   filesystem (PhysFS).
2. This container has **no Android SDK/NDK/emulator** and a full build pulls
   ~20 native dependencies. So this session delivers **config-correct scaffolding
   and structural source changes** — it compiles into an APK *in a real build
   environment*, but cannot be built or run here. A truly polished, playable port
   (touch controls, asset packaging validated on-device, multiplayer) spans
   multiple stages that require a real Android toolchain; those are explicitly
   staged below as Phase B/C.

A genuine bug also surfaced: `lib/framework/wzglobal.h:109` matches Android as
desktop Linux (Android defines `__linux__`), which must be fixed for any Android
build to behave correctly.

## Approach (single recommended path)

- Build the game on Android as a **shared library `libmain.so`** (not an
  executable). SDL3's `SDLActivity` `dlopen`s `libSDL3.so` + `libmain.so` and
  calls `SDL_main`; WZ's existing `main()` works unchanged because
  `<SDL3/SDL_main.h>` remaps `main`→`SDL_main` on Android.
- Default graphics backend = **OpenGL ES 3** (the existing GLES3 path). Vulkan is
  deferred.
- Drive the existing top-level CMake via Gradle `externalNativeBuild`; the NDK
  supplies the toolchain (`CMAKE_SYSTEM_NAME=Android`, `ANDROID_ABI`, etc.) — we
  only *react* to `CMAKE_SYSTEM_NAME MATCHES "Android"`.
- Dependencies via **vcpkg `arm64-android` triplet**; structurally gate out
  Android-incompatible deps now, document the real-build commands.
- Follow SDL3's canonical `android-project/` template layout.

API levels: `compileSdk = 36`, `targetSdk = 36`, `minSdk = 26`. ABI: `arm64-v8a`
only for the foundation (others a one-line follow-on).

## Phase A — Buildable scaffold (this session: config + structure)

### Source / CMake edits

- **`lib/framework/wzglobal.h`** — insert before line 109 (`#elif defined(__linux__)`):
  a `#elif defined(__ANDROID__)` branch defining `WZ_OS_ANDROID` (and a marker that
  it is *not* desktop Linux). Keep `WZ_OS_UNIX` defined (POSIX prefs paths). Audit
  `grep -rn WZ_OS_LINUX` for desktop-only assumptions (XDG, fontconfig, `/usr/share`)
  and re-gate any with `!defined(WZ_OS_ANDROID)` — most become inactive automatically
  since `WZ_OS_LINUX` is no longer defined on Android.
- **`src/CMakeLists.txt`** (the load-bearing change, near the `add_executable` at
  ~line 91): on Android, `add_library(warzone2100 SHARED ...)` with
  `OUTPUT_NAME "main"` (→ `libmain.so`) and link `android log`. Wrap the desktop
  `install(TARGETS warzone2100 ...)` and Windows DLL-copy blocks to also exclude
  Android (extend the existing Emscripten exclusion to `MATCHES "(Emscripten|Android)"`).
- **Top-level `CMakeLists.txt`** — add `CMAKE_SYSTEM_NAME MATCHES "Android"`
  branches: `WZ_DATADIR`/`WZ_LOCALEDIR` placeholder (assets come via AAssetManager),
  disable `ENABLE_DOCS`, keep GLES, leave Vulkan/Discord off for Android.
- **`lib/sdl/CMakeLists.txt`** — generally fine (`find_package(SDL3 ... CONFIG)`
  resolves the NDK-built SDL3). Add an `if(Android)` block linking `android log`
  only if link errors are anticipated.
- **`lib/sdl/main_sdl.cpp`** — in `wzAvailableGfxBackends()` (~line 413) add a
  `#elif defined(WZ_OS_ANDROID)` branch listing `video_backend::opengles` first
  (no desktop GL). Entry point needs no change (add a clarifying comment only).

### New files under `platforms/android/` (mirror SDL3 `android-project/`)

- `settings.gradle`, top-level `build.gradle`, `gradle.properties`,
  `gradlew`/`gradlew.bat`, `gradle/wrapper/gradle-wrapper.properties`
  (pin AGP 8.6+/Gradle 8.7+ for `compileSdk 36`).
- `app/build.gradle` — Android application module, `externalNativeBuild { cmake {
  path = <repo-root>/CMakeLists.txt } }`, `ndk { abiFilters "arm64-v8a" }`,
  `compileSdk/targetSdk 36`, `minSdk 26`, `jniLibs` srcDir for `libSDL3.so`.
- `app/src/main/AndroidManifest.xml` — fullscreen game activity, `WZActivity`.
- `app/src/main/java/net/wz2100/wz/WZActivity.java` — `extends SDLActivity`,
  override `getLibraries()` → `{"SDL3","main"}`.
- `app/src/main/java/org/libsdl/app/*.java` — SDL3's Android Java layer copied
  verbatim, pinned to the SDL3 3.2.x release WZ targets (documented in README).
- `app/src/main/res/...` (strings, launcher icons reusing WZ artwork),
  `proguard-rules.pro`.
- `platforms/android/README-build.md` — documents: the SDL3 Java version,
  `gradle wrapper` step for the binary `gradle-wrapper.jar` (cannot be authored in
  text), vcpkg `arm64-android` triplet env, and the real build commands.
- **`vcpkg.json`** — add `"platform"` selectors (mirroring existing
  `!emscripten`/`emscripten`) to exclude Android-incompatible deps (Discord,
  crashpad, and defer GameNetworkingSockets) for Android.

### GitHub Actions CI (new workflow)

Add **`.github/workflows/CI_android.yml`**, following the naming and structure of
the existing per-platform workflows (`CI_windows.yml`, `CI_macos.yml`,
`CI_emscripten.yml`, `CI_ubuntu.yml`). The job:

- Triggers on `push`/`pull_request` (mirror the filters used by the other
  `CI_*` workflows in this repo) and `workflow_dispatch` for manual runs.
- Runs on `ubuntu-latest`; `actions/checkout` with `submodules: recursive`
  (WZ relies on git submodules).
- Sets up the toolchain: JDK (`actions/setup-java`, Temurin 17), the Android SDK
  (`android-actions/setup-android` or `gradle/actions`), `compileSdk`/platform 36,
  and the Android NDK pinned to the version the Gradle project declares.
- Provisions vcpkg for the `arm64-android` triplet (cache vcpkg + NDK to keep runs
  reasonable), exporting `VCPKG_DEFAULT_TRIPLET=arm64-android` /
  `ANDROID_NDK_HOME` as the Gradle/CMake integration expects.
- Builds via Gradle: `cd platforms/android && ./gradlew assembleDebug`
  (debug APK for `arm64-v8a`). The existing top-level CMake is driven through
  Gradle `externalNativeBuild`, so no separate CMake invocation is needed.
- Uploads the resulting APK with `actions/upload-artifact` so the build output is
  downloadable from the workflow run.

Note: because this container has no Android toolchain, the workflow is authored to
convention and validated by inspection (YAML correctness, step ordering, matching
the repo's existing `CI_*` patterns) — its first real execution happens on GitHub's
runners.

### Achievable-now caveat
Phase A produces a config-correct tree and structural code. It **cannot be
compiled here** (no NDK). `gradle-wrapper.jar` is binary and is generated on a dev
machine. Output is verified by inspection against Android/SDL3/vcpkg conventions.

## Phase B — Launch + asset loading (requires real build env)

- Stage `data/` (`.wz` archives) into `app/src/main/assets/` (CMake/Gradle copy
  task in `platforms/android/cmake/android_assets.cmake`).
- **`src/main.cpp` `scanDataDirs()`** (~line 602): add a `WZ_OS_ANDROID` branch
  registering the Android asset source as a PhysFS search path *before* the
  `gamedesc.lev` checks. Target design: **mount the APK (a zip) and its `assets/`
  subpath in PhysFS**; lower-risk first step: **extract-on-first-run** to
  `SDL_GetAndroidInternalStoragePath()` then register that real dir.
- Map the PhysFS write/pref dir to `SDL_GetAndroidInternalStoragePath()`.
- Add `WZ_OS_ANDROID` guards in `main_sdl.cpp` window/fullscreen/icon code (use the
  existing Emscripten guards as a template) so it reaches the main menu.

## Phase C — Touch input + playability (requires real build env)

- Touch→mouse shim in the `main_sdl.cpp` event loop (enable SDL touch synthesis
  first), then real gestures: two-finger pan/zoom camera, long-press = right-click,
  on-screen affordances for keyboard-only actions.
- Deferred: multiplayer (GameNetworkingSockets on Android), crash reporting,
  additional ABIs (`x86_64`, `armeabi-v7a`), Vulkan backend, OBB/Play Asset
  Delivery for distribution.

## Critical files

- `lib/framework/wzglobal.h` — `__ANDROID__`/`WZ_OS_ANDROID` branch (bug fix)
- `src/CMakeLists.txt` — SHARED `libmain.so` on Android, exclude desktop install
- `CMakeLists.txt` — top-level Android platform branches
- `lib/sdl/main_sdl.cpp` — GLES3-first backend branch (A); window guards + touch (B/C)
- `src/main.cpp` — `scanDataDirs()` Android asset/pref-dir branch (B)
- New: `platforms/android/**` (Gradle project, manifest, `WZActivity.java`,
  copied `org/libsdl/app/*.java`, README-build.md), `vcpkg.json` selectors
- New: `.github/workflows/CI_android.yml` — GitHub Actions workflow that builds
  the APK (SDK/NDK + vcpkg `arm64-android` setup, `./gradlew assembleDebug`,
  uploads the APK artifact), matching the repo's existing `CI_*` workflows

## Verification

- **This session (Phase A):** no compile possible. Verify by inspection: the
  `wzglobal.h` branch precedes the `__linux__` check; `src/CMakeLists.txt` produces
  `libmain.so` on Android; Gradle/manifest/`build.gradle` match SDL3
  `android-project` conventions and API-36 settings; `vcpkg.json` selectors parse;
  `.github/workflows/CI_android.yml` is valid YAML and mirrors the existing
  `CI_*` workflow structure.
- **CI:** once pushed, the `CI_android.yml` workflow performs the actual APK build
  on GitHub's runners — this is the first real compile of the Android target and
  the practical validation that the scaffolding is correct.
- **Real build environment (documented in `README-build.md`, Phase B/C):**
  install Android SDK (API 36) + NDK, set `VCPKG_DEFAULT_TRIPLET=arm64-android`,
  run `gradle wrapper`, then `./gradlew assembleDebug`. Install the APK on an
  Android 16 device/emulator; confirm it launches to the main menu (Phase B) and
  is controllable via touch (Phase C).
