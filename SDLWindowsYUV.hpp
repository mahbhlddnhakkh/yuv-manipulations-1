#pragma once

#include <string>
#include <SDL3/SDL.h>
#include "MyYUV.hpp"

class SDLWindowsYUV {
public:
  SDLWindowsYUV(const MyYUV& myyuv, const std::string& title, int w, int h, SDL_WindowFlags flags = 0);
  ~SDLWindowsYUV();
  SDL_Window* getWindow() const;
  SDL_Renderer* getRenderer() const;
  bool isFocused() const;
  bool handleEvent(SDL_Event& e);
  void render();
protected:
  MyYUV myyuv;
  SDL_Window* win;
  SDL_WindowID win_id;
  SDL_Renderer* renderer;
  bool focused;
};
