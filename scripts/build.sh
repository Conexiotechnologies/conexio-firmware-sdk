#!/usr/bin/env bash
# =============================================================================
# build.sh — Conexio Firmware SDK build helper for NCS 3.4.1
#
# Automatically sets up the correct NCS toolchain environment and runs
# west build for any sample in this SDK.
#
# Usage:
#   build.sh [OPTIONS] <sample>
#
# Arguments:
#   <sample>    Path to the sample directory. Can be:
#               - Relative to the SDK root:  samples/conexio_stratus/at_client
#               - Absolute:                  /opt/nordic/ncs/v3.4.1/conexio-firmware-sdk/samples/...
#               - Relative to CWD:           ./samples/conexio_stratus/at_client
#
# Options:
#   -b, --board <board>      Target board qualifier (default: conexio_stratus_pro/nrf9151/ns)
#   -d, --build-dir <dir>    Build output directory (default: <sample>/build)
#   -p, --pristine           Force a clean (pristine) rebuild
#   -t, --toolchain <id>     Override toolchain bundle ID (auto-detected by default)
#   -h, --help               Show this help message and exit
#
# Examples:
#   build.sh samples/conexio_stratus/at_client
#   build.sh -p samples/conexio_stratus/at_client
#   build.sh -b conexio_stratus_pro/nrf9161/ns samples/conexio_stratus/at_client
#   build.sh -b conexio_stratus_pro/nrf9151/ns -d /tmp/my_build samples/conexio_stratus/cellular_modem
# =============================================================================

set -euo pipefail

# ---------------------------------------------------------------------------
# Paths — resolve SDK root and NCS root from this script's location
# ---------------------------------------------------------------------------

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SDK_DIR="$(dirname "$SCRIPT_DIR")"          # conexio-firmware-sdk/
NCS_ROOT="$(dirname "$SDK_DIR")"            # ncs/v3.4.1/
NCS_VERSION="$(basename "$NCS_ROOT")"       # v3.4.1
TOOLCHAINS_DIR="$(dirname "$NCS_ROOT")/toolchains"   # ncs/toolchains/
TOOLCHAINS_JSON="${TOOLCHAINS_DIR}/toolchains.json"

# ---------------------------------------------------------------------------
# Defaults
# ---------------------------------------------------------------------------

DEFAULT_BOARD="conexio_stratus_pro/nrf9151/ns"
BOARD=""
BUILD_DIR=""
PRISTINE=""
TOOLCHAIN_OVERRIDE=""
SAMPLE_ARG=""

# ---------------------------------------------------------------------------
# Argument parsing
# ---------------------------------------------------------------------------

usage() {
    sed -n '/^# Usage:/,/^# =====/{ /^# =====/d; s/^# \{0,3\}//; p }' "${BASH_SOURCE[0]}"
    exit 0
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        -b|--board)       BOARD="$2";             shift 2 ;;
        -d|--build-dir)   BUILD_DIR="$2";         shift 2 ;;
        -p|--pristine)    PRISTINE="--pristine";  shift   ;;
        -t|--toolchain)   TOOLCHAIN_OVERRIDE="$2"; shift 2 ;;
        -h|--help)        usage ;;
        -*)  echo "ERROR: Unknown option: $1" >&2; echo "Run with --help for usage." >&2; exit 1 ;;
        *)   SAMPLE_ARG="$1"; shift ;;
    esac
done

BOARD="${BOARD:-$DEFAULT_BOARD}"

# ---------------------------------------------------------------------------
# Resolve sample path
# ---------------------------------------------------------------------------

if [[ -z "$SAMPLE_ARG" ]]; then
    echo "ERROR: No sample specified." >&2
    echo "Run with --help for usage." >&2
    exit 1
fi

# Try absolute, then relative to CWD, then relative to SDK root
if [[ "$SAMPLE_ARG" = /* && -d "$SAMPLE_ARG" ]]; then
    SAMPLE_DIR="$(realpath "$SAMPLE_ARG")"
elif [[ -d "$SAMPLE_ARG" ]]; then
    SAMPLE_DIR="$(realpath "$SAMPLE_ARG")"
elif [[ -d "${SDK_DIR}/${SAMPLE_ARG}" ]]; then
    SAMPLE_DIR="$(realpath "${SDK_DIR}/${SAMPLE_ARG}")"
else
    echo "ERROR: Sample directory not found: ${SAMPLE_ARG}" >&2
    echo "  Tried: ${SAMPLE_ARG}" >&2
    echo "  Tried: ${SDK_DIR}/${SAMPLE_ARG}" >&2
    exit 1
fi

if [[ ! -f "${SAMPLE_DIR}/CMakeLists.txt" ]]; then
    echo "ERROR: No CMakeLists.txt in ${SAMPLE_DIR}" >&2
    exit 1
fi

BUILD_DIR="${BUILD_DIR:-${SAMPLE_DIR}/build}"

# ---------------------------------------------------------------------------
# Detect toolchain bundle ID from toolchains.json
# ---------------------------------------------------------------------------

find_toolchain() {
    local version="$1"
    local json="$2"

    # Use Python to parse the JSON — Python is guaranteed available in the toolchain.
    # Try the system Python first (fast path), then fall back to searching the toolchains.
    local py
    py="$(command -v python3 2>/dev/null || true)"

    if [[ -z "$py" ]]; then
        # Last resort: find any python3 under the toolchains dir
        py="$(find "$TOOLCHAINS_DIR" -maxdepth 6 -name "python3*" -type f 2>/dev/null | head -1)"
    fi

    if [[ -z "$py" ]]; then
        echo "" ; return
    fi

    "$py" - "$version" "$json" <<'EOF'
import sys, json
version, path = sys.argv[1], sys.argv[2]
try:
    data = json.load(open(path))
    for entry in data[0].get("toolchains", []):
        if version in entry.get("ncs_versions", []):
            print(entry["identifier"]["bundle_id"])
            sys.exit(0)
except Exception:
    pass
sys.exit(1)
EOF
}

if [[ -n "$TOOLCHAIN_OVERRIDE" ]]; then
    BUNDLE_ID="$TOOLCHAIN_OVERRIDE"
elif [[ -f "$TOOLCHAINS_JSON" ]]; then
    BUNDLE_ID="$(find_toolchain "$NCS_VERSION" "$TOOLCHAINS_JSON" || true)"
else
    BUNDLE_ID=""
fi

if [[ -z "$BUNDLE_ID" ]]; then
    echo "WARNING: Could not auto-detect toolchain for ${NCS_VERSION}." >&2
    echo "         Pass -t <bundle_id> to specify one explicitly." >&2
    echo "         Continuing with current PATH (may fail if toolchain is not active)." >&2
    TOOLCHAIN_ROOT=""
else
    TOOLCHAIN_ROOT="${TOOLCHAINS_DIR}/${BUNDLE_ID}"
    if [[ ! -d "$TOOLCHAIN_ROOT" ]]; then
        echo "ERROR: Toolchain directory not found: ${TOOLCHAIN_ROOT}" >&2
        exit 1
    fi
fi

# ---------------------------------------------------------------------------
# Set up environment from the toolchain's environment.json
# (mirrors what the nRF Connect IDE does at build time)
# ---------------------------------------------------------------------------

if [[ -n "$TOOLCHAIN_ROOT" ]]; then
    ENV_JSON="${TOOLCHAIN_ROOT}/environment.json"

    # Build a PATH from the relative_paths entries for PATH in environment.json
    TOOLCHAIN_PATHS=""
    if [[ -f "$ENV_JSON" ]]; then
        TOOLCHAIN_PATHS="$(python3 - "$TOOLCHAIN_ROOT" "$ENV_JSON" <<'EOF'
import sys, json, os
root, path = sys.argv[1], sys.argv[2]
try:
    data = json.load(open(path))
    parts = []
    for var in data.get("env_vars", []):
        if var.get("key") == "PATH" and var.get("type") == "relative_paths":
            for v in var.get("values", []):
                p = os.path.join(root, v)
                if os.path.isdir(p):
                    parts.append(p)
    print(":".join(parts))
except Exception as e:
    print("", end="")
EOF
)"
    fi

    if [[ -n "$TOOLCHAIN_PATHS" ]]; then
        export PATH="${TOOLCHAIN_PATHS}:${PATH}"
    fi

    export ZEPHYR_SDK_INSTALL_DIR="${TOOLCHAIN_ROOT}/opt/zephyr-sdk"
    export ZEPHYR_TOOLCHAIN_VARIANT="zephyr"
fi

# ---------------------------------------------------------------------------
# Verify west is available
# ---------------------------------------------------------------------------

if ! command -v west &>/dev/null; then
    echo "ERROR: 'west' not found on PATH even after toolchain setup." >&2
    echo "       Toolchain root: ${TOOLCHAIN_ROOT:-<not set>}" >&2
    exit 1
fi

# ---------------------------------------------------------------------------
# Print build summary
# ---------------------------------------------------------------------------

echo "╔══════════════════════════════════════════════════════════════╗"
echo "║          Conexio Firmware SDK — west build                   ║"
echo "╚══════════════════════════════════════════════════════════════╝"
echo "  NCS root     : ${NCS_ROOT}"
echo "  SDK root     : ${SDK_DIR}"
echo "  Toolchain    : ${BUNDLE_ID:-<system>}"
echo "  Sample       : ${SAMPLE_DIR}"
echo "  Board        : ${BOARD}"
echo "  Build dir    : ${BUILD_DIR}"
echo "  Pristine     : ${PRISTINE:---no--}"
echo "  west         : $(command -v west)"
echo ""

# ---------------------------------------------------------------------------
# Run west build
# (cd to NCS root so west resolves the correct workspace + Zephyr version)
# ---------------------------------------------------------------------------

cd "$NCS_ROOT"

# shellcheck disable=SC2086
exec west build \
    --build-dir  "$BUILD_DIR" \
    "$SAMPLE_DIR" \
    ${PRISTINE} \
    --board "$BOARD" \
    -- \
    -DBOARD_ROOT="$SDK_DIR"
