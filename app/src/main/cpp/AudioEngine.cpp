//
// Created by Dmitry R. on 9/26/17.
//

#include "AudioEngine.h"
#include <SuperpoweredSimple.h>
#include <stdlib.h>
#include <android/log.h>
#include <SuperpoweredCPU.h>
#include <SLES/OpenSLES_AndroidConfiguration.h>
#include <SLES/OpenSLES.h>


JavaVM *javaVM;
jobject g_jniCallbackInstance = NULL;
jclass g_jniCallbackClazz = NULL;
// methods
jmethodID jniMethodOnPlayersPrepared;
jmethodID jniMethodOnError;   // params: int - error code
jmethodID jniMethodOnPlayerEnded; // params: int - index of player
jmethodID jniMethodOnRecordFinished;

bool needDetachJvm = false;


#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "AudioEngine", __VA_ARGS__)

JNIEnv* getEnv() {
    JNIEnv *env;
    int status = javaVM->GetEnv((void**)&env, JNI_VERSION_1_6);
    LOGI("getEnv: status: %d", status);
    if(status != JNI_OK) {
        needDetachJvm = true;
        status = javaVM->AttachCurrentThread(&env, NULL);
        if(status != JNI_OK) {
            return NULL;
        }
    } else {
        needDetachJvm = false;
    }
    return env;
}

void detachAfterCallbackDone() {
    if (needDetachJvm) {
        needDetachJvm = false;
        javaVM->DetachCurrentThread();
    }
}



static void onRecorderFlushedData(void *clientData) {
    LOGI("recorder flushed data to file!");
    AudioEngine *recorder = *((AudioEngine **)clientData);
    if (recorder != NULL) {
        recorder->notifyRecordFinished();
    }
}

static void playerEventCallback(void *clientData, SuperpoweredAdvancedAudioPlayerEvent event, void * __unused value) {
    void **params = (void **)(clientData);
    AudioEngine *recorder = (AudioEngine *) params[0];
    PlayerWrapper *playerWrapper = (PlayerWrapper *) params[1];
//    int *playerIndex = (int *) params[1];
    if (recorder != NULL) {
        recorder->onPlayerStateChangedPrepared(playerWrapper, event);
    }
}

static bool audioProcessing(void *clientdata, short int *audioIO, int numberOfSamples, int __unused samplerate) {
    return ((AudioEngine *)clientdata)->process(audioIO, (unsigned int)numberOfSamples);
}

void freePlayersMemory(PlayerWrapper **players, int playersCount) {
    for (int i = 0; i < playersCount; i++) {
        players[i]->player->pause();
        delete players[i]->player;
        delete players[i];
    }
    delete[] players;
}

void cacheObjects() {
    JNIEnv *env = getEnv();
    if (env != NULL) {
        jclass jniCallbackClazz = env->FindClass("com/delicacyset/superpowered/AudioEngine");
        // check error
        g_jniCallbackClazz = reinterpret_cast<jclass>(env->NewGlobalRef(jniCallbackClazz));


        jniMethodOnPlayersPrepared = env->GetMethodID(g_jniCallbackClazz,
                                                      "onPlayersPrepared", "()V");
        jniMethodOnError = env->GetMethodID(g_jniCallbackClazz,
                                            "onError", "(I)V");
        jniMethodOnPlayerEnded = env->GetMethodID(g_jniCallbackClazz,
                                                  "onPlayerEnded", "(I)V");
        jniMethodOnRecordFinished = env->GetMethodID(g_jniCallbackClazz,
                                                     "onRecordFinished", "()V");
    }
}

AudioEngine::AudioEngine(int sampleRate, int bufferSize) : sampleRate(sampleRate),
                                                               bufferSize(bufferSize) {
    cacheObjects();

    pthread_mutex_init(&mutex, NULL); // This will keep our player volumes and playback states in sync.
    stereoBufferPlayback = (float *)memalign(16, (bufferSize + 16) * sizeof(float) * 2);
    stereoBufferRecording = (float *)memalign(16, (bufferSize + 16) * sizeof(float) * 2);
}

void AudioEngine::init(int numberOfChannels, int playersCount, bool loop, int mainPlayerIndex) {
    initialized = true;
    this->numberOfChannels = numberOfChannels;
    this->playersCount = playersCount;
    this->mainPlayerIndex = mainPlayerIndex;

    players = new PlayerWrapper *[MAX_PLAYERS_COUNT];
    if (playersCount == 0) {
        prepared = true;
        notifyPlayersPrepared();
    }
}

AudioEngine::~AudioEngine() {
    reset();
    if (audioSystem != NULL) {
        audioSystem->stop();
        delete audioSystem;
        audioSystem = NULL;
    }
    freePlayersMemory(players, preparedPlayersCount);
    players = NULL;

    if (recorder != NULL) {
        delete recorder;
        recorder = NULL;
    }
    free(stereoBufferPlayback);
    free(stereoBufferRecording);

    // DeleteGlobalRef
    JNIEnv * env = getEnv();
    if (env != NULL) {
        if (g_jniCallbackInstance != NULL) {
            env->DeleteGlobalRef(g_jniCallbackInstance);
            g_jniCallbackInstance = NULL;
        }
        if (g_jniCallbackClazz != NULL) {
            env->DeleteGlobalRef(g_jniCallbackClazz);
            g_jniCallbackClazz = NULL;
        }
    }

    pthread_mutex_destroy(&mutex);

    LOGI("DESTROYED");
}

bool AudioEngine::isPrepared() const {
    return prepared;
}

void AudioEngine::preparePlayer(const char *path, int fileOffset, int fileSize) {
    SuperpoweredAdvancedAudioPlayer *player;
    void **params = (void **) malloc(sizeof(AudioEngine) + sizeof(PlayerWrapper));
    params[0] = static_cast<void *>(this);

    PlayerWrapper *playerWrapper = new PlayerWrapper();

    params[1] = static_cast<void *>(playerWrapper);

    player =
            new SuperpoweredAdvancedAudioPlayer(params,
                                                playerEventCallback,
                                                (unsigned int) sampleRate,
                                                0);
    playerWrapper->player = player;
    playerWrapper->index = playerIndexCounter;
    players[playerIndexCounter++] = playerWrapper;

    player->open(path, fileOffset, fileSize);

    player->syncMode = SuperpoweredAdvancedAudioPlayerSyncMode_TempoAndBeat;
}

void AudioEngine::startRecording(const char *tempPath, const char *destinationPath) {
    LOGI("startRecording");
    if (!isReady()) {
        return;
    }
    tempRecorderPath = tempPath;
    destinationRecorderPath = destinationPath;
    if (audioSystem == NULL) {
        LOGI("audio system NULL");
        audioSystem =
                new SuperpoweredAndroidAudioIO(
                        sampleRate,
                        bufferSize,
                        true,
                        true,
                        audioProcessing,
                        this,
                        SL_ANDROID_RECORDING_PRESET_GENERIC,
                        SL_ANDROID_STREAM_MEDIA,
                        bufferSize * 2);
    } else {
        audioSystem->start();
    }
    LOGI("start recording %s |\n %s", tempRecorderPath, destinationRecorderPath);
    if (!isReady()) {
        return;
    }
    recorder = new SuperpoweredRecorder(tempRecorderPath,
                                        (unsigned int) sampleRate,
                                        1,
                                        (unsigned char) numberOfChannels,
                                        false,
                                        onRecorderFlushedData, this);
    recorder->start(destinationRecorderPath);
    recording = true;
    setPlay(true);
}

void AudioEngine::stopRecording() {
    if (recording && recorder != NULL) {
        LOGI("stop recording");
        recorder->stop();
        recording = false;
        setPlay(false);
    }
}

void AudioEngine::startPlaying(bool fromBeginning) {
    LOGI("startPlaying");
    if (!isReady()) {
        return;
    }
    if (audioSystem == NULL) {
        LOGI("audio system NULL");
        audioSystem =
                new SuperpoweredAndroidAudioIO(
                        sampleRate,
                        bufferSize,
                        true,
                        true,
                        audioProcessing,
                        this,
                        SL_ANDROID_RECORDING_PRESET_GENERIC,
                        SL_ANDROID_STREAM_MEDIA,
                        bufferSize * 2);
    } else {
        audioSystem->start();
    }
    if (fromBeginning) {
        for (int i = 0; i < preparedPlayersCount; i++) {
            players[i]->player->setPosition(0, true, false);
        }
    }
    setPlay(true);
    playing = true;
    recording = false;
}

void AudioEngine::setPlay(bool shouldPlay) {
    for (int i = 0; i < preparedPlayersCount; i++) {
        if (shouldPlay) {
            players[i]->player->play(false);
        } else{
            players[i]->player->pause();
        }
    }
    playing = shouldPlay;
    SuperpoweredCPU::setSustainedPerformanceMode(shouldPlay); // <-- Important to prevent audio dropouts.
}

bool AudioEngine::process(short int *audioIO, unsigned int numberOfSamples) {

    bool silence = preparedPlayersCount > 0;
    for (int i = 0; i < preparedPlayersCount; i++) {
        bool processed = players[i]->player->process(stereoBufferPlayback, !silence, numberOfSamples, players[i]->volume);
        if (processed) {
            silence = false;
        }
    }

    if (recording) {
        if (silence) {
            recorder->process(NULL, numberOfSamples);
        } else {
            SuperpoweredShortIntToFloat(audioIO, stereoBufferRecording, numberOfSamples);
            recorder->process(stereoBufferRecording, NULL, numberOfSamples);
        }
    }

    if (preparedPlayersCount > 0 && !silence) {
        // write playback buffer to io stream audio
        SuperpoweredFloatToShortInt(stereoBufferPlayback, audioIO, numberOfSamples);
    }
    return preparedPlayersCount > 0 && !silence;
}

// -------------------- PRIVATE ---------------------------------------

void AudioEngine::onPlayerStateChangedPrepared(PlayerWrapper *playerWrapper, SuperpoweredAdvancedAudioPlayerEvent state) {
    LOGI("player prepared: %d", playerWrapper->index);
    if (state == SuperpoweredAdvancedAudioPlayerEvent_LoadSuccess) {
        pthread_mutex_lock(&mutex);
        if (++preparedPlayersCount == playersCount) {
            prepared = true;
            // call onPreparedSuccess!
            notifyPlayersPrepared();
        }
        pthread_mutex_unlock(&mutex);
    } else if (state == SuperpoweredAdvancedAudioPlayerEvent_LoadError) {
        LOGI("error player prepare: %d", playerWrapper->index);
        notifyError(ERROR_PLAYER_PREPARE);
    } else if (state == SuperpoweredAdvancedAudioPlayerEvent_EOF) {
        if (!loop && recording && mainPlayerIndex == playerWrapper->index) {
            stopRecording();
        }
        if (loop && mainPlayerIndex == playerWrapper->index) {
            startPlaying(true);
        }
        LOGI("end of player: %d", playerWrapper->index);
        notifyPlayerEnded(playerWrapper->index);
    }
}

void AudioEngine::notifyPlayersPrepared() {
    JNIEnv *env = getEnv();
    if (env != NULL && g_jniCallbackInstance != NULL && jniMethodOnPlayersPrepared != NULL) {
        env->CallVoidMethod(g_jniCallbackInstance, jniMethodOnPlayersPrepared);
    }
    detachAfterCallbackDone();
}

void AudioEngine::notifyError(int errorCode) {
    JNIEnv *env = getEnv();
    if (env != NULL && g_jniCallbackInstance != NULL && jniMethodOnError != NULL) {
        env->CallVoidMethod(g_jniCallbackInstance, jniMethodOnError, errorCode);
    }
    detachAfterCallbackDone();
}

void AudioEngine::notifyPlayerEnded(int index) {
    JNIEnv *env = getEnv();
    if (env != NULL && g_jniCallbackInstance != NULL && jniMethodOnPlayerEnded != NULL) {
        env->CallVoidMethod(g_jniCallbackInstance, jniMethodOnPlayerEnded, index);
    }
    detachAfterCallbackDone();
}

void AudioEngine::notifyRecordFinished() {
    JNIEnv *env = getEnv();
    if (env != NULL && g_jniCallbackInstance != NULL && jniMethodOnRecordFinished != NULL) {
        env->CallVoidMethod(g_jniCallbackInstance, jniMethodOnRecordFinished);
    }
    detachAfterCallbackDone();
}

void AudioEngine::reset() {
    LOGI("reset called!");
    if (audioSystem != NULL) {
        audioSystem->stop();
    }
    initialized = false;
    prepared = false;
    setPlay(false);
    stopRecording();
    playing = false;
    recording = false;
    // change or reset players
    freePlayersMemory(players, this->preparedPlayersCount);
    players = NULL;

    if (recorder != NULL) {
        delete recorder;
        recorder = NULL;
    }

    playersCount = 0;
    preparedPlayersCount = 0;
    playerIndexCounter = 0;
}

bool AudioEngine::isReady() {
    if (!initialized) {
        notifyError(ERROR_ENGINE_NOT_INITIALIZED);
        return false;
    }
    if (!prepared) {
        notifyError(ERROR_ENGINE_NOT_PREPARED);
        return false;
    }
    return true;
}

// ------------------------------------ JNI ------------------------------------

static AudioEngine *sEngine = NULL;

extern "C"
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) {
    LOGI("onLoad");
    JNIEnv *env;

    javaVM = vm;
    if (vm->GetEnv((void**)&env, JNI_VERSION_1_6) != JNI_OK) {
        return JNI_ERR; // JNI version not supported.
    }

    return  JNI_VERSION_1_6;
}


extern "C"
JNIEXPORT void Java_com_delicacyset_superpowered_AudioEngine_AudioEngine__II(JNIEnv *javaEnvironment,
                                                                             jobject self,
                                                                             jint sampleRate,
                                                                             jint bufferSize,
                                                                             jboolean stereo) {
    g_jniCallbackInstance = javaEnvironment->NewGlobalRef(self);
    sEngine = new AudioEngine(sampleRate, bufferSize);
}

extern "C"
JNIEXPORT void Java_com_delicacyset_superpowered_AudioEngine_releaseNative(JNIEnv *javaEnvironment,
                                                                           jobject self) {
    if (NULL != sEngine) {
        delete sEngine;
        sEngine = NULL;
    }
}

extern "C"
JNIEXPORT void Java_com_delicacyset_superpowered_AudioEngine_initNative__II(JNIEnv *javaEnvironment,
                                                                            jobject self,
                                                                            jint numberOfChannels,
                                                                            jint playersCount,
                                                                            jboolean loop,
                                                                            jint mainPlayerIndex) {
    sEngine->init(numberOfChannels, playersCount, loop, mainPlayerIndex);
}

extern "C"
JNIEXPORT void Java_com_delicacyset_superpowered_AudioEngine_preparePlayer__Ljava_lang_String_2II(JNIEnv *javaEnvironment,
                                                                                                  jobject self,
                                                                                                  jstring path,
                                                                                                  jint fileOffset,
                                                                                                  jint fileSize) {
    const char *pathC = javaEnvironment->GetStringUTFChars(path, JNI_FALSE);
    LOGI("initPlayer: %s | %i | %i", pathC, fileOffset, fileSize);
    sEngine->preparePlayer(pathC, fileOffset, fileSize);
    javaEnvironment->ReleaseStringUTFChars(path, pathC);
}

extern "C"
JNIEXPORT void Java_com_delicacyset_superpowered_AudioEngine_startPlayingNative(JNIEnv *javaEnvironment,
                                                                                jobject self,
                                                                                jboolean fromBeginning) {
    sEngine->startPlaying(fromBeginning);
}

extern "C"
JNIEXPORT void Java_com_delicacyset_superpowered_AudioEngine_setPlayNative(JNIEnv *javaEnvironment,
                                                                           jobject self,
                                                                           jboolean shouldPlay) {
    sEngine->setPlay(shouldPlay);
}

extern "C"
JNIEXPORT void Java_com_delicacyset_superpowered_AudioEngine_startRecordingNative(JNIEnv *javaEnvironment,
                                                                                  jobject self,
                                                                                  jstring tempPath,
                                                                                  jstring path) {
    const char *tempPathC = javaEnvironment->GetStringUTFChars(tempPath, JNI_FALSE);
    const char *destinationPathC = javaEnvironment->GetStringUTFChars(path, JNI_FALSE);

    sEngine->startRecording(tempPathC, destinationPathC);

    javaEnvironment->ReleaseStringUTFChars(tempPath, tempPathC);
    javaEnvironment->ReleaseStringUTFChars(path, destinationPathC);
}

extern "C"
JNIEXPORT void Java_com_delicacyset_superpowered_AudioEngine_stopRecordingNative(JNIEnv *javaEnvironment,
                                                                                 jobject self) {
    sEngine->stopRecording();
}

extern "C"
JNIEXPORT void Java_com_delicacyset_superpowered_AudioEngine_resetNative(JNIEnv *javaEnvironment,
                                                                         jobject self) {
    sEngine->reset();
}

extern "C"
JNIEXPORT void Java_com_delicacyset_superpowered_AudioEngine_initNative(JNIEnv *javaEnvironment,
                                                                        jobject self,
                                                                        int numberOfChannels,
                                                                        int playersCount,
                                                                        jboolean loop,
                                                                        int mainPlayerIndex) {
    sEngine->init(numberOfChannels, playersCount, loop, mainPlayerIndex);
}

extern "C"
JNIEXPORT jboolean Java_com_delicacyset_superpowered_AudioEngine_isPrepared(JNIEnv *javaEnvironment,
                                                                            jobject self) {
    return (jboolean) sEngine->isPrepared();
}
