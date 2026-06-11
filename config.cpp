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
    } else {
        currentSettings.x = 100;
        currentSettings.y = 100;
        currentSettings.w = 400;
        currentSettings.h = 400;
        currentSettings.shake = 5;
        currentSettings.threshold = 0.05f;
    }
}

void savePresetSettings(const std::string& name) {
    fs::path dir = fs::path("presets") / name;
    fs::create_directories(dir);
    fs::path file = dir / "settings.txt";
    
    std::ofstream out(file);
    if (out.is_open()) {
        out << currentSettings.x << " " << currentSettings.y << " " << currentSettings.w << " " << currentSettings.h << " " << currentSettings.shake << " " << currentSettings.threshold;
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
        }
        if (!(in >> globalSettings.controlX >> globalSettings.controlY >> globalSettings.controlW >> globalSettings.controlH)) {
            globalSettings.controlX = 100;
            globalSettings.controlY = 100;
            globalSettings.controlW = 450;
            globalSettings.controlH = 550;
        }
    } else {
        globalSettings.currentPreset = "default";
        globalSettings.activeMicName = "None";
        globalSettings.bgColorMode = 0;
        globalSettings.controlX = 100;
        globalSettings.controlY = 100;
        globalSettings.controlW = 450;
        globalSettings.controlH = 550;
    }
}

void saveGlobalSettings() {
    std::ofstream out("config.txt");
    if (out.is_open()) {
        out << globalSettings.currentPreset << "\n";
        out << globalSettings.activeMicName << "\n";
        out << globalSettings.bgColorMode << "\n";
        out << globalSettings.controlX << " " << globalSettings.controlY << " " << globalSettings.controlW << " " << globalSettings.controlH << "\n";
    }
}
