mkdir -p build/linux_cuda
# shellcheck disable=SC2164
cd build/linux_cuda
# export cross_compile_toolchain=/home/s4129/software/gcc-arm-8.3-2019.03-x86_64-arm-linux-gnueabihf
cmake -DCMAKE_SYSTEM_NAME=Linux \
  -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_WITH_SAMPLE=ON \
  -DBUILD_WITH_TEST=ON \
  -DENABLE_BENCHMARK=ON \
  -DENABLE_USE_LFW_DATA=ON \
  -DENABLE_TEST_EVALUATION=ON \
  -DGLOBAL_INFERENCE_BACKEND_USE_MNN_CUDA=ON \
  -DLINUX_MNN_CUDA=/home/tunm/software/MNN-2.7.0/build_cuda/install \
  -DBUILD_SHARED_LIBS=ON ../..

make -j4
