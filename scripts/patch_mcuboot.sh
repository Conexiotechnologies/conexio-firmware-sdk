#!/usr/bin/env bash
# =============================================================================
# patch_mcuboot.sh — Conexio Stratus Pro MCUBoot patch
#
# Copies the MCUBoot board configuration files for Conexio Stratus Pro devices
# into the correct location inside the NCS tree.
#
# Usage:
#   Run from anywhere inside the NCS workspace:
#     bash /path/to/conexio-firmware-sdk/scripts/patch_mcuboot.sh
#
#   Or, if you want to specify the NCS root explicitly:
#     bash patch_mcuboot.sh /opt/nordic/ncs/v2.9.0
#
# What it does:
#   Copies:
#     conexio-firmware-sdk/bootloader/mcuboot/conexio_stratus_pro_nrf9151.conf
#     conexio-firmware-sdk/bootloader/mcuboot/conexio_stratus_pro_nrf9161.conf
#   Into:
#     bootloader/mcuboot/boot/zephyr/boards/
#
# This script is a fallback alternative to `west conexio-patch`.
# Both do the same thing.
# =============================================================================

set -euo pipefail

# ---------------------------------------------------------------------------
# Resolve the NCS workspace root
#
# Priority:
#   1. First argument passed to the script  ($1)
#   2. Auto-detect by walking up from this script's location
# ---------------------------------------------------------------------------

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SDK_DIR="$(dirname "$SCRIPT_DIR")"   # one level up from scripts/

if [[ $# -ge 1 ]]; then
    NCS_ROOT="$(realpath "$1")"
else
    # Walk up from the SDK directory to find the NCS root.
    # The NCS root is the parent of conexio-firmware-sdk/.
    NCS_ROOT="$(dirname "$SDK_DIR")"
fi

# ---------------------------------------------------------------------------
# Validate paths
# ---------------------------------------------------------------------------

SRC_DIR="${SDK_DIR}/bootloader/mcuboot"
DEST_DIR="${NCS_ROOT}/bootloader/mcuboot/boot/zephyr/boards"

echo "Conexio Stratus Pro MCUBoot patch"
echo "=================================="
echo "NCS root : ${NCS_ROOT}"
echo "Source   : ${SRC_DIR}"
echo "Dest     : ${DEST_DIR}"
echo ""

if [[ ! -d "$SRC_DIR" ]]; then
    echo "ERROR: Source directory not found: ${SRC_DIR}" >&2
    echo "       Make sure you are running this from inside the conexio-firmware-sdk." >&2
    exit 1
fi

if [[ ! -d "$DEST_DIR" ]]; then
    echo "ERROR: Destination directory not found: ${DEST_DIR}" >&2
    echo "       Make sure the NCS workspace is set up and 'west update' has been run." >&2
    exit 1
fi

# ---------------------------------------------------------------------------
# Copy each conf file, skipping if already up to date
# ---------------------------------------------------------------------------

COPIED=0

for SRC_FILE in "${SRC_DIR}"/conexio_stratus_pro_*.conf; do
    if [[ ! -f "$SRC_FILE" ]]; then
        echo "WARNING: No matching .conf files found in ${SRC_DIR}" >&2
        exit 1
    fi

    FILENAME="$(basename "$SRC_FILE")"
    DEST_FILE="${DEST_DIR}/${FILENAME}"

    if [[ -f "$DEST_FILE" ]] && cmp -s "$SRC_FILE" "$DEST_FILE"; then
        echo "  [skip]   ${FILENAME}  (already up to date)"
    else
        if [[ -f "$DEST_FILE" ]]; then
            echo "  [update] ${FILENAME}"
        else
            echo "  [copy]   ${FILENAME}"
        fi
        cp "$SRC_FILE" "$DEST_FILE"
        COPIED=$((COPIED + 1))
    fi
done

# ---------------------------------------------------------------------------
# Remove the outdated generic board conf (replaced by the nrf9151/nrf9161
# variants copied above)
# ---------------------------------------------------------------------------

LEGACY_FILE="${DEST_DIR}/conexio_stratus_pro.conf"

if [[ -f "$LEGACY_FILE" ]]; then
    echo "  [delete] conexio_stratus_pro.conf  (outdated — replaced by nrf9151/nrf9161 variants)"
    rm "$LEGACY_FILE"
fi

# ---------------------------------------------------------------------------
# Summary
# ---------------------------------------------------------------------------

echo ""
if [[ $COPIED -gt 0 ]]; then
    echo "Conexio Stratus Pro MCUBoot patch applied: ${COPIED} file(s) copied to:"
    echo "  ${DEST_DIR}"
else
    echo "Conexio Stratus Pro MCUBoot patch: all files already up to date."
fi
