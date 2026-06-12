#ifndef AVATAR_H
#define AVATAR_H

#include <SDL2/SDL.h>
#include <string>

extern SDL_Window* avatarWindow;
extern SDL_Renderer* avatarRenderer;

bool initAvatar();
void cleanupAvatar();
bool applyAvatarPreset(const std::string& name);
void renderAvatar();
void updateAvatarWindowPosition();
Uint32 getAvatarWindowID();
void loadCustomBgTexture(const std::string& path);
void updateWindowTransparency();

#endif // AVATAR_H
