mkdir -p build/armhf_rv1109rv1126
# shellcheck disable=SC2164
cd build/armhf_rv1109rv1126
# export cross_compile_toolchain=/home/s4129/software/gcc-arm-8.3-2019.03-x86_64-arm-linux-gnueabihf
cmake -DCMAKE_SYSTEM_NAME=Linux \
  -DCMAKE_SYSTEM_VERSION=1 \
  -DCMAKE_SYSTEM_PROCESSOR=armv7 \
  -DCMAKE_C_COMPILER=$ARM_CROSS_COMPILE_TOOLCHAIN/bin/arm-linux-gnueabihf-gcc \
  -DCMAKE_CXX_COMPILER=$ARM_CROSS_COMPILE_TOOLCHAIN/bin/arm-linux-gnueabihf-g++ \
  -DTARGET_PLATFORM=armlinux \
  -DBUILD_LINUX_ARM7=ON \
  -DENABLE_RKNN=ON \
  -DRK_DEVICE_TYPE=RV1109RV1126 \
  -DBUILD_WITH_SAMPLE=OFF \
  -DBUILD_WITH_TEST=OFF \
  -DBUILD_SHARED_LIBS=ON ../../

make -j4