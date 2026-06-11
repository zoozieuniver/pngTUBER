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

#endif // AVATAR_H
