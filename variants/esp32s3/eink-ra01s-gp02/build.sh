#!/usr/bin/env bash

# Specify interpreter
# First run `chmod +x build.sh` to grant permission, then run `./build.sh` directly — no need for `source` or `bash` prefix

RED='\033[0;31m'
GREEN='\033[0;32m'
RESET='\033[0m'

# Enable strict mode
set -euo pipefail

TAG="build.sh:info"
printf "[${TAG}] Running script...\n"

PIO_DEFAULT="${HOME}/.platformio/penv/bin/pio"
if [ -x "${PIO_DEFAULT}" ]; then
	PIO_BIN="${PIO_DEFAULT}"
else
	PIO_BIN="$(command -v pio 2>/dev/null)" || {
		printf "${RED}[${TAG}] pio not found, please install PlatformIO first${RESET}\n"
		exit 1
	}
fi
printf "[${TAG}] pio path: ${PIO_BIN}\n"

VARIANT_NAME="$(basename "$(dirname "$(realpath "${BASH_SOURCE[0]}")")")"
printf "[${TAG}] Current variant: ${VARIANT_NAME}\n"

ROOT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../../.." && pwd)"
printf "[${TAG}] Project root: ${ROOT_DIR}\n"

LOG_DIR="${ROOT_DIR}/.pio/logs/${VARIANT_NAME}"
mkdir -p "${LOG_DIR}"
printf "[${TAG}] Log directory: ${LOG_DIR}\n"

LOG_FILE="${LOG_DIR}/build-$(date +%Y%m%d-%H%M%S).log"

set +e
"${PIO_BIN}" run --verbose --environment "${VARIANT_NAME}" "$@" 2>&1 | tee "${LOG_FILE}"
EXIT_CODE=${PIPESTATUS[0]}
set -e

if [ "${EXIT_CODE}" -ne 0 ]; then
	printf "\n${RED}[${TAG}] Failed, log: ${LOG_FILE}${RESET}\n"
	exit "${EXIT_CODE}"
else
	printf "\n${GREEN}[${TAG}] Success, log: ${LOG_FILE}${RESET}\n"
fi
