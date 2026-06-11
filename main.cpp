#define MINIAUDIO_IMPLEMENTATION
#define SDL_MAIN_HANDLED
#include "miniaudio.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <cstdlib>
#include <ctime>

#include "config.h"
#include "audio.h"
#include "avatar.h"
#include "gui.h"

namespace fs = std::filesystem;

int main() {
    std::srand(std::time(0));
    
#ifndef _WIN32
    try {
        fs::path exePath = fs::canonical("/proc/self/exe").parent_path();
        fs::current_path(exePath);
    } catch (const std::exception& e) {
        std::cerr << "Path normalization error: " << e.what() << std::endl;
    }
#endif

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "Failed to initialize SDL2: " << SDL_GetError() << std::endl;
        return 1;
    }
    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
        std::cerr << "Failed to initialize SDL2_image: " << IMG_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    loadGlobalSettings();
    scanPresets();

    if (!initAudio()) {
        std::cerr << "Audio initialization failed." << std::endl;
    } else {
        switchMicrophoneByName(globalSettings.activeMicName);
    }

    if (!initGUI()) {
        std::cerr << "GUI initialization failed." << std::endl;
        cleanupAudio();
        SDL_Quit();
        return 1;
    }

    applyAvatarPreset(globalSettings.currentPreset);

    bool running = true;
    SDL_Event event;

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }
            
            processGUIEvent(&event);
            
            if (event.type == SDL_WINDOWEVENT) {
                if (event.window.windowID == getAvatarWindowID()) {
                    if (event.window.event == SDL_WINDOWEVENT_MOVED) {
                        updateAvatarWindowPosition();
                    }
                } else if (event.window.windowID == getControlWindowID()) {
                    if (event.window.event == SDL_WINDOWEVENT_MOVED || event.window.event == SDL_WINDOWEVENT_RESIZED) {
                        updateControlWindowPosition();
                    }
                }
            }
        }

        renderAvatar();
        renderGUI();

        SDL_Delay(10);
    }

    updateControlWindowPosition();
    updateAvatarWindowPosition();
    saveGlobalSettings();
    savePresetSettings(globalSettings.currentPreset);

    cleanupGUI();
    cleanupAvatar();
    cleanupAudio();
    
    IMG_Quit();
    SDL_Quit();
    
    return 0;
}