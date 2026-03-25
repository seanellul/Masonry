#!/usr/bin/env python3
"""
Visual regression comparison for Ingnomia screenshots.

Compares two screenshots and reports pixel-level differences.
Outputs JSON for easy consumption by agents.

Usage:
    python compare_screenshots.py reference.png current.png [--threshold 5] [--diff-out /tmp/diff.png]

Output (JSON to stdout):
    {"match": true, "diff_percent": 1.2, "diff_image": "/tmp/diff.png"}
"""

import argparse
import json
import sys

try:
    from PIL import Image, ImageChops, ImageDraw
except ImportError:
    print(json.dumps({
        "status": "error",
        "message": "Pillow not installed. Run: pip install Pillow"
    }))
    sys.exit(1)


def compare_images(reference_path: str, current_path: str, diff_out_path: str = None) -> dict:
    """Compare two images and return difference metrics."""
    try:
        ref = Image.open(reference_path).convert("RGB")
        cur = Image.open(current_path).convert("RGB")
    except FileNotFoundError as e:
        return {"status": "error", "message": str(e)}
    except Exception as e:
        return {"status": "error", "message": f"Failed to open images: {e}"}

    # Handle size mismatch
    if ref.size != cur.size:
        return {
            "status": "warning",
            "match": False,
            "diff_percent": 100.0,
            "message": f"Size mismatch: reference={ref.size}, current={cur.size}",
            "reference_size": list(ref.size),
            "current_size": list(cur.size),
        }

    # Compute pixel-level difference
    diff = ImageChops.difference(ref, cur)

    # Calculate diff percentage
    pixels = ref.size[0] * ref.size[1]
    diff_data = diff.getdata()

    # A pixel is "different" if any channel differs by more than a small tolerance
    tolerance = 2  # Allow tiny rounding differences
    diff_count = sum(1 for r, g, b in diff_data if r > tolerance or g > tolerance or b > tolerance)
    diff_percent = (diff_count / pixels) * 100.0

    # Total difference magnitude (for more nuanced comparison)
    total_diff = sum(r + g + b for r, g, b in diff_data)
    max_possible = pixels * 255 * 3
    magnitude_percent = (total_diff / max_possible) * 100.0 if max_possible > 0 else 0

    result = {
        "status": "ok",
        "diff_percent": round(diff_percent, 2),
        "magnitude_percent": round(magnitude_percent, 4),
        "diff_pixels": diff_count,
        "total_pixels": pixels,
        "image_size": list(ref.size),
    }

    # Generate diff visualization if requested
    if diff_out_path:
        # Amplify differences for visibility
        amplified = ImageChops.multiply(diff, Image.new("RGB", diff.size, (10, 10, 10)))
        # Clamp to valid range
        amplified = amplified.point(lambda x: min(255, x))

        # Create side-by-side comparison: reference | current | diff
        total_width = ref.size[0] * 3
        comparison = Image.new("RGB", (total_width, ref.size[1] + 30), (32, 32, 32))
        comparison.paste(ref, (0, 30))
        comparison.paste(cur, (ref.size[0], 30))
        comparison.paste(amplified, (ref.size[0] * 2, 30))

        # Add labels
        draw = ImageDraw.Draw(comparison)
        draw.text((10, 8), "Reference", fill=(200, 200, 200))
        draw.text((ref.size[0] + 10, 8), "Current", fill=(200, 200, 200))
        draw.text((ref.size[0] * 2 + 10, 8), f"Diff ({diff_percent:.1f}%)", fill=(255, 100, 100))

        comparison.save(diff_out_path)
        result["diff_image"] = diff_out_path

    return result


def main():
    parser = argparse.ArgumentParser(description="Compare screenshots for visual regression")
    parser.add_argument("reference", help="Path to reference screenshot")
    parser.add_argument("current", help="Path to current screenshot")
    parser.add_argument("--threshold", type=float, default=5.0,
                        help="Maximum allowed diff percentage (default: 5.0)")
    parser.add_argument("--diff-out", default=None,
                        help="Path to save diff visualization image")

    args = parser.parse_args()

    result = compare_images(args.reference, args.current, args.diff_out)

    if result.get("status") == "error":
        print(json.dumps(result, indent=2))
        sys.exit(1)

    # Determine pass/fail based on threshold
    diff_pct = result.get("diff_percent", 100)
    result["match"] = diff_pct <= args.threshold
    result["threshold"] = args.threshold

    print(json.dumps(result, indent=2))

    # Exit code: 0 = match (within threshold), 1 = regression
    sys.exit(0 if result["match"] else 1)


if __name__ == "__main__":
    main()
