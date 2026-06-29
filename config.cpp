#include "config.h"
#include <fstream>
#include <filesystem>
#include <iostream>
#include <sstream>

namespace fs = std::filesystem;

PresetSettings currentSettings;
GlobalSettings globalSettings;
std::vector<std::string> presetList;
std::vector<AvatarLayer> currentLayers;

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
        
        if (!(in >> currentSettings.winW >> currentSettings.winH)) {
            currentSettings.winW = currentSettings.w;
            currentSettings.winH = currentSettings.h + (int)currentSettings.jumpHeight + 150;
        }
        if (!(in >> currentSettings.avatarX >> currentSettings.avatarY)) {
            currentSettings.avatarX = 0;
            currentSettings.avatarY = 0;
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
        currentSettings.winW = 400;
        currentSettings.winH = 565;
        currentSettings.avatarX = 0;
        currentSettings.avatarY = 0;
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
            << (currentSettings.jellyEnabled ? 1 : 0) << " " << currentSettings.jellyIntensity << " " << currentSettings.jellySpeed << " "
            << currentSettings.winW << " " << currentSettings.winH << " "
            << currentSettings.avatarX << " " << currentSettings.avatarY;
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
        if (!(in >> globalSettings.language)) {
            globalSettings.language = 0;
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
        globalSettings.language = 0;
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
        out << globalSettings.language << "\n";
    }
}

void loadPresetLayers(const std::string& name) {
    currentLayers.clear();
    fs::path dir = fs::path("presets") / name;
    fs::path file = dir / "layers.txt";
    if (fs::exists(file)) {
        std::ifstream in(file);
        std::string line;
        while (std::getline(in, line)) {
            if (line.empty()) continue;
            std::stringstream ss(line);
            std::string item;
            AvatarLayer layer;
            if (std::getline(ss, item, '|')) layer.name = item;
            if (std::getline(ss, item, '|')) layer.filename = item;
            if (std::getline(ss, item, '|')) layer.type = std::stoi(item);
            if (std::getline(ss, item, '|')) layer.x = std::stoi(item);
            if (std::getline(ss, item, '|')) layer.y = std::stoi(item);
            if (std::getline(ss, item, '|')) layer.w = std::stoi(item);
            if (std::getline(ss, item, '|')) layer.h = std::stoi(item);
            if (std::getline(ss, item, '|')) layer.z = std::stoi(item);
            if (std::getline(ss, item, '|')) {
                layer.visible = (std::stoi(item) != 0);
            } else {
                layer.visible = true;
            }
            if (std::getline(ss, item, '|')) layer.overridePhysics = (std::stoi(item) != 0);
            else layer.overridePhysics = false;
            if (std::getline(ss, item, '|')) layer.jumpEnabled = (std::stoi(item) != 0);
            else layer.jumpEnabled = true;
            if (std::getline(ss, item, '|')) layer.jumpHeight = std::stof(item);
            else layer.jumpHeight = 15.0f;
            if (std::getline(ss, item, '|')) layer.jumpSpeed = std::stof(item);
            else layer.jumpSpeed = 1.0f;
            if (std::getline(ss, item, '|')) layer.jellyEnabled = (std::stoi(item) != 0);
            else layer.jellyEnabled = true;
            if (std::getline(ss, item, '|')) layer.jellyIntensity = std::stof(item);
            else layer.jellyIntensity = 1.0f;
            if (std::getline(ss, item, '|')) layer.jellySpeed = std::stof(item);
            else layer.jellySpeed = 1.0f;
            if (std::getline(ss, item, '|')) layer.shake = std::stoi(item);
            else layer.shake = 5;
            if (std::getline(ss, item, '|')) layer.animType = std::stoi(item);
            else layer.animType = 0;
            if (std::getline(ss, item, '|')) layer.animSpeed = std::stof(item);
            else layer.animSpeed = 1.0f;
            if (std::getline(ss, item, '|')) layer.animAmp = std::stof(item);
            else layer.animAmp = 1.0f;
            if (std::getline(ss, item, '|')) layer.hotkey = item;
            else layer.hotkey = "";
            
            layer.texAvatar = nullptr;
            layer.texControl = nullptr;
            currentLayers.push_back(layer);
        }
    } else {
        // Fallback: Check if idle.png exists
        if (fs::exists(dir / "idle.png")) {
            AvatarLayer layer;
            layer.name = "idle";
            layer.filename = "idle.png";
            layer.type = 2; // Mouth Closed
            layer.x = 0;
            layer.y = 0;
            layer.w = 400;
            layer.h = 400;
            layer.z = 0;
            layer.visible = true;
            currentLayers.push_back(layer);
        }
        // Check if talk.png exists
        if (fs::exists(dir / "talk.png")) {
            AvatarLayer layer;
            layer.name = "talk";
            layer.filename = "talk.png";
            layer.type = 1; // Mouth Open;
            layer.x = 0;
            layer.y = 0;
            layer.w = 400;
            layer.h = 400;
            layer.z = 0;
            layer.visible = true;
            currentLayers.push_back(layer);
        }
    }
}

void savePresetLayers(const std::string& name) {
    fs::path dir = fs::path("presets") / name;
    fs::create_directories(dir);
    fs::path file = dir / "layers.txt";
    std::ofstream out(file);
    if (out.is_open()) {
        for (const auto& layer : currentLayers) {
            out << layer.name << "|"
                << layer.filename << "|"
                << layer.type << "|"
                << layer.x << "|"
                << layer.y << "|"
                << layer.w << "|"
                << layer.h << "|"
                << layer.z << "|"
                << (layer.visible ? 1 : 0) << "|"
                << (layer.overridePhysics ? 1 : 0) << "|"
                << (layer.jumpEnabled ? 1 : 0) << "|"
                << layer.jumpHeight << "|"
                << layer.jumpSpeed << "|"
                << (layer.jellyEnabled ? 1 : 0) << "|"
                << layer.jellyIntensity << "|"
                << layer.jellySpeed << "|"
                << layer.shake << "|"
                << layer.animType << "|"
                << layer.animSpeed << "|"
                << layer.animAmp << "|"
                << layer.hotkey << "\n";
        }
    }
}
