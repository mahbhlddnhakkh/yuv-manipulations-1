# yuv-manipulations-1

Build:
```
mkdir -p build
cd build
cmake ..
cmake --build .
cd ..
```
or
```
make
```

Run:
```
BMP /path/to/file.bmp format [output_name] [actions...]
```

Note: `ARGB` or `XRBG` only.

Example:
```
BMP "chef-with-trumpet.bmp" IYUV test-chef-with-trumpet show DCT:50,50,50 show
```

YUV formats: `IYUV`

Also can use format `RBG`.

Actions: `DCT` `show`

Parameters for `DCT`: quality for each channel. For example: `DCT:50,50,50`. `DCT` only works for `IYUV`.

Can't dump/save for now.

Check console output for:
- Original RGB size
- Original RGB size without alpha channel
- Sizes after manipulations.
