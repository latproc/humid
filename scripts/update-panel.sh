#!/usr/bin/env bash
# Update Humid + pinned Clockwork client on a panel.
#
# Handles the failures seen on multi-panel deploys:
#   - dirty clockwork submodule blocking pin checkout
#   - stale CMake cache selecting /opt/latproc client
#   - building humid against old headers (missing addSetupResponder)
#
# Usage (on the panel):
#   cd /opt/humid && ./scripts/update-panel.sh
#   ./scripts/update-panel.sh --branch cw-no-ec-tools-compatiblity --jobs 6
#   ./scripts/update-panel.sh --keep-local          # do not reset submodule dirt
#   ./scripts/update-panel.sh --restart             # kill humid after install
#   ./scripts/update-panel.sh --restart --start-cmd '/opt/humid/stage/bin/humid --run_only=1 ...'
#
set -euo pipefail

BRANCH="${HUMID_BRANCH:-cw-no-ec-tools-compatiblity}"
JOBS="${HUMID_JOBS:-4}"
FORCE_SUBMODULES=1
DO_PULL=1
DO_BUILD=1
DO_RESTART=0
START_CMD="${HUMID_START_CMD:-}"
ROOT=""

usage() {
  sed -n '2,20p' "$0" | sed 's/^# \{0,1\}//'
  exit "${1:-0}"
}

log()  { printf '==> %s\n' "$*"; }
die()  { printf 'ERROR: %s\n' "$*" >&2; exit 1; }
ok()   { printf 'OK: %s\n' "$*"; }

while [[ $# -gt 0 ]]; do
  case "$1" in
    -h|--help) usage 0 ;;
    --branch) BRANCH="$2"; shift 2 ;;
    --jobs|-j) JOBS="$2"; shift 2 ;;
    --force|--force-submodules) FORCE_SUBMODULES=1; shift ;;
    --keep-local) FORCE_SUBMODULES=0; shift ;;
    --no-pull) DO_PULL=0; shift ;;
    --no-build) DO_BUILD=0; shift ;;
    --restart) DO_RESTART=1; shift ;;
    --start-cmd) START_CMD="$2"; shift 2 ;;
    --root) ROOT="$2"; shift 2 ;;
    *) die "unknown option: $1 (try --help)" ;;
  esac
done

if [[ -z "$ROOT" ]]; then
  ROOT="$(cd "$(dirname "$0")/.." && pwd)"
fi
cd "$ROOT" || die "cannot cd to $ROOT"
[[ -f CMakeLists.txt && -d clockwork ]] || die "not a humid tree: $ROOT"

export JOBS
export MAKEFLAGS="${MAKEFLAGS:-} -j${JOBS}"

log "Humid root: $ROOT"
log "Host: $(hostname)  Branch: $BRANCH  Jobs: $JOBS  Force submodules: $FORCE_SUBMODULES"

# --- git / submodule -------------------------------------------------------
# Never use plain `git pull` on panels: it recurses nested submodules (SOEM,
# eigen, glfw, …) and dies on missing dirs / forced-away refs. Humid client
# build only needs the top-level clockwork pin (+ existing nanogui tree).

if [[ "$DO_PULL" -eq 1 ]]; then
  log "Checkout and update $BRANCH (no recursive submodule fetch)"
  git -c fetch.recurseSubmodules=no fetch origin
  git checkout "$BRANCH" 2>/dev/null || git checkout -B "$BRANCH" "origin/$BRANCH"
  if [[ "$FORCE_SUBMODULES" -eq 1 ]]; then
    if git rev-parse --verify "origin/$BRANCH" >/dev/null 2>&1; then
      log "Reset humid to origin/$BRANCH (panel deploy mode)"
      git reset --hard "origin/$BRANCH"
    else
      die "origin/$BRANCH not found after fetch"
    fi
  else
    git -c fetch.recurseSubmodules=no pull --ff-only origin "$BRANCH" || \
      die "cannot fast-forward $BRANCH. On panels use default force mode, or fix divergence manually."
  fi
fi

PIN="$(git rev-parse ":clockwork" 2>/dev/null || git ls-tree HEAD clockwork | awk '{print $3}')"
[[ -n "$PIN" ]] || die "cannot read clockwork submodule pin from humid"
log "Humid pins clockwork at $PIN"

log "Sync top-level submodule URLs"
git submodule sync clockwork 2>/dev/null || true
git submodule sync lib/nanogui 2>/dev/null || true

if [[ "$FORCE_SUBMODULES" -eq 1 ]]; then
  log "Reset local clockwork dirt (panel deploy mode)"
  if [[ -e clockwork/.git || -f clockwork/.git ]]; then
    git -C clockwork reset --hard HEAD 2>/dev/null || true
    git -C clockwork clean -fd 2>/dev/null || true
  fi
fi

log "Checkout pinned clockwork (top-level only)"
# Do not --recursive: nested iod/ext/* and nanogui/ext/* often break on panels.
if ! git submodule update --init --force clockwork; then
  log "submodule update clockwork failed; fetching pin directly"
  git -C clockwork fetch origin 2>/dev/null || \
    git -C clockwork fetch https://github.com/latproc/clockwork.git 2>/dev/null || true
  git -C clockwork checkout -f "$PIN" || git -C clockwork reset --hard "$PIN"
fi
# Ensure exact pin even if update left an old dirty HEAD
if [[ -e clockwork/.git || -f clockwork/.git ]]; then
  git -C clockwork fetch origin "$PIN" 2>/dev/null || \
    git -C clockwork fetch origin 2>/dev/null || true
  git -C clockwork checkout -f "$PIN" 2>/dev/null || \
    git -C clockwork reset --hard "$PIN"
  git -C clockwork clean -fd 2>/dev/null || true
fi

# nanogui: best-effort top-level only (existing tree is enough if already built)
if git ls-tree HEAD lib/nanogui >/dev/null 2>&1; then
  log "Checkout pinned lib/nanogui (top-level only, nested optional)"
  git submodule update --init --force lib/nanogui 2>/dev/null || \
    log "WARNING: lib/nanogui submodule update failed; using existing tree if present"
fi

CW_HEAD="$(git -C clockwork rev-parse HEAD)"
log "clockwork HEAD: $(git -C clockwork log -1 --oneline)"
[[ "$CW_HEAD" == "$PIN" || "$CW_HEAD" == "$PIN"* ]] || \
  die "clockwork HEAD $CW_HEAD does not match humid pin $PIN"

CM_HDR="clockwork/iod/src/ConnectionManager.h"
[[ -f "$CM_HDR" ]] || die "missing $CM_HDR"
if ! grep -q 'addSetupResponder' "$CM_HDR"; then
  die "$CM_HDR has no addSetupResponder — wrong clockwork pin or incomplete checkout"
fi
ok "ConnectionManager.h exposes addSetupResponder"

# --- clockwork client ------------------------------------------------------

if [[ "$DO_BUILD" -eq 1 ]]; then
  log "Build + install libcw_client from submodule"
  # Ensure objects rebuild after pin moves (make can think Release is current)
  rm -f clockwork/iod/build/Release/CMakeFiles/cw_client.dir/src/ConnectionManager.cpp.o \
        clockwork/iod/build/Release/CMakeFiles/cw_client.dir/src/SocketMonitor.cpp.o \
        clockwork/iod/build/CMakeFiles/cw_client.dir/src/ConnectionManager.cpp.o \
        clockwork/iod/build/CMakeFiles/cw_client.dir/src/SocketMonitor.cpp.o 2>/dev/null || true

  ( cd clockwork/iod && make client-install JOBS="-j${JOBS}" )

  CLIENT_LIB="$ROOT/clockwork/iod/stage/lib/libcw_client.a"
  [[ -f "$CLIENT_LIB" ]] || die "missing $CLIENT_LIB after client-install"

  # Confirm the new API is in the built objects.
  # Use grep -a on the binary (symbol name is in the object). Do not use
  # `nm | grep -q` under pipefail: grep -q closes the pipe on match, nm gets
  # SIGPIPE, and the check false-fails even when the symbol exists.
  has_setup_responder_sym() {
    local f="$1"
    [[ -f "$f" ]] || return 1
    grep -aF 'addSetupResponder' "$f" >/dev/null 2>&1
  }

  SYM_OK=0
  for o in \
    "$ROOT/clockwork/iod/build/Release/CMakeFiles/cw_client.dir/src/ConnectionManager.cpp.o" \
    "$ROOT/clockwork/iod/build/CMakeFiles/cw_client.dir/src/ConnectionManager.cpp.o" \
    "$CLIENT_LIB"
  do
    if has_setup_responder_sym "$o"; then
      SYM_OK=1
      ok "built client contains addSetupResponder ($(basename "$o"))"
      break
    fi
  done
  if [[ "$SYM_OK" -eq 0 ]]; then
    die "built client lacks addSetupResponder (header ok, object/lib check failed). On the panel run: nm clockwork/iod/build/Release/CMakeFiles/cw_client.dir/src/ConnectionManager.cpp.o | grep Setup"
  fi

  # --- humid ---------------------------------------------------------------

  log "Configure and build humid (prefer submodule client)"
  mkdir -p build
  # Drop stale cache entries that pointed at /opt/latproc on older panels
  if [[ -f build/CMakeCache.txt ]] && grep -q '/opt/latproc' build/CMakeCache.txt 2>/dev/null; then
    log "Clearing CMakeCache (contained /opt/latproc)"
    rm -f build/CMakeCache.txt
  fi

  (
    cd build
    cmake \
      -DClockworkClient_LIBRARY="$CLIENT_LIB" \
      -DClockworkClient_INCLUDE_DIR="$ROOT/clockwork/iod/src" \
      .. 2>&1 | tee /tmp/humid-cmake-$$.log
  )

  if ! grep -q 'clockwork/iod/stage/lib/libcw_client.a' /tmp/humid-cmake-$$.log; then
    if grep -q '/opt/latproc' /tmp/humid-cmake-$$.log; then
      die "cmake still selected /opt/latproc client — check LocalCMakeLists.txt"
    fi
    log "WARNING: could not confirm submodule client path in cmake log; continuing"
  else
    ok "cmake uses submodule libcw_client.a"
  fi
  rm -f /tmp/humid-cmake-$$.log

  (
    cd build
    make -j"${JOBS}"
    make install
  )

  [[ -x build/humid || -x stage/bin/humid ]] || die "humid binary not produced"
  ok "humid build/install finished"
fi

# --- optional restart ------------------------------------------------------

if [[ "$DO_RESTART" -eq 1 ]]; then
  log "Stopping humid"
  killall -9 humid 2>/dev/null || true
  sleep 1
  if [[ -n "$START_CMD" ]]; then
    log "Starting: $START_CMD"
    # shellcheck disable=SC2086
    nohup $START_CMD >/tmp/humid-start.log 2>&1 &
    sleep 2
    pgrep -a humid || die "humid did not start (see /tmp/humid-start.log)"
    ok "humid running: $(pgrep -a humid | head -1)"
  else
    log "humid stopped; no --start-cmd given (set HUMID_START_CMD or pass --start-cmd)"
  fi
fi

log "Done on $(hostname)"
echo
echo "Verify:"
echo "  git -C $ROOT log -1 --oneline"
echo "  git -C $ROOT/clockwork log -1 --oneline   # should be $PIN"
echo "  ls -la $ROOT/stage/bin/humid $ROOT/build/humid 2>/dev/null"
