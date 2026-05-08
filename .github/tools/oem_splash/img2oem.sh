#!/usr/bin/env bash
# =============================================================================
# img2oem.sh — Convert an image to the OEM splash screen header.
#
# General-purpose tool — lives in .github/tools/oem_splash/.
# Sets up an isolated Python venv on first run (Pillow only), then calls
# img2xbm.py with the arguments you provide.
#
# Usage (run from any directory):
#   /path/to/img2oem.sh --input /path/to/pointer.ani
#   /path/to/img2oem.sh --input logo.png --width 80 --height 60 --invert
#   /path/to/img2oem.sh --input icon.png --text "MY BOARD" --preview
#
# All flags are forwarded to img2xbm.py.  Run with --help for full list:
#   /path/to/img2oem.sh --help
#
# Output:
#   Writes/overwrites oem_splash.h in the CURRENT WORKING DIRECTORY by default.
#   Pass --output /other/path/oem_splash.h to change the destination.
# =============================================================================
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
VENV="$SCRIPT_DIR/.venv"
PY_SCRIPT="$SCRIPT_DIR/img2xbm.py"
DEFAULT_OUTPUT="$PWD/oem_splash.h"

# ---------------------------------------------------------------------------
# Bootstrap venv if needed
# ---------------------------------------------------------------------------
if [[ ! -x "$VENV/bin/python" ]]; then
	echo "[img2oem] Creating Python venv in $VENV …"
	python3 -m venv "$VENV"
	echo "[img2oem] Installing Pillow …"
	"$VENV/bin/pip" install --quiet --upgrade pip
	"$VENV/bin/pip" install --quiet Pillow
	echo "[img2oem] Done."
fi

# ---------------------------------------------------------------------------
# Inject --output default unless the caller already provided one
# ---------------------------------------------------------------------------
EXTRA_ARGS=()
HAS_OUTPUT=false
for arg in "$@"; do
	if [[ $arg == "--output" || $arg == "-o" ]]; then
		HAS_OUTPUT=true
		break
	fi
done

if [[ $HAS_OUTPUT == false ]]; then
	EXTRA_ARGS+=(--output "$DEFAULT_OUTPUT")
fi

# ---------------------------------------------------------------------------
# Run
# ---------------------------------------------------------------------------
echo "[img2oem] Running conversion …"
"$VENV/bin/python" "$PY_SCRIPT" "$@" "${EXTRA_ARGS[@]}"

if [[ $HAS_OUTPUT == false ]]; then
	echo "[img2oem] oem_splash.h written to: $DEFAULT_OUTPUT"
fi
