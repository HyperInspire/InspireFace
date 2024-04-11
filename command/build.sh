mkdir -p build/linux_local
# shellcheck disable=SC2164
cd build/linux_local
# export cross_compile_toolchain=/home/s4129/software/gcc-arm-8.3-2019.03-x86_64-arm-linux-gnueabihf
cmake -DBUILD_WITH_SAMPLE=ON \
  -DBUILD_WITH_TEST=ON \
  -DENABLE_BENCHMARK=ON \
  -DENABLE_USE_LFW_DATA=ON \
  -DENABLE_TEST_EVALUATION=ON \
  -DBUILD_SHARED_LIBS=ON ../..

make -j4
