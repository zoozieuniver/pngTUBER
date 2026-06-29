#ifndef AVATAR_H
#define AVATAR_H

#include <SDL2/SDL.h>
#include <string>

extern SDL_Window* avatarWindow;
extern SDL_Renderer* avatarRenderer;
extern bool editorModeActive;
extern float previewZoom;
extern float previewPanX;
extern float previewPanY;

bool initAvatar();
void cleanupAvatar();
bool applyAvatarPreset(const std::string& name);
void renderAvatar();
void updateAvatarWindowPosition();
Uint32 getAvatarWindowID();
void loadCustomBgTexture(const std::string& path);
void updateWindowTransparency();
void adjustWindowToBgTextureSize();

void setEditorMode(bool enable);
void reloadLayerTextures();
void clearLayerTextures();
void renderAvatarPreviewToTexture(SDL_Texture* targetTexture, int w, int h);
void updateAvatarWindowSize();
int hitTestLayers(int mouseX, int mouseY, int targetW, int targetH);

#endif // AVATAR_H
