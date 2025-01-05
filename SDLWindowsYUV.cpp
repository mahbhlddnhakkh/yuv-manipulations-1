#include "SDLWindowsYUV.hpp"
#include <cassert>

SDLWindowsYUV::SDLWindowsYUV(const MyYUV& myyuv, const std::string& title, int w, int h, SDL_WindowFlags flags) : myyuv(myyuv.clone()) {
  win = SDL_CreateWindow(title.c_str(), w, h, flags);
  assert(win);
  win_id = SDL_GetWindowID(win);
  SDL_Surface* surf = this->myyuv.createSurface();
  assert(surf);
  renderer = SDL_CreateRenderer(win, nullptr);
  assert(renderer);
  SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surf);
  SDL_DestroySurface(surf);
  assert(texture);
  SDL_RenderClear(renderer);
  SDL_RenderTexture(renderer, texture, nullptr, nullptr);
  SDL_RenderPresent(renderer);
  SDL_DestroyTexture(texture);
}

SDLWindowsYUV::~SDLWindowsYUV() {
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(win);
}

SDL_Window* SDLWindowsYUV::getWindow() const {
  return win;
}

SDL_Renderer* SDLWindowsYUV::getRenderer() const {
  return renderer;
}

bool SDLWindowsYUV::isFocused() const {
  return focused;
}

bool SDLWindowsYUV::handleEvent(SDL_Event& e) {
  if (e.type >= SDL_EVENT_WINDOW_FIRST && e.type <= SDL_EVENT_WINDOW_LAST && e.window.windowID == win_id) {
    switch (e.type) {
      case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
        return false;
        break;
      case SDL_EVENT_WINDOW_FOCUS_GAINED:
        focused = true;
        break;
      case SDL_EVENT_WINDOW_FOCUS_LOST:
        focused = false;
        break;
    }
  }
  return true;
}

void SDLWindowsYUV::render() {
  SDL_RenderPresent(renderer);
}
