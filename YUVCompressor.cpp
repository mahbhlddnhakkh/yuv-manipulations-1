#include "YUVCompressor.hpp"
#include "Huffman.hpp"
#include "MyYUV.hpp"
#include <cassert>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <cmath>
#include <algorithm>
#include <omp.h>
#include <chrono>
#include "MyUtil.hpp"

#ifdef USE_DCT
static float lum_q_table[64] = {
  16, 11, 10, 16, 24, 40, 51, 61,
  12, 12, 14, 19, 26, 58, 60, 55,
  14, 13, 16, 24, 40, 57, 69, 56,
  14, 17, 22, 29, 51, 87, 80, 62,
  18, 22, 37, 56, 68, 109, 103, 77,
  24, 35, 55, 64, 81, 104, 113, 92,
  49, 64, 78, 87, 103, 121, 120, 101,
  72, 92, 95, 98, 112, 100, 103, 99,
};
static float chroma_q_table[64] = {
  17, 18, 24, 47, 99, 99, 99, 99,
  18, 21, 26, 66, 99, 99, 99, 99,
  24, 26, 56, 99, 99, 99, 99, 99,
  47, 66, 99, 99, 99, 99, 99, 99,
  99, 99, 99, 99, 99, 99, 99, 99,
  99, 99, 99, 99, 99, 99, 99, 99,
  99, 99, 99, 99, 99, 99, 99, 99,
  99, 99, 99, 99, 99, 99, 99, 99,
};
static float DCT_matrix_8[64];
//static float DCT_matrix_4[16];
template<size_t size>
static void calculateDCTMatrix(float DCT_matrix[size * size]) {
  const float inv_size_sqrt = 1.0f / std::sqrt(static_cast<float>(size));
  const float inv_size_float = 1.0f / static_cast<float>(size);
  const float sqrt_2 = std::sqrt(2.0f);
  for (size_t j = 0u; j < size; j++) {
    for (size_t i = 0u; i < size; i++) {
      if (j == 0) {
        DCT_matrix[i + j * size] = inv_size_sqrt;
      } else {
        DCT_matrix[i + j * size] = sqrt_2 * inv_size_sqrt * std::cos(static_cast<float>((2u*i+1u)*j) * M_PIf32 * 0.5f * inv_size_float);
      }
    }
  }
}

template<int DCT_matrix_size>
static void applyDCTtoBlock(float data_block[DCT_matrix_size * DCT_matrix_size], const float DCT_matrix[DCT_matrix_size * DCT_matrix_size], int8_t res[DCT_matrix_size * DCT_matrix_size], const float q_table[DCT_matrix_size * DCT_matrix_size]) {
  float data_block_2[DCT_matrix_size * DCT_matrix_size];
  squareMatrixMul<DCT_matrix_size>(DCT_matrix, data_block, data_block_2);
  squareMatrixMulT<DCT_matrix_size>(data_block_2, DCT_matrix, data_block);
  for (int i = 0; i < DCT_matrix_size * DCT_matrix_size; i++) {
    res[i] = std::clamp(static_cast<int>(std::round(data_block[i] / q_table[i])), INT8_MIN, INT8_MAX);
  }
}

template<int DCT_matrix_size>
static void applyDCTtoPane(uint8_t** res, uint8_t* res_size, const uint8_t* data, const int& width, const int& height, const float DCT_matrix[DCT_matrix_size * DCT_matrix_size], const float q, const float q_50_table[DCT_matrix_size * DCT_matrix_size]) {
  assert(DCT_matrix_size > 0);
  assert(width % DCT_matrix_size == 0);
  assert(height % DCT_matrix_size == 0);
  // Calculate quality matrix
  float q_table[DCT_matrix_size * DCT_matrix_size];
  const float q_table_mul = (q >= 50.5f) ? (100.0f - q) / 50.0f : 50.0f / q;
  for (int i = 0; i < DCT_matrix_size * DCT_matrix_size; i++) {
    q_table[i] = std::clamp(std::round(q_50_table[i] * q_table_mul), 1.0f, 255.0f);
  }
  // Split into blocks
  #pragma omp parallel for collapse(2)
  for (int j = 0; j < height; j += DCT_matrix_size) {
    for (int i = 0; i < width; i += DCT_matrix_size) {
      int8_t block_res[DCT_matrix_size * DCT_matrix_size];
      float data_block[DCT_matrix_size * DCT_matrix_size];
      for (int jj = 0; jj < DCT_matrix_size; jj++) {
        for (int ii = 0; ii < DCT_matrix_size; ii++) {
          data_block[ii + jj * DCT_matrix_size] = static_cast<float>(data[(i + ii) + (j + jj) * width]) - 128.0f;
        }
      }
      applyDCTtoBlock<DCT_matrix_size>(data_block, DCT_matrix, block_res, q_table);
      Huffman<DCT_matrix_size * DCT_matrix_size> huffman = Huffman<DCT_matrix_size * DCT_matrix_size>::fromData(block_res);
      const int k = (i + j * width / DCT_matrix_size) / DCT_matrix_size;
      huffman.dump(res[k], res_size[k]);
    }
  }
}

template<int DCT_matrix_size>
static void restoreDCTBlock(const uint8_t* huffman, const uint8_t& huffman_size, const float DCT_matrix[DCT_matrix_size * DCT_matrix_size], float res[DCT_matrix_size * DCT_matrix_size], const float q_table[DCT_matrix_size * DCT_matrix_size]) {
  Huffman<DCT_matrix_size * DCT_matrix_size> _huffman = Huffman<DCT_matrix_size * DCT_matrix_size>::fromDump(huffman, huffman_size);
  int8_t* data = _huffman.getData();
  float data_block[64];
  for (int i = 0; i < DCT_matrix_size * DCT_matrix_size; i++) {
    res[i] = static_cast<float>(data[i]) * q_table[i];
  }
  squareMatrixMulT2<DCT_matrix_size>(DCT_matrix, res, data_block);
  squareMatrixMul<DCT_matrix_size>(data_block, DCT_matrix, res);
}

template<int DCT_matrix_size>
static void restoreDCTPane(uint8_t* data, const uint8_t* huffman_data, const int& huffman_data_size, const int* huffman_indexes, const int& width, const int& height, const float DCT_matrix[DCT_matrix_size * DCT_matrix_size], const float q, const float q_50_table[DCT_matrix_size * DCT_matrix_size]) {
  assert(DCT_matrix_size > 0);
  assert(width % DCT_matrix_size == 0);
  assert(height % DCT_matrix_size == 0);
  // Calculate quality matrix
  float q_table[DCT_matrix_size * DCT_matrix_size];
  const float q_table_mul = (q >= 50.5f) ? (100.0f - q) / 50.0f : 50.0f / q;
  for (int i = 0; i < DCT_matrix_size * DCT_matrix_size; i++) {
    q_table[i] = std::clamp(std::round(q_50_table[i] * q_table_mul), 1.0f, 255.0f);
  }
  // Split into blocks
  #pragma omp parallel for collapse(2)
  for (int j = 0; j < height; j += DCT_matrix_size) {
    for (int i = 0; i < width; i += DCT_matrix_size) {
      const int huffman_index = huffman_indexes[(i + j * width / DCT_matrix_size) / DCT_matrix_size];
      float block_res[DCT_matrix_size * DCT_matrix_size];
      restoreDCTBlock<DCT_matrix_size>(&huffman_data[huffman_index + 1], huffman_data[huffman_index], DCT_matrix, block_res, q_table);
      assert(huffman_index <= huffman_data_size);
      for (int jj = 0; jj < DCT_matrix_size; jj++) {
        for (int ii = 0; ii < DCT_matrix_size; ii++) {
          data[(i + ii) + (j + jj) * width] = std::clamp(static_cast<int>(std::round(block_res[ii + jj * DCT_matrix_size])) + 128, 0, UINT8_MAX);
        }
      }
    }
  }
}

extern void test_dct() {
  float q = 50.0f;
  const float q_table_mul = (q >= 50.5f) ? (100.0f - q) / 50.0f : 50.0f / q;
  float q_table[64];
  for (int i = 0; i < 64; i++) {
    q_table[i] = std::clamp(std::round(lum_q_table[i] * q_table_mul), 1.0f, 255.0f);
  }
  for (int j = 0; j < 8; j++) {
    for (int i = 0; i < 8; i++) {
      std::cout << q_table[i + j * 8] << '\t';
    }
    std::cout << '\n';
  }
  std::cout << '\n';
  float data_block[64] = {
    26, -5, -5, -5, -5, -5, -5, 8,
    64, 52, 8, 26, 26, 26, 8, -18,
    126, 70, 26, 26, 52, 26, -5, -5,
    111, 52, 8, 52, 52, 38, -5, -5,
    52, 26, 8, 39, 38, 21, 8, 8,
    0, 8, -5, 8, 26, 52, 70, 26,
    -5, -23, -18, 21, 8, 8, 52, 38,
    -18, 8, -5, -5, -5, 8, 26, 8,
  };
  int8_t block_res[64];
  applyDCTtoBlock<8>(data_block, DCT_matrix_8, block_res, q_table);
  for (int j = 0; j < 8; j++) {
    for (int i = 0; i < 8; i++) {
      std::cout << (int)block_res[i + j * 8] << '\t';
    }
    std::cout << '\n';
  }
  std::cout << '\n';
  float block_res2[64];
  uint8_t res[64];
  Huffman<64> huffman = Huffman<64>::fromData(block_res);
  uint8_t* huffman_dump;
  uint8_t huffman_size;
  huffman.dump(huffman_dump, huffman_size);
  restoreDCTBlock<8>(huffman_dump, huffman_size, DCT_matrix_8, block_res2, q_table);
  for (int i = 0; i < 64; i++) {
    res[i] = std::clamp(static_cast<int>(std::round(block_res2[i])) + 128, 0, UINT8_MAX);
  }
  for (int j = 0; j < 8; j++) {
    for (int i = 0; i < 8; i++) {
      std::cout << (int)res[i + j * 8] << '\t';
    }
    std::cout << '\n';
  }
  std::cout << '\n';
}
#endif

#if defined (USE_DCT) && defined (USE_DCT_IYUV)
void YUVCompressor::DST_IYUV_compress(MyYUV& myyuv, const uint8_t q[3]) {
  assert(myyuv.width % 32 == 0);
  assert(myyuv.height % 32 == 0);
  assert(myyuv.format == SDL_PIXELFORMAT_IYUV);
  assert(!myyuv.isCompressed());
  for (int i = 0; i < 3; i++) {
    if (q[i] < 1 || q[i] > 100) {
      throw std::runtime_error("Level of quality must be between 1 and 100");
    }
  }
  uint8_t* _q = new uint8_t[3];
  for (int i = 0; i < 3; i++) {
    _q[i] = q[i];
  }
  uint8_t** Y = new uint8_t*[myyuv.width * myyuv.height / 64];
  uint8_t* Y_size = new uint8_t[myyuv.width * myyuv.height / 64];
  uint8_t** U = new uint8_t*[myyuv.width * myyuv.height / 256];
  uint8_t* U_size = new uint8_t[myyuv.width * myyuv.height / 256];
  uint8_t** V = new uint8_t*[myyuv.width * myyuv.height / 256];
  uint8_t* V_size = new uint8_t[myyuv.width * myyuv.height / 256];
  myyuv.compressor.data_for_decompress = _q;
  myyuv.compressor.data_for_decompress_size = 3;
  #pragma omp parallel sections
  {
    #pragma omp section
    {
      applyDCTtoPane<8>(Y, Y_size, myyuv.data, myyuv.width, myyuv.height, DCT_matrix_8, q[0], lum_q_table);
    }
    #pragma omp section
    {
      applyDCTtoPane<8>(U, U_size, myyuv.data + myyuv.width * myyuv.height, myyuv.width / 2, myyuv.height / 2, DCT_matrix_8, q[1], chroma_q_table);
    }
    #pragma omp section
    {
      applyDCTtoPane<8>(V, V_size, myyuv.data + myyuv.width * myyuv.height * 5 / 4, myyuv.width / 2, myyuv.height / 2, DCT_matrix_8, q[2], chroma_q_table);
    }
  }
  int total_size = myyuv.width * myyuv.height * 6 / 256;
  int tmp_count = myyuv.width * myyuv.height / 64; // TODO: delete
  //std::cout << total_size << '\n';
  for (int i = 0; i < myyuv.width * myyuv.height / 64; i++) {
    total_size += Y_size[i];
    tmp_count += Y_size[i];
  }
  //std::cout << tmp_count << '\n';
  tmp_count = myyuv.width * myyuv.height / 256;
  for (int i = 0; i < myyuv.width * myyuv.height / 256; i++) {
    total_size += U_size[i];
    tmp_count += U_size[i];
  }
  //std::cout << tmp_count << '\n';
  tmp_count = myyuv.width * myyuv.height / 256;
  for (int i = 0; i < myyuv.width * myyuv.height / 256; i++) {
    total_size += V_size[i];
    tmp_count += V_size[i];
  }
  //std::cout << tmp_count << '\n';
  //std::cout << "DCT IYUV compressed size " << total_size << '\n';
  delete[] myyuv.data;
  myyuv.data = new uint8_t[total_size];
  myyuv.compressor.type = "DCT";
  myyuv.compressor.size = total_size;
  int k = 0;
  for (int i = 0; i < myyuv.width * myyuv.height / 64; i++) {
    myyuv.data[k++] = Y_size[i];
    for (uint8_t j = 0; j < Y_size[i]; j++) {
      myyuv.data[k++] = Y[i][j];
    }
  }
  for (int i = 0; i < myyuv.width * myyuv.height / 256; i++) {
    myyuv.data[k++] = U_size[i];
    for (uint8_t j = 0; j < U_size[i]; j++) {
      myyuv.data[k++] = U[i][j];
    }
  }
  for (int i = 0; i < myyuv.width * myyuv.height / 256; i++) {
    myyuv.data[k++] = V_size[i];
    for (uint8_t j = 0; j < V_size[i]; j++) {
      myyuv.data[k++] = V[i][j];
    }
  }
  assert(k == total_size);
}

void YUVCompressor::DST_IYUV_decompress(MyYUV& myyuv) {
  assert(myyuv.width % 32 == 0);
  assert(myyuv.height % 32 == 0);
  assert(myyuv.format == SDL_PIXELFORMAT_IYUV);
  assert(myyuv.isCompressed());
  assert(myyuv.compressor.data_for_decompress_size == 3);
  uint8_t* q = reinterpret_cast<uint8_t*>(myyuv.compressor.data_for_decompress);
  for (int i = 0; i < 3; i++) {
    if (q[i] < 1 || q[i] > 100) {
      throw std::runtime_error("Level of quality must be between 1 and 100");
    }
  }
  uint8_t* data = new uint8_t[myyuv.size];
  uint8_t* Y = data;
  uint8_t* U = &(data[myyuv.width * myyuv.height]);
  uint8_t* V = &(data[myyuv.width * myyuv.height * 5 / 4]);
  int* Y_huffman_ind = new int[myyuv.width * myyuv.height / 64];
  int* U_huffman_ind = new int[myyuv.width * myyuv.height / 256];
  int* V_huffman_ind = new int[myyuv.width * myyuv.height / 256];
  int k = 0;
  int k_arr[3];
  int saved_k_arr[3];
  saved_k_arr[0] = 0;
  for (int i = 0; i < myyuv.width * myyuv.height / 64; i++) {
    Y_huffman_ind[i] = k - saved_k_arr[0];
    k += myyuv.data[k] + 1;
  }
  //std::cout << k - saved_k_arr[0] << '\n';
  k_arr[0] = k;
  saved_k_arr[1] = k;
  for (int i = 0; i < myyuv.width * myyuv.height / 256; i++) {
    U_huffman_ind[i] = k - saved_k_arr[1];
    k += myyuv.data[k] + 1;
  }
  //std::cout << k - saved_k_arr[1] << '\n';
  k_arr[1] = k;
  saved_k_arr[2] = k;
  for (int i = 0; i < myyuv.width * myyuv.height / 256; i++) {
    V_huffman_ind[i] = k - saved_k_arr[2];
    k += myyuv.data[k] + 1;
  }
  //std::cout << k - saved_k_arr[2] << '\n';
  assert(k == myyuv.compressor.size);
  k_arr[2] = k;
  #pragma omp parallel sections
  {
    #pragma omp section
    {
      restoreDCTPane<8>(Y, myyuv.data + saved_k_arr[0], k_arr[0] - saved_k_arr[0], Y_huffman_ind, myyuv.width, myyuv.height, DCT_matrix_8, q[0], lum_q_table);
    }
    #pragma omp section
    {
      restoreDCTPane<8>(U, myyuv.data + saved_k_arr[1], k_arr[1] - saved_k_arr[1], U_huffman_ind, myyuv.width / 2, myyuv.height / 2, DCT_matrix_8, q[1], chroma_q_table);
    }
    #pragma omp section
    {
      restoreDCTPane<8>(V, myyuv.data + saved_k_arr[2], k_arr[2] - saved_k_arr[2], V_huffman_ind, myyuv.width / 2, myyuv.height / 2, DCT_matrix_8, q[2], chroma_q_table);
    }
  }
  delete[] Y_huffman_ind;
  delete[] U_huffman_ind;
  delete[] V_huffman_ind;
  delete[] myyuv.data;
  myyuv.data = data;
  myyuv.compressor.removeDataForDecompress();
  myyuv.compressor.type = "";
}
#endif

const std::map<std::string, std::function<void(YUVCompressor&, const std::vector<std::string>&)>> YUVCompressor::compression_map {
#ifdef USE_DCT
  {"DCT", [](YUVCompressor& compressor, const std::vector<std::string>& params)->void{
    MyYUV* myyuv = compressor.yuv;
    assert(myyuv->width % 8 == 0);
    assert(myyuv->height % 8 == 0);
    assert(!myyuv->isCompressed());
    uint8_t q[3] = { 85, 50, 50 };
    for (size_t i = 0; i < 3 && i < params.size(); i++) {
      try {
        q[i] = std::clamp(std::stoi(params[i]), 0, 255);
      } catch (std::invalid_argument const&) {
      } catch (std::out_of_range const&) {}
    }
    switch (myyuv->format) {
#ifdef USE_DCT_IYUV
      case SDL_PIXELFORMAT_IYUV:
        DST_IYUV_compress(*myyuv, q);
        break;
#endif
      default:
        throw std::runtime_error("Not implemented for that format yet");
        break;
    }
  }},
#endif
};

const std::map<std::string, std::function<void(YUVCompressor&)>> YUVCompressor::decompression_map {
  {"DCT", [](YUVCompressor& compressor)->void{
    MyYUV* myyuv = compressor.yuv;
    assert(myyuv->width % 8 == 0);
    assert(myyuv->height % 8 == 0);
    switch (myyuv->format) {
#ifdef USE_DCT_IYUV
      case SDL_PIXELFORMAT_IYUV:
        DST_IYUV_decompress(*myyuv);
        break;
#endif
      default:
        throw std::runtime_error("Not implemented for that format yet");
        break;
    }
  }},
};

bool YUVCompressor::isCompressedType(const std::string& type) {
  return type.length() != 0 && type != "none";
}

void YUVCompressor::precompute() {
#ifdef USE_DCT
  //calculateDCTMatrix<4>(DCT_matrix_4);
  calculateDCTMatrix<8>(DCT_matrix_8);
#endif
  done_precomputed = true;
}

YUVCompressor::YUVCompressor(MyYUV* yuv) {
  setYUV(yuv);
}

YUVCompressor::~YUVCompressor() {
  removeDataForDecompress();
}

static std::map<std::string, std::function<void(void*)>> remove_data_decompress_map = {
#ifdef USE_DCT
  {"DCT", [](void* data)->void{
    delete[] reinterpret_cast<uint8_t*>(data);
  }},
#endif
};

static std::map<std::string, std::function<void*(void*)>> clone_data_decompress_map = {
#ifdef USE_DCT
  {"DCT", [](void* data)->void* {
    uint8_t* cloned_data = new uint8_t[3];
    uint8_t* _data = reinterpret_cast<uint8_t*>(data);
    cloned_data[0] = _data[0];
    cloned_data[1] = _data[1];
    cloned_data[2] = _data[2];
    return cloned_data;
  }},
#endif
};

void* YUVCompressor::cloneDataForDecompress() const {
  if (data_for_decompress) {
    auto search = clone_data_decompress_map.find(type);
    if (search != clone_data_decompress_map.end()) {
      return search->second(data_for_decompress);
    } else {
      throw std::runtime_error("Can't clone data for decompress, unknown type");
    }
  }
  return nullptr;
}

void YUVCompressor::removeDataForDecompress() {
  if (data_for_decompress) {
    auto search = remove_data_decompress_map.find(type);
    if (search != remove_data_decompress_map.end()) {
      search->second(data_for_decompress);
      data_for_decompress = nullptr;
    } else {
      throw std::runtime_error("Can't remove data for decompress, unknown type");
    }
  }
}

void YUVCompressor::setYUV(MyYUV* yuv) {
  this->yuv = yuv;
  if (yuv != nullptr) {
    updateSize();
    type = yuv->compressor.type;
  } else {
    size = -1;
    type = "";
  }
}

void YUVCompressor::updateSize() {
  assert(yuv);
  size = yuv->getSize();
}

void YUVCompressor::decompress() {
  assert(done_precomputed);
  if (isCompressed()) {
    assert(yuv);
    auto search = decompression_map.find(type);
    if (search != decompression_map.end()) {
      std::string prev_type = type;
      auto start = std::chrono::high_resolution_clock::now();
      search->second(*this);
      auto end = std::chrono::high_resolution_clock::now();
      auto duration = end - start;
      std::cout << prev_type << " decompression time " << std::chrono::duration_cast<std::chrono::milliseconds>(duration).count() << " ms\n";
      return;
    } else {
      throw std::runtime_error(type + " not found");
    }
  }
}
void YUVCompressor::compress(const std::string& type, const std::vector<std::string>& params) {
  assert(isCompressedType(type));
  assert(yuv);
  assert(done_precomputed);
  auto search = compression_map.find(type);
  if (search != compression_map.end()) {
    if (isCompressed()) {
      std::cout << "Warning. Already compressed. Multiple compressions not supported. Decompressing...\n";
      decompress();
    }
    auto start = std::chrono::high_resolution_clock::now();
    search->second(*this, params);
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = end - start;
    std::cout << type << " compression time " << std::chrono::duration_cast<std::chrono::milliseconds>(duration).count() << " ms\n";
    std::cout << type << " compressed size " << size << " bytes\n";
    return;
  } else {
    throw std::runtime_error(type + " not found");
  }
}

bool YUVCompressor::isCompressed() const {
  assert(yuv);
  return isCompressedType(type);
}

std::string YUVCompressor::compressType() const {
  assert(yuv);
  return type;
}

int YUVCompressor::getSize() const {
  assert(yuv);
  return size;
}

bool YUVCompressor::done_precomputed = false;
