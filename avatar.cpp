#include "avatar.h"
#include "config.h"
#include "audio.h"
#include "gui.h"
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_syswm.h>
#include <iostream>
#include <cstdlib>
#include <unordered_map>
#include <algorithm>

#ifdef _WIN32
#include <windows.h>
#endif

SDL_Window* avatarWindow = nullptr;
SDL_Renderer* avatarRenderer = nullptr;
bool editorModeActive = false;
extern int selectedLayerIdx;
float previewZoom = 1.0f;
float previewPanX = 0.0f;
float previewPanY = 0.0f;

static SDL_Texture* customBgTexAvatar = nullptr;
static SDL_Texture* customBgTexControl = nullptr;
static float offsetX = 0.0f;
static float offsetY = 0.0f;

static float jumpY = 0.0f;
static float jumpVelocity = 0.0f;
static float scaleY = 1.0f;
static float scaleYVelocity = 0.0f;
static float scaleX = 1.0f;
static bool isBlinking = false;

struct HairPhysics {
    float dx = 0.0f;
    float dy = 0.0f;
    float vx = 0.0f;
    float vy = 0.0f;
};
static std::unordered_map<std::string, HairPhysics> hairStates;

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
            SetLayeredWindowAttributes(hwnd, RGB(255, 0, 255), 0, LWA_COLORKEY);
        } else {
            SetWindowLong(hwnd, GWL_EXSTYLE, style & ~WS_EX_LAYERED);
            RedrawWindow(hwnd, NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_FRAME | RDW_ALLCHILDREN);
        }
    }
#endif
}

void loadCustomBgTexture(const std::string& path) {
    if (customBgTexAvatar) {
        SDL_DestroyTexture(customBgTexAvatar);
        customBgTexAvatar = nullptr;
    }
    if (customBgTexControl) {
        SDL_DestroyTexture(customBgTexControl);
        customBgTexControl = nullptr;
    }
    if (!path.empty()) {
        if (avatarRenderer) {
            customBgTexAvatar = IMG_LoadTexture(avatarRenderer, path.c_str());
            if (!customBgTexAvatar) {
                std::cerr << "Failed to load custom background for avatar: " << IMG_GetError() << std::endl;
            }
        }
        if (controlRenderer) {
            customBgTexControl = IMG_LoadTexture(controlRenderer, path.c_str());
            if (!customBgTexControl) {
                std::cerr << "Failed to load custom background for control: " << IMG_GetError() << std::endl;
            }
        }
    }
}

void adjustWindowToBgTextureSize() {
    if (customBgTexAvatar) {
        int w = 0, h = 0;
        SDL_QueryTexture(customBgTexAvatar, NULL, NULL, &w, &h);
        if (w > 0 && h > 0) {
            currentSettings.winW = w;
            currentSettings.winH = h;
            updateAvatarWindowSize();
        }
    }
}

bool initAvatar() {
    return true;
}

void reloadLayerTextures() {
    for (auto& layer : currentLayers) {
        if (layer.texAvatar) {
            SDL_DestroyTexture(layer.texAvatar);
            layer.texAvatar = nullptr;
        }
        if (layer.texControl) {
            SDL_DestroyTexture(layer.texControl);
            layer.texControl = nullptr;
        }
        
        std::string path = "presets/" + globalSettings.currentPreset + "/" + layer.filename;
        if (avatarRenderer) {
            layer.texAvatar = IMG_LoadTexture(avatarRenderer, path.c_str());
            if (!layer.texAvatar) {
                std::cerr << "Failed to load layer avatar texture: " << path << " error: " << IMG_GetError() << std::endl;
            }
        }
        if (controlRenderer) {
            layer.texControl = IMG_LoadTexture(controlRenderer, path.c_str());
            if (!layer.texControl) {
                std::cerr << "Failed to load layer control texture: " << path << " error: " << IMG_GetError() << std::endl;
            }
        }
    }
}

void clearLayerTextures() {
    for (auto& layer : currentLayers) {
        if (layer.texAvatar) {
            SDL_DestroyTexture(layer.texAvatar);
            layer.texAvatar = nullptr;
        }
        if (layer.texControl) {
            SDL_DestroyTexture(layer.texControl);
            layer.texControl = nullptr;
        }
    }
}

void cleanupAvatar() {
    clearLayerTextures();
    if (customBgTexAvatar) {
        SDL_DestroyTexture(customBgTexAvatar);
        customBgTexAvatar = nullptr;
    }
    if (customBgTexControl) {
        SDL_DestroyTexture(customBgTexControl);
        customBgTexControl = nullptr;
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
        int x, y;
        SDL_GetWindowPosition(avatarWindow, &x, &y);
        currentSettings.x = x;
        currentSettings.y = y;
        savePresetSettings(globalSettings.currentPreset);
    }
    
    cleanupAvatar();
    
    globalSettings.currentPreset = name;
    loadPresetSettings(name);
    loadPresetLayers(name);
    
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
    
    int winW = currentSettings.winW;
    int winH = currentSettings.winH;
    
    avatarWindow = SDL_CreateWindow(
        name.c_str(),
        currentSettings.x,
        currentSettings.y,
        winW,
        winH,
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
    
    reloadLayerTextures();
    
    if (globalSettings.bgColorMode == 4 && !globalSettings.customBgImagePath.empty()) {
        loadCustomBgTexture(globalSettings.customBgImagePath);
    }
    
    // Hide avatar window if in editor mode initially
    if (editorModeActive) {
        SDL_HideWindow(avatarWindow);
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

void updateAvatarWindowSize() {
    if (avatarWindow) {
        SDL_SetWindowSize(avatarWindow, currentSettings.winW, currentSettings.winH);
    }
}

Uint32 getAvatarWindowID() {
    if (avatarWindow) {
        return SDL_GetWindowID(avatarWindow);
    }
    return 0;
}

void setEditorMode(bool enable) {
    editorModeActive = enable;
    if (enable) {
        if (avatarWindow) {
            SDL_HideWindow(avatarWindow);
        }
    } else {
        if (avatarWindow) {
            updateAvatarWindowSize();
            SDL_ShowWindow(avatarWindow);
            updateWindowTransparency();
        }
    }
    // Make sure textures are synchronized
    reloadLayerTextures();
    if (globalSettings.bgColorMode == 4 && !globalSettings.customBgImagePath.empty()) {
        loadCustomBgTexture(globalSettings.customBgImagePath);
    }
}

const float PI = 3.1415926535f;

static void getLayerAnimationState(const AvatarLayer& layer, float& animOffsetX, float& animOffsetY, float& animScaleX, float& animScaleY, double& animAngle) {
    animOffsetX = 0.0f;
    animOffsetY = 0.0f;
    animScaleX = 1.0f;
    animScaleY = 1.0f;
    animAngle = 0.0;
    
    if (layer.animType <= 0) return;
    
    float t = SDL_GetTicks() * 0.001f * layer.animSpeed;
    
    switch (layer.animType) {
        case 1: // Breathe: scaleY oscillates, scaleX oscillates in reverse
            {
                float breathe = sinf(t * 2.0f * PI); // -1 to 1
                animScaleY = 1.0f + breathe * 0.05f * layer.animAmp;
                animScaleX = 1.0f - breathe * 0.03f * layer.animAmp;
            }
            break;
        case 2: // Float: vertical position Y oscillates
            {
                animOffsetY = sinf(t * 2.0f * PI) * 10.0f * layer.animAmp;
            }
            break;
        case 3: // Wobble: rotation angle oscillates
            {
                animAngle = sinf(t * 2.0f * PI) * 15.0 * (double)layer.animAmp;
            }
            break;
        case 4: // Spin: continuous rotation
            {
                animAngle = t * 360.0 * 0.1 * (double)layer.animAmp;
                animAngle = fmod(animAngle, 360.0);
            }
            break;
    }
}

static void drawLayers(SDL_Renderer* renderer, bool useControlTex) {
    if (currentLayers.empty()) return;
    
    std::vector<int> sortedIndices(currentLayers.size());
    for (size_t i = 0; i < currentLayers.size(); ++i) sortedIndices[i] = i;
    std::sort(sortedIndices.begin(), sortedIndices.end(), [](int a, int b) {
        return currentLayers[a].z < currentLayers[b].z;
    });
    
    int targetW = 0, targetH = 0;
    SDL_GetRendererOutputSize(renderer, &targetW, &targetH);
    
    float W_neutral = (float)currentSettings.w;
    float H_neutral = (float)currentSettings.h;
    
    // Fixed reference canvas size
    const float RefW = 400.0f;
    const float RefH = 400.0f;
    
    float winH = (float)currentSettings.winH;
    
    float previewScale = 1.0f;
    if (useControlTex) {
        previewScale = (float)targetH / winH;
    }
    
    float effZoom = useControlTex ? previewZoom : 1.0f;
    
    // Scale factors relative to the reference canvas
    float avatarScaleX = W_neutral / RefW;
    float avatarScaleY = H_neutral / RefH;
    
    for (int idx : sortedIndices) {
        const auto& layer = currentLayers[idx];
        
        // Visibility check
        if (!layer.visible) continue;
        
        if (layer.type == 1 && !isTalking) continue;
        if (layer.type == 2 && isTalking) continue;
        
        if (layer.type == 3 && isBlinking) {
            bool hasEyesClosed = false;
            for (const auto& l : currentLayers) {
                if (l.type == 4) {
                    hasEyesClosed = true;
                    break;
                }
            }
            if (hasEyesClosed) continue;
        }
        if (layer.type == 4 && !isBlinking) continue;
        
        SDL_Texture* tex = useControlTex ? layer.texControl : layer.texAvatar;
        if (!tex) continue;
        
        // Determine local or global physics offsets & scales
        float lOffsetX = layer.overridePhysics ? layer.offsetX : offsetX;
        float lOffsetY = layer.overridePhysics ? layer.offsetY : offsetY;
        float lJumpY = layer.overridePhysics ? layer.jumpY : jumpY;
        
        float lScaleX = layer.overridePhysics ? layer.scaleX : scaleX;
        float lScaleY = layer.overridePhysics ? layer.scaleY : scaleY;
        
        float animOffsetX = 0.0f, animOffsetY = 0.0f;
        float animScaleX = 1.0f, animScaleY = 1.0f;
        double animAngle = 0.0;
        getLayerAnimationState(layer, animOffsetX, animOffsetY, animScaleX, animScaleY, animAngle);
        
        float totalScaleX = avatarScaleX * lScaleX * previewScale * effZoom * animScaleX;
        float totalScaleY = avatarScaleY * lScaleY * previewScale * effZoom * animScaleY;
        
        float baseBottomX;
        if (useControlTex) {
            baseBottomX = (float)targetW / 2.0f + (lOffsetX + (float)currentSettings.avatarX) * previewScale * effZoom + previewPanX;
        } else {
            baseBottomX = (lOffsetX + (float)currentSettings.avatarX) + (float)currentSettings.winW / 2.0f;
        }
        
        float baseBottomY;
        if (useControlTex) {
            baseBottomY = (float)targetH + (lOffsetY + lJumpY - (float)currentSettings.avatarY) * previewScale * effZoom + previewPanY;
        } else {
            baseBottomY = lOffsetY + lJumpY - (float)currentSettings.avatarY + (float)currentSettings.winH;
        }
        
        // Bottom-center of the layer relative to the baseline of the avatar
        // layer.x is horizontal offset from center (positive = right)
        // layer.y is vertical offset from bottom (positive = up)
        float anchorX = baseBottomX + (float)layer.x * totalScaleX;
        float anchorY = baseBottomY - (float)layer.y * totalScaleY;
        
        float w_scaled = (float)layer.w * totalScaleX;
        float h_scaled = (float)layer.h * totalScaleY;
        
        float hairDx = 0.0f;
        float hairDy = 0.0f;
        if (layer.type == 5 || layer.type == 6) {
            auto it = hairStates.find(layer.name);
            if (it != hairStates.end()) {
                hairDx = it->second.dx * previewScale * effZoom;
                hairDy = it->second.dy * previewScale * effZoom;
            }
        }
        
        SDL_Rect rect;
        rect.x = (int)(anchorX - w_scaled / 2.0f + hairDx + animOffsetX * previewScale * effZoom);
        rect.y = (int)(anchorY - h_scaled + hairDy + animOffsetY * previewScale * effZoom);
        rect.w = (int)w_scaled;
        rect.h = (int)h_scaled;
        
        if (animAngle != 0.0) {
            SDL_RenderCopyEx(renderer, tex, NULL, &rect, animAngle, NULL, SDL_FLIP_NONE);
        } else {
            SDL_RenderCopy(renderer, tex, NULL, &rect);
        }
        
        if (useControlTex && idx == selectedLayerIdx) {
            SDL_SetRenderDrawColor(renderer, 0, 255, 255, 255); // Cyan outline
            SDL_RenderDrawRect(renderer, &rect);
            SDL_Rect outerRect = { rect.x - 1, rect.y - 1, rect.w + 2, rect.h + 2 };
            SDL_RenderDrawRect(renderer, &outerRect);
        }
    }
}

void renderAvatar() {
    static bool wasTalking = false;
    bool startedTalking = (isTalking && !wasTalking);
    wasTalking = isTalking;
    
    if (currentSettings.jumpEnabled) {
        if (startedTalking) {
            jumpVelocity = -currentSettings.jumpHeight;
            if (currentSettings.jellyEnabled) {
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
                
                if (currentSettings.jellyEnabled) {
                    scaleYVelocity = -currentSettings.jumpHeight * 0.03f * currentSettings.jellyIntensity;
                }
            }
        }
    } else {
        jumpY = 0.0f;
        jumpVelocity = 0.0f;
    }
    
    if (currentSettings.jellyEnabled) {
        float springK = 0.12f * currentSettings.jellySpeed;
        float damping = 0.75f;
        
        float forceY = -springK * (scaleY - 1.0f) - damping * scaleYVelocity;
        scaleYVelocity += forceY;
        scaleY += scaleYVelocity;
        
        if (scaleY < 0.2f) scaleY = 0.2f;
        if (scaleY > 2.0f) scaleY = 2.0f;
    } else {
        scaleY = 1.0f;
        scaleYVelocity = 0.0f;
    }
    
    scaleX = 2.0f - scaleY;
    
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
    
    // Per-layer physics updates
    for (auto& layer : currentLayers) {
        if (!layer.overridePhysics) {
            layer.jumpY = 0.0f;
            layer.jumpVelocity = 0.0f;
            layer.scaleY = 1.0f;
            layer.scaleYVelocity = 0.0f;
            layer.scaleX = 1.0f;
            layer.offsetX = 0.0f;
            layer.offsetY = 0.0f;
            continue;
        }
        
        if (layer.jumpEnabled) {
            if (startedTalking) {
                layer.jumpVelocity = -layer.jumpHeight;
                if (layer.jellyEnabled) {
                    layer.scaleYVelocity = layer.jumpHeight * 0.02f * layer.jellyIntensity;
                }
            }
            if (layer.jumpY < 0.0f || layer.jumpVelocity != 0.0f) {
                float gravity = 1.0f * layer.jumpSpeed;
                layer.jumpVelocity += gravity;
                layer.jumpY += layer.jumpVelocity;
                if (layer.jumpY >= 0.0f) {
                    layer.jumpY = 0.0f;
                    layer.jumpVelocity = 0.0f;
                    if (layer.jellyEnabled) {
                        layer.scaleYVelocity = -layer.jumpHeight * 0.03f * layer.jellyIntensity;
                    }
                }
            }
        } else {
            layer.jumpY = 0.0f;
            layer.jumpVelocity = 0.0f;
        }
        
        if (layer.jellyEnabled) {
            float springK = 0.12f * layer.jellySpeed;
            float damping = 0.75f;
            float forceY = -springK * (layer.scaleY - 1.0f) - damping * layer.scaleYVelocity;
            layer.scaleYVelocity += forceY;
            layer.scaleY += layer.scaleYVelocity;
            if (layer.scaleY < 0.2f) layer.scaleY = 0.2f;
            if (layer.scaleY > 2.0f) layer.scaleY = 2.0f;
        } else {
            layer.scaleY = 1.0f;
            layer.scaleYVelocity = 0.0f;
        }
        layer.scaleX = 2.0f - layer.scaleY;
        
        if (isTalking) {
            float s = (float)layer.shake;
            if (s > 0) {
                layer.offsetX = (std::rand() % (int)(s * 2 + 1)) - s;
                layer.offsetY = (std::rand() % (int)(s * 2 + 1)) - (s * 0.5f);
            } else {
                layer.offsetX = 0.0f;
                layer.offsetY = 0.0f;
            }
        } else {
            layer.offsetX *= 0.8f;
            layer.offsetY *= 0.8f;
        }
    }
    
    static Uint32 lastBlinkTime = 0;
    static Uint32 blinkDuration = 150;
    static Uint32 nextBlinkDelay = 3000;
    Uint32 now = SDL_GetTicks();
    if (!isBlinking) {
        if (now - lastBlinkTime > nextBlinkDelay) {
            isBlinking = true;
            lastBlinkTime = now;
            nextBlinkDelay = 2500 + std::rand() % 2500;
        }
    } else {
        if (now - lastBlinkTime > blinkDuration) {
            isBlinking = false;
            lastBlinkTime = now;
        }
    }
    
    for (const auto& layer : currentLayers) {
        if (layer.type == 5 || layer.type == 6) {
            float lOffsetX = layer.overridePhysics ? layer.offsetX : offsetX;
            float lJumpVel = layer.overridePhysics ? layer.jumpVelocity : jumpVelocity;
            float lScaleY = layer.overridePhysics ? layer.scaleY : scaleY;
            
            float forceX = lOffsetX * 0.4f;
            float forceY = -lJumpVel * 0.25f + (lScaleY - 1.0f) * 8.0f;
            
            float k = (layer.type == 5) ? 0.12f : 0.06f;
            float damping = (layer.type == 5) ? 0.82f : 0.88f;
            
            auto& state = hairStates[layer.name];
            float ax = -k * state.dx - (1.0f - damping) * state.vx + forceX;
            float ay = -k * state.dy - (1.0f - damping) * state.vy + forceY;
            state.vx += ax;
            state.vy += ay;
            state.dx += state.vx;
            state.dy += state.vy;
            
            state.vx *= damping;
            state.vy *= damping;
        }
    }
    
    if (editorModeActive) {
        return;
    }
    
    if (!avatarRenderer) return;
    
    Uint8 r = 0, g = 255, b = 0, a = 255;
    switch (globalSettings.bgColorMode) {
        case 0: r = 0;   g = 255; b = 0;   a = 255; break;
        case 1: r = 0;   g = 0;   b = 255; a = 255; break;
        case 2:
#ifdef _WIN32
            r = 255; g = 0; b = 255; a = 255;
#else
            r = 0; g = 0; b = 0; a = 0;
#endif
            break;
        case 3:
            r = (Uint8)(globalSettings.customBgColor[0] * 255.0f);
            g = (Uint8)(globalSettings.customBgColor[1] * 255.0f);
            b = (Uint8)(globalSettings.customBgColor[2] * 255.0f);
            a = (Uint8)(globalSettings.customBgColor[3] * 255.0f);
            break;
        case 4:
            r = 0; g = 0; b = 0; a = 255;
            break;
    }
    
    SDL_SetRenderDrawColor(avatarRenderer, r, g, b, a);
    SDL_RenderClear(avatarRenderer);
    
    if (globalSettings.bgColorMode == 4 && customBgTexAvatar) {
        SDL_RenderCopy(avatarRenderer, customBgTexAvatar, NULL, NULL);
    }
    
    drawLayers(avatarRenderer, false);
    
    SDL_RenderPresent(avatarRenderer);
}

void renderAvatarPreviewToTexture(SDL_Texture* targetTexture, int w, int h) {
    if (!controlRenderer || !targetTexture) return;
    
    SDL_SetRenderTarget(controlRenderer, targetTexture);
    
    // Clear entire preview to dark gray neutral background
    SDL_SetRenderDrawColor(controlRenderer, 30, 30, 35, 255);
    SDL_RenderClear(controlRenderer);
    
    // Calculate the window dimensions rectangle in preview coordinates
    float winH = (float)currentSettings.winH;
    float previewScale = (float)h / winH;
    float effZoom = previewZoom;
    
    SDL_Rect winRect;
    winRect.w = (int)(currentSettings.winW * previewScale * effZoom);
    winRect.h = (int)(currentSettings.winH * previewScale * effZoom);
    winRect.x = (int)((float)w / 2.0f + previewPanX - (float)winRect.w / 2.0f);
    winRect.y = (int)((float)h + previewPanY - (float)winRect.h);
    
    // Set Clip Rect to keep rendering inside window bounds
    SDL_RenderSetClipRect(controlRenderer, &winRect);
    
    Uint8 r = 0, g = 255, b = 0, a = 255;
    switch (globalSettings.bgColorMode) {
        case 0: r = 0;   g = 255; b = 0;   a = 255; break;
        case 1: r = 0;   g = 0;   b = 255; a = 255; break;
        case 2: r = 0;   g = 0;   b = 0;   a = 0;   break;
        case 3:
            r = (Uint8)(globalSettings.customBgColor[0] * 255.0f);
            g = (Uint8)(globalSettings.customBgColor[1] * 255.0f);
            b = (Uint8)(globalSettings.customBgColor[2] * 255.0f);
            a = (Uint8)(globalSettings.customBgColor[3] * 255.0f);
            break;
        case 4:
            r = 0; g = 0; b = 0; a = 255;
            break;
    }
    
    SDL_SetRenderDrawColor(controlRenderer, r, g, b, a);
    SDL_RenderFillRect(controlRenderer, &winRect);
    
    if (globalSettings.bgColorMode == 4 && customBgTexControl) {
        SDL_RenderCopy(controlRenderer, customBgTexControl, NULL, &winRect);
    }
    
    drawLayers(controlRenderer, true);
    
    // Disable Clip Rect to render border/outline
    SDL_RenderSetClipRect(controlRenderer, nullptr);
    
    // Render boundary outline (neon violet)
    SDL_SetRenderDrawColor(controlRenderer, 140, 50, 230, 255);
    SDL_Rect outlineRect = winRect;
    SDL_RenderDrawRect(controlRenderer, &outlineRect);
    outlineRect.x -= 1;
    outlineRect.y -= 1;
    outlineRect.w += 2;
    outlineRect.h += 2;
    SDL_RenderDrawRect(controlRenderer, &outlineRect);
    
    SDL_SetRenderTarget(controlRenderer, nullptr);
}

int hitTestLayers(int mouseX, int mouseY, int targetW, int targetH) {
    if (currentLayers.empty()) return -1;
    
    std::vector<int> sortedIndices(currentLayers.size());
    for (size_t i = 0; i < currentLayers.size(); ++i) sortedIndices[i] = i;
    std::sort(sortedIndices.begin(), sortedIndices.end(), [](int a, int b) {
        return currentLayers[a].z < currentLayers[b].z;
    });
    
    float W_neutral = (float)currentSettings.w;
    float H_neutral = (float)currentSettings.h;
    
    const float RefW = 400.0f;
    const float RefH = 400.0f;
    
    float winH = (float)currentSettings.winH;
    float previewScale = (float)targetH / winH;
    float effZoom = previewZoom;
    
    float avatarScaleX = W_neutral / RefW;
    float avatarScaleY = H_neutral / RefH;
    
    for (auto it = sortedIndices.rbegin(); it != sortedIndices.rend(); ++it) {
        int idx = *it;
        const auto& layer = currentLayers[idx];
        
        if (!layer.visible) continue;
        if (layer.type == 1 && !isTalking) continue;
        if (layer.type == 2 && isTalking) continue;
        
        if (layer.type == 3 && isBlinking) {
            bool hasEyesClosed = false;
            for (const auto& l : currentLayers) {
                if (l.type == 4) {
                    hasEyesClosed = true;
                    break;
                }
            }
            if (hasEyesClosed) continue;
        }
        if (layer.type == 4 && !isBlinking) continue;
        
        SDL_Texture* tex = layer.texControl;
        if (!tex) continue;
        
        float lOffsetX = layer.overridePhysics ? layer.offsetX : offsetX;
        float lOffsetY = layer.overridePhysics ? layer.offsetY : offsetY;
        float lJumpY = layer.overridePhysics ? layer.jumpY : jumpY;
        
        float lScaleX = layer.overridePhysics ? layer.scaleX : scaleX;
        float lScaleY = layer.overridePhysics ? layer.scaleY : scaleY;
        
        float animOffsetX = 0.0f, animOffsetY = 0.0f;
        float animScaleX = 1.0f, animScaleY = 1.0f;
        double animAngle = 0.0;
        getLayerAnimationState(layer, animOffsetX, animOffsetY, animScaleX, animScaleY, animAngle);
        
        float totalScaleX = avatarScaleX * lScaleX * previewScale * effZoom * animScaleX;
        float totalScaleY = avatarScaleY * lScaleY * previewScale * effZoom * animScaleY;
        
        float baseBottomX = (float)targetW / 2.0f + (lOffsetX + (float)currentSettings.avatarX) * previewScale * effZoom + previewPanX;
        float baseBottomY = (float)targetH + (lOffsetY + lJumpY - (float)currentSettings.avatarY) * previewScale * effZoom + previewPanY;
        
        float anchorX = baseBottomX + (float)layer.x * totalScaleX;
        float anchorY = baseBottomY - (float)layer.y * totalScaleY;
        
        float w_scaled = (float)layer.w * totalScaleX;
        float h_scaled = (float)layer.h * totalScaleY;
        
        float hairDx = 0.0f;
        float hairDy = 0.0f;
        if (layer.type == 5 || layer.type == 6) {
            auto it2 = hairStates.find(layer.name);
            if (it2 != hairStates.end()) {
                hairDx = it2->second.dx * previewScale * effZoom;
                hairDy = it2->second.dy * previewScale * effZoom;
            }
        }
        
        SDL_Rect rect;
        rect.x = (int)(anchorX - w_scaled / 2.0f + hairDx + animOffsetX * previewScale * effZoom);
        rect.y = (int)(anchorY - h_scaled + hairDy + animOffsetY * previewScale * effZoom);
        rect.w = (int)w_scaled;
        rect.h = (int)h_scaled;
        
        if (mouseX >= rect.x && mouseX < rect.x + rect.w &&
            mouseY >= rect.y && mouseY < rect.y + rect.h) {
            return idx;
        }
    }
    
    return -1;
}
