#!/usr/bin/env python3
"""
Measure the geometry of an isometric grass tile: how much of the cell is
the diamond top face vs. the block side face. Helps diagnose whether the
AI candidates are using the right projection.
"""
from pathlib import Path
from PIL import Image

REPO = Path(__file__).resolve().parent.parent

def measure(img: Image.Image, label: str, magenta_key: bool = False):
    if img.mode != "RGBA":
        img = img.convert("RGBA")
    w, h = img.size
    px = img.load()

    # For each row, count "content" pixels — anything that isn't transparent
    # or magenta-keyed or near-white background
    row_widths = []
    for y in range(h):
        n = 0
        for x in range(w):
            r, g, b, a = px[x, y]
            if a < 16:
                continue
            if magenta_key and r > 240 and b > 240 and g < 16:
                continue
            if r > 230 and g > 230 and b > 230 and not magenta_key:
                # white-ish background (1024 thumbnails)
                continue
            n += 1
        row_widths.append(n)

    # Find first/last non-empty rows
    first = next((i for i, n in enumerate(row_widths) if n > 0), None)
    last = next((i for i in range(h - 1, -1, -1) if row_widths[i] > 0), None)
    if first is None:
        print(f"{label}: empty")
        return

    # Find peak width row (widest = midline of diamond)
    peak_w = max(row_widths)
    peak_y = row_widths.index(peak_w)

    # Diamond top portion = first..peak (widening), block side = peak..last (constant or shrinking)
    top_height = peak_y - first + 1
    side_height = last - peak_y

    print(f"{label:25} size={w}x{h}  content y={first}..{last}  peak_w={peak_w}  "
          f"diamond_top={top_height}px  side_below_peak={side_height}px  "
          f"side_ratio={side_height / (top_height + side_height):.0%}")


def main():
    sheet = Image.open(REPO / "content" / "tilesheet" / "seasonalgrass.png").convert("RGBA")
    print("=== Originals (magenta-keyed) ===")
    for grass in [1, 2, 3, 4, 5, 6]:
        cell = sheet.crop((0, grass * 36, 32, grass * 36 + 36))
        measure(cell, f"Grass_{grass}_0 (Spring)", magenta_key=True)

    print("\n=== Generated candidates (1024x1024) ===")
    gen = REPO / "generated_imgs"
    cands = [
        ("nb-1", "generated-2026-04-07T12-17-05-589Z-5o1al1.png"),
        ("nb-2", "generated-2026-04-07T12-17-15-150Z-dwu5d3.png"),
        ("nb-3", "generated-2026-04-07T12-17-21-729Z-ew9rid.png"),
        ("nb-4", "generated-2026-04-07T12-18-11-549Z-hycco3.png"),
        ("nb-5", "generated-2026-04-07T12-18-17-549Z-8097l6.png"),
        ("nb-6", "generated-2026-04-07T12-18-23-178Z-p4vuhk.png"),
    ]
    for label, fname in cands:
        img = Image.open(gen / fname)
        measure(img, label)


if __name__ == "__main__":
    main()
