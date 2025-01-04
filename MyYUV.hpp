#pragma once

#include <SDL3/SDL.h>
#include <cassert>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include <cstring>
#include "YUVCompressor.hpp"

class MyYUV {
public:
  static MyYUV fromBMP(const std::string& filename, const std::string& to_type);
  static MyYUV fromDump(const std::string& filename);
  static MyYUV from(const std::string& type, const std::string& filename, const std::string& to_type = "");
  static bool isFormatRGBA(const SDL_PixelFormat& format);
public:
  MyYUV(MyYUV&& myyuv);
  MyYUV& operator=(MyYUV&& myyuv);
  ~MyYUV();
  SDL_PixelFormat getPixelFormat() const;
  SDL_Surface* createSurface();
  void dump(const std::string& filename) const;
  bool isGood() const;
  int getSize() const;
  void decompress();
  void compress(const std::string& type, const std::vector<std::string>& params);
  bool isCompressed() const;
  MyYUV clone() const;
  int getWidth() const;
  int getHeight() const;
  int getPitch() const;
protected:
  int width = -1;
  int height = -1;
  int pitch = -1;
  int size = -1;
  SDL_PixelFormat format = SDL_PIXELFORMAT_UNKNOWN;
  YUVCompressor compressor;
  uint8_t* data = nullptr;
protected:
  static uint8_t* getAYUVFromXRGB(const uint8_t* xrgb, const int& size);
private:
  MyYUV();
  MyYUV(const int& width, const int& height, const int& pitch, const int& size, const SDL_PixelFormat& format, uint8_t* data);
  MyYUV(const MyYUV&) = delete;
  friend class YUVCompressor;
  friend class YUVConverter;
};
