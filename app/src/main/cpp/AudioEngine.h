//
// Created by Dmitry R. on 9/26/17.
//

#ifndef AUDIO_AUDIORECORDER_H
#define AUDIO_AUDIORECORDER_H

#include <jni.h>
#include <pthread.h>

#include "SuperpoweredAdvancedAudioPlayer.h"
#include <AndroidIO/SuperpoweredAndroidAudioIO.h>
#include "SuperpoweredRecorder.h"

#define MAX_PLAYERS_COUNT 5

#define ERROR_GENERIC 0
#define ERROR_PLAYER_PREPARE 1
#define ERROR_ENGINE_NOT_INITIALIZED 2
#define ERROR_ENGINE_NOT_PREPARED 3

struct PlayerWrapper {
    SuperpoweredAdvancedAudioPlayer *player = NULL;
    int index;
    float volume = 1.f;
};

class AudioEngine {
public:

    AudioEngine(int sampleRate, int bufferSize);

    virtual ~AudioEngine();

    void init(int numberOfChannels, int playersCount, bool loop, int mainPlayerIndex);

    void preparePlayer(const char *path, int fileOffset, int fileSize);

    void startRecording(const char *tempPath, const char *destinationPath);
    void stopRecording();

    void startPlaying(bool fromBeginning);

    void setPlay(bool shouldPlay);

    bool process(short int *audioIO, unsigned int numberOfSamples);

    void onPlayerStateChangedPrepared(PlayerWrapper *playerWrapper, SuperpoweredAdvancedAudioPlayerEvent state);

    void reset();

    bool isPrepared() const;

    void notifyRecordFinished();


private:

    pthread_mutex_t mutex;
    SuperpoweredAndroidAudioIO *audioSystem = NULL;
    PlayerWrapper **players = NULL;
    SuperpoweredRecorder *recorder = NULL;
    float *stereoBufferPlayback = NULL;
    float *stereoBufferRecording = NULL;
    int sampleRate, bufferSize;

    bool initialized = false;
    bool prepared = false;
    bool recording = false;
    bool playing = false;
    int playersCount = 0;
    int preparedPlayersCount = 0;
    int playerIndexCounter = 0;
    int numberOfChannels = 2;
    int mainPlayerIndex = 0;
    bool loop;

    const char *tempRecorderPath;
    const char *destinationRecorderPath;

    void notifyPlayersPrepared();
    void notifyError(int errorCode);
    void notifyPlayerEnded(int index);

    bool isReady();
};


#endif //AUDIO_AUDIORECORDER_H
