#!/usr/bin/env python3
"""
Build a bake-off contact sheet comparing candidate grass tiles against the
original Grass2_0 (Spring) reference.

For each candidate:
  - Show the raw 1024x1024 thumbnail
  - Downsample to 32x36 (the actual in-game size) via two strategies:
      a) Direct LANCZOS resize to 32x36 (squashes the diamond)
      b) Smart-crop to content bounds, fit into 32x36 preserving aspect, pad
  - Show the downsampled result at 8x zoom for fair comparison

Usage:
    python3 tools/grass_bakeoff.py
"""
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont

REPO = Path(__file__).resolve().parent.parent
SHEET = REPO / "content" / "tilesheet" / "seasonalgrass.png"
GEN_DIR = REPO / "generated_imgs"
OUT = REPO / "tools" / "sprite_pipeline" / "inventory" / "seasonalgrass" / "bakeoff.png"

# The 6 candidates we just generated (today, Spring grass)
CANDIDATES = [
    ("nb-1", "generated-2026-04-07T12-17-05-589Z-5o1al1.png"),
    ("nb-2", "generated-2026-04-07T12-17-15-150Z-dwu5d3.png"),
    ("nb-3", "generated-2026-04-07T12-17-21-729Z-ew9rid.png"),
    ("nb-4", "generated-2026-04-07T12-18-11-549Z-hycco3.png"),
    ("nb-5", "generated-2026-04-07T12-18-17-549Z-8097l6.png"),
    ("nb-6", "generated-2026-04-07T12-18-23-178Z-p4vuhk.png"),
]

CELL_W, CELL_H = 32, 36
ZOOM = 8           # how big to render each 32x36 result
THUMB = 192        # size of raw 1024 thumbnail
PAD = 12
LABEL_H = 18
HEADER = 32


def trim_alpha(img: Image.Image) -> Image.Image:
    """Crop to non-transparent / non-near-white bounding box."""
    if img.mode != "RGBA":
        img = img.convert("RGBA")
    # Build mask of "content" pixels: alpha > 16 AND not pure white-ish
    px = img.load()
    w, h = img.size
    minx, miny, maxx, maxy = w, h, 0, 0
    found = False
    for y in range(h):
        for x in range(w):
            r, g, b, a = px[x, y]
            if a < 16:
                continue
            # treat near-white checkerboard as bg too
            if r > 230 and g > 230 and b > 230:
                continue
            if x < minx: minx = x
            if y < miny: miny = y
            if x > maxx: maxx = x
            if y > maxy: maxy = y
            found = True
    if not found:
        return img
    return img.crop((minx, miny, maxx + 1, maxy + 1))


def resize_direct(img: Image.Image) -> Image.Image:
    """Squash directly into 32x36."""
    return img.convert("RGBA").resize((CELL_W, CELL_H), Image.LANCZOS)


def resize_fit(img: Image.Image) -> Image.Image:
    """Trim, fit into 32x36 preserving aspect, center."""
    trimmed = trim_alpha(img)
    tw, th = trimmed.size
    scale = min(CELL_W / tw, CELL_H / th)
    nw = max(1, int(round(tw * scale)))
    nh = max(1, int(round(th * scale)))
    small = trimmed.resize((nw, nh), Image.LANCZOS)
    out = Image.new("RGBA", (CELL_W, CELL_H), (0, 0, 0, 0))
    out.paste(small, ((CELL_W - nw) // 2, (CELL_H - nh) // 2), small)
    return out


def main():
    src = Image.open(SHEET).convert("RGBA")
    ref = src.crop((0, 2 * CELL_H, CELL_W, 3 * CELL_H))  # Grass_2_0 — Spring

    cw, ch = CELL_W * ZOOM, CELL_H * ZOOM
    col_w = max(THUMB, cw) + PAD
    row_h = THUMB + LABEL_H + PAD + ch + LABEL_H + PAD + ch + LABEL_H + PAD

    # Layout: header row showing "Reference" + each candidate
    n_cols = 1 + len(CANDIDATES)
    img_w = PAD + n_cols * col_w + PAD
    img_h = HEADER + row_h + PAD

    out = Image.new("RGBA", (img_w, img_h), (22, 22, 26, 255))
    draw = ImageDraw.Draw(out)
    try:
        font = ImageFont.truetype("/System/Library/Fonts/Supplemental/Arial.ttf", 11)
        big = ImageFont.truetype("/System/Library/Fonts/Supplemental/Arial Bold.ttf", 14)
    except OSError:
        font = ImageFont.load_default()
        big = font

    def draw_column(col_idx, label, raw_img, ref_mode=False):
        x0 = PAD + col_idx * col_w
        draw.text((x0 + 4, 6), label, fill=(220, 220, 220, 255), font=big)
        y = HEADER

        # Raw thumbnail
        thumb = raw_img.convert("RGBA").resize((THUMB, THUMB), Image.LANCZOS)
        out.paste(thumb, (x0 + (col_w - THUMB) // 2, y), thumb)
        draw.text((x0 + 4, y + THUMB + 2), "raw 1024 (thumb)" if not ref_mode else "raw cell",
                  fill=(160, 200, 255, 255), font=font)
        y += THUMB + LABEL_H + PAD

        # Downsample (direct)
        if ref_mode:
            shown = raw_img
        else:
            shown = resize_direct(raw_img)
        zoomed = shown.resize((cw, ch), Image.NEAREST)
        out.paste(zoomed, (x0 + (col_w - cw) // 2, y), zoomed)
        draw.text((x0 + 4, y + ch + 2),
                  "32x36 native" if ref_mode else "→ 32x36 (squash)",
                  fill=(160, 200, 255, 255), font=font)
        y += ch + LABEL_H + PAD

        # Downsample (fit) — only for candidates
        if not ref_mode:
            fit = resize_fit(raw_img)
            zoomed2 = fit.resize((cw, ch), Image.NEAREST)
            out.paste(zoomed2, (x0 + (col_w - cw) // 2, y), zoomed2)
            draw.text((x0 + 4, y + ch + 2), "→ 32x36 (fit)",
                      fill=(160, 200, 255, 255), font=font)

    draw_column(0, "REFERENCE Grass_2_0", ref, ref_mode=True)
    for i, (name, fname) in enumerate(CANDIDATES):
        path = GEN_DIR / fname
        if not path.exists():
            print(f"Missing: {path}")
            continue
        img = Image.open(path)
        draw_column(i + 1, name, img)

    OUT.parent.mkdir(parents=True, exist_ok=True)
    out.save(OUT)
    print(f"Wrote {OUT}  ({out.size})")


if __name__ == "__main__":
    main()
