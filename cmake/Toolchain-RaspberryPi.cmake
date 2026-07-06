#
# CMake toolchain file for cross-compiling humid for Raspberry Pi on macOS
#
# Prerequisites:
#   1. Cross-compiler: brew install messense/macos-cross-toolchains/aarch64-unknown-linux-gnu
#      (or download from https://github.com/raspberrypi/tools)
#
#   2. Sysroot: copy /lib, /usr from your RPi:
#      mkdir -p ~/rpi-sysroot
#      rsync -avz pi@raspberrypi:/lib ~/rpi-sysroot/
#      rsync -avz pi@raspberrypi:/usr ~/rpi-sysroot/
#
# Build: cmake -B build_pi \
#            -DCMAKE_TOOLCHAIN_FILE=cmake/Toolchain-RaspberryPi.cmake \
#            -DNANOGUI_USE_GLES=ON \
#            -DCMAKE_BUILD_TYPE=Release
#        cmake --build build_pi
#
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

# -- Cross-compiler paths (adjust to your toolchain) --
set(TOOLCHAIN_PREFIX aarch64-unknown-linux-gnu)
set(CMAKE_C_COMPILER   ${TOOLCHAIN_PREFIX}-gcc)
set(CMAKE_CXX_COMPILER ${TOOLCHAIN_PREFIX}-g++)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# -- Sysroot (RPi filesystem root) --
set(RPI_SYSROOT $ENV{HOME}/rpi-sysroot)
set(CMAKE_SYSROOT ${RPI_SYSROOT})
set(CMAKE_FIND_ROOT_PATH ${RPI_SYSROOT})

# -- Locate pkg-config inside sysroot --
set(PKG_CONFIG_EXECUTABLE /usr/bin/pkg-config)
set(ENV{PKG_CONFIG_SYSROOT_DIR} ${RPI_SYSROOT})
set(ENV{PKG_CONFIG_PATH} ${RPI_SYSROOT}/usr/lib/aarch64-linux-gnu/pkgconfig)
