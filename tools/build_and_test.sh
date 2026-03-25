#!/bin/bash
# Build and smoke test Ingnomia
# Usage: ./tools/build_and_test.sh [save_path]
#
# If no save_path is provided, uses content/test_saves/smoke_test/
# Outputs: screenshot to /tmp/ingnomia_test.png, metrics to /tmp/ingnomia_metrics.json

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_DIR/build"

SAVE_PATH="${1:-$PROJECT_DIR/content/test_saves/smoke_test/}"
SCREENSHOT_PATH="/tmp/ingnomia_test.png"
METRICS_PATH="/tmp/ingnomia_metrics.json"
REFERENCE_DIR="$PROJECT_DIR/content/test_reference"
DIFF_PATH="/tmp/ingnomia_diff.png"

echo "=== Building Ingnomia ==="
cmake --build "$BUILD_DIR" --parallel "$(sysctl -n hw.ncpu 2>/dev/null || nproc)" 2>&1
if [ $? -ne 0 ]; then
    echo '{"build":"failed"}' > "$METRICS_PATH"
    echo "BUILD FAILED"
    exit 1
fi
echo "Build succeeded."

# Check if save path exists
if [ ! -d "$SAVE_PATH" ]; then
    echo "Warning: Save path does not exist: $SAVE_PATH"
    echo "Skipping game test. Create a test save first."
    echo '{"build":"success","save_loaded":false,"error":"no test save found"}' > "$METRICS_PATH"
    exit 0
fi

echo "=== Running smoke test ==="
echo "Save: $SAVE_PATH"
echo "Screenshot: $SCREENSHOT_PATH"
echo "Metrics: $METRICS_PATH"

"$BUILD_DIR/Ingnomia" \
    --test-mode \
    --load-save "$SAVE_PATH" \
    --screenshot "$SCREENSHOT_PATH" \
    --screenshot-delay 60 \
    --run-ticks 100 \
    --metrics-out "$METRICS_PATH"

echo "=== Test complete ==="
if [ -f "$METRICS_PATH" ]; then
    echo "Metrics:"
    cat "$METRICS_PATH"
fi
if [ -f "$SCREENSHOT_PATH" ]; then
    echo "Screenshot saved to: $SCREENSHOT_PATH"
fi

# Visual regression check (if reference exists)
REFERENCE_FILE="$REFERENCE_DIR/game_hud.png"
if [ -f "$REFERENCE_FILE" ] && [ -f "$SCREENSHOT_PATH" ]; then
    echo ""
    echo "=== Visual regression check ==="
    python3 "$SCRIPT_DIR/compare_screenshots.py" \
        "$REFERENCE_FILE" "$SCREENSHOT_PATH" \
        --threshold 5 \
        --diff-out "$DIFF_PATH" || true
    if [ -f "$DIFF_PATH" ]; then
        echo "Diff image: $DIFF_PATH"
    fi
else
    echo ""
    echo "=== Skipping visual regression (no reference at $REFERENCE_FILE) ==="
    echo "To create reference: cp $SCREENSHOT_PATH $REFERENCE_FILE"
fi
