#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <iostream>
#include <string>
#include <algorithm>

// --- Глобальні змінні ---
bool isTalking = false;
float threshold = 0.05f; 

// --- Функція обробки звуку ---
void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
    const float* samples = (const float*)pInput;
    float maxAmplitude = 0.0f;
    for (ma_uint32 i = 0; i < frameCount; i++) {
        float absSample = std::abs(samples[i]);
        if (absSample > maxAmplitude) maxAmplitude = absSample;
    }
    isTalking = (maxAmplitude > threshold);
    (void)pOutput; 
}

// --- Функція завантаження та масштабування ---
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
    // 1. Вибір мікрофона
    ma_context context;
    ma_context_init(NULL, 0, NULL, &context);
    ma_device_info* pCaptureDeviceInfos;
    ma_uint32 captureDeviceCount;
    ma_context_get_devices(&context, NULL, NULL, &pCaptureDeviceInfos, &captureDeviceCount);

    std::cout << "--- Оберіть мікрофон ---" << std::endl;
    for (ma_uint32 i = 0; i < captureDeviceCount; i++) std::cout << i << ": " << pCaptureDeviceInfos[i].name << std::endl;
    int choice; std::cin >> choice;

    ma_device_config deviceConfig = ma_device_config_init(ma_device_type_capture);
    deviceConfig.capture.pDeviceID = &pCaptureDeviceInfos[choice].id;
    deviceConfig.capture.format = ma_format_f32;
    deviceConfig.capture.channels = 1;
    deviceConfig.sampleRate = 44100;
    deviceConfig.dataCallback = data_callback;

    ma_device device;
    ma_device_init(&context, &deviceConfig, &device);
    ma_device_start(&device);

    // 2. SDL2 та Графіка
    SDL_Init(SDL_INIT_VIDEO);
    IMG_Init(IMG_INIT_PNG);

    SDL_Window* window = SDL_CreateWindow("PNGTuber v1.0", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1000, 1000, SDL_WINDOW_SHOWN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    SDL_Rect idleRect, talkRect;
    SDL_Texture* idleTex = loadTexture("idle.png", renderer, idleRect);
    SDL_Texture* talkTex = loadTexture("talk.png", renderer, talkRect);

    // 3. Головний цикл
    bool running = true;
    SDL_Event event;
    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = false;
            if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_UP) threshold += 0.01f;
                if (event.key.keysym.sym == SDLK_DOWN) threshold -= 0.01f;
                std::cout << "Поріг: " << threshold << std::endl;
            }
        }

        SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255); // Зеленка 🟢
        SDL_RenderClear(renderer);

        if (isTalking) SDL_RenderCopy(renderer, talkTex, NULL, &talkRect);
        else SDL_RenderCopy(renderer, idleTex, NULL, &idleRect);

        SDL_RenderPresent(renderer);
        SDL_Delay(10);
    }

    // 4. Очищення
    SDL_DestroyTexture(idleTex);
    SDL_DestroyTexture(talkTex);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    ma_device_uninit(&device);
    ma_context_uninit(&context);
    SDL_Quit();
    return 0;
}

