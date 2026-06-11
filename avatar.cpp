#include "avatar.h"
#include "config.h"
#include "audio.h"
#include <SDL2/SDL_image.h>
#include <iostream>
#include <cstdlib>

SDL_Window* avatarWindow = nullptr;
SDL_Renderer* avatarRenderer = nullptr;

static SDL_Texture* idleTex = nullptr;
static SDL_Texture* talkTex = nullptr;
static float offsetX = 0.0f;
static float offsetY = 0.0f;

bool initAvatar() {
    return true;
}

void cleanupAvatar() {
    if (idleTex) {
        SDL_DestroyTexture(idleTex);
        idleTex = nullptr;
    }
    if (talkTex) {
        SDL_DestroyTexture(talkTex);
        talkTex = nullptr;
    }
    if (avatarRenderer) {
        SDL_DestroyRenderer(avatarRenderer);
        avatarRenderer = nullptr;
    }
    if (avatarWindow) {
        SDL_DestroyWindow(avatarWindow);
        avatarWindow = nullptr;
    }
}

bool applyAvatarPreset(const std::string& name) {
    if (avatarWindow) {
        int x, y, w, h;
        SDL_GetWindowPosition(avatarWindow, &x, &y);
        SDL_GetWindowSize(avatarWindow, &w, &h);
        currentSettings.x = x;
        currentSettings.y = y;
        currentSettings.w = w;
        currentSettings.h = h;
        savePresetSettings(globalSettings.currentPreset);
    }
    
    cleanupAvatar();
    
    globalSettings.currentPreset = name;
    loadPresetSettings(name);
    
    avatarWindow = SDL_CreateWindow(
        name.c_str(),
        currentSettings.x,
        currentSettings.y,
        currentSettings.w,
        currentSettings.h,
        SDL_WINDOW_BORDERLESS
    );
    if (!avatarWindow) {
        std::cerr << "Failed to create avatar window: " << SDL_GetError() << std::endl;
        return false;
    }
    
    SDL_SetWindowHitTest(avatarWindow, [](SDL_Window*, const SDL_Point*, void*) {
        return SDL_HITTEST_DRAGGABLE;
    }, NULL);
    
    avatarRenderer = SDL_CreateRenderer(avatarWindow, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!avatarRenderer) {
        avatarRenderer = SDL_CreateRenderer(avatarWindow, -1, SDL_RENDERER_SOFTWARE);
    }
    if (!avatarRenderer) {
        std::cerr << "Failed to create avatar renderer: " << SDL_GetError() << std::endl;
        return false;
    }
    
    SDL_Surface* iconSurf = IMG_Load("assets/icons/app_icon.png");
    if (iconSurf) {
        SDL_SetWindowIcon(avatarWindow, iconSurf);
        SDL_FreeSurface(iconSurf);
    }
    
    std::string idlePath = "presets/" + name + "/idle.png";
    std::string talkPath = "presets/" + name + "/talk.png";
    
    idleTex = IMG_LoadTexture(avatarRenderer, idlePath.c_str());
    talkTex = IMG_LoadTexture(avatarRenderer, talkPath.c_str());
    
    if (!idleTex || !talkTex) {
        std::cerr << "Warning: Failed to load textures for preset " << name << std::endl;
    }
    
    return true;
}

void updateAvatarWindowPosition() {
    if (avatarWindow) {
        int x, y;
        SDL_GetWindowPosition(avatarWindow, &x, &y);
        currentSettings.x = x;
        currentSettings.y = y;
    }
}

Uint32 getAvatarWindowID() {
    if (avatarWindow) {
        return SDL_GetWindowID(avatarWindow);
    }
    return 0;
}

void renderAvatar() {
    if (!avatarRenderer) return;
    
    if (isTalking) {
        float s = (float)currentSettings.shake;
        if (s > 0) {
            offsetX = (std::rand() % (int)(s * 2 + 1)) - s;
            offsetY = (std::rand() % (int)(s * 2 + 1)) - (s * 0.5f);
        } else {
            offsetX = 0;
            offsetY = 0;
        }
    } else {
        offsetX *= 0.8f;
        offsetY *= 0.8f;
    }
    
    Uint8 r = 0, g = 255, b = 0;
    switch (globalSettings.bgColorMode) {
        case 0: r = 0;   g = 255; b = 0;   break;
        case 1: r = 0;   g = 0;   b = 255; break;
        case 2: r = 255; g = 0;   b = 255; break;
        case 3: r = 0;   g = 0;   b = 0;   break;
        case 4: r = 255; g = 255; b = 255; break;
    }
    
    SDL_SetRenderDrawColor(avatarRenderer, r, g, b, 255);
    SDL_RenderClear(avatarRenderer);
    
    SDL_Texture* activeTex = isTalking ? talkTex : idleTex;
    if (activeTex) {
        SDL_Rect rect = {(int)offsetX, (int)offsetY, currentSettings.w, currentSettings.h};
        SDL_RenderCopy(avatarRenderer, activeTex, NULL, &rect);
    }
    
    SDL_RenderPresent(avatarRenderer);
}
