mkdir -p build/linux_cuda
# shellcheck disable=SC2164
cd build/linux_cuda
# export cross_compile_toolchain=/home/s4129/software/gcc-arm-8.3-2019.03-x86_64-arm-linux-gnueabihf
cmake -DCMAKE_SYSTEM_NAME=Linux \
  -DCMAKE_BUILD_TYPE=Release \
  -DISF_BUILD_WITH_SAMPLE=OFF \
  -DISF_BUILD_WITH_TEST=OFF \
  -DISF_ENABLE_BENCHMARK=OFF \
  -DISF_ENABLE_USE_LFW_DATA=OFF \
  -DISF_ENABLE_TEST_EVALUATION=OFF \
  -DISF_GLOBAL_INFERENCE_BACKEND_USE_MNN_CUDA=ON \
  -DISF_LINUX_MNN_CUDA=/home/tunm/software/MNN-2.7.0/build_cuda/install \
  -DISF_BUILD_SHARED_LIBS=ON ../..

make -j4
