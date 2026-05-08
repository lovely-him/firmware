#!/usr/bin/env bash
# Thin wrapper — delegates to the central guard check script.
# First run `chmod +x check-guards.sh` to grant permission.
#
# Guard macro is derived automatically from the variant directory name:
#   eink-ra01s-gp02  →  EINK_RA01S_GP02  (hyphens → underscores, uppercased)
#
# Override: set GUARD_MACRO in the environment to use a different macro name.
#   GUARD_MACRO=MY_CUSTOM_MACRO ./check-guards.sh
#
# Usage:
#   ./check-guards.sh          — report mode (exits 1 if issues found)
#   ./check-guards.sh --fix    — report, then prompt to auto-fix

SELF_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
VARIANT_NAME="$(basename "${SELF_DIR}")"

# Derive macro: hyphens → underscores, then uppercase
_DERIVED_MACRO="$(printf '%s' "${VARIANT_NAME}" | tr '-' '_' | tr '[:lower:]' '[:upper:]' || true)"

# ENV override takes priority; fall back to derived value
GUARD_MACRO="${GUARD_MACRO:-${_DERIVED_MACRO}}"

exec python3 "${SELF_DIR}/../../../.github/tools/check-eink-guards/check_eink_guards.py" \
	--macro "${GUARD_MACRO}" \
	"$@"
