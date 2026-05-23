package net.wz2100.wz;

import android.os.Bundle;
import android.util.Log;

import org.libsdl.app.SDLActivity;

import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;

/**
 * Warzone 2100 Android activity.
 *
 * Extends SDLActivity (SDL3 3.4.2 Java layer). SDL3's SDLActivity.dlopen's
 * libSDL3.so then libmain.so (our game library) and calls SDL_main.
 */
public class WZActivity extends SDLActivity {

    private static final String TAG = "WZActivity";

    /**
     * Return the list of shared libraries to load.
     * SDL3 is statically linked into libmain.so by vcpkg (arm64-android triplet
     * uses static linkage). Load only the game library; SDL3's JNI_OnLoad runs
     * inside libmain.so.
     */
    @Override
    protected String[] getLibraries() {
        return new String[] {
            "main"
        };
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        copyDataAssetsIfNeeded();
        super.onCreate(savedInstanceState);
    }

    /**
     * Copies base.wz and mp.wz from APK assets to internal storage so that
     * PhysFS can mount them via real filesystem paths. Skips the copy when the
     * version code on disk already matches the current APK.
     */
    private void copyDataAssetsIfNeeded() {
        File filesDir = getFilesDir();
        File dataDir = new File(filesDir, "data");
        File versionFile = new File(filesDir, "data_version");

        // Determine current APK version code
        long currentVersionCode;
        try {
            currentVersionCode = getPackageManager()
                    .getPackageInfo(getPackageName(), 0)
                    .getLongVersionCode();
        } catch (Exception e) {
            Log.e(TAG, "Could not read version code", e);
            return;
        }

        // Skip if already extracted for this version
        if (versionFile.exists()) {
            try {
                byte[] buf = new byte[(int) versionFile.length()];
                try (java.io.FileInputStream fis = new java.io.FileInputStream(versionFile)) {
                    //noinspection ResultOfMethodCallIgnored
                    fis.read(buf);
                }
                long existingVersion = Long.parseLong(new String(buf).trim());
                if (existingVersion == currentVersionCode) {
                    return;
                }
            } catch (Exception e) {
                // Version mismatch or parse error — fall through and re-copy
            }
        }

        Log.i(TAG, "Copying game data assets to internal storage...");
        dataDir.mkdirs();

        String[] dataFiles = {"base.wz", "mp.wz"};
        for (String filename : dataFiles) {
            try (InputStream is = getAssets().open("data/" + filename);
                 FileOutputStream fos = new FileOutputStream(new File(dataDir, filename))) {
                byte[] buffer = new byte[65536];
                int len;
                while ((len = is.read(buffer)) > 0) {
                    fos.write(buffer, 0, len);
                }
                Log.i(TAG, "Copied " + filename);
            } catch (Exception e) {
                Log.e(TAG, "Failed to copy " + filename, e);
            }
        }

        // Write version file so we skip extraction next time
        try (FileOutputStream fos = new FileOutputStream(versionFile)) {
            fos.write(String.valueOf(currentVersionCode).getBytes());
        } catch (Exception e) {
            Log.e(TAG, "Failed to write version file", e);
        }
    }
}
