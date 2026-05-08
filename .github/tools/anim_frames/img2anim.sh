#!/usr/bin/env bash
# =============================================================================
# img2anim.sh — Convert images or an ANI file to the animation frames header.
#
# General-purpose tool — lives in .github/tools/anim_frames/.
# Sets up an isolated Python venv on first run (Pillow only), then calls
# img2anim.py with the arguments you provide.
#
# Usage (run from your variant directory, e.g. variants/nrf52840/eink-ra01s-gp02/):
#   /path/to/img2anim.sh --input /path/to/pointer.ani
#   /path/to/img2anim.sh --input /path/to/pointer.ani --count 10 --width 64 --height 64
#   /path/to/img2anim.sh --input a.png,b.png,c.png   --count 10 --preview
#   /path/to/img2anim.sh --input busy.ani --invert --preview
#
# All flags are forwarded to img2anim.py.  Run with --help for full list.
#
# Output:
#   Writes anim_frames.h in the CURRENT WORKING DIRECTORY by default.
#   Pass --output /other/path/anim_frames.h to change the destination.
# =============================================================================
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
VENV="$SCRIPT_DIR/.venv"
PY_SCRIPT="$SCRIPT_DIR/img2anim.py"
DEFAULT_OUTPUT="$PWD/anim_frames.h"

# ---------------------------------------------------------------------------
# Bootstrap venv if needed (shared with oem_splash tool if present)
# ---------------------------------------------------------------------------
if [[ ! -x "$VENV/bin/python" ]]; then
	echo "[img2anim] Creating Python venv in $VENV …"
	python3 -m venv "$VENV"
	echo "[img2anim] Installing Pillow …"
	"$VENV/bin/pip" install --quiet --upgrade pip
	"$VENV/bin/pip" install --quiet Pillow
	echo "[img2anim] Done."
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
echo "[img2anim] Running conversion …"
"$VENV/bin/python" "$PY_SCRIPT" "$@" "${EXTRA_ARGS[@]}"
