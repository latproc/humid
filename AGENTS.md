# Humid Agent Context

## Repository And Submodules

- On `master`, `clockwork/` is vendored source, not a Git submodule. It contains
  the trimmed Clockwork client code Humid needs under `clockwork/src/`; the
  Clockwork repository's `iod/` tree is deliberately not copied into Humid.
- `lib/nanogui/` remains a Git submodule pinned by the parent Humid commit.
- Do not update a submodule only by moving the parent pointer to an untested
  remote commit. Keep unrelated dirty submodule changes out of commits and
  inspect status and diffs in both the parent and each affected submodule.
- A local submodule commit is not available to other machines until it is
  pushed. Do not push unless the operator explicitly approves publication.

## Vendored Clockwork Client Source Sync

Humid carries only the Clockwork source files needed by `cw_client`. The
canonical counterparts live under `iod/src/` in a separate Clockwork checkout.
Use `clockwork/compare-clockwork-src.sh` to compare or transfer them; do not
manually copy whole Clockwork directories into Humid.

Set `CLOCKWORK` to the root of the separate Clockwork checkout, not its `iod/`
or `iod/src/` directory:

```bash
CLOCKWORK=/path/to/clockwork ./clockwork/compare-clockwork-src.sh
CLOCKWORK=/path/to/clockwork ./clockwork/compare-clockwork-src.sh --list
CLOCKWORK=/path/to/clockwork ./clockwork/compare-clockwork-src.sh --pull [filename]
CLOCKWORK=/path/to/clockwork ./clockwork/compare-clockwork-src.sh --push [filename]
```

- With no option, the script shows unified content differences; `--list`
  prints only differing filenames. License-header differences are ignored.
- `--pull` copies bodies from Clockwork `iod/src/` into Humid
  `clockwork/src/`. `--push` copies bodies in the other direction.
- An optional filename limits `--pull` or `--push` to one file. It may be
  written as `src/Foo.cpp` or `Foo.cpp`; otherwise all local vendored source
  files are considered.
- Transfers preserve the destination file's existing initial license header.
  This matters because the vendored Humid copies use Humid's license header,
  which differs from the canonical Clockwork files.
- The script reports files that are missing from the separate Clockwork tree;
  it does not create a new counterpart for a Humid-only file.
- After a pull or push, inspect the diffs in both repositories and build/test
  both affected sides. A push changes the separate Clockwork working tree but
  does not create a commit or publish it; never `git push` either repository
  without explicit operator approval.

## Production Compatibility

The deployed 2G4C panels include Ubuntu 18.04 systems with older build and
runtime interfaces. A change that builds on a development computer is not
proof that it builds or runs on these panels.

### ZeroMQ

- Production panels use `/usr/local/lib/libzmq.so.5.2.3` and an older `zmq.hpp`
  that provides blocking `monitor()` but not `init()`/`check_event()`.
- Newer macOS/dev systems have cppzmq with `init()` + `check_event()`.
- Clockwork `SocketMonitor` selects the API at compile time; do not force
  only one path. Prefer the abortable `check_event` loop when available.
- Track Clockwork humid-client / CHANNEL / ZMQ fixes on
  **`prod-client-zmq-fix`** (clockwork line **C**, `scope: client-zmq`).
  On `master`, transfer tested client sources into the vendored `clockwork/src/`
  tree with `compare-clockwork-src.sh --pull`. The older
  `cw-no-ec-tools-compatiblity` deployment branch pins a Clockwork submodule.
  Port shared sources to plant lines A/B as needed. Details:
  [docs/CLOCKWORK_CLIENT_BRANCHES.md](docs/CLOCKWORK_CLIENT_BRANCHES.md).
  Do **not** treat `prod-experimental-mqtt-fix` as the client trunk anymore
  (it remains plant `iod_sdo` / line A).
- Humid must not dereference public `SubscriptionManager` implementation
  fields such as `monit_setup`. Register callbacks through Clockwork methods
  such as `addSetupResponder()` so construction and access use one compiled
  class layout.

### Boost

- `cw_client` (humid) must build on Ubuntu Bionic **Boost 1.65** without
  `boost/context/fiber.hpp` (Bionic packages do not ship that header). JSON
  path evaluation batches parser tokens instead of using fibers. Full iod
  process/scheduler still use Boost.Context when available.
- Some panels have Boost 1.74 headers and a small/header-only
  `libboost_date_time.so.1.74.0` that does not export the legacy
  `boost::gregorian::greg_month` string symbols used by Clockwork.
- Those panels also provide `libboost_date_time.so.1.65.1`, which supplies the
  required symbols.
- Humid's CMake appends the 1.65.1 compatibility library when it is present.
  Do not remove that fallback without checking the exported symbols on every
  supported production OS.

### CMake

- Panels range from CMake **3.5.1** (old Ubuntu) to **3.10+**. Clockwork
  `iod/CMakeLists.txt` accepts 3.5+ for client-install; gate newer-only APIs.
- Do not assume `cmake -S ... -B ...` is supported. Use an out-of-tree
  directory and run `cmake ..` from it.
- Older CMake accepts one `--target` per `cmake --build` invocation. Build
  `humid` and `hmifile_check` in separate commands when needed.
- `scripts/update-panel.sh` picks the newest `cmake`/`cmake3` on PATH.

### Raspberry Pi / OpenGL ES (merged from feature-rpi)

- RPi graphics support lives on the standard panel branch
  (`cw-no-ec-tools-compatiblity`); do not maintain a long-lived separate
  product line for Pi-only GLES unless necessary.
- **Build-time auto-detect:** CMake turns `NANOGUI_USE_GLES` **ON** by default
  on Linux ARM (`aarch64`/`arm*`) and when `/proc/device-tree/model` (or
  RPi OS markers) indicates a Raspberry Pi, or when using
  `cmake/Toolchain-RaspberryPi.cmake`. Override with
  `-DNANOGUI_USE_GLES=ON|OFF`. Apple Silicon keeps desktop OpenGL.
- On GLES builds, GLFW Wayland and X11 backends are both enabled by default;
  **runtime** compositor choice is Wayland if `WAYLAND_DISPLAY` is set, else
  X11 (`DISPLAY`).
- **Runtime:** `humid` logs OS/arch/GL build/presentation and warns if an
  RPi host is running a desktop-OpenGL binary (or the reverse).
- NanoGUI pin must include GLES + GLFW 3.4 Wayland support (see
  `lib/nanogui` history: `NANOGUI_USE_GLES`, upgraded GLFW).
- Cross-build from macOS: `cmake/Toolchain-RaspberryPi.cmake` (sysroot +
  aarch64 cross-compiler). Still prefer native builds on the Pi when practical.
- Keep the normal panel **client-install** path for `cw_client`; do not
  replace it with RPi-only in-tree `cmake -S/-B` clockwork builds that break
  CMake 3.5 panels.

## Panel fleet update (preferred)

Full failure-mode notes, dual-dependency history, and manual fallback:
**[doc/panel-update-notes.md](doc/panel-update-notes.md)**.

On each panel (never `git push` from panels; avoid bare `git pull` when nested
submodules break):

```bash
cd /opt/humid
git -c fetch.recurseSubmodules=no fetch origin
git checkout master
git reset --hard origin/master
./scripts/update-panel.sh --no-pull
# or after tree already matches origin: make panel-update
```

From a laptop after **pushing** Humid:

```bash
./scripts/update-panels.sh -p 2222 root@172.29.52.10 root@172.29.53.11
./scripts/update-panels.sh --hosts-file panels.txt -- --restart
```

The script defaults to `master`: it hard-resets Humid to origin, verifies the
vendored `addSetupResponder` API, builds `cw_client` into
`clockwork/stage/lib`, installs HTMLVIEW system packages (Cairo/Pango/Fontconfig)
when possible or warns if it cannot, re-enables `HUMID_WITH_HTMLVIEW` after a
previous missing-package disable, reconfigures Humid against that library (not
`/opt/latproc`), builds, and installs. It still detects the pinned-submodule
layout when an older branch is selected explicitly. It clears CMake caches
from renamed trees (`/opt/humid_next`). Use `--keep-local` to refuse resets.
Use `--restart` / `--start-cmd` only when intentional.

Clockwork panel-client work tracks **`prod-experimental-mqtt-fix`**.

New client library work: land on Clockwork **`prod-client-zmq-fix`**. For the
compatibility deployment branch, advance the tested submodule pin; for
`master`, pull the tested source bodies into the vendored tree with the sync
script (see [docs/CLOCKWORK_CLIENT_BRANCHES.md](docs/CLOCKWORK_CLIENT_BRANCHES.md)).

## Required Build Order (manual)

After a Clockwork client or public-header change:

1. On `master`, build and stage the vendored Clockwork client:
   `cd clockwork && make client-install`
   (or `cmake --build clockwork/build/Release --target install_client`).
2. Reconfigure Humid from `build/` with `cmake ..`.
   Confirm the log shows the **vendored** client, not `/opt/latproc`:
   `Found clockwork: .../clockwork/stage/lib/libcw_client.a`
   If it still shows `/opt/latproc/...`, clear the cache and reconfigure:
   `rm -f build/CMakeCache.txt` then `cmake ..` again from `build/`.
   Also check for a local override in `LocalCMakeLists.txt` (gitignored).
3. Rebuild Humid: `cmake --build build --target humid -- -j4`
   (or top-level `make`, which reconfigures and installs to `stage/bin`).
4. Rebuild the checker separately if not covered by `make`:
   `cmake --build build --target hmifile_check -- -j4`.
5. Run `hmifile_check` against the deployed screen files.

Do not only relink Humid after a public Clockwork header changes. Force the
dependent Humid objects to recompile; stale objects combined with a new static
`libcw_client.a` can produce a valid link followed by deterministic startup
segfaults.

## Live Panel Change Control

- Confirm the target panel, operating state, test window, and rollback before
  changing a production panel.
- Preserve the current executable or a known-good source revision before a
  live replacement.
- Do not restart Clockwork, `iod`, or machine control as part of a Humid-only
  recovery without separate explicit approval.
- After deployment, verify one Humid PID remains stable, both configured
  controller connections complete, the subscriber channels initialize, and
  the expected active screen loads.
- Treat `attempt to call socket connect from a thread that isn't the owner` as
  a misleading Clockwork diagnostic unless a separate connection failure is
  present; the current condition in `MessagingInterface::connect()` is
  inverted and does not itself stop `socket->connect()`.

## Capture Mode

- `--capture` waits up to `--capture_timeout` for Clockwork snapshots on the
  **connections the active screen actually binds** (Core, Grab, or both).
  Layout-only pages do not wait. When the wait expires, Humid writes the
  current frame anyway instead of exiting with no PNG.
- Do not capture against a live plant HMI that already owns the same
  `PANEL_CHANNEL_*` names; use local simulation or capture on the panel.
- Incomplete ZMQ multipart receives must be drained (or the SUB socket
  replaced). Returning after a `more()` frame aborts libzmq
  (`Assertion failed: !_more (src/fq.cpp:80)`, humid exit 134).
