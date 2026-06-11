#ifndef GUI_H
#define GUI_H

#include <SDL2/SDL.h>

extern SDL_Window* controlWindow;
extern SDL_Renderer* controlRenderer;

bool initGUI();
void cleanupGUI();
void processGUIEvent(SDL_Event* event);
void renderGUI();
void updateControlWindowPosition();
Uint32 getControlWindowID();

#endif // GUI_H
