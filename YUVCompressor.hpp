#pragma once

#include <map>
#include <functional>
#include <string>
#include <vector>
#include "YUVCompressorFeatures.hpp"

#if defined(USE_DCT_CHROMA_IYUV)
#define USE_DCT_CHROMA
#endif

#if defined (USE_DCT_IYUV) || defined (USE_DCT_CHROMA)
#define USE_DCT
#endif

class MyYUV;

class YUVCompressor {
public:
  static const std::map<std::string, std::function<void(YUVCompressor&, const std::vector<std::string>&)>> compression_map;
  static const std::map<std::string, std::function<void(YUVCompressor&)>> decompression_map;
  static bool isCompressedType(const std::string& type);
public:
#ifdef USE_DCT
  static void test_dct();
#endif
#ifdef USE_DCT_IYUV
  static void DST_IYUV_compress(MyYUV& myyuv, const uint8_t q[3]);
  static void DST_IYUV_decompress(MyYUV& myyuv);
#endif
#ifdef USE_DCT_CHROMA_IYUV
  static void DST_CHROMA_IYUV_compress(MyYUV& myyuv, const uint8_t q[2]);
  static void DST_CHROMA_IYUV_decompress(MyYUV& myyuv);
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
