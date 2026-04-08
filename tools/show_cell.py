#!/usr/bin/env python3
"""
Render a single grass cell at 16x zoom with a pixel-grid overlay so we can
see the exact tile geometry the engine expects.
"""
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont

REPO = Path(__file__).resolve().parent.parent
SHEET = REPO / "content" / "tilesheet" / "seasonalgrass.png"
OUT = REPO / "tools" / "sprite_pipeline" / "inventory" / "seasonalgrass" / "geometry.png"

CELL_W, CELL_H = 32, 36
ZOOM = 16
PAD = 24
HEADER = 40

ROWS = [
    ("Grass_2_0 Spring", 2, 0),
    ("Grass_2_4 Summer", 2, 4),
    ("Grass_2_7 Autumn", 2, 7),
    ("Grass_2_10 Winter", 2, 10),
]

src = Image.open(SHEET).convert("RGBA")
# Replace magenta key with transparent for visual clarity
px = src.load()
for y in range(src.size[1]):
    for x in range(src.size[0]):
        r, g, b, a = px[x, y]
        if r > 240 and b > 240 and g < 16:
            px[x, y] = (0, 0, 0, 0)

cw, ch = CELL_W * ZOOM, CELL_H * ZOOM
img_w = PAD + len(ROWS) * (cw + PAD)
img_h = HEADER + ch + PAD + 60

out = Image.new("RGBA", (img_w, img_h), (24, 24, 28, 255))
draw = ImageDraw.Draw(out)
try:
    font = ImageFont.truetype("/System/Library/Fonts/Supplemental/Arial.ttf", 11)
    big = ImageFont.truetype("/System/Library/Fonts/Supplemental/Arial Bold.ttf", 14)
except OSError:
    font = ImageFont.load_default()
    big = font

for i, (label, row, col) in enumerate(ROWS):
    cell = src.crop((col * CELL_W, row * CELL_H, col * CELL_W + CELL_W, row * CELL_H + CELL_H))
    zoomed = cell.resize((cw, ch), Image.NEAREST)
    x0 = PAD + i * (cw + PAD)
    y0 = HEADER

    # Background panel
    draw.rectangle((x0, y0, x0 + cw, y0 + ch), fill=(40, 40, 48, 255))
    out.paste(zoomed, (x0, y0), zoomed)

    # Pixel grid
    for gx in range(0, CELL_W + 1):
        line_x = x0 + gx * ZOOM
        draw.line((line_x, y0, line_x, y0 + ch), fill=(255, 255, 255, 30))
    for gy in range(0, CELL_H + 1):
        line_y = y0 + gy * ZOOM
        draw.line((x0, line_y, x0 + cw, line_y), fill=(255, 255, 255, 30))

    # Highlight the empty top region (rows 0..15) and the content region (16..34)
    draw.rectangle((x0, y0, x0 + cw, y0 + 16 * ZOOM),
                   outline=(255, 80, 80, 200), width=2)
    draw.rectangle((x0, y0 + 16 * ZOOM, x0 + cw, y0 + 35 * ZOOM),
                   outline=(80, 220, 120, 200), width=2)

    draw.text((x0 + 4, 8), label, fill=(220, 220, 220, 255), font=big)
    draw.text((x0 + 4, y0 + ch + 6),
              "RED = empty (16 rows)\nGREEN = tile content (19 rows)",
              fill=(200, 200, 200, 255), font=font)

OUT.parent.mkdir(parents=True, exist_ok=True)
out.save(OUT)
print(f"Wrote {OUT}  ({out.size})")
