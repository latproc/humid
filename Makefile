all:	
	[ -d "build" ] || mkdir build
	cd build && cmake .. && make -j4


release:
	[ -d "build" ] || mkdir build
	[ -d "build/Release" ] || mkdir build/Release
	cd build/Release && cmake -DCMAKE_BUILD_TYPE=Release ../.. && make -j 3

release-install:
	cd build/Release && make -j 6 install

mingw:
	[ -d "mingw_build" ] || mkdir mingw_build
	cd mingw_build && cmake .. -DCMAKE_TOOLCHAIN_FILE=../lib/clockwork/cmake_modules/mingw_toolchain.cmake -DMINGW_BUILD=YES && make

debug:
	[ -d "build" ] || mkdir build
	[ -d "build/Debug" ] || mkdir build/Debug
	cd build/Debug && cmake -DCMAKE_BUILD_TYPE=Debug ../.. && make -j

debug-install:	debug
	cd build/Debug && make -j install

xcode:
	[ -d "xcode" ] || mkdir xcode
	cd xcode && cmake -G Xcode .. && xcodebuild -parallelizeTargets -jobs 6
#	[ -d "xcode/Debug" ] || mkdir xcode/Debug
#	cd xcode/Debug && cmake -G Xcode -DCMAKE_BUILD_TYPE=Debug ../.. && open humid.xcodeproj

debug-test:
	cd build/Debug && make test

debug-install:	debug
	cd build/Debug && make install
