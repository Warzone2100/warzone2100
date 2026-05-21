package org.libsdl.app;

import android.content.Context;
import android.graphics.PixelFormat;
import android.util.Log;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

/**
 * SDL3 Android surface view (SDL3 3.2.x Java layer).
 *
 * Hosts the EGL/OpenGL ES surface. Forwards touch and key events to native SDL3.
 */
class SDLSurface extends SurfaceView implements SurfaceHolder.Callback {

    private static final String TAG = "SDL";

    SDLSurface(Context context) {
        super(context);
        getHolder().addCallback(this);
        getHolder().setFormat(PixelFormat.RGBA_8888);
        setFocusable(true);
        setFocusableInTouchMode(true);
        requestFocus();
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {
        Log.v(TAG, "SDLSurface.surfaceCreated()");
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
        Log.v(TAG, "SDLSurface.surfaceChanged() " + width + "x" + height);
        SDLActivity.nativeSetScreenResolution(width, height, 0, 0, format, 60.0f);
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {
        Log.v(TAG, "SDLSurface.surfaceDestroyed()");
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        int pointerCount = event.getPointerCount();
        for (int i = 0; i < pointerCount; i++) {
            int action = event.getActionMasked();
            int pointerId = event.getPointerId(i);
            float x = event.getX(i) / getWidth();
            float y = event.getY(i) / getHeight();
            float p = event.getPressure(i);
            SDLActivity.onNativeTouch(0, pointerId, action, x, y, p);
        }
        return true;
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        SDLActivity.onNativeKeyDown(keyCode);
        return true;
    }

    @Override
    public boolean onKeyUp(int keyCode, KeyEvent event) {
        SDLActivity.onNativeKeyUp(keyCode);
        return true;
    }
}
