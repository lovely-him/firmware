#!/usr/bin/env bash
# Central build script for firmware-nrf52840.
#
# Usage:
#   .github/tools/build/build.sh <env_name>              		— build firmware (+ upload, etc.)
#   .github/tools/build/build.sh <env_name> project init 		— update VSCode IntelliSense + compile_commands.json
#   .github/tools/build/build.sh <env_name> -t upload    		— upload (protocol from platformio.ini)
#   .github/tools/build/build.sh <env_name> -t monitor [opts] 	— open serial monitor (interactive, no log)
#
# Variant thin wrapper: variants/nrf52840/<name>/build.sh

set -euo pipefail

# ── 路径与常量 ───────────────────────────────────────────────────────
RED='\033[0;31m'
GREEN='\033[0;32m'
RESET='\033[0m'
TAG="build.sh"

# ── 函数 ─────────────────────────────────────────────────────────────
log_info() { printf "[${TAG}:info] %s\n" "$*"; }
log_success() { printf "${GREEN}[${TAG}:ok] %s${RESET}\n" "$*"; }
log_error() { printf "${RED}[${TAG}:error] %s${RESET}\n" "$*"; }

resolve_pio() {
	local pio_default="${HOME}/.platformio/penv/bin/pio"
	if [[ -x ${pio_default} ]]; then
		PIO_BIN="${pio_default}"
	else
		PIO_BIN="$(command -v pio 2>/dev/null)" || {
			log_error "pio not found, please install PlatformIO first"
			exit 1
		}
	fi
}

resolve_root() {
	SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
	ROOT_DIR="$(git -C "${SCRIPT_DIR}" rev-parse --show-toplevel 2>/dev/null)" ||
		ROOT_DIR="$(cd -- "${SCRIPT_DIR}/../../.." && pwd)"
}

alloc_log_file() {
	local env_name=$1
	LOG_DIR="${ROOT_DIR}/.pio/.tmp/${env_name}"
	LOG_FILE="${LOG_DIR}/$(date +%Y%m%d-%H%M%S)-$$.log"
}

_run_logged() {
	local log_file=$1
	shift
	mkdir -p "$(dirname "${log_file}")"
	(
		exec > >(tee "${log_file}") 2>&1
		"$@"
	)
}

_target_has_monitor() {
	local target=$1
	local part
	IFS=',' read -ra parts <<<"${target}"
	for part in "${parts[@]}"; do
		part="${part#"${part%%[![:space:]]*}"}"
		part="${part%"${part##*[![:space:]]}"}"
		if [[ ${part} == "monitor" ]]; then
			return 0
		fi
	done
	return 1
}

args_contain_monitor() {
	local i=1
	while [[ ${i} -le $# ]]; do
		local arg="${!i}"
		if [[ ${arg} == "-t" ]]; then
			i=$((i + 1))
			if [[ ${i} -le $# ]] && _target_has_monitor "${!i}"; then
				return 0
			fi
		fi
		i=$((i + 1))
	done
	return 1
}

_monitor_forward_args() {
	local out=()
	local i=1
	while [[ ${i} -le $# ]]; do
		local arg="${!i}"
		if [[ ${arg} == "-t" ]]; then
			i=$((i + 1))
			if [[ ${i} -le $# ]] && _target_has_monitor "${!i}"; then
				i=$((i + 1))
				continue
			fi
			out+=("-t")
			if [[ ${i} -le $# ]]; then
				out+=("${!i}")
			fi
		else
			out+=("${arg}")
		fi
		i=$((i + 1))
	done
	printf '%s\n' "${out[@]}"
}

_run_project_init() {
	log_info "Running script..."
	log_info "pio path: ${PIO_BIN}"
	log_info "Current environment: ${ENV_NAME}"
	log_info "Project root: ${ROOT_DIR}"
	log_info "Log file: ${LOG_FILE}"
	log_info "Updating VSCode IntelliSense..."
	"${PIO_BIN}" project init --ide vscode --environment "${ENV_NAME}" --project-dir "${ROOT_DIR}"
}

_run_compiledb_only() {
	log_info "Running script..."
	log_info "pio path: ${PIO_BIN}"
	log_info "Current environment: ${ENV_NAME}"
	log_info "Project root: ${ROOT_DIR}"
	log_info "Log file: ${LOG_FILE}"
	log_info "Phase 1: compiledb"
	"${PIO_BIN}" run --verbose -t compiledb --environment "${ENV_NAME}"
}

_run_build() {
	log_info "Running script..."
	log_info "pio path: ${PIO_BIN}"
	log_info "Current environment: ${ENV_NAME}"
	log_info "Project root: ${ROOT_DIR}"
	log_info "Log file: ${LOG_FILE}"
	"${PIO_BIN}" run --verbose --environment "${ENV_NAME}" "$@"
}

# ── main ─────────────────────────────────────────────────────────────
main() {
	if [[ $# -lt 1 ]]; then
		log_error "Usage: $0 <env_name> [pio run args...]"
		exit 1
	fi

	ENV_NAME="${1}"
	shift

	resolve_pio
	resolve_root
	alloc_log_file "${ENV_NAME}"

	if [[ ${1-} == "project" && ${2-} == "init" ]]; then
		# Step 1: project init
		local init_exit=0
		_run_logged "${LOG_FILE}" _run_project_init || init_exit=$?
		if [[ ${init_exit} -ne 0 ]]; then
			log_error "Failed, log: ${LOG_FILE}"
			return "${init_exit}"
		fi
		log_success "IntelliSense updated, log: ${LOG_FILE}"

		# Step 2: compile_commands.json
		alloc_log_file "${ENV_NAME}"
		local compdb_exit=0
		_run_logged "${LOG_FILE}" _run_compiledb_only || compdb_exit=$?
		if [[ ${compdb_exit} -ne 0 ]]; then
			log_error "Failed, log: ${LOG_FILE}"
			return "${compdb_exit}"
		fi
		log_success "compiledb succeeded, log: ${LOG_FILE}"
		return 0
	fi

	if args_contain_monitor "$@"; then
		log_info "monitor is interactive; no log file"

		local -a monitor_args=()
		while IFS= read -r arg; do
			monitor_args+=("${arg}")
		done < <(_monitor_forward_args "$@")

		exec "${PIO_BIN}" run --verbose -t monitor \
			--environment "${ENV_NAME}" \
			"${monitor_args[@]}"
	fi

	local build_exit=0
	_run_logged "${LOG_FILE}" _run_build "$@" || build_exit=$?
	if [[ ${build_exit} -ne 0 ]]; then
		log_error "Failed, log: ${LOG_FILE}"
	else
		log_success "Build succeeded, log: ${LOG_FILE}"
	fi
	return "${build_exit}"
}

main "$@"
