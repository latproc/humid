-include LocalMakefile

ifndef JOBS
    JOBS:=-j4
endif

# Vendored Clockwork client. Humid sources can call new cw_client symbols
# after a git pull; the staged archive is not rebuilt by the humid target.
client:
	$(MAKE) -C clockwork client-install JOBS=$(JOBS)

all: client
	[ -d "build" ] || mkdir build
	cd build && cmake -DCMAKE_BUILD_TYPE=Release .. && make $(JOBS) && make install

# Full panel update: pull master, build the vendored client, then build Humid.
# See scripts/update-panel.sh --help
panel-update:
	./scripts/update-panel.sh --jobs $(subst -j,,$(JOBS))

panel-update-restart:
	./scripts/update-panel.sh --jobs $(subst -j,,$(JOBS)) --restart


# Release is the default tree under build/ (not build/Release).
release: client
	[ -d "build" ] || mkdir build
	cd build && cmake -DCMAKE_BUILD_TYPE=Release .. && make $(JOBS)

release-install: release
	cd build && make $(JOBS) install

# Optional Debug build in a separate tree so it does not clobber build/.
debug:
	[ -d "build-debug" ] || mkdir build-debug
	cd build-debug && cmake -DCMAKE_BUILD_TYPE=Debug .. && make $(JOBS)

debug-install:	debug
	cd build-debug && make $(JOBS) install

xcode:
	[ -d "xcode" ] || mkdir xcode
	cd xcode && cmake -G Xcode .. && open humid.xcodeproj
#	[ -d "xcode/Debug" ] || mkdir xcode/Debug
#	cd xcode/Debug && cmake -G Xcode -DCMAKE_BUILD_TYPE=Debug ../.. && open humid.xcodeproj

test:
	[ -d ".test" ] || mkdir .test
	cd .test && cmake -DCMAKE_BUILD_TYPE=Debug -DRUN_TESTS=ON .. && make $(JOBS) && make test
