#!/usr/bin/env bash
# Run scripts/update-panel.sh on one or more panels over SSH.
#
# Usage (from a machine with SSH access):
#   ./scripts/update-panels.sh root@172.29.52.10 root@172.29.53.11
#   ./scripts/update-panels.sh -p 2222 root@172.29.52.10
#   ./scripts/update-panels.sh --hosts-file panels.txt -- --restart
#
# Extra args after -- are passed to update-panel.sh on each host.
# First push Humid so panels can fetch the script, vendored client, and fixes.
#
set -euo pipefail

SSH_PORT="${SSH_PORT:-2222}"
HUMID_DIR="${HUMID_DIR:-/opt/humid}"
HOSTS=()
PANEL_ARGS=()
HOSTS_FILE=""

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

REMOTE_CMD=$(cat <<EOF
set -euo pipefail
cd '$HUMID_DIR'
# bootstrap: pull first so a newly added script is available
git -c fetch.recurseSubmodules=no fetch origin
git checkout master 2>/dev/null || git checkout -B master origin/master
git pull --ff-only origin master
if [[ ! -x scripts/update-panel.sh ]]; then
  echo "ERROR: $HUMID_DIR/scripts/update-panel.sh missing after pull" >&2
  exit 1
fi
exec ./scripts/update-panel.sh --force-submodules ${PANEL_ARGS[@]+"${PANEL_ARGS[@]}"}
EOF
)

fail=0
for host in "${HOSTS[@]}"; do
  echo
  echo "######## $host ########"
  if ssh -p "$SSH_PORT" -o StrictHostKeyChecking=accept-new "$host" "bash -s" <<<"$REMOTE_CMD"; then
    echo "######## $host OK ########"
  else
    echo "######## $host FAILED ########" >&2
    fail=1
  fi
done

exit "$fail"
