package com.delicacyset.superpowered;

import android.support.annotation.Keep;

import java.io.File;

/**
 * <p/>
 * Created by Dmitry Rusak on 9/26/17.
 */

public class AudioEngine {



    private OnPlayerEventsListener mOnPlayerEventsListener;
    private OnRecorderEventsListener mOnRecorderEventsListener;

    public AudioEngine(int sampleRate, int bufferSize) {
        AudioEngine(sampleRate, bufferSize);
    }

    public void setAudioEngineListener(AudioEngineListener audioEngineListener) {
        setOnPlayerEventsListener(audioEngineListener);
        setOnRecorderEventsListener(audioEngineListener);
    }

    public void setOnPlayerEventsListener(OnPlayerEventsListener onPlayerEventsListener) {
        mOnPlayerEventsListener = onPlayerEventsListener;
    }

    public void setOnRecorderEventsListener(OnRecorderEventsListener onRecorderEventsListener) {
        mOnRecorderEventsListener = onRecorderEventsListener;
    }

    public void init(int numberOfChannels, int playersCount, boolean loop, int mainPlayerIndex) {
        initNative(numberOfChannels, playersCount, loop, mainPlayerIndex);
    }

    public void reset() {
        resetNative();
    }

    public void preparePlayer(File file) {
        preparePlayer(file.getAbsolutePath(), 0, (int) file.length());
    }

    public void startRecording(File fileTemp, File fileDestination) {
        startRecordingNative(fileTemp.getAbsolutePath(), fileDestination.getAbsolutePath());
    }

    public void stopRecording() {
        stopRecordingNative();
    }

    public void startPlaying() {
        startPlayingNative(true);
    }

    public void setPlay(boolean shouldPlay) {
        setPlayNative(shouldPlay);
    }

    public void release() {
        releaseNative();
    }

    @Keep
    public void onPlayersPrepared() {
        if (mOnPlayerEventsListener != null) {
            mOnPlayerEventsListener.onPlayersPrepared();
        }
    }

    @Keep
    public void onError(int errorCode) {
        if (mOnPlayerEventsListener != null) {
            mOnPlayerEventsListener.onError(errorCode);
        }
    }

    @Keep
    public void onPlayerEnded(int indexOfPlayer) {
        if (mOnPlayerEventsListener != null) {
            mOnPlayerEventsListener.onPlayerEnded(indexOfPlayer);
        }
    }

    @Keep
    public void onRecordFinished() {
        if (mOnRecorderEventsListener != null) {
            mOnRecorderEventsListener.onRecordFinished();
        }
    }

    public interface OnPlayerEventsListener {
        void onPlayersPrepared();

        void onError(int errorCode);

        void onPlayerEnded(int indexOfPlayer);
    }

    public interface OnRecorderEventsListener {
        void onRecordFinished();
    }

    public interface AudioEngineListener
            extends AudioEngine.OnPlayerEventsListener, AudioEngine.OnRecorderEventsListener {

    }


    static {
        System.loadLibrary("AudioEngine");
    }

    private native void AudioEngine(int sampleRate, int bufferSize);
    private native void releaseNative();
    private native void initNative(int numberOfChannels, int playersCount, boolean loop, int mainPlayerIndex);
    private native void preparePlayer(String path, int fileOffset, int fileSize);
    private native void startRecordingNative(String tempPath, String destinationPath);
    private native void stopRecordingNative();
    private native void startPlayingNative(boolean fromBeginning);
    private native void setPlayNative(boolean shouldPlay);
    private native void resetNative();

    public native boolean isPrepared();

}
