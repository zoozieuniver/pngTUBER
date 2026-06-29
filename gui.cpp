#include "gui.h"
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer2.h"
#include "config.h"
#include "audio.h"
#include "avatar.h"
#include "localization.h"
#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <filesystem>

namespace fs = std::filesystem;

#ifdef _WIN32
#include <windows.h>
#include <commdlg.h>
#endif

std::string openFileDialog() {
#ifdef _WIN32
    char szFile[260] = { 0 };
    OPENFILENAMEA ofn;
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = "Images (*.png;*.jpg;*.jpeg;*.bmp)\0*.png;*.jpg;*.jpeg;*.bmp\0All Files (*.*)\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
    if (GetOpenFileNameA(&ofn)) {
        return std::string(szFile);
    }
    return "";
#else
    std::string path = "";
    char buffer[1024];
    FILE* pipe = nullptr;
    
    if (system("which zenity > /dev/null 2>&1") == 0) {
        pipe = popen("zenity --file-selection --title=\"Select Background Image\" --file-filter=\"Images | *.png *.jpg *.jpeg *.bmp\"", "r");
    } else if (system("which kdialog > /dev/null 2>&1") == 0) {
        pipe = popen("kdialog --getopenfilename . \"*.png *.jpg *.jpeg *.bmp |Images\"", "r");
    } else {
        std::cerr << "No zenity or kdialog found!" << std::endl;
        return "";
    }
    
    if (pipe) {
        if (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            path = buffer;
            if (!path.empty() && path.back() == '\n') {
                path.pop_back();
            }
        }
        pclose(pipe);
    }
    return path;
#endif
}

SDL_Window* controlWindow = nullptr;
SDL_Renderer* controlRenderer = nullptr;
static SDL_Texture* previewTexture = nullptr;
int savedControlW = 450;
int savedControlH = 550;
int selectedLayerIdx = -1;

bool initGUI() {
    savedControlW = globalSettings.controlW;
    savedControlH = globalSettings.controlH;
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
    io.FontGlobalScale = 1.25f;
    
    // Load DejaVuSans font to support Cyrillic characters
    ImFont* mainFont = nullptr;
    std::string fontPath = "/usr/share/fonts/dejavu/DejaVuSans.ttf";
    if (fs::exists(fontPath)) {
        ImFontConfig config;
        config.MergeMode = false;
        mainFont = io.Fonts->AddFontFromFileTTF(fontPath.c_str(), 15.0f, &config, io.Fonts->GetGlyphRangesCyrillic());
        
        // Merge Noto Sans CJK to support hieroglyphs if installed
        std::string cjkFontPath = "/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc";
        if (!fs::exists(cjkFontPath)) {
            cjkFontPath = "/usr/share/fonts/truetype/noto/NotoSansCJK-Regular.ttc";
        }
        if (!fs::exists(cjkFontPath)) {
            cjkFontPath = "assets/wqy-microhei.ttc";
        }
        if (fs::exists(cjkFontPath)) {
            config.MergeMode = true;
            io.Fonts->AddFontFromFileTTF(cjkFontPath.c_str(), 15.0f, &config, io.Fonts->GetGlyphRangesChineseFull());
        }
    }
    if (!mainFont) {
        io.Fonts->AddFontDefault();
    }
    
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
    if (previewTexture) {
        SDL_DestroyTexture(previewTexture);
        previewTexture = nullptr;
    }
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
        if (!editorModeActive) {
            globalSettings.controlW = w;
            globalSettings.controlH = h;
        }
    }
}

Uint32 getControlWindowID() {
    if (controlWindow) {
        return SDL_GetWindowID(controlWindow);
    }
    return 0;
}

#include <fstream>
#include <algorithm>
#include <cctype>
#include <vector>



static std::vector<std::string> getAvailableImages(const std::string& presetName) {
    std::vector<std::string> images;
    fs::path dir = fs::path("presets") / presetName;
    if (fs::exists(dir)) {
        for (const auto& entry : fs::directory_iterator(dir)) {
            if (entry.is_regular_file()) {
                std::string ext = entry.path().extension().string();
                std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return std::tolower(c); });
                if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp") {
                    images.push_back(entry.path().filename().string());
                }
            }
        }
    }
    return images;
}

static bool SliderIntWithReset(const char* label, int* value, int min_val, int max_val, int default_val) {
    ImGui::PushID(label);
    ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x - 70.0f);
    bool changed = ImGui::SliderInt("##slider", value, min_val, max_val);
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Ctrl+Click to type value manually");
    }
    ImGui::PopItemWidth();
    ImGui::SameLine();
    if (ImGui::Button("↺", ImVec2(24, 20))) {
        *value = default_val;
        changed = true;
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Reset to Default (%d)", default_val);
    }
    ImGui::SameLine();
    ImGui::Text("%s", label);
    ImGui::PopID();
    return changed;
}

static bool SliderFloatWithReset(const char* label, float* value, float min_val, float max_val, float default_val, const char* format = "%.2f") {
    ImGui::PushID(label);
    ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x - 70.0f);
    bool changed = ImGui::SliderFloat("##slider", value, min_val, max_val, format);
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Ctrl+Click to type value manually");
    }
    ImGui::PopItemWidth();
    ImGui::SameLine();
    if (ImGui::Button("↺", ImVec2(24, 20))) {
        *value = default_val;
        changed = true;
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Reset to Default");
    }
    ImGui::SameLine();
    ImGui::Text("%s", label);
    ImGui::PopID();
    return changed;
}

void renderGUI() {
    static bool forceLayersHeaderOpen = false;
    static int scrollFrames = 0;
    static bool dragEntireCharacter = false;
    
    ImGui_ImplSDLRenderer2_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();
    
    int w, h;
    SDL_GetWindowSize(controlWindow, &w, &h);
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(w, h));
    
    ImGui::Begin("Settings Panel", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);
    
    ImGui::TextColored(ImVec4(0.6f, 0.4f, 1.0f, 1.0f), "%s", tr("PNGTuber - Control Panel", "PNGTuber - Панель керування"));
    ImGui::SameLine(ImGui::GetWindowWidth() - 170.0f);
    if (ImGui::Button(globalSettings.language == 1 ? "English" : "Українська", ImVec2(150.0f, 25.0f))) {
        globalSettings.language = (globalSettings.language == 1) ? 0 : 1;
        saveGlobalSettings();
    }
    ImGui::Separator();
    ImGui::Spacing();
    
    ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
    if (ImGui::BeginTabBar("MainTabBar", tab_bar_flags)) {
        
        // ==========================================
        // TAB 1: CONTROL PANEL
        // ==========================================
        if (ImGui::BeginTabItem(tr("Control Panel", "Панель керування"))) {
            if (editorModeActive) {
                SDL_SetWindowSize(controlWindow, savedControlW, savedControlH);
                setEditorMode(false);
            }
            
            // Microphone settings
            if (ImGui::CollapsingHeader(tr("Microphone settings", "Налаштування мікрофона"), ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::Spacing();
                std::string currentMicName = globalSettings.activeMicName;
                if (ImGui::BeginCombo(tr("Select microphone", "Виберіть мікрофон"), currentMicName.c_str())) {
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
                ImGui::Text("%s", tr("Voice Threshold (type or drag red line below):", "Поріг гучності голосу (введіть або перетягніть червону лінію нижче):"));
                ImGui::SameLine();
                ImGui::PushItemWidth(100.0f);
                ImGui::InputFloat("##ThresholdInput", &currentSettings.threshold, 0.001f, 0.01f, "%.3f");
                ImGui::PopItemWidth();
                ImGui::SameLine();
                if (ImGui::Button("↺##ThresholdReset", ImVec2(24, 20))) {
                    currentSettings.threshold = 0.05f;
                }
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("%s", tr("Reset to Default", "Скинути до початкових значень"));
                }
                
                ImGui::Text("%s", tr("Live Volume Level (Drag red line to adjust threshold):", "Поточний рівень гучності (перетягніть червону лінію для зміни порогу):"));
                ImVec4 volColor = isTalking ? ImVec4(0.2f, 0.9f, 0.2f, 1.0f) : ImVec4(0.8f, 0.8f, 0.2f, 1.0f);
                ImGui::PushStyleColor(ImGuiCol_PlotHistogram, volColor);
                
                float progressVal = currentVolume / 0.5f;
                if (progressVal > 1.0f) progressVal = 1.0f;
                if (progressVal < 0.0f) progressVal = 0.0f;
                
                ImGui::ProgressBar(progressVal, ImVec2(-1.0f, 24.0f), "");
                ImGui::PopStyleColor();
                
                ImVec2 pMin = ImGui::GetItemRectMin();
                ImVec2 pMax = ImGui::GetItemRectMax();
                
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
                    ImGui::TextColored(ImVec4(0.2f, 0.9f, 0.2f, 1.0f), "%s", tr("Talking...", "Розмовляє..."));
                } else {
                    ImGui::Text("%s", tr("Silent", "Мовчить"));
                }
                ImGui::Spacing();
            }
            
            // OBS Background Color
            if (ImGui::CollapsingHeader(tr("OBS Background Color", "Колір фону OBS"), ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::Spacing();
                const char* bgOptions[] = { 
                    tr("Green Screen", "Зелений екран"), 
                    tr("Blue Screen", "Синій екран"), 
                    tr("Transparent / No Background", "Прозорий фоновий режим"), 
                    tr("Custom Color", "Власний колір"), 
                    tr("Custom Image", "Власне зображення") 
                };
                
                int prevMode = globalSettings.bgColorMode;
                if (ImGui::Combo(tr("Background Color", "Колір фону"), &globalSettings.bgColorMode, bgOptions, IM_ARRAYSIZE(bgOptions))) {
                    if (prevMode != globalSettings.bgColorMode) {
                        updateWindowTransparency();
                        if (globalSettings.bgColorMode == 4) {
                            loadCustomBgTexture(globalSettings.customBgImagePath);
                        }
                    }
                }
                
                if (globalSettings.bgColorMode == 3) {
                    ImGui::Spacing();
                    ImGui::Text("%s", tr("Choose Custom Color (RGBA):", "Оберіть власний колір (RGBA):"));
                    ImGui::ColorEdit4("##CustomColorPicker", globalSettings.customBgColor);
                } else if (globalSettings.bgColorMode == 4) {
                    ImGui::Spacing();
                    ImGui::Text("%s", tr("Custom Background Image Path:", "Шлях до власного зображення фону:"));
                    
                    static char pathBuf[512] = "";
                    static bool initBuf = false;
                    if (!initBuf || strcmp(pathBuf, globalSettings.customBgImagePath.c_str()) != 0) {
                        strncpy(pathBuf, globalSettings.customBgImagePath.c_str(), sizeof(pathBuf));
                        pathBuf[sizeof(pathBuf) - 1] = '\0';
                        initBuf = true;
                    }
                    
                    ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.60f);
                    ImGui::InputText("##BgImagePath", pathBuf, IM_ARRAYSIZE(pathBuf));
                    ImGui::PopItemWidth();
                    
                    ImGui::SameLine();
                    if (ImGui::Button(tr("Browse...", "Огляд..."))) {
                        std::string chosenPath = openFileDialog();
                        if (!chosenPath.empty()) {
                            strncpy(pathBuf, chosenPath.c_str(), sizeof(pathBuf));
                            pathBuf[sizeof(pathBuf) - 1] = '\0';
                            globalSettings.customBgImagePath = chosenPath;
                            loadCustomBgTexture(globalSettings.customBgImagePath);
                            adjustWindowToBgTextureSize();
                        }
                    }
                    
                    ImGui::SameLine();
                    if (ImGui::Button(tr("Load", "Завантажити"))) {
                        globalSettings.customBgImagePath = pathBuf;
                        loadCustomBgTexture(globalSettings.customBgImagePath);
                        adjustWindowToBgTextureSize();
                    }
                    ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "%s", tr("Click 'Browse...' to select an image, or enter path manually and click 'Load'", "Натисніть 'Огляд...', щоб вибрати зображення, або введіть шлях вручную та натисніть 'Завантажити'"));
                }
                
                ImGui::Spacing();
                ImGui::TextWrapped("%s", tr("To use in OBS: Add a 'Window Capture (PipeWire)' or 'Window Capture (X11)' source targeting the avatar window, then add a 'Chroma Key' effect in OBS and select the matching color (or use 'Transparent' mode directly if supported).", "Для використання в OBS: Додайте джерело 'Захоплення вікна (PipeWire)' або 'Захоплення вікна (X11)' з вікном аватара, після чого додайте ефект 'Хромакей' в OBS та виберіть відповідний колір (або використовуйте режим 'Прозорий', якщо підтримується)."));
                ImGui::Spacing();
            }
            
            if (ImGui::CollapsingHeader(tr("Character Position Offset", "Зміщення позиції персонажа"), ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::Spacing();
                SliderIntWithReset(tr("Avatar Position X Offset", "Зміщення позиції аватара X"), &currentSettings.avatarX, -1000, 1000, 0);
                SliderIntWithReset(tr("Avatar Position Y Offset", "Зміщення позиції аватара Y"), &currentSettings.avatarY, -1000, 1000, 0);
                ImGui::Spacing();
            }
            
            ImGui::EndTabItem();
        }
        
        // ==========================================
        // TAB 2: LAYER EDITOR
        // ==========================================
        if (ImGui::BeginTabItem(tr("Layer Editor", "Редактор шарів"))) {
            if (!editorModeActive) {
                SDL_GetWindowSize(controlWindow, &savedControlW, &savedControlH);
                SDL_SetWindowSize(controlWindow, 900, 650);
                setEditorMode(true);
            }
            
            ImGui::Columns(2, "EditorColumns", true);
            
            // --- COLUMN 1: LIVE PREVIEW ---
            ImGui::TextColored(ImVec4(0.2f, 0.9f, 0.2f, 1.0f), "%s", tr("Live Character Preview", "Попередній перегляд персонажа"));
            ImGui::Spacing();
            
            ImGui::Text("%s", tr("Volume:", "Гучність:"));
            ImGui::SameLine();
            float progressVal = currentVolume / 0.5f;
            if (progressVal > 1.0f) progressVal = 1.0f;
            if (progressVal < 0.0f) progressVal = 0.0f;
            ImGui::ProgressBar(progressVal, ImVec2(-1.0f, 15.0f), "");
            
            ImGui::Spacing();
            
            float colWidth = ImGui::GetColumnWidth() - 20.0f;
            float previewSize = colWidth;
            if (previewSize > 550.0f) previewSize = 550.0f;
            if (previewSize < 100.0f) previewSize = 100.0f;
            
            int texW = (int)previewSize;
            int texH = (int)previewSize;
            
            static int prevTexW = 0, prevTexH = 0;
            if (!previewTexture || prevTexW != texW || prevTexH != texH) {
                if (previewTexture) {
                    SDL_DestroyTexture(previewTexture);
                }
                previewTexture = SDL_CreateTexture(controlRenderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, texW, texH);
                prevTexW = texW;
                prevTexH = texH;
            }
            
            if (previewTexture) {
                renderAvatarPreviewToTexture(previewTexture, texW, texH);
                
                ImVec2 screenPos = ImGui::GetCursorScreenPos();
                ImGui::Image((void*)previewTexture, ImVec2(previewSize, previewSize));
                
                // Left-mouse interaction: Dragging and selection
                bool isHovered = ImGui::IsItemHovered();
                static bool isDragging = false;
                
                if (isHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                    isDragging = true;
                    ImVec2 mousePos = ImGui::GetMousePos();
                    float relX = mousePos.x - screenPos.x;
                    float relY = mousePos.y - screenPos.y;
                    
                    float texMouseX = relX * ((float)texW / previewSize);
                    float texMouseY = relY * ((float)texH / previewSize);
                    
                    int hitIdx = hitTestLayers((int)texMouseX, (int)texMouseY, texW, texH);
                    if (hitIdx != -1) {
                        selectedLayerIdx = hitIdx;
                        forceLayersHeaderOpen = true;
                        scrollFrames = 2;
                    }
                }
                
                if (isDragging) {
                    if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
                        ImVec2 dragDelta = ImGui::GetIO().MouseDelta;
                        if (dragDelta.x != 0.0f || dragDelta.y != 0.0f) {
                            float scaleX_fit = (float)texW / (float)currentSettings.winW;
                            float scaleY_fit = (float)texH / (float)currentSettings.winH;
                            float previewScale = std::min(scaleX_fit, scaleY_fit);
                            float effZoom = previewZoom;
                            
                            if (dragEntireCharacter) {
                                // Drag entire character globally
                                if (previewScale * effZoom > 0.0f) {
                                    currentSettings.avatarX += (int)(dragDelta.x / (previewScale * effZoom));
                                    currentSettings.avatarY -= (int)(dragDelta.y / (previewScale * effZoom));
                                }
                            } else if (selectedLayerIdx >= 0 && selectedLayerIdx < (int)currentLayers.size()) {
                                // Drag individual selected layer
                                auto& layer = currentLayers[selectedLayerIdx];
                                
                                float W_neutral = (float)currentSettings.w;
                                float H_neutral = (float)currentSettings.h;
                                const float RefW = 400.0f;
                                const float RefH = 400.0f;
                                
                                float avatarScaleX = W_neutral / RefW;
                                float avatarScaleY = H_neutral / RefH;
                                
                                float lScaleX = layer.overridePhysics ? layer.scaleX : 1.0f;
                                float lScaleY = layer.overridePhysics ? layer.scaleY : 1.0f;
                                
                                float totalScaleX = avatarScaleX * lScaleX * previewScale * effZoom;
                                float totalScaleY = avatarScaleY * lScaleY * previewScale * effZoom;
                                
                                if (totalScaleX > 0.0f && totalScaleY > 0.0f) {
                                    layer.x += (int)(dragDelta.x / totalScaleX);
                                    layer.y -= (int)(dragDelta.y / totalScaleY);
                                }
                            }
                        }
                    } else {
                        isDragging = false;
                    }
                }
                
                // Track panning with right-mouse drag
                static bool isPanning = false;
                if (isHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
                    isPanning = true;
                }
                if (isPanning) {
                    if (ImGui::IsMouseDown(ImGuiMouseButton_Right)) {
                        ImVec2 dragDelta = ImGui::GetIO().MouseDelta;
                        previewPanX += dragDelta.x;
                        previewPanY += dragDelta.y;
                    } else {
                        isPanning = false;
                    }
                }
                
                // Track zoom with scroll wheel
                if (isHovered) {
                    float wheel = ImGui::GetIO().MouseWheel;
                    if (wheel != 0.0f) {
                        previewZoom += wheel * 0.1f;
                        if (previewZoom < 0.1f) previewZoom = 0.1f;
                        if (previewZoom > 10.0f) previewZoom = 10.0f;
                    }
                }
                
                ImGui::GetWindowDrawList()->AddRect(screenPos, ImVec2(screenPos.x + previewSize, screenPos.y + previewSize), IM_COL32(100, 100, 100, 255), 0.0f, 0, 1.5f);
            }
            
            ImGui::Spacing();
            
            // Zoom Slider with Reset View button
            ImGui::Text("%s", tr("Preview Zoom:", "Масштаб прев'ю:"));
            ImGui::SameLine();
            ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x - 120.0f);
            ImGui::SliderFloat("##ZoomSlider", &previewZoom, 0.2f, 5.0f, "%.2f");
            ImGui::PopItemWidth();
            ImGui::SameLine();
            if (ImGui::Button(tr("Reset View", "Скинути вигляд"))) {
                previewZoom = 1.0f;
                previewPanX = 0.0f;
                previewPanY = 0.0f;
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("%s", tr("Reset Zoom and Pan", "Скинути масштаб і зміщення"));
            }
            
            ImGui::Spacing();
            ImGui::Checkbox(tr("Drag Entire Character on Preview", "Перетягувати всього персонажа на прев'ю"), &dragEntireCharacter);
            
            ImGui::Spacing();
            ImGui::Text(tr("Talking status: %s", "Статус розмови: %s"), isTalking ? tr("Talking", "Розмовляє") : tr("Silent", "Мовчить"));
            ImGui::SameLine();
            if (ImGui::Button(tr("Trigger Speak", "Тест розмови"))) {
                isTalking = !isTalking;
            }
            
            ImGui::NextColumn();
            
            // --- COLUMN 2: CONTROLS & LAYERS ---
            ImGui::BeginChild("EditorControlsScroll", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
            
            // 1. Preset Management
            if (ImGui::CollapsingHeader(tr("Preset Management", "Керування пресетами"), 0)) {
                // Selector
                std::string activePreset = globalSettings.currentPreset;
                if (ImGui::BeginCombo(tr("Active Preset", "Активний пресет"), activePreset.c_str())) {
                    for (int i = 0; i < (int)presetList.size(); i++) {
                        bool isSelected = (activePreset == presetList[i]);
                        if (ImGui::Selectable(presetList[i].c_str(), isSelected)) {
                            applyAvatarPreset(presetList[i]);
                            selectedLayerIdx = -1;
                        }
                        if (isSelected) {
                            ImGui::SetItemDefaultFocus();
                        }
                    }
                    ImGui::EndCombo();
                }
                
                ImGui::Spacing();
                
                // Create preset
                static char newPresetNameBuf[64] = "";
                ImGui::InputText(tr("New Preset Name", "Назва нового пресету"), newPresetNameBuf, sizeof(newPresetNameBuf));
                ImGui::SameLine();
                if (ImGui::Button(tr("Create", "Створити"))) {
                    std::string newName = newPresetNameBuf;
                    if (!newName.empty()) {
                        fs::path newDir = fs::path("presets") / newName;
                        if (!fs::exists(newDir)) {
                            fs::create_directories(newDir);
                            PresetSettings defaultSettings;
                            std::ofstream out(newDir / "settings.txt");
                            if (out.is_open()) {
                                out << defaultSettings.x << " " << defaultSettings.y << " " << defaultSettings.w << " " << defaultSettings.h << " " << defaultSettings.shake << " " << defaultSettings.threshold << " 1 15.0 1.0 1 1.0 1.0";
                            }
                            std::ofstream outL(newDir / "layers.txt");
                            scanPresets();
                            applyAvatarPreset(newName);
                            selectedLayerIdx = -1;
                            newPresetNameBuf[0] = '\0';
                        }
                    }
                }
                
                // Rename preset
                static char renameBuf[64] = "";
                static std::string lastActivePreset = "";
                if (globalSettings.currentPreset != lastActivePreset) {
                    strncpy(renameBuf, globalSettings.currentPreset.c_str(), sizeof(renameBuf));
                    renameBuf[sizeof(renameBuf) - 1] = '\0';
                    lastActivePreset = globalSettings.currentPreset;
                }
                
                ImGui::InputText(tr("Rename Preset To", "Перейменувати пресет на"), renameBuf, sizeof(renameBuf));
                ImGui::SameLine();
                if (ImGui::Button(tr("Rename", "Перейменувати"))) {
                    std::string newName = renameBuf;
                    std::string oldName = globalSettings.currentPreset;
                    if (!newName.empty() && newName != oldName) {
                        fs::path oldDir = fs::path("presets") / oldName;
                        fs::path newDir = fs::path("presets") / newName;
                        if (fs::exists(oldDir) && !fs::exists(newDir)) {
                            fs::rename(oldDir, newDir);
                            globalSettings.currentPreset = newName;
                            scanPresets();
                            saveGlobalSettings();
                            applyAvatarPreset(newName);
                            selectedLayerIdx = -1;
                        }
                    }
                }
                ImGui::Spacing();
            }
            
            // 2. Character Global Properties
            if (ImGui::CollapsingHeader(tr("Character Global Settings", "Глобальні налаштування персонажа"), 0)) {
                SliderIntWithReset(tr("Avatar Width", "Ширина аватара"), &currentSettings.w, 100, 1500, 400);
                SliderIntWithReset(tr("Avatar Height", "Висота аватара"), &currentSettings.h, 100, 1500, 400);
                
                if (ImGui::Button(tr("Scale +10%", "Масштаб +10%"))) {
                    currentSettings.w = (int)(currentSettings.w * 1.10f);
                    currentSettings.h = (int)(currentSettings.h * 1.10f);
                }
                ImGui::SameLine();
                if (ImGui::Button(tr("Scale -10%", "Масштаб -10%"))) {
                    currentSettings.w = (int)(currentSettings.w * 0.90f);
                    currentSettings.h = (int)(currentSettings.h * 0.90f);
                }
                
                SliderIntWithReset(tr("Avatar Position X Offset", "Зміщення позиції аватара X"), &currentSettings.avatarX, -1000, 1000, 0);
                SliderIntWithReset(tr("Avatar Position Y Offset", "Зміщення позиції аватара Y"), &currentSettings.avatarY, -1000, 1000, 0);
                
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Text("%s", tr("Window Dimensions:", "Розміри вікна:"));
                
                bool winSizeChanged = false;
                if (SliderIntWithReset(tr("Window Width", "Ширина вікна"), &currentSettings.winW, 100, 2500, 400)) {
                    winSizeChanged = true;
                }
                if (SliderIntWithReset(tr("Window Height", "Висота вікна"), &currentSettings.winH, 100, 2500, 400)) {
                    winSizeChanged = true;
                }
                if (winSizeChanged) {
                    updateAvatarWindowSize();
                }
                
                ImGui::Spacing();
                ImGui::Separator();
                
                SliderIntWithReset(tr("Shake Intensity (Speech Tremor)", "Інтенсивність тряски (тремор при розмові)"), &currentSettings.shake, 0, 20, 5);
                
                ImGui::Spacing();
                ImGui::Checkbox(tr("Enable Jumping", "Увімкнути стрибки"), &currentSettings.jumpEnabled);
                if (currentSettings.jumpEnabled) {
                    SliderFloatWithReset(tr("Jump Height", "Висота стрибка"), &currentSettings.jumpHeight, 1.0f, 150.0f, 15.0f, "%.1f");
                    SliderFloatWithReset(tr("Jump Gravity/Speed", "Гравітація/Швидкість стрибка"), &currentSettings.jumpSpeed, 0.1f, 3.0f, 1.0f);
                }
                
                ImGui::Spacing();
                ImGui::Checkbox(tr("Enable Jelly (Squash/Stretch)", "Увімкнути желе (стискання/розтягування)"), &currentSettings.jellyEnabled);
                if (currentSettings.jellyEnabled) {
                    SliderFloatWithReset(tr("Jelly Intensity", "Інтенсивність желе"), &currentSettings.jellyIntensity, 0.1f, 3.0f, 1.0f);
                    SliderFloatWithReset(tr("Jelly Elasticity/Speed", "Еластичність/Швидкість желе"), &currentSettings.jellySpeed, 0.1f, 3.0f, 1.0f);
                }
                ImGui::Spacing();
            }
            
            // 3. File Loader / Importer
            if (ImGui::CollapsingHeader(tr("Import Files to Preset", "Імпортувати файли в пресет"), 0)) {
                if (ImGui::Button(tr("Copy External Image File to Preset Folder", "Скопіювати зовнішній файл зображення в папку пресету"))) {
                    std::string srcPath = openFileDialog();
                    if (!srcPath.empty()) {
                        fs::path src = srcPath;
                        fs::path dest = fs::path("presets") / globalSettings.currentPreset / src.filename();
                        try {
                            fs::copy_file(src, dest, fs::copy_options::overwrite_existing);
                            std::cout << "Successfully copied file: " << dest << std::endl;
                        } catch (const std::exception& e) {
                            std::cerr << "File copy failed: " << e.what() << std::endl;
                        }
                    }
                }
                ImGui::Spacing();
                
                std::vector<std::string> presetImages = getAvailableImages(globalSettings.currentPreset);
                static int selectedImgIdx = 0;
                if (selectedImgIdx >= (int)presetImages.size()) selectedImgIdx = 0;
                
                if (!presetImages.empty()) {
                    std::string currentSel = presetImages[selectedImgIdx];
                    if (ImGui::BeginCombo(tr("Preset Files", "Файли пресету"), currentSel.c_str())) {
                        for (int i = 0; i < (int)presetImages.size(); ++i) {
                            bool isSel = (i == selectedImgIdx);
                            if (ImGui::Selectable(presetImages[i].c_str(), isSel)) {
                                selectedImgIdx = i;
                            }
                        }
                        ImGui::EndCombo();
                    }
                    
                    ImGui::SameLine();
                    if (ImGui::Button(tr("Add as Layer", "Додати як шар"))) {
                        std::string file = presetImages[selectedImgIdx];
                        bool exists = false;
                        for (const auto& l : currentLayers) {
                            if (l.filename == file) {
                                exists = true;
                                break;
                            }
                        }
                        if (!exists) {
                            AvatarLayer newLayer;
                            newLayer.filename = file;
                            newLayer.name = fs::path(file).stem().string();
                            newLayer.type = 0;
                            newLayer.x = 0;
                            newLayer.y = 0;
                            newLayer.w = 400; // placeholder
                            newLayer.h = 400;
                            newLayer.z = (int)currentLayers.size();
                            newLayer.visible = true;
                            newLayer.overridePhysics = false;
                            newLayer.jumpEnabled = true;
                            newLayer.jumpHeight = 15.0f;
                            newLayer.jumpSpeed = 1.0f;
                            newLayer.jellyEnabled = true;
                            newLayer.jellyIntensity = 1.0f;
                            newLayer.jellySpeed = 1.0f;
                            newLayer.shake = 5;
                            newLayer.animType = 0;
                            newLayer.animSpeed = 1.0f;
                            newLayer.animAmp = 1.0f;
                            newLayer.hotkey = "";
                            newLayer.texAvatar = nullptr;
                            newLayer.texControl = nullptr;
                            
                            currentLayers.push_back(newLayer);
                            reloadLayerTextures();
                            
                            auto& added = currentLayers.back();
                            if (added.texControl) {
                                int imgW = 400, imgH = 400;
                                SDL_QueryTexture(added.texControl, NULL, NULL, &imgW, &imgH);
                                added.w = imgW;
                                added.h = imgH;
                            }
                            selectedLayerIdx = (int)currentLayers.size() - 1;
                        }
                    }
                } else {
                    ImGui::TextColored(ImVec4(0.8f, 0.4f, 0.4f, 1.0f), "%s", tr("No image files inside preset folder yet.", "В папці пресету ще немає зображень."));
                }
                ImGui::Spacing();
            }
            
            // 4. Layers Settings
            if (forceLayersHeaderOpen) {
                ImGui::SetNextItemOpen(true);
                forceLayersHeaderOpen = false;
            }
            if (ImGui::CollapsingHeader(tr("Layers List & Stacking", "Список шарів та сортування"), ImGuiTreeNodeFlags_DefaultOpen)) {
                std::vector<int> listIndices(currentLayers.size());
                for (size_t i = 0; i < currentLayers.size(); ++i) listIndices[i] = i;
                std::sort(listIndices.begin(), listIndices.end(), [](int a, int b) {
                    return currentLayers[a].z > currentLayers[b].z;
                });
                
                ImGui::Text("%s", tr("Edit and stack layers (Top to Bottom):", "Редагувати та сортувати шари (Зверху вниз):"));
                ImGui::Spacing();
                
                ImGui::BeginChild("LayersTileScroll", ImVec2(0, 250.0f), true);
                for (size_t i = 0; i < listIndices.size(); ++i) {
                    int idx = listIndices[i];
                    auto& layer = currentLayers[idx];
                    ImGui::PushID(idx);
                    
                    bool isSelected = (idx == selectedLayerIdx);
                    if (isSelected && scrollFrames > 0) {
                        ImGui::SetScrollHereY(0.5f);
                    }
                    
                    ImGui::BeginGroup();
                    
                    // 1. Visibility toggle
                    bool vis = layer.visible;
                    if (ImGui::Button(vis ? "👁" : "   ", ImVec2(24, 24))) {
                        layer.visible = !layer.visible;
                    }
                    if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", tr("Toggle Visibility", "Перемкнути видимість"));
                    
                    ImGui::SameLine();
                    
                    // 2. Thumbnail
                    ImVec2 thumbPos = ImGui::GetCursorScreenPos();
                    if (layer.texControl) {
                        ImGui::Image((void*)layer.texControl, ImVec2(24, 24));
                    } else {
                        ImGui::Dummy(ImVec2(24, 24));
                        ImGui::GetWindowDrawList()->AddRectFilled(thumbPos, ImVec2(thumbPos.x + 24, thumbPos.y + 24), IM_COL32(80, 80, 80, 255));
                    }
                    
                    ImGui::SameLine();
                    
                    // 3. Name & Type
                    char label[256];
                    const char* typeStr = tr("Base", "Статичний");
                    if (layer.type == 1) typeStr = tr("Mouth Open", "Рот Відкритий");
                    else if (layer.type == 2) typeStr = tr("Mouth Closed", "Рот Закритий");
                    else if (layer.type == 3) typeStr = tr("Eyes Open", "Очі Відкриті");
                    else if (layer.type == 4) typeStr = tr("Eyes Closed", "Очі Закриті");
                    else if (layer.type == 5) typeStr = tr("Hair Front", "Волосся Спереду");
                    else if (layer.type == 6) typeStr = tr("Hair Back", "Волосся Ззаду");
                    
                    snprintf(label, sizeof(label), "%s (%s)##%d", layer.name.c_str(), typeStr, idx);
                    
                    float widthRemaining = ImGui::GetContentRegionAvail().x - 60.0f;
                    if (ImGui::Selectable(label, isSelected, ImGuiSelectableFlags_None, ImVec2(widthRemaining, 24))) {
                        selectedLayerIdx = idx;
                    }
                    
                    ImGui::SameLine();
                    
                    // 4. Reordering buttons
                    if (ImGui::Button("▲", ImVec2(22, 24))) {
                        if (i > 0) {
                            int otherIdx = listIndices[i - 1];
                            int tempZ = currentLayers[idx].z;
                            currentLayers[idx].z = currentLayers[otherIdx].z;
                            currentLayers[otherIdx].z = tempZ;
                            if (currentLayers[idx].z == currentLayers[otherIdx].z) {
                                currentLayers[idx].z++;
                            }
                        }
                    }
                    if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", tr("Move Layer Up (Bring Forward)", "Перемістити шар вгору (на передній план)"));
                    
                    ImGui::SameLine();
                    
                    if (ImGui::Button("▼", ImVec2(22, 24))) {
                        if (i < listIndices.size() - 1) {
                            int otherIdx = listIndices[i + 1];
                            int tempZ = currentLayers[idx].z;
                            currentLayers[idx].z = currentLayers[otherIdx].z;
                            currentLayers[otherIdx].z = tempZ;
                            if (currentLayers[idx].z == currentLayers[otherIdx].z) {
                                currentLayers[idx].z--;
                            }
                        }
                    }
                    if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", tr("Move Layer Down (Send Backward)", "Перемістити шар вниз (на задній план)"));
                    
                    ImGui::EndGroup();
                    ImGui::PopID();
                    ImGui::Separator();
                }
                ImGui::EndChild();
                
                ImGui::Spacing();
                
                if (selectedLayerIdx >= 0 && selectedLayerIdx < (int)currentLayers.size()) {
                    if (scrollFrames > 0) {
                        ImGui::SetScrollHereY(0.0f);
                        scrollFrames--;
                    }
                    auto& layer = currentLayers[selectedLayerIdx];
                    ImGui::PushID("SelectedLayerProps");
                    
                    ImGui::TextColored(ImVec4(0.38f, 0.73f, 1.0f, 1.0f), tr("Selected Layer Properties: %s", "Властивості вибраного шару: %s"), layer.filename.c_str());
                    ImGui::Spacing();
                    
                    char nameBuf[128];
                    strncpy(nameBuf, layer.name.c_str(), sizeof(nameBuf));
                    nameBuf[sizeof(nameBuf) - 1] = '\0';
                    if (ImGui::InputText(tr("Layer Name", "Назва шару"), nameBuf, sizeof(nameBuf))) {
                        layer.name = nameBuf;
                    }
                    
                    const char* typeOptions[] = {
                        tr("0: Base (Static)", "0: Статичний (Основний)"),
                        tr("1: Mouth Open (Talking)", "1: Відкритий рот (Розмова)"),
                        tr("2: Mouth Closed (Silent)", "2: Закритий рот (Мовчання)"),
                        tr("3: Eyes Open", "3: Відкриті очі"),
                        tr("4: Eyes Closed", "4: Закриті очі"),
                        tr("5: Hair Front (Jiggle)", "5: Переднє волосся (Коливання)"),
                        tr("6: Hair Back (Jiggle)", "6: Заднє волосся (Коливання)")
                    };
                    ImGui::Combo(tr("Behavior Type", "Тип поведінки"), &layer.type, typeOptions, IM_ARRAYSIZE(typeOptions));
                    
                    SliderIntWithReset(tr("Offset X", "Зміщення X"), &layer.x, -1000, 1000, 0);
                    SliderIntWithReset(tr("Offset Y", "Зміщення Y"), &layer.y, -1000, 1000, 0);
                    SliderIntWithReset(tr("Width", "Ширина"), &layer.w, 10, 2000, 400);
                    SliderIntWithReset(tr("Height", "Висота"), &layer.h, 10, 2000, 400);
                    SliderIntWithReset(tr("Depth (Z-index)", "Глибина (Z-index)"), &layer.z, -100, 100, 0);
                    
                    // Idle behavior settings
                    ImGui::Spacing();
                    ImGui::Text("%s", tr("Idle Animation & Hotkey Settings", "Налаштування анімації спокою та гарячої клавіші"));
                    ImGui::Indent();
                    
                    const char* animOptions[] = {
                        tr("0: None (Static)", "0: Немає (Статичний)"),
                        tr("1: Breathe (Scale loop)", "1: Дихання (Масштабування)"),
                        tr("2: Float (Vertical offset)", "2: Політ (Зміщення Y)"),
                        tr("3: Wobble (Angular oscillation)", "3: Похитування (Кутове коливання)"),
                        tr("4: Spin (Continuous rotation)", "4: Обертання (Повний оберт)")
                    };
                    ImGui::Combo(tr("Idle Animation Type", "Тип анімації спокою"), &layer.animType, animOptions, IM_ARRAYSIZE(animOptions));
                    
                    if (layer.animType > 0) {
                        SliderFloatWithReset(tr("Animation Speed", "Швидкість анімації"), &layer.animSpeed, 0.1f, 5.0f, 1.0f);
                        SliderFloatWithReset(tr("Animation Amplitude", "Амплітуда анімації"), &layer.animAmp, 0.1f, 5.0f, 1.0f);
                    }
                    
                    char hkBuf[8] = "";
                    strncpy(hkBuf, layer.hotkey.c_str(), sizeof(hkBuf));
                    hkBuf[sizeof(hkBuf) - 1] = '\0';
                    if (ImGui::InputText(tr("Visibility Hotkey", "Клавіша видимості"), hkBuf, sizeof(hkBuf))) {
                        std::string hkStr = hkBuf;
                        if (!hkStr.empty()) {
                            hkStr.erase(std::remove_if(hkStr.begin(), hkStr.end(), ::isspace), hkStr.end());
                        }
                        layer.hotkey = hkStr;
                    }
                    if (ImGui::IsItemHovered()) {
                        ImGui::SetTooltip("%s", tr("Press this key to toggle visibility of this layer in live mode", "Натисніть цю клавішу в живому режимі для увімкнення/вимкнення шару"));
                    }
                    
                    ImGui::Unindent();
                    ImGui::Spacing();
                    
                    ImGui::Spacing();
                    ImGui::Checkbox(tr("Override Global Physics", "Перевизначити глобальну фізику"), &layer.overridePhysics);
                    if (layer.overridePhysics) {
                        ImGui::Indent();
                        ImGui::Checkbox(tr("Enable Layer Jumping", "Увімкнути стрибки шару"), &layer.jumpEnabled);
                        if (layer.jumpEnabled) {
                            SliderFloatWithReset(tr("Layer Jump Height", "Висота стрибка шару"), &layer.jumpHeight, 1.0f, 150.0f, 15.0f, "%.1f");
                            SliderFloatWithReset(tr("Layer Jump Gravity/Speed", "Гравітація/Швидкість стрибка шару"), &layer.jumpSpeed, 0.1f, 3.0f, 1.0f);
                        }
                        
                        ImGui::Spacing();
                        ImGui::Checkbox(tr("Enable Layer Jelly (Squash/Stretch)", "Увімкнути желе шару (стискання/розтягування)"), &layer.jellyEnabled);
                        if (layer.jellyEnabled) {
                            SliderFloatWithReset(tr("Layer Jelly Intensity", "Інтенсивність желе шару"), &layer.jellyIntensity, 0.1f, 3.0f, 1.0f);
                            SliderFloatWithReset(tr("Layer Jelly Elasticity/Speed", "Еластичність/Швидкість желе шару"), &layer.jellySpeed, 0.1f, 3.0f, 1.0f);
                        }
                        
                        ImGui::Spacing();
                        SliderIntWithReset(tr("Layer Shake Intensity", "Інтенсивність тряски шару"), &layer.shake, 0, 20, 5);
                        ImGui::Unindent();
                    }
                    
                    ImGui::Spacing();
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.1f, 0.1f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.9f, 0.3f, 0.3f, 1.0f));
                    if (ImGui::Button(tr("Delete Layer", "Видалити шар"))) {
                        if (layer.texAvatar) SDL_DestroyTexture(layer.texAvatar);
                        if (layer.texControl) SDL_DestroyTexture(layer.texControl);
                        currentLayers.erase(currentLayers.begin() + selectedLayerIdx);
                        selectedLayerIdx = -1;
                    }
                    ImGui::PopStyleColor(3);
                    
                    ImGui::PopID();
                } else {
                    ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "%s", tr("No layer selected. Click on a layer above to edit it.", "Шар не вибрано. Натисніть на шар вище або на прев'ю, щоб редагувати його."));
                }
            }
            ImGui::EndChild();
            ImGui::Columns(1);
            
            ImGui::EndTabItem();
        }
        
        ImGui::EndTabBar();
    }
    
    // Save/Exit Buttons at the bottom
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    
    if (ImGui::Button(tr("Save Settings", "Зберегти налаштування"), ImVec2(160, 30))) {
        savePresetSettings(globalSettings.currentPreset);
        savePresetLayers(globalSettings.currentPreset);
        
        int x, y;
        SDL_GetWindowPosition(controlWindow, &x, &y);
        globalSettings.controlX = x;
        globalSettings.controlY = y;
        if (!editorModeActive) {
            globalSettings.controlW = w;
            globalSettings.controlH = h;
        } else {
            globalSettings.controlW = savedControlW;
            globalSettings.controlH = savedControlH;
        }
        saveGlobalSettings();
    }
    
    ImGui::SameLine();
    if (ImGui::Button(tr("Exit Application", "Вийти з програми"), ImVec2(160, 30))) {
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
