#include <SDL3/SDL.h>
#include <cassert>
#include <iostream>
#include <map>
#include <functional>
#include <algorithm>
#include "MyYUV.hpp"
#include "YUVCompressor.hpp"
#include "YUVConverter.hpp"
#include "Huffman.hpp"
#include "SDLWindowsYUV.hpp"

static std::string output_name;

static std::string commands_before_show;

static int show_count = 0;

static MyYUV* myyuv_backup;

static std::vector<SDLWindowsYUV*> yuv_windows;

static std::map<std::string, std::function<void(MyYUV&, std::vector<std::string>&)>> action_map = {
  {"show", [](MyYUV& myyuv, std::vector<std::string>&)->void{
    yuv_windows.push_back(new SDLWindowsYUV(myyuv, output_name + " " + commands_before_show + " (" + std::to_string(++show_count) + ")", myyuv.getWidth(), myyuv.getHeight()));
    commands_before_show = "";
  }},
  {"reset", [](MyYUV& myyuv, std::vector<std::string>&)->void{
    myyuv = myyuv_backup->clone();
  }},
};

static void addCompressionToActionMap() {
  for (auto const& compression : YUVCompressor::compression_map) {
    const std::string& alg_name = compression.first;
    action_map[alg_name] = [&alg_name](MyYUV& myyuv, std::vector<std::string>& params)->void{
      myyuv.compress(alg_name, params);
    };
  }
}

extern void test_dct();

static void pre_test() {
  int16_t tmp[64];
  for (int i = 0; i < 64; i++) {
    tmp[i] = (64 - i);
  }
  for (int j = 0; j < 8; j++) {
    for (int i = 0; i < 8; i++) {
      std::cout << (int)tmp[i + j * 8] << '\t';
    }
    std::cout << '\n';
  }
  std::cout << '\n';
  Huffman<64> h = Huffman<64>::fromData(tmp);
  //h.printTree();
  uint8_t* data;
  uint8_t size;
  h.dump(data, size);
  h = Huffman<64>::fromDump(data, size);
  int16_t* t =  h.getData();
  for (int j = 0; j < 8; j++) {
    for (int i = 0; i < 8; i++) {
      std::cout << (int)t[i + j * 8] << '\t';
      assert(t[i + j * 8] == tmp[i + j * 8]);
    }
    std::cout << '\n';
  }
  std::cout << "\n\nTEST DCT\n";
  test_dct();
}

// Init stuff
static void preMain() {
  addCompressionToActionMap();
  YUVCompressor::precompute();
  //pre_test();
}

static std::vector<std::string> split(std::string& str, const std::string& del) {
  std::vector<std::string> res;
  size_t pos;
  const size_t del_length = del.length();
  while ((pos = str.find(del)) != std::string::npos) {
    res.push_back(str.substr(0, pos));
    str.erase(0, pos + del_length);
  }
  res.push_back(str);
  return res;
}

int main(int argc, char* argv[]) {
  preMain();
  if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
    std::cout << "Error initializing SDL: " << SDL_GetError() << '\n';
    return 1;
  }
  std::cout << "BMP /path/to/file.bmp format [output_name] [actions...]\n";
  std::cout << "(not implemented yet) YUV /path/to/file.yuv [output_name] [actions...]\n";
  std::cout << "YUV formats:";
  for (auto const& format : YUVConverter::yuv_types_map) {
    std::cout << ' ' << format.first;
  }
  std::cout << '\n';
  std::cout << "Also can use format `RGB` instead.\n";
  std::cout << "Actions:";
  for (auto const& action : action_map) {
    std::cout << ' ' << action.first;
  }
  std::cout << "\n\n";
  assert(argc > 2);
  MyYUV myyuv = MyYUV::from(argv[1], argv[2], argc > 3 ? argv[3] : "");
  myyuv_backup = new MyYUV(myyuv.clone());
  int argi = std::string(argv[1]) == "YUV" ? 3 : 4;
  if (argi < argc) {
    output_name = argv[argi];
  } else {
    return 0;
  }
  argi++;
  const int argi_start = argi;
  for (; argi < argc; argi++) {
    std::cout << (argi - argi_start + 1) << ") " << argv[argi] << '\n';
    if (std::string("show") != argv[argi]) {
      if (!commands_before_show.empty()) {
        commands_before_show += ";";
      }
      commands_before_show += argv[argi];
    }
    std::string action = argv[argi];
    std::vector<std::string> params;
    size_t pos;
    if ((pos = action.find(":")) != std::string::npos) {
      std::string params_str = action.substr(pos + 1);
      action = action.substr(0, pos);
      params = split(params_str, ",");
    }
    auto search = action_map.find(action);
    if (search != action_map.end()) {
      search->second(myyuv, params);
    } else {
      std::cout << "Unknown action " << action << '\n';
    }
  }
  bool close = false;
  std::queue<SDLWindowsYUV*> deletion_queue;
  while (!close) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      for (auto& w : yuv_windows) {
        if(!w->handleEvent(event)) {
          deletion_queue.push(w);
        }
      }
      switch (event.type) {
        case SDL_EVENT_QUIT:
          close = true;
          break;
      }
    }
    while (!deletion_queue.empty()) {
      SDLWindowsYUV* w = deletion_queue.front();
      yuv_windows.erase(std::find(yuv_windows.begin(), yuv_windows.end(), w));
      delete w;
      deletion_queue.pop();
    }
    SDL_Delay(40);
  }
  SDL_Quit();
  delete myyuv_backup;
  return 0;
}
