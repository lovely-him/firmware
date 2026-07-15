#!/usr/bin/env bash
# Thin wrapper for the InkHUD variant — delegates to the central build script.
# First run `chmod +x build-inkhud.sh` to grant permission.
# Usage:
#   ./build-inkhud.sh                 — build InkHUD firmware
#   ./build-inkhud.sh project init    — update VSCode IntelliSense only
#   ./build-inkhud.sh [pio run args]  — build with extra args passed to `pio run`

SELF_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
VARIANT_NAME="$(basename "${SELF_DIR}")-inkhud"
exec bash "${SELF_DIR}/../../../.github/tools/build/build.sh" "${VARIANT_NAME}" "$@"
