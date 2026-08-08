-include LocalMakefile

ifndef JOBS
    JOBS:=-j4
endif

all:	
	[ -d "build" ] || mkdir build
	cd build && cmake .. && make $(JOBS) && make install

# Full panel update: pull master, build the vendored client, then build Humid.
# See scripts/update-panel.sh --help
panel-update:
	./scripts/update-panel.sh --jobs $(subst -j,,$(JOBS))

panel-update-restart:
	./scripts/update-panel.sh --jobs $(subst -j,,$(JOBS)) --restart


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
