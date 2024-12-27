#include "MyYUV.hpp"
#include <cassert>
#include <iostream>
#include <stdexcept>
#include "YUVCompressor.hpp"
#include "YUVConverter.hpp"

MyYUV MyYUV::fromBMP(const std::string& filename, const std::string& to_type) {
  //uint8_t* tmp;
  SDL_Surface* surf = SDL_LoadBMP(filename.c_str());
  uint8_t* yuv444 = nullptr;
  auto cleanup = [&](){
    SDL_DestroySurface(surf);
    delete[] yuv444;
  };
  try {
    if (surf == nullptr) {
      throw std::runtime_error(SDL_GetError());
    }
    assert(isFormatRGBA(surf->format));
    assert(surf->w % 2 == 0);
    assert(surf->h % 2 == 0);
    //tmp = (uint8_t*)surf->pixels;
    int size = surf->h * surf->pitch;
    //std::cout << (Uint32)tmp[0] << ' ' << (Uint32)tmp[1] << ' ' << (Uint32)tmp[2] << ' ' << (Uint32)tmp[3] << '\n';
    std::cout << "RGB size " << size << '\n';
    std::cout << "RGB size without alpha " << size - surf->w * surf->h << '\n';
    if (to_type == "RGB") {
      uint8_t* data = new uint8_t[size];
      uint8_t* surf_pixels = static_cast<uint8_t*>(surf->pixels);
      std::copy(surf_pixels, surf_pixels + size, data);
      MyYUV res(surf->w, surf->h, surf->pitch, size, surf->format, data);
      cleanup();
      return res;
    }
    yuv444 = getAYUVFromXRGB(static_cast<uint8_t*>(surf->pixels), size);
    //std::cout << (Uint32)yuv444[0] << ' ' << (Uint32)yuv444[1] << ' ' << (Uint32)yuv444[2] << '\n';
    MyYUV res = YUVConverter::convertFromAYUV(to_type, yuv444, surf->w, surf->h);
    //tmp = (uint8_t*)res.data;
    //std::cout << (Uint32)tmp[0] << ' ' << (Uint32)tmp[surf->w*surf->h] << ' ' << (Uint32)tmp[surf->w*surf->h+1] << '\n';
    //for (int i = 0; i < surf->w * surf->h * 3 / 2; i++) {
    //  std::cout << i << ' ' << (Uint32)tmp[i] << '\n';
    //}
    cleanup();
    std::cout << "YUV size " << res.size << '\n';
    return res;
  } catch(...) {
    cleanup();
    throw;
  }
}

MyYUV MyYUV::fromDump(const std::string& filename) {
  // TODO
  assert(false && "Unimplemented");
}

MyYUV MyYUV::from(const std::string &type, const std::string &filename, const std::string& to_type) {
  if (type == "BMP") {
    return fromBMP(filename, to_type);
  } else if (type == "YUV") {
    return fromDump(filename);
  } else {
    throw std::runtime_error("Unknown file type " + type);
  }
}

bool MyYUV::isFormatRGBA(const SDL_PixelFormat& format) {
  return format == SDL_PIXELFORMAT_ARGB8888 || format == SDL_PIXELFORMAT_XRGB8888;
}

MyYUV::MyYUV(MyYUV&& myyuv) : compressor(nullptr) {
  std::swap(width, myyuv.width);
  std::swap(height, myyuv.height);
  std::swap(pitch, myyuv.pitch);
  std::swap(format, myyuv.format);
  std::swap(data, myyuv.data);
  std::swap(size, myyuv.size);
  compressor.yuv = this;
  std::swap(compressor.type, myyuv.compressor.type);
  std::swap(compressor.size, myyuv.compressor.size);
  std::swap(compressor.data_for_decompress, myyuv.compressor.data_for_decompress);
  std::swap(compressor.data_for_decompress_size, myyuv.compressor.data_for_decompress_size);
}

MyYUV::~MyYUV() {
  compressor.yuv = nullptr;
  delete[] data;
}

SDL_PixelFormat MyYUV::getPixelFormat() const {
  assert(isGood());
  return format;
}

SDL_Surface* MyYUV::createSurface() {
  assert(isGood());
  decompress();
  SDL_Surface* surf = SDL_CreateSurfaceFrom(width, height, format, data, pitch);
  //std::cout << surf->w << ' ' << surf->h << ' ' << surf->pitch << '\n';
  //uint8_t* tmp = (uint8_t*)surf->pixels;
  //std::cout << (Uint32)tmp[0] << ' ' << (Uint32)tmp[surf->w*surf->h] << ' ' << (Uint32)tmp[surf->w*surf->h+1] << '\n';
  //for (int i = 0; i < surf->w * surf->h * 3 / 2; i++) {
  //  std::cout << i << ' ' << (Uint32)tmp[i] << '\n';
  //}
  if (!surf) {
    throw std::runtime_error("Bad surface");
  }
  return surf;
}

bool MyYUV::isGood() const {
  return (
    width > 0 && height > 0 && data != nullptr && pitch > 0 && format != SDL_PIXELFORMAT_UNKNOWN
  );
}

int MyYUV::getSize() const {
  assert(isGood());
  if (isCompressed()) {
    return compressor.getSize();
  } else {
    return size;
  }
}

void MyYUV::decompress() {
  compressor.decompress();
}
void MyYUV::compress(const std::string& type, const std::vector<std::string>& params) {
  compressor.compress(type, params);
}

bool MyYUV::isCompressed() const {
  assert(isGood());
  return compressor.isCompressed();
}

MyYUV MyYUV::clone() const {
  assert(isGood());
  MyYUV yuv;
  yuv.width = width;
  yuv.height = height;
  yuv.pitch = pitch;
  yuv.size = size;
  yuv.format = format;
  const int real_size = getSize();
  uint8_t* data_copy = new uint8_t[real_size];
  std::copy(data, data + real_size , data_copy);
  yuv.data = data_copy;
  yuv.compressor = YUVCompressor();
  yuv.compressor.yuv = &yuv;
  yuv.compressor.size = compressor.size;
  yuv.compressor.type = compressor.type;
  yuv.compressor.data_for_decompress = compressor.cloneDataForDecompress();
  yuv.compressor.data_for_decompress_size = compressor.data_for_decompress_size;
  return yuv;
}
int MyYUV::getWidth() const {
  assert(isGood());
  return width;
}
int MyYUV::getHeight() const {
  assert(isGood());
  return height;
}

uint8_t* MyYUV::getAYUVFromXRGB(const uint8_t* rgbx, const int& size) {
  assert(size % 4 == 0);
  uint8_t* yuv444 = new uint8_t[size];
  for (int i = 0; i < size; i += 4) {
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    const float R = static_cast<float>(rgbx[i]);
    const float G = static_cast<float>(rgbx[i+1]);
    const float B = static_cast<float>(rgbx[i+2]);
#else
    const float B = static_cast<float>(rgbx[i]);
    const float G = static_cast<float>(rgbx[i+1]);
    const float R = static_cast<float>(rgbx[i+2]);
#endif
    const uint8_t& A = rgbx[i+3];
    const float Y = 0.299f * R + 0.587f * G + 0.114f * B;
    yuv444[i] = static_cast<uint8_t>(Y); // Y
    yuv444[i+1] = static_cast<uint8_t>((B - Y) * 0.564f) + 128; // Cb
    yuv444[i+2] = static_cast<uint8_t>((R - Y) * 0.713f) + 128; // Cr
    yuv444[i+3] = A; // A
  }
  return yuv444;
}

MyYUV::MyYUV() {}

MyYUV::MyYUV(const int& width, const int& height, const int& pitch, const int& size, const SDL_PixelFormat& format, uint8_t* data) {
  this->width = width;
  this->height = height;
  this->pitch = pitch;
  this->size = size;
  this->format = format;
  this->data = data;
  compressor.setYUV(this);
}
