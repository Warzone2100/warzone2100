package net.wz2100.wz;

import org.libsdl.app.SDLActivity;

/**
 * Warzone 2100 Android activity.
 *
 * Extends SDLActivity (SDL3 3.2.x Java layer). SDL3's SDLActivity.dlopen's
 * libSDL3.so then libmain.so (our game library) and calls SDL_main.
 */
public class WZActivity extends SDLActivity {

    /**
     * Return the list of shared libraries to load.
     * SDL3 loads these in order: libSDL3.so, then libmain.so (the game).
     */
    @Override
    protected String[] getLibraries() {
        return new String[] {
            "SDL3",
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
