ASTYLE := $(shell command -v astyle 2>/dev/null)
CLANGFORMAT := $(shell command -v clang-format 2>/dev/null)
ifeq ($(JOBS),)
  JOBS=-j3
endif

all:	
	[ -d "build" ] || mkdir build
	cd build && cmake .. && make $(JOBS)


release:
	[ -d "build" ] || mkdir build
	[ -d "build/Release" ] || mkdir build/Release
	cd build/Release && cmake -DCMAKE_BUILD_TYPE=Release ../.. && make $(JOBS)

release-install:
	cd build/Release && make $(JOBS) install

debug:
	[ -d "build" ] || mkdir build
	[ -d "build/Debug" ] || mkdir build/Debug
	cd build/Debug && cmake -DCMAKE_BUILD_TYPE=Debug ../.. && make $(JOBS)

debug-install:	debug
	cd build/Debug && make $(JOBS) install

xcode:
	[ -d "xcode" ] || mkdir xcode
	cd xcode && cmake -G Xcode .. && open humid.xcodeproj
#	[ -d "xcode/Debug" ] || mkdir xcode/Debug
#	cd xcode/Debug && cmake -G Xcode -DCMAKE_BUILD_TYPE=Debug ../.. && open humid.xcodeproj

test:
	[ -d ".test" ] || mkdir .test
	cd .test && cmake -DCMAKE_BUILD_TYPE=Debug -DRUN_TESTS=ON .. && make $(JOBS) && make test

	cd .test && cmake -DCMAKE_BUILD_TYPE=Debug -DRUN_TESTS=ON .. && make $(JOBS) && make test

style:
ifdef CLANGFORMAT
	clang-format --style=file -i `find src -name \*.c -o -name \*.cpp -o -name \*.h -o -name \*.hpp`
	clang-format --style=file -i `find tests -name \*.c -o -name \*.cpp -o -name \*.h -o -name \*.hpp`
endif

