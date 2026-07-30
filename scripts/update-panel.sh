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

if [[ "$DO_PULL" -eq 1 ]]; then
  log "Checkout and pull $BRANCH"
  git fetch origin
  git checkout "$BRANCH"
  # allow local dirty .gitmodules / nanogui noise; pull only advances branch tip
  git pull --ff-only origin "$BRANCH" || git pull --ff-only
fi

PIN="$(git rev-parse ":clockwork" 2>/dev/null || git ls-tree HEAD clockwork | awk '{print $3}')"
[[ -n "$PIN" ]] || die "cannot read clockwork submodule pin from humid"
log "Humid pins clockwork at $PIN"

log "Sync submodule URLs"
git submodule sync --recursive

if [[ "$FORCE_SUBMODULES" -eq 1 ]]; then
  log "Reset local submodule dirt (panel deploy mode)"
  # Only reset the clockwork working tree; nanogui nested noise is common and
  # usually irrelevant to the humid client link.
  if [[ -e clockwork/.git || -f clockwork/.git ]]; then
    git -C clockwork reset --hard HEAD || true
    git -C clockwork clean -fd || true
  fi
fi

log "Checkout pinned submodules"
if ! git submodule update --init --recursive --force; then
  if [[ "$FORCE_SUBMODULES" -eq 1 ]]; then
    log "submodule update failed; forcing clockwork pin $PIN"
    git -C clockwork fetch origin || true
    git -C clockwork checkout -f "$PIN"
    git -C clockwork reset --hard "$PIN"
    git -C clockwork clean -fd
  else
    die "submodule update failed (local changes?). Re-run with --force-submodules"
  fi
fi

CW_HEAD="$(git -C clockwork rev-parse HEAD)"
log "clockwork HEAD: $(git -C clockwork log -1 --oneline)"
[[ "$CW_HEAD" == "$PIN"* || "$CW_HEAD" == "$PIN" ]] || \
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

  # Confirm the new API is in the built objects. Prefer the .o (reliable on all
  # nm variants); fall back to the .a and strings(1) — some Debian nm builds
  # are awkward with archives and previously false-failed this check.
  has_setup_responder_sym() {
    local f="$1"
    [[ -f "$f" ]] || return 1
    if command -v nm >/dev/null 2>&1; then
      nm "$f" 2>/dev/null | grep -q 'addSetupResponder' && return 0
      nm -A "$f" 2>/dev/null | grep -q 'addSetupResponder' && return 0
      nm -C "$f" 2>/dev/null | grep -q 'addSetupResponder' && return 0
    fi
    if command -v objdump >/dev/null 2>&1; then
      objdump -t "$f" 2>/dev/null | grep -q 'addSetupResponder' && return 0
    fi
    if command -v strings >/dev/null 2>&1; then
      strings "$f" 2>/dev/null | grep -q 'addSetupResponder' && return 0
    fi
    return 1
  }

  SYM_OK=0
  for o in \
    clockwork/iod/build/Release/CMakeFiles/cw_client.dir/src/ConnectionManager.cpp.o \
    clockwork/iod/build/CMakeFiles/cw_client.dir/src/ConnectionManager.cpp.o
  do
    if has_setup_responder_sym "$o"; then
      SYM_OK=1
      ok "ConnectionManager.cpp.o contains addSetupResponder ($o)"
      break
    fi
  done
  if [[ "$SYM_OK" -eq 0 ]] && has_setup_responder_sym "$CLIENT_LIB"; then
    SYM_OK=1
    ok "libcw_client.a contains addSetupResponder"
  fi
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
