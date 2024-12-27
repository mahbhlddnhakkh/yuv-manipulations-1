#!/bin/bash
export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH
set -e

make clean
make
./YUVExp BMP "chef-with-trumpet.bmp" RGB test-chef-with-trumpet-rgb show > /dev/null &
./YUVExp BMP "chef-with-trumpet.bmp" IYUV test-chef-with-trumpet show DCT:50,50,50 show
make clean
