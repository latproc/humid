# Panel Humid / Clockwork client update notes

Record of the July 2026 multi-panel recovery: humid crash / build failures after
picking up ZMQ REQ recreate fixes, dual Boost/CMake/ZeroMQ constraints, and the
`scripts/update-panel.sh` fleet workflow.

Current deployment branches:

| Tree | Branch | Role |
|------|--------|------|
| **humid** | `master` | Panel HMI; vendors the trimmed Clockwork client |
| **clockwork** | `prod-client-zmq-fix` | Canonical client / CHANNEL / ZMQ work |

Canonical agent rules for agents/CI: [AGENTS.md](../AGENTS.md).

---

## Problem summary

A newer-machine change (ZMQ REQ recreate / socket monitor) broke older HMI
panels when humid was rebuilt against mismatched Clockwork headers or libraries.

Symptoms seen across the fleet:

- Startup segfaults after linking a new `libcw_client.a` with stale humid objects
- Compile errors: missing `SubscriptionManager::addSetupResponder`
- CMake selecting **`/opt/latproc/.../libcw_client.a`** instead of the submodule
- Submodule update blocked by local dirt in `clockwork/`
- Nested submodule fetch failures (SOEM, eigen, glfw) on `git pull`
- CMake cache still pointing at **`/opt/humid_next`** after tree renames
- Full iod configure failing without **libmodbus** (HMI only needs `cw_client`)
- CMake 3.5.1 vs `cmake_minimum_required(3.10)` on older panels
- **Boost.Context `fiber.hpp`** missing on Ubuntu Bionic 1.65 (package installs
  do not provide that header even with `libboost-context-dev`)
- `update-panel.sh` falsely rejecting **cmake 3.10.x** (`3.1*` pattern matched 3.10)

---

## Correct architecture

1. **Clockwork fixes live on** `prod-experimental-mqtt-fix`, not ad-hoc panel
   edits of the submodule.
2. **Humid master vendors** the tested client source set and uses
   `addSetupResponder()` (never public `monit_setup` layout).
3. **SocketMonitor** is dual-API: `init`+`check_event` when cppzmq provides them;
   otherwise blocking `monitor()` (older panel `zmq.hpp`).
4. **JSON path evaluation** in `cw_client` must **not** require
   `boost/context/fiber.hpp` (batch tokens; Bionic-safe).
5. **`process.cpp` (fiber scheduler helpers)** stays in full iod builds, **not**
   in `cw_client`.
6. **Build order:** stage vendored client → reconfigure humid against
   `clockwork/stage/lib/libcw_client.a` → rebuild humid (force recompile
   after public header changes).

---

## Fleet update (preferred)

On each panel:

```bash
cd /opt/humid
git -c fetch.recurseSubmodules=no fetch origin
git checkout master
git reset --hard origin/master
./scripts/update-panel.sh --no-pull
# or: make panel-update   # after tree already matches origin
```

Do **not** use bare `git pull` on panels when nested submodules are broken, and
do **not** `git push` from panels (remotes often have work the panel lacks).

What `scripts/update-panel.sh` does (default force mode):

1. Fetch humid **without** recursive submodule fetch
2. `git reset --hard origin/<branch>` (discards local humid divergence)
3. Detect the vendored Clockwork layout (or a legacy branch's submodule)
4. Update the remaining top-level `lib/nanogui` submodule
5. Verify `addSetupResponder` in `ConnectionManager.h`
6. Clear CMake caches that reference another tree path (`/opt/humid_next`, etc.)
7. Configure/build/install `cw_client` with the newest cmake on PATH
8. Reconfigure humid forcing the vendored client library + includes
9. Build and install humid

Useful flags:

| Flag | Meaning |
|------|---------|
| `--no-pull` | Skip fetch/reset (tree already correct) |
| `--keep-local` | Do not hard-reset humid/submodule dirt |
| `--restart` | `killall -9 humid` after install |
| `--start-cmd '...'` | Start humid after kill |
| `--jobs N` | Parallelism |

Multi-host from a laptop (after origin is updated):

```bash
./scripts/update-panels.sh -p 2222 root@172.29.52.10 root@172.29.53.11
```

---

## Verify after update

```bash
git -C /opt/humid log -1 --oneline
grep -n addSetupResponder /opt/humid/clockwork/src/ConnectionManager.h
# cmake log must show:
#   Found clockwork: .../clockwork/stage/lib/libcw_client.a
# not /opt/latproc/...
ls -la /opt/humid/stage/bin/humid /opt/humid/build/humid
```

Runtime: one stable humid PID, both controller connections (where configured),
subscriber channels up, expected active screen.

---

## Failure modes and fixes

| Symptom | Cause | Fix |
|---------|--------|-----|
| `addSetupResponder` unknown | Wrong client headers (`/opt/latproc` or stale archive) | Clear `build/CMakeCache.txt`; rebuild vendored `stage/lib` client |
| Legacy submodule update refuses checkout | Local dirt in `clockwork/` | `git -C clockwork reset --hard` then pin checkout |
| `git pull` dies on SOEM/eigen/glfw | Nested submodule recursion | `fetch.recurseSubmodules=no` + top-level pin only |
| CMake path `/opt/humid_next` | Cache from renamed tree | Remove `clockwork/build/Release` cache (script clears this) |
| `MODBUS_LIBRARIES NOTFOUND` | Full iod always linked modbusd/dbd | Clockwork skips modbusd/dbd when libmodbus missing |
| `fiber.hpp` not found (Bionic) | 1.65 packages lack fiber headers | Fiber-free JSON path + no `process.cpp` in `cw_client` |
| Script says cmake 3.10 “too old” | Bug: pattern `3.1*` matched 3.10 | Fixed with `sort -V` compare in update-panel |
| Divergent branch / pull.rebase | Local panel commits | `git reset --hard origin/<branch>` |
| Push rejected from panel | Origin has newer work | Do not push from panels; pull/reset only |

---

## Dual dependency notes (panels)

### ZeroMQ

- Prefer `/usr/local/lib/libzmq` (e.g. 5.2.3) when that is production.
- Old `zmq.hpp`: `monitor()` only; new cppzmq: `init` + `check_event`.
- Compile-time selection in `SocketMonitor` — do not hardcode one path.

### Boost

- Headers may be 1.74 while some symbols need **1.65** `libboost_date_time`.
- Humid CMake appends `libboost_date_time.so.1.65.1` when present.
- Do **not** assume installing another Boost metapackage fixes fiber; Bionic’s
  1.65 context-dev still has no `fiber.hpp`. Prefer fiber-free client code.

### CMake

- Range observed: **3.5.1** … **3.10.2**+.
- Vendored Clockwork client minimum is **3.10**; legacy submodule builds permit
  **3.5** with gated newer APIs.
- `target_link_directories` needs 3.13+ → use `target_link_directories_compat`
  / `-L` fallbacks for Homebrew on macOS.

---

## Manual build order (if not using the script)

```bash
cd /opt/humid
git reset --hard origin/master
git submodule update --init --force lib/nanogui

cd clockwork
rm -rf build/Release   # if path or pin changed
mkdir -p build/Release && cd build/Release
cmake -DCMAKE_BUILD_TYPE=Release -DRUN_TESTS=OFF ../..
cmake --build . --target cw_client -- -j4
cmake --build . --target install_client -- -j4

cd /opt/humid/build
rm -f CMakeCache.txt   # if latproc or wrong path was cached
cmake \
  -DClockworkClient_LIBRARY=/opt/humid/clockwork/stage/lib/libcw_client.a \
  -DClockworkClient_INCLUDE_DIR=/opt/humid/clockwork/src \
  ..
cmake --build . --target humid -- -j4
cmake --build . --target hmifile_check -- -j4
# or: cd /opt/humid && make
```

---

## Git hygiene

- **Dev machine:** sync tested client changes from Clockwork
  `prod-client-zmq-fix`, then commit and push the vendored Humid sources.
- **Panels:** fetch + hard reset + update script only.
- Local tracked changes on panels are discarded by the script on purpose.
- Nested `lib/nanogui` dirt is usually noise; do not commit unless intentional.

---

## Related scripts

| Path | Purpose |
|------|---------|
| `scripts/update-panel.sh` | Single-panel update/build |
| `scripts/update-panels.sh` | SSH wrapper over many hosts |
| `make panel-update` | Invokes `update-panel.sh` |

---

## Live change control (reminder)

- Confirm panel, production state, test window, and rollback first.
- Humid-only recovery must not restart iod/machine control without approval.
- After install, confirm connections and active screen, not only “binary exists”.
