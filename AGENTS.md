# Humid Agent Context

## Repository And Submodules

- This repository uses Git. `clockwork/` and `lib/nanogui/` are Git
  submodules pinned by the parent Humid commit.
- Do not update a submodule only by moving the parent pointer to an untested
  remote commit. Check out the intended submodule branch, make and test the
  fix there, commit it, then commit the new submodule pointer in Humid.
- A normal submodule checkout is detached. Before committing a Clockwork fix,
  create or check out the local branch that tracks the intended remote branch.
- Keep unrelated dirty submodule changes out of commits. Inspect status and
  diffs in both the parent and each affected submodule.
- A local submodule commit is not available to other machines until it is
  pushed. Do not push unless the operator explicitly approves publication.

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
- Track Clockwork humid-client fixes on `prod-experimental-mqtt-fix`.
- Humid must not dereference public `SubscriptionManager` implementation
  fields such as `monit_setup`. Register callbacks through Clockwork methods
  such as `addSetupResponder()` so construction and access use one compiled
  class layout.

### Boost Date-Time

- Some panels have Boost 1.74 headers and a small/header-only
  `libboost_date_time.so.1.74.0` that does not export the legacy
  `boost::gregorian::greg_month` string symbols used by Clockwork.
- Those panels also provide `libboost_date_time.so.1.65.1`, which supplies the
  required symbols.
- Humid's CMake appends the 1.65.1 compatibility library when it is installed.
  Do not remove that fallback without checking the exported symbols on every
  supported production OS.

### CMake

- Production CMake is 3.10. Do not assume `cmake -S ... -B ...` is supported.
  Use an out-of-tree directory and run `cmake ..` from it.
- This CMake accepts one `--target` per `cmake --build` invocation. Build
  `humid` and `hmifile_check` in separate commands.

## Required Build Order

After a Clockwork client or public-header change:

1. Build and stage the Clockwork client from the checked-out submodule:
   `cd clockwork/iod && make client-install`
   (or `cmake --build clockwork/iod/build/Release --target install_client`)
2. Reconfigure Humid from `build/` with `cmake ..`.
   Confirm the log shows the **submodule** client, not `/opt/latproc`:
   `Found clockwork: .../clockwork/iod/stage/lib/libcw_client.a`
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
