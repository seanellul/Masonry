#!/usr/bin/env python3
"""
Extract a handful of sprites from the original tilesheets and save them as
4× upscaled PNGs suitable for feeding into nano-banana edit_image.

Run this first, then call nano-banana edit_image on each input_*.png with the
cozy style prompt. Afterwards, run post_restyle.py to mask-lock the results
back to the original alpha shape.
"""
from pathlib import Path
from PIL import Image

REPO = Path(__file__).resolve().parent.parent
OUT = REPO / "tools" / "sprite_pipeline" / "cozy_test"
OUT.mkdir(parents=True, exist_ok=True)

# (label, tilesheet, sx, sy, w, h)
SPRITES = [
    ("gnome_head",  "gnomes.png",    32,   0, 32, 36),
    ("workbench",   "workshops.png", 256, 36, 32, 36),
    ("rabbit",      "animals.png",   192, 216, 32, 36),
]

UPSCALE = 16   # 32x36 → 512x576 — big enough for the edit model to "see" it

def is_magenta(rgba):
    r, g, b, a = rgba
    return r > 240 and b > 240 and g < 16

for label, sheet, x, y, w, h in SPRITES:
    src = Image.open(REPO / "content" / "tilesheet" / sheet).convert("RGBA")
    cell = src.crop((x, y, x + w, y + h))

    # Save original at native + zoomed for reference
    cell.save(OUT / f"{label}_orig_native.png")
    cell_z = cell.resize((w * UPSCALE, h * UPSCALE), Image.NEAREST)
    cell_z.save(OUT / f"{label}_orig_zoomed.png")

    # Build the input we'll send to the AI: demagenta the background so the
    # edit model sees transparency, upscale with nearest-neighbour to keep
    # pixel shapes crisp.
    input_img = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    px_src = cell.load()
    px_dst = input_img.load()
    for yy in range(h):
        for xx in range(w):
            rgba = px_src[xx, yy]
            if is_magenta(rgba):
                continue
            px_dst[xx, yy] = rgba
    input_up = input_img.resize((w * UPSCALE, h * UPSCALE), Image.NEAREST)
    input_up.save(OUT / f"{label}_input.png")

    # Also save just the alpha mask at native size — we'll need it for the
    # post-process mask-lock step.
    mask = Image.new("L", (w, h), 0)
    pm = mask.load()
    for yy in range(h):
        for xx in range(w):
            if px_dst[xx, yy][3] > 16:
                pm[xx, yy] = 255
    mask.save(OUT / f"{label}_mask.png")

    print(f"Wrote {label}: {w}×{h} → {w*UPSCALE}×{h*UPSCALE} (input + mask + orig)")

print(f"\nAll files in: {OUT}")
