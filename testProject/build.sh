#!/usr/bin/env bash
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
build_dir="$root/build"

if [[ $# -eq 0 ]]; then
    cmake -S "$root" -B "$build_dir"
    echo "Configured project in $build_dir"
    exit 0
fi

cmake -S "$root" -B "$build_dir"

if [[ $# -eq 1 ]]; then
    target="$1"
    cmake --build "$build_dir" --target "$target"
    exit 0
fi

cmake --build "$build_dir" --target "$1" -- ${@:2}
