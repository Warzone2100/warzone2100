# Warzone 2100 Android Port — Engineering Notes

Platform: Pixel 9 Pro Fold (Android 16 / API 36, arm64-v8a)  
Build: Debug APK via Docker (`./docker/android/build-apk.sh Debug`)  
Status: **Campaign launches and runs** as of 2026-05-24

---

## Crash sequence fixed

### 1 · 16 KB page-alignment (SIGABRT at `System.loadLibrary`)

**Cause:** `libmain.so` built with NDK r27 does not produce 16 KB-aligned segments, which Android 16 requires for arm64-v8a.  
**Fix:** Dockerfile already pins NDK r28c (`28.2.13676358`). Added `ndkVersion "28.2.13676358"` to `platforms/android/app/build.gradle` so AGP does not silently download its default r27. Added `export ANDROID_NDK="${ANDROID_NDK_HOME}"` in `entrypoint.sh` so the NDK r27 path that AGP writes to `ANDROID_NDK` does not leak into the CMake toolchain.

---

### 2 · Missing SDL3 Java layer (`NoSuchMethodError`)

**Cause:** `SDLActivity.java` was a hand-written stub from an older SDL3 draft (~177 lines, ~15 native methods). SDL3 3.4.2 (the vcpkg version) registers ~50 native methods including `nativeGetVersion`, `nativeInitMainThread`, `nativeRunMain` (returns `int` not `void`), etc.  
**Fix:** Replaced all four SDL Java stubs and added seven missing files with the real SDL3 3.4.2 Java layer from `libsdl-org/SDL release-3.4.2`.

Files replaced/added under `platforms/android/app/src/main/java/org/libsdl/app/`:
- `SDLActivity.java`, `SDLSurface.java`, `SDLAudioManager.java`, `SDLControllerManager.java` — replaced
- `SDL.java`, `SDLDummyEdit.java`, `SDLInputConnection.java` — new
- `HIDDevice.java`, `HIDDeviceBLESteamController.java`, `HIDDeviceManager.java`, `HIDDeviceUSB.java` — new

---

### 3 · `dlopen` failure: `libSDL3.so` not found

**Cause:** `WZActivity.getLibraries()` returned `["SDL3", "main"]`. vcpkg's `arm64-android` triplet sets `VCPKG_LIBRARY_LINKAGE=static`, so SDL3 is statically linked into `libmain.so`. There is no `libSDL3.so` in the APK.  
**Fix:** `WZActivity.getLibraries()` now returns only `["main"]`.

---

### 4 · SIGSEGV in `PHYSFS_init+140` (fault addr `0x636f72705f707061` = "app\_proc")

**Cause (first pass):** `WZActivity.getMainSharedObject()` was overridden to return `"libmain.so"` (bare name). SDL3 passes this string as `argv[0]` to `realmain`. PhysFS on Linux ignores `argv[0]` in favour of `readlink("/proc/self/exe")` → `/system/bin/app_process64`. Bytes `app_proc` at offset 12 of that string are read as a 64-bit pointer → SIGSEGV.  
**Fix (first pass):** Removed the `getMainSharedObject()` override so SDL3's default returns the full native library path (`nativeLibraryDir + "/libmain.so"`).

**Cause (second pass — same crash):** Even with the full path, PhysFS 3.2.0 on Android ignores `argv[0]` entirely and requires a `PHYSFS_AndroidInit` struct (containing `JNIEnv*` and Android `Context*`) to be passed as `argv0` cast to `const char*`. Without it, PhysFS falls back to `/proc/self/exe` → same SIGSEGV.  
**Fix (second pass):** In `src/main.cpp` `initialize_PhysicsFS()`:

```cpp
#if defined(__ANDROID__)
    PHYSFS_AndroidInit physfsAndroidInit;
    physfsAndroidInit.jnienv  = SDL_GetAndroidJNIEnv();
    physfsAndroidInit.context = SDL_GetAndroidActivity();
    result = PHYSFS_init((const char *) &physfsAndroidInit);
#else
    result = PHYSFS_init(argv_0);
#endif
```

`<SDL3/SDL_system.h>` included in the `WZ_OS_UNIX && __ANDROID__` guard at the top of `main.cpp`.

---

### 5 · `[scanDataDirs:709] Could not find game data. Aborting.`

**Cause:** `scanDataDirs()` has no Android path. On Android, `PHYSFS_getBaseDir()` returns the APK path; `getWZInstallPrefix()` derives a non-existent directory from it.  
**Fix — three-part:**

**a) Build: create game data archives (`entrypoint.sh`)**

```bash
(cd data/base && zip -r .../assets/data/base.wz .)
(cd data/mp   && zip -r .../assets/data/mp.wz   .)
# Append fonts to base.wz so fonts/DejaVuSans.ttf is accessible:
cp data/fonts/DejaVu/DejaVuSans*.ttf FONTS_TMPDIR/fonts/
(cd FONTS_TMPDIR && zip -r .../assets/data/base.wz fonts/)
```

`app/build.gradle` adds `aaptOptions { noCompress "wz" }` to prevent AAPT2 double-compression.

**b) Java: copy archives to internal storage (`WZActivity.java`)**

`WZActivity.onCreate()` calls `copyDataAssetsIfNeeded()` before `super.onCreate()`. Copies `assets/data/base.wz` and `assets/data/mp.wz` from the APK to `getFilesDir()/data/`. Skips the copy if the version code on disk matches the current APK.

**c) C++: register Android data path (`src/main.cpp scanDataDirs()`)**

```cpp
#if defined(__ANDROID__)
    if (!PHYSFS_exists("gamedesc.lev"))
    {
        const char *internalStorage = SDL_GetAndroidInternalStoragePath();
        if (internalStorage)
        {
            std::string androidDataDir = std::string(internalStorage) + "/data/";
            registerSearchPath(androidDataDir, 3);
            rebuildSearchPath(mod_multiplay, true);
        }
    }
#endif
```

`rebuildSearchPath` mounts `base.wz` and `mp.wz` from internal storage as real filesystem paths, making `gamedesc.lev` visible.

---

### 6 · `[iV_TextInit:1314] Failed to load base font: fonts/DejaVuSans.ttf`

**Cause:** `DejaVuSans.ttf` and `DejaVuSans-Bold.ttf` are in the `data/fonts` git submodule (`data-fonts.git`) under `DejaVu/` but were not included in `base.wz`.  
**Fix:** `entrypoint.sh` appends the font files from `data/fonts/DejaVu/` into `base.wz` under the `fonts/` path so `fonts/DejaVuSans.ttf` is accessible once `base.wz` is mounted.

---

### 7 · `[IV_TextInit:1323] Missing data file: fonts/NotoSansCJK-VF.otf.ttc`

**Cause:** Startup sanity check calls `debug(LOG_FATAL, ...)` when the CJK font is absent. The Noto font is ~53 MB and is not bundled.  
**Fix:** Added `!defined(__ANDROID__)` to the existing `!defined(__EMSCRIPTEN__)` guard. The CJK font is loaded on-demand; the on-demand path already handles failure gracefully (`failedToLoadCJKFonts = true`).

```cpp
#if !defined(__EMSCRIPTEN__) && !defined(__ANDROID__)
    debug(LOG_FATAL, "Missing data file: %s", CJK_FONT_PATH);
#endif
```

---

### 8 · SIGSEGV in `drawTerrain → gl_context::bind_pipeline` (null PSO)

**Cause:** `determineDefaultTerrainQuality()` picks `NORMAL_MAPPING` on a device with ≥8 GB RAM and high estimated VRAM (Pixel 9 Pro Fold: 16 GB, Adreno 830). The `TerrainCombined_High` pipeline (used for NORMAL\_MAPPING) fails silently on GLES, leaving a null PSO. `bind_pipeline` crashes dereferencing it at offset +12.  
**Fix:**

1. `src/terrain.cpp` — force `CLASSIC` terrain quality on Android until higher qualities are validated against GLES:

```cpp
#if defined(__ANDROID__)
    return TerrainShaderQuality::CLASSIC;
#endif
```

2. `lib/ivis_opengl/gfx_api_gl.cpp` — null guard in `bind_pipeline` to log and return instead of crashing if another pipeline fails:

```cpp
ASSERT_OR_RETURN(, pso != nullptr, "bind_pipeline called with null pipeline_state_object");
```

---

## Files changed

| File | Change |
|------|--------|
| `docker/android/Dockerfile` | NDK r28c, build-tools 36.0.0 (pre-existing) |
| `docker/android/build-apk.sh` | Fixed `docker volume rm` failure with `set -euo pipefail`; improved vcpkg error logging |
| `docker/android/entrypoint.sh` | Force `ANDROID_NDK="${ANDROID_NDK_HOME}"`; create `base.wz` + `mp.wz` from game data; append fonts to `base.wz`; improved vcpkg error output via `tee` |
| `platforms/android/app/build.gradle` | `ndkVersion "28.2.13676358"`; `aaptOptions { noCompress "wz" }` |
| `platforms/android/build.gradle` | AGP 8.9.1 (pre-existing) |
| `platforms/android/gradle/wrapper/gradle-wrapper.properties` | Gradle 8.11.1 (pre-existing) |
| `platforms/android/app/src/main/java/net/wz2100/wz/WZActivity.java` | `getLibraries()` returns `["main"]` only; `copyDataAssetsIfNeeded()` extracts `.wz` archives to internal storage |
| `platforms/android/app/src/main/java/org/libsdl/app/*.java` | Replaced stubs with full SDL3 3.4.2 Java layer (11 files) |
| `src/main.cpp` | Android-specific `PHYSFS_init` with `PHYSFS_AndroidInit`; Android data path in `scanDataDirs()` |
| `lib/ivis_opengl/gfx_api_gl.cpp` | Null guard in `bind_pipeline` |
| `lib/ivis_opengl/textdraw.cpp` | CJK font fatal check guarded for Android |
| `src/terrain.cpp` | Android defaults to `CLASSIC` terrain quality |

---

## Known limitations / future work

- **CJK text:** `NotoSansCJK-VF.otf.ttc` (~53 MB) is not bundled. CJK characters render as blanks (fail-safe, not a crash).
- **Terrain quality:** Forced to `CLASSIC` on Android. `MEDIUM` and `NORMAL_MAPPING` need GLES shader validation before enabling.
- **Touch input:** No touch→mouse shim; game is controlled via mouse emulation only. On-screen controls for keyboard-only actions (Ctrl, Tab, etc.) are not implemented.
- **Data update flow:** On APK upgrade, `copyDataAssetsIfNeeded()` detects the new `versionCode` and re-copies. First copy takes ~1–2 s on device (no progress indicator).
- **Music / videos:** `data/music/` and sequences are empty/not bundled.
- **Mods:** `data/mods/` not packaged in the APK.
