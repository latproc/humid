#!/usr/bin/env bash
# Run scripts/update-panel.sh on one or more panels over SSH.
#
# Usage (from a machine with SSH access):
#   ./scripts/update-panels.sh root@172.29.52.10 root@172.29.53.11
#   ./scripts/update-panels.sh -p 2222 root@172.29.52.10
#   ./scripts/update-panels.sh --hosts-file panels.txt -- --restart
#
# Extra args after -- are passed to update-panel.sh on each host.
# First push humid (and clockwork) so panels can git pull the script + fixes.
#
set -euo pipefail

SSH_PORT="${SSH_PORT:-2222}"
HUMID_DIR="${HUMID_DIR:-/opt/humid}"
HOSTS=()
PANEL_ARGS=()
HOSTS_FILE=""

usage() {
  sed -n '2,14p' "$0" | sed 's/^# \{0,1\}//'
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
git fetch origin
git checkout cw-no-ec-tools-compatiblity 2>/dev/null || true
git pull --ff-only || git pull --ff-only origin cw-no-ec-tools-compatiblity || true
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
