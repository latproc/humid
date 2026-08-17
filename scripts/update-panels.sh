#!/usr/bin/env bash
# Run scripts/update-panel.sh on one or more panels over SSH.
#
# Usage (from a machine with SSH access):
#   ./scripts/update-panels.sh root@172.29.52.10 root@172.29.53.11
#   ./scripts/update-panels.sh -p 2222 root@172.29.52.10
#   ./scripts/update-panels.sh --hosts-file panels.txt -- --restart
#
# Extra args after -- are passed to update-panel.sh on each host.
#
# The local scripts/update-panel.sh is streamed to the panel and executed
# there. Script fixes do not have to be pushed first. Humid source the
# panel builds still comes from origin (update-panel.sh resets to that).
# Push source/CMake changes you want on the panel before running this.
#
set -euo pipefail

SSH_PORT="${SSH_PORT:-2222}"
HUMID_DIR="${HUMID_DIR:-/opt/humid}"
HOSTS=()
PANEL_ARGS=()
HOSTS_FILE=""
HERE="$(cd "$(dirname "$0")" && pwd)"
LOCAL_PANEL_SCRIPT="${HERE}/update-panel.sh"

usage() {
  cat <<'EOF'
Run scripts/update-panel.sh on one or more panels over SSH.

Usage:
  ./scripts/update-panels.sh [options] HOST... [-- PANEL_OPTIONS...]

Options:
  -p, --port PORT        SSH port (default: 2222)
  --humid-dir PATH       remote Humid checkout (default: /opt/humid)
  --hosts-file FILE      read panel hosts from FILE
  --help                 show this help

Arguments after -- are passed to update-panel.sh on each host.

This wrapper streams the local update-panel.sh to each panel, so HTMLVIEW
package install and other script fixes apply even before those script
changes are on origin. The panel still hard-resets Humid sources to
origin/<branch>.

HTMLVIEW packages (libcairo2-dev, libpango1.0-dev, libfontconfig1-dev) are
installed on the panel by update-panel.sh when missing. Pass
--without-htmlview or --no-htmlview-deps after -- to skip that.
EOF
  exit "${1:-0}"
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    -h|--help) usage 0 ;;
    -p|--port) SSH_PORT="$2"; shift 2 ;;
    --humid-dir) HUMID_DIR="$2"; shift 2 ;;
    --hosts-file) HOSTS_FILE="$2"; shift 2 ;;
    --) shift; PANEL_ARGS+=("$@"); break ;;
    -*)
      # treat unknown flags as start of panel args if we already have hosts
      if [[ ${#HOSTS[@]} -gt 0 ]]; then
        PANEL_ARGS+=("$@")
        break
      fi
      echo "unknown option: $1" >&2
      usage 1
      ;;
    *) HOSTS+=("$1"); shift ;;
  esac
done

if [[ -n "$HOSTS_FILE" ]]; then
  while IFS= read -r line || [[ -n "$line" ]]; do
    [[ -z "$line" || "$line" =~ ^# ]] && continue
    HOSTS+=("$line")
  done < "$HOSTS_FILE"
fi

[[ ${#HOSTS[@]} -gt 0 ]] || { echo "no hosts given" >&2; usage 1; }
[[ -f "$LOCAL_PANEL_SCRIPT" ]] || { echo "missing $LOCAL_PANEL_SCRIPT" >&2; exit 1; }

quote_remote() {
  printf '%q' "$1"
}

REMOTE_DIR_Q="$(quote_remote "$HUMID_DIR")"
# --root is required when the script is run via bash -s ($0 is "bash").
REMOTE_ARGS=(--root "$HUMID_DIR" --force-submodules)
if [[ ${#PANEL_ARGS[@]} -gt 0 ]]; then
  REMOTE_ARGS+=("${PANEL_ARGS[@]}")
fi
REMOTE_ARGS_Q=""
for a in "${REMOTE_ARGS[@]}"; do
  REMOTE_ARGS_Q+=" $(quote_remote "$a")"
done

fail=0
for host in "${HOSTS[@]}"; do
  echo
  echo "######## $host ########"
  # Stream the laptop's update-panel.sh. A remote git reset cannot replace
  # the script already being executed from stdin.
  if ssh -p "$SSH_PORT" -o StrictHostKeyChecking=accept-new "$host" \
      "cd ${REMOTE_DIR_Q} && bash -s --${REMOTE_ARGS_Q}" \
      < "$LOCAL_PANEL_SCRIPT"; then
    echo "######## $host OK ########"
  else
    echo "######## $host FAILED ########" >&2
    fail=1
  fi
done

exit "$fail"
