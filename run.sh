#!/bin/bash
export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH
set -e

RGB_PID=""

int_handler()
{
  echo "Interrupted."
  if [ -n "$RGB_PID" ]; then
    kill $RGB_PID
  fi
  exit 1
}

trap 'int_handler' INT

make clean
make
./YUVExp BMP "chef-with-trumpet.bmp" RGB test-chef-with-trumpet-rgb show > /dev/null &
RGB_PID=$!
./YUVExp BMP "chef-with-trumpet.bmp" IYUV test-chef-with-trumpet show DCT:50,50,50 show
wait $RGB_PID
make clean
