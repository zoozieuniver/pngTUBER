#ifndef AUDIO_H
#define AUDIO_H

#include "miniaudio.h"
#include <string>
#include <vector>

struct AudioDevice {
    std::string name;
    ma_device_id id;
};

extern std::vector<AudioDevice> deviceList;
extern float currentVolume;
extern bool isTalking;

bool initAudio();
void cleanupAudio();
void switchMicrophone(int index);
void switchMicrophoneByName(const std::string& name);

#endif // AUDIO_H
