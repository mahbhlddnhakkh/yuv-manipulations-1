#pragma once

#include <SDL3/SDL_render.h>
#include <string>
#include <SDL3/SDL.h>
#include "MyYUV.hpp"

class SDLWindowsYUV {
public:
  SDLWindowsYUV(const MyYUV& myyuv, const std::string& title, int w, int h, SDL_WindowFlags flags = 0) : myyuv(myyuv.clone()) {
    win = SDL_CreateWindow(title.c_str(), w, h, flags);
    assert(win);
    win_id = SDL_GetWindowID(win);
    SDL_Surface* surf = this->myyuv.createSurface();
    //SDL_Surface* surf = SDL_ConvertSurface(SDL_LoadBMP("Untitled.bmp"), SDL_PIXELFORMAT_NV12); // test
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

  ~SDLWindowsYUV() {
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(win);
  }

  SDL_Window* getWindow() const {
    return win;
  }

  SDL_Renderer* getRenderer() const {
    return renderer;
  }

  bool isFocused() const {
    return focused;
  }

  bool handleEvent(SDL_Event& e) {
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

  void render() {
    SDL_RenderPresent(renderer);
  }
protected:
  MyYUV myyuv;
  SDL_Window* win;
  SDL_WindowID win_id;
  SDL_Renderer* renderer;
  bool focused;
};
