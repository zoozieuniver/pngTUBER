#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <fstream>
#include <cmath>

// --- Глобальні змінні та Стан ---
enum AppState { STATE_MAIN_MENU, STATE_THRESHOLD, STATE_MIC_SELECTION };
AppState currentState = STATE_MAIN_MENU;

bool isTalking = false;
float threshold = 0.05f; 
float currentVolume = 0.0f;
std::string activeMicName = "";
bool running = true;
bool needsRefresh = true;

// --- Дані для мікрофонів ---
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

// --- Допоміжні функції конфігурації ---
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
        }
    }
}

void saveConfig() {
    std::ofstream file("config.txt");
    if (file.is_open()) {
        file << "threshold=" << threshold << "\n";
        file << "mic_name=" << activeMicName << "\n";
    }
}

// --- Обробка звуку ---
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
    if (isMicInitialized) {
        ma_device_uninit(&device);
    }

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

// --- Малювання інтерфейсу ---
void drawVolumeBar(float currentVol, float thresholdVal) {
    int width = 20;
    int v = (int)(currentVol * width);
    int t = (int)(thresholdVal * width);
    std::string bar = "[";
    for (int i = 0; i < width; ++i) {
        if (i < v) bar += "▮";
        else if (i == t) bar += "|";
        else bar += "-";
    }
    bar += "]";
    std::cout << "\r" << bar << " " << (int)(currentVol * 100) << "%   " << std::flush;
}

void drawTerminalUI() {
    system("cls");
    std::cout << "=== PNGTuber Control Panel ===\n\n";

    if (currentState == STATE_MAIN_MENU) {
        std::cout << "Select an option (Up/Down) and press Enter:\n\n";
        std::cout << (selectedIndex == 0 ? " -> " : "    ") << "1. Select Microphone\n";
        std::cout << (selectedIndex == 1 ? " -> " : "    ") << "2. Adjust Threshold\n";
        std::cout << (selectedIndex == 2 ? " -> " : "    ") << "3. Exit\n";
        std::cout << "\n[Current Mic]: " << (activeMicName.empty() ? "None" : activeMicName) << "\n";
    } 
    else if (currentState == STATE_THRESHOLD) {
        std::cout << "Adjusting Threshold (Up/Down to change, Enter to save):\n\n";
        drawVolumeBar(currentVolume, threshold);
        std::cout << "\n";
    }
    else if (currentState == STATE_MIC_SELECTION) {
        std::cout << "Select a Microphone (Up/Down, Enter to select, Esc to cancel):\n\n";
        int maxVisible = 5;
        int totalMics = deviceList.size();
        int endView = std::min(viewStart + maxVisible, totalMics);

        for (int i = viewStart; i < endView; ++i) {
            if (i == selectedIndex) std::cout << " -> ";
            else std::cout << "    ";
            std::cout << i + 1 << ". " << deviceList[i].name << "\n";
        }
        std::cout << "\n[Showing " << viewStart + 1 << " - " << endView << " of " << totalMics << "]\n";
    }
}

// --- Графіка SDL ---
SDL_Texture* loadTexture(const std::string& path, SDL_Renderer* renderer, SDL_Rect& destRect) {
    SDL_Surface* tempSurface = IMG_Load(path.c_str());
    if (!tempSurface) return NULL;

    float ratio = 1.0f;
    if (tempSurface->w > 1000 || tempSurface->h > 1000) {
        float ratioW = 1000.0f / tempSurface->w;
        float ratioH = 1000.0f / tempSurface->h;
        ratio = std::min(ratioW, ratioH);
    }

    destRect.w = (int)(tempSurface->w * ratio);
    destRect.h = (int)(tempSurface->h * ratio);
    destRect.x = (1000 - destRect.w) / 2;
    destRect.y = (1000 - destRect.h) / 2;

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, tempSurface);
    SDL_FreeSurface(tempSurface);
    return texture;
}

int main(int argc, char* argv[]) {
    // 1. Завантаження конфігурації та Ініціалізація Аудіо
    loadConfig();

    ma_context_init(NULL, 0, NULL, &context);
    ma_device_info* pCaptureDeviceInfos;
    ma_uint32 captureDeviceCount;
    if (ma_context_get_devices(&context, NULL, NULL, &pCaptureDeviceInfos, &captureDeviceCount) == MA_SUCCESS) {
        for (ma_uint32 i = 0; i < captureDeviceCount; i++) {
            AudioDevice dev;
            dev.name = pCaptureDeviceInfos[i].name;
            dev.id = pCaptureDeviceInfos[i].id;
            deviceList.push_back(dev);
        }
    }

    if (!deviceList.empty()) {
        int startIndex = 0;
        for (int i = 0; i < deviceList.size(); i++) {
            if (deviceList[i].name == activeMicName) {
                startIndex = i;
                break;
            }
        }
        switchMicrophone(startIndex);
    }

    // 2. Ініціалізація SDL2
    SDL_Init(SDL_INIT_VIDEO);
    IMG_Init(IMG_INIT_PNG);

    SDL_Window* window = SDL_CreateWindow("PNGTuber v1.0", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1000, 1000, SDL_WINDOW_SHOWN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    SDL_Rect idleRect, talkRect;
    SDL_Texture* idleTex = loadTexture("idle.png", renderer, idleRect);
    SDL_Texture* talkTex = loadTexture("talk.png", renderer, talkRect);

    if (!idleTex || !talkTex) {
        std::cout << "Помилка завантаження текстур!" << std::endl;
    }

    // 3. Головний цикл
    SDL_Event event;
    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = false;
            
            if (event.type == SDL_KEYDOWN) {
                if (currentState == STATE_MAIN_MENU) {
                    if (event.key.keysym.sym == SDLK_UP) selectedIndex = std::max(0, selectedIndex - 1);
                    if (event.key.keysym.sym == SDLK_DOWN) selectedIndex = std::min(2, selectedIndex + 1);
                    if (event.key.keysym.sym == SDLK_RETURN) {
                        if (selectedIndex == 0) { currentState = STATE_MIC_SELECTION; selectedIndex = 0; viewStart = 0; }
                        else if (selectedIndex == 1) { currentState = STATE_THRESHOLD; }
                        else if (selectedIndex == 2) { running = false; }
                    }
                }
                else if (currentState == STATE_THRESHOLD) {
                    if (event.key.keysym.sym == SDLK_UP) threshold = std::min(1.0f, threshold + 0.01f);
                    if (event.key.keysym.sym == SDLK_DOWN) threshold = std::max(0.0f, threshold - 0.01f);
                    if (event.key.keysym.sym == SDLK_RETURN || event.key.keysym.sym == SDLK_ESCAPE) {
                        currentState = STATE_MAIN_MENU;
                        selectedIndex = 1;
                    }
                }
                else if (currentState == STATE_MIC_SELECTION) {
                    int totalMics = deviceList.size();
                    if (event.key.keysym.sym == SDLK_UP) {
                        if (selectedIndex > 0) {
                            selectedIndex--;
                            if (selectedIndex < viewStart) viewStart--; 
                        }
                    }
                    if (event.key.keysym.sym == SDLK_DOWN) {
                        if (selectedIndex < totalMics - 1) {
                            selectedIndex++;
                            if (selectedIndex >= viewStart + 5) viewStart++; 
                        }
                    }
                    if (event.key.keysym.sym == SDLK_RETURN) {
                        switchMicrophone(selectedIndex);
                        currentState = STATE_MAIN_MENU; 
                        selectedIndex = 0; 
                    }
                    if (event.key.keysym.sym == SDLK_ESCAPE) {
                        currentState = STATE_MAIN_MENU; 
                        selectedIndex = 0;
                    }
                }
                needsRefresh = true;
            }
        }

        // Візуалізація Консолі
        if (needsRefresh || currentState == STATE_THRESHOLD) {
            drawTerminalUI();
            needsRefresh = false;
        }

        // Візуалізація SDL
        SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
        SDL_RenderClear(renderer);
        if (isTalking) SDL_RenderCopy(renderer, talkTex, NULL, &talkRect);
        else SDL_RenderCopy(renderer, idleTex, NULL, &idleRect);
        SDL_RenderPresent(renderer);

        SDL_Delay(30); // Затримка 30 мс, щоб не мерехтіла консоль і не грузився процесор
    }

    // 4. Очищення та Збереження
    saveConfig();
    SDL_DestroyTexture(idleTex);
    SDL_DestroyTexture(talkTex);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    if (isMicInitialized) ma_device_uninit(&device);
    ma_context_uninit(&context);
    SDL_Quit();
    return 0;
}