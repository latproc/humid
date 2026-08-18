#!/usr/bin/env bash
# Update Humid + its Clockwork client on a panel.
#
# Handles the failures seen on multi-panel deploys:
#   - dirty clockwork submodule blocking pin checkout
#   - stale CMake cache selecting /opt/latproc client
#   - building humid against old headers (missing addSetupResponder)
#
# Usage (on the panel):
#   cd /opt/humid && ./scripts/update-panel.sh
#   ./scripts/update-panel.sh --branch master --jobs 6
#   ./scripts/update-panel.sh --keep-local          # do not reset submodule dirt
#   ./scripts/update-panel.sh --restart             # kill humid after install
#   ./scripts/update-panel.sh --restart --start-cmd '/opt/humid/stage/bin/humid --run_only=1 ...'
#
# HTMLVIEW (operators-manual viewer) needs Cairo/Pango/Fontconfig. The script
# installs those development packages when possible; otherwise it warns and
# continues without HTMLVIEW. A previous CMake cache that forced
# HUMID_WITH_HTMLVIEW=OFF is re-enabled after the packages are present.
#
set -euo pipefail

BRANCH="${HUMID_BRANCH:-master}"
JOBS="${HUMID_JOBS:-4}"
FORCE_SUBMODULES=1
DO_PULL=1
DO_BUILD=1
DO_RESTART=0
INSTALL_HTMLVIEW_DEPS=1
WANT_HTMLVIEW=1
START_CMD="${HUMID_START_CMD:-}"
ROOT=""

usage() {
  cat <<'EOF'
Update Humid + its Clockwork client on a panel.

Usage:
  ./scripts/update-panel.sh [options]

Options:
  --branch BRANCH       Humid branch to deploy (default: master)
  --jobs, -j N          parallel build jobs (default: 4)
  --force               reset to origin and force submodule checkout (default)
  --keep-local          require a fast-forward and preserve local submodule dirt
  --no-pull             build the currently checked-out tree
  --no-build            update sources without building
  --restart             stop Humid after installation
  --start-cmd COMMAND   command used to restart Humid
  --root PATH           Humid checkout (default: script parent)
  --no-htmlview-deps    do not apt/brew HTMLVIEW packages (cairo/pango/fontconfig)
  --without-htmlview    build without HTMLVIEW (skip package install, cmake OFF)
  --help                show this help

HTMLVIEW (litehtml) needs a C++17 compiler with <variant> (GCC 7+, Clang 5+)
and pkg-config modules cairo, pangocairo, and fontconfig. Ubuntu 16.04
g++-5 cannot build it; Cairo/Pango packages do not change that. The script
checks the compiler first, only then installs the development packages (and
g++-7 from apt if that package exists). If either requirement cannot be
met, humid is built without the operators-manual viewer and a warning is
printed (the panel update still succeeds).
EOF
  exit "${1:-0}"
}

log()  { printf '==> %s\n' "$*"; }
die()  { printf 'ERROR: %s\n' "$*" >&2; exit 1; }
ok()   { printf 'OK: %s\n' "$*"; }
warn() { printf 'WARNING: %s\n' "$*"; }

# pkg-config modules required by cmake/Modules/HumidLitehtml.cmake
HTMLVIEW_PC_MODULES=(cairo pangocairo fontconfig)
HTMLVIEW_APT_PKGS=(libcairo2-dev libpango1.0-dev libfontconfig1-dev pkg-config)
HTMLVIEW_BREW_PKGS=(cairo pango fontconfig pkg-config)
HTMLVIEW_CXX=""

htmlview_pc_ok() {
  command -v pkg-config >/dev/null 2>&1 || return 1
  pkg-config --exists "${HTMLVIEW_PC_MODULES[@]}"
}

htmlview_missing_pc() {
  local missing=() m
  if ! command -v pkg-config >/dev/null 2>&1; then
    printf '%s\n' "pkg-config"
    return
  fi
  for m in "${HTMLVIEW_PC_MODULES[@]}"; do
    pkg-config --exists "$m" || missing+=("$m")
  done
  if [[ ${#missing[@]} -gt 0 ]]; then
    printf '%s\n' "${missing[*]}"
  fi
}

run_apt_get() {
  if [[ "$(id -u)" -eq 0 ]]; then
    DEBIAN_FRONTEND=noninteractive apt-get "$@"
  else
    DEBIAN_FRONTEND=noninteractive sudo -n apt-get "$@"
  fi
}

# apt-get install can fail on a stale index (404s for moved packages).
# Refresh once per run before any extra-package install.
APT_UPDATED=0
ensure_apt_updated() {
  if [[ "$APT_UPDATED" -eq 1 ]]; then
    return 0
  fi
  log "Refreshing apt package lists"
  if run_apt_get update; then
    APT_UPDATED=1
    return 0
  fi
  warn "apt-get update failed; package install may not be able to download packages"
  return 1
}

# Install Cairo/Pango/Fontconfig so cmake can enable HTMLVIEW.
# Returns 0 if pkg-config modules are present afterwards, 1 if not.
# Never aborts the panel update: missing HTMLVIEW is a warning.
ensure_htmlview_deps() {
  local missing
  missing="$(htmlview_missing_pc)"
  if [[ -z "$missing" ]]; then
    ok "HTMLVIEW pkg-config modules present (${HTMLVIEW_PC_MODULES[*]})"
    return 0
  fi

  log "HTMLVIEW development packages missing ($missing)"

  if [[ "$INSTALL_HTMLVIEW_DEPS" -eq 0 ]]; then
    warn "not installing HTMLVIEW packages (--no-htmlview-deps / --without-htmlview)"
    return 1
  fi

  if command -v apt-get >/dev/null 2>&1; then
    if [[ "$(id -u)" -ne 0 ]] && ! sudo -n true >/dev/null 2>&1; then
      warn "cannot install HTMLVIEW packages (need root or passwordless sudo)"
      warn "  apt-get install -y ${HTMLVIEW_APT_PKGS[*]}"
      return 1
    fi
    log "Installing HTMLVIEW packages: ${HTMLVIEW_APT_PKGS[*]}"
    ensure_apt_updated || true
    if ! run_apt_get install -y "${HTMLVIEW_APT_PKGS[@]}"; then
      warn "could not install HTMLVIEW packages (offline panel or broken apt sources?)"
      warn "  humid will configure without the operators-manual HTML viewer"
      return 1
    fi
  elif command -v brew >/dev/null 2>&1; then
    log "Installing HTMLVIEW packages via Homebrew: ${HTMLVIEW_BREW_PKGS[*]}"
    if ! brew install "${HTMLVIEW_BREW_PKGS[@]}"; then
      warn "Homebrew HTMLVIEW package install failed"
      return 1
    fi
  else
    warn "no apt-get or brew — cannot install HTMLVIEW packages automatically"
    warn "  Debian / Ubuntu / Raspberry Pi OS:"
    warn "    apt-get install -y ${HTMLVIEW_APT_PKGS[*]}"
    warn "  macOS: brew install ${HTMLVIEW_BREW_PKGS[*]}"
    return 1
  fi

  if htmlview_pc_ok; then
    ok "HTMLVIEW packages installed (${HTMLVIEW_PC_MODULES[*]})"
    return 0
  fi
  warn "packages installed but pkg-config still cannot find: $(htmlview_missing_pc)"
  return 1
}

htmlview_compiler_id() {
  local cc="${1:-${CXX:-g++}}"
  if ! command -v "$cc" >/dev/null 2>&1; then
    printf '%s\n' "no $cc on PATH"
    return
  fi
  "$cc" --version 2>/dev/null | head -1
}

# litehtml needs C++17 <variant> (GCC 7+ / Clang 5+). Ubuntu 16.04 g++-5
# accepts -std=c++17 but the compile still dies on #include <variant>.
htmlview_try_compile_variant() {
  local cc="$1" src bin
  command -v "$cc" >/dev/null 2>&1 || return 1
  src="$(mktemp /tmp/humid-cxx17-XXXXXX.cpp)"
  bin="$(mktemp /tmp/humid-cxx17-XXXXXX)"
  cat >"$src" <<'EOF'
#include <variant>
int main() {
  std::variant<int, double> v = 1;
  return std::get<int>(v) - 1;
}
EOF
  if "$cc" -std=c++17 -o "$bin" "$src" >/dev/null 2>&1; then
    rm -f "$src" "$bin"
    return 0
  fi
  rm -f "$src" "$bin"
  return 1
}

# Pick a compiler that can build litehtml. Optionally apt-get g++-7.
# Sets HTMLVIEW_CXX on success. Never aborts the panel update.
ensure_htmlview_cxx17() {
  local cc
  local candidates=()
  if [[ -n "${CXX:-}" ]]; then
    candidates+=("$CXX")
  fi
  candidates+=(g++ clang++ g++-13 g++-12 g++-11 g++-10 g++-9 g++-8 g++-7)

  for cc in "${candidates[@]}"; do
    if htmlview_try_compile_variant "$cc"; then
      HTMLVIEW_CXX="$cc"
      ok "HTMLVIEW C++17 <variant> ok ($cc: $(htmlview_compiler_id "$cc"))"
      return 0
    fi
  done

  if [[ "$INSTALL_HTMLVIEW_DEPS" -eq 1 ]] && command -v apt-get >/dev/null 2>&1; then
    if [[ "$(id -u)" -eq 0 ]] || sudo -n true >/dev/null 2>&1; then
      ensure_apt_updated || true
      if apt-cache show g++-7 >/dev/null 2>&1; then
        log "Trying apt-get install g++-7 (litehtml needs C++17 <variant>)"
        if run_apt_get install -y g++-7 && htmlview_try_compile_variant g++-7; then
          HTMLVIEW_CXX="g++-7"
          ok "installed g++-7 for HTMLVIEW ($(htmlview_compiler_id g++-7))"
          return 0
        fi
        warn "g++-7 is in apt but install or compile check failed"
      else
        warn "g++-7 is not in this OS apt archive (typical on Ubuntu 16.04)"
      fi
    fi
  fi

  warn "HTMLVIEW cannot be built: litehtml needs C++17 <variant> (GCC 7+, Clang 5+)"
  warn "  compiler: $(htmlview_compiler_id "${CXX:-g++}")"
  warn "  Cairo/Pango packages do not fix this — they are only used after a C++17 compiler exists"
  return 1
}

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
    --no-htmlview-deps) INSTALL_HTMLVIEW_DEPS=0; shift ;;
    --without-htmlview) WANT_HTMLVIEW=0; INSTALL_HTMLVIEW_DEPS=0; shift ;;
    *) die "unknown option: $1 (try --help)" ;;
  esac
done

is_humid_tree() {
  [[ -f "${1:-.}/CMakeLists.txt" && -d "${1:-.}/clockwork" ]]
}

if [[ -z "$ROOT" ]]; then
  # bash -s (streamed by update-panels.sh) has $0=bash, so dirname $0/.. is
  # the parent of cwd (/opt/humid -> /opt). Prefer cwd when it is the tree.
  if is_humid_tree "."; then
    ROOT="$(pwd)"
  else
    _src="${BASH_SOURCE[0]:-$0}"
    case "$_src" in
      bash|-|/dev/stdin|/bin/bash|/usr/bin/bash) _src="" ;;
    esac
    if [[ -n "$_src" && -f "$_src" ]]; then
      ROOT="$(cd "$(dirname "$_src")/.." && pwd)"
    fi
  fi
fi
[[ -n "$ROOT" ]] || die "not a humid tree (pass --root, or cd to the checkout). cwd=$(pwd)"
cd "$ROOT" || die "cannot cd to $ROOT"
is_humid_tree "$ROOT" || die "not a humid tree: $ROOT"

export JOBS
export MAKEFLAGS="${MAKEFLAGS:-} -j${JOBS}"

log "Humid root: $ROOT"
log "Host: $(hostname)  Branch: $BRANCH  Jobs: $JOBS  Force checkout: $FORCE_SUBMODULES"

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

CW_ENTRY_MODE="$(git ls-files -s clockwork | awk 'NR == 1 {print $1}')"
PIN=""
if [[ "$CW_ENTRY_MODE" == "160000" ]]; then
  CLOCKWORK_LAYOUT="submodule"
  PIN="$(git rev-parse ":clockwork" 2>/dev/null || git ls-tree HEAD clockwork | awk '{print $3}')"
  [[ -n "$PIN" ]] || die "cannot read clockwork submodule pin from humid"
  log "Humid pins clockwork at $PIN"

  log "Sync top-level submodule URLs"
  git submodule sync clockwork 2>/dev/null || true

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
  # Ensure exact pin even if update left an old dirty HEAD.
  if [[ -e clockwork/.git || -f clockwork/.git ]]; then
    git -C clockwork fetch origin "$PIN" 2>/dev/null || \
      git -C clockwork fetch origin 2>/dev/null || true
    git -C clockwork checkout -f "$PIN" 2>/dev/null || \
      git -C clockwork reset --hard "$PIN"
    git -C clockwork clean -fd 2>/dev/null || true
  fi

  CW_PROJECT_DIR="$ROOT/clockwork/iod"
else
  CLOCKWORK_LAYOUT="vendored"
  CW_PROJECT_DIR="$ROOT/clockwork"
  [[ -f "$CW_PROJECT_DIR/CMakeLists.txt" && -d "$CW_PROJECT_DIR/src" ]] || \
    die "master requires vendored Clockwork sources under clockwork/src"
  log "Using vendored Clockwork client sources from Humid"
fi

CW_SOURCE_DIR="$CW_PROJECT_DIR/src"
CW_BUILD_DIR="$CW_PROJECT_DIR/build/Release"
CW_STAGE_DIR="$CW_PROJECT_DIR/stage/lib"

git submodule sync lib/nanogui 2>/dev/null || true

# nanogui: best-effort top-level only (existing tree is enough if already built)
if git ls-tree HEAD lib/nanogui >/dev/null 2>&1; then
  log "Checkout pinned lib/nanogui (top-level only, nested optional)"
  git submodule update --init --force lib/nanogui 2>/dev/null || \
    log "WARNING: lib/nanogui submodule update failed; using existing tree if present"
fi

if [[ "$CLOCKWORK_LAYOUT" == "submodule" ]]; then
  CW_HEAD="$(git -C clockwork rev-parse HEAD)"
  log "clockwork HEAD: $(git -C clockwork log -1 --oneline)"
  [[ "$CW_HEAD" == "$PIN" || "$CW_HEAD" == "$PIN"* ]] || \
    die "clockwork HEAD $CW_HEAD does not match humid pin $PIN"
fi

CM_HDR="$CW_SOURCE_DIR/ConnectionManager.h"
[[ -f "$CM_HDR" ]] || die "missing $CM_HDR"
if ! grep -q 'addSetupResponder' "$CM_HDR"; then
  die "$CM_HDR has no addSetupResponder — wrong clockwork pin or incomplete checkout"
fi
ok "ConnectionManager.h exposes addSetupResponder"

# --- clockwork client ------------------------------------------------------

# Prefer the newest cmake on PATH for clockwork (panels range from 3.5 to 3.x).
find_cmake() {
  local c ver best="" best_ver="0.0.0"
  version_ge() { # $1 >= $2 ?
    printf '%s\n%s\n' "$2" "$1" | sort -V | head -1 | grep -qx "$2"
  }
  for c in "${CMAKE_BIN:-}" cmake cmake3 \
      /usr/local/bin/cmake /usr/bin/cmake3 /opt/cmake/bin/cmake; do
    [[ -z "$c" ]] && continue
    command -v "$c" >/dev/null 2>&1 || [[ -x "$c" ]] || continue
    c="$(command -v "$c" 2>/dev/null || echo "$c")"
    ver="$("$c" --version 2>/dev/null | head -1 | sed -n 's/.* \([0-9][0-9.]*\).*/\1/p')"
    [[ -n "$ver" ]] || continue
    if version_ge "$ver" "$best_ver"; then
      best="$c"
      best_ver="$ver"
    fi
  done
  if [[ -z "$best" ]]; then
    return 1
  fi
  printf '%s %s\n' "$best" "$best_ver"
}

if [[ "$DO_BUILD" -eq 1 ]]; then
  log "Build + install libcw_client from $CLOCKWORK_LAYOUT sources"
  CMAKE_INFO="$(find_cmake)" || die "no cmake found on PATH"
  read -r CMAKE_CMD CMAKE_VER <<<"$CMAKE_INFO"
  log "Using cmake $CMAKE_VER ($CMAKE_CMD)"
  # Vendored client CMakeLists is 3.5-compatible (newer APIs are version-gated).
  REQUIRED_CMAKE="3.5"
  version_ge() { printf '%s\n%s\n' "$2" "$1" | sort -V | head -1 | grep -qx "$2"; }
  if ! version_ge "$CMAKE_VER" "$REQUIRED_CMAKE"; then
    die "cmake $CMAKE_VER is too old for $CLOCKWORK_LAYOUT Clockwork (need >= $REQUIRED_CMAKE)"
  fi
  # Drop CMake caches that were generated under a different tree path
  # (e.g. /opt/humid_next → /opt/humid copy/rename).
  for cache in "$CW_PROJECT_DIR/build/Release/CMakeCache.txt" \
               "$CW_PROJECT_DIR/build/CMakeCache.txt" \
               "$CW_PROJECT_DIR/build/Debug/CMakeCache.txt"; do
    if [[ -f "$cache" ]] && ! grep -q "$CW_PROJECT_DIR" "$cache" 2>/dev/null; then
      log "Removing stale CMake cache $cache (path mismatch)"
      rm -f "$cache"
      # Also drop the sibling CMakeFiles so cmake fully reconfigures
      rm -rf "$(dirname "$cache")/CMakeFiles"
    fi
  done
  # Ensure objects rebuild after pin moves (make can think Release is current)
  rm -f "$CW_PROJECT_DIR/build/Release/CMakeFiles/cw_client.dir/src/ConnectionManager.cpp.o" \
        "$CW_PROJECT_DIR/build/Release/CMakeFiles/cw_client.dir/src/SocketMonitor.cpp.o" \
        "$CW_PROJECT_DIR/build/CMakeFiles/cw_client.dir/src/ConnectionManager.cpp.o" \
        "$CW_PROJECT_DIR/build/CMakeFiles/cw_client.dir/src/SocketMonitor.cpp.o" 2>/dev/null || true

  # Invoke cmake directly (the Makefile hardcodes "cmake" which may be 3.5.1
  # while a newer binary exists elsewhere; also avoids make client quirks).
  (
    set -e
    cd "$CW_PROJECT_DIR"
    mkdir -p build/Release
    cd build/Release
    "$CMAKE_CMD" -DCMAKE_BUILD_TYPE=Release -DRUN_TESTS=OFF ../..
    "$CMAKE_CMD" --build . --target cw_client -- -j"${JOBS}"
    "$CMAKE_CMD" --build . --target install_client -- -j"${JOBS}"
  )

  CLIENT_LIB="$CW_STAGE_DIR/libcw_client.a"
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
    "$CW_PROJECT_DIR/build/Release/CMakeFiles/cw_client.dir/src/ConnectionManager.cpp.o" \
    "$CW_PROJECT_DIR/build/CMakeFiles/cw_client.dir/src/ConnectionManager.cpp.o" \
    "$CLIENT_LIB"
  do
    if has_setup_responder_sym "$o"; then
      SYM_OK=1
      ok "built client contains addSetupResponder ($(basename "$o"))"
      break
    fi
  done
  if [[ "$SYM_OK" -eq 0 ]]; then
    die "built client lacks addSetupResponder (header ok, object/lib check failed). Inspect $CW_BUILD_DIR/CMakeFiles/cw_client.dir/src/ConnectionManager.cpp.o"
  fi

  # --- humid ---------------------------------------------------------------

  log "Configure and build humid (use $CLOCKWORK_LAYOUT client)"
  mkdir -p build

  HTMLVIEW_CMAKE_ARGS=("-DHUMID_WITH_HTMLVIEW=OFF")
  if [[ "$WANT_HTMLVIEW" -eq 1 ]]; then
    if [[ ! -f "$ROOT/lib/litehtml/include/litehtml.h" ]]; then
      warn "lib/litehtml missing at $ROOT/lib/litehtml — HTMLVIEW cannot be built"
    fi
    # Compiler first: Cairo/Pango -dev packages cannot make GCC 5 compile
    # litehtml. Do not install them on a panel that cannot build HTMLVIEW.
    _htmlview_deps=0
    _htmlview_cxx=0
    if ensure_htmlview_cxx17; then
      _htmlview_cxx=1
      ensure_htmlview_deps && _htmlview_deps=1
    else
      warn "skipping Cairo/Pango package install — compiler cannot build litehtml"
    fi
    if [[ "$_htmlview_deps" -eq 1 && "$_htmlview_cxx" -eq 1 ]]; then
      # Previous configures FORCE this OFF when packages were missing.
      # Command-line -D re-enables it now that cairo/pango/fontconfig exist.
      HTMLVIEW_CMAKE_ARGS=("-DHUMID_WITH_HTMLVIEW=ON")
      if [[ -n "$HTMLVIEW_CXX" ]]; then
        _default_cc="$(command -v "${CXX:-g++}" 2>/dev/null || true)"
        _picked_cc="$(command -v "$HTMLVIEW_CXX" 2>/dev/null || true)"
        if [[ -n "$_picked_cc" && "$_picked_cc" != "$_default_cc" ]]; then
          log "Using $HTMLVIEW_CXX for humid so litehtml can compile"
          HTMLVIEW_CMAKE_ARGS+=("-DCMAKE_CXX_COMPILER=$HTMLVIEW_CXX")
        fi
      fi
    else
      warn "building humid without HTMLVIEW (operators-manual viewer unavailable)"
    fi
  else
    log "HTMLVIEW disabled (--without-htmlview)"
  fi

  # Drop stale cache from old tree paths or /opt/latproc client selection
  if [[ -f build/CMakeCache.txt ]]; then
    if grep -qE '/opt/latproc|/opt/humid_next' build/CMakeCache.txt 2>/dev/null || \
       ! grep -q "$ROOT" build/CMakeCache.txt 2>/dev/null; then
      log "Clearing humid CMakeCache (stale path or latproc)"
      rm -f build/CMakeCache.txt
      rm -rf build/CMakeFiles
    fi
  fi

  (
    cd build
    "$CMAKE_CMD" \
      -DClockworkClient_LIBRARY="$CLIENT_LIB" \
      -DClockworkClient_INCLUDE_DIR="$CW_SOURCE_DIR" \
      "${HTMLVIEW_CMAKE_ARGS[@]}" \
      .. 2>&1 | tee /tmp/humid-cmake-$$.log
  )

  if ! grep -Fq "$CLIENT_LIB" /tmp/humid-cmake-$$.log; then
    if grep -q '/opt/latproc' /tmp/humid-cmake-$$.log; then
      die "cmake still selected /opt/latproc client — check LocalCMakeLists.txt"
    fi
    log "WARNING: could not confirm $CLOCKWORK_LAYOUT client path in cmake log; continuing"
  else
    ok "cmake uses $CLOCKWORK_LAYOUT libcw_client.a"
  fi

  HTMLVIEW_ENABLED=0
  if grep -Fq 'HTMLVIEW: enabled' /tmp/humid-cmake-$$.log; then
    HTMLVIEW_ENABLED=1
    ok "HTMLVIEW enabled (litehtml + Cairo/Pango)"
  elif [[ "$WANT_HTMLVIEW" -eq 1 ]]; then
    warn "cmake did not enable HTMLVIEW"
    grep -E 'HTMLVIEW' /tmp/humid-cmake-$$.log || true
    warn "operators-manual screens will not render until HTMLVIEW packages and a C++17 compiler (GCC 7+) are available"
  else
    log "HTMLVIEW left disabled"
  fi
  rm -f /tmp/humid-cmake-$$.log

  if ( cd build && make -j"${JOBS}" && make install ); then
    :
  elif [[ "$HTMLVIEW_ENABLED" -eq 1 ]]; then
    warn "humid build failed with HTMLVIEW enabled; retrying without litehtml"
    (
      cd build
      "$CMAKE_CMD" \
        -DClockworkClient_LIBRARY="$CLIENT_LIB" \
        -DClockworkClient_INCLUDE_DIR="$CW_SOURCE_DIR" \
        -DHUMID_WITH_HTMLVIEW=OFF \
        ..
      make -j"${JOBS}"
      make install
    ) || die "humid build failed even without HTMLVIEW"
    warn "humid installed without HTMLVIEW (litehtml did not compile on this compiler)"
  else
    die "humid build/install failed"
  fi

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
if [[ "$CLOCKWORK_LAYOUT" == "submodule" ]]; then
  echo "  git -C $ROOT/clockwork log -1 --oneline   # should be $PIN"
else
  echo "  ls -la $ROOT/clockwork/src/ConnectionManager.h"
fi
echo "  ls -la $ROOT/stage/bin/humid $ROOT/build/humid 2>/dev/null"
echo "  # HTMLVIEW: cmake log should say 'HTMLVIEW: enabled' when cairo/pango/fontconfig are present"
