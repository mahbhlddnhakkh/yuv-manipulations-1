#include "MyUtil.hpp"
#include <cassert>

void pack11bit(uint8_t* packed_res, std::set<int16_t>::iterator& it, uint8_t count) {
  std::fill(packed_res, packed_res + (count * 11 + 7) / 8, 0);
  int bit_offset = 0;
  for (uint8_t i = 0; i < count; i++) {
    int byte_ind = bit_offset / 8;
    int bit_ind = bit_offset % 8;
    int16_t _num = *it;
    uint16_t num = (_num < 0 ) ? (2048 + _num) : _num;
    packed_res[byte_ind] |= (num << bit_ind) & 0xFF;
    packed_res[byte_ind + 1] |= (num >> (8 - bit_ind)) & 0xFF;
    if (bit_ind > 5) { // 16 - 11
      packed_res[byte_ind + 2] |= (num >> (16 - bit_ind)) & 0xFF;
    }
    bit_offset += 11;
    it = std::next(it);
  }
}

void unpack11bit(const uint8_t* packed_arr, std::set<int16_t>& res, uint8_t count) {
  int bit_offset = 0;
  for (uint8_t i = 0; i < count; i++) {
    int byte_ind = bit_offset / 8;
    int bit_ind = bit_offset % 8;
    uint16_t _num = (packed_arr[byte_ind] >> bit_ind) & 0xFF;
    _num |= (packed_arr[byte_ind + 1] << (8 - bit_ind)) & 0x7FF;
    if (bit_ind > 5) { // 16 - 11
      _num |= (packed_arr[byte_ind + 2] << (16 - bit_ind)) & 0x7FF;
    }
    _num &= 0x7FF;
    int16_t num = (_num >= 1024) ? (_num - 2048) : _num;
    res.insert(num);
    bit_offset += 11;
  }
}
