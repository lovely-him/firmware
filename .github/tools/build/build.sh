#!/usr/bin/env bash

# Central build script — called by per-variant thin wrappers.
# Usage (via wrapper):
#   ./build.sh                 — build firmware
#   ./build.sh project init    — update VSCode IntelliSense only
#   ./build.sh [pio run args]  — build with extra args passed to `pio run`
#
# Direct usage:
#   build.sh <variant> [args...]
#   build.sh <variant> project init

RED='\033[0;31m'
GREEN='\033[0;32m'
RESET='\033[0m'

TAG="build.sh"

log_info() { printf "[${TAG}:info] %s\n" "$*"; }
log_success() { printf "${GREEN}[${TAG}:ok] %s${RESET}\n" "$*"; }
log_error() { printf "${RED}[${TAG}:error] %s${RESET}\n" "$*"; }

set -euo pipefail

if [[ $# -lt 1 ]]; then
	log_error "Usage: $0 <variant_name> [args...]"
	exit 1
fi

VARIANT_NAME="${1}"
shift # remaining args are passed through to pio or handled below

log_info "Running script..."

PIO_DEFAULT="${HOME}/.platformio/penv/bin/pio"
if [[ -x ${PIO_DEFAULT} ]]; then
	PIO_BIN="${PIO_DEFAULT}"
else
	PIO_BIN="$(command -v pio 2>/dev/null)" || {
		log_error "pio not found, please install PlatformIO first"
		exit 1
	}
fi
log_info "pio path: ${PIO_BIN}"

log_info "Current variant: ${VARIANT_NAME}"

# Resolve project root via git, falling back to relative path from this script
SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(git -C "${SCRIPT_DIR}" rev-parse --show-toplevel 2>/dev/null)" ||
	ROOT_DIR="$(cd -- "${SCRIPT_DIR}/../../.." && pwd)"
log_info "Project root: ${ROOT_DIR}"

# ── project init mode ──────────────────────────────────────────────────────────
if [[ ${1-} == "project" && ${2-} == "init" ]]; then
	log_info "Updating VSCode IntelliSense (includePath)..."
	"${PIO_BIN}" project init --ide vscode --environment "${VARIANT_NAME}" --project-dir "${ROOT_DIR}"
	log_info "Patching c_cpp_properties.json with dynamic defines (USERPREFS_*, APP_VERSION, etc.)..."
	python3 "${ROOT_DIR}/.github/tools/build/patch-intellisense.py" "${ROOT_DIR}" "${VARIANT_NAME}"
	log_success "IntelliSense updated."
	exit 0
fi

# ── build mode ─────────────────────────────────────────────────────────────────
LOG_DIR="${ROOT_DIR}/.pio/logs/${VARIANT_NAME}"
mkdir -p "${LOG_DIR}"
log_info "Log directory: ${LOG_DIR}"

LOG_FILE="${LOG_DIR}/build-$(date +%Y%m%d-%H%M%S).log"

set +e
"${PIO_BIN}" run --verbose -t compiledb --environment "${VARIANT_NAME}" "$@" 2>&1 | tee "${LOG_FILE}"
EXIT_CODE=${PIPESTATUS[0]}
set -e

if [[ ${EXIT_CODE} -ne 0 ]]; then
	log_error "Failed, log: ${LOG_FILE}"
	exit "${EXIT_CODE}"
else
	log_success "Build succeeded, log: ${LOG_FILE}"
fi
