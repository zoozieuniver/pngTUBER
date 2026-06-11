#include "audio.h"
#include "config.h"
#include <SDL2/SDL.h>
#include <cmath>
#include <algorithm>
#include <iostream>

std::vector<AudioDevice> deviceList;
float currentVolume = 0.0f;
bool isTalking = false;

static ma_context context;
static ma_device device;
static bool isMicInitialized = false;
static Uint32 lastSoundTime = 0;
const int mouthDelay = 150;

static void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
    const float* samples = (const float*)pInput;
    float maxAmp = 0.0f;
    for (ma_uint32 i = 0; i < frameCount; i++) {
        float val = std::abs(samples[i]);
        if (val > maxAmp) {
            maxAmp = val;
        }
    }
    currentVolume = maxAmp;
    if (currentVolume > currentSettings.threshold) {
        isTalking = true;
        lastSoundTime = SDL_GetTicks();
    } else if (SDL_GetTicks() - lastSoundTime > mouthDelay) {
        isTalking = false;
    }
}

bool initAudio() {
    if (ma_context_init(NULL, 0, NULL, &context) != MA_SUCCESS) {
        std::cerr << "Failed to initialize miniaudio context.\n";
        return false;
    }
    
    ma_device_info* pInfos;
    ma_uint32 count;
    deviceList.clear();
    if (ma_context_get_devices(&context, NULL, NULL, &pInfos, &count) == MA_SUCCESS) {
        for (ma_uint32 i = 0; i < count; i++) {
            deviceList.push_back({pInfos[i].name, pInfos[i].id});
        }
    }
    return true;
}

void cleanupAudio() {
    if (isMicInitialized) {
        ma_device_uninit(&device);
        isMicInitialized = false;
    }
    ma_context_uninit(&context);
}

void switchMicrophone(int index) {
    if (index < 0 || index >= (int)deviceList.size()) return;
    if (isMicInitialized) {
        ma_device_uninit(&device);
        isMicInitialized = false;
    }
    ma_device_config config = ma_device_config_init(ma_device_type_capture);
    config.capture.pDeviceID = &deviceList[index].id;
    config.capture.format = ma_format_f32;
    config.capture.channels = 1;
    config.sampleRate = 44100;
    config.dataCallback = data_callback;
    
    if (ma_device_init(&context, &config, &device) == MA_SUCCESS) {
        ma_device_start(&device);
        globalSettings.activeMicName = deviceList[index].name;
        isMicInitialized = true;
    } else {
        std::cerr << "Failed to initialize device: " << deviceList[index].name << std::endl;
    }
}

void switchMicrophoneByName(const std::string& name) {
    int foundIndex = -1;
    for (int i = 0; i < (int)deviceList.size(); ++i) {
        if (deviceList[i].name == name) {
            foundIndex = i;
            break;
        }
    }
    if (foundIndex != -1) {
        switchMicrophone(foundIndex);
    } else if (!deviceList.empty()) {
        switchMicrophone(0);
    }
}
