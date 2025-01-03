#pragma once

#include <bitset>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <queue>
#include <set>
#include <unordered_map>
#include <map>
#include <functional>
#include <algorithm>

struct HFMNode {
  int8_t ch;
  uint8_t freq;
  HFMNode* left;
  HFMNode* right;
  HFMNode(int8_t ch = 0, uint8_t freq = 0, HFMNode* left = nullptr, HFMNode* right = nullptr) : ch(ch), freq(freq), left(left), right(right) {}
  ~HFMNode() {
    deleteNodes();
  }
  void deleteNodes() {
    if (left) {
      left->deleteNodes();
      delete left;
      left = nullptr;
    }
    if (right) {
      right->deleteNodes();
      delete right;
      right = nullptr;
    }
  }
  bool isLeaf() const {
    return left == nullptr && right == nullptr;
  }
  struct Compare {
    bool operator()(HFMNode* a, HFMNode* b) {
      return a->freq > b->freq;
    }
  };
};

template<size_t msg_size>
class Huffman {
public:
  static Huffman<msg_size> fromData(const int8_t data[msg_size]) {
    // create huffman tree
    std::unordered_map<int8_t, uint8_t> freq;
    size_t last_seen_zero = 0;
    const std::vector<size_t>& zigzag = Huffman<msg_size>::zigzagIndexes[msg_size];
    int8_t _data[msg_size];
    for (size_t i = 0; i < msg_size; i++) {
      _data[i] = data[zigzag[i]];
    }
    for (size_t i = 0; i < msg_size; i++) {
      int8_t d = _data[i];
      if (d == 0) {
        last_seen_zero++;
      } else {
        last_seen_zero = 0;
      }
      freq[_data[i]]++;
    }
    size_t actual_msg_size = msg_size;
    if (freq.find(0) != freq.end()) {
      freq[0] -= last_seen_zero;
      actual_msg_size -= last_seen_zero;
    }
    if (freq[0] == 0) {
      if (actual_msg_size == 0) {
        freq[0] = 1;
        actual_msg_size = 1;
      } else {
        freq.erase(0);
      }
    }
    std::priority_queue<HFMNode*, std::vector<HFMNode*>, HFMNode::Compare> pq;
    for (auto pair : freq) {
      pq.push(new HFMNode(pair.first, pair.second));
    }
    HFMNode* root = nullptr;
    while (pq.size() > 1) {
      HFMNode* left = pq.top();
      pq.pop();
      HFMNode* right = pq.top();
      pq.pop();
      HFMNode* tmp_root = new HFMNode(0, left->freq + right->freq, left, right);
      root = tmp_root;
      pq.push(tmp_root);
    }
    if (root == nullptr) {
      // means only 1 ch
      root = new HFMNode(_data[0], freq[_data[0]]);
    }
    assert(root);
    Huffman<msg_size> huffman;
    std::copy(data, data + msg_size, huffman.data);
    // generate code for canonical huffman tree
    std::function<void(HFMNode*, uint8_t)> gen_code = [&](HFMNode* node, uint8_t code_length)->void{
      if (node == nullptr) {
        return;
      }
      if (node->isLeaf()) {
        huffman.tree_data[code_length + (code_length == 0)].insert(node->ch);
        return;
      }
      gen_code(node->left, code_length + 1);
      gen_code(node->right, code_length + 1);
    };
    gen_code(root, 0);
    delete root;
    // create canonical tree
    std::map<int8_t, std::string> codes = huffman.generateCanonicalTreeFromData();
    // encode
    size_t encoded_data_size_bits = 0;
    for (size_t i = 0; i < actual_msg_size; i++) {
      std::string code = codes[_data[i]];
      size_t code_size = code.length();
      for (size_t k = 0; k < code_size; k++) {
        huffman.encoded_data.set(encoded_data_size_bits + k, code[k] - '0');
      }
      encoded_data_size_bits += code_size;
    }
    //std::cout << encoded_data_size_bits << '\n';
    //std::cout << huffman.encoded_data.to_string() << '\n';
    assert(encoded_data_size_bits / 8 + (encoded_data_size_bits % 8 != 0) <= 255);
    huffman.encoded_data_size = encoded_data_size_bits / 8;
    huffman.encoded_data_size_extra_bits = encoded_data_size_bits % 8;
    return huffman;
  }

  static Huffman<msg_size> fromDump(const uint8_t* data, const uint8_t& size) {
    const std::vector<size_t>& zigzag = Huffman<msg_size>::zigzagIndexes[msg_size];
    Huffman<msg_size> huffman;
    uint8_t i = 0;
    uint8_t tree_data_size = data[i++];
    assert(tree_data_size <= size);
    while (i - 1 < tree_data_size) {
      uint8_t ch_info = data[i++];
      uint8_t code_count = ch_info >> 5;
      uint8_t chs_count = (ch_info & 31) + 1;
      for (uint8_t j = 0; j < chs_count; j++) {
        huffman.tree_data[code_count].insert(data[i++]);
      }
    }
    huffman.generateCanonicalTreeFromData();
    huffman.encoded_data_size = data[i++];
    huffman.encoded_data_size_extra_bits = data[i++];
    assert(tree_data_size + huffman.encoded_data_size + (huffman.encoded_data_size_extra_bits > 0) + 3 <= size);
    for (uint8_t j = 0; j < huffman.encoded_data_size + (huffman.encoded_data_size_extra_bits > 0); j++) {
      size_t _j = j * 8;
      std::bitset<8> tmp = data[i++];
      for (size_t jj = 0; jj < 8; jj++) {
        huffman.encoded_data.set(_j + jj, tmp.test(jj));
      }
    }
    //std::cout << huffman.encoded_data.to_string() << '\n';
    HFMNode* node = huffman.root;
    size_t encoded_data_size_bits = huffman.encoded_data_size * 8 + huffman.encoded_data_size_extra_bits;
    size_t k = 0;
    for (size_t j = 0; j < encoded_data_size_bits; j++) {
      bool bit = huffman.encoded_data.test(j);
      if (!bit) {
        node = node->left;
      } else {
        node = node->right;
      }
      if (node->isLeaf()) {
        huffman.data[zigzag[k++]] = node->ch;
        node = huffman.root;
      }
    }
    /*
    for (int j = 0; j < 8; j++) {
      for (int i = 0; i < 8; i++) {
        std::cout << (int)huffman.data[i + j * 8] << '\t';
      }
      std::cout << '\n';
    }
    */
    return huffman;
  }

  int8_t* getData() {
    return data;
  }

  void printTree() {
    // https://stackoverflow.com/questions/36802354/print-binary-tree-in-a-pretty-way-using-c/51730733#51730733
    std::function<void(const std::string&, HFMNode*, bool)> print_rec = [&](const std::string& prefix, HFMNode* node, bool is_left)->void{
      if (node == nullptr) {
        return;
      }
      std::cout << prefix;
      std::cout << (is_left ? "L├──" : "R└──");
      std::cout << (int)node->ch << '\n';
      print_rec(prefix + (is_left ? " │   " : "    "), node->left, true);
      print_rec(prefix + (is_left ? " │   " : "    "), node->right, false);
    };
    print_rec("", root, false);
  }

  void dump(uint8_t*& dat, uint8_t& size) {
    // 1 byte for tree code size chunk
    // each tree code: 1 byte for code length (1..9) and ch count (1..32) + ch count bytes
    // 1 byte for message size
    // message size bytes for message
    assert(msg_size <= 64 && "Not implemented for msg_size more than 64"); // effective for 64
    size = 3 + encoded_data_size + (encoded_data_size_extra_bits > 0); // tree code size chunk + message size chunk + message
    for (auto it = tree_data.begin(); it != tree_data.end(); it++) {
      int tmp = it->second.size();
      if (tmp <= 32) {
        tmp += 1;
      } else {
        tmp += 2;
      }
      size += tmp;
    }
    //std::cout << "Chunk size " << size << '\n';
    dat = new uint8_t[size];
    assert(size - 3 - encoded_data_size - (encoded_data_size_extra_bits > 0) <= 255);
    uint8_t i = 0;
    dat[i++] = size - 3 - encoded_data_size - (encoded_data_size_extra_bits > 0);
    for (auto it = tree_data.begin(); it != tree_data.end(); it++) {
      auto& chs = it->second;
      assert(chs.size() <= 64);
      uint8_t chs_count = chs.size();
      assert(it->first <= 7);
      uint8_t code_count = it->first;
      auto chs_iter = chs.begin();
temp_label:
      uint8_t _chs_count = std::min<uint8_t>(chs_count, 32);
      dat[i++] = (code_count << 5) | (_chs_count - 1);
      for (uint8_t j = 0; j < _chs_count; j++) {
        dat[i++] = *chs_iter;
        chs_iter = std::next(chs_iter);
      }
      if (chs_count > 32) {
        chs_count -= 32;
        goto temp_label;
      }
    }
    dat[i++] = encoded_data_size;
    dat[i++] = encoded_data_size_extra_bits;
    for (uint8_t j = 0; j < encoded_data_size + (encoded_data_size_extra_bits > 0); j++) {
      std::bitset<8> tmp;
      size_t _j = j * 8;
      for (size_t jj = 0; jj < 8; jj++) {
        tmp.set(jj, encoded_data.test(_j + jj));
      }
      dat[i++] = tmp.to_ulong();
    }
    //std::cout << "SIZE " << size << '\n';
  }
protected:
  std::map<uint8_t, std::set<int8_t>> tree_data;
  HFMNode* root;
  int8_t data[msg_size];
  std::bitset<msg_size * 8> encoded_data;
  uint8_t encoded_data_size; // in bytes
  uint8_t encoded_data_size_extra_bits;
protected:
  Huffman() : data{0} {}
  std::map<int8_t, std::string> generateCanonicalTreeFromData() {
    root = new HFMNode();
    std::map<int8_t, std::string> codes;
    uint8_t curr_len = 0, next_len = 0;
    uint64_t c_code = 0;
    for (auto it = tree_data.begin(); it != tree_data.end(); it++) {
      std::set<int8_t> s = it->second;
      curr_len = it->first;
      for (auto i = s.begin(); i != s.end(); i++) {
        std::string d = std::bitset<sizeof(c_code)*8>(c_code).to_string().substr(sizeof(c_code)*8 - curr_len, sizeof(c_code)*8);
        //std::cout << (int)*i << ':' << d << '\n';
        codes[*i] = d;
        HFMNode* tmp = root;
        HFMNode** direction;
        for (auto c : d) {
          if (c == '0') {
            direction = &tmp->left;
          } else {
            direction = &tmp->right;
          }
          if (*direction == nullptr) {
            *direction = new HFMNode();
          }
          tmp = *direction;
        }
        tmp->ch = *i;
        if (next(i) != s.end() || next(it) == tree_data.end()) {
          next_len = curr_len;
        } else {
          next_len = next(it)->first;
        }
        c_code = (c_code + 1) << (next_len - curr_len);
      }
    }
    return codes;
  }
protected:
  static std::map<size_t, std::vector<size_t>> zigzagIndexes;
};

template<size_t msg_size>
std::map<size_t, std::vector<size_t>> Huffman<msg_size>::zigzagIndexes = {
  {64, {
    0, 8, 1, 2, 9, 16, 24, 17, 10, 3, 4, 11, 18, 25, 32, 40, 33, 26, 19, 12, 5, 6, 13, 20, 27, 34, 41, 48, 56, 49, 42, 35, 28, 21, 14, 7, 15, 22, 29, 36, 43, 50, 57, 58, 51, 44, 37, 30, 23, 31, 38, 45, 52, 59, 60, 53, 46, 39, 47, 54, 61, 62, 55, 63,
  }}
};
