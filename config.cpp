#include "config.h"
#include <fstream>
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

PresetSettings currentSettings;
GlobalSettings globalSettings;
std::vector<std::string> presetList;

void scanPresets() {
    presetList.clear();
    if (fs::exists("presets")) {
        for (const auto& entry : fs::directory_iterator("presets")) {
            if (entry.is_directory()) {
                presetList.push_back(entry.path().filename().string());
            }
        }
    }
    if (presetList.empty()) {
        presetList.push_back("default");
    }
}

void loadPresetSettings(const std::string& name) {
    fs::path dir = fs::path("presets") / name;
    fs::create_directories(dir);
    fs::path file = dir / "settings.txt";
    
    std::ifstream in(file);
    if (in.is_open()) {
        in >> currentSettings.x >> currentSettings.y >> currentSettings.w >> currentSettings.h >> currentSettings.shake >> currentSettings.threshold;
        
        int jEnabled = 1, jelEnabled = 1;
        if (in >> jEnabled >> currentSettings.jumpHeight >> currentSettings.jumpSpeed >> jelEnabled >> currentSettings.jellyIntensity >> currentSettings.jellySpeed) {
            currentSettings.jumpEnabled = (jEnabled != 0);
            currentSettings.jellyEnabled = (jelEnabled != 0);
        } else {
            currentSettings.jumpEnabled = true;
            currentSettings.jumpHeight = 15.0f;
            currentSettings.jumpSpeed = 1.0f;
            currentSettings.jellyEnabled = true;
            currentSettings.jellyIntensity = 1.0f;
            currentSettings.jellySpeed = 1.0f;
        }
    } else {
        currentSettings.x = 100;
        currentSettings.y = 100;
        currentSettings.w = 400;
        currentSettings.h = 400;
        currentSettings.shake = 5;
        currentSettings.threshold = 0.05f;
        currentSettings.jumpEnabled = true;
        currentSettings.jumpHeight = 15.0f;
        currentSettings.jumpSpeed = 1.0f;
        currentSettings.jellyEnabled = true;
        currentSettings.jellyIntensity = 1.0f;
        currentSettings.jellySpeed = 1.0f;
    }
}

void savePresetSettings(const std::string& name) {
    fs::path dir = fs::path("presets") / name;
    fs::create_directories(dir);
    fs::path file = dir / "settings.txt";
    
    std::ofstream out(file);
    if (out.is_open()) {
        out << currentSettings.x << " " << currentSettings.y << " " << currentSettings.w << " " << currentSettings.h << " " << currentSettings.shake << " " << currentSettings.threshold << " "
            << (currentSettings.jumpEnabled ? 1 : 0) << " " << currentSettings.jumpHeight << " " << currentSettings.jumpSpeed << " "
            << (currentSettings.jellyEnabled ? 1 : 0) << " " << currentSettings.jellyIntensity << " " << currentSettings.jellySpeed;
    }
}

void loadGlobalSettings() {
    std::ifstream in("config.txt");
    if (in.is_open()) {
        in >> globalSettings.currentPreset;
        
        std::string dummy;
        std::getline(in, dummy); // consume newline
        std::getline(in, globalSettings.activeMicName);
        
        if (!(in >> globalSettings.bgColorMode)) {
            globalSettings.bgColorMode = 0;
        } else {
            // Backwards compatibility mapping:
            if (globalSettings.bgColorMode == 5) {
                globalSettings.bgColorMode = 2; // Transparent
            } else if (globalSettings.bgColorMode == 6) {
                globalSettings.bgColorMode = 3; // Custom Color
            } else if (globalSettings.bgColorMode == 7) {
                globalSettings.bgColorMode = 4; // Custom Image
            } else if (globalSettings.bgColorMode < 0 || globalSettings.bgColorMode > 4) {
                globalSettings.bgColorMode = 0; // Fallback to Green Screen
            }
        }
        if (!(in >> globalSettings.controlX >> globalSettings.controlY >> globalSettings.controlW >> globalSettings.controlH)) {
            globalSettings.controlX = 100;
            globalSettings.controlY = 100;
            globalSettings.controlW = 450;
            globalSettings.controlH = 550;
        }
        if (!(in >> globalSettings.customBgColor[0] >> globalSettings.customBgColor[1] >> globalSettings.customBgColor[2] >> globalSettings.customBgColor[3])) {
            globalSettings.customBgColor[0] = 0.0f;
            globalSettings.customBgColor[1] = 1.0f;
            globalSettings.customBgColor[2] = 0.0f;
            globalSettings.customBgColor[3] = 1.0f;
        }
        std::getline(in, dummy); // consume newline
        if (!std::getline(in, globalSettings.customBgImagePath)) {
            globalSettings.customBgImagePath = "";
        }
    } else {
        globalSettings.currentPreset = "default";
        globalSettings.activeMicName = "None";
        globalSettings.bgColorMode = 0;
        globalSettings.controlX = 100;
        globalSettings.controlY = 100;
        globalSettings.controlW = 450;
        globalSettings.controlH = 550;
        globalSettings.customBgColor[0] = 0.0f;
        globalSettings.customBgColor[1] = 1.0f;
        globalSettings.customBgColor[2] = 0.0f;
        globalSettings.customBgColor[3] = 1.0f;
        globalSettings.customBgImagePath = "";
    }
}

void saveGlobalSettings() {
    std::ofstream out("config.txt");
    if (out.is_open()) {
        out << globalSettings.currentPreset << "\n";
        out << globalSettings.activeMicName << "\n";
        out << globalSettings.bgColorMode << "\n";
        out << globalSettings.controlX << " " << globalSettings.controlY << " " << globalSettings.controlW << " " << globalSettings.controlH << "\n";
        out << globalSettings.customBgColor[0] << " " << globalSettings.customBgColor[1] << " " 
            << globalSettings.customBgColor[2] << " " << globalSettings.customBgColor[3] << "\n";
        out << globalSettings.customBgImagePath << "\n";
    }
}
