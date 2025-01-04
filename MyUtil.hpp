#pragma once

#include <cstdint>
#include <set>
#include <limits>
#include <algorithm>

// https://stackoverflow.com/a/58568736
template <typename T>
T divide_roundnearest(T numer, T denom) {
  static_assert(std::numeric_limits<T>::is_integer, "Only integer types are allowed");

  T result = ((numer) < 0) != ((denom) < 0) ?
    ((numer) - ((denom)/2)) / (denom) :
    ((numer) + ((denom)/2)) / (denom);
  return result;
}

// https://stackoverflow.com/a/36835959
inline constexpr unsigned char operator "" _uchar( unsigned long long arg ) noexcept {
  return static_cast< unsigned char >( arg );
}

template<int size>
void squareMatrixMul(const float a[size * size], const float b[size * size], float c[size * size]) {
  std::fill_n(c, size * size, 0.0f);
  for (int i = 0; i < size; i++) {
    for (int k = 0; k < size; k++) {
      for (int j = 0; j < size; j++) {
        c[i * size + j] += a[k + i * size] * b[j + k * size];
      }
    }
  }
}

template<int size>
void squareMatrixMulT(const float a[size * size], const float bT[size * size], float c[size * size]) {
  std::fill_n(c, size * size, 0.0f);
  for (int i = 0; i < size; i++) {
    for (int k = 0; k < size; k++) {
      for (int j = 0; j < size; j++) {
        c[i * size + j] += a[k + i * size] * bT[k + j * size];
      }
    }
  }
}

template<int size>
void squareMatrixMulT2(const float aT[size * size], const float b[size * size], float c[size * size]) {
  std::fill_n(c, size * size, 0.0f);
  for (int i = 0; i < size; i++) {
    for (int k = 0; k < size; k++) {
      for (int j = 0; j < size; j++) {
        c[i * size + j] += aT[i + k * size] * b[j + k * size];
      }
    }
  }
}

template<typename T>
bool checkOverflowAdd(const T& a, const T& b) {
  return !(b > std::numeric_limits<T>::max() - a);
}

void pack11bit(uint8_t* packed_res, std::set<int16_t>::iterator& it, uint8_t count);

void unpack11bit(const uint8_t* packed_arr, std::set<int16_t>& res, uint8_t count);
