#include "gui.h"
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer2.h"
#include "config.h"
#include "audio.h"
#include "avatar.h"
#include <iostream>

SDL_Window* controlWindow = nullptr;
SDL_Renderer* controlRenderer = nullptr;

bool initGUI() {
    controlWindow = SDL_CreateWindow(
        "PNGTuber CLI - Control Panel",
        globalSettings.controlX,
        globalSettings.controlY,
        globalSettings.controlW,
        globalSettings.controlH,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
    );
    if (!controlWindow) {
        std::cerr << "Failed to create control window: " << SDL_GetError() << std::endl;
        return false;
    }
    
    controlRenderer = SDL_CreateRenderer(controlWindow, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!controlRenderer) {
        controlRenderer = SDL_CreateRenderer(controlWindow, -1, SDL_RENDERER_SOFTWARE);
    }
    if (!controlRenderer) {
        std::cerr << "Failed to create control renderer: " << SDL_GetError() << std::endl;
        return false;
    }
    
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    
    ImGui_ImplSDL2_InitForSDLRenderer(controlWindow, controlRenderer);
    ImGui_ImplSDLRenderer2_Init(controlRenderer);
    
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 8.0f;
    style.FrameRounding = 5.0f;
    style.GrabRounding = 5.0f;
    style.PopupRounding = 6.0f;
    style.ScrollbarRounding = 6.0f;
    
    ImVec4* colors = style.Colors;
    colors[ImGuiCol_WindowBg]           = ImVec4(0.09f, 0.09f, 0.12f, 0.94f);
    colors[ImGuiCol_Header]             = ImVec4(0.38f, 0.14f, 0.73f, 0.70f);
    colors[ImGuiCol_HeaderHovered]      = ImVec4(0.46f, 0.17f, 0.89f, 0.80f);
    colors[ImGuiCol_HeaderActive]       = ImVec4(0.53f, 0.20f, 1.00f, 1.00f);
    colors[ImGuiCol_Button]             = ImVec4(0.24f, 0.16f, 0.38f, 1.00f);
    colors[ImGuiCol_ButtonHovered]      = ImVec4(0.38f, 0.14f, 0.73f, 1.00f);
    colors[ImGuiCol_ButtonActive]       = ImVec4(0.46f, 0.17f, 0.89f, 1.00f);
    colors[ImGuiCol_FrameBg]            = ImVec4(0.16f, 0.16f, 0.21f, 1.00f);
    colors[ImGuiCol_FrameBgHovered]     = ImVec4(0.24f, 0.24f, 0.32f, 1.00f);
    colors[ImGuiCol_FrameBgActive]      = ImVec4(0.31f, 0.31f, 0.40f, 1.00f);
    colors[ImGuiCol_SliderGrab]         = ImVec4(0.46f, 0.17f, 0.89f, 1.00f);
    colors[ImGuiCol_SliderGrabActive]   = ImVec4(0.53f, 0.20f, 1.00f, 1.00f);
    colors[ImGuiCol_CheckMark]          = ImVec4(0.53f, 0.20f, 1.00f, 1.00f);
    
    return true;
}

void cleanupGUI() {
    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    if (controlRenderer) {
        SDL_DestroyRenderer(controlRenderer);
        controlRenderer = nullptr;
    }
    if (controlWindow) {
        SDL_DestroyWindow(controlWindow);
        controlWindow = nullptr;
    }
}

void processGUIEvent(SDL_Event* event) {
    ImGui_ImplSDL2_ProcessEvent(event);
}

void updateControlWindowPosition() {
    if (controlWindow) {
        int x, y, w, h;
        SDL_GetWindowPosition(controlWindow, &x, &y);
        SDL_GetWindowSize(controlWindow, &w, &h);
        globalSettings.controlX = x;
        globalSettings.controlY = y;
        globalSettings.controlW = w;
        globalSettings.controlH = h;
    }
}

Uint32 getControlWindowID() {
    if (controlWindow) {
        return SDL_GetWindowID(controlWindow);
    }
    return 0;
}

void renderGUI() {
    ImGui_ImplSDLRenderer2_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();
    
    int w, h;
    SDL_GetWindowSize(controlWindow, &w, &h);
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(w, h));
    
    ImGui::Begin("Settings Panel", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);
    
    ImGui::TextColored(ImVec4(0.6f, 0.4f, 1.0f, 1.0f), "PNGTuber - Control Panel");
    ImGui::Separator();
    ImGui::Spacing();
    
    // 1. Microphone Settings
    if (ImGui::CollapsingHeader("Microphone settings", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Spacing();
        std::string currentMicName = globalSettings.activeMicName;
        if (ImGui::BeginCombo("Select microphone", currentMicName.c_str())) {
            for (int i = 0; i < (int)deviceList.size(); i++) {
                bool isSelected = (currentMicName == deviceList[i].name);
                if (ImGui::Selectable(deviceList[i].name.c_str(), isSelected)) {
                    switchMicrophone(i);
                }
                if (isSelected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
        
        ImGui::Spacing();
        ImGui::Text("Voice Threshold (type or drag red line below):");
        ImGui::SameLine();
        ImGui::PushItemWidth(100.0f);
        ImGui::InputFloat("##ThresholdInput", &currentSettings.threshold, 0.001f, 0.01f, "%.3f");
        ImGui::PopItemWidth();
        
        ImGui::Text("Live Volume Level (Drag red line to adjust threshold):");
        ImVec4 volColor = isTalking ? ImVec4(0.2f, 0.9f, 0.2f, 1.0f) : ImVec4(0.8f, 0.8f, 0.2f, 1.0f);
        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, volColor);
        
        float progressVal = currentVolume / 0.5f;
        if (progressVal > 1.0f) progressVal = 1.0f;
        if (progressVal < 0.0f) progressVal = 0.0f;
        
        ImGui::ProgressBar(progressVal, ImVec2(-1.0f, 24.0f), "");
        ImGui::PopStyleColor();
        
        ImVec2 pMin = ImGui::GetItemRectMin();
        ImVec2 pMax = ImGui::GetItemRectMax();
        
        // Handle dragging the threshold directly on the progress bar
        if (ImGui::IsItemHovered() || ImGui::IsItemActive()) {
            if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
                ImVec2 mousePos = ImGui::GetMousePos();
                float relativeX = (mousePos.x - pMin.x) / (pMax.x - pMin.x);
                if (relativeX < 0.0f) relativeX = 0.0f;
                if (relativeX > 1.0f) relativeX = 1.0f;
                currentSettings.threshold = relativeX * 0.5f;
            }
        }
        
        float tFraction = currentSettings.threshold / 0.5f;
        if (tFraction > 1.0f) tFraction = 1.0f;
        if (tFraction < 0.0f) tFraction = 0.0f;
        
        float lineX = pMin.x + (pMax.x - pMin.x) * tFraction;
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        drawList->AddLine(ImVec2(lineX, pMin.y), ImVec2(lineX, pMax.y), IM_COL32(255, 50, 50, 255), 3.0f);
        
        if (isTalking) {
            ImGui::TextColored(ImVec4(0.2f, 0.9f, 0.2f, 1.0f), "🎙️ Talking...");
        } else {
            ImGui::Text("🎙️ Silent");
        }
        ImGui::Spacing();
    }
    
    // 2. Character & Presets
    if (ImGui::CollapsingHeader("Character & Presets", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Spacing();
        std::string activePreset = globalSettings.currentPreset;
        if (ImGui::BeginCombo("Active Preset", activePreset.c_str())) {
            for (int i = 0; i < (int)presetList.size(); i++) {
                bool isSelected = (activePreset == presetList[i]);
                if (ImGui::Selectable(presetList[i].c_str(), isSelected)) {
                    applyAvatarPreset(presetList[i]);
                }
                if (isSelected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
        
        bool sizeChanged = false;
        ImGui::Text("Avatar Width:");
        ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.65f);
        if (ImGui::SliderInt("##WidthSlider", &currentSettings.w, 100, 1500)) sizeChanged = true;
        ImGui::PopItemWidth();
        ImGui::SameLine();
        ImGui::PushItemWidth(-FLT_MIN);
        if (ImGui::InputInt("##WidthInput", &currentSettings.w, 1, 10)) sizeChanged = true;
        ImGui::PopItemWidth();
        
        ImGui::Text("Avatar Height:");
        ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.65f);
        if (ImGui::SliderInt("##HeightSlider", &currentSettings.h, 100, 1500)) sizeChanged = true;
        ImGui::PopItemWidth();
        ImGui::SameLine();
        ImGui::PushItemWidth(-FLT_MIN);
        if (ImGui::InputInt("##HeightInput", &currentSettings.h, 1, 10)) sizeChanged = true;
        ImGui::PopItemWidth();
        
        if (ImGui::Button("Scale +10%")) {
            currentSettings.w = (int)(currentSettings.w * 1.10f);
            currentSettings.h = (int)(currentSettings.h * 1.10f);
            sizeChanged = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("Scale -10%")) {
            currentSettings.w = (int)(currentSettings.w * 0.90f);
            currentSettings.h = (int)(currentSettings.h * 0.90f);
            sizeChanged = true;
        }
        
        if (sizeChanged && avatarWindow) {
            SDL_SetWindowSize(avatarWindow, currentSettings.w, currentSettings.h);
        }
        
        ImGui::Spacing();
        ImGui::Text("Shake Intensity:");
        ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.65f);
        ImGui::SliderInt("##ShakeSlider", &currentSettings.shake, 0, 20);
        ImGui::PopItemWidth();
        ImGui::SameLine();
        ImGui::PushItemWidth(-FLT_MIN);
        ImGui::InputInt("##ShakeInput", &currentSettings.shake, 1, 5);
        ImGui::PopItemWidth();
        ImGui::Spacing();
    }
    
    // 3. Background Settings (OBS Integration)
    if (ImGui::CollapsingHeader("OBS Background Color", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Spacing();
        const char* bgOptions[] = { "Green Screen", "Blue Screen", "Magenta Screen", "Black", "White" };
        ImGui::Combo("Background Color", &globalSettings.bgColorMode, bgOptions, IM_ARRAYSIZE(bgOptions));
        
        ImGui::Spacing();
        ImGui::TextWrapped("💡 To use in OBS: Add a 'Window Capture (PipeWire)' or 'Window Capture (X11)' source targeting the avatar window, then add a 'Chroma Key' effect in OBS and select the matching color.");
        ImGui::Spacing();
    }
    
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    
    if (ImGui::Button("Save Settings", ImVec2(120, 30))) {
        savePresetSettings(globalSettings.currentPreset);
        
        int x, y;
        SDL_GetWindowPosition(controlWindow, &x, &y);
        globalSettings.controlX = x;
        globalSettings.controlY = y;
        globalSettings.controlW = w;
        globalSettings.controlH = h;
        saveGlobalSettings();
    }
    
    ImGui::SameLine();
    if (ImGui::Button("Exit Application", ImVec2(120, 30))) {
        SDL_Event quitEv;
        quitEv.type = SDL_QUIT;
        SDL_PushEvent(&quitEv);
    }
    
    ImGui::End();
    
    ImGui::Render();
    SDL_SetRenderDrawColor(controlRenderer, 0, 0, 0, 255);
    SDL_RenderClear(controlRenderer);
    ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), controlRenderer);
    SDL_RenderPresent(controlRenderer);
}
