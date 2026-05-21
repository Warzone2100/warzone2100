# ProGuard rules for Warzone 2100 Android.
# SDL3's Java layer and WZActivity use JNI; keep all SDL classes and WZActivity.
-keep class org.libsdl.app.** { *; }
-keep class net.wz2100.wz.** { *; }
