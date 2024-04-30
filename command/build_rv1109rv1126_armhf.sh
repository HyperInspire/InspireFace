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
  -DTARGET_PLATFORM=armlinux \
  -DBUILD_LINUX_ARM7=ON \
  -DENABLE_RKNN=ON \
  -DRK_DEVICE_TYPE=RV1109RV1126 \
  -DBUILD_WITH_SAMPLE=ON \
  -DBUILD_WITH_TEST=ON \
  -DENABLE_BENCHMARK=ON \
  -DENABLE_USE_LFW_DATA=ON \
  -DENABLE_TEST_EVALUATION=ON \
  -DBUILD_SHARED_LIBS=ON ../..

make -j4
make install

