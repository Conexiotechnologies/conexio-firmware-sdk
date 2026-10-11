#!/usr/bin/env bash
# =============================================================================
# batch_build.sh — Build all Conexio Firmware SDK samples and report results
#
# Usage:
#   batch_build.sh [board]
#   Default board: conexio_stratus_pro/nrf9151/ns
#
# Output:
#   Per-sample build logs: scripts/build_logs/<sample_name>.log
#   Summary report:        scripts/build_logs/summary.txt
# =============================================================================

set -uo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SDK_DIR="$(dirname "$SCRIPT_DIR")"
BOARD="${1:-conexio_stratus_pro/nrf9151/ns}"
LOG_DIR="${SCRIPT_DIR}/build_logs"
SUMMARY="${LOG_DIR}/summary.txt"

mkdir -p "$LOG_DIR"

# Collect all buildable samples
mapfile -t SAMPLES < <(
    find "${SDK_DIR}/samples" -name "CMakeLists.txt" \
    | xargs grep -l "find_package(Zephyr" \
    | sed 's|/CMakeLists.txt||' \
    | sort
)

TOTAL=${#SAMPLES[@]}
PASS=0
FAIL=0
SKIP=0
RESULTS=()

echo "╔══════════════════════════════════════════════════════════════╗"
echo "║       Conexio Firmware SDK — Batch Build                     ║"
echo "╚══════════════════════════════════════════════════════════════╝"
echo "  Board   : ${BOARD}"
echo "  Samples : ${TOTAL}"
echo "  Logs    : ${LOG_DIR}"
echo ""

START_TOTAL=$(date +%s)

for SAMPLE_DIR in "${SAMPLES[@]}"; do
    SAMPLE_NAME="$(basename "$SAMPLE_DIR")"
    LOG_FILE="${LOG_DIR}/${SAMPLE_NAME}.log"

    printf "  [%2d/%2d] %-40s " "$((PASS + FAIL + SKIP + 1))" "$TOTAL" "$SAMPLE_NAME"

    START=$(date +%s)

    # Run the build — reuse existing build dir (no --pristine) for speed on
    # subsequent runs; the first run will configure fresh into <sample>/build.
    if "${SCRIPT_DIR}/build.sh" \
            --board "$BOARD" \
            "$SAMPLE_DIR" \
            >"$LOG_FILE" 2>&1; then
        END=$(date +%s)
        ELAPSED=$((END - START))
        printf "✅  PASS  (%ds)\n" "$ELAPSED"
        PASS=$((PASS + 1))
        RESULTS+=("PASS  ${SAMPLE_NAME}")
    else
        END=$(date +%s)
        ELAPSED=$((END - START))
        printf "❌  FAIL  (%ds)\n" "$ELAPSED"
        FAIL=$((FAIL + 1))
        RESULTS+=("FAIL  ${SAMPLE_NAME}")
        # Print the last 10 lines of the log so failures are visible immediately
        echo ""
        echo "         ── last lines of ${SAMPLE_NAME}.log ──"
        tail -10 "$LOG_FILE" | sed 's/^/         /'
        echo ""
    fi
done

END_TOTAL=$(date +%s)
TOTAL_ELAPSED=$((END_TOTAL - START_TOTAL))
TOTAL_MIN=$((TOTAL_ELAPSED / 60))
TOTAL_SEC=$((TOTAL_ELAPSED % 60))

# Write summary report
{
    echo "Batch build summary — $(date)"
    echo "Board  : ${BOARD}"
    echo "Total  : ${TOTAL}   Pass: ${PASS}   Fail: ${FAIL}   Time: ${TOTAL_MIN}m${TOTAL_SEC}s"
    echo ""
    for R in "${RESULTS[@]}"; do
        echo "  $R"
    done
} | tee "$SUMMARY"

echo ""
if [[ $FAIL -eq 0 ]]; then
    echo "All ${PASS}/${TOTAL} samples built successfully."
else
    echo "${FAIL} sample(s) failed. See logs in: ${LOG_DIR}"
    echo "Quick error view per failed sample:"
    echo "  grep -i 'error:' ${LOG_DIR}/<sample>.log"
fi
