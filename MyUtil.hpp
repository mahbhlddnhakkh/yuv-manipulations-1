#pragma once

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
inline constexpr unsigned char operator "" _uchar( unsigned long long arg ) noexcept
{
    return static_cast< unsigned char >( arg );
}

template<int size>
static void squareMatrixMul(const float a[size * size], const float b[size * size], float c[size * size]) {
  std::fill_n(c, size * size, 0.0f);
  for (int i = 0; i < size; i++) {
    for (int k = 0; k < size; k++) {
      for (int j = 0; j < size; j++) {
        c[i + j * size] += a[i + k * size] * b[k + j * size];
      }
    }
  }
}

template<int size>
static void squareMatrixMulT(const float a[size * size], const float bT[size * size], float c[size * size]) {
  std::fill_n(c, size * size, 0.0f);
  for (int i = 0; i < size; i++) {
    for (int k = 0; k < size; k++) {
      for (int j = 0; j < size; j++) {
        c[i + j * size] += a[i + k * size] * bT[j + k * size];
      }
    }
  }
}

template<int size>
static void squareMatrixMulT2(const float aT[size * size], const float b[size * size], float c[size * size]) {
  std::fill_n(c, size * size, 0.0f);
  for (int i = 0; i < size; i++) {
    for (int k = 0; k < size; k++) {
      for (int j = 0; j < size; j++) {
        c[i + j * size] += aT[k + i * size] * b[k + j * size];
      }
    }
  }
}
