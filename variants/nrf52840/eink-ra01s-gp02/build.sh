#!/usr/bin/env bash
# Thin wrapper — delegates to the central build script.
# First run `chmod +x build.sh` to grant permission, then run `./build.sh` directly.
# Usage:
#   ./build.sh                 — build firmware
#   ./build.sh project init    — update VSCode IntelliSense only
#   ./build.sh [pio run args]  — build with extra args passed to `pio run`

SELF_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
VARIANT_NAME="$(basename "${SELF_DIR}")"
exec bash "${SELF_DIR}/../../../.github/tools/build/build.sh" "${VARIANT_NAME}" "$@"
