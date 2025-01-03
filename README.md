# yuv-manipulations-1

Requirements: SDL3, OpenMP, the compiler with C++17 support, cmake or make.

Build:
```
mkdir -p build
cd build
cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo ..
cmake --build .
cd ..
```
or make with gcc:
```
make
```

Run with arguments:
```
BMP /path/to/file.bmp format [output_name] [actions...]
```

Note: BMP `ARGB` or `XRBG` only.

Example:
```
BMP "chef-with-trumpet.bmp" IYUV test-chef-with-trumpet show DCT:50,50,50 show
```

YUV formats: `IYUV`

Also can use format `RBG`.

Actions: `DCT` `show`

Parameters for `DCT`: quality for each channel. For example: `DCT:50,50,50`. `DCT` only works for planar storage types like `IYUV`.

Check console output for:
- Original RGB size
- Original RGB size without alpha channel
- Sizes after manipulations in bytes
- Compression, decompression and converting time in ms

TODOs:
- Bug: broken colors for q > 75 in DCT (for some reason)
- Dump/restore YUV
