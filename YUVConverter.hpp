#pragma once

#include <SDL3/SDL.h>
#include <string>
#include <map>
#include <functional>

class MyYUV;

class YUVConverter {
public:
  static MyYUV convertFromAYUV(const std::string& type, const uint8_t* ayuv, const int& width, const int& height);
  static const std::map<std::string, std::function<MyYUV(const uint8_t* ayuv, const int& width, const int& height)>> yuv_types_map;
};
