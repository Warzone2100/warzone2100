package org.libsdl.app;

import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioRecord;
import android.media.AudioTrack;
import android.media.MediaRecorder;
import android.util.Log;

/**
 * SDL3 Android audio manager (SDL3 3.2.x Java layer).
 *
 * Provides Java-side audio output and capture for SDL3's Android audio backend.
 */
class SDLAudioManager {

    private static final String TAG = "SDL";

    protected static AudioTrack mAudioTrack;
    protected static AudioRecord mAudioRecord;

    public SDLAudioManager() {
        mAudioTrack = null;
        mAudioRecord = null;
    }

    public static int[] audioOpen(int sampleRate, int audioFormat, int desiredChannels, int desiredFrames) {
        int channelConfig;
        int audioFormatConst;

        switch (desiredChannels) {
            case 1: channelConfig = AudioFormat.CHANNEL_OUT_MONO; break;
            case 2: channelConfig = AudioFormat.CHANNEL_OUT_STEREO; break;
            default: channelConfig = AudioFormat.CHANNEL_OUT_STEREO; desiredChannels = 2; break;
        }

        switch (audioFormat) {
            case 0x8010: audioFormatConst = AudioFormat.ENCODING_PCM_16BIT; break; // AUDIO_S16
            default: audioFormatConst = AudioFormat.ENCODING_PCM_16BIT; break;
        }

        int frameSize = desiredChannels * 2; // 16-bit samples
        int minBufferSize = AudioTrack.getMinBufferSize(sampleRate, channelConfig, audioFormatConst);
        int desiredBufferSize = desiredFrames * frameSize;
        int bufferSize = Math.max(minBufferSize, desiredBufferSize);

        mAudioTrack = new AudioTrack(
            AudioManager.STREAM_MUSIC, sampleRate, channelConfig,
            audioFormatConst, bufferSize, AudioTrack.MODE_STREAM);

        int[] results = { sampleRate, audioFormat, desiredChannels, desiredFrames };
        Log.v(TAG, "audioOpen: " + sampleRate + " Hz, " + desiredChannels + " ch, " + desiredFrames + " frames");
        return results;
    }

    public static void audioWriteShortBuffer(short[] buffer) {
        if (mAudioTrack == null) return;
        if (mAudioTrack.getPlayState() != AudioTrack.PLAYSTATE_PLAYING) {
            mAudioTrack.play();
        }
        for (int i = 0; i < buffer.length; ) {
            int written = mAudioTrack.write(buffer, i, buffer.length - i);
            if (written <= 0) break;
            i += written;
        }
    }

    public static void audioWriteByteBuffer(byte[] buffer) {
        if (mAudioTrack == null) return;
        if (mAudioTrack.getPlayState() != AudioTrack.PLAYSTATE_PLAYING) {
            mAudioTrack.play();
        }
        for (int i = 0; i < buffer.length; ) {
            int written = mAudioTrack.write(buffer, i, buffer.length - i);
            if (written <= 0) break;
            i += written;
        }
    }

    public static void audioClose() {
        if (mAudioTrack != null) {
            mAudioTrack.stop();
            mAudioTrack.release();
            mAudioTrack = null;
        }
    }
}
