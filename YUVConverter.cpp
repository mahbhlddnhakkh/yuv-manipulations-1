#include "YUVConverter.hpp"
#include <map>
#include <functional>
#include <stdexcept>
#include <iostream>
#include "MyYUV.hpp"
#include "MyUtil.hpp"

MyYUV YUVConverter::convertFromAYUV(const std::string& type, const uint8_t* ayuv, const int& width, const int& height) {
  auto search = yuv_types_map.find(type);
  if (search != yuv_types_map.end()) {
    return search->second(ayuv, width, height);
  } else {
    throw std::runtime_error(type + " not found");
  }
}

const std::map<std::string, std::function<MyYUV(const uint8_t* ayuv, const int& width, const int& height)>> YUVConverter::yuv_types_map = {
/*
  {"NV12", [](const uint8_t* ayuv, const int& width, const int& height)->MyYUV{
    const int new_size = width * height * 3 / 2; // width*height+(width/2)*height/2
    uint8_t* pixels = new uint8_t[new_size];
    uint8_t* uv = &(pixels[width * height]);
    int k = 0;
    for (int j = 0; j < height; j+=2) {
      for (int i = 0; i < width; i+=2) {
        const int loc = i + j * width;
        const int loc_down = loc + width;
        const int loc_ayuv = loc * 4;
        const int loc_down_ayuv = loc_down * 4;
        //std::cout << i << ' ' << j << ' ' << k << ' ' << loc << '\n';
        //const uint8_t Cb = ayuv[loc_ayuv + 1] / 4 + ayuv[loc_ayuv + 5] / 4 + ayuv[loc_down_ayuv + 1] / 4 + ayuv[loc_down_ayuv + 5] / 4;
        //const uint8_t Cr = ayuv[loc_ayuv + 2] / 4 + ayuv[loc_ayuv + 6] / 4 + ayuv[loc_down_ayuv + 2] / 4 + ayuv[loc_down_ayuv + 6] / 4;
        const uint8_t Cb = divide_roundnearest(ayuv[loc_ayuv + 1], 4_uchar) + divide_roundnearest(ayuv[loc_ayuv + 5], 4_uchar) + divide_roundnearest(ayuv[loc_down_ayuv + 1], 4_uchar) + divide_roundnearest(ayuv[loc_down_ayuv + 5], 4_uchar);
        const uint8_t Cr = divide_roundnearest(ayuv[loc_ayuv + 2], 4_uchar) + divide_roundnearest(ayuv[loc_ayuv + 6], 4_uchar) + divide_roundnearest(ayuv[loc_down_ayuv + 2], 4_uchar) + divide_roundnearest(ayuv[loc_down_ayuv + 6], 4_uchar);
        pixels[loc] = ayuv[loc_ayuv];
        pixels[loc + 1] = ayuv[loc_ayuv + 4];
        pixels[loc_down] = ayuv[loc_down_ayuv];
        pixels[loc_down + 1] = ayuv[loc_down_ayuv + 4];
        uv[k] = Cb;
        uv[k + 1] = Cr;
        k+=2;
      }
    }
    MyYUV res = MyYUV(width, height, width, new_size, SDL_PIXELFORMAT_NV12, pixels);
    return res;
  }},
*/
  {"IYUV", [](const uint8_t* ayuv, const int& width, const int& height)->MyYUV{
    const int new_size = width * height * 3 / 2; // width * height + width * height / 2
    uint8_t* pixels = new uint8_t[new_size];
    uint8_t* u = &(pixels[width * height]);
    uint8_t* v = &(pixels[width * height * 5 / 4]);
    int k = 0;
    for (int j = 0; j < height; j+=2) {
      for (int i = 0; i < width; i+=2) {
        const int loc = i + j * width;
        const int loc_down = loc + width;
        const int loc_ayuv = loc * 4;
        const int loc_down_ayuv = loc_down * 4;
        //std::cout << i << ' ' << j << ' ' << k << ' ' << loc << '\n';
        //const uint8_t Cb = ayuv[loc_ayuv + 1] / 4 + ayuv[loc_ayuv + 5] / 4 + ayuv[loc_down_ayuv + 1] / 4 + ayuv[loc_down_ayuv + 5] / 4;
        //const uint8_t Cr = ayuv[loc_ayuv + 2] / 4 + ayuv[loc_ayuv + 6] / 4 + ayuv[loc_down_ayuv + 2] / 4 + ayuv[loc_down_ayuv + 6] / 4;
        const uint8_t Cb = divide_roundnearest(ayuv[loc_ayuv + 1], 4_uchar) + divide_roundnearest(ayuv[loc_ayuv + 5], 4_uchar) + divide_roundnearest(ayuv[loc_down_ayuv + 1], 4_uchar) + divide_roundnearest(ayuv[loc_down_ayuv + 5], 4_uchar);
        const uint8_t Cr = divide_roundnearest(ayuv[loc_ayuv + 2], 4_uchar) + divide_roundnearest(ayuv[loc_ayuv + 6], 4_uchar) + divide_roundnearest(ayuv[loc_down_ayuv + 2], 4_uchar) + divide_roundnearest(ayuv[loc_down_ayuv + 6], 4_uchar);
        pixels[loc] = ayuv[loc_ayuv];
        pixels[loc + 1] = ayuv[loc_ayuv + 4];
        pixels[loc_down] = ayuv[loc_down_ayuv];
        pixels[loc_down + 1] = ayuv[loc_down_ayuv + 4];
        u[k] = Cb;
        v[k] = Cr;
        k++;
      }
    }
    MyYUV res = MyYUV(width, height, width, new_size, SDL_PIXELFORMAT_IYUV, pixels);
    return res;
  }},
};
