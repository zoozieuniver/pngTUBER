#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <vector>

struct PresetSettings {
    int x = 100;
    int y = 100;
    int w = 400;
    int h = 400;
    int shake = 5;
    float threshold = 0.05f;
};

struct GlobalSettings {
    std::string currentPreset = "default";
    std::string activeMicName = "None";
    int bgColorMode = 0; // 0: Green, 1: Blue, 2: Magenta, 3: Black, 4: White
    int controlX = 100;
    int controlY = 100;
    int controlW = 450;
    int controlH = 550;
};

extern PresetSettings currentSettings;
extern GlobalSettings globalSettings;
extern std::vector<std::string> presetList;

void scanPresets();
void loadPresetSettings(const std::string& name);
void savePresetSettings(const std::string& name);
void loadGlobalSettings();
void saveGlobalSettings();

#endif // CONFIG_H
