#!/usr/bin/env bash

set -uo pipefail

usage() {
    cat <<EOF
usage: CLOCKWORK=/path/to/clockwork $0 [--list]
       CLOCKWORK=/path/to/clockwork $0 --pull [filename]
       CLOCKWORK=/path/to/clockwork $0 --push [filename]

Compare local src files with CLOCKWORK/iod/src, ignoring license headers.

  --list             list only files whose contents differ
  --pull [filename]  copy from Clockwork, preserving local license headers
  --push [filename]  copy to Clockwork, preserving its license headers
  --help             show this help message
EOF
}

mode=compare
filename=
case "${1:-}" in
    "") ;;
    --help)
        usage
        exit 0
        ;;
    --list)
        mode=list
        shift
        ;;
    --pull|--push)
        mode=${1#--}
        shift
        filename=${1:-}
        [[ $# -eq 0 ]] || shift
        ;;
    *)
        usage >&2
        exit 2
        ;;
esac

if [[ $# -ne 0 ]]; then
    usage >&2
    exit 2
fi

script_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
local_src="$script_dir/src"

if [[ -z "${CLOCKWORK:-}" ]]; then
    echo "CLOCKWORK is not set" >&2
    usage >&2
    exit 2
fi

clockwork_src="${CLOCKWORK%/}/iod/src"
if [[ ! -d "$clockwork_src" ]]; then
    echo "Clockwork source directory not found: $clockwork_src" >&2
    exit 2
fi

temporary_dir=$(mktemp -d "${TMPDIR:-/tmp}/compare-clockwork-src.XXXXXX") || exit 2
trap 'rm -rf "$temporary_dir"' EXIT

# Print either the initial license comment or everything after it.
filter_initial_license() {
    awk -v part="$1" '
        BEGIN { state = "leading"; buffered = ""; license = 0 }
        state == "leading" {
            if ($0 ~ /^[[:space:]]*$/) {
                buffered = buffered $0 ORS
                next
            }
            if ($0 ~ /^[[:space:]]*\/\*/) {
                buffered = buffered $0 ORS
                state = "comment"
                if ($0 ~ /(Copyright|License|free software|All rights reserved)/)
                    license = 1
                if ($0 ~ /\*\//) {
                    if (part == "header" && license)
                        printf "%s", buffered
                    if (part == "body" && !license)
                        printf "%s", buffered
                    state = "body"
                }
                next
            }
            if (part == "body")
                printf "%s", buffered
            state = "body"
        }
        state == "comment" {
            buffered = buffered $0 ORS
            if ($0 ~ /(Copyright|License|free software|All rights reserved)/)
                license = 1
            if ($0 ~ /\*\//) {
                if (part == "header" && license)
                    printf "%s", buffered
                if (part == "body" && !license)
                    printf "%s", buffered
                state = "body"
            }
            next
        }
        state == "body" && part == "body" { print }
        END {
            if (part == "body" && (state == "leading" || state == "comment"))
                printf "%s", buffered
        }
    ' "$2"
}

copy_preserving_destination_license() {
    source_file=$1
    destination_file=$2
    combined_file="$temporary_dir/transfer"

    {
        filter_initial_license header "$destination_file"
        filter_initial_license body "$source_file"
    } > "$combined_file" || return 2

    if cmp -s "$combined_file" "$destination_file"; then
        return 1
    fi
    cp "$combined_file" "$destination_file" || return 2
    return 0
}

files=()
if [[ -n "$filename" ]]; then
    relative=${filename#src/}
    if [[ -z "$relative" || "$relative" == /* || "$relative" == ".." ||
          "$relative" == ../* || "$relative" == */../* || "$relative" == */.. ]]; then
        echo "Invalid source filename: $filename" >&2
        exit 2
    fi
    if [[ ! -f "$local_src/$relative" ]]; then
        echo "Local source file not found: $relative" >&2
        exit 2
    fi
    files+=("$local_src/$relative")
else
    while IFS= read -r -d '' local_file; do
        files+=("$local_file")
    done < <(find "$local_src" -type f -print0)
fi

status=0
different=0
changed=0
count=0

for local_file in "${files[@]}"; do
    relative=${local_file#"$local_src"/}
    clockwork_file="$clockwork_src/$relative"
    count=$((count + 1))

    if [[ ! -f "$clockwork_file" ]]; then
        if [[ "$mode" == list ]]; then
            echo "$relative"
        else
            echo "Missing from Clockwork: $relative" >&2
        fi
        status=1
        different=$((different + 1))
        continue
    fi

    if [[ "$mode" == pull || "$mode" == push ]]; then
        if [[ "$mode" == pull ]]; then
            source_file=$clockwork_file
            destination_file=$local_file
        else
            source_file=$local_file
            destination_file=$clockwork_file
        fi

        copy_preserving_destination_license "$source_file" "$destination_file"
        copy_status=$?
        if [[ $copy_status -eq 0 ]]; then
            echo "$relative"
            changed=$((changed + 1))
        elif [[ $copy_status -gt 1 ]]; then
            echo "Failed to $mode $relative" >&2
            exit "$copy_status"
        fi
        continue
    fi

    local_comparison="$temporary_dir/local-$count"
    clockwork_comparison="$temporary_dir/clockwork-$count"
    filter_initial_license body "$local_file" > "$local_comparison" || exit 2
    filter_initial_license body "$clockwork_file" > "$clockwork_comparison" || exit 2

    if [[ "$mode" == list ]]; then
        if ! cmp -s "$clockwork_comparison" "$local_comparison"; then
            echo "$relative"
            status=1
            different=$((different + 1))
        fi
        continue
    fi

    diff -u --label "$clockwork_file" --label "$local_file" \
        "$clockwork_comparison" "$local_comparison"
    diff_status=$?
    if [[ $diff_status -ne 0 ]]; then
        if [[ $diff_status -gt 1 ]]; then
            exit "$diff_status"
        fi
        status=1
        different=$((different + 1))
    fi
done

if [[ "$mode" == pull || "$mode" == push ]]; then
    if [[ "$mode" == pull ]]; then
        operation=Pull
    else
        operation=Push
    fi
    echo "$operation: updated $changed of $count local source files" >&2
elif [[ "$mode" == compare && $status -eq 0 ]]; then
    echo "All $count local source files match $clockwork_src"
elif [[ "$mode" == compare ]]; then
    echo "$different of $count local source files differ from $clockwork_src" >&2
fi

exit "$status"
