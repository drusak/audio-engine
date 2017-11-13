# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.4.1)

set(
	PATH_TO_SUPERPOWERED
	CACHE STRING ""
)

message(${ANDROID_ABI})

file(GLOB CPP_FILES "*.cpp")

# --------------- Recorder V2 -----------------------------------------------

add_library( AudioEngine SHARED
             src/main/cpp/AudioEngine.cpp
             ${PATH_TO_SUPERPOWERED}/AndroidIO/SuperpoweredAndroidAudioIO.cpp
)

include_directories(src/main/jni)
include_directories(${PATH_TO_SUPERPOWERED})

target_link_libraries(
                       AudioEngine
                       log
                       android
                       OpenSLES
                       ${PATH_TO_SUPERPOWERED}/libSuperpoweredAndroid${ANDROID_ABI}.a
)