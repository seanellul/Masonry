#!/usr/bin/env python3
"""
Side-by-side comparison of original vs AI seasonalgrass.png anchor cells.
"""
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont

REPO = Path(__file__).resolve().parent.parent
ORIG = REPO / "content" / "tilesheet" / "seasonalgrass.png"
ALT = REPO / "content" / "tilesheet_ai" / "seasonalgrass.png"
OUT = REPO / "tools" / "sprite_pipeline" / "inventory" / "seasonalgrass" / "compare.png"

SEASONS = [("Spring", 0), ("Summer", 4), ("Autumn", 7), ("Winter", 10)]
ROWS = list(range(1, 7))
CELL_W, CELL_H = 32, 36
ZOOM = 6
PAD = 6
LABEL = 14
HEADER = 28
GROUP_GAP = 20

orig = Image.open(ORIG).convert("RGBA")
alt = Image.open(ALT).convert("RGBA")

# Drop magenta → transparent on both for visual clarity
def demagenta(img):
    out = img.copy()
    px = out.load()
    for y in range(out.size[1]):
        for x in range(out.size[0]):
            r, g, b, a = px[x, y]
            if r > 240 and b > 240 and g < 16:
                px[x, y] = (0, 0, 0, 0)
    return out

orig = demagenta(orig)
alt = demagenta(alt)

cw, ch = CELL_W * ZOOM, CELL_H * ZOOM
group_w = len(SEASONS) * (cw + PAD) + PAD
img_w = PAD + 2 * group_w + GROUP_GAP + PAD
img_h = HEADER + len(ROWS) * (ch + LABEL + PAD) + PAD

img = Image.new("RGBA", (img_w, img_h), (22, 22, 26, 255))
draw = ImageDraw.Draw(img)
try:
    font = ImageFont.truetype("/System/Library/Fonts/Supplemental/Arial.ttf", 10)
    big = ImageFont.truetype("/System/Library/Fonts/Supplemental/Arial Bold.ttf", 15)
except OSError:
    font = ImageFont.load_default()
    big = font

draw.text((PAD + 20, 6), "ORIGINAL", fill=(200, 220, 255, 255), font=big)
draw.text((PAD + group_w + GROUP_GAP + 20, 6), "AI TEXTURE PACK", fill=(255, 220, 150, 255), font=big)

def draw_group(src_sheet, x_off):
    for ri, row in enumerate(ROWS):
        y_top = HEADER + ri * (ch + LABEL + PAD)
        for ci, (_, col) in enumerate(SEASONS):
            cx = col * CELL_W
            cy = row * CELL_H
            cell = src_sheet.crop((cx, cy, cx + CELL_W, cy + CELL_H))
            cell_z = cell.resize((cw, ch), Image.NEAREST)
            px_x = x_off + PAD + ci * (cw + PAD)
            img.paste(cell_z, (px_x, y_top), cell_z)
            if ri == 0:
                draw.text((px_x + 4, HEADER - 16), SEASONS[ci][0],
                          fill=(200, 200, 200, 255), font=font)
            if ci == 0:
                draw.text((x_off + 2, y_top + ch // 2 - 6), f"G{row}",
                          fill=(200, 200, 200, 255), font=font)

draw_group(orig, 0)
draw_group(alt, group_w + GROUP_GAP)

img.save(OUT)
print(f"Wrote {OUT}  ({img.size})")
