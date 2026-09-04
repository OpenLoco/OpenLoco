#!/usr/bin/env bash

set -euo pipefail

basedir="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$basedir"

ZIG="${ZIG:-zig}"
ZIG_TARGET="${ZIG_TARGET:-x86_64-linux-gnu.2.28}"
PRESET="${PRESET:-linux-zig-static}"
SHIM_DIR="${SHIM_DIR:-${basedir}/build/zig-shim}"

if ! command -v "$ZIG" >/dev/null 2>&1 && [ ! -x "$ZIG" ]; then
    echo "zig not found, set ZIG to the zig executable" >&2
    exit 1
fi

mkdir -p "$SHIM_DIR"

write_shim() {
    name="$1"
    shift
    printf '#!/usr/bin/env sh\nexec "%s" %s "$@"\n' "$ZIG" "$*" > "${SHIM_DIR}/${name}"
    chmod +x "${SHIM_DIR}/${name}"
}

write_shim zig-cc cc -target "$ZIG_TARGET"
write_shim zig-c++ c++ -target "$ZIG_TARGET"
write_shim zig-ar ar
write_shim zig-ranlib ranlib

export OPENLOCO_ZIG_SHIM_DIR="$SHIM_DIR"

cmake --preset "$PRESET" ${ADDITIONAL_CMAKE_ARGS}
cmake --build --preset "$PRESET"
