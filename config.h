#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <vector>
#include <SDL2/SDL.h>

struct AvatarLayer {
    std::string name;
    std::string filename;
    int type = 0; // 0: Base, 1: Mouth Open, 2: Mouth Closed, 3: Eyes Open, 4: Eyes Closed, 5: Hair Front, 6: Hair Back
    int x = 0;
    int y = 0;
    int w = 400;
    int h = 400;
    int z = 0;
    bool visible = true;
    
    // Physics Override Settings
    bool overridePhysics = false;
    bool jumpEnabled = true;
    float jumpHeight = 15.0f;
    float jumpSpeed = 1.0f;
    bool jellyEnabled = true;
    float jellyIntensity = 1.0f;
    float jellySpeed = 1.0f;
    int shake = 5;
    
    // Idle Animation & Hotkey Settings
    int animType = 0; // 0: None, 1: Breathe, 2: Float, 3: Wobble, 4: Spin
    float animSpeed = 1.0f;
    float animAmp = 1.0f;
    std::string hotkey = "";
    
    // Runtime physics state variables
    float jumpY = 0.0f;
    float jumpVelocity = 0.0f;
    float scaleY = 1.0f;
    float scaleYVelocity = 0.0f;
    float scaleX = 1.0f;
    float offsetX = 0.0f;
    float offsetY = 0.0f;
    
    // Runtime textures
    SDL_Texture* texAvatar = nullptr;
    SDL_Texture* texControl = nullptr;
};

struct PresetSettings {
    int x = 100;
    int y = 100;
    int w = 400;
    int h = 400;
    int shake = 5;
    float threshold = 0.05f;
    bool jumpEnabled = true;
    float jumpHeight = 15.0f;
    float jumpSpeed = 1.0f;
    bool jellyEnabled = true;
    float jellyIntensity = 1.0f;
    float jellySpeed = 1.0f;
    int winW = 400;
    int winH = 400;
    int avatarX = 0;
    int avatarY = 0;
};

struct GlobalSettings {
    std::string currentPreset = "default";
    std::string activeMicName = "None";
    int bgColorMode = 0; // 0: Green, 1: Blue, 2: Magenta, 3: Black, 4: White, 5: Transparent, 6: Custom Color, 7: Custom Image
    int controlX = 100;
    int controlY = 100;
    int controlW = 450;
    int controlH = 550;
    float customBgColor[4] = {0.0f, 1.0f, 0.0f, 1.0f};
    std::string customBgImagePath = "";
    int language = 0; // 0: English, 1: Ukrainian
};

extern PresetSettings currentSettings;
extern GlobalSettings globalSettings;
extern std::vector<std::string> presetList;
extern std::vector<AvatarLayer> currentLayers;

void scanPresets();
void loadPresetSettings(const std::string& name);
void savePresetSettings(const std::string& name);
void loadPresetLayers(const std::string& name);
void savePresetLayers(const std::string& name);
void loadGlobalSettings();
void saveGlobalSettings();

#endif // CONFIG_H
