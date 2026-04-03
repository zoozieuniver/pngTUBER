#define MINIAUDIO_IMPLEMENTATION
#define SDL_MAIN_HANDLED
#include "miniaudio.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <filesystem>
#include <cmath>
#include <algorithm>
#include <ctime>

namespace fs = std::filesystem;

enum AppState { STATE_MAIN_MENU, STATE_MIC_SELECTION, STATE_THRESHOLD, STATE_SHAKE_ADJUST, STATE_CHARACTER_SELECT };
AppState currentState = STATE_MAIN_MENU;

struct AudioDevice { std::string name; ma_device_id id; };
struct PresetSettings {
    int x = SDL_WINDOWPOS_CENTERED, y = SDL_WINDOWPOS_CENTERED;
    int w = 400, h = 400;
    int shake = 5;
};

// Глобальні змінні
std::string currentPreset = "default";
std::vector<std::string> presetList;
std::vector<AudioDevice> deviceList;
std::string activeMicName = "None";
int selectedIndex = 0;
int viewStart = 0;
float threshold = 0.05f;
float currentVolume = 0.0f;
bool isTalking = false;
Uint32 lastSoundTime = 0;
const int mouthDelay = 150;
float offsetX = 0, offsetY = 0;

SDL_Window* window = nullptr;
SDL_Renderer* renderer = nullptr;
SDL_Texture *idleTex = nullptr, *talkTex = nullptr;
PresetSettings currentSettings;

ma_context context;
ma_device device;
bool isMicInitialized = false;

void clearScreen() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

void saveSettings(const std::string& name, int x, int y, int w, int h, int shake) {
    std::ofstream out("presets/" + name + "/settings.txt");
    out << x << " " << y << " " << w << " " << h << " " << shake << " " << threshold;
}

void loadSettings(const std::string& name) {
    std::ifstream in("presets/" + name + "/settings.txt");
    float savedThreshold = 0.05f;
    if (in >> currentSettings.x >> currentSettings.y >> currentSettings.w >> currentSettings.h >> currentSettings.shake >> savedThreshold) {
        threshold = savedThreshold;
    }
}

void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
    const float* samples = (const float*)pInput;
    float maxAmp = 0;
    for (ma_uint32 i = 0; i < frameCount; i++) if (std::abs(samples[i]) > maxAmp) maxAmp = std::abs(samples[i]);
    currentVolume = maxAmp;
    if (currentVolume > threshold) { isTalking = true; lastSoundTime = SDL_GetTicks(); }
    else if (SDL_GetTicks() - lastSoundTime > mouthDelay) isTalking = false;
}

void switchMicrophone(int index) {
    if (index < 0 || index >= (int)deviceList.size()) return;
    if (isMicInitialized) ma_device_uninit(&device);
    ma_device_config config = ma_device_config_init(ma_device_type_capture);
    config.capture.pDeviceID = &deviceList[index].id;
    config.capture.format = ma_format_f32; config.capture.channels = 1; config.sampleRate = 44100;
    config.dataCallback = data_callback;
    if (ma_device_init(&context, &config, &device) == MA_SUCCESS) {
        ma_device_start(&device);
        activeMicName = deviceList[index].name;
        isMicInitialized = true;
    }
}

void applyPreset(std::string name) {
    if (window) {
        int x, y, w, h; SDL_GetWindowPosition(window, &x, &y); SDL_GetWindowSize(window, &w, &h);
        saveSettings(currentPreset, x, y, w, h, currentSettings.shake);
    }
    if (idleTex) SDL_DestroyTexture(idleTex); if (talkTex) SDL_DestroyTexture(talkTex);
    if (renderer) SDL_DestroyRenderer(renderer); if (window) SDL_DestroyWindow(window);

    currentPreset = name;
    loadSettings(name);
    window = SDL_CreateWindow(name.c_str(), currentSettings.x, currentSettings.y, currentSettings.w, currentSettings.h, SDL_WINDOW_BORDERLESS);
    
    SDL_Surface* iconSurf = IMG_Load("assets/icons/app_icon.png");
    if (iconSurf) { SDL_SetWindowIcon(window, iconSurf); SDL_FreeSurface(iconSurf); }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    SDL_SetWindowHitTest(window, [](SDL_Window*, const SDL_Point*, void*) { return SDL_HITTEST_DRAGGABLE; }, NULL);
    idleTex = IMG_LoadTexture(renderer, ("presets/" + name + "/idle.png").c_str());
    talkTex = IMG_LoadTexture(renderer, ("presets/" + name + "/talk.png").c_str());
}

void drawVolumeBar(float currentVol, float thresholdVal) {
    int width = 20; int v = (int)(currentVol * width * 2.5f); int t = (int)(thresholdVal * width * 2.5f);
    std::cout << "Vol: [";
    for (int i = 0; i < width; ++i) {
        if (i < v) std::cout << "▮"; else if (i == t) std::cout << "|"; else std::cout << "-";
    }
    std::cout << "] " << (int)(currentVol * 100) << "%";
}

void drawTerminalUI() {
    clearScreen();
    std::cout << "=== PNGTuber Control Panel ===\n\n";
    if (currentState == STATE_MAIN_MENU) {
        std::cout << (selectedIndex == 0 ? " -> " : "    ") << "1. Select Microphone (" << activeMicName << ")\n";
        std::cout << (selectedIndex == 1 ? " -> " : "    ") << "2. Adjust Threshold (" << threshold << ")\n";
        std::cout << (selectedIndex == 2 ? " -> " : "    ") << "3. Adjust Shake Intensity (" << currentSettings.shake << ")\n";
        std::cout << (selectedIndex == 3 ? " -> " : "    ") << "4. Character & Scale Settings\n";
        std::cout << (selectedIndex == 4 ? " -> " : "    ") << "5. Exit\n";
    } else if (currentState == STATE_CHARACTER_SELECT) {
        std::cout << "=== Scale & Presets ===\n";
        std::cout << (selectedIndex == 0 ? " -> " : "    ") << "[Scale +10%] (Size: " << currentSettings.w << "x" << currentSettings.h << ")\n";
        std::cout << (selectedIndex == 1 ? " -> " : "    ") << "[Scale -10%]\n";
        std::cout << (selectedIndex == 2 ? " -> " : "    ") << "[Back to Main Menu]\n\n";
        for (int i = 0; i < (int)presetList.size(); i++)
            std::cout << (selectedIndex == (i + 3) ? " -> " : "    ") << "Load: " << presetList[i] << (presetList[i] == currentPreset ? " (Active)" : "") << "\n";
    } else if (currentState == STATE_THRESHOLD) {
        std::cout << "Adjust Threshold (Up/Down):\n"; drawVolumeBar(currentVolume, threshold);
        std::cout << "\n\n[Enter/Esc] Back";
    } else if (currentState == STATE_SHAKE_ADJUST) {
        std::cout << "Shake Intensity: " << currentSettings.shake << "\n[ ";
        for(int i=0; i<20; i++) std::cout << (i < currentSettings.shake ? "▮" : "-");
        std::cout << " ]\n\n[Enter/Esc] Back";
    } else if (currentState == STATE_MIC_SELECTION) {
        int endView = std::min(viewStart + 5, (int)deviceList.size());
        for (int i = viewStart; i < endView; ++i)
            std::cout << (i == selectedIndex ? " -> " : "    ") << i + 1 << ". " << deviceList[i].name << (deviceList[i].name == activeMicName ? " (*)" : "") << "\n";
    }
}

int main() {
    srand(time(0)); 
    SDL_Init(SDL_INIT_VIDEO); 
    IMG_Init(IMG_INIT_PNG);
    
    // --- ПРИМУСОВЕ ВСТАНОВЛЕННЯ РОБОЧОЇ ДИРЕКТОРІЇ (для Linux/Unix) ---
#ifndef _WIN32
    try {
        // Отримуємо шлях до самого виконуваного файлу і переходимо в його папку
        fs::path exePath = fs::canonical("/proc/self/exe").parent_path();
        fs::current_path(exePath);
    } catch (const std::exception& e) {
        std::cerr << "Could not set working directory: " << e.what() << std::endl;
    }
#endif

    // Ініціалізація Аудіо
    ma_context_init(NULL, 0, NULL, &context);
    ma_device_info* pInfos; ma_uint32 count;
    if (ma_context_get_devices(&context, NULL, NULL, &pInfos, &count) == MA_SUCCESS) {
        for (ma_uint32 i = 0; i < count; i++) {
            deviceList.push_back({pInfos[i].name, pInfos[i].id});
        }
    }

    // --- ДІАГНОСТИКА ТА ПОШУК ПРЕСЕТІВ ---
    std::cout << "--- System Info ---" << std::endl;
    std::cout << "Current execution path: " << fs::current_path() << std::endl;

    if (fs::exists("presets")) {
        std::cout << "Presets folder: FOUND" << std::endl;
        for (const auto& entry : fs::directory_iterator("presets")) {
            if (entry.is_directory()) {
                std::string pName = entry.path().filename().string();
                presetList.push_back(pName);
                std::cout << "  > Loaded preset: " << pName << std::endl;
            }
        }
    } else {
        std::cout << "[ERROR] Presets folder NOT FOUND at: " << fs::current_path() << std::endl;
        std::cout << "Please ensure 'presets' folder is in the same directory as the app." << std::endl;
    }
    std::cout << "-------------------\n" << std::endl;

    // Початкове завантаження
    applyPreset(presetList.empty() ? "default" : presetList[0]);
    if (!deviceList.empty()) switchMicrophone(0);

    bool running = true; SDL_Event ev;
    drawTerminalUI();

    while (running) {
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT) running = false;
            if (ev.type == SDL_KEYDOWN) {
                // ОБРОБКА ЦИФРОВИХ ЗНАЧЕНЬ (Поріг / Тряска)
                if (currentState == STATE_THRESHOLD) {
                    if (ev.key.keysym.sym == SDLK_UP) threshold = std::min(1.0f, threshold + 0.01f);
                    else if (ev.key.keysym.sym == SDLK_DOWN) threshold = std::max(0.0f, threshold - 0.01f);
                    else if (ev.key.keysym.sym == SDLK_RETURN || ev.key.keysym.sym == SDLK_ESCAPE) currentState = STATE_MAIN_MENU;
                }
                else if (currentState == STATE_SHAKE_ADJUST) {
                    if (ev.key.keysym.sym == SDLK_UP) currentSettings.shake = std::min(20, currentSettings.shake + 1);
                    else if (ev.key.keysym.sym == SDLK_DOWN) currentSettings.shake = std::max(0, currentSettings.shake - 1);
                    else if (ev.key.keysym.sym == SDLK_RETURN || ev.key.keysym.sym == SDLK_ESCAPE) currentState = STATE_MAIN_MENU;
                }
                // ОБРОБКА НАВІГАЦІЇ
                else {
                    int totalOptions = 5; 
                    if (currentState == STATE_CHARACTER_SELECT) totalOptions = 3 + (int)presetList.size();
                    else if (currentState == STATE_MIC_SELECTION) totalOptions = (int)deviceList.size();

                    if (ev.key.keysym.sym == SDLK_DOWN) selectedIndex = (selectedIndex + 1) % totalOptions;
                    else if (ev.key.keysym.sym == SDLK_UP) selectedIndex = (selectedIndex + totalOptions - 1) % totalOptions;
                    else if (ev.key.keysym.sym == SDLK_RETURN) {
                        if (currentState == STATE_MAIN_MENU) {
                            if (selectedIndex == 0) { currentState = STATE_MIC_SELECTION; selectedIndex = 0; viewStart = 0; }
                            else if (selectedIndex == 1) currentState = STATE_THRESHOLD;
                            else if (selectedIndex == 2) currentState = STATE_SHAKE_ADJUST;
                            else if (selectedIndex == 3) { currentState = STATE_CHARACTER_SELECT; selectedIndex = 0; }
                            else if (selectedIndex == 4) running = false;
                        } 
                        else if (currentState == STATE_CHARACTER_SELECT) {
                            if (selectedIndex == 0) { 
                                currentSettings.w *= 1.1; currentSettings.h *= 1.1; 
                                SDL_SetWindowSize(window, currentSettings.w, currentSettings.h); 
                            }
                            else if (selectedIndex == 1) { 
                                currentSettings.w *= 0.9; currentSettings.h *= 0.9; 
                                SDL_SetWindowSize(window, currentSettings.w, currentSettings.h); 
                            }
                            else if (selectedIndex == 2) currentState = STATE_MAIN_MENU;
                            else if (selectedIndex >= 3) applyPreset(presetList[selectedIndex - 3]);
                        } 
                        else if (currentState == STATE_MIC_SELECTION) { 
                            switchMicrophone(selectedIndex); 
                            currentState = STATE_MAIN_MENU; 
                        }
                    }
                    else if (ev.key.keysym.sym == SDLK_ESCAPE) currentState = STATE_MAIN_MENU;
                }
                drawTerminalUI();
            }
        }
        
        // Логіка тряски
        if (isTalking) {
            float s = (float)currentSettings.shake;
            offsetX = (rand() % (int)(s * 2 + 1)) - s;
            offsetY = (rand() % (int)(s * 2 + 1)) - (s * 0.5f);
        } else { offsetX *= 0.8f; offsetY *= 0.8f; }

        SDL_Rect r = {(int)offsetX, (int)offsetY, currentSettings.w, currentSettings.h};
        SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255); 
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, isTalking ? talkTex : idleTex, NULL, &r);
        SDL_RenderPresent(renderer);
        
        if (currentState == STATE_THRESHOLD) drawTerminalUI();
        SDL_Delay(15);
    }
    
    saveSettings(currentPreset, currentSettings.x, currentSettings.y, currentSettings.w, currentSettings.h, currentSettings.shake);
    ma_device_uninit(&device); 
    ma_context_uninit(&context); 
    SDL_Quit();
    return 0;
}