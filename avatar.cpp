#include "avatar.h"
#include "config.h"
#include "audio.h"
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_syswm.h>
#include <iostream>
#include <cstdlib>

#ifdef _WIN32
#include <windows.h>
#endif

SDL_Window* avatarWindow = nullptr;
SDL_Renderer* avatarRenderer = nullptr;

static SDL_Texture* idleTex = nullptr;
static SDL_Texture* talkTex = nullptr;
static SDL_Texture* customBgTex = nullptr;
static float offsetX = 0.0f;
static float offsetY = 0.0f;

void updateWindowTransparency() {
    if (!avatarWindow) return;
#ifdef _WIN32
    SDL_SysWMinfo wmInfo;
    SDL_VERSION(&wmInfo.version);
    if (SDL_GetWindowWMInfo(avatarWindow, &wmInfo)) {
        HWND hwnd = wmInfo.info.win.window;
        LONG style = GetWindowLong(hwnd, GWL_EXSTYLE);
        if (globalSettings.bgColorMode == 2) { // Transparent / No Background
            SetWindowLong(hwnd, GWL_EXSTYLE, style | WS_EX_LAYERED);
            // Chroma-key magenta (255, 0, 255)
            SetLayeredWindowAttributes(hwnd, RGB(255, 0, 255), 0, LWA_COLORKEY);
        } else {
            SetWindowLong(hwnd, GWL_EXSTYLE, style & ~WS_EX_LAYERED);
            RedrawWindow(hwnd, NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_FRAME | RDW_ALLCHILDREN);
        }
    }
#endif
}

void loadCustomBgTexture(const std::string& path) {
    if (customBgTex) {
        SDL_DestroyTexture(customBgTex);
        customBgTex = nullptr;
    }
    if (!path.empty() && avatarRenderer) {
        customBgTex = IMG_LoadTexture(avatarRenderer, path.c_str());
        if (!customBgTex) {
            std::cerr << "Failed to load custom background: " << IMG_GetError() << std::endl;
        }
    }
}

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
    if (customBgTex) {
        SDL_DestroyTexture(customBgTex);
        customBgTex = nullptr;
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
    
    // Request an alpha visual channel to support transparent window background on Linux
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
    
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
    
    updateWindowTransparency();
    
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
    
    if (globalSettings.bgColorMode == 4 && !globalSettings.customBgImagePath.empty()) {
        loadCustomBgTexture(globalSettings.customBgImagePath);
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
    
    // Track transition from silent to talking
    static bool wasTalking = false;
    bool startedTalking = (isTalking && !wasTalking);
    wasTalking = isTalking;
    
    // Jump animation physics
    static float jumpY = 0.0f;
    static float jumpVelocity = 0.0f;
    
    // Jelly spring animation variables
    static float scaleY = 1.0f;
    static float scaleYVelocity = 0.0f;
    
    if (currentSettings.jumpEnabled) {
        if (startedTalking) {
            // Launch the jump
            jumpVelocity = -currentSettings.jumpHeight;
            
            if (currentSettings.jellyEnabled) {
                // Stretch Y as it launches
                scaleYVelocity = currentSettings.jumpHeight * 0.02f * currentSettings.jellyIntensity;
            }
        }
        
        if (jumpY < 0.0f || jumpVelocity != 0.0f) {
            float gravity = 1.0f * currentSettings.jumpSpeed;
            jumpVelocity += gravity;
            jumpY += jumpVelocity;
            
            if (jumpY >= 0.0f) {
                jumpY = 0.0f;
                jumpVelocity = 0.0f;
                
                // Squash Y as it lands
                if (currentSettings.jellyEnabled) {
                    scaleYVelocity = -currentSettings.jumpHeight * 0.03f * currentSettings.jellyIntensity;
                }
            }
        }
    } else {
        jumpY = 0.0f;
        jumpVelocity = 0.0f;
    }
    
    // Jelly spring physics update
    if (currentSettings.jellyEnabled) {
        float springK = 0.12f * currentSettings.jellySpeed;
        float damping = 0.75f;
        
        float forceY = -springK * (scaleY - 1.0f) - damping * scaleYVelocity;
        scaleYVelocity += forceY;
        scaleY += scaleYVelocity;
        
        // Safety bounds
        if (scaleY < 0.2f) scaleY = 0.2f;
        if (scaleY > 2.0f) scaleY = 2.0f;
    } else {
        scaleY = 1.0f;
        scaleYVelocity = 0.0f;
    }
    
    float scaleX = 2.0f - scaleY;
    
    // Shake offset logic (only when talking)
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
    
    Uint8 r = 0, g = 255, b = 0, a = 255;
    switch (globalSettings.bgColorMode) {
        case 0: r = 0;   g = 255; b = 0;   a = 255; break;
        case 1: r = 0;   g = 0;   b = 255; a = 255; break;
        case 2: // Transparent / No Background
#ifdef _WIN32
            // On Windows, use Magenta as the chroma key for transparent layered window
            r = 255; g = 0; b = 255; a = 255;
#else
            // On Linux, clear to transparent alpha
            r = 0; g = 0; b = 0; a = 0;
#endif
            break;
        case 3: // Custom Color
            r = (Uint8)(globalSettings.customBgColor[0] * 255.0f);
            g = (Uint8)(globalSettings.customBgColor[1] * 255.0f);
            b = (Uint8)(globalSettings.customBgColor[2] * 255.0f);
            a = (Uint8)(globalSettings.customBgColor[3] * 255.0f);
            break;
        case 4: // Custom Image
            r = 0; g = 0; b = 0; a = 255;
            break;
    }
    
    SDL_SetRenderDrawColor(avatarRenderer, r, g, b, a);
    SDL_RenderClear(avatarRenderer);
    
    // If Custom Image mode is selected and texture is loaded, draw it
    if (globalSettings.bgColorMode == 4 && customBgTex) {
        SDL_RenderCopy(avatarRenderer, customBgTex, NULL, NULL);
    }
    
    SDL_Texture* activeTex = isTalking ? talkTex : idleTex;
    if (activeTex) {
        int renderW = currentSettings.w;
        int renderH = currentSettings.h;
        
        if (currentSettings.jellyEnabled) {
            renderW = (int)(currentSettings.w * scaleX);
            renderH = (int)(currentSettings.h * scaleY);
        }
        
        // Pivot around the bottom-center of the avatar bounding box
        int baseBottomX = (int)offsetX + currentSettings.w / 2;
        int baseBottomY = (int)(offsetY + jumpY) + currentSettings.h;
        
        SDL_Rect rect;
        rect.w = renderW;
        rect.h = renderH;
        rect.x = baseBottomX - renderW / 2;
        rect.y = baseBottomY - renderH;
        
        SDL_RenderCopy(avatarRenderer, activeTex, NULL, &rect);
    }
    
    SDL_RenderPresent(avatarRenderer);
}
