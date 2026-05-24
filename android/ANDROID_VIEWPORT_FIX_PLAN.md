# Android: fix map viewport (1/4 screen, top-left) + initial zoom

## Context

Android build now launches via SDL3 and renders, but the 3D map renders into
roughly the top-left quarter of the physical display instead of filling the
screen. Additionally, the user reports the in-game map "feels small" at level
start — they want a closer default zoom for the touch/handheld experience.

The 1/4-at-top-left pattern is the classic signature of a viewport / drawable-size
mismatch under OpenGL: the framebuffer is allocated at the full physical surface
size (e.g. 2400×1080 on the test device), but `glViewport(0, 0, w, h)` is being
called with a smaller logical size (e.g. 1280×720). Because GL origin is
bottom-left while the on-screen presentation is top-left, the rendered region
lands in the top-left quarter.

`SDL_PROP_WINDOW_CREATE_HIGH_PIXEL_DENSITY_BOOLEAN` is already set
(`lib/sdl/main_sdl.cpp:2876`), but on Android the explicit
`SDL_PROP_WINDOW_CREATE_WIDTH/HEIGHT_NUMBER` request (lines 2860–2861) is
honored — SDL creates a sub-screen-sized window even though the activity is
fullscreen. The OS does not auto-stretch; it places the smaller surface at the
top-left of the fullscreen activity.

## Recommended approach

Two independent fixes, in this order:

### Fix 1 — viewport fills the device screen

In `lib/sdl/main_sdl.cpp`, inside
`wzMainScreenSetup_CreateVideoWindow()` (~line 2646), gate the explicit
window-size and position properties so that on Android we let SDL/Android pick
the actual surface size and force fullscreen mode at creation:

- Wrap the four property setters at lines 2858–2861
  (`SDL_PROP_WINDOW_CREATE_X/Y/WIDTH/HEIGHT_NUMBER`) so they are **not** set on
  `#if defined(__ANDROID__)`. Instead, set
  `SDL_PROP_WINDOW_CREATE_FULLSCREEN_BOOLEAN = true` so SDL creates the window
  at the SurfaceView's actual size.
- Skip the `displayUsableBounds` clamp block (lines 2796–2810ish) on Android —
  it is meaningless when the surface size comes from the Activity.
- After `SDL_GetWindowSize`/`SDL_GetWindowSizeInPixels` are read back
  (lines 2911, and the `screenWidth = windowWidth / displayScaleFactor` block
  at ~2960–2968), add an Android-only branch that, if the reported sizes are
  still smaller than the device's display bounds, retries by calling
  `SDL_SetWindowFullscreen(WZwindow, true)` and re-reads via
  `SDL_GetWindowSize` and `SDL_GetWindowSizeInPixels`.
- Inside `handleWindowSizeChange()` (~line 1863) the existing logic already
  recomputes `screenWidth/Height` and forwards to
  `gfx_api::context::get().handleWindowSizeChange()`, which is what triggers
  `glViewport` updates in `lib/ivis_opengl/gfx_api_gl.cpp:3485-3501`. No
  change needed there; we just need the first resize event after the
  SurfaceView becomes ready to be respected.

Diagnostic logging (kept behind `--debug 3d`, i.e. `LOG_3D`):
- After window creation, log `windowWidth/Height` (logical),
  `SDL_GetWindowSizeInPixels` (drawable), and `current_displayScaleFactor` —
  these are the three numbers that disambiguate which path is wrong if the
  symptom recurs.
- In `handleWindowSizeChange()`, log the old/new logical and drawable sizes.

Reuse, do not rewrite:
- `SDL_WZBackend_GetDrawableSize()` (`lib/sdl/main_sdl.cpp:541`) for drawable
  size queries.
- `wzChangeWindowMode()` (~line 750) for the fullscreen transition path — do
  not invent a parallel one.
- Existing `WZ_OS_ANDROID` / `__ANDROID__` guards (e.g. `lib/sdl/main_sdl.cpp:413`,
  `src/terrain.cpp:159-163`) as the conditional-compile pattern.

### Fix 2 — closer default zoom at level start, Android only

In `src/display3d.cpp`, in `init3DView()` at line 1590 the camera distance is
set via `distance = war_GetMapZoom();`. Add an Android-only override
immediately after that line:

```cpp
#if defined(__ANDROID__)
    // Mobile screens are small; start closer to make the map readable.
    distance = std::min<float>(distance, 1800.0f);
#endif
```

`1800` sits between `MINDISTANCE_CONFIG` (1600) and `STARTDISTANCE` (2600)
defined in `src/display.h:218-223`. This does not change the user's
`mapZoom` config — only the per-level starting distance — so manual zoom-out
via existing keybinds still works.

Do **not** edit `STARTDISTANCE` in `src/display.h` or the default in
`src/warzoneconfig.cpp:69`; those affect desktop builds too.

## Files to modify

- `lib/sdl/main_sdl.cpp` — Android window-creation properties, optional
  post-create fullscreen retry, debug logging in two spots.
- `src/display3d.cpp` — Android-only camera distance clamp inside
  `init3DView()`.

## Files to read for context (no changes)

- `lib/ivis_opengl/gfx_api_gl.cpp:3485-3501, 4177` — confirms `glViewport`
  already uses drawable size; only the input numbers need fixing upstream.
- `platforms/android/app/src/main/java/org/libsdl/app/SDLSurface.java:129-148`
  — the density value already flows to native via
  `nativeSetScreenResolution`; we are not (yet) wiring it into the C++ side,
  because the high-pixel-density window property should be sufficient once the
  explicit logical size is removed.

## Verification

1. **Local build (host):** `cmake --build build` (existing CI flow). Ensure no
   regression in desktop window creation — the Android conditionals must not
   alter the desktop code path. Spot-check by running the desktop build and
   confirming the window opens at the requested resolution.
2. **Android device build:** rebuild the APK via
   `platforms/android/gradlew assembleDebug`. Install with
   `adb install -r <apk>`.
3. **Boot smoke test on Pixel 9 Pro Fold:**
   - Launch the app. Expect the title screen / 3D scene to fill the entire
     physical display, not the top-left quarter.
   - Pull `adb logcat -s SDL/APP wz2100:*` and confirm the new `LOG_3D` lines
     report `windowSize ≈ drawableSize ≈ physical surface (e.g. 2400×1080)`.
4. **Mission start:** start a campaign or skirmish level. Confirm the map
     appears at a closer/readable zoom (camera distance reported by
     `setViewDistance()` log should be ≤ 1800 on Android, unchanged on desktop).
5. **Rotation / fold (optional but recommended on the Fold):** unfold or rotate
     mid-game. The `SDL_EVENT_WINDOW_RESIZED` path should fire, the new
     `handleWindowSizeChange()` logs should show updated sizes, and the
     viewport should re-fit.
6. **Regression sweep:** play 30s of gameplay — confirm UI elements (radar,
     command panel, build queue) are positioned correctly relative to the now-
     correct screen bounds. If they are off, the issue is in the UI screen
     coordinate system (`screenWidth/Height` vs window pixels) and a follow-up
     pass on `handleGameScreenSizeChange` will be needed.
