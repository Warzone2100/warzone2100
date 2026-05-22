# Android 16 APK Build — Findings

Audit of the Android build (`platforms/android/`, `docker/android/`, `.github/workflows/CI_android.yml`) for correctness against **Android 16** (`compileSdk 36` / `targetSdk 36`, `minSdk 28`, arm64-v8a only).

## Summary

The build is misconfigured for `compileSdk 36`. The Android Gradle Plugin and build-tools pinned in the project are **too old to compile API level 36**, and the NDK version is inconsistent across Docker, CI, and the docs. The uncommitted working-tree edits are 16 KB page-size mitigations that are fragile because they depend on the exact NDK revision.

## Findings (root causes)

1. **AGP too old for `compileSdk 36`.**
   `platforms/android/build.gradle` pins AGP `8.6.0`. **AGP 8.6 supports a maximum API level of 35.** Building `compileSdk 36` requires **AGP 8.9.1+** (paired with Gradle 8.11.1+). This is the primary "not correct for Android 16" defect — AGP warns or fails when `compileSdk` exceeds its supported max.

2. **build-tools too old.**
   Both `docker/android/Dockerfile` and `CI_android.yml` install `build-tools;34.0.0`. API 36 needs **build-tools 36.0.0**.

3. **NDK version inconsistency across the three sources.**
   - `docker/android/Dockerfile`: NDK `27.2.12479018` (r27)
   - `CI_android.yml` + `platforms/android/README-build.md`: NDK `26.3.11579264` (r26c)

   These do not agree. It matters because the 16 KB page-size flag below behaves differently per NDK revision.

4. **16 KB page-size mitigations are version-fragile (uncommitted working-tree edits).**
   - `platforms/android/app/build.gradle` adds `-DANDROID_SUPPORT_FLEXIBLE_PAGE_SIZES=ON` — this flag exists **only in NDK r27+**. It is unknown on r26c (the version CI/README declare).
   - New overlay triplet `.ci/vcpkg/overlay-triplets/arm64-android.cmake` adds `-Wl,-z,max-page-size=16384` so vcpkg-built shared libs (e.g. `libSDL3.so`) get 16 KB alignment.

   On **NDK r28+, 16 KB alignment is the default** for `arm64-v8a` / `x86_64`, so both of these become redundant. On r26c they don't work at all.

5. **Stale docs.**
   `platforms/android/README-build.md` lists NDK r26c and AGP 8.6.0 — no longer matches the Docker setup or the requirements for Android 16.

## The 16 KB page-size requirement

Since **November 1, 2025**, new apps and updates submitted to Google Play that target Android 15+ devices must support **16 KB memory page sizes** on 64-bit devices. Any app with native (`.so`) code — Warzone 2100 included — must be rebuilt with 16 KB-aligned segments.

How alignment is achieved:
- **NDK r28+** links `arm64-v8a` / `x86_64` shared libraries 16 KB-aligned **by default** (no flags needed). NDK r27 requires opt-in via `ANDROID_SUPPORT_FLEXIBLE_PAGE_SIZES=ON`; r26c and earlier cannot do it.
- **AGP 8.5.1+** automatically aligns uncompressed `.so` files inside the APK during packaging.

So the cleanest correct setup is **NDK r28+** (alignment by default) + **AGP 8.5.1+** (packaging alignment) — which also means the manual flag and overlay triplet can be dropped.

## Recommended toolchain (correct for Android 16)

| Tool | Current | Recommended |
|------|---------|-------------|
| NDK | r27 (Docker) / r26c (CI, README) | **r28+** — e.g. `28.2.13676358` (r28c); 16 KB alignment default |
| AGP | 8.6.0 | **8.9.1** (min for `compileSdk 36`) |
| Gradle | 8.7 | **8.11.1** (pairs with AGP 8.9.x) |
| build-tools | 34.0.0 | **36.0.0** |
| compileSdk / targetSdk | 36 | 36 (unchanged) |
| minSdk | 28 | 28 (unchanged) |
| JDK | 17 | 17 (unchanged — AGP 8.9 supports JDK 17) |
| CMake | 3.22.1 | 3.22.1 (unchanged; above NDK r28 minimum of 3.10) |

## Files that would need changing to fix it

(Not changed in this audit — listed for reference.)

- `docker/android/Dockerfile` — `NDK_VERSION` → r28; `build-tools;34.0.0` → `build-tools;36.0.0`; Gradle 8.7 → 8.11.1 download + PATH.
- `platforms/android/build.gradle` (top-level) — AGP `8.6.0` → `8.9.1`.
- `platforms/android/app/build.gradle` — remove `-DANDROID_SUPPORT_FLEXIBLE_PAGE_SIZES=ON` (redundant on r28).
- `platforms/android/gradle/wrapper/gradle-wrapper.properties` — `gradle-8.7-bin.zip` → `gradle-8.11.1-bin.zip`.
- `docker/android/entrypoint.sh` — `gradle wrapper --gradle-version 8.7` → `8.11.1`; drop `--overlay-triplets` (default triplet handles alignment on r28).
- `.ci/vcpkg/overlay-triplets/arm64-android.cmake` — delete (untracked; not needed on r28).
- `.github/workflows/CI_android.yml` — bump `NDK_VERSION` → r28, `build-tools` → 36.0.0, gradle wrapper version → 8.11.1.
- `platforms/android/README-build.md` — refresh the prerequisites table to the recommended toolchain.

## Verifying 16 KB alignment after a build

```sh
# NDK helper, or manually:
objdump -p path/to/libSDL3.so | grep LOAD     # align should be 2**14 (16 KB)
```

## Sources

- [Support 16 KB page sizes — Android Developers](https://developer.android.com/guide/practices/page-sizes)
- [Prepare your apps for Google Play's 16 KB page size requirement (May 2025)](https://android-developers.googleblog.com/2025/05/prepare-play-apps-for-devices-with-16kb-page-size.html)
- [Transition to 16 KB page sizes with Android Studio (July 2025)](https://android-developers.googleblog.com/2025/07/transition-to-16-kb-page-sizes-android-apps-games-android-studio.html)
- [NDK r28 Changelog](https://github.com/android/ndk/wiki/Changelog-r28)
- [NDK r27 Changelog](https://github.com/android/ndk/wiki/Changelog-r27)
- [NDK Revision History](https://developer.android.com/ndk/downloads/revision_history)
- [AGP 8.6.0 release notes](https://developer.android.com/build/releases/past-releases/agp-8-6-0-release-notes)
- [About the Android Gradle plugin (version compatibility)](https://developer.android.com/build/releases/about-agp)
