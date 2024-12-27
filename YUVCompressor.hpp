#pragma once

#include <SDL3/SDL.h>
#include <map>
#include <functional>
#include <string>
#include <vector>

#define USE_DCT
#define USE_DCT_IYUV

class MyYUV;

class YUVCompressor {
public:
  static const std::map<std::string, std::function<void(YUVCompressor&, const std::vector<std::string>&)>> compression_map;
  static const std::map<std::string, std::function<void(YUVCompressor&)>> decompression_map;
  static bool isCompressedType(const std::string& type);
public:
#if defined (USE_DCT) && defined (USE_DCT_IYUV)
  static void DST_IYUV_compress(MyYUV& myyuv, const uint8_t q[3]);
  static void DST_IYUV_decompress(MyYUV& myyuv);
#endif
public:
  static void precompute();
  YUVCompressor(MyYUV* yuv = nullptr);
  YUVCompressor(const YUVCompressor&) = delete;
  ~YUVCompressor();
  void removeDataForDecompress();
  void* cloneDataForDecompress() const;
  void setYUV(MyYUV* yuv);
  void updateSize();
  void decompress();
  void compress(const std::string& type, const std::vector<std::string>& params);
  bool isCompressed() const;
  std::string compressType() const;
  int getSize() const;
protected:
  MyYUV* yuv;
  int size;
  std::string type;
  void* data_for_decompress = nullptr;
  uint8_t data_for_decompress_size;
  static bool done_precomputed;
private:
  friend class MyYUV;
};
