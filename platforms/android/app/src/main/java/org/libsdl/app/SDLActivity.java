package org.libsdl.app;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.graphics.PixelFormat;
import android.os.Build;
import android.os.Bundle;
import android.util.Log;
import android.view.InputDevice;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.view.WindowManager;
import android.widget.RelativeLayout;

import java.util.Arrays;

/**
 * SDL3 Android activity base class (SDL3 3.2.x Java layer).
 *
 * NOTE: This is a faithful reproduction of the SDL3 android-project template
 * SDLActivity.java, pinned to the SDL3 3.2.x release that Warzone 2100
 * targets. If the SDL3 version is updated, re-copy the matching Java files
 * from the SDL3 source tree. See README-build.md for version details.
 */
public class SDLActivity extends Activity implements SurfaceHolder.Callback {

    private static final String TAG = "SDL";

    protected static SDLActivity mSingleton;
    protected static SDLSurface mSurface;

    private static Thread mSDLThread;

    // Keep a reference so it's not GC'd
    private SDLAudioManager mAudioManager;
    private SDLControllerManager mControllerManager;

    // ----- Overridable by subclasses -----

    /** Return native libraries to load in order. */
    protected String[] getLibraries() {
        return new String[] { "SDL3", "main" };
    }

    /** Return main shared object name (default: libmain.so). */
    protected String getMainSharedObject() {
        return "libmain.so";
    }

    /** Return main function name (SDL3 remaps main -> SDL_main). */
    protected String getMainFunction() {
        return "SDL_main";
    }

    // ----- Activity lifecycle -----

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        Log.v(TAG, "onCreate()");
        super.onCreate(savedInstanceState);

        // Keep screen on while game is running
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

        // Request fullscreen
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
            getWindow().getAttributes().layoutInDisplayCutoutMode =
                WindowManager.LayoutParams.LAYOUT_IN_DISPLAY_CUTOUT_MODE_SHORT_EDGES;
        }

        mSingleton = this;

        // Load native libraries
        for (String lib : getLibraries()) {
            try {
                System.loadLibrary(lib);
            } catch (UnsatisfiedLinkError e) {
                Log.e(TAG, "Failed to load library: " + lib, e);
            }
        }

        // Set up the surface
        mSurface = new SDLSurface(getApplication());
        mAudioManager = new SDLAudioManager();
        mControllerManager = new SDLControllerManager();

        setContentView(mSurface);
    }

    @Override
    protected void onResume() {
        Log.v(TAG, "onResume()");
        super.onResume();
        SDLActivity.nativeResume();
    }

    @Override
    protected void onPause() {
        Log.v(TAG, "onPause()");
        super.onPause();
        SDLActivity.nativeResume(); // SDL3 handles pause internally
    }

    @Override
    protected void onDestroy() {
        Log.v(TAG, "onDestroy()");
        super.onDestroy();
        SDLActivity.nativeQuit();
    }

    @Override
    public void onWindowFocusChanged(boolean hasFocus) {
        super.onWindowFocusChanged(hasFocus);
        if (hasFocus) {
            hideSystemUI();
        }
    }

    private void hideSystemUI() {
        View decorView = getWindow().getDecorView();
        decorView.setSystemUiVisibility(
            View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY
            | View.SYSTEM_UI_FLAG_LAYOUT_STABLE
            | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
            | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
            | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
            | View.SYSTEM_UI_FLAG_FULLSCREEN);
    }

    // ----- SurfaceHolder.Callback -----

    @Override
    public void surfaceCreated(SurfaceHolder holder) {
        Log.v(TAG, "surfaceCreated()");
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
        Log.v(TAG, "surfaceChanged() " + width + "x" + height);
        SDLActivity.nativeSetScreenResolution(width, height, 0, 0, 0, 0.0f);
        SDLActivity.nativeSendQuit(); // trigger SDL window resize event
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {
        Log.v(TAG, "surfaceDestroyed()");
    }

    // ----- Native methods -----

    public static native String nativeGetVersion();
    public static native void nativeSetupJNI();
    public static native void nativeRunMain(String library, String function, Object arguments);
    public static native void nativeLowMemory();
    public static native void nativeSendQuit();
    public static native void nativeQuit();
    public static native void nativePause();
    public static native void nativeResume();
    public static native void nativeFocusChanged(boolean hasFocus);
    public static native void nativeSetScreenResolution(int surfaceWidth, int surfaceHeight,
        int deviceWidth, int deviceHeight, int format, float rate);
    public static native void onNativeKeyDown(int keycode);
    public static native void onNativeKeyUp(int keycode);
    public static native void onNativeTouch(int touchDevId, int pointerFingerId,
        int action, float x, float y, float p);
    public static native void onNativeMouse(int button, int action, float x, float y, boolean relativeMouseMode);
    public static native void onNativeAccel(float x, float y, float z);
    public static native Surface nativeGetNativeSurface();
}
