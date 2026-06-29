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
#include "imgui.h"
#include <algorithm>
#include <cctype>

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
    // Enable transparency for EGL window surfaces on Wayland
    setenv("SDL_VIDEO_EGL_ALLOW_TRANSPARENCY", "1", 1);
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
            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE) {
                running = false;
            }
            
            processGUIEvent(&event);
            
            if (event.type == SDL_KEYDOWN) {
                ImGuiIO& io = ImGui::GetIO();
                if (!io.WantCaptureKeyboard) {
                    SDL_Keycode code = event.key.keysym.sym;
                    std::string keyChar = "";
                    if (code >= SDLK_a && code <= SDLK_z) {
                        char c = 'a' + (code - SDLK_a);
                        keyChar = std::string(1, c);
                    } else if (code >= SDLK_0 && code <= SDLK_9) {
                        char c = '0' + (code - SDLK_0);
                        keyChar = std::string(1, c);
                    } else {
                        const char* name = SDL_GetKeyName(code);
                        if (name) {
                            keyChar = name;
                            std::transform(keyChar.begin(), keyChar.end(), keyChar.begin(), ::tolower);
                        }
                    }
                    
                    if (!keyChar.empty()) {
                        for (auto& layer : currentLayers) {
                            std::string lHk = layer.hotkey;
                            std::transform(lHk.begin(), lHk.end(), lHk.begin(), ::tolower);
                            if (lHk == keyChar) {
                                layer.visible = !layer.visible;
                            }
                        }
                    }
                }
            }
            
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

    if (editorModeActive) {
        globalSettings.controlW = savedControlW;
        globalSettings.controlH = savedControlH;
    } else {
        int w, h;
        SDL_GetWindowSize(controlWindow, &w, &h);
        globalSettings.controlW = w;
        globalSettings.controlH = h;
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