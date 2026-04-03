#define MINIAUDIO_IMPLEMENTATION
#define SDL_MAIN_HANDLED // Вирішує проблему з WinMain на Windows
#include "miniaudio.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <fstream>
#include <cmath>
#include <ctime>

//some_text to check if updates are actually working

// --- App State ---
enum AppState { STATE_MAIN_MENU, STATE_THRESHOLD, STATE_MIC_SELECTION, STATE_SHAKE_ADJUST };
AppState currentState = STATE_MAIN_MENU;

bool isTalking = false;
float threshold = 0.05f; 
float currentVolume = 0.0f;
int shakeIntensity = 5; // Налаштування за замовчуванням
std::string preset = "default";
std::string activeMicName = "";
bool running = true;
bool needsRefresh = true;

struct AudioDevice {
    std::string name;
    ma_device_id id;
};
std::vector<AudioDevice> deviceList;
int selectedIndex = 0;
int viewStart = 0;

ma_context context;
ma_device device;
bool isMicInitialized = false;

// --- Helper Functions ---
std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (std::string::npos == first) return "";
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, (last - first + 1));
}

void loadConfig() {
    std::ifstream file("config.txt");
    if (!file.is_open()) return;
    std::string line;
    while (std::getline(file, line)) {
        size_t sep = line.find('=');
        if (sep != std::string::npos) {
            std::string key = trim(line.substr(0, sep));
            std::string value = trim(line.substr(sep + 1));
            if (key == "threshold") threshold = std::stof(value);
            else if (key == "mic_name") activeMicName = value;
            else if (key == "shake_intensity") shakeIntensity = std::stoi(value);
            else if (key == "preset") preset = value;
        }
    }
}

void saveConfig() {
    std::ofstream file("config.txt");
    if (file.is_open()) {
        file << "threshold=" << threshold << "\n";
        file << "mic_name=" << activeMicName << "\n";
        file << "shake_intensity=" << shakeIntensity << "\n";
        file << "preset=" << preset << "\n";
    }
}

// --- Audio Handling ---
void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
    const float* samples = (const float*)pInput;
    float maxAmplitude = 0.0f;
    for (ma_uint32 i = 0; i < frameCount; i++) {
        float absSample = std::abs(samples[i]);
        if (absSample > maxAmplitude) maxAmplitude = absSample;
    }
    currentVolume = maxAmplitude;
    isTalking = (maxAmplitude > threshold);
    (void)pOutput; 
}

void switchMicrophone(int index) {
    if (isMicInitialized) ma_device_uninit(&device);
    ma_device_config deviceConfig = ma_device_config_init(ma_device_type_capture);
    deviceConfig.capture.pDeviceID = &deviceList[index].id;
    deviceConfig.capture.format = ma_format_f32;
    deviceConfig.capture.channels = 1;
    deviceConfig.sampleRate = 44100;
    deviceConfig.dataCallback = data_callback;

    if (ma_device_init(&context, &deviceConfig, &device) == MA_SUCCESS) {
        ma_device_start(&device);
        activeMicName = deviceList[index].name;
        isMicInitialized = true;
    }
}

// --- UI Rendering ---
void drawVolumeBar(float currentVol, float thresholdVal) {
    int width = 20;
    int v = (int)(currentVol * width);
    int t = (int)(thresholdVal * width);
    std::string bar = "[";
    for (int i = 0; i < width; ++i) {
        if (i < v) bar += "#";
        else if (i == t) bar += "|";
        else bar += "-";
    }
    bar += "]";
    std::cout << "\r" << bar << " " << (int)(currentVol * 100) << "%   " << std::flush;
}

void drawTerminalUI() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
    std::cout << "=== PNGTuber Control Panel ===\n\n";
    if (currentState == STATE_MAIN_MENU) {
        std::cout << "Select an option (Up/Down) and press Enter:\n\n";
        std::cout << (selectedIndex == 0 ? " -> " : "    ") << "1. Select Microphone\n";
        std::cout << (selectedIndex == 1 ? " -> " : "    ") << "2. Adjust Threshold\n";
        std::cout << (selectedIndex == 2 ? " -> " : "    ") << "3. Adjust Shake Intensity\n";
        std::cout << (selectedIndex == 3 ? " -> " : "    ") << "4. Exit\n";
        std::cout << "\n[Current Mic]: " << (activeMicName.empty() ? "None" : activeMicName) << "\n";
        std::cout << "[Active Preset]: " << preset << "\n";
    } 
    else if (currentState == STATE_SHAKE_ADJUST) {
        std::cout << "Adjusting Shake Intensity (Up/Down to change, Enter to save):\n\n";
        std::cout << "Current Intensity: " << shakeIntensity << "\n";
        std::cout << "[ " << std::string(shakeIntensity, '#') << std::string(20 - shakeIntensity, '-') << " ]\n";
    }
    else if (currentState == STATE_THRESHOLD) {
        std::cout << "Adjusting Threshold (Up/Down to change, Enter to save):\n\n";
        drawVolumeBar(currentVolume, threshold);
        std::cout << "\n\nThreshold: " << threshold << "\n";
    }
    else if (currentState == STATE_MIC_SELECTION) {
        std::cout << "Select a Microphone (Up/Down, Enter to select, Esc to cancel):\n\n";
        int maxVisible = 5;
        int totalMics = deviceList.size();
        int endView = std::min(viewStart + maxVisible, totalMics);
        for (int i = viewStart; i < endView; ++i) {
            std::cout << (i == selectedIndex ? " -> " : "    ") << i + 1 << ". " << deviceList[i].name << "\n";
        }
    }
}

// --- Texture Loading with Fallback ---
SDL_Texture* loadTexture(const std::string& path, const std::string& fallback, SDL_Renderer* renderer, SDL_Rect& destRect) {
    SDL_Texture* texture = IMG_LoadTexture(renderer, path.c_str());
    if (!texture) {
        std::cout << "[WARNING] Texture not found: " << path << ". Using fallback." << std::endl;
        texture = IMG_LoadTexture(renderer, fallback.c_str());
        if (!texture) return NULL;
    }

    int w, h;
    SDL_QueryTexture(texture, NULL, NULL, &w, &h);
    float ratio = std::min(800.0f / w, 800.0f / h);
    destRect.w = (int)(w * ratio);
    destRect.h = (int)(h * ratio);
    destRect.x = (1000 - destRect.w) / 2;
    destRect.y = (1000 - destRect.h) / 2;
    return texture;
}

int main(int argc, char* argv[]) {
    SDL_SetMainReady(); // Важливо для Windows-компіляції
    srand(time(0));
    loadConfig();

    // 1. Ініціалізація Аудіо
    ma_context_init(NULL, 0, NULL, &context);
    ma_device_info* pCaptureDeviceInfos;
    ma_uint32 captureDeviceCount;
    if (ma_context_get_devices(&context, NULL, NULL, &pCaptureDeviceInfos, &captureDeviceCount) == MA_SUCCESS) {
        for (ma_uint32 i = 0; i < captureDeviceCount; i++) {
            deviceList.push_back({pCaptureDeviceInfos[i].name, pCaptureDeviceInfos[i].id});
        }
    }
    if (!deviceList.empty()) {
        int idx = 0;
        for (int i = 0; i < deviceList.size(); i++) if (deviceList[i].name == activeMicName) idx = i;
        switchMicrophone(idx);
    }

    // 2. Ініціалізація SDL2
    SDL_Init(SDL_INIT_VIDEO);
    IMG_Init(IMG_INIT_PNG);
    SDL_Window* window = SDL_CreateWindow("PNGTuber v1.0", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1000, 1000, SDL_WINDOW_SHOWN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    // Встановлення іконки вікна
    SDL_Surface* iconSurf = IMG_Load("assets/icons/app_icon.png");
    if (iconSurf) {
        SDL_SetWindowIcon(window, iconSurf);
        SDL_FreeSurface(iconSurf);
    }

    SDL_Rect idleRect, talkRect;
    std::string idlePath = "presets/" + preset + "/idle.png";
    std::string talkPath = "presets/" + preset + "/talk.png";
    SDL_Texture* idleTex = loadTexture(idlePath, "assets/errors/error_idle.png", renderer, idleRect);
    SDL_Texture* talkTex = loadTexture(talkPath, "assets/errors/error_talk.png", renderer, talkRect);

    // 3. Головний цикл
    SDL_Event event;
    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = false;
            
            if (event.type == SDL_KEYDOWN) { // ОБРОБКА КЛАВІШ
                if (currentState == STATE_MAIN_MENU) {
                    if (event.key.keysym.sym == SDLK_UP) selectedIndex = std::max(0, selectedIndex - 1);
                    if (event.key.keysym.sym == SDLK_DOWN) selectedIndex = std::min(3, selectedIndex + 1);
                    if (event.key.keysym.sym == SDLK_RETURN) {
                        if (selectedIndex == 0) { currentState = STATE_MIC_SELECTION; selectedIndex = 0; viewStart = 0; }
                        else if (selectedIndex == 1) currentState = STATE_THRESHOLD;
                        else if (selectedIndex == 2) currentState = STATE_SHAKE_ADJUST;
                        else if (selectedIndex == 3) running = false;
                    }
                }
                else if (currentState == STATE_SHAKE_ADJUST) {
                    if (event.key.keysym.sym == SDLK_UP) shakeIntensity = std::min(20, shakeIntensity + 1);
                    if (event.key.keysym.sym == SDLK_DOWN) shakeIntensity = std::max(0, shakeIntensity - 1);
                    if (event.key.keysym.sym == SDLK_RETURN || event.key.keysym.sym == SDLK_ESCAPE) currentState = STATE_MAIN_MENU;
                }
                else if (currentState == STATE_THRESHOLD) {
                    if (event.key.keysym.sym == SDLK_UP) threshold = std::min(1.0f, threshold + 0.01f);
                    if (event.key.keysym.sym == SDLK_DOWN) threshold = std::max(0.0f, threshold - 0.01f);
                    if (event.key.keysym.sym == SDLK_RETURN || event.key.keysym.sym == SDLK_ESCAPE) currentState = STATE_MAIN_MENU;
                }
                else if (currentState == STATE_MIC_SELECTION) {
                    if (event.key.keysym.sym == SDLK_UP && selectedIndex > 0) {
                        selectedIndex--; if (selectedIndex < viewStart) viewStart--;
                    }
                    if (event.key.keysym.sym == SDLK_DOWN && selectedIndex < (int)deviceList.size() - 1) {
                        selectedIndex++; if (selectedIndex >= viewStart + 5) viewStart++;
                    }
                    if (event.key.keysym.sym == SDLK_RETURN) { switchMicrophone(selectedIndex); currentState = STATE_MAIN_MENU; }
                    if (event.key.keysym.sym == SDLK_ESCAPE) currentState = STATE_MAIN_MENU;
                }
                needsRefresh = true;
            }
        }

        if (needsRefresh || currentState == STATE_THRESHOLD) { drawTerminalUI(); needsRefresh = false; }

        // Розрахунок трясіння
        int offX = 0, offY = 0;
        if (isTalking && shakeIntensity > 0) {
            offX = (rand() % (shakeIntensity * 2 + 1)) - shakeIntensity;
            offY = (rand() % (shakeIntensity * 2 + 1)) - shakeIntensity;
        }

        SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255); // Зелений фон
        SDL_RenderClear(renderer);
        
        SDL_Rect dRect = isTalking ? talkRect : idleRect;
        dRect.x += offX; dRect.y += offY;
        
        SDL_RenderCopy(renderer, isTalking ? talkTex : idleTex, NULL, &dRect);
        SDL_RenderPresent(renderer);
        SDL_Delay(30);
    }

    // 4. Очищення
    saveConfig();
    SDL_DestroyTexture(idleTex); SDL_DestroyTexture(talkTex);
    SDL_DestroyRenderer(renderer); SDL_DestroyWindow(window);
    if (isMicInitialized) ma_device_uninit(&device);
    ma_context_uninit(&context);
    SDL_Quit();
    return 0;
}