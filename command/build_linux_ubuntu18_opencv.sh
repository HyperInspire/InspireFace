mkdir -p build/linux_x86_ubuntu18_opencv/
# shellcheck disable=SC2164
cd build/linux_x86_ubuntu18_opencv
# export cross_compile_toolchain=/home/s4129/software/gcc-arm-8.3-2019.03-x86_64-arm-linux-gnueabihf
cmake -DCMAKE_BUILD_TYPE=Release \
  -DISF_BUILD_WITH_SAMPLE=ON \
  -DISF_BUILD_WITH_TEST=OFF \
  -DISF_ENABLE_BENCHMARK=OFF \
  -DISF_ENABLE_USE_LFW_DATA=OFF \
  -DISF_ENABLE_TEST_EVALUATION=OFF \
  -DOpenCV_DIR=3rdparty/inspireface-precompile/opencv/4.5.1/opencv-ubuntu18-x86/lib/cmake/opencv4 \
  -DISF_BUILD_SHARED_LIBS=ON ../../

make -j4
make install