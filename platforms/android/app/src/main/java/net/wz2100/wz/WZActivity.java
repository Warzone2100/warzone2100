package net.wz2100.wz;

import org.libsdl.app.SDLActivity;

/**
 * Warzone 2100 Android activity.
 *
 * Extends SDLActivity (SDL3 3.4.2 Java layer). SDL3's SDLActivity.dlopen's
 * libSDL3.so then libmain.so (our game library) and calls SDL_main.
 */
public class WZActivity extends SDLActivity {

    /**
     * Return the list of shared libraries to load.
     * SDL3 loads these in order: libSDL3.so, then libmain.so (the game).
     */
    @Override
    protected String[] getLibraries() {
        // SDL3 is statically linked into libmain.so by vcpkg (arm64-android triplet
        // uses static linkage). Load only the game library; SDL3's JNI_OnLoad runs
        // inside libmain.so.
        return new String[] {
            "main"
        };
    }

    /**
     * Return the application name shown in the SDL window title.
     */
    @Override
    protected String getMainSharedObject() {
        return "libmain.so";
    }
}
