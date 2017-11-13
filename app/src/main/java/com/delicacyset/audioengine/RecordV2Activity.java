package com.delicacyset.audioengine;

import android.Manifest;
import android.content.Context;
import android.content.pm.PackageManager;
import android.databinding.DataBindingUtil;
import android.media.AudioManager;
import android.os.Handler;
import android.support.annotation.NonNull;
import android.support.v4.app.ActivityCompat;
import android.support.v4.content.ContextCompat;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.text.TextUtils;
import android.view.View;
import android.widget.CompoundButton;
import android.widget.Toast;

import com.delicacyset.audioengine.databinding.ActivityRecordV2Binding;
import com.delicacyset.superpowered.AudioEngine;

import java.io.File;

public class RecordV2Activity
        extends AppCompatActivity  {

    private ActivityRecordV2Binding mBinding;

    private int mSampleRate;
    private int mBufferSize;

    private String mCurrentPlaybackName;

    private Handler mUiHandler;

    private AudioEngine mAudioEngine;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mBinding = DataBindingUtil.setContentView(this, R.layout.activity_record_v2);

        mUiHandler = new Handler();

        AudioManager audioManager = (AudioManager) this.getSystemService(Context.AUDIO_SERVICE);
        String sampleRateString = audioManager.getProperty(AudioManager.PROPERTY_OUTPUT_SAMPLE_RATE);
        mSampleRate = TextUtils.isEmpty(sampleRateString) ? 44100 : Integer.parseInt(sampleRateString);

        String bufferSizeString = audioManager.getProperty(AudioManager.PROPERTY_OUTPUT_FRAMES_PER_BUFFER);
        mBufferSize = TextUtils.isEmpty(bufferSizeString) ? 512 : Integer.parseInt(bufferSizeString);

        mBinding.tvSampleRate.setText(String.valueOf(mSampleRate));
        mBinding.tvBufferSize.setText(String.valueOf(mBufferSize));

        boolean hasLowLatencyFeature =
                getPackageManager().hasSystemFeature(PackageManager.FEATURE_AUDIO_LOW_LATENCY);

        mBinding.tvLowLatencyFeature.setText(String.valueOf(hasLowLatencyFeature));

        mBinding.btnRecordBeat.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                onRecordBeatToggle(isChecked);
            }
        });

        mBinding.btnRecordVoice1.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                onRecordVoice1Toggle(isChecked);
            }
        });

        mBinding.btnPlaybackBeat.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                onPlaybackToggle(buttonView, isChecked, "beat_v2");
            }
        });
        mBinding.btnPlaybackVoice1.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                onPlaybackToggle(buttonView, isChecked, "beat_v2", "voice1_v2");
            }
        });

        if (ContextCompat.checkSelfPermission(this, Manifest.permission.RECORD_AUDIO) !=
                PackageManager.PERMISSION_GRANTED) {
            ActivityCompat.requestPermissions(this,
                    new String[]{Manifest.permission.RECORD_AUDIO},
                    1);
        }

        if (new File(getExternalFilesDir(null), "beat_v2" + ".wav").exists()) {
            mBinding.btnPlaybackBeat.setVisibility(View.VISIBLE);
            mBinding.containerVoice1.setVisibility(View.VISIBLE);
        }
        if (new File(getExternalFilesDir(null), "voice1_v2" + ".wav").exists()) {
            mBinding.btnPlaybackBeat.setVisibility(View.VISIBLE);
            mBinding.containerVoice1.setVisibility(View.VISIBLE);
            mBinding.btnPlaybackVoice1.setVisibility(View.VISIBLE);
        }

        if (new File(getExternalFilesDir(null), "voice2_v2" + ".wav").exists()) {
            mBinding.btnPlaybackBeat.setVisibility(View.VISIBLE);
            mBinding.containerVoice1.setVisibility(View.VISIBLE);
            mBinding.btnPlaybackVoice1.setVisibility(View.VISIBLE);
        }

        mAudioEngine = new AudioEngine(mSampleRate, mBufferSize);
    }

    @Override
    protected void onStop() {
        super.onStop();
        mCurrentPlaybackName = null;
        mAudioEngine.reset();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        releaseAudioEngine();
    }

    private void releaseAudioEngine() {
        if (mAudioEngine != null) {
            mAudioEngine.release();
            mAudioEngine = null;
            mBinding.progressBeat.setVisibility(View.GONE);
            mBinding.progressVoice1.setVisibility(View.GONE);
        }
    }

    private void onRecordBeatToggle(boolean startRecord) {
        if (!startRecord) {
            if (mAudioEngine != null) {
                mAudioEngine.stopRecording();
            }
            return;
        }
        mCurrentPlaybackName = null;
        mAudioEngine.reset();
        new File(getExternalFilesDir(null), "beat_v2" + ".wav").delete();
        mBinding.progressBeat.setVisibility(View.VISIBLE);

        mAudioEngine.setAudioEngineListener(new SimpleAudioEngineListener() {
            @Override
            public void onPlayersPrepared() {
                mAudioEngine.startRecording(
                        new File(getExternalFilesDir(null), "temp"),
                        new File(getExternalFilesDir(null), "beat_v2"));
            }

            @Override
            public void onRecordFinished() {
                mUiHandler.post(new Runnable() {
                    @Override
                    public void run() {
                        mBinding.containerVoice1.setVisibility(View.VISIBLE);
                        mBinding.btnPlaybackBeat.setVisibility(View.VISIBLE);
                        mBinding.progressBeat.setVisibility(View.GONE);
                        mBinding.progressVoice1.setVisibility(View.GONE);
                    }
                });
            }
        });
        mAudioEngine.init(2, 0, true, 0);
    }


    // -------------------- Voice 1

    private void onRecordVoice1Toggle(boolean startRecord) {
        if (!startRecord) {
            if (mAudioEngine != null) {
                mAudioEngine.stopRecording();
            }
            return;
        }
        mCurrentPlaybackName = null;
        mAudioEngine.reset();
        new File(getExternalFilesDir(null), "voice1_v2" + ".wav").delete();
        mBinding.progressVoice1.setVisibility(View.VISIBLE);

        mAudioEngine.setAudioEngineListener(new SimpleAudioEngineListener() {
            @Override
            public void onPlayersPrepared() {
                mAudioEngine.startRecording(
                        new File(getExternalFilesDir(null), "temp"),
                        new File(getExternalFilesDir(null), "voice1_v2"));
            }

            @Override
            public void onRecordFinished() {
                mUiHandler.post(new Runnable() {
                    @Override
                    public void run() {
                        mBinding.btnPlaybackVoice1.setVisibility(View.VISIBLE);
                        mBinding.progressBeat.setVisibility(View.GONE);
                        mBinding.progressVoice1.setVisibility(View.GONE);
                    }
                });
            }
        });
        mAudioEngine.init(2, 1, true, 0);
        mAudioEngine.preparePlayer(new File(getExternalFilesDir(null), "beat_v2" + ".wav"));
    }

    // ------------------ Voice 2

    private void onRecordVoice2Toggle(boolean startRecord) {
        if (!startRecord) {
            if (mAudioEngine != null) {
                mAudioEngine.stopRecording();
            }
            return;
        }
        mCurrentPlaybackName = null;
        mAudioEngine.reset();
        new File(getExternalFilesDir(null), "voice2_v2" + ".wav").delete();


        mAudioEngine.setAudioEngineListener(new SimpleAudioEngineListener() {
            @Override
            public void onPlayersPrepared() {
                mAudioEngine.startRecording(
                        new File(getExternalFilesDir(null), "temp"),
                        new File(getExternalFilesDir(null), "voice2_v2"));
            }

            @Override
            public void onRecordFinished() {
                mUiHandler.post(new Runnable() {
                    @Override
                    public void run() {
                        mBinding.progressBeat.setVisibility(View.GONE);
                        mBinding.progressVoice1.setVisibility(View.GONE);
                    }
                });
            }
        });
        mAudioEngine.init(2, 2, true, 1);
        mAudioEngine.preparePlayer(new File(getExternalFilesDir(null), "beat_v2" + ".wav"));
        mAudioEngine.preparePlayer(new File(getExternalFilesDir(null), "voice1_v2" + ".wav"));
    }

    private void onPlaybackToggle(CompoundButton btn, boolean play, @NonNull String... names) {

        if (mAudioEngine.isPrepared()) {
            if (TextUtils.equals(names[names.length - 1], mCurrentPlaybackName)) {
                mAudioEngine.setPlay(play);
                return;
            }
        }

        mAudioEngine.reset();

        mAudioEngine.setAudioEngineListener(new SimpleAudioEngineListener() {
            @Override
            public void onPlayersPrepared() {
                mAudioEngine.startPlaying();
            }
        });

        mAudioEngine.init(2, names.length, true, names.length > 1 ? 1 : 0);
        if (play) {
            mCurrentPlaybackName = names[names.length - 1];
            if (btn == mBinding.btnPlaybackBeat) {
                mBinding.btnPlaybackVoice1.setChecked(false);
            } else if (btn == mBinding.btnPlaybackVoice1) {
                mBinding.btnPlaybackBeat.setChecked(false);
            }

            if (names.length > 0) {
                mAudioEngine.preparePlayer(new File(getExternalFilesDir(null), names[0] + ".wav"));
            }
            if (names.length > 1) {
                mAudioEngine.preparePlayer(new File(getExternalFilesDir(null), names[1] + ".wav"));
            }
            if (names.length > 2) {
                mAudioEngine.preparePlayer(new File(getExternalFilesDir(null), names[2] + ".wav"));
            }

        }
    }

    private class SimpleAudioEngineListener
            implements AudioEngine.AudioEngineListener {

        @Override
        public void onPlayersPrepared() {

        }

        @Override
        public void onError(final int errorCode) {
            mUiHandler.post(new Runnable() {
                @Override
                public void run() {
                    Toast.makeText(RecordV2Activity.this, "Error: " + errorCode, Toast.LENGTH_SHORT).show();
                }
            });

        }

        @Override
        public void onPlayerEnded(int indexOfPlayer) {

        }

        @Override
        public void onRecordFinished() {

        }
    }
}
