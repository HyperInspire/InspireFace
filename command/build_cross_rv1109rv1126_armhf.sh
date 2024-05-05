mkdir -p build/rv1109rv1126_armhf
# shellcheck disable=SC2164
cd build/rv1109rv1126_armhf
# export cross_compile_toolchain=/home/s4129/software/gcc-arm-8.3-2019.03-x86_64-arm-linux-gnueabihf
cmake -DCMAKE_SYSTEM_NAME=Linux \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_SYSTEM_VERSION=1 \
  -DCMAKE_SYSTEM_PROCESSOR=armv7 \
  -DCMAKE_C_COMPILER=$ARM_CROSS_COMPILE_TOOLCHAIN/bin/arm-linux-gnueabihf-gcc \
  -DCMAKE_CXX_COMPILER=$ARM_CROSS_COMPILE_TOOLCHAIN/bin/arm-linux-gnueabihf-g++ \
  -DCMAKE_CXX_FLAGS="${CMAKE_CXX_FLAGS} -flax-vector-conversions" \
  -DTARGET_PLATFORM=armlinux \
  -DISF_BUILD_LINUX_ARM7=ON \
  -DISF_ENABLE_RKNN=ON \
  -DISF_RK_DEVICE_TYPE=RV1109RV1126 \
  -DISF_BUILD_WITH_SAMPLE=OFF \
  -DISF_BUILD_WITH_TEST=OFF \
  -DISF_ENABLE_BENCHMARK=OFF \
  -DISF_ENABLE_USE_LFW_DATA=OFF \
  -DISF_ENABLE_TEST_EVALUATION=OFF \
  -DISF_BUILD_SHARED_LIBS=ON ../..

make -j4
make install

