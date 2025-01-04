# yuv-manipulations-1

### Requirements:
SDL3, OpenMP, the compiler with C++17 support, cmake or make.

### Build:
With cmake:
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

### Run with arguments:
```
BMP /path/to/file.bmp format [output_name] [actions...]
```

Note: BMP `ARGB` or `XRBG` only.

### Example:
```
BMP "chef-with-trumpet.bmp" IYUV test-chef-with-trumpet show DCT:50,50,50 show
```

### YUV formats:
- `IYUV`: YUV 4:2:0 with planar storage type.

Also can use format `RBG`.

### Actions:
- `DCT`: DCT compression
- `DCTChroma`: DCT but applied only to chroma channels (U and V). Leaves Y (luma) uncompressed
- `show`: show current resulting image with SDL3
- `reset`: resets the image to the state prior to any actions

Parameters for `DCT`: quality for each channel (maximum 3 parameters). For example: `DCT:50,50,50`. `DCT` only works for planar storage types like `IYUV`. By default all parameters are 50.

Parameters for `DCTChroma`: quality channels for U and V (maximum 3 parameters), Y is not compressed. For example: `DCTChroma:50,50`. `DCTChroma` only works for planar storage types like `IYUV`. By default all parameters are 50.

You can select implementations for compression in `YUVCompressorFeatures.hpp`.

### You can check console output for:
- Original RGB size in bytes
- Original RGB size without alpha channel in bytes
- Sizes after manipulations in bytes
- Compression, decompression and converting time in ms

### TODOs:
- Bug: broken colors for q > 75 in DCT and DCTChroma. I think I've found the cause: integer overflow when saving matrix after DCT transformation: attempt to put 11 bit signed number into 8 bit signed number. I'll try to fix this later.
- Dump/restore YUV
- Change (most) asserts to C++ exceptions
- Generate compression algorithms and YUV formats with macros
